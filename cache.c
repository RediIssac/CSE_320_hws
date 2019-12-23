// Rediet Negash
// 111820799
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRQUIT(msg)            (err_quit((msg), __FILE__, __func__, __LINE__))
#define CHKBOOLQUIT(exp, msg)   ((exp) || ERRQUIT(msg))

#define ADRSBITS    (20)                    //1MB
#define VPNBITS     (10)                    //# of bits in Virtual Page Number
#define VPOBITS     (ADRSBITS - VPNBITS)    //# of bits in Virtual Page Offset
#define PTEBYTES    ((1 + VPNBITS + 7)/8)   //# of bytes in PTE. 1: valid bit, 7, 8: round up
#define VPNVALID    (1 << (1+VPNBITS))      //valid bit of VPN
#define PAGESIZE    (1 << VPOBITS)          //size of each page
#define PAGECOUNT   (1 << VPNBITS)          //# of pages
#define MAXPROCESS  (4)                     //max number of process

typedef unsigned short Word;
typedef unsigned int   DWord;
typedef struct {
    char valid;
    int tag;
    int bufSize;
    char *buf;
} CacheLine;
typedef struct {
    int nl;  //# of lines (E)
    CacheLine *lines;
} CacheSet;
typedef struct {
    int m, s, b, t;
    int ns;  //# of sets 2^s (S)
    int mask_set, mask_tag, mask_off;
    CacheSet *sets;
} Cache;
typedef struct {
    int pid;
    Word PTBR;      //page table base register
} Process;
typedef struct {
    char *mem;      //physical memory
    char *disk;     //disk
    Cache Lx;       //cache: physical address -> contents
    Cache TLB;      //translation lookahead buffer
    Word PTBR;      //the current process’ PTBR
    DWord *rmap;    //page frame number -> physical PTE address
    Word mask_vpn, mask_vpo;
} MemSystem;

int err_quit(char *msg, const char *file, const char *func, const int line) {
    printf("Error in function %s (%s, line %d): %s\n", func, file, line, msg);
    exit(1);
    return 0;
}
static unsigned long int next = 1;
int rand(void) {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}
void srand(unsigned int seed) {
    next = seed;
}
////////////////////////////////////////////////////////////
// Cache related functions
//
void initCacheLine(CacheLine *cl, int bufSize) {
    cl->valid = 0;
    cl->tag = 0;
    cl->bufSize = bufSize;
    cl->buf = (char*)malloc(bufSize);
}
void initCacheSet(CacheSet *cs, int numLines, int bufSize) {
    int i;
    cs->nl = numLines;
    cs->lines = (CacheLine*)malloc(sizeof(CacheLine)*numLines);
    for(i = 0; i < numLines; i++)
        initCacheLine(cs->lines + i, bufSize);
}
void initCache(Cache* c, int m, int s, int b, int E) {
    int i;
    //TBD memset(c, 0, sizeof(Cache));
    //TODO: delete above line with memset,
    // initialize c->m, c->s, c->b, c->t, c->ns,
    // c->mask_set, c->mask_tag, c->mask_off
    //Metries
    c->m=m;
    c->s=s;
    c->b=b;
    c->t=m - s - b;
    c->ns= 1<<s;
    c->mask_off=(1<<b)-1;
    //from tag,set,offset(mask the bits from the size)
    c->mask_set=(1<<s)-1;
    c->mask_tag=(1<<c->t)-1;
    //endMetries
    c->sets = (CacheSet*)malloc(sizeof(CacheSet)*c->ns);
    for(i = 0; i < c->ns; i++)
    //TODO: initialize c->sets+i by initCacheSet
      initCacheSet(c->sets+i, E, c->mask_off+1);
}
CacheLine *findCacheLine(Cache *c, DWord adrs) {
    DWord set = 0;
    DWord tag = 0;
    
    //TODO: find set index from adrs
    set=(adrs&(c->mask_set<<c->b))>>c->b;
    tag=(adrs&(c->mask_tag<<(c->s+c->b)))>>(c->s+c->b);
    //TODO: find tag from adrs
    CacheSet *cs = &c->sets[set];
    int i;
    for(i = 0; i < cs->nl; i++)
        if( cs->lines[i].valid &&
            (c->t == 0 || cs->lines[i].tag == tag) )

        return &cs->lines[i];
    return NULL;
}
int tryReadFromCache(Cache *c, DWord adrs, Word *w) {
    DWord off = adrs & c->mask_off;
    DWord set,tag=0;
    tag=(adrs&(c->mask_tag<<(c->s+c->b)))>>(c->s+c->b);
    set=(adrs&(c->mask_set<<c->b))>>c->b;
    CacheLine *cl = findCacheLine(c, adrs);
    if(cl == NULL)
        return 0;
    memcpy(w, cl->buf+off, sizeof(Word));
    return 1;
}

