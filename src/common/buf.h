#pragma once

#include <common/defines.h>
#include <common/string.h>
#include <common/list.h>

#define BSIZE   512

#define B_VALID 0x2     /* Buffer has been read from disk. */
#define B_DIRTY 0x4     /* Buffer needs to be written to disk. */

struct buf {
    int flags;
    u32 blockno;
    u8 data[BSIZE]; // 1B*512

    /* TODO: Your code here. */
    // struct buf* qnext;
    ListNode *node;
};

static inline void
init_buflist(struct buf* head)
{
    head->blockno = 0;
    head->flags = 0;
    memset(head->data, 0, sizeof(head->data));
    // head->qnext = NULL;
    init_list_node(head->node);
}

static inline void
buflist_push(struct buf* n,
    struct buf* head)
{
    /* struct buf* tmp = head;
    while (tmp->qnext != NULL)
        tmp = tmp->qnext;

    tmp->qnext = n;
    n->qnext = NULL; */
    merge_list(head->node, n->node);
}

static inline struct buf*
buflist_front(struct buf* head)//head != NULL
{
    // return head->qnext;
	return container_of(head->node->next, struct buf, node);
}

static inline void
buflist_pop(struct buf* head)//head != NULL
{
    // head->qnext = (head->qnext)->qnext;
    detach_from_list(head->node);
}

static inline int
buflist_empty(struct buf* head)
{
    // return (head->qnext == NULL);
    return head->node->next == head->node;
}
