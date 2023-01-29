#include <iostream>

//------- copied from cimh-sw infra -------
typedef uint8_t  EmbTblId_t;
typedef uint64_t Addr_t;
typedef uint32_t SrcID_t;  // (core id, thread id) --> srcID
typedef uint32_t InfId_t;  // (batch num, inference num) --> InfID
typedef uint32_t InstanceId_t;  // model instance id
typedef uint64_t Sparse_Id_t;
typedef uint32_t TagID_t;
typedef uint32_t MemData_t;
typedef uint32_t ComputeUnitId_t;
typedef uint8_t  Entry_Size;
typedef uint8_t  ModelTagId;
typedef uint64_t OutputMemId; // address of the partial sum results

enum CimhOp_t {
    OP_SUM = 0,
    OP_CONCATE = 1,
    OP_AVERAGE = 2,
    OP_GRAD_ADD = 3,
    NUM_CIMH_OPS = 4
};

typedef struct __Tag_t {
    SrcID_t src_id; // {coreId,threadId}
    InfId_t inf_id; // inference id {batchNumber, inferenceNumber}
    EmbTblId_t embTbl_id;
} Tag_t;

typedef struct __NMP_Ins {
    uint8_t opcode; // gather, reduce ??
    uint64_t phyAddr;
    Entry_Size  size; // in bytes
    TagID_t srcTag;
    ModelTagId modelTag;
    ComputeUnitId_t cimhUnit;
    OutputMemId outputId; // partial sums are accumulated here, NMP instructions with same SrcId and TagId would accumulate result here
    // already accumulated partial sum + embedding row result --> new partial sum
    // new partial sum --> accumulate it in memory location identified by outputId
} NMP_Ins;
//------- copied from cimh-sw infra (ENDS here) -------

using clk_t = uint64_t;

class ComputeRequest {
public:
  NMP_Ins ins;
  clk_t arrive = clk_invalid;
  clk_t depart = clk_invalid;
    
  /*
  int address; // memory address of data
  int resAddress; // memory address of output resultant
  int operation; // operation to be performed on data
  */
  int result; // result of computation
  
  ComputeRequest(Addr_t address, OutputMemId resAddress, uint8_t opcode, Entry_Size entrySize, TagID_t srcTag, ModelTagId modelTag, ComputeUnitId_t cimhUnit ) {
    ins.opcode  = operation;
    ins.phyAddr = address;
    ins.size    = entrySize;
    ins.srcTag  = srcTag;
    ins.modelTag = modelTag;
    ins.cimhUnit = cimhUnit;
    ins.outputId = resAddress;
  }
  
};
