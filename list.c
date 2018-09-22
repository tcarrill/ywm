#include "list.h"

List *list_new() 
{
	List *list = malloc(sizeof(List));
	list->length = 0;
	list->head = NULL;
	return list;
}

void list_add(List *list, const void *data)
{
	Node *node = malloc(sizeof(Node));
	if (node != NULL) {
		node->data = data;
		node->next = list->head;
		list->head = node;
		list->length++;
	}
}

void *list_remove(List *list)
{
	if (list->head == NULL) {
		return NULL;
	}
	
	Node *curr = list->head;
	list->head = curr->next;
	list->length--;
	const void *data = curr->data;
	free(curr);
	
	return data;
}

void list_destroy(List *list) 
{
	Node *curr = list->head;
	while (curr != NULL) {
		Node *tmp = curr;
		curr = tmp->next;
		free(tmp->data);
		free(tmp);
	}
	free(list);
}