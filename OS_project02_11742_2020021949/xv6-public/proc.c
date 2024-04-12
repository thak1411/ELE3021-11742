#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define USE_RN_CUSTOM_SCHEDULER

struct rn_queue {
  int size;
  int front;
  int tail;
  struct proc* proc[NPROC * 2];
};

static void rn_queue_insert(struct rn_queue* q, struct proc* p) {
  if (q->size == NPROC * 2) return;
  q->proc[q->tail] = p;
  q->tail = (q->tail + 1) % (NPROC * 2);
  q->size++;
}

static int rn_queue_is_empty(struct rn_queue* q) {
  return q->size == 0;
}

static struct proc* rn_queue_pop(struct rn_queue* q) {
  struct proc* ret;

  if (q->size == 0) return 0;
  
  ret = q->proc[q->front];
  q->front = (q->front + 1) % (NPROC * 2);
  q->size--;
  return ret;
}

static int rn_queue_size(struct rn_queue* q) {
  return q->size;
}

struct rn_heap {
  int size;
  struct proc* proc[NPROC * 2 + 1];
};

static void rn_heap_swap(struct rn_heap* h, int i, int j) {
  struct proc* tmp;

  tmp = h->proc[i];
  h->proc[i] = h->proc[j];
  h->proc[j] = tmp;
}

static void rn_heap_decrease(struct rn_heap* h, int i) {
  int max_i;

  for (; ;) {
    if ((i << 1) <= h->size && h->proc[i << 1]->priority > h->proc[i]->priority) {
      max_i = i << 1;
    } else max_i = i;

    if ((i << 1 | 1) <= h->size && h->proc[i << 1 | 1]->priority > h->proc[max_i]->priority) {
      max_i = i << 1 | 1;
    }
    if (max_i != i) {
      rn_heap_swap(h, i, max_i);
      i = max_i;
    } else break;
  }
}

static struct proc* rn_heap_pop(struct rn_heap* h) {
  struct proc* ret;

  if (h->size == 0) return 0;
  ret = h->proc[1];
  h->proc[1] = h->proc[h->size];
  h->size--;
  rn_heap_decrease(h, 1);
  return ret;
}

static void rn_heap_ify(struct rn_heap* h, int i) {
  int max_i;

  for (; i > 0; ) {
    if ((i << 1) <= h->size && h->proc[i << 1]->priority > h->proc[i]->priority) {
      max_i = i << 1;
    } else max_i = i;

    if ((i << 1 | 1) <= h->size && h->proc[i << 1 | 1]->priority > h->proc[max_i]->priority) {
      max_i = i << 1 | 1;
    }
    if (max_i != i) {
      rn_heap_swap(h, i, max_i);
      i >>= 1;
    } else break;
  }
}

static void rn_heap_insert(struct rn_heap* h, struct proc* p) {
  if (h->size == NPROC * 2) return;
  h->size++;
  h->proc[h->size] = p;
  rn_heap_ify(h, h->size >> 1);
}

static int rn_heap_is_empty(struct rn_heap* h) {
  return h->size == 0;
}

static int rn_heap_delete(struct rn_heap* h, struct proc* p) {
  int i, ret;

  ret = 0;
  for (i = 1; i <= h->size; ++i) {
    if (h->proc[i] == p) {
      h->proc[i] = h->proc[h->size];
      h->size--;
      rn_heap_decrease(h, i);
      ret = 1;
      break;
    }
  }

  return ret;
}

// static int rn_heap_size(struct rn_heap* h) {
//   return h->size;
// }


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  struct rn_queue l[3];
  struct rn_heap l3;
  struct rn_queue monopoly;
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  p->priority = 0;
  p->q_level = 0;
  rn_queue_insert(&ptable.l[0], p);

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  np->priority = 0;
  np->q_level = 0;
  rn_queue_insert(&ptable.l[0], np);

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

#ifdef USE_RN_CUSTOM_SCHEDULER
void
_deprecated_scheduler(void)
#else
void
scheduler(void)
#endif

{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
  }
}

