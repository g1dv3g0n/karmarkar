#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include "karmarkar.h"
#include "lp_utils.h"

// Выводит инструкцию по использованию программы
void print_usage(const char *program_name) {
    printf("Использование: %s <input_file> <tol> <max_iter> <alpha> <output_file>\n", program_name);
    printf("  input_file  - имя файла с задачей ЛП\n");
    printf("  tol         - допустимая погрешность целевой функции (например, 1e-6)\n");
    printf("  max_iter    - максимальное количество итераций (например, 1000)\n");
    printf("  alpha       - параметр шага алгоритма, 0 < alpha < 1 (например, 0.25)\n");
    printf("  output_file - имя файла для сохранения решения\n");
    printf("\nПример: %s problem.txt 1e-6 1000 0.25 solution.txt\n", program_name);
}

// Парсит double из строки с проверкой ошибок
int parse_double(const char *str, double *value) {
    char *endptr;
    *value = strtod(str, &endptr);
    if (*endptr != '\0') {
        return -1;
    }
    return 0;
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

// Сохраняет решение в текстовый файл
int save_solution(const char *filename, const KarmarkarResult *result, int n, double elapsed_time) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        return -1;
    }
    
    const char *status_str;
    switch (result->status) {
        case KARMARKAR_SUCCESS:
            status_str = "SUCCESS";
            break;
        case KARMARKAR_MAX_ITER_REACHED:
            status_str = "MAX_ITER_REACHED";
            break;
        case KARMARKAR_NUMERIC_ERROR:
            status_str = "NUMERIC_ERROR";
            break;
        default:
            status_str = "UNKNOWN";
    }
    
    fprintf(file, "# Решение задачи ЛП алгоритмом Кармаркара\n");
    fprintf(file, "status=%s\n", status_str);
    fprintf(file, "iterations=%d\n", result->iterations);
    fprintf(file, "time_seconds=%.6f\n", elapsed_time);
    fprintf(file, "objective=%.15e\n", result->objective);
    fprintf(file, "n_variables=%d\n\n", n);
    
    fprintf(file, "# Вектор решения x (по одному значению в строке):\n");
    for (int i = 0; i < n; i++) {
        fprintf(file, "x[%d]=%.15e\n", i, result->x[i]);
    }
    
    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    // Проверяем количество аргументов
    if (argc != 6) {
        fprintf(stderr, "Ошибка: неверное количество аргументов\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    const char *input_file = argv[1];
    double tol;
    int max_iter;
    double alpha;
    const char *output_file = argv[5];
    
    // Парсим аргументы
    if (parse_double(argv[2], &tol) != 0) {
        fprintf(stderr, "Ошибка: некорректное значение tol '%s'\n", argv[2]);
        return EXIT_FAILURE;
    }
    
    if (parse_int(argv[3], &max_iter) != 0) {
        fprintf(stderr, "Ошибка: некорректное значение max_iter '%s'\n", argv[3]);
        return EXIT_FAILURE;
    }
    
    if (parse_double(argv[4], &alpha) != 0) {
        fprintf(stderr, "Ошибка: некорректное значение alpha '%s'\n", argv[4]);
        return EXIT_FAILURE;
    }
    
    // Проверяем диапазоны параметров
    if (tol <= 0) {
        fprintf(stderr, "Ошибка: tol должно быть положительным числом\n");
        return EXIT_FAILURE;
    }
    
    if (max_iter <= 0) {
        fprintf(stderr, "Ошибка: max_iter должно быть положительным числом\n");
        return EXIT_FAILURE;
    }
    
    if (alpha <= 0 || alpha >= 1) {
        fprintf(stderr, "Ошибка: alpha должно быть в диапазоне (0, 1)\n");
        return EXIT_FAILURE;
    }
    
    printf("Загрузка задачи ЛП из файла '%s'...\n", input_file);
    
    // Загружаем задачу из файла
    KarmarkarProblem prob = lp_utils_load_problem(input_file);
    
    if (!prob.A || !prob.c) {
        fprintf(stderr, "Ошибка: не удалось загрузить задачу из файла '%s'\n", input_file);
        return EXIT_FAILURE;
    }
    
    printf("Задача загружена успешно:\n");
    printf("  m = %d (ограничений)\n", prob.m);
    printf("  n = %d (переменных)\n", prob.n);
    
    // Проверяем задачу
    if (!lp_utils_validate_problem(&prob)) {
        fprintf(stderr, "Ошибка: задача не удовлетворяет условиям алгоритма Кармаркара\n");
        lp_utils_free_problem(&prob);
        return EXIT_FAILURE;
    }
    
    printf("Задача прошла валидацию\n\n");
    
    printf("Запуск алгоритма Кармаркара...\n");
    printf("  tol = %.2e\n", tol);
    printf("  max_iter = %d\n", max_iter);
    printf("  alpha = %.2f\n\n", alpha);
    
    // Засекаем время начала
    clock_t start_time = clock();
    
    // Решаем задачу
    KarmarkarResult result = karmarkar_solve(&prob, tol, max_iter, alpha);
    
    // Засекаем время окончания
    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    // Выводим результаты
    printf("\n=== Результаты ===\n");
    
    const char *status_str;
    switch (result.status) {
        case KARMARKAR_SUCCESS:
            status_str = "УСПЕХ (достигнута требуемая точность)";
            break;
        case KARMARKAR_MAX_ITER_REACHED:
            status_str = "ПРЕВЫШЕНО МАКС. ЧИСЛО ИТЕРАЦИЙ";
            break;
        case KARMARKAR_NUMERIC_ERROR:
            status_str = "ЧИСЛЕННАЯ ОШИБКА";
            break;
        default:
            status_str = "НЕИЗВЕСТНЫЙ СТАТУС";
    }
    
    printf("Статус: %s\n", status_str);
    printf("Итераций выполнено: %d\n", result.iterations);
    printf("Значение целевой функции: %.15e\n", result.objective);
    printf("Время выполнения: %.6f сек\n", elapsed_time);
    
    // Выводим статистику по решению
    if (result.status == KARMARKAR_SUCCESS || result.status == KARMARKAR_MAX_ITER_REACHED) {
        double min_x = DBL_MAX;
        double max_x = -DBL_MAX;
        double sum_x = 0.0;
        
        for (int i = 0; i < prob.n; i++) {
            if (result.x[i] < min_x) min_x = result.x[i];
            if (result.x[i] > max_x) max_x = result.x[i];
            sum_x += result.x[i];
        }
        
        printf("\nСтатистика решения:\n");
        printf("  Минимальный x[i]: %.15e\n", min_x);
        printf("  Максимальный x[i]: %.15e\n", max_x);
        printf("  Сумма x[i]: %.15e (должно быть ~1.0)\n", sum_x);
    }
    
    // Сохраняем решение в файл
    printf("\nСохранение решения в файл '%s'...\n", output_file);
    
    int save_result = save_solution(output_file, &result, prob.n, elapsed_time);
    
    if (save_result != 0) {
        fprintf(stderr, "Предупреждение: не удалось сохранить решение в файл '%s'\n", output_file);
    } else {
        printf("Решение успешно сохранено\n");
    }
    
    // Освобождаем память
    lp_utils_free_problem(&prob);
    karmarkar_result_free(&result);
    
    printf("\nГотово!\n");
    
    // Возвращаем код завершения в зависимости от статуса
    if (result.status == KARMARKAR_SUCCESS) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}