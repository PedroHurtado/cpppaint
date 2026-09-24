# Tests sin framework externo: solo <cassert>. Suficiente para lo didáctico y
# se integra con CTest (ejecútalos con `ctest` tras compilar).
add_executable(paint_tests test_paint.cpp)
target_link_libraries(paint_tests PRIVATE paint_core)

# IMPRESCINDIBLE: en Release, CMake define NDEBUG y <cassert> convierte todos
# los assert en no-ops. Sin esto los tests compilan, enlazan, devuelven 0...
# y no comprueban absolutamente nada. Lo quitamos solo para este target.
if(MSVC)
    target_compile_options(paint_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(paint_tests PRIVATE -UNDEBUG)
endif()

add_test(NAME paint_tests COMMAND paint_tests)

# --- Dominio con GoogleTest (Point, contrato IShape, Circle, Square) --------
find_package(GTest REQUIRED)

add_executable(domain_tests test_domain.cpp)
target_link_libraries(domain_tests PRIVATE paint_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(domain_tests)
