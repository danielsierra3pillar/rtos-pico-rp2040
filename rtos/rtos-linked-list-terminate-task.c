#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

typedef enum {
    THREAD_RUNNING,
    THREAD_SUSPENDED,
    THREAD_TERMINATED,
} ThreadState;

typedef struct ThreadControlBlock {
    void (*thread_func)(void);
    uint32_t priority;
    uint32_t remaining_time;
    uint32_t thread_id;
    uint32_t waiting;
    ThreadState state;
    struct ThreadControlBlock* next;
} ThreadControlBlock;

#define NUM_THREADS 2
ThreadControlBlock* thread_list = NULL;
volatile uint32_t current_thread;

typedef struct {
    uint32_t count;
} Semaphore;

Semaphore led_semaphore;

void semaphore_init(Semaphore* semaphore, uint32_t initial_count) {
    semaphore->count = initial_count;
}

void semaphore_acquire(Semaphore* semaphore) {
    while (semaphore->count == 0) {
        // Spin until the semaphore becomes available
        printf("Thread ID %d is waiting for the semaphore.\n", thread_list[current_thread].thread_id);
        yield(); // Release the CPU to allow other threads to execute
    }
    semaphore->count--;
}

void semaphore_release(Semaphore* semaphore) {
    if (semaphore->count < UINT32_MAX) // Check for count overflow
        semaphore->count++;
}

void thread1_func(void) {
    while (1) {
        if (thread_list[current_thread].state == THREAD_SUSPENDED) {
            printf("Thread ID %d is suspended.\n", thread_list[current_thread].thread_id);
            sleep_ms(1000);
            continue;
        } else if (thread_list[current_thread].state == THREAD_TERMINATED) {
            printf("Thread ID %d is terminated.\n", thread_list[current_thread].thread_id);
            sleep_ms(1000);
            return;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore. State: ", thread_list[current_thread].thread_id);

        switch (thread_list[current_thread].state) {
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
        printf("Thread ID %d released the semaphore. State:", thread_list[current_thread].thread_id);

        switch (thread_list[current_thread].state) {
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
        if (thread_list[current_thread].state == THREAD_SUSPENDED) {
            printf("Thread ID %d is suspended.\n", thread_list[current_thread].thread_id);
            sleep_ms(1000);
            continue;
        } else if (thread_list[current_thread].state == THREAD_TERMINATED) {
            printf("Thread ID %d is terminated.\n", thread_list[current_thread].thread_id);
            sleep_ms(1000);
            return;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore. State: ", thread_list[current_thread].thread_id);

        switch (thread_list[current_thread].state) {
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
        printf("Thread ID %d released the semaphore. State: ", thread_list[current_thread].thread_id);

        switch (thread_list[current_thread].state) {
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

void kernel_tick_handler(struct repeating_timer* t) {
    static uint32_t tick = 0;
    tick++;

    ThreadControlBlock* current = thread_list;
    while (current != NULL) {
        if (current->thread_id != current_thread) {
            if (tick % current->priority == 0)
                current->remaining_time = 0;
        }
        current = current->next;
    }
}

void kernel_thread_scheduler() {
    uint32_t highest_priority = 0;
    uint32_t next_thread = current_thread;

    ThreadControlBlock* current = thread_list;
    while (current != NULL) {
        if (current->thread_id != current_thread) {
            if (current->priority > highest_priority && current->remaining_time == 0) {
                highest_priority = current->priority;
                next_thread = current->thread_id;
            }
        }
        current = current->next;
    }

    if (next_thread != current_thread) {
        current_thread = next_thread;
        printf("Context Switch: Thread ID %d | Priority %d\n", thread_list[current_thread].thread_id,
               thread_list[current_thread].priority);
        thread_list[current_thread].thread_func();
        thread_list[current_thread].remaining_time = thread_list[current_thread].priority;
    } else {
        // All threads are suspended or terminated
        printf("No active threads. Exiting.\n");
        sleep_ms(1000);
        exit(0);
    }
}

void yield() {
    uint32_t next_thread = (current_thread + 1) % NUM_THREADS;
    current_thread = next_thread;

    printf("Context Switch: Thread ID %d | Priority %d\n", thread_list[current_thread].thread_id,
           thread_list[current_thread].priority);
    thread_list[current_thread].remaining_time = thread_list[current_thread].priority;
}

void create_thread(void (*thread_func)(void), uint32_t priority, uint32_t delay) {
    ThreadControlBlock* new_thread = malloc(sizeof(ThreadControlBlock));
    new_thread->thread_func = thread_func;
    new_thread->priority = priority;
    new_thread->remaining_time = delay;
    new_thread->thread_id = NUM_THREADS;
    new_thread->waiting = 0;
    new_thread->state = THREAD_RUNNING;
    new_thread->next = NULL;

    if (thread_list == NULL) {
        thread_list = new_thread;
    } else {
        ThreadControlBlock* current = thread_list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_thread;
    }
    NUM_THREADS++;
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    semaphore_init(&led_semaphore, 1);

    create_thread(thread1_func, 1, 0);
    create_thread(thread2_func, 2, 1000);

    struct repeating_timer tick_timer;
    add_repeating_timer_us(-1, kernel_tick_handler, NULL, &tick_timer);

    current_thread = 0;

    while (1) {
        kernel_thread_scheduler();
    }

    return 0;
}