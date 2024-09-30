#include<stdio.h>
#include "umalloc.h"

//Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 *
 * STUDENT TODO:
 * Required to be completed for checkpoint 1:
 *
 *      - Ensure that the free block list is in the order you expect it to be in.
 *        As an example, if you maintain the free list in memory-address order,
 *        lowest address first, ensure that memory addresses strictly ascend as you
 *        traverse the free list.
 *
 *      - Check if any free memory_blocks overlap with each other. 
 *
 *      - Ensure that each memory_block is aligned. 
 * 
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {
    memory_block_t *cur = free_head;
    memory_block_t *prev = cur;
    if(cur!=NULL){
        while (cur) {
            if (is_allocated(cur)) {
                return -1;
            
            //Is every block in increasing address order?
            } else if(prev > cur){
                printf("Not in increasing-address order, Previous Block: %p, Next Block: %p\n"
                    , (void*)prev, (void*)cur);
                return -1;

            //Are there any overlapping free blocks?
            } else if((prev != cur) && ((void*)prev + 16 + get_size(prev) > (void*)cur)){
                printf("Free blocks are overlapped, Previous Block: %p, Next Block: %p\n"
                    , (void*)prev, (void*)cur);
                return -1;
            //Is every free block aligned?
            } else if((cur->block_metadata%16)!=0){
                printf("Free Block is not 16-byte aligned, Block: %p\n", (void*)cur);
                return -1;
            }
            prev = cur;
            cur = cur->next;
        }
    }
    return 0;
}