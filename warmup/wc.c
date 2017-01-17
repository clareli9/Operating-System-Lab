#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
//#include "common.h"
#include "wc.h"

#define hashsize 100000

struct node{
    int count;
    char* word;

    struct node* next;
};
struct wc{
    struct node* elem;
};



struct wc* wc_init(char *word_array, long size){
    // Initialize all the elements
    struct wc* wc = malloc(sizeof(struct node*)*hashsize);
    int i;
    int j,m;
    int start = 0;
    char* target = NULL;
    unsigned long hashval = 0;
    int pos = 0;
    //bool repeat = false;
    //struct node* curr = NULL;
    for (i = 0; i < hashsize; i++){
        wc[i].elem = NULL;
    }
    // Sketch the words and return hash values
    i = 0;
    while (i < size){
        for (; i < size; i++){
            if (!isspace(word_array[i])){
               
                hashval = hashval*31 + (int)(word_array[i]);
            }
            else{
                break;
            }
        }
        pos = (int)(hashval % hashsize);
        target = malloc(sizeof(char)*(i-start+1));
        for (m = 0,j = start; m < i-start && j < i; m++,j++){
            target[m] = word_array[j];
        }
        target[i-start] = '\0';
        
        // Input the target to the correct slot
        //curr = wc[pos].elem;
         //printf("**Target begining loop-- %s\n",target);
        if (wc[pos].elem == NULL){
                             //  
           // printf("**Target before -- %s , %d\n",target, pos);

            wc[pos].elem = malloc(sizeof(struct node));
            wc[pos].elem->word = malloc(sizeof(char)*(strlen(target)+1));
            strcpy(wc[pos].elem->word,target);
            wc[pos].elem->count = 1;
            wc[pos].elem->next = NULL;
                             //   printf("**Target after-- %s\n",target);

        }
        else{
                                //printf("**Target -- %s, %d\n",target,pos);

            struct node* curr = wc[pos].elem;
            struct node* prev = NULL;
            while ( curr != NULL && strcmp(curr->word, target)!=0 ){
                prev = curr;
                curr = curr->next;
            }
            if (curr != NULL){
               // printf("%s-%s, Match!!!!!\n", target, curr->word);
                curr->count ++;
            }
            else{
                //printf("%s-%s, Not Match!!!!!\n", target, curr->word);
                curr = (struct node*)malloc(sizeof(struct node));
                curr->word = malloc(sizeof(char)*(strlen(target)+1));
                strcpy(curr->word,target);
                curr->count = 1;
                curr->next = NULL;
                prev->next = curr;
            }
        }
        // Continue to the next word
        free(target);
        target = NULL;
        start = i + 1;
        while (isspace(word_array[start])){
            start ++;
        }
        i = start;
        hashval = 0;
    }
    return wc;
}

void wc_output(struct wc* wc){
    int i;
    for (i = 0; i < hashsize; i++){
        if (wc[i].elem != NULL){
            struct node* curr = wc[i].elem;
            while (curr != NULL){
                printf("%s:%d\n",curr->word,curr->count);
                curr = curr->next;
            }
        }
    }
}
void wc_destroy(struct wc* wc){
    int i;
    for (i = 0; i < hashsize; i++){
        if (wc[i].elem != NULL){
            struct node* curr = wc[i].elem;
            struct node* temp;
            while (curr != NULL){
                temp = curr;
                curr = curr->next;
                //temp->next = NULL;
                free(temp->word);
                free(temp);
            }
        }
        //free(wc[i].elem);
    }
    
    free(wc);
}
/*
struct wc* wc_init(char *word_array, long size){
    struct wc* wc = malloc(sizeof(struct wc));
    long hashsize = size;
    char* target;
    int i;
    
    // Initialize the hash table and allocate memory to each node
    for (i = 0; i < hashsize; i++){
        wc->elem[i] = malloc(sizeof(node));
        wc->elem[i]->word = NULL;
    }
    
    // Input elements
    target = strtok(word_array," ");
    while (target != NULL){
        long hashval, pos = 0;
        // Hash function
        while (*target){
            hashval = (*target++) + (hashval<<6) + (hashval<<16) - hashval;
        }
        pos = hashval & 0x7FFFFFFF;
        // Input the element into table
        while (wc->elem[pos]->word != NULL){
            if (strcmp(wc->elem[pos]->word,target)){
                wc->elem[pos]->count ++;
            }
            else{
                pos = (pos + 1) % hashsize;
            }
        }
        // Create a new element
        if (wc->elem[pos]->word == NULL){
            wc->elem[pos]->word = malloc(sizeof(char)*(strlen(target)+1));
            strcpy(wc->elem[pos]->word, target);
            wc->elem[pos]->count = 1;
        }
        
        target = strtok(NULL, " ");
    }
    
    return wc;
}
*/
/*
void wc_output(struct wc* wc){
    int i;
    for (i = 0; i < wc->size; i++){
        printf("%s:%d\n",wc->elem[i]->word,wc->elem[i]->count);
    }
    
}

void wc_destroy(struct wc* wc){
    int i;
    for (i = 0; i < wc->size; i++){
        if (wc->elem[i]->word != NULL){
            free(wc->elem[i]->word);
            wc->elem[i]->word = NULL;
        }
        free(wc->elem[i]);
    }
    free(wc);
}

*/