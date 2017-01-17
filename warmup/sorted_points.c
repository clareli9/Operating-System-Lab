#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//#include "common.h"
#include "point.h"
#include "sorted_points.h"

struct sorted_points {
  struct point *pt;
  struct sorted_points *next;
  
};

struct sorted_points * sp_init()
{
	struct sorted_points *sp;

	sp = (struct sorted_points *)malloc(sizeof(struct sorted_points));
    //sp->pt = NULL;
    //sp->next = NULL;
	//assert(sp);

	//TBD();
    sp->pt = malloc(sizeof(struct point));
    sp->pt = point_set(sp->pt,0,0);
    sp->next = NULL;
	return sp;
}

void sp_destroy(struct sorted_points *sp)
{
   struct sorted_points *temp;
   temp = sp;
   while (temp!=NULL){
      sp = temp->next;
       free(temp->pt);
      free(temp);
      temp = sp;
   }
}

int sp_add_point(struct sorted_points *sp, double x, double y){
    /*
    if (sp->pt == NULL){
        //sp = malloc(sizeof(struct sorted_points));
        sp->pt = malloc(sizeof(struct point));
        sp->pt = point_set(sp->pt,x,y);
        return 1;
    }*/
    if (sp->next == NULL){
        sp->next = malloc(sizeof(struct sorted_points));
        sp->next->pt = malloc(sizeof(struct point));
        sp->next->next = NULL;
        sp->next->pt = point_set(sp->next->pt,x,y);
        return 1;
    }
    
    struct sorted_points *newNode;
    newNode = malloc(sizeof(struct sorted_points));
    newNode->pt = malloc(sizeof(struct point));
    newNode->pt = point_set(newNode->pt,x,y);
    newNode->next = NULL;
    /*
    //see if new node should be placed before head
    if (point_compare(newNode->pt,sp->pt) < 0){
        newNode->next = sp;
        return 1;
    }
    if (point_compare(newNode->pt,sp->pt) == 0){
        if (newNode->pt->x < sp->pt->x){
            newNode->next = sp;
        }
        if (newNode->pt->x == sp->pt->x){
            if (newNode->pt->y <= sp->pt->x){
                newNode->next = sp;

            }
        }
        return 1;
    }*/
    /*search through to find the correct spot and insert the node*/
    struct sorted_points *curr = sp;
    while (curr->next != NULL){
        if (point_compare(newNode->pt,curr->next->pt) > 0){
            curr = curr->next;
        }
        else{
            break;
        }
    }
    //check the situation
    if (curr->next == NULL || point_compare(newNode->pt,curr->next->pt) < 0){
        newNode->next = curr->next;
        curr->next = newNode;
        return 1;
    }
    if (point_compare(newNode->pt,curr->next->pt) == 0){
        if (newNode->pt->x < curr->next->pt->x){
            newNode->next = curr->next;
            curr->next = newNode;
        }
        else if (newNode->pt->x > curr->next->pt->x){
            newNode->next = curr->next->next;
            curr->next->next = newNode;
        }
        else{
            if (newNode->pt->y < curr->next->pt->y){
                newNode->next = curr->next;
                curr->next = newNode;
            }
            else{
                newNode->next = curr->next->next;
                curr->next->next = newNode;
            }
        }
        return 1;
    }
    else {
        return 1;
    }
}

int sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret){
    if (sp == NULL || index < 0 || sp->next == NULL){
        return 0;
    }
    /*
    //see if delete the head or not
    if (index == 0){
        struct sorted_points *temp = sp;
        sp = (sp)->next;
        ret = point_set(ret,temp->pt->x,temp->pt->y);
        if (temp->pt != NULL){
            free(temp->pt);
        }
        free(temp);
        return 1;
    }*/
    index = index + 1;
    /*search through to find correct spot and delete it*/
    int i = 0;
    struct sorted_points *curr = sp;
    struct sorted_points *prev = sp;
    while (i < index){
        if (curr->next == NULL){
            break;
        }
        prev = curr;
        curr = curr->next;
        i ++;
    }
    //list is too short
    if (i < index){
        return 0;
    }
    ret = point_set(ret,curr->pt->x,curr->pt->y);
    prev->next = curr->next;
    curr->next = NULL;
    if (curr->pt != NULL){
        free(curr->pt);
    }
    free(curr);
    return 1;
}

void sp_printout(struct sorted_points *sp){
    if (sp->pt == NULL){
        return;
    }
    struct sorted_points *curr = sp;
    while (curr != NULL){
        printf("%.1lf %.1lf;",curr->pt->x,curr->pt->y);
        curr = curr->next;
    }
}

