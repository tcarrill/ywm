#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ylist.h"

typedef struct MenuItem {
	int id;
	char *label;
	char *command;
} MenuItem;

MenuItem * menuItem_new(char *label, char *command, int index) 
{
 	MenuItem *menuItem = malloc(sizeof(MenuItem));
	menuItem->id = index;
	menuItem->label = malloc(sizeof(char) * strlen(label));
	menuItem->command = malloc(sizeof(char) * strlen(command));
	printf("Alloc'ed: Label(%p) Command(%p)\n", menuItem->label, menuItem->command);
	strncpy(menuItem->label, label, strlen(label));
	strncpy(menuItem->command, command, strlen(command));
	
	return menuItem;
}

void destroy_menuItem(void *data) {
	MenuItem *item = (MenuItem *)data;
	printf("Freed: Label(%p) Command(%p)\n", item->label, item->command);
	free(item->label);
	free(item->command);
}

int main() 
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
       
    char menupath[1024]; 
	snprintf(menupath, sizeof(menupath), "%s/.ywm/ywm.menu", getenv("HOME"));
	fp = fopen(menupath, "r");
	if (fp == NULL) {
		printf("Cannot open file\n");
		exit(EXIT_FAILURE);
	}
	
	YList *list = ylist_new();
	
	int index = 0;
	while ((read = getline(&line, &len, fp)) != -1) {
		size_t ln = strlen(line) - 1;
		if (line[ln] == '\n') {
		    line[ln] = '\0';
		}
		char *label = strtok(line, "|");
		char *command = strtok(NULL, "|");
		
	 	MenuItem *menuItem = menuItem_new(label, command, index);
		ylist_add_tail(list, menuItem);
		
		index++;
	}
	
	fclose(fp);	
	if (line) {
		free(line);
	}
	
	printf("Menu Items: %d\n", list->length);
	YNode *curr = list->head;
	while (curr != NULL) {
		MenuItem *menuItem = (MenuItem *)curr->data;
		printf("%d: %s (%s)\n", menuItem->id, menuItem->label, menuItem->command);
		curr = curr->next;
	}
	
	ylist_destroy_deep(list, destroy_menuItem);
	exit(EXIT_SUCCESS);
}
