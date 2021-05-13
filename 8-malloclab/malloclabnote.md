# Malloc Lab
## 做什么？
实现一个内存分配器

## 怎么做？
非常建议看完书后，自己写一遍，进步非常大，可以检测出你哪块理解不够深刻，可以将这块知识点吃的很透彻。在遇到瓶颈的时候看看人家怎么写的，不然写出的代码有局限性，结果都不太好。比如说我的方案只按照书上的提示走，没有考虑各种方法相结合所以具有局限性，博客可以看https://zhuanlan.zhihu.com/p/126341872 这位大佬的
### 隐式链表实现（按照9.9.12思路编写）
```c
/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
//隐式空闲链表，实现
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
/* */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst 

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    //line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp


/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp

static char *heap_listp;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *best_fit(size_t asize);
/* */
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    //四个W的空间，1个W是头,2个W的序言块，1个W的结尾块
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));//序言块头
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));//序言块尾
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));//结尾块
    heap_listp += (2*WSIZE);

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}
static void *extend_heap(size_t words){//新申请的空间，中的head会覆盖原有空间的结尾块，并自己申请一个新块
    char *bp;
    size_t size;

    size = (words % 2)? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == (void *)-1)
        return NULL;
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);

}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0){
        return NULL;
    }
    if(size <= DSIZE){
        asize = 2*DSIZE;//8字节满足对齐要求，8字节满足head和foot空间
    }else{
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }
    /*
    if((bp = find_fit(asize))!=NULL){
        place(bp, asize);
        return bp;
    }
    */
    if((bp = best_fit(asize))!=NULL){
        place(bp, asize);
        return bp;
    }
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

void *mm_realloc(void *ptr, size_t size){
    void *newptr;
    size_t oldsize = GET_SIZE(HDRP(ptr));
    if(ptr==NULL)
        return mm_malloc(size);
    if(oldsize == size)
        return ptr;
    if(size == 0){
        mm_free(ptr);
        return NULL;
    }
    if(!(newptr = mm_malloc(size))){
        return NULL;
    }
    if(size < oldsize)
        oldsize = size;
    memcpy(newptr, ptr, oldsize);
    mm_free(ptr);
    return newptr;
    
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(FTRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(FTRP(bp));

    if(prev_alloc && next_alloc){//case 1：两边都在用
        return bp;
    }
    else if(prev_alloc && !next_alloc){//case 2:下面空闲块
        size += (GET_SIZE(HDRP(NEXT_BLKP(bp))));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if(!prev_alloc && next_alloc){//case 3:上面空闲块
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {//case 4:两边都空闲块
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }
    return bp;

}
static void place(void *bp, size_t asize){
    size_t size = GET_SIZE(HDRP(bp));
    if((size - asize) >= 2*DSIZE){//剩下的空间足以构成一个最小块了。
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);//便于操作
        PUT(HDRP(bp), PACK(size - asize, 0));
        PUT(FTRP(bp), PACK(size - asize, 0));
    }
    else{//化为内部碎片
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
}
static void *find_fit(size_t asize){//首次适配
    void *bp;
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if((!GET_ALLOC(HDRP(bp))) && (GET_SIZE(HDRP(bp)) >= asize)){
            return bp;
        }
    }
    return NULL;
}

static void *best_fit(size_t asize){
    void *bp;
    size_t min_size, size;
    void *best_bp = NULL;
    int flag = 1;
    for(bp = heap_listp; (size = GET_SIZE(HDRP(bp))) > 0; bp = NEXT_BLKP(bp)){
        if((!GET_ALLOC(HDRP(bp))) && (size >= asize)){
            if(flag == 1){
                min_size = size;
                best_bp = bp;
                flag = 0;
                continue;
            }
            if(size < min_size){
                min_size = size;
                best_bp = bp;
            }
        }
    }
    return best_bp;
}
```
#### 重要函数、变量以及注意点和思路：
1. `static char *heap_listp`:
	一个标记变量相当于第0个块的指向有效载荷的指针，其中第0个块并不会被分配。
