#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

// Define the pin for the LED
#define LED_PIN PICO_DEFAULT_LED_PIN

// Enum to represent thread states
typedef enum
{
    THREAD_RUNNING,
    THREAD_SUSPENDED,
    THREAD_TERMINATED,
} ThreadState;

// Structure to store thread information
typedef struct
{
    void (*thread_func)(void);
    uint32_t priority;
    uint32_t remaining_time;
    uint32_t thread_id;
    uint32_t waiting;
    ThreadState state;
} ThreadControlBlock;

// Number of threads
#define NUM_THREADS 2

// Array of thread control blocks
ThreadControlBlock thread_blocks[NUM_THREADS];

// Variable to track the current thread
volatile uint32_t current_thread;

// Variable to track execution time
uint32_t execution_time = 0;

// Structure representing a semaphore
typedef struct
{
    uint32_t count;
} Semaphore;

// Semaphore instance for controlling LED access
Semaphore led_semaphore;

// Function to initialize a semaphore
void semaphore_init(Semaphore *semaphore, uint32_t initial_count)
{
    semaphore->count = initial_count;
}

// Function to acquire a semaphore
void semaphore_acquire(Semaphore *semaphore)
{
    // Spin until the semaphore becomes available
    while (semaphore->count == 0)
    {
        printf("Thread ID %d is waiting for the semaphore.\n", thread_blocks[current_thread].thread_id);
        yield(); // Release the CPU to allow other threads to execute
    }
    semaphore->count--;
}

// Function to release a semaphore
void semaphore_release(Semaphore *semaphore)
{
    // Check for count overflow
    if (semaphore->count < UINT32_MAX)
        semaphore->count++;
}

// Thread 1 function
void thread1_func(void)
{
    while (1)
    {
        // Check if the thread is suspended
        if (thread_blocks[current_thread].state == THREAD_SUSPENDED)
        {
            printf("Thread ID %d is suspended.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
        }

        // Check if the thread is terminated
        if (thread_blocks[current_thread].state == THREAD_TERMINATED)
        {
            printf("Thread ID %d is terminated.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            continue; // Skip the remaining code in the loop and start over
        }

        // Check if the thread is running
        if (thread_blocks[current_thread].state == THREAD_RUNNING)
        {
            // Acquire the semaphore
            semaphore_acquire(&led_semaphore);
            printf("Thread ID %d acquired the semaphore. State: ", thread_blocks[current_thread].thread_id);

            // Print the state of the thread
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

            // Toggle the LED
            gpio_put(LED_PIN, 1);
            sleep_ms(300);
            gpio_put(LED_PIN, 0);
            sleep_ms(300);

            // Release the semaphore
            semaphore_release(&led_semaphore);
            printf("Thread ID %d released the semaphore. State: ", thread_blocks[current_thread].thread_id);

            // Print the state of the thread
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

        // Perform a context switch to the next thread
        yield();
    }
}

// Thread 2 function
void thread2_func(void)
{
    while (1)
    {
        // Check if the thread is suspended
        if (thread_blocks[current_thread].state == THREAD_SUSPENDED)
        {
            printf("Thread ID %d is suspended.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            continue; // Skip the remaining code in the loop and start over
        }

        // Check if the thread is terminated
        if (thread_blocks[current_thread].state == THREAD_TERMINATED)
        {
            printf("Thread ID %d is terminated.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            continue; // Skip the remaining code in the loop and start over
        }

        // Check if the thread is running
        if (thread_blocks[current_thread].state == THREAD_RUNNING)
        {
            // Acquire the semaphore
            semaphore_acquire(&led_semaphore);
            printf("Thread ID %d acquired the semaphore. State: ", thread_blocks[current_thread].thread_id);

            // Print the state of the thread
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

            // Toggle the LED
            gpio_put(LED_PIN, 1);
            sleep_ms(300);
            gpio_put(LED_PIN, 0);
            sleep_ms(300);

            // Release the semaphore
            semaphore_release(&led_semaphore);
            printf("Thread ID %d released the semaphore. State: ", thread_blocks[current_thread].thread_id);

            // Print the state of the thread
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

        // Perform a context switch to the next thread
        yield();
    }
}

// Kernel tick handler for timer interrupt
void kernel_tick_handler(struct repeating_timer *t)
{
    static uint32_t tick = 0;
    tick++;

    // Iterate through all threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        // Skip the current thread
        if (i != current_thread)
        {
            // Check if it's time to reset the remaining time of the thread
            if (tick % thread_blocks[i].priority == 0)
                thread_blocks[i].remaining_time = 0;
        }
    }
}

// Kernel thread scheduler
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

        // Switch to the next thread
        current_thread = next_thread;
    }
    else
    {
        // Find the next available thread based on priority
        uint32_t next_thread = (current_thread + 1) % NUM_THREADS;
        uint32_t min_priority = UINT32_MAX;
        while (thread_blocks[next_thread].state != THREAD_RUNNING || thread_blocks[next_thread].priority >= min_priority)
        {
            if (thread_blocks[next_thread].state == THREAD_RUNNING && thread_blocks[next_thread].priority < min_priority)
                min_priority = thread_blocks[next_thread].priority;

            next_thread = (next_thread + 1) % NUM_THREADS;

            // Check if all threads are suspended or terminated
            if (next_thread == current_thread)
            {
                printf("No active threads. Exiting.\n");
                sleep_ms(1000);
                exit(0);
            }
        }

        // Switch to the next thread
        current_thread = next_thread;
    }
}

// Function to initialize threads
void init_threads()
{
    // Initialize thread 1
    thread_blocks[0].thread_func = thread1_func;
    thread_blocks[0].priority = 3;
    thread_blocks[0].remaining_time = 0;
    thread_blocks[0].thread_id = 1;
    thread_blocks[0].waiting = 0;
    thread_blocks[0].state = THREAD_RUNNING;

    // Initialize thread 2
    thread_blocks[1].thread_func = thread2_func;
    thread_blocks[1].priority = 2;
    thread_blocks[1].remaining_time = 0;
    thread_blocks[1].thread_id = 2;
    thread_blocks[1].waiting = 0;
    thread_blocks[1].state = THREAD_RUNNING;

    // Set the current thread to thread 1
    current_thread = 0;
}

int main()
{
    // Initialize the LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Initialize the semaphore for LED access
    semaphore_init(&led_semaphore, 1);

    // Initialize the thread blocks
    init_threads();

    // Set up the timer interrupt for kernel tick
    struct repeating_timer kernel_timer;
    add_repeating_timer_us(1000, kernel_tick_handler, NULL, &kernel_timer);

    while (1)
    {
        // Execute the current thread function
        thread_blocks[current_thread].thread_func();

        // Perform thread scheduling
        kernel_thread_scheduler();

        // Update execution time
        execution_time++;

        // Check if the execution time has reached a threshold
        if (execution_time >= 1000)
        {
            printf("Execution time limit reached. Exiting.\n");
            sleep_ms(1000);
            exit(0);
        }
    }

    return 0;
}
