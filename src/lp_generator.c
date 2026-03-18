#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "karmarkar.h"
#include "lp_utils.h"

// Выводит инструкцию по использованию программы
void print_usage(const char *program_name) {
    printf("Использование: %s <m> <n> <seed> <output_file>\n", program_name);
    printf("  m           - количество ограничений (строк матрицы A)\n");
    printf("  n           - количество переменных (столбцов матрицы A, должно быть > m)\n");
    printf("  seed        - значение для инициализации генератора случайных чисел\n");
    printf("  output_file - имя файла для сохранения задачи\n");
    printf("\nПример: %s 5 20 12345 problem.txt\n", program_name);
}

// Парсит целое число из строки с проверкой ошибок
int parse_int(const char *str, int *value) {
    char *endptr;
    long val = strtol(str, &endptr, 10);
    if (*endptr != '\0' || val < 0 || val > INT_MAX) {
        return -1;
    }
    *value = (int)val;
    return 0;
}

// Парсит unsigned int из строки с проверкой ошибок
int parse_uint(const char *str, unsigned int *value) {
    char *endptr;
    unsigned long val = strtoul(str, &endptr, 10);
    if (*endptr != '\0' || val > UINT_MAX) {
        return -1;
    }
    *value = (unsigned int)val;
    return 0;
}

int main(int argc, char *argv[]) {
    // Проверяем количество аргументов
    if (argc != 5) {
        fprintf(stderr, "Ошибка: неверное количество аргументов\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    int m, n;
    unsigned int seed;
    const char *output_file = argv[4];
    
    // Парсим аргументы
    if (parse_int(argv[1], &m) != 0) {
        fprintf(stderr, "Ошибка: некорректное значение m '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }
    
    if (parse_int(argv[2], &n) != 0) {
        fprintf(stderr, "Ошибка: некорректное значение n '%s'\n", argv[2]);
        return EXIT_FAILURE;
    }
    
    if (parse_uint(argv[3], &seed) != 0) {
        fprintf(stderr, "Ошибка: некорректное значение seed '%s'\n", argv[3]);
        return EXIT_FAILURE;
    }
    
    // Проверяем ограничения на размеры задачи
    if (m <= 0) {
        fprintf(stderr, "Ошибка: m должно быть положительным числом\n");
        return EXIT_FAILURE;
    }
    
    if (n <= m) {
        fprintf(stderr, "Ошибка: n должно быть больше m (требуется n > m)\n");
        return EXIT_FAILURE;
    }
    
    printf("Генерация задачи ЛП...\n");
    printf("  m = %d (ограничений)\n", m);
    printf("  n = %d (переменных)\n", n);
    printf("  seed = %u\n", seed);
    printf("  файл = %s\n", output_file);
    
    // Генерируем задачу
    KarmarkarProblem prob = lp_utils_generate_problem(m, n, seed);
    
    if (!prob.A || !prob.c) {
        fprintf(stderr, "Ошибка: не удалось выделить память для задачи\n");
        return EXIT_FAILURE;
    }
    
    // Проверяем сгенерированную задачу
    if (!lp_utils_validate_problem(&prob)) {
        fprintf(stderr, "Ошибка: сгенерированная задача не удовлетворяет условиям алгоритма\n");
        lp_utils_free_problem(&prob);
        return EXIT_FAILURE;
    }
    
    printf("Задача успешно сгенерирована и проверена\n");
    
    // Сохраняем задачу в файл
    int save_result = lp_utils_save_problem(&prob, output_file);
    
    if (save_result != 0) {
        fprintf(stderr, "Ошибка: не удалось сохранить задачу в файл '%s'\n", output_file);
        lp_utils_free_problem(&prob);
        return EXIT_FAILURE;
    }
    
    printf("Задача успешно сохранена в файл '%s'\n", output_file);
    
    // Освобождаем память
    lp_utils_free_problem(&prob);
    
    printf("Готово!\n");
    return EXIT_SUCCESS;
}