#include "karmarkar.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cblas.h>
#include <lapacke.h>

// Вычисляет проекцию градиента на ядро ограничений с использованием BLAS/LAPACK
static void compute_projection(const double *A, int m, int n, const double *D, 
                               const double *c, double *c_proj, double *workspace) {
    double *AD = workspace;
    double *ADADT = workspace + m * n;
    double *Dc = workspace + m * n + m * m;
    double *temp = workspace + m * n + m * m + n;
    int *ipiv = (int*)(workspace + m * n + m * m + n + m);
    
    // Строим матрицу AD = A * D (по столбцам)
    for (int j = 0; j < n; j++) {
        cblas_dscal(m, D[j], (double*)&A[j * m], 1);
        cblas_dcopy(m, (double*)&A[j * m], 1, &AD[j * m], 1);
        cblas_dscal(m, 1.0 / D[j], (double*)&A[j * m], 1);
    }
    
    // Вычисляем Dc = D * c (поэлементное умножение)
    for (int i = 0; i < n; i++) {
        Dc[i] = D[i] * c[i];
    }
    
    // Вычисляем ADADT = AD * AD^T
    cblas_dgemm(CblasColMajor, CblasNoTrans, CblasTrans, m, m, n, 
                1.0, AD, m, AD, m, 0.0, ADADT, m);
    
    // Копируем ADADT для LU-разложения
    cblas_dcopy(m * m, ADADT, 1, ADADT + m * m, 1);
    
    // Выполняем LU-разложение ADADT
    LAPACKE_dgetrf(LAPACK_COL_MAJOR, m, m, ADADT, m, ipiv);
    
    // Вычисляем temp = AD * Dc
    cblas_dgemv(CblasColMajor, CblasNoTrans, m, n, 1.0, AD, m, Dc, 1, 0.0, temp, 1);
    
    // Решаем систему ADADT * lambda = temp
    LAPACKE_dgetrs(LAPACK_COL_MAJOR, CblasNoTrans, m, 1, ADADT, m, ipiv, temp, m);
    
    // Вычисляем c_proj = Dc - AD^T * lambda
    cblas_dgemv(CblasColMajor, CblasTrans, m, n, 1.0, AD, m, temp, 1, 0.0, c_proj, 1);
    cblas_daxpy(n, -1.0, c_proj, 1, Dc, 1);
    cblas_dcopy(n, Dc, 1, c_proj, 1);
}

// Основная функция решения задачи ЛП алгоритмом Кармаркара
KarmarkarResult karmarkar_solve(const KarmarkarProblem *prob, double tol, int max_iter, double alpha) {
    KarmarkarResult result;
    int n = prob->n;
    int m = prob->m;
    
    // Выделяем память под вектор решения
    result.x = (double*)malloc(n * sizeof(double));
    if (!result.x) {
        result.status = KARMARKAR_NUMERIC_ERROR;
        return result;
    }
    
    // Инициализируем начальную точку в центре симплекса
    cblas_dscal(n, 1.0 / n, result.x, 1);
    for (int i = 0; i < n; i++) result.x[i] = 1.0 / n;
    
    // Выделяем рабочую память для вычислений
    int workspace_size = m * n + m * m + n + m + n + n + m;
    double *workspace = (double*)malloc(workspace_size * sizeof(double));
    if (!workspace) {
        free(result.x);
        result.status = KARMARKAR_NUMERIC_ERROR;
        return result;
    }
    
    double *D = workspace;
    double *c_proj = workspace + n;
    double *x_new = workspace + 2 * n;
    double *x_tilde = workspace + 3 * n;
    double *a = workspace + 4 * n;
    
    result.iterations = 0;
    result.status = KARMARKAR_MAX_ITER_REACHED;
    
    // Основной цикл алгоритма
    for (int k = 0; k < max_iter; k++) {
        result.iterations = k + 1;
        
        // Вычисляем текущее значение целевой функции
        result.objective = cblas_ddot(n, prob->c, 1, result.x, 1);
        
        // Проверяем критерий остановки
        if (fabs(result.objective) < tol) {
            result.status = KARMARKAR_SUCCESS;
            break;
        }
        
        // Строим диагональную матрицу масштабирования D
        cblas_dcopy(n, result.x, 1, D, 1);
        
        // Вычисляем проекцию градиента
        compute_projection(prob->A, m, n, D, prob->c, c_proj, workspace + n);
        
        // Вычисляем норму проекции
        double norm_cp = cblas_dnrm2(n, c_proj, 1);
        
        // Проверяем на численные ошибки
        if (norm_cp < 1e-10) {
            result.status = KARMARKAR_NUMERIC_ERROR;
            break;
        }
        
        // Инициализируем центр симплекса
        for (int i = 0; i < n; i++) {
            a[i] = 1.0 / n;
        }
        
        // y_new = a - alpha * c_proj / ||c_proj||
        cblas_dcopy(n, a, 1, x_new, 1);
        cblas_daxpy(n, -alpha / norm_cp, c_proj, 1, x_new, 1);
        
        // Проверяем что точка внутри допустимой области
        bool feasible = true;
        for (int i = 0; i < n; i++) {
            if (x_new[i] <= 1e-10) {
                feasible = false;
                break;
            }
        }
        
        if (!feasible) {
            result.status = KARMARKAR_NUMERIC_ERROR;
            break;
        }
        
        // Обратное преобразование: x_tilde = D * y_new (поэлементное умножение)
        for (int i = 0; i < n; i++) {
            x_tilde[i] = D[i] * x_new[i];
        }
        
        // Нормализация: сумма компонент должна равняться 1
        double sum = cblas_dasum(n, x_tilde, 1);
        
        if (sum < 1e-10) {
            result.status = KARMARKAR_NUMERIC_ERROR;
            break;
        }
        
        // Обновляем решение
        cblas_dscal(n, 1.0 / sum, x_tilde, 1);
        cblas_dcopy(n, x_tilde, 1, result.x, 1);
    }
    
    // Вычисляем финальное значение целевой функции
    result.objective = cblas_ddot(n, prob->c, 1, result.x, 1);
    
    // Освобождаем рабочую память
    free(workspace);
    
    return result;
}

// Функция освобождает память структуры результата
void karmarkar_result_free(KarmarkarResult *res) {
    if (res && res->x) {
        free(res->x);
        res->x = NULL;
    }
}