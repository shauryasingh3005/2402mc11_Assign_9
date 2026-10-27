#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
extern char end[];
#define NSEMS 10

struct semaphore {
  struct spinlock lk;
  int value;
  int initialized;
};

static struct semaphore sems[NSEMS];
static int sem_locks_ready = 0;

uint64
sys_sem_init(void)
{
  int id, value;

  argint(0, &id);
  argint(1, &value);

  if (id < 0 || id >= NSEMS || value < 0)
    return -1;

  // Initialize all locks once, before worker threads start.
  if (!sem_locks_ready) {
    for (int i = 0; i < NSEMS; i++) {
      initlock(&sems[i].lk, "semaphore");
      sems[i].value = 0;
      sems[i].initialized = 0;
    }
    sem_locks_ready = 1;
  }

  acquire(&sems[id].lk);
  sems[id].value = value;
  sems[id].initialized = 1;
  release(&sems[id].lk);

  return 0;
}

uint64
sys_sem_wait(void)
{
  int id;
  argint(0, &id);

  if (id < 0 || id >= NSEMS || !sem_locks_ready)
    return -1;

  struct semaphore *s = &sems[id];
  acquire(&s->lk);

  if (!s->initialized) {
    release(&s->lk);
    return -1;
  }

  while (s->value == 0) {
    sleep_prepare(s);
    release(&s->lk);
    sleep();
    acquire(&s->lk);

    if (killed(myproc())) {
      release(&s->lk);
      return -1;
    }
  }

  s->value--;
  release(&s->lk);
  return 0;
}

uint64
sys_sem_signal(void)
{
  int id;
  argint(0, &id);

  if (id < 0 || id >= NSEMS || !sem_locks_ready)
    return -1;

  struct semaphore *s = &sems[id];
  acquire(&s->lk);

  if (!s->initialized) {
    release(&s->lk);
    return -1;
  }

  s->value++;
  wakeup(s);
  release(&s->lk);

  return 0;
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  int n;
  uint64 addr;
  struct proc *p = myproc();

  argint(0, &n);
  addr = p->sz;

  if(n > 0){
    // Reserve virtual address space only.
    if(addr + n < addr || addr + n >= MAXVA)
      return -1;

    p->sz += n;
  } else if(n < 0){
    // Shrink and free any pages that were actually allocated.
    uint64 new_sz = p->sz + n;

    if(new_sz > p->sz)
      return -1;

    p->sz = uvmdealloc(p->pagetable, p->sz, new_sz);
  }

  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_clone(void)
{
  uint64 fcn, arg, stack;

  argaddr(0, &fcn);
  argaddr(1, &arg);
  argaddr(2, &stack);

  return kclone(fcn, arg, stack);
}

uint64
sys_join(void)
{
  return kjoin();
}

uint64
sys_set_priority(void)
{
  int pid, priority;

  argint(0, &pid);
  argint(1, &priority);

  return set_priority(pid, priority);
}

uint64
sys_get_priority(void)
{
  int pid;

  argint(0, &pid);

  return get_priority(pid);
}

uint64
sys_va2pa(void)
{
  uint64 va;
  uint64 pa;
  pte_t *pte;
  struct proc *p = myproc();

  argaddr(0, &va);

  // Reject addresses outside the process's address space.
  if(va >= p->sz)
    return -1;

  pte = walk(p->pagetable, va, 0);

  // Page must exist, be valid, and be accessible from user mode.
  if(pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_U) == 0)
    return -1;

  pa = PTE2PA(*pte) + (va % PGSIZE);
  
  return pa;
}

uint64
sys_getmeminfo(void)
{
  uint64 addr;
  struct proc *p = myproc();
  struct meminfo mi;

  argaddr(0, &addr);

  mi.total_pages = (PHYSTOP - PGROUNDUP((uint64)end)) / PGSIZE;
  mi.free_pages = get_free_pages();
  mi.process_pages = (p->sz + PGSIZE - 1) / PGSIZE;
  mi.page_table_pages = countpagetablepages(p->pagetable);

  if(copyout(p->pagetable, p->sz, addr,
             (char *)&mi, sizeof(mi)) < 0)
    return -1;

  return 0;
}

uint64
sys_vmprint(void)
{
  struct proc *p = myproc();
  vmprint(p->pagetable);
  return 0;
}