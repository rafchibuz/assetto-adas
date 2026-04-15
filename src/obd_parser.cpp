#include "obd_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>

// Вспомогательная функция для маппинга строк (для тестов)
int OBDParser::stringToLabel(const std::string& label) {
    if (label == "SLOW") return 0;
    if (label == "NORMAL") return 1;
    if (label == "AGGRESSIVE") return 2;
    return -1;
}

OBDRecord OBDParser::getRecord(int index) const {
    return records.at(index);
}

int OBDParser::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[-] Error: File " << filename << " not found!" << std::endl;
        return -1;
    }

    records.clear();
    std::string line;
    
    // Пропускаем первые две строки: заголовки и единицы измерения (s, km/h, G...)
    if (!std::getline(file, line)) return -1;
    if (!std::getline(file, line)) return -1;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string val;
        std::vector<std::string> row;

        // Читаем CSV строку, учитывая кавычки
        while (std::getline(ss, val, ',')) {
            val.erase(std::remove(val.begin(), val.end(), '\"'), val.end());
            row.push_back(val);
        }

        // Проверяем, что в строке достаточно данных (минимум до Throttle Pos на 110 индексе)
        if (row.size() > 110) {
            try {
                OBDRecord rec;
                // Парсим по индексам из твоего списка
                rec.time        = std::stod(row[0]);   // Time
                rec.accel_lat   = std::stod(row[23]);  // CG Accel Lateral
                rec.accel_lon   = std::stod(row[24]);  // CG Accel Longitudinal
                rec.rpm         = std::stod(row[60]);  // Engine RPM
                rec.speed       = std::stod(row[64]);  // Ground Speed
                rec.throttle    = std::stod(row[110]); // Throttle Pos

                // Вычисляем суммарный вектор перегрузки (G-Total)
                double g_total = std::sqrt(rec.accel_lon * rec.accel_lon + rec.accel_lat * rec.accel_lat);

                // --- ЛОГИКА РАЗМЕТКИ (Adaptive Logic) ---
                
                // 1. Приоритет SLOW: если ускорения мизерные (штиль), это всегда SLOW
                // Порог 0.15G — это очень спокойная езда
                if ((g_total < 0.35 && rec.throttle < 40.0) || rec.speed < 25) {
                    rec.driver_style = 0; 
                }
                // 2. Агрессия: большие G или газ в пол
                else if ((g_total > 0.75 && rec.throttle > 90.0) || rec.speed > 130.0) {
                    rec.driver_style = 2;
                }
                // 3. Остальное — обычный рабочий режим
                else {
                    rec.driver_style = 1;
                }

                records.push_back(rec);
            } catch (const std::exception& e) {
                // Пропускаем битые строки или строки с текстом
                continue;
            }
        }
    }
    return static_cast<int>(records.size());
}

bool OBDParser::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[-] Error: Could not create " << filename << std::endl;
        return false;
    }

    // Записываем шапку для нашего нового датасета
    file << "Time,Speed,RPM,Throttle,Accel_Lon,Accel_Lat,label\n";

    for (const auto& rec : records) {
        file << rec.time << "," 
             << rec.speed << "," 
             << rec.rpm << "," 
             << rec.throttle << "," 
             << rec.accel_lon << "," 
             << rec.accel_lat << "," 
             << rec.driver_style << "\n";
    }

    file.close();
    return true;
}