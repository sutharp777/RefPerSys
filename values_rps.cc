/****************************************************************
 * file values_rps.cc
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *      Implementation related to immutable values and quasivalues
 *
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      Abhishek Chakravarti <abhishek@taranjali.org>
 *      Nimesh Neema <nimeshneema@gmail.com>
 *
 *      © Copyright 2019 The Reflective Persistent System Team
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

extern "C" const char rps_values_gitid[];
const char rps_values_gitid[]= RPS_GITID;

extern "C" const char rps_values_date[];
const char rps_values_date[]= __DATE__;


void
Rps_Id::to_cbuf24(char cbuf[]) const
{
  /// example cbuf = "_0abcdefghijABCDEFG"
  ///                  |0         |11    |19
  static_assert(sizeof("_0abcdefghijABCDEFG")-1 == 1+nbdigits_hi+nbdigits_lo);
  RPS_ASSERT (cbuf != nullptr);
  memset(cbuf, 0, buflen);
  char*last = cbuf+nbdigits_hi;
  auto pc = last;
  cbuf[0] = '_';
  uint64_t n = _id_hi;
  do
    {
      unsigned d = n % base;
      n = n / base;
      *pc = b62digits[d];
      pc--;
    }
  while (pc>cbuf);
  auto start = cbuf+nbdigits_hi;
  last = start+nbdigits_lo;
  pc = last;
  n = _id_lo;
  do
    {
      unsigned d = n % base;
      n = n / base;
      *pc = b62digits[d];
      pc--;
    }
  while (pc>start);
};				// end Rps_Id::to_cbuf24


/// opposite conversion from cbuf to oid
Rps_Id::Rps_Id (const char*cbuf, const char**pend, bool *pok) : Rps_Id ()
{
  auto lasthi = cbuf+nbdigits_hi+1;
  auto lastlo = lasthi + nbdigits_lo;
  uint64_t hi=0, lo=0;
  if (cbuf[0] != '_' && !isdigit(cbuf[1])) goto fail;
  for (auto pcb = cbuf+1; *pcb && pcb<lasthi; pcb++)
    {
      auto pcs = strchr(b62digits, *pcb);
      if (!pcs) goto fail;
      hi = hi*62 + (pcs-b62digits);
    }
  if (hi < min_hi || hi >= max_hi) goto fail;
  for (auto pcb = lasthi; *pcb && pcb < lastlo; pcb++)
    {
      auto pcs = strchr(b62digits, *pcb);
      if (!pcs) goto fail;
      lo = lo*62 + (pcs-b62digits);
    }
  if (lo < min_lo || lo >= max_lo) goto fail;
  _id_hi = hi;
  _id_lo = lo;
  if (pend) *pend = lastlo;
  if (pok) *pok = true;
  return;
fail:
  if (pend) *pend = cbuf;
  if (pok) *pok = false;
  return;
} // end Rps_Id::Rps_Id (const char*, char**, bool*)



//////////////////////////////////////////////// sets

Rps_SetOb::Rps_SetOb(const std::set<Rps_ObjectRef>& setob, Rps_SetOb::Rps_SetTag)
  : Rps_SetOb::Rps_SetOb((unsigned) setob.size(), Rps_SetOb::Rps_SetTag{})
{
  int ix=0;
  for (auto ob : setob)
    {
      RPS_ASSERT (ob);
      _seqob[ix++] = ob;
    }
} // end Rps_SetOb::Rps_SetOb



const Rps_SetOb*
Rps_SetOb::make(const std::set<Rps_ObjectRef>& setob)
{
  auto setsiz = setob.size();
  if (RPS_UNLIKELY(setsiz >= Rps_SeqObjRef::maxsize))
    throw std::length_error("Rps_SetOb::make with too many elements");
  for (auto ob : setob)
    if (RPS_UNLIKELY(!ob))
      throw std::invalid_argument("empty element to Rps_SetOb::make");
  return
    rps_allocate_with_wordgap<Rps_SetOb,const std::set<Rps_ObjectRef>&,Rps_SetTag>
    (setsiz,setob,Rps_SetTag{});
} // end of Rps_SetOb::make with set




const Rps_SetOb*
Rps_SetOb::make(const std::initializer_list<Rps_ObjectRef>&elemil)
{
  std::set<Rps_ObjectRef>elemset;
  for (auto elem: elemil)
    if (elem)
      elemset.insert(elem);
  return make(elemset);
} // end of Rps_SetOb::make with initializer_list



const Rps_SetOb*
Rps_SetOb::make(const std::vector<Rps_ObjectRef>&vecob)
{
  std::set<Rps_ObjectRef>elemset;
  for (auto ob: vecob)
    if (ob)
      elemset.insert(ob);
  return make(elemset);
} // end of Rps_SetOb::make with vector


const Rps_SetOb*
Rps_SetOb::collect(const std::vector<Rps_Value>&vecval)
{
  std::set<Rps_ObjectRef>elemset;
  for (auto val: vecval)
    {
      if (val.is_object())
        elemset.insert(Rps_ObjectRef(val.as_object()));
      else if (val.is_tuple())
        {
          auto tup = val.as_tuple();
          for (auto ob: *tup)
            if (ob)
              elemset.insert(ob);
        }
      else if (val.is_set())
        {
          auto set = val.as_set();
          for (auto ob: *set)
            elemset.insert(ob);
        }
    }
  return make(elemset);
} // end of Rps_SetOb::collect with vector




/* end of file value_rps.cc */

