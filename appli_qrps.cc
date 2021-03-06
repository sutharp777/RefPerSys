/****************************************************************
 * file appli_qrps.cc
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It has the Qt5 code related to the Qt5 application
 *
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      Abhishek Chakravarti <abhishek@taranjali.org>
 *      Nimesh Neema <nimeshneema@gmail.com>
 *
 *      © Copyright 2019 - 2020 The Reflective Persistent System Team
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
#include "qthead_qrps.hh"


extern "C" const char rps_appli_gitid[];
const char rps_appli_gitid[]= RPS_GITID;

extern "C" const char rps_appli_date[];
const char rps_appli_date[]= __DATE__;

// these functions are here, because other files might not include <QString>
const Rps_String*
Rps_String::make(const QString&qs)
{
  return Rps_String::make(qs.toStdString());
} // end Rps_String::make(const QString&qs)

Rps_StringValue::Rps_StringValue(const QString&qs)
  : Rps_Value(Rps_String::make(qs), Rps_ValPtrTag{})
{
} // end of Rps_StringValue::Rps_StringValue(const QString&qs)

////////////////

static char rps_bufpath_homedir[384];

const char* rps_homedir(void)
{
  static std::mutex homedirmtx;
  std::lock_guard<std::mutex> gu(homedirmtx);
  if (RPS_UNLIKELY(rps_bufpath_homedir[0] == (char)0))
    {
      const char*rpshome = getenv("REFPERSYS_HOME");
      const char*home = getenv("HOME");
      const char*path = rpshome?rpshome:home;
      if (!path)
        RPS_FATAL("no RefPerSys home ($REFPERSYS_HOME or $HOME)");
      char* rp = realpath(path, nullptr);
      if (!rp)
        RPS_FATAL("realpath failed on RefPerSys home %s - %m",
                  path);
      if (strlen(rp) >= sizeof(rps_bufpath_homedir) -1)
        RPS_FATAL("too long realpath %s on RefPerSys home %s", rp, path);
      strncpy(rps_bufpath_homedir, rp, sizeof(rps_bufpath_homedir) -1);
    }
  return rps_bufpath_homedir;
} // end rps_homedir

QThread* RpsQApplication::app_mainqthread;
pthread_t RpsQApplication::app_mainselfthread;
std::thread::id RpsQApplication::app_mainthreadid;

///////////////////////////////////////////////////////// debugging support
void
rps_set_debug(const std::string &deblev)
{
  if (deblev == "help")
    {
      fprintf(stderr, "Comma separated debugging levels with -d<debug-level> or --debug=<debug-level>:\n");
#define Rps_SHOW_DEBUG(Opt) fprintf(stderr, "\t%s\n", #Opt);
      RPS_DEBUG_OPTIONS(Rps_SHOW_DEBUG);
#undef Rps_SHOW_DEBUG
      fflush(nullptr);
    }
  else
    {
      const char*comma=nullptr;
      for (const char*pc = deblev.c_str(); pc && *pc; pc = comma?(comma+1):nullptr)
        {
          comma = strchr(pc, ',');
          std::string curlev;
          if (comma && comma>pc)
            curlev = std::string(pc, comma-pc);
          else
            curlev = std::string(pc);

#define Rps_SET_DEBUG(Opt) \
      else if (curlev == #Opt) {			\
	rps_debug_flags |= (1 << RPS_DEBUG_##Opt);	\
      RPS_INFORMOUT("debugging flag "			\
		    << #Opt << " is set.");		\
      }
          if (curlev == "NEVER")
            RPS_WARNOUT("forbidden debug level " << curlev);
          RPS_DEBUG_OPTIONS(Rps_SET_DEBUG)
          else
            RPS_WARNOUT("unknown debug level " << curlev);
#undef Rps_SET_DEBUG
        }
    };
  RPS_DEBUG_LOG(MISC, "rps_debug_flags=" << rps_debug_flags);
} // end rps_set_debug


////////////////////////////////////////////////////////////////
Json::Value
RpsQApplication::read_application_json(void)
{
  std::string pathapp = rps_topdirectory;
  pathapp += "/app-refpersys.json";
  if (access(pathapp.c_str(), R_OK))
    throw RPS_RUNTIME_ERROR_OUT("failed to access application JSON file "
                                << pathapp << ":" << strerror(errno));
  Json::Reader jread(Json::Features::all());
  std::ifstream ins(pathapp);
  if (!ins)
    throw RPS_RUNTIME_ERROR_OUT("failed to open application JSON file "
                                << pathapp << ":" << strerror(errno));

  Json::Value root(Json::nullValue);
  if (!jread.parse(ins,root,false))
    throw RPS_RUNTIME_ERROR_OUT("failed to read application JSON file "
                                << pathapp
                                << ":" << jread.getFormattedErrorMessages());
  return root;
} // end RpsQApplication::read_application_json

Json::Value
RpsQApplication::read_user_json(void)
{
  std::string pathapp = rps_homedir();
  pathapp += "/refpersys-user.json";
  if (access(pathapp.c_str(), R_OK))
    {
      RPS_WARNOUT("failed to access user JSON file "
                  << pathapp << ":" << strerror(errno));
      return Json::Value::nullSingleton();
    }
  Json::Reader jread(Json::Features::all());
  std::ifstream ins(pathapp);
  if (!ins)
    throw RPS_RUNTIME_ERROR_OUT("failed to open user JSON file "
                                << pathapp << ":" << strerror(errno));
  Json::Value root(Json::nullValue);
  if (!jread.parse(ins,root,false))
    throw RPS_RUNTIME_ERROR_OUT("failed to read user JSON file "
                                << pathapp
                                << ":" << jread.getFormattedErrorMessages());
  return root;
} // end RpsQApplication::read_user_json


Json::Value
RpsQApplication::read_current_json(std::string jsonpath)
{
  if (jsonpath.find('/') || jsonpath.find(".."))
    throw RPS_RUNTIME_ERROR_OUT("read_current_json bad path " << jsonpath);
  std::ifstream ins(jsonpath);
  {
    int olderrno = errno;
    char cwdbuf[128];
    memset(cwdbuf, 0, sizeof(cwdbuf));
    getcwd(cwdbuf, sizeof(cwdbuf)-1);
    if (!ins)
      {
        RPS_RUNTIME_ERROR_OUT("failed to open current JSON file "
                              << jsonpath
                              << " in " << cwdbuf
                              << ":" << strerror(olderrno));
      }
  }
  Json::Reader jread(Json::Features::all());
  Json::Value root(Json::nullValue);
  if (!jread.parse(ins,root,false))
    throw RPS_RUNTIME_ERROR_OUT("failed to read current JSON file "
                                << jsonpath
                                << ":" << jread.getFormattedErrorMessages());
  return root;
} // end RpsQApplication::read_current_json

void
RpsQApplication::do_add_new_window(Rps_CallFrame*callerframe)
{
  RPS_LOCALFRAME(nullptr,
                 callerframe,
                 Rps_ObjectRef obwin; // the attribute
                );
  std::lock_guard gu(app_mutex);
  int winrk = -1;
  RPS_ASSERT(app_windvec.size()>0);
  for (int rk=1; rk<(int) app_windvec.size(); rk++)
    {
      if (app_windvec[rk] == nullptr)
        {
          winrk = rk;
          break;
        }
    };
  if (winrk<0)
    {
      winrk = app_windvec.size();
      app_windvec.push_back(nullptr);
    }
  auto window = new RpsQWindow(nullptr,winrk);
  QString gitidqs (rps_gitid);
  gitidqs.remove (10, strlen(rps_gitid)-15);
  gitidqs.insert (10, "⋯"); // U+22EF MIDLINE HORIZONTAL ELLIPSIS
  window->setWindowTitle(QString("RefPerSys #%1 - %2").arg(winrk).arg(gitidqs));
  int w = 600;
  int h = 480;
  Json::Value juser(Json::nullValue);
  Json::Value japp(Json::nullValue);
  try
    {
      juser = read_user_json();
    }
  catch (std::exception&exc)
    {
      RPS_WARNOUT("failed to read user JSON:"
                  << std::endl << exc.what());
    };
  try
    {
      japp = read_application_json();
    }
  catch(std::exception&exc)
    {
      RPS_WARNOUT("failed to read application JSON:"
                  << std::endl << exc.what());
    };
  if (juser.isMember("window-width"))
    w = juser["window-width"].asInt();
  else if (japp.isMember("window-width"))
    w = japp["window-width"].asInt();
  if (juser.isMember("window-height"))
    w = juser["window-height"].asInt();
  else if (japp.isMember("window-height"))
    w = japp["window-height"].asInt();
  auto screengeom = desktop()->screenGeometry();
  if (w > (3*screengeom.width())/4)
    w = 3*screengeom.width()/4;
  if (w < screengeom.width()/4)
    w = 16 + screengeom.width()/4;
  if (h >  (3*screengeom.height())/4)
    h = 3*screengeom.height()/4;
  if (h < screengeom.height()/4)
    h = 16 + screengeom.height()/4;
  window->resize (w, h);
  RPS_DEBUG_LOG(GUI, "RpsQApplication::add_new_window winrk#" << winrk
                << " w=" << w << " h=" << h);
  window->create_winobj(&_);
  window->show();
  app_windvec[winrk] = window;
  app_wndcount++;
} // end of RpsQApplication::add_new_window


void
RpsQApplication::do_dump_then_exit(QString dir)
{
  rps_dump_into(dir.toStdString());
  QApplication::exit(0);
} // end RpsQApplication::do_dump_then_exit

void
RpsQWindow::do_quit()
{
  auto res = QMessageBox::question(this, "Quit RefPerSys?",
                                   "quit without dumping the persistent state?");

  if (res == QMessageBox::Yes)
    QApplication::exit(0);
} // end RpsQWindow::do_quit

RpsQApplication::RpsQApplication(int &argc, char*argv[])
  : QApplication(argc, argv),
    app_mutex(),
    app_windvec(),
    app_wndcount (0),
    app_settings(nullptr)
{
  RPS_LOCALFRAME(nullptr /*no descr*/,
                 nullptr /*no calling frame*/,
                 Rps_Value val; // the value
                );
  setApplicationName("RefPerSys");
  setOrganizationDomain("refpersys.org");
  setOrganizationName("RefPerSys community");
  setApplicationVersion(rps_lastgitcommit);
} // end of RpsQApplication::RpsQApplication


