cmake_minimum_required(VERSION 3.16)

project(paint
    VERSION 1.0.0
    DESCRIPTION "Paint de consola: Singleton, Prototype, Factory y Command"
    LANGUAGES CXX)

# --- Estándar de C++ -------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # -std=c++17, no -std=gnu++17

# Build type por defecto (en generadores de configuración única).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de build" FORCE)
endif()

# --- Biblioteca con toda la lógica (todo menos main) -----------------------
# Separar lógica (lib) de punto de entrada (exe) permite enlazar los tests
# contra la misma lógica sin recompilarla.
add_library(paint_core
    src/ConsoleReader.cpp
    src/ConsoleWriter.cpp
    src/Circle.cpp
    src/Square.cpp
    src/ShapeFactory.cpp
    src/ShapeRegistration.cpp
    src/Canvas.cpp
    src/AddShapeCommand.cpp
    src/MoveShapeCommand.cpp
    src/DuplicateShapeCommand.cpp
    src/CommandManager.cpp
    src/CommandRegistry.cpp
    src/CommandRegistration.cpp
    src/App.cpp)

# 'include' es PUBLIC: quien enlace con paint_core hereda esta ruta de headers.
target_include_directories(paint_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(paint_core PUBLIC cxx_std_17)

# Avisos estrictos según el compilador.
if(MSVC)
    target_compile_options(paint_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(paint_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# En MinGW, enlazar el runtime de forma estática evita que el .exe cargue una
# libstdc++-6.dll ajena del PATH (p. ej. la de Git), lo que provoca cuelgues.
# El binario queda autónomo. (En MSVC/Linux no aplica.)
if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

# --- Cobertura (opcional) ---------------------------------------------------
# cmake -S . -B build-cov -DCMAKE_BUILD_TYPE=Debug -DPAINT_COVERAGE=ON
option(PAINT_COVERAGE "Instrumentar paint_core y los tests con gcov" OFF)
if(PAINT_COVERAGE AND NOT MSVC)
    target_compile_options(paint_core PUBLIC --coverage -O0)
    target_link_options(paint_core PUBLIC --coverage)
endif()

# --- Ejecutable ------------------------------------------------------------
add_executable(paint src/main.cpp)
target_link_libraries(paint PRIVATE paint_core)

# --- Tests -----------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