2. `static void *extend_heap(size_t words)`
	从堆栈中申请新的空间，本质上是将mem_brk指针后移。申请之后执行立即合并，查看新申请的空间在物理的前后是否存在可合并的额外空闲块。
3. `static void *coalesce(void *bp)`
	根据书中的`图9-40`来分别合并空闲块。
4. `static void place(void *bp, size_t asize)`
	分割空闲块，其中`(size - asize) >= 2*DSIZE`为什么是2个`DSIZE`因为对于这个分割器最小块的大小为`2*DSIZE`字节，从哪里可以看出来呢？在`mm_malloc`的`size <= DSIZE`和其下一行可以看出，在书中也说明了这一点。
5. `static void *find_fit(size_t asize)`
	首次匹配，逻辑简单，好实现。
6. `static void *best_fit(size_t asize)`
	最佳匹配，在首次匹配的基础上记录最优指针和大小。
7. `static char *mem_start_brk;  /* points to first byte of heap */`
	heap中的起始指针。
8. `static char *mem_brk;        /* points to last byte of heap */`
	该指针指向heap已经分配出去的空间中的最后一个字节后面的那个字节
9. `static char *mem_max_addr;   /* largest legal heap address */` 
	允许可以申请的空间的最大地址。该值固定，其大小在`config.h`中定义
