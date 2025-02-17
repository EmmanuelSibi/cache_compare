#ifndef CACHE_H
#define CACHE_H
typedef struct Cache Cache;

typedef struct CacheOps {
    void (*put)(Cache *cache, int key, int value);
    int (*get)(Cache *cache, int key, int *value);
    void (*destroy)(Cache *cache);
    double (*hit_ratio)(Cache *cache);
} CacheOps;

struct Cache {
    CacheOps *ops;   
    void *data;    
};

typedef enum {
    CACHE_LRU = 1,
    CACHE_CLOCK,
    CACHE_2QADV,
    CACHE_ARC
} cache_algo_t;

#endif 
