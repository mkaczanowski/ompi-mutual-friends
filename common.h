#ifndef OMPI_FRIENDS_COMMON
#define OMPI_FRIENDS_COMMON

#define MAX(a, b) ((a) > (b) ? a : b)
#define MIN(a, b) ((a) < (b) ? a : b)
#define PRINT_OUTPUT 0
#define OUTPUT_TO_FILE 1

typedef struct {
    int f1, f2;
} friend_tuple_t;

#endif // OMPI_FRIENDS_COMMON