##### 注意点：
1. 有了书中代码的提示其实并没有什么注意点，但是在写第二种方法时，发现还是了解的不够细致，比如
2. 在该模型下使用mm_malloc到底在底层就申请这么多空间吗？在哪里额外多申请了一个DSIZE来放foot和head。
3. 还有在`extend_heap`时，他还会新创立一个结尾块，那之前申请的结尾块会不会影响模型呢？答：新创立的空间的head会覆盖掉之前的结尾块，所以不用担心模型中会出现多个结尾块的问题。
### 显式空闲链表实现（书中9.9.13思路即显式链表+最佳匹配+LIFO）
```c
/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
//显式空闲链表+LIFO,序言块部分是16字节，4W,前驱后继都指向对应快的bp，这个方案只靠pred个succ指针来跳转方向
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
/* */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst 

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    //line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp


/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp


#define SUCC_INDEX(bp) ((char *)(bp) + WSIZE)
#define PRED_INDEX(bp) ((char *)(bp))

static char *heap_listp;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *best_fit(size_t asize);
static void* addlist(void *bp);//将合并的链表新增到头
/* */
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    //printf("进入mm_init\n");
    //6个W的空间，1个W是头,4个W的序言块，1个W的结尾块
    if((heap_listp = mem_sbrk(6*WSIZE)) == (void *)-1){
        //printf("退出mm_init_1\n");
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(2*DSIZE, 1));//序言块头
    PUT(heap_listp + (2*WSIZE), NULL);
    PUT(heap_listp + (3*WSIZE), NULL);
    PUT(heap_listp + (4*WSIZE), PACK(2*DSIZE, 1));//序言块尾
    PUT(heap_listp + (5*WSIZE), PACK(0, 1));//结尾块-1w

    heap_listp += (2*WSIZE);//相当于第一个块的bp

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
        //printf("退出mm_init_2\n");
        return -1;
    }
    //printf("退出mm_init_3\n");
    return 0;
}
static void *extend_heap(size_t words){
    char *bp;
    size_t size;
    //printf("进入extend_heap\n");
    size = (words % 2)? (words + 1) * WSIZE : words * WSIZE;
    if(size < 2*DSIZE)
        size = 2*DSIZE;//显式空闲链表最小空间为4W
    if((long)(bp = mem_sbrk(size)) == (void *)-1){
        //printf("退出extend_heap_1\n");
        return NULL;
    }
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    //printf("退出extend_heap_2\n");
    return coalesce(bp);

}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;
    //printf("进入mm_malloc\n");
    
    if(size == 0){
        //printf("退出mm_malloc\n");
        return NULL;
    }
    if(size < DSIZE){
        asize = 2*DSIZE;//8字节满足对齐要求，8字节满足head和foot空间
    }else{
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);//在这里多申请一个DISZE来存放foot和head
    }
    //printf("想找size:%d\n",asize);
    /*
    if((bp = find_fit(asize))!=NULL){
        //printf("退出mm_malloc_1\n");
        place(bp, asize);
        return bp;
    }
   */
    if((bp = best_fit(asize))!=NULL){
        place(bp, asize);
        //printf("退出mm_malloc_1\n");
        return bp;
    }
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL){
        //printf("退出mm_malloc_2\n");
        return NULL;
    }
    place(bp, asize);
    //printf("退出mm_malloc_3\n");
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    //printf("进入mm_free\n");
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    //LIFO

    //PUT(SUCC_INDEX(bp), GET(SUCC_INDEX(heap_listp)));//本节点的后继为原heap_listp的后继
    //PUT(PRED_INDEX(GET(SUCC_INDEX(heap_listp))), bp);//修改heap_listp的后继的前驱为本节点
    //PUT(PRED_INDEX(bp), heap_listp);//本节点的前驱为heap_listp
    //PUT(SUCC_INDEX(heap_listp), bp);//头指针的后继修改
    
    coalesce(bp);
    //printf("退出mm_free\n");
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

void *mm_realloc(void *ptr, size_t size){
    //printf("进入mm_realloc\n");
    
    void *newptr;
    size_t oldsize = GET_SIZE(HDRP(ptr));
    if(ptr==NULL){
        //printf("退出mm_realloc_1\n");
        return mm_malloc(size);
    }
    if(oldsize == size){
        //printf("退出mm_realloc_2\n");
        return ptr;    
    }
    if(size == 0){
        mm_free(ptr);
        //printf("退出mm_realloc_3\n");
        return NULL;
    }
    if(!(newptr = mm_malloc(size))){
        //printf("退出mm_realloc_4\n");
        return NULL;
    }
    if(size < oldsize)
        oldsize = size;
    memcpy(newptr, ptr, oldsize);
    mm_free(ptr);
    //printf("退出mm_realloc_5\n");
    return newptr;
    
}
static void* addlist(void *bp){//新的空闲块，foot和head已经填写完，需要在逻辑上挂到heap_listp后面
    //printf("进入addlist\n");
    unsigned int succ_num;
    //bp的前驱为heap_listp
    PUT(PRED_INDEX(bp), heap_listp);
    //bp的后继为原heap_listp的后继
    PUT(SUCC_INDEX(bp), GET(SUCC_INDEX(heap_listp)));//第一次运行时，heap_listp中的NULL后继就被继承到这个节点上
    //原heap_listp后继的前驱为bp
    if((succ_num = GET(SUCC_INDEX(heap_listp)))!=NULL)
        PUT(PRED_INDEX(succ_num), bp);
    //heap_listp的后继为bp
    PUT(SUCC_INDEX(heap_listp), bp);
    //printf("退出addlist\n");
    return bp;
}

static void *coalesce(void *bp){//物理上相邻的将其变为逻辑上相邻
    //1.断掉物理上的所有块的逻辑上前后的指针
    //2.逻辑上其前驱后继更新。(addlist)
    //PREV_BLKP(bp)->GET(PRED_INDEX(bp))
    //NEXT_BLKP(bp)->GET(SUCC_INDEX(bp))
    //printf("进入coalesce\n");
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));//物理上的前驱
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));//物理上的后继,不能用foot来求，因为结尾快没有foot
    size_t size = GET_SIZE(FTRP(bp));
    void *bp1,*bp2,*bp3;
    //printf("%d\t%d\t\n",prev_alloc, next_alloc);
    if(prev_alloc && next_alloc){//case 1：两边都在用
        PUT(SUCC_INDEX(bp), NULL);
        PUT(PRED_INDEX(bp), NULL);
        //printf("退出coalesce\n");
    }
    else if(prev_alloc && !next_alloc){//case 2:下面空闲块
        //printf("qk2\n");
        bp1 = bp;
        bp2 = NEXT_BLKP(bp);
        size += (GET_SIZE(HDRP(bp2)));
        //这里是物理上将其相邻，逻辑上需要加pred和succ指针在addlist中
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        //这里的目的是断开逻辑上的块
        //第一块前驱的后继
        //PUT(SUCC_INDEX(GET(PRED_INDEX(bp1))), GET(SUCC_INDEX(bp1)));
        //第一块后继的前驱
        //PUT(PRED_INDEX(GET(SUCC_INDEX(bp1))), GET(PRED_INDEX(bp1)));
        //第二块前驱的后继
        PUT(SUCC_INDEX(GET(PRED_INDEX(bp2))), GET(SUCC_INDEX(bp2)));
        //第二块后继的前驱
        if(GET(SUCC_INDEX(bp2))!=NULL)
            PUT(PRED_INDEX(GET(SUCC_INDEX(bp2))), GET(PRED_INDEX(bp2)));
        PUT(SUCC_INDEX(bp), NULL);
        PUT(PRED_INDEX(bp), NULL);
    }
    else if(!prev_alloc && next_alloc){//case 3:上面空闲块
        //printf("qk3\n");
        bp1 = PREV_BLKP(bp);
        bp2 = bp;
        size += (GET_SIZE(HDRP(bp1)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        //第一块前驱的后继
        PUT(SUCC_INDEX(GET(PRED_INDEX(bp1))), GET(SUCC_INDEX(bp1)));
        //第一块后继的前驱
        if((GET(SUCC_INDEX(bp1)))!=NULL)//如果他没有后继（链表最后一块）则啥也不用管
            PUT(PRED_INDEX(GET(SUCC_INDEX(bp1))), GET(PRED_INDEX(bp1)));
        //第二块前驱的后继
        //PUT(SUCC_INDEX(GET(PRED_INDEX(bp2))), GET(SUCC_INDEX(bp2)));
        //第二块后继的前驱
        //PUT(PRED_INDEX(GET(SUCC_INDEX(bp2))), GET(PRED_INDEX(bp2)));
        bp = bp1;
        PUT(SUCC_INDEX(bp), NULL);
        PUT(PRED_INDEX(bp), NULL);
    }
    else {//case 4:两边都空闲块
        //printf("qk4\n");
        void *bp1 = PREV_BLKP(bp);//第一块
        void *bp2 = bp;//第二块
        void *bp3 = NEXT_BLKP(bp);//第三块

        size += (GET_SIZE(HDRP(bp1)) + GET_SIZE(HDRP(bp3)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        
        //第一块前驱的后继
        PUT(SUCC_INDEX(GET(PRED_INDEX(bp1))), GET(SUCC_INDEX(bp1)));

        //第一块后继的前驱
        if((GET(SUCC_INDEX(bp1)))!=NULL)
            PUT(PRED_INDEX(GET(SUCC_INDEX(bp1))), GET(PRED_INDEX(bp1)));
        //第二块前驱的后继
        //PUT(SUCC_INDEX(GET(PRED_INDEX(bp2))), GET(SUCC_INDEX(bp2)));
        //第二块后继的前驱
        //PUT(PRED_INDEX(GET(SUCC_INDEX(bp2))), GET(PRED_INDEX(bp2)));
        //第三块前驱的后继
        PUT(SUCC_INDEX(GET(PRED_INDEX(bp3))), GET(SUCC_INDEX(bp3)));
        //第三块后继的前驱
        if((GET(SUCC_INDEX(bp3)))!=NULL)
            PUT(PRED_INDEX(GET(SUCC_INDEX(bp3))), GET(PRED_INDEX(bp3)));
        bp = bp1;
        PUT(SUCC_INDEX(bp), NULL);
        PUT(PRED_INDEX(bp), NULL);
    }
    //printf("退出coalesce\n");
    return addlist(bp);

}
static void place(void *bp, size_t asize){
    //printf("进入place\n");
    size_t size = GET_SIZE(HDRP(bp));
    void *new_bp;
    if((size - asize) >= (2*DSIZE)){//剩下的空间足以构成一个最小块了。
        //printf("qka\n");
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        new_bp = NEXT_BLKP(bp);//根据长度，分割块
        PUT(HDRP(new_bp), PACK(size - asize, 0));
        PUT(FTRP(new_bp), PACK(size - asize, 0));

        PUT(PRED_INDEX(new_bp), GET(PRED_INDEX(bp)));//余下空闲部分的前驱为原本的前驱
        PUT(SUCC_INDEX(new_bp), GET(SUCC_INDEX(bp)));//余下空闲部分的后继为原本的后继
        PUT(SUCC_INDEX(GET(PRED_INDEX(bp))), new_bp);//前驱空闲快的后继为新bp
        if((GET(SUCC_INDEX(bp)))!=NULL)
            PUT(PRED_INDEX(GET(SUCC_INDEX(bp))), new_bp);//后继空闲快的前驱为新bp
        bp = new_bp;
    }
    else{//化为内部碎片 
        //printf("qkb\n");
        if(GET(SUCC_INDEX(bp))!=NULL)
            PUT(PRED_INDEX(GET(SUCC_INDEX(bp))), GET(PRED_INDEX(bp)));//修改后继的前驱(后继空闲快的前驱为自己的前驱)
        PUT(SUCC_INDEX(GET(PRED_INDEX(bp))), GET(SUCC_INDEX(bp)));//修改前驱的后继(前驱空闲快的后继为自己的后继)/如果此节点后继为NULL即，空闲链表自后一个节点，则这个节点的后继NULL则被逻辑上的pred节点继承
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
    //printf("退出place\n");
}
static void *find_fit(size_t asize){//首次适配
    //printf("进入find_fit\n");
    void *bp;
    for(bp = heap_listp; bp&&(GET_SIZE(HDRP(bp)) > 0); bp = GET(SUCC_INDEX(bp))){
        if((!GET_ALLOC(HDRP(bp))) && (GET_SIZE(HDRP(bp)) >= asize)){
            //printf("退出find_fit_1\n");
            return bp;
        }
    }
    //printf("退出find_fir_2\n");
    return NULL;
}

static void *best_fit(size_t asize){
    //printf("进入best_fit\n");
    void *bp;
    size_t min_size, size;
    void *best_bp = NULL;
    int flag = 1;
    for(bp = heap_listp; bp&&(size = GET_SIZE(HDRP(bp))) > 0;bp = GET(SUCC_INDEX(bp))){
        //printf("bp:%u\t%u\tsucc:%u\tpred:%u\n",bp,GET(bp),GET(SUCC_INDEX(bp)), GET(PRED_INDEX(bp)));
        if((!GET_ALLOC(HDRP(bp))) && (size >= asize)){
            if(flag == 1){
                min_size = size;
                best_bp = bp;
                flag = 0;
                continue;
            }
            if(size < min_size){
                min_size = size;
                best_bp = bp;
            }
        }
    }
    //printf("退出best_fit\n");
    return best_bp;
}
```
#### 重要函数、变量以及思路和注意点
##### 重要函数、变量
1. `static char *heap_listp`
2. `static void *extend_heap(size_t words)`
3. `static void *coalesce(void *bp)`
4. `static void place(void *bp, size_t asize)`
5. `static void *find_fit(size_t asize)`
6. `static void *best_fit(size_t asize)`
7. `static void* addlist(void *bp);//将合并的链表新增到头`
##### 注意点：
相较于上一个算法其改进点为，上一个算法的遍历方式是物理上临近的方式从一个遍历到下一个的，然后在判断是不是空闲的块。而这个算法管理空闲块的方式是通过`succ`和`pred`指针，即逻辑上的相连在常数时间内可以寻找到下一个空闲块。然后另一个不同是`heap_listp`所在块的大小为`2*DSIZE`也就是空闲链表的头指针为`heap_listp`。
##### 思路：
1. `int mm_init(void)`
	申请6个`WSIZE`，一W头，4W序言块，1W结尾块，初始的头指针的SUCC和PRED都为NULL，我一开始是将其指向了结尾块，结尾块也申请了4W的空间来放SUCC和PRED发现写完逻辑十分混乱，无法达到统一逻辑的情况，必须得考虑特殊地方。（也有可能当时存在其他BUG导致的）
