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

#ifndef __PAL2_h__
#define __PAL2_h__

#include "SimpleSSD_types.h"

#include "base_config.hh"
#include "Latency.h"
#include "PALStatistics.h"

#include "PAL2_TimeSlot.h"

#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
using namespace std;

class PALStatistics;

class PAL2 //let's not inherit PAL1
{
  public:
    PAL2(PALStatistics* statistics, BaseConfig *c, Latency *l);
    ~PAL2();

    BaseConfig *gconf;
    Latency *lat;

    TimeSlot** ChTimeSlots;
    TimeSlot** DieTimeSlots;
    TimeSlot** MergedTimeSlots; //for gathering busy time

    std::map<uint64_t, uint64_t> OpTimeStamp[3];

    std::map<uint64_t, std::map<uint64_t, uint64_t>* > * ChFreeSlots;
    uint64_t* ChStartPoint; //record the start point of rightmost free slot
    std::map<uint64_t, std::map<uint64_t, uint64_t>* > * DieFreeSlots;
    uint64_t* DieStartPoint;

    void submit(Command &cmd);
    void TimelineScheduling(Command& req);
    PALStatistics* stats; //statistics of PAL2, not created by itself
    void InquireBusyTime(uint64_t currentTick);
    void FlushTimeSlots(uint64_t currentTick);
    void FlushOpTimeStamp();
    TimeSlot* FlushATimeSlot(TimeSlot* tgtTimeSlot, uint64_t currentTick);
    TimeSlot* FlushATimeSlotBusyTime(TimeSlot* tgtTimeSlot, uint64_t currentTick, uint64_t* TimeSum);
    //Jie: merge time slotss
    void MergeATimeSlot(TimeSlot* tgtTimeSlot);
    void MergeATimeSlot(TimeSlot* startTimeSlot, TimeSlot* endTimeSlot);
    void MergeATimeSlotCH(TimeSlot* tgtTimeSlot);
    void MergeATimeSlotDIE(TimeSlot* tgtTimeSlot);
    TimeSlot* InsertAfter(TimeSlot* tgtTimeSlot, uint64_t tickLen, uint64_t tickFrom);

    TimeSlot* FindFreeTime(TimeSlot* tgtTimeSlot, uint64_t tickLen, uint64_t tickFrom); // you can insert a tickLen TimeSlot after Returned TimeSlot.

    //Jie: return: FreeSlot is found?
    bool FindFreeTime(std::map<uint64_t, std::map<uint64_t, uint64_t>* >& tgtFreeSlot, uint64_t tickLen, uint64_t & tickFrom, uint64_t & startTick, bool & conflicts);
    void InsertFreeSlot(std::map<uint64_t, std::map<uint64_t, uint64_t>* >& tgtFreeSlot, uint64_t tickLen, uint64_t tickFrom, uint64_t startTick, uint64_t & startPoint, bool split);
    void AddFreeSlot(std::map<uint64_t, std::map<uint64_t, uint64_t>* >& tgtFreeSlot, uint64_t tickLen, uint64_t tickFrom);
    void FlushFreeSlots(uint64_t currentTick);
    void FlushAFreeSlot(std::map<uint64_t, std::map<uint64_t, uint64_t>* >& tgtFreeSlot, uint64_t currentTick);
    uint8_t VerifyTimeLines(uint8_t print_on);


    //PPN Conversion related //ToDo: Shifted-Mode is also required for better performance.
    uint32_t RearrangedSizes[7];
    uint32_t CPDPBPtoDieIdx(CPDPBP* pCPDPBP);
    void printCPDPBP(CPDPBP* pCPDPBP);
    void PPNdisassemble(uint64_t* pPPN, CPDPBP* pCPDPBP);
    void AssemblePPN(CPDPBP* pCPDPBP, uint64_t* pPPN);
};

#endif
