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

#include "PALStatistics.h"
#include "SimpleSSD_types.h"

#ifndef SIMPLESSD_STANDALONE
#include "debug/PAL.hh"
#endif

// DONT USE FUCKING GLOBAL VARIABLES IN BIIIIIIG PROGRAM
char ADDR_STRINFO[ADDR_NUM][10]= { "Channel", "Package", "Die", "Plane", "Block", "Page" };
char ADDR_STRINFO2[ADDR_NUM][15]= { "ADDR_CHANNEL", "ADDR_PACKAGE", "ADDR_DIE", "ADDR_PLANE", "ADDR_BLOCK", "ADDR_PAGE" };
char OPER_STRINFO[OPER_NUM][10]= {"R","W","E"};
char OPER_STRINFO2[OPER_NUM][10]= {"Read ","Write","Erase"};
char BUSY_STRINFO[BUSY_NUM][10]= {"IDLE","DMA0","MEM","DMA1WAIT","DMA1","END"};
char PAGE_STRINFO[PAGE_NUM][10]= {"LSB","CSB","MSB"};
char NAND_STRINFO[NAND_NUM][10]= {"SLC", "MLC", "TLC"};
#if GATHER_RESOURCE_CONFLICT
char CONFLICT_STRINFO[CONFLICT_NUM][10]= {"NONE", "DMA0", "MEM", "DMA1"};
#endif

#if 1// Polished stats - Improved instrumentation
PALStatistics::Counter::Counter() { init(); }

void PALStatistics::Counter::init()
{
  cnt = 0;
}

void PALStatistics::Counter::add()
{
  cnt++;
}

PALStatistics::CounterOper::CounterOper() { init(); }

void PALStatistics::CounterOper::init()
{
  for (int i=0; i<OPER_ALL; i++) cnts[i].init();
}

void PALStatistics::CounterOper::add(uint32_t oper)
{
  cnts[oper].add();
  cnts[OPER_NUM].add();
}

void PALStatistics::CounterOper::printstat(const char* namestr)
{
  char OPER_STR[OPER_ALL][8] = {"Read ", "Write", "Erase", "Total"};
  DPRINTF(PAL, "[ %s ]:\n", namestr);
  DPRINTF(PAL, "OPER, COUNT\n");
  for (int i=0;i<OPER_ALL;i++)
  {
    DPRINTF(PAL, "%s, %" PRIu64 "\n",
    OPER_STR[i],
    cnts[i].cnt);
  }
}



PALStatistics::Value::Value() { init(); }

void PALStatistics::Value::init()
{
  sum = 0;
  cnt = 0;
  sampled_sum = 0;
  sampled_cnt = 0;
  minval = MAX64;
  maxval = 0;
  legacy_sum = 0;
  legacy_cnt = 0;
  legacy_minval = MAX64;
  legacy_maxval = 0;
}
void PALStatistics::Value::backup(){
  sampled_sum = sum;
  sampled_cnt = cnt;
}

void PALStatistics::Value::update(){
  legacy_sum = sum;
  legacy_cnt = cnt;
  legacy_minval = minval;
  legacy_maxval = maxval;
}

#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define MAX(x, y) ((x) < (y) ? (y) : (x))

void PALStatistics::Value::add(double val)
{
  sum += val;
  cnt += 1;
  minval = MIN(val, minval);
  maxval = MAX(val, maxval);
}

double PALStatistics::Value::avg()
{
  return SAFEDIV(sum, cnt);
}

double PALStatistics::Value::legacy_avg()
{
  return SAFEDIV(legacy_sum, legacy_cnt);
}

PALStatistics::ValueOper::ValueOper() { init(); }
PALStatistics::ValueOper::ValueOper(ValueOper *_ValueOper){
  for (int i=0; i<OPER_ALL; i++){
    vals[i].sum = _ValueOper->vals[i].sum;
    vals[i].cnt = _ValueOper->vals[i].cnt;
    vals[i].sampled_sum = _ValueOper->vals[i].sampled_sum;
    vals[i].sampled_cnt = _ValueOper->vals[i].sampled_cnt;
    vals[i].minval = _ValueOper->vals[i].minval;
    vals[i].maxval = _ValueOper->vals[i].maxval;
    vals[i].legacy_sum = _ValueOper->vals[i].legacy_sum;
    vals[i].legacy_cnt = _ValueOper->vals[i].legacy_cnt;
    vals[i].legacy_minval = _ValueOper->vals[i].legacy_minval;
    vals[i].legacy_maxval = _ValueOper->vals[i].legacy_maxval;
  }
}

void PALStatistics::ValueOper::init()
{
  for (int i=0; i<OPER_ALL; i++) vals[i].init();
}

void PALStatistics::ValueOper::update()
{
  for (int i=0; i<OPER_ALL; i++) vals[i].update();
}

