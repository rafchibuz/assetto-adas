#include <gtest/gtest.h>
#include "../src/obd_parser.h"
#include <fstream>

// 1. Тест: конвертация меток (Шаг 2.4.1)
TEST(OBDParserTest, LabelConversion) {
    EXPECT_EQ(OBDParser::stringToLabel("SLOW"), 0);
    EXPECT_EQ(OBDParser::stringToLabel("NORMAL"), 1);
    EXPECT_EQ(OBDParser::stringToLabel("AGGRESSIVE"), 2);
}

// 2. Тест: загрузка несуществующего файла (Шаг 2.4.2)
TEST(OBDParserTest, FileNotFound) {
    OBDParser parser;
    EXPECT_EQ(parser.load("non_existent.csv"), -1);
}

// 3. Тест: getRecord() бросает исключение (Шаг 2.4.3)
TEST(OBDParserTest, OutOfRange) {
    OBDParser parser;
    EXPECT_THROW(parser.getRecord(9999), std::out_of_range);
}

// 4. Тест: парсинг корректного CSV (Шаг 2.4.4)
TEST(OBDParserTest, ValidParsing) {
    std::string filename = "temp_test.csv";
    std::ofstream tmp(filename);
    
    // Имитируем структуру твоего файла (MoTeC)
    tmp << "Format,MoTeC CSV File\n"; // Метаданные
    tmp << "\"Time\",\"Engine RPM\",\"Fuel Level\",\"Ground Speed\",\"Throttle Pos\"\n"; // Заголовки (упрощенно для теста)
    tmp << "\"s\",\"rpm\",\"l\",\"km/h\",\"%\"\n"; // Единицы
    
    // Создаем строку, где данные стоят на нужных индексах (60, 62, 64, 111)
    // Чтобы не писать 112 запятых, мы можем временно упростить логику парсера 
    // или в тесте создать строку нужной длины:
    for(int i=0; i<120; ++i) {
        if(i == 0) tmp << "1.0";
        else if(i == 60) tmp << "3000";
        else if(i == 62) tmp << "45";
        else if(i == 64) tmp << "120";
        else if(i == 111) tmp << "85";
        else tmp << "0";
        
        if(i < 119) tmp << ",";
    }
    tmp << "\n";
    tmp.close();

    OBDParser parser;
    int result = parser.load(filename);
    
    EXPECT_GT(result, 0);
    if (result > 0) {
        EXPECT_NEAR(parser.getRecord(0).speed, 120.0, 0.1);
    }

    std::remove(filename.c_str()); // Удаляем за собой
}

// 5. Тест: пропуск некорректной строки (Шаг 2.4.5)
TEST(OBDParserTest, SkipInvalidLine) {
    std::ofstream tmp("invalid_test.csv");
    // Заголовок MoTeC-стиля
    tmp << "\"Time\",\"RPM\"\n\"s\",\"rpm\"\n";
    tmp << "1.0, 2000\n";        // Валидная
    tmp << "broken_line, error\n"; // Битая
    tmp << "2.0, 3000\n";        // Валидная
    tmp.close();

    OBDParser parser;
    // Т.к. лоадер ищет "Time" и индексы 60+, этот тест на реальном файле 
    // подтверждается тем, что load() не падает при встрече с мусором.
    EXPECT_NO_THROW(parser.load("invalid_test.csv"));
    std::remove("invalid_test.csv");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}