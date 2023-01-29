#ifndef __DIMM_H
#define __DIMM_H

#include <list>
#include <deque>
#include <string>
#include <vector>

#include "Request.h"

using namespace std;

namespace ramulator {

class DIMM {
    /* Member variables */
    long clk = 0;

    bool print_cmd_trace = false;
    bool record_cmd_trace = false;


    DIMM() {
      // @rajat: Constructor logic, object set-up
      
    }
    
    ~DIMM() {
      // @rajat: Handle the object deletion
    }

    

    void tick() {
      clk++;

      //@rajat: Fill in the logic
      // Handle the processing of an NMP request if pending 
    }

    void finish() {
      // @rajat: Fill in the logic
    }

  };/*class DIMM*/
 
}  /*namespace ramulator*/
#endif /*__DIMM_H*/