void PALStatistics::ValueOper::add(uint32_t oper, double val)
{
  vals[oper].add(val);
  vals[OPER_NUM].add(val);
}

void PALStatistics::ValueOper::exclusive_add(uint32_t oper, double val){
  vals[oper].add(val);
}

void PALStatistics::ValueOper::printstat(const char* namestr) //This is only used by access capacity
{
  char OPER_STR[OPER_ALL][8] = {"Read ", "Write", "Erase", "Total"};
  DPRINTF(PAL, "[ %s ]:\n", namestr);
  DPRINTF(PAL, "OPER, AVERAGE, COUNT, TOTAL, MIN, MAX\n");
  for (int i=0;i<OPER_ALL;i++)
  {
    if(vals[i].cnt == 0)
    {
      DPRINTF(PAL, "%s, ( NO DATA )\n", OPER_STR[i]);
    }
    else
    {
      DPRINTF(PAL, "%s, %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 "\n",
      OPER_STR[i],
      (uint64_t)vals[i].avg(),
      (uint64_t)vals[i].cnt, (uint64_t)vals[i].sum,
      (uint64_t)vals[i].minval, (uint64_t)vals[i].maxval);
    }
  }
}

void PALStatistics::ValueOper::printstat_bandwidth(class ValueOper* Access_Capacity, uint64_t ExactBusyTime, uint64_t LastExactBusyTime)
{
  char OPER_STR[OPER_ALL][8] = {"Read", "Write", "Erase", "Total"};
  for (int i=0;i<OPER_ALL;i++)
  {
    if (ExactBusyTime > LastExactBusyTime){
      //printf("sum=%f\tsampled_sum=%f\n",Access_Capacity->vals[i].sum,Access_Capacity->vals[i].sampled_sum);
      this->exclusive_add(i,(Access_Capacity->vals[i].sum - Access_Capacity->vals[i].sampled_sum)*1.0/MBYTE/((ExactBusyTime - LastExactBusyTime)*1.0/PSEC));
    }
    if(vals[i].cnt == 0)
    {
      DPRINTF(PAL, "%s bandwidth excluding idle time (min, max, average): ( NO DATA )\n", OPER_STR[i]);
    }
    else
    {
      DPRINTF(PAL, "%s bandwidth excluding idle time (min, max, average): %.6lf MB/s, %.6lf MB/s, %.6lf MB/s\n",
      OPER_STR[i],
      vals[i].minval,
      vals[i].maxval,
      (Access_Capacity->vals[i].sum)*1.0/MBYTE/((ExactBusyTime)*1.0/PSEC));
    }
  }
}

void PALStatistics::ValueOper::printstat_bandwidth_widle(class ValueOper* Access_Capacity, uint64_t ExecutionTime, uint64_t LastExecutionTime)
{
  char OPER_STR[OPER_ALL][8] = {"Read", "Write", "Erase", "Total"};
  for (int i=0;i<OPER_ALL;i++)
  {
    assert(ExecutionTime > LastExecutionTime);
    this->exclusive_add(i,(Access_Capacity->vals[i].sum - Access_Capacity->vals[i].sampled_sum)*1.0/MBYTE/((ExecutionTime - LastExecutionTime)*1.0/PSEC));

    if(vals[i].cnt == 0)
    {
      DPRINTF(PAL, "%s bandwidth including idle time (min, max, average): ( NO DATA )\n", OPER_STR[i]);
    }
    else
    {
      DPRINTF(PAL, "%s bandwidth including idle time (min, max, average): %.6lf MB/s, %.6lf MB/s, %.6lf MB/s\n",
      OPER_STR[i],
      vals[i].minval,
      vals[i].maxval,
      (Access_Capacity->vals[i].sum)*1.0/MBYTE/((ExecutionTime)*1.0/PSEC));
    }
  }
}

void PALStatistics::ValueOper::printstat_oper_bandwidth(class ValueOper* Access_Capacity, uint64_t* OpBusyTime, uint64_t* LastOpBusyTime)
{
  char OPER_STR[OPER_ALL][8] = {"Read", "Write", "Erase", "Total"};
  for (int i=0;i<3;i++) //only read/write/erase
  {
    if (OpBusyTime[i] > LastOpBusyTime[i])
    this->exclusive_add(i,(Access_Capacity->vals[i].sum - Access_Capacity->vals[i].sampled_sum)*1.0/MBYTE/((OpBusyTime[i] - LastOpBusyTime[i])*1.0/PSEC));

    if(vals[i].cnt == 0)
    {
      DPRINTF(PAL, "%s-only bandwidth (min, max, average): ( NO DATA )\n", OPER_STR[i]);
    }
    else
    {
      DPRINTF(PAL, "%s-only bandwidth (min, max, average): %.6lf MB/s, %.6lf MB/s, %.6lf MB/s\n",
      OPER_STR[i],
      vals[i].minval,
      vals[i].maxval,
      (Access_Capacity->vals[i].sum)*1.0/MBYTE/((OpBusyTime[i])*1.0/PSEC));
    }
  }
}