static void scheduler_run_process(struct cpu* c, struct proc* p) {
  c->proc = p;
  switchuvm(p);
  p->state = RUNNING;
  p->time_quantum = 0;
  swtch(&(c->scheduler), p->context);
  switchkvm();
  c->proc = 0;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifdef USE_RN_CUSTOM_SCHEDULER
void
scheduler(void)
#else
void
_deprecated_scheduler(void)
#endif
{
  int i;
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for (; ;) {
    // Enable interrupts on this processor.
    sti();

    acquire(&ptable.lock);

    if (c->cpu_schedule_mode == 0) { // MLFQ
      for (i = 0; i < 3; ++i) {
        // process L_i (0 <= i <= 2)
        for (; !rn_queue_is_empty(&ptable.l[i]); ) {
          p = rn_queue_pop(&ptable.l[i]);

          if (p->state != RUNNABLE) continue;

          if (p->q_level != i) { // lazy propagate queue level 
            // if changed queue level is monoplized then skip
            if (p->q_level != 99) {
              if (p->q_level >= 0 && p->q_level <= 2) rn_queue_insert(&ptable.l[p->q_level], p);
              else rn_heap_insert(&ptable.l3, p);
            }
            // if queue level is changed then break and recheck from L0
            goto cont;
          }
          scheduler_run_process(c, p);

          if (p->state == RUNNABLE) {
            if (p->q_level >= 0 && p->q_level <= 2) rn_queue_insert(&ptable.l[p->q_level], p);
            else rn_heap_insert(&ptable.l3, p);
          }

          goto cont;
        }
      }
      // process L3
      for (; !rn_heap_is_empty(&ptable.l3); ) {
        p = rn_heap_pop(&ptable.l3);

        if (p->state != RUNNABLE) continue;

        if (p->q_level != 3) { // lazy propagate queue level
          // if changed queue level is monoplized then skip
          if (p->q_level != 99) {
            if (p->q_level >= 0 && p->q_level <= 2) rn_queue_insert(&ptable.l[p->q_level], p);
            else rn_heap_insert(&ptable.l3, p);
          }
          // if queue level is changed then break and recheck from L0
          goto cont;
        }

        scheduler_run_process(c, p);

        if (p->state == RUNNABLE) {
          if (p->q_level >= 0 && p->q_level <= 2) rn_queue_insert(&ptable.l[p->q_level], p);
          else rn_heap_insert(&ptable.l3, p);
        }
        goto cont;
      }
    } else { // MoQ
      for (; !rn_queue_is_empty(&ptable.monopoly); ) {
        p = rn_queue_pop(&ptable.monopoly);
        if (p->state != RUNNABLE) continue;

        scheduler_run_process(c, p);

        if (p->state == RUNNABLE) {
          rn_queue_insert(&ptable.monopoly, p);
        }
      }
      unmonopolize();
    }
cont:
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
      if (p->q_level == 99) rn_queue_insert(&ptable.monopoly, p);
      else if (p->q_level >= 0 && p->q_level <= 2) rn_queue_insert(&ptable.l[p->q_level], p);
      else rn_heap_insert(&ptable.l3, p);
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING) {
        p->state = RUNNABLE;
        if (p->q_level == 99) rn_queue_insert(&ptable.monopoly, p);
        else if (p->q_level >= 0 && p->q_level <= 2) rn_queue_insert(&ptable.l[p->q_level], p);
        else rn_heap_insert(&ptable.l3, p);
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// get current process's queue level
int getlev(void) {
  struct proc *p;
  int ret;

  ret = -1;

  acquire(&ptable.lock);
  p = myproc();
  ret = p->q_level;
  release(&ptable.lock);

  return ret;
}

// set process's priority
int setpriority(int pid, int priority) {
  struct proc *p;
  int ret;

  if (priority < 0 || priority > 10) return -2;
  ret = -1;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) {
      p->priority = priority;
      ret = 0;
      break;
    }
  }
  if (rn_heap_delete(&ptable.l3, p)) {
    rn_heap_insert(&ptable.l3, p);
  }
  release(&ptable.lock);

  return ret;
}

// set pid's process to be monopolized
int setmonopoly(int pid, int password) {
  struct proc *p;
  int ret;

  if (password != 2020021949) return -2;
  ret = -1;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) {
      if (p->q_level != 99) { // if process is not in MoQ
        p->q_level = 99;
        rn_queue_insert(&ptable.monopoly, p);
      }
      ret = rn_queue_size(&ptable.monopoly);
      break;
    }
  }
  release(&ptable.lock);

  return ret;
}

// make cpu using MoQ
void monopolize(void) {
  struct cpu* c;

  pushcli();
  c = mycpu();
  c->cpu_schedule_mode = 1;
  popcli();

  yield();
}

// make cpu using MLFQ
void unmonopolize(void) {
  struct cpu* c;

  pushcli();
  c = mycpu();
  c->cpu_schedule_mode = 0;
  popcli();

  acquire(&tickslock);
  ticks = 0;
  release(&tickslock);
}

// priority boosting
void priority_boosting(void) {
  int i;
  struct proc *p;

  acquire(&ptable.lock);
  for (i = 1; i < 3; ++i) {
    for (; !rn_queue_is_empty(&ptable.l[i]); ) {
      p = rn_queue_pop(&ptable.l[i]);
      if (p->q_level < 0 || p->q_level > 3) continue;

      p->q_level = 0;
      rn_queue_insert(&ptable.l[0], p);
    }
  }
  for (; !rn_heap_is_empty(&ptable.l3); ) {
    p = rn_heap_pop(&ptable.l3);
    if (p->q_level < 0 || p->q_level > 3) continue;

    p->q_level = 0;
    rn_queue_insert(&ptable.l[0], p);
  }

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->q_level != 99) p->q_level = 0;
  }

  release(&ptable.lock);
}