/// the initialize_app is running only when rps_batch is false
void
RpsQApplication::initialize_app(void)
{
  RPS_LOCALFRAME(nullptr /*no descr*/,
                 nullptr /*no calling frame*/,
                 Rps_Value val; // the value
                );
  app_windvec.reserve(16);
  app_windvec.push_back(nullptr); // we don't want a 0 index.
  do_add_new_window(&_);
} // end RpsQApplication::initialize_app

RpsQApplication::~RpsQApplication()
{
  delete app_settings;
} // end RpsQApplication::~RpsQApplication

void
RpsQApplication::gc_mark(Rps_GarbageCollector&gc) const
{
  std::lock_guard guapp(app_mutex);
  unsigned nbwin = app_windvec.size();
  for (unsigned winix=0; winix<nbwin; winix++)
    {
      if (app_windvec[winix])
        app_windvec[winix]->gc_mark(gc);
    }
} // end RpsQApplication::gc_mark

void
rps_garbcoll_application(Rps_GarbageCollector&gc)
{
  RPS_ASSERT(rps_is_main_gui_thread());
  auto thisapp = RpsQApplication::the_app();
  if (thisapp)
    thisapp->gc_mark(gc);
} // end rps_garbcoll_application

void
RpsQApplication::do_remove_window_by_index(int ix)
{
  std::lock_guard guapp(app_mutex);
  int wincount = app_windvec.size();
  if (ix <= 0)
    throw RPS_RUNTIME_ERROR_OUT("do_remove_window: negative index " << ix);
  if (ix >= wincount)
    throw RPS_RUNTIME_ERROR_OUT("do_remove_window: too large index " << ix
                                << " is more than " << wincount);
  RpsQWindow* win = app_windvec[ix].data();
  win->close();
  app_windvec[ix].clear();
} // end RpsQApplication::do_remove_window

