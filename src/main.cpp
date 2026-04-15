#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include "obd_parser.h"
#include "onnx_classifier.h"
#include "dashboard.h"
#include "dms_monitor.h"
#include "dms_hud.h"

int main() {
    try {
        OBDParser parser;
        Dashboard dash(640, 480);
        if (parser.load("data/telemetry.csv") == -1) return -1;

        ONNXClassifier classifier("models/driver_classifier.onnx", "models/normalization_params.json");

        cv::VideoCapture cap(0); 
        DMSMonitor dms("models/deploy.prototxt", 
                       "models/res10_300x300_ssd_iter_140000.caffemodel", 
                       "models/haarcascade_eye.xml");
        DMSHUD dms_hud;

        std::string win_name = "ADAS Integrated System";
        cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE);

        for (int i = 0; i < parser.getCount(); ++i) {
            cv::Mat cam_frame_raw, cam_final;
            DriverState dms_state;

            // --- 1. ОБРАБОТКА КАМЕРЫ ---
            if (cap.isOpened()) {
                cap >> cam_frame_raw;
                if (!cam_frame_raw.empty()) {
                    cv::rotate(cam_frame_raw, cam_frame_raw, cv::ROTATE_90_COUNTERCLOCKWISE);
                    
                    int target_h = 480; 
                    int target_w = static_cast<int>(target_h * (static_cast<float>(cam_frame_raw.cols) / cam_frame_raw.rows));
                    
                    cv::resize(cam_frame_raw, cam_final, cv::Size(target_w, target_h));
                    cv::flip(cam_final, cam_final, 1);

                    dms_state = dms.analyze(cam_final);
                    // Рисуем рамку лица, но БЕЗ текста сверху (текст вынесем в main)
                    dms_hud.draw(cam_final, dms_state); 
                }
            }

            // --- 2. ГЕОМЕТРИЯ ОКНА ---
            // Судя по твоему скрину, приборы реально заканчиваются на 320-350 пикселях. 
            // Попробуем поставить 350 для плотной стыковки.
            int dash_visible_width = 350; 
            int total_width = dash_visible_width + (cam_final.empty() ? 0 : cam_final.cols);
            
            cv::Mat main_frame = cv::Mat::zeros(480, total_width, CV_8UC3);

            // --- 3. ОТРИСОВКА ПРИБОРОВ ---
            cv::Mat dash_zone = main_frame(cv::Rect(0, 0, dash_visible_width, 480));
            OBDRecord rec = parser.getRecord(i);
            std::vector<float> features = {(float)rec.time, (float)rec.speed, (float)rec.rpm, 
                                           (float)rec.throttle, (float)rec.accel_lon, (float)rec.accel_lat};
            ClassificationResult res = classifier.classify(features);
            dash.draw(dash_zone, rec, res.label, res.confidence);

            // --- 4. ВСТАВКА ВЕБКИ ВПРИТЫК ---
            if (!cam_final.empty()) {
                cv::Mat dms_zone = main_frame(cv::Rect(dash_visible_width, 0, cam_final.cols, 480));
                cam_final.copyTo(dms_zone);

                // --- 5. ТЕКСТ ПО ЦЕНТРУ СВЕРХУ НА ТЕМНОМ ФОНЕ ---
                // Рисуем статус глаз прямо на main_frame, чтобы он был над видео
                std::string eye_label = dms_state.eyes_open ? "EYES: OPEN" : "EYES: CLOSED";
                cv::Scalar eye_color = dms_state.eyes_open ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
                
                // Вычисляем центр зоны вебки для текста
                int text_x = dash_visible_width + (cam_final.cols / 2) - 80; 
                cv::putText(main_frame, eye_label, cv::Point(text_x, 30), 
                            cv::FONT_HERSHEY_SIMPLEX, 0.8, eye_color, 2);
            }

            cv::imshow(win_name, main_frame);
            
            int key = cv::waitKey(30);
            if (key == 27) break;
            if (key == 32) cv::waitKey(0);
        }
        cap.release();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}