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
} ThreadControlBlock;

#define NUM_THREADS 2
ThreadControlBlock thread_blocks[NUM_THREADS];
volatile uint32_t current_thread;

typedef struct
{
    uint32_t count;
    uint32_t waiters;
} Semaphore;

Semaphore led_semaphore;
Semaphore condition_mutex;
Semaphore condition_sem;

void semaphore_init(Semaphore *semaphore, uint32_t initial_count)
{
    semaphore->count = initial_count;
    semaphore->waiters = 0;
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

void condition_init()
{
    semaphore_init(&condition_mutex, 1);
    semaphore_init(&condition_sem, 0);
}

void condition_wait()
{
    semaphore_acquire(&condition_mutex);
    condition_sem.waiters++;
    semaphore_release(&condition_mutex);

    semaphore_acquire(&condition_sem);
}

void condition_signal()
{
    semaphore_acquire(&condition_mutex);
    if (condition_sem.waiters > 0)
    {
        condition_sem.waiters--;
        semaphore_release(&condition_sem);
    }
    else
    {
        semaphore_release(&condition_mutex);
    }
}

void condition_broadcast()
{
    semaphore_acquire(&condition_mutex);
    for (int i = 0; i < condition_sem.waiters; i++)
    {
        semaphore_release(&condition_sem);
    }
    condition_sem.waiters = 0;
    semaphore_release(&condition_mutex);
}

void thread1_func(void)
{
    while (1)
    {
        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore.\n", thread_blocks[current_thread].thread_id);
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
        semaphore_release(&led_semaphore);
        printf("Thread ID %d released the semaphore.\n", thread_blocks[current_thread].thread_id);
        condition_signal(); // Signal the condition variable
        yield(); // Context switch to the next thread
    }
}

void thread2_func(void)
{
    while (1)
    {
        condition_wait(); // Wait for the condition variable
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

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    semaphore_init(&led_semaphore, 1);
    condition_init(); // Initialize the condition variable

    thread_blocks[0].thread_func = thread1_func;
    thread_blocks[0].priority = 1000;
    thread_blocks[0].remaining_time = 0;
    thread_blocks[0].thread_id = 1;
    thread_blocks[0].waiting = 0;

    thread_blocks[1].thread_func = thread2_func;
    thread_blocks[1].priority = 2000;
    thread_blocks[1].remaining_time = 0;
    thread_blocks[1].thread_id = 2;
    thread_blocks[1].waiting = 0;

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


// Semaphore condition_mutex is a semaphore used to protect the condition variable.
// Semaphore condition_sem is a semaphore used to block and wake up threads waiting on the condition variable.
// condition_init() initializes the condition variable by initializing the condition_mutex and condition_sem.
// condition_wait() is used by a thread to wait on the condition variable. It acquires the condition_mutex, increments the waiters count, and releases the condition_mutex. Then it waits on the condition_sem semaphore.
// condition_signal() is used to signal one waiting thread. It acquires the condition_mutex, checks if there are any waiting threads, and either releases the condition_sem or releases the condition_mutex.
// condition_broadcast() is used to wake up all waiting threads. It acquires the condition_mutex and releases the condition_sem for each waiting thread, resetting the waiters count to zero.