void
RpsQApplication::do_remove_window(RpsQWindow*win)
{
  if (win)
    do_remove_window_by_index(win->window_rank());
} // end RpsQApplication::do_remove_window


void
RpsQApplication::do_dump_state(QString dirpath)
{
  rps_dump_into(dirpath.toStdString());
} // end of RpsQApplication::do_dump_state

void
RpsQApplication::do_dump_current_state(void)
{
  rps_dump_into(".");
} // end of RpsQApplication::do_dump_current_state


void
RpsQApplication::do_dump_current_then_exit(void)
{
  do_dump_then_exit(".");
} // end of RpsQApplication::do_dump_current_then_exit




RpsQWindow*
RpsQApplication::getWindowPtr(int ix)
{
  std::lock_guard gu(app_mutex);
  if (ix < 0)
    ix += app_windvec.size();
  if (ix <= 0 || ix > (int)app_windvec.size())
    return nullptr;
  return app_windvec.at(ix).data();
} // end RpsQApplication::getWindowPtr

RpsQWindow*
RpsQApplication::first_window_ptr(int *prank)
{
  std::lock_guard gu(app_mutex);
  if (prank)
    *prank = 0;
  int maxwinrk = (int)app_windvec.size();
  for (int ix=1; ix<maxwinrk; ix++)
    {
      QPointer<RpsQWindow> curwinptr = app_windvec[ix];
      if (!curwinptr) continue;
      if (prank) *prank = ix;
      return curwinptr;
    }
  return nullptr;
} // end RpsQApplication::first_window_ptr



