
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "list.h"
#include "hashtable.h"
#include "lru.h"





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
    //free(key);
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

    int buffer_index;
    //if not full then place in data buffer will be the coubt index
    if (cache_obj->count < cache_obj->capacity) {
        buffer_index = cache_obj->count;
        cache_obj->count++;
    } else {
        // full need to evict 
        ListNode *tail = cache->lru_list->tail;
        if (tail) {
            LRUEntry *old_entry = (LRUEntry *)tail->data;
            buffer_index = old_entry->buff_index;
            int old_key = cache_obj->data_buffer[buffer_index].key;
            hashtable_remove(cache->table, &old_key);
         LRUEntry* removed =   list_remove_tail(cache->lru_list);
         free(removed);
        } else {
            buffer_index = 0;
        }
    }
    LRUEntry *new_entry = (LRUEntry*)malloc(sizeof(LRUEntry));
    // new_entry->key_ptr = (int*)malloc(sizeof(int));
    // *(new_entry->key_ptr) = key;
    // new_entry->value = value;
    new_entry->buff_index = buffer_index;
  
    ListNode *new_node = list_insert_node_head_return(cache->lru_list, new_entry);

    cache_obj->data_buffer[new_entry->buff_index].key = key;
    cache_obj->data_buffer[new_entry->buff_index].value = value;
   // int* key_ptr =  (int*)malloc(sizeof(int));
    //*key_ptr = key;
   // hashtable_insert(cache->table, new_entry->key_ptr, new_node);
       hashtable_insert(cache->table, &cache_obj->data_buffer[new_entry->buff_index].key, new_node);


  
}


static int lru_get(Cache *cache_obj, int key, int *value)
{
    LRUCache *cache = (LRUCache*)cache_obj->data;
     //cache->accesses += 1; 
    int temp_key = key;
    ListNode *node = (ListNode *)hashtable_lookup(cache->table, &temp_key);

    if (!node) {
     
        return 0;
    }
    LRUEntry *entry = (LRUEntry *)node->data;
    int buff_index   = entry->buff_index;
    
    move_node_to_head(cache->lru_list, node);
    *value = cache_obj->data_buffer[buff_index].value;
    //cache->hits++;
   // printf("HIT BUFFER INDEX %d\n",buff_index);
   // printf("HIT VALUE %d\n",cache_obj->data_buffer[buff_index].key);
    return 1;
}



static void lru_destroy(Cache *cache_obj)
{
    if (!cache_obj) return;
    LRUCache *cache = (LRUCache*)cache_obj->data;

    list_destroy(cache->lru_list, destroy_lru_entry);

    hashtable_destroy(cache->table);

    free(cache);
    free(cache_obj->data_buffer);
    free(cache_obj);
}

static CacheOps lru_ops = {
    .put       = lru_put,
    .get       = lru_get,
    .destroy   = lru_destroy,
};


Cache* create_cache_lru(int capacity)
{
    Cache *cache_obj = (Cache*)malloc(sizeof(Cache));
    cache_obj->accessess = 0;
    cache_obj->hits = 0;
    cache_obj->capacity = capacity;
    cache_obj->count = 0;

    cache_obj->data_buffer = (CacheEntry*)malloc(sizeof(CacheEntry) * capacity);

    LRUCache *cache = (LRUCache*)malloc(sizeof(LRUCache));

    // cache->capacity = capacity;
    // cache->count = 0;

    cache->lru_list = list_create();

   
    int table_size = capacity * 2 + 1;
    cache->table = hashtable_create(table_size,
                                    int_hash_func,
                                    int_key_cmp,
                                    NULL,
                                    NULL);

  
    cache_obj->ops  = &lru_ops;
    cache_obj->data = cache;
    return cache_obj;
}
