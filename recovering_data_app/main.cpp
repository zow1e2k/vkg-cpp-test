#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <regex>
#include <limits>
#include <cstdarg>
#include <cstdlib>
#include <list>
#include "Quote.h"

const unsigned int
    MAX_CANDLE_LENGTH = 60,
    MIN_CANDLE_LENGTH = 5,
    MIN_SMA_PERIOD = 100,
    MAX_SMA_PERIOD = 500,
    MAX_ARGS = 2;

enum MessageType {
    MESSAGE_TYPE_LOG = 0,
    MESSAGE_TYPE_INFO,
    MESSAGE_TYPE_ERROR
};

bool isStreamDigits(const std::string& _str);
void sendClientMessage(MessageType type, const char* text, ...);
std::vector<Quote> readQuotes(const std::string& _filename, const char _delimiter);
void writeCandles(const std::string& _filename, const std::vector<Quote> _quotes, unsigned int _length);
std::list<unsigned int>* p_close_prices = new std::list<unsigned int>();
void writeSMA(const std::string& _filename, unsigned int _period);

int main(int argc, char* argv[]) {
    sendClientMessage(MessageType::MESSAGE_TYPE_LOG, "START");

    if (argc < MAX_ARGS + 1) {
        sendClientMessage(MessageType::MESSAGE_TYPE_ERROR, "You must input unsigned integer params: candle_length, sma_period");
        return 1;
    }

    unsigned int
        candle_length,
        sma_period;

    if (sscanf_s(argv[1], "%u", &candle_length) != 1 || sscanf_s(argv[2], "%u", &sma_period) != 1) {
        sendClientMessage(MessageType::MESSAGE_TYPE_ERROR, "You must input unsigned integer params: candle_length, sma_period 2");
        return 1;
    }

    if (candle_length < MIN_CANDLE_LENGTH || candle_length > MAX_CANDLE_LENGTH) {
        sendClientMessage(MessageType::MESSAGE_TYPE_ERROR, "candle_length must be from %u to %u", MIN_CANDLE_LENGTH, MAX_CANDLE_LENGTH);
        return 1;
    }

    if (sma_period < MIN_SMA_PERIOD || sma_period > MAX_SMA_PERIOD) {
        sendClientMessage(MessageType::MESSAGE_TYPE_ERROR, "sma_period must be from %u to %u", MIN_SMA_PERIOD, MAX_SMA_PERIOD);
        return 1;
    }

    std::string inputFilename = "ETHUSDT_1.csv";
    std::string candlesFilename = "candles_ETHUSDT_1.csv";
    std::string smaFilename = "SMA_ETHUSDT_1.csv";
    std::vector<Quote> quotes = readQuotes(inputFilename, ',');

    for (const Quote &q : quotes) {
        sendClientMessage(
            MessageType::MESSAGE_TYPE_INFO,
            "quote = %lld, %ld, %ld",
            *q.unix_timestamp,
            q.price,
            q.volume
        );
    }

    writeCandles(candlesFilename, quotes, candle_length);
    writeSMA(smaFilename, sma_period);

    sendClientMessage(MessageType::MESSAGE_TYPE_LOG, "END");
    getchar();
    return 0;
}

std::vector<Quote> readQuotes(const std::string& _filename, const char _delimiter) {
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        file.open(_filename);
    } catch (const std::ifstream::failure& e) {
        //std::cerr << "File not found" << std::endl;
        sendClientMessage(MessageType::MESSAGE_TYPE_ERROR, "file: %s is not found", _filename.c_str());
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

void writeSMA(const std::string& _filename, unsigned int _period) {
    long double sum = 0, currSMA;
    unsigned int
        counter = 0,
        numPeriods = 0;

    std::fstream file(_filename, std::ios::out);
    file.clear();
    file << "Period,SMA" << std::endl;

    while (!p_close_prices->empty()) {
        if (counter == _period) {
            currSMA = std::pow(1, _period) * sum;
            file << numPeriods << "," << currSMA << std::endl;
            counter = 0;
            numPeriods++;
        }

        sum += p_close_prices->front();
        p_close_prices->pop_front();
        counter++;
    }

    file.close();
    return;
}

void writeCandles(const std::string& _filename, const std::vector<Quote> _quotes, unsigned int _length) {
    std::time_t timestamp = *_quotes.begin()->unix_timestamp;

    long double
        maxPeriodPrice = 0.0,
        minPeriodPrice = std::numeric_limits<long double>::max(),
        open = 0.0,
        volume = 0.0;

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

        // actual data : maxPrice | open | minPrice | close (current q.price) | volume
        file << timestamp << "," << open << "," << maxPeriodPrice << "," << minPeriodPrice << "," << q.price << "," << volume << std::endl;
        p_close_prices->push_front(q.price);

        isNewPeriod = true;
        timestamp = *q.unix_timestamp;

        volume = open = maxPeriodPrice = 0.0;
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

void sendClientMessage(MessageType type, const char* text, ...) {
    va_list args;

    switch (type) {
        case MessageType::MESSAGE_TYPE_ERROR: {
            va_start(args, text);
            vprintf(text, args);
            va_end(args);
            getchar();
            EXIT_FAILURE;
            return;
        }

        case MessageType::MESSAGE_TYPE_LOG: {
        #ifndef DEBUG
            return;
        #elif
            break;
        #endif
        }

        default: break;
    }

    va_start(args, text);
    vprintf(text, args);
    va_end(args);
    std::cout << std::endl;
    return;
}
