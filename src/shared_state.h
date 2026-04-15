#pragma once
#include <mutex>
#include <atomic>
#include "obd_parser.h"
#include "onnx_classifier.h"

/**
 * @brief Структура для обмена данными между потоками OBD и Main.
 */
struct SharedState {
    std::mutex mtx;                 // Защита данных
    std::atomic<bool> running{true}; // Флаг работы системы
    
    // Данные для передачи
    OBDRecord current_record;
    int current_style = 1;          // 0: SLOW, 1: NORMAL, 2: AGGRESSIVE
    float confidence = 0.0f;

    // Статистика
    int total_obd_processed = 0;
    int alert_drowsy_count = 0;
    int alert_distracted_count = 0;
    int alert_aggressive_count = 0;
};