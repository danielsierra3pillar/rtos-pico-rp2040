#include <stdio.h>
#include <stdlib.h>
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
uint32_t execution_time = 0;

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
        }

        if (thread_blocks[current_thread].state == THREAD_TERMINATED)
        {
            printf("Thread ID %d is terminated.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            continue;
        }

        if (thread_blocks[current_thread].state == THREAD_RUNNING)
        {
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
            sleep_ms(3000);
            gpio_put(LED_PIN, 0);
            sleep_ms(1000);
            semaphore_release(&led_semaphore);
            printf("Thread ID %d released the semaphore. State: ", thread_blocks[current_thread].thread_id);

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
        }

        yield(); // Context switch to the next thread
    }
}

void thread2_func(void)
{
    while (1)
    {
        if (thread_blocks[current_thread].state == THREAD_SUSPENDED)
        {
            printf("Thread ID %d is suspended.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            continue;
        }

        if (thread_blocks[current_thread].state == THREAD_TERMINATED)
        {
            printf("Thread ID %d is terminated.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            continue;
        }

        if (thread_blocks[current_thread].state == THREAD_RUNNING)
        {

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
            sleep_ms(10000);
            gpio_put(LED_PIN, 0);
            sleep_ms(1000);
            semaphore_release(&led_semaphore);
            printf("Thread ID %d released the semaphore. State: ", thread_blocks[current_thread].thread_id);

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
        }
        yield(); // Context switch to the next thread
    }
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
    // Check if the current thread is suspended
    if (thread_blocks[current_thread].state == THREAD_SUSPENDED)
    {
        // Find the next available thread
        uint32_t next_thread = (current_thread + 1) % NUM_THREADS;
        while (thread_blocks[next_thread].state != THREAD_RUNNING)
        {
            next_thread = (next_thread + 1) % NUM_THREADS;
            // Check if all threads are suspended or terminated
            if (next_thread == current_thread)
            {
                printf("No active threads. Exiting.\n");
                sleep_ms(1000);
                exit(0);
            }
        }
        current_thread = next_thread;
    }

    // Execute the thread function
    thread_blocks[current_thread].thread_func();
    thread_blocks[current_thread].remaining_time = thread_blocks[current_thread].priority;
}

void suspend_thread(uint32_t thread_id)
{
    thread_blocks[thread_id].state = THREAD_SUSPENDED;
    printf("Thread ID %d suspended.\n", thread_id);
}

void resume_thread(uint32_t thread_id)
{
    thread_blocks[thread_id].state = THREAD_RUNNING;
    printf("Thread ID %d resumed.\n", thread_id);
}

void terminate_thread(uint32_t thread_id)
{
    thread_blocks[thread_id].state = THREAD_TERMINATED;
    printf("Thread ID %d terminated.\n", thread_id);
}

void yield()
{
    printf("execution_time -----> %d\n", execution_time / 1000);
    uint32_t next_thread = (current_thread + 1) % NUM_THREADS;
    current_thread = next_thread;

    printf("Context Switch: Thread ID %d | Priority %d\n", thread_blocks[current_thread].thread_id, thread_blocks[current_thread].priority);
    thread_blocks[current_thread].remaining_time = thread_blocks[current_thread].priority;

    if (execution_time == 30 * 1000)
    {
        suspend_thread(0);
    }
    else if (execution_time == 60 * 1000)
    {
        resume_thread(0);
    }
    else if (execution_time == 90 * 1000)
    {
        terminate_thread(0);
    }
    execution_time += 1000;
}

void init_threads()
{
    // thread 1
    thread_blocks[0].thread_func = thread1_func;
    thread_blocks[0].priority = 1000;
    thread_blocks[0].remaining_time = 0;
    thread_blocks[0].thread_id = 0;
    thread_blocks[0].waiting = 0;
    thread_blocks[0].state = THREAD_RUNNING;

    // thread 2
    thread_blocks[1].thread_func = thread2_func;
    thread_blocks[1].priority = 2000;
    thread_blocks[1].remaining_time = 0;
    thread_blocks[1].thread_id = 1;
    thread_blocks[1].waiting = 0;
    thread_blocks[1].state = THREAD_RUNNING;
}

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    semaphore_init(&led_semaphore, 1);

    current_thread = 0;

    init_threads();

    uint32_t interval_us = 1000; // 1ms
    struct repeating_timer timer;
    add_repeating_timer_us(-interval_us, kernel_tick_handler, NULL, &timer);

    while (1)
    {
        printf("Thread ID %d | Priority %d | ", thread_blocks[current_thread].thread_id, thread_blocks[current_thread].priority);
        kernel_thread_scheduler();
        sleep_ms(1000); // Introduce a delay of 1 second between iterations
    }

    return 0;
}
