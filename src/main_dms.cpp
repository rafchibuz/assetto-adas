#include <opencv2/opencv.hpp>
#include "dms_monitor.h"
#include "dms_hud.h"
#include <iostream>

int main() {
    // Индекс 0 - обычно DroidCam, если нет других камер. 
    // Если будет ошибка или черный экран, поменяй на 1.
    cv::VideoCapture cap(0); 

    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open camera!" << std::endl;
        return -1;
    }

    // Укажи правильные пути к моделям (относительно места запуска)
    DMSMonitor dms("models/deploy.prototxt", 
                "models/res10_300x300_ssd_iter_140000.caffemodel", 
                "models/haarcascade_eye.xml");
    
    DMSHUD hud;

    std::cout << "DMS Test Started. Press 'ESC' to exit." << std::endl;

    while (true) {
        cv::Mat frame_raw;
        cap >> frame_raw;
        if (frame_raw.empty()) break;

        // 1. Поворот (выполняем ДО изменения размера, чтобы не искажать пропорции)
        // Если после этого ты боком - поменяй CLOCKWISE на COUNTERCLOCKWISE
        cv::rotate(frame_raw, frame_raw, cv::ROTATE_90_COUNTERCLOCKWISE);

        // 2. Получаем оригинальные размеры после поворота
        int orig_w = frame_raw.cols;
        int orig_h = frame_raw.rows;

        // 3. Вычисляем целевой размер, сохраняя пропорции (Scaling)
        // Например, ограничим высоту до 600 пикселей, ширина подстроится сама
        int target_h = 600; 
        int target_w = static_cast<int>(target_h * (static_cast<float>(orig_w) / orig_h));

        cv::Mat frame;
        cv::resize(frame_raw, frame, cv::Size(target_w, target_h));

        // 4. Зеркалим (только если нужно, обычно для фронталки так привычнее)
        cv::flip(frame, frame, 1);

        // Анализ и HUD
        DriverState state = dms.analyze(frame);
        hud.draw(frame, state);

        cv::imshow("DMS Test Stage", frame);
        if (cv::waitKey(1) == 27) break;
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}