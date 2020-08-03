#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/stat.h"
#include "mapreduce.h"
#include "keyvalue.h"
#include <sstream>
#include <libmemcached/memcached.h>

#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)
#define PRINT_OUTPUT 0
#define OUTPUT_TO_FILE 0

using namespace MAPREDUCE_NS;
typedef struct {
    int f1, f2;
} FriendTuple;

void fileread(int, char *, KeyValue *, void *);
void transform_user_friends(char *, int, char *, int, int *, KeyValue *, void *);
void transform_intersect(char *, int, char *, int, int *, KeyValue *, void *);
void first_map(uint64_t, char *, int, char *, int, KeyValue *, void *);
void swap(int*, int*);
int cmpfunc (const void*, const void*);
int find_intersection(int[], int, int[], int, int[]);
void output(uint64_t, char *, int, char *, int, KeyValue *, void *);
void set_key(char*, const char*);
void set_key(FriendTuple*, int*, int);

memcached_st *memc;
void initialize_memcached(){
    memcached_server_st *servers = NULL;
    memcached_return rc;

    memcached_server_st *memcached_servers_parse (char *server_strings);
    memc = memcached_create(NULL);

    servers= memcached_server_list_append(servers, "192.168.4.106", 11211, &rc);
    rc= memcached_server_push(memc, servers);

    if (rc != MEMCACHED_SUCCESS)
        fprintf(stderr,"Couldn't add server: %s\n",memcached_strerror(memc, rc));
}

