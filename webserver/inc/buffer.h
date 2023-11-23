#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <semaphore.h>

#define BUFFER_SIZE 20
#define SHI_MEM_KEY 0x123

typedef struct shared_buffer
{
    float temp_celsius[BUFFER_SIZE];
    float time[BUFFER_SIZE];
    sem_t * sem;
} shared_buffer;

int buffer_init(struct shared_buffer **buffer, int *shmid);
int buffer_put(struct shared_buffer *buffer, float data);
int buffer_avg(struct shared_buffer *buffer, float *data);
void buffer_destroy(struct shared_buffer **buffer, int shmid);
void print_buffer(struct shared_buffer *buffer);
int buffer_get_temp(struct shared_buffer *buffer , unsigned int position, float *data);
int buffer_get_time(struct shared_buffer *buffer , unsigned int position, float *data);

#endif /* BUFFER_H */ // Add comment here
