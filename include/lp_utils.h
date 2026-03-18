#ifndef LP_UTILS_H
#define LP_UTILS_H

#include "karmarkar.h"

// Генерирует тестовую задачу ЛП в канонической форме для алгоритма Кармаркара
// seed - значение для инициализации генератора случайных чисел (повторяемость тестов)
KarmarkarProblem lp_utils_generate_problem(int m, int n, unsigned int seed);

// Загружает задачу ЛП из текстового файла
KarmarkarProblem lp_utils_load_problem(const char *filename);

// Сохраняет задачу ЛП в текстовый файл
// Возвращает 0 при успехе, -1 при ошибке
int lp_utils_save_problem(const KarmarkarProblem *prob, const char *filename);

// Освобождает память, выделенную под структуру задачи
void lp_utils_free_problem(KarmarkarProblem *prob);

// Проверяет, удовлетворяет ли задача условиям алгоритма Кармаркара
// Возвращает 1 если задача валидна, 0 если нет
int lp_utils_validate_problem(const KarmarkarProblem *prob);

#endif // LP_UTILS_H