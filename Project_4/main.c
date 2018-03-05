#include <stdio.h>
#include <stdlib.h>
#include <LPC17xx.h>
#include <RTL.h>
#include "DoublyLinkedList.h"
#include "glcd.h"
#include "joystick.h"
#include "led.h"
#include "block.h"
#include "delay.h"

// bool is undefined, so define it
#define bool unsigned char

// To transform the x and y location stored to to location on the GLCD
#define X(a) (a*12+34)
#define Y(a) (a*12-6)


// Variable declaration
unsigned char move, pre_move;
unsigned char speed, length;
unsigned char fruit_x, fruit_y;
unsigned char del_x, del_y;
bool game_over, draw_fruit;

// Declare mutex, task ids and semephores
OS_MUT mutex;
OS_TID id_read, id_logic, id_draw, id_startup;
OS_SEM sem_logic, sem_draw, sem_startup;


// Forward declaration of the 4 tasks used in this project
__task void read (void);
__task void logic (void);
__task void draw (void);
__task void startup (void);

unsigned char find_bucket(unsigned char block_size)
{
  unsigned short bucket = 0;
  while (block_size)
  {
    block_size >>= 1;
    bucket++;
  }
  return --bucket;
}

// Initializes hardware
void hardware_init(void) {
	// Call before using LED
	LED_setup();
	// Call before using joystick
	joystick_setup();
	// Call before using push button
	pushbutton_setup();
	// Call before using the GLCD screen
	GLCD_Init();
	// Clear the screen to white for other features to be added on
	GLCD_Clear(White);
}

// Initializes the data structure
// Every time the game gets reset, this function gets to run
void software_init(void) {
	
	// Enable mutex to protect the data structure from corrupted reading and writing
	os_mut_wait(&mutex,0xFFFF);
	move = pre_move = 0;
	speed = length = 1;
	draw_fruit = 0;
	game_over = 0;
	// The initial location of the fruit is (12,7)
	fruit_x = 12;
	fruit_y = 7;
	
	// Nothing to delete from the Doubly Linked List
	del_x = del_y = 0;
	LED_display(0x01);
	// Clear the Doubly Linked List
    while (head) {
        delete_rear();
    }
		//
    add_front(3,5);
		// Releases the mutex to allow other tasks to perform operation
		os_mut_release(&mutex);
    GLCD_Clear(White);
		// Draw the 4 boundaries
    GLCD_Bitmap (40,0,240,6,(unsigned char*)blue_block);
    GLCD_Bitmap (40,0,6,240,(unsigned char*)blue_block);
    GLCD_Bitmap (40,234,240,6,(unsigned char*)blue_block);
    GLCD_Bitmap (274,0,6,240,(unsigned char*)blue_block);
		// Draw the head and the fruit
	GLCD_Bitmap(X(3),Y(5),12,12,(unsigned char*)black_block);
	GLCD_Bitmap(X(fruit_x),Y(fruit_y),12,12,(unsigned char*)red_block);
    
	
}

// Check if a new location is located within the snake (Doubly Linked List)
bool exist(char x, char y) {
	unsigned short i;
	node *temp = head;
	for (i=0;i<length;i++) {
		if (temp->x == x && temp->y == y)
			return 1;
		temp = temp->next;
	}
	return 0;
}

// This task constantly reads from the sensor and updates the value in the data structure
__task void read (void) {
	
	unsigned char move_reading;
	for (;;) {
		
		os_mut_wait(&mutex,0xFFFF);
		move_reading = joystick_read();
		// Break into cases, for different direction of movement
		switch(move_reading) {
			// 16 is up
			// Only a change in joystick will result in change in the moving direction
			case 16 :
				// It cannot be the exact opposite direction,
				// as that is how snake works
				if (pre_move != 1 && move != 2) {
					pre_move = 1;
					move = 1;
				}
				break;
			// 64 is down
			case 64 :
				if (pre_move != 2 && move != 1) {
					pre_move = 2;
					move = 2;
				}
				break;
			// 8 is left
			case 8 :
				if (pre_move != 3 && move != 4) {
					pre_move = 3;
					move = 3;
				}
				break;
			// 32 is right
			case 32 :
				if (pre_move != 4 && move !=3) {
					pre_move = 4;
					move = 4;
				}
				break;
		}
		
		os_mut_release(&mutex);
	}
}

