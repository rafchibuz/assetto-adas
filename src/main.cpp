#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <random> // Для честного рандома
#include <ctime>
#include "obd_parser.h"
#include "onnx_classifier.h"

namespace fs = std::filesystem;

int main() {
    try {
        OBDParser parser;
        
        // --- ЧАСТЬ 1: ПОДГОТОВКА И РАЗМЕТКА ДАННЫХ ---
        if (!fs::exists("data")) {
            fs::create_directory("data");
            std::cout << "[*] Created 'data' directory." << std::endl;
        }

        std::cout << ">>> Step 1: Processing Raw Telemetry..." << std::endl;
        // Загружаем исходник (telemetry.csv должен быть в папке data)
        if (parser.load("data/telemetry.csv") == -1) {
            std::cerr << "[-] Error: 'data/telemetry.csv' not found!" << std::endl;
            return -1;
        }

        std::string output = "data/processed_telemetry.csv";
        if (parser.save(output)) {
            std::cout << "[+] Dataset labeled and saved to: " << fs::absolute(output) << std::endl;
        }

        // --- ЧАСТЬ 2: ИНИЦИАЛИЗАЦИЯ НЕЙРОСЕТИ ---
        std::cout << "\n>>> Step 2: Loading AI Model..." << std::endl;
        // Убедись, что файлы лежат в папке models/
        ONNXClassifier classifier("models/driver_classifier.onnx", "models/normalization_params.json");
        std::cout << "[+] ONNX model loaded successfully." << std::endl;

        // --- ЧАСТЬ 3: ВАЛИДАЦИЯ (Случайные 20 записей) ---
        int total_records = parser.getCount();
        int test_count = (total_records < 20) ? total_records : 20;

        std::cout << "\n>>> Step 3: Neural Network Validation (Random " << test_count << " Samples)" << std::endl;
        std::cout << std::string(90, '-') << std::endl;
        std::cout << std::left << std::setw(8) << "Index"
                  << std::setw(10) << "Target" 
                  << std::setw(10) << "Predict" 
                  << std::setw(15) << "Confidence" 
                  << "Raw Class Scores (Slow, Normal, Aggressive)" << std::endl;
        std::cout << std::string(90, '-') << std::endl;

        // Настройка рандома
        std::mt19937 gen(static_cast<unsigned int>(std::time(nullptr)));
        std::uniform_int_distribution<> dis(0, total_records - 1);

        int correct = 0;
        for (int i = 0; i < test_count; ++i) {
            int random_idx = dis(gen);
            OBDRecord rec = parser.getRecord(random_idx);
            
            // ВАЖНО: Порядок признаков должен СТРОГО совпадать с тем, что был в Google Colab
            std::vector<float> features = {
                (float)rec.time, 
                (float)rec.speed, 
                (float)rec.rpm, 
                (float)rec.throttle, 
                (float)rec.accel_lon, 
                (float)rec.accel_lat
            };

            // Предсказание
            ClassificationResult res = classifier.classify(features);

            if (res.label == rec.driver_style) correct++;

            // Вывод строки
            std::cout << std::left << std::setw(8) << random_idx
                      << std::setw(10) << rec.driver_style 
                      << std::setw(10) << res.label 
                      << std::fixed << std::setprecision(2) << std::setw(15) << (res.confidence * 100.0f)
                      << "[" << std::setprecision(3) << res.scores[0] << ", " 
                      << res.scores[1] << ", " << res.scores[2] << "]" << std::endl;
        }

        // --- ЧАСТЬ 4: ИТОГИ ---
        std::cout << std::string(90, '-') << std::endl;
        double accuracy = (correct * 100.0) / test_count;
        std::cout << "RANDOM VALIDATION RESULT: " << correct << "/" << test_count 
                  << " correct. Accuracy: " << accuracy << "%" << std::endl;
        
        // Общая статистика распределения классов в данных
        int stats[3] = {0, 0, 0};
        for (int i = 0; i < total_records; ++i) {
            stats[parser.getRecord(i).driver_style]++;
        }
        std::cout << "\nOVERALL DATA DISTRIBUTION: Slow: " << stats[0] 
                  << " | Normal: " << stats[1] << " | Aggressive: " << stats[2] << std::endl;
        std::cout << std::string(90, '=') << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n[!] EXCEPTION: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}