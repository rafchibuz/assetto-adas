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
    int driver_style; // 0: SLOW, 1: NORMAL, 2: AGGRESSIVE
};

class OBDParser {
public:
    // Загрузка и разметка
    int load(const std::string& filename);
    // Сохранение размеченного датасета
    bool save(const std::string& filename); 
    
    OBDRecord getRecord(int index) const;
    int getCount() const { return static_cast<int>(records.size()); }
    static int stringToLabel(const std::string& label); 

private:
    std::vector<OBDRecord> records;
};