#ifndef __MEMORYHUB_H
#define __MEMORYHUB_H

#include "Config.h"
#include "DRAM.h"
#include "Request.h"
#include "Controller.h"
//#include "SpeedyController.h"
#include "Statistics.h"
#include "GDDR5.h"
#include "HBM.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "WideIO2.h"
#include "DSARP.h"
#include <vector>
#include <functional>
#include <cmath>
#include <cassert>
#include <tuple>
#include "MasterUXISwitch.h"

using namespace std;

namespace ramulator
{

class MemoryHubBase{
public:
    MemoryHubBase() {}
    virtual ~MemoryHubBase() {}
    virtual double clk_ns() = 0;
    virtual void tick() = 0;
    virtual bool send(Request req) = 0;
    virtual int pending_requests() = 0;
    virtual void finish(void) = 0;
    /*
    virtual long page_allocator(long addr, int coreid) = 0;
    virtual void record_core(int coreid) = 0;
    virtual void set_high_writeq_watermark(const float watermark) = 0;
    virtual void set_low_writeq_watermark(const float watermark) = 0;
    */
};

//template <class T, template<typename> class Controller = Controller >
//template <class T, template<typename> class MasterUXISwitch = MasterUXISwitch>
template <class T>
class MemoryHub : public MemoryHubBase
{
protected:
  long max_address;
  //MapScheme mapping_scheme;

 public:
  enum class Type {  // @rajat: Add the MasterUXISwitch and SlaveUXISwitch config handling bits as well
      // Ms: Master Switch, Ss: Slave switch, Ch: Channel, Ra: Rank, Ba: Bank, Ro: Row#, Co: Col#
      MsSsChRaBaRoCo,
          MsSsRoBaRaCoCh,
          MAX,
          } type = Type::MsSsChRaBaRoCo;

    
    // Ms---- Ss----- Ro----- Ba----------- Ch-------- Ro--------- Co-------- Physical address 
    vector<MasterUXISwitch<T>*> masterUxiSwitches;
    T * spec;
    vector<int> addr_bits;

    vector<int> free_physical_pages;
    long free_physical_pages_remaining;

    int tx_bits;

    MemoryHub(const Config& configs, vector<MasterUXISwitch<T>*> masterUxiSwitches)
        : masterUxiSwitches(masterUxiSwitches),
          spec(masterUxiSwitches[0]->slaveSwitches[0]->memoryControllers[0]->channel->spec),
          addr_bits(int(T::Level::MAX))
    {
        // make sure 2^N channels/ranks
        // TODO support channel number that is not powers of 2
        int *sz = spec->org_entry.count;
        assert((sz[2] & (sz[2] - 1)) == 0);  // @rajat: assertion for the # of channels to be power of 2
        assert((sz[3] & (sz[3] - 1)) == 0);  // @rajat: assertion for the # of ranks to be power of 2
	assert((sz[0] & (sz[0] - 1)) == 0);  // @rajat: assertion for the # of master uxi switches to be power of 2
	assert((sz[int(T::Level::SlaveUxiSwitch)] & (sz[int(T::Level::SlaveUxiSwitch)] - 1)) == 0);  // @rajat: assertion for the # of slave uxi switches to be power of 2
	
        // validate size of one transaction
        int tx = (spec->prefetch_size * spec->channel_width / 8);
        tx_bits = calc_log2(tx);
        assert((1<<tx_bits) == tx);

        max_address = spec->channel_width / 8;

        for (unsigned int lev = 0; lev < addr_bits.size(); lev++) {
            addr_bits[lev] = calc_log2(sz[lev]);
            //std::cout << "addr_bits[" << lev << "] : " << addr_bits[lev] << std::endl;
            max_address *= sz[lev];
        }

        //addr_bits[int(T::Level::MAX) - 1] -= calc_log2(spec->prefetch_size); //OLD code. @rajat: check this out, should this be -= calc_log2(spec->prefetch_size) alone or -= tx_bits
        addr_bits[int(T::Level::MAX) - 1] -= tx_bits;  // NEW code. @rajat: ensure this is right

        // construct a list of available pages
        free_physical_pages_remaining = max_address >> 12;

        free_physical_pages.resize(free_physical_pages_remaining, -1);
    }

    ~MemoryHub()
    {
        for (auto ms: masterUxiSwitches)
            delete ms;
        delete spec;
    }

    double clk_ns()
    {
        return spec->speed_entry.tCK;
    }

    void record_core(int coreid) {
        for (auto ms : masterUxiSwitches) {
            ms->record_core(coreid);
        }
    }

    void tick()
    {
      /*
        ++num_dram_cycles;
        int cur_que_req_num = 0;
        int cur_que_readreq_num = 0;
        int cur_que_writereq_num = 0;
        for (auto ctrl : ctrls) {
          cur_que_req_num += ctrl->readq.size() + ctrl->writeq.size() + ctrl->pending.size();
          cur_que_readreq_num += ctrl->readq.size() + ctrl->pending.size();
          cur_que_writereq_num += ctrl->writeq.size();
        }
        in_queue_req_num_sum += cur_que_req_num;
        in_queue_read_req_num_sum += cur_que_readreq_num;
        in_queue_write_req_num_sum += cur_que_writereq_num;
      */
        for (auto ms : masterUxiSwitches) {
            ms->tick();
        }
    }

