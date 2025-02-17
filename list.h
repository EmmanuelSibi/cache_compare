#ifndef LIST_H
#define LIST_H


typedef struct ListNode {
    void *data;
    struct ListNode *prev;
    struct ListNode *next;
} ListNode;


typedef struct List {
    ListNode *head;
    ListNode *tail;
    int count;
} List;

List *list_create(void);

ListNode* list_insert_node_head_return(List *list, void *data);



void list_insert_head(List *list, void *data);

void move_node_to_head(List* list, ListNode *node);


void list_remove_node(List *list, ListNode *node);


void *list_remove_tail(List *list);




void list_destroy(List *list, void (*destroy_data)(void*));

#endif
