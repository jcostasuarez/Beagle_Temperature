/**
 * @file buffer.h
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <semaphore.h>

#define BUFFER_SIZE 5
#define SHI_MEM_KEY 0x444

struct shared_buffer
{
  float temp_celcius[BUFFER_SIZE];
  sem_t sem;
};

void buffer_init(struct shared_buffer *buffer);
void buffer_put(struct shared_buffer *buffer, float data);
float buffer_get(struct shared_buffer *buffer);
float buffer_avg(struct shared_buffer *buffer);
void buffer_destroy(struct shared_buffer *buffer);


#endif /* BUFFER_H */