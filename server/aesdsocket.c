#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define SERVER_PORT 9000
#define RECEIVE_SIZE 1024
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define TIMESTAMP_INTERVAL_SECONDS 10

struct thread_data {
    pthread_t thread;
    int client_fd;
    char client_ip[INET_ADDRSTRLEN];
    pthread_mutex_t *file_mutex;
    atomic_bool complete;
    struct thread_data *next;
};

struct timer_data {
    pthread_mutex_t state_mutex;
    pthread_cond_t state_changed;
    pthread_mutex_t *file_mutex;
    bool stop;
};

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

static bool append_data(const char *data, size_t length)
{
    int fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    bool success;

    if (fd == -1) {
        syslog(LOG_ERR, "Unable to open %s: %s", DATA_FILE, strerror(errno));
        return false;
    }
    success = write_all(fd, data, length);
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

static bool process_packet(struct thread_data *data, const char *packet,
                           size_t length)
{
    bool success;

    if (pthread_mutex_lock(data->file_mutex) != 0) {
        syslog(LOG_ERR, "Unable to lock data-file mutex");
        return false;
    }
    success = append_data(packet, length);
    if (success) {
        success = send_file(data->client_fd);
    }
    if (pthread_mutex_unlock(data->file_mutex) != 0) {
        syslog(LOG_ERR, "Unable to unlock data-file mutex");
        success = false;
    }
    return success;
}

static void *handle_client(void *argument)
{
    struct thread_data *data = argument;
    char receive_buffer[RECEIVE_SIZE];
    char *packet = NULL;
    size_t packet_length = 0;
    size_t packet_capacity = 0;
    bool keep_receiving = true;

    while (keep_receiving && !signal_caught) {
        ssize_t received = recv(data->client_fd, receive_buffer,
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
                    keep_receiving = process_packet(data, packet,
                                                    packet_length);
                    packet_length = 0;
                }
            }
        } else if (received == 0) {
            break;
        } else if ((errno != EINTR) || signal_caught) {
            if (!signal_caught) {
                syslog(LOG_ERR, "Receive failed from %s: %s", data->client_ip,
                       strerror(errno));
            }
            break;
        }
    }

    free(packet);
    syslog(LOG_INFO, "Closed connection from %s", data->client_ip);
    atomic_store(&data->complete, true);
    return NULL;
}

static bool append_timestamp(pthread_mutex_t *file_mutex)
{
    char timestamp[128];
    time_t current_time = time(NULL);
    struct tm local_time;
    bool success = false;
    size_t length;

    if ((current_time == (time_t)-1) ||
        (localtime_r(&current_time, &local_time) == NULL)) {
        syslog(LOG_ERR, "Unable to obtain local time");
        return false;
    }
    length = strftime(timestamp, sizeof(timestamp),
                      "timestamp:%a, %d %b %Y %T %z\n", &local_time);
    if (length == 0) {
        syslog(LOG_ERR, "Unable to format timestamp");
        return false;
    }

    if (pthread_mutex_lock(file_mutex) != 0) {
        syslog(LOG_ERR, "Unable to lock data-file mutex for timestamp");
        return false;
    }
    success = append_data(timestamp, length);
    if (pthread_mutex_unlock(file_mutex) != 0) {
        syslog(LOG_ERR, "Unable to unlock data-file mutex after timestamp");
        success = false;
    }
    return success;
}

static void *timestamp_worker(void *argument)
{
    struct timer_data *data = argument;

    if (pthread_mutex_lock(&data->state_mutex) != 0) {
        syslog(LOG_ERR, "Unable to lock timer state mutex");
        return NULL;
    }
    while (!data->stop) {
        struct timespec deadline;
        int wait_result;

        if (clock_gettime(CLOCK_REALTIME, &deadline) == -1) {
            syslog(LOG_ERR, "Unable to read clock: %s", strerror(errno));
            break;
        }
        deadline.tv_sec += TIMESTAMP_INTERVAL_SECONDS;
        wait_result = pthread_cond_timedwait(&data->state_changed,
                                             &data->state_mutex, &deadline);
        if (data->stop) {
            break;
        }
        if (wait_result == ETIMEDOUT) {
            if (!append_timestamp(data->file_mutex)) {
                syslog(LOG_ERR, "Unable to append periodic timestamp");
            }
        } else if (wait_result != 0) {
            syslog(LOG_ERR, "Timer wait failed: %s", strerror(wait_result));
            break;
        }
    }
    if (pthread_mutex_unlock(&data->state_mutex) != 0) {
        syslog(LOG_ERR, "Unable to unlock timer state mutex");
    }
    return NULL;
}

