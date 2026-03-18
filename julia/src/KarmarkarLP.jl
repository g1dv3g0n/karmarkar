# KarmarkarLP.jl
# Основной модуль пакета KarmarkarLP

module KarmarkarLP

# Используем стандартные библиотеки
using LinearAlgebra
using Printf

# Включаем компоненты модуля
include("karmarkar.jl")
include("io.jl")

# Экспортируем публичный API
export KarmarkarProblem, KarmarkarResult, KarmarkarStatus
export karmarkar_solve, validate_problem
export load_problem, save_solution
export KARMARKAR_SUCCESS, KARMARKAR_MAX_ITER_REACHED, KARMARKAR_NUMERIC_ERROR

end # module KarmarkarLP