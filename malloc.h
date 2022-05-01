/**
 * @file malloc.h
 * @author Anshul Kamath
 * @brief Implementation of C's malloc functions
 * @version 0.1
 * @date 2022-04-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MALLOC_H
#define MALLOC_H

#include <unistd.h>

/**
 * @brief Implementation of malloc in C. Allocates memory on the heap,
 *        which must be free'd using `free`
 * 
 * @param size The number of bytes needed to allocate
 * @return void* 
 */
void *malloc(size_t size);

/**
 * @brief Frees a dynamically allocated pointer on the heap
 * 
 * @param ptr The pointer to free
 */
void free(void *ptr);

#if DEBUG
/**
 * @brief returns true if and only if the heap is clear
 *        (no elements are allocated)
 * 
 * @return int 
 */
int is_heap_clear();
#endif

/**
 * @brief Allocates `count` contiguous objects, each of size `size` and
 *        returns a pointer to the first one
 * 
 * @param count The number of objects to allocate space for
 * @param size  The size of each object to allocate
 */
void *calloc(size_t count, size_t size);

/**
 * @brief Reallocates the memory for `ptr` to a new place with the size
 *        `size.`
 * 
 * @param ptr   A pointer to the memory to reallocate
 * @param size  The new size of the memory
 * @return void* 
 */
void *realloc(void *ptr, size_t size);

#endif