    bool send(Request req)
    {
        req.addr_vec.resize(addr_bits.size());
        long addr = req.addr;
        int coreid = req.coreid;

        // Each transaction size is 2^tx_bits, so first clear the lowest tx_bits bits
        clear_lower_bits(addr, tx_bits); //@rajat: Check this whether this is needed, feels to be wrong, that's why commenting it out

        switch(int(type)){
        case int(Type::MsSsChRaBaRoCo):
            for (int i = addr_bits.size() - 1; i >= 0; i--) {
                req.addr_vec[i] = slice_lower_bits(addr, addr_bits[i]);
            }
            break;
        case int(Type::MsSsRoBaRaCoCh):
            req.addr_vec[0] = slice_lower_bits(addr, addr_bits[0]);
            req.addr_vec[addr_bits.size() - 1] = slice_lower_bits(addr, addr_bits[addr_bits.size() - 1]);
            for (int i = 1; i <= int(T::Level::Row); i++) {
                req.addr_vec[i] = slice_lower_bits(addr, addr_bits[i]);
            }
            break;
        default:
            assert(false);
        }
        
        if(masterUxiSwitches[req.addr_vec[int(T::Level::MasterUxiSwitch)]]->enqueue(req)) {
            // tally stats here to avoid double counting for requests that aren't enqueued
            if (req.type == Request::Type::READ) {
                // @rajat: collect Stat
            }
            if (req.type == Request::Type::WRITE) {
                // @rajat: collect Stat
            }
            if (req.type == Request::Type::NMP) {
                // @rajat: collect Stat
            }
            return true;
        }

        return false;
    }
/*
    void init_mapping_with_file(string filename){
        ifstream file(filename);
        assert(file.good() && "Bad mapping file");
        // possible line types are:
        // 0. Empty line
        // 1. Direct bit assignment   : component N   = x
        // 2. Direct range assignment : component N:M = x:y
        // 3. XOR bit assignment      : component N   = x y z ...
        // 4. Comment line            : # comment here
        string line;
        char delim[] = " \t";
        while (getline(file, line)) {
            short capture_flags = 0;
            int level = -1;
            int target_bit = -1, target_bit2 = -1;
            int source_bit = -1, source_bit2 = -1;
            // cout << "Processing: " << line << endl;
            bool is_range = false;
            while (true) { // process next word
                size_t start = line.find_first_not_of(delim);
                if (start == string::npos) // no more words
                    break;
                size_t end = line.find_first_of(delim, start);
                string word = line.substr(start, end - start);
                
                if (word.at(0) == '#')// starting a comment
                    break;
                
                size_t col_index;
                int source_min, target_min, target_max;
                switch (capture_flags){
                    case 0: // capturing the component name
                        // fetch component level from channel spec
                        for (int i = 0; i < int(T::Level::MAX); i++)
                            if (word.find(T::level_str[i]) != string::npos) {
                                level = i;
                                capture_flags ++;
                            }
                        break;

                    case 1: // capturing target bit(s)
                        col_index = word.find(":");
                        if ( col_index != string::npos ){
                            target_bit2 = stoi(word.substr(col_index+1));
                            word = word.substr(0,col_index);
                            is_range = true;
                        }
                        target_bit = stoi(word);
                        capture_flags ++;
                        break;

                    case 2: //this should be the delimiter
                        assert(word.find("=") != string::npos);
                        capture_flags ++;
                        break;

                    case 3:
                        if (is_range){
                            col_index = word.find(":");
                            source_bit  = stoi(word.substr(0,col_index));
                            source_bit2 = stoi(word.substr(col_index+1));
                            assert(source_bit2 - source_bit == target_bit2 - target_bit);
                            source_min = min(source_bit, source_bit2);
                            target_min = min(target_bit, target_bit2);
                            target_max = max(target_bit, target_bit2);
                            while (target_min <= target_max){
                                mapping_scheme[level][target_min].push_back(source_min);
                                // cout << target_min << " <- " << source_min << endl;
                                source_min ++;
                                target_min ++;
                            }
                        }
                        else {
                            source_bit = stoi(word);
                            mapping_scheme[level][target_bit].push_back(source_bit);
                        }
                }
                if (end == string::npos) { // this is the last word
                    break;
                }
                line = line.substr(end);
            }
        }
        if (dump_mapping)
            dump_mapping_scheme();
    }
*/
/*
    void dump_mapping_scheme(){
        cout << "Mapping Scheme: " << endl;
        for (MapScheme::iterator mapit = mapping_scheme.begin(); mapit != mapping_scheme.end(); mapit++)
        {
            int level = mapit->first;
            for (MapSchemeEntry::iterator entit = mapit->second.begin(); entit != mapit->second.end(); entit++){
                cout << T::level_str[level] << "[" << entit->first << "] := ";
                cout << "PhysicalAddress[" << *(entit->second.begin()) << "]";
                entit->second.erase(entit->second.begin());
                for (MapSrcVector::iterator it = entit->second.begin() ; it != entit->second.end(); it ++)
                    cout << " xor PhysicalAddress[" << *it << "]";
                cout << endl;
            }
        }
    }
*/
/*
    void apply_mapping(long addr, std::vector<int>& addr_vec){
        int *sz = spec->org_entry.count;
        int addr_total_bits = sizeof(addr_vec)*8;
        int addr_bits [int(T::Level::MAX)];
        for (int i = 0 ; i < int(T::Level::MAX) ; i ++)
        {
            if ( i != int(T::Level::Row))
            {
                addr_bits[i] = calc_log2(sz[i]);
                addr_total_bits -= addr_bits[i];
            }
        }
        // Row address is an integer.
        addr_bits[int(T::Level::Row)] = min((int)sizeof(int)*8, max(addr_total_bits, calc_log2(sz[int(T::Level::Row)])));

        // printf("Address: %lx => ",addr);
        for (unsigned int lvl = 0; lvl < int(T::Level::MAX); lvl++)
        {
            unsigned int lvl_bits = addr_bits[lvl];
            addr_vec[lvl] = 0;
            for (unsigned int bitindex = 0 ; bitindex < lvl_bits ; bitindex++){
                bool bitvalue = false;
                for (MapSrcVector::iterator it = mapping_scheme[lvl][bitindex].begin() ;
                    it != mapping_scheme[lvl][bitindex].end(); it ++)
                {
                    bitvalue = bitvalue xor get_bit_at(addr, *it);
                }
                addr_vec[lvl] |= (bitvalue << bitindex);
            }
            // printf("%s: %x, ",T::level_str[lvl].c_str(),addr_vec[lvl]);
        }
        // printf("\n");
    }
*/
    int pending_requests()
    {
        int reqs = 0;
        for (auto ms: masterUxiSwitches) { 
	  //reqs += ctrl->readq.size() + ctrl->writeq.size() + ctrl->otherq.size() + ctrl->actq.size() + ctrl->pending.size();
	  // @rajat: Add the logic here for the count of pending requests
	}
        return reqs;
    }

