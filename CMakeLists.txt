cmake_minimum_required(VERSION 3.20)
project(RealCarADASMonitor)

# Настройки стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. OpenCV ---
# Укажи путь до папки build твоего OpenCV
set(OpenCV_DIR "C:/opencv/build") 
find_package(OpenCV REQUIRED)

# --- 2. ONNX Runtime ---
# Твой реальный путь к библиотеке
set(ONNXRUNTIME_ROOT "C:/onnxruntime") 

# --- 3. Исходные коды проекта ---
set(MAIN_SOURCES 
    "src/main.cpp"
    "src/obd_parser.cpp"
    "src/onnx_classifier.cpp"
)
set(MAIN_HEADERS
    "src/obd_parser.h"
    "src/onnx_classifier.h"
)

# --- 4. Основное приложение ---
add_executable(RealCarMonitor ${MAIN_SOURCES} ${MAIN_HEADERS})

# Пути к заголовочным файлам
target_include_directories(RealCarMonitor PRIVATE 
    "src"
    "${ONNXRUNTIME_ROOT}/include"
    ${OpenCV_INCLUDE_DIRS}
)

# Пути к либам
target_link_directories(RealCarMonitor PRIVATE "${ONNXRUNTIME_ROOT}/lib")

# Линковка всех библиотек
target_link_libraries(RealCarMonitor 
    PRIVATE 
    ${OpenCV_LIBS} 
    onnxruntime
)

# --- 5. Копирование DLL после сборки ---
# Это критически важно, чтобы .exe запустился
add_custom_command(TARGET RealCarMonitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll"
    $<TARGET_FILE_DIR:RealCarMonitor>
)

# --- 6. Google Test (Автозагрузка) ---
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