#if !defined (TWO_Q_H)
#define TWO_Q_H

#include "list.h"
#include "hashtable.h"
#include "cache.h"

typedef enum {
    TWOQ_A1IN  = 1,
    TWOQ_AM    = 2,
    TWOQ_A1OUT = 3
} TwoQListType;


typedef struct {
    int buff_index;   ///ijn case of ghost A1out buff index = -1

        int ghost_key ; //only used by ghost list A1out entries A1in & Am are in cache h so will use direct cache array index location

    TwoQListType list_type;
} TwoQEntry;


typedef struct {
    int capacity;

    int a1in_capacity;
    int am_capacity;
    int a1out_capacity;

    List *a1in;
    List *am;
    List *a1out;

    int a1in_count;
    int am_count;
    int a1out_count;

    int ghost_hits;

    HashTable *table;

    int next_buffer_index;  

    int *free_indices;      
    int free_count;
} TwoQCache;


Cache* create_cache_2q(int capacity);

#endif // TWO_Q_H

