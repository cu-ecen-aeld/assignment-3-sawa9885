#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#define SERVER_PORT 9000
#define RECEIVE_SIZE 1024
#define DATA_FILE "/var/tmp/aesdsocketdata"

static volatile sig_atomic_t signal_caught;

static void signal_handler(int signo)
{
    (void)signo;
    signal_caught = 1;
}

static bool install_signal_handlers(void)
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = signal_handler;
    if (sigemptyset(&action.sa_mask) == -1) {
        return false;
    }
    if ((sigaction(SIGINT, &action, NULL) == -1) ||
        (sigaction(SIGTERM, &action, NULL) == -1)) {
        return false;
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        return false;
    }
    return true;
}

static bool write_all(int fd, const char *buffer, size_t length)
{
    size_t written = 0;

    while (written < length) {
        ssize_t result = write(fd, buffer + written, length - written);
        if (result > 0) {
            written += (size_t)result;
        } else if ((result == -1) && (errno == EINTR)) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

static bool send_all(int fd, const char *buffer, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t result = send(fd, buffer + sent, length - sent, 0);
        if (result > 0) {
            sent += (size_t)result;
        } else if ((result == -1) && (errno == EINTR) && !signal_caught) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

static bool append_packet(const char *packet, size_t length)
{
    int fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    bool success;

    if (fd == -1) {
        syslog(LOG_ERR, "Unable to open %s: %s", DATA_FILE, strerror(errno));
        return false;
    }
    success = write_all(fd, packet, length);
    if (!success) {
        syslog(LOG_ERR, "Unable to append to %s: %s", DATA_FILE,
               strerror(errno));
    }
    if (close(fd) == -1) {
        syslog(LOG_ERR, "Unable to close %s: %s", DATA_FILE, strerror(errno));
        success = false;
    }
    return success;
}

static bool send_file(int client_fd)
{
    char buffer[RECEIVE_SIZE];
    int fd = open(DATA_FILE, O_RDONLY);
    bool success = true;

    if (fd == -1) {
        syslog(LOG_ERR, "Unable to read %s: %s", DATA_FILE, strerror(errno));
        return false;
    }

    while (success) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            success = send_all(client_fd, buffer, (size_t)bytes_read);
        } else if (bytes_read == 0) {
            break;
        } else if (errno != EINTR) {
            syslog(LOG_ERR, "Unable to read %s: %s", DATA_FILE,
                   strerror(errno));
            success = false;
        } else if (signal_caught) {
            success = false;
        }
    }

    if (close(fd) == -1) {
        syslog(LOG_ERR, "Unable to close %s: %s", DATA_FILE, strerror(errno));
        success = false;
    }
    return success;
}

static bool grow_packet(char **packet, size_t *capacity, size_t required)
{
    size_t new_capacity = *capacity == 0 ? RECEIVE_SIZE : *capacity;
    char *new_packet;

    while (new_capacity < required) {
        if (new_capacity > SIZE_MAX / 2) {
            syslog(LOG_ERR, "Received packet is too large");
            return false;
        }
        new_capacity *= 2;
    }

    new_packet = realloc(*packet, new_capacity);
    if (new_packet == NULL) {
        syslog(LOG_ERR, "Unable to allocate packet buffer: %s",
               strerror(errno));
        return false;
    }
    *packet = new_packet;
    *capacity = new_capacity;
    return true;
}

static void handle_client(int client_fd)
{
    char receive_buffer[RECEIVE_SIZE];
    char *packet = NULL;
    size_t packet_length = 0;
    size_t packet_capacity = 0;
    bool keep_receiving = true;

    while (keep_receiving && !signal_caught) {
        ssize_t received = recv(client_fd, receive_buffer,
                                sizeof(receive_buffer), 0);
        if (received > 0) {
            size_t offset = 0;

            while ((offset < (size_t)received) && keep_receiving) {
                char *newline = memchr(receive_buffer + offset, '\n',
                                       (size_t)received - offset);
                size_t chunk_length = newline == NULL
                                          ? (size_t)received - offset
                                          : (size_t)(newline -
                                                     (receive_buffer + offset)) +
                                                1;

                if (!grow_packet(&packet, &packet_capacity,
                                 packet_length + chunk_length)) {
                    keep_receiving = false;
                    break;
                }
                memcpy(packet + packet_length, receive_buffer + offset,
                       chunk_length);
                packet_length += chunk_length;
                offset += chunk_length;

                if (newline != NULL) {
                    if (!append_packet(packet, packet_length) ||
                        !send_file(client_fd)) {
                        keep_receiving = false;
                    }
                    packet_length = 0;
                }
            }
        } else if (received == 0) {
            break;
        } else if ((errno != EINTR) || signal_caught) {
            if (!signal_caught) {
                syslog(LOG_ERR, "Receive failed: %s", strerror(errno));
            }
            break;
        }
    }

    free(packet);
}

static int create_server_socket(void)
{
    struct sockaddr_in address;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;

    if (server_fd == -1) {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        return -1;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) == -1) {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    if (listen(server_fd, 10) == -1) {
        syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    return server_fd;
}

static int become_daemon(void)
{
    pid_t pid = fork();

    if (pid == -1) {
        syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
        return -1;
    }
    if (pid > 0) {
        return 1;
    }
    if (setsid() == -1) {
        syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
        return -1;
    }
    if (chdir("/") == -1) {
        syslog(LOG_ERR, "chdir failed: %s", strerror(errno));
        return -1;
    }

    {
        int null_fd = open("/dev/null", O_RDWR);
        if (null_fd == -1) {
            syslog(LOG_ERR, "Unable to open /dev/null: %s", strerror(errno));
            return -1;
        }
        if ((dup2(null_fd, STDIN_FILENO) == -1) ||
            (dup2(null_fd, STDOUT_FILENO) == -1) ||
            (dup2(null_fd, STDERR_FILENO) == -1)) {
            syslog(LOG_ERR, "Unable to redirect standard streams: %s",
                   strerror(errno));
            close(null_fd);
            return -1;
        }
        if (null_fd > STDERR_FILENO) {
            close(null_fd);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    bool daemon_mode = false;
    int server_fd;
    int exit_status = EXIT_SUCCESS;

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = true;
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        return -1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);
    if (!install_signal_handlers()) {
        syslog(LOG_ERR, "Unable to install signal handlers: %s",
               strerror(errno));
        closelog();
        return -1;
    }

    server_fd = create_server_socket();
    if (server_fd == -1) {
        closelog();
        return -1;
    }

    if (daemon_mode) {
        int daemon_result = become_daemon();
        if (daemon_result != 0) {
            close(server_fd);
            closelog();
            return daemon_result > 0 ? EXIT_SUCCESS : -1;
        }
    }

    while (!signal_caught) {
        struct sockaddr_in client_address;
        socklen_t client_length = sizeof(client_address);
        char client_ip[INET_ADDRSTRLEN] = "unknown";
        int client_fd = accept(server_fd, (struct sockaddr *)&client_address,
                               &client_length);

        if (client_fd == -1) {
            if ((errno == EINTR) && signal_caught) {
                break;
            }
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            exit_status = -1;
            break;
        }

        if (inet_ntop(AF_INET, &client_address.sin_addr, client_ip,
                      sizeof(client_ip)) == NULL) {
            strncpy(client_ip, "unknown", sizeof(client_ip));
            client_ip[sizeof(client_ip) - 1] = '\0';
        }
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        handle_client(client_fd);
        if (close(client_fd) == -1) {
            syslog(LOG_ERR, "Unable to close client socket: %s",
                   strerror(errno));
        }
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }

    if (signal_caught) {
        syslog(LOG_INFO, "Caught signal, exiting");
    }
    if (close(server_fd) == -1) {
        syslog(LOG_ERR, "Unable to close server socket: %s", strerror(errno));
        exit_status = -1;
    }
    if ((unlink(DATA_FILE) == -1) && (errno != ENOENT)) {
        syslog(LOG_ERR, "Unable to remove %s: %s", DATA_FILE,
               strerror(errno));
        exit_status = -1;
    }
    closelog();
    return exit_status;
}
