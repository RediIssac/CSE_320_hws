#include <stdio.h>
# include <stdlib.h>


//Name: Rediet Negash
//ID 111820799

struct List { 
	struct List *prev, *next; 
	char data; 
}; 

void init_head(struct List *head) { 
	head->next = head->prev = head; 
}

int is_empty(struct List *head) {
	return head-> next == head; 
	
} 

void add_to_last(struct List *head, struct List *list) { 
	list->next = head; 
	list->prev = head-> prev; 
	head->prev->next = list; 
	head->prev = list; 
}

void add_to_first(struct List *head, struct List *list) { 
	list-> next = head->next;
	list -> next-> prev = list;
	list -> prev = head;
	head-> next = list;

 } 
struct List* remove_last(struct List *head) { 
	struct List *list = head->prev;
	head->prev->prev->next = head;
	head->prev = head->prev->prev;
	list->next = list->prev = NULL; 
	return list;
} 
struct List* remove_first(struct List *head) {
	struct List *list = head -> next;
	head-> next->next ->prev = head;
	head -> next = head -> next -> next;
	list->next = list->prev = NULL;
 
} 
struct List free_list[500];
struct List free_head; 
void init_free_list() {
	int i; 
	init_head(&free_head); 
	for(i = 0; i < 500; i++) //link the 500 free lists
		  add_to_last(&free_head, free_list + i);
	}
struct List *new_list() { //get a new List struct
	if(is_empty(&free_head)) { 
		printf("out of list\n"); 
		exit(0);
	} 
	return remove_first(&free_head); //queue
}

 
void del_list(struct List *list) { //free a List struct
	add_to_first(&free_head, list); //queue 

} 

char *reverse(char *str) { 
	struct List stack;
	int i;

	init_head(&stack); 

	for(i = 0; str[i]; i++) { 
		struct List *list = new_list(); 
		list-> data = str[i];
		add_to_last(&stack, list);
		//update list's data with str[i] and push list to stack 
	}

 	for(i = 0; !is_empty(&stack); i++) { 
		struct List *list = remove_last(&stack);
		str[i] = list-> data;
		 //pop list from stack and update str[i] 
		del_list(list);
	} 

	return str; 
} 


int main() { 
	char str[100]; init_free_list(); 
	printf("enter a word: "); 
	scanf("%99s", str); 
	printf("reverse: %s\n", reverse(str)); 
}  
