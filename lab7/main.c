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
#include <time.h>
#include <sys/inotify.h>
#include <sys/epoll.h>

#define CONFIG_FILE "config.txt"
#define BUF_SIZE 128
#define EPOLL_MAX_EVENTS 10
#define INOTIFY_EVENT_SIZE (sizeof(struct inotify_event))
#define INOTIFY_BUF_LEN (1024 * (INOTIFY_EVENT_SIZE + 16))

static char file_path[BUF_SIZE];
static char socket_path[BUF_SIZE];
static int server_sock;
static int epoll_fd;
static int inotify_fd;
static int inotify_watch_fd;
static off_t cached_file_size = -1;

void handle_signal(int sig) {
    if (server_sock > 0) {
        close(server_sock);
        unlink(socket_path);
    }
    if (epoll_fd > 0) {
        close(epoll_fd);
    }
    if (inotify_fd > 0) {
        close(inotify_fd);
    }
    exit(EXIT_SUCCESS);
}

int rewatch_file() {
    if (inotify_watch_fd != -1) {
        inotify_rm_watch(inotify_fd, inotify_watch_fd);
    }

    inotify_watch_fd = inotify_add_watch(inotify_fd, file_path, IN_MODIFY | IN_DELETE_SELF);
    if (inotify_watch_fd == -1) {
        perror("Ошибка повторного добавления наблюдения за файлом. Ожидаем восстановления файла...");
        return 1;
    }
    return 0;
}

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

int setup_inotify() {
    inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        perror("Ошибка при инициализации inotify");
        return 1;
    }

    inotify_watch_fd = inotify_add_watch(inotify_fd, file_path, IN_MODIFY | IN_DELETE_SELF);
    if (inotify_watch_fd == -1) {
        perror("Ошибка при добавлении наблюдения inotify за файлом");
        return 1;
    }

    return 0;
}

int run_server() {
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

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Ошибка при создании epoll");
        return 1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &ev) == -1) {
        perror("Ошибка при добавлении сокета в epoll");
        return 1;
    }

    if (setup_inotify() != 0) {
        return 1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = inotify_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, inotify_fd, &ev) == -1) {
        perror("Ошибка при добавлении inotify в epoll");
        return 1;
    }

    if (get_file_size(file_path, &file_size) == 0) {
        cached_file_size = file_size;
    } else {
        perror("Ошибка чтения файла");
    }

    struct epoll_event events[EPOLL_MAX_EVENTS];
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, -1);  // Ожидаем события
        if (nfds == -1) {
            perror("Ошибка epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_sock) {
                int client_sock = accept(server_sock, NULL, NULL);
                if (client_sock < 0) {
                    perror("Ошибка при accept");
                    continue;
                }

                snprintf(buf, BUF_SIZE, "Размер файла: %ld байт\n", cached_file_size);
                write(client_sock, buf, strlen(buf));

                close(client_sock);
            } else if (events[i].data.fd == inotify_fd) {
                char inotify_buf[INOTIFY_BUF_LEN];
                int length = read(inotify_fd, inotify_buf, INOTIFY_BUF_LEN);

                if (length < 0) {
                    perror("Ошибка чтения inotify событий");
                }

                int i = 0;
                while (i < length) {
                    struct inotify_event *event = (struct inotify_event *) &inotify_buf[i];
                    if (event->mask & IN_MODIFY) {
                        if (get_file_size(file_path, &file_size) == 0) {
                            cached_file_size = file_size;
                        } else {
                            perror("Ошибка чтения файла");
                        }
                    } else if (event->mask & IN_DELETE_SELF) {
                        fprintf(stderr, "Файл был удален, ожидаем его восстановления...\n");
                        rewatch_file();
                        
                        if (get_file_size(file_path, &file_size) == 0) {
                            cached_file_size = file_size;
                        } else {
                            perror("Ошибка чтения файла");
                        }
                    }
                    i += INOTIFY_EVENT_SIZE + event->len;
                }
            }
        }
    }

    close(server_sock);
    unlink(socket_path);
    close(epoll_fd);
    close(inotify_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (read_config(CONFIG_FILE) != 0) {
        return 1;
    }
    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (argc == 2 && strcmp(argv[1], "--daemon") == 0) {
        if (daemonize() != 0)
            return 1;
    }

    if (run_server() != 0) {
        return 1;
    }

    return 0;
}

