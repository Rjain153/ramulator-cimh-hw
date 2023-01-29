#ifndef __COMPUTECORE_H
#define __COMPUTECORE_H

#include <list>
#include <deque>
#include <string>
#include <vector>
#include <map>

#include "ComputeRequest.h"

using namespace std;

namespace ramulator {

uint32_t num_cycles_NMP_Ins_Execution (CimhOp_t op) {
  uint32_t cycles = 0;
  switch (op)
    {
    case CimhOp_t::OP_SUM:
      cycles = 2;
	  break;
    case CimhOp_t::OP_CONCATE:
      cycles = 1;
      break;
    case CimhOp_t::OP_AVERAGE:
      cycles = 4;
      break;
    case CimhOp_t::OP_GRAD_ADD:
      cycles = 2;
      break;
    }
  return cycles;
};

clasa ComputeCore {
  public:
    Memory *memory; // @rajat: need to connect to the right memory object to fetch the operands

    queue<ComputeRequest> inpQueue;
    
    long clk = 0;

    ComputeCore( clk_t memory_clk) {
        // @rajat: verify with Omji whether ComputeCore and memory frequency would be same or different
        clk = memory_clk;
    }
  
    bool enqueue(ComputeRequest& req) {
        // @rajat: Handle the reception of a new request
        // 1. See whether we need to introduce a queue for the incoming requests
        // 2. There will be push back from the memory controller to Slave UXI switches and that
        //    will be propagated to MasterUxiSwitch
        inpQueue.push(req);
        req.arrive = clk;
    }

    void execute( ComputeRequest creq ) {

        int data = memory->read(creq.ins.phyAddr);
        int partialRes = memory->result(creq.ins.outputId);
        
        switch (creq.ins.opcode)
        {
        case CimhOp_t::OP_SUM:
            creq.result = data + partialRes;
            break;
        case CimhOp_t::OP_CONCATE:
            creq.result = data + partialRes;
            break;
        case CimhOp_t::OP_AVERAGE:
            creq.result = data + partialRes;
            break;
        case CimhOp_t::OP_GRAD_ADD:
            creq.result = data + partialRes;
            break;
        }

        // request would finish only after depart cycles have been modeled
        creq.depart = num_cycles_NMP_Ins_Execution(creq.ins.opcode);
    }


    void retireReq( ComputeRequest *creq ) {
        if( creq->depart >= clk) { // retire only when execution cycles have passed
            memory->write( creq->ins.outputId. creq->result);
        }
    }
  
    void tick() {
        clk++;

        vector<int> finishedReqs;
        // Check if any inProgress request has reached retire stage
        while( int i = 0; i < inProgressReqs.size(); i++  ) {
            if( inProgressReqs[i].depart >= clk ) {
                retireReq(inProgressReqs[i]);
                finishedReqs.push_back(i);
            }
        }

        // Now, remove finished requests from the inProgressReqs vector
        for( auto itr = finishedReqs.begin(); itr != finishedReqs.end(); itr++ ) {
            inProgressReqs.erase(inProgressReqs.begin() + *itr);
        }
        
        // Handle the processing of an NMP request if pending
        // check the input request queue for the NMP instructions, if pending, pick the request in fifo order
        // and process it
        if( !inpQueue.empty() ) {
            ComputeRequest creq = inpQueue.front();
            execute(creq);
            inpQueue.pop();
            inProgressReqs.push_back(creq);
        }
    }

    void finish() {
        bool pending = true;
        while(pending) {
            // check for any inProgress request and wait for them to finish
            while( int i = 0; i < inProgressReqs.size(); i++  ) {
                if( inProgressReqs[i].depart >= clk ) {
                    retireReq(inProgressReqs[i]);
                }
            }
            pending = inProgressReqs.size();
            clk++;
        }
    }

};// class ComputeCore
  
}  // namespace ramulator
#endif //__COMPUTECORE_H
