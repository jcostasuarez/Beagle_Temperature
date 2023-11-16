#include "../inc/server.h"
#include "../inc/utils.h"

int main(int argc, char *argv[])
{
  int s;
  struct sockaddr_in datosServidor;
  socklen_t longDirec;

  if (argc != 2)
  {
    printf("\n\nLinea de comandos: webserver Puerto\n\n");
    exit(1);
  }
  // Creamos el socket
  s = socket(AF_INET, SOCK_STREAM,0);
  if (s == -1)
  {
    printf("ERROR: El socket no se ha creado correctamente!\n");
    exit(1);
  }
  // Asigna el puerto indicado y una IP de la maquina
  datosServidor.sin_family = AF_INET;
  datosServidor.sin_port = htons(atoi(argv[1]));
  datosServidor.sin_addr.s_addr = htonl(INADDR_ANY);

  // Obtiene el puerto para este proceso.
  if( bind(s, (struct sockaddr*)&datosServidor,
           sizeof(datosServidor)) == -1)
  {
    printf("ERROR: este proceso no puede tomar el puerto %s\n",
           argv[1]);
    exit(1);
  }
  printf("\nIngrese en el navegador http://dir ip servidor:%s/gradosCelsius\n",
         argv[1]);
  // Indicar que el socket encole hasta MAX_CONN pedidos
  // de conexion simultaneas.
  if (listen(s, MAX_CONN) < 0)
  {
    perror("Error en listen");
    close(s);
    exit(1);
  }
  // Permite atender a multiples usuarios
  while (1)
  {
    int pid, s_aux;
    struct sockaddr_in datosCliente;
    // La funcion accept rellena la estructura address con
    // informacion del cliente y pone en longDirec la longitud
    // de la estructura.
    longDirec = sizeof(datosCliente);
    s_aux = accept(s, (struct sockaddr*) &datosCliente, &longDirec);
    if (s_aux < 0)
    {
      perror("Error en accept");
      close(s);
      exit(1);
    }
    pid = fork();
    if (pid < 0)
    {
      perror("No se puede crear un nuevo proceso mediante fork");
      close(s);
      exit(1);
    }
    if (pid == 0)
    {       // Proceso hijo.
      ProcesarCliente(s_aux, &datosCliente, atoi(argv[1]));
      exit(0);
    }
    close(s_aux);  // El proceso padre debe cerrar el socket
                   // que usa el hijo.
  }
}

void ProcesarCliente(int s_aux, struct sockaddr_in *pDireccionCliente,
                     int puerto)
{
  char bufferComunic[16384];
  char ipAddr[20];
  int Port;
  float tempCelsius;
  int tempValida = 0;
  char HTML[8192];
  char encabezadoHTML[4096];

  char file_html_header[] = "../public/server_header.html";
  char file_css[] = "../public/css/style.css/0";

  strcpy(ipAddr, inet_ntoa(pDireccionCliente->sin_addr));
  Port = ntohs(pDireccionCliente->sin_port);
  // Recibe el mensaje del cliente
  if (recv(s_aux, bufferComunic, sizeof(bufferComunic), 0) == -1)
  {
    perror("Error en recv");
    exit(1);
  }
  printf("* Recibido del navegador Web %s:%d:\n%s\n",
         ipAddr, Port, bufferComunic);
  
  // Obtener la temperatura desde la ruta.
  if (memcmp(bufferComunic, "GET /", 5) == 0)
  {
    printf("%c\n\n", bufferComunic[5]);
    if (sscanf(&bufferComunic[5], "%f", &tempCelsius) == 1)
    {      // Conversion done successfully.
      tempValida = 1;
    }
  }
  
  if(get_sting_from_file(file_html_header,encabezadoHTML))
  {
    perror("Error en get_sting_from_file");
    exit(1);  
  }

  if (tempValida)
  {
    sprintf(HTML, 
            "%s<p>%f grados Celsius equivale a %f grados Fahrenheit</p>",
            encabezadoHTML, tempCelsius, tempCelsius * 1.8 + 32);
  }
  else
  {
    sprintf(HTML, 
            "%s<p>El URL debe ser http://dominio:%d/gradosCelsius.</p>",
            encabezadoHTML, puerto);
  }

  sprintf(bufferComunic,
          "HTTP/1.1 200 OK\n"
          "Content-Length: %ld\n"
          "Content-Type: text/html; charset=utf-8\n"
          "Connection: Closed\n\n%s",
          strlen(HTML), HTML);

  printf("* Enviado al navegador Web %s:%d:\n%s\n",
         ipAddr, Port, bufferComunic);
  
  // Envia el mensaje al cliente
  if (send(s_aux, bufferComunic, strlen(bufferComunic), 0) == -1)
  {
    perror("Error en send");
    exit(1);
  }
  // Cierra la conexion con el cliente actual
  close(s_aux);
}

/**
 * \brief Obtiene la cadena de un archivo.
 *
 * La función `get_string_from_file()` abre el archivo especificado por el nombre de archivo `filename` y coloca su contenido en la string `string`.
 *
 * \param filename El nombre del archivo que se va a abrir.
 * \param string La string en la que se va a almacenar el contenido del archivo.
 *
 * \return `0` si la cadena se generó correctamente. Devuelve un valor distinto de cero si se produjo un error.
 *
 * \error
 * * `1`: El archivo no existe.
 * * `2`: La string no tiene suficiente espacio para almacenar el contenido del archivo.
 * ```
 */

