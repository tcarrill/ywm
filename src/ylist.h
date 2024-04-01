#ifndef DLIST_H
#define DLIST_H

typedef struct YNode {
  void *data;
  struct YNode *prev;
  struct YNode *next;

} YNode;

typedef struct YList {
  unsigned int size;

  int (*match)(const void *key1, const void *key2);
  void (*destroy)(void *data);

  struct YNode *head;
  struct YNode *tail;
} YList;

void ylist_init(YList *list, void (*destroy)(void *data));
void ylist_destroy(YList *list);
int ylist_ins_next(YList *list, YNode *node, const void *data);
int ylist_ins_prev(YList *list, YNode *node, const void *data);
int ylist_remove(YList *list, YNode *node, void **data);

#define ylist_size(list) ((list)->size)
#define ylist_head(list) ((list)->head)
#define ylist_tail(list) ((list)->tail)
#define ylist_is_head(node) ((node)->prev == NULL ? 1 : 0)
#define ylist_is_tail(node) ((node)->next == NULL ? 1 : 0)
#define ylist_data(node) ((node)->data)
#define ylist_next(node) ((node)->next)
#define ylist_prev(node) ((node)->prev)

#endif
