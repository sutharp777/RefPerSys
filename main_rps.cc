/****************************************************************
 * file main_rps.cc
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It has the main function and related, program option parsing,
 *      code.
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

thread_local Rps_Random Rps_Random::_rand_thr_;

enum rps_option_key_en
{
  Rps_Key_PrintRandomId = 1024,
  Rps_Key_ParseId,
  Rps_Key_ExplainTypes,
};

const struct argp_option rps_argopt_vec[] =
{
  {
    .name = "print-random-id",
    .key = Rps_Key_PrintRandomId,
    .arg = NULL,
    .flags = OPTION_ARG_OPTIONAL,
    .doc = "print a random objectid",
    .group = 0,
  },
  {
    .name = "explain-types",
    .key = Rps_Key_ExplainTypes,
    .arg = NULL,
    .flags = OPTION_ARG_OPTIONAL,
    .doc = "print information about types",
    .group = 0,
  },
  {
    .name = "parse-id",
    .key = Rps_Key_ParseId,
    .arg = "oid",
    .flags = 0,
    .doc = "parse a objectid",
    .group = 0,
  },
  { },
};

typedef std::map<intptr_t,void*> rps_map_intptr_t_to_pointer;
typedef std::unordered_map<intptr_t,void*> rps_unordered_map_intptr_to_pointer;
typedef std::variant<std::string,void*> rps_variant_string_or_pointer;
typedef std::map<std::string,void*> rps_map_string_to_pointer;

void print_types_info(void)
{
  printf("%-36s:   size  align   (in bytes)\n", "**TYPE**");
#define EXPLAIN_TYPE(Ty) printf("%-36s: %5d %5d\n", #Ty,	\
				(int)sizeof(Ty), (int)alignof(Ty))
  EXPLAIN_TYPE(int);
  EXPLAIN_TYPE(double);
  EXPLAIN_TYPE(char);
  EXPLAIN_TYPE(bool);
  EXPLAIN_TYPE(Rps_HashInt);
  EXPLAIN_TYPE(intptr_t);
  EXPLAIN_TYPE(Rps_Id);
  EXPLAIN_TYPE(Rps_Type);
  EXPLAIN_TYPE(Rps_ObjectRef);
  EXPLAIN_TYPE(Rps_ZoneValue);
  EXPLAIN_TYPE(Rps_MarkSweepZoneValue);
  EXPLAIN_TYPE(Rps_CopyingSizedZoneValue);
  EXPLAIN_TYPE(Rps_Value);
  EXPLAIN_TYPE(Rps_ObjectZone);
  EXPLAIN_TYPE(Rps_QuasiAttributeArray);
  EXPLAIN_TYPE(Rps_TupleObrefZone);
  EXPLAIN_TYPE(Rps_SetObrefZone);
  EXPLAIN_TYPE(Rps_DoubleZone);
  EXPLAIN_TYPE(Rps_StringZone);
  EXPLAIN_TYPE(Rps_PaylSetObjrefZone);
  EXPLAIN_TYPE(std::set<intptr_t>);
  EXPLAIN_TYPE(std::set<std::string>);
  EXPLAIN_TYPE(rps_map_intptr_t_to_pointer);
  EXPLAIN_TYPE(rps_unordered_map_intptr_to_pointer);
  EXPLAIN_TYPE(rps_map_string_to_pointer);
  EXPLAIN_TYPE(rps_variant_string_or_pointer);
  EXPLAIN_TYPE(std::vector<intptr_t>);
  EXPLAIN_TYPE(std::atomic<intptr_t>);
  EXPLAIN_TYPE(std::string);
  EXPLAIN_TYPE(std::mutex);
#ifdef RPS_HAVE_MPS
  EXPLAIN_TYPE(mps_ss_t);
  EXPLAIN_TYPE(mps_arena_t);
  EXPLAIN_TYPE(mps_ap_t);
#endif /*RPS_HAVE_MPS*/
#undef EXPLAIN_TYPE
  putchar('\n');
  fflush(nullptr);
} // end print_types_info

error_t rps_argopt_parse(int key, char*arg, struct argp_state*state)
{
  (void) arg;
  (void) state;
  switch (key)
    {
    case Rps_Key_PrintRandomId:
    {
      auto rdid = Rps_Id::random();
      char cbuf[24];
      memset (cbuf,0,sizeof(cbuf));
      rdid.to_cbuf24(cbuf);
      printf("random object id: %s (hi=%#llx,lo=%#llx) h=%u\n",
             cbuf,
             (unsigned long long)rdid.hi(), (unsigned long long)rdid.lo(),
             (unsigned)rdid.hash());
    }
    break;
    case Rps_Key_ExplainTypes:
      print_types_info();
      break;
    case Rps_Key_ParseId:
    {
      const char*idstr = arg;
      const char*end = nullptr;
      bool ok = false;
      assert (idstr != nullptr);
      Rps_Id pid (idstr, &end, &ok);
      if (ok)
        {
          char cbuf[24];
          memset (cbuf,0,sizeof(cbuf));
          pid.to_cbuf24(cbuf);
          printf("parsed '%s' as object id %s (hi=%#llx,lo=%#llx) h=%u\n",
                 idstr, cbuf,
                 (unsigned long long)pid.hi(), (unsigned long long)pid.lo(),
                 (unsigned)pid.hash());
          if (end && *end)
            printf("trailing '%s' of '%s' is not parsed\n", end, idstr);
        }
      else fprintf(stderr, "failed to parse id %s\n", idstr);
    }
    break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
} // end rps_argopt_parse




int
main(int argc, char** argv)
{
  struct argp argopt =
  {
    .options = rps_argopt_vec,
    .parser = rps_argopt_parse,
    .args_doc = NULL,
    .doc = NULL,
    .children = NULL,
    .help_filter = NULL,
    .argp_domain = NULL
  };
  if (argc < 2)
    {
      printf("missing argument to %s, try %s --help\n", argv[0], argv[0]);
      exit(EXIT_FAILURE);
    }
  argp_parse (&argopt, argc, argv, 0, NULL, NULL);
  return 0;
} // end of main

void
rps_fatal_stop_at (const char *fil, int lin)
{
#warning rps_fatal_stop_at should show a backtrace
  // FIXME: should show a backtrace
  assert(fil != nullptr);
  assert (lin>=0);
  abort();
} // end rps_fatal_stop_at