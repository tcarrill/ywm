typedef struct Node {
	void *data;
	struct Node *next;
} Node;

typedef struct List {
	unsigned int length;
	struct Node *head;
} List;

List *list_new();
void list_add(List *list, const void *data);
void *list_remove(List *list);
void list_destroy(List *list);