int sp_remove_first(struct sorted_points *sp, struct point *ret){
    if (sp == NULL || sp->next == NULL){
        return 0;
    }
    struct sorted_points *curr = sp->next;
    ret = point_set(ret,curr->pt->x,curr->pt->y);
    sp->next = curr->next;
    curr->next = NULL;
    free(curr->pt);
    free(curr);
    return 1;
}

int sp_remove_last(struct sorted_points *sp, struct point *ret){
    if (sp == NULL || sp->next == NULL){
        return 0;
    }
    struct sorted_points *curr = sp;
    struct sorted_points *prev = NULL;
    while (curr -> next != NULL){
        prev = curr;
        curr = curr -> next;
    }
    ret = point_set(ret,curr->pt->x,curr->pt->y);
    prev->next = NULL;
    curr->next = NULL;
    if (curr->pt != NULL){
        free(curr->pt);
    }
    free(curr);
    return 1;
}


/*
int sp_add_point(struct sorted_points *sp, double x, double y){
    struct sorted_points *elem, *temp, *prev;
    if (sp == NULL){
        sp = malloc(sizeof(struct sorted_points));
        sp->pt = malloc(sizeof(struct point));
        sp->pt = point_set(sp->pt,x,y);
    }
    else{
        elem = malloc(sizeof(struct sorted_points));
        elem->pt = malloc(sizeof(struct point));
        elem->pt = point_set(elem->pt,x,y);
        temp = sp -> next;
        prev = sp;
        while (temp != NULL && point_compare(temp->pt,elem->pt) < 0){
            prev = temp;
            temp = temp -> next;
        }
        if (temp != NULL){
        if (point_compare(temp->pt,elem->pt) > 0){
            elem -> next = temp;
            prev -> next = elem;
        }
        else if (point_compare(temp->pt,elem->pt) == 0){
            if (temp->pt->x > elem->pt->x){
                elem -> next = temp;
                prev -> next = elem;
            }
            else if (temp->pt->x < elem->pt->x){
                prev = temp -> next;
                elem -> next = prev;
                temp -> next = elem;
            }
            else{
                if (temp->pt->y > elem->pt->y){
                    elem -> next = temp;
                    prev -> next = elem;
                }
                else{
                    prev = temp -> next;
                    elem -> next = prev;
                    temp -> next = elem;
                }
            }
        }
        }
        else {
            prev -> next = elem;
            elem -> next = NULL;
        }
    }
    
    return 1;
}
*/
/*
int sp_remove_first(struct sorted_points *sp, struct point *ret){
    struct sorted_points *temp;
    if (sp == NULL){
        return 0;
    }
    else{
        if (sp->pt != NULL){
        temp = sp;
        sp = sp -> next;
        ret = point_set(ret,temp->pt->x,temp->pt->y);
        free(temp->pt);
        free(temp);
        return 1;
        }
        else{
            //The reason?
            //free(sp);
            ret = NULL;
            return 0;
        }
    }
}*/
/*
int sp_remove_last(struct sorted_points *sp, struct point *ret){
    struct sorted_points *temp, *prev;
    if (sp == NULL || sp -> next == NULL){
        return 0;
    }
    else {
        temp = sp -> next;
        prev = sp;
        while (temp -> next != NULL){
            prev = temp;
            temp = temp -> next;
        }
        ret = point_set(ret,temp->pt->x,temp->pt->y);
        prev -> next = NULL;
        free(temp->pt);
        free(temp);
        return 1;
    }
}*/

/*
int sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret)
{
    int i = 0;
    struct sorted_points *temp, *prev;
    prev = sp;
    temp = prev;
    while (temp != NULL && i != index){
        prev = temp;
        temp = temp -> next;
        i ++;
    }
    if (i != index || temp == NULL){
        ret = NULL;
        return 0;
    }
    else{
        if (temp->pt != NULL){
        ret = point_set(ret,temp->pt->x,temp->pt->y);
        struct sorted_points *anotherTemp;
        anotherTemp = temp -> next;
        prev -> next = anotherTemp;
        free(temp->pt);
        free(temp);
        return 1;
        }
        else{
            free(temp);
            ret = NULL;
            return 0;
        }
    }
}*/

int sp_delete_duplicates(struct sorted_points *sp){
    if (sp == NULL || sp->next == NULL){
        return 0;
    }
    int count = 0;
    struct sorted_points *curr,*prev,*target;
    target = sp->next;
    while (target != NULL){
        curr = target->next;
        prev = target;
        while (curr != NULL){
            if (curr->pt->x == target->pt->x && curr->pt->y == target->pt->y){
                prev->next = curr->next;
                curr->next = NULL;
                free(curr->pt);
                free(curr);
                curr = prev->next;
                count ++;
            }
            else{
                prev = curr;
                curr = curr->next;
            }
        }
        target = target->next;
    }
    return count;
}


