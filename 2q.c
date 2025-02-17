#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "list.h"
#include "hashtable.h"


typedef struct {
    int *key_ptr;
    int  value;
    int  list_id;
} TwoQEntry;

typedef struct {
    int capacity;
    double hits;
    double accesses;

    int a1in_capacity;
    int am_capacity;
    int a1out_capacity;

    List *a1in;
    List *am;
    List *a1out;

    int a1in_count;
    int am_count;
    int a1out_count;

    HashTable *table;
} TwoQCache;


static unsigned int twoq_hash_func(const void *key) {
    int test_key = (unsigned int)(*(const int *)key);
    return (unsigned int)(*(const int *)key);
}

static int twoq_key_cmp(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return ia - ib;
}

static void twoq_destroy_key(void *key) {
    free(key);  
}


static void twoq_destroy_entry(void *data) {
    if (!data) return;
    free((TwoQEntry *)data);
}

 // move from one list to another A1in-> A1out --- A1in-> Am ---------A1->Am + hashtable update old pointer is freed
static ListNode* twoq_move_node(
    TwoQCache *cache,
    List      *old_list,
    List      *new_list,
    ListNode  *node,
    int        new_list_id)
{
    if (!node) return NULL;
    TwoQEntry *entry = (TwoQEntry *)node->data;
    int *key_ptr      = entry->key_ptr;
    int *new_key_ptr = (int *)malloc(sizeof(int));

    if(!new_key_ptr) exit(EXIT_FAILURE);

    *new_key_ptr = * key_ptr;
    entry->key_ptr = new_key_ptr;



    // Remove node from the old list  frees -> listnode but not entry whuch is at listnode->data;
    list_remove_node(old_list, node);
   // insert to new list head
    ListNode *new_node = list_insert_node_head_return(new_list, entry);
    entry->list_id = new_list_id;

    // we must update hashtable as key->list node* now address has changed 
    hashtable_remove(cache->table, key_ptr);
    hashtable_insert(cache->table, new_key_ptr, new_node);

    return new_node;
}

//evict from tail + hashtable remove + free entry
static void twoq_evict_tail(TwoQCache *cache, List *list, int *count) {
    void *data = list_remove_tail(list);   // freees only node ie LISTNOde need to free entry also 
    if (!data) return;

    TwoQEntry *entry = (TwoQEntry *)data;
    hashtable_remove(cache->table, entry->key_ptr);  // during hashtable remove key is freed
    free(entry);                                     //now  only free the entry cuz if not memory leak by entruy->key_ptr
    (*count)--;
}

// for new entries insert + eviction
static void twoq_insert_new(TwoQCache *cache, int key, int value) {
    TwoQEntry *entry = (TwoQEntry *)malloc(sizeof(TwoQEntry));
    if (!entry) exit(EXIT_FAILURE);

    entry->key_ptr = (int *)malloc(sizeof(int));
    if (!entry->key_ptr) exit(EXIT_FAILURE);

    *(entry->key_ptr) = key;
    entry->value      = value;
    entry->list_id    = 1; 

    // insert to head of A1in 
    list_insert_node_head_return(cache->a1in, entry);
    cache->a1in_count++;

    // for insertion into hashtable we need key --> Listnode * but we have now only TWOQentry but we know it is at the head of A1in
    ListNode *head_node = cache->a1in->head; 
    hashtable_insert(cache->table, entry->key_ptr, head_node);
    // if a1in is full -->a1out
    if (cache->a1in->count > cache->a1in_capacity )
    {
        if (cache->a1in->tail) {
            ListNode *tail_node = cache->a1in->tail;  /// tail in A1in
            TwoQEntry *tail_entry = (TwoQEntry *)tail_node->data;
            twoq_move_node(cache, cache->a1in, cache->a1out, tail_node, 3);
            cache->a1in_count--;
            cache->a1out_count++;

            // a1out full 
            if (cache->a1out_count > cache->a1out_capacity) {
                twoq_evict_tail(cache, cache->a1out, &cache->a1out_count);
            }
        }
    }
}


