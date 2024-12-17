#include <systemc>

// modules
#include "modules/cpu.hpp"
#include "modules/direct_mapped_cache.hpp"
#include "modules/four_way_cache.hpp"

// helper structs
#include "helper_structs/request.h"
#include "helper_structs/result.h"

/* Self created logarithm without using double or float
 * so that narrowing conversation never happens. */
unsigned log2(unsigned a) {
    unsigned digits = 0;
    while (a > 1) {
        a >>= 1;
        digits++;
    }
    return digits;
}

// Linking the function with C
extern "C" struct Result run_simulation(
    int cycles,
    int directMapped,
    unsigned cacheLines,  
    unsigned cacheLineSize,
    unsigned cacheLatency,
    unsigned memoryLatency,
    size_t numRequests,
    struct Request* requests,
    const char* tracefile) 
    {
        // split in offset and index bits
        unsigned offsetBitsCount = log2(cacheLineSize);
        unsigned offsetBitsMask = (1 << offsetBitsCount) - 1;
        unsigned indexBitsCount = log2(cacheLines);
        unsigned indexBitsMask = (cacheLines - 1) << offsetBitsCount;
        unsigned numberOfSets =  cacheLines / 4;
        unsigned setIndexBitsCount = log2(numberOfSets);
        unsigned setIndexMask = (numberOfSets - 1) << offsetBitsCount;

        // result signals
        sc_signal<size_t> cycleCountSignal;
        sc_signal<size_t, SC_MANY_WRITERS> missCountSignal;
        sc_signal<size_t, SC_MANY_WRITERS> hitCountSignal;
        size_t primitiveGateCount = 0;

        //communication signals
        sc_signal<int> weSignal;
        sc_signal<sc_uint<32>, SC_MANY_WRITERS> dataSignal;
        sc_signal<sc_uint<32>> addrSignal;
        sc_signal<bool, SC_MANY_WRITERS> readySignal;
        readySignal.write(true);

        // clock
        sc_clock clk("clk", 1,SC_NS);

        sc_trace_file* traceFile;

        // If wanted then create tracefile with all signals
        if(tracefile != NULL) {
            traceFile = sc_create_vcd_trace_file (tracefile) ;
            sc_trace(traceFile, cycleCountSignal, " cycles ");
            sc_trace(traceFile, missCountSignal, " misses ");
            sc_trace(traceFile, hitCountSignal, " hits ");
        
            sc_trace(traceFile, addrSignal, " addr ");
            sc_trace(traceFile, dataSignal, " data ");
            sc_trace(traceFile, weSignal, " we ");
            sc_trace(traceFile, readySignal, " cache ready ");
        }

        // Creating and port binding of cpu
        CPU cpu("cpu", numRequests, requests, cycles);
        cpu.clk(clk);
        cpu.cycles.bind(cycleCountSignal);
        cpu.we(weSignal);
        cpu.data(dataSignal);
        cpu.addr(addrSignal);
        cpu.cache_ready(readySignal);


        // Choosing which cache to use
        if(directMapped) {
            // defining the components for this case
            DIRECT_MAPPED_CACHE direct_mapped_cache("direct_cache", cacheLineSize, cacheLatency, memoryLatency, offsetBitsCount, offsetBitsMask, indexBitsCount, indexBitsMask);
            
            // functional bindings
            direct_mapped_cache.cache_ready(readySignal); // inout
            direct_mapped_cache.addrFromCPU(addrSignal);
            direct_mapped_cache.dataFromCPU(dataSignal); // inout
            direct_mapped_cache.weFromCPU(weSignal);

            // result related bindings
            direct_mapped_cache.missesResult.bind(missCountSignal);
            direct_mapped_cache.hitsResult.bind(hitCountSignal);

            // primitive gate count calculation based on the inputs for cacheLines and cacheLineSize
            // 2 Multiplexers
            primitiveGateCount += log2(cacheLines) * 4 * 2;
            // 1 Comparator
            primitiveGateCount += (32 - indexBitsCount - offsetBitsCount) * 2;
            // for each bit in cache 1 SRAM (2 gates) for data and tag
            primitiveGateCount += (cacheLines * 2 * (cacheLineSize * 8 + 32 - indexBitsCount - offsetBitsCount));


        } else {
            // defining the components for this case
            FOURWAY_CACHE fourwaycache("fourwaycache", cacheLineSize, cacheLatency, memoryLatency,
                                       offsetBitsCount, offsetBitsMask, setIndexBitsCount, setIndexMask);
            
            // functional bindings
            fourwaycache.ready(readySignal); // inout
            fourwaycache.addr(addrSignal);
            fourwaycache.data(dataSignal); // inout
            fourwaycache.we(weSignal);

            // result related bindings
            fourwaycache.missCount.bind(missCountSignal);
            fourwaycache.hitCount.bind(hitCountSignal);

            // primitive gate count calculation based on the inputs for cacheLines and cacheLineSize
            //2 numberOfSets-to-1 multiplexers
            primitiveGateCount += log2(numberOfSets) * 4 * 2;
            //4 32-bits comparator
            primitiveGateCount += (2 * (32 - setIndexBitsCount - offsetBitsCount)) * 4;
            //4 32-bits 3-state-buffers
            primitiveGateCount += 32 * 3 * 4;
            //for each bit in cache 1 SRAM (2 gates) for data and tag
            primitiveGateCount += (cacheLines * 2 * (cacheLineSize * 8 + 32 - setIndexBitsCount - offsetBitsCount));
            //replace algorithm
            primitiveGateCount += numberOfSets * 110;


        }

        sc_start();

        // It is used for suppressing a message from systemC about stopping simulation
        std::cout.clear();

        // Closing the tracefile if before opened
        if(tracefile != NULL) {
            sc_close_vcd_trace_file ( traceFile ) ;
        }

        // Creating result struct
        Result result = {
                .cycles = cycleCountSignal.read(),
                .misses = missCountSignal.read(),
                .hits = hitCountSignal.read(),
                .primitiveGateCount = primitiveGateCount + (100 - (primitiveGateCount % 100)) // just round up
        };
        
        return result;
    }

int sc_main(int argc, char* argv[]) {

    // Never used so prints error
    std::cout << "ERROR" << std::endl;
    return 1;
}