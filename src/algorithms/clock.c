#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cache.h"
#include "list.h"
#include "hashtable.h"
#include "clock.h"



static void clock_destroy_key(void *key) {
    //free(key);  // no need now as points to the buffer index->key in data_buffer
}

static void clock_destroy_entry(void *data) {
    if (!data)
        return;
    ClockEntry *entry = (ClockEntry *)data;
    free(entry);
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


static ListNode* clock_hashtable_lookup(ClockCache *cache, int key) {
    return (ListNode *)hashtable_lookup(cache->table, &key);
}


static ListNode* clock_advance_hand(ClockCache *cache) {
    if (!cache->hand)
        return NULL;
    if (cache->hand->next) //prev
        return cache->hand->next;
    else
        return cache->slots->head;
}


static void clock_evict_and_reuse(ClockCache *cache, Cache *cache_obj, int new_key, int new_value) {
    while (1) {
        ClockEntry *entry = (ClockEntry *)cache->hand->data;
        int idx = entry->buff_index; // buffer_index instead of key_ptr
        if (!entry->ref) {
            int old_key = cache_obj->data_buffer[idx].key;  // index instead of key_ptr
            hashtable_remove(cache->table, &old_key);
            cache_obj->data_buffer[idx].key = new_key; //updating buffer
            cache_obj->data_buffer[idx].value = new_value;
            entry->ref = 1;
            hashtable_insert(cache->table, &cache_obj->data_buffer[idx].key, cache->hand);
            cache->hand = clock_advance_hand(cache); //hand pointing to next 
            break;
        } else {
            entry->ref = 0; //give second chance
            cache->hand = clock_advance_hand(cache);
        }
    }
}

static void clock_put(Cache *cache_obj, int key, int value) {
    ClockCache *cache = (ClockCache *)cache_obj->data;

    //space is there 
    if (cache_obj->count < cache_obj->capacity) {
        int buffer_index = cache_obj->count;
        cache_obj->count++;
        ClockEntry *new_entry = (ClockEntry *)malloc(sizeof(ClockEntry));
        new_entry->buff_index = buffer_index;
        new_entry->ref = 1;
        cache_obj->data_buffer[buffer_index].key = key;  // set the key and valye
        cache_obj->data_buffer[buffer_index].value = value;
        ListNode *new_node = list_insert_node_head_return(cache->slots, new_entry);
    
        hashtable_insert(cache->table, &(cache_obj->data_buffer[buffer_index].key), new_node);
        if (!cache->hand)
            cache->hand = new_node;
        return;
    }
    //no space evict and reuse
    clock_evict_and_reuse(cache, cache_obj, key, value);
}

static int clock_get(Cache *cache_obj, int key, int *value) {
    ClockCache *cache = (ClockCache *)cache_obj->data;
    ListNode *node = clock_hashtable_lookup(cache, key);
    if (!node)
        return 0;

    // hit
    ClockEntry *entry = (ClockEntry *)node->data;
    int idx = entry->buff_index;
    if (value)
        *value = cache_obj->data_buffer[idx].value;
    entry->ref = 1;
    return 1;
}

static void clock_destroy(Cache *cache_obj) {
    if (!cache_obj)
        return;
    ClockCache *cache = (ClockCache *)cache_obj->data;
    hashtable_destroy(cache->table);
    list_destroy(cache->slots, clock_destroy_entry);
    free(cache);
    free(cache_obj->data_buffer);
    free(cache_obj);
}


static CacheOps clock_ops = {
    .put     = clock_put,
    .get     = clock_get,
    .destroy = clock_destroy
};

Cache* create_cache_clock(int capacity) {
    Cache *cache_obj = (Cache *)malloc(sizeof(Cache));
    cache_obj->accessess = 0;
    cache_obj->hits = 0;
    cache_obj->capacity = capacity;
    cache_obj->count = 0;
    //intializsing cache buffer
    cache_obj->data_buffer = (CacheEntry *)malloc(sizeof(CacheEntry) * capacity);
    
    ClockCache *cache = (ClockCache *)malloc(sizeof(ClockCache));
    cache_obj->capacity= capacity;
    cache_obj->count  = 0;
    cache->hand = NULL;
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
