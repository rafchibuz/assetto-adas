#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <fstream>
#include <chrono>
#include <opencv2/opencv.hpp>

#include "obd_parser.h"
#include "onnx_classifier.h"
#include "dashboard.h"
#include "dms_monitor.h"
#include "dms_hud.h"
#include "shared_state.h"

/**
 * @brief Поток обработки данных OBD и классификации стиля вождения (10 Hz).
 */
void obdThreadFunc(SharedState& state, OBDParser& parser, ONNXClassifier& classifier) {
    int record_idx = 0;
    int total = parser.getCount();

    while (state.running && record_idx < total) {
        OBDRecord rec = parser.getRecord(record_idx);
        
        // Инференс нейросети
        std::vector<float> features = {
            (float)rec.time, (float)rec.speed, (float)rec.rpm, 
            (float)rec.throttle, (float)rec.accel_lon, (float)rec.accel_lat
        };
        ClassificationResult res = classifier.classify(features);

        // Синхронизация и обновление SharedState
        {
            std::lock_guard<std::mutex> lock(state.mtx);
            state.current_record = rec;
            state.current_style = res.label;
            state.confidence = res.confidence;
            state.total_obd_processed++;
            
            if (res.label == 2) state.alert_aggressive_count++;
        }

        record_idx++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10 Hz
    }
    state.running = false;
}

int main() {
    try {
        // 1. Инициализация ресурсов
        OBDParser parser;
        if (parser.load("data/telemetry.csv") == -1) return -1;

        ONNXClassifier classifier("models/driver_classifier.onnx", "models/normalization_params.json");
        DMSMonitor dms("models/deploy.prototxt", 
                       "models/res10_300x300_ssd_iter_140000.caffemodel", 
                       "models/haarcascade_eye.xml");
        DMSHUD dms_hud;
        Dashboard dash(640, 480);
        
        SharedState state;
        std::ofstream alert_log("output/dms_alerts.log");
        auto start_time = std::chrono::steady_clock::now();

        // 2. Запуск OBD потока
        std::thread obd_thread(obdThreadFunc, std::ref(state), std::ref(parser), std::ref(classifier));

        // 3. Настройка VideoWriter (output/result_situation2.mp4)
        // Размер будет 350 (dash) + ширина_вебки. Возьмем 750 как среднее, 
        // или инициализируем после первого кадра.
        cv::VideoWriter video;
        cv::VideoCapture cap(0);
        
        std::string win_name = "ADAS Multithreaded System";
        cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE);

        std::cout << "[+] System started. Press 'Q' to quit, 'S' for screenshot, 'SPACE' to pause." << std::endl;

        while (state.running) {
            cv::Mat cam_raw, cam_final;
            DriverState dms_s;

            // --- ОБРАБОТКА КАМЕРЫ ---
            if (cap.isOpened()) {
                cap >> cam_raw;
                if (!cam_raw.empty()) {
                    cv::rotate(cam_raw, cam_raw, cv::ROTATE_90_COUNTERCLOCKWISE);
                    int target_h = 480;
                    int target_w = static_cast<int>(target_h * (static_cast<float>(cam_raw.cols) / cam_raw.rows));
                    cv::resize(cam_raw, cam_final, cv::Size(target_w, target_h));
                    cv::flip(cam_final, cam_final, 1);

                    dms_s = dms.analyze(cam_final);
                    dms_hud.draw(cam_final, dms_s);
                }
            }

            // --- СИНХРОНИЗАЦИЯ С OBD ПОТОКОМ ---
            OBDRecord cur_rec;
            int cur_style;
            float cur_conf;
            {
                std::lock_guard<std::mutex> lock(state.mtx);
                cur_rec = state.current_record;
                cur_style = state.current_style;
                cur_conf = state.confidence;

                // Логирование алертов
                if (dms_s.alert_drowsy) {
                    alert_log << "[" << cur_rec.time << "s] ALERT: DROWSY\n";
                    state.alert_drowsy_count++;
                }
                if (dms_s.alert_distracted) {
                    alert_log << "[" << cur_rec.time << "s] ALERT: DISTRACTED\n";
                    state.alert_distracted_count++;
                }
            }

            // --- СБОРКА ФИНАЛЬНОГО КАДРА ---
            int dash_w = 350; // Твоя настроенная ширина приборов
            int total_width = dash_w + (cam_final.empty() ? 400 : cam_final.cols);
            cv::Mat main_frame = cv::Mat::zeros(480, total_width, CV_8UC3);

            // Лево: Dashboard
            cv::Mat dash_zone = main_frame(cv::Rect(0, 0, dash_w, 480));
            dash.draw(dash_zone, cur_rec, cur_style, cur_conf);

            // Право: DMS
            if (!cam_final.empty()) {
                cv::Mat dms_zone = main_frame(cv::Rect(dash_w, 0, cam_final.cols, 480));
                cam_final.copyTo(dms_zone);

                // Отрисовка статуса глаз (как мы делали в прошлый раз)
                std::string eye_label = dms_s.eyes_open ? "EYES: OPEN" : "EYES: CLOSED";
                cv::Scalar eye_col = dms_s.eyes_open ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
                cv::putText(main_frame, eye_label, cv::Point(dash_w + 20, 40), 
                            cv::FONT_HERSHEY_SIMPLEX, 0.7, eye_col, 2);
            }

            // Инициализация записи видео при первом кадре
            if (!video.isOpened() && !main_frame.empty()) {
                video.open("output/result_situation2.mp4", 
                           cv::VideoWriter::fourcc('M','P','4','V'), 20, main_frame.size());
            }
            if (video.isOpened()) video.write(main_frame);

            cv::imshow(win_name, main_frame);

            // --- УПРАВЛЕНИЕ ---
            int key = cv::waitKey(1);
            if (key == 'q' || key == 'Q') state.running = false;
            if (key == 's' || key == 'S') {
                std::string fs_name = "output/screenshot_" + std::to_string(cur_rec.time) + ".png";
                cv::imwrite(fs_name, main_frame);
                std::cout << "[*] Screenshot saved: " << fs_name << std::endl;
            }
            if (key == 32) cv::waitKey(0); // Pause
        }

        // 4. ЗАВЕРШЕНИЕ
        state.running = false;
        if (obd_thread.joinable()) obd_thread.join();
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

        // Итоговая статистика (Шаг 7.5)
        std::cout << "\n========================================" << std::endl;
        std::cout << "         ADAS FINAL STATISTICS          " << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total Runtime:       " << duration.count() << " sec" << std::endl;
        std::cout << "OBD Records:         " << state.total_obd_processed << std::endl;
        std::cout << "Drowsy Alerts:       " << state.alert_drowsy_count << std::endl;
        std::cout << "Distracted Alerts:   " << state.alert_distracted_count << std::endl;
        std::cout << "Aggressive Style:    " << state.alert_aggressive_count << " frames" << std::endl;
        std::cout << "========================================" << std::endl;

        cap.release();
        video.release();
        alert_log.close();

    } catch (const std::exception& e) {
        std::cerr << "[!] CRITICAL ERROR: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}