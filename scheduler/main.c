#include "pico/stdlib.h"

#include "FreeRTOS.h"

#include "task.h"

#include "queue.h"

 

QueueHandle_t blinkQueue;

 

void vBlinkReceiverTask() {

    for (;;) {

    int blinks = 0;

        if (xQueueReceive(blinkQueue, &blinks, 0) == pdPASS) {

            for (int i = 0; i < blinks; i++) {

                gpio_put(PICO_DEFAULT_LED_PIN, 1);

                vTaskDelay(200);

                gpio_put(PICO_DEFAULT_LED_PIN, 0);

                vTaskDelay(200);

            }

        }

    }

}

 

void vBlinkSenderTask() {

    int loops = 4;

    for (;;) {

        for (int i = 1; i <= loops; i++) {

            xQueueSend(blinkQueue, &i, 0);

            vTaskDelay(500 + (i * 500));

        }

    }

}

 

void main() {

    gpio_init(PICO_DEFAULT_LED_PIN);

    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    blinkQueue = xQueueCreate(1, sizeof(int));

    xTaskCreate(vBlinkSenderTask, "Blink Sender", 128, NULL, 1, NULL);

    xTaskCreate(vBlinkReceiverTask, "Blink Receiver", 128, NULL, 1, NULL);

    vTaskStartScheduler();

}

// #include <string.h>

// #include <stdio.h>

// #include <stdlib.h>

// #include "pico/stdlib.h"

// #include "FreeRTOS.h"

// #include "task.h"

// #include "message_buffer.h"

// const size_t BUFFER_SIZE = 32;

// void vReceiverTask(void *pvParameters)
// {

//     MessageBufferHandle_t buffer = (MessageBufferHandle_t)pvParameters;

//     size_t messageSize = BUFFER_SIZE - 4;

//     char *message = malloc(messageSize);

//     memset(message, '\0', messageSize);

//     size_t lengthReceived;

//     for (;;)
//     {

//         lengthReceived = xMessageBufferReceive(buffer, (void *)message, BUFFER_SIZE, 0);

//         if (lengthReceived > 0)
//         {

//             printf("length: %d, message: %s\n", lengthReceived, message);

//             memset(message, '\0', messageSize);
//         }
//     }
// }

// void vSenderTask(void *pvParameters)
// {

//     MessageBufferHandle_t buffer = (MessageBufferHandle_t)pvParameters;

//     char message[] = "FreeRTOS + Pi Pico";

//     for (;;)
//     {

//         xMessageBufferSend(buffer, (void *)message, strlen(message), 0);

//         vTaskDelay(1000);
//     }
// }

// void main()
// {

//     stdio_init_all();

//     busy_wait_ms(1000);

//     MessageBufferHandle_t buffer = xMessageBufferCreate(BUFFER_SIZE);

//     xTaskCreate(vSenderTask, "Sender", 128, (void *)buffer, 1, NULL);

//     xTaskCreate(vReceiverTask, "Receiver", 128, (void *)buffer, 1, NULL);

//     vTaskStartScheduler();
// }
