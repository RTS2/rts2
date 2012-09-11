/*
 *
 * Copyright (C) 2010 Markus Wildi, version for RTS2, minor adaptions
 * Copyright (C) 2009 Lukas Zimmermann, Basel, Switzerland
 *
 *
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Or visit http://www.gnu.org/licenses/gpl.html.

 */

/*****************************************************************************
 * Collection of utility functions  which might be useful for other indi
 * drivers.
 *****************************************************************************/

/* _GNU_SOURCE is needed for mempcpy which is a GNU libc extension */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if USE_SYSLOG
#include <syslog.h>
#endif

#ifdef RTS2_HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef WIN32
#define HOMEVAR "USERPROFILE"
#define DIR_SEP '\\'
#else
#define HOMEVAR "HOME"
#define DIR_SEP '/'
#endif

#ifndef IS_DIR_SEP
#define IS_DIR_SEP(ch) ((ch) == DIR_SEP)
#endif

#include <vermes/util.h>

#if USE_SYSLOG
int syslog_opened = 0;
#endif

int sys_log_mask = 0;
char * log_string = NULL;

extern char * me;
extern int debug;


/*****************************************************************************
 * concat(const char *str, ...)
 * Concats the given strings to a new allocated memory block and returns a
 * pointer to it. Last string argument must be NULL.
 * (Taken from Gnu libc documentation.)
 *****************************************************************************/
char *
concat(const char *str, ...) {

  va_list ap;
  size_t allocated = 100;
  const char *s;
  char *result = (char *) realloc(NULL, allocated);

  if (result != NULL) {
    char *newp;
    char *wp;

    va_start(ap, str);

    wp = result;
    for (s = str; s != NULL; s = va_arg(ap, const char *)) {
      size_t len = strlen (s);

      /* Resize the allocated memory if necessary.  */
      if (wp + len + 1 > result + allocated) {
        allocated = (allocated + len) * 2;
        newp = (char *) realloc(result, allocated);
        if (newp == NULL) {
          free(result);
          return NULL;
        }
        wp = newp + (wp - result);
        result = newp;
      }

      wp = mempcpy(wp, s, len);
    }

    /* Terminate the result string.  */
    *wp++ = '\0';

    /* Resize memory to the optimal size.  */
    newp = realloc(result, wp - result);
    if (newp != NULL) result = newp;

    va_end(ap);
  }

  return result;
}

/******************************************************************************
 * millisleep(...)
 * Sleeps for given number of milliseconds by using nanosleep(). Maximum
 * sleep time is 1000 ms.
 *****************************************************************************/
 void 
 millisleep(int ms) 
 { 
   struct timespec req_ts, remain_ts; 
   if (ms < 0 || ms > 1000) ms = 1000; 
   req_ts.tv_sec = 0; 
   req_ts.tv_nsec = 1000000 * ms; 
   nanosleep(&req_ts, &remain_ts); 
 } 

/******************************************************************************
 * If NAME has a leading ~ or ~user, Unix-style, expand it to the user's
 *  home directory, and return a new malloced string.  If no ~, or no
 *  <pwd.h>, just return NAME.
 * 
 * Adapted from kpathsea library kpathsea/tilde.c
 *   Copyright 1997, 1998, 2005, Olaf Weber.
 *   Copyright 1993, 1995, 1996, 1997, 2008 Karl Berry.
 *   licenced under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of the
 *   License, or (at your option) any later version.
 *
 *****************************************************************************/
