#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "list.h"
#include "hashtable.h"
#include "2q.h"



// traversing the whole buffer to find the free index
//  static int get_free_slot(Cache* cache_obj){
//     // o(n) complexity 
//     int capacity = cache_obj->capacity;
//     for(int i = 0; i < capacity; i++){
//         if(cache_obj->data_buffer[i].free){
//             return i;
//         }
//     }
//     return -1;
//  }/


static int* get_entry_key(Cache *cache_obj, TwoQEntry *entry) {
    if (entry->list_type == TWOQ_A1OUT) {
        return &entry->ghost_key;
    } else {
        return &cache_obj->data_buffer[entry->buff_index].key;
    }
}

static int* get_entry_value(Cache *cache_obj, TwoQEntry *entry) {
    return &cache_obj->data_buffer[entry->buff_index].value;
}


static unsigned int twoq_hash_func(const void *key) {
    int k = *(const int*)key;
    return (unsigned int)k;
}

static int twoq_key_cmp(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return ia - ib;
}

static void twoq_destroy_key(void *key) {

}

static void twoq_destroy_entry(void *data) {
    if (!data) return;
    free((TwoQEntry *)data);
}


static ListNode* twoq_move_node(Cache *cache_obj, TwoQCache *two_q,
                                List *old_list, List *new_list,
                                ListNode *node, TwoQListType new_list_type) {
    if (!node) return NULL;
    TwoQEntry *entry = (TwoQEntry *)node->data;
    int *old_key_ptr = (entry->list_type == TWOQ_A1OUT)
                           ? &entry->ghost_key
                           : &cache_obj->data_buffer[entry->buff_index].key;

    // reorganising the buffer index and ghost key and freelist array
    //active to ghost -- A1in ----> A1ouyt
    if (entry->list_type != TWOQ_A1OUT && new_list_type == TWOQ_A1OUT) {
        /// activee to ghost list so need to free the index
        //int active_index = entry->buff_index;
        //cache_obj->data_buffer[active_index].free = 1;
        entry->ghost_key = cache_obj->data_buffer[entry->buff_index].key;
        // adding that index to freelist array 
        two_q->free_indices[two_q->free_count] = entry->buff_index;
        two_q->free_count++;
        entry->buff_index = -1;
        cache_obj->count--;
    } else if (entry->list_type == TWOQ_A1OUT && new_list_type != TWOQ_A1OUT) {
        // ghost to active list 
        // A1out to Am

        int buffer_index;
       // buffer_index = get_free_slot(cache_obj);
     

        if (two_q->free_count > 0) {
            buffer_index = two_q->free_indices[two_q->free_count - 1];
            two_q->free_count--;
        } 
        else if (two_q->next_buffer_index < cache_obj->capacity) {
            buffer_index = two_q->next_buffer_index;
            two_q->next_buffer_index++;
        } 
        entry->buff_index = buffer_index;
        cache_obj->data_buffer[buffer_index].key = entry->ghost_key;
        cache_obj->data_buffer[buffer_index].value = 0;
        cache_obj->count++;
    }
    // removing old node from old list
    list_remove_node(old_list, node);
    //adding ne w node to head of new list
    ListNode *new_node = list_insert_node_head_return(new_list, entry);
    entry->list_type = new_list_type;
   
    
    int *new_key_ptr = (entry->list_type == TWOQ_A1OUT)
                        ? &entry->ghost_key
                        : &cache_obj->data_buffer[entry->buff_index].key;
    //removing old key from hashtable
    hashtable_remove(two_q->table, old_key_ptr);
    hashtable_insert(two_q->table, new_key_ptr, new_node);
    return new_node;
}

// removal if fulll
static void twoq_evict_tail(Cache *cache_obj, TwoQCache *two_q,
                            List *list, int *count) {
    void *data = list_remove_tail(list);
    if (!data) return;
    TwoQEntry *entry = (TwoQEntry *)data;
    if (entry->list_type != TWOQ_A1OUT) {
        // activr list entry so need to indicate that its index is free now for use

        //int active_index = entry->buff_index;
        //two_q_obj->data_buffer[active_index].free = 1;
        two_q->free_indices[two_q->free_count] = entry->buff_index;
        two_q->free_count++;
        cache_obj->count--;
    }
    int *key_ptr = (entry->list_type == TWOQ_A1OUT)
                        ? &entry->ghost_key
                        : &cache_obj->data_buffer[entry->buff_index].key;
    hashtable_remove(two_q->table, key_ptr);
    free(entry);
    (*count)--;
}


