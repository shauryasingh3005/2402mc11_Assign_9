
#include "kernel/types.h"
#include "user/user.h"

#define PGSIZE 4096

int
main(void)
{
  char *p;

  printf("Before sbrk:\n");
  vmprint();

  p = sbrk(4 * PGSIZE);

  if(p == (char *)-1){
    printf("sbrk failed\n");
    exit(1);
  }

  // Touch one byte in each newly allocated page.
  p[0] = 1;
  p[PGSIZE] = 2;
  p[2 * PGSIZE] = 3;
  p[3 * PGSIZE] = 4;

  printf("After sbrk and touching four pages:\n");
  vmprint();

  exit(0);
}
