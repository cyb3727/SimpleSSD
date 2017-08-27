# This file is part of SimpleSSD.
#
# SimpleSSD is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# SimpleSSD is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with SimpleSSD.  If not, see <http://www.gnu.org/licenses/>.
#
# Authors: Ahmed Abulila <abulila2@illinois.edu>
#          Donghyun Gouk <kukdh1@camelab.org>

Import('*')

if env['TARGET_ISA'] == 'null':
    Return()

Source('ftl.cc')
Source('ftl_block.cc')
Source('ftl_hybridmapping.cc')
Source('ftl_mappingtable.cc')
Source('ftl_statistics.cc')
Source('Latency.cc')
Source('LatencySLC.cc')
Source('LatencyMLC.cc')
Source('LatencyTLC.cc')
Source('PAL2.cc')
Source('PAL2_TimeSlot.cc')
Source('PALStatistics.cc')
Source('base_config.cc')
Source('ini.c')

DebugFlag('FTLOut')
DebugFlag('PAL')

CompoundFlag('SimpleSSD', [ 'FTLOut', 'PAL' ])
