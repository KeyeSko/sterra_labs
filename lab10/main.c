#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define MAX_EVENTS 1024
#define BUFFER_SIZE 1024

int set_nonblocking(int fd) {
    int flags, s;
    flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) {
        perror("fcntl");
        return 1;
    }
    flags |= O_NONBLOCK;
    s = fcntl(fd, F_SETFL, flags);
    if(s == -1) {
        perror("fcntl");
        return 1;
    }
    return 0;
}

int create_server_socket(const char *address, int *server_fd, int port) {
    int fd;
    struct sockaddr_in server_addr;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(address);

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(fd, SOMAXCONN) == -1) {
        perror("listen");
        return 1;
    }

    set_nonblocking(fd);
    *server_fd = fd;
    return 0;
}

int handle_request(int client_fd, const char *root_dir) {
    char buffer[BUFFER_SIZE];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        close(client_fd);
        return 1;
    }

    buffer[bytes_read] = '\0';

    char method[16], path[256];
    sscanf(buffer, "%s %s", method, path);

    if (strcmp(method, "GET") != 0) {
        char *response = "HTTP/1.1 403 Forbidden\r\n\r\n";
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        return 0;
    }

    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s", root_dir, path);
    
    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
        if (errno == ENOENT) {
            char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
            send(client_fd, response, strlen(response), 0);
        } else {
            char *response = "HTTP/1.1 403 Forbidden\r\n";
            send(client_fd, response, strlen(response), 0);
        }
        close(client_fd);
        return 0;
    }

    if (!S_ISREG(file_stat.st_mode)) {
        char *response = "HTTP/1.1 403 Forbidden\r\n\r\n";
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        return 0;
    }

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        char *response = "HTTP/1.1 403 Forbidden\r\n\r\n";
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        return 0;
    }

    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", file_stat.st_size);
    send(client_fd, header, strlen(header), 0);  

    send(client_fd, header, strlen(header), 0);
    sendfile(client_fd, file_fd, NULL, file_stat.st_size);

    close(file_fd);
    close(client_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <directory> <address> <port>\n", argv[0]);
        return 0;
    }

    int server_fd;
    char *root_dir = argv[1];
    char *address = argv[2];
    int port = atoi(argv[3]);

    if (create_server_socket(address, &server_fd, port) == 1) {
        perror("server create");
        return 1;
    }   

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return 1;
    }

    struct epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    struct epoll_event events[MAX_EVENTS];

    while(1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }
                set_nonblocking(client_fd);

                event.data.fd = client_fd;
                event.events = EPOLLIN;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
            }
            else {
                handle_request(events[i].data.fd, root_dir);
            }
        }
    }

    close(server_fd);
    return 0;
}

