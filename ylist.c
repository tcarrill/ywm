#include <stdlib.h>
#include "ylist.h"

YList *ylist_new() 
{
  YList *list = malloc(sizeof(YList));
  list->length = 0;
  list->head = NULL;
  list->tail = NULL;
  return list;
}

void ylist_add_head(YList *list, void *data)
{
  YNode *node = malloc(sizeof(YNode));
  if (node != NULL) {
    node->data = data;
    node->prev = NULL;
    node->next = list->head;
    if (list->head != NULL) {
      list->head->prev = node;
    }
    list->head = node;
    list->length++;
  }
}

void ylist_add_tail(YList *list, void *data)
{
  YNode *node = malloc(sizeof(YNode));
  if (node != NULL) {
    node->data = data;
    node->prev = list->tail;
    node->next = NULL;
    if (list->tail != NULL) {
      list->tail->next = node;
    } 
	if (list->head == NULL) {
		list->head = node;
	}
    list->tail = node;
    list->length++;
  }
}

YNode *ylist_remove_head(YList *list)
{
  if (list->head == NULL) {
    return NULL; 
  }
  YNode *node = list->head;
  list->head = node->next;
  if (list->head != NULL) {
    list->head->prev = NULL;
  }
  list->length--;
  return node;
}

YNode *ylist_remove_tail(YList *list)
{
  if (list->tail == NULL) {
    return NULL;  
  }
  YNode *node = list->tail;
  list->tail = node->prev;
  if (list->tail != NULL) {
    list->tail->next = NULL;
  }
  list->length--;
  return node;
}

void ylist_destroy(YList *list) 
{
  YDestroyFunc destroy_func = free;
  ylist_destroy_deep(list, destroy_func);
}

void ylist_destroy_deep(YList *list, YDestroyFunc destroy_func)
{
  YNode *current = list->head;
  while (current != NULL) {
    YNode *next = current->next;
    destroy_func(current->data);
    free(current);
    current = next;
  }
  free(list);
}