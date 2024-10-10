#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <execinfo.h>
#include <pthread.h>
#include <time.h>

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int log_init(const char *filename) {
    log_file = fopen(filename, "a");
    if (!log_file) {
        return 1;
    }
    return 0;
}

void log_close() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

static void get_time_str(char *buffer, size_t size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void log_message(log_level_t level, const char *file, int line, const char *fmt, ...) {
    pthread_mutex_lock(&log_mutex);

    if (!log_file) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    const char *level_str;
    switch (level) {
        case LOG_DEBUG: level_str = "DEBUG"; break;
        case LOG_INFO: level_str = "INFO"; break;
        case LOG_WARNING: level_str = "WARNING"; break;
        case LOG_ERROR: level_str = "ERROR"; break;
        default: level_str = "UNKNOWN"; break;
    }

    char time_str[20];
    get_time_str(time_str, sizeof(time_str));

    fprintf(log_file, "[%s] [%s] (%s:%d) ", time_str, level_str, file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);

    fprintf(log_file, "\n");

    if (level == LOG_ERROR) {
        void *buffer[10];
        int nptrs = backtrace(buffer, 10);
        fprintf(log_file, "Stack trace:\n");
        char **symbols = backtrace_symbols(buffer, nptrs);
        for (int i = 0; i < nptrs; i++) {
            fprintf(log_file, "%s\n", symbols[i]);
        }
        free(symbols);
    }

    fflush(log_file);
    pthread_mutex_unlock(&log_mutex);
}

