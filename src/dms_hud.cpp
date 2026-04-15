#include "dms_hud.h"

void DMSHUD::draw(cv::Mat& frame, const DriverState& state) {
    cv::Scalar color = state.face_detected ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
    if (state.alert_drowsy) color = cv::Scalar(0, 165, 255); // Оранжевый

    // 1. Угловая рамка лица
    if (state.face_detected) {
        cv::Rect r = state.face_rect;
        int l = 20; // длина угла
        // Левый верх
        cv::line(frame, {r.x, r.y}, {r.x + l, r.y}, color, 2);
        cv::line(frame, {r.x, r.y}, {r.x, r.y + l}, color, 2);
        // Правый верх
        cv::line(frame, {r.x + r.width, r.y}, {r.x + r.width - l, r.y}, color, 2);
        cv::line(frame, {r.x + r.width, r.y}, {r.x + r.width, r.y + l}, color, 2);
        // ... и так далее для низа
    }

    // 2. Индикаторы состояния
    cv::putText(frame, "EYES: " + std::string(state.eyes_open ? "OPEN" : "CLOSED"), {20, 60}, 0, 0.6, color, 2);

    // 3. Алерты
    if (state.alert_drowsy) {
        cv::rectangle(frame, {frame.cols/4, frame.rows/2 - 30, frame.cols/2, 60}, cv::Scalar(0, 0, 0), -1);
        cv::putText(frame, "DROWSINESS ALERT!", {frame.cols/4 + 10, frame.rows/2 + 10}, 0, 0.7, cv::Scalar(0, 165, 255), 2);
    }

    if (state.alert_distracted) {
        cv::rectangle(frame, {0, frame.rows - 30, frame.cols, 30}, cv::Scalar(0, 0, 255), -1);
        cv::putText(frame, "DISTRACTION ALERT", {frame.cols/3, frame.rows - 10}, 0, 0.5, cv::Scalar(255, 255, 255), 1);
    }
}