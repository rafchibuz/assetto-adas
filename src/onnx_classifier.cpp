#include "onnx_classifier.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <numeric>
#include <cmath>
#include <algorithm>

ONNXClassifier::ONNXClassifier(const std::string& model_path, const std::string& config_path) {
    loadNormalizationParams(config_path);

    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

#ifdef _WIN32
    std::wstring w_model_path(model_path.begin(), model_path.end());
    session = std::make_unique<Ort::Session>(env, w_model_path.c_str(), session_options);
#else
    session = std::make_unique<Ort::Session>(env, model_path.c_str(), session_options);
#endif
}

// Ручной парсинг JSON (Шаг 4.4)
void ONNXClassifier::loadNormalizationParams(const std::string& config_path) {
    std::ifstream f(config_path);
    if (!f.is_open()) throw std::runtime_error("Cannot open config JSON");
    
    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string content = buffer.str();

    auto parseArray = [&](const std::string& key) {
        std::vector<float> vec;
        size_t pos = content.find(key);
        pos = content.find("[", pos);
        size_t end = content.find("]", pos);
        std::string arrayStr = content.substr(pos + 1, end - pos - 1);
        std::stringstream ss(arrayStr);
        std::string item;
        while (std::getline(ss, item, ',')) vec.push_back(std::stof(item));
        return vec;
    };

    means = parseArray("\"mean\"");
    stds = parseArray("\"std\"");
}

std::vector<float> ONNXClassifier::softmax(const std::vector<float>& logits) {
    std::vector<float> exp_logits(logits.size());
    float max_logit = *std::max_element(logits.begin(), logits.end());
    float sum = 0.0f;
    for (size_t i = 0; i < logits.size(); ++i) {
        exp_logits[i] = std::exp(logits[i] - max_logit);
        sum += exp_logits[i];
    }
    for (float& val : exp_logits) val /= sum;
    return exp_logits;
}

ClassificationResult ONNXClassifier::classify(const std::vector<float>& features) {
    // 1. Нормализация (Z-score)
    std::vector<float> input_tensor_values(features.size());
    for (size_t i = 0; i < features.size(); ++i) {
        input_tensor_values[i] = (features[i] - means[i]) / stds[i];
    }

    // 2. Создание тензора
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    int64_t input_shape[] = {1, 6};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, input_tensor_values.data(), input_tensor_values.size(), input_shape, 2);

    // 3. Запуск сессии
    auto output_tensors = session->Run(Ort::RunOptions{nullptr}, 
                                       input_names.data(), &input_tensor, 1, 
                                       output_names.data(), 1);

    float* output_data = output_tensors.front().GetTensorMutableData<float>();
    std::vector<float> logits(output_data, output_data + 3);
    
    // 4. Softmax и результат
    std::vector<float> probs = softmax(logits);
    int max_idx = std::distance(probs.begin(), std::max_element(probs.begin(), probs.end()));

    return {max_idx, probs[max_idx], probs};
}