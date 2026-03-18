# solver.jl
# Основная программа для решения задач ЛП алгоритмом Кармаркара

# Добавляем каталог src в путь поиска
push!(LOAD_PATH, @__DIR__)

# Импортируем модуль KarmarkarLP
using KarmarkarLP

using Printf
using LinearAlgebra

"""
    print_usage(program_name)

Выводит инструкцию по использованию программы.
"""
function print_usage(program_name::String)
    println("Использование: julia ", program_name, " <input_file> <tol> <max_iter> <alpha> <output_file>")
    println("  input_file  - имя файла с задачей ЛП")
    println("  tol         - допустимая погрешность целевой функции (например, 1e-6)")
    println("  max_iter    - максимальное количество итераций (например, 1000)")
    println("  alpha       - параметр шага алгоритма, 0 < alpha < 1 (например, 0.25)")
    println("  output_file - имя файла для сохранения решения")
    println()
    println("Пример: julia ", program_name, " problem.txt 1e-6 1000 0.25 solution.txt")
end

"""
    main(args)

Точка входа программы.
"""
function main(args::Vector{String})
    # Проверяем количество аргументов
    if length(args) != 5
        @stderr "Ошибка: неверное количество аргументов\n\n"
        print_usage("solver.jl")
        return 1
    end
    
    input_file = args[1]
    output_file = args[5]
    
    # Парсим аргументы
    local tol::Float64
    local max_iter::Int
    local alpha::Float64
    
    try
        tol = parse(Float64, args[2])
    catch
        @stderr "Ошибка: некорректное значение tol '$(args[2])'\n"
        return 1
    end
    
    try
        max_iter = parse(Int, args[3])
    catch
        @stderr "Ошибка: некорректное значение max_iter '$(args[3])'\n"
        return 1
    end
    
    try
        alpha = parse(Float64, args[4])
    catch
        @stderr "Ошибка: некорректное значение alpha '$(args[4])'\n"
        return 1
    end
    
    # Проверяем диапазоны параметров
    if tol <= 0
        @stderr "Ошибка: tol должно быть положительным числом\n"
        return 1
    end
    
    if max_iter <= 0
        @stderr "Ошибка: max_iter должно быть положительным числом\n"
        return 1
    end
    
    if alpha <= 0 || alpha >= 1
        @stderr "Ошибка: alpha должно быть в диапазоне (0, 1)\n"
        return 1
    end
    
    @printf("Загрузка задачи ЛП из файла '%s'...\n", input_file)
    
    # Загружаем задачу из файла
    local prob::KarmarkarProblem
    try
        prob = load_problem(input_file)
    catch e
        @stderr "Ошибка: не удалось загрузить задачу из файла '$(input_file)': $(e)\n"
        return 1
    end
    
    @printf("Задача загружена успешно:\n")
    @printf("  m = %d (ограничений)\n", prob.m)
    @printf("  n = %d (переменных)\n", prob.n)
    
    # Проверяем задачу
    if !validate_problem(prob)
        @stderr "Ошибка: задача не удовлетворяет условиям алгоритма Кармаркара\n"
        return 1
    end
    
    @printf("Задача прошла валидацию\n\n")
    
    @printf("Запуск алгоритма Кармаркара...\n")
    @printf("  tol = %.2e\n", tol)
    @printf("  max_iter = %d\n", max_iter)
    @printf("  alpha = %.2f\n\n", alpha)
    
    # Засекаем время начала
    start_time = time()
    
    # Решаем задачу
    result = karmarkar_solve(prob, tol, max_iter, alpha)
    
    # Засекаем время окончания
    elapsed_time = time() - start_time
    
    # Выводим результаты
    @printf("\n=== Результаты ===\n")
    
    status_str = if result.status == KARMARKAR_SUCCESS
        "УСПЕХ (достигнута требуемая точность)"
    elseif result.status == KARMARKAR_MAX_ITER_REACHED
        "ПРЕВЫШЕНО МАКС. ЧИСЛО ИТЕРАЦИЙ"
    else
        "ЧИСЛЕННАЯ ОШИБКА"
    end
    
    @printf("Статус: %s\n", status_str)
    @printf("Итераций выполнено: %d\n", result.iterations)
    @printf("Значение целевой функции: %.15e\n", result.objective)
    @printf("Время выполнения: %.6f сек\n", elapsed_time)
    
    # Выводим статистику по решению
    if result.status == KARMARKAR_SUCCESS || result.status == KARMARKAR_MAX_ITER_REACHED
        min_x = minimum(result.x)
        max_x = maximum(result.x)
        sum_x = sum(result.x)
        
        @printf("\nСтатистика решения:\n")
        @printf("  Минимальный x[i]: %.15e\n", min_x)
        @printf("  Максимальный x[i]: %.15e\n", max_x)
        @printf("  Сумма x[i]: %.15e (должно быть ~1.0)\n", sum_x)
    end
    
    # Сохраняем решение в файл
    @printf("\nСохранение решения в файл '%s'...\n", output_file)
    
    try
        save_solution(output_file, result, prob.n, elapsed_time)
        @printf("Решение успешно сохранено\n")
    catch e
        @stderr "Предупреждение: не удалось сохранить решение в файл '$(output_file)': $(e)\n"
    end
    
    # Освобождаем память (в Julia это делает GC, но явно обнуляем большие объекты)
    prob = nothing
    result = nothing
    
    @printf("\nГотово!\n")
    
    # Возвращаем код завершения в зависимости от статуса
    return result.status == KARMARKAR_SUCCESS ? 0 : 1
end

# Запуск программы
if abspath(PROGRAM_FILE) == @__FILE__
    exit(main(ARGS))
end