int main(int narg, char **args) {
    MPI_Init(&narg,&args);

    int me,nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD,&me);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

    if (narg <= 1) {
        if (me == 0) printf("Syntax: mfriends file1 file2 ...\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    initialize_memcached();

    MapReduce *mr = new MapReduce(MPI_COMM_WORLD);
    mr->verbosity = 1;
    mr->timer = 1;
    //mr->memsize = 1;
    //mr->outofcore = 1;

    MPI_Barrier(MPI_COMM_WORLD);
    double tstart = MPI_Wtime();

    if(me == 0) printf("1. Map to form A -> B C D\n");
    mr->map(narg-1,&args[1],0,1,0,fileread,NULL);
    int nfiles = mr->mapfilecount;
    mr->collate(NULL);
    mr->reduce(transform_user_friends,NULL);

    MPI_Barrier(MPI_COMM_WORLD);
    double tstop = MPI_Wtime();

    if(me == 0) printf("2. Map to form (A,B) -> B C D\n");
    mr->map(mr,first_map, NULL);

    if(me == 0) printf("3. Reduce to form (A,B) -> (B C D)(B D E)\n");
    mr->collate(NULL);
    mr->reduce(transform_intersect,NULL);

    if(OUTPUT_TO_FILE){
        char fname[] = "output.txt";
        FILE* fp = fopen(fname,"w");
        if (fp == NULL) {
            printf("ERROR: Could not open output file");
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        mr->gather(1);
        mr->map(mr, output, (void*)fp);
        fclose(fp);
    }

    delete mr;
    if (me == 0) {
        printf("Time to process %d files on %d procs = %g (secs)\n",
                nfiles,nprocs,tstop-tstart);
    }

    MPI_Finalize();
}

void fileread(int itask, char *fname, KeyValue *kv, void *ptr){
    struct stat stbuf;
    int flag = stat(fname,&stbuf);
    if (flag < 0) {
        printf("ERROR: Could not query file size\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }
    int filesize = stbuf.st_size;

    FILE *fp = fopen(fname,"r");
    char *text = new char[filesize+1];
    int nchar = fread(text,1,filesize,fp);
    text[nchar] = '\0';
    fclose(fp);

    const char *whitespace = " \t\n\f\r\0";
    char *id = strtok(text,whitespace);
    char *value = strtok(NULL,whitespace);
    int id1;
    int id2;
    while (id) {
        id1 = atoi(id);
        id2 = atoi(value);
        kv->add((char*)&id1,sizeof(int),(char*)&id2,sizeof(int));
        id = strtok(NULL,whitespace);
        value = strtok(NULL,whitespace);
    }

    delete [] text;
}

void transform_user_friends(char *key, int keybytes, char *multivalue,
        int nvalues, int *valuebytes, KeyValue *kv, void *ptr) {

    size_t t_block_len = (sizeof(int)*nvalues)+1;
    int* t_block = new int[t_block_len];
    int* t_len = &t_block[0];
    int* t_friends = &t_block[1];
    *t_len = nvalues;

    int* values = (int*) multivalue;
    for(int i = 0; i < *t_len; i++){
        t_friends[i] = values[i];
    }

    kv->add(key,keybytes, (char*) t_block, t_block_len*sizeof(int));
    delete [] t_block;
}

void set_key(char* key, const char* value){
    memcached_return rc;
    rc = memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0, (uint32_t)0);

    if (rc != MEMCACHED_SUCCESS)
        fprintf(stderr,"Couldn't store key: %s\n",memcached_strerror(memc, rc)); 

    if(PRINT_OUTPUT) printf("M_SET KEY: %s, VALUE: %s\n", key, value);
}

void set_key(FriendTuple* ft, int* fl, int len){
    char key[100];
    int cx = snprintf(key, 100, "m_%d_%d", ft->f1, ft->f2);

    if (cx>=0 && cx < 100){
        std::stringstream s;
        for(int i = 0; i < len; i++){
            s << std::to_string(fl[i]);
            if(i+1 < len) s << ",";
        }

        if(len == 0) s << "11111";
        const char* value = s.str().c_str();
        set_key(key, value);
    } else {
        fprintf(stderr,"Couldn't add key: m_%d_%d\n", ft->f1, ft->f2);
    }
}

void transform_intersect(char *key, int keybytes, char *multivalue,
        int nvalues, int *valuebytes, KeyValue *kv, void *ptr) {
    FriendTuple* ft = (FriendTuple*) key;

    int* t_block;
    int* t_len;
    int* t_len2;
    int* t_friends;
    int* t_friends2;

    if(nvalues > 1){ // If <= 1 then there is no user where u1 has u2 && u2 has u1
        t_block = (int*) (multivalue);
        t_len = &t_block[0];
        t_friends = &t_block[1];

        t_block = (int*) (multivalue+valuebytes[0]);
        t_len2 = &t_block[0];
        t_friends2 = &t_block[1];

        int* res_block = new int[MIN(*t_len, *t_len2)+1]; // arr[Length, intersection]
        int* intersection_array = &res_block[1];
        int* intersection_array_len = &res_block[0];

        *intersection_array_len = find_intersection(t_friends, *t_len, t_friends2,
                *t_len2, intersection_array);

        if(OUTPUT_TO_FILE){
            kv->add(key, keybytes, (char*) res_block, (*intersection_array_len+1)*(sizeof(int)));
        }else {
            set_key(ft, intersection_array, *intersection_array_len);
        }

        delete [] res_block;
    }
}

void output(uint64_t itask, char *key, int keybytes, char *value,
        int valuebytes, KeyValue *kv, void *ptr) {
    FILE* fp = (FILE*) ptr;
    FriendTuple* ft = (FriendTuple*) key;

    int* t_block = (int*) (value);
    int* t_len = &t_block[0];
    int* t_friends = &t_block[1]; // mutual friends

    fprintf(fp, "[%d, %d]: ",ft->f1, ft->f2);
    for(int j = 0; j < *t_len; j++){
        fprintf(fp, "%d ", t_friends[j]);
    }
    fprintf(fp, "\n");

    if(PRINT_OUTPUT){
        printf("[%d, %d]: ",ft->f1, ft->f2);
        for(int j = 0; j < *t_len; j++){
            printf("%d ", t_friends[j]);
        }
        printf("\n");
    }
}

int find_intersection(int a1[], int s1, int a2[], int s2, int iarr[]){
    int i = 0, j = 0, k = 0;
    while ((i < s1) && (j < s2)){
        if (a1[i] < a2[j]){
            i++;
        } else if (a1[i] > a2[j]) {
            j++;
        } else {
            iarr[k] = a1[i];
            i++;
            j++;
            k++;
        }
    }

    return(k);
}

int cmpfunc (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

void swap(int* a, int* b){
    if(*a > *b){
        int tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

//Pair (A,B) -> (B,C,D)
void first_map(uint64_t itask, char *key, int keybytes, char *value,
        int valuebytes, KeyValue *kv, void *ptr) {

    int* t_block = (int*) value;
    int* t_len = &t_block[0];
    int* t_friends = &t_block[1];

    qsort(t_friends, *t_len, sizeof(int), cmpfunc); // Executed on diff procs
    FriendTuple ft;
    memset(&ft,0,sizeof(FriendTuple));

    for(int i = 0; i < *t_len; i++){
        ft.f1 = *((int*) key);
        ft.f2 = t_friends[i];
        swap(&ft.f1, &ft.f2);

        kv->add((char*)&ft, sizeof(FriendTuple),(char*) t_block, valuebytes);
    }
}
