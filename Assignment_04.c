#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#define FLASH_SIZE (1024 * 1024)

void* thread_airbag(void* arg) {

    struct timespec next; 
    clock_gettime(CLOCK_MONOTONIC, &next);
    for(;;)
    {
    
        // Sample Accelerometer Data
         
        int random_number = rand() % 100 + 1;
        printf("Random number: %d\n", random_number);
        if(random_number <= 10)
        {
            printf("Airbag deployed!\n");
        }
        else
        {
            printf("Airbag not deployed.\n");
        }

        // Advance deadline by period

        next.tv_nsec += 1000000; // 1 ms
        if(next.tv_nsec >= 1000000000)
        {
            next.tv_sec += 1;
            next.tv_nsec -= 1000000000;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
   
    }
    return NULL;
}

void* thread_checksum(void* arg) {
    static uint8_t flash_buffer[FLASH_SIZE];
    for(;;)
    {
        (void)flash_buffer[0];
        printf("Checking checksum...\n");
        sleep(1); // Sleep for 1 second before checking again
    }
    return NULL;
}

void* thread_diagnostic(void* arg) {
    for(;;)
    {
        printf("Running diagnostics...\n");
        sleep(5); // Sleep for 5 seconds before running diagnostics again
    }
    return NULL;
}

int main() {
    pthread_t airbag_thread;
    pthread_t checksum_thread;
    pthread_t diagnostic_thread;

    pthread_create(&airbag_thread, NULL, thread_airbag, NULL);
    pthread_create(&checksum_thread, NULL, thread_checksum, NULL);
    pthread_create(&diagnostic_thread, NULL, thread_diagnostic, NULL);

    for(;;); 
    return EXIT_SUCCESS;
}