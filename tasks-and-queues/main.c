#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

typedef struct
{
    void (*thread_func)(void);
    uint32_t priority;
    uint32_t remaining_time;
    uint32_t thread_id;
} ThreadControlBlock;

#define NUM_THREADS 2
ThreadControlBlock thread_blocks[NUM_THREADS];
volatile uint32_t current_thread;

void thread1_func(void)
{
    while (1)
    {
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
        yield(); // Context switch to the next thread
    }
}

void thread2_func(void)
{
    while (1)
    {
        gpio_put(LED_PIN, 1);
        sleep_ms(200);
        gpio_put(LED_PIN, 0);
        sleep_ms(800);
        yield(); // Context switch to the next thread
    }
}

void kernel_tick_handler()
{
    static uint32_t tick = 0;
    tick++;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (i != current_thread)
        {
            if (tick % thread_blocks[i].priority == 0)
                thread_blocks[i].remaining_time = 0;
        }
    }
}

void kernel_thread_scheduler()
{
    uint32_t highest_priority = 0;
    uint32_t next_thread = current_thread;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (i != current_thread)
        {
            if (thread_blocks[i].priority > highest_priority && thread_blocks[i].remaining_time == 0)
            {
                highest_priority = thread_blocks[i].priority;
                next_thread = i;
            }
        }
    }

    if (next_thread != current_thread)
    {
        current_thread = next_thread;
        printf("Context Switch: Thread ID %d | Priority %d\n", thread_blocks[current_thread].thread_id, thread_blocks[current_thread].priority);
        thread_blocks[current_thread].thread_func();
        thread_blocks[current_thread].remaining_time = thread_blocks[current_thread].priority;
    }
}

void yield()
{
    uint32_t next_thread = (current_thread + 1) % NUM_THREADS;
    current_thread = next_thread;

    printf("Context Switch: Thread ID %d | Priority %d\n", thread_blocks[current_thread].thread_id, thread_blocks[current_thread].priority);
    thread_blocks[current_thread].remaining_time = thread_blocks[current_thread].priority;
}

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    thread_blocks[0].thread_func = thread1_func;
    thread_blocks[0].priority = 1000;
    thread_blocks[0].remaining_time = 0;
    thread_blocks[0].thread_id = 1;

    thread_blocks[1].thread_func = thread2_func;
    thread_blocks[1].priority = 2000;
    thread_blocks[1].remaining_time = 0;
    thread_blocks[1].thread_id = 2;

    current_thread = 0;

    while (1)
    {
        printf("Thread ID %d | Priority %d\n", thread_blocks[current_thread].thread_id, thread_blocks[current_thread].priority);
        kernel_thread_scheduler();
    }

    return 0;
}