static void twoq_put(Cache *cache_obj, int key, int value) {
    TwoQCache *cache = (TwoQCache *)cache_obj->data;
 

    int temp_key = key;
    ListNode *node = (ListNode *)hashtable_lookup(cache->table, &temp_key);
    if (node) {
        // in A1in || AM || A1out
        TwoQEntry *entry = (TwoQEntry *)node->data;
        entry->value = value;
        //printf("GOT HIT KEY VALUE %d ---  IS IN %d --LIST AT ACCESS _---%f--\n",*(entry->key_ptr),entry->list_id, cache->accesses);
        

        if (entry->list_id == 1) {
            //A1in hit 
          //  printf("GOT HIT KEY VALUE %d ---  IS IN %d --LIST AT ACCESS _---%f--\n",*(entry->key_ptr),entry->list_id, cache->accesses);

           // cache->hits++;
            //move_node_to_head(cache->a1in, node);
        }
        else if (entry->list_id == 2) {
            // Am hit move node to head of Am
                   // printf("GOT HIT KEY VALUE %d ---  IS IN %d --LIST AT ACCESS _---%f--\n",*(entry->key_ptr),entry->list_id, cache->accesses);

            //cache->hits++;
            move_node_to_head(cache->am, node);
        }
        else {
            // a1out hit -->am head
            twoq_move_node(cache, cache->a1out, cache->am, node, 2);
            cache->a1out_count--;
            cache->am_count++;
                  //  printf("GOT HIT BUT IN FHOST LIST KEY VALUE %d ---  IS IN %d --LIST AT ACCESS _---%f--\n",*(entry->key_ptr),entry->list_id, cache->accesses);

            // if am full
            if (cache->am_count > cache->am_capacity) {
                twoq_evict_tail(cache, cache->am, &cache->am_count);
            }
        }
        return;
    }

    // miss - new -> a1in 
    twoq_insert_new(cache, key, value);
}



// get --> check in hashmap -> find listnode * -> data -> entry-> value -> set value -> return 1;

static int twoq_get(Cache *cache_obj, int key, int *value) {
    TwoQCache *cache = (TwoQCache *)cache_obj->data;
    int temp_key = key;
       cache->accesses++;

    ListNode *node = (ListNode *)hashtable_lookup(cache->table, &temp_key);
    if (!node) {
        return 0;  
    }
    TwoQEntry *entry = (TwoQEntry *)node->data;
    *value = entry->value;

    
    if (entry->list_id == 1) {
        move_node_to_head(cache->a1in, node);
        cache->hits++;
        return 1;
    } else if (entry->list_id == 2) {
        move_node_to_head(cache->am, node);
        cache->hits++;

        return 1;

    }
   
    return 0;
}

static double twoq_hit_ratio(Cache *cache_obj) {
    TwoQCache *cache = (TwoQCache *)cache_obj->data;
    //printf("TOTAL ACCESSES: %f\n", cache->accesses);
   // printf("TOTAL HITS:     %f\n", cache->hits);
    if (cache->accesses == 0.0) return 0.0;
    return cache->hits / cache->accesses;
}


static void twoq_destroy(Cache *cache_obj) {
    if (!cache_obj) return;
    TwoQCache *cache = (TwoQCache *)cache_obj->data;

    hashtable_destroy(cache->table); // desrtoy  array , hash pointer , key ptr in entry

    
    list_destroy(cache->a1in,  twoq_destroy_entry); // frees node -ie List node and also entry 
    list_destroy(cache->am,    twoq_destroy_entry);
    list_destroy(cache->a1out, twoq_destroy_entry);

    free(cache);
    free(cache_obj);
}


static CacheOps twoq_ops = {
    .put       = twoq_put,
    .get       = twoq_get,
    .destroy   = twoq_destroy,
    .hit_ratio = twoq_hit_ratio
};


Cache* create_cache_2q(int capacity) {
    Cache *cache_obj = (Cache *)malloc(sizeof(Cache));
    if (!cache_obj) exit(EXIT_FAILURE);

    TwoQCache *cache = (TwoQCache *)malloc(sizeof(TwoQCache));
    if (!cache) exit(EXIT_FAILURE);

    cache->capacity = capacity;
    cache->hits = 0.0;
    cache->accesses = 0.0;
    cache->a1in_capacity  = capacity / 4;       
    cache->am_capacity    = capacity - cache->a1in_capacity;  
    cache->a1out_capacity = cache->a1in_capacity*2;        


    cache->a1in  = list_create();
    cache->am    = list_create();
    cache->a1out = list_create();

    cache->a1in_count  = 0;
    cache->am_count    = 0;
    cache->a1out_count = 0;

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



