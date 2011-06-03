#ifndef _READYLIST_H_
#define _READYLIST_H_
#include "ULT.h"


typedef struct readyListStruct {
   int size; //size of readyList
   ThrdCtlBlk *head; //The first element in the readyList
   ThrdCtlBlk *runningThread; //The currently running thread
} readyList;


readyList *rl_init(readyList *rl);
void addTCB(readyList *rl, ThrdCtlBlk *TCBtoAdd);
ThrdCtlBlk* findTCB(readyList *rl, Tid target);


#endif