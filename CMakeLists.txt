cmake_minimum_required(VERSION 3.20)
project(RealCarADASMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. OpenCV
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib") # Твой путь
find_package(OpenCV REQUIRED)

# 2. ONNX Runtime
set(ONNXRUNTIME_ROOT "C:/onnxruntime")
include_directories(${ONNXRUNTIME_ROOT}/include)
link_directories(${ONNXRUNTIME_ROOT}/lib)

# 3. ЯВНО указываем исходники для основного приложения
set(MAIN_SOURCES 
    "src/main.cpp"
    "src/obd_parser.cpp"
)
set(MAIN_HEADERS
    "src/obd_parser.h"
)

# 4. Создание экзешника
add_executable(RealCarMonitor ${MAIN_SOURCES} ${MAIN_HEADERS})
target_link_libraries(RealCarMonitor ${OpenCV_LIBS} onnxruntime)
target_include_directories(RealCarMonitor PRIVATE "src")

# 5. Google Test (оставляем как было, раз тесты работают)
include(FetchContent)
set(CMAKE_TLS_VERIFY OFF)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(RunTests tests/test_obd_parser.cpp src/obd_parser.cpp)
target_link_libraries(RunTests GTest::gtest_main)

