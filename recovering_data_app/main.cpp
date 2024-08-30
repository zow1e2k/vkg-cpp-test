#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <regex>

const unsigned int
    MAX_CANDLE_LENGTH = 60,
    MIN_CANDLE_LENGTH = 5,
    MIN_SMA_PERIOD = 100,
    MAX_SMA_PERIOD = 500;

bool isStreamDigits(const std::string& _str);

struct Quote {
    std::time_t* unix_timestamp;
    long double price;
    long double volume;
};

std::vector<Quote> readQuotes(const std::string& _filename, const char _delimiter) {
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        file.open(_filename);
    } catch (const std::ifstream::failure& e) {
        std::cerr << "File not found exception" << std::endl;
    }

    std::vector<Quote> quotes;
    std::string line;
    Quote quote;
    std::string str;

    while (!file.eof()) {
        std::getline(file, line);
        std::istringstream buffer(line);

        std::getline(buffer, str, _delimiter);
        if (!isStreamDigits(str)) {
            continue;
        }
        quote.unix_timestamp = new time_t(std::stoll(str)); // time64 equals int64 => long long

        std::getline(buffer, str, _delimiter);
        if (!isStreamDigits(str)) {
            continue;
        }
        quote.price = std::stold(str);

        std::getline(buffer, str, _delimiter);
        if (!isStreamDigits(str)) {
            continue;
        }
        quote.volume = std::stold(str);

        quotes.push_back(quote);
    }

    file.close();
    return quotes;
}

bool isStreamDigits(const std::string& str) {
    std::regex numberRegex("^\\d+(\\.\\d+)?$"); // we could get float & int from raw data
    bool result = std::regex_match(str, numberRegex);

    return result;
}

int main(int argc, char* argv[]) {
    std::cout << "START" << std::endl;
    std::string inputFilename = "ETHUSDT_1.csv";
    std::string candlesFilename = "candles_ETHUSDT_1.csv";
    std::vector<Quote> quotes = readQuotes(inputFilename, ',');

    if (quotes.empty()) {
        std::cout << "Quotes vector is empty" << std::endl;
        getchar();
        return 1;
    }

    for (const Quote &a : quotes) {
        std::cout << a.unix_timestamp << ", " << a.price << ", " << a.volume << std::endl;
    }

    std::cout << "END" << std::endl;
    getchar();
    return 0;
}
