#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BOOTSEL_BUTTON_PIN 15

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
    ThreadState state;
} ThreadControlBlock;

#define NUM_THREADS 2
ThreadControlBlock thread_blocks[NUM_THREADS];
volatile uint32_t current_thread;

volatile bool button_pressed = false;
volatile bool button_handled = false;

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
        sleep_ms(200);
        gpio_put(LED_PIN, 0);
        sleep_ms(800);
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
    else
    {
        // All threads are suspended or terminated
        printf("No active threads. Exiting.\n");
        sleep_ms(1000);
        exit(0);
    }
}

void suspend_thread(uint32_t thread_id)
{
    thread_blocks[thread_id].state = THREAD_SUSPENDED;
    printf("Thread ID %d suspended.\n", thread_id);
}

void terminate_thread(uint32_t thread_id)
{
    thread_blocks[thread_id].state = THREAD_TERMINATED;
    printf("Thread ID %d terminated.\n", thread_id);
}

void bootsel_button_handler()
{
    // Check if the button is pressed
    if (gpio_get(BOOTSEL_BUTTON_PIN) == 0)
    {
        button_pressed = true;
    }
    // Clear the interrupt flag
    gpio_acknowledge_irq(BOOTSEL_BUTTON_PIN);
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
    gpio_init(BOOTSEL_BUTTON_PIN);
    gpio_set_dir(BOOTSEL_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BOOTSEL_BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(BOOTSEL_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &bootsel_button_handler);

    for (int i = 0; i < NUM_THREADS; i++)
    {
        thread_blocks[i].thread_id = i;
        thread_blocks[i].state = THREAD_RUNNING;
        thread_blocks[i].remaining_time = 0;
    }

    semaphore_init(&led_semaphore, 1);

    thread_blocks[0].thread_func = &thread1_func;
    thread_blocks[0].priority = 1;
    thread_blocks[1].thread_func = &thread2_func;
    thread_blocks[1].priority = 2;

    current_thread = 0;
    thread_blocks[current_thread].remaining_time = thread_blocks[current_thread].priority;

    struct repeating_timer kernel_timer;
    add_repeating_timer_us(-1000, &kernel_tick_handler, NULL, &kernel_timer);

    while (1)
    {
        if (button_pressed && !button_handled)
        {
            suspend_thread(0);
            terminate_thread(1);
            button_handled = true;
        }

        kernel_thread_scheduler();
    }

    return 0;
}
