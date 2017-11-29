# include <stdio.h>
# include <stdlib.h> 
# include <string.h>
//its a queue created like list with pointers at the end and at the start.
typedef struct intNode{
	int info;
	int delay;
	struct intNode* next;
}intNode;


typedef struct intList{
	intNode* head;
	int size;
}intList;


intNode* create_intNode(int info, int delay){
	intNode* n = malloc(sizeof(intNode));
	n->info=info;	
	n->delay=delay;
	n->next=NULL;
	return n;
}

void delete_intNode(intNode* n){
	free(n);
}

intList* createList(){
	intList *list = malloc(sizeof(intList));
	list->head=NULL;
	return list;
}

void destroyList(intList *list){
	intNode *temp = list->head;
	intNode *temp2;
	while(temp!=NULL){
		temp2=temp->next;
		delete_intNode(temp);
		temp=temp2;
	}
	free(list);
}

int findList(intList *list, int id){
	intNode *temp = list->head;
	while(temp!=NULL){
		printf("::::%d\n",temp->info );
		if(temp->info==id)
			return temp->delay;
		temp=temp->next;
	}
	printf("!!!!!!!!!!!!\n");
	return 0;
}

void add2List(intList *list, int info, int d){//always puting in start
	intNode *n=create_intNode(info,d);
	n->next=list->head;
	list->head=n;
	list->size++;
}