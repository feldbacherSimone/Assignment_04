#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define FLASH_SIZE (1024 * 1024)

#define T_SIZE1 3000
#define T_SIZE2 6000

typedef enum 
{
    NORMAL, 
    STANDBY, 
    FIRE,
} airbag_state;

static void slideWindow(int window[], int size, int newSample)
{
    for (int i = size - 1; i > 0; --i)
    {
        window[i] = window[i - 1];
    }
    window[0] = newSample;
}

static int determineDeltaV(int window[], int size)
{
    double deltaV = 0;

    for (int i = 0; i < size; i++)
    {
        deltaV += window[i];
    }
    return deltaV;
}

static int determineL(int window[], int size)
{

    int l = 0, dy = 0;
    int dx = 1;
    for (int i = 0; i < size - 1; i++)
    {
        dy = window[i + 1] - window[i];
        l += sqrt(dy * dy + dx * dx);
    }
    return (int)l;
}

static void processSample(int t1[], int size1, int t2[], int size2, int *deltaV, int *L)
{
    int currentSample = rand() % (10 + 1 + 30) - 30;
    printf("Current Sample: %d\n", currentSample);

    slideWindow(t1, size1, currentSample);
    slideWindow(t2, size2, currentSample);

    *deltaV = determineDeltaV(t1, size1);
    *L = determineL(t2, size2);

    printf("DeltaV: %d, L: %d\n", *deltaV, *L);
}

void* thread_airbag(void* arg) {

int t1[T_SIZE1] = {0};
    // Airbag system parameters 
    int t2[T_SIZE2] = {0};

    int deltaV1 = 100000;
    int deltaV2 = 700000;
    int LThreshold = 70000;

    int deltaV = 0;
    int L = 0;


    airbag_state state = NORMAL;


    struct timespec next; 
    clock_gettime(CLOCK_MONOTONIC, &next);
    for(;;)
    {
        switch (state)
        {
        case NORMAL:
            printf("NORMAL\n");
            processSample(t1, T_SIZE1, t2, T_SIZE2, &deltaV, &L);

            if (abs(deltaV) > deltaV1)
            {
                printf("go to STANDBY\n");
                state = STANDBY;
            }
            break;

        case STANDBY:
           printf("STANDBY\n");
            processSample(t1, T_SIZE1, t2, T_SIZE2, &deltaV, &L);
            if (abs(deltaV) > deltaV2 || L > LThreshold)
            {
                printf("go to FIRE\n");
                state = FIRE;
            }
            if (abs(deltaV) < deltaV1)
            {
                printf("go to NORMAL\n");
                state = NORMAL;
            }
            break;

        case FIRE:
            printf("FIRE AIRBAG\n");
            break;

        default:
            printf("Unknown State\n");
            break;
        }
        

        // Advance deadline by period

        next.tv_nsec += 1000000000; // 1 ms
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

    struct timespec next; 
    clock_gettime(CLOCK_MONOTONIC, &next);
    for(;;)
    {
        (void)flash_buffer[0];
        printf("Checking checksum...\n");

        next.tv_nsec += 100000000; // 10 ms
        if(next.tv_nsec >= 1000000000)
        {
            next.tv_sec += 1;
            next.tv_nsec -= 1000000000;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
    return NULL;
}

void* thread_diagnostic(void* arg) {
    struct timespec next; 
    clock_gettime(CLOCK_MONOTONIC, &next);
    for(;;)
    {
        printf("Running diagnostics...\n");
        next.tv_nsec += 500000000; // 5 ms
        if(next.tv_nsec >= 1000000000)
        {
            next.tv_sec += 1;
            next.tv_nsec -= 1000000000;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
    return NULL;
}

int main() {
    pthread_t airbag_thread;
    //pthread_t checksum_thread;
    //pthread_t diagnostic_thread;

    pthread_create(&airbag_thread, NULL, thread_airbag, NULL);
    //pthread_create(&checksum_thread, NULL, thread_checksum, NULL);
    //pthread_create(&diagnostic_thread, NULL, thread_diagnostic, NULL);

    for(;;); 
    return EXIT_SUCCESS;
}