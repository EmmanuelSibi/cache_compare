#ifndef HASH_TABLE_H
#define HASH_TABLE_H


typedef struct HashTableEntry {
    void *key;
    void *value;
    struct HashTableEntry *next;
} HashTableEntry;

typedef struct HashTable {
    int size;                   
    HashTableEntry **buckets;   
    unsigned int (*hash_func)(const void *key);
    int (*key_cmp)(const void *key1, const void *key2);
    // destroy functions cause in generic ones --> we may not know if key or value is what structure and if dynamicaaly allocated
    void (*destroy_key)(void *key);
    void (*destroy_value)(void *value);
} HashTable;

HashTable *hashtable_create(int size,
                              unsigned int (*hash_func)(const void *),
                              int (*key_cmp)(const void *, const void *),
                              void (*destroy_key)(void *),
                              void (*destroy_value)(void *));

void hashtable_insert(HashTable *ht, void *key, void *value);

void *hashtable_lookup(HashTable *ht, const void *key);

void hashtable_remove(HashTable *ht, const void *key);

void hashtable_destroy(HashTable *ht);

#endif
