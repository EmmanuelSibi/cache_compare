#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "list.h"
#include "hashtable.h"


typedef struct {
    int  *key_ptr;
    int   value;
    int   list_id;  
} ARCEntry;

typedef struct {
    int capacity;
    double hits;
    double accesses;

    int p;

    List *T1;
    List *T2;
    List *B1;
    List *B2;

    
    int t1_count;
    int t2_count;
    int b1_count;
    int b2_count;

    
    HashTable *table;
} ARCCache;

static unsigned int arc_hash_func(const void *key) {
    return (unsigned int)(*(const int *)key);
}

static int arc_key_cmp(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return (ia - ib);
}

 static void arc_destroy_key(void *key) {
    free(key);
 }

static void arc_destroy_entry(void *data) {
    if (!data) return;
    ARCEntry *entry = (ARCEntry *)data;
    free(entry);
}


static void arc_move_node(
    ARCCache *cache,
    List     *old_list,
    List     *new_list,
    ListNode *node,
    int       new_list_id
) {
    if (!node) return;
    ARCEntry *entry = (ARCEntry *)node->data;




    list_remove_node(old_list, node);

      int* new_key_ptr = (int*) malloc(sizeof(int));
    if(!new_key_ptr) exit(EXIT_FAILURE);

    *new_key_ptr = *entry->key_ptr;
    entry->key_ptr  = new_key_ptr;

    ListNode *new_node = list_insert_node_head_return(new_list, entry);

    entry->list_id = new_list_id;

  


    hashtable_remove(cache->table, entry->key_ptr);
    hashtable_insert(cache->table, entry->key_ptr, new_node);
}

static ARCEntry* arc_remove_tail(ARCCache *cache, List *list, int *list_count) {
    ARCEntry *entry = (ARCEntry *)list_remove_tail(list);
    if (!entry) return NULL;
    int* new_key_ptr  = (int *) malloc(sizeof(int));
   *new_key_ptr = *entry->key_ptr;  // why new key -> cuz we may need this key value or this ARCentry again to be in another list maybe 
   // frees only the hASHTABLE ENTRY inside key and data are remaining to be clean data is Listnode cleaning is handled when list remove_Tail / remove_
   // NOT ARCENTRY HSH_ENTRY->VALUE->DATA = ARC_ENTY
    hashtable_remove(cache->table, entry->key_ptr); 
    entry->key_ptr = new_key_ptr;  // new allocated key if we remove this  ARC entry then we need to deallocate this key 
    (*list_count)--;
    return entry;
}

static ListNode* arc_insert_head(
    ARCCache *cache,
    List     *list,
    int      *count,
    int       key,
    int       value,
    int       list_id
) {
    ARCEntry *entry = (ARCEntry *)malloc(sizeof(ARCEntry));
    if (!entry) exit(EXIT_FAILURE);

    entry->key_ptr = (int *)malloc(sizeof(int));
    if (!entry->key_ptr) exit(EXIT_FAILURE);

    *(entry->key_ptr) = key;
    entry->value      = value;
    entry->list_id    = list_id;

    ListNode *node = list_insert_node_head_return(list, entry);
    (*count)++;

    hashtable_insert(cache->table, entry->key_ptr, node);
    return node;
}

static ListNode* arc_lookup_node(ARCCache *cache, int key) {
    int temp_key = key;
    return (ListNode *)hashtable_lookup(cache->table, &temp_key);
}


static void arc_replace(ARCCache *cache, int inB2, int request_key);


