#include "lp_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <cblas.h>

// Генерирует тестовую задачу ЛП в канонической форме для алгоритма Кармаркара
KarmarkarProblem lp_utils_generate_problem(int m, int n, unsigned int seed) {
    KarmarkarProblem prob;
    prob.m = m;
    prob.n = n;
    
    // Выделяем память под матрицу A и вектор c
    prob.A = (double*)malloc(m * n * sizeof(double));
    prob.c = (double*)malloc(n * sizeof(double));
    
    if (!prob.A || !prob.c) {
        free(prob.A);
        free(prob.c);
        prob.A = NULL;
        prob.c = NULL;
        return prob;
    }
    
    // Инициализируем ГСЧ
    srand(seed);
    
    // Генерируем матрицу A так, чтобы сумма каждого столбца равнялась 0
    // Это гарантирует, что точка (1/n, ..., 1/n) удовлетворяет Ax = 0
    for (int i = 0; i < m; i++) {
        double row_sum = 0.0;
        // Заполняем первые n-1 элементов строки случайными значениями
        for (int j = 0; j < n - 1; j++) {
            prob.A[i + j * m] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            row_sum += prob.A[i + j * m];
        }
        // Последний элемент делаем таким, чтобы сумма строки равнялась 0
        prob.A[i + (n - 1) * m] = -row_sum;
    }
    
    // Генерируем вектор c
    // Для простоты делаем так, чтобы оптимальное значение было 0
    for (int i = 0; i < n; i++) {
        prob.c[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    }
    
    // Корректируем c так, чтобы c^T * (1/n, ..., 1/n) = 0
    // Это гарантирует, что оптимальное значение целевой функции равно 0
    double c_sum = 0.0;
    for (int i = 0; i < n; i++) {
        c_sum += prob.c[i];
    }
    double c_mean = c_sum / n;
    for (int i = 0; i < n; i++) {
        prob.c[i] -= c_mean;
    }
    
    return prob;
}

// Загружает задачу ЛП из текстового файла
KarmarkarProblem lp_utils_load_problem(const char *filename) {
    KarmarkarProblem prob;
    prob.A = NULL;
    prob.c = NULL;
    prob.m = 0;
    prob.n = 0;
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        return prob;
    }
    
    // Читаем размеры задачи
    if (fscanf(file, "%d %d", &prob.m, &prob.n) != 2) {
        fclose(file);
        prob.m = 0;
        prob.n = 0;
        return prob;
    }
    
    // Выделяем память
    prob.A = (double*)malloc(prob.m * prob.n * sizeof(double));
    prob.c = (double*)malloc(prob.n * sizeof(double));
    
    if (!prob.A || !prob.c) {
        free(prob.A);
        free(prob.c);
        prob.A = NULL;
        prob.c = NULL;
        fclose(file);
        return prob;
    }
    
    // Читаем матрицу A (column-major формат)
    for (int j = 0; j < prob.n; j++) {
        for (int i = 0; i < prob.m; i++) {
            if (fscanf(file, "%lf", &prob.A[i + j * prob.m]) != 1) {
                fclose(file);
                free(prob.A);
                free(prob.c);
                prob.A = NULL;
                prob.c = NULL;
                prob.m = 0;
                prob.n = 0;
                return prob;
            }
        }
    }
    
    // Читаем вектор c
    for (int i = 0; i < prob.n; i++) {
        if (fscanf(file, "%lf", &prob.c[i]) != 1) {
            fclose(file);
            free(prob.A);
            free(prob.c);
            prob.A = NULL;
            prob.c = NULL;
            prob.m = 0;
            prob.n = 0;
            return prob;
        }
    }
    
    fclose(file);
    return prob;
}

// Сохраняет задачу ЛП в текстовый файл
int lp_utils_save_problem(const KarmarkarProblem *prob, const char *filename) {
    if (!prob || !prob->A || !prob->c) {
        return -1;
    }
    
    FILE *file = fopen(filename, "w");
    if (!file) {
        return -1;
    }
    
    // Записываем размеры задачи
    fprintf(file, "%d %d\n", prob->m, prob->n);
    
    // Записываем матрицу A (column-major формат)
    for (int j = 0; j < prob->n; j++) {
        for (int i = 0; i < prob->m; i++) {
            fprintf(file, "%.15e ", prob->A[i + j * prob->m]);
        }
        fprintf(file, "\n");
    }
    
    // Записываем вектор c
    for (int i = 0; i < prob->n; i++) {
        fprintf(file, "%.15e ", prob->c[i]);
    }
    fprintf(file, "\n");
    
    fclose(file);
    return 0;
}

// Освобождает память, выделенную под структуру задачи
void lp_utils_free_problem(KarmarkarProblem *prob) {
    if (prob) {
        free(prob->A);
        free(prob->c);
        prob->A = NULL;
        prob->c = NULL;
        prob->m = 0;
        prob->n = 0;
    }
}

// Проверяет, удовлетворяет ли задача условиям алгоритма Кармаркара
int lp_utils_validate_problem(const KarmarkarProblem *prob) {
    if (!prob || !prob->A || !prob->c) {
        return 0;
    }
    
    if (prob->m <= 0 || prob->n <= 0) {
        return 0;
    }
    
    if (prob->n <= prob->m) {
        // Нужно больше переменных чем ограничений для нетривиальной задачи
        return 0;
    }
    
    // Проверяем что сумма каждого столбца матрицы A равна 0
    // Это гарантирует что точка (1/n, ..., 1/n) удовлетворяет Ax = 0
    double *row_sums = (double*)calloc(prob->m, sizeof(double));
    if (!row_sums) {
        return 0;
    }
    
    for (int j = 0; j < prob->n; j++) {
        for (int i = 0; i < prob->m; i++) {
            row_sums[i] += prob->A[i + j * prob->m];
        }
    }
    
    double tol = 1e-10;
    for (int i = 0; i < prob->m; i++) {
        if (fabs(row_sums[i]) > tol) {
            free(row_sums);
            return 0;
        }
    }
    free(row_sums);
    
    // Проверяем что c^T * (1/n, ..., 1/n) = 0
    // Это гарантирует что оптимальное значение целевой функции равно 0
    double c_sum = 0.0;
    for (int i = 0; i < prob->n; i++) {
        c_sum += prob->c[i];
    }
    
    if (fabs(c_sum / prob->n) > tol) {
        return 0;
    }
    
    return 1;
}