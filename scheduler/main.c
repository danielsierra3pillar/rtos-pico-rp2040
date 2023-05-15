#include "pico/stdlib.h" // Standard library for Pico boards
#include "FreeRTOS.h"    // FreeRTOS library for multitasking
#include "task.h"        // Task management functions for FreeRTOS
#include "queue.h"       // Queue management functions for FreeRTOS

// Create a global variable to hold the queue handle
QueueHandle_t blinkQueue;

// Define a task to receive blinking instructions from the queue and blink the LED accordingly
void vBlinkReceiverTask()
{

    for (;;)
    { // Loop forever

        int blinks = 0; // Create a variable to hold the number of blinks

        // Check if there is a message in the queue and receive it if so
        if (xQueueReceive(blinkQueue, &blinks, 0) == pdPASS)
        {

            // Loop through the specified number of blinks and turn the LED on and off
            for (int i = 0; i < blinks; i++)
            {

                gpio_put(PICO_DEFAULT_LED_PIN, 1); // Turn the LED on

                vTaskDelay(200); // Delay for 200ms

                gpio_put(PICO_DEFAULT_LED_PIN, 0); // Turn the LED off

                vTaskDelay(200); // Delay for another 200ms
            }
        }
    }
}

// Define a task to send blinking instructions to the queue
void vBlinkSenderTask()
{

    int loops = 4; // Define the number of loops to execute

    for (;;)
    { // Loop forever

        // Loop through the specified number of loops and send messages to the queue
        for (int i = 1; i <= loops; i++)
        {

            xQueueSend(blinkQueue, &i, 0); // Send the message to the queue

            vTaskDelay(500 + (i * 500)); // Delay for a variable amount of time
        }
    }
}

// Main function
void main()
{

    gpio_init(PICO_DEFAULT_LED_PIN); // Initialize the GPIO pin connected to the LED

    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT); // Set the direction of the pin to output

    blinkQueue = xQueueCreate(1, sizeof(int)); // Create a queue with a length of 1 and a size of 4 bytes

    // Create two tasks: one to send messages to the queue and another to receive messages from the queue
    xTaskCreate(vBlinkSenderTask, "Blink Sender", 128, NULL, 1, NULL);
    xTaskCreate(vBlinkReceiverTask, "Blink Receiver", 128, NULL, 1, NULL);

    vTaskStartScheduler(); // Start the FreeRTOS scheduler
}
