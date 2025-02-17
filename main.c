#include <stdio.h>
#include <stdlib.h>
#include "cache.h"

extern Cache* create_cache_lru(int capacity);
extern Cache* create_cache_clock(int capacity);
extern Cache* create_cache_2q(int capacity);
extern Cache* create_cache_arc(int capacity);

Cache* create_cache_by_type(cache_algo_t algo, int capacity) {
    switch(algo) {
        case CACHE_LRU:
            return create_cache_lru(capacity);
        case CACHE_CLOCK:
            return create_cache_clock(capacity);
        case CACHE_2QADV:
            return create_cache_2q(capacity);
        case CACHE_ARC:
            return create_cache_arc(capacity);
        default:
            fprintf(stderr, "Unknown cache type\n");
            return NULL;
    }
}

int main(void) {
    int choice, capacity;
    printf("Select Cache Algorithm:\n");
    printf("1. LRU\n");
    printf("2. CLOCK\n");
    printf("3. 2Q\n");
    printf("4. ARC\n");
    printf("Enter choice: ");
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Invalid input\n");
        return 1;
    }
    printf("Enter cache capacity: ");
    if (scanf("%d", &capacity) != 1 || capacity <= 0) {
        fprintf(stderr, "Invalid capacity\n");
        return 1;
    }
    Cache *cache = create_cache_by_type((cache_algo_t)choice, capacity);
    if (!cache) {
        fprintf(stderr, "Failed to create cache\n");
        return 1;
    }
    int keys[] = {1, 2, 1,2,3, 4, 5, 6,1,2, 7,1,2 ,8, 9};
    int nkeys = sizeof(keys) / sizeof(keys[0]);
    int value;
    for (int i = 0; i < nkeys; i++) {
        printf("NOW PROCESSING %d\n",keys[i]);
        if (!cache->ops->get(cache, keys[i], &value)) {
            cache->ops->put(cache, keys[i], keys[i] * 2);
        }
    }
    
    //   if (cache->ops->get(cache, 8, &value))
    //         printf("Get key %d: %d\n", 8, value);
    //       if (cache->ops->get(cache, 8, &value))
    //         printf("Get key %d: %d\n", 8, value);
    // // for (int i = 0; i < nkeys; i++) {
    //     if (cache->ops->get(cache, keys[i], &value))
    //         printf("Get key %d: %d\n", keys[i], value);
    //     else
    //         printf("Key %d not found\n", keys[i]);
    // }
    printf("Cache Hit Ratio: %.2f%%\n", cache->ops->hit_ratio(cache) * 100);
    cache->ops->destroy(cache);
    return 0;
}
