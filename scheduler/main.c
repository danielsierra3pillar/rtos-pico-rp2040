#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

const size_t BUFFER_SIZE = 32; // Define a constant size for the message buffer

void vReceiverTask(void *pvParameters) // Define a function for the receiver task
{
    MessageBufferHandle_t buffer = (MessageBufferHandle_t)pvParameters; // Get the message buffer handle from the input parameters

    size_t messageSize = BUFFER_SIZE - 4; // Define the size of the message buffer as BUFFER_SIZE - 4, since 4 bytes are used for metadata

    char *message = malloc(messageSize); // Allocate memory for the message buffer using malloc

    memset(message, '\0', messageSize); // Initialize the message buffer with null characters

    size_t lengthReceived; // Define a variable to hold the length of the message received

    for (;;) // Loop indefinitely
    {
        lengthReceived = xMessageBufferReceive(buffer, (void *)message, BUFFER_SIZE, 0); // Receive a message from the buffer and store it in the message buffer

        if (lengthReceived > 0) // If a message was received successfully
        {
            printf("length: %d, message: %s\n", lengthReceived, message); // Print the length and content of the received message to the console

            memset(message, '\0', messageSize); // Reset the message buffer with null characters
        }
    }
}

void vSenderTask(void *pvParameters) // Define a function for the sender task
{
    MessageBufferHandle_t buffer = (MessageBufferHandle_t)pvParameters; // Get the message buffer handle from the input parameters

    char message[] = "FreeRTOS + Pi Pico"; // Define a message to be sent

    for (;;) // Loop indefinitely
    {
        xMessageBufferSend(buffer, (void *)message, strlen(message), 0); // Send the message to the message buffer

        vTaskDelay(1000); // Delay the task for 1000 milliseconds
    }
}

void main() // Define the main function
{
    stdio_init_all(); // Initialize the standard input and output

    busy_wait_ms(1000); // Wait for 1000 milliseconds

    MessageBufferHandle_t buffer = xMessageBufferCreate(BUFFER_SIZE); // Create a message buffer of size BUFFER_SIZE

    xTaskCreate(vSenderTask, "Sender", 128, (void *)buffer, 1, NULL); // Create a sender task with priority 1

    xTaskCreate(vReceiverTask, "Receiver", 128, (void *)buffer, 1, NULL); // Create a receiver task with priority 1

    vTaskStartScheduler(); // Start the FreeRTOS task scheduler
}
