/****************************************************************
 * file main_rps.cc
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It implements the agenda mechanism.
 *
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      Abhishek Chakravarti <abhishek@taranjali.org>
 *      Nimesh Neema <nimeshneema@gmail.com>
 *
 *      © Copyright 2020 The Reflective Persistent System Team
 *      team@refpersys.org & http://refpersys.org/
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


std::recursive_mutex Rps_Agenda::agenda_mtx_;
std::deque<Rps_ObjectRef> Rps_Agenda::agenda_fifo_[Rps_Agenda::AgPrio__Last];

const char*
Rps_Agenda::agenda_priority_names[Rps_Agenda::AgPrio__Last];


void
Rps_Agenda::initialize(void)
{
  agenda_priority_names[AgPrio_Low]= "low_priority";
  agenda_priority_names[AgPrio_Normal]= "normal_priority";
  agenda_priority_names[AgPrio_High]= "high_priority";
} // end Rps_Agenda::initialize

void
Rps_Agenda::gc_mark(Rps_GarbageCollector&gc)
{
  std::lock_guard<std::recursive_mutex> gu(agenda_mtx_);
  for (int ix=AgPrio_Low; ix<AgPrio__Last; ix++)
    {
      auto& curfifo = agenda_fifo_[ix];
      for (auto it: curfifo)
        {
          Rps_ObjectRef ob = *it;
          if (ob)
            ob->gc_mark(gc);
        }
    }
} // end Rps_Agenda::gc_mark

void
Rps_Agenda::dump_scan_agenda(Rps_Dumper*du)
{
  RPS_ASSERT (du != nullptr);
  std::lock_guard<std::recursive_mutex> gu(agenda_mtx_);
  for (int ix=AgPrio_Low; ix<AgPrio__Last; ix++)
    {
      auto& curfifo = agenda_fifo_[ix];
      for (auto it: curfifo)
        {
          Rps_ObjectRef ob = *it;
          if (ob)
            rps_dump_scan_object(du, ob);
        };
    }
} // end Rps_Agenda::dump_scan_agenda

void
Rps_Agenda::dump_json_agenda(Rps_Dumper*du, Json::Value&jv)
{
  RPS_ASSERT (du != nullptr);
  std::lock_guard<std::recursive_mutex> gu(agenda_mtx_);
  for (int ix=AgPrio_Low; ix<AgPrio__Last; ix++)
    {
      auto& curfifo = agenda_fifo_[ix];
      const char*prioname = agenda_priority_names[ix];
      RPS_ASSERT(prioname != nullptr);
      if (!curfifo.empty())
        {
          Json::Value jseq(Json::arrayValue);
          for (auto it: curfifo)
            {
              Rps_ObjectRef ob = *it;
              if (ob && rps_is_dumpable_objref(du, ob))
                {
                  Json::Value job = rps_dump_json_objectref(du, ob);
                  jseq.append(job);
                }
            };
          jv[prioname] = jseq;
        }
    }
} // end Rps_Agenda::dump_json_agenda


//// loading of agenda related payload
void
rpsldpy_agenda(Rps_ObjectZone*obz, Rps_Loader*ld, const Json::Value& jv, Rps_Id spacid, unsigned lineno)
{
  RPS_ASSERT(obz != nullptr);
  RPS_ASSERT(ld != nullptr);
  RPS_ASSERT(obz->get_payload() == nullptr);
  RPS_ASSERT(jv.type() == Json::objectValue);
  for (int  ix= Rps_Agenda::AgPrio_Low; ix< Rps_Agenda::AgPrio__Last; ix++)
    {
      const char*prioname =  Rps_Agenda::agenda_priority_names[ix];
      RPS_ASSERT(prioname != nullptr);
      auto jseq = jv [prioname];
      if (jseq.type() == Json::arrayValue)
        {
          /// TODO: missing code here...
        }
    }
  RPS_FATALOUT("unimplemented rpsldpy_agenda obz=" << obz
               << " spacid=" << spacid
               << " lineno=" << lineno);
#warning unimplemented rpsldpy_agenda
} // end of rpsldpy_agenda

Rps_PayloadAgenda::~Rps_PayloadAgenda()
{
  RPS_ASSERT (owner() == Rps_Agenda::the_agenda());
} // end Rps_PayloadAgenda::~Rps_PayloadAgenda

void
Rps_PayloadAgenda::gc_mark(Rps_GarbageCollector&gc) const
{
  RPS_ASSERT (owner() == Rps_Agenda::the_agenda());
  Rps_Agenda::gc_mark(gc);
} // end Rps_PayloadAgenda::gc_mark

void
Rps_PayloadAgenda::dump_scan(Rps_Dumper*du) const
{
  RPS_ASSERT (owner() == Rps_Agenda::the_agenda());
  RPS_ASSERT (du != nullptr);
  Rps_Agenda::dump_scan_agenda(du);
} // end Rps_PayloadAgenda::dump_scan

void
Rps_PayloadAgenda::dump_json_content(Rps_Dumper*du, Json::Value&jv) const
{
  RPS_ASSERT (owner() == Rps_Agenda::the_agenda());
  RPS_ASSERT (du != nullptr);
  Rps_Agenda::dump_json_agenda(du,jv);
} // end Rps_PayloadAgenda::dump_json_content


bool
Rps_PayloadAgenda::is_erasable() const
{
  RPS_ASSERT (owner() == Rps_Agenda::the_agenda());
  return false;
} // end Rps_PayloadAgenda::is_erasable

//// end of file agenda_rps.cc
