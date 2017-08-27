/**
 * This file is part of SimpleSSD.
 *
 * SimpleSSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSSD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleSSD.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Jie Zhang <jie@camelab.org>
 */

#ifndef __PALStatistics_h__
#define __PALStatistics_h__

#include "SimpleSSD_types.h"

#include "base_config.hh"
#include "Latency.h"
#include "PAL2_TimeSlot.h"
#include "ftl.hh"

#include "base/types.hh"
#include "base/trace.hh"

#ifndef SIMPLESSD_STANDALONE
#include "debug/PAL.hh"
#endif

#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <map>
using namespace std;

#define OPER_ALL (OPER_NUM+1)
#define PAGE_ALL (PAGE_NUM+1)

class PALStatistics
{
  public:
    enum
    {
      /*
      BUSY_DMA0WAIT = 0,
      BUSY_DMA0 = 1,
      BUSY_MEM  = 2,
      BUSY_DMA1WAIT = 3,
      BUSY_DMA1 = 4,
      BUSY_END = 5
      */
      TICK_DMA0_CHANNEL_CONFLICT = TICK_NUM,
      TICK_DMA0_PLANE_CONFLICT,
      TICK_DMA1_CONFLICT,
      TICK_DMA0_SUSPEND,
      TICK_DMA1_SUSPEND,
      TICK_PROC,
      TICK_FULL,
      TICK_STAT_NUM
    };

    BaseConfig *gconf;
    Latency *lat;

    #if 0 //ch-die io count (legacy)
    class mini_cnt_page
    {
        public:
        mini_cnt_page();
        uint64_t c[OPER_NUM][PAGE_NUM];
    };
    mini_cnt_page* ch_io_cnt;
    mini_cnt_page* die_io_cnt;
    #endif

    #if GATHER_RESOURCE_CONFLICT && 0 //conflict gather (legacy)
    class mini_cnt_conflict
    {
        public:
        mini_cnt_conflict();
        uint64_t c[OPER_NUM][CONFLICT_NUM];
    };
    mini_cnt_conflict* ch_conflict_cnt;
    mini_cnt_conflict* ch_conflict_length;
    mini_cnt_conflict* die_conflict_cnt;
    mini_cnt_conflict* die_conflict_length;
    #endif

    #if 0 //latency (legacy method)
    uint64_t latency_cnt[OPER_NUM][TICK_STAT_NUM]; // RWE count for: (page_size * cnt)/SIM_TIME
    uint64_t latency_sum[OPER_NUM][TICK_STAT_NUM];
    #endif

    #if 0 //util time (legacy method)
    uint64_t* channel_util_time_sum;
    uint64_t* die_util_time_sum_mem;   //mem only-mem occupy
    uint64_t* die_util_time_sum_optimum; //mem oper occupy optimum (DMA0+MEM+DMA1)
    uint64_t* die_util_time_sum_all;  //mem oper actual occupy
    #endif

    #if 0 //stall
    /*  StallInfo:
           Actually, only use 5 entries --- 1a) TICK_DMA0_CHANNEL_CONFLICT, 1b) TICK_DMA0_PLANE_CONFLICT,
                                            2) TICK_DMA1WAIT, 3) TICK_DMA0_SUSPEND, 4) TICK_DMA1_SUSPEND  */
    uint64_t stall_sum[OPER_NUM][TICK_STAT_NUM];
    uint64_t stall_cnt[OPER_NUM][TICK_STAT_NUM];
    uint8_t LastDMA0Stall;
    uint64_t LastDMA0AttemptTick;

    void CountStall(uint8_t oper, uint8_t stall_kind);
    void AddStall(uint8_t oper, uint8_t stall_kind, uint64_t lat);
    #endif

    PALStatistics(BaseConfig *, Latency *);

    ~PALStatistics();

    /*
    void AddLatency(uint32_t lat, uint8_t oper, uint8_t busyfor);
    */

    void AddLatency(Task* task);

    /*
    void AddOccupy(uint32_t ch, uint64_t ch_time, uint64_t pl, uint64_t pl_time, uint64_t pl2_time);
    */

    uint64_t sim_start_time_ps;
    uint64_t LastTick;
    void UpdateLastTick(uint64_t tick);
    uint64_t GetLastTick();
    #if GATHER_RESOURCE_CONFLICT
    void AddLatency(Command& CMD, CPDPBP* CPD, uint32_t dieIdx, TimeSlot* DMA0, TimeSlot* MEM, TimeSlot* DMA1, uint8_t confType, uint64_t confLength );
    #else
    void AddLatency(Command& CMD, CPDPBP* CPD, uint32_t dieIdx, TimeSlot* DMA0, TimeSlot* MEM, TimeSlot* DMA1);
    #endif
    void MergeSnapshot();
    uint64_t ExactBusyTime, SampledExactBusyTime;
    uint64_t OpBusyTime[3], LastOpBusyTime[3]; // 0: Read, 1: Write, 2: Erase;
    uint64_t LastExactBusyTime;
    uint64_t LastExecutionTime;

    void PrintStats(uint64_t sim_time_ps);
    void ResetStats();
    void PrintFinalStats(uint64_t sim_time_ps);

    ///////////////////////////////// polished stats
    class Counter
    {
      public:
        Counter();
        void init();
        void add();
        uint64_t cnt;
    };

    class CounterOper
    {
      public:
        CounterOper();
        Counter cnts[OPER_ALL];
        void init();
        void add(uint32_t oper);
        void printstat(const char* namestr);
    };

    CounterOper  PPN_requested_rwe;
    CounterOper  PPN_requested_pagetype[PAGE_ALL];
    CounterOper* PPN_requested_ch; //channels
    CounterOper* PPN_requested_die; //dies
    CounterOper  CF_DMA0_dma;
    CounterOper  CF_DMA0_mem;
    CounterOper  CF_DMA0_none;
    CounterOper  CF_DMA1_dma;
    CounterOper  CF_DMA1_none;

    class Value
    {
      public:
        Value();
        void init();
        void add(double val);
        void backup();
        void update();
        double avg();
        double legacy_avg();
        double sum, minval, maxval, cnt, sampled_sum, sampled_cnt, legacy_sum, legacy_cnt, legacy_minval, legacy_maxval;
    };

    class ValueOper
    {
      public:
        ValueOper();
        ValueOper(ValueOper *_ValueOper);
        Value vals[OPER_ALL];
        void init();
        void update();
        void add(uint32_t oper, double val);
        void exclusive_add(uint32_t oper, double val);
        void printstat(const char* namestr);
        void printstat_bandwidth(class ValueOper*, uint64_t , uint64_t ); //bandwidth excluding idle time
        void printstat_bandwidth_widle(class ValueOper*, uint64_t, uint64_t); //bandwidth including idle time
        void printstat_oper_bandwidth(class ValueOper*, uint64_t*, uint64_t*); // read/write/erase-only bandwidth
        void printstat_latency(const char* namestr);
        void printstat_iops(class ValueOper*, uint64_t , uint64_t );
        void printstat_iops_widle(class ValueOper*, uint64_t , uint64_t );
        void printstat_oper_iops(class ValueOper*, uint64_t* , uint64_t* );
    };

    ValueOper  Ticks_DMA0WAIT;
    ValueOper  Ticks_DMA0;
    ValueOper  Ticks_MEM;
    ValueOper  Ticks_DMA1WAIT;
    ValueOper  Ticks_DMA1;
    ValueOper  Ticks_Total; // Total = D0W+D0+M+D1W+D1
    std::map<uint64_t, ValueOper*> Ticks_Total_snapshot;
    ValueOper  Ticks_TotalOpti; //TotalOpti = D0+M+D1 --- exclude WAIT
    ValueOper* Ticks_Active_ch; //channels
    ValueOper* Ticks_Active_die; //dies
    ValueOper  Access_Capacity;
    std::map<uint64_t, ValueOper*> Access_Capacity_snapshot;
    ValueOper  Access_Bandwidth;
    ValueOper  Access_Bandwidth_widle;
    ValueOper  Access_Oper_Bandwidth;
    ValueOper  Access_Iops;
    ValueOper  Access_Iops_widle;
    ValueOper  Access_Oper_Iops;
    uint64_t SampledTick;
    bool skip;


    /////////////////////////////////


  private:
    void ClearStats();
    void InitStats();
};

#endif //__PALStatistics_h__
