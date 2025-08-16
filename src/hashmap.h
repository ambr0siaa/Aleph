#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <stdlib.h>
#include <string.h>
#include "arena.h"

typedef unsigned long hash_t;

typedef struct {
    hash_t hash;
    size_t size;
    void   *data;
} map_header;

typedef struct {
    Arena      allocs;
    size_t     capacity;
    map_header *items;
} Map;

extern void map_expand_(Map *m);
extern void map_init_(Map *m, size_t capacity);
extern void map_insert_(Map *m, map_header *itm);
extern int  map_search_(Map *m, const char *key, size_t len, size_t *index);

extern hash_t hash_string(const char *s, size_t len);

#define cast_ptr(T, a)      (T*)(a)
#define item_data(A, I, D)  (I).data = arena_alloc(A, (I).size); memcpy((I).data, (D), (I).size);
#define map_item(p, i)      (map_header*)((p) + (i))

#define map_init(m, c) map_init_((Map*)(m), (c))

#define mapins(m, k, l, d) ({ \
    map_header itm = { .hash = hash_string(k, (l)), .size = sizeof(*(d)) }; \
    item_data(&(m)->allocs, itm, (d)); map_insert_((Map*)(m), &itm); \
})

#define map_ins(m, k, d) mapins(m, k, strlen(k), d)

#define map_strget(m, k, l, d) ({ \
    size_t idx = 0; \
    if (map_search_((Map*)(m), (k), (l), &idx)) { \
        (d) = (m)->items[idx].data; \
    } else { \
        (d) = NULL; \
    } \
})

#define map_free(m) ({ arena_free(&(m)->allocs); (m)->items = NULL; })
#define map_get(m, k, d) mapget_(m, k, strlen(k), d)

#endif // HASHMAP_H_