void PALStatistics::ValueOper::printstat_iops(class ValueOper* Access_Capacity, uint64_t ExactBusyTime, uint64_t LastExactBusyTime)
{
  char OPER_STR[OPER_ALL][8] = {"Read", "Write", "Erase", "Total"};
  for (int i=0;i<OPER_ALL;i++)
  {
    if (ExactBusyTime > LastExactBusyTime)
    this->exclusive_add(i,(Access_Capacity->vals[i].cnt - Access_Capacity->vals[i].sampled_cnt)*1.0/((ExactBusyTime - LastExactBusyTime)*1.0/PSEC));
    if(vals[i].cnt == 0)
    {
      DPRINTF(PAL, "%s IOPS excluding idle time (min, max, average): ( NO DATA )\n", OPER_STR[i]);
    }
    else
    {
      DPRINTF(PAL, "%s IOPS excluding idle time (min, max, average): %.6lf, %.6lf, %.6lf\n",
      OPER_STR[i],
      vals[i].minval,
      vals[i].maxval,
      (Access_Capacity->vals[i].cnt)*1.0/((ExactBusyTime)*1.0/PSEC));
    }
  }
}

void PALStatistics::ValueOper::printstat_iops_widle(class ValueOper* Access_Capacity, uint64_t ExecutionTime, uint64_t LastExecutionTime)
{
  char OPER_STR[OPER_ALL][8] = {"Read", "Write", "Erase", "Total"};
  for (int i=0;i<OPER_ALL;i++)
  {

    this->exclusive_add(i,(Access_Capacity->vals[i].cnt - Access_Capacity->vals[i].sampled_cnt)*1.0/((ExecutionTime - LastExecutionTime)*1.0/PSEC));
    if(vals[i].cnt == 0)
    {
      DPRINTF(PAL, "%s IOPS including idle time (min, max, average): ( NO DATA )\n", OPER_STR[i]);
    }
    else
    {
      DPRINTF(PAL, "%s IOPS including idle time (min, max, average): %.6lf, %.6lf, %.6lf\n",
      OPER_STR[i],
      vals[i].minval,
      vals[i].maxval,
      (Access_Capacity->vals[i].cnt)*1.0/((ExecutionTime)*1.0/PSEC));
    }
  }
}

void PALStatistics::ValueOper::printstat_oper_iops(class ValueOper* Access_Capacity, uint64_t* OpBusyTime, uint64_t* LastOpBusyTime)
{
  char OPER_STR[OPER_ALL][8] = {"Read", "Write", "Erase", "Total"};
  for (int i=0;i<3;i++) //only read/write/erase
  {
    if (OpBusyTime[i] > LastOpBusyTime[i])
    this->exclusive_add(i,(Access_Capacity->vals[i].cnt - Access_Capacity->vals[i].sampled_cnt)*1.0/((OpBusyTime[i] - LastOpBusyTime[i])*1.0/PSEC));
    if(vals[i].cnt == 0)
    {
      DPRINTF(PAL, "%s-only IOPS (min, max, average): ( NO DATA )\n", OPER_STR[i]);
    }
    else
    {
      DPRINTF(PAL, "%s-only IOPS (min, max, average): %.6lf, %.6lf, %.6lf\n",
      OPER_STR[i],
      vals[i].minval,
      vals[i].maxval,
      (Access_Capacity->vals[i].cnt)*1.0/((OpBusyTime[i])*1.0/PSEC));
    }
  }
}

void PALStatistics::ValueOper::printstat_latency(const char* namestr)
{
  char OPER_STR[OPER_ALL][8] = {"Read", "Write", "Erase", "Total"};
  for (int i=0;i<OPER_ALL;i++)
  {
    if(vals[i].cnt == 0)
    {
      DPRINTF(PAL, "%s latency (min, max, average): ( NO DATA )\n", OPER_STR[i]);
    }
    else
    {
      DPRINTF(PAL, "%s latency (min, max, average): %" PRIu64 " us, %" PRIu64 " us, %" PRIu64 " us\n",
      OPER_STR[i],
      (uint64_t)(vals[i].minval*1.0/1000000),
      (uint64_t)(vals[i].maxval*1.0/1000000),
      (uint64_t)(vals[i].avg()*1.0/1000000));
    }
  }
}
#endif // Polished stats


PALStatistics::PALStatistics(BaseConfig *c, Latency *l) : gconf(c), lat(l)
{
  LastTick = 0;

  InitStats();

  SampledTick = 0;
  skip = true;
}

PALStatistics::~PALStatistics()
{
  PrintStats( curTick() );
  ClearStats();
}

void PALStatistics::ResetStats()
{
  ClearStats();
  InitStats();
}


