#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include "obd_parser.h"

class Dashboard {
public:
    // Конструктор задает размер окна (по умолчанию 640x480)
    Dashboard(int width = 640, int height = 480);
    
    // Основной метод отрисовки
    void draw(cv::Mat& frame, const OBDRecord& data, int predicted_style, float confidence);

private:
    int w, h;

    // Круговой прибор со стрелкой (Спидометр / Тахометр)
    void drawGauge(cv::Mat& img, cv::Point center, int radius, float value, float min_val, float max_val, 
                    const std::string& label, cv::Scalar color);

    // Линейный прибор (Топливо / Дроссель)
    void drawLinearGauge(cv::Mat& img, cv::Rect rect, float value, float min_val, float max_val, 
                          const std::string& label, cv::Scalar color, bool is_fuel);

    // G-G Diagram (G-Bowl)
    void drawGBowl(cv::Mat& img, cv::Point center, int radius, float lat_g, float lon_g);
};