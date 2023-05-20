#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

typedef enum
{
    THREAD_RUNNING,
    THREAD_SUSPENDED,
    THREAD_TERMINATED,
} ThreadState;

typedef struct
{
    void (*thread_func)(void);
    uint32_t priority;
    uint32_t remaining_time;
    uint32_t thread_id;
    uint32_t waiting;
    ThreadState state;
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
        yield(); // Release the CPU to allow other threads to execute
    }
    semaphore->count--;
}

void semaphore_release(Semaphore *semaphore)
{
    if (semaphore->count < UINT32_MAX) // Check for count overflow
        semaphore->count++;
}

void thread1_func(void)
{
    while (1)
    {
        if (thread_blocks[current_thread].state == THREAD_SUSPENDED)
        {
            printf("Thread ID %d is suspended.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            continue;
        }
        else if (thread_blocks[current_thread].state == THREAD_TERMINATED)
        {
            printf("Thread ID %d is terminated.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            return;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore. State: ", thread_blocks[current_thread].thread_id);

        switch (thread_blocks[current_thread].state)
        {
        case THREAD_RUNNING:
            printf("Running\n");
            break;
        case THREAD_SUSPENDED:
            printf("Suspended\n");
            break;
        case THREAD_TERMINATED:
            printf("Terminated\n");
            break;
        }

        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
        semaphore_release(&led_semaphore);
        printf("Thread ID %d released the semaphore. State:", thread_blocks[current_thread].thread_id);

        switch (thread_blocks[current_thread].state)
        {
        case THREAD_RUNNING:
            printf("Running\n");
            break;
        case THREAD_SUSPENDED:
            printf("Suspended\n");
            break;
        case THREAD_TERMINATED:
            printf("Terminated\n");
            break;
        }

        yield(); // Context