int tryWriteToCache(Cache *c, DWord adrs, Word w) {
    DWord off = adrs & c->mask_off;
    CacheLine *cl = findCacheLine(c, adrs);
    if(cl == NULL)
        return 0;
    memcpy(cl->buf+off, &w, sizeof(Word));
    return 1;
}

//return if there is a cacheLine for adrs; otherwise
//pick a cacheLine and load it with the block from memory
void loadCacheLine(Cache *c, DWord adrs, char *mem) {
    DWord set = 0; //TODO: find set index from adrs
    DWord tag = 0; //TODO: find tag from adrs
    DWord padrs=0;
    set=c->mask_set&(adrs>>c->b);
    tag=adrs>>(c->b+c->s);
    DWord off = adrs & c->mask_off;
    CacheSet *cs = &c->sets[set];
    CacheLine *cl = NULL;

    //TODO: try to find the line that contains adrs
    cl=findCacheLine(c,adrs);
    if (cl!= NULL){
      return;
    }
    //2. try to find an invalid line
    int i;
    for (i = 0; i < cs->nl; i++) {
        //TODO: find an invalid line
        if(cs->lines[i].valid==0){
          cl=&cs->lines[i];
          break;
        }
    }
    //3. evict a line
    if (cl == NULL) {
        cl = &cs->lines[rand() % cs->nl]; //randomly pick a line
        if (mem != NULL) {//write back to mem
            //TODO: construct padrs using cl->tag, c->s, c->b, and set
             padrs=((cl->tag<<(c->s+c->b) | (set<<c->b)));
            //TODO: copy from c->buf to mem + padrs using memcpy
            memcpy(mem+padrs, cl->buf, cl->bufSize);
        }
    }
    if (mem != NULL) {//load the line from mem
       //TODO: copy from mem + adrs - off to cl->buf using memcpy
       memcpy(cl->buf,mem+adrs-off,cl->bufSize);
   }
   //TODO: update cl->valid and cl->tag
    cl->valid=1;
    cl->tag=tag;
    return;
}

//write w to a loaded cacheLine
void writeToCache(Cache *c, DWord adrs, Word w, char *mem) {
    loadCacheLine(c, adrs, mem);
    CHKBOOLQUIT(tryWriteToCache(c, adrs, w), "cache is not loaded");
}

