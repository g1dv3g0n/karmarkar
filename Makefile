# Makefile для проекта Karmarkar-LP
# Поддерживает сборку C-версии и запуск Julia-версии

# ==============================================================================
# Компилятор и флаги
# ==============================================================================
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -I./c/include
LDFLAGS = -lblas -llapacke -llapack -lm

# Флаги для отладочной сборки
DEBUG_FLAGS = -g -DDEBUG

# Julia команда
JULIA = julia
JULIA_PROJECT = --project=./julia

# ==============================================================================
# Директории
# ==============================================================================
C_SRC_DIR = c/src
C_INC_DIR = c/include
C_BUILD_DIR = c/build
C_BIN_DIR = c/bin

JULIA_SRC_DIR = julia/src
JULIA_BIN_DIR = julia/bin

DATA_DIR = data
PROBLEMS_DIR = $(DATA_DIR)/problems
SOLUTIONS_DIR = $(DATA_DIR)/solutions

# ==============================================================================
# C-версия: исходные и объектные файлы
# ==============================================================================
C_LIB_SRCS = $(C_SRC_DIR)/karmarkar.c $(C_SRC_DIR)/lp_utils.c
C_GEN_SRC = $(C_SRC_DIR)/generator.c
C_SOLVER_SRC = $(C_SRC_DIR)/solver.c

C_LIB_OBJS = $(C_BUILD_DIR)/karmarkar.o $(C_BUILD_DIR)/lp_utils.o
C_GEN_OBJ = $(C_BUILD_DIR)/generator.o
C_SOLVER_OBJ = $(C_BUILD_DIR)/solver.o

C_GEN_BIN = $(C_BIN_DIR)/generator
C_SOLVER_BIN = $(C_BIN_DIR)/solver

# ==============================================================================
# Julia-версия: пути
# ==============================================================================
JULIA_SOLVER = $(JULIA_BIN_DIR)/solver.jl

# ==============================================================================
# Тестовые параметры
# ==============================================================================
TEST_M = 10
TEST_N = 50
TEST_SEED = 42
TEST_TOL = 1e-6
TEST_MAX_ITER = 500
TEST_ALPHA = 0.25

TEST_PROBLEM = $(PROBLEMS_DIR)/test_problem.txt
C_SOLUTION = $(SOLUTIONS_DIR)/c_solution.txt
JULIA_SOLUTION = $(SOLUTIONS_DIR)/julia_solution.txt

# ==============================================================================
# Цели
# ==============================================================================
.PHONY: all c julia dirs clean rebuild debug test compare help

# Сборка по умолчанию (только C, Julia не компилируется)
all: dirs c

# Создание директорий
dirs:
	@mkdir -p $(C_BUILD_DIR)
	@mkdir -p $(C_BIN_DIR)
	@mkdir -p $(JULIA_BIN_DIR)
	@mkdir -p $(PROBLEMS_DIR)
	@mkdir -p $(SOLUTIONS_DIR)

# ==============================================================================
# C-версия
# ==============================================================================
c: dirs $(C_GEN_BIN) $(C_SOLVER_BIN)
	@echo "C-версия собрана успешно"

# Сборка генератора
$(C_GEN_BIN): $(C_LIB_OBJS) $(C_GEN_OBJ)
	$(CC) $(CFLAGS) -o $@ $(C_LIB_OBJS) $(C_GEN_OBJ) $(LDFLAGS)

