#include "../inc/utils.h"
#include <sys/stat.h>

/// @brief Gets the string from a file.
/// @param filename filename The name of the file to open.
/// @param string The string in which to store the contents of the file.
/// @return  '0' if the string was generated successfully. Returns a non-zero value if an error occurred.
///          '1' The file does not exist.
///          '2' The string does not have enough space to store the contents of the file.
int get_sting_from_file(const char * filename, char * string, size_t buffer_size)
{
    struct stat archivo_info;

    // Abre el archivo para lectura.
    FILE *archivo = fopen(filename, "r");
    if (archivo == NULL)
    {
        perror("El archivo no existe..");
        return 1;
    }

    if (stat(filename, &archivo_info) != 0)
    {
        perror("Error obteniendo información del archivo.");
        fclose(archivo);
        return 3;
    }

    // Verifica que la string tenga suficiente espacio para almacenar el contenido del archivo.
    if (archivo_info.st_size >= buffer_size - 1)
    {
        printf("%i\t%zu",(int)archivo_info.st_size,buffer_size - 1);
        perror("No hay suficiente espacio en la string.");
        fclose(archivo);
        return 2;
    }

    size_t read_size = fread(string, 1, archivo_info.st_size, archivo);
    if (read_size != archivo_info.st_size)
    {
        perror("Error leyendo el archivo.");
        fclose(archivo);
        return 4;
    }
    
    string[archivo_info.st_size] = '\0';

    // Cierra el archivo.
    fclose(archivo);

    return 0;
}