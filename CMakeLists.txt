cmake_minimum_required(VERSION 3.20)
project(RealCarADASMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Поиск OpenCV 
# Убедись, что переменная окружения OpenCV_DIR указывает на C:/opencv/build
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 2. Настройка ONNX Runtime
set(ONNXRUNTIME_ROOT "C:/onnxruntime")
include_directories(${ONNXRUNTIME_ROOT}/include)

# Для MSVC линкуем .lib файлы
link_directories(${ONNXRUNTIME_ROOT}/lib)

# 3. Исходники
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

# 4. Исполняемый файл
add_executable(RealCarMonitor ${SOURCES} ${HEADERS})
# Добавляем onnxruntime.lib (имя может отличаться в зависимости от версии, проверь папку lib)
target_link_libraries(RealCarMonitor ${OpenCV_LIBS} onnxruntime)

# 5. Google Test (через FetchContent - отлично работает в VS)
include(FetchContent)
set(CMAKE_TLS_VERIFY OFF)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Тесты
add_executable(RunTests tests/test_obd_parser.cpp src/obd_parser.cpp)
target_link_libraries(RunTests GTest::gtest_main)