    void finish(void) {
      for (auto ms : masterUxiSwitches) {
        ms->finish();
      }
    }

/*
    long page_allocator(long addr, int coreid) {
        long virtual_page_number = addr >> 12;

        switch(int(translation)) {
            case int(Translation::None): {
              return addr;
            }
            case int(Translation::Random): {
                auto target = make_pair(coreid, virtual_page_number);
                if(page_translation.find(target) == page_translation.end()) {
                    // page doesn't exist, so assign a new page
                    // make sure there are physical pages left to be assigned

                    // if physical page doesn't remain, replace a previous assigned
                    // physical page.
                    if (!free_physical_pages_remaining) {
                      physical_page_replacement++;
                      long phys_page_to_read = lrand() % free_physical_pages.size();
                      assert(free_physical_pages[phys_page_to_read] != -1);
                      page_translation[target] = phys_page_to_read;
                    } else {
                        // assign a new page
                        long phys_page_to_read = lrand() % free_physical_pages.size();
                        // if the randomly-selected page was already assigned
                        if(free_physical_pages[phys_page_to_read] != -1) {
                            long starting_page_of_search = phys_page_to_read;

                            do {
                                // iterate through the list until we find a free page
                                // TODO: does this introduce serious non-randomness?
                                ++phys_page_to_read;
                                phys_page_to_read %= free_physical_pages.size();
                            }
                            while((phys_page_to_read != starting_page_of_search) && free_physical_pages[phys_page_to_read] != -1);
                        }

                        assert(free_physical_pages[phys_page_to_read] == -1);

                        page_translation[target] = phys_page_to_read;
                        free_physical_pages[phys_page_to_read] = coreid;
                        --free_physical_pages_remaining;
                    }
                }

                // SAUGATA TODO: page size should not always be fixed to 4KB
                return (page_translation[target] << 12) | (addr & ((1 << 12) - 1));
            }
            default:
                assert(false);
        }

    }
*/
private:

    int calc_log2(int val){
        int n = 0;
        while ((val >>= 1))
            n++;
        return n;
    }
    int slice_lower_bits(long& addr, int bits)
    {
        int lbits = addr & ((1<<bits) - 1);
        addr >>= bits;
        return lbits;
    }
    bool get_bit_at(long addr, int bit)
    {
        return (((addr >> bit) & 1) == 1);
    }
    void clear_lower_bits(long& addr, int bits)
    {
        addr >>= bits;
    }
    long lrand(void) {
        if(sizeof(int) < sizeof(long)) {
            return static_cast<long>(rand()) << (sizeof(int) * 8) | rand();
        }

        return rand();
    }
 }; /* class MemoryHub */

 
} /*namespace ramulator*/

#endif /*__MEMORYHUB_H*/
