#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "list.h"
#include "hashtable.h"
#include "arc.h"




static int* get_arc_entry_key(Cache *cache_obj, ARCEntry *entry) {
    if (entry->list_type == ARC_B1 || entry->list_type == ARC_B2) {
        return &entry->ghost_key;
    }
    else {
        return &cache_obj->data_buffer[entry->buff_index].key;
    }
}

static int* get_arc_entry_value(Cache *cache_obj, ARCEntry *entry) {
    // Only valid for active entries
    return &cache_obj->data_buffer[entry->buff_index].value;
}

static unsigned int arc_hash_func(const void *key) {
    int k = *(const int *)key;
    return (unsigned int)k;
}

static int arc_key_cmp(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return ia - ib;
}

static void arc_destroy_key(void *key) {
    // Keys are stored within ARCEntry/data_buffer, so no separate free
}


static void arc_destroy_entry(void *data) {
    if (data) {
        free((ARCEntry *)data);
    }
}






static ListNode* arc_move_node(Cache *cache_obj, ARCCache *arc,
                               List *old_list, List *new_list,
                               ListNode *old_node, ARCListType new_list_type) {

        
    ARCEntry *entry = (ARCEntry *)old_node->data;
    int *old_key_ptr = (entry->list_type == ARC_B1 || entry->list_type == ARC_B2)
                       ? &entry->ghost_key
                       : &cache_obj->data_buffer[entry->buff_index].key;

    // active to ghost list 
    if ((entry->list_type == ARC_T1 || entry->list_type == ARC_T2) &&
        (new_list_type == ARC_B1 || new_list_type == ARC_B2)) {
        entry->ghost_key = cache_obj->data_buffer[entry->buff_index].key;
        // free that index
        arc->free_indices[arc->free_count++] = entry->buff_index;
        entry->buff_index = -1;
    }
    // ghost list to active 
    else if ((entry->list_type == ARC_B1 || entry->list_type == ARC_B2) &&
             (new_list_type == ARC_T1 || new_list_type == ARC_T2)) {
        int buffer_index;
        if (arc->free_count > 0) {
            buffer_index = arc->free_indices[--arc->free_count];
        } else if (arc->next_buffer_index < cache_obj->capacity) {
            buffer_index = arc->next_buffer_index++;
        }
        entry->buff_index = buffer_index;
        cache_obj->data_buffer[buffer_index].key   = entry->ghost_key;
        cache_obj->data_buffer[buffer_index].value = 0; //updates later
    }

    list_remove_node(old_list, old_node);
    ListNode *new_node = list_insert_node_head_return(new_list, entry);
    entry->list_type = new_list_type;
    hashtable_remove(arc->table, old_key_ptr);
    int *new_key_ptr = (new_list_type == ARC_B1 || new_list_type == ARC_B2)
                       ? &entry->ghost_key
                       : &cache_obj->data_buffer[entry->buff_index].key;
    hashtable_insert(arc->table, new_key_ptr, new_node);
    return new_node;
}


static void arc_replace(Cache *cache_obj, ARCCache *arc, bool inB2, int request_key) {

    // t1>=1 && ( t1=p && x E B2 || t1>p)

    if (arc->t1_count >= 1 &&
        ((inB2 && arc->t1_count == arc->p) || (arc->t1_count > arc->p))) {
        // Evict from T1 -> put into  B1
        ARCEntry *victim = (ARCEntry *)list_remove_tail(arc->T1);
        if (victim) {
            int *active_key_ptr = &cache_obj->data_buffer[victim->buff_index].key;
            hashtable_remove(arc->table, active_key_ptr);

            victim->ghost_key = *active_key_ptr;

            // that index is now free put it into free list 
            arc->free_indices[arc->free_count++] = victim->buff_index;
            victim->buff_index = -1;

            victim->list_type = ARC_B1;
            ListNode *ghost_node = list_insert_node_head_return(arc->B1, victim);
            hashtable_insert(arc->table, &victim->ghost_key, ghost_node);

            arc->t1_count--;
            arc->b1_count++;
        }
    } else {
        // ELSE  Evict from T2 -> B2
        
        ARCEntry *victim = (ARCEntry *)list_remove_tail(arc->T2);
        if (victim) {
            int *active_key_ptr = &cache_obj->data_buffer[victim->buff_index].key;
            hashtable_remove(arc->table, active_key_ptr);

            victim->ghost_key = *active_key_ptr;

            //put index into freeindex list 
            arc->free_indices[arc->free_count++] = victim->buff_index;
            victim->buff_index = -1;

            victim->list_type = ARC_B2;
            ListNode *ghost_node = list_insert_node_head_return(arc->B2, victim);
            hashtable_insert(arc->table, &victim->ghost_key, ghost_node);

            arc->t2_count--;
            arc->b2_count++;
        }
    }
}



