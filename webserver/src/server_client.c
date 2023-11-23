/**
 * @file server_client.c
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "../inc/server_client.h"
#include "../inc/buffer.h"

#include <arpa/inet.h> // Add this line to include the header file
#include <time.h>
#include <stdbool.h>

#define BUFFER_COMUNIC_SIZE 16384
#define IP_ADDR_SIZE 20
#define HTML_SIZE 8192
#define HTML_HEADER_SIZE 4096

#define FILE_HTML_HEADER_ADDR  "public/html/header.html\0"
#define FILE_CSS_ADDR "public/css/style.css\0"
//#define FILE_HTML_BODY_ADDR "public/html/body.html\0"
//#define FILE_HTML_FOOTER_ADDR "public/html/footer.html\0"

/*Funciones privadas*/

static int get_string_from_file(char *file_name, char *string);
static void send_png(int client_socket, const char *file_path, const char *content_type);
static void send_response(int client_socket, const char *response);
static void generate_json(char *json, float * temp_data, float * time_data, int size);

/*Funciones de la biblioteca*/

/**
 * @brief 
 * 
 * @param s_aux 
 * @param pDireccionCliente 
 * @param puerto 
 * @param buffer 
 * @return int 
 */
int ProcesarCliente(int s_aux, struct sockaddr_in *pDireccionCliente, int puerto, shared_buffer *buffer)
{
  char bufferComunic[BUFFER_COMUNIC_SIZE];
  char ipAddr[IP_ADDR_SIZE];
  int Port;
  float tempCelsius;
  char HTML[HTML_SIZE];
  char encabezadoHTML[HTML_HEADER_SIZE];

  float time[BUFFER_SIZE];
  float temp[BUFFER_SIZE];

  strcpy(ipAddr, inet_ntoa(pDireccionCliente->sin_addr));
  Port = ntohs(pDireccionCliente->sin_port);

  // Recibe el mensaje del cliente
  if (recv(s_aux, bufferComunic, sizeof(bufferComunic), 0) == -1)
  {
    fprintf(stderr, "Error en recv");
    return -1;
  }
  printf("* Recibido del navegador Web %s:%d:\n%s\n",
          ipAddr, Port, bufferComunic);

  // Obtiene la temperatura del buffer
  
  if (buffer_avg(buffer, &tempCelsius))
  {
    fprintf(stderr, "Error en buffer_avg");
    return -1;
  }

  // Cargo el vector time con datos del buffer

  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    if (buffer_get_time(buffer, i, &time[i]))
    {
      fprintf(stderr, "Error en buffer_get_time");
      return -1;
    }
  }

  // Cargo el vector temp con datos del buffer

  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    if (buffer_get_temp(buffer, i, &temp[i]))
    {
      fprintf(stderr, "Error en buffer_get_temp");
      return -1;
    }
  }

  // Comparamos el mensaje recibido con el mensaje esperado
  // Genera el mensaje a enviar al cliente

  if (get_string_from_file(FILE_HTML_HEADER_ADDR, encabezadoHTML))
  {
    fprintf(stderr, "Error en get_sting_from_file");
    return -1;
  }

  if(strstr(bufferComunic, "GET / HTTP/1.1") != NULL)
  {
    sprintf(HTML,
            "%s<p>%f grados Celsius equivale a %f grados Fahrenheit</p>",
            encabezadoHTML, tempCelsius, tempCelsius * 1.8 + 32);
    
    sprintf(bufferComunic,
    "HTTP/1.1 200 OK\n"
    "Content-Length: %ld\n"
    "Content-Type: text/html; charset=utf-8\n"
    "Connection: Closed\n\n%s",
    strlen(HTML), HTML);
  }

  else if(strstr(bufferComunic, "GET /styles.css HTTP/1.1") != NULL)
  {
    if (get_string_from_file(FILE_CSS_ADDR, HTML))
    {
      fprintf(stderr, "Error buscando el archivo css\n");
      return -1;
    }

    sprintf(bufferComunic,
    "HTTP/1.1 200 OK\n"
    "Content-Length: %ld\n"
    "Content-Type: text/css; charset=utf-8\n"
    "Connection: Closed\n\n%s",
    strlen(HTML), HTML);
  }
  else if(strstr(bufferComunic, "GET /logo-utn-frba.png HTTP/1.1") != NULL)
  {
    send_png(s_aux, "public/image/logo-utn-frba.png", "image/png");

    close(s_aux);

    return 0;
  }
  else if(strstr(bufferComunic, "GET /GetData HTTP/1.1") != NULL)
  {
    
    char json[1024];
    
    generate_json(json, temp, time, BUFFER_SIZE);

    sprintf(bufferComunic, 
    "HTTP/1.1 200 OK\n"
    "Content-Length: %ld\n"
    "Content-Type: application/json; charset=utf-8\n"
    "Connection: Closed\n\n%s",
    strlen(json), json);
  }
  else
  {
    // Mensaje de error
    sprintf(bufferComunic,
            "HTTP/1.1 404 Not Found\n"
            "Content-Length: 0\n"
            "Content-Type: text/html; charset=utf-8\n"
            "Connection: Closed\n\n");

    printf("Mensaje de error\n");
  }

  //printf("* Enviado al navegador Web %s:%d:\n%s\n",ipAddr, Port, bufferComunic);

  // Envia el mensaje al cliente
  if (send(s_aux, bufferComunic, strlen(bufferComunic), 0) == -1)
  {
      fprintf(stderr, "Error en send");
      return -1;
  }

  // Cierra la conexion con el cliente actual
  close(s_aux);

  return 0;
}


