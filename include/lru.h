#if !defined(LRU_H)
#define LRU_H

#include "list.h"
#include "hashtable.h"
#include "cache.h"

typedef struct {
    int buff_index;
} LRUEntry;


typedef struct {
    // int count;
    // int capacity;
    List *lru_list;         // stores LRUEntry* as node->data 
    HashTable *table;       // maps (int*) key =----> ListNode* 
} LRUCache;


Cache* create_cache_lru(int capacity);

#endif // LRU_H