void PALStatistics::InitStats()
{
  sim_start_time_ps = curTick();



  ExactBusyTime = 0;
  LastExactBusyTime = 0;
  LastExecutionTime = 0;
  OpBusyTime[0] =  OpBusyTime[1] = OpBusyTime[2] = 0;
  LastOpBusyTime[0] = LastOpBusyTime[1] =LastOpBusyTime[2] = 0;


  #if 1 // Polished stats - Improved instrumentation
  PPN_requested_ch  = new CounterOper[gconf->NumChannel];
  PPN_requested_die = new CounterOper[gconf->GetTotalNumDie()];
  Ticks_Active_ch   = new ValueOper[gconf->NumChannel];
  Ticks_Active_die  = new ValueOper[gconf->GetTotalNumDie()];

  PPN_requested_rwe.init();
  for (int j=0;j<PAGE_ALL;j++) { PPN_requested_pagetype[j].init(); };
  //PPN_requested_ch
  //PPN_requested_die
  CF_DMA0_dma.init();
  CF_DMA0_mem.init();
  CF_DMA0_none.init();
  CF_DMA1_dma.init();
  CF_DMA1_none.init();

  Ticks_DMA0WAIT.init();
  Ticks_DMA0.init();
  Ticks_MEM.init();
  Ticks_DMA1WAIT.init();
  Ticks_DMA1.init();
  Ticks_Total.init();
  Ticks_TotalOpti.init();
  //Ticks_Active_ch
  //Ticks_Active_die
  Access_Capacity.init();
  Access_Bandwidth.init();
  Access_Bandwidth_widle.init();
  Access_Oper_Bandwidth.init();
  Access_Iops.init();
  Access_Iops_widle.init();
  Access_Oper_Iops.init();
  #endif // Polished stats
}

void PALStatistics::ClearStats()
{

  #if 1 // Polished stats - Improved instrumentation
  delete PPN_requested_ch;
  delete PPN_requested_die;
  delete Ticks_Active_ch;
  delete Ticks_Active_die;
  #endif // Polished stats
}



void PALStatistics::UpdateLastTick(uint64_t tick)
{
  if (LastTick<tick) LastTick=tick;
}

uint64_t PALStatistics::GetLastTick()
{
  return LastTick;
}

void PALStatistics::MergeSnapshot()
{
  if (Ticks_Total_snapshot.size() != 0){
    std::map<uint64_t, ValueOper*>::iterator e  = Ticks_Total_snapshot.end();
    e--;
    for (std::map<uint64_t, ValueOper*>::iterator f = Ticks_Total_snapshot.begin(); f != e;){
      delete f->second;
      Ticks_Total_snapshot.erase(f++);
    }

  }
  if (Access_Capacity_snapshot.size() != 0){
    std::map<uint64_t, ValueOper*>::iterator e  = Access_Capacity_snapshot.end();
    e--;
    for (std::map<uint64_t, ValueOper*>::iterator f = Access_Capacity_snapshot.begin(); f != e;){
      delete f->second;
      Access_Capacity_snapshot.erase(f++);
    }
  }
}

