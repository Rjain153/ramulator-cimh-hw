#include "Processor.h"
#include "Config.h"
#include "Controller.h"
//#include "SpeedyController.h"
//#include "Memory.h"
#include "DRAM.h"
#include "Statistics.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <functional>
#include <map>

/* Standards */
#include "Gem5Wrapper.h"
#include "DDR3.h"
#include "DDR4.h"
#include "DSARP.h"
#include "GDDR5.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "SALP.h"
#include "ALDRAM.h"
#include "TLDRAM.h"
#include "STTMRAM.h"
#include "PCM.h"

/* Disaggregated memory */
#include "MasterUXISwitch.h"
#include "SlaveUXISwitch.h"
#include "DIMM.h"
#include "MemoryHub.h"

using namespace std;
using namespace ramulator;

bool ramulator::warmup_complete = false;

template<typename T>
void run_dramtrace(const Config& configs, MemoryHub<T>& memoryHub, const char* tracename) {

    /* initialize DRAM trace */
    Trace trace(tracename);

    /* run simulation */
    bool stall = false, end = false;
    int reads = 0, writes = 0, clks = 0, nmps = 0;
    long addr = 0;
    Request::Type type = Request::Type::READ;
    map<int, int> latencies;
    auto read_complete = [&latencies](Request& r){latencies[r.depart - r.arrive]++;};

    Request req(addr, type, read_complete);
    //return;

    while (!end || memoryHub.pending_requests()){
        if (!end && !stall){
            end = !trace.get_dramtrace_request(addr, type);
        }

        if (!end){
            req.addr = addr;
            req.type = type;
            stall = !memoryHub.send(req);
            if (!stall){
                if (type == Request::Type::READ) {
                    reads++;
                } else {
                    if (type == Request::Type::WRITE) {
                        writes++;
                    } else {
                        nmps++;
                    }
                }
            }
        }
        memoryHub.tick();
        clks ++;
        Stats::curTick++; // memory clock, global, for Statistics
    }
    // This a workaround for statistics set only initially lost in the end
    memoryHub.tick();
    memoryHub.finish();
    Stats::statlist.printall();
}

template <typename T>
void run_cputrace(const Config& configs, Memory<T, Controller>& memory, const std::vector<const char *>& files)
{
    int cpu_tick = configs.get_cpu_tick();
    int mem_tick = configs.get_mem_tick();
    auto send = bind(&Memory<T, Controller>::send, &memory, placeholders::_1);
    Processor proc(configs, files, send, memory);

    long warmup_insts = configs.get_warmup_insts();
    bool is_warming_up = (warmup_insts != 0);

    for(long i = 0; is_warming_up; i++){
        proc.tick();
        Stats::curTick++;
        if (i % cpu_tick == (cpu_tick - 1))
            for (int j = 0; j < mem_tick; j++)
                memory.tick();

        is_warming_up = false;
        for(int c = 0; c < proc.cores.size(); c++){
            if(proc.cores[c]->get_insts() < warmup_insts)
                is_warming_up = true;
        }

        if (is_warming_up && proc.has_reached_limit()) {
            printf("WARNING: The end of the input trace file was reached during warmup. "
                    "Consider changing warmup_insts in the config file. \n");
            break;
        }

    }

    warmup_complete = true;
    printf("Warmup complete! Resetting stats...\n");
    Stats::reset_stats();
    proc.reset_stats();
    assert(proc.get_insts() == 0);

    printf("Starting the simulation...\n");

    int tick_mult = cpu_tick * mem_tick;
    for (long i = 0; ; i++) {
        if (((i % tick_mult) % mem_tick) == 0) { // When the CPU is ticked cpu_tick times,
                                                 // the memory controller should be ticked mem_tick times
            proc.tick();
            Stats::curTick++; // processor clock, global, for Statistics

            if (configs.calc_weighted_speedup()) {
                if (proc.has_reached_limit()) {
                    break;
                }
            } else {
                if (configs.is_early_exit()) {
                    if (proc.finished())
                    break;
                } else {
                if (proc.finished() && (memory.pending_requests() == 0))
                    break;
                }
            }
        }

        if (((i % tick_mult) % cpu_tick) == 0) // TODO_hasan: Better if the processor ticks the memory controller
            memory.tick();

    }
    // This a workaround for statistics set only initially lost in the end
    memory.finish();
    Stats::statlist.printall();
}

