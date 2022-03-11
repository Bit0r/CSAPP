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
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "hybrid",
    /* First member's full name */
    "Bit0r",
    /* First member's email address */
    "nie_wang@outlook.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    "",
};

/* single word (4) or double word (8) alignment */
static const size_t alignment = 8;

/* rounds up to the nearest multiple of ALIGNMENT */
inline static size_t round_up(size_t size, size_t round_to) {
    return (size + round_to - 1) & -round_to;
}

static const size_t size_t_size = sizeof(size_t);
static size_t increment_min;

typedef struct block block;
struct block {
    size_t size;
    union {
        block *next;
        char payload[4];
    };
};

static block *find_fit(size_t size);
static block *find_pre(block *bp);
static void *place(block *bp, size_t size);
static void insert_after(block *pre, block *bp);
static void extend_heap(block *tail, size_t size);
static bool coalesce(block *bp);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    // 设置最小的sbrk参数
    increment_min = mem_pagesize();

    mem_sbrk(increment_min);
    block *bp = mem_heap_lo();
    // 初始化空闲链表的“哑头”
    bp->size = alignment / 2;
    bp->next = (block *)bp[1].payload;
    // 初始化真正的空闲链表，且留下“哑头块”和“尾哑块”
    bp = (block *)bp[1].payload;
    bp->size = mem_heapsize() - 2 * alignment;
    bp->next = NULL;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size = round_up(size + size_t_size, alignment);
    block *pre = find_fit(size);
    if (pre->next == NULL) {
        assert(pre != NULL);
        extend_heap(pre, size);
        pre = find_fit(size);
    }
    return place(pre, size);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    block *bp = ptr - size_t_size, *pre = find_pre(bp);
    insert_after(pre, bp);
    coalesce(pre->next);
    coalesce(pre);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return mm_malloc(size);
    } else if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t raw_size = size;
    block *bp = ptr - size_t_size, *bp_next;
    size = round_up(size + size_t_size, alignment);

    if (bp->size == size) {
        return ptr;
    } else if (bp->size > size) {
        bp_next = (void *)bp + size;
        bp_next->size = bp->size - size;
        mm_free(bp_next->payload);

        bp->size = size;
        return ptr;
    }

    block *pre = find_pre(bp);
    bp_next = (void *)bp + bp->size;
    if (pre->next == bp_next && bp->size + bp_next->size >= size) {
        place(pre, size - bp->size);
        bp->size = size;
    } else {
        ptr = mm_malloc(raw_size);
        memcpy(ptr, bp->payload, bp->size - size_t_size);
        mm_free(bp->payload);
    }
    return ptr;
}

static block *find_fit(size_t size) {
    block *pre;
    for (pre = mem_heap_lo(); pre->next != NULL; pre = pre->next) {
        if (pre->next->size >= size) {
            break;
        }
    }
    return pre;
}

static block *find_pre(block *bp) {
    block *pre;
    for (pre = mem_heap_lo(); pre->next != NULL; pre = pre->next) {
        assert(pre != pre->next);
        if (pre < bp && pre->next > bp) {
            return pre;
        }
    }
    return pre;
}

static void *place(block *pre, size_t size) {
    block *bp = pre->next, *bp_next = (void *)bp + size;

    if (bp->size == size) {
        assert(bp != NULL);
        pre->next = bp->next;
    } else {
        pre->next = bp_next;
        bp_next->size = bp->size - size;
        bp_next->next = bp->next;
    }

    bp->size = size;
    return bp->payload;
}

static void insert_after(block *pre, block *bp) {
    bp->next = pre->next;
    pre->next = bp;
}

static void extend_heap(block *tail, size_t size) {
    size = round_up(size, increment_min);

    block *bp = mem_heap_hi() - 3;
    mem_sbrk(size);
    tail->next = bp;

    bp->size = size;
    bp->next = NULL;
    coalesce(tail);
}

static bool coalesce(block *bp) {
    if (bp->next != NULL && bp->next == (void *)bp + bp->size) {
        bp->size = bp->size + bp->next->size;
        bp->next = bp->next->next;
        return true;
    } else {
        return false;
    }
}
