#include <stdlib.h>
#include<stdio.h>
#include "list.h"


// HERE HASHTABLE ENTRY key_ptr ---> LISTNODE* -> data == LRU/CLOCK/2Q/ARC
List *list_create(void) {
    List *list = (List *)malloc(sizeof(List));
    list->head = list->tail = NULL;
    list->count = 0;
    return list;
}

void list_insert_head(List *list, void *data) {
    ListNode *node = (ListNode *)malloc(sizeof(ListNode));
    node->data = data;
    node->prev = NULL;
    node->next = list->head;
    if (list->head)
        list->head->prev = node;
    else
        list->tail = node;
    list->head = node;
    list->count++;
}

void list_remove_node(List *list, ListNode *node) {
    if (!node) return;
    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;
    free(node);
    list->count--;
}
 ListNode* list_insert_node_head_return(List *list, void *data)
{
    ListNode *node = (ListNode *)malloc(sizeof(ListNode));

    node->data = data;
    node->prev = NULL;
    node->next = list->head;

    if (list->head)
        list->head->prev = node;
    else
        list->tail = node;

    list->head = node;
    list->count++;

    return node;
}


 void move_node_to_head(List *list, ListNode *node)
{
    if (!list || !node || list->head == node) {
       
        return;
    }

    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;

    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;

    /* Re-insert at the head */
    node->prev = NULL;
    node->next = list->head;

    if (list->head)
        list->head->prev = node;
    else
        list->tail = node;  

    list->head = node;
    
}


void *list_remove_tail(List *list) {
    if (!list->tail) return NULL;
    ListNode *node = list->tail;
    void *data = node->data;
    if (node->prev)
        node->prev->next = NULL;
    else
        list->head = NULL;
    list->tail = node->prev;
    free(node);
    list->count--;
    return data;
}



void list_destroy(List *list, void (*destroy_data)(void*)) {
    ListNode *node = list->head;
    while (node) {
        ListNode *next = node->next;
        if(destroy_data){
            //printf("FRREEINg  NODE_DATA IE CACHE/LRU/2Q ENTRY\n");
            destroy_data(node->data);
        }
       // printf("FREEING NODE --LIST NODE\n");
        free(node);
        node = next;
    }
    
    free(list);
}
