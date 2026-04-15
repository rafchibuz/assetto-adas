#include "dms_monitor.h"
#include <iostream>

DMSMonitor::DMSMonitor(const std::string& face_proto, const std::string& face_model, const std::string& eye_cascade_path) {
    // Проверка путей перед загрузкой, чтобы OpenCV не выкидывал исключение (Шаг 7.6)
    if (!face_proto.empty() && !face_model.empty()) {
        try {
            face_net = cv::dnn::readNetFromCaffe(face_proto, face_model);
        } catch (const cv::Exception& e) {
            std::cerr << "DMS Error: Cannot load DNN: " << e.what() << std::endl;
        }
    }

    if (!eye_cascade_path.empty()) {
        if (!eye_cascade.load(eye_cascade_path)) {
            std::cerr << "DMS Error: Cannot load Haar Cascade: " << eye_cascade_path << std::endl;
        }
    }
}

cv::Rect DMSMonitor::detectFace(const cv::Mat& frame) {
    // Защита: если сеть не загружена или кадр пустой
    if (face_net.empty() || frame.empty()) return cv::Rect();

    try {
        cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0, cv::Size(300, 300), cv::Scalar(104, 177, 123));
        face_net.setInput(blob);
        cv::Mat detection = face_net.forward();
        cv::Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

        for (int i = 0; i < detectionMat.rows; i++) {
            float confidence = detectionMat.at<float>(i, 2);
            if (confidence > 0.5) {
                int x1 = static_cast<int>(detectionMat.at<float>(i, 3) * frame.cols);
                int y1 = static_cast<int>(detectionMat.at<float>(i, 4) * frame.rows);
                int x2 = static_cast<int>(detectionMat.at<float>(i, 5) * frame.cols);
                int y2 = static_cast<int>(detectionMat.at<float>(i, 6) * frame.rows);
                return cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)) & cv::Rect(0, 0, frame.cols, frame.rows);
            }
        }
    } catch (...) {
        return cv::Rect();
    }
    return cv::Rect();
}

bool DMSMonitor::estimateEyeOpenness(const cv::Mat& frame, const cv::Rect& face_rect) {
    if (face_rect.empty() || eye_cascade.empty() || frame.empty()) return false;

    cv::Rect eyes_roi_rect(face_rect.x, face_rect.y + face_rect.height/5, 
                           face_rect.width, face_rect.height/2.5);
    
    eyes_roi_rect &= cv::Rect(0, 0, frame.cols, frame.rows);
    if (eyes_roi_rect.width <= 0 || eyes_roi_rect.height <= 0) return false;

    cv::Mat eyes_roi = frame(eyes_roi_rect);
    
    cv::Mat gray_roi;
    cv::cvtColor(eyes_roi, gray_roi, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray_roi, gray_roi);

    std::vector<cv::Rect> eyes;
    eye_cascade.detectMultiScale(gray_roi, eyes, 1.1, 2, 0, cv::Size(20, 20));
    
    return !eyes.empty(); 
}

float DMSMonitor::estimateHeadTurn(const cv::Rect& face_rect, int frame_width) {
    if (frame_width <= 0) return 0.0f;
    float face_center = face_rect.x + face_rect.width / 2.0f;
    float frame_center = frame_width / 2.0f;
    return (face_center - frame_center) / (frame_width / 2.0f) * 45.0f;
}

DriverState DMSMonitor::analyze(cv::Mat& frame) {
    DriverState state;
    
    // Если кадр пустой, просто возвращаем пустой стейт (Шаг 7.6)
    if (frame.empty()) {
        state.face_detected = false;
        return state;
    }

    state.face_rect = detectFace(frame);
    
    if (!state.face_rect.empty()) {
        state.face_detected = true;
        state.eyes_open = estimateEyeOpenness(frame, state.face_rect);
        state.head_turn_deg = estimateHeadTurn(state.face_rect, frame.cols);
        state.looking_forward = std::abs(state.head_turn_deg) < 15.0f;
        state.alert_distracted = !state.looking_forward;
    }

    // История обновляется только если есть кадр
    eye_history.push_back(state.eyes_open);
    if (eye_history.size() > HISTORY_SIZE) eye_history.pop_front();

    int closed_count = 0;
    for (bool open : eye_history) if (!open) closed_count++;

    if (closed_count >= DROWSY_THRESHOLD && state.face_detected) {
        state.alert_drowsy = true;
    }

    return state;
}