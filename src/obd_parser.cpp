#include "obd_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>

// Вернули для совместимости с тестами
int OBDParser::stringToLabel(const std::string& label) {
    if (label == "SLOW") return 0;
    if (label == "NORMAL") return 1;
    if (label == "AGGRESSIVE") return 2;
    return -1;
}

int OBDParser::getCount() const { return static_cast<int>(records.size()); }
OBDRecord OBDParser::getRecord(int index) const { return records.at(index); }

int OBDParser::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return -1;

    records.clear();
    std::string line;
    std::getline(file, line); // Header
    std::getline(file, line); // Units

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string val;
        std::vector<std::string> row;

        while (std::getline(ss, val, ',')) {
            val.erase(std::remove(val.begin(), val.end(), '\"'), val.end());
            row.push_back(val);
        }

        if (row.size() > 110) {
            try {
                OBDRecord rec;
                rec.time      = std::stod(row[0]);
                rec.accel_lat = std::stod(row[23]);
                rec.accel_lon = std::stod(row[24]);
                rec.rpm       = std::stod(row[60]);
                rec.fuel      = std::stod(row[62]); 
                rec.speed     = std::stod(row[64]);
                rec.throttle  = std::stod(row[110]);

                double g_total = std::sqrt(rec.accel_lon * rec.accel_lon + rec.accel_lat * rec.accel_lat);

                if ((g_total > 0.75 && rec.throttle > 90.0) || rec.speed > 130.0) {
                    rec.driver_style = 2; // AGGRESSIVE
                } else if ((g_total < 0.35 && rec.throttle < 40.0) || rec.speed < 25.0) {
                    rec.driver_style = 0; // SLOW
                } else {
                    rec.driver_style = 1; // NORMAL
                }

                records.push_back(rec);
            } catch (...) { continue; }
        }
    }
    return (int)records.size();
}

bool OBDParser::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    file << "Time,Speed,RPM,Throttle,Accel_Lon,Accel_Lat,Fuel,label\n";
    for (const auto& rec : records) {
        file << rec.time << "," << rec.speed << "," << rec.rpm << "," 
             << rec.throttle << "," << rec.accel_lon << "," << rec.accel_lat << "," 
             << rec.fuel << "," << rec.driver_style << "\n";
    }
    file.close();
    return true;
}