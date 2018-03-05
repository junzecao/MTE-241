#ifndef HALF_FIT_H_
#define HALF_FIT_H_

/*
 * Author names:
 *   1.  uWaterloo User ID:  s94yu@edu.uwaterloo.ca
 *   2.  uWaterloo User ID:  j56cao@edu.uwaterloo.ca
 */

#define smlst_blk                       5 
#define smlst_blk_sz  ( 1 << smlst_blk ) 
#define lrgst_blk                       15 
#define lrgst_blk_sz    ( 1 << lrgst_blk ) 


typedef struct {
  
  unsigned int previous_adjacent_block : 10;
  unsigned int next_adjacent_block : 10;
  unsigned int block_size : 10;
  unsigned short allocated : 1;
  
} BlockHeader;

typedef struct DoublyLinkedList {
  
  struct DoublyLinkedList* previous_DoublyLinkedList;
  struct DoublyLinkedList* next_DoublyLinkedList;
  
} DoublyLinkedList;

void  half_init( void );
void *half_alloc( unsigned int );
void  half_free( void * );

unsigned short find_largest_bucket( unsigned short bucket );
void delete_first_DoublyLinkedList( unsigned short bucket );
void delete_DoublyLinkedList( BlockHeader* current_BlockHeader );
void insert_DoublyLinkedList( BlockHeader* current_BlockHeader );
unsigned short find_free_bucket( unsigned int block_size );
unsigned short find_bucket( unsigned int block_size );

void* index_to_pointer( unsigned short index );
unsigned short pointer_to_index( void* pointer );

#endif
