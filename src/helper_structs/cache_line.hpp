#ifndef CACHE_LINE_HPP
#define CACHE_LINE_HPP

struct CacheLine {
    unsigned tag = 0;
    bool valid = false;
    std::map<uint32_t, uint8_t> data;
};

#endif