2. `static void *extend_heap(size_t words)`
	与上一个算法相似，无特别改变之处。
3. `void *mm_malloc(size_t size)`
	这个函数的bug困扰我几天之久，当时使用` ./mdriver -t traces -V`测试时一共10个文件前4个文件会发生分配空间越界问题，后面的没有问题，重复查看逻辑了很多次，没有发现问题，于是最后想到和前面算法的代码进行对比，发现`if(size < DSIZE)`部分我写成了`if(size < 2*DSIZE)`，至今我也没搞清楚为啥吧2去掉就可以了。这个逻辑与上面算法一致。
4. `void mm_free(void *bp)`
	逻辑一致
5. `void *mm_realloc(void *ptr, size_t size)`
	逻辑一致
6. `static void* addlist(void *bp)`
	将新申请的空闲块挂到`heap_listp`的后面，要考虑原`heap_listp`的后继为`NULL`的问题。其余的就是将`SUCC`和`PRED`按顺序改变。
7. `static void *coalesce(void *bp)`
	逻辑上可能会很混乱的函数，注意在这个函数中要合并的是物理上相邻的块，断开和改变的是逻辑上的块，断开是将相邻的空闲块（已经在空闲链表的某个位置）将其与其前驱后继断开，然后物理上合并（改变foot和head）,然后挂到heap_listp后面。注意后继可能为NULL问题，可能会导致段错误。
