#include <stdio.h>

#include "pico/stdlib.h"

#include "FreeRTOS.h"

#include "task.h"

void vBlinkTask()
{

    for (;;)
    {

        gpio_put(PICO_DEFAULT_LED_PIN, 1);

        printf("LED ON!\n");
        vTaskDelay(4000);

        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        printf("LED OFF!\n");
        vTaskDelay(4000);
    }
}

void main()
{

    gpio_init(PICO_DEFAULT_LED_PIN);

    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    xTaskCreate(vBlinkTask, "Blink Task", 128, NULL, 1, NULL);

    vTaskStartScheduler();
}