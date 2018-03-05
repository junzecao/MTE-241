// Junze Cao 20618673
// Scottie Yu 20604499


#include <stdio.h>
#include <cstdint>
#include <RTL.h>
#include "MPU9250.h"
#include "led.h"
#include "sensor_fusion.h"

// Contains task IDs at run-time
OS_TID id_reading, id_processing, id_writing;

// Tell the algorithm what our sample frequency will be
// It has been noticed that,
// high value results less drift, but the IMU becomes less responsive
// 50 seems like a number that's not too high or too low
const unsigned short frequency = 100;

// Forward declaration of tasks
__task void tsk_reading (void);
__task void tsk_processing(void);
__task void tsk_writing(void);


// Set up and initialize the hardware
void setup() {
    // Initialize the MPU 9250
    MPU9250_init(1,1);
    // Initialize and tell the algorithm what our sample frequency will be
    sensor_fusion_init();
    sensor_fusion_begin(frequency);
    // Call before using LED
    LED_setup();
    // Test if communication with the MPU 9250 is working correctly
    // If it is, the LED should display 0x71
    LED_display(MPU9250_whoami());

}

// Task1 (reading)
// This is the task initialized in main()
// Creates task2 (processing), begins event flags
// Reads raw IMU data
__task void tsk_reading (void) {

    // Obtains own system task ID
    id_reading = os_tsk_self();
    // Creates task2 (processing), obtains its task ID
    id_processing = os_tsk_create(tsk_processing, 0);
    
    for (;;) {
        // Reads raw IMU data
        MPU9250_read_gyro();
        MPU9250_read_acc();
        MPU9250_read_mag();
        
        // Signals to task2 (processing) that task1 has completed
        os_evt_set(0x0004, id_processing);
        // Waits for completion of other task (task3 writing)
        // 0xFFFF makes it wait without timeout
        os_evt_wait_or(0x0004, 0xFFFF);
    }
}

// Task2 (processing)
// Creates task3 (writing)
// Sensor fusion algorithm to compute pitch, yaw and roll
__task void tsk_processing (void) {
    
    // Creates task 3 (writing), obtains its task ID
    id_writing = os_tsk_create(tsk_writing, 0);

    for(;;){
        // Waits for completion of other task (task 1 reading)
        os_evt_wait_or(0x0004, 0xFFFF);
        
        // Compute pitch, yaw and roll from raw data
        sensor_fusion_update(
            MPU9250_gyro_data[0],
            MPU9250_gyro_data[1],
            MPU9250_gyro_data[2],
            MPU9250_accel_data[0],
            MPU9250_accel_data[1],
            MPU9250_accel_data[2],
            MPU9250_mag_data[0],
            MPU9250_mag_data[1],
            MPU9250_mag_data[2]);

        // Signals to task3 (writing) that task2 has completed
        os_evt_set(0x0004, id_writing);
    }
}

// Task3 (writing)
// Send pitch, yaw and roll values to the computer via the UART
__task void tsk_writing (void) {

    for(;;){

        // Waits for completion of other task (task 2 processing)
        os_evt_wait_or(0x0004, 0xFFFF);

        // Prints tp the application via serial port
        // The box uses right handed coordinates, so the
        // setup is the same as the box
        printf("%f,%f,%f\n", -sensor_fusion_getYaw(),-sensor_fusion_getPitch(), -sensor_fusion_getRoll());
        
        // Signals to task1 (reading) that task3 has completed
        os_evt_set(0x0004, id_reading);
    }
}



int main()
{
    // Set up and initialize the hardware
    setup();
    // Allows other tasks to print via serial port
    printf("0,0,0\n");
    // Starts the RTX kernel, and then create and execute task1 (reading)
    os_sys_init(tsk_reading);

    return 0;
}
