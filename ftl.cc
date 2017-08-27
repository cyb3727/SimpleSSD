//
//  FTL.cpp
//  FTL-3
//
//  Created by Narges on 7/3/15.
//  Copyright (c) 2015 narges shahidi. All rights reserved.
//
//  Modified by Donghyun Gouk <kukdh1@camelab.org>
//

#include "ftl.hh"
#include "ftl_statistics.hh"
#include "PAL2.h"

#ifndef MAX
#define MAX(x, y)   ((x) > (y) ? (x) : (y))
#endif

FTL::FTL(Parameter *p, PAL2 *pal2) :
  param(p), pal(pal2) {
  FTLmapping = new HybridMapping(this);
}

FTL::~FTL(){
  delete param;
}

void FTL::initialize(){
  std::cout << "Total physical block/page "  << param->physical_block_number << "  " << param->physical_page_number << endl;
  std::cout << "Total logical block/page "  << param->logical_block_number << "  " << param->logical_page_number << endl;

  for (Addr i = 0; i < param->logical_block_number; i++){
    int to_fill_page_number = (param->page_per_block * param->warmup);
    if (to_fill_page_number > param->page_per_block)  {
      cout << "error in initialization " << endl;
      return;
    }
    if (to_fill_page_number != 0) {
      write (i * param->page_per_block, to_fill_page_number, 0, true);
    }
  }
  std::cout << "Initialization done! " << std::endl;
}

Tick FTL::read(Addr lpn, size_t npages, Tick arrived) {
  Tick finished = 0;
  Addr ppn;

  ftl_statistics.read_req_arrive(arrived);

  for (size_t i = 0; i < npages; i++) {
    FTLmapping->read(lpn + i, ppn);
    finished = MAX(readInternal(ppn, arrived), finished);
  }

  Command cmd = Command(arrived, lpn, OPER_READ, param->page_byte * npages);
  cmd.finished = finished;

  ftl_statistics.updateStats(&cmd);

  return finished - arrived;
}

Tick FTL::write(Addr lpn, size_t npages, Tick arrived, bool init) {
  Tick finished = 0;
  Addr ppn;

  ftl_statistics.write_req_arrive(arrived);

  for (size_t i = 0; i < npages; i++) {
    FTLmapping->write(lpn + i, ppn);
    if (!init) {
      finished = MAX(writeInternal(ppn, arrived), finished);
    }
  }

  if (FTLmapping->need_gc()) {
    FTLmapping->GarbageCollection();
  }

  Command cmd = Command(arrived, lpn, OPER_WRITE, param->page_byte * npages);
  cmd.finished = finished;

  ftl_statistics.updateStats(&cmd);

  return finished - arrived;
}

Tick FTL::trim(Addr lpn, size_t npages) {
  // FIXME: Not implemented
  return 0;
}

void FTL::translate(Addr lpn, CPDPBP *pa) {
  Addr ppn;

  FTLmapping->read(lpn, ppn);
  pal->PPNdisassemble(&ppn, pa);
}

Tick FTL::readInternal(Addr ppn, Tick now, bool flag) {
  Command cmd = Command(now, ppn, OPER_READ, param->page_byte);
  cmd.mergeSnapshot = flag;

  pal->submit(cmd);

  return cmd.finished;
}

Tick FTL::writeInternal(Addr ppn, Tick now, bool flag) {
  Command cmd = Command(now, ppn, OPER_WRITE, param->page_byte);
  cmd.mergeSnapshot = flag;

  pal->submit(cmd);

  return cmd.finished;
}

Tick FTL::eraseInternal(Addr ppn, Tick now) {
  Command cmd = Command(now, ppn, OPER_ERASE, 0);

  pal->submit(cmd);

  return cmd.finished;
}

void FTL::PrintStats(Tick sim_time) {
  // release finished requests
  ftl_statistics.print_epoch_stats(sim_time);
  FTLmapping->PrintStats();
}

void FTL::PrintFinalStats(Tick sim_time) {
  //que->PrintStats();
  //FTLmapping->PrintStats();
  ftl_statistics.print_final_stats(sim_time);
}
