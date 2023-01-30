#ifndef __REQUEST_H
#define __REQUEST_H

#include <vector>
#include <functional>
#include <map>

using namespace std;

namespace ramulator {

#define NMP_OP_CYCLES 4

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
};

typedef struct __Tag_t {
    SrcID_t src_id; // {coreId,threadId}
    InfId_t inf_id; // inference id {batchNumber, inferenceNumber}
    EmbTblId_t embTbl_id;
} Tag_t;

typedef struct __NMP_Ins {
    CimhOp_t opcode; // gather, reduce ??
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

class Request {
public:
    bool is_first_command;
    long addr;
    // long addr_row;
    vector<int> addr_vec;
    // specify which core this request sent from, for virtual address translation
    int coreid;

    // ------ NMP specific starts here ------
    NMP_Ins ins;
    int result; // result of computation

    // ------ NMP specific ends here --------
    map<CimhOp_t,uint32_t> num_cycles_NMP_Ins_Execution = { {CimhOp_t::OP_SUM, 2},
                                                            {CimhOp_t::OP_CONCATE, 1},
                                                            {CimhOp_t::OP_AVERAGE, 4},
                                                            {CimhOp_t::OP_GRAD_ADD, 2} };
    
    enum class Type {
        READ,
            WRITE,
            REFRESH,
            POWERDOWN,
            SELFREFRESH,
            EXTENSION,
            NMP,
            MAX
            } type;
    
    long arrive = -1;
    long depart = -1;
    function<void(Request&)> callback; // call back with more info

    Request(long addr, Type type, int coreid = 0)
        : is_first_command(true), addr(addr), coreid(coreid), type(type),
      callback([](Request& req){}) {}

    Request(long addr, Type type, function<void(Request&)> callback, int coreid = 0)
        : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback) {}

    Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback, int coreid = 0)
        : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), callback(callback) {}

    Request()
        : is_first_command(true), coreid(0) {}
};

} // namespace ramulator

#endif //__REQUEST_H

