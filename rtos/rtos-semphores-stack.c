#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

typedef struct
{
    void (*thread_func)(void);
    uint32_t priority;
    uint32_t remaining_time;
    uint32_t thread_id;
    uint32_t waiting;
    uint32_t stack_size;
    uint32_t *stack_ptr;
} ThreadControlBlock;

#define NUM_THREADS 2
ThreadControlBlock thread_blocks[NUM_THREADS];
volatile uint32_t current_thread;

typedef struct
{
    uint32_t count;
} Semaphore;

Semaphore led_semaphore;

void semaphore_init(Semaphore *semaphore, uint32_t initial_count)
{
    semaphore->count = initial_count;
}

void semaphore_acquire(Semaphore *semaphore)
{
    while (semaphore->count == 0)
    {
        // Spin until the semaphore becomes available
        printf("Thread ID %d is waiting for the semaphore.\n", thread_blocks[current_thread].thread_id);
    }
    semaphore->count--;
}

void semaphore_release(Semaphore *semaphore)
{
    semaphore->count++;
}

void thread1_func(void)
{
    // Store the initial stack pointer
    uint32_t *initial_sp = thread_blocks[current_thread].stack_ptr;

    while (1)
    {
        // Check for stack overflow
        if (thread_blocks[current_thread].stack_ptr < (thread_blocks[current_thread].stack_ptr - thread_blocks[current_thread].stack_size))
        {
            printf("Thread ID %d encountered a stack overflow!\n", thread_blocks[current_thread].thread_id);
            // Handle the stack overflow situation (e.g., reset the system or terminate the thread)
            break;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore.\n", thread_blocks[current_thread].thread_id);
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
        semaphore_release(&led_semaphore);
        printf("Thread ID %d released the semaphore.\n", thread_blocks[current_thread].thread_id);
        yield(); // Context switch to the next thread
    }

    // Restore the initial stack pointer
    thread_blocks[current_thread].stack_ptr = initial_sp;
}

void thread2_func(void)
{
    // Store the initial stack pointer
    uint32_t *initial_sp = thread_blocks[current_thread].stack_ptr;

    while (1)
    {
        // Check for stack overflow
        if (thread_blocks[current_thread].stack_ptr < (thread_blocks[current_thread].stack_ptr - thread_blocks[current_thread].stack_size))
        {
            printf("Thread ID %d encountered a stack overflow!\n", thread_blocks[current_thread].thread_id);
            // Handle the stack overflow situation (e.g., reset the system or terminate the thread)
            break;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore.\n", thread_blocks[current_thread].thread_id);
        gpio_put(LED_PIN, 1);
        sleep_ms(200);
        gpio_put(LED_PIN, 0);
        sleep_ms(800);
        semaphore_release(&led_semaphore);
        printf("Thread ID %d released the semaphore.\n", thread_blocks[current_thread].thread_id);
        yield(); // Context switch to the next thread
    }

    // Restore the initial stack pointer
    thread_blocks[current_thread].stack_ptr = initial_sp;
}

void kernel_tick_handler(struct repeating_timer *t)
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

#define STACK_SIZE 1024 // Adjust the stack size as needed

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    semaphore_init(&led_semaphore, 1);

    // Allocate stacks for each thread
    uint32_t thread1_stack[STACK_SIZE / sizeof(uint32_t)];
    uint32_t thread2_stack[STACK_SIZE / sizeof(uint32_t)];

    thread_blocks[0].thread_func = thread1_func;
    thread_blocks[0].priority = 1000;
    thread_blocks[0].remaining_time = 0;
    thread_blocks[0].thread_id = 1;
    thread_blocks[0].waiting = 0;
    thread_blocks[0].stack_size = STACK_SIZE;
    thread_blocks[0].stack_ptr = thread1_stack + STACK_SIZE / sizeof(uint32_t); // Set the stack pointer to the top of the stack

    thread_blocks[1].thread_func = thread2_func;
    thread_blocks[1].priority = 2000;
    thread_blocks[1].remaining_time = 0;
    thread_blocks[1].thread_id = 2;
    thread_blocks[1].waiting = 0;
    thread_blocks[1].stack_size = STACK_SIZE;
    thread_blocks[1].stack_ptr = thread2_stack + STACK_SIZE / sizeof(uint32_t); // Set the stack pointer to the top of the stack

    current_thread = 0;

    uint32_t interval_us = 1000; // 1ms
    struct repeating_timer timer;
    add_repeating_timer_us(-interval_us, kernel_tick_handler, NULL, &timer);

    while (1)
    {
        printf("Thread ID %d | Priority %d\n", thread_blocks[current_thread].thread_id, thread_blocks[current_thread].priority);
        kernel_thread_scheduler();
    }

    return 0;
}
