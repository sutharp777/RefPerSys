/****************************************************************
 * file perstore_rps.cc
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *      The persistent store related routines (load and dump)
 *
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      Niklas Rosencrantz <niklasro@gmail.com>
 *      Abhishek Chakravarti <abhishek@taranjali.org>
 *
 *      © Copyright 2019 The Reflective Persistent System Team
 *      <https://refpersys.gitlab.io>
 *
 * License:
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "refpersys.hh"



void
Rps_Loader::do_load(void)
{
  RPS_LOCALFRAME(call_frame(), /*descr:*/nullptr,
                 Rps_Value v1;
                );
#warning Rps_Loader::do_load unimplemented
  RPS_FATAL("Rps_Loader::do_load unimplemented this @%p", (void*)this);
} // end Rps_Loader::do_load()


/// an example internal loader function which does not use the GC but does
/// access loader pointers
void
Rps_Loader::example_func()
{
  assert(RPS_LDATA(lp_v1));
} // end Rps_Loader::example_func()



/// an other example of internal loader function which uses the GC,
/// e.g. because it is allocating data.
void
Rps_Loader::example_gc_func(Rps_CallFrameZone*callfram)
{
  RPS_LOCALFRAME(callfram, /*descr:*/nullptr,
                 Rps_Value w1;
                );
  _.w1 = RPS_LDATA(lp_v1);
} // end of Rps_Loader::example_gc_func




/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////





void
Rps_Dumper::do_dump(void)
{
  RPS_LOCALFRAME(call_frame(), /*descr:*/nullptr,
                 Rps_Value v1;
                );
#warning Rps_Dumper::do_dump unimplemented
  RPS_FATAL("Rps_Dumper::do_dump unimplemented this @%p", (void*)this);
} // end Rps_Dumper::do_load()


/// an example internal dumper function which does not use the GC but does
/// access dumper pointers
void
Rps_Dumper::example_func()
{
  assert(RPS_DUDAT(dp_v1));
} // end Rps_Dumper::example_func()



/// an other example of internal dumper function which uses the GC,
/// e.g. because it is allocating data.
void
Rps_Dumper::example_gc_func(Rps_CallFrameZone*callfram)
{
  RPS_LOCALFRAME(callfram, /*descr:*/nullptr,
                 Rps_Value w1;
                );
  _.w1 = RPS_DUDAT(dp_v1);
} // end of Rps_Dumper::example_gc_func
