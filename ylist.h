#ifndef YLIST_H
#define YLIST_H

typedef struct YNode {
  void *data;
  struct YNode *prev;
  struct YNode *next;
} YNode;

typedef struct YList {
  unsigned int length;
  struct YNode *head;
  struct YNode *tail;
} YList;

typedef void (*YDestroyFunc)(void *);

YList *ylist_new();
void ylist_add_head(YList *list, void *data);
void ylist_add_tail(YList *list, void *data);
YNode *ylist_remove_head(YList *list);
YNode *ylist_remove_tail(YList *list);
void ylist_destroy(YList *list);
void ylist_destroy_deep(YList *list, YDestroyFunc destroy_func);

#endif