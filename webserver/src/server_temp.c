/**
 * @file server_temp.c
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#define FILE_TEMP "/dev/bmp280_sitara"
#define MAX_SIZE 30

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

float get_temp(void)
{
    float temp = 0;
    FILE *fp;
    int i = 0;
    char *line = NULL;

    // Get space for line
    line = (char *)malloc(sizeof(char) * MAX_SIZE);

    if(line == NULL)
    {
        printf("ERROR: No se pudo reservar memoria para la linea\n");
        return -1;
    }
    if((fp = fopen(FILE_TEMP, "r")) == NULL)
    {
        printf("ERROR: No se pudo abrir el archivo %s\n", FILE_TEMP);
        return -1;
    }

    // Read until /n

    while(i < MAX_SIZE)
    {
        if(fread(&line[i], sizeof(char), 1, fp) == 0)
        {
            printf("ERROR: No se pudo leer el archivo %s\n", FILE_TEMP);
        }
        if(line[i] == '\n')
        {
            break;
        }
        i++;
    }

    printf("Line: %s\n", line);

    if(fgets(line, MAX_SIZE, fp) == NULL)
    {
        printf("ERROR: No se pudo leer el archivo %s\n", FILE_TEMP);
        return -1;
    }

    temp = atof(line)/100;

    // Free memory

    free(line);

    fclose(fp);

    return temp;
}