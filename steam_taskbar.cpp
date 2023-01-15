/*
  Copyright (c) 2023 djcj <djcj@gmx.de>

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions: 

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software. 

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#include <FL/Fl.H>
#include <FL/Fl_Single_Window.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/fl_ask.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <unistd.h>

#ifdef USE_DLOPEN
/* header file created with gendlopen (https://github.com/darealshinji/gendlopen):
 * echo "bool SteamAPI_IsSteamRunning();" | gendlopen -i- -llibsteam_api.so -tcpp-static > steamapi_dlopen.h
 */
#include "steamapi_dlopen.h"
#else
/* libsteam_api.so */
extern "C" bool SteamAPI_IsSteamRunning();
#endif

#define WINDOW_TITLE       "Steam"
#define STEAM_COMMAND      "steam"  /* /usr/games/steam */
#define PROGNAME           "steam_taskbar"
#define ICON_FILE          "steam.png"
#define ICON_FILE_DEFAULT  "/usr/share/pixmaps/steam.png"
#define REPEAT_INTERVAL    1.0 /* seconds */
#define MAXPATH            4096


/* glibc */
extern const char *__progname;


class MyWindow : public Fl_Single_Window
{
private:
  static Fl_PNG_Image *m_icon;
  static bool m_timeout;
  static bool m_idle;
  static void taskbar_handler(void *);
  static void pidcheck_handler(void *);

  int handle(int event);

public:
  MyWindow(int W, int H, const char *L, bool idle)
  : Fl_Single_Window(W,H,L)
  {
    m_idle = idle;
    Fl::add_timeout(0.5, pidcheck_handler, reinterpret_cast<void *>(this));
  }

  virtual ~MyWindow() {
    if (m_icon) delete m_icon;
  }

  static void set_window_decoration();
};

class MySteam
{
private:
  MyWindow *m_win = NULL;

public:
  MySteam(bool idle);

  ~MySteam() {
    if (m_win) delete m_win;
  }

  int launch();
};

Fl_PNG_Image *MyWindow::m_icon = NULL;
bool MyWindow::m_timeout = false;
bool MyWindow::m_idle = false;


void MyWindow::taskbar_handler(void *)
{
  if (m_idle && !SteamAPI_IsSteamRunning()) {
    m_timeout = false;
    return;
  }

  m_idle = false;

  if (m_timeout && SteamAPI_IsSteamRunning()) {
    int rv = system(STEAM_COMMAND); /* pop-up Steam window */
    (void)rv; /* unused */
  }
  m_timeout = false;
}

void MyWindow::pidcheck_handler(void *o)
{
  if (m_idle && SteamAPI_IsSteamRunning()) {
    m_idle = false;
  }

  if (!m_idle && !SteamAPI_IsSteamRunning()) {
    reinterpret_cast<MyWindow *>(o)->hide();
  }

  Fl::repeat_timeout(REPEAT_INTERVAL, pidcheck_handler, o);
}

int MyWindow::handle(int event)
{
  if (event == FL_NO_EVENT) return 0;

  if (shown()) {
    /* do this timeout stuff to not call
     * system() multiple times in a row */
    m_timeout = true;
    Fl::add_timeout(0.2, taskbar_handler);
    iconize();
  }

  return Fl_Window::handle(event);
}

void MyWindow::set_window_decoration()
{
  char buf[MAXPATH];
  char path[MAXPATH] = {0};

  /* look for icon next to executable */
  ssize_t n = readlink("/proc/self/exe", buf, MAXPATH);
  if (n > 0 && n < MAXPATH) {
    char *dir = dirname(buf);
    if (dir && *dir && (strlen(dir) + sizeof(ICON_FILE) + 2) < MAXPATH) {
      sprintf(path, "%s/" ICON_FILE, dir);
    }
  }

  if (*path) m_icon = new Fl_PNG_Image(path);

  if (m_icon && m_icon->fail()) {
    delete m_icon;
    m_icon = NULL;
  }

  if (!m_icon) m_icon = new Fl_PNG_Image(ICON_FILE_DEFAULT);

  if (m_icon && !m_icon->fail()) {
    Fl_Window::default_icon(m_icon);
  }

  Fl::get_system_colors();
  Fl::scheme("gtk+");
  fl_message_title(WINDOW_TITLE);
}

MySteam::MySteam(bool idle)
{
  MyWindow::set_window_decoration();

  if (!idle && !SteamAPI_IsSteamRunning()) {
    fl_alert("Steam is not running");
    return;
  }

  /* launch window */
  m_win = new MyWindow(50, 20, WINDOW_TITLE, idle);
  m_win->iconize();
}

static void print_help()
{
  MyWindow::set_window_decoration();

  fl_message(
    "Keep an additional \"Steam\" entry in your taskbar.\n"
    "This should help to restore the Steam window on desktops\n"
    "where the tray icon is not present.\n"
    "\n"
    "usage:\n"
    "%s [--idle]\n"
    "%s --help\n",
    __progname,
    __progname);
}

int main(int argc, char *argv[])
{
  if (argc > 1) {
    if (strcmp(argv[1], "--help") == 0) {
      print_help();
      return 0;
    } else if (strcmp(argv[1], "--idle") != 0) {
      print_help();
      return 1;
    }
  }

#ifdef USE_DLOPEN
  gendlopen dlo;

  if (!(dlo.load() && dlo.sym())) {
    MyWindow::set_window_decoration();
    fl_alert("%s", dlo.error());
    return 1;
  }
#endif

  bool idle = false;

  if ((argc > 1 && strcmp(argv[1], "--idle") == 0) ||
      strcmp(__progname, PROGNAME "_idle") == 0)
  {
    idle = true;
  }

  MySteam steam(idle);

  return Fl::run();
}
