#include <stdio.h>

//Blank
unsigned short white_block[225];
//Fruit
unsigned short red_block[225];
//Block
unsigned short black_block[225];
//Boundary
unsigned short blue_block[1440];


// Initializes the colour array declared above
void colour_init(void) {
	unsigned int i;
	for (i=0;i<144;i++) {
		white_block[i] = White;
		red_block[i] = Red;
		black_block[i] = Black;
	}
	for (i=0;i<1440;i++) {
		blue_block[i] = Blue;
	}
}
