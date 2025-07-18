#include "hashmap.h"

#define index_step(i) (i * i)

#define hmins_itm(m, i) ({ \
    map_header item_ = { .hash = (i)->hash, .size = (i)->size }; \
    item_data(item_, (i)->data); hm_insert((m), &item_); \
})

void hm_init(Map *m, size_t capacity) {
    size_t size = sizeof(*m->items)*capacity;
    m->items = (map_header*)malloc(size);
    memset(m->items, 0, size);
    m->capacity = capacity;
}

void hm_free(Map *m) {
    for (size_t i = 0; i < m->capacity; ++i)
        if (m->items[i].hash != 0) free((m)->items[i].data);
    free(m->items); m->items = NULL;
    m->capacity = 0;
}

void hm_insert(Map *m, map_header *itm) {
    size_t idx = itm->hash % m->capacity;
    map_header *cur = hm_item(m->items, idx);
    if (cur->hash == 0) {
        *cur = *itm;
    } else {
        size_t step = index_step(idx);
        map_header *slot = hm_item(cur, 1);
        for (;;) {
            if (slot >= m->items + m->capacity) {
                hm_expand(m);
                idx = itm->hash % m->capacity;
                cur = hm_item(m->items, idx);
                slot = hm_item(cur, 1);
            }
            if (slot->hash == 0) {
                *slot = *itm;
                break;
            } else {
                slot = hm_item(slot, 1);
            }
            slot = hm_item(slot, step);
        }
    }
}

void hm_expand(Map *m) {
    Map nt = {0};
    hm_init(&nt, m->capacity*2);
    for (size_t j = 0; j < m->capacity; ++j) {
        map_header *itm = hm_item(m->items, j);
        if (itm->hash != 0) hmins_itm(&nt, itm);
    }
    hm_free(m);
    *m = nt;
}

int hm_search(Map *m, const char *key, size_t len, size_t *index) {
    hash_t hash = hash_string(key, len);
    size_t idx = hash % m->capacity;
    map_header *cur = hm_item(m->items, idx);
    if (cur->hash == hash) {
        *index = idx;
        return 1;
    } else {
        size_t step = index_step(idx);
        map_header *slot = hm_item(cur, 1);
        while (slot < m->items + m->capacity) {
            if (slot->hash == hash) {
                *index = idx + (size_t)(slot - cur);
                return 1;
            } else {
                slot = hm_item(slot, 1);
            }
            slot = hm_item(slot, step);
        }
        return 0;
    }
}

static const unsigned long hfk1[128] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
    59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
    191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251,
    257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317,
    331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397,
    401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
    467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557,
    563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619,
    631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701,
    709, 719
};

static const unsigned long hfk2[128] = {
    0x65992, 0xf1b28, 0x4540e, 0xb80ee, 0x1149, 0xdd8b6, 0x432db, 0xf1620,
    0x82161, 0xb6a19, 0xdf536, 0xbade6, 0xf10cb, 0x87869, 0xf244f, 0xe030a,
    0xbc408, 0x3f035, 0x7e689, 0x4b71e, 0xb3bc0, 0xdabe2, 0x3cec4, 0x7c8e1,
    0xe1ddb, 0xd9c64, 0xbef96, 0xefefb, 0x8cdea, 0xf2fc2, 0x38ba7, 0x8ea10,
    0x51955, 0xaf48c, 0x369fc, 0xef19a, 0xf35de, 0xe43f4, 0xd6c48, 0xeea76,
    0xf3878, 0x55a4d, 0xc43d1, 0x32674, 0x93d6f, 0x73249, 0xd4adc, 0xf3cbf,
    0xa928a, 0xe677a, 0xd39c0, 0xed341, 0x97418, 0xe72bd, 0xc8069, 0x98f23,
    0x5daee, 0xa6041, 0xd16bb, 0x29ea7, 0xec24a, 0xa46cc, 0x9c4a7, 0xf41a3,
    0x61a8c, 0xe92ca, 0x678d7, 0xa1346, 0x2383e, 0xea6a1, 0x65977, 0x9f934,
    0xce05e, 0xa1373, 0x6790e, 0xeb036, 0xcf2ca, 0x61a55, 0x6984d, 0xca76c,
    0x5fa95, 0xf40de, 0xc93fa, 0xd16da, 0xa606d, 0xf3fcc, 0xecafa, 0x18cb3,
    0xa79ac, 0x5bab9, 0x6f53b, 0x16a4a, 0xf3cbc, 0x958ac, 0xd4afa, 0xe5bd8,
    0xf3abf, 0xd5bd2, 0x12562, 0xc2efb, 0xe43df, 0x905d9, 0xc1a0b, 0xe3774,
    0xaf4b6, 0x78d4c, 0xd8cbc, 0xb0caa, 0x7ab44, 0x8cdb8, 0xbef71, 0xe1dc4,
    0x4d7bb, 0xf2c39, 0xdabfd, 0xf2867, 0x7e6bd, 0xf0b32, 0xdbb35, 0xb5332,
    0x495f7, 0xbadbf, 0x33a0,  0x474f1, 0xf1629, 0xde701, 0x83ec1, 0xde733
};

static inline unsigned long ror(unsigned long n, unsigned long s) { return (n << s | n >> (64 - s)); }
static inline unsigned long rol(unsigned long n, unsigned long s) { return (n >> s | n << (64 - s)); }

hash_t hash_string(const char *s, size_t len)
{
    hash_t h = 0x9d200; // sine of 271 
    for (size_t i = 0; i < len; ++i) {
        unsigned long a = (unsigned long)s[i]*hfk1[(unsigned long)s[i]];
        hash_t b = a*(hfk2[(a+1)%128]^hfk2[(a-1)%128]);
        h = rol(h*b, hfk1[(a+1)%128]%64);
        h ^= hfk2[a%128];
        h = ror(h^hfk2[b%128], hfk1[(a-1)%128]%64);
    }
    h = ror(h^hfk2[(h-37)%128], hfk1[(h+97)%128]%64);
    h = rol(h*hfk2[h%128], hfk1[(h+67)%128]%64);
    return h;
}
