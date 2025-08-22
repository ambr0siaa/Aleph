#ifndef ARENA_H_
#define ARENA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_DEFAULT_CAP (4 * 1024)
#define ARENA_CMP(size)   ((size) < ARENA_DEFAULT_CAP ? ARENA_DEFAULT_CAP : (size) * 2)

typedef struct Region Region;

struct Region {
    size_t alloc_pos;
    size_t capacity;
    Region *next;
    char   *data;
};

extern Region *region_create(size_t capacity);

typedef struct {
    Region *head;
    Region *tail;
} Arena;

extern void arena_dump(Arena *arena);
extern void arena_free(Arena *arena);
extern void arena_reset(Arena *arena);

extern void *arena_alloc(Arena *arena, size_t size);
extern void *arena_realloc(Arena *arena, void *old_ptr, size_t old_size, size_t new_size);

#define ARENA_DA_INIT_CAP 32

#define arena_da_append(a, da, item) do { \
        if ((da)->count + 1 >= (da)->capacity) { \
            size_t old_size = (da)->capacity*sizeof(*(da)->items); \
            (da)->capacity = (da)->capacity == 0 ? ARENA_DA_INIT_CAP : (da)->capacity*2; \
            (da)->items = arena_realloc((a), (da)->items, old_size, (da)->capacity*sizeof(*(da)->items)); \
        } \
        (da)->items[(da)->count++] = (item); \
    } while(0)

#define arena_da_append_many(a, da, item, item_count) do { \
        if ((da)->count + (item_count) >= (da)->capacity) { \
            size_t old_size = (da)->capacity*sizeof(*(da)->items); \
            (da)->capacity = (da)->capacity == 0 ? ARENA_DA_INIT_CAP : (da)->capacity; \
            while ((da)->count + (item_count) >= (da)->capacity) { (da)->capacity *= 2; } \
            (da)->items = arena_realloc((a), (da)->items, old_size, (da)->capacity*sizeof(*(da)->items)); \
        } \
        memcpy((da)->items + (da)->count, (item), (item_count)*sizeof(*(da)->items)); \
        (da)->count += (item_count); \
    } while(0)

#define arena_da_append_parts(a, items, count, cap, item) do { \
        if ((count) + 1 >= (cap)) { \
            size_t itm_sz = sizeof(*(items)); \
            size_t old_size = (cap)*itm_sz; \
            (cap) = (cap) == 0 ? ARENA_DA_INIT_CAP : (cap)*2; \
            (items) = arena_realloc((a), (items), old_size, (cap)*itm_sz); \
        } \
        (items)[(count)++] = (item); \
    } while(0)

#endif /* ARENA_H_ */
