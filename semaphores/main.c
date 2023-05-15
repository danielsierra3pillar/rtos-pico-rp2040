#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t mutex; // Declare a semaphore handle

// Function to safely print to stdout using a semaphore
void vSafePrint(char *out)
{
    xSemaphoreTake(mutex, portMAX_DELAY); // Take the mutex semaphore

    puts(out); // Print the string

    xSemaphoreGive(mutex); // Give back the mutex semaphore
}

// Function to print the task ID and timestamp
void vPrintTime(int task, uint32_t time)
{
    char out[32];
    sprintf(out, "Task: %d, Timestamp: %d", task, time); // Format the output string
    vSafePrint(out);                                     // Safely print the output string
}

// Timer callback function to notify the task with the current timestamp
bool vTimerCallback(struct repeating_timer *timer)
{
    uint32_t time = time_us_32();                                   // Get the current timestamp in microseconds
    TaskHandle_t handle = timer->user_data;                         // Get the handle of the current task
    xTaskNotifyFromISR(handle, time, eSetValueWithOverwrite, NULL); // Notify the task with the timestamp
    return true;
}

// Task function to wait for notifications from the timer and print the timestamp
void vNotifyTask(void *pvParameters)
{
    int task = (int)pvParameters;                                         // Get the task ID from the parameter
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();                    // Get the handle of the current task
    struct repeating_timer timer;                                         // Declare a repeating timer structure
    add_repeating_timer_ms(1000, vTimerCallback, (void *)handle, &timer); // Add the repeating timer with the callback function
    uint32_t time;                                                        // Declare a variable to hold the timestamp
    for (;;)
    {
        xTaskNotifyWait(0, 0, &time, portMAX_DELAY); // Wait for a notification from the timer
        vPrintTime(task, time);                      // Print the task ID and timestamp
    }
}

void main()
{
    stdio_init_all();                                                   // Initialize the standard I/O library
    mutex = xSemaphoreCreateMutex();                                    // Create a mutex semaphore
    xTaskCreate(vNotifyTask, "Notify Task 1", 256, (void *)1, 1, NULL); // Create three tasks
    xTaskCreate(vNotifyTask, "Notify Task 2", 256, (void *)2, 1, NULL);
    xTaskCreate(vNotifyTask, "Notify Task 3", 256, (void *)3, 1, NULL);
    vTaskStartScheduler(); // Start the FreeRTOS scheduler
}
