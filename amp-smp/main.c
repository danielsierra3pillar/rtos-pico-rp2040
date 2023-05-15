#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t mutex;

// Function to safely print to console by taking a mutex lock
void vSafePrint(char *out)
{
    // Take the mutex lock
    xSemaphoreTake(mutex, portMAX_DELAY);
    // Print the output to the console
    puts(out);
    // Release the mutex lock
    xSemaphoreGive(mutex);
}

// Function to print task ID and timestamp
void vPrintTime(int task, uint32_t time)
{
    // Create a string to hold the output message
    char out[32];
    // Format the output message
    sprintf(out, "Task: %d, Timestamp: %d", task, time);
    // Print the output message safely
    vSafePrint(out);
}

// Timer callback function to notify a task with timestamp
bool vTimerCallback(struct repeating_timer *timer)
{
    // Get the current timestamp
    uint32_t time = time_us_32();
    // Get the handle of the task that set the timer
    TaskHandle_t handle = timer->user_data;
    // Notify the task with the timestamp as value
    xTaskNotifyFromISR(handle, time, eSetValueWithOverwrite, NULL);
    // Return true to keep the timer repeating
    return true;
}

// Function to wait for notifications from timer and print task ID and timestamp
void vNotifyTask(void *pvParameters)
{
    // Get the task ID from the parameter
    int task = (int)pvParameters;
    // Get the handle of the current task
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();
    // Create a repeating timer to trigger notifications
    struct repeating_timer timer;
    add_repeating_timer_ms(1000, vTimerCallback, (void *)handle, &timer);
    // Variable to hold the notification value
    uint32_t time;
    // Loop to wait for notifications and print timestamps
    for (;;)
    {
        // Wait for a notification from the timer
        xTaskNotifyWait(0, 0, &time, portMAX_DELAY);
        // Print the task ID and timestamp safely
        vPrintTime(task, time);
    }
}

// Main function
void main()
{
    // Initialize the standard input/output
    stdio_init_all();
    // Create a mutex for safe console printing
    mutex = xSemaphoreCreateMutex();
    // Create three tasks to wait for timer notifications
    xTaskCreate(vNotifyTask, "Notify Task 1", 256, (void *)1, 1, NULL);
    xTaskCreate(vNotifyTask, "Notify Task 2", 256, (void *)2, 1, NULL);
    xTaskCreate(vNotifyTask, "Notify Task 3", 256, (void *)3, 1, NULL);
    // Start the FreeRTOS scheduler
    vTaskStartScheduler();
}
