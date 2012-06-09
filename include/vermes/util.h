/*
 * Copyright (C) 2010 Markus Wildi, version for RTS2
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
 *
 */

#ifndef __util_h__
#define __util_h__

/* Constants defining indi_log priorities */
#define ILOG_DEBUG      1
#define ILOG_INFO       2
#define ILOG_NOTICE     4
#define ILOG_WARNING    8
#define ILOG_ERR       16
#define ILOG_CRIT      32
#define ILOG_ALERT     64
#define ILOG_EMERG    128

/*****************************************************************************
 * xrealloc(void *ptr, size_t size)
 * Allocates or resizes storage space. New space is allocated when ptr == NULL.
 *****************************************************************************/
void * xrealloc(void *ptr, size_t size);

/*****************************************************************************
 * concat(const char *str, ...)
 * Concats the given strings to a new allocated memory block and return a
 * pointer to it. Last string argument must be NULL.
 * (Taken from Gnu libc documentation.)
 *****************************************************************************/
char * concat(const char *str, ...);

/******************************************************************************
 * catf(...)
 * Appends the given formatted string to the given string and reallocates the
 * storage for it, if more space is needed.
 * Return:
 *   The pointer to the appended string.
 *****************************************************************************/
char * catf(char * str, char * format, ...);

/** \brief Sleeps for given number of milliseconds by using nanosleep().
 *          Maximum sleep time is 1000 ms.
 *****************************************************************************/
void millisleep(int ms);

/** \brief  Expands leading ~ or ~user to the user's home directory, and
 *          return a new malloced string.  If no ~, or no <pwd.h>, just return 
 *          the given name.
 *****************************************************************************/
const char * tilde_expand(const char * name);

/** \brief Creates syslog entries (if preprocessor define USE_SYSLOG) with the
           given priority and prints the message to stderr.
    \param prio Priority of the log message. Will get translated to a syslog
                priority.
    \param format Format string like those used with printf() and friends.
    \param ...
 *****************************************************************************/
void sys_log(int prio, char * format, ...);

/** \brief Creates syslog entries for debugging (if preprocessor define
           USE_SYSLOG)* and prints the message to stderr depending on the
           global debug variable.
    \param level Importance level of message. Increased level is more verbose.
    \param format Format string like those used with printf() and friends.
    \param ...
 *****************************************************************************/
void sys_debug_log(int level, char * format, ...);


char * ctrl2hex(char *str);

#endif   // #ifndef __util_h__


