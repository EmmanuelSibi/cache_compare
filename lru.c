
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "list.h"
#include "hashtable.h"


typedef struct {
    int *key_ptr;  
    int  value;
} LRUEntry;


typedef struct {
    int capacity;
    int count;

    double hits;
    double accesses;

    List *lru_list;         // stores LRUEntry* as node->data 
    HashTable *table;       // maps (int*) key =----> ListNode* 
} LRUCache;


static unsigned int int_hash_func(const void *key)
{
    return (unsigned int)(*(const int*)key);
}


 
static int int_key_cmp(const void *a, const void *b)
{
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return (ia - ib);
}

static void destroy_int_key(void *key)
{
    free(key);
}


static void destroy_lru_entry(void *data)
{
    if (!data) return;
    LRUEntry *entry = (LRUEntry *)data;
   
    free(entry);
}



static void lru_put(Cache *cache_obj, int key, int value)
{
    LRUCache *cache = (LRUCache*)cache_obj->data;
   


    int temp_key = key; 
    ListNode *node = (ListNode *)hashtable_lookup(cache->table, &temp_key);

    if (node) {
        
        LRUEntry *entry = (LRUEntry *)node->data;
        entry->value = value;
        move_node_to_head(cache->lru_list, node);
       // cache->hits += 1; 
        return;
    }

    
    LRUEntry *new_entry = (LRUEntry*)malloc(sizeof(LRUEntry));
    if (!new_entry) {
        fprintf(stderr, "LRU: Memory allocation failed for LRUEntry\n");
        exit(EXIT_FAILURE);
    }
    new_entry->key_ptr = (int*)malloc(sizeof(int));
    if (!new_entry->key_ptr) {
        fprintf(stderr, "LRU: Memory allocation failed for key_ptr\n");
        exit(EXIT_FAILURE);
    }
    *(new_entry->key_ptr) = key;
    new_entry->value = value;

  
    ListNode *new_node = list_insert_node_head_return(cache->lru_list, new_entry);
    cache->count++;

    hashtable_insert(cache->table, new_entry->key_ptr, new_node);

    if (cache->count > cache->capacity) {
        ListNode *tail_node = cache->lru_list->tail;
        if (tail_node) {
            LRUEntry *tail_entry = (LRUEntry *)tail_node->data;
            hashtable_remove(cache->table, tail_entry->key_ptr);

          
            list_remove_node(cache->lru_list, tail_node);
            // free lru entry
            free(tail_entry);  

            cache->count--;
        }
    }
}


static int lru_get(Cache *cache_obj, int key, int *value)
{
    LRUCache *cache = (LRUCache*)cache_obj->data;
     cache->accesses += 1; 
    int temp_key = key;
    ListNode *node = (ListNode *)hashtable_lookup(cache->table, &temp_key);

    if (!node) {
     
        return 0;
    }

    LRUEntry *entry = (LRUEntry *)node->data;
    move_node_to_head(cache->lru_list, node);
    *value = entry->value;
    cache->hits++;
  
    return 1;
}


static double lru_hit_ratio(Cache *cache_obj)
{
    LRUCache *cache = (LRUCache*)cache_obj->data;
   // printf("TOTAL ACCESSES--%f\n", cache->accesses);
     //   printf("TOTAL HITSS--%f\n", cache->hits);

    if (cache->accesses == 0.0) 
        return 0.0;
    return cache->hits / cache->accesses;
}


static void lru_destroy(Cache *cache_obj)
{
    if (!cache_obj) return;
    LRUCache *cache = (LRUCache*)cache_obj->data;

    list_destroy(cache->lru_list, destroy_lru_entry);

    hashtable_destroy(cache->table);

    free(cache);
    free(cache_obj);
}

static CacheOps lru_ops = {
    .put       = lru_put,
    .get       = lru_get,
    .destroy   = lru_destroy,
    .hit_ratio = lru_hit_ratio
};


Cache* create_cache_lru(int capacity)
{
    Cache *cache_obj = (Cache*)malloc(sizeof(Cache));
    if (!cache_obj) {
        fprintf(stderr, "LRU: Memory allocation failed for Cache object\n");
        exit(EXIT_FAILURE);
    }

    LRUCache *cache = (LRUCache*)malloc(sizeof(LRUCache));
    if (!cache) {
        fprintf(stderr, "LRU: Memory allocation failed for LRUCache\n");
        exit(EXIT_FAILURE);
    }

    cache->capacity = capacity;
    cache->count = 0;
    cache->hits = 0.0;
    cache->accesses = 0.0;

    cache->lru_list = list_create();

   
    int table_size = capacity * 2 + 1;
    cache->table = hashtable_create(table_size,
                                    int_hash_func,
                                    int_key_cmp,
                                    destroy_int_key,
                                    NULL);

  
    cache_obj->ops  = &lru_ops;
    cache_obj->data = cache;

    return cache_obj;
}