static ListNode* arc_insert_head(Cache *cache_obj, ARCCache *arc,
                                 List *list, int *count,
                                 int key, int value,
                                 ARCListType list_type) {
    ARCEntry *entry = (ARCEntry *)malloc(sizeof(ARCEntry));
    

    entry->list_type = list_type;
    if (list_type == ARC_T1 || list_type == ARC_T2) {
        int buffer_index;
        if (arc->free_count > 0) {
            buffer_index = arc->free_indices[--arc->free_count];
        } else if (arc->next_buffer_index < cache_obj->capacity) {
            buffer_index = arc->next_buffer_index++;
        } 
        entry->buff_index = buffer_index;
        cache_obj->data_buffer[buffer_index].key   = key;
        cache_obj->data_buffer[buffer_index].value = value;
    } else {
        //ghosyt list entry
        entry->buff_index = -1;
        entry->ghost_key  = key;
    }

    ListNode *node = list_insert_node_head_return(list, entry);
    (*count)++;

    int *key_ptr = (list_type == ARC_T1 || list_type == ARC_T2)
                   ? &cache_obj->data_buffer[entry->buff_index].key
                   : &entry->ghost_key;
    hashtable_insert(arc->table, key_ptr, node);

    return node;
}



static void arc_request(Cache *cache_obj, int key, int value) {
    ARCCache *arc = (ARCCache *)cache_obj->data;
    ListNode *node = hashtable_lookup(arc->table, &key);

    if (node) {
        ARCEntry *entry = (ARCEntry *)node->data;
        ARCListType lt = entry->list_type;
        // CASE 1
        if (lt == ARC_T1) {
            //  T1 hit --> chnage value and move to T2
            //*get_arc_entry_value(cache_obj, entry) = value;
            arc_move_node(cache_obj, arc, arc->T1, arc->T2, node, ARC_T2);
            arc->t1_count--;
            arc->t2_count++;
            return;
        }
        //CASE 1
        else if (lt == ARC_T2) {
            // T2 hit  ----> move to head
           // *get_arc_entry_value(cache_obj, entry) = value;
            //arc_move_node_to_head(cache_obj, arc, arc->T2, node);
            move_node_to_head(arc->T2, node);
            return;
        }

        //CASE 2
        else if (lt == ARC_B1) {
            // hit in B1 increment p call replace and movd to T2.
            int delta = (arc->b1_count == 0) ? 1 : (arc->b2_count / arc->b1_count);
            if (delta < 1) delta = 1;
            arc->p += delta;
            if (arc->p > arc->capacity) arc->p = arc->capacity;

            arc_replace(cache_obj, arc, 0, key);

            // Promote B1 -> T2.
            node = arc_move_node(cache_obj, arc, arc->B1, arc->T2, node, ARC_T2);
            arc->b1_count--;
            arc->t2_count++;

            ARCEntry *new_entry = (ARCEntry *)node->data;
            *get_arc_entry_value(cache_obj, new_entry) = value;
            return;
        }
        // CASE 3
        else if (lt == ARC_B2) {
            // hit in B2 so decrement p and call replace and move to T2
            int delta = (arc->b2_count == 0) ? 1 : (arc->b1_count / arc->b2_count);
            if (delta < 1) delta = 1;
            arc->p = (arc->p < delta) ? 0 : (arc->p - delta);

            arc_replace(cache_obj, arc, true, key);

            node = arc_move_node(cache_obj, arc, arc->B2, arc->T2, node, ARC_T2);
            arc->b2_count--;
            arc->t2_count++;

            ARCEntry *new_entry = (ARCEntry *)node->data;
            *get_arc_entry_value(cache_obj, new_entry) = value;
            return;
        }
    }

    //CASE 4
    // not in any list --> CASE 4 complete miss need to insert at top pf T1
    int l1 = arc->t1_count + arc->b1_count;
    int total = arc->t1_count + arc->t2_count + arc->b1_count + arc->b2_count;

    //l1 = |c|
    if (l1 == arc->capacity) {
        //t1<c so remove lru from b1
        if (arc->t1_count < arc->capacity) {
            // Remove tail of B1.
            ARCEntry *victim = (ARCEntry *)list_remove_tail(arc->B1);
            if (victim) {
                arc->b1_count--;
                hashtable_remove(arc->table, &victim->ghost_key);
                free(victim);
            }
            arc_replace(cache_obj, arc, 0, key);
        } else {
            // Remove tail of T1.
            ARCEntry *victim = (ARCEntry *)list_remove_tail(arc->T1);
            if (victim) {
                arc->t1_count--;
                int *active_key_ptr = &cache_obj->data_buffer[victim->buff_index].key;
                hashtable_remove(arc->table, active_key_ptr);
                arc->free_indices[arc->free_count++] = victim->buff_index;
                free(victim);
            }
        }
    }
    // l1<c and l1+l2 >=c
    else if (l1 < arc->capacity && total >= arc->capacity) {
        // If total reaches 2*capacity, remove tail of B2.
        if (total == 2 * arc->capacity) {
            ARCEntry *victim = (ARCEntry *)list_remove_tail(arc->B2);
            if (victim) {
                arc->b2_count--;
                hashtable_remove(arc->table, &victim->ghost_key);
                free(victim);
            }
        }
        arc_replace(cache_obj, arc, 0, key);
    }

    // insert new entry in T1.
    arc_insert_head(cache_obj, arc, arc->T1, &arc->t1_count, key, value, ARC_T1);
}


