#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

typedef enum {
    THREAD_RUNNING,
    THREAD_SUSPENDED,
    THREAD_TERMINATED,
} ThreadState;

typedef struct {
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

typedef struct {
    uint32_t count;
} Semaphore;

Semaphore led_semaphore;

void semaphore_init(Semaphore *semaphore, uint32_t initial_count) {
    semaphore->count = initial_count;
}

void semaphore_acquire(Semaphore *semaphore) {
    while (semaphore->count == 0) {
        // Spin until the semaphore becomes available
        printf("Thread ID %d is waiting for the semaphore.\n", thread_blocks[current_thread].thread_id);
        yield(); // Release the CPU to allow other threads to execute
    }
    semaphore->count--;
}

void semaphore_release(Semaphore *semaphore) {
    if (semaphore->count < UINT32_MAX) // Check for count overflow
        semaphore->count++;
}

void thread1_func(void) {
    while (1) {
        if (thread_blocks[current_thread].state == THREAD_SUSPENDED) {
            printf("Thread ID %d is suspended.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            continue;
        } else if (thread_blocks[current_thread].state == THREAD_TERMINATED) {
            printf("Thread ID %d is terminated.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            return;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore. State: ", thread_blocks[current_thread].thread_id);

        switch (thread_blocks[current_thread].state) {
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

        switch (thread_blocks[current_thread].state) {
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

void thread2_func(void) {
    while (1) {
        if (thread_blocks[current_thread].state == THREAD_SUSPENDED) {
            printf("Thread ID %d is suspended.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            continue;
        } else if (thread_blocks[current_thread].state == THREAD_TERMINATED) {
            printf("Thread ID %d is terminated.\n", thread_blocks[current_thread].thread_id);
            sleep_ms(1000);
            return;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore. State: ", thread_blocks[current_thread].thread_id);

        switch (thread_blocks[current_thread].state) {
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
        sleep_ms(200);
        semaphore_release(&led_semaphore);
        printf("Thread ID %d released the semaphore. State:", thread_blocks[current_thread].thread_id);

        switch (thread_blocks[current_thread].state) {
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

void kernel_tick_handler(repeating_timer_t *rt) {
    gpio_put(LED_PIN, 1);
    sleep_ms(100);
    gpio_put(LED_PIN, 0);
    sleep_ms(100);
    yield(); // Context switch to the next thread
}

void kernel_thread_scheduler() {
    uint32_t current_priority = 0xFFFFFFFF;
    uint32_t next_thread = current_thread;
    for (uint32_t i = 0; i < NUM_THREADS; i++) {
        if (thread_blocks[i].state == THREAD_RUNNING && thread_blocks[i].priority < current_priority &&
            thread_blocks[i].remaining_time > 0) {
            current_priority = thread_blocks[i].priority;
            next_thread = i;
        }
    }
    current_thread = next_thread;
}

void suspend_thread(uint32_t thread_id) {
    thread_blocks[thread_id].state = THREAD_SUSPENDED;
    printf("Thread ID %d suspended.\n", thread_id);
}

void resume_thread(uint32_t thread_id) {
    thread_blocks[thread_id].state = THREAD_RUNNING;
    printf("Thread ID %d resumed.\n", thread_id);
}

void terminate_thread(uint32_t thread_id) {
    thread_blocks[thread_id].state = THREAD_TERMINATED;
    printf("Thread ID %d terminated.\n", thread_id);
}


void yield()
{
    uint32_t next_thread = (current_thread + 1) % NUM_THREADS;
    current_thread = next_thread;

    printf("Context Switch: Thread ID %d | Priority %d\n", thread_blocks[current_thread].thread_id, thread_blocks[current_thread].priority);
    thread_blocks[current_thread].remaining_time = thread_blocks[current_thread].priority;
}

int main() {
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    semaphore_init(&led_semaphore, 1);

    thread_blocks[0].thread_func = thread1_func;
    thread_blocks[0].priority = 1000;
    thread_blocks[0].remaining_time = 0;
    thread_blocks[0].thread_id = 0;
    thread_blocks[0].waiting = 0;
    thread_blocks[0].state = THREAD_RUNNING;

    thread_blocks[1].thread_func = thread2_func;
    thread_blocks[1].priority = 2000;
    thread_blocks[1].remaining_time = 0;
    thread_blocks[1].thread_id = 1;
    thread_blocks[1].waiting = 0;
    thread_blocks[1].state = THREAD_RUNNING;

    current_thread = 0;

    struct repeating_timer kernel_timer;
    add_repeating_timer_ms(1000, kernel_tick_handler, NULL, &kernel_timer);

    int thread1_runs = 0;
    int thread2_runs = 0;

    while (1) {
        kernel_thread_scheduler();

        // Suspend thread 0 after running 10 times
        if (current_thread == 0 && thread1_runs >= 10) {
            suspend_thread(0);
        }

        // Suspend thread 1 after running 10 times
        if (current_thread == 1 && thread2_runs >= 10) {
            suspend_thread(1);
        }

        // Resume thread 0 after thread 1 is suspended
        if (current_thread == 0 && thread_blocks[1].state == THREAD_SUSPENDED) {
            resume_thread(0);
        }

        // Resume thread 1 after thread 0 is suspended
        if (current_thread == 1 && thread_blocks[0].state == THREAD_SUSPENDED) {
            resume_thread(1);
        }

        // Terminate thread 0 after resuming once
        if (current_thread == 0 && thread1_runs >= 11) {
            terminate_thread(0);
        }

        // Terminate thread 1 after resuming once
        if (current_thread == 1 && thread2_runs >= 11) {
            terminate_thread(1);
        }

        thread_blocks[current_thread].thread_func();

        if (current_thread == 0) {
            thread1_runs++;
        } else if (current_thread == 1) {
            thread2_runs++;
        }
    }

    return 0;
}



// Context Switch: Thread ID 0 | Priority 1000
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Context Switch: Thread ID 1 | Priority 2000
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 is suspended.
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 is suspended.
// Thread ID 1 resumed.
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 acquired the semaphore. State: Running
// Thread ID 1 released the semaphore. State: Running
// Thread ID 1 is terminated.
// Thread ID 0 resumed.
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 acquired the semaphore. State: Running
// Thread ID 0 released the semaphore. State: Running
// Thread ID 0 is terminated.
// No active threads. Exiting.
