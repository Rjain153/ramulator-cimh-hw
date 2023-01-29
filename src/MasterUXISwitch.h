#ifndef __MASTERUXISWITCH_H
#define __MASTERUXISWITCH_H

#include <list>
#include <deque>
#include <string>
#include <vector>
#include <map>

#include "Request.h"
#include "SlaveUXISwitch.h"
#include "ComputeCore.h"

using namespace std;

namespace ramulator {

template<typename T>
class MasterUXISwitch {
 private:


 public:
    long clk = 0;

    bool print_cmd_trace = false;
    bool record_cmd_trace = false;

    vector<SlaveUXISwitch<T>*> slaveSwitches;
    //map<int, SlaveUXISwitch<T>*> slaveSwitches;
    
    MasterUXISwitch() {
    }
    
    ~MasterUXISwitch() {
        for (auto ss: slaveSwitches)
            delete ss;
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

            // Check whether the request has to be computed at Compute Core at MasterUXiSwitch
        }

        if(slaveSwitches[req.addr_vec[int(T::Level::SlaveUxiSwitch)]]->enqueue(req)) {
            //cout << "request enqueued to slaveUxiSwitch " << req.addr_vec[int(T::Level::SlaveUxiSwitch)] << endl;
            return true;
        }
        return false;
    }

    void tick() {
        clk++;
        
        // Handle the processing of an NMP request if pending
        for( auto ss: slaveSwitches) {
            ss->tick();
        }
    }

    void record_core(int coreid) {
        
    }
    
    void finish() {

    }

};// class MasterUXISwitch
  


  
}  // namespace ramulator
#endif //__MASTERUXISWITCH_H
