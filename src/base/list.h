#ifndef _MY_LIST_H_
#define _MY_LIST_H_
#include <stdint.h>

#define offsetof(s,m) ((uint64_t)(&(((s*)0)->m)))

typedef struct list_node_t {
    struct list_node_t *next;
    struct list_node_t *prev;
} list_node_t;

#define LIST_PPREV(ptr) (((ptr)->lnode).prev)
#define LIST_PREV(p) ((p.lnode).prev) 

#define LIST_PNEXT(ptr) (((ptr)->lnode).next)
#define LIST_NEXT(p) ((p.lnode).next)

#define LIST_CONTIANER(type, node_ptr)              \
    ((type *)((uint64_t)(node_ptr) - offsetof(type, lnode)))

// 移除当前节点 一定是非头部节点（头部是空节点）
#define LIST_REMOVE(node) \
do {                                                \
    if ((node)->next)                               \
        (node)->next->prev = (node)->prev;          \
    if ((node)->prev)                               \
        (node)->prev->next = (node)->next;          \
    (node)->next = NULL;                            \
    (node)->prev = NULL;                            \
} while(0)

// 加入到当前节点之后
#define LIST_NODE_APPEND(current, node) \
do {                                                \
    if ((current)->next)                            \
        (current)->next->prev = node;               \
    (node)->next = (current)->next;                 \
    (current)->next = node;                         \
    (node)->prev = current;                         \
} while(0)

#define LIST_FOREACH(list, p) \
for (list_node_t *p = (list)->next; p; p = (p->next))

#define LIST_EMPTY(head) ((head)->next == NULL)


#define LIST_NODE_LIST  list_node_t lnode;    

#endif