static void arc_request(Cache *cache_obj, int key, int value) {
    ARCCache *cache = (ARCCache *)cache_obj->data;


    ListNode *node = arc_lookup_node(cache, key);

    // in hashtable
    if (node) {
        ARCEntry *entry = (ARCEntry *)node->data;
        int list_id = entry->list_id;

        if (list_id == 1 || list_id == 2) {
            if (list_id == 1) {
                arc_move_node(cache, cache->T1, cache->T2, node, 2);
                cache->t1_count--;
                cache->t2_count++;
            } else {
                move_node_to_head(cache->T2, node); 
            }
            entry->value = value;
            return;
        }
        // in b1 increment p and replace() and put in t2
        else if (list_id == 3) {
            int delta = (cache->b1_count == 0) 
                          ? 1
                          : (cache->b2_count / cache->b1_count);
            if (delta < 1) delta = 1;
            cache->p = (cache->p + delta > cache->capacity) 
                         ? cache->capacity 
                         : cache->p + delta;

            arc_replace(cache, 0, key);

            list_remove_node(cache->B1, node);
            cache->b1_count--;

              hashtable_remove(cache->table, entry->key_ptr);

            arc_destroy_entry(entry); // free the old ghost entry

            /* Now insert a new entry in T2 (with the new value). */
            arc_insert_head(cache, cache->T2, &cache->t2_count, key, value, 2);
            return;
        }
        else if (list_id == 4) {
            // b2 hit --> decrement p and move to t2
            int delta = (cache->b2_count == 0)
                          ? 1
                          : (cache->b1_count / cache->b2_count);
            if (delta < 1) delta = 1;
            cache->p = (cache->p - delta < 0)
                         ? 0
                         : (cache->p - delta);

            arc_replace(cache, 1, key);
             hashtable_remove(cache->table, entry->key_ptr);

            list_remove_node(cache->B2, node);
            cache->b2_count--;

            arc_destroy_entry(entry);

            arc_insert_head(cache, cache->T2, &cache->t2_count, key, value, 2);
            return;
        }
    }

  // miss
    int t1b1 = cache->t1_count + cache->b1_count;
    int all_lists = cache->t1_count + cache->t2_count + cache->b1_count + cache->b2_count;

    if (t1b1 == cache->capacity) {
        //l1 = c
        if (cache->t1_count < cache->capacity) {
            //T1<c 

            // remove entry from b1 so no need of that ever so free key also 
            ARCEntry *victim = arc_remove_tail(cache, cache->B1, &cache->b1_count);
            if (victim) {

                    free(victim->key_ptr);
                arc_destroy_entry(victim);
            }
            arc_replace(cache, 0, key);
        } else {
            // T1 == c no need to store in ghpst

            // evict from t1 here internally hashtable remove frees old pointer 
            // this victim contains a new ptr with same why do this --> need to insert this in b1
            // if this was not to be reinserted then no need to assign new_ptr here 
            // but so we need to keep in mind to   free key ptr
            ARCEntry *victim = arc_remove_tail(cache, cache->T1, &cache->t1_count);
            if (victim) {
              
               // ListNode *ghost_node = list_insert_node_head_return(cache->B1, victim);
                //victim->list_id = 3; 
               // cache->b1_count++;

               /// FREEING CUZ NOT TO BE USED EVER AS IT IS NOT BEING PUT IM B1
                       free(victim->key_ptr);
                arc_destroy_entry(victim);
             
                //hashtable_insert(cache->table, victim->key_ptr, ghost_node);
            }
        }
    }
    else if (t1b1 < cache->capacity && (all_lists >= cache->capacity)) {
        if (all_lists == 2 * cache->capacity) {
            //L1+L2 = 2c
            ARCEntry *victim = arc_remove_tail(cache, cache->B2, &cache->b2_count);
            if (victim) {
               free(victim->key_ptr);
                arc_destroy_entry(victim);
            }
        }
        arc_replace(cache, 0, key);
    }

    arc_insert_head(cache, cache->T1, &cache->t1_count, key, value, 1);
}

static void arc_replace(ARCCache *cache, int inB2, int request_key) {
    int t1_size = cache->t1_count; 
    // If T1 >= 1 and ((inB2 == 1 and T1 == p) or (T1 > p)) => evict from T1 -> B1 
    if (t1_size >= 1 &&
        ((inB2 && t1_size == cache->p) || (t1_size > cache->p))) {

        //Move LRU of T1 to the head of B1. here even though we cleared old key pointer we have new pointer in victim
        // so that we can add to hashtable --> if used earlier cleared ptr we would be garbage value
        ARCEntry *victim = arc_remove_tail(cache, cache->T1, &cache->t1_count);
      //  printf("REMOVED %d key from T1 as T1--size --%d but permitted --%d\n",*(victim->key_ptr),t1_size,cache->p);
        if (victim) {
            // Insert it into B1. 
            ListNode *ghost_node = list_insert_node_head_return(cache->B1, victim);
            victim->list_id = 3; // B1
            cache->b1_count++;
            hashtable_insert(cache->table, victim->key_ptr, ghost_node);
           
        }
    }
    else {
        // Evict from T2 -> B2. 
        ARCEntry *victim = arc_remove_tail(cache, cache->T2, &cache->t2_count);
        if (victim) {
            ListNode *ghost_node = list_insert_node_head_return(cache->B2, victim);
            victim->list_id = 4; // B2
            cache->b2_count++;
            hashtable_insert(cache->table, victim->key_ptr, ghost_node);
        
        }
    }
}


