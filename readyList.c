#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "readyList.h"
#include "ULT.h"

/*
 * Initialize data structures, returning pointer
 * to the object.
 */
readyList *rl_init(readyList *rl){ //Initialize the readyList
    
    rl->size = 0;
    rl->head = NULL;
    
    ThrdCtlBlk *runner = malloc(sizeof(ThrdCtlBlk)); //Next 4 lines will set the current running thread
    int ret = getcontext(&(runner->uc));
    assert(!ret);
    rl->runningThread = runner;
    return rl;
}

/*
 * Add a new TCB to the end of the ReadyList. 
 */
void addTCB(readyList *rl, ThrdCtlBlk *TCBtoAdd)
{
    if (rl->size == 0)
    { //Base Case.
	  rl->head = TCBtoAdd;
	  (rl->size)++;
	  return;
    }
    //Otherwise, append it at end of list  
    //Go to end of list:
    
    ThrdCtlBlk *temp = rl->head;
    while(temp->next != NULL)
    {
        temp = temp->next;
    }
    
    //insert TCB at end
    temp->next = TCBtoAdd;
    TCBtoAdd->prev = temp;
    TCBtoAdd->next = NULL;
    
    (rl->size)++; //increment size
}
/*
 * Find TCB based on given Thread ID and return TCB
 */
ThrdCtlBlk* findTCB(readyList *rl, Tid target){
    if(rl->runningThread->tid == target) //Base case I, look at the running thread.
    {
      return rl->runningThread;
    }
    if(rl->size == 0) //Base Case II, list is empty
    {
      return NULL;
    }
    ThrdCtlBlk *temp = rl->head; 
    ThrdCtlBlk *fail = NULL;
    //if first item in list
    if (temp->tid == target){ 
        return temp;
    }
    temp = temp->next; 
    //NOT first item in list
    while (temp != NULL){
        if (temp->tid == target)
	{
	    if(temp->zombie)
	    {
	      return NULL; //A TCB was found, but it was a zombie. DAMN!!!
	    }
            return temp; 
        }
        temp = temp->next;
    }
    
    //if can't find Tid in list, return 0
    return fail;
}



