# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -I./include
LDFLAGS = -lblas -llapacke -llapack -lm

# Флаги для отладочной сборки
DEBUG_FLAGS = -g -DDEBUG

# Директории
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Исходные файлы
LIB_SRCS = $(SRC_DIR)/karmarkar.c $(SRC_DIR)/lp_utils.c
GEN_SRC = $(SRC_DIR)/generator.c
SOLVER_SRC = $(SRC_DIR)/solver.c

# Объектные файлы
LIB_OBJS = $(BUILD_DIR)/karmarkar.o $(BUILD_DIR)/lp_utils.o
GEN_OBJ = $(BUILD_DIR)/generator.o
SOLVER_OBJ = $(BUILD_DIR)/solver.o

# Целевые файлы
GEN_BIN = $(BIN_DIR)/generator
SOLVER_BIN = $(BIN_DIR)/solver

# Все цели
all: dirs $(GEN_BIN) $(SOLVER_BIN)

# Создаём директории
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Сборка генератора
$(GEN_BIN): $(LIB_OBJS) $(GEN_OBJ)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(GEN_OBJ) $(LDFLAGS)

# Сборка решателя
$(SOLVER_BIN): $(LIB_OBJS) $(SOLVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(SOLVER_OBJ) $(LDFLAGS)

# Компиляция объектных файлов
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Отладочная сборка
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all

# Очистка
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)

# Перекомпиляция
rebuild: clean all

# Запуск тестов (пример)
test: all
	@echo "Генерация тестовой задачи..."
	$(GEN_BIN) 5 20 42 test_problem.txt
	@echo "Решение задачи..."
	$(SOLVER_BIN) test_problem.txt 1e-6 500 0.25 test_solution.txt
	@echo "Тест завершён!"

# Помощь
help:
	@echo "Доступные цели:"
	@echo "  all      - собрать все программы (по умолчанию)"
	@echo "  debug    - собрать с отладочной информацией"
	@echo "  clean    - удалить скомпилированные файлы"
	@echo "  rebuild  - полная перекомпиляция"
	@echo "  test     - запустить быстрый тест"
	@echo "  help     - показать эту справку"

.PHONY: all dirs clean rebuild debug test help