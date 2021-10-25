#ifndef INC_BUF_H
#define INC_BUF_H

#include <stdint.h>
#include <common/string.h>

#define BSIZE   512

#define B_VALID 0x2     /* Buffer has been read from disk. */
#define B_DIRTY 0x4     /* Buffer needs to be written to disk. */

struct buf {
    int flags;
    uint32_t blockno;
    uint8_t data[BSIZE]; // 1B*512

    /* TODO: Your code here. */
    struct buf* qnext;
};

static inline void
init_buflist(struct buf* head)
{
    head->blockno = 0;
    head->flags = 0;
    memset(head->data, 0, sizeof(head->data));
    head->qnext = NULL;
}

static inline void
buflist_push(struct buf* n,
    struct buf* head)
{
    struct buf* tmp = head;
    while (tmp->qnext != NULL)
        tmp = tmp->qnext;

    tmp->qnext = n;
    n->qnext = NULL;
}

static inline struct buf*
buflist_front(struct buf* head)//head != NULL
{
    return head->qnext;
}

static inline void
buflist_pop(struct buf* head)//head != NULL
{
    head->qnext = (head->qnext)->qnext;
}

static inline int
buflist_empty(struct buf* head)
{
    return (head->qnext == NULL);
}

#endif
