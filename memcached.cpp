#include <libmemcached/memcached.h>
#include <iostream>
#include <sstream>
#include "common.h"

memcached_st* memc;

int init_memcached(const char *host) {
    memcached_server_st* servers = NULL;
    memcached_return rc;

    memcached_server_st* memcached_servers_parse(char* server_strings);
    memc = memcached_create(NULL);

    servers = memcached_server_list_append(servers, host, 11211, &rc);
    if (rc != MEMCACHED_SUCCESS) {
        std::cerr << "Couldn't append server: " << memcached_strerror(memc, rc) << std::endl;
        return 255;
    }

    rc = memcached_server_push(memc, servers);
    if (rc != MEMCACHED_SUCCESS) {
        std::cerr << "Couldn't add server: " << memcached_strerror(memc, rc) << std::endl;
        return 255;
    }

    return 0;
}

void set_key(char* key, const char* value) {
    memcached_return rc;
    rc = memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0,
                       (uint32_t)0);

    if (rc != MEMCACHED_SUCCESS)
        std::cerr << "Couldn't store key: " << memcached_strerror(memc, rc) << std::endl;

    if (PRINT_OUTPUT)
        std::cout << "M_SET KEY: " << key << ", VALUE: " << value << std::endl;
}

void set_key(friend_tuple_t* ft, int* fl, int len) {
    char key[100];
    int cx = snprintf(key, 100, "m_%d_%d", ft->f1, ft->f2);

    if (cx >= 0 && cx < 100) {
        std::stringstream s;
        for (int i = 0; i < len; i++) {
            s << std::to_string(fl[i]);
            if (i + 1 < len)
                s << ",";
        }

        if (len == 0)
            s << "11111";
        set_key(key, s.str().c_str());
    } else {
        std::cerr << "couldn't add key: m_" << ft->f1 << "_" << ft->f2 << std::endl;
    }
}
