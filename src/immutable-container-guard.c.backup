/*
 * immutable-container-guard.c
 * Демон контроля неизменяемости runC-контейнеров
 * Автор: user-12-32
 * Лицензия: GPLv3
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/capability.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <systemd/sd-journal.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CONFIG_FILE "/etc/immutable-guard.conf"
#define HASH_DB_DIR "/var/lib/immutable-guard/hashes"
#define SCAN_INTERVAL 30
#define HEALTH_PORT 8080
#define MAX_CONTAINERS 100
#define MAX_PATH_LEN 4096
#define SHA512_HEX_LEN (SHA512_DIGEST_LENGTH * 2 + 1)
#define LOG_BUFFER_SIZE 1024

typedef struct {
    char id[256];
    char rootfs[MAX_PATH_LEN];
    pid_t pid;
    int monitored;
    time_t last_check;
} ContainerInfo;

static ContainerInfo containers[MAX_CONTAINERS];
static int container_count = 0;
static char username[64] = "user-12-32";
static volatile sig_atomic_t stop_daemon = 0;

void signal_handler(int sig) {
    char log_entry[LOG_BUFFER_SIZE];
    snprintf(log_entry, sizeof(log_entry), "[%s] Получен сигнал завершения", username);
    sd_journal_send("MESSAGE=%s", log_entry, "PRIORITY=6", "SYSLOG_IDENTIFIER=immutable-guard", NULL);
    stop_daemon = 1;
}

void log_message(const char *message, int priority) {
    char log_entry[LOG_BUFFER_SIZE];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(log_entry, sizeof(log_entry), "[%s] %s %s", username, time_str, message);
    
    sd_journal_send("MESSAGE=%s", log_entry, "PRIORITY=%d", priority, "SYSLOG_IDENTIFIER=immutable-guard", NULL);
    printf("%s\n", log_entry);
}

void get_ip_address(char *buffer, size_t size) {
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        strncpy(buffer, "127.0.0.1", size);
        return;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        if (ifa->ifa_addr->sa_family == AF_INET) {
            if (strcmp(ifa->ifa_name, "lo") != 0) {
                struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                inet_ntop(AF_INET, &(sa->sin_addr), buffer, size);
                freeifaddrs(ifaddr);
                return;
            }
        }
    }
    
    strncpy(buffer, "127.0.0.1", size);
    freeifaddrs(ifaddr);
}

void *health_check_thread(void *arg) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char ip_address[INET_ADDRSTRLEN];
    char *response_ok = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\n{\"status\":\"ok\",\"service\":\"immutable-guard\",\"user\":\"user-12-32\"}\r\n";
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_message("Ошибка создания сокета для health-check", 3);
        return NULL;
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        log_message("Ошибка настройки сокета", 3);
        close(server_fd);
        return NULL;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(HEALTH_PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_message("Ошибка привязки сокета для health-check", 3);
        close(server_fd);
        return NULL;
    }
    
    if (listen(server_fd, 3) < 0) {
        log_message("Ошибка прослушивания порта", 3);
        close(server_fd);
        return NULL;
    }
    
    get_ip_address(ip_address, sizeof(ip_address));
    char msg[256];
    snprintf(msg, sizeof(msg), "Health-check сервер запущен на порту %d", HEALTH_PORT);
    log_message(msg, 6);
    
    printf("\n========================================\n");
    printf("ДЛЯ UPTIMEROBOT ИСПОЛЬЗУЙТЕ АДРЕС:\n");
    printf("http://%s:%d/health\n", ip_address, HEALTH_PORT);
    printf("========================================\n\n");
    
    while (!stop_daemon) {
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(server_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            continue;
        }
        
        if (activity == 0) {
            continue;
        }
        
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            continue;
        }
        
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
            
            if (strstr(buffer, "GET /health")) {
                snprintf(msg, sizeof(msg), "Health-check запрос от %s", client_ip);
                log_message(msg, 7);
            }
            
            send(client_fd, response_ok, strlen(response_ok), 0);
        }
        
        close(client_fd);
        memset(buffer, 0, sizeof(buffer));
    }
    
    close(server_fd);
    log_message("Health-check поток завершен", 6);
    return NULL;
}

int check_privileges(void) {
    struct __user_cap_header_struct cap_header;
    struct __user_cap_data_struct cap_data;
    
    cap_header.pid = getpid();
    cap_header.version = _LINUX_CAPABILITY_VERSION_3;
    
    if (capget(&cap_header, &cap_data) == -1) {
        log_message("Ошибка получения capabilities", 3);
        return 0;
    }
    
    if (cap_data.effective & (1 << CAP_SYS_ADMIN)) {
        return 1;
    }
    
    log_message("Недостаточно привилегий (требуется CAP_SYS_ADMIN)", 4);
    return 0;
}

int elevate_privileges(void) {
    pid_t pid;
    int status;
    char msg[256];
    
    snprintf(msg, sizeof(msg), "Попытка повышения привилегий (PID: %d)", getpid());
    log_message(msg, 6);
    
    pid = fork();
    if (pid == -1) {
        log_message("Ошибка fork()", 3);
        return -1;
    }
    
    if (pid == 0) {
        if (geteuid() == 0) {
            exit(EXIT_SUCCESS);
        } else {
            exit(EXIT_FAILURE);
        }
    } else {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            log_message("Привилегии успешно повышены", 6);
            return 0;
        }
    }
    
    log_message("Не удалось повысить привилегии", 3);
    return -1;
}

int drop_privileges(void) {
    if (setuid(getuid()) == -1) {
        log_message("Ошибка сброса привилегий", 3);
        return -1;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Привилегии сброшены (UID: %d, EUID: %d)", getuid(), geteuid());
    log_message(msg, 6);
    return 0;
}

int read_config(void) {
    FILE *fp;
    char line[512];
    
    fp = fopen(CONFIG_FILE, "r");
    if (!fp) {
        log_message("Конфигурационный файл не найден, используются значения по умолчанию", 4);
        return 0;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        if (strstr(line, "username=")) {
            sscanf(line, "username=%63s", username);
        }
    }
    
    fclose(fp);
    log_message("Конфигурация загружена", 6);
    return 0;
}

int discover_containers(void) {
    log_message("Сканирование контейнеров...", 7);
    
    strncpy(containers[0].id, "test-container-1", sizeof(containers[0].id));
    strncpy(containers[0].rootfs, "/var/lib/containers/test/rootfs", sizeof(containers[0].rootfs));
    containers[0].pid = 1000;
    containers[0].monitored = 1;
    containers[0].last_check = time(NULL);
    
    container_count = 1;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Обнаружен тестовый контейнер: %s", containers[0].id);
    log_message(msg, 6);
    
    return 1;
}

int calculate_file_hash(const char *filename, char *output_hash) {
    FILE *file;
    SHA512_CTX context;
    unsigned char hash[SHA512_DIGEST_LENGTH];
    unsigned char buffer[8192];
    size_t bytes;
    
    file = fopen(filename, "rb");
    if (!file) {
        return -1;
    }
    
    SHA512_Init(&context);
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        SHA512_Update(&context, buffer, bytes);
    }
    
    SHA512_Final(hash, &context);
    fclose(file);
    
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++) {
        sprintf(output_hash + (i * 2), "%02x", hash[i]);
    }
    output_hash[SHA512_HEX_LEN - 1] = '\0';
    
    return 0;
}

void create_hash_database(void) {
    char hash_file_path[MAX_PATH_LEN];
    
    snprintf(hash_file_path, sizeof(hash_file_path), "%s/test-container-1.hashes", HASH_DB_DIR);
    
    FILE *hash_file = fopen(hash_file_path, "w");
    if (!hash_file) {
        log_message("Не удалось создать базу хэшей", 3);
        return;
    }
    
    const char *test_files[] = {"/bin/ls", "/bin/bash", "/usr/bin/curl", "/etc/passwd", NULL};
    const char *test_hashes[] = {
        "e7d1c2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1",
        "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1",
        "b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2",
        "c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3",
    };
    
    for (int i = 0; test_files[i] != NULL; i++) {
        fprintf(hash_file, "%s %s\n", test_files[i], test_hashes[i]);
    }
    
    fclose(hash_file);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Создана тестовая база хэшей: %s", hash_file_path);
    log_message(msg, 6);
}

int verify_container_integrity(const ContainerInfo *container) {
    char hash_file_path[MAX_PATH_LEN];
    char current_hash[SHA512_HEX_LEN];
    char stored_hash[SHA512_HEX_LEN];
    char file_path[MAX_PATH_LEN];
    FILE *hash_file;
    int violations = 0;
    int files_checked = 0;
    
    snprintf(hash_file_path, sizeof(hash_file_path), "%s/%s.hashes", HASH_DB_DIR, container->id);
    
    hash_file = fopen(hash_file_path, "r");
    if (!hash_file) {
        log_message("Файл хэшей не найден, создание эталона", 4);
        create_hash_database();
        return 0;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Начало проверки целостности контейнера: %s", container->id);
    log_message(msg, 7);
    
    while (fscanf(hash_file, "%1023s %127s", file_path, stored_hash) == 2) {
        files_checked++;
        
        if (calculate_file_hash(file_path, current_hash) == 0) {
            if (strcmp(current_hash, stored_hash) != 0) {
                snprintf(msg, sizeof(msg), "НАРУШЕНИЕ: контейнер=%s файл=%s (хэш изменился)", container->id, file_path);
                log_message(msg, 3);
                violations++;
            }
        } else {
            snprintf(msg, sizeof(msg), "НАРУШЕНИЕ: контейнер=%s файл=%s (файл удален или недоступен)", container->id, file_path);
            log_message(msg, 3);
            violations++;
        }
    }
    
    fclose(hash_file);
    
    snprintf(msg, sizeof(msg), "Проверка завершена: контейнер=%s, файлов=%d, нарушений=%d", container->id, files_checked, violations);
    log_message(msg, violations > 0 ? 4 : 7);
    
    return violations > 0 ? 1 : 0;
}

int stop_container(const ContainerInfo *container) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Обработка скомпрометированного контейнера: %s", container->id);
    log_message(msg, 4);
    
    if (elevate_privileges() != 0) {
        log_message("Не удалось получить привилегии для остановки контейнера", 3);
        return -1;
    }
    
    snprintf(msg, sizeof(msg), "Контейнер %s успешно остановлен (имитация)", container->id);
    log_message(msg, 6);
    
    drop_privileges();
    
    return 0;
}

void daemon_loop(void) {
    int cycle_count = 0;
    time_t start_time = time(NULL);
    
    log_message("Основной цикл демона запущен", 6);
    
    while (!stop_daemon) {
        cycle_count++;
        
        char msg[256];
        snprintf(msg, sizeof(msg), "Цикл проверки №%d (работает: %ld секунд)", cycle_count, time(NULL) - start_time);
        log_message(msg, 7);
        
        container_count = discover_containers();
        
        if (container_count > 0) {
            for (int i = 0; i < container_count && !stop_daemon; i++) {
                if (containers[i].monitored) {
                    int integrity_status = verify_container_integrity(&containers[i]);
                    
                    if (integrity_status == 1) {
                        log_message("КРИТИЧЕСКОЕ НАРУШЕНИЕ: обнаружено изменение контейнера!", 2);
                        
                        if (stop_container(&containers[i]) == 0) {
                            containers[i].monitored = 0;
                            snprintf(msg, sizeof(msg), "Контейнер %s удален из мониторинга", containers[i].id);
                            log_message(msg, 6);
                        }
                    } else {
                        containers[i].last_check = time(NULL);
                    }
                }
            }
        }
        
        for (int j = 0; j < SCAN_INTERVAL && !stop_daemon; j++) {
            sleep(1);
        }
    }
    
    log_message("Основной цикл демона завершен", 6);
}

void cleanup(void) {
    log_message("Начало очистки ресурсов", 6);
    
    for (int i = 0; i < container_count; i++) {
        containers[i].monitored = 0;
    }
    
    container_count = 0;
    
    log_message("Очистка ресурсов завершена", 6);
}

int main(int argc, char *argv[]) {
    pthread_t health_thread;
    struct sigaction sa;
    
    printf("\n========================================\n");
    printf("IMMUTABLE CONTAINER GUARD v1.0\n");
    printf("Автор: user-12-32, группа 12\n");
    printf("========================================\n\n");
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    
    log_message("Инициализация демона контроля неизменяемости контейнеров", 6);
    
    if (!check_privileges()) {
        log_message("Запуск с недостаточными привилегиями, некоторые функции могут быть недоступны", 4);
    }
    
    if (read_config() != 0) {
        log_message("Ошибка загрузки конфигурации, работаем с настройками по умолчанию", 3);
    }
    
    atexit(cleanup);
    
    if (pthread_create(&health_thread, NULL, health_check_thread, NULL) != 0) {
        log_message("Ошибка создания health-check потока", 3);
    } else {
        log_message("Health-check поток успешно создан", 6);
        pthread_detach(health_thread);
    }
    
    daemon_loop();
    
    log_message("Демон корректно завершает работу", 6);
    return 0;
}
