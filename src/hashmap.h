#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <stdlib.h>
#include <string.h>

typedef unsigned long hash_t;

typedef struct {
    hash_t hash;
    size_t size;
    void *data;
} map_header;

typedef struct {
    size_t capacity;
    map_header *items;
} Map;

extern void hm_free(Map *m);
extern void hm_expand(Map *m);
extern void hm_init(Map *m, size_t capacity);
extern void hm_insert(Map *m, map_header *itm);
extern int  hm_search(Map *m, const char *key, size_t len, size_t *index);

extern hash_t hash_string(const char *s, size_t len);

#define cast_ptr(T, a)  (T*)(a)
#define item_data(I, D) (I).data = malloc((I).size); memcpy((I).data, (D), (I).size);
#define hm_item(p, i)   (map_header*)((p) + (i))

#define hminit(m, c)    hm_init((Map*)(m), (c))
#define hmfree(m)       hm_free((Map*)(m))

#define hmins(m, k, d) ({ \
    map_header itm = { .hash = hash_string(k, strlen(k)), .size = sizeof(*(d)) }; \
    item_data(itm, (d)); hm_insert((Map*)(m), &itm); \
})

#define hmget_(m, k, l, d) ({ \
    size_t idx = 0; \
    if (hm_search((Map*)(m), (k), (l), &idx)) { \
        (d) = (m)->items[idx].data; \
    } else { \
        (d) = NULL; \
    } \
})

#define hmget(m, k, d) hmget_(m, k, strlen(k), d)

#endif // HASHMAP_H_
