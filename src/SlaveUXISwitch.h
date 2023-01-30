#ifndef __SLAVEUXISWITCH_H
#define __SLAVEUXISWITCH_H

#include <list>
#include <deque>
#include <string>
#include <vector>
#include <map>

#include "Request.h"
#include "Controller.h"

using namespace std;

namespace ramulator {

template <typename T>
class SlaveUXISwitch {
public:
    long clk = 0;

    bool print_cmd_trace = false;
    bool record_cmd_trace = false;

    ComputeUnitId_t cimhUnitId;
    vector<Controller<T>*> memoryControllers;
    vector<Request> nmpIns;
    
    SlaveUXISwitch() = default;
    SlaveUXISwitch( ComputeUnitId_t cimhUnitId) : cimhUnitId(cimhUnitId) {}
    ~SlaveUXISwitch() {
        for (auto ctrl: memoryControllers)
            delete ctrl;
    }

    bool enqueue(Request &req) {
        req.arrive = clk;
        req.depart = clk+1;

        if(req.type == Request::Type::NMP) {
            // NMP instruction request has arrived
            // Check whether the request has to be computed at Compute Core at this SlaveUXiSwitch

            if(req.ins.cimhUnit == this->cimhUnitId) {
                req.depart = req.num_cycles_NMP_Ins_Execution[req.ins.opcode];
                nmpIns.push_back(req);
                return true;
            }
        }

        if( memoryControllers[req.addr_vec[int(T::Level::Channel)]]->enqueue(req)) {
            //cout << "request enqueued to controller " << req.addr_vec[int(T::Level::Channel)] << endl;
            return true;
        }
        return false;
    }

    void processNMPIns() {
        // iterate over nmpIns and check the depart time and remove the ins that is done executing
        for( vector<Request>::iterator itr = nmpIns.begin(); itr != nmpIns.end();)
        {
            if( itr->depart <= clk ) {
                // remove the request
                itr = nmpIns.erase(itr);
            } else {
                ++itr;
            }
        }
    }

    void tick() {
        clk++;
        processNMPIns();
        
        for( auto ctrl: memoryControllers) {
            ctrl->tick();
        }
    }

    bool processPendingNMPIns() {
        // iterate over nmpIns and check the depart time and remove the ins that is done executing
        for( vector<Request>::iterator itr = nmpIns.begin(); itr != nmpIns.end();)
        {
            if( itr->depart <= clk ) {
                // remove the request
                itr = nmpIns.erase(itr);
            } else {
                ++itr;
            }
        }
        return (nmpIns.size() == 0);
    }
    
    void record_core(int coreid) {}
    void finish() {
        bool pending = processPendingNMPIns();
        if( pending ) {
            // more requests pending
        }
    }

};// class SlaveUXISwitch
  
}  // namespace ramulator
#endif //__SLAVEUXISWITCH_H
