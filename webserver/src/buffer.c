/**
 * @file buffer.c
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "../inc/buffer.h"

#include <linux/ipc.h>
#include <stdlib.h>
#include <fcntl.h>

int shmid = 0;

int buffer_init(struct shared_buffer *buffer)
{
    sem_t * sem = NULL;
    int i = 0;

    /* Creamos la región de memoria compartida */
    shmid = shmget(SHI_MEM_KEY, sizeof(struct shared_buffer), IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("Error en shmget");
        return -1;
    }

    /* Mapeamos la memoria compartida en el proceso */
    buffer = (struct shared_buffer *)shmat(shmid, NULL, 0);
    if (buffer == (struct shared_buffer *)-1)
    {
        perror("Error en shmat");
        shmctl(shmid, IPC_RMID, NULL); // Liberamos la memoria compartida
        return -1;
    }

    /* Inicializamos el semáforo */
    sem = &(buffer->sem);

    if ((sem = sem_open("sem", O_CREAT, 0644, 1)) == SEM_FAILED)
    {
        perror("Error en sem_open");
        shmdt(buffer);                  // Desmapeamos la memoria compartida
        shmctl(shmid, IPC_RMID, NULL);  // Liberamos la memoria compartida
        return -1;
    }

    /* Inicializamos el buffer */
    for (i = 0; i < BUFFER_SIZE; i++)
    {
        buffer->temp_celcius[i] = 0;
    }

    return 0;
}

int buffer_put(struct shared_buffer *buffer, float data)
{
    if(buffer == NULL)
    {
        perror("Error en buffer_put");
        return -1;
    }

    sem_wait(&(buffer->sem)); // Esperamos a que el semáforo esté libre

    // Actualizamos el buffer
    // El buffer es de tipo FIFO

    for (int i = 0; i < BUFFER_SIZE - 1; i++)
    {
        buffer->temp_celcius[i] = buffer->temp_celcius[i + 1];
    }

    buffer->temp_celcius[BUFFER_SIZE - 1] = data;

    sem_post(&(buffer->sem)); // Liberamos el semáforo
}

/**
 * @brief Lee el buffer
 * 
 * @param buffer 
 * @param data 
 * @return float 
 */
float buffer_get(struct shared_buffer *buffer, float *data)
{
    if(data == NULL)
    {
        return -1;
    }

    if(buffer == NULL)
    {
        return -1;
    }

    sem_wait(&(buffer->sem)); // Esperamos a que el semáforo esté libre

    // Leemos el buffer
    // El buffer es de tipo FIFO

    *data = buffer->temp_celcius[0];

    sem_post(&(buffer->sem)); // Liberamos el semáforo

    return 0;
}

/**
 * @brief Calcula el promedio de los datos del buffer
 * 
 * @param buffer 
 * @param data 
 * @return int 
 */
int buffer_avg(struct shared_buffer *buffer, float *data)
{
    float sum = 0;

    if(data == NULL)
    {
        return -1;
    }

    if(buffer == NULL)
    {
        return -1;
    }

    sem_wait(&(buffer->sem)); // Esperamos a que el semáforo esté libre

    // Calculamos el promedio

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        sum += buffer->temp_celcius[i];
    }

    sem_post(&(buffer->sem)); // Liberamos el semáforo

    *data = sum / BUFFER_SIZE;

    return 0;
}

void buffer_destroy(struct shared_buffer *buffer)
{
    sem_close(&(buffer->sem)); // Cerramos el semáforo

    sem_unlink("sem"); // Eliminamos el semáforo

    shmdt(buffer); // Desmapeamos la memoria compartida

    shmctl(shmid, IPC_RMID, NULL); // Liberamos la memoria compartida

    buffer = NULL;

}