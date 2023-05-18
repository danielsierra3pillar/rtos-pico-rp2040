

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

// Thread control block structure
typedef struct {
    void (*thread_func)(void); // Function pointer to the thread function
    uint32_t priority;         // Priority of the thread
    uint32_t remaining_time;   // Remaining time for thread execution
    uint32_t thread_id;        // ID of the thread
} ThreadControlBlock;

#define NUM_THREADS 2
ThreadControlBlock thread_blocks[NUM_THREADS];
volatile uint32_t current_thread; // Index of the currently running thread

SemaphoreHandle_t binary_semaphore;

void thread1_func(void)
{
    while (1)
    {
        xSemaphoreTake(binary_semaphore, portMAX_DELAY);
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
        xSemaphoreGive(binary_semaphore);
        yield(); // Context switch to the next thread
    }
}

void thread2_func(void)
{
    while (1)
    {
        xSemaphoreTake(binary_semaphore, portMAX_DELAY);
        gpio_put(LED_PIN, 1);
        sleep_ms(200);
        gpio_put(LED_PIN, 0);
        sleep_ms(800);
        xSemaphoreGive(binary_semaphore);
        yield(); // Context switch to the next thread
    }
}

void kernel_tick_handler()
{
    static uint32_t tick = 0;

    // Increment the tick count
    tick++;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (i != current_thread) // Skip the currently running thread
        {
            if (tick % thread_blocks[i].priority == 0)
                thread_blocks[i].remaining_time = 0;
        }
    }
}

void kernel_thread_scheduler()
{
    uint32_t highest_priority = 0;
    uint32_t next_thread = 0;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (i != current_thread) // Skip the currently running thread
        {
            if (thread_blocks[i].priority > highest_priority && thread_blocks[i].remaining_time == 0)
            {
                highest_priority = thread_blocks[i].priority;
                next_thread = i;
            }
        }
    }

    if (highest_priority > 0)
    {
        current_thread = next_thread;
        thread_blocks[current_thread].thread_func();
        thread_blocks[current_thread].remaining_time = thread_blocks[current_thread].priority;
    }
}

void yield()
{
    // Perform context switch
    uint32_t next_thread = (current_thread + 1) % NUM_THREADS;
    current_thread = next_thread;

    printf("Context Switch: Thread ID %d | Priority %d\n", thread_blocks[current_thread].thread_id, thread_blocks[current_thread].priority);
    thread_blocks[current_thread].thread_func();
    thread_blocks[current_thread].remaining_time = thread_blocks[current_thread].priority;
}

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    binary_semaphore = xSemaphoreCreateBinary();

    // Initialize thread control blocks
    thread_blocks[0].thread_func = thread1_func;
    thread_blocks[0].priority = 1000; // High priority
    thread_blocks[0].remaining_time = 0;
    thread_blocks[0].thread_id = 1;

    thread_blocks[1].thread_func = thread2_func;
    thread_blocks[1].priority = 2000; // Low priority
    thread_blocks[1].remaining_time = 0;
    thread_blocks[1].thread_id = 2;

    current_thread = 0; // Start with the first thread

    while (1)
    {
        printf("Thread ID %d | Priority %d\n", thread_blocks[current_thread].thread_id, thread_blocks[current_thread].priority);
        kernel_thread_scheduler();
    }

    return 0;
}
