#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define CONFIG_FILE "config.txt"
#define BUF_SIZE 128

static char file_path[BUF_SIZE];
static char socket_path[BUF_SIZE];

int read_config(const char *config_file) {
    FILE *file = fopen(config_file, "r");
    if (!file) {
        perror("Не удалось открыть файл конфигурации");
        return 1;
    }

    if (fscanf(file, "file=%s\nsocket=%s", file_path, socket_path) != 2) {
        perror("Ошибка чтения конфигурации");
        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}

int daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Ошибка при fork");
        return 1;
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("Ошибка при создании нового сеанса");
        return 1;
    }

    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) {
        perror("Ошибка при втором fork");
        return 1;
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (chdir("/") < 0) {
        perror("Ошибка при смене директории на корневую");
        return 1;
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    return 0;
}

int get_file_size(const char *filename, off_t *size) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        *size = st.st_size;
        return 0;
    }
    return 1;
}

int run_server() {
    int server_sock, client_sock;
    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    off_t file_size;

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Ошибка при создании сокета");
        return 1;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path);
    if (bind(server_sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) < 0) {
        perror("Ошибка при bind");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 5) < 0) {
        perror("Ошибка при listen");
        close(server_sock);
        return 1;
    }

    while (1) {
        client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) {
            perror("Ошибка при accept");
            continue;
        }

        if (get_file_size(file_path, &file_size) == 0) {
            snprintf(buf, BUF_SIZE, "Размер файла: %ld байт\n", file_size);
        } else {
            snprintf(buf, BUF_SIZE, "Ошибка: %s\n", strerror(errno));
        }

        write(client_sock, buf, strlen(buf));
        close(client_sock);
    }

    close(server_sock);
    unlink(socket_path);
    return 0;
}

int main(int argc, char *argv[]) {
    off_t file_size;

    if (read_config(CONFIG_FILE) != 0) {
        return 1;
    }

    if (argc == 2 && strcmp(argv[1], "--daemon") == 0) {
        if (daemonize() != 0)
            return 1;

        if (run_server() != 0)
            return 1;

    } else {
        if (get_file_size(file_path, &file_size) == 0) {
            printf("Размер файла: %ld байт\n", file_size);
        } else {
            perror("Ошибка получения размера файла");
            return 1;
        }
    }

    return 0;
}

