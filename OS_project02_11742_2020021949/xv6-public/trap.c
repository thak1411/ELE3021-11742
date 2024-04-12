#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  struct proc* p;
  int nticks;
  nticks = 1;

  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      nticks = ticks;
      wakeup(&ticks);
      if (mycpu()->cpu_schedule_mode == 0 && nticks % 100 == 0) {
        priority_boosting();
      }
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  if (myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0 + IRQ_TIMER && nticks % 100 == 0 &&
     mycpu()->cpu_schedule_mode == 0) {
    yield();
  } else if (myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0 + IRQ_TIMER) {
    // Force process to give up CPU on clock tick.
    // If interrupts were on while locks held, would need to check nlock.
    p = myproc();
    p->time_quantum++;
    if (p->q_level != 99) { // If the process is monoplized then don't yield cpu
      if (p->time_quantum >= p->q_level * 2 + 2) { // If the process has used up its time quantum
        p->time_quantum = 0;

        if (p->q_level == 0) {
          if (p->pid & 1) { // make the process to be in the L1 queue
            p->q_level = 1;
          } else { // make the process to be in the L2 queue
            p->q_level = 2;
          }
        } else if (p->q_level < 3) { // If the process is in the L1 or L2 queue
          // make the process to be in the L3 queue
          p->q_level = 3;
        } else if (p->q_level == 3) { // If the process is in the L3 queue
          // sub process's priority by 1
          p->priority--;
          // priority can't be negative
          if (p->priority < 0) p->priority = 0;
        }
        yield();
      }
    }
  }
    

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
