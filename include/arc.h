#if !defined(ARC_H)
#define ARC_H

#include "list.h"
#include "hashtable.h"
#include "cache.h"


typedef enum {
    ARC_T1 = 1,
    ARC_T2 = 2, 
    ARC_B1 = 3, 
    ARC_B2 = 4  
} ARCListType;

typedef struct {
    int buff_index;   
    
        int ghost_key; 
    
    ARCListType list_type;
} ARCEntry;

typedef struct {
    int capacity;
    int p;   //t1 s size 

    List *T1;
    List *T2;
    List *B1;
    List *B2;

    int t1_count;
    int t2_count;
    int b1_count;
    int b2_count;

    HashTable *table;

  
  int next_buffer_index;  

    int *free_indices;      
    int free_count;
} ARCCache;

Cache* create_cache_arc(int capacity);



#endif // ARC_H