static void twoq_insert_new(Cache *cache_obj, TwoQCache *two_q, int key, int value) {
    // first check if size of a1in is full
    if (two_q->a1in_count >= two_q->a1in_capacity) {
        if (two_q->a1in->tail) {
            ListNode *tail_node = two_q->a1in->tail;
            //size of a1out is full or not
            if (two_q->a1out_count >= two_q->a1out_capacity) {
                twoq_evict_tail(cache_obj, two_q, two_q->a1out, &two_q->a1out_count);
            }
            twoq_move_node(cache_obj, two_q, two_q->a1in, two_q->a1out, tail_node, TWOQ_A1OUT);
            two_q->a1in_count--;
            two_q->a1out_count++;
            
        }
    }
    //space is there in A1in so no need of eviction 
    TwoQEntry *entry = malloc(sizeof(TwoQEntry));
    entry->list_type = TWOQ_A1IN;
    int buffer_index;
    //int free_index = get_free_slot(cache_obj);
    // entry->buff_index = free_index;
    // cache_obj->data_buffer[free_index].free = 0;
    //cache_obj->data_buffer[free_index].key = key;
    //cache_obj->data_buffer[free_index].value = value;

     if (two_q->free_count > 0) {
        buffer_index = two_q->free_indices[two_q->free_count - 1];
        two_q->free_count--;
     } 
    else if (two_q->next_buffer_index < cache_obj->capacity) {
        buffer_index = two_q->next_buffer_index;
        two_q->next_buffer_index++;
    } 
    entry->buff_index = buffer_index;
    cache_obj->data_buffer[buffer_index].key = key;
    cache_obj->data_buffer[buffer_index].value = value;
    ListNode *node = list_insert_node_head_return(two_q->a1in, entry);
    two_q->a1in_count++;
    cache_obj->count++;
    int *key_ptr = &cache_obj->data_buffer[buffer_index].key;
    hashtable_insert(two_q->table, key_ptr, node);
}


static void twoq_put(Cache *cache_obj, int key, int value) {
    TwoQCache *cache = (TwoQCache *)cache_obj->data;
    ListNode *node = hashtable_lookup(cache->table, &key);
    if (node) {
        TwoQEntry *entry = (TwoQEntry *)node->data;
        if(entry->list_type == TWOQ_A1OUT){
            // A1ou t hit move it to Am

            // ghost hits in A1out
            cache->ghost_hits++;
            if (cache->am_count >= cache->am_capacity) {
                twoq_evict_tail(cache_obj, cache, cache->am, &cache->am_count);
            }
            twoq_move_node(cache_obj, cache, cache->a1out, cache->am, node, TWOQ_AM);
            cache->a1out_count--;
            cache->am_count++;
            *get_entry_value(cache_obj, entry) = value;
    
        return;
    }}
    // complete miss not in any of 3 lists
    twoq_insert_new(cache_obj, cache, key, value);
}
//get if in A1in or in Am get return 1 else return 0;
static int twoq_get(Cache *cache_obj, int key, int *value) {
    TwoQCache *cache = (TwoQCache *)cache_obj->data;
    ListNode *node = hashtable_lookup(cache->table, &key);
    if (!node)
        return 0;
    TwoQEntry *entry = (TwoQEntry *)node->data;
    if (entry->list_type == TWOQ_A1IN) {
       // move_node_to_head(cache->a1in, node);
        *value = *get_entry_value(cache_obj, entry);
        return 1;
    } else if (entry->list_type == TWOQ_AM) {
        move_node_to_head(cache->am, node);
        *value = *get_entry_value(cache_obj, entry);
        return 1;
    }
    return 0;
}


static void twoq_destroy(Cache *cache_obj) {
    if (!cache_obj) return;
    TwoQCache *cache = (TwoQCache *)cache_obj->data;
    hashtable_destroy(cache->table);
    list_destroy(cache->a1in, twoq_destroy_entry);
    list_destroy(cache->am, twoq_destroy_entry);
    list_destroy(cache->a1out, twoq_destroy_entry);
    free(cache->free_indices);
    free(cache);
    free(cache_obj->data_buffer);
    free(cache_obj);
}

static CacheOps twoq_ops = {
    .put     = twoq_put,
    .get     = twoq_get,
    .destroy = twoq_destroy,
};


Cache* create_cache_2q(int capacity) {
    Cache *cache_obj = malloc(sizeof(Cache));
    cache_obj->accessess = 0;
    cache_obj->hits = 0;
    cache_obj->capacity = capacity;
    cache_obj->data_buffer = calloc(capacity, sizeof(CacheEntry));
    TwoQCache *cache = malloc(sizeof(TwoQCache));
    cache->capacity = capacity;
    cache->a1in_capacity  = capacity / 4;
    cache->am_capacity    = capacity - cache->a1in_capacity;
    cache->a1out_capacity = cache->a1in_capacity * 2;
    cache->a1in  = list_create();
    cache->am    = list_create();
    cache->a1out = list_create();
    cache->a1in_count  = 0;
    cache->am_count    = 0;
    cache->a1out_count = 0;
    cache->ghost_hits = 0;
    cache->next_buffer_index = 0;
    cache->free_indices = malloc(sizeof(int) * capacity);
    cache->free_count = 0;
    int table_size = capacity * 2 + 1;
    cache->table = hashtable_create(table_size,
                                    twoq_hash_func,
                                    twoq_key_cmp,
                                    twoq_destroy_key,
                                    NULL);
    cache_obj->ops  = &twoq_ops;
    cache_obj->data = cache;
    return cache_obj;
}
