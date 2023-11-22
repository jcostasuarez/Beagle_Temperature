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

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/shm.h>
#include <errno.h>

int buffer_init(struct shared_buffer **buffer, int *shmid)
{
    int i = 0;

    printf("Inicializando buffer de memoria compartida\n");
    
    /* Creamos la región de memoria compartida */
    *shmid = shmget(SHI_MEM_KEY, sizeof(struct shared_buffer), IPC_CREAT | 0666);
    if (*shmid < 0)
    {
        fprintf(stderr, "Error en shmget");
        return -1;
    }

    /* Mapeamos la memoria compartida en el proceso */
    *buffer = (struct shared_buffer *)shmat(*shmid, NULL, 0);
    if (*buffer == (struct shared_buffer *)-1)
    {
        fprintf(stderr, "Error en shmat");
        shmctl(*shmid, IPC_RMID, NULL); // Liberamos la memoria compartida
        return -1;
    }

    /* Inicializamos el semáforo */

    (*buffer)->sem = sem_open("sem", O_CREAT, 0644, 1);

    if ((*buffer)->sem == SEM_FAILED)
    {
        fprintf(stderr, "Error en sem_open");
        shmdt(*buffer); // Desmapeamos la memoria compartida
        shmctl(*shmid, IPC_RMID, NULL); // Liberamos la memoria compartida
        return -1;
    }

/*
    sem = &( (*buffer)->sem );  // Obtenemos el semáforo

    sem_unlink("sem"); // Eliminamos el semáforo

    if (sem_init(sem, 1, 1) < 0) // Inicializamos el semáforo
    {
        fprintf(stderr, "Error en sem_init");
        shmdt(*buffer); // Desmapeamos la memoria compartida
        shmctl(*shmid, IPC_RMID, NULL); // Liberamos la memoria compartida
        return -1;
    }
*/

    /*    sem = &((*buffer)->sem); 
    //sem = &(*buffer->sem);

    if ((sem = sem_open("sem", O_CREAT, 0644, 1)) == SEM_FAILED)
    {
        fprintf(stderr, "Error en sem_open");
        shmdt(*buffer);                  // Desmapeamos la memoria compartida
        shmctl(*shmid, IPC_RMID, NULL);  // Liberamos la memoria compartida
        return -1;
    }
    */
   
    /* Inicializamos el buffer */
    for (i = 0; i < BUFFER_SIZE; i++)
    {
        (*buffer)->temp_celsius[i] = 0;
    }

    print_buffer(*buffer);

    printf("Buffer de memoria compartida creado satisfactoriamente\n");

    return 0;
}

int buffer_put(struct shared_buffer *buffer, float data)
{
    printf("Escribiendo en buffer de memoria compartida\n");

    if(buffer == NULL)
    {
        fprintf(stderr, "Error en buffer_put: NULL ptr\n"); 
        return -1;
    }

    sem_wait(buffer->sem); // Esperamos a que el semáforo esté libre

    // Actualizamos el buffer
    // El buffer es de tipo FIFO

    for (int i = 0; i < BUFFER_SIZE - 1; i++)
    {
        buffer->temp_celsius[i] = buffer->temp_celsius[i + 1];
    }

    buffer->temp_celsius[BUFFER_SIZE - 1] = data;

    sem_post(buffer->sem); // Liberamos el semáforo
    
    printf("El valor %f se ha escrito en el buffer de memoria compartida\n", data);

    return 0;
}

void print_buffer(struct shared_buffer *buffer)
{
    printf("Imprimiendo buffer de memoria compartida\n");

    if(buffer == NULL)
    {
        fprintf(stderr, "Error en print_buffer: NULL ptr\n"); 
        return;
    }

    sem_wait(buffer->sem); // Esperamos a que el semáforo esté libre

    // Imprimimos el buffer

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        printf("buffer[%d] = %f\n", i, buffer->temp_celsius[i]);
    }

    sem_post(buffer->sem); // Liberamos el semáforo
}

int buffer_get(struct shared_buffer *buffer , float *data)
{
    if(data == NULL)
    {
        fprintf(stderr, "Error en buffer_get\n");      
        return -1;
    }

    if(buffer == NULL)
    {
        fprintf(stderr, "Error en buffer_get\n");       
        return -1;
    }

    sem_wait(buffer->sem); // Esperamos a que el semáforo esté libre

    // Leemos el buffer
    // El buffer es de tipo FIFO

    *data = buffer->temp_celsius[0];

    sem_post(buffer->sem); // Liberamos el semáforo

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
        fprintf(stderr, "Error en buffer_get\n");       
        return -1;
    }

    if(buffer == NULL)
    {
        fprintf(stderr, "Error en buffer_get\n");      
        return -1;
    }

    sem_wait(buffer->sem); // Esperamos a que el semáforo esté libre

    // Calculamos el promedio

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        sum += buffer->temp_celsius[i];
    }

    sem_post(buffer->sem); // Liberamos el semáforo

    *data = sum / BUFFER_SIZE;

    return 0;
}

void buffer_destroy(struct shared_buffer **buffer, int shmid)
{

    sem_close((*buffer)->sem); // Cerramos el semáforo
    
    sem_unlink("sem"); // Eliminamos el semáforo

    shmdt(*buffer); // Desmapeamos la memoria compartida

    shmctl(shmid, IPC_RMID, NULL); // Liberamos la memoria compartida

    *buffer = NULL;

    printf("Buffer de memoria compartida destruido\n"); 
}