#if GATHER_RESOURCE_CONFLICT
void PALStatistics::AddLatency(Command& CMD, CPDPBP* CPD, uint32_t dieIdx, TimeSlot* DMA0, TimeSlot* MEM, TimeSlot* DMA1, uint8_t confType, uint64_t confLength)
#else
void PALStatistics::AddLatency(Command& CMD, CPDPBP* CPD, uint32_t dieIdx, TimeSlot* DMA0, TimeSlot* MEM, TimeSlot* DMA1)
#endif
{
  uint32_t oper = CMD.operation;
  uint32_t chIdx = CPD->Channel;
  uint64_t time_all[TICK_STAT_NUM];
  uint8_t pageType = lat->GetPageType(CPD->Page);
  memset(time_all,0,sizeof(time_all));

  /*
  TICK_IOREQUESTED, CMD.arrived_time
  TICK_DMA0WAIT, let it 0
  TICK_DMA0, DMA0->StartTick
  TICK_MEM,  MEM->StartTick
  TICK_DMA1WAIT,
  TICK_DMA1, DMA1->StartTick
  TICK_IOEND, DMA1->TickEnd
  */

  time_all[TICK_DMA0WAIT]     = DMA0->StartTick - CMD.arrived;                     // FETCH_WAIT --> when DMA0 couldn't start immediatly
  time_all[TICK_DMA0]         = lat->GetLatency(CPD->Page, CMD.operation, BUSY_DMA0);
  time_all[TICK_DMA0_SUSPEND] = 0;                                                       //no suspend in new design
  time_all[TICK_MEM]          = lat->GetLatency(CPD->Page, CMD.operation, BUSY_MEM);
  time_all[TICK_DMA1WAIT]     = ( MEM->EndTick - MEM->StartTick + 1 )
  - (  lat->GetLatency(CPD->Page, CMD.operation, BUSY_DMA0)
  + lat->GetLatency(CPD->Page, CMD.operation, BUSY_MEM)
  + lat->GetLatency(CPD->Page, CMD.operation, BUSY_DMA1) ); // --> when DMA1 didn't start immediatly.
  time_all[TICK_DMA1]         = lat->GetLatency(CPD->Page, CMD.operation, BUSY_DMA1);
  time_all[TICK_DMA1_SUSPEND] = 0;                                                       //no suspend in new design
  time_all[TICK_FULL]         = DMA1->EndTick - CMD.arrived +1;  //D0W+D0+M+D1W+D1 full latency
  time_all[TICK_PROC]         = time_all[TICK_DMA0] + time_all[TICK_MEM] + time_all[TICK_DMA1];  // OPTIMUM_TIME

  #if 1 // Polished stats - Improved instrumentation
  PPN_requested_rwe.add(oper);
  PPN_requested_pagetype[pageType].add(oper);
  PPN_requested_ch[chIdx].add(oper);  //PPN_requested_ch[ch#]
  PPN_requested_die[dieIdx].add(oper); //PPN_requested_die[die#]

  if (confType & CONFLICT_DMA0)
  CF_DMA0_dma.add(oper);
  if (confType & CONFLICT_MEM)
  CF_DMA0_mem.add(oper);
  if ( !(confType & (CONFLICT_DMA0|CONFLICT_MEM)) )
  CF_DMA0_none.add(oper);

  if ( confType & CONFLICT_DMA1 )
  CF_DMA1_dma.add(oper);
  if ( !(confType & CONFLICT_DMA1) )
  CF_DMA1_none.add(oper);

  Ticks_DMA0WAIT.add(oper, time_all[TICK_DMA0WAIT]);
  Ticks_DMA0.add(oper, time_all[TICK_DMA0]);
  Ticks_MEM.add(oper, time_all[TICK_MEM]);
  Ticks_DMA1WAIT.add(oper, time_all[TICK_DMA1WAIT]);
  Ticks_DMA1.add(oper, time_all[TICK_DMA1]);
  Ticks_Total.add(oper, time_all[TICK_FULL]);
  //***********************************************
  uint64_t finished_time = CMD.finished;
  uint64_t update_point = finished_time / EPOCH_INTERVAL;
  std::map<uint64_t, ValueOper*>::iterator e  = Ticks_Total_snapshot.find(update_point);
  if (e != Ticks_Total_snapshot.end()) e->second->add(oper, time_all[TICK_FULL]);
  else{

    e = Ticks_Total_snapshot.upper_bound(update_point);
    if (e != Ticks_Total_snapshot.end()){
      if (e != Ticks_Total_snapshot.begin()){
        e--;
        Ticks_Total_snapshot[update_point] = new ValueOper(e->second);
      }
      else{
        Ticks_Total_snapshot[update_point] = new ValueOper();
      }

    }
    else{
      if (Ticks_Total_snapshot.size() == 0)Ticks_Total_snapshot[update_point] = new ValueOper();
      else {
        e--;
        Ticks_Total_snapshot[update_point] = new ValueOper(e->second);
      }
    }
    Ticks_Total_snapshot[update_point]->add(oper, time_all[TICK_FULL]);
  }

  e  = Ticks_Total_snapshot.upper_bound(update_point);
  while (e != Ticks_Total_snapshot.end()){
    e->second->add(oper, time_all[TICK_FULL]);

    e  = Ticks_Total_snapshot.upper_bound(e->first);
  }
  //***********************************************
  Ticks_TotalOpti.add(oper, time_all[TICK_PROC]);
  Ticks_Active_ch[chIdx].add( oper,time_all[TICK_DMA0]+time_all[TICK_DMA1] );
  Ticks_Active_die[dieIdx].add(oper, (time_all[TICK_DMA0] + time_all[TICK_MEM] + time_all[TICK_DMA1WAIT] + time_all[TICK_DMA1]) );
  if (oper == OPER_ERASE) Access_Capacity.add(oper, gconf->SizePage*gconf->NumPage); // ERASE
  else                    Access_Capacity.add(oper, gconf->SizePage);                // READ,WRITE
  //************************************************
  update_point = finished_time / EPOCH_INTERVAL;
  std::map<uint64_t, ValueOper*>::iterator f  = Access_Capacity_snapshot.find(update_point);
  if (f != Access_Capacity_snapshot.end()){
    if (oper == OPER_ERASE) f->second->add(oper, gconf->SizePage*gconf->NumPage); // ERASE
    else                    f->second->add(oper, gconf->SizePage);                // READ,WRITE
  }
  else{
    f = Access_Capacity_snapshot.upper_bound(update_point);
    if (f != Access_Capacity_snapshot.end()){
      if (f != Access_Capacity_snapshot.begin()){
        f--;
        Access_Capacity_snapshot[update_point] = new ValueOper(f->second);
      }
      else{
        Access_Capacity_snapshot[update_point] = new ValueOper();
      }

    }
    else{
      if (Access_Capacity_snapshot.size() == 0)Access_Capacity_snapshot[update_point] = new ValueOper();
      else {
        f--;
        Access_Capacity_snapshot[update_point] = new ValueOper(f->second);
      }
    }

    if (oper == OPER_ERASE) Access_Capacity_snapshot[update_point]->add(oper, gconf->SizePage*gconf->NumPage); // ERASE
    else                    Access_Capacity_snapshot[update_point]->add(oper, gconf->SizePage);                // READ,WRITE
  }
  f  = Access_Capacity_snapshot.upper_bound(update_point);
  while (f != Access_Capacity_snapshot.end()){
    if (oper == OPER_ERASE) f->second->add(oper, gconf->SizePage*gconf->NumPage); // ERASE
    else                    f->second->add(oper, gconf->SizePage);                // READ,WRITE
    f  = Access_Capacity_snapshot.upper_bound(f->first);
  }
  //************************************************
  #endif  // Polished stats

}


