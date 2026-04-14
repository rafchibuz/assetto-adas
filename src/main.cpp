#include <iostream>
#include <opencv2/opencv.hpp>

/**
 * @brief Точка входа в систему ADAS.
 * На этапе Шага 1.4 проверяет инициализацию проекта и версию OpenCV.
 */
int main() {
    // Вывод приветствия согласно заданию 
    std::cout << "========================================" << std::endl;
    std::cout << "   Real-Car ADAS Monitor Initialized    " << std::endl;
    std::cout << "========================================" << std::endl;

    // Проверка версии OpenCV для отчета
    std::cout << "C++ Standard: 17" << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;

    // Попытка создать пустую матрицу, чтобы убедиться, что линковка прошла успешно
    cv::Mat test_matrix = cv::Mat::zeros(100, 100, CV_8UC3);
    
    if (!test_matrix.empty()) {
        std::cout << "OpenCV Liking: SUCCESS" << std::endl;
    } else {
        std::cerr << "OpenCV Liking: FAILED" << std::endl;
        return -1;
    }

    std::cout << "\nPress Any Key to Exit..." << std::endl;
    return 0;
}