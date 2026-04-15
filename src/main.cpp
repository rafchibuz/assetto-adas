#include <iostream>
#include <vector>
#include <map>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include "obd_parser.h"
#include "onnx_classifier.h"
#include "dashboard.h"

namespace fs = std::filesystem;

// Функция для быстрого вывода статистики разметки в консоль
void printGroundTruthStats(const OBDParser& parser) {
    std::map<int, int> stats;
    int total = parser.getCount();
    for (int i = 0; i < total; ++i) {
        stats[parser.getRecord(i).driver_style]++;
    }

    std::string names[] = {"SLOW", "NORMAL", "AGGRESSIVE"};
    std::cout << "\n--- Parser Labeling Statistics (Ground Truth) ---" << std::endl;
    for (int i = 0; i <= 2; ++i) {
        double percent = (total > 0) ? (static_cast<double>(stats[i]) / total * 100.0) : 0;
        std::cout << names[i] << ": " << stats[i] << " frames (" 
                  << std::fixed << std::setprecision(1) << percent << "%)" << std::endl;
    }
    std::cout << "------------------------------------------------\n" << std::endl;
}

int main() {
    try {
        // 1. Инициализация компонентов
        OBDParser parser;
        Dashboard dash(640, 480);
        
        std::string inputPath = "data/telemetry.csv";
        std::string outputPath = "data/processed_telemetry.csv";

        // Загружаем данные
        if (parser.load(inputPath) == -1) {
            std::cerr << "[-] Error: " << inputPath << " not found!" << std::endl;
            return -1;
        }
        std::cout << "[+] Telemetry loaded: " << parser.getCount() << " records." << std::endl;

        // --- ВОТ ТУТ МЫ ВОЗВРАЩАЕМ СОХРАНЕНИЕ ---
        printGroundTruthStats(parser);
        if (parser.save(outputPath)) {
            std::cout << "[+] Processed data for training saved to: " << outputPath << std::endl;
        } else {
            std::cerr << "[-] Warning: Could not save " << outputPath << " (Check if file is open in Excel!)" << std::endl;
        }

        // Загружаем нейронку
        // Убедись, что файлы .onnx и .json лежат в папке models/
        ONNXClassifier classifier("models/driver_classifier.onnx", "models/normalization_params.json");
        std::cout << "[+] System initialized. Starting playback..." << std::endl;

        // 2. Основной цикл визуализации
        int total_records = parser.getCount();
        std::string win_name = "ADAS Real-Time Monitor";
        cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE);

        for (int i = 0; i < total_records; ++i) {
            // Создаем черный фон (640x480)
            cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC3);

            // Получаем текущую запись
            OBDRecord rec = parser.getRecord(i);

            // 3. Инференс нейросети (предсказание модели в реальном времени)
            std::vector<float> features = {
                (float)rec.time, (float)rec.speed, (float)rec.rpm, 
                (float)rec.throttle, (float)rec.accel_lon, (float)rec.accel_lat
            };
            ClassificationResult res = classifier.classify(features);

            // 4. Отрисовка Dashboard
            // Передаем результат нейросети (res.label) для индикации на панели
            dash.draw(frame, rec, res.label, res.confidence);

            // Служебная информация на экране
            std::string info = "Rec: " + std::to_string(i) + " / " + std::to_string(total_records);
            cv::putText(frame, info, cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);

            // 5. Вывод изображения
            cv::imshow(win_name, frame);

            // Управление воспроизведением
            int key = cv::waitKey(30);
            if (key == 27) break; // ESC - выход
            if (key == 32) cv::waitKey(0); // Space - пауза
        }

        cv::destroyAllWindows();
        std::cout << "[+] Playback finished." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[!] CRITICAL ERROR: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}