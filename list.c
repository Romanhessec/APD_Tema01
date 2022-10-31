#include <stdio.h>
#include <stdlib.h>
#include "list.h"

struct node {
    int val;
    struct node* next;
};

void add(struct node **head, int val) {
    
    struct node *new = (struct node*) malloc(sizeof(struct node));
    new->val = val;
    new->next = *head;
    *head = new;
}