const char *
tilde_expand(const char * name)
{
  const char * expansion;
  const char * home;

  /* If no leading tilde, do nothing, and return the original string.  */
  if (*name != '~') {
    expansion = name;

  /* If a bare tilde, return the home directory or `.'.  (Very unlikely
     that the directory name will do anyone any good, but ...  */
  } else if (name[1] == 0) {
    home = getenv(HOMEVAR);
    if (!home) {
      home = ".";
    }
    expansion = home;

  /* If `~/', remove any trailing / or replace leading // in $HOME.
     Should really check for doubled intermediate slashes, too.  */
  } else if (IS_DIR_SEP (name[1])) {
    unsigned c = 1;
    home = getenv(HOMEVAR);
    if (!home) {
      home = ".";
    }
    if (IS_DIR_SEP (*home) && IS_DIR_SEP (home[1])) {  /* handle leading // */
      home++;
    }
    if (IS_DIR_SEP (home[strlen (home) - 1])) {        /* omit / after ~ */
      c++;
    }
    expansion = concat(home, name + c, NULL);

  /* If `~user' or `~user/', look up user in the passwd database (but
     OS/2 doesn't have this concept.  */
  } else {
#ifdef RTS2_HAVE_PWD_H
      struct passwd *p;
      char * user;
      unsigned c = 2;
      while (!IS_DIR_SEP(name[c]) && name[c] != 0) /* find user name */
        c++;
      
      user = (char *)malloc(c);
      strncpy(user, name + 1, c - 1);
      user[c - 1] = 0;
      
      /* We only need the cast here for (deficient) systems
         which do not declare `getpwnam' in <pwd.h>.  */
      p = (struct passwd *)getpwnam(user);
      free(user);

      /* If no such user, just use `.'.  */
      home = p ? p->pw_dir : ".";
      if (IS_DIR_SEP(*home) && IS_DIR_SEP(home[1])) { /* handle leading // */
        home++;
      }
      if (IS_DIR_SEP(home[strlen(home) - 1]) && name[c] != 0)
        c++; /* If HOME ends in /, omit the / after ~user. */

      expansion = concat(home, name + c, NULL);
#else /* not RTS2_HAVE_PWD_H */
      /* Since we don't know how to look up a user name, just return the
         original string. */
      expansion = name;
#endif /* not RTS2_HAVE_PWD_H */
  }

  /* We may return the same thing as the original, and then we might not
     be returning a malloc-ed string.  Callers beware.  Sorry.  */
  return expansion;
}

/******************************************************************************
 * ctrl2hex(...)
 * Takes the given string and replaces all non-printing characters by a hexa-
 * decimal representation. The created string gets dynamically allocated and
 * returned. The allocated memory must be explicitly freed, when the returned
 * string is not needed anymore.
 *****************************************************************************/
char *
ctrl2hex(char *str)
{
  int str_l = strlen(str);
  int hex_l = str_l;
  char *hex_str = (char *)realloc(NULL, str_l + 1);
  int i;
  int pos = 0;
  for (i = 0; i < str_l; i++) {
    if (str[i] >= ' ') {
      hex_str[pos++] = str[i];
    } else {
      if (str[i] == '\0') {
        hex_str[pos] = '\0';
        return hex_str;
      }
      hex_l += 6;
      hex_str = (char *)realloc(hex_str, hex_l);
      sprintf(&hex_str[pos], "(0x%02x)", str[i]);
      pos += 6;
    }
  }
  hex_str[pos] = '\0';
  return hex_str;
}

/******************************************************************************
 * sprintfv(...)
 * Creates a string from the variadic arguments in the way of vsprintf() but
 * allocates the memory for it if needed.
 *****************************************************************************/
char*
sprintfv(char* str, char* format, va_list ap)
{
  va_list cp_ap;
  size_t new_len;
  size_t s_len = str == NULL ? 0 : strlen(str) + 1;
    
#ifdef __va_copy
  __va_copy(cp_ap, ap);
#else
  cp_ap = ap;
#endif
  /* check out whether the current string buffer size is sufficent */
  new_len = vsnprintf(str, s_len, format, ap);
  if (debug > 4) {
    fprintf(stderr, "str: %p, cur len: %ld, new len: %ld.\n", str, (long int)s_len, (long int)new_len);
  }
  if (new_len < 0) {
    sys_debug_log(0, "sprintfv(): could not determine formatted length.");
    return NULL;
  }

  if (new_len >= s_len) {
    /* Reallocate buffer now that we know how much space is needed. */
    new_len++;
    str = realloc(str, new_len);

    if (str != NULL) {
      /* print again. */
      vsnprintf(str, new_len, format, cp_ap);
      if (debug > 4) {
        fprintf(stderr, "str: %p (\"%s\"), new len: %ld.\n", str, str, (long int)new_len);
      }
    } else {
      sys_log(ILOG_ERR, "sprintfv(): realloc failed.");
    }
  }

  return str;
}

/******************************************************************************
 * catfv(...)
 *****************************************************************************/
