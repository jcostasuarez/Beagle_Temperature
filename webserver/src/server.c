/**
 * @file server.c
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "../inc/server.h"
#include "../inc/buffer.h"
#include "../inc/server_client.h"

#define FILENAME_DIR_MAX 100
char cCurrentPath[FILENAME_DIR_MAX];

int main(int argc, char *argv[])
{
  int socket_id;
  int shmid;
  struct shared_buffer *buffer = NULL;

  struct sockaddr_in datosServidor;
  socklen_t longDirec;

  printf("\n\nServidor Web\n\n");

  getcwd(cCurrentPath, sizeof(cCurrentPath));

  printf("Current folder: %s\n", cCurrentPath);

  if (argc != 2)
  {
    printf("\n\nLinea de comandos: webserver Puerto\n\n");
    return -1;
  }

  // Creamos el socket
  socket_id = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_id == -1)
  {
    printf("ERROR: El socket no se ha creado correctamente!\n");
    return -1;
  }

  // Asigna el puerto indicado y una IP de la maquina
  datosServidor.sin_family = AF_INET;
  datosServidor.sin_port = htons(atoi(argv[1]));
  datosServidor.sin_addr.s_addr = htonl(INADDR_ANY);

  // Obtiene el puerto para este proceso.
  if (bind(socket_id, (struct sockaddr *)&datosServidor, sizeof(datosServidor)) == -1)
  {
    printf("ERROR: este proceso no puede tomar el puerto %s\n", argv[1]);
    return -1;
  }

  printf("\nServidor Web iniciado en el puerto %s\n", argv[1]);

  printf("\nIngrese en el navegador http://dir ip servidor:%s/gradosCelsius\n", argv[1]);

  // Indicar que el socket encole hasta MAX_CONN pedidos de conexion simultaneas.

  if (listen(socket_id, MAX_CONN) < 0)
  {
    fprintf(stderr, "Error en listen.\n");
    close(socket_id);
    return -1;
  }

  // Inicializamos el buffer

  if (buffer_init(&buffer , &shmid) < 0)
  {
    fprintf(stderr, "Error en buffer_init.\n");
    close(socket_id);
    return -1;
  }

  printf("Buffer de memoria compartida creado: %p\n", buffer);

  print_buffer(buffer);
  
  // Creamos un proceso hijo que carga el buffer

  pid_t pid = fork();

  if (pid < 0)
  {
    fprintf(stderr, "Error en fork.\n");
    buffer_destroy(&buffer , shmid); // Destruimos el buffer
    close(socket_id);      // Cerramos el socket
    return -1;
  }

  fprintf(stdout, "PID: %d\n", pid);
  fprintf(stdout, "buffer: %p\n", buffer);

  if (pid == 0)
  {
    // Proceso hijo

    // Demon: Permite que el proceso hijo se ejecute en segundo plano
    // y que el proceso padre pueda terminar sin que el hijo termine.


    while (1)
    {
      // Carga el buffer
      if (buffer_put(buffer, 24) < 0)
      {
        fprintf(stderr, "Error en buffer_put.\n");
        buffer_destroy(&buffer, shmid); // Destruimos el buffer
        close(socket_id);      // Cerramos el socket
        exit(1);
      }
      printf("Cargando buffer de memoria compartida\n");

      sleep(BUFFER_TIME_SLEEP);
    } // End of while loop
  } // End of child process

  else if (pid > 0)
  {
    // Proceso padre

    // Permite atender a multiples usuarios
    while (1)
    {
      int pid_padre, s_aux_padre;
      struct sockaddr_in datosCliente;
      // La funcion accept rellena la estructura address con
      // informacion del cliente y pone en longDirec la longitud
      // de la estructura.
      longDirec = sizeof(datosCliente);
      s_aux_padre = accept(socket_id, (struct sockaddr*) &datosCliente, &longDirec);

      if (s_aux_padre < 0)
      {
        perror("Error en accept");
        buffer_destroy(&buffer , shmid); // Destruimos el buffer
        close(socket_id);
        return -1;
      }

      pid_padre = fork();
      if (pid_padre < 0)
      {
        perror("No se puede crear un nuevo proceso mediante fork");
        buffer_destroy(&buffer , shmid); // Destruimos el buffer
        close(socket_id);
        return -1;
      }
      if (pid_padre == 0)
      {       // Proceso hijo.
        ProcesarCliente(s_aux_padre, &datosCliente, atoi(argv[1]), buffer);
        return -1;
      }
    
      close(s_aux_padre);  // El proceso padre debe cerrar el socket que usa el hijo.
    }
  }

  printf("\nServidor Web finalizado\n");

  buffer_destroy(&buffer , shmid); // Destruimos el buffer
  close(socket_id); // Cerramos el socket

  return 0;
}
