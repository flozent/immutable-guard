/*
 * immutable-guard-helper.c
 * Вспомогательная программа для повышения привилегий
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define LOG_FILE "/var/log/immutable-guard-helper.log"

void log_action(const char *action, const char *status) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        fprintf(log, "[%ld] UID:%d EUID:%d PID:%d - %s: %s\n",
                (long)time(NULL), getuid(), geteuid(), getpid(),
                action, status);
        fclose(log);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2 || strcmp(argv[1], "elevate") != 0) {
        fprintf(stderr, "Использование: %s elevate\n", argv[0]);
        log_action("INVALID_ARGS", "Неверные аргументы командной строки");
        exit(EXIT_FAILURE);
    }
    
    if (geteuid() != 0) {
        fprintf(stderr, "Ошибка: программа должна быть setuid root\n");
        fprintf(stderr, "Текущий EUID: %d, UID: %d\n", geteuid(), getuid());
        log_action("NOT_SETUID", "Программа запущена без setuid бита");
        exit(EXIT_FAILURE);
    }
    
    log_action("PRIVILEGES_ELEVATED", "Привилегии успешно повышены");
    exit(EXIT_SUCCESS);
}