static int create_thread_with_signals_blocked(pthread_t *thread,
                                              void *(*worker)(void *),
                                              void *argument)
{
    sigset_t blocked_signals;
    sigset_t previous_mask;
    int result;

    if ((sigemptyset(&blocked_signals) == -1) ||
        (sigaddset(&blocked_signals, SIGINT) == -1) ||
        (sigaddset(&blocked_signals, SIGTERM) == -1)) {
        return errno;
    }
    result = pthread_sigmask(SIG_BLOCK, &blocked_signals, &previous_mask);
    if (result != 0) {
        return result;
    }
    result = pthread_create(thread, NULL, worker, argument);
    {
        int restore_result = pthread_sigmask(SIG_SETMASK, &previous_mask, NULL);
        if (restore_result != 0) {
            syslog(LOG_ERR, "Unable to restore signal mask: %s",
                   strerror(restore_result));
        }
    }
    return result;
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

static void reap_completed_threads(struct thread_data **head)
{
    struct thread_data **current = head;

    while (*current != NULL) {
        struct thread_data *data = *current;
        if (atomic_load(&data->complete)) {
            *current = data->next;
            (void)pthread_join(data->thread, NULL);
            (void)close(data->client_fd);
            free(data);
        } else {
            current = &data->next;
        }
    }
}

static void stop_and_join_threads(struct thread_data *head)
{
    struct thread_data *current;

    for (current = head; current != NULL; current = current->next) {
        (void)shutdown(current->client_fd, SHUT_RDWR);
    }
    while (head != NULL) {
        struct thread_data *next = head->next;
        (void)pthread_join(head->thread, NULL);
        (void)close(head->client_fd);
        free(head);
        head = next;
    }
}

int main(int argc, char *argv[])
{
    bool daemon_mode = false;
    int server_fd;
    int exit_status = EXIT_SUCCESS;
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct timer_data timer = {
        .state_mutex = PTHREAD_MUTEX_INITIALIZER,
        .state_changed = PTHREAD_COND_INITIALIZER,
        .file_mutex = &file_mutex,
        .stop = false,
    };
    pthread_t timer_thread;
    bool timer_started = false;
    struct thread_data *threads = NULL;

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

    if (create_thread_with_signals_blocked(&timer_thread, timestamp_worker,
                                           &timer) != 0) {
        syslog(LOG_ERR, "Unable to create timestamp thread");
        exit_status = -1;
        signal_caught = 1;
    } else {
        timer_started = true;
    }

    while (!signal_caught) {
        struct sockaddr_in client_address;
        socklen_t client_length = sizeof(client_address);
        struct thread_data *data;
        int client_fd = accept(server_fd, (struct sockaddr *)&client_address,
                               &client_length);

        if (client_fd == -1) {
            if (errno == EINTR) {
                continue;
            }
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            exit_status = -1;
            break;
        }

        data = calloc(1, sizeof(*data));
        if (data == NULL) {
            syslog(LOG_ERR, "Unable to allocate thread data: %s",
                   strerror(errno));
            close(client_fd);
            exit_status = -1;
            break;
        }
        data->client_fd = client_fd;
        data->file_mutex = &file_mutex;
        atomic_init(&data->complete, false);
        if (inet_ntop(AF_INET, &client_address.sin_addr, data->client_ip,
                      sizeof(data->client_ip)) == NULL) {
            strcpy(data->client_ip, "unknown");
        }
        syslog(LOG_INFO, "Accepted connection from %s", data->client_ip);

        if (create_thread_with_signals_blocked(&data->thread, handle_client,
                                               data) != 0) {
            syslog(LOG_ERR, "Unable to create client thread for %s",
                   data->client_ip);
            close(client_fd);
            free(data);
            exit_status = -1;
            break;
        }
        data->next = threads;
        threads = data;
        reap_completed_threads(&threads);
    }

    if (signal_caught) {
        syslog(LOG_INFO, "Caught signal, exiting");
    }
    if (close(server_fd) == -1) {
        syslog(LOG_ERR, "Unable to close server socket: %s", strerror(errno));
        exit_status = -1;
    }
    stop_and_join_threads(threads);

    if (timer_started) {
        if (pthread_mutex_lock(&timer.state_mutex) == 0) {
            timer.stop = true;
            (void)pthread_cond_signal(&timer.state_changed);
            (void)pthread_mutex_unlock(&timer.state_mutex);
        }
        (void)pthread_join(timer_thread, NULL);
    }

    if ((unlink(DATA_FILE) == -1) && (errno != ENOENT)) {
        syslog(LOG_ERR, "Unable to remove %s: %s", DATA_FILE,
               strerror(errno));
        exit_status = -1;
    }
    (void)pthread_cond_destroy(&timer.state_changed);
    (void)pthread_mutex_destroy(&timer.state_mutex);
    (void)pthread_mutex_destroy(&file_mutex);
    closelog();
    return exit_status;
}