//copy the contents of the cache to memory
void flushCache(Cache *c, char *mem, int invalidate) {
    int i, j;
    DWord padrs,tags;
    for(i = 0; i < c->ns; i++) {
        // flush set i
        int nl = c->sets[i].nl;
        for(j = 0; j < nl; j++) {
            // flush line j of set i
            CacheLine *cl = &c->sets[i].lines[j];
            if(cl->valid && mem != NULL) {
                tags=cl->tag;
                padrs=((cl->tag<<(c->s+c->b)) | (i<<c->b));
                //TODO: construct padrs using cl->tag, c->s, c->b, and set
                 memcpy(mem+padrs,cl->buf,cl->bufSize);
                 //TODO: copy from c->buf to mem + padrs using memcpy
            }
            if(invalidate)
                cl->valid = 0;
        }
    }
}
////////////////////////////////////////////////////////////
// Virtual memory related functions
//
void readFromPhysicalMem(MemSystem *ms, DWord padrs, Word *w) {
    if(!tryReadFromCache(&ms->Lx, padrs, w)) {
        writeToCache(&ms->Lx, padrs, *(Word*)(ms->mem+padrs), ms->mem);
        CHKBOOLQUIT(tryReadFromCache(&ms->Lx, padrs, w), "cache is not loaded");
    }
}
void writeToPhysicalMem(MemSystem *ms, DWord padrs, Word w) {
    //TODO: implement write back, write allocate policy by
    // 1. try to write to cache
    // 2. on cache miss, load the memory to the cache line
    // 3. try to write to cache again
    //Metries
    if(!tryWriteToCache(&ms->Lx, padrs, w)) {
        //readFromPhysicalMem(ms, padrs, &w);
        writeToCache(&ms->Lx, padrs, *(Word*)(ms->mem+padrs), ms->mem);
        CHKBOOLQUIT(tryWriteToCache(&ms->Lx, padrs, w), "Physical memory not loaded");
    }

}
void readPTE(MemSystem *ms, Word vpn, Word *pte) {
  //TODO: get pte by
    // 1. try to read pte from TLB
    //    use tryReadFromCache(&ms->TLB, vpn*2, pte)
    // 2. on cache miss, get pte from the physical mem and update TLB
    //    to read from physical mem use readFromPhysicalMem
    //    to update TLB: writeToCache(&ms->TLB, vpn*2, *pte, NULL)
  //Metries
  //tryReadFromCache(&ms->TLB, vpn*2, pte);
  if(!tryReadFromCache(&ms->TLB, vpn*PTEBYTES, pte)){
    readFromPhysicalMem(ms,ms->PTBR+vpn*PTEBYTES,pte);
    writeToCache(&ms->TLB, vpn*PTEBYTES, *pte, NULL);
  }

}
void readPageFromDisk(MemSystem *ms, DWord vadrs) {
    Word vpn = (vadrs >> VPOBITS) & ms->mask_vpn;
    Word vpo = vadrs & ms->mask_vpo;
    Word pte, pfn, fn; //pfn, fn: page frame number
    DWord padrs, doff;
    readPTE(ms, vpn, &pte);
    if(!(pte & VPNVALID)) {
        //1. try to find an empty page frame
        for(fn = 2*MAXPROCESS; fn < PAGECOUNT; fn++) //0 .. 2*MAXPROCESS-1: page tables
            if(ms->rmap[fn] == (DWord)-1)
                break;
        //2. if none is found, pick a page and swap it out
        if(fn == PAGECOUNT) {
            fn = (rand() % (PAGECOUNT-2*MAXPROCESS)) + 2*MAXPROCESS;
            doff = (ms->rmap[fn]/PTEBYTES) << VPOBITS;
            //TODO: copy the picked page frame to disk by
            // 1. flush Lx cache to memory before copying the page to disk
            // 2. set padrs to the beginning of the page frame address for fn
            // 3. using memcpy, copy the page at ms->mem+padrs to disk at ms->disk+doff
            // 4. invalidate the page table entry by writing 0 to it (use rmap)
            // 5. invalidate TLB
          
            flushCache(&ms->Lx,ms->mem,0);
            padrs=fn<<VPOBITS;
            memcpy(ms->disk+doff,ms->mem+padrs,PAGESIZE);
            writeToPhysicalMem(ms, ms->rmap[fn], 0);
            flushCache(&ms->TLB, NULL,  1);
        }
        ms->rmap[fn] = ms->PTBR + vpn*PTEBYTES; //update rmap
        pte = fn | VPNVALID;
        writeToCache(&ms->TLB, vpn*2, pte, NULL); //update TLB
        writeToPhysicalMem(ms, ms->PTBR + vpn*PTEBYTES, pte); //update page table
    }
    pfn = pte & ms->mask_vpn;
    doff = ((ms->PTBR + vpn*PTEBYTES)/PTEBYTES) << VPOBITS;

    padrs=pfn<<VPOBITS;
    flushCache(&ms->Lx,ms->mem,1);
    memcpy(ms->mem+padrs,ms->disk+doff,PAGESIZE);
    //TODO: copy the page from disk to memory by
    // 1. set padrs to the beginning of the page frame address for pfn
    // 2. invalidate cache rather than find and update the lines in the page
    // 3. using memcpy, copy the page in disk disk ms->disk+doff, to mem at ms->mem+padrs
}
void writePageToDisk(MemSystem *ms, DWord vadrs) {
    Word vpn = (vadrs >> VPOBITS) & ms->mask_vpn;
    Word vpo = vadrs & ms->mask_vpo;
    Word pte, pfn; //page frame number, physical mem address
    DWord padrs, doff;
    readPTE(ms, vpn, &pte);
    CHKBOOLQUIT(pte & VPNVALID, "writing an invalid page");
    pfn = pte & ms->mask_vpn;
    doff = ((ms->PTBR + vpn*PTEBYTES)/PTEBYTES) << VPOBITS;
    //TODO: copy the page frame to disk by
    // 1. flush the cache to memory before copy the page to disk
    // 2. set padrs to the beginning of the page frame address
    // 3. using memcpy, copy the page at ms->mem+padrs to disk at ms->disk+ms->doff
    //MeTries
    flushCache(&ms->Lx ,ms->mem,0);
    padrs=pfn<<VPOBITS;
    memcpy(ms->disk+doff,ms->mem+padrs,PAGESIZE);

}
void readFromVirtualMem(MemSystem *ms, DWord vadrs, Word *w) {
    Word vpn = (vadrs >> VPOBITS) & ms->mask_vpn;
    Word vpo = vadrs & ms->mask_vpo;
    Word pfn, pte;
    DWord padrs;
    readPTE(ms, vpn, &pte);
    if(!(pte & VPNVALID)) {
        readPageFromDisk(ms, vadrs);
        readFromPhysicalMem(ms, ms->PTBR + vpn*PTEBYTES, &pte);
        CHKBOOLQUIT(pte & VPNVALID, "valid bit is not set after reading the page");
    }
    pfn = pte & ms->mask_vpn;
    padrs = (pfn << VPOBITS) | vpo;
    readFromPhysicalMem(ms, padrs, w);
}
void writeToVirtualMem(MemSystem *ms, DWord vadrs, Word w) {
    Word vpn = (vadrs >> VPOBITS) & ms->mask_vpn;
    Word vpo = vadrs & ms->mask_vpo;
    Word pfn, pte;
    DWord padrs;
    readPTE(ms, vpn, &pte);
    if(!(pte & VPNVALID)) {
        readPageFromDisk(ms, vadrs);
        readFromPhysicalMem(ms, ms->PTBR + vpn*PTEBYTES, &pte);
        CHKBOOLQUIT(pte & VPNVALID, "valid bit is not set after reading the page");
    }
    pfn = pte & ms->mask_vpn;
    padrs = (pfn << VPOBITS) | vpo;
    writeToPhysicalMem(ms, padrs, w);
}
////////////////////////////////////////////////////////////
// Memory system and process
//
void initMemSystem(MemSystem *ms) {
    ms->mem  = (char*)malloc(1 << ADRSBITS);
    ms->disk = (char*)malloc(MAXPROCESS * (1 << ADRSBITS));
    ms->rmap = (DWord*)malloc(PAGECOUNT * sizeof(DWord));
    memset(ms->rmap, 0xff, PAGECOUNT * sizeof(DWord));
    ms->mask_vpn = (1 << VPNBITS)-1;
    ms->mask_vpo = (1 << VPOBITS)-1;
    ms->PTBR = 0;
    initCache(&ms->Lx, ADRSBITS, 6/*s*/, 8/*b*/, 8/*E*/);
    initCache(&ms->TLB, VPNBITS, 0/*s*/, 1/*b: Word size*/, 16/*E*/);
}
void initProcess(Process *proc, int pid, MemSystem *ms) {
    proc->pid = pid;
    proc->PTBR = pid * 2*PAGESIZE;             //2 pages per page table
    ms->rmap[pid*2]   = VPNVALID | proc->PTBR; //mark the frames as being used
    ms->rmap[pid*2+1] = VPNVALID | (proc->PTBR+PAGESIZE);
    memset(ms->mem + proc->PTBR, 0, 2*PAGESIZE);
}
void switchToProcess(Process *proc, MemSystem *ms) {
    flushCache(&ms->TLB, NULL, 1/*invalidate*/);
    ms->PTBR = proc->PTBR;
}
void unitTestCache() {
    char mem[256] = {0, 0, 1, 0, 2, 0, 3, 0, 4, 0,};
    Cache c;
    Word w;
    printf("-- Testing cache initialization...\n");
    initCache(&c, 8/*m*/, 2/*s*/, 3/*b*/, 2/*E*/);
    CHKBOOLQUIT(c.t + c.s + c.b == c.m, "incorrect # of bits");
    CHKBOOLQUIT(c.mask_set == 0x3, "incorrect set mask");
    CHKBOOLQUIT(c.mask_tag == 0x7, "incorrect tag mask");
    CHKBOOLQUIT(c.mask_off == 0x7, "incorrect offset mask");
    CHKBOOLQUIT(c.ns == 0x4, "incorrect number of sets");
    CHKBOOLQUIT(c.sets[0].nl == 0x2, "incorrect number of lines");
    CHKBOOLQUIT(c.sets[0].lines[0].bufSize == 0x8, "incorrect buffer size");
    CHKBOOLQUIT(!tryReadFromCache(&c, 0, &w), "unexpected cache hit");
    CHKBOOLQUIT(!tryWriteToCache(&c, 0, w), "unexpected cache hit");

    printf("-- Testing writeToCache...\n");
    writeToCache(&c, 0xa, 5, mem);
    CHKBOOLQUIT(c.sets[1].lines[0].valid, "valid bit not set");
    CHKBOOLQUIT(c.sets[1].lines[0].tag == 0, "incorrect tag");
    CHKBOOLQUIT(*((Word*)(c.sets[1].lines[0].buf+2)) == 5, "incorrect line");
    CHKBOOLQUIT(*((Word*)(c.sets[1].lines[0].buf+0)) == 4, "line not loaded correctly");
    CHKBOOLQUIT(tryReadFromCache(&c, 0xa, &w), "unexpected cache miss");
    CHKBOOLQUIT(w == 5, "incorrect read");
    CHKBOOLQUIT(tryReadFromCache(&c, 0x8, &w), "unexpected cache miss");
    CHKBOOLQUIT(w == 4, "incorrect read");
    CHKBOOLQUIT(tryWriteToCache(&c, 0xc, 6), "unexpected cache miss");
    CHKBOOLQUIT(tryReadFromCache(&c, 0xc, &w), "unexpected cache miss");
    CHKBOOLQUIT(w == 6, "incorrect read");

    printf("-- Testing writeToCache 2nd line...\n");
    writeToCache(&c, 0x2a, 0x12, mem);
    CHKBOOLQUIT(c.sets[1].lines[1].valid, "valid bit not set");
    CHKBOOLQUIT(c.sets[1].lines[1].tag == 1, "incorrect tag");
    CHKBOOLQUIT(*((Word*)(c.sets[1].lines[1].buf+2)) == 0x12, "incorrect line");
    CHKBOOLQUIT(tryReadFromCache(&c, 0x2a, &w), "unexpected cache miss");
    CHKBOOLQUIT(w == 0x12, "incorrect read");

    printf("-- Testing writeToCache eviction...\n");
    writeToCache(&c, 0x4a, 0x13, mem);
    CHKBOOLQUIT(c.sets[1].lines[1].valid, "valid bit not set");
    printf("%d", c.sets[1].lines[1].tag);
    CHKBOOLQUIT(c.sets[1].lines[1].tag == 2, "incorrect tag");
    CHKBOOLQUIT(*((Word*)(c.sets[1].lines[1].buf+2)) == 0x13, "incorrect line");
    CHKBOOLQUIT(tryReadFromCache(&c, 0x4a, &w), "unexpected cache miss");
    CHKBOOLQUIT(w == 0x13, "incorrect read");
    CHKBOOLQUIT(*(Word*)(mem + 0x4a) == 0, "mem shouldn't be updated yet");
    CHKBOOLQUIT(!tryReadFromCache(&c, 0x2a, &w), "unexpected cache hit");
    CHKBOOLQUIT(*(Word*)(mem + 0x2a) == 0x12, "line is not flushed to mem");
    CHKBOOLQUIT(tryReadFromCache(&c, 0xa, &w), "unexpected cache miss");
    CHKBOOLQUIT(*(Word*)(mem + 0xa) == 0, "incorrect line is flushed to mem");

    printf("-- Testing flushCache...\n");
    flushCache(&c, mem, 1/*invalidate*/);
    CHKBOOLQUIT(*(Word*)(mem + 0x8) == 4, "overwritten after flush");
    CHKBOOLQUIT(*(Word*)(mem + 0xa) == 5, "not updated after flush");
    CHKBOOLQUIT(*(Word*)(mem + 0x2a) == 0x12, "not updated after flush");
    CHKBOOLQUIT(!c.sets[1].lines[0].valid, "not invalidated");
    CHKBOOLQUIT(!c.sets[1].lines[1].valid, "not invalidated");
    CHKBOOLQUIT(!tryReadFromCache(&c, 0xa, &w), "unexpected cache hit");
    CHKBOOLQUIT(!tryReadFromCache(&c, 0x4a, &w), "unexpected cache hit");
}
void unitTestVirtualMemory() {
    MemSystem ms;
    Process proc[MAXPROCESS];
    DWord vadrs = 0x12345, vpo = 0x345, vpn = 0x48;
    DWord padrs, i, j, k;
    Word  w, pte, pfn;

    printf("-- Testing initMemSystem...\n");
    initMemSystem(&ms);
    CHKBOOLQUIT((vadrs & ms.mask_vpo) == vpo, "incorrect mask_vpo");
    CHKBOOLQUIT(((vadrs >> VPNBITS) & ms.mask_vpn) == vpn, "incorrect mask_vpn");

    printf("-- Testing switchToProcess...\n");
    for(i = 0; i < MAXPROCESS; i++) {
        initProcess(proc + i, i, &ms);
        switchToProcess(proc + i, &ms);
        CHKBOOLQUIT(ms.TLB.sets[0].lines[0].valid == 0, "TLB not invalidated");
        CHKBOOLQUIT(ms.PTBR == proc[i].PTBR, "PTBR not set correctly");
    }

    printf("-- Testing writeToVirtualMem...\n");
    switchToProcess(proc + 0, &ms);
    CHKBOOLQUIT(!tryReadFromCache(&ms.TLB, vpn*2, &pte), "unexpected cache hit");
    CHKBOOLQUIT(!tryReadFromCache(&ms.Lx, ms.PTBR + vpn*PTEBYTES, &pte), "unexpected cache hit");
    writeToVirtualMem(&ms, vadrs, 1);
    CHKBOOLQUIT(tryReadFromCache(&ms.TLB, vpn*2, &pte), "unexpected cache miss");
    CHKBOOLQUIT(tryReadFromCache(&ms.Lx, ms.PTBR + vpn*PTEBYTES, &pte), "unexpected cache miss");
    CHKBOOLQUIT(pte & VPNVALID, "pte not valid");
    pfn = pte & ms.mask_vpn;
    padrs = (pfn << VPOBITS) | vpo;
    readFromPhysicalMem(&ms, padrs, &w);
    CHKBOOLQUIT(w == 1, "incorrect value is read");
    readFromVirtualMem(&ms, vadrs, &w);
    CHKBOOLQUIT(w == 1, "incorrect value is read");
    CHKBOOLQUIT(*(Word*)(ms.mem+padrs) == 0, "mem shouldn't be updated yet");
    readFromVirtualMem(&ms, 0x11345, &w);   //page miss
    CHKBOOLQUIT(*(Word*)(ms.mem+padrs) == 1, "mem should be updated now");

    printf("-- Testing writeToVirtualMem by proc[1]...\n");
    switchToProcess(proc + 1, &ms);
    CHKBOOLQUIT(!tryReadFromCache(&ms.TLB, vpn*2, &pte), "unexpected cache hit");
    CHKBOOLQUIT(!tryReadFromCache(&ms.Lx, ms.PTBR + vpn*PTEBYTES, &pte), "unexpected cache hit");
    writeToVirtualMem(&ms, vadrs, 2);
    CHKBOOLQUIT(tryReadFromCache(&ms.TLB, vpn*2, &pte), "unexpected cache miss");
    CHKBOOLQUIT(tryReadFromCache(&ms.Lx, ms.PTBR + vpn*PTEBYTES, &pte), "unexpected cache miss");
    CHKBOOLQUIT(pte & VPNVALID, "pte not valid");
    pfn = pte & ms.mask_vpn;
    padrs = (pfn << VPOBITS) | vpo;
    readFromPhysicalMem(&ms, padrs, &w);
    CHKBOOLQUIT(w == 2, "incorrect value is read");
    readFromVirtualMem(&ms, vadrs, &w);
    CHKBOOLQUIT(w == 2, "incorrect value is read");
    CHKBOOLQUIT(*(Word*)(ms.mem+padrs) == 0, "mem shouldn't be updated yet");

    printf("-- Testing readFromVirtualMem by proc[0]...\n");
    switchToProcess(proc + 0, &ms);
    CHKBOOLQUIT(!tryReadFromCache(&ms.TLB, vpn*2, &pte), "unexpected cache hit");
    CHKBOOLQUIT(!tryReadFromCache(&ms.Lx, ms.PTBR + vpn*PTEBYTES, &pte), "unexpected cache hit");
    readFromVirtualMem(&ms, vadrs, &w);
    CHKBOOLQUIT(tryReadFromCache(&ms.TLB, vpn*2, &pte), "unexpected cache miss");
    CHKBOOLQUIT(tryReadFromCache(&ms.Lx, ms.PTBR + vpn*PTEBYTES, &pte), "unexpected cache miss");
    CHKBOOLQUIT(pte & VPNVALID, "pte not valid");
    pfn = pte & ms.mask_vpn;
    padrs = (pfn << VPOBITS) | vpo;
    CHKBOOLQUIT(*(Word*)(ms.mem+padrs) == 1, "incorrect value at mem");
    CHKBOOLQUIT(w == 1, "incorrect value is read");
    readFromPhysicalMem(&ms, padrs, &w);
    CHKBOOLQUIT(w == 1, "incorrect value is read");

    printf("-- Testing page swapping...\n");
    for(j = 0; j < MAXPROCESS; j++) {
        switchToProcess(proc + j, &ms);
        writeToVirtualMem(&ms, 0, j + 0);
        readFromVirtualMem(&ms, 0, &w);
        CHKBOOLQUIT(w == j + 0, "incorrect read");
    }
    for(i = 0; i < 3; i++) {
        for(j = 0; j < MAXPROCESS; j++) {
            switchToProcess(proc + j, &ms);
            for(k = 0; k < 300*1024; k += sizeof(Word)) {
                readFromVirtualMem(&ms, k, &w);
                CHKBOOLQUIT(w == j + k % 13, "incorrect read");
                w = j + (k + 2)%13;
                writeToVirtualMem(&ms, k+sizeof(Word), w);
            }
            for(k = 300*1024; k > 0; k -= sizeof(Word)) {
                readFromVirtualMem(&ms, k, &w);
                CHKBOOLQUIT(w == j + k % 13, "incorrect read");
            }
        }
    }
}

int main() {
    CHKBOOLQUIT(2*PAGESIZE >= sizeof(Word)*(1 << VPNBITS), "a page cannot contain a page table");
    CHKBOOLQUIT(PTEBYTES == sizeof(Word), "PTEBYTES != sizeof(Word)");

    srand(10);
    unitTestCache();
    unitTestVirtualMemory();
    printf("SUCCESS!!!\n");
}
