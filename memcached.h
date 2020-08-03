#ifndef OMPI_FRIENDS_MEMCACHED
#define OMPI_FRIENDS_MEMCACHED

#include "common.h"

int init_memcached(const char *host);
void set_key(char*, const char*);
void set_key(friend_tuple_t*, int*, int);

#endif // OMPI_FRIENDS_MEMCACHED
