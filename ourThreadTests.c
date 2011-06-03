#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "ULT.h"
#include "ourThreadTests.h"
#include <time.h>

int main(){
  

  int ret;
  int ret2;
  ret2 = 0;

  printf("Now lets test some yields. Create 3 more threads, then let them run and clean themselves up:\n");
  ret = ULT_CreateThread((void (*)(void *))fact, (void*) 4);
  assert(ULT_isOKRet(ret));
  ret = ULT_CreateThread((void (*)(void *))fact, (void*) 4);
  assert(ULT_isOKRet(ret));
  ret = ULT_CreateThread((void (*)(void *))fact, (void*) 4);
  assert(ULT_isOKRet(ret));
  ULT_Yield(ULT_ANY); //let functions run and delete themselves
  printf("PASS\n");
  
  printf("Create 6 threads, triggering the creation of the current thread's TCB into the running list, then killing them.\n");
  ULT_Yield(ULT_ANY);
  ret = ULT_CreateThread((void (*)(void *))hello, "I");
  assert(ULT_isOKRet(ret));
  ret = ULT_CreateThread((void (*)(void *))fact, (void*) 4);
  assert(ULT_isOKRet(ret));
  ret = ULT_CreateThread((void (*)(void *))hello, "I");
  assert(ULT_isOKRet(ret));
  ret = ULT_CreateThread((void (*)(void *))printSquare,(void*)8);
  assert(ULT_isOKRet(ret));
  ret = ULT_CreateThread((void (*)(void *))hello, "dead, ");
  assert(ULT_isOKRet(ret));
  ret = ULT_CreateThread((void (*)(void *))hello, "****!");
  assert(ULT_isOKRet(ret));
  printf("Finished creating thread\n");
  printf("PASS\n");
  
  printf("Test calling yield with invalid tid\n");
  ret = ULT_Yield(234);
  assert(ret == ULT_INVALID);
  printf("PASS\n");
  

  printf("Test Destroying all elements in list, then 1 Invalid Destroy!\n");  
  ret = ULT_DestroyThread(ULT_ANY);
  assert(ULT_isOKRet(ret));
  ret = ULT_DestroyThread(ULT_ANY);
  assert(ULT_isOKRet(ret));
  ret = ULT_DestroyThread(ULT_ANY);
  assert(ULT_isOKRet(ret));
  ret = ULT_DestroyThread(ULT_ANY);
  assert(ULT_isOKRet(ret));
  ret = ULT_DestroyThread(ULT_ANY);
  assert(ULT_isOKRet(ret));
  ret = ULT_DestroyThread(ULT_ANY);
  assert(ULT_isOKRet(ret));
  ret = ULT_DestroyThread(ULT_ANY);
  assert(ret == ULT_NONE);
  printf("PASS\n");



  printf("Test yielding to self when ready list is empty\n");
  ret = ULT_Yield(ULT_ANY);
  assert(ret == ULT_NONE);
  printf("PASS\n");
  
  printf("Test that attempting to destroy thread with invalid tid returns ULT_INVALID\n");
  ret = ULT_DestroyThread(34); //attempt to destroy invald TID, should return ULT_INVALID
  assert(ret == ULT_INVALID);
  printf("PASS\n");
  
  printf("Now, destroy running thread: \n");
  ULT_DestroyThread(ULT_SELF); //destroy running thread and make one on list new running thread
  printf("Done destroying. Now no elements in ready list but there is a runningThread \n"); 
  printf("THAT WAS OUR GRAND FINALE\n");
  
  printf("Test Destroying thread when ready list is empty\n");
  ret = ULT_DestroyThread(ULT_ANY);//attempt to destroy thread when none on list, should return ULT_NONE
  assert(ret == ULT_NONE);
  printf("PASS\n");
  
  printf("\nPASSED ALL TESTS!\n\n");
  return 0;
}

static int fact(int n)
{
  if(n == 1){
    return 1;
   }
   return n*fact(n-1);
 }

static void printSquare(int n)
{
  printf("%d^2 = %d\n",n,n*n);
}

static void hello(char *msg)
{
  Tid ret;

  printf("Made it to hello() in called thread\n");
  printf("Message: %s\n", msg);
  ret = ULT_Yield(ULT_SELF);
  assert(ULT_isOKRet(ret));
  printf("Thread returns from first yield\n");

  ret = ULT_Yield(ULT_SELF);
  assert(ULT_isOKRet(ret));
  printf("Thread returns from second yield\n");

  while(1){
    ULT_Yield(ULT_ANY);
  }
  
}
