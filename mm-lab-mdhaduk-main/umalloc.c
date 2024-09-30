#include "umalloc.h"
#include "ansicolors.h"
#include "csbrk.h"
#include <assert.h>
#include <stdio.h>

const char author[] =
ANSI_BOLD ANSI_COLOR_RED "Milan Dhaduk mpd2292" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

 // A sample pointer to the start of the free list.
memory_block_t* free_head;

/*
 * block_metadata - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t* block) {
    assert(block != NULL);
    return block->block_metadata & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t* block) {
    assert(block != NULL);
    block->block_metadata |= 0x1;
}

/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t* block) {
    assert(block != NULL);
    block->block_metadata &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t* block) {
    assert(block != NULL);
    return block->block_metadata & ~(ALIGNMENT - 1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t* get_next(memory_block_t* block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next
 * field.
 */
void put_block(memory_block_t* block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_metadata = size | alloc;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void* get_payload(memory_block_t* block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t* get_block(void* payload) {
    assert(payload != NULL);
    return ((memory_block_t*)payload) - 1;
}

/*
 * remove_from_list - Remove allocated block from free list
 */
void remove_from_list(memory_block_t* allocated_block) {
    memory_block_t* temp_free_head = free_head;
    memory_block_t* trailing_node = temp_free_head;

    // Removing head
    if (free_head == allocated_block) {
        free_head = free_head->next;

    // Removing any other node
    } else {
        while (temp_free_head != allocated_block) {
            trailing_node = temp_free_head;
            temp_free_head = temp_free_head->next;
        }

        trailing_node->next = temp_free_head->next;
    }
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required.
 */

 /*
  * find - finds a free block that can satisfy the umalloc request.
  */
memory_block_t* find(size_t size) {
    memory_block_t* cur = free_head;

    // First fit approach
    while (cur != NULL) {
        if (cur->block_metadata >= size) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t* extend(size_t size) {
    memory_block_t* new_block = (memory_block_t*)csbrk(size * 15);
    put_block(new_block, (size * 15) - sizeof(memory_block_t), false);

    // If freehead is empty
    if (free_head == NULL) {
        free_head = new_block;

    // Otherwise, go to end of free list and append the new block of memory
    } else {
        memory_block_t* temp_free_head = free_head;
        while (temp_free_head->next != NULL) {
            temp_free_head = temp_free_head->next;
        }
        temp_free_head->next = new_block;
    }

    return new_block;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t* split(memory_block_t* block, size_t size) {
    size_t block_payload_size = get_size(block);
    memory_block_t* next_block = block->next;

    // Size of new freeblock (header + payload)
    size_t free_size = block_payload_size - size - sizeof(memory_block_t);

    if (get_size(block) > (ALIGNMENT * 5) + size + sizeof(memory_block_t)) {
        // First half, FREE
        put_block(block, free_size, false);

        // Second half, ALLOCATED
        put_block((void*)block + sizeof(memory_block_t) + free_size, size, true);

        block->next = next_block;
        memory_block_t* alloc_block =
            (void*)block + sizeof(memory_block_t) + free_size;
        return alloc_block;
    }

    return NULL;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t* coalesce(memory_block_t* cur_block) {
    memory_block_t* temp_free_head = free_head;
    memory_block_t* trailing_node = temp_free_head;
    while (temp_free_head < cur_block) {
        trailing_node = temp_free_head;
        temp_free_head = temp_free_head->next;
    }

    // Is the free block before adjacent and not beginning of list?
    if (trailing_node != cur_block && (void*)trailing_node + get_size(trailing_node) + 
        sizeof(memory_block_t) == cur_block) {

        // Update size and next
        size_t new_size =
            get_size(trailing_node) + get_size(cur_block) + sizeof(memory_block_t);
        trailing_node->block_metadata = new_size | false;
        trailing_node->next = cur_block->next;

    // No coalesce, so must move trailing_node to cur_block
    } else {
        trailing_node = cur_block;
    }

    // Is the free block after cur_block adjacent and not end of free list?
    if (trailing_node->next != NULL && (void*)trailing_node + get_size(trailing_node) + 
        sizeof(memory_block_t) == trailing_node->next) {
        // Update size and next
        size_t new_size = get_size(trailing_node) + get_size(trailing_node->next) +
            sizeof(memory_block_t);
        trailing_node->block_metadata = new_size | false;
        trailing_node->next = trailing_node->next->next;
    }

    return trailing_node;
}

/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    free_head = (memory_block_t*)csbrk(PAGESIZE);
    put_block(free_head, PAGESIZE - 16, false);

    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated
 * memory.
 */
void* umalloc(size_t size) {
    memory_block_t* new_malloc = find(ALIGN(size));

    // No free blocks, extend
    if (new_malloc == NULL) {
        new_malloc = extend(PAGESIZE);
    }
    coalesce(new_malloc);
    memory_block_t* split_block = split(new_malloc, ALIGN(size));
    if (split_block != NULL) {
        return get_payload(split_block);
    } else {
        allocate(new_malloc);
        remove_from_list(new_malloc);
        new_malloc->next = NULL;
        return get_payload(new_malloc);
    }
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been
 * called by a previous call to malloc.
 */
void ufree(void* ptr) {
    memory_block_t* temp_free_head = free_head;
    memory_block_t* trailing_node = temp_free_head;

    memory_block_t* cur_block = get_block(ptr);
    if (is_allocated(cur_block)) {
        deallocate(cur_block);

        // Free list is empty
        if (free_head == NULL) {
            free_head = cur_block;
        } else {

            // Move until find free_block with address GREATER than address of block to be stored
            while (temp_free_head != NULL && temp_free_head < cur_block) {
                trailing_node = temp_free_head;
                temp_free_head = temp_free_head->next;
            }

            //Address is in between or after last element in free head
            if (trailing_node < cur_block) {
                cur_block->next = temp_free_head;
                trailing_node->next = cur_block;

            //Address of thing being freed is behind free head
            } else {
                cur_block->next = free_head;
                free_head = cur_block;
            }

            coalesce(cur_block);
        }
    }
}