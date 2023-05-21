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

typedef struct {
    ThreadControlBlock* head;
    ThreadControlBlock* tail;
} ThreadList;

typedef struct {
    uint32_t count;
} Semaphore;

Semaphore led_semaphore;
ThreadList thread_list;
volatile uint32_t current_thread;

void semaphore_init(Semaphore* semaphore, uint32_t initial_count) {
    semaphore->count = initial_count;
}

void semaphore_acquire(Semaphore* semaphore) {
    while (semaphore->count == 0) {
        // Spin until the semaphore becomes available
        printf("Thread ID %d is waiting for the semaphore.\n", thread_list.head->thread_id);
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
        if (thread_list.head->state == THREAD_SUSPENDED) {
            printf("Thread ID %d is suspended.\n", thread_list.head->thread_id);
            sleep_ms(1000);
            continue;
        } else if (thread_list.head->state == THREAD_TERMINATED) {
            printf("Thread ID %d is terminated.\n", thread_list.head->thread_id);
            sleep_ms(1000);
            return;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore. State: ", thread_list.head->thread_id);

        switch (thread_list.head->state) {
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
        printf("Thread ID %d released the semaphore. State:", thread_list.head->thread_id);

        switch (thread_list.head->state) {
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
        if (thread_list.head->state == THREAD_SUSPENDED) {
            printf("Thread ID %d is suspended.\n", thread_list.head->thread_id);
            sleep_ms(1000);
            continue;
        } else if (thread_list.head->state == THREAD_TERMINATED) {
            printf("Thread ID %d is terminated.\n", thread_list.head->thread_id);
            sleep_ms(1000);
            return;
        }

        semaphore_acquire(&led_semaphore);
        printf("Thread ID %d acquired the semaphore. State: ", thread_list.head->thread_id);

        switch (thread_list.head->state) {
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
        printf("Thread ID %d released the semaphore. State: ", thread_list.head->thread_id);

        switch (thread_list.head->state) {
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

    ThreadControlBlock* current = thread_list.head;
    while (current != NULL) {
        if (current->remaining_time > 0)
            current->remaining_time--;
        current = current->next;
    }
}

void kernel_thread_scheduler() {
    uint32_t highest_priority = 0;
    ThreadControlBlock* next_thread = thread_list.head;
    ThreadControlBlock* current = thread_list.head;

    while (current != NULL) {
        if (current->priority > highest_priority && current->remaining_time == 0) {
            highest_priority = current->priority;
            next_thread = current;
        }
        current = current->next;
    }

    if (next_thread != thread_list.head) {
        thread_list.head = next_thread;
        printf("Context Switch: Thread ID %d | Priority %d\n", thread_list.head->thread_id, thread_list.head->priority);
        thread_list.head->thread_func();
        thread_list.head->remaining_time = thread_list.head->priority;
    } else {
        // All threads are suspended or terminated
        printf("No active threads. Exiting.\n");
        sleep_ms(1000);
        exit(0);
    }
}

void yield() {
    ThreadControlBlock* next_thread = thread_list.head->next;
    thread_list.head = next_thread;

    printf("Context Switch: Thread ID %d | Priority %d\n", thread_list.head->thread_id, thread_list.head->priority);
    thread_list.head->remaining_time = thread_list.head->priority;
}

void suspend_thread(uint32_t thread_id) {
    ThreadControlBlock* current = thread_list.head;

    while (current != NULL) {
        if (current->thread_id == thread_id) {
            current->state = THREAD_SUSPENDED;
            break;
        }
        current = current->next;
    }
}

void resume_thread(uint32_t thread_id) {
    ThreadControlBlock* current = thread_list.head;

    while (current != NULL) {
        if (current->thread_id == thread_id) {
            current->state = THREAD_RUNNING;
            break;
        }
        current = current->next;
    }
}

void terminate_thread(uint32_t thread_id) {
    ThreadControlBlock* current = thread_list.head;
    ThreadControlBlock* prev = NULL;

    while (current != NULL) {
        if (current->thread_id == thread_id) {
            if (prev != NULL) {
                prev->next = current->next;
            } else {
                thread_list.head = current->next;
            }
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }
}

void create_thread(void (*thread_func)(void), uint32_t priority) {
    ThreadControlBlock* tcb = malloc(sizeof(ThreadControlBlock));
    tcb->thread_func = thread_func;
    tcb->priority = priority;
    tcb->remaining_time = priority;
    tcb->thread_id = thread_list.tail != NULL ? thread_list.tail->thread_id + 1 : 1;
    tcb->state = THREAD_RUNNING;
    tcb->next = NULL;

    if (thread_list.head == NULL) {
        thread_list.head = tcb;
        thread_list.tail = tcb;
    } else {
        thread_list.tail->next = tcb;
        thread_list.tail = tcb;
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    semaphore_init(&led_semaphore, 1);

    thread_list.head = NULL;
    thread_list.tail = NULL;
    current_thread = 0;

    create_thread(thread1_func, 2);
    create_thread(thread2_func, 1);

    struct repeating_timer tick_timer;
    add_repeating_timer_us(1000, kernel_tick_handler, NULL, &tick_timer);

    while (1) {
        kernel_thread_scheduler();
    }

    return 0;
}