char*
catfv(char* str, char* format, va_list ap)
{
  va_list ap2;
  size_t new_size;
  int len;
  int add_len;

#ifdef __va_copy
  __va_copy(ap2, ap);
#else
  ap2 = ap;
#endif
  len = str == NULL ? 0 : strlen(str);
  if (debug > 4) {
    fprintf(stderr, "str: %p, len: %d\n", str, len);
  }
  /* check out whether the current string buffer size is sufficent */
  add_len = vsnprintf(str + len, 0, format, ap);
  if (debug > 4) {
    fprintf(stderr, "addlen: %d\n", add_len);
  }

  /* Reallocate buffer now that we know how much space is needed. */
  new_size = len + add_len;
  str = realloc(str, new_size);
  if (debug > 4) {
    fprintf(stderr, "str: %p, addlen: %d, new_size: %ld\n",
                     str, add_len, (long int)new_size);
  }
  if (str != NULL) {
    /* print again. */
    vsnprintf(str + len, add_len + 1, format, ap2);
  } else {
    sys_log(ILOG_ERR, "catfv(): realloc failed.");
  }

  return str;
}

/******************************************************************************
 * catf(...)
 * Appends the given formatted string to the given string and reallocates the
 * storage for it, if more space is needed.
 * Return:
 *   The pointer to the appended string.
 *****************************************************************************/
char *
catf(char * str, char * format, ...)
{
  va_list ap;

  va_start(ap, format);
  return catfv(str, format, ap);
}

/******************************************************************************
 * sys_log(...)
 * Creates syslog entries (if preprocessor define USE_SYSLOG) with the given
 * priority and prints the message to stderr.
 *****************************************************************************/
void
sys_log(int prio, char * format, ...)
{
  va_list ap;
  char * s;

#if USE_SYSLOG
  if (!syslog_opened) {
    openlog(me, LOG_PERROR | LOG_CONS | LOG_PID, LOG_LOCAL0);
    syslog_opened = 1;
  }
  va_start(ap, format);
  s = sprintfv(log_string, format, ap);
  va_end(ap);

  if (s) {
    int lvl = LOG_DEBUG;
    switch (prio) {
      case ILOG_DEBUG:
        lvl = LOG_DEBUG;
        break;
      case ILOG_INFO:
        lvl = LOG_INFO;
        break;
      case ILOG_NOTICE:
        lvl = LOG_NOTICE;
        break;
      case ILOG_WARNING:
        lvl = LOG_WARNING;
        break;
      case ILOG_ERR:
        lvl = LOG_ERR;
        break;
      case ILOG_CRIT:
        lvl = LOG_CRIT;
        break;
      case ILOG_ALERT:
        lvl = LOG_ALERT;
        break;
      case ILOG_EMERG:
        lvl = LOG_EMERG;
        break;
    }
    syslog(lvl, s);
    if (prio & sys_log_mask) {
      fprintf(stderr, "%s\n", s);
    }
  } else {
    fprintf(stderr, "failed creating log string\n");
  }
#else
  va_start(ap, format);
  s = sprintfv(log_string, format, ap);
  va_end(ap);

  if (s) {
    if (prio & sys_log_mask) {
      fprintf(stderr, "%s\n", s);
    }
  } else {
    fprintf(stderr, "failed creating log string\n");
  }
#endif
}

/******************************************************************************
 * sys_debug_log(...)
 * Creates syslog entries for debugging (if preprocessor define USE_SYSLOG)
 * and prints the message to stderr depending on the global debug variable.
 *****************************************************************************/
void
sys_debug_log(int level, char * format, ...)
{
  va_list ap;
  char * s;

  if (level < debug) {
#ifdef USE_SYSLOG
    if (!syslog_opened) {
      openlog(me, LOG_PERROR | LOG_CONS | LOG_PID, LOG_LOCAL0);
      syslog_opened = 1;
    }
    va_start(ap, format);
    s = sprintfv(log_string, format, ap);
    va_end(ap);

    if (s) {
      syslog(LOG_DEBUG, s);
      fprintf(stderr, "%s\n", s);
    } else {
      fprintf(stderr, "failed creating log string\n");
    }
#else
    va_start(ap, format);
    s = sprintfv(log_string, format, ap);
    va_end(ap);
    if (s) {
      fprintf(stderr, "%s\n", s);
    } else {
      fprintf(stderr, "failed creating log string\n");
    }
#endif
  }
}


