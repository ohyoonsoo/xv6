// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


// Hash function
int hash(uint n);

struct bucket {
	struct spinlock lock;
	struct buf head;
};

struct bucket bucketlist[HASH_N];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

void
binit(void)
{
  initlock(&bcache.lock, "bcache");

	// Initialize the buckets in the bucketlist
	for(int i = 0; i < HASH_N; i++){
		initlock(&bucketlist[i].lock, "bcache.bucket");
		bucketlist[i].head.prev = &bucketlist[i].head;
		bucketlist[i].head.next = &bucketlist[i].head;
	}

	// Initialize the buffer cache in the bcache.buf
	for(int i = 0; i < NBUF; i++){
		bcache.buf[i].blockno = DEFAULT_BLOCKNO;
		bcache.buf[i].refcnt = 0;
	}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
	struct buf *candidate_b = (struct buf*)0;
	int n_hash;

	n_hash = hash(blockno);
	
	acquire(&bucketlist[n_hash].lock);

	// Is the block already cached?
	for(b = bucketlist[n_hash].head.next; b != &bucketlist[n_hash].head; b = b->next){
		if(b->dev == dev && b->blockno == blockno){
			b->refcnt++;
			release(&bucketlist[n_hash].lock);
			acquiresleep(&b->lock);
			return b;
		} else if(b->refcnt == 0){
			// find the candidate to replace in the same bucket.
			candidate_b = b;
		}
	}
	// If we can find the candidate in the same bucket, recycle it.
	if(candidate_b){
		candidate_b->dev = dev;
		candidate_b->blockno = blockno;
		candidate_b->valid = 0;
		candidate_b->refcnt = 1;
		release(&bucketlist[n_hash].lock);
		acquiresleep(&candidate_b->lock);
		return candidate_b;
	}

	// Not cached && can't find the candiate in the same bucket.
	release(&bucketlist[n_hash].lock);
	acquire(&bcache.lock);

	for(int i = 0; i < NBUF; i++){
		b = &bcache.buf[i];
		if(b->blockno != DEFAULT_BLOCKNO){
			int prev_hash = hash(b->blockno);

			acquire(&bucketlist[prev_hash].lock);
			if(b->refcnt == 0){
				b->prev->next = b->next;
				b->next->prev = b->prev;
				candidate_b = b;
			}
			release(&bucketlist[prev_hash].lock);
		} else {
			candidate_b = b;
		}

		if(candidate_b){
			b->dev = dev;
			b->blockno = blockno;
			b->valid = 0;
			b->refcnt = 1;
			release(&bcache.lock);
			acquire(&bucketlist[n_hash].lock);
			b->next = bucketlist[n_hash].head.next;
			b->prev = &bucketlist[n_hash].head;
			bucketlist[n_hash].head.next->prev = b;
			bucketlist[n_hash].head.next = b;
			release(&bucketlist[n_hash].lock);
			acquiresleep(&b->lock);
			return b;
		}
	}
	panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
	int n_hash = hash(b->blockno);
  if(!holdingsleep(&b->lock))
    panic("brelse");

	acquire(&bucketlist[n_hash].lock);
	b->refcnt--;
	release(&bucketlist[n_hash].lock);
	
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
	int n_hash = hash(b->blockno);
  acquire(&bucketlist[n_hash].lock);
  b->refcnt++;
  release(&bucketlist[n_hash].lock);
}

void
bunpin(struct buf *b) {
	int n_hash = hash(b->blockno);
  acquire(&bucketlist[n_hash].lock);
  b->refcnt--;
  release(&bucketlist[n_hash].lock);
}

// Hash function
int
hash(uint n)
{
	return (int)(n % HASH_N);
}
