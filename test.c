#include <stdio.h>
#include <stdlib.h>

typedef struct MenuItem {
	int id;
	char *label;
	char *command;
} MenuItem;

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
	
	int index = 0;
	while ((read = getline(&line, &len, fp)) != -1) {
		printf("%s", line);
		index++;
	}
	
	fclose(fp);	
	if (line) {
		printf("free(line)\n");
		free(line);
	}
	exit(EXIT_SUCCESS);
}