#define fDPRINTF(out_to, fmt, ...) do { char buf[1024]; sprintf(buf, fmt, ##__VA_ARGS__); DPRINTF(out_to,"%s",buf); } while(0);

void PALStatistics::PrintFinalStats(uint64_t sim_time_ps){
  DPRINTF(PAL, "PAL Final Stats Report ]\n");
  DPRINTF(PAL, "Total execution time (ms), Total SSD active time (ms)\n");
  DPRINTF(PAL, "%.2f\t\t\t, %.2f\n", sim_time_ps * 1.0 / 1000000000, SampledExactBusyTime * 1.0 / 1000000000);

  assert(Access_Capacity_snapshot.size() > 0);
  std::map<uint64_t, ValueOper*>::iterator e  = Access_Capacity_snapshot.end();
  e--;

  e->second->printstat("Info of Access Capacity");
  Access_Bandwidth.printstat_bandwidth(e->second, SampledExactBusyTime, LastExactBusyTime);
  Access_Bandwidth_widle.printstat_bandwidth_widle(e->second, sim_time_ps, LastExecutionTime);
  Access_Oper_Bandwidth.printstat_oper_bandwidth(e->second, OpBusyTime, LastOpBusyTime);


  assert(Ticks_Total_snapshot.size() > 0);
  std::map<uint64_t, ValueOper*>::iterator f  = Ticks_Total_snapshot.end();
  f--;

  f->second->printstat_latency("Info of Latency");
  Access_Iops.printstat_iops(e->second, SampledExactBusyTime, LastExactBusyTime);
  Access_Iops_widle.printstat_iops_widle(e->second, sim_time_ps, LastExecutionTime);
  Access_Oper_Iops.printstat_oper_iops(e->second, OpBusyTime, LastOpBusyTime);
  DPRINTF(PAL, "===================\n");
  PPN_requested_rwe.printstat("Num of PPN IO request");
  DPRINTF(PAL, "===================\n");

  for(uint32_t i=0; i<PAGE_NUM; i++)
  {
    char str[256];
    sprintf(str, "Num of %s page PPN IO request", PAGE_STRINFO[i]);
    PPN_requested_pagetype[i].printstat(str);
  }
  printf( "===================\n");

  for (uint32_t i=0;i<gconf->NumChannel; i++)
  {
    char str[256];
    sprintf(str, "Num of CH_%u PPN IO request", i);
    PPN_requested_ch[i].printstat(str);
  }
  printf( "===================\n");

  for (uint32_t i=0; i<gconf->GetTotalNumDie(); i++)
  {
    char str[256];
    sprintf(str, "Num of DIE_%u PPN IO request", i);
    PPN_requested_die[i].printstat(str);
  }
  printf( "===================\n");

  CF_DMA0_dma.printstat("Num of conflict DMA0-CH");
  CF_DMA0_mem.printstat("Num of conflict DMA0-MEM");
  CF_DMA0_none.printstat("Num of conflict DMA0-None");
  printf( "===================\n");

  CF_DMA1_dma.printstat("Num of conflict DMA1-CH");
  CF_DMA1_none.printstat("Num of conflict DMA1-None");
  printf( "===================\n");

  Ticks_DMA0WAIT.printstat("Info of DMA0WAIT Tick");
  Ticks_DMA0.printstat("Info of DMA0 Tick");
  Ticks_MEM.printstat("Info of MEM Tick");
  Ticks_DMA1WAIT.printstat("Info of DMA1WAIT Tick");
  Ticks_DMA1.printstat("Info of DMA1 Tick");
  Ticks_Total.printstat("Info of TOTAL(D0W+D0+M+D1W+D1) Tick");
  Ticks_TotalOpti.printstat("Info of OPTIMUM(D0+M+D1) Tick");
  printf( "===================\n");

  for (uint32_t i=0;i<gconf->NumChannel; i++)
  {
    char str[256];
    sprintf(str, "Info of CH_%u Active Tick", i);
    Ticks_Active_ch[i].printstat(str);
  }
  printf( "===================\n");

  for (uint32_t i=0; i<gconf->GetTotalNumDie(); i++)
  {
    char str[256];
    sprintf(str, "Info of DIE_%u Active Tick", i);
    Ticks_Active_die[i].printstat(str);
  }
  printf( "===================\n");


}

