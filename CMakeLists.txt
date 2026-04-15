cmake_minimum_required(VERSION 3.20)
project(RealCarADASMonitor)

# Настройки стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. OpenCV ---
set(OpenCV_DIR "C:/opencv/build") 
find_package(OpenCV REQUIRED)

# --- 2. ONNX Runtime ---
set(ONNXRUNTIME_ROOT "C:/onnxruntime") 

# --- 3. Исходные коды проекта ---
# Добавляем новые файлы DMS сюда
set(MAIN_SOURCES 
    "src/main.cpp"
    "src/obd_parser.cpp"
    "src/onnx_classifier.cpp"
    "src/dashboard.cpp"
    "src/dms_monitor.cpp"
    "src/dms_hud.cpp"
)
set(MAIN_HEADERS
    "src/obd_parser.h"
    "src/onnx_classifier.h"
    "src/dashboard.h"
    "src/dms_monitor.h"
    "src/dms_hud.h"
)

# --- 4. Основное приложение ---
add_executable(RealCarMonitor ${MAIN_SOURCES} ${MAIN_HEADERS})

target_include_directories(RealCarMonitor PRIVATE 
    "src"
    "${ONNXRUNTIME_ROOT}/include"
    ${OpenCV_INCLUDE_DIRS}
)

target_link_directories(RealCarMonitor PRIVATE "${ONNXRUNTIME_ROOT}/lib")

target_link_libraries(RealCarMonitor 
    PRIVATE 
    ${OpenCV_LIBS} 
    onnxruntime
)

# --- 5. ТЕСТОВЫЙ ЗАПУСК DMS (Задание 6.5) ---
# Отдельный exe для проверки камеры без логов телеметрии
add_executable(DMSTest "src/main_dms.cpp" "src/dms_monitor.cpp" "src/dms_hud.cpp")
target_link_libraries(DMSTest PRIVATE ${OpenCV_LIBS})
target_include_directories(DMSTest PRIVATE "src")

# --- 6. Копирование DLL после сборки ---
add_custom_command(TARGET RealCarMonitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll"
    $<TARGET_FILE_DIR:RealCarMonitor>
)

# --- 7. Google Test ---
include(FetchContent)
set(CMAKE_TLS_VERIFY OFF)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Тестовое приложение
add_executable(RunTests tests/test_obd_parser.cpp src/obd_parser.cpp)
target_link_libraries(RunTests GTest::gtest_main)
target_include_directories(RunTests PRIVATE "src")