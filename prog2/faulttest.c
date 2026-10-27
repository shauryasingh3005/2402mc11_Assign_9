
#include "kernel/types.h"
#include "user/user.h"

#define PGSIZE 4096

int
main(void)
{
  char *p;
  int before, after;
  int pid;

  p = sbrk(32 * PGSIZE);

  if(p == (char *)-1){
    printf("sbrk failed\n");
    exit(1);
  }

  before = faults();

  // Touch five distinct pages.
  for(int i = 0; i < 5; i++)
    p[i * 2 * PGSIZE] = i + 1;

  after = faults();

  printf("Faults before: %d\n", before);
  printf("Faults after:  %d\n", after);
  printf("New faults:    %d\n", after - before);

  if(after - before == 5)
    printf("PASS: five lazy page faults\n");
  else
    printf("FAIL: expected five new faults\n");

  // Check that an invalid access kills only the child.
  pid = fork();

  if(pid == 0){
    char *bad = p + 32 * PGSIZE;
    *bad = 1;
    printf("FAIL: invalid access survived\n");
    exit(1);
  }

  if(pid > 0){
    wait(0);
    printf("Parent survived invalid access test\n");
  }

  exit(0);
}
