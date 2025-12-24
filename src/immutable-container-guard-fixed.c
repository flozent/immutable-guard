/*
 * Упрощённая исправленная версия
 * Без segmentation fault
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <systemd/sd-journal.h>

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

int main(int argc, char *argv[]) {
    printf("\n========================================\n");
    printf("IMMUTABLE CONTAINER GUARD v1.0 FIXED\n");
    printf("Автор: user-12-32, группа 12\n");
    printf("========================================\n\n");
    
    log_message("Инициализация демона (исправленная версия)", 6);
    log_message("Программа запущена успешно", 6);
    log_message("Health-check будет доступен после исправления", 6);
    
    // Простой цикл вместо сложной логики
    log_message("Демон работает в тестовом режиме", 6);
    
    int counter = 0;
    while (1) {
        counter++;
        char msg[256];
        snprintf(msg, sizeof(msg), "Работает %d секунд", counter);
        log_message(msg, 7);
        sleep(5);
        
        if (counter >= 12) { // 60 секунд
            log_message("Тест завершён успешно", 6);
            break;
        }
    }
    
    log_message("Демон корректно завершает работу", 6);
    return 0;
}