static void arc_put(Cache *cache_obj, int key, int value) {
    ARCCache *arc = (ARCCache *)cache_obj->data;

    // If key is already in T1/T2, update its value.
    ListNode *node = hashtable_lookup(arc->table, &key);
    if (node) {
        ARCEntry *entry = (ARCEntry *)node->data;
        if (entry->list_type == ARC_T1 || entry->list_type == ARC_T2) {
            *get_arc_entry_value(cache_obj, entry) = value;
        }
    }
    // Process the request.
    arc_request(cache_obj, key, value);
}



static int arc_get(Cache *cache_obj, int key, int *value) {
    ARCCache *arc = (ARCCache *)cache_obj->data;
    ListNode *node = hashtable_lookup(arc->table, & key);
    //miss
    if (!node) {
        return 0;
    }
    ARCEntry *entry = (ARCEntry *)node->data;
    if (entry->list_type == ARC_T1 || entry->list_type == ARC_T2) {
        //  hit in T1/T2.
        *value = *get_arc_entry_value(cache_obj, entry);
        arc_request(cache_obj, key, *value);
        return 1;
    } else {
       // ghost lisrt hit 
        arc_request(cache_obj, key, 0);
        return 0;
    }
}



static void arc_destroy(Cache *cache_obj) {
    if (!cache_obj) return;
    ARCCache *arc = (ARCCache *)cache_obj->data;
    // printf("T1 count = %d\n",arc->t1_count);
    // printf("T2 count = %d\n",arc->t2_count);
    // printf("B1 count = %d\n",arc->b1_count);
    // printf("B2 count = %d\n",arc->b2_count);
    //     //printing the t1_list
    //     ListNode *node_t1_head = arc->T1->head;
    //     while(node_t1_head){
    //         ARCEntry *entry = (ARCEntry *)node_t1_head->data;
    //         printf("T1: %d\n ",*get_arc_entry_key(cache_obj, entry));
    //         node_t1_head = node_t1_head->next;
    //     }
    //     printf("\n");
    
 
    //     //printing the t2_list
    //     ListNode *node_t2_head = arc->T2->head;
    //     while(node_t2_head){
    //         ARCEntry *entry = (ARCEntry *)node_t2_head->data;
    //         printf("T2: %d \n",*get_arc_entry_key(cache_obj, entry));
    //         node_t2_head = node_t2_head->next;
    //     }
    //     printf("\n");
    
    
    //     //printing the b1_list
    //     ListNode *node_b1_head = arc->B1->head;
    //     while(node_b1_head){
    //         ARCEntry *entry = (ARCEntry *)node_b1_head->data;
    //         printf("B1: %d \n",*get_arc_entry_key(cache_obj, entry));
    //         node_b1_head = node_b1_head->next;
    //     }
    //     printf("\n");
    
    
    //     //printing the b2_list
    //     ListNode *node_b2_head = arc->B2->head;
    //     while(node_b2_head){
    //         ARCEntry *entry = (ARCEntry *)node_b2_head->data;
    //         printf("B2: %d \n",*get_arc_entry_key(cache_obj, entry));
    //         node_b2_head = node_b2_head->next;
    //     }
    //     printf("\n");
    
    hashtable_destroy(arc->table);

    list_destroy(arc->T1, arc_destroy_entry);
    list_destroy(arc->T2, arc_destroy_entry);
    list_destroy(arc->B1, arc_destroy_entry);
    list_destroy(arc->B2, arc_destroy_entry);

    free(arc->free_indices);
    free(arc);
    free(cache_obj->data_buffer);
    free(cache_obj);
}



static CacheOps arc_ops = {
    .put     = arc_put,
    .get     = arc_get,
    .destroy = arc_destroy,
};



Cache* create_cache_arc(int capacity) {
    Cache *cache_obj = (Cache *)malloc(sizeof(Cache));

    cache_obj->accessess   = 0;
    cache_obj->hits        = 0;
    cache_obj->capacity    = capacity;
    cache_obj->data_buffer = (CacheEntry *)calloc(capacity, sizeof(CacheEntry));

    ARCCache *arc = (ARCCache *)malloc(sizeof(ARCCache));

    arc->capacity = capacity;
    arc->p = 0;  

    arc->T1 = list_create();
    arc->T2 = list_create();
    arc->B1 = list_create();
    arc->B2 = list_create();

    arc->t1_count = 0;
    arc->t2_count = 0;
    arc->b1_count = 0;
    arc->b2_count = 0;

    arc->next_buffer_index = 0;
    arc->free_indices      = (int *)malloc(sizeof(int) * capacity);
    arc->free_count        = 0;

    int table_size = capacity * 2 + 1;
    arc->table = hashtable_create(table_size,
                                  arc_hash_func,
                                  arc_key_cmp,
                                  arc_destroy_key,
                                  NULL);

    cache_obj->ops  = &arc_ops;
    cache_obj->data = arc;
    return cache_obj;
}
