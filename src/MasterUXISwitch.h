#ifndef __MASTERUXISWITCH_H
#define __MASTERUXISWITCH_H

#include <list>
#include <deque>
#include <string>
#include <vector>
#include <map>

#include "Request.h"
#include "SlaveUXISwitch.h"
//#include "ComputeCore.h"

using namespace std;

namespace ramulator {

template<typename T>
class MasterUXISwitch {
public:
    long clk = 0;

    bool print_cmd_trace = false;
    bool record_cmd_trace = false;

    vector<SlaveUXISwitch<T>*> slaveSwitches;
    vector<Request> nmpIns;
    ComputeUnitId_t cimhUnitId;
    //map<int, SlaveUXISwitch<T>*> slaveSwitches;
    
    MasterUXISwitch() = default;
    MasterUXISwitch( ComputeUnitId_t cimhUnitId ) : cimhUnitId(cimhUnitId) {}
    
    ~MasterUXISwitch() {
        for (auto ss: slaveSwitches)
            delete ss;
    }

    bool enqueue(Request& req) {
        req.arrive = clk;
        req.depart = clk+1;

        if(req.type == Request::Type::NMP) {
            // NMP instruction request has arrived
            // Check whether the request has to be computed at Compute Core at this MasterUXiSwitch

            if(req.ins.cimhUnit == this->cimhUnitId) {
                req.depart = req.num_cycles_NMP_Ins_Execution[req.ins.opcode];
                nmpIns.push_back(req);
                return true;
            }
        }

        if(slaveSwitches[req.addr_vec[int(T::Level::SlaveUxiSwitch)]]->enqueue(req)) {
            //cout << "request enqueued to slaveUxiSwitch " << req.addr_vec[int(T::Level::SlaveUxiSwitch)] << endl;
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
        
        // Handle the processing of an NMP request if pending
        for( auto ss: slaveSwitches) {
            ss->tick();
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

};// class MasterUXISwitch
  


  
}  // namespace ramulator
#endif //__MASTERUXISWITCH_H
