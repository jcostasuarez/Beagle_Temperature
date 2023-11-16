#ifndef __UTILS_H
#define __UTILS_H
    
    #include <stdio.h>
    #include <stdlib.h>

    /// @brief Gets the string from a file.
    /// @param filename filename The name of the file to open.
    /// @param string The string in which to store the contents of the file.
    /// @return  '0' if the string was generated successfully. Returns a non-zero value if an error occurred.
    ///          '1' The file does not exist.
    ///          '2' The string does not have enough space to store the contents of the file.
    int get_sting_from_file(const char * filename, char * string);

#endif