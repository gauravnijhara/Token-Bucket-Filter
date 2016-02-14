//
//  my402list.c
//  CS402WarmUp1
//
//  Created by Gaurav Nijhara on 1/23/16.
//  Copyright © 2016 Gaurav Nijhara. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "my402list.h"


#define TRUE 1
#define FALSE 0

My402ListElem *createNewNode(void *obj);

transaction* createTransaction()
{
    transaction *newObject = (transaction*)malloc(sizeof(transaction));
    newObject->type = (char*)malloc(sizeof(char)*2);
    newObject->timeStampDesc = (char*)malloc(sizeof(char)*20);
    newObject->amount = (char*)malloc(sizeof(char)*14);
    newObject->desc = (char*)malloc(sizeof(char)*30);
    
    return newObject;
}


int My402ListInit(My402List* list)
{
    if (list) {
        list->num_members = 0;
        list->anchor.next = NULL;
        list->anchor.prev = NULL;
        list->anchor.obj = NULL;
        return TRUE;
    }
    return FALSE;
}

int  My402ListLength(My402List* list)
{
    return list->num_members;
}


int  My402ListEmpty(My402List* list)
{
    return (list->num_members == 0);
}


int  My402ListAppend(My402List* list, void* obj)
{
    My402ListElem *newnode = createNewNode(obj);
    if(list && newnode)
    {
        My402ListElem *last = My402ListLast(list);
        if (!last) {
           newnode->prev = &list->anchor;
            list->anchor.next = newnode;
        }
        else {
            last->next = newnode;
            newnode->prev = last;
        }
        newnode->next = &list->anchor;
        list->anchor.prev = newnode;
        list->num_members++;
        return TRUE;
    }
    return FALSE;
}

int  My402ListPrepend(My402List* list, void* obj)
{
    My402ListElem *newnode = createNewNode(obj);
    if(list && newnode)
    {
        My402ListElem *first = My402ListFirst(list);
        if (!first) {
            newnode->next = &list->anchor;
            list->anchor.prev = newnode;
        }
        else {
            first->prev = newnode;
            newnode->next = first;
        }
        newnode->prev = &list->anchor;
        list->anchor.next = newnode;
        list->num_members++;
        return TRUE;
    }
    return FALSE;

}

void My402ListUnlink(My402List* list, My402ListElem* element)
{
    if (list && element) {
        element->prev->next = element->next;
        element->next->prev = element->prev;
        free(element);
        list->num_members--;
    }
}

void My402ListUnlinkAll(My402List* list)
{
    My402ListElem *iterator;
  for (iterator=My402ListFirst(list); iterator != NULL; iterator=My402ListNext(list, iterator)) {
       My402ListUnlink(list,iterator);     
   }
        
}


int  My402ListInsertAfter(My402List* list, void* obj, My402ListElem* element)
{
    My402ListElem *newnode = createNewNode(obj);
    if (list && newnode) {
        if (!element) {
            return My402ListAppend(list, obj);
        }
        else {
            newnode->next =  element->next;
            newnode->next->prev = newnode;
            newnode->prev = element;
            element->next = newnode;
            list->num_members++;
            return TRUE;
        }
    }
    return FALSE;
}

int  My402ListInsertBefore(My402List* list, void* obj, My402ListElem* element)
{
    My402ListElem *newnode = createNewNode(obj);
    if (list && newnode) {
        if (!element) {
            return My402ListPrepend(list, obj);
        }
        else {
            newnode->prev =  element->prev;
            newnode->prev->next = newnode;
            newnode->next = element;
            element->prev = newnode;
            list->num_members++;
            return TRUE;
        }
    }
    return FALSE;

}

My402ListElem *My402ListFirst(My402List* list)
{
    if (list) {
        return list->anchor.next;
    }
    return NULL;
}

My402ListElem *My402ListLast(My402List* list)
{
    if (list) {
        return list->anchor.prev;
    }
    return NULL;
}

My402ListElem *My402ListNext(My402List* list, My402ListElem* element)
{
    if (list && element && !(element->next == &list->anchor)) {
        return element->next;
    }
    return NULL;
}

My402ListElem *My402ListPrev(My402List* list, My402ListElem* element)
{
    if (list && element && !(element->prev == &list->anchor)) {
        return element->prev;
    }
    return NULL;
}

My402ListElem *My402ListFind(My402List* list, void* obj)
{
    My402ListElem *iterator = list->anchor.next;
    while (iterator != &list->anchor) {
        if (obj == iterator->obj) {
            return iterator;
        }
        iterator = iterator->next;
    }
    return NULL;
}

My402ListElem *createNewNode(void *obj)
{
    My402ListElem *node = (My402ListElem*)malloc(sizeof(My402ListElem));
    if (node) {
        node->obj = obj;
        node->next = NULL;
        node->prev = NULL;
        return node;
    }
    return NULL;
}

