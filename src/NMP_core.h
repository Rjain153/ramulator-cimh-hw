#ifndef __NMPCORE_H
#define __NMPCORE_H

#include <list>
#include <deque>
#include <string>
#include <vector>

#include "Request.h"

using namespace std;


namespace ramulator {
  
  class NMP_Core{
    /* Member variables */
    long clk = 0;

    struct Queue {
      list<Request> q;
      unsigned int max = 32;
      unsigned int size() { return q.size(); };
    };
    
    Queue inputQ; // queue for embedding vectors read
    Queue outputQ; // queue for embedding pooling operation output

    enum class Pooling_Operation
    {
      SUM,
	AVG,
	WEIGHTED_SUM,
	MAX
    } pooling_op;
              
    bool is_busy = false;
    bool is_ready = true;

    bool print_cmd_trace = false;
    bool record_cmd_trace = false;


    NMP_Core() {
      // @rajat: Constructor logic, object set-up
      
    }
    
    ~NMP_Core() {
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

  };/*class NMP_Core*/
  


  
}  /*namespace ramulator*/
#endif /*__NMPCORE_H*/
