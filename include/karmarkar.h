#ifndef KARMARKAR_H
#define KARMARKAR_H
#include <stddef.h>
#include <stdbool.h>

// Перечисление возможных статусов завершения работы алгоритма

typedef enum {
    KARMARKAR_SUCCESS,
    KARMARKAR_MAX_ITER_REACHED,
    KARMARKAR_NUMERIC_ERROR
} KarmarkarStatus;

// Структура хранит матрицу A, вектор c и размеры задачи в канонической форме

typedef struct {
    int m; // Количество ограничений (строк матрицы A)
    int n; // Количество переменных (столбцов матрицы A)
    double *A; // Матрица ограничений размера m x n (хранение column-major)
    double *c; // Вектор целевой функции размера n
} KarmarkarProblem;

// Структура хранит вектор решения, количество итераций и статус завершения

typedef struct {
    double *x; // Вектор решения размера n
    int iterations; // Количество выполненных итераций
    KarmarkarStatus status; // Статус завершения алгоритма
    double objective; // Значение целевой функции в найденной точке
} KarmarkarResult;

// Функция решает задачу ЛП алгоритмом Кармаркара и возвращает структуру результата

KarmarkarResult karmarkar_solve(const KarmarkarProblem *prob, double tol, int max_iter, double alpha);

// Функция освобождает память, выделенную под вектор решения в структуре результата

void karmarkar_result_free(KarmarkarResult *res);

#endif // KARMARKAR_H