void
RpsQApplication::do_display_object(const QString& obqstr, Rps_CallFrame*callerframe)
{
  RPS_LOCALFRAME(RPS_ROOT_OB(_8xCV6GDXYMa02mK5xy), ///display_object_content_qt∈symbol
                 callerframe,
                 Rps_ObjectRef dispob; // the object to display
                 Rps_ObjectRef winob;  // the window where we display
                 Rps_ObjectRef selob; // the selector
                 Rps_Value dispv; //
                 Rps_Value depthv; //
                );
  std::string obstr = obqstr.toStdString();
  _.selob = RPS_ROOT_OB(_02iWbXmFx8f04ldLRt); // display_object_content_qt selector
  _.depthv = Rps_Value((intptr_t)0);
  RPS_INFORMOUT("RpsQApplication::do_display_object should display obstr='"
                << obstr << "' with selob=" << _.selob);
  RPS_DEBUG_LOG(GUI, "do_display_object dispob=" << _.dispob << ", selob=" << _.selob << ", depthv=" << _.depthv);
  if (obstr.empty())
    {
      RPS_WARNOUT("RpsQApplication::do_display_object with empty string");
      return;
    }
  else if (obstr[0] == '_')
    {
      Rps_Id oid(obstr);
      if (oid.empty())
        RPS_WARNOUT("RpsQApplication::do_display_object with empty oid " << obstr);
      else
        _.dispob = Rps_ObjectRef::find_object_by_oid(&_, oid);
    }
  else if (isalpha(obstr[0]))
    {
      _.dispob = Rps_ObjectRef::find_object_by_string(&_, obstr,  Rps_ObjectRef:: Fail_When_Missing);
    }
  else
    {
      RPS_WARNOUT("RpsQApplication::do_display_object with bad string '" << obstr << "'");
      return;
    }
  RPS_DEBUG_LOG(GUI, "do_display_object dispob=" << _.dispob);
  // compute the winob
  {
    int winrk=0;
    RpsQWindow* firstwinptr = first_window_ptr(&winrk);
    if (firstwinptr)
      {
        _.winob = firstwinptr->window_object();
      }
    else
      RPS_WARNOUT("RpsQApplication::do_display_object " << obstr << " without first window"
                  << std::endl << RPS_FULL_BACKTRACE_HERE(1, "do_display_object nowindow")
                  << std::endl);
    RPS_DEBUG_LOG(GUI,
                  "RpsQApplication::do_display_object dispob=" << _.dispob
                  << " winrk=" << winrk
                  << " winob=" << _.winob
                  << " selob=" << _.selob
                  << " depthv=" << _.depthv);
  }
  // send display_object_content_qt selector to dispob
  // with winob and depth=0
  _.dispv = Rps_ObjectValue(_.dispob);
  if (_.dispv)
    RPS_DEBUG_LOG(GUI, "RpsQApplication::do_display_object dispv=" << _.dispv
                  << " dispob=" << _.dispob << std::endl
                  << ".. of class=" << _.dispob->compute_class(&_) << std::endl
                  << "... selob=" << _.selob
                  << " of class=" << _.selob->compute_class(&_)<< std::endl
                  << "... winob=" << _.winob
                  << " of class=" << _.winob->compute_class(&_)<< std::endl);
  else
    RPS_WARNOUT("RpsQApplication::do_display_object no dispv");
  RPS_DEBUG_LOG(GUI, "RpsQApplication::do_display_object dispv=" << _.dispv

                << ", selob=" << _.selob
                << ", winob=" << _.winob
                << ", depthv=" << _.depthv << std::endl
                << RPS_DEBUG_BACKTRACE_HERE(1, "do_display_object fullhere")
                << "**+++** do_display_object fullbacktrace **+++**" << std::endl
               );
  Rps_TwoValues twores =
    _.dispv.send2(&_, _.selob,
                  _.winob,
                  _.depthv);
  RPS_DEBUG_LOG(GUI, "RpsQApplication::do_display_object result for dispv=" << _.dispv
                << " is main="
                << twores.main()
                << ", xtra="
                << twores.xtra()
                << std::endl
                << "--------- ending do_display_object -------\n\n");
} // end RpsQApplication::do_display_object


void
rps_run_application(int &argc, char **argv)
{
  rps_batch = false;
  std::string dumpdirstr;
  {
    char cwdbuf[128];
    memset (cwdbuf, 0, sizeof(cwdbuf));
    getcwd(cwdbuf, sizeof(cwdbuf)-1);
    RPS_INFORM("rps_run_application: start of %s\n"
               ".. gitid %s\n"
               ".. build timestamp %s\n"
               ".. last git commit %s\n"
               ".. md5sum %s\n"
               ".. in %s\n"
               ".. on host %s pid %d\n",
               argv[0], rps_gitid,
               rps_timestamp,
               rps_lastgitcommit,
               rps_md5sum,
               cwdbuf,
               rps_hostname(), (int)getpid());
  }
  /// early debugging if given as the first argument
  if (argc>2 && !strncmp("-d", argv[1], strlen("-d")))
    rps_set_debug(std::string(argv[1]+strlen("-d")));
  if (argc>2 && !strncmp("--debug=", argv[1], strlen("--debug=")))
    rps_set_debug(std::string(argv[1]+strlen("--debug=")));
  ///
  RpsQApplication app (argc, argv);
  RpsQApplication::app_mainqthread = QThread::currentThread();
  RpsQApplication::app_mainselfthread = pthread_self();
  RpsQApplication::app_mainthreadid = std::this_thread::get_id();
  QString displayedobjectqstr;
  std::string loadtopdir(rps_topdirectory);
  {
    QCommandLineParser argparser;
    argparser.setApplicationDescription("a REFlexive PERsistent SYStem - see refpersys.org");
    argparser.addHelpOption();
    argparser.addVersionOption();
    // refpersys home
    const QCommandLineOption rpshomeOption("refpersys-home",
                                           "RefPerSys homedir, default to $REFPERSYS_HOME or $HOME", "refpersys-home");
    argparser.addOption(rpshomeOption);

    // load directory
    const QCommandLineOption loadOption(QStringList() << "L" << "load",
                                        "The load directory", "load-dir");
    argparser.addOption(loadOption);
    // load directory
    const QCommandLineOption debugOption(QStringList() << "d" << "debug",
                                         "debugging messages, perhaps 'help' for debug-level to get their list",
                                         "debug-level");
    argparser.addOption(debugOption);
    // random oids
    const QCommandLineOption randoidOption("random-oid",
                                           "output some random oids", "nb-oids");
    argparser.addOption(randoidOption);
    // type information
    const QCommandLineOption typeOption("type-info", "Show type information.");
    argparser.addOption(typeOption);
    // use syslog
    const QCommandLineOption syslogOption("syslog", "Use syslog(3).");
    argparser.addOption(syslogOption);
    // disable ANSI escape codes even if output is a terminal
    const QCommandLineOption withoutTerminalOption("without-terminal", "Forcibly disable terminal escape code");
    argparser.addOption(withoutTerminalOption);
    // settings information
    const QCommandLineOption settingsOption("settings", "The Qt settings file.");
    argparser.addOption(settingsOption);
    // display a given object
    const QCommandLineOption displayOption(QStringList() << "display",
                                           "display a given object by oid or by name",
                                           "displayed-object");
    argparser.addOption(displayOption);
    // batch flag
    const QCommandLineOption batchOption(QStringList() << "B" << "batch", "batch mode, without any windows");
    argparser.addOption(batchOption);
    // ASLR flag, actually handled very early in main!
    const QCommandLineOption disableAslrOption("no-aslr", "disable Adress Space Layout Randomization");
    argparser.addOption(disableAslrOption);
    // number of jobs, for multi threading
    const QCommandLineOption nbjobOption(QStringList() << "j" << "jobs", "number of threads", "nb-jobs");
    argparser.addOption(nbjobOption);
    // dump after load
    const QCommandLineOption dumpafterloadOption(QStringList() << "D" << "dump", "dump after load", "dump-dir");
    argparser.addOption(dumpafterloadOption);
    //
    argparser.process(app);
    if (argparser.isSet(disableAslrOption))
      {
        RPS_ASSERT(rps_disable_aslr);
      }
    if (argparser.isSet(withoutTerminalOption))
      {
        rps_without_terminal_escape = true;
        RPS_INFORM("disabling ANSI escape codes as if without terminal");
      };
    ///// --refpersys-home <dir>
    if (argparser.isSet(rpshomeOption))
      {
        const QString rhomqs = argparser.value(rpshomeOption);
        std::string rhompath = rhomqs.toStdString();
        struct stat rhomstat;
        memset (&rhomstat, 0, sizeof(rhomstat));
        if (stat(rhompath.c_str(), &rhomstat))
          RPS_FATAL("failed to stat --refpersys-home %s: %m",
                    rhompath.c_str());
        if (!S_ISDIR(rhomstat.st_mode))
          RPS_FATAL("given --refpersys-home %s is not a directory",
                    rhompath.c_str());
        if ((rhomstat.st_mode & (S_IRUSR|S_IXUSR)) !=  (S_IRUSR|S_IXUSR))
          RPS_FATAL("given --refpersys-home %s is not user readable and executable",
                    rhompath.c_str());
        char*rhomrp = realpath(rhompath.c_str(), nullptr);
        if (!rhomrp)
          RPS_FATAL("realpath failed on given --refpersys-home %s - %m",
                    rhompath.c_str());
        if (strlen(rhomrp) >= sizeof(rps_bufpath_homedir) -1)
          RPS_FATAL("too long realpath %s on given --refpersys-home %s - %m",
                    rhomrp, rhompath.c_str());
        strncpy(rps_bufpath_homedir, rhomrp, sizeof(rps_bufpath_homedir) -1);
        free (rhomrp), rhomrp = nullptr;
      };
    if (argparser.isSet(syslogOption))
      {
        printf("RegPerSys uses syslog(3)\n");
        rps_syslog_enabled = true;
        openlog("RefPerSys", LOG_PERROR|LOG_PID, LOG_USER);
        RPS_INFORM("using syslog");
      }
    RPS_INFORM("using %s as the RefPerSys home directory", rps_homedir());
    //// --load <dir>
    if (argparser.isSet(loadOption))
      {
        const QString loadpathqs = argparser.value(loadOption);
        loadtopdir = loadpathqs.toStdString();
      };
    /// --type-info
    if (argparser.isSet(typeOption))
      {
        rps_print_types_info ();
        rps_batch = true;
      }
    /// --batch or -B
    if (argparser.isSet(batchOption))
      rps_batch = true;
    /// --random-oid <nbrand>
    if (argparser.isSet(randoidOption))
      {
        int nbrand = 5;
        const QString randoidqs = argparser.value(randoidOption);
        sscanf(randoidqs.toStdString().c_str(), "%d", &nbrand);
        if (nbrand <= 0) nbrand = 2;
        else if (nbrand > 100) nbrand = 100;
        RPS_INFORM("output of %d random objids generated on %.2f\n", nbrand,
                   rps_wallclock_real_time());
        printf("*    %-20s" "\t  %-19s" "   %-12s" "\t %-10s\n",
               " objid", "hi", "lo", "hash");
        printf("========================================================"
               "===========================\n");
        for (int ix = 0; ix<nbrand; ix++)
          {
            auto rid = Rps_Id::random();
            printf("! %22s" "\t  %19lld" " %12lld" "\t %10u\n",
                   rid.to_string().c_str(),
                   (long long) rid.hi(),
                   (long long) rid.lo(),
                   (unsigned) rid.hash());
          }
        printf("--------------------------------------------------------"
               "---------------------------\n");
        fflush(nullptr);
      };
    if (argparser.isSet(nbjobOption))
      {
        int nbjobs = 3;
        const QString nbjqs = argparser.value(nbjobOption);
        if (sscanf(nbjqs.toStdString().c_str(), "%d", &nbjobs) >= 1)
          {
            if (nbjobs <= RPS_NBJOBS_MIN)
              nbjobs = RPS_NBJOBS_MIN;
            else if (nbjobs > RPS_NBJOBS_MAX)
              nbjobs = RPS_NBJOBS_MAX;
          }
        else
          RPS_WARNOUT("invalid number of jobs (-j option) " << (nbjqs.toStdString()));
        rps_nbjobs = nbjobs;
      }
    ///// --debug <level>
    if (argparser.isSet(debugOption))
      {
        const QString debugqs = argparser.value(debugOption);
        auto debugstr = debugqs.toStdString();
        rps_set_debug(debugstr);
        if (rps_syslog_enabled && rps_debug_flags != 0)
          openlog("RefPerSys", LOG_PERROR|LOG_PID, LOG_USER);
      }
    ///// --dump <dump-dir>
    if (argparser.isSet(dumpafterloadOption))
      {
        const QString dumpqs = argparser.value(dumpafterloadOption);
        dumpdirstr = dumpqs.toStdString();
        RPS_INFORMOUT("should dump into " << dumpdirstr);
      }
    ///// --display <object-oid-or-name>
    if (argparser.isSet(displayOption))
      {
        const QString dispqs = argparser.value(displayOption);
        if (!displayedobjectqstr.isEmpty())
          RPS_FATALOUT("--display option should be passed once, but got both " << displayedobjectqstr.toStdString()
                       << " and " << dispqs.toStdString());
        displayedobjectqstr = dispqs;
        RPS_INFORMOUT("should display " << displayedobjectqstr.toStdString());
      }
    ///// --settings <ini-file>
    if (argparser.isSet(settingsOption))
      {
        const QString settingqs = argparser.value(settingsOption);
        auto settingstr = settingqs.toStdString();
        if (access(settingstr.c_str(), R_OK))
          RPS_FATAL("cannot access settings %s: %m", settingstr.c_str());
        app.app_settings = new QSettings(settingqs, QSettings::IniFormat);
      }
  }
  if (!app.app_settings)
    {
      std::string usersettings(rps_homedir());
      std::string defaultsettings(rps_topdirectory);
      usersettings += "/" RPS_QTSETTINGS_BASEPATH;
      defaultsettings += "/" RPS_QTSETTINGS_BASEPATH;
      if (!access(usersettings.c_str(), R_OK))
        {
          app.app_settings = new QSettings(QString(usersettings.c_str()), QSettings::IniFormat);
          RPS_INFORMOUT("using user Qt settings from " << usersettings);
        }
      else if (!access(defaultsettings.c_str(), R_OK))
        {
          app.app_settings = new QSettings(QString(defaultsettings.c_str()), QSettings::IniFormat);
          RPS_INFORMOUT("using default Qt settings from " << defaultsettings);
        }
      else
        RPS_FATALOUT("No Qt settings found in " << usersettings << " or " << defaultsettings);
    }
  RPS_INFORMOUT("using " << rps_nbjobs << " jobs (or threads)");
  rps_load_from (loadtopdir);
  RPS_DEBUG_LOG(MISC, "rps_run_application after loading from " << loadtopdir);
  if (!dumpdirstr.empty())
    {
      RPS_INFORMOUT("RefPerSys dumping after load to " << dumpdirstr);
      rps_dump_into(dumpdirstr);
      RPS_INFORMOUT("RefPerSys dumped after load to " << dumpdirstr << "========"
                    << std::endl);
    }
  ///
  if (rps_disable_aslr)
    {
      // if any debug flag is set, we show the address space by running
      // http://man7.org/linux/man-pages/man1/pmap.1.html
      if (rps_debug_flags)
        {
          char pmapout[64];
          memset (pmapout, 0, sizeof(pmapout));
          snprintf(pmapout, sizeof(pmapout), "_refpersys°%.8sPmap_%.30s-p%ld.tmp", rps_gitid, rps_hostname(),  (long)getpid());
          RPS_INFORMOUT("Address Space Layout Randomization is disabled! running pmap with output to " << pmapout);
          fflush(nullptr);
          char pmapcmdbuf[80];
          memset (pmapcmdbuf, 0, sizeof(pmapcmdbuf));
          snprintf(pmapcmdbuf, sizeof(pmapcmdbuf), "/usr/bin/pmap -p %ld > %s", (long)getpid(), pmapout);
          int badpmap = system(pmapcmdbuf);
          if (badpmap)
            RPS_FATALOUT("command " << pmapcmdbuf << " failed -> status " << badpmap);
        }
      else
        RPS_INFORMOUT("Address Space Layout Randomization is disabled for pid " << (long) getpid());
    }
  if (!rps_batch)
    {
      RPS_INFORMOUT("running the GUI since no batch mode");
      app.initialize_app();
      RPS_DEBUG_LOG(GUI, "rps_run_application before RpsQOutputTextEdit::initialize");
      RpsQOutputTextEdit::initialize();
      if (!displayedobjectqstr.isEmpty())
        {
          RPS_DEBUG_LOG(GUI, "rps_run_application should display: " << displayedobjectqstr.toStdString()
                        << std::endl);
          app.do_display_object(displayedobjectqstr);
        }
      RPS_DEBUG_LOG(GUI, "rps_run_application before app.exec");
      (void) app.exec ();
    }
  else
    RPS_INFORMOUT("RefPerSys GUI was not run in batch mode");
  RPS_DEBUG_LOG(GUI, "rps_run_application ending");
} // end of rps_run_application

bool
rps_is_main_gui_thread(void)
{
  return pthread_self() == RpsQApplication::app_mainselfthread
         ||  RpsQApplication::app_mainqthread == QThread::currentThread()
         ||  RpsQApplication::app_mainthreadid == std::this_thread::get_id();
} // end rps_is_main_gui_thread

//////////////// moc generated file
#include "_qthead_qrps.inc.hh"

/// we are explicitly instanciating this template....
template class Rps_PayloadQt<RpsQWindow>;
//////////////////////////////////////// end of file appli_qrps.cc
