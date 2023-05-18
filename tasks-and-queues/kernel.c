#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

// Task control block structure
typedef struct
{
    void (*task_func)(void); // Function pointer to the task function
    uint32_t delay;          // Delay in milliseconds
    uint32_t remaining_time; // Remaining time for task execution
} TaskControlBlock;

#define NUM_TASKS 2
TaskControlBlock task_blocks[NUM_TASKS];

void task1_func(void)
{
    while (1)
    {
        printf("Task 1: LED ON\n");
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        printf("Task 1: LED OFF\n");
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
}

void task2_func(void)
{
    while (1)
    {
        printf("Task 2: LED ON\n");
        gpio_put(LED_PIN, 1);
        sleep_ms(200);
        printf("Task 2: LED OFF\n");
        gpio_put(LED_PIN, 0);
        sleep_ms(800);
    }
}

void kernel_tick_handler()
{
    for (int i = 0; i < NUM_TASKS; i++)
    {
        if (task_blocks[i].remaining_time > 0)
            task_blocks[i].remaining_time--;
    }
}

void kernel_task_scheduler()
{
    for (int i = 0; i < NUM_TASKS; i++)
    {
        if (task_blocks[i].remaining_time == 0)
        {
            printf("Executing Task %d\n", i + 1);
            task_blocks[i].task_func();
            task_blocks[i].remaining_time = task_blocks[i].delay;
        }
    }
}

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Configure a periodic interrupt for the kernel tick
    uint32_t interval = 1000; // 1ms
    uint32_t tick = 100;      // 100ms
    gpio_set_function(26, GPIO_FUNC_TIMER);
    timer_init(TIMER_IRQ_0);
    timer_set_alarm_value(TIMER_IRQ_0, interval - 1);
    timer_set_ticks(TIMER_IRQ_0, tick - 1);
    timer_set_irq_enabled(TIMER_IRQ_0, true);
    irq_set_exclusive_handler(TIMER_IRQ_0, kernel_tick_handler);
    irq_set_enabled(TIMER_IRQ_0, true);

    // Initialize task control blocks
    task_blocks[0].task_func = task1_func;
    task_blocks[0].delay = 1000; // 1 second
    task_blocks[0].remaining_time = task_blocks[0].delay;

    task_blocks[1].task_func = task2_func;
    task_blocks[1].delay = 2000; // 2 seconds
    task_blocks[1].remaining_time = task_blocks[1].delay;

    while (1)
    {
        kernel_task_scheduler();
    }

    return 0;
}
