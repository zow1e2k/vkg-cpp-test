#ifndef QUOTE_H
#define QUOTE_H

#include <ctime>

struct Quote {
    std::time_t* unix_timestamp;
    long double price;
    long double volume;
};

#endif // QUOTE_H
