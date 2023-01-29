#ifndef __SLAVEUXISWITCH_H
#define __SLAVEUXISWITCH_H

#include <list>
#include <deque>
#include <string>
#include <vector>
#include <map>

#include "Request.h"
//#include "MemoryController.h"
#include "Controller.h"
#include "ComputeCore.h"

using namespace std;

namespace ramulator {

template <typename T>
class SlaveUXISwitch {
private:
    

public:
    long clk = 0;

    bool print_cmd_trace = false;
    bool record_cmd_trace = false;

    ComputeCore cc; // cc associated with this SlaveUXISwitch
    
    //map<int,MemoryController*> memoryControllers;
    vector<Controller<T>*> memoryControllers;
    //vector<Controller<T>*> memoryControllers;
    

    SlaveUXISwitch() {
      
    }
    
    ~SlaveUXISwitch() {
        for (auto ctrl: memoryControllers)
            delete ctrl;
    }

    bool enqueue(Request& req = nullptr, ComputeReq& creq= nullptr) {

        req.arrive = clk;
        req.depart = clk+1;

        if( creq != nullptr ) {
            // NMP instruction request has arrived

            // Check whether the request has to be computed at Compute Core at SlaveUXISwitch
        }
        
        if( memoryControllers[req.addr_vec[int(T::Level::Channel)]]->enqueue(req)) {
            //cout << "request enqueued to controller " << req.addr_vec[int(T::Level::Channel)] << endl;
            return true;
        }
        return false;
    }

    void tick() {
        clk++;

        for( auto ctrl: memoryControllers) {
            ctrl->tick();
        }
    }

    void finish() {

    }

};// class SlaveUXISwitch
  
}  // namespace ramulator
#endif //__SLAVEUXISWITCH_H