// This task computes the result from the data strcture
// such as game_over, drawing a new fruit, etc.
__task void logic (void) {
	
	unsigned short del;
  
	for(;;) {
		os_sem_wait (&sem_logic, 0xffff);
		os_mut_wait(&mutex,0xFFFF);
		
		// Breaks into cases, for each direction of movement
		switch(move) {
			// Left
			case 3:
				// If the head of the snake touches the boundary
				if ((head->y)-1 == 0 || exist(head->x,(head->y)-1)) {
					game_over = 1;
				}
				// If the snake is about eat the fruit
				else if (fruit_x == head->x && fruit_y == (head->y)-1) {
					add_front(fruit_x,fruit_y);
					length++;
					draw_fruit = 1;
				}
				// The snake keeps on moving
				else {
					add_front(head->x,(head->y)-1);
					del = delete_rear();
					del_x = del % 32;
					del_y = del / 32;
				
				}
				break;
			// Right
			case 4:
				if ((head->y)+1 > 19 || exist(head->x,(head->y)+1)) {
					game_over = 1;
				}
				else if (fruit_x == head->x && fruit_y == (head->y)+1) {
					add_front(fruit_x,fruit_y);
					length++;
					draw_fruit = 1;
				}
				else {
					add_front(head->x,(head->y)+1);
					del = delete_rear();
					del_x = del % 32;
					del_y = del / 32;
				}
        break;
			// Up
			case 1:
				if ((head->x)+1 > 19 || exist((head->x)+1,head->y)) {
					game_over = 1;
				}
				else if (fruit_x == (head->x)+1 && fruit_y == head->y) {
					add_front(fruit_x,fruit_y);
					length++;
					draw_fruit = 1;
				}
				else {
					add_front((head->x)+1,head->y);
					del = delete_rear();
					del_x = del % 32;
					del_y = del / 32;
				}
        break;
			// Down
			case 2:
				if ((head->x)-1 == 0 || exist((head->x)-1,head->y)) {
					game_over = 1;
				}
				else if (fruit_x == (head->x)-1 && fruit_y == head->y) {
					add_front(fruit_x,fruit_y);
					length++;
					draw_fruit = 1;
				}
				else {
					add_front((head->x)-1,head->y);
					del = delete_rear();
					del_x = del % 32;
					del_y = del / 32;
				}
        break;
		}
		
		
		if (draw_fruit) {
			do {
				// The rand() function creates a number between 0 and 18
				// The limit is 1 to 19, so increment the number by 1
				fruit_x = rand() % 19;
        fruit_x++;
				fruit_y = rand() % 19;
        fruit_y++;
				// Check if the fruit generated is on the snake
				// If it is, re-generate another one
			} while(exist(fruit_x, fruit_y));
		}
		// This function turns the length into the speed
		speed = find_bucket(length);
		
		os_mut_release(&mutex);
		// Sends a signal to the draw task so the screen can get updated
		os_sem_send (&sem_draw);
		
		
	}
}




__task void draw (void) {
  unsigned char delay;
	for(;;) {
    // Waits for a signal from task logic
		os_sem_wait (&sem_draw, 0xffff);
		os_mut_wait(&mutex,0xFFFF);
		if (game_over) {
						// Displays GAME OVER
            GLCD_DisplayChar (4,8,1,'G');
            GLCD_DisplayChar (4,9,1,'A');
            GLCD_DisplayChar (4,10,1,'M');
            GLCD_DisplayChar (4,11,1,'E');
            GLCD_DisplayChar (5,8,1,'O');
            GLCD_DisplayChar (5,9,1,'V');
            GLCD_DisplayChar (5,10,1,'E');
            GLCD_DisplayChar (5,11,1,'R');
		} else {
			// Updates the LED according to speed
      LED_display(0x1 << (speed-1));
			// Add the new Snake block
			GLCD_Bitmap(X(head->x),Y(head->y),12,12,(unsigned char*)black_block);
			if (draw_fruit) {
				// Draw the new fruit block
				GLCD_Bitmap(X(fruit_x),Y(fruit_y),12,12,(unsigned char*)red_block);
				// Set it to 0 back to normal
				draw_fruit = 0;
			}
			if (del_x && del_y) {
				// Delete the old Snake block, so overwriting the block with blank colour
				GLCD_Bitmap(X(del_x),Y(del_y),12,12,(unsigned char*)white_block);
				// Set it normal so the program knows that there is nothing to delete afterwards
				del_x = del_y = 0;
			}
			
		}
		
		// The speed increases as the level goes up, meaning less delay time
    delay = speed;
		if (game_over) {
			// Sends a signal to the startup task so it can re-schedule all the tasks again
			os_sem_send (&sem_startup);
		} else {
			// Sends a signal to the logic task so it keeps on running
			os_sem_send (&sem_logic);
		}
		os_mut_release(&mutex);
		// Delay time is set according to the speed level
		DelayMilliseconds(1000-delay*100);
	}
}

__task void startup(void) {
  
	// Initializes the mutex
	os_mut_init(&mutex);
	// Assigns the task id to itself
	id_startup = os_tsk_self();
	// Initializes all semephores so they can be used to handle signal passing
  os_sem_init (&sem_logic, 0);
	os_sem_init (&sem_draw, 0);
	os_sem_init (&sem_startup, 0);
	
	// Create all remaining tasks and get their id numbers
  id_read = os_tsk_create(read, 0);
	id_logic = os_tsk_create(logic, 0);
	id_draw = os_tsk_create(draw, 0);
	
	// Initializes the hardware
	// This function only needs to get run once
	hardware_init();
	
	for (;;) {
		// Initializes the data structure
		software_init();
		// Wait for the joystick to keep running
		// The joystick starts the movement of the snake
		while(!joystick_read()) {}
		
		// At this point the tasks are ready to run, so pass the semephore to the logic task
		os_sem_send (&sem_logic);
			
		// Waits for the game to end, so it can re-intialize the data structure
		os_sem_wait (&sem_startup, 0xffff);
			
		// The screen and logic will not change unless the user presses the INT0 button
		while (pushbutton_read()) {}
			
		


	}
}

int main() {

	colour_init();
	
  //hardware_init();
	
	// Call the startup task to everything starts to run
	os_sys_init(startup);

	while(1);
	return 0;
	
}
