/**
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * For more information, please refer to <http://unlicense.org/
 */
#ifndef GENDLOPEN_H
#define GENDLOPEN_H

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <dlfcn.h>

#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/* Convenience macro to create versioned library names for DLLs, dylibs
 * and DSOs. LIB(z,1) for example will be resolved to either libz-1.dll,
 * libz.1.dylib or libz.so.1
 */
#ifndef LIB
#  ifdef _WIN32
#    define LIB(NAME, API)  "lib" #NAME "-" #API ".dll"
#  elif __APPLE__
#    define LIB(NAME, API)  "lib" #NAME "." #API ".dylib"
#  else
#    define LIB(NAME, API)  "lib" #NAME ".so." #API
#  endif
#endif

/* shared library file extension (useful i.e. on plugins) */
#ifndef LIBEXT
#  ifdef _WIN32
#    define LIBEXT "dll"
#  elif __APPLE__
#    define LIBEXT "dylib"
#  else
#    define LIBEXT "so"
#  endif
#endif


class gendlopen
{
private:
  char *m_error = NULL;
  char *m_origin = NULL;
  const char *m_lib = "libsteam_api.so";

  /* needed for dladdr() */
  void *m_lastsym = NULL;

  static void *m_handle;
  enum { DEFAULT_FLAGS = RTLD_LAZY };

  int m_flags = DEFAULT_FLAGS;

  /* save an error message, providing an alternative message */
  void save_error(const char *msg, const char *alt_msg) {
    clear_error();
    m_error = strdup(msg ? msg : alt_msg);
  }

  /* clear error message */
  void clear_error() {
    if (m_error) free(m_error);
    m_error = NULL;
  }

  /* load symbol and cast to fnptr */
  bool load_sym(void **fnptr, const char *symbol)
  {
    clear_error();

    if ((m_lastsym = dlsym(m_handle, symbol)) == NULL) {
      save_error(dlerror(), "dlsym(): unknown error");
      return false;
    }

    *fnptr = m_lastsym;

    return true;
  }

public:
  /* default c'tor */
  gendlopen()
  {}

  /* c'tor with arguments */
  gendlopen(const char *filename, int flags=DEFAULT_FLAGS)
  {
    m_lib = filename;
    m_flags = flags;
  }

  /* d'tor */
  virtual ~gendlopen()
  {
    lib_free();
    if (m_error) free(m_error);
    if (m_origin) free(m_origin);
  }

  /* load library */
  bool load()
  {
    clear_error();

    if ((m_handle = dlopen(m_lib, m_flags)) == NULL) {
      save_error(dlerror(), "dlopen(): unknown error");
      return false;
    }

    return true;    
  }

  /* load library with arguments */
  bool load(const char *filename, int flags=DEFAULT_FLAGS) {
    m_lib = filename;
    m_flags = flags;
    return load();
  }

  /* load from a list of filenames */
  bool load(std::list<const char *> list, int flags=DEFAULT_FLAGS)
  {
    for (const auto &e : list) {
      if (load(e, flags)) {
        return true;
      }
    }
    return false;
  }

  /* free library */
  bool lib_free()
  {
    clear_error();

    if (!m_handle) return true;

    if (dlclose(m_handle) != 0) {
      save_error(dlerror(), "dlclose(): unknown error");
      return false;
    }

    return true;
  }

  /* load all symbols */
  bool sym()
  {
    if (!SteamAPI_IsSteamRunning && !load_sym((void **)&SteamAPI_IsSteamRunning, "SteamAPI_IsSteamRunning")) {
      return false;
    }

    clear_error();

    return true;
  }

  /* return path to loaded library */
  const char *origin()
  {
    if (m_origin) return m_origin;

    Dl_info dli;

    if (!m_lastsym) {
      save_error("dladdr(): address pointer is NULL", NULL);
      return NULL;
    }

    if (dladdr(m_lastsym, &dli) == 0) {
      save_error("dladdr(): unknown error", NULL);
      return NULL;
    }

    m_origin = strdup(dli.dli_fname);

    return m_origin;
  }

  /* return the last saved error message */
  const char *error() const {
    return m_error;
  }

  /* library name */
  const char *lib() const {
    return m_lib;
  }

  /* function pointers */
  static bool (*SteamAPI_IsSteamRunning)(void);
};

/* link against the static class member functions */
#define SteamAPI_IsSteamRunning gendlopen::SteamAPI_IsSteamRunning

void *gendlopen::m_handle = NULL;

/* function pointers */
bool (*gendlopen::SteamAPI_IsSteamRunning)(void) = NULL;

#endif //GENDLOPEN_H
