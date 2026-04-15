#ifndef DMS_HUD_H
#define DMS_HUD_H

#include <opencv2/opencv.hpp>
#include "dms_monitor.h"

class DMSHUD {
public:
    void draw(cv::Mat& frame, const DriverState& state);
};

#endif