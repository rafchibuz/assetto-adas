#pragma once
#include <vector>
#include <string>

struct OBDRecord {
    double time;
    double speed;
    double rpm;
    double throttle;
    double accel_lon;
    double accel_lat;
    double fuel;
    int driver_style;
};

class OBDParser {
public:
    int load(const std::string& filename);
    bool save(const std::string& filename);
    
    // Только объявление, без реализации
    int getCount() const;
    OBDRecord getRecord(int index) const;

    // Добавляем объявление функции, на которую ругался компилятор
    static int stringToLabel(const std::string& label);

private:
    std::vector<OBDRecord> records;
};