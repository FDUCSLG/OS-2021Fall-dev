#pragma once

#include <common/defines.h>

// ListNode represents one node on a circular list.

typedef struct ListNode {
    struct ListNode *prev, *next;
} ListNode;

// initialize a sigle node circular list.
void init_list_node(ListNode *node);

// merge the list containing node1 and the list containing
// node2 into one list. Make sure node1->next == node.
void merge_list(ListNode *node1, ListNode *node2);

// remove node from the list, and then node becomes a sigle
// node list.
void detach_from_list(ListNode *node);
