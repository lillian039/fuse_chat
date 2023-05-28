#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_FILES 1024
#define MAX_DOC 1024
#define PATH_MAX 256
#define NAME_LENTH 256

enum fileType{file = 1, directory = 2};

struct node{
    char name[NAME_LENTH];
    struct node* prev;
    struct node* next;
    enum fileType type;
    char *contents;
    struct node* sons;
    struct node* father;
};


static int insert_file(struct node* target, struct node* new_node){
    // puts("insert!");
    // printf("fa:%s son:%s\n",target->name,new_node->name);
    struct node* head = target->sons;
    if(head == NULL){
        target->sons = new_node;
        return 0;
    }
    if(strcmp(new_node->name,head->name) < 0){
        new_node->next = head;
        new_node->prev = NULL;
        head->prev = new_node;
        target->sons = new_node;
        return 0;
    }
    while(head->next){
        if(strcmp(new_node->name, head->name) >= 0 && strcmp(new_node->name, head->next->name) < 0){
            if(strcmp(new_node->name, head->name) == 0)return -1;//duplicate name
            new_node->prev = head;
            new_node->next = head->next;
            head->next->prev = new_node;
            head->next = new_node;
            return 0;
        }
        head = head->next;
    }
    new_node->prev = head;
    new_node->next = NULL;
    head->next = new_node;
    return 0;
}

static int remove_file(struct node* target, char* remove_name){
    // puts("rm!");
    struct node* head = target->sons;
    if(head == NULL)return -1;
    if(strcmp(remove_name, head->name) == 0){
        target->sons = head->next;
        if(head->next!=NULL)head->next->prev = NULL;
        free(head);
        return 0;
    }
    head = head->next;
    while(head->next){
        if(strcmp(remove_name, head->name) == 0){
           if(head->prev!=NULL)head->prev->next = head->next;
           if(head->next!=NULL)head->next->prev = head->prev;
           free(head);
           return 0;
        }
        head = head->next;
    }
    if(strcmp(remove_name, head->name) == 0){
        head->prev->next = NULL;
        free(head);
        return 0;
    }
    return -1;
}

static struct node* find_node(struct node* target, char* name){
    // puts("find node!");
    // printf("fa:%s name:%s\n",target->name,name);
    struct node* head = target->sons;
    //if(head == NULL)puts("???");
    while(head != NULL){
        //printf("%s %s\n",head->name,name);
        if(strcmp(head->name,name) == 0)break;
        head = head->next;
    }
    return head;
}

