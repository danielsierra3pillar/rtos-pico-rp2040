#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static QueueHandle_t xQueue = NULL;

void led_task(void *pvParameters)
{
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    uint uIValueToSend = 0;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Obtain task handle and information
    TaskHandle_t taskHandle = xTaskGetCurrentTaskHandle();
    UBaseType_t taskPriority = uxTaskPriorityGet(taskHandle);
    const char *taskName = pcTaskGetName(taskHandle);
    uint32_t taskID = (uint32_t)taskHandle;

    while (true)
    {
        gpio_put(LED_PIN, 1);
        uIValueToSend = 1;
        xQueueSend(xQueue, &uIValueToSend, 0U);

        // Print thread information
        printf("ID: %lu | Thread: %s | Priority: %u | State: Running\n", taskID, taskName, taskPriority);

        vTaskDelay(3000);

        gpio_put(LED_PIN, 0);
        uIValueToSend = 0;
        xQueueSend(xQueue, &uIValueToSend, 0U);

        // Print thread information
        printf("ID: %lu | Thread: %s | Priority: %u | State: Running\n", taskID, taskName, taskPriority);

        vTaskDelay(3000);
    }
}

void usb_task(void *pvParameters)
{
    uint uIReceivedValue;

    // Obtain task handle and information
    TaskHandle_t taskHandle = xTaskGetCurrentTaskHandle();
    UBaseType_t taskPriority = uxTaskPriorityGet(taskHandle);
    const char *taskName = pcTaskGetName(taskHandle);
    uint32_t taskID = (uint32_t)taskHandle;

    while (1)
    {
        xQueueReceive(xQueue, &uIReceivedValue, portMAX_DELAY);

        // Print thread information
        printf("ID: %lu | Thread: %s | Priority: %u | State: Running\n", taskID, taskName, taskPriority);

        if (uIReceivedValue == 1)
        {
            printf("LED is ON! \n");
        }
        if (uIReceivedValue == 0)
        {
            printf("LED is OFF! \n");
        }
    }
}

int main()
{
    stdio_init_all();

    xQueue = xQueueCreate(1, sizeof(uint));

    // Create the LED task
    xTaskCreate(led_task, "LED_Task", 256, NULL, 1, NULL);

    // Create the USB task
    xTaskCreate(usb_task, "USB_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1)
    {
    };
}
