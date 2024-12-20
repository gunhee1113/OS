#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
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

/* Do not touch sys_time() */
uint64 
sys_time(void)
{
  uint64 x;

  asm volatile("rdtime %0" : "=r" (x));
  return x;
}
/* Do not touch sys_time() */

extern struct proc proc[NPROC];

uint64
sys_sched_setattr(void)
{
  struct proc *p = myproc();
  int pid, runtime, period;
  argint(0, &pid);
  argint(1, &runtime);
  argint(2, &period);
  if(pid == 0) pid = p->pid;
  if(runtime>period || runtime<0 || period<0) return -1;
  if(runtime!=0 && period!=0 && runtime==period) return -1;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->pid && p->pid == pid) {
      p->runtime = runtime;
      p->period = period;
      acquire(&tickslock);
      p->deadline = ticks + p->period;
      release(&tickslock);
      release(&p->lock);
      break;
    }
    release(&p->lock);
  }

  return 0;
}

uint64
sys_sched_yield(void)
{
  struct proc *p = myproc();
  if(p->runtime == 0 && p->period == 0) yield();
  else{
    acquire(&p->lock);
    p->state = SLEEPING;
    sched();
    release(&p->lock);
  }
  return 0;
}