// alloc.c
// Rediet Negash
// 111820799

#include <stdio.h>
#include <stdlib.h>

#define offsetof(st, m)         ((size_t) &(((st *)0)->m))
#define containerof(ptr, st, m) ((st *) (((char*)(ptr)) - offsetof(st, m)))
#define ERRQUIT(msg)            (err_quit((msg), __FILE__, __func__, __LINE__))
#define CHKBOOLEXIT(exp, msg)   ((exp) || ERRQUIT(msg))

#define MEM_SIZE            (1<<20) //1MB
#define BRK_INCR            (1<<12) //4KB: minimum brk increase size
#define MEM_ALIGN_SIZE      16      //memory alignment
#define MEM_ALIGN(size)     ((size) + (MEM_ALIGN_SIZE - (size)%MEM_ALIGN_SIZE) % MEM_ALIGN_SIZE)
#define HDR_TYPE            unsigned long
#define HDR_INUSE(hdr)      ((hdr) & 0x1)
#define HDR_SIZE(hdr)       ((hdr) & ~0x7)
#define HEADER(size, inuse) ((size) | (inuse))
#define EPILOG              HEADER(0, 1)
#define NEXT_BLOCK(block)   ((struct Block*)(((char*)block) + HDR_SIZE(block->header)))
#define PREV_BLOCK(block)   ((struct Block*)(((char*)block) - HDR_SIZE(block->prev_footer)))

struct Mem {
    char *start;
    char *end;
    char *brk;
};
struct Block{
    HDR_TYPE    prev_footer;    //previous block's footer
    HDR_TYPE    header;         //current block's header
    char        payload;
};

int err_quit(char *msg, const char* file, const char *func, int line) {
    printf("Error in function %s (%s, line %d): %s\n", func, file, line, msg);
    exit(1);
    return 0;
}

// Simulated mem
struct Mem mem;
void InitMem() {
    mem.start = (char*)malloc(MEM_SIZE);
    mem.brk = mem.start;
    mem.end = mem.start + MEM_SIZE;
}
void* Sbrk(int incr) {
    char *oldbrk = mem.brk;
    if(incr < 0 || mem.brk + incr >= mem.end)
        ERRQUIT("out of memory");
    mem.brk += incr;
    return oldbrk;
}

// Malloc
struct Block *blockHead;    //head of the Block list
void InitMalloc() {
    struct Block *curr, *next;
    curr = (struct Block*)Sbrk(4 * sizeof(HDR_TYPE));
    curr->prev_footer = 0;                          //padding
    curr->header = HEADER(2*sizeof(HDR_TYPE), 1);   //prolog
    next = NEXT_BLOCK(curr);
    next->prev_footer = curr->header;
    next->header = EPILOG;                          //epilog
    blockHead = curr;
}
struct Block* ExtendHeap(size_t size) {
    struct Block* Coalesce(struct Block *curr);
    struct Block *curr, *next;
    char *ptr;

    size = size > BRK_INCR ? size : BRK_INCR;
    size = MEM_ALIGN(size);

    ptr = (char*)Sbrk(size);
    curr = (struct Block*)(ptr - 2*sizeof(HDR_TYPE)); //-... : previous epilog
    
    //update curr, update next, set epilog, return coalesced block
    curr->header = HEADER(HDR_SIZE(size),0);
    next = NEXT_BLOCK(curr);
    next->prev_footer = HEADER(HDR_SIZE(size),0);
    next->header = EPILOG;
    return Coalesce(curr);
}