8. `static void place(void *bp, size_t asize)`
	逻辑与上一个算法一致，只是需要在新出现的空闲块里，赋给其新的SUCC和PRED值。化为内部碎片情况时，需要概念其原本的前驱后继。
9. `static void *best_fit(size_t asize)`
	终止寻找的条件改为`bp`值为`NULL`,即到达空闲链表尾部
## 结果：
隐式空闲链表+最佳匹配
```
Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.008530   668
 1       yes   99%    5848  0.007493   781
 2       yes   99%    6648  0.011497   578
 3       yes  100%    5380  0.008861   607
 4       yes   66%   14400  0.000213 67669
 5       yes   96%    4800  0.014915   322
 6       yes   95%    4800  0.013938   344
 7       yes   55%   12000  0.158764    76
 8       yes   51%   24000  0.281268    85
 9       yes   31%   14401  0.153106    94
10       yes   30%   14401  0.002869  5020
Total          75%  112372  0.661453   170

Perf index = 45 (util) + 11 (thru) = 56/100
```
显式空闲链表+最佳匹配+LIFO
```
Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.000194 29381
 1       yes   99%    5848  0.000192 30427
 2       yes   99%    6648  0.000224 29652
 3       yes  100%    5380  0.000257 20901
 4       yes   66%   14400  0.000161 89164
 5       yes   96%    4800  0.002751  1745
 6       yes   95%    4800  0.002706  1774
 7       yes   55%   12000  0.025049   479
 8       yes   51%   24000  0.073440   327
 9       yes   31%   14401  0.147296    98
10       yes   30%   14401  0.002728  5278
Total          75%  112372  0.254998   441

Perf index = 45 (util) + 29 (thru) = 74/100
```
## 参考：
https://zhuanlan.zhihu.com/p/126341872
这位大佬的思路是我的+分离空闲链表。写的十分详细。膜拜^_^