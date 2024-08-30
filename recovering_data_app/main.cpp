#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <regex>
#include <limits>

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
        } else {
            quote.unix_timestamp = new time_t(std::stoll(str)); // time64 equals int64 => long long
        }

        std::getline(buffer, str, _delimiter);
        if (!isStreamDigits(str)) {
            quote.price = !quotes.empty() ? quotes.back().price : 0.0; // raw data could have not price
        } else {
            quote.price = std::stold(str);
        }

        std::getline(buffer, str, _delimiter);
        if (!isStreamDigits(str)) {
            continue;
            quote.volume = !quotes.empty() ? quotes.back().volume : 0.0; // raw data could have not volume
        } else {
            quote.volume = std::stold(str);
        }

        quotes.push_back(quote);
    }

    file.close();
    return quotes;
}

void writeCandles(const std::string& _filename, const std::vector<Quote> _quotes, unsigned int _length) {
    std::time_t timestamp = *_quotes.begin()->unix_timestamp;

    long double
        maxPeriodPrice = 0.0,
        minPeriodPrice = std::numeric_limits<long double>::max(),
        open = 0.0,
        volume = 0.0,
        close = 0.0;

    bool isNewPeriod = true;

    std::fstream file(_filename, std::ios::out);
    file.clear();
    file << "Time,Open,High,Low,Close,Volume" << std::endl;

    // it would be work ONLY if rawdata's timestamp column goes ASC
    for (const Quote &q : _quotes) { // foreach iterations from begin to the end
        if (timestamp + (_length * 60) > *q.unix_timestamp) {
            maxPeriodPrice = maxPeriodPrice < q.price ? q.price : maxPeriodPrice;
            minPeriodPrice = minPeriodPrice > q.price ? q.price : minPeriodPrice;
            volume += q.volume;

            if (isNewPeriod) {
                isNewPeriod = false;
                open = q.price;
            }

            continue;
        }

        close = q.price;
        // actual data : maxPrice | open | minPrice | close | volume
        file << timestamp << "," << open << "," << maxPeriodPrice << "," << minPeriodPrice << "," << close << "," << volume << std::endl;

        isNewPeriod = true;
        timestamp = *q.unix_timestamp;

        volume = open = close = maxPeriodPrice = 0.0;
        minPeriodPrice = std::numeric_limits<long double>::max();
    }

    file.close();
    return;
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

    for (const Quote &q : quotes) {
        std::cout << *q.unix_timestamp << ", " << q.price << ", " << q.volume << std::endl;
    }

    writeCandles(candlesFilename, quotes, 60);

    std::cout << "END" << std::endl;
    getchar();
    return 0;
}
