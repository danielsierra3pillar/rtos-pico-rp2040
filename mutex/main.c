#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// Declare the mutex
static SemaphoreHandle_t mutex;

// Task 1 function
void task1(void *pvParameters)
{
    char ch = '1';
    while (true)
    {
        // Take the mutex and block until it's available
        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
        {
            // Print 'ch' character 10 times
            for (int i = 0; i < 10; i++)
            {
                putchar(ch);
            }
            // Print separator for clarity
            puts(" --- ");
            // Release the mutex
            xSemaphoreGive(mutex);
        }
    }
}

// Task 2 function
void task2(void *pvParameters)
{
    char ch = '2';
    while (true)
    {
        // Take the mutex and block until it's available
        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
        {
            // Print 'ch' character 10 times
            for (int i = 0; i < 10; i++)
            {
                putchar(ch);
            }
            // Print separator for clarity
            puts(" --- ");
            // Release the mutex
            xSemaphoreGive(mutex);
        }
    }
}

int main()
{
    // Initialize stdio library for printing to console
    stdio_init_all();

    // Create the mutex
    mutex = xSemaphoreCreateMutex();

    // Create task 1 with a priority of 1 and stack size of 256 words
    xTaskCreate(task1, "Task 1", 256, NULL, 1, NULL);
    // Create task 2 with a priority of 1 and stack size of 256 words
    xTaskCreate(task2, "Task 2", 256, NULL, 1, NULL);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    // This line should never be reached
    while (1)
    {
    };
}
