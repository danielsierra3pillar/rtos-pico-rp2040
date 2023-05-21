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
ThreadControlBlock* thread_list = NULL;  // Linked list of thread control blocks
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
        yield();  // Release the CPU to allow other threads to execute
    }
    semaphore->count--;
}

void semaphore_release(Semaphore* semaphore) {
    if (semaphore->count < UINT32_MAX)  // Check for count overflow
        semaphore->count++;
}

void thread1_func(void) {
    while (1) {
        ThreadControlBlock* current_tcb = &thread_list[current_thread];

        if (current_tcb->state == THREAD_SUSPENDED) {
            printf("Thread ID %d is suspended.\n", current_tcb->thread_id);
            sleep_ms(1000);
            continue;
        } else if (current_tcb->state == THREAD_TERMINATED) {
            printf("Thread ID %d is terminated.\n", current_tcb->thread_id);
            sleep_ms(1000);
            return;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore. State: ", current_tcb->thread_id);

        switch (current_tcb->state) {
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
        printf("Thread ID %d released the semaphore. State:", current_tcb->thread_id);

        switch (current_tcb->state) {
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

        yield();  // Context switch to the next thread
    }
}

void thread2_func(void) {
    while (1) {
        ThreadControlBlock* current_tcb = &thread_list[current_thread];

        if (current_tcb->state == THREAD_SUSPENDED) {
            printf("Thread ID %d is suspended.\n", current_tcb->thread_id);
            sleep_ms(1000);
            continue;
        } else if (current_tcb->state == THREAD_TERMINATED) {
            printf("Thread ID %d is terminated.\n", current_tcb->thread_id);
            sleep_ms(1000);
            return;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore. State: ", current_tcb->thread_id);

        switch (current_tcb->state) {
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
        printf("Thread ID %d released the semaphore. State: ", current_tcb->thread_id);

        switch (current_tcb->state) {
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

        yield();  // Context switch to the next thread
    }
}

void kernel_tick_handler(struct repeating_timer* t) {
    static uint32_t tick = 0;
    tick++;

    ThreadControlBlock* tcb = thread_list;
    while (tcb != NULL) {
        if (tcb->thread_id != current_thread) {
            if (tick % tcb->priority == 0)
                tcb->remaining_time = 0;
        }
        tcb = tcb->next;
    }
}

void kernel_thread_scheduler() {
    uint32_t highest_priority = 0;
    uint32_t next_thread = current_thread;

    ThreadControlBlock* tcb = thread_list;
    while (tcb != NULL) {
        if (tcb->thread_id != current_thread) {
            if (tcb->priority > highest_priority && tcb->remaining_time == 0) {
                highest_priority = tcb->priority;
                next_thread = tcb->thread_id;
            }
        }
        tcb = tcb->next;
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

void create_thread(void (*thread_func)(void), uint32_t priority, uint32_t thread_id) {
    ThreadControlBlock* new_thread = (ThreadControlBlock*)malloc(sizeof(ThreadControlBlock));
    new_thread->thread_func = thread_func;
    new_thread->priority = priority;
    new_thread->remaining_time = 0;
    new_thread->thread_id = thread_id;
    new_thread->waiting = 0;
    new_thread->state = THREAD_RUNNING;
    new_thread->next = NULL;

    // Add the new thread to the end of the thread list
    if (thread_list == NULL) {
        thread_list = new_thread;
    } else {
        ThreadControlBlock* current_tcb = thread_list;
        while (current_tcb->next != NULL) {
            current_tcb = current_tcb->next;
        }
        current_tcb->next = new_thread;
    }
}

void suspend_thread(uint32_t thread_id) {
    ThreadControlBlock* tcb = thread_list;
    while (tcb != NULL) {
        if (tcb->thread_id == thread_id) {
            tcb->state = THREAD_SUSPENDED;
            return;
        }
        tcb = tcb->next;
    }
}

void resume_thread(uint32_t thread_id) {
    ThreadControlBlock* tcb = thread_list;
    while (tcb != NULL) {
        if (tcb->thread_id == thread_id) {
            tcb->state = THREAD_RUNNING;
            return;
        }
        tcb = tcb->next;
    }
}

void terminate_thread(uint32_t thread_id) {
    ThreadControlBlock* tcb = thread_list;
    while (tcb != NULL) {
        if (tcb->thread_id == thread_id) {
            tcb->state = THREAD_TERMINATED;
            return;
        }
        tcb = tcb->next;
    }
}

int main() {
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Initialize the LED semaphore
    semaphore_init(&led_semaphore, 1);

    // Create threads
    create_thread(thread1_func, 1, 0);
    create_thread(thread2_func, 2, 1);

    // Initialize and start the kernel timer
    struct repeating_timer kernel_timer;
    add_repeating_timer_ms(100, kernel_tick_handler, NULL, &kernel_timer);

    // Start the first thread
    current_thread = 0;
    thread_list[current_thread].thread_func();
    thread_list[current_thread].remaining_time = thread_list[current_thread].priority;

    while (1) {
        kernel_thread_scheduler();
    }

    return 0;
}
