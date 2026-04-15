#pragma once
#include <vector>
#include <string>
#include <onnxruntime_cxx_api.h>

struct ClassificationResult {
    int label;
    float confidence;
    std::vector<float> scores;
};

class ONNXClassifier {
public:
    ONNXClassifier(const std::string& model_path, const std::string& config_path);
    ClassificationResult classify(const std::vector<float>& features);

private:
    void loadNormalizationParams(const std::string& config_path);
    std::vector<float> softmax(const std::vector<float>& logits);

    // ONNX объекты
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "DriverClassifier"};
    std::unique_ptr<Ort::Session> session;
    
    // Параметры нормализации
    std::vector<float> means;
    std::vector<float> stds;
    
    // Имена входов/выходов
    std::vector<const char*> input_names = {"features"};
    std::vector<const char*> output_names = {"class_scores"};
};