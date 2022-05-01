#include "malloc.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

typedef struct meta_block meta_block_t;

#define META_SIZE sizeof(meta_block_t)
#define ALIGN 8
#define ALIGN_OFFSET(x) (ALIGN - ((x) % ALIGN))

typedef struct meta_block {
    size_t size;        // the number of bytes allocated to this block
    meta_block_t *next; // a pointer to the next block
    int is_free;        // a boolean indicating if this block is free
    int magic;          // a magic number for debugging
} meta_block_t;

static meta_block_t *global_base = NULL;

/**
 * @brief traverses the linked list to find the next block that can fit the given size
 * 
 * @param last a pointer to the element from which the req_size can be gotten
 * @param block_size a pointer to the block size of the coalesced free block
 * @param req_size the amount of data requested in the block
 * @return meta_block_t* 
 */
meta_block_t *find_free_block(meta_block_t **last, size_t *block_size, size_t req_size) {
    meta_block_t *curr = global_base;
    meta_block_t *res = curr;
    *block_size = curr && curr->is_free ? curr->size : 0;

    // traverse through linked list until we find an adequate
    // block, keeping track of the last block we saw
    while (curr && !(curr->is_free && (req_size <= *block_size))) {
        *last = res;
        
        curr = curr->next;

        // keep track of largest free block
        if (!(curr && res->is_free && curr->is_free)) res = curr;

        // allow for coalescing of blocks
        if (curr) {
            *block_size += curr->size;
            *block_size += (res != curr) * META_SIZE; // add the header size for successive blocks only
            *block_size *= curr->is_free;
        }
    }

    // aborted early since we found a block
    // in this case, we want to ensure that we coalesce all the free blocks we are using
    if (curr || (req_size <= *block_size)) {
        res->next = curr ? curr->next : NULL;
        res->size = *block_size;
        res->magic = 0x12344321;
        return res;
    }

    return curr;
}

/**
 * @brief requests space from the OS using sbrk
 * 
 * @param last a pointer to the last element in the linked list
 * @param block_size the number of bytes of free memory from the end of the list
 * @param size the number of bytes to allocate
 * @return meta_block_t* 
 */
meta_block_t *request_space(meta_block_t *last, size_t block_size, size_t size) {
    meta_block_t *new_block;

    // must be true from find_free_block
    assert(size + META_SIZE > block_size);

    // should be true since we only ever add data to heap in aligned fashion
    assert(block_size % ALIGN == 0);

    // making sure the new size is aligned
    size_t new_size = size - block_size;
    new_size += ALIGN_OFFSET(new_size);

    new_block = sbrk(0);
    void *req = sbrk(new_size + META_SIZE);
    
    if (req == (void *) -1) {
        printf("Error while allocating space on heap.\n");
        return NULL;
    }

    // if block_size is non-zero, then we can coalesce the last `n` blocks of the
    // linked list and update the size of the `-n`th block to fit the current size
    if (block_size) {
        assert(last->is_free);
        last->size = size + ALIGN_OFFSET(size);
        last->next = NULL; // get rid of all (empty) successive blocks
        return last;
    }

    // case where we cannot coalesce blocks to make new one --
    // must make new block from scratch

    // we can only add to the end of the list - never the middle
    // if we were able to add in the middle, then we must have found
    // a block in the `find_free_block` call
    if (last) last->next = new_block;
    new_block->size = new_size;
    new_block->is_free = 1;
    new_block->next = NULL;
    new_block->magic = 0x12345678; // magic number for debugging purposes
    
    return new_block;
}

/**
 * @brief if a block is added to the middle of the heap and needs less
 *        space than is allocated, split the free memory into 2 blocks
 * 
 * @param block a pointer to the block we added to the heap
 * @param block_size the amount of free space available
 * @param size  the size of the block that needs to be allocated
 */
void split_block(meta_block_t *block, size_t block_size, size_t size) {
    // block should not be NULL here - covered by request_space command
    assert(block != NULL);

    // block size should be at least as big as size
    assert(block_size >= size);

    // the address of where the DATA of the next block can start
    size_t next_addr = (size_t) block + META_SIZE + size;
    size_t align_factor = (ALIGN - (long int) next_addr % ALIGN);
    next_addr += align_factor;

    // if the there is no room for the header, we cannot split
    if (next_addr + META_SIZE >= (long int) (block + 1) + block_size) return;
    
    meta_block_t *next_block = (meta_block_t*) next_addr;
    // we are able to split
    next_block->is_free = 1;
    next_block->next = block->next;
    next_block->size = block_size - size - align_factor - META_SIZE;
    next_block->magic = 0x55555555;

    block->next = next_block;
    block->size = size + align_factor;
}

/**
 * @brief returns a pointer to the head of the actual data (with header)
 * 
 * @param ptr a pointer to the block on the heap with our data (w/o header)
 * @return meta_block_t* 
 */
meta_block_t *get_block_ptr(void *ptr) {
    return (meta_block_t *) ptr - 1;
}

void *malloc(size_t size) {
    // cannot allocate non-positive memory
    if (size <= 0) return NULL;

    meta_block_t *block;

    // if the global base is NULL, we must `initialize` the heap
    if (!global_base) {
        block = request_space(NULL, 0, size);
        if (!block) return NULL;
        global_base = block;
    } else {
        meta_block_t *last = global_base;
        size_t block_size = 0;

        // find a free block on the heap, if possible
        block = find_free_block(&last, &block_size, size);

        // if the block is null, no space is available, so we must request more space
        if (!block) {
            block = request_space(last, block_size, size);
            if (!block) return NULL;
        } else {
            // there is space on the heap already for the block
            block->magic = 0x77777777;
            block->is_free = 0;

            split_block(block, block_size, size);
        }
    }
    
    return block + 1;
}

void free(void *ptr) {
    if (!ptr) return;

    meta_block_t *block = get_block_ptr(ptr);
    block->is_free = 1;
    block->magic = 0xffffffff;
}

int is_heap_clear() {
    meta_block_t *curr = global_base;

    // check if everything is free on the heap
    while (curr) {
        if (!curr->is_free) return 0;
        curr = curr->next;
    }

    return 1;
}

void *calloc(size_t count, size_t size) {
    void *arr = malloc(count * size);
    memset(arr, 0, count * size);
    return arr;
}

void *realloc(void *ptr, size_t size) {
    size_t old_size = get_block_ptr(ptr)->size;
    void *temp[old_size];
    memcpy(temp, ptr, old_size);
    
    free(ptr);

    void *new_ptr = malloc(size);
    memcpy(new_ptr, temp, size);

    return new_ptr;
}
