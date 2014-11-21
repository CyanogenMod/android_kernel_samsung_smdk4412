#ifndef LKLIST_INC
#define LKLIST_INC

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef int position;

typedef struct node *nodeptr;
typedef struct node {
    void *data;
    nodeptr next;
    nodeptr prior;
} listnode;

typedef struct listrec *listptr;
typedef struct listrec {
   nodeptr head;
   nodeptr current;
   position elSize;
   position n;
   position icurrent;
} list;

position LKAdvanceList(listptr);
void     LKClearList(listptr);
int     LKCreateList(listptr*, position);
position LKCurrent(listptr);
void     LKDeleteFromList(listptr, position);
int      LKEmptyList(listptr);
void     LKGetFromList(listptr, position, void *);
position LKGotoInList(listptr, position);
int      LKInsertAfterInList(listptr, position, void *);
int      LKInsertBeforeInList(listptr, position, void *);
position LKSearchList(listptr, void *);
position LKLastInList(listptr);
void     LKPutInList(listptr, position, void *);
position LKReverseList(listptr);
void     LKZapList(listptr *);


#endif
/* LKLIST_INC */