template<typename T>
void start_run(const Config& configs, T* spec, const vector<const char*>& files) {
  // @rajat: This logic here needs to be changed for the Near Memory Hub processing
  // Need to first read the UXI switch configuration from the config file and instantiate the UXI Switch objects
  // There could be multiple Master UXI switch and multiple slave UXI switch
  // Attached to each slave UXI switch could be multiple memory channels
  // Attached to each memory channel could be multiple DIMMs

  int num_master_uxi_switch = configs.get_master_uxi_switches();
  int num_slave_uxi_switch_per_master = configs.get_slave_uxi_switches_per_master();
  
  spec->set_master_uxi_switch_number(num_master_uxi_switch);
  spec->set_slave_uxi_switch_number(num_slave_uxi_switch_per_master);
  
  // initiate controller and memory
  int C = configs.get_channels(); // @rajat: This now becomes # of channels per slave UXI switch
  int R = configs.get_ranks();
  // Check and Set channel, rank number
  spec->set_channel_number(C);
  spec->set_rank_number(R);

  std::vector<MasterUXISwitch<T>*> master_uxi_switches;
  for( int ms = 0; ms < num_master_uxi_switch; ms++ ) {
      MasterUXISwitch<T>* msObj = new MasterUXISwitch<T>((ComputeUnitId_t)ms);

      for( int slv_switch = 0; slv_switch < num_slave_uxi_switch_per_master; slv_switch++) {
          SlaveUXISwitch<T>* slvObj = new SlaveUXISwitch<T>((ComputeUnitId_t)slv_switch);

      for(int c = 0; c < C; c++) {
          DRAM<T>* channel = new DRAM<T>(spec, T::Level::Channel);
          channel->id = c;
          channel->regStats("");
          Controller<T>* ctrl = new Controller<T>(configs, channel);
          slvObj->memoryControllers.push_back(ctrl);
      }
      msObj->slaveSwitches.push_back(slvObj);
    }
    master_uxi_switches.push_back(msObj);
  }

  MemoryHub<T> memoryHub(configs, master_uxi_switches);

  assert(files.size() != 0);
  if (configs["trace_type"] == "CPU") {
    //run_cputrace(configs, memory, files); // @rajat: disabled cputrace for memoryHub, later enable it
  } else if (configs["trace_type"] == "DRAM") {
    run_dramtrace(configs, memoryHub, files[0]);
  }

}

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <configs-file> --mode=cpu,dram [--stats <filename>] <trace-filename1> <trace-filename2>\n"
            "Example: %s ramulator-configs.cfg --mode=cpu cpu.trace cpu.trace\n", argv[0], argv[0]);
        return 0;
    }

    Config configs(argv[1]);

    const std::string& standard = configs["standard"];
    assert(standard != "" || "DRAM standard should be specified.");

    const char *trace_type = strstr(argv[2], "=");
    trace_type++;
    if (strcmp(trace_type, "cpu") == 0) {
      configs.add("trace_type", "CPU");
    } else if (strcmp(trace_type, "dram") == 0) {
      configs.add("trace_type", "DRAM");
    } else {
      printf("invalid trace type: %s\n", trace_type);
      assert(false);
    }

    int trace_start = 3;
    string stats_out;
    if (strcmp(argv[trace_start], "--stats") == 0) {
      Stats::statlist.output(argv[trace_start+1]);
      stats_out = argv[trace_start+1];
      trace_start += 2;
    } else {
      Stats::statlist.output(standard+".stats");
      stats_out = standard + string(".stats");
    }

    // A separate file defines mapping for easy config.
    if (strcmp(argv[trace_start], "--mapping") == 0) {
      configs.add("mapping", argv[trace_start+1]);
      trace_start += 2;
    } else {
      configs.add("mapping", "defaultmapping");
    }

    std::vector<const char*> files(&argv[trace_start], &argv[argc]);
    configs.set_core_num(argc - trace_start);
    DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
    start_run(configs, ddr4, files);

    printf("Simulation done. Statistics written to %s\n", stats_out.c_str());

    return 0;
}
