#include <stdlib.h>
#include <string.h>

#include "ylist.h"

void ylist_init(YList *list, void (*destroy)(void *data)) {
  list->size = 0;
  list->destroy = destroy;
  list->head = NULL;
  list->tail = NULL;

  return;
}

void ylist_destroy(YList *list) {
  void *data;

  while (ylist_size(list) > 0) {
    if (ylist_remove(list, ylist_tail(list), (void **)&data) == 0 && list->destroy != NULL) {
      list->destroy(data);
    }
  }

  memset(list, 0, sizeof(YList));
  
  return;
}

int ylist_ins_next(YList *list, YNode *element, const void *data) {
  YNode *new_element;

  if (element == NULL && ylist_size(list) != 0) {
    return -1;	
  }

  if ((new_element = (YNode *)malloc(sizeof(YNode))) == NULL) {
    return -1;	
  }

  new_element->data = (void *)data;

  if (ylist_size(list) == 0) {
    list->head = new_element;
    list->head->prev = NULL;
    list->head->next = NULL;
    list->tail = new_element;
  } else {
    new_element->next = element->next;
    new_element->prev = element;

    if (element->next == NULL) {
      list->tail = new_element;
    } else {
      element->next->prev = new_element;
    }

    element->next = new_element;
  }

  list->size++;

  return 0;
}

int ylist_ins_prev(YList *list, YNode *element, const void *data) {
  YNode *new_element;

  if (element == NULL && ylist_size(list) != 0) {
    return -1;
  }
   

  if ((new_element = (YNode *)malloc(sizeof(YNode))) == NULL) {
    return -1;
  }
  
  new_element->data = (void *)data;

  if (ylist_size(list) == 0) {
    list->head = new_element;
    list->head->prev = NULL;
    list->head->next = NULL;
    list->tail = new_element;
  } else {
    new_element->next = element; 
    new_element->prev = element->prev;

    if (element->prev == NULL) {
      list->head = new_element;	
    } else {
      element->prev->next = new_element;    	
    }

    element->prev = new_element;
  }

  list->size++;

  return 0;
}

int ylist_remove(YList *list, YNode *element, void **data) {
  if (element == NULL || ylist_size(list) == 0) {
    return -1;
  }

  *data = element->data;

  if (element == list->head) {
    list->head = element->next;

    if (list->head == NULL)
      list->tail = NULL;
    else
      element->next->prev = NULL;

  } else {
    element->prev->next = element->next;

    if (element->next == NULL) {
      list->tail = element->prev;	
    } else {
      element->next->prev = element->prev;	
    }
  }

  free(element);
  list->size--;

  return 0;
}