// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "proc.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;

  // count the number of references to each physical page
  int refcount[PHYSTOP >> LGPGSIZE];
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if ((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  if (kmem.use_lock) acquire(&kmem.lock);

  if (--kmem.refcount[V2P(v) >> LGPGSIZE] > 0) {
    if (kmem.use_lock) release(&kmem.lock);
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if (kmem.use_lock) release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if (kmem.use_lock) acquire(&kmem.lock);

  r = kmem.freelist;
  if (r) {
    kmem.refcount[V2P(r) >> LGPGSIZE] = 1;
    kmem.freelist = r->next;
  }

  if (kmem.use_lock) release(&kmem.lock);
  return (char*)r;
}

// Increment the reference count of a physical page
void incr_refc(uint pa) {
  if (pa % PGSIZE || (char*)pa < end || V2P(pa) >= PHYSTOP)
    panic("incr_refc");

  if (kmem.use_lock) acquire(&kmem.lock);

  ++kmem.refcount[V2P(pa) >> LGPGSIZE];

  if (kmem.use_lock) release(&kmem.lock);
}

// Decrement the reference count of a physical page
void decr_refc(uint pa) {
  if (pa % PGSIZE || (char*)pa < end || V2P(pa) >= PHYSTOP)
    panic("decr_refc");

  if (kmem.use_lock) acquire(&kmem.lock);

  --kmem.refcount[V2P(pa) >> LGPGSIZE];

  if (kmem.use_lock) release(&kmem.lock);
}

// Get the reference count of a physical page
int get_refc(uint pa) {
  int refc;

  if (pa % PGSIZE || (char*)pa < end || V2P(pa) >= PHYSTOP)
    panic("get_refc");  

  if (kmem.use_lock) acquire(&kmem.lock);

  refc = kmem.refcount[V2P(pa) >> LGPGSIZE];

  if (kmem.use_lock) release(&kmem.lock);

  return refc;
}

int sys_countfp(void) {
  int fps;
  struct run* r;

  if (kmem.use_lock) acquire(&kmem.lock);

  fps = 0;
  r = kmem.freelist;
  for (; r; ) {
    ++fps;
    r = r->next;
  }

  if (kmem.use_lock) release(&kmem.lock);

  return fps;
}

int sys_countvp(void) {
  return myproc()->sz / PGSIZE;
}

int sys_countptp(void) {
  int count;
  struct proc* curproc;

  curproc = myproc();
  count = 1; // for pgdir's page

  for (int i = 0; i < NPDENTRIES; ++i) {
    if (curproc->pgdir[i] & PTE_PTE) {
      ++count;

      pte_t *pgtab = (pte_t*)P2V(PTE_ADDR(curproc->pgdir[i]));
      for (int j = 0; j < NPTENTRIES; ++j) {
        if (pgtab[j] & PTE_PTE) {
          ++count;
        }
      }
    }
  }
  return count;
}
