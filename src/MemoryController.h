#ifndef __MEMORYCONTROLLER_H
#define __MEMORYCONTROLLER_H

#include <list>
#include <deque>
#include <string>
#include <vector>
#include <map>

#include "Request.h"
#include "DIMM.h"
#include "ComputeCore.h"

using namespace std;

namespace ramulator {

class MemoryController {
private:
    


public:
    long clk = 0;
    
    bool print_cmd_trace = false;
    bool record_cmd_trace = false;

    ComputeCore cc; // cc associated with this controller

    // @rajat :placeholder for DIMMs attached to this memory controller instance
    map<int, DIMM*> dimms;

    MemoryController() {
      
    }
    
    ~MemoryController() {
        for( auto dd: dimms )
            delete dd;
    }

    bool enqueue(Request& req = nullptr, ComputeReq& creq = nullptr) {
      // @rajat: Handle the reception of a new request
      // 1. See whether we need to introduce a queue for the incoming requests
      // 2. There will be push back from the memory controller to Slave UXI switches and that
      // will be propagated to MasterUxiSwitch
        
        req.arrive = clk;
        req.depart = clk+1;
        
        if(creq != nullptr) {
            // NMP instruction request has arrived

            // Check whether the request has to be computed at Compute Core at MemoryController
        }

        if(dimms[req.addr_vec[int(T::Level::DIMM)]]->enqueue(req)) {
            //cout << "request enqueued to DIMM " << req.addr_vec[int(T::Level::DIMM)] << endl;
            return true;
        }
        return false;
    }

    void tick() {
        clk++;
        
        // Handle the processing of an NMP request if pending
        for( auto dd: dimms) {
            dd->tick();
        }
    }

    void finish() {
        // @rajat: Fill in the logic
    }

  };/*class MemoryController*/
  


  
}  /*namespace ramulator*/
#endif /*__MEMORYCONTROLLER_H*/