static void arc_put(Cache *cache_obj, int key, int value) {
    ARCCache *cache = (ARCCache *)cache_obj->data;
   

    ListNode *node = arc_lookup_node(cache, key);
    if (node) {
        ARCEntry *entry = (ARCEntry *)node->data;
        if (entry->list_id == 1 || entry->list_id == 2) {
           // cache->hits++;
        }
    }
    arc_request(cache_obj, key, value);
}


static int arc_get(Cache *cache_obj, int key, int *value) {
    ARCCache *cache = (ARCCache *)cache_obj->data;
 cache->accesses++;

    //arc_request(cache_obj, key, *value);

    /* After that, see if it's now in T1 or T2. */
    ListNode *node = arc_lookup_node(cache, key);
    if (!node) {
        return 0; /* not present */
    }

    ARCEntry *entry = (ARCEntry *)node->data;
    if (entry->list_id == 1 || entry->list_id == 2) {

       // printf("KEY--%d HIT IN LIST T%d \n",key,entry->list_id);
        cache->hits++;
        if (value) {
            *value = entry->value;
        }
         arc_request(cache_obj, key, *value);

        return 1;
    }
     arc_request(cache_obj, key, *value);

    return 0;
}

static double arc_hit_ratio(Cache *cache_obj) {
    ARCCache *cache = (ARCCache *)cache_obj->data;
   // printf("ARC: Total accesses = %.0f\n", cache->accesses);
   // printf("ARC: Total hits     = %.0f\n", cache->hits);
    if (cache->accesses == 0.0) return 0.0;
    return (cache->hits / cache->accesses);
}

/* 
 *  1) The hashtable --> Hashtable entry + key if destroy_key function is given
 *  2) The 4 lists T1/T2/B1/B2, each of which will free the node +  each ARCEntry via arc_destroy_entry.
 *  3) The ARCCache struct itself and the parent Cache. */
static void arc_destroy(Cache *cache_obj) {
    if (!cache_obj) return;
    ARCCache *cache = (ARCCache *)cache_obj->data;


    hashtable_destroy(cache->table);

    list_destroy(cache->T1, arc_destroy_entry);
    list_destroy(cache->T2, arc_destroy_entry);
    list_destroy(cache->B1, arc_destroy_entry);
    list_destroy(cache->B2, arc_destroy_entry);

    free(cache);
    free(cache_obj);
}

static CacheOps arc_ops = {
    .put       = arc_put,
    .get       = arc_get,
    .destroy   = arc_destroy,
    .hit_ratio = arc_hit_ratio
};

Cache* create_cache_arc(int capacity) {
    Cache *cache_obj = (Cache *)malloc(sizeof(Cache));
    if (!cache_obj) exit(EXIT_FAILURE);

    ARCCache *cache = (ARCCache *)malloc(sizeof(ARCCache));
    if (!cache) exit(EXIT_FAILURE);

    cache->capacity = capacity;
    cache->hits = 0.0;
    cache->accesses = 0.0;
    cache->p = 0;  


    cache->T1 = list_create();
    cache->T2 = list_create();
    cache->B1 = list_create();
    cache->B2 = list_create();

    cache->t1_count = 0;
    cache->t2_count = 0;
    cache->b1_count = 0;
    cache->b2_count = 0;

    int table_size = capacity * 2 + 1;
    cache->table = hashtable_create(
        table_size,
        arc_hash_func,
        arc_key_cmp,
        arc_destroy_key,
         NULL
    );

    cache_obj->ops  = &arc_ops;
    cache_obj->data = cache;
    return cache_obj;
}
