#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "aes.h" 
#include <string.h>

#define FLASH_SIZE (1024 * 1024)
static uint8_t flash_segement[FLASH_SIZE];


static const uint8_t aes_key[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static const uint8_t aes_iv[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

#define T_SIZE1 6000
#define T_SIZE2 8000

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
    //printf("Current Sample: %d\n", currentSample);

    slideWindow(t1, size1, currentSample);
    slideWindow(t2, size2, currentSample);

    *deltaV = determineDeltaV(t1, size1);
    *L = determineL(t2, size2);

    //printf("DeltaV: %d, L: %d\n", *deltaV, *L);
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
            //printf("NORMAL\n");
            processSample(t1, T_SIZE1, t2, T_SIZE2, &deltaV, &L);

            if (abs(deltaV) > deltaV1)
            {
                //printf("go to STANDBY\n");
                state = STANDBY;
            }
            break;

        case STANDBY:
           //printf("STANDBY\n");
            processSample(t1, T_SIZE1, t2, T_SIZE2, &deltaV, &L);
            if (abs(deltaV) > deltaV2 || L > LThreshold)
            {
                //printf("go to FIRE\n");
                state = FIRE;
            }
            if (abs(deltaV) < deltaV1)
            {
                //printf("go to NORMAL\n");
                state = NORMAL;
            }
            break;

        case FIRE:
            //printf("FIRE AIRBAG\n");
            break;

        default:
            //printf("Unknown State\n");
            break;
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

uint16_t crc16(const uint8_t* data, size_t length){
    uint16_t crc = 0xFFFF; 
    for(size_t i = 0; i < length; i++){
        crc ^= (uint16_t)data[i] << 8; 

        for(int j = 0; j < 8; j++){
            if(crc & 0x8000){
                crc = (crc<< 1) ^ 0x1021; 
            }
            else {
                crc <<= 1; 
            }
        }
    }
    return crc; 
}

void encrypt_crc(uint16_t crc, uint8_t *out, size_t *out_len){

    struct AES_ctx ctx; 

    out[0] = (crc >> 8) & 0xFF;
    out[1] = crc & 0xFF;  

    AES_init_ctx_iv(&ctx, aes_key, aes_iv);

    AES_CTR_xcrypt_buffer(&ctx, out, 2);

    *out_len = 2; 
}

void* thread_checksum(void* arg) {
    memset(flash_segement, 0xA5, FLASH_SIZE);


    struct timespec next; 
    clock_gettime(CLOCK_MONOTONIC, &next);
    for(;;)
    {
        printf("Getting checksum...\n");

        uint16_t crc = crc16(flash_segement, FLASH_SIZE);

        uint8_t crc_encrypted[2]; 
        size_t encrypted_len = 0; 

        encrypt_crc(crc, crc_encrypted, &encrypted_len);

        // simulate send 
        printf("CRC16: 0x%04X, Encrypted CRC: 0x%02X%02X\n", crc, crc_encrypted[0], crc_encrypted[1]);

        //timing
        next.tv_nsec += 500000000; // 500 ms
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
        // simulate data reciving 
        uint8_t recived[2]; 
        memset(recived, 0x42, sizeof(recived));

        // decrypt with AES 
        
        struct AES_ctx ctx_in; 

        AES_init_ctx_iv(&ctx_in, aes_key, aes_iv);
        AES_CTR_xcrypt_buffer(&ctx_in, recived, sizeof(recived));

        // status 
        uint8_t status[2] = {0x10, 0x11};
        struct AES_ctx ctx_out; 
        AES_init_ctx_iv(&ctx_out, aes_key, aes_iv);
        AES_CTR_xcrypt_buffer(&ctx_out, status, sizeof(status));

        // simulate send 
        printf("Sending status: 0x%02X%02x\n", status[0], status[1]);

        //timing
        printf("Running diagnostics...\n");
        next.tv_nsec += 100000000; // 100 ms
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
    pthread_create(&airbag_thread, NULL, thread_airbag, NULL);
    pthread_setname_np(airbag_thread, "airbag");

    pthread_t checksum_thread;
    pthread_create(&checksum_thread, NULL, thread_checksum, NULL);
    pthread_setname_np(checksum_thread, "checksum"); 


    pthread_t diagnostic_thread;
    pthread_create(&diagnostic_thread, NULL, thread_diagnostic, NULL);
    pthread_setname_np(diagnostic_thread, "diagnostics");

    for(;;); 
    return EXIT_SUCCESS;
}