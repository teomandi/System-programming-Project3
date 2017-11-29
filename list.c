# include <stdio.h>
# include <stdlib.h> 
# include <string.h>
//its a queue created like list with pointers at the end and at the start.



typedef struct node{
	char *info;
	struct node* next;
}node;

typedef struct List{
	node* head;
	int size;
	int max;
}List;

node* createNode(char* info){
	node* n = malloc(sizeof(node));
	n->info=malloc(sizeof(char)*strlen(info));	//this free will happen in the worker process
	strcpy(n->info,info);
	n->next=NULL;
	return n;
}

void deleteNode(node* n){
	free(n->info); //in the worker
	free(n);
}

List* createList(int max){
	List *list = malloc(sizeof(List));
	list->max = max;
	list->head=NULL;
	list->size=0;
	return list;
}

void destroyList(List *list){
	node *temp = list->head;
	node *temp2;
	while(temp!=NULL){
		temp2=temp->next;
		deleteNode(temp);
		temp=temp2;
	}
	free(list);
}

int add2List(List *list, char* info){//always puting in start
	if(list->max <= list->size)
		return 0;//fail to input
	node *n=createNode(info);
	n->next=list->head;
	list->head=n;
	list->size++;
	return 1;
}

char* removeLast(List *list){
	if(isEmpty())
		return NULL;
	node *temp1=list->head;
	node *temp2=list->head;
	while(temp1->next!=NULL)
		temp1=temp1->next;
	if(sizeList(list)>1){
		while(temp2->next!=temp1)
			temp2=temp2->next;
		temp2->next=NULL;//now he is the last
	}
	else
		list->head=NULL;
	list->size--;
	char *t = malloc(sizeof(char)*strlen(temp1->info));
	strcpy(t,temp1->info);
	deleteNode(temp1);
	return t;
}

int sizeList(List *list){
	return list->size;
}

int isFull(List *list){
	return list->size==list->max;
}

int isEmpty(List *list){
	return list->size==0;
}

void printList(List *list){
	printf("Print List**\n");
	node *temp=list->head;
	int i=1;
	if(isEmpty(list)){
		printf("List is Empty.\n");
		return;
	}
	do{
		printf("%d)%s->\n",i,temp->info);
		temp=temp->next;
		i++;
	}while(temp!=NULL);
	printf("->NULL\n");
}