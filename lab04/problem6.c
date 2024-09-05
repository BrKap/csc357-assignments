#include<stdio.h>
#include <stdlib.h>

struct node_st {
    int data;
    struct node_st *next;
};

/* Most of this function is completely different
   from my answers on the exam */
struct node_st *sorted_insert_list(int num, struct node_st *list) {
    struct node_st *current, *new_node;

    /* Create new node */
    new_node = (struct node_st*)malloc(sizeof(struct node_st));
    if (new_node == NULL) {
        return NULL;
    }
    /* Initilize the data */
    new_node->data = num;
    new_node->next = NULL;

    /* Check if list is empty or if the new node
       should be inserted before the head */
    if (list == NULL || num < list->data) {
        new_node->next = list;
        return new_node;
    } 
    
    current = list;
    /* Find spot to insert */
    while (current->next != NULL && current->next->data < num) {
        current = current->next;
    }

    /* Insert node at that spot */
    new_node->next = current->next;
    current->next = new_node;

    return list;
}