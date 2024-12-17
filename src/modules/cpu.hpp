#ifndef CPU_HPP
#define CPU_HPP

#include <systemc>
#include "systemc.h"

// helper structs
#include "../helper_structs/request.h"
#include "../helper_structs/result.h"

using namespace sc_core;

// It's used for scheduling the requests to the cache
SC_MODULE(CPU) {

    // signals
    sc_in<bool> clk;
    // cache signals that it's ready
    sc_inout<bool> cache_ready;

    // Request signals
    sc_out<sc_uint<32>> addr;
    sc_inout<sc_uint<32>> data;
    sc_out<int> we;

    // Request related variables
    Request* requests;
    size_t numRequests;
    size_t currentRequest;

    // Cycle related variables
    size_t maxCycles;
    size_t elapsedCycles;

    // Result related signals

    // only result variable that comes from the CPU
    sc_out<size_t> cycles;


    SC_CTOR(CPU);
    CPU(sc_module_name name, size_t numRequests, Request* requests, int cycles) :
    sc_module(name), numRequests(numRequests), requests(requests), maxCycles(cycles) {

        elapsedCycles = 0;
        currentRequest = 0;

        SC_THREAD(run);
        sensitive << clk.pos();
        // It is set so that it can't send request when it's created
        dont_initialize();
    }
    

    void run() {
        while(true) {

            cycles->write(++elapsedCycles);

            // If cache ready send the next request
            if(cache_ready->read()) {

                // Writing request signals
                addr->write(requests[currentRequest].addr);
                data->write(requests[currentRequest].data);
                we->write(requests[currentRequest].we);

                // Telling cache that it sent a request
                cache_ready->write(false);

                // For next request incrementing
                ++currentRequest;
            }

            // Wait for cache to process the current request
            wait();

            // Cache is finished with the current request
            if(cache_ready->read()) {

                // Update the data of the last request if it was a read operation
                if(currentRequest > 0 && !requests[currentRequest - 1].we) {
                    requests[currentRequest - 1].data = data->read();
                }

                // Check whether there are requests to send
                if(currentRequest >= numRequests) {
                    break;
                }
            }

            // Check if the all cycles are used
            if(elapsedCycles >= maxCycles) {
                // last cycle reached so the CPU will not wait for another clock tick and stop immediately
                break;
            }
        }

        // Check whether there are request that not processed
        if(currentRequest < numRequests || !cache_ready) {
            // Cache has either not processed all requests or was currently processing one -> also not finished
            cycles->write(SIZE_MAX);
        }

        // If sc_stop() used some message is shown in the console. It is used to suppress this.
        std::cout.setstate(std::ios_base::failbit);
        sc_stop();
    }
};

#endif