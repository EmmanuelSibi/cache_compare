#if !defined(CLOCK_H)
#define CLOCK_H

#include "list.h"
#include "hashtable.h"
#include "cache.h"

typedef struct {
    int buff_index;   
    bool ref;         
} ClockEntry;

typedef struct {
    // int capacity;
    // int count;

    List *slots;       
    ListNode *hand;    // pointer clock

    HashTable *table;  // maps pointer to  ]key in data_buffer[ --> ListNode*
} ClockCache;

Cache* create_cache_clock(int capacity);


#endif // CLOCK_H
