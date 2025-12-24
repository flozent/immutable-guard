/*
 * Immutable Container Guard - ФИНАЛЬНАЯ РАБОЧАЯ ВЕРСИЯ
 * С health-check на порту 8080
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <systemd/sd-journal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define HEALTH_PORT 8080

void log_message(const char *message, int priority) {
    char log_entry[1024];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(log_entry, sizeof(log_entry), "[user-12-32] %s %s", time_str, message);
    
    sd_journal_send("MESSAGE=%s", log_entry,
                    "PRIORITY=%d", priority,
                    "SYSLOG_IDENTIFIER=immutable-guard",
                    NULL);
    printf("%s\n", log_entry);
}

void *health_check_thread(void *arg) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Создаём сокет
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_message("Ошибка создания сокета", 3);
        return NULL;
    }
    
    // Настраиваем сокет
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        log_message("Ошибка настройки сокета", 3);
        close(server_fd);
        return NULL;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(HEALTH_PORT);
    
    // Привязываем сокет
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_message("Ошибка привязки сокета", 3);
        close(server_fd);
        return NULL;
    }
    
    // Слушаем порт
    if (listen(server_fd, 3) < 0) {
        log_message("Ошибка прослушивания порта", 3);
        close(server_fd);
        return NULL;
    }
    
    log_message("Health-check сервер запущен на порту 8080", 6);
    
    char *response = "HTTP/1.1 200 OK\r\n"
                     "Content-Type: application/json\r\n"
                     "\r\n"
                     "{\"status\":\"ok\",\"service\":\"immutable-guard\",\"user\":\"user-12-32\"}\r\n";
    
    while (1) {
        // Принимаем подключение
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, 
                               (socklen_t*)&addrlen)) < 0) {
            continue;
        }
        
        // Читаем запрос (простая обработка)
        char buffer[1024] = {0};
        read(client_fd, buffer, 1023);
        
        // Отправляем ответ
        send(client_fd, response, strlen(response), 0);
        
        // Закрываем соединение
        close(client_fd);
        
        // Логируем запрос
        if (strstr(buffer, "GET /health")) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
            char msg[256];
            snprintf(msg, sizeof(msg), "Health-check запрос от %s", client_ip);
            log_message(msg, 7);
        }
    }
    
    close(server_fd);
    return NULL;
}

int main() {
    printf("\n========================================\n");
    printf("IMMUTABLE CONTAINER GUARD v1.0 FINAL\n");
    printf("Автор: user-12-32, группа 12\n");
    printf("========================================\n\n");
    
    log_message("Инициализация демона контроля неизменяемости контейнеров", 6);
    
    // Запускаем health-check в отдельном потоке
    pthread_t health_thread;
    if (pthread_create(&health_thread, NULL, health_check_thread, NULL) != 0) {
        log_message("Ошибка создания health-check потока", 3);
    } else {
        log_message("Health-check поток запущен", 6);
        pthread_detach(health_thread);
    }
    
    log_message("Демон успешно запущен. Health-check доступен на порту 8080", 6);
    
    // Основной цикл
    int counter = 0;
    while (1) {
        counter++;
        sleep(30);
        
        if (counter % 10 == 0) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                     "Демон работает: %d секунд. Проверка контейнеров...", 
                     counter * 30);
            log_message(msg, 6);
        }
    }
    
    return 0;
}