// Block functions
struct Block* Coalesce(struct Block *curr) {
    struct Block *prev = PREV_BLOCK(curr);
    struct Block *next = NEXT_BLOCK(curr);
    int size;
    if(HDR_INUSE(curr->prev_footer) &&
       HDR_INUSE(next->header)) {
        return curr;
    }
    else if( HDR_INUSE(curr->prev_footer) &&
            !HDR_INUSE(next->header)) {
	size = HDR_SIZE(curr->header) + HDR_SIZE(next->header);
	curr-> header = HEADER(size,0);
	NEXT_BLOCK(next)-> prev_footer = HEADER(size,0);
    }  
    else if(!HDR_INUSE(curr->prev_footer) &&
             HDR_INUSE(next->header)) {
        next= NEXT_BLOCK(curr);
	size = HDR_SIZE(curr->header) + HDR_SIZE(prev->header);
	prev->header = HEADER(size,0);
        next-> prev_footer = HEADER(size,0);
    }
    else {
	size = HDR_SIZE(prev->header) + HDR_SIZE(curr->header) + HDR_SIZE(next->header);
	prev->header = HEADER(size,0);
	NEXT_BLOCK(next)->prev_footer = HEADER(size,0);
    }
}
struct Block *FindFreeBlock(size_t size) {
    struct Block *pos;
    for(pos = blockHead; pos->header != EPILOG; pos = NEXT_BLOCK(pos)) {
	if((HDR_SIZE(pos->header)>= size)&&(HDR_INUSE(pos-> header) == 0X0)){
		return pos;
	}
    }
    return NULL;
}
// Malloc & Free
void *Malloc(size_t size) {
    struct Block *curr, *next;
    size_t next_size;

    size = MEM_ALIGN(size + 2*sizeof(HDR_TYPE));
    curr = FindFreeBlock(size);
    if(curr == NULL)
        curr = ExtendHeap(size);
    next_size = HDR_SIZE(curr->header) - size;
    curr->header = HEADER(size,1);
    next = NEXT_BLOCK(curr);
    next->prev_footer = curr->header;

    //update current block's header and footer
    if (next_size >= 2 * sizeof(HDR_TYPE)) {
        //update next block's header and footer
	next->header = HEADER(next_size,0);
        NEXT_BLOCK(next)->prev_footer = HEADER(next_size,0);
    }
    else if (next_size != 0)
        ERRQUIT("unexpected remaing size");
    return &curr->payload;
}
void Free(void *mem) {
    struct Block *curr = containerof(mem, struct Block, payload);
    int size;
    //get the block from mem,
    size = HDR_SIZE(curr->header);
    //update the header and footer of the block
    curr->header = HEADER(size,0);
    NEXT_BLOCK(curr)-> prev_footer = HEADER(size,0);
    //coalesce the block with the neighboring ones
    Coalesce(curr);
}
// Test
void UnitTest() {
    const int nptr = 10;
    char *ptr[nptr], *p;
    int i, j;

    for(i = 0; i < nptr; i++) {
        ptr[i] = Malloc(1000);
        printf("Malloc ptr[%d] = %p\n", i, ptr[i]);
    }
    for(i = 1; i < nptr; i += 2) {
        Free(ptr[i]);
        printf("Free ptr[%d] = %p\n", i, ptr[i]);
    }

    i = 1;
    p = Malloc(1000);
    printf("p:%p == ptr[%d]:%p\n", p, i, ptr[i]);
    CHKBOOLEXIT(p == ptr[i], "unexpected alloc");

    i = nptr - 1;
    p = Malloc(1500);
    printf("p:%p == ptr[%d]:%p\n", p, i, ptr[i]);
    CHKBOOLEXIT(p == ptr[i], "unexpected alloc");

    i = 3;
    Free(ptr[4]);
    p = Malloc(500);
    printf("p:%p == ptr[%d]:%p\n", p, i, ptr[i]);
    CHKBOOLEXIT(p == ptr[i], "unexpected alloc");

    i = 3, j = 4;
    p = Malloc(2500);
    printf("p:%p > ptr[%d]:%p\n", p, i, ptr[i]);
    CHKBOOLEXIT(ptr[i] < p, "unexpected alloc");
    printf("p:%p < ptr[%d]:%p\n", p, j, ptr[j]);
    CHKBOOLEXIT(ptr[j] > p, "unexpected alloc");

    printf("SUCCESS!\n");

}
int main() {
    InitMem();
    InitMalloc();

    UnitTest();

    return 0;
}
