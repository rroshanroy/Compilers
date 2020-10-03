/* LINKED LIST IMPLEMENTATION OF THE SERVER'S BUFFER */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "packet.h"


//Structure: Node for Buffer
typedef struct node {
	int offset;
	char data[PACKET_SIZE+5];
	struct node* right;
	struct node* left;
}node;

//Structure: Linkedlist header 
typedef struct linkedlist {
	int size;
	struct node* head;
}linkedlist;


//Function: Create a linked list
linkedlist* createList() {
    linkedlist* list = (linkedlist*)malloc(sizeof(linkedlist));
    list->size = 0;
    list->head = NULL;

    return list;
}

// Function: Insert a DATA_PKT to the buffer
//          - Insert in ascending order of seq_no
//          - Avoid inserting duplicates

void insertPacketdata(linkedlist* list, DATA_PKT* packet) {
    list->size = list->size + 1;
    node* new = (node*)malloc(sizeof(node));
    strcpy(new->data, packet->data);
    new->offset = packet->seq_no;
    
    if(list->head == NULL) {
        new->right = NULL;
        new->left = NULL;

        list->head = new;
        return;
    }
    node* temp = list->head;
    while(temp->right != NULL && temp->offset < new->offset) {
        temp = temp->right;
    }
    if(temp->right==NULL) {
        //Last element
        if(temp->offset < new->offset) {
            //Right side of last element
            new->right = temp->right;
            new->left = temp;
            temp->right = new;
        }

        else if(temp->offset > new->offset) {
            //Left side of last element
            new->right = temp;
            new->left = temp->left;

            if(new->left == NULL)
                list->head = new;

            if(temp->left != NULL)
                temp->left->right = new;
            
            new->right->left = new;
        }

        else {
            //Equal to last element
            list->size = list->size - 1;
            free(new);
            return;
        }
    }
    else {
        //Middle element
        if(temp->offset == new->offset) {
            //Equal to middle element
            list->size = list->size - 1;
            free(new);
            return;
        }

        new->right = temp;
        new->left = temp->left;

        if(new->left == NULL)
            list->head = new;

        if(temp->left != NULL)
            temp->left->right = new;
        
        new->right->left = new;   
    }
    return; 
}

//Function: Print the buffer to the output file specified in server.c
void printBuffertoFile(linkedlist* list, FILE* fp) {
    node* temp = list->head;
    while(temp != NULL) {
        //printf("%s, %d", temp->data, temp->offset);
        //printf("%s", temp->data);
        fwrite((char *)temp->data,1,PACKET_SIZE,fp);
        temp = temp->right;
    }
    return;
}