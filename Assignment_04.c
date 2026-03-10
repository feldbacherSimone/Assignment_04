#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <unistd.h>

void* thread_airbag(void* arg) {
    for(;;)
    {
    
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
    }
    return NULL;
}

int main() {
    pthread_create(NULL, NULL, thread_airbag, NULL); 
    for(;;); 
    return EXIT_SUCCESS;
}