static int get_string_from_file(char *file_name, char *string)
{
  FILE *file = NULL;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  file = fopen(file_name, "r");

  if (file == NULL)
  {
    fprintf(stderr, "Error al abrir el archivo %s\n", file_name);
    return -1;
  }

  while ((read = getline(&line, &len, file)) != -1)
  {
    strcat(string, line);
  }

  fclose(file);

  if (line)
  {
    free(line);
  }

  return 0;
}

static void send_png(int client_socket, const char *file_path, const char *content_type)
{
  FILE *file = NULL;
  char *png_buffer = NULL;
  int file_size = 0;

  file = fopen(file_path, "rb");

  if (file == NULL)
  {
    fprintf(stderr, "Error al abrir el archivo %s\n", file_path);
    return;
  }

  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  rewind(file);

  png_buffer = (char *)malloc(sizeof(char) * file_size);

  if (png_buffer == NULL)
  {
    fprintf(stderr, "Error al reservar memoria para el png_buffer\n");
    return;
  }

  fread(png_buffer, sizeof(char), file_size, file);

  fclose(file);

  send_response(client_socket, "HTTP/1.1 200 OK\n");
  send_response(client_socket, "Content-Type: image/png\n");
  send_response(client_socket, "Connection: Closed\n");
  send_response(client_socket, "\n");

  send(client_socket, png_buffer, file_size, 0);

  free(png_buffer);
}


static void send_response(int client_socket, const char *response)
{
  send(client_socket, response, strlen(response), 0);
}

static void generate_json(char *json, float * temp_data, float * time_data, int size)
{
  char temp[256], time[256];

  // Start the JSON string
  strcpy(json, "{\"temp\":[");
  
  // Add the temperature data to the JSON string
  for (int i = 0; i < size; i++) {
      sprintf(temp, "%.2f", temp_data[i]);
      strcat(json, temp);
      if (i < size - 1) {
          strcat(json, ",");
      }
  }
  
  // Add the time data to the JSON string
  strcat(json, "],\"time\":[");
  for (int i = 0; i < size; i++) {
      sprintf(time, "%.2f", time_data[i]);
      strcat(json, time);
      if (i < size - 1) {
          strcat(json, ",");
      }
  }
  
  // End the JSON string
  strcat(json, "]}");
}