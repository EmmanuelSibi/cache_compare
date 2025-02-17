#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "list.h"
#include "hashtable.h"


typedef struct {
    int  *key_ptr; 
    int   value;
    int   ref;      
} ClockEntry;

typedef struct {
    int capacity;     
    int count;        
    double hits;
    double accesses;

    List      *slots; 
    ListNode  *hand;  

    HashTable *table; 
} ClockCache;

static void clock_destroy_entry(void *data) {
    if (!data) return;
    ClockEntry *entry = (ClockEntry *)data;
   
    free(entry);
}
static  void clock_destroy_key(void *key){
    if(!key) return;

    free(key);
}

static unsigned int clock_hash_func(const void *key) {
    int val = *(const int *)key;
    return (unsigned int)val;
}
static int clock_key_cmp(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return ia - ib;
}

static void clock_hashtable_insert(ClockCache *cache, int *key_ptr, ListNode *node) {
    hashtable_insert(cache->table, key_ptr, node);
}

static ListNode* clock_hashtable_lookup(ClockCache *cache, int key) {
    int tmp = key;
    return (ListNode *)hashtable_lookup(cache->table, &tmp);
}

static void clock_hashtable_remove(ClockCache *cache, int key) {
    int tmp = key;
    hashtable_remove(cache->table, &tmp);
}

static ListNode* clock_advance_hand(ClockCache *cache) {
    if (!cache->hand) return NULL;
    if (cache->hand->next) { // reached previous of tail 
        return cache->hand->next;
    } else { // reached tail
        return cache->slots->head;
    }
}


static ListNode* clock_evict_and_reuse(ClockCache *cache, int new_key, int new_value) {
    if (!cache->hand) return NULL;  
    while (1) {
        ClockEntry *entry = (ClockEntry *)cache->hand->data;
        if (entry->ref == 0) {
            int old_key = *(entry->key_ptr);
            clock_hashtable_remove(cache, old_key);

           // free(entry->key_ptr);  // no need to free here if free key fnc is given to hashtable
            entry->key_ptr = (int *)malloc(sizeof(int));
            if (!entry->key_ptr) {
                fprintf(stderr, "CLOCK: Memory allocation failed for key_ptr\n");
                exit(EXIT_FAILURE);
            }
            *(entry->key_ptr) = new_key;
            entry->value = new_value;
            entry->ref = 1;

            clock_hashtable_insert(cache, entry->key_ptr, cache->hand);

            cache->hand = clock_advance_hand(cache);
            return cache->hand;
        } else {
            entry->ref = 0;
            cache->hand = clock_advance_hand(cache);
        }
    }
}

static void clock_put(Cache *cache_obj, int key, int value) {
    ClockCache *cache = (ClockCache *)cache_obj->data;
  

    ListNode *node = clock_hashtable_lookup(cache, key);
    if (node) {
        ClockEntry *entry = (ClockEntry *)node->data;
        entry->value = value;
        entry->ref   = 1;
        return;
    }

    if (cache->count < cache->capacity) {
        
        ClockEntry *new_entry = (ClockEntry *)malloc(sizeof(ClockEntry));
        if (!new_entry) {
            fprintf(stderr, "CLOCK: Memory allocation failed for ClockEntry\n");
            exit(EXIT_FAILURE);
        }
        new_entry->key_ptr = (int *)malloc(sizeof(int));
        if (!new_entry->key_ptr) {
            fprintf(stderr, "CLOCK: Memory allocation failed for key_ptr\n");
            exit(EXIT_FAILURE);
        }
        *(new_entry->key_ptr) = key;
        new_entry->value = value;
        new_entry->ref   = 1;

        /* Insert at the tail, for example. (Or head, your choice.)
           We'll pick tail to keep the "clock" notion simple. */
        ListNode *new_node = list_insert_node_head_return(cache->slots, new_entry);
        cache->count++;

        clock_hashtable_insert(cache, new_entry->key_ptr, new_node);
        // first toime hand is null so set it to this node
        if (!cache->hand) {
            cache->hand = new_node;
        }
        return;
    }

    // no space evict and reuse

    clock_evict_and_reuse(cache, key, value);
}

static int clock_get(Cache *cache_obj, int key, int *value) {
    ClockCache *cache = (ClockCache *)cache_obj->data;
    
      cache->accesses++;
    ListNode *node = clock_hashtable_lookup(cache, key);
    if (!node) { //miss
        return 0; 
    }
    // hit  
    ClockEntry *entry = (ClockEntry *)node->data;
    if (value) {
        *value = entry->value;
    }
    entry->ref = 1;
    cache->hits++;
    return 1;
}

static double clock_hit_ratio(Cache *cache_obj) {
    ClockCache *cache = (ClockCache *)cache_obj->data;
   // printf("CLOCK: Total accesses = %.0f\n", cache->accesses);
    //printf("CLOCK: Total hits     = %.0f\n",   cache->hits);

    if (cache->accesses == 0.0) return 0.0;
    return cache->hits / cache->accesses;
}

static void clock_destroy(Cache *cache_obj) {
    if (!cache_obj) return;
    ClockCache *cache = (ClockCache *)cache_obj->data;

    hashtable_destroy(cache->table);  // hashnode + keyptr

    list_destroy(cache->slots, clock_destroy_entry); // listnode + clocknode ie data of listnode

    free(cache);
    free(cache_obj);
}

static CacheOps clock_ops = {
    .put       = clock_put,
    .get       = clock_get,
    .destroy   = clock_destroy,
    .hit_ratio = clock_hit_ratio
};

Cache* create_cache_clock(int capacity) {
    Cache *cache_obj = (Cache *)malloc(sizeof(Cache));
    if (!cache_obj) {
        fprintf(stderr, "CLOCK: Memory allocation failed for Cache object\n");
        exit(EXIT_FAILURE);
    }

    ClockCache *cache = (ClockCache *)malloc(sizeof(ClockCache));
    if (!cache) {
        fprintf(stderr, "CLOCK: Memory allocation failed for ClockCache\n");
        exit(EXIT_FAILURE);
    }
    cache->capacity = capacity;
    cache->count    = 0;
    cache->hits     = 0.0;
    cache->accesses = 0.0;
    cache->hand     = NULL;

    cache->slots = list_create();

    
    int table_size = capacity * 2 + 1;
    cache->table = hashtable_create(
        table_size,
        clock_hash_func,
        clock_key_cmp,
         clock_destroy_key,
     NULL
    );

    cache_obj->ops  = &clock_ops;
    cache_obj->data = cache;
    return cache_obj;
}
