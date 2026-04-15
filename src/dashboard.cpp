#include "dashboard.h"
#include <cmath>

Dashboard::Dashboard(int width, int height) : w(width), h(height) {}

void Dashboard::drawGauge(cv::Mat& img, cv::Point center, int radius, float value, float min_val, float max_val, const std::string& label, cv::Scalar color) {
    // Статичная дуга
    cv::ellipse(img, center, cv::Size(radius, radius), 0, 135, 405, cv::Scalar(40, 40, 40), 4, cv::LINE_AA);
    
    // Активная дуга
    float percentage = std::min(1.0f, std::max(0.0f, (value - min_val) / (max_val - min_val)));
    float end_angle = 135.0f + (270.0f * percentage);
    cv::ellipse(img, center, cv::Size(radius, radius), 0, 135, end_angle, color, 4, cv::LINE_AA);

    // Стрелка
    float rad = (end_angle * 3.14159f / 180.0f);
    cv::Point tip(center.x + (radius - 6) * cos(rad), center.y + (radius - 6) * sin(rad));
    cv::line(img, center, tip, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);

    // Числа ниже центра
    cv::putText(img, std::to_string((int)value), center + cv::Point(-12, 20), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
    // Название прибора сверху
    cv::putText(img, label, center + cv::Point(-18, -radius - 8), cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(150, 150, 150), 1);
}

void Dashboard::drawLinearGauge(cv::Mat& img, cv::Rect rect, float value, float min_val, float max_val, const std::string& label, cv::Scalar color, bool is_fuel) {
    cv::rectangle(img, rect, cv::Scalar(30, 30, 30), -1);
    float percentage = std::min(1.0f, std::max(0.0f, (value - min_val) / (max_val - min_val)));
    cv::rectangle(img, cv::Rect(rect.x, rect.y, (int)(rect.width * percentage), rect.height), color, -1);
    
    std::string val_text = std::to_string((int)value) + (is_fuel ? " L" : " %");
    cv::putText(img, label, cv::Point(rect.x, rect.y - 6), cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(160, 160, 160), 1);
    cv::putText(img, val_text, cv::Point(rect.x + rect.width + 5, rect.y + 10), cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(255, 255, 255), 1);
}

void Dashboard::drawGBowl(cv::Mat& img, cv::Point center, int radius, float lat_g, float lon_g) {
    cv::circle(img, center, radius, cv::Scalar(50, 50, 50), 1, cv::LINE_AA);
    cv::line(img, center - cv::Point(radius, 0), center + cv::Point(radius, 0), cv::Scalar(40, 40, 40), 1);
    cv::line(img, center - cv::Point(0, radius), center + cv::Point(0, radius), cv::Scalar(40, 40, 40), 1);

    float scale = radius / 1.5f;
    int gx = center.x + (int)(lat_g * scale);
    int gy = center.y - (int)(lon_g * scale); 
    cv::circle(img, cv::Point(gx, gy), 3, cv::Scalar(0, 255, 255), -1, cv::LINE_AA);
    cv::putText(img, "G-G", center + cv::Point(-10, radius + 12), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(100, 100, 100), 1);
}

void Dashboard::draw(cv::Mat& frame, const OBDRecord& data, int style, float conf) {
    // 1. Подложка (начинается с y=40, чтобы не перекрывать верхнюю инфо-строку)
    cv::Mat overlay = frame.clone();
    cv::rectangle(overlay, cv::Rect(0, 40, 310, h - 40), cv::Scalar(10, 10, 10), -1);
    cv::addWeighted(overlay, 0.85, frame, 0.15, 0, frame);

    // 2. Ряд приборов (сдвинут на y=110 вместо 70)
    int r = 38; 
    int y_row = 110;
    
    drawGauge(frame, cv::Point(50, y_row), r, (float)data.speed, 0, 180, "SPEED", cv::Scalar(0, 255, 0));
    drawGBowl(frame, cv::Point(150, y_row), r - 5, (float)data.accel_lat, (float)data.accel_lon);
    drawGauge(frame, cv::Point(250, y_row), r, (float)data.rpm, 0, 8000, "RPM", cv::Scalar(255, 140, 0));

    // 3. Линейные блоки (тоже уехали ниже)
    drawLinearGauge(frame, cv::Rect(30, 200, 180, 12), (float)data.fuel, 0, 100, "FUEL", cv::Scalar(0, 255, 255), true);
    // Поменяли GAS на THROTTLE
    drawLinearGauge(frame, cv::Rect(30, 240, 180, 12), (float)data.throttle, 0, 100, "THROTTLE", cv::Scalar(255, 255, 255), false);

    // 4. AI Блок
    std::string names[] = {"SLOW", "NORMAL", "AGGRESSIVE"};
    cv::Scalar colors[] = {cv::Scalar(255, 200, 0), cv::Scalar(0, 255, 0), cv::Scalar(0, 0, 255)};
    
    cv::putText(frame, "AI STYLE:", cv::Point(30, 300), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(150, 150, 150), 1);
    cv::putText(frame, names[style], cv::Point(30, 335), cv::FONT_HERSHEY_SIMPLEX, 0.9, colors[style], 2, cv::LINE_AA);
    
    drawLinearGauge(frame, cv::Rect(30, 365, 180, 6), conf * 100.0f, 0, 100, "AI CONFIDENCE", cv::Scalar(120, 120, 120), false);
}