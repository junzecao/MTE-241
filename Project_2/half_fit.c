#include "half_fit.h"
#include <lpc17xx.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "uart.h"

#define MAX_SIZE 32768
unsigned char array[MAX_SIZE] __attribute__ ((section(".ARM.__at_0x10000000"), zero_init));

// Bit-vector to track whether the buckets are empty
unsigned short bit_vector;

// Array of 11 buckets
DoublyLinkedList* bucket_array[11];

// Assumes no memory has been allocated
void half_init(void)
{
  // Populate header
  BlockHeader* init_BlockHeader = (BlockHeader*) 0x10000000;
  
  init_BlockHeader->previous_adjacent_block = 0;
  init_BlockHeader->next_adjacent_block = 0;
  init_BlockHeader->block_size = 1023;
  init_BlockHeader->allocated = 0;
  
  bit_vector = 0;
  
  insert_DoublyLinkedList(init_BlockHeader);
  
}


void* half_alloc(unsigned int size)
{
  // size is in bytes
  unsigned short block_size;
  unsigned short free_bucket;
  unsigned short largest_bucket;
  unsigned short new_block_size;
  
  BlockHeader* current_BlockHeader;
  BlockHeader* new_BlockHeader;
  BlockHeader* next_BlockHeader;
  DoublyLinkedList* current_DoublyLinkedList;
  
  if(!size || size > (MAX_SIZE-4)) return NULL;
  block_size = (size + 3)/32 + 1;
  
  free_bucket = find_free_bucket(block_size);
  if (free_bucket > 10) return NULL;
  
  largest_bucket = find_largest_bucket(free_bucket);
  if (largest_bucket == 11) return NULL;
  
  current_DoublyLinkedList = bucket_array[largest_bucket]; // The only reason using current_DoublyLinkedList is to get current_BlockHeader
  current_BlockHeader = (BlockHeader*)((char*)current_DoublyLinkedList - 4);
  
  new_block_size = current_BlockHeader->block_size + 1 - block_size;
  
  // Allocate memory
  
  current_BlockHeader->allocated = 1;
  current_BlockHeader->block_size = block_size - 1;
  delete_first_DoublyLinkedList(largest_bucket);
  
  if (!new_block_size)
    return (current_BlockHeader + 1);
  
  next_BlockHeader = index_to_pointer(current_BlockHeader->next_adjacent_block);
  
  new_BlockHeader = (BlockHeader*)((char*)current_BlockHeader + block_size*32);
  new_BlockHeader->allocated = 0;
  new_BlockHeader->block_size = new_block_size - 1;
  current_BlockHeader->next_adjacent_block = pointer_to_index(new_BlockHeader);
  new_BlockHeader->previous_adjacent_block = pointer_to_index(current_BlockHeader);
  
  // Testing if the leftover memory is at the end of the chunk
  if (current_BlockHeader != next_BlockHeader)
  {
    new_BlockHeader->next_adjacent_block = pointer_to_index(next_BlockHeader);
    next_BlockHeader->previous_adjacent_block = pointer_to_index(new_BlockHeader);
  }
  else
    new_BlockHeader->next_adjacent_block = pointer_to_index(new_BlockHeader);
  
  insert_DoublyLinkedList(new_BlockHeader);
  return (current_BlockHeader + 1);
  
}

void half_free(void* address)
{
  
  BlockHeader* current_BlockHeader = (BlockHeader*)address - 1;
  BlockHeader* previous_BlockHeader = index_to_pointer(current_BlockHeader->previous_adjacent_block);
  BlockHeader* next_BlockHeader = index_to_pointer(current_BlockHeader->next_adjacent_block);
  BlockHeader* next_next_BlockHeader = index_to_pointer(next_BlockHeader->next_adjacent_block);
  
  unsigned short new_block_size = current_BlockHeader->block_size + 1;
  
  if (!address) return;
  
  if (!previous_BlockHeader->allocated)
  {
    delete_DoublyLinkedList(previous_BlockHeader);
    new_block_size += (previous_BlockHeader->block_size + 1);
    
    // Checks if current_BlockHeader is at the end of memory
    if(current_BlockHeader != next_BlockHeader)
    {
      previous_BlockHeader->next_adjacent_block = pointer_to_index(next_BlockHeader);
      next_BlockHeader->previous_adjacent_block = pointer_to_index(previous_BlockHeader);
    }
    else
    {
      previous_BlockHeader->next_adjacent_block = pointer_to_index(previous_BlockHeader);
      //next_BlockHeader = previous_BlockHeader;
    }
    
    current_BlockHeader = previous_BlockHeader;
  }
  
  
  if (!next_BlockHeader->allocated)
  {
    delete_DoublyLinkedList(next_BlockHeader);
    new_block_size += (next_BlockHeader->block_size + 1);
    
    if(next_BlockHeader != index_to_pointer(next_BlockHeader->next_adjacent_block))
    {
      current_BlockHeader->next_adjacent_block = next_BlockHeader->next_adjacent_block;
      next_next_BlockHeader->previous_adjacent_block = pointer_to_index(current_BlockHeader);
    }
    else
      current_BlockHeader->next_adjacent_block = pointer_to_index(current_BlockHeader);
    
  }
  
  current_BlockHeader->allocated = 0;
  current_BlockHeader->block_size = new_block_size - 1;
  
  insert_DoublyLinkedList(current_BlockHeader);
  
}

