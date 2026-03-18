# io.jl
# Функции для загрузки и сохранения задач ЛП

using Printf

"""
    load_problem(filename)

Загружает задачу ЛП из текстового файла (формат, совместимый с C-генератором).
Возвращает структуру KarmarkarProblem.
"""
function load_problem(filename::String)::KarmarkarProblem
    open(filename, "r") do io
        # Читаем размеры задачи
        line = readline(io)
        parts = split(line)
        m = parse(Int, parts[1])
        n = parse(Int, parts[2])
        
        # Выделяем матрицу A (m × n)
        A = Matrix{Float64}(undef, m, n)
        
        # Читаем матрицу A (column-major формат, как в C/BLAS)
        for j in 1:n
            line = readline(io)
            values = parse.(Float64, split(line))
            for i in 1:m
                A[i, j] = values[i]
            end
        end
        
        # Читаем вектор c
        line = readline(io)
        c = parse.(Float64, split(line))
        
        return KarmarkarProblem(m, n, A, c)
    end
end

"""
    save_solution(filename, result, n, elapsed_time)

Сохраняет решение в текстовый файл (формат, совместимый с C-решателем).
"""
function save_solution(filename::String, result::KarmarkarResult, n::Int, elapsed_time::Float64)
    open(filename, "w") do io
        # Статус
        status_str = if result.status == KARMARKAR_SUCCESS
            "SUCCESS"
        elseif result.status == KARMARKAR_MAX_ITER_REACHED
            "MAX_ITER_REACHED"
        else
            "NUMERIC_ERROR"
        end
        
        # Заголовок
        println(io, "# Решение задачи ЛП алгоритмом Кармаркара")
        @printf(io, "status=%s\n", status_str)
        @printf(io, "iterations=%d\n", result.iterations)
        @printf(io, "time_seconds=%.6f\n", elapsed_time)
        @printf(io, "objective=%.15e\n", result.objective)
        @printf(io, "n_variables=%d\n\n", n)
        
        # Вектор решения
        println(io, "# Вектор решения x (по одному значению в строке):")
        for i in 1:n
            @printf(io, "x[%d]=%.15e\n", i-1, result.x[i])
        end
    end
end

export load_problem, save_solution