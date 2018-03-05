#include <stdio.h>

typedef struct node {
	char x;
	char y;
	struct node *prev;
	struct node *next;
} node;

node *head = NULL;
node *tail = NULL;

void add_front(char x_in, char y_in) {
	node *new_node;
	
	new_node = (node*) malloc(sizeof(node));
	new_node->x = x_in;
	new_node->y = y_in;
	new_node->next = NULL;
	new_node->prev = NULL;
	
	if (!head) {
		head = new_node;
		tail = new_node;
	} else {
		new_node->next = head;
		head->prev = new_node;
		head = new_node;
	}
}

unsigned short delete_rear(void) {
	node *temp = tail;
	// This function only has one return, and we know both x and y are under 32
	// As a result, x value can be calculated using ret % 32
	// y value can be calculated using ret / 32
	unsigned short ret = temp->x + 32*(temp->y);
	
	if (head == tail) {
		head = NULL;
		tail = NULL;
	} else {
		tail = tail->prev;
		tail->next = NULL;
	}
	
	free(temp);
	return ret;
}
