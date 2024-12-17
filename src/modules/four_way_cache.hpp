#ifndef FOURWAY_CACHE_HPP
#define FOURWAY_CACHE_HPP

#include <systemc>
#include "systemc.h"
#include <queue>
#include <map>
#include "../helper_structs/request.h"
#include "../helper_structs/result.h"
#include "../helper_structs/cache_line.hpp"

using namespace sc_core;

SC_MODULE(FOURWAY_CACHE) {

    // Used for letting cpu know that the current request is processed.
    sc_inout<bool> ready;

    // Signal to cpu
    sc_out<size_t> missCount, hitCount;

    // Signals coming from cpu
    sc_in<sc_uint<32>> addr;
    sc_inout<sc_uint<32>> data;
    sc_in<int> we;

    /* Abstracting memory as a map. Each address (uint32_t)
     * is mapped to a byte (uint8_t). Default values are 0.*/
    std::map<uint32_t, uint8_t> mainMem;

    /* Abstracting cache also as a map. Each set
     * is mapped to a deque which used for representing FIFO.
     * Default values are empty FIFOs.*/
    std::map<uint32_t, std::deque<CacheLine>> cacheMem;

    unsigned cacheLineSize = 0, cacheLatency = 0, memoryLatency = 0, setIndexBitsCount = 0, offsetBitsCount = 0, setIndexBitMask = 0, offsetBitMask = 0;

    // Variables for counting hits and misses
    size_t hits, misses;

    // Variables used for easier bit selection
    sc_uint<32> d;
    sc_uint<32> a;

    // Variable used for determining cache hits or misses
    bool foundInCache = false;

    SC_CTOR(FOURWAY_CACHE);
    FOURWAY_CACHE(sc_module_name name, unsigned cacheLineSize, unsigned cacheLatency, unsigned memoryLatency,
                  unsigned offsetBitsCount, unsigned offsetBitMask, unsigned setIndexBitsCount, unsigned setIndexBitMask):
    sc_module(name), cacheLineSize(cacheLineSize), cacheLatency(cacheLatency), memoryLatency(memoryLatency),
    offsetBitsCount(offsetBitsCount), offsetBitMask(offsetBitMask), setIndexBitsCount(setIndexBitsCount), setIndexBitMask(setIndexBitMask) {

        hits = 0;
        misses = 0;

        SC_THREAD(read);
        SC_THREAD(write);

    };

    // Fetching cache block from main memory
    void addToCache(uint32_t address) {
        // Finding which set should it mapped
        uint32_t setIndex = (address & setIndexBitMask) >> offsetBitsCount;
        // If set is full (4 cache lines are present) remove the front cache line.
        if(cacheMem[setIndex].size() >= 4) {
            cacheMem[setIndex].pop_front();
        }

        // Adding the new cache line to the back of FIFO. Now the new cache line overwrites the old one according to FIFO principals.
        cacheMem[setIndex].push_back(CacheLine {.tag = address >> (offsetBitsCount + setIndexBitsCount), .valid = true});

        // Getting the cache block from main memory
        for(uint32_t add = address; add < (address + cacheLineSize); add++) {
            cacheMem[setIndex].back().data[add & offsetBitMask] = mainMem[add];
        }

        // Simulate the memory latency
        wait(memoryLatency, SC_NS);
    }

    // Writing a byte to the cache. If not found fetch from main memory.
    void writeByte(uint32_t address, uint8_t val) {
        // Extracting the functional bits
        uint32_t tag = address >> (offsetBitsCount + setIndexBitsCount);
        uint32_t setIndex = (address & setIndexBitMask) >> offsetBitsCount;
        uint32_t offset = address & offsetBitMask;

        // Iterating every cache line in the set and look for the tag
        for(CacheLine r : cacheMem[setIndex]) {
            /*Because an operation can be unaligned, that's why
             * it is used bitwise and operation for found boolean.
             * If at least 1 byte can't be found the whole operation
             * will be counted as miss.*/

            //If found then write the new data in according to cache cell.
            if(r.tag == tag) {
                r.data[offset] = val;
                foundInCache &= true;
                return;
            }
        }

        // Not found in cache. Now fetch and update found boolean
        addToCache((address >> offsetBitsCount) << offsetBitsCount);
        foundInCache &= false;
    }


    // Will be called for every request and works if write enable 1 is.
    void write() {
        while (true) {
            // Cpu updates the ready signal to false when it sends a request. So it waits for the fall edge.
            wait(ready->negedge_event());
            // Check if request is a write operation
            if (we -> read()) {

                // Using temp variables for bit selection
                d = data -> read();
                a = addr -> read();

                // Must be true before operations because always used with bitwise and
                foundInCache = true;

                /* Updating the values first because if cache miss then it fetches from main memory.
                 * If cache hits then the cpu can continue its process and writing to memory happens parallel
                 * so just cache latency. But for miss cache latency + memory latency*/
                mainMem[a] = d(31, 24);
                mainMem[a+1] = d(23, 16);
                mainMem[a+2] = d(15, 8);
                mainMem[a+3] = d(7, 0);

                // Writing to cache
                writeByte(a, d(31, 24));
                writeByte(a+1,d(23, 16));
                writeByte(a+2,d(15, 8));
                writeByte(a+3,d(7, 0));

                // Simulating cache latency
                wait(cacheLatency, SC_NS);

                // Writing to the signals
                hitCount ->write(foundInCache ? ++hits:hits);
                missCount ->write(foundInCache ? misses: ++misses);
                // Tell cpu that it's ready for the next request
                ready ->write(true);
            }
        }
    }

    // Reading byte from cache. If not found fetch from main memory.
    uint8_t readByte(uint32_t address) {
        // Extracting the functional bits
        uint32_t tag = address >> (offsetBitsCount + setIndexBitsCount);
        uint32_t setIndex = (address & setIndexBitMask) >> offsetBitsCount;
        uint32_t offset = address & offsetBitMask;

        // Iterating every cache line in the set and look for the tag
        for(CacheLine r : cacheMem[setIndex]) {
            /*Because an operation can be unaligned, that's why
             * it is used bitwise and operation for found boolean.
             * If at least 1 byte can't be found the whole operation
             * will be counted as miss.*/

            //If found then return the found data.
            if(r.tag == tag) {
                foundInCache &= true;
                return r.data[offset];
            }
        }

        // Not found in cache. Now fetch from main memory, update the found boolean and return the wanted data.
        addToCache((address >> offsetBitsCount) << offsetBitsCount);
        foundInCache &= false;
        return mainMem[address];

    }

    // Will be called for every request and works if write enable 0 is.
    void read() {
        while(true) {
            // Cpu updates the ready signal to false when it sends a request. So it waits for the fall edge.
            wait(ready-> negedge_event());
            // Check if request is a read operation
            if (!we -> read()) {

                // Using temp variables for bit selection
                a = addr -> read();

                // Must be true before operations because always used with bitwise and
                foundInCache = true;

                // Reading bytes from cache
                d(7, 0) = readByte(a+3);
                d(15, 8) = readByte(a+2);
                d(23, 16) = readByte(a+1);
                d(31, 24) = readByte(a);

                // Send to cpu so it can updates the data section of request
                data ->write(d);

                // Simulate the cache latency
                wait(cacheLatency, SC_NS);

                // Writing to signals
                hitCount ->write(foundInCache ? ++hits: hits);
                missCount ->write(foundInCache ? misses: ++misses);
                // Tell cpu that it's ready for the next request
                ready ->write(true);
            }

        }
    }
};

#endif
