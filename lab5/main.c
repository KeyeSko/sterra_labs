#include <stdio.h>
#include <stdlib.h>
#include "logger.h"

int main() {
    if (log_init("application.log") != 0) {
        printf("Failed to open log file\n");
        return 1;
    }

    LOG_INFO("Application started %d", 1);
    LOG_DEBUG("Debug mode active %d", 1);


    char *ptr = (char *)malloc(10);
    if (!ptr) {
        LOG_ERROR("Memory allocation failed %d", 1);
    } else {
        LOG_INFO("Memory allocated successfully %d", 1);
        free(ptr);
    }

    LOG_WARNING("This is a warning message %d", 1);

    LOG_ERROR("This is a critical error! %d", 1);

    log_close();
    return 0;
}

