#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

// Server info
#define SRV_ADDR "127.0.0.1"
#define SRV_PORT 8080

// Buffer info
#define BUF_SIZE 1024

// Print infomation message
#define PRINT_INFO(str) printf("[Info] " str)

// Print error message
#define PRINT_ERROR(str) printf("[Error] " str)

#endif // COMMON_H
