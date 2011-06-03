#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>
#include "readyList.h"

Tid firstCall = 0; 
readyList *rl;
int available_Tids[ULT_MAX_THREADS]; //ULT_MAX_THREADS vacant slots for Tids (0 = available, 1 = occupied)
//Tid firstDestroy = 0;

void listInit() //function for initializing the ReadyList
{
  initializeRL(); //Goes into readyList.c and does the actual initialization
  available_Tids[0] = 1;
  rl->runningThread->tid = 0;
  firstCall++;
}

void stub(void (*fn)(void *), void *arg)
{
  Tid ret;
 // //printf("IN STUB\n");
  (*fn)(arg); // call root function
 // //printf("IN STUB, CALLED FN SUCCESSFULLY\n");
  ret = ULT_DestroyThread(ULT_SELF);
  assert(ret == ULT_NONE); // we should only get here if we are the last thread.
  free(rl->runningThread->stack_orig); //Free all memory for the readyList before final termination
  free(rl->runningThread);
  free(rl);
  exit(0); // all threads are done, so process should exit
}

Tid ULT_CreateThread(void (*fn)(void *), void *parg)
{
  int setTid; //When we find an available tid, we'll set the new thread's tid to this

  /*
  If this is the first call (i.e. firstCall == 0),
  then we will call the listInit
  which will initialize the readyList
  */
  
  if(!firstCall)
  {
    listInit();
  }
  killZombie();
  ThrdCtlBlk *new_tcb; //create a new control block
  new_tcb = malloc(sizeof(ThrdCtlBlk)); //allocate memory for it
  
  //If unable to malloc, then ULT_NOMEMORY;
  if(!new_tcb)
  {
    return ULT_NOMEMORY; //OUT OF MEMORY!
  }
  
  //look for available tid (i.e. Check in the available_Tids array for an i such that available_Tids[i] == 0)
  int i = 0;
  while (i<ULT_MAX_THREADS)
  {
    if (available_Tids[i] == 0) //F*** YEAH! We found one!
    {
      setTid = i;
      break;
    }
    else
    {
      i++;
    }
  }
  
  if(i == ULT_MAX_THREADS) //Indicates that we cannot add in another thread because we've max out on the # of threads.
  {
      free(new_tcb); //free back the memory
      return ULT_NOMORE;
  }
  
  //tid no longer available
  available_Tids[i] = 1;
  
  int did_it_work = getcontext(&new_tcb->uc); //save the current context into the new Thread's TCB
  assert(did_it_work == 0); //Yes...did it work?
  new_tcb->zombie = 0; 
  new_tcb->tid = setTid;
  new_tcb->sp = malloc(ULT_MIN_STACK); //allocate memory for the stack.
  if (!new_tcb->sp)
  {
    free(new_tcb); //Since we can't allocate for the stack, we must free new_tcb
    return ULT_NOMEMORY;
  }
  new_tcb->stack_orig = new_tcb->sp; //original stack pointer used in order to free.
  new_tcb->sp += (ULT_MIN_STACK/4); //Set the stack pointer to the top of the stack
  (new_tcb->sp)--; //Decrement stack pointer
  *(new_tcb->sp) = (unsigned int) parg; //push the address of argument
  (new_tcb->sp)--;
  *(new_tcb->sp) = (unsigned int) fn; //push the address of the function
  new_tcb->uc.uc_mcontext.gregs[REG_EIP] = (unsigned int)stub; //We'll set the program counter to the address of stub
  new_tcb->uc.uc_mcontext.gregs[REG_ESP] = (unsigned int)(--new_tcb->sp); //Set the stack pointer register to our stack pointer
  addTCB(rl,new_tcb); //Add Thread to the readyList
  
  return new_tcb->tid;
}

Tid ULT_Yield(Tid wantTid)
{   
    
    volatile int doneThat = 0; //Indicates that the context save and loading is complete.
    Tid retTid;
    if(!firstCall) //Initialize readyList, like we do with Create()
    {
      listInit();
    }
    killZombie(); //Any zombie found will be free'd
    
    if(wantTid < -2 || wantTid >= ULT_MAX_THREADS) //Query out of bounds
    {
      return ULT_INVALID;
    }
    
    if(rl == NULL) //There's nothing in here?! Looks like it
    {
      return ULT_NONE;
    }
    
    if(available_Tids[wantTid] == 0 && (wantTid != ULT_SELF && wantTid != ULT_ANY)) //Probably inquiring a thread with a not-yet-made tid.
    {
      return ULT_INVALID;
    }
    
    if(rl->size == 0 && (wantTid != ULT_SELF)) //empty readyList and not ULT_SELF
    {
      if(rl->runningThread->tid == wantTid)
      {
	  /*Huh. Looks like we got lucky. 
	    This essentially a successful Yield(ULT_SELF), 
	    and return the currently running Tid.*/
	    return rl->runningThread->tid;
      }
      else
      {
	return ULT_NONE;
      }
    }
        
    //If calling yield on yourself 
    if (wantTid == ULT_SELF || rl->runningThread->tid == wantTid)
    {
        return rl->runningThread->tid; //return the running Thread's tid
    }

    if(wantTid == ULT_ANY)
    {
      ThrdCtlBlk *tmp = rl->head;
      if (!tmp)
      { //if empty list
        return ULT_NONE;
      }
      
      while(tmp) //Traverse through non-empty list to search for a non-zombie thread
      { //not an empty list    
	   if (tmp->zombie)
	   {
	        tmp = tmp->next;
	   }
	   wantTid = tmp->tid;
	   break;
     }
     
     if(!tmp) //All of the threads in the list are unusable?!!?!!?!
     {
	    return ULT_NONE;
     }
   }
    
    ThrdCtlBlk *TCB_To_RunThread = findTCB(rl,wantTid); //Get the TCB to yield to in the readyList.

    if(!TCB_To_RunThread)
    {
      return ULT_INVALID; //TCB not found
    }

    int ret = getcontext(&(rl->runningThread->uc)); //save thread state (to TCB)
    assert(!ret);
    
    if(!doneThat)
    {
        doneThat = 1;
	ThrdCtlBlk *RT_To_List = rl->runningThread; //Does a swap with the runningThread and the queried Thread
    
    
    if(!TCB_To_RunThread) //Wasn't sure whether to delete this or not, but I think not because of context reasons.
    {
      return ULT_INVALID; //The thread with the queried tid is likely dead or just not there.
    }
    
    if(TCB_To_RunThread->prev == NULL && TCB_To_RunThread->next == NULL) //Case 0: TCB_To_RunThread is a lone element (head) in the readylist
    {
      //Simple pointer manipulation
      rl->runningThread = TCB_To_RunThread;
      rl->head = RT_To_List;
      RT_To_List->prev = NULL; //It's probably already true, but we're want to be absolutely sure.
      RT_To_List->next = NULL;
    }
    else if(TCB_To_RunThread->prev == NULL && TCB_To_RunThread->next != NULL) //Case 1: switch out the head
    {
      addTCB(rl,RT_To_List);
      rl->size -= 1; //stablize the size.
      rl->head = TCB_To_RunThread->next;
      TCB_To_RunThread->next->prev = NULL;
      TCB_To_RunThread->next = NULL;
    }
    else if(TCB_To_RunThread->prev != NULL && TCB_To_RunThread->next != NULL) //Case 2: swtich out element in Middle of the List
    {
      addTCB(rl,RT_To_List);
      rl->size -= 1; //stablize the size.
      TCB_To_RunThread->prev->next = TCB_To_RunThread->next;
      TCB_To_RunThread->next->prev = TCB_To_RunThread->prev;
      TCB_To_RunThread->prev = NULL;
      TCB_To_RunThread->next = NULL;
    }
    else if(TCB_To_RunThread->prev != NULL && TCB_To_RunThread->next == NULL)//Case 3: switch out the tail
    {
      TCB_To_RunThread->prev->next = RT_To_List;
      RT_To_List->prev = TCB_To_RunThread->prev;
      TCB_To_RunThread->prev = NULL;
    }

    rl->runningThread = TCB_To_RunThread; //set the runningThread to the queried thread.

    retTid = rl->runningThread->tid;
    setcontext(&(rl->runningThread->uc)); //load its state (from TCB)
    //when we do this, if there is a fn in the thread, it runs NOW
    
    }
    return retTid;
}

