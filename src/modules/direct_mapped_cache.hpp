#ifndef DIRECT_MAPPED_CACHE_HPP
#define DIRECT_MAPPED_CACHE_HPP

#include <systemc>
#include "systemc.h"
#include <map>

// helper structs
#include "../helper_structs/request.h"
#include "../helper_structs/result.h"
#include "../helper_structs/cache_line.hpp"

using namespace sc_core;
   
SC_MODULE(DIRECT_MAPPED_CACHE) {

    // I/O signals
    // ----------------------------------------------------------------------------------------------------
    sc_inout<bool> cache_ready; // checked by cpu to see if cache is able to recieve the next request
    
    // split up request from the CPU
    sc_in<sc_uint<32>> addrFromCPU;
    sc_inout<sc_uint<32>> dataFromCPU;
    sc_in<int> weFromCPU;

    // result related
    sc_out<size_t> missesResult, hitsResult;
    // ----------------------------------------------------------------------------------------------------

    // cache related
    unsigned
    cacheLineSize = 0,
 
    // latency related
    cacheLatency = 0,
    memoryLatency = 0,

    // address related
    offsetBitsCount = 0,
    offsetBitsMask = 0,
    indexBitsCount = 0,
    indexBitsMask = 0;

    // memory related
    //////////////////////////////////////////////////////////////////////////////////////////////////

    std::map<uint32_t, CacheLine> cache; // cache with cacheLines and cacheLineSize defined during runtime

    std::map<uint32_t, uint8_t> mainMemory; // main memory that doesn't need to be initialized fully but only the needed values

    //////////////////////////////////////////////////////////////////////////////////////////////////

    // local helper variables for easier counting of the result signals
    size_t misses = 0;
    size_t hits = 0;


    SC_CTOR(DIRECT_MAPPED_CACHE);
    DIRECT_MAPPED_CACHE(sc_module_name name, unsigned cacheLineSize, unsigned cacheLatency, unsigned memoryLatency,
    unsigned offsetBitsCount, unsigned offsetBitsMask, unsigned indexBitsCount, unsigned indexBitsMask) :
    
    sc_module(name), cacheLineSize(cacheLineSize), cacheLatency(cacheLatency), memoryLatency(memoryLatency),
    offsetBitsCount(offsetBitsCount), offsetBitsMask(offsetBitsMask), indexBitsCount(indexBitsCount), indexBitsMask(indexBitsMask)   {
        
        // initialize the result related variables
        misses = 0;
        hits = 0;

        SC_THREAD(processRequest);

    }

    void processRequest() {
        
        while(true) {
            wait(cache_ready -> negedge_event());

            // splitting the request into it's attributes
            sc_uint<32> addr = addrFromCPU -> read();
            sc_uint<32> data = dataFromCPU->read();
            int we = weFromCPU->read();

            if(we) { // write 
               write(addr, data);
                
            } else { // read
                read(addr, data);
            }

            cache_ready->write(true); // lets the cpu know that it can send the next request
        }
    }

    void write(sc_uint<32> addr, sc_uint<32> data) {
        bool isHit = true;

        // write 1 byte 4 times because size of data is uint32_t, so 4 bytes
        for(int i = 0; i < 4; ++i) {
            unsigned
            offset = (addr + i) & offsetBitsMask,
            index = ((addr + i) & indexBitsMask) >> offsetBitsCount,
            tag = (addr + i) >> indexBitsCount >> offsetBitsCount;

            CacheLine& currentLine = cache[index];

            if(!currentLine.valid || currentLine.tag != tag) { // cache miss causes overhead 

                wait(memoryLatency, SC_NS);

                // writes the whole line so some bytes are written from main memory and then instantly rewritten again by the data input -> could be improved
                // write the whole line from main memory / identical to read
                unsigned cacheLineAddr = addr ^ offsetBitsMask;
                for(int j = 0; j < cacheLineSize; ++j) {
                    currentLine.data[offset] = mainMemory[cacheLineAddr + j];
                }
                
                isHit = false;
                currentLine.valid = true; // could only be set if false -> could be improved (?)
                currentLine.tag = tag; // "-"

            }

            sc_uint<8> currentBlock = data.range(32 - (8 * i) - 1, 32 - (8 * (i + 1))); // splitting the data into each of it's bytes
            currentLine.data[offset] = currentBlock;
            mainMemory[addr + i] = currentBlock; // main memory access but could happen parallel due to hit
        }

        wait(cacheLatency, SC_NS);

        if(isHit) {
            hitsResult->write(++hits);
        } else {
            missesResult->write(++misses);
        }

    }

    void read(sc_uint<32> addr, sc_uint<32> data) {
        sc_uint<32> tempData;
        bool isHit = true;

        // read 1 byte 4 times because size of data is uint32_t, so 4 bytes
        for(int i = 0; i < 4; ++i) {
            unsigned
            offset = (addr + i) & offsetBitsMask,
            index = ((addr + i) & indexBitsMask) >> offsetBitsCount,
            tag = (addr + i) >> indexBitsCount >> offsetBitsCount;

            CacheLine& currentLine = cache[index];

            if(!currentLine.valid || currentLine.tag != tag) { // cache miss causes overhead

                wait(memoryLatency, SC_NS);

                // fetch the whole line from main memory / identical to write
                unsigned cacheLineAddr = addr ^ offsetBitsMask;
                for(int j = 0; j < cacheLineSize; ++j) {
                    currentLine.data[offset] = mainMemory[cacheLineAddr + j];
                }
            
                isHit = false;
                currentLine.valid = true; // could only be set if false -> could be improved (?)
                currentLine.tag = tag; // "-"

            }

            tempData.range(32 - (8 * i) - 1, 32 - (8 * (i + 1))) = cache[index].data[offset]; // read the cache block (either hit and no changes needed or newly fetched data)
        }

        wait(cacheLatency, SC_NS);

        if(isHit) {
            hitsResult->write(++hits);
        } else {
            missesResult->write(++misses);
        }
        dataFromCPU = tempData;
    }

};

#endif