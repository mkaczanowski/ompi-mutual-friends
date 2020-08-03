#ifndef OMPI_FRIENDS_FRIENDS
#define OMPI_FRIENDS_FRIENDS

#include <keyvalue.h>

using namespace MAPREDUCE_NS;

void parse_file(int itask, char* fname, KeyValue* kv, void* ptr);
void mpi_reduce(char* key, int keybytes, char* multivalue,
        int nvalues, int* valuebytes, KeyValue* kv,
        void* ptr);
void mpi_reduce_intersect(char* key, int keybytes, char* multivalue, int nvalues,
                         int* valuebytes, KeyValue* kv, void* ptr);
void mpi_map(uint64_t itask, char* key, int keybytes, char* value,
               int valuebytes, KeyValue* kv, void* ptr);
void mpi_output(uint64_t itask, char* key, int keybytes, char* value,
            int valuebytes, KeyValue* kv, void* ptr);

int find_intersection(int a1[], int s1, int a2[], int s2, int iarr[]);
int cmpfunc(const void* a, const void* b);

void swap(int* a, int* b);

#endif // OMPI_FRIENDS_FRIENDS