Tid killZombie() //Finds a zombie thread and free's any data structures associated with it. Tid of killed zombie on success, ULT_NONE on failure.
{

	ThrdCtlBlk *iter = rl->head;
	Tid rettid;
	while(iter) //Iterate through the readyList to find a zombie
	{
	  if (!iter->zombie) //Keep going through the readyList until we find one or we reach the end
	  {
	    iter = iter->next;
	  }
	  else
	  {
	    break;
	  }
	}
	if (!iter) //No zombies found. 
	{
	  return ULT_NONE;
	}
	else
	{
	  //Remove the zombie from the readyList in preparation for it's..."freeing".
	  //Pointer manipulations done in the following if/else if statements.
	  if(iter->prev == NULL && iter->next == NULL) //Case 0: Sole element in list.
	  {
	    rl->head = NULL;
	  }
	  else if(iter->prev == NULL && iter->next != NULL) //Case 1: It's the head!
	  {
	    rl->head = iter->next;
	    rl->head->prev = NULL;
	    iter->next = NULL;
	  }
	  else if(iter->prev != NULL && iter->next != NULL) //Case 2: It's between the head and the tail
	  {
	    iter->next->prev = iter->prev;
	    iter->prev->next = iter->next;
	    iter->prev = NULL;
	    iter->next = NULL;
	  }
	  else if(iter->prev != NULL && iter->next == NULL) //Case 3: It's a tail!
	  {
	    iter->prev->next = NULL;
	    iter->prev = NULL;
	  }
	  rl->size -= 1; //Decrement size in response to removing the zombie.
	  rettid = iter->tid;
	  available_Tids[rettid] = 0;
	  free(iter->stack_orig);
	  free(iter);
	  return rettid;
	}
}
    
Tid ULT_DestroyThread(Tid tid) //Destroy policy for any. Kill the head of the readyList.
{
    
    if(!firstCall) //Initialize List like with Create and Yield.
    {
      listInit();
      return ULT_NONE; //Obviously, nothings in the readyList.
    }
    if (rl->size == 0 && tid == ULT_ANY) //
    {
      return ULT_NONE;
    }
    if(tid == ULT_SELF || rl->runningThread->tid == tid) 
    {    
      
      if(rl->size == 0) //Base Case: There are no threads in the ready list.
      {
	   return ULT_NONE; //We won't kill the last remaining thread yet. We'll let the stub do it.
      }
      else //Default case: The size of the list is > 0
      {
	rl->runningThread->zombie = 1; //I GET ZOMBIFIED! IT'S ALL THE SAME YOU SAY!
	ULT_Yield(ULT_ANY); //"IT'S A TRAP!" - Zombie
      }
    }
    
    Tid rettid;
    ThrdCtlBlk *iter = rl->head;
    if(tid == ULT_ANY)
    {
      rettid = killZombie(); //Kill the zombie thread, then destroy the head thread like in the policy.
    }
    else //Default case
    {
	while (iter)
	{ //while temp is not null and zombie = 0, keep going until find zombie
	    if (iter->tid != tid){
	      iter = iter->next;}
	    else
	    {
	      break;
	    }
	}
	//either exited because found a zombie or because at end of list 
	
	if(iter == NULL)
	{
	  //didn't find a zombie.
	  return ULT_INVALID;
	}
    }
    rettid = iter->tid;
    
    //Pull off the Thread from the readyList and free it and its associated data.
    if(iter->prev == NULL && iter->next == NULL) //Sole element in list.
    {
      rl->head = NULL;
    }
    else if(iter->prev == NULL && iter->next != NULL) //It's the head!
    {
      rl->head = iter->next;
      rl->head->prev = NULL;
      iter->next = NULL;
    }
    else if(iter->prev != NULL && iter->next != NULL) //It's between the head and the tail
    {
      iter->next->prev = iter->prev;
      iter->prev->next = iter->next;
      iter->prev = NULL;
      iter->next = NULL;
    }
    else if(iter->prev != NULL && iter->next == NULL) //It's a tail!
    {
      iter->prev->next = NULL;
      iter->prev = NULL;
    }
    rl->size -= 1; //Decrement List
    available_Tids[rettid] = 0; //Tid now vacant
    free(iter->stack_orig); //FREE STACK! No purchase necessary.
    free(iter); //Free the Thread
    return rettid;
}

void initializeRL()
{
  rl = malloc(sizeof(readyList)); //Allocate memory for readyList
  rl_init(rl); //Initialize said readyList
}