unsigned short find_largest_bucket(unsigned short bucket)
{
  while (!(1<<bucket & bit_vector) && (bucket < 11))
    bucket++;
  return bucket;
}

void delete_first_DoublyLinkedList(unsigned short bucket)
{
  DoublyLinkedList* current_DoublyLinkedList = (DoublyLinkedList*)bucket_array[bucket];
  if (current_DoublyLinkedList->next_DoublyLinkedList)
    bucket_array[bucket] = (DoublyLinkedList*)current_DoublyLinkedList->next_DoublyLinkedList;
  else
    bit_vector &= ~(1<<bucket);
}

void delete_DoublyLinkedList(BlockHeader* current_BlockHeader)
{
  DoublyLinkedList* current_DoublyLinkedList = (DoublyLinkedList*)(current_BlockHeader + 1);
  unsigned short bucket = find_bucket(current_BlockHeader->block_size + 1);
  if (current_DoublyLinkedList->previous_DoublyLinkedList && current_DoublyLinkedList->next_DoublyLinkedList)
    current_DoublyLinkedList->previous_DoublyLinkedList->next_DoublyLinkedList = current_DoublyLinkedList->next_DoublyLinkedList;
  else if (current_DoublyLinkedList->previous_DoublyLinkedList)
    current_DoublyLinkedList->previous_DoublyLinkedList->next_DoublyLinkedList = NULL;
  else if (current_DoublyLinkedList->next_DoublyLinkedList)
    bucket_array[bucket] = current_DoublyLinkedList->next_DoublyLinkedList;
  else
    bit_vector &= ~(1<<bucket);
  
}

// Inserts a doubly linked list element to front of bucket list.
void insert_DoublyLinkedList(BlockHeader* current_BlockHeader)
{
  unsigned short bucket;
  unsigned short block_size;
  DoublyLinkedList* next_DoublyLinkedList;
  DoublyLinkedList* new_DoublyLinkedList = (DoublyLinkedList*)(current_BlockHeader + 1);
  
  block_size = current_BlockHeader->block_size + 1; // +1 to account for size variable always being 1 less than value
  bucket = find_bucket(block_size);
  
  new_DoublyLinkedList->previous_DoublyLinkedList = NULL;
  if (1<<bucket & bit_vector)
  {
    next_DoublyLinkedList = bucket_array[bucket];
    new_DoublyLinkedList->next_DoublyLinkedList = next_DoublyLinkedList;
    next_DoublyLinkedList->previous_DoublyLinkedList = new_DoublyLinkedList;
  }
  else
  {
    new_DoublyLinkedList->next_DoublyLinkedList = NULL;
    bit_vector |= 1<<bucket;
    bucket_array[bucket] = new_DoublyLinkedList;
  }
}


unsigned short find_free_bucket(unsigned int block_size)
{
  unsigned short bucket = 0;
  block_size--;
  
  while (block_size)
  {
    block_size >>= 1;
    bucket++;
  }
  return bucket;
}

unsigned short find_bucket(unsigned int block_size)
{
  unsigned short bucket = 0;
  while (block_size)
  {
    block_size >>= 1;
    bucket++;
  }
  return --bucket;
}



void* index_to_pointer(unsigned short index)
{
  // Size of char is 1 byte, this allows moving in units of bytes.
  return ((char*)0x10000000 + index*32);
}
unsigned short pointer_to_index(void* pointer)
{
  return (((unsigned long)pointer - 0x10000000)/32);
}


