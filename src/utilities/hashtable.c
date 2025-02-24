#include <stdlib.h>
#include<stdio.h>
#include "hashtable.h"

HashTable *hashtable_create(int size,
                              unsigned int (*hash_func)(const void *),
                              int (*key_cmp)(const void *, const void *),
                            void (*destroy_key)(void *),
                              void (*destroy_value)(void *)) {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
   
    ht->size = size;
    ht->buckets = (HashTableEntry **)calloc(size, sizeof(HashTableEntry *));
   
    ht->hash_func = hash_func;
    ht->key_cmp = key_cmp;
    ht->destroy_key = destroy_key;
    ht->destroy_value = destroy_value;
    return ht;
}

void hashtable_insert(HashTable *ht, void *key, void *value) {
    unsigned int hash = ht->hash_func(key);
    int index = hash % ht->size;
    HashTableEntry *entry = (HashTableEntry *)malloc(sizeof(HashTableEntry));
  
    entry->key = key;
    entry->value = value;
    entry->next = ht->buckets[index];
    ht->buckets[index] = entry;
}

void *hashtable_lookup(HashTable *ht, const void *key) {
    unsigned int hash = ht->hash_func(key);
    int index = hash % ht->size;
    HashTableEntry *entry = ht->buckets[index];
    while (entry) {
        if (ht->key_cmp(entry->key, key) == 0)
            return entry->value;
        entry = entry->next;
    }
    return NULL;
}

void hashtable_remove(HashTable *ht, const void *key) {
    unsigned int hash = ht->hash_func(key);
    int index = hash % ht->size;
    HashTableEntry *entry = ht->buckets[index];
    HashTableEntry *prev = NULL;
    while (entry) {
        if (ht->key_cmp(entry->key, key) == 0) {
            if (prev)
                prev->next = entry->next;
            else
                ht->buckets[index] = entry->next;
            
           if (ht->destroy_key) ht->destroy_key(entry->key);
           // if (ht->destroy_value) ht->destroy_value(entry->value); // not needed here as here value is LISTNODE * it will be freed while doing list_remove_tail and list_remove_tail
            free(entry);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

void hashtable_destroy(HashTable *ht) {
    for (int i = 0; i < ht->size; i++) {
        HashTableEntry *entry = ht->buckets[i];
        while (entry) {
            HashTableEntry *next = entry->next;
            int * key =(int*) entry->key;
            if (ht->destroy_key) {
                
                ht->destroy_key(entry->key);
               // printf("FREE IN  KEY  IN HAHSTABLE\n");

                }
            //if (ht->destroy_value) ht->destroy_value(entry->value);
   // printf("ABOUT TO FREE HASHENTRY\n");
            free(entry);
            entry = next;
        }
    }
    free(ht->buckets);
    free(ht);
}