# Сборка решателя
$(C_SOLVER_BIN): $(C_LIB_OBJS) $(C_SOLVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $(C_LIB_OBJS) $(C_SOLVER_OBJ) $(LDFLAGS)

# Компиляция объектных файлов
$(C_BUILD_DIR)/%.o: $(C_SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Отладочная сборка C
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean c

# ==============================================================================
# Julia-версия
# ==============================================================================
julia: dirs
	@echo "Проверка Julia-окружения..."
	@$(JULIA) $(JULIA_PROJECT) -e 'using Pkg; Pkg.instantiate()'
	@echo "Julia-версия готова к запуску"

# Запуск Julia-решателя (требует аргументы)
julia-run: julia
	@$(JULIA) $(JULIA_PROJECT) $(JULIA_SOLVER) $(ARGS)

# ==============================================================================
# Тестирование
# ==============================================================================
test: c julia
	@echo ""
	@echo "=== Тестирование алгоритма Кармаркара ==="
	@echo ""
	@echo "1. Генерация тестовой задачи (m=$(TEST_M), n=$(TEST_N), seed=$(TEST_SEED))..."
	@$(C_GEN_BIN) $(TEST_M) $(TEST_N) $(TEST_SEED) $(TEST_PROBLEM)
	@echo ""
	@echo "2. Решение C-версией..."
	@time $(C_SOLVER_BIN) $(TEST_PROBLEM) $(TEST_TOL) $(TEST_MAX_ITER) $(TEST_ALPHA) $(C_SOLUTION)
	@echo ""
	@echo "3. Решение Julia-версией..."
	@time $(JULIA) $(JULIA_PROJECT) $(JULIA_SOLVER) $(TEST_PROBLEM) $(TEST_TOL) $(TEST_MAX_ITER) $(TEST_ALPHA) $(JULIA_SOLUTION)
	@echo ""
	@echo "4. Сравнение результатов..."
	@echo "   C-решение:      $(C_SOLUTION)"
	@echo "   Julia-решение:  $(JULIA_SOLUTION)"
	@echo ""
	@echo "=== Тесты завершены ==="

# Быстрый тест (только C)
test-c: c
	@echo "Быстрый тест C-версии..."
	@$(C_GEN_BIN) $(TEST_M) $(TEST_N) $(TEST_SEED) $(TEST_PROBLEM)
	@$(C_SOLVER_BIN) $(TEST_PROBLEM) $(TEST_TOL) $(TEST_MAX_ITER) $(TEST_ALPHA) $(C_SOLUTION)
	@echo "Тест завершён!"

# Быстрый тест (только Julia)
test-julia: julia
	@echo "Быстрый тест Julia-версии..."
	@$(JULIA) $(JULIA_PROJECT) $(JULIA_SOLVER) $(TEST_PROBLEM) $(TEST_TOL) $(TEST_MAX_ITER) $(TEST_ALPHA) $(JULIA_SOLUTION)
	@echo "Тест завершён!"

# Сравнение результатов
compare:
	@if [ -f "$(C_SOLUTION)" ] && [ -f "$(JULIA_SOLUTION)" ]; then \
		echo "Сравнение решений..."; \
		python3 scripts/compare_results.py $(C_SOLUTION) $(JULIA_SOLUTION); \
	else \
		echo "Ошибка: файлы решений не найдены. Запустите 'make test'."; \
		exit 1; \
	fi

# ==============================================================================
# Очистка
# ==============================================================================
clean:
	@echo "Очистка скомпилированных файлов..."
	rm -rf $(C_BUILD_DIR)
	rm -rf $(C_BIN_DIR)
	@echo "C-версия очищена"

clean-julia:
	@echo "Очистка Julia-кэша..."
	rm -rf julia/Manifest.toml
	rm -rf julia/LocalPreferences.toml
	@echo "Julia-кэш очищен"

clean-all: clean clean-julia
	rm -rf $(DATA_DIR)
	@echo "Все данные очищены"

# Перекомпиляция
rebuild: clean all

# ==============================================================================
# Помощь
# ==============================================================================
help:
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║  Makefile для проекта Karmarkar-LP                           ║"
	@echo "╠══════════════════════════════════════════════════════════════╣"
	@echo "║  Сборка:                                                     ║"
	@echo "║    make all      - собрать C-версию (по умолчанию)           ║"
	@echo "║    make c        - собрать только C-версию                   ║"
	@echo "║    make julia    - подготовить Julia-окружение               ║"
	@echo "║    make debug    - собрать с отладочной информацией          ║"
	@echo "╠══════════════════════════════════════════════════════════════╣"
	@echo "║  Тестирование:                                               ║"
	@echo "║    make test     - запустить полные тесты (C + Julia)        ║"
	@echo "║    make test-c   - быстрый тест C-версии                     ║"
	@echo "║    make test-julia - быстрый тест Julia-версии               ║"
	@echo "║    make compare  - сравнить результаты C и Julia             ║"
	@echo "╠══════════════════════════════════════════════════════════════╣"
	@echo "║  Очистка:                                                    ║"
	@echo "║    make clean    - удалить C-объекты и бинарники             ║"
	@echo "║    make clean-julia - очистить Julia-кэш                     ║"
	@echo "║    make clean-all - удалить всё (включая данные)             ║"
	@echo "║    make rebuild  - полная перекомпиляция                     ║"
	@echo "╠══════════════════════════════════════════════════════════════╣"
	@echo "║  Запуск:                                                     ║"
	@echo "║    Генератор:    ./c/bin/generator <m> <n> <seed> <file>     ║"
	@echo "║    C-решатель:   ./c/bin/solver <file> <tol> <iter> <a> <out>║"
	@echo "║    Julia:        make julia-run ARGS=\"<args>\"              ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"

# ==============================================================================
# Зависимости заголовков (автоматическая генерация)
# ==============================================================================
-include $(C_BUILD_DIR)/*.d

$(C_BUILD_DIR)/%.d: $(C_SRC_DIR)/%.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(C_BUILD_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$