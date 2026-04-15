#ifndef DMS_MONITOR_H
#define DMS_MONITOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <deque>
#include <vector>

struct DriverState {
    bool face_detected = false;
    bool eyes_open = false;
    bool looking_forward = true;
    float eye_openness = 0.0f; // 0.0 - 1.0
    float head_turn_deg = 0.0f;
    bool alert_drowsy = false;
    bool alert_distracted = false;
    cv::Rect face_rect;
};

class DMSMonitor {
public:
    DMSMonitor(const std::string& face_proto, const std::string& face_model, const std::string& eye_cascade);
    DriverState analyze(cv::Mat& frame);

private:
    cv::dnn::Net face_net;
    cv::CascadeClassifier eye_cascade;
    std::deque<bool> eye_history;
    const int HISTORY_SIZE = 15;
    const int DROWSY_THRESHOLD = 10;

    cv::Rect detectFace(const cv::Mat& frame);
    bool estimateEyeOpenness(const cv::Mat& frame, const cv::Rect& face_rect);
    float estimateHeadTurn(const cv::Rect& face_rect, int frame_width);
};

#endif