# karmarkar.jl
# Реализация алгоритма Кармаркара для решения задач линейного программирования

using LinearAlgebra

# Перечисление возможных статусов завершения работы алгоритма
@enum KarmarkarStatus begin
    KARMARKAR_SUCCESS
    KARMARKAR_MAX_ITER_REACHED
    KARMARKAR_NUMERIC_ERROR
end

# Структура для хранения задачи ЛП в канонической форме
struct KarmarkarProblem
    m::Int                    # Количество ограничений
    n::Int                    # Количество переменных
    A::Matrix{Float64}        # Матрица ограничений (m × n)
    c::Vector{Float64}        # Вектор целевой функции (n)
end

# Структура для хранения результата решения
struct KarmarkarResult
    x::Vector{Float64}        # Вектор решения
    iterations::Int           # Количество итераций
    status::KarmarkarStatus   # Статус завершения
    objective::Float64        # Значение целевой функции
end

"""
    compute_projection(A, D, c)

Вычисляет проекцию градиента на ядро ограничений.
Возвращает вектор проекции размера n.
"""
function compute_projection(A::Matrix{Float64}, D::Vector{Float64}, c::Vector{Float64})::Vector{Float64}
    m, n = size(A)
    
    # Строим матрицу AD = A * D (масштабируем каждый столбец A на соответствующий элемент D)
    AD = A * Diagonal(D)
    
    # Вычисляем Dc = D .* c (поэлементное умножение)
    Dc = D .* c
    
    # Вычисляем ADADT = AD * AD'
    ADADT = AD * AD'
    
    # Выполняем LU-разложение и решаем систему ADADT * λ = AD * Dc
    # В Julia это делается через факторизацию
    F = lu(ADADT)
    temp = F \ (AD * Dc)
    
    # Вычисляем проекцию: c_proj = Dc - AD' * λ
    c_proj = Dc - AD' * temp
    
    return c_proj
end

"""
    karmarkar_solve(prob, tol, max_iter, alpha)

Решает задачу ЛП алгоритмом Кармаркара.

Аргументы:
- prob: структура KarmarkarProblem с задачей
- tol: допустимая погрешность целевой функции
- max_iter: максимальное количество итераций
- alpha: параметр шага алгоритма (0 < alpha < 1)

Возвращает:
- KarmarkarResult с решением и статистикой
"""
function karmarkar_solve(prob::KarmarkarProblem, tol::Float64, max_iter::Int, alpha::Float64)::KarmarkarResult
    m, n = prob.m, prob.n
    
    # Инициализируем начальную точку в центре симплекса
    x = fill(1.0 / n, n)
    
    iterations = 0
    status = KARMARKAR_MAX_ITER_REACHED
    objective = 0.0
    
    # Основной цикл алгоритма
    for k in 1:max_iter
        iterations = k
        
        # Вычисляем текущее значение целевой функции
        objective = dot(prob.c, x)
        
        # Проверяем критерий остановки
        if abs(objective) < tol
            status = KARMARKAR_SUCCESS
            break
        end
        
        # Строим диагональную матрицу масштабирования D (как вектор)
        D = copy(x)
        
        # Вычисляем проекцию градиента
        c_proj = compute_projection(prob.A, D, prob.c)
        
        # Вычисляем норму проекции
        norm_cp = norm(c_proj)
        
        # Проверяем на численные ошибки
        if norm_cp < 1e-10
            status = KARMARKAR_NUMERIC_ERROR
            break
        end
        
        # Центр симплекса
        a = fill(1.0 / n, n)
        
        # y_new = a - alpha * c_proj / ||c_proj||
        y_new = a - alpha * c_proj / norm_cp
        
        # Проверяем, что точка внутри допустимой области
        if any(y_new .<= 1e-10)
            status = KARMARKAR_NUMERIC_ERROR
            break
        end
        
        # Обратное преобразование: x_tilde = D .* y_new (поэлементное умножение)
        x_tilde = D .* y_new
        
        # Нормализация: сумма компонент должна равняться 1
        sum_x = sum(x_tilde)
        
        if sum_x < 1e-10
            status = KARMARKAR_NUMERIC_ERROR
            break
        end
        
        # Обновляем решение
        x = x_tilde / sum_x
    end
    
    # Вычисляем финальное значение целевой функции
    objective = dot(prob.c, x)
    
    return KarmarkarResult(x, iterations, status, objective)
end

"""
    validate_problem(prob)

Проверяет, удовлетворяет ли задача условиям алгоритма Кармаркара.
Возвращает true если задача валидна, false если нет.
"""
function validate_problem(prob::KarmarkarProblem)::Bool
    # Проверяем размеры
    if prob.m <= 0 || prob.n <= 0
        return false
    end
    
    if prob.n <= prob.m
        return false
    end
    
    # Проверяем, что сумма каждого столбца матрицы A равна 0
    # Это гарантирует, что точка (1/n, ..., 1/n) удовлетворяет Ax = 0
    col_sums = sum(prob.A, dims=1)
    if any(abs.(col_sums) .> 1e-10)
        return false
    end
    
    # Проверяем, что c^T * (1/n, ..., 1/n) = 0
    c_mean = mean(prob.c)
    if abs(c_mean) > 1e-10
        return false
    end
    
    return true
end

# Экспорт функций для использования в других модулях
export KarmarkarProblem, KarmarkarResult, KarmarkarStatus
export karmarkar_solve, validate_problem
export KARMARKAR_SUCCESS, KARMARKAR_MAX_ITER_REACHED, KARMARKAR_NUMERIC_ERROR