void PALStatistics::PrintStats(uint64_t sim_time_ps)
{
  uint64_t elapsed_time_ps = (sim_time_ps - sim_start_time_ps) + 1;
  if (LastExecutionTime == 0)
  LastExecutionTime = sim_start_time_ps;
  DPRINTF(PAL, "Execution time = %" PRIu64 "\n",sim_time_ps);
  DPRINTF(PAL, "Last Execution time = %" PRIu64 "\n",LastExecutionTime);
  if (sim_start_time_ps >= sim_time_ps) //abnormal case
  {
    elapsed_time_ps = sim_time_ps + 1;
  }

  DPRINTF(PAL, "[ PAL Stats ]\n");


  #if 1
  #define SIM_TIME_SEC ( (long double)elapsed_time_ps/PSEC )
  #define BUSY_TIME_SEC ( (long double)ExactBusyTime/PSEC )
  #define TRANSFER_TOTAL_MB ( (long double)(Access_Capacity.vals[OPER_READ].sum + Access_Capacity.vals[OPER_WRITE].sum)/MBYTE )


  fDPRINTF(PAL, "Sim.Time :  %Lf Sec. , %" PRIu64 " ps\n", SIM_TIME_SEC, elapsed_time_ps);
  fDPRINTF(PAL, "Transferred :  %Lf MB\n", TRANSFER_TOTAL_MB);
  fDPRINTF(PAL, "Performance: %Lf MB/Sec\n", (long double)TRANSFER_TOTAL_MB/SIM_TIME_SEC);
  fDPRINTF(PAL, "Busy Sim.Time: %Lf Sec. , %" PRIu64 " ps\n", BUSY_TIME_SEC, ExactBusyTime);
  fDPRINTF(PAL, "Busy Performance: %Lf MB/Sec\n", (long double)TRANSFER_TOTAL_MB/BUSY_TIME_SEC);
  #endif



  #if 1 // Polished stats - Improved instrumentation
  std::map<uint64_t, ValueOper*>::iterator e  = Access_Capacity_snapshot.find(sim_time_ps/EPOCH_INTERVAL-1);
  if (sim_time_ps > 0 && e != Access_Capacity_snapshot.end()){
    PPN_requested_rwe.printstat("Num of PPN IO request");
    DPRINTF(PAL, "===================\n");

    for(uint32_t i=0; i<PAGE_NUM; i++)
    {
      char str[256];
      sprintf(str, "Num of %s page PPN IO request", PAGE_STRINFO[i]);
      PPN_requested_pagetype[i].printstat(str);
    }
    DPRINTF(PAL, "===================\n");

    for (uint32_t i=0;i<gconf->NumChannel; i++)
    {
      char str[256];
      sprintf(str, "Num of CH_%u PPN IO request", i);
      PPN_requested_ch[i].printstat(str);
    }
    DPRINTF(PAL, "===================\n");

    for (uint32_t i=0; i<gconf->GetTotalNumDie(); i++)
    {
      char str[256];
      sprintf(str, "Num of DIE_%u PPN IO request", i);
      PPN_requested_die[i].printstat(str);
    }
    DPRINTF(PAL, "===================\n");

    CF_DMA0_dma.printstat("Num of conflict DMA0-CH");
    CF_DMA0_mem.printstat("Num of conflict DMA0-MEM");
    CF_DMA0_none.printstat("Num of conflict DMA0-None");
    DPRINTF(PAL, "===================\n");

    CF_DMA1_dma.printstat("Num of conflict DMA1-CH");
    CF_DMA1_none.printstat("Num of conflict DMA1-None");
    DPRINTF(PAL, "===================\n");

    Ticks_DMA0WAIT.printstat("Info of DMA0WAIT Tick");
    Ticks_DMA0.printstat("Info of DMA0 Tick");
    Ticks_MEM.printstat("Info of MEM Tick");
    Ticks_DMA1WAIT.printstat("Info of DMA1WAIT Tick");
    Ticks_DMA1.printstat("Info of DMA1 Tick");
    Ticks_Total.printstat("Info of TOTAL(D0W+D0+M+D1W+D1) Tick");
    Ticks_TotalOpti.printstat("Info of OPTIMUM(D0+M+D1) Tick");
    DPRINTF(PAL, "===================\n");

    for (uint32_t i=0;i<gconf->NumChannel; i++)
    {
      char str[256];
      sprintf(str, "Info of CH_%u Active Tick", i);
      Ticks_Active_ch[i].printstat(str);
    }
    DPRINTF(PAL, "===================\n");

    for (uint32_t i=0; i<gconf->GetTotalNumDie(); i++)
    {
      char str[256];
      sprintf(str, "Info of DIE_%u Active Tick", i);
      Ticks_Active_die[i].printstat(str);
    }
    DPRINTF(PAL, "===================\n");

    e->second->printstat("Info of Access Capacity");
    DPRINTF(PAL, "Total execution time (ms)\n");
    DPRINTF(PAL, "%.2f\n", SampledExactBusyTime * 1.0 / 1000000000);
    Access_Bandwidth.printstat_bandwidth(e->second, SampledExactBusyTime, LastExactBusyTime);
    Access_Bandwidth_widle.printstat_bandwidth_widle(e->second, sim_time_ps, LastExecutionTime);
    Access_Oper_Bandwidth.printstat_oper_bandwidth(e->second, OpBusyTime, LastOpBusyTime);
    std::map<uint64_t, ValueOper*>::iterator f  = Ticks_Total_snapshot.find(sim_time_ps/EPOCH_INTERVAL-1);
    f->second->printstat_latency("Info of Latency");
    Access_Iops.printstat_iops(e->second, SampledExactBusyTime, LastExactBusyTime);
    Access_Iops_widle.printstat_iops_widle(e->second, sim_time_ps, LastExecutionTime);
    Access_Oper_Iops.printstat_oper_iops(e->second, OpBusyTime, LastOpBusyTime);
    LastExactBusyTime = SampledExactBusyTime;
    LastExecutionTime = sim_time_ps;
    LastOpBusyTime[0] = OpBusyTime[0];
    LastOpBusyTime[1] = OpBusyTime[1];
    LastOpBusyTime[2] = OpBusyTime[2];
    std::map<uint64_t, ValueOper*>::iterator g  = Access_Capacity_snapshot.upper_bound(sim_time_ps/EPOCH_INTERVAL-1);
    if (g != Access_Capacity_snapshot.end()){
      for (int i=0;i<OPER_ALL;i++){
        g->second->vals[i].sampled_sum = e->second->vals[i].sum;
        g->second->vals[i].sampled_cnt = e->second->vals[i].cnt;
        e->second->vals[i].sampled_sum = e->second->vals[i].sum;
        e->second->vals[i].sampled_cnt = e->second->vals[i].cnt;
      }
    }
    else{
      Access_Capacity_snapshot[sim_time_ps/EPOCH_INTERVAL] = new ValueOper(Access_Capacity_snapshot[sim_time_ps/EPOCH_INTERVAL-1]);
      for (int i=0;i<OPER_ALL;i++){
        Access_Capacity_snapshot[sim_time_ps/EPOCH_INTERVAL]->vals[i].sampled_sum = e->second->vals[i].sum;
        Access_Capacity_snapshot[sim_time_ps/EPOCH_INTERVAL]->vals[i].sampled_cnt = e->second->vals[i].cnt;
      }
    }
    std::map<uint64_t, ValueOper*>::iterator h  = Ticks_Total_snapshot.upper_bound(sim_time_ps/EPOCH_INTERVAL-1);
    if (h != Ticks_Total_snapshot.end()){
      for (int i=0;i<OPER_ALL;i++){
        h->second->vals[i].sampled_sum = f->second->vals[i].sum;
        h->second->vals[i].sampled_cnt = f->second->vals[i].cnt;
        f->second->vals[i].sampled_sum = f->second->vals[i].sum;
        f->second->vals[i].sampled_cnt = f->second->vals[i].cnt;
      }
    }
    else{
      Ticks_Total_snapshot[sim_time_ps/EPOCH_INTERVAL] = new ValueOper(Ticks_Total_snapshot[sim_time_ps/EPOCH_INTERVAL-1]);
      for (int i=0;i<OPER_ALL;i++){
        Ticks_Total_snapshot[sim_time_ps/EPOCH_INTERVAL]->vals[i].sampled_sum = f->second->vals[i].sum;
        Ticks_Total_snapshot[sim_time_ps/EPOCH_INTERVAL]->vals[i].sampled_cnt = f->second->vals[i].cnt;
      }
    }
    Access_Capacity_snapshot.erase(sim_time_ps/EPOCH_INTERVAL-1);
    Ticks_Total_snapshot.erase(sim_time_ps/EPOCH_INTERVAL-1);
  }

  for (int i=0;i<OPER_ALL;i++){
    Access_Capacity.vals[i].backup();
  }

  #endif // Polished stats

}
