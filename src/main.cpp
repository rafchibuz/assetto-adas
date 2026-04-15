#include <iostream>
#include <iomanip>
#include <filesystem> // Требует C++17
#include "obd_parser.h"

namespace fs = std::filesystem;

int main() {
    OBDParser parser;
    
    // Проверяем наличие папки data, если нет — создаем
    if (!fs::exists("data")) {
        fs::create_directory("data");
        std::cout << "[*] Created 'data' directory." << std::endl;
    }

    std::cout << "Starting Pipeline..." << std::endl;

    // 1. Загрузка (Убедись, что исходный файл лежит в assetto-adas/data/telemetry.csv)
    if (parser.load("data/telemetry.csv") == -1) {
        std::cerr << "[-] Error: 'data/telemetry.csv' not found!" << std::endl;
        return -1;
    }

    // 2. Сохранение
    std::string output = "data/processed_telemetry.csv";
    if (parser.save(output)) {
        std::cout << "[+] SUCCESS! File saved: " << fs::absolute(output) << std::endl;
    }

    // Статистика
    int stats[3] = {0, 0, 0};
    for (int i = 0; i < parser.getCount(); ++i) {
        stats[parser.getRecord(i).driver_style]++;
    }

    std::cout << "====================================" << std::endl;
    std::cout << "0 (SLOW): " << stats[0] << " | 1 (NORMAL): " << stats[1] << " | 2 (AGGRESSIVE): " << stats[2] << std::endl;
    std::cout << "====================================" << std::endl;

    return 0;
}