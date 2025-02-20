// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};

// Per-CPU freelists
struct kmem kmemlist[NCPU];

void
kinit()
{
	int i;
	for(i = 0; i < NCPU; i++){
		// Use same name "kmem" for all lock in freelists
		initlock(&kmemlist[i].lock, "kmem");

		// Initialize the empty freelists
		kmemlist[i].freelist = (struct run*)0;
	}
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
	int npages;
	struct run *r;

  p = (char*)PGROUNDUP((uint64)pa_start);

	// number of pages for each freelist
	npages = (int)(PGROUNDDOWN((uint64)((char*)pa_end - p) / 8) / PGSIZE);
	
	// Put the page into the freelist except for the last CPU.	
	// It is because there would be remainder when we compute
	// npages above.
	for(int i = 0; i < NCPU - 1; i++){
		for(int j = 0; j < npages; j++){
			// Lock is not needed for this case because
			// only one process is exectuing this work.
			memset(p, 1, PGSIZE);
			r = (struct run*)p;	
			r->next = kmemlist[i].freelist;
			kmemlist[i].freelist = r;
			p += PGSIZE;
		}
	}

	// Put the page into the freelist for the last CPU.
	for(; p + PGSIZE <= (char *)pa_end; p+= PGSIZE){
		memset(p, 1, PGSIZE);
		r = (struct run*)p;
		r->next = kmemlist[NCPU-1].freelist;
		kmemlist[NCPU-1].freelist = r;
	}
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
	int cpu_id;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

	// Check the CPU ID
	push_off();
	cpu_id = cpuid();
	pop_off();
	
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmemlist[cpu_id].lock);
  r->next = kmemlist[cpu_id].freelist;
  kmemlist[cpu_id].freelist = r;
  release(&kmemlist[cpu_id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
	int cpu_id;

	// Check the CPU ID
	push_off();
	cpu_id = cpuid();
	pop_off();

  acquire(&kmemlist[cpu_id].lock);
  r = kmemlist[cpu_id].freelist;
  if(r)
    kmemlist[cpu_id].freelist = r->next;
  release(&kmemlist[cpu_id].lock);
	if(!r){
		r = steal();
	}

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
	
//	if(!r){
//		printf("not enough memory\n");
//	}
  return (void*)r;
}

void *
steal(void)
{
	struct run *r;
	//int cpu_id;
	
	push_off();
	//cpu_id = cpuid();
	pop_off();

	for(int i = NCPU-1; i >= 0; i--){
//		if(i == cpu_id)
//			continue;

		acquire(&kmemlist[i].lock);
		r = kmemlist[i].freelist;
		if(r)
			kmemlist[i].freelist = r->next;
		release(&kmemlist[i].lock);
		if(r){
			return (void*)r;
		}
	}
	return (void*)0;
}

void
printstat(void)
{
	struct run *r;
	for(int i = 0; i < NCPU; i++){
		int j = 0;
		acquire(&kmemlist[i].lock);
		for(r = kmemlist[i].freelist; r != (struct run *)0; r = r->next){
			j++;
		}
		release(&kmemlist[i].lock);
		printf("cpu %d: %d page left\n", i, j);
	}
}
