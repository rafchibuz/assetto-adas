#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "dms_monitor.h"

// 1. Тест: по умолчанию модуль не загружен (пустые пути)
TEST(DMSMonitorTest, ModuleNotLoadedDefault) {
    DMSMonitor dms("", "", ""); 
    cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC3);
    
    // Не должно падать, должно вернуть false
    DriverState state = dms.analyze(frame);
    EXPECT_FALSE(state.face_detected);
}

// 2. Тест: analyze() на пустом кадре не падает
TEST(DMSMonitorTest, AnalyzeEmptyFrameNoCrash) {
    DMSMonitor dms("", "", ""); 
    cv::Mat empty_frame; // 0x0
    
    ASSERT_NO_THROW({
        DriverState state = dms.analyze(empty_frame);
        EXPECT_FALSE(state.face_detected);
    });
}

// 3. Тест: загрузка моделей (настоящая проверка)
TEST(DMSMonitorTest, LoadModelsCheck) {
    DMSMonitor dms("models/deploy.prototxt", 
                   "models/res10_300x300_ssd_iter_140000.caffemodel", 
                   "models/haarcascade_eye.xml");
    
    cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC3);
    DriverState state = dms.analyze(frame);
    EXPECT_FALSE(state.face_detected); 
}