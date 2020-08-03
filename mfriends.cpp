#include <keyvalue.h>
#include <mpi.h>
#include <iostream>
#include <sys/stat.h>
#include "mfriends.h"
#include "memcached.h"
#include "common.h"

using namespace MAPREDUCE_NS;

void parse_file(int itask, char* fname, KeyValue* kv, void* ptr) {
    struct stat stbuf;
    int flag = stat(fname, &stbuf);
    if (flag < 0) {
        std::cerr << "ERROR: Could not query file size" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    int filesize = stbuf.st_size;

    FILE* fp = fopen(fname, "r");
    char* text = new char[filesize + 1];
    int nchar = fread(text, 1, filesize, fp);
    text[nchar] = '\0';
    fclose(fp);

    const char* whitespace = " \t\n\f\r\0";
    char* id = strtok(text, whitespace);
    char* value = strtok(NULL, whitespace);
    int id1, id2;

    while (id) {
        id1 = atoi(id);
        id2 = atoi(value);
        kv->add((char*)&id1, sizeof(int), (char*)&id2, sizeof(int));
        id = strtok(NULL, whitespace);
        value = strtok(NULL, whitespace);
    }

    delete[] text;
}

void mpi_reduce(char* key, int keybytes, char* multivalue,
        int nvalues, int* valuebytes, KeyValue* kv,
        void* ptr) {

    size_t t_block_len = (sizeof(int) * nvalues) + 1;
    int* t_block = new int[t_block_len];
    int* t_len = &t_block[0];
    int* t_friends = &t_block[1];
    *t_len = nvalues;

    int* values = (int*)multivalue;
    for (int i = 0; i < *t_len; i++) {
        t_friends[i] = values[i];
    }

    kv->add(key, keybytes, (char*)t_block, t_block_len * sizeof(int));
    delete[] t_block;
}


void mpi_reduce_intersect(char* key, int keybytes, char* multivalue, int nvalues,
                         int* valuebytes, KeyValue* kv, void* ptr) {
    friend_tuple_t* ft = (friend_tuple_t*)key;

    int* t_block;
    int* t_len;
    int* t_len2;
    int* t_friends;
    int* t_friends2;

    // if <= 1 then there is no user where u1 has u2 && u2 has u1
    if (nvalues > 1) { 
        t_block = (int*)(multivalue);
        t_len = &t_block[0];
        t_friends = &t_block[1];

        t_block = (int*)(multivalue + valuebytes[0]);
        t_len2 = &t_block[0];
        t_friends2 = &t_block[1];

        int* res_block =
            new int[MIN(*t_len, *t_len2) + 1]; // arr[length, intersection]
        int* intersection_array = &res_block[1];
        int* intersection_array_len = &res_block[0];

        *intersection_array_len = find_intersection(
            t_friends, *t_len, t_friends2, *t_len2, intersection_array);

        if (OUTPUT_TO_FILE) {
            kv->add(key, keybytes, (char*)res_block,
                    (*intersection_array_len + 1) * (sizeof(int)));
        } else {
            set_key(ft, intersection_array, *intersection_array_len);
        }

        delete[] res_block;
    }
}

// Pair (A,B) -> (B,C,D)
void mpi_map(uint64_t itask, char* key, int keybytes, char* value,
               int valuebytes, KeyValue* kv, void* ptr) {

    int* t_block = (int*)value;
    int* t_len = &t_block[0];
    int* t_friends = &t_block[1];

    qsort(t_friends, *t_len, sizeof(int), cmpfunc); // executed on diff procs
    friend_tuple_t ft;
    memset(&ft, 0, sizeof(friend_tuple_t));

    for (int i = 0; i < *t_len; i++) {
        ft.f1 = *((int*)key);
        ft.f2 = t_friends[i];
        swap(&ft.f1, &ft.f2);

        kv->add((char*)&ft, sizeof(friend_tuple_t), (char*)t_block, valuebytes);
    }
}

void mpi_output(uint64_t itask, char* key, int keybytes, char* value,
            int valuebytes, KeyValue* kv, void* ptr) {
    FILE* fp = (FILE*)ptr;
    friend_tuple_t* ft = (friend_tuple_t*)key;

    int* t_block = (int*)(value);
    int* t_len = &t_block[0];
    int* t_friends = &t_block[1]; // mutual friends

    fprintf(fp, "[%d, %d]: ", ft->f1, ft->f2);
    for (int j = 0; j < *t_len; j++) {
        fprintf(fp, "%d ", t_friends[j]);
    }
    fprintf(fp, "\n");

    if (PRINT_OUTPUT) {
        printf("[%d, %d]: ", ft->f1, ft->f2);
        for (int j = 0; j < *t_len; j++) {
            printf("%d ", t_friends[j]);
        }
        printf("\n");
    }
}

int find_intersection(int a1[], int s1, int a2[], int s2, int iarr[]) {
    int i = 0, j = 0, k = 0;

    while ((i < s1) && (j < s2)) {
        if (a1[i] < a2[j]) {
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

    return k;
}

int cmpfunc(const void* a, const void* b) { return (*(int*)a - *(int*)b); }

void swap(int* a, int* b) {
    if (*a > *b) {
        int tmp = *a;
        *a = *b;
        *b = tmp;
    }
}
