#include <assert.h>
#include <stdio.h>
#include "interrupt.h"
#include "basicThreadTests.h"
#include "ULT.h"




int 
main(int argc, char **argv)
{
  basicThreadTests();
  grandFinale();

  printf("\n\n*** BUG **** Should not get here\n\n");
  assert(0);
  //printf("passed\n");
  return 0;
}



