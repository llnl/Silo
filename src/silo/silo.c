/*
Copyright (C) 1994-2016 Lawrence Livermore National Security, LLC.
LLNL-CODE-425250.
All rights reserved.

This file is part of Silo. For details, see silo.llnl.gov.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the disclaimer below.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the disclaimer (as noted
     below) in the documentation and/or other materials provided with
     the distribution.
   * Neither the name of the LLNS/LLNL nor the names of its
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
"AS  IS" AND  ANY EXPRESS  OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A  PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN  NO  EVENT SHALL  LAWRENCE
LIVERMORE  NATIONAL SECURITY, LLC,  THE U.S.  DEPARTMENT OF  ENERGY OR
CONTRIBUTORS BE LIABLE FOR  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR  CONSEQUENTIAL DAMAGES  (INCLUDING, BUT NOT  LIMITED TO,
PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS  OF USE,  DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER  IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING
NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT  OF THE USE  OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This work was produced at Lawrence Livermore National Laboratory under
Contract No.  DE-AC52-07NA27344 with the DOE.

Neither the  United States Government nor  Lawrence Livermore National
Security, LLC nor any of  their employees, makes any warranty, express
or  implied,  or  assumes  any  liability or  responsibility  for  the
accuracy, completeness,  or usefulness of  any information, apparatus,
product, or  process disclosed, or  represents that its use  would not
infringe privately-owned rights.

Any reference herein to  any specific commercial products, process, or
services by trade name,  trademark, manufacturer or otherwise does not
necessarily  constitute or imply  its endorsement,  recommendation, or
favoring  by  the  United  States  Government  or  Lawrence  Livermore
National Security,  LLC. The views  and opinions of  authors expressed
herein do not necessarily state  or reflect those of the United States
Government or Lawrence Livermore National Security, LLC, and shall not
be used for advertising or product endorsement purposes.
*/


/* File-wide modifications:
 *
 *  Sean Ahern, Mon Mar 3 15:38:51 PST 1997 Rearranged most functions, adding
 *  local storage of the return value, to facilitate instrumenting each
 *  function.  (e.g. timing routines)
 *
 *  Sean Ahern, Thu Apr 29 15:41:27 PDT 1999
 *  Made all function definitions ANSI.  Removed unused local variables.
 *
 *  Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *  Added a global _uzl structure.
 *
 *  Brad Whitlock, Mon Apr 8 15:17:17 PST 2002
 *  Changed some headers to allow compilation under windows.
 *
 *  Thomas R. Treadway, Fri Jan  5 13:46:26 PST 2007
 *  Added backward compatible symbols
 */

#ifndef _WIN32
#warning SIMPLIFY HEADER INCLUSION LOGIC
#endif

/* Private SILO functions.  */
#include "config.h" /* For a possible redefinition of setjmp/longjmp.
                       Also for SDX driver detection.  */
#include <stdio.h>
#include <float.h>
#include <math.h>
#if HAVE_STDLIB_H
#include <stdlib.h>         /* For abort(). */
#endif
#if !defined(_WIN32)
#include <sys/file.h>       /* For R_OK and F_OK definitions. */
#endif
#include <errno.h>          /* For errno definitions. */
#if HAVE_STRING_H
#include <string.h>         /* For strerror */
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if !defined(_WIN32)
#include <sys/errno.h>      /* For errno definitions. */
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
    #if SIZEOF_OFF64_T > 4 && defined(__APPLE__)
        #define _DARWIN_USE_64_BIT_INODE
    #endif
    #include <sys/stat.h>
#endif
#include <ctype.h>          /* For isalnum */
#if HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>      /* for O_RDONLY */
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>          /* for O_RDONLY */
#endif
#ifdef _WIN32
#include <windows.h>        /* for FileInfo funcs */
#include <io.h>             /* for FileInfo funcs */
#endif

#include <stdarg.h>

/* DB_MAIN must be defined before including silo_private.h. */
#define DB_MAIN
#include "silo_private.h"
#include "silo_drivers.h"

/* The Silo_version_* variable is used to guarantee that code can't include
 * one version of silo.h and link with a different version of libsilo.a.  This
 * variable's name must change with every version of Silo.
 *
 * I would ordinary have silo.h be generated by configure so that the
 * version number will automatically get compiled in.  But this isn't good
 * for development with clearmake, since that would make silo.h be a
 * view-private file.  Thus, any object that depends on silo.h would be
 * invalid as a wink-in candidate.  */
int SILO_VERS_TAG = 0;

/* Specify versions which are backward compatible with the current. */
/* No lines of  the form 'int Silo_version_Maj_Min_Pat = 0;' below
   here indicates that this version is not backwards compatible with
   any previous versions.*/
int Silo_version_4_12_0_pre1 = 0;
int Silo_version_4_12_0_pre2 = 0;

/* Symbols for error handling */
PUBLIC int     DBDebugAPI = 0;  /*file desc for API debug messages      */
PUBLIC int     db_errno = 0;    /*last error number                     */
PUBLIC char    db_errfunc[64];  /*name of erring function               */
PUBLIC char   *_db_err_list[] =
{
    "No error",                               /*00 */
    "Bad file format type",                   /*01 */
    "Not implemented",                        /*02 */
    "File not found or invalid permissions",  /*03 */
    "<<Reserved>>",                           /*04 */
    "Internal error",                         /*05 */
    "Not enough memory",                      /*06 */
    "Invalid argument",                       /*07 */
    "Low-level function call failed",         /*08 */
    "Object not found",                       /*09 */
    "Taurus database state error",            /*10 */
    "Too many server connections",            /*11 */
    "Protocol error",                         /*12 */
    "Not a directory",                        /*13 */
    "Too many open files",                    /*14 */
    "Requested filter(s) not found",          /*15 */
    "Too many filters registered",            /*16 */
    "File already exists",                    /*17 */
    "Specified file is actually a directory", /*18 */
    "File lacks read permission",             /*19 */
    "System level error occured",             /*20 */
    "File lacks write permission",            /*21 */
    "Invalid variable name - only alphanumeric and `_'", /* 22 */
    "Overwrite not allowed. See DBSetAllowOverwrites()", /* 23 */
    "Checksum failure.",                      /* 24 */
    "Compression failure.",                   /* 25 */
    "Grab driver enabled.",                   /* 26 */
    "File was closed or never opened/created.",/* 27 */
    "File multiply opened w/>1 not read-only.", /* 28 */
    "Specified driver cannot open this file.",/* 29 */
    "Optlist contains options for wrong class.",/* 30 */
    "Feature not enabled in this build.", /* 31 */
    "Too many file options sets (missing DBUnregisterFileOptionsSet?).", /* 32 */
    "\nYou have tried to open or create a Silo file using\n"
    "the HDF5 driver. However, the installation of Silo\n"
    "you are using does not have the HDF5 driver enabled.\n"
    "You need to configure the Silo library using the\n"
    "--with-hdf5=<INC,LIB> option and re-compile and\n"
    "re-install Silo. If you do not have an installation\n"
    "of HDF5 already on your system, you will also need\n"
    "to obtain HDF5 from www.hdfgroup.org and install it.", /* 33 */
    "Empty objects not permitted. See DBSetAllowEmptyObjects().", /* 34 */
    "No more tiny array buffer space for custom object.", /* 35 */
    "Although this appears to be an HDF5 file,\n"
    "it does not appear to be one produced by Silo\n"
    "and so cannot be open and read by Silo." /* 36 */
};

/* Table of contents object count */
static int _db_nobj_types = ((int)(sizeof(DBtoc)/sizeof(struct{char **p;int n;})));

PRIVATE unsigned char _db_fstatus[DB_NFILES];  /*file status  */
typedef struct reg_status_t {
    DBfile *f;
    unsigned int n;
    int w;
} reg_status_t;
PRIVATE reg_status_t _db_regstatus[DB_NFILES] = /* DB_NFILES triples of zeros */
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

PRIVATE filter_t _db_filter[DB_NFILTERS];
#ifndef _WIN32
#warning REDUCE USE OF THIS CONSTRUCT
#endif
const static char *api_dummy = 0;

/* stat struct definition */
typedef struct db_silo_stat_t {
#ifndef SIZEOF_OFF64_T
#error missing definition for SIZEOF_OFF64_T in silo_private.h
#else
#if SIZEOF_OFF64_T > 4 && (defined(HAVE_STAT64) || !defined(HAVE_STAT))
    struct stat64 s;
#else
    struct stat s;
#endif
#endif
#ifdef _WIN32
    DWORD fileindexlo;
    DWORD fileindexhi;
#endif
} db_silo_stat_t;

/* Forward declarations */
PRIVATE int db_isregistered_file(DBfile *dbfile, const db_silo_stat_t *filestate);

/* Global structures for option lists.  */
struct _ma     _ma;
struct _ms     _ms;
struct _csgm   _csgm;
struct _pm     _pm;
struct _qm     _qm;
struct _um     _um;
struct _uzl    _uzl;
struct _phzl   _phzl;
struct _csgzl  _csgzl;
struct _mm     _mm;
struct _cu     _cu;
struct _dv     _dv;
struct _mrgt   _mrgt;

static char const *db_static_char_ptr_not_set = "db_static_char_ptr_not_set";
static void const *db_static_void_ptr_not_set = (void*) "db_static_void_ptr_not_set";

/* magic memory value for optlists */
#define DBOPT_MAGIC ((unsigned long long)0x5ca1ab1eDa7aBa5eULL)

/* NOT SET values for global and file properties */
#define DB_MASK_NOT_SET ((unsigned long long)0xAAAAAAAAAAAAAAAAULL)
#define DB_INTBOOL_NOT_SET ((int) -1)
#define DB_FLOAT_NOT_SET ((float) FLT_MIN)
#define DB_CHAR_PTR_NOT_SET ((char const *)db_static_char_ptr_not_set)
#define DB_VOID_PTR_NOT_SET ((void const *)db_static_void_ptr_not_set)

SILO_Globals_t SILO_Globals = {
    DBAll, /* dataReadMask */
    TRUE,  /* allowOverwrites */
    FALSE, /* allowEmptyObjects */
    FALSE, /* enableChecksums */
    FALSE, /* enableFriendlyHDF5Names */
    FALSE, /* enableGrabDriver */
#ifndef _WIN32
#warning FIX THIS
#endif
    FALSE, /* darshanEnabled */
    FALSE, /* allowLongStrComponents */
    3,     /* maxDeprecateWarnings */
    0,     /* compressionParams (null) */
    2.0,   /* compressionMinratio */
    0,     /* compressionErrmode (fallback) */
    0,     /* compatability mode */
    0,     /* evalNameschemes */
    {      /* file options sets [32 of them] */
        0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
    },
    DB_TOP,/* _db_err_level */
    0,     /* _db_err_func */
    DB_NONE,/* _db_err_level_drvr */
    0,     /* Jstk */
    DEFAULT_DRIVER_PRIORITIES
};

INTERNAL int
db_FullyDeprecatedConvention(const char *name)
{
    if (strcmp(name, "_visit_defvars") == 0)
    {
        DEPRECATE_MSG(name,4,6,"DBPutDefvars")
    }
    else if (strcmp(name, "_visit_domain_groups") == 0)
    {
        DEPRECATE_MSG(name,4,6,"DBPutMrgtree")
    }
    else if (strcmp(name, "_disjoint_elements") == 0)
    {
        DEPRECATE_MSG(name,4,6,"DBOPT_DISJOINT_MODE option")
    }
    else if (strncmp(name, "MultivarToMultimeshMap_",23) == 0)
    {
        DEPRECATE_MSG(name,4,6,"DBOPT_MMESH_NAME option for DBPutMultivar")
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_perror
 *
 * Purpose:     Print error message to standard error
 *
 * Return:      Success:        -1
 *
 *              Failure:        never fails
 *
 * Programmer:  matzke@viper
 *              Thu Nov  3 15:16:42 PST 1994
 *
 * Modifications:
 *    Robb Matzke, Tue Dec 20 20:55:14 EST 1994
 *    If s is "" then we use the previous value of s.
 *-------------------------------------------------------------------------*/
INTERNAL int
db_perror(char const *s, int errorno, char const *fname)
{
    int            call_abort = 0;
    static char    old_s[256] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    /*
     * Save error number and function name so application
     * can read them later.
     */
    db_errno = errorno;
    if (fname)
        strncpy(db_errfunc, fname, sizeof(db_errfunc) - 1);
    db_errfunc[sizeof(db_errfunc) - 1] = '\0';

    /*
     * If `s' is an empty string, then use the same string
     * as last time.
     */
    if (s && !*s) {
        s = old_s;
    }
    else if (s) {
        strncpy(old_s, s, sizeof(old_s));
        old_s[sizeof(old_s) - 1] = '\0';
    }
    else {
        old_s[0] = '\0';
    }

    switch (SILO_Globals._db_err_level) {
        case DB_NONE:
            if (SILO_Globals.Jstk)
                longjmp(SILO_Globals.Jstk->jbuf, -1);
            return -1;
        case DB_TOP:
            if (SILO_Globals.Jstk)
                longjmp(SILO_Globals.Jstk->jbuf, -1);
            break;
        case DB_ALL:
            break;
        case DB_ABORT:
            call_abort = 1;
            break;
        default:
            call_abort = 1;
            break;
    }

    /*
     * Issue the error message to standard error or by calling
     * the indicated error handling routine.
     */
    if (SILO_Globals._db_err_func) {
        int flen = 0, elen = 0, slen = 0;
        char *better_s;

        elen = strlen(db_strerror(errorno));
        if (fname && *fname) flen = strlen(fname) + 2;
        if (s && *s) slen = strlen(s) + 2;
        better_s = (char *) malloc(elen + flen + slen + 1);

        if (fname && *fname)
            sprintf(better_s, "%s: ", fname);
        sprintf(better_s + flen, "%s", db_strerror(errorno));
        if (s && *s)
            sprintf(better_s + flen + elen, ": %s", s);

        SILO_Globals._db_err_func((char*)better_s);

        free(better_s);
    }
    else {
        if (fname && *fname)
            fprintf(stderr, "%s: ", fname);
        fprintf(stderr, "%s", db_strerror(errorno));
        if (s && *s)
            fprintf(stderr, ": %s", s);
        putc('\n', stderr);
    }

    if (call_abort) {
        fflush(stdout);
        fprintf(stderr, "SILO Aborting...\n");
        fflush(stderr);
        abort();
    }

    return -1;
}

/*-------------------------------------------------------------------------
 * Function:    db_strerror
 *
 * Purpose:     Return message associated with error number
 *
 * Return:      Success:        ptr to string
 *
 *              Failure:        ptr to "No error"
 *
 * Programmer:  matzke@viper
 *              Thu Nov  3 15:18:15 PST 1994
 *
 * Modifications:
 *    Robb Matzke, Tue Feb 28 11:08:47 EST 1995
 *    If error number is out of range, we make a new error message that
 *    has the error number.  That makes this function act like DBErrString
 *    and allows for the use of user-defined error numbers that are
 *    larger than E_NERRORS or less than zero.
 *-------------------------------------------------------------------------*/
INTERNAL char *
db_strerror(int errorno)
{
    static char    s[32];

    if (errorno < 0 || errorno >= NELMTS(_db_err_list)) {
        sprintf(s, "Error %d", errorno);
        return s;
    }
    return _db_err_list[errorno];
}

/*----------------------------------------------------------------------
 *  Function                                                  db_strndup
 *
 *  Purpose
 *
 *      Return a duplicate of the given string (with length), where
 *      default mem-mgr was used to allocate the necessary space.
 *
 *  Modified
 *    Robb Matzke, Thu Nov 10 12:27:10 EST 1994
 *    Added error mechanism
 *
 *    Eric Brugger, Wed Jul 25 14:57:28 PDT 2001
 *    Renamed the routine.
 *
 *---------------------------------------------------------------------*/
INTERNAL char *
db_strndup(const char *string, int len)
{
    char          *out = NULL;
    char          *me = "strndup";

    if (string == NULL || len <= 0)
        return (NULL);

    if (NULL == (out = ALLOC_N(char, len + 1))) {
        db_perror(NULL, E_NOMEM, me);
        return NULL;
    }

    strncpy(out, string, len);
    out[len] = '\0';

    return (out);
}

/*----------------------------------------------------------------------
 * Function: DBVariableNameValid
 *
 * Purpose   Check the validity of a Silo variable name.
 *
 * Author:   Sean Ahern, Tue Sep 28 10:47:52 PDT 1999
 *
 * Returns:  1 if the name is valid
 *           0 otherwise
 *
 * Modified:
 *    Sean Ahern, Fri Oct  1 11:36:34 PDT 1999
 *    Added '/' to the list of allowed characters.  We need this for putting 
 *    variables in subdirectories.
 *
 *    Sean Ahern, Tue Oct  5 13:51:07 PDT 1999
 *    Added ':' processing so that we can reference variables in other files.
 *
 *    Lisa J. Roberts, Thu Dec 16 17:33:26 PST 1999
 *    Removed the abort called if the name validation fails.
 *
 *    Robb Matzke, 2000-06-02
 *    Omit printing error message if error handling mode is DB_NONE. After
 *    all, some applications check return values and then print their own
 *    error message.
 *
 *    Hank Childs, Thu Sep  7 14:17:13 PDT 2000
 *    Allow variable names to be relative. [HYPer02087]
 *
 *    Mark C. Miller, Mon Oct 22 22:08:09 PDT 2007
 *    Made it part of the public API.
 *
 *---------------------------------------------------------------------*/
PUBLIC int
DBVariableNameValid(const char *s)
{
    int             len;
    int             i;
    char           *p = NULL;

    /* If there's a ':' in the name, allow anything before the ':'.  After the 
     * ':' we have to be more strict. */

    p = (char *)strchr(s,':');
    if (p == NULL)
        p = (char *)s;
    else
        p++;    /* Move one character past the ':'. */

    len = strlen(p);

    /* Every character has to be alphanumeric or the `_' character. */
    for(i=0;i<len;i++)
    {
        int  okay = 0;

        if (isalnum(p[i]) || (p[i] == '_') || (p[i] == '/') || p[i] == '.')
        {
            okay = 1;
        }

        /* Don't need to check for the end of the string because of the
         * short circuit rule and the null character at end of string. */
        if ((p[i] == '.') && (p[i+1] == '.') && (p[i+2] == '/'))
        {
            okay = 1;
            i += 2;  /* 2 = strlen("../") - 1 (from `for' loop's i++) */
        }

        if (! okay)
        {
#if 0
            if (DB_NONE!=SILO_Globals._db_err_level)
            {
                fprintf(stderr,"\"%s\" is an invalid name.  Silo variable\n"
                        "names may contain only alphanumeric characters\n"
                        "or the _ character.\n", s);
            }
#endif
            return 0;
        }
    }

    return 1;
}
/* kept this to deal with non-const qualified API */
INTERNAL int
db_VariableNameValid(char const *s)
{
    return DBVariableNameValid(s);
}


/*----------------------------------------------------------------------
 *  Routine                                              _DBQQCalcStride
 *
 *  Purpose
 *
 *      Calculate the strides given the dimensions and major-order.
 *
 *      Works for 1D, 2D and 3D variables/meshes, collinear or
 *      non-collinear, materials, too.
 *
 *--------------------------------------------------------------------*/
INTERNAL void
_DBQQCalcStride(int stride[], int dims[], int ndims, int major_order)
{
    int            i;

     /*------------------------------------------------------
      * Define strides for accessing adjacent elements based
      * on whether arrays are stored row-major or column-major.
      *-----------------------------------------------------*/

    if (major_order == DB_ROWMAJOR) {
        stride[0] = 1;

        for (i = 1; i < ndims; i++) {
            stride[i] = stride[i - 1] * dims[i - 1];
        }
    }
    else {
        stride[ndims - 1] = 1;

        for (i = ndims - 2; i >= 0; i--) {
            stride[i] = stride[i + 1] * dims[i + 1];
        }
    }
}

/*----------------------------------------------------------------------
 *  Routine                                                 _DBQMSetStride
 *
 *  Purpose
 *
 *      Set the stride component for the given quad mesh.
 *
 *      Works for 1D, 2D and 3D meshes, collinear or non-collinear.
 *
 * Modified
 *    Robb Matzke, Wed Jan 11 06:35:33 PST 1995
 *    Changed name from QM_SetStride because it conflicted with MeshTV.
 *--------------------------------------------------------------------*/
INTERNAL void
_DBQMSetStride(DBquadmesh *qmesh)
{
    _DBQQCalcStride(qmesh->stride, qmesh->dims, qmesh->ndims,
                    qmesh->major_order);
}

/*----------------------------------------------------------------------
 *  Routine                                         SW_GetDatatypeString
 *
 *  Function
 *
 *      Return the string representation of the given SWAT data type.
 *
 *  Modified
 *    Robb Matzke, Thu Nov 10 12:28:44 EST 1994
 *    Added error mechanism
 *
 *    Robb Matzke, Thu Nov 10 12:30:43 EST 1994
 *    An invalid `type' is now an error.
 *
 *    Mark C. Miller, Mon Sep 21 15:17:08 PDT 2009
 *    Adding support for long long type.
 *
 *    Mark C. Miller, Fri Nov 13 15:32:02 PST 2009
 *    Changed name of "long long" type to "longlong" as PDB is
 *    sensitive to spaces in type names.
 *
 *    Mark C. Miller, Tue Nov 17 22:29:35 PST 2009
 *    Fixed memory error by extending length of alloc'd string to
 *    support "long_long". Changed name of long long data type
 *    to match what PDB proper does.
 *
 *    Mark C. Miller, Mon Dec  7 09:50:19 PST 2009
 *    Conditionally compile long long support only when its
 *    different from long.
 *
 *    Mark C. Miller, Mon Jan 11 16:02:16 PST 2010
 *    Made long long support UNconditionally compiled.
 *---------------------------------------------------------------------*/
INTERNAL char *
db_GetDatatypeString(int type)
{
    char          *str = NULL;
    char          *me = "db_GetDatatypeString";

    if (NULL == (str = ALLOC_N(char, 10))) {
        db_perror(NULL, E_NOMEM, me);
        return NULL;
    }

    switch (type) {
        case DB_INT:
            strcpy(str, "integer");
            break;
        case DB_SHORT:
            strcpy(str, "short");
            break;
        case DB_LONG:
            strcpy(str, "long");
            break;
        case DB_LONG_LONG:
            strcpy(str, "long_long");
            break;
        case DB_FLOAT:
            strcpy(str, "float");
            break;
        case DB_DOUBLE:
            strcpy(str, "double");
            break;
        case DB_CHAR:
            strcpy(str, "char");
            break;
        default:
            db_perror("type", E_BADARGS, me);
            FREE(str);
            return NULL;
    }

    return (str);
}

/*-------------------------------------------------------------------------
 * Function:    silo_db_close
 *
 * Purpose:     Free public parts of DBfile.  This function is called
 *              after the file has been closed and the private parts
 *              have been freed.
 *
 * Return:      Success:        NULL
 *
 *              Failure:        never fails
 *
 * Programmer:  matzke@viper
 *              Wed Nov  2 13:55:22 PST 1994
 *
 * Modifications:
 *   Mark C. Miller, Tue Feb  3 09:53:53 PST 2009
 *   Changed name to silo_db_close to avoid collision with popular BRLCAD
 *   libs. Added stuff to free GrabId and set Grab related stuff to zero.
 *-------------------------------------------------------------------------*/
INTERNAL DBfile *
silo_db_close(DBfile *dbfile)
{
    if (dbfile) {
        db_FreeToc(dbfile);
        FREE(dbfile->pub.GrabId);
        dbfile->pub.GrabId = 0;
        dbfile->pub.Grab = FALSE;
        FREE(dbfile->pub.name);
        FREE(dbfile);
    }

    return NULL;
}

/*-------------------------------------------------------------------------
 * Function:    db_AllocToc
 *
 * Purpose:     Allocate an empty table of contents for a new file.
 *
 * Return:      Success:        ptr to new DBtoc
 *
 *              Failure:        NULL, db_errno set.
 *
 * Modifications:
 *    Eric Brugger, Thu Feb  9 14:43:50 PST 1995
 *    I modified the routine to handle the obj in the table of contents.
 *
 *    Sean Ahern, Fri Aug 23 16:59:02 PDT 1996
 *    Added multimats.
 *
 *    Jeremy Meredith, Sept 18 1998
 *    Added multi-block material species.
 *-------------------------------------------------------------------------*/
INTERNAL DBtoc *
db_AllocToc(void)
{
    DBtoc         *toc = NULL;
    char          *me = "db_AllocToc";

    if (NULL == (toc = ALLOC(DBtoc))) {
        db_perror(NULL, E_NOMEM, me);
        return NULL;
    }

    toc->curve_names = NULL;
    toc->ncurve = 0;

    toc->csgmesh_names = NULL;
    toc->ncsgmesh = 0;

    toc->csgvar_names = NULL;
    toc->ncsgvar = 0;

    toc->defvars_names = NULL;
    toc->ndefvars = 0;

    toc->multimesh_names = NULL;
    toc->nmultimesh = 0;

    toc->multimeshadj_names = NULL;
    toc->nmultimeshadj = 0;

    toc->multivar_names = NULL;
    toc->nmultivar = 0;

    toc->multimat_names = NULL;
    toc->nmultimat = 0;

    toc->multimatspecies_names = NULL;
    toc->nmultimatspecies = 0;

    toc->qmesh_names = NULL;
    toc->nqmesh = 0;

    toc->qvar_names = NULL;
    toc->nqvar = 0;

    toc->ucdmesh_names = NULL;
    toc->nucdmesh = 0;

    toc->ucdvar_names = NULL;
    toc->nucdvar = 0;

    toc->ptmesh_names = NULL;
    toc->nptmesh = 0;

    toc->ptvar_names = NULL;
    toc->nptvar = 0;

    toc->var_names = NULL;
    toc->nvar = 0;

    toc->mat_names = NULL;
    toc->nmat = 0;

    toc->obj_names = NULL;
    toc->nobj = 0;

    toc->dir_names = NULL;
    toc->ndir = 0;

    toc->array_names = NULL;
    toc->narray = 0;

    toc->mrgtree_names = NULL;
    toc->nmrgtree = 0;

    toc->groupelmap_names = NULL;
    toc->ngroupelmap = 0;

    toc->mrgvar_names = NULL;
    toc->nmrgvar = 0;

    toc->symlink_target_names = NULL;
    toc->nsymlink = 0;
    toc->symlink_names = NULL;

    return(toc);
}

/*-------------------------------------------------------------------------
 * Function:    db_FreeToc
 *
 * Purpose:     Free the table of contents associated with a file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Modifications:
 *    Robb Matzke, Thu Dec 1 10:22:11 PST 1994
 *    Errors in device drivers might leave the table of
 *    contents in a not-fully-initialized state.  Therefore,
 *    this routine always checks for nil pointers before
 *    dereferencing them.
 *
 *    Robb Matzke, Fri Dec 2 13:13:01 PST 1994
 *    Removed all references to SCORE memory management.
 *
 *    Eric Brugger, Thu Feb  9 14:43:50 PST 1995
 *    I modified the routine to handle the obj in the table of contents.
 *
 *    Sean Ahern, Fri Jun 21 10:56:15 PDT 1996
 *    Added handling for multimats.
 *
 *    Jeremy Meredith, Sept 18 1998
 *    Added multi-block species.
 *-------------------------------------------------------------------------*/
INTERNAL int
db_FreeToc(DBfile *dbfile)
{
    int            i;
    DBtoc         *toc = NULL;
    char          *me = "db_FreeToc";

    if (!dbfile)
        return db_perror(NULL, E_NOFILE, me);
    if (!dbfile->pub.toc)
        return 0;

    toc = dbfile->pub.toc;

    if (toc->ncurve > 0) {
        if (toc->curve_names) {
            for (i = 0; i < toc->ncurve; i++) {
                FREE(toc->curve_names[i]);
            }
            FREE(toc->curve_names);
        }
    }

    if (toc->nmultimesh > 0) {
        if (toc->multimesh_names) {
            for (i = 0; i < toc->nmultimesh; i++) {
                FREE(toc->multimesh_names[i]);
            }
            FREE(toc->multimesh_names);
        }
    }

    if (toc->nmultimeshadj > 0) {
        if (toc->multimeshadj_names) {
            for (i = 0; i < toc->nmultimeshadj; i++) {
                FREE(toc->multimeshadj_names[i]);
            }
            FREE(toc->multimeshadj_names);
        }
    }

    if (toc->nmultivar > 0) {
        if (toc->multivar_names) {
            for (i = 0; i < toc->nmultivar; i++) {
                FREE(toc->multivar_names[i]);
            }
            FREE(toc->multivar_names);
        }
    }

    if (toc->nmultimat > 0) {
        if (toc->multimat_names) {
            for(i=0; i < toc->nmultimat; i++) {
                FREE(toc->multimat_names[i]);
            }
            FREE(toc->multimat_names);
        }
    }

    if (toc->nmultimatspecies > 0) {
        if (toc->multimatspecies_names) {
            for(i=0; i < toc->nmultimatspecies; i++) {
                FREE(toc->multimatspecies_names[i]);
            }
            FREE(toc->multimatspecies_names);
        }
    }

    if (toc->ncsgmesh > 0) {
        if (toc->csgmesh_names) {
            for (i = 0; i < toc->ncsgmesh; i++) {
                FREE(toc->csgmesh_names[i]);
            }
            FREE(toc->csgmesh_names);
        }
    }
    if (toc->ncsgvar > 0) {
        if (toc->csgvar_names) {
            for (i = 0; i < toc->ncsgvar; i++) {
                FREE(toc->csgvar_names[i]);
            }
            FREE(toc->csgvar_names);
        }
    }

    if (toc->ndefvars > 0) {
        if (toc->defvars_names) {
            for (i = 0; i < toc->ndefvars; i++) {
                FREE(toc->defvars_names[i]);
            }
            FREE(toc->defvars_names);
        }
    }

    if (toc->nqmesh > 0) {
        if (toc->qmesh_names) {
            for (i = 0; i < toc->nqmesh; i++) {
                FREE(toc->qmesh_names[i]);
            }
            FREE(toc->qmesh_names);
        }
    }

    if (toc->nqvar > 0) {
        if (toc->qvar_names) {
            for (i = 0; i < toc->nqvar; i++) {
                FREE(toc->qvar_names[i]);
            }
            FREE(toc->qvar_names);
        }
    }

    if (toc->nptmesh > 0) {
        if (toc->ptmesh_names) {
            for (i = 0; i < toc->nptmesh; i++) {
                FREE(toc->ptmesh_names[i]);
            }
            FREE(toc->ptmesh_names);
        }
    }

    if (toc->nptvar > 0) {
        if (toc->ptvar_names) {
            for (i = 0; i < toc->nptvar; i++) {
                FREE(toc->ptvar_names[i]);
            }
            FREE(toc->ptvar_names);
        }
    }

    if (toc->nmat > 0) {
        if (toc->mat_names) {
            for (i = 0; i < toc->nmat; i++) {
                FREE(toc->mat_names[i]);
            }
            FREE(toc->mat_names);
        }
    }

    if (toc->nucdmesh > 0) {
        if (toc->ucdmesh_names) {
            for (i = 0; i < toc->nucdmesh; i++) {
                FREE(toc->ucdmesh_names[i]);
            }
            FREE(toc->ucdmesh_names);
        }
    }

    if (toc->nucdvar > 0) {
        if (toc->ucdvar_names) {
            for (i = 0; i < toc->nucdvar; i++) {
                FREE(toc->ucdvar_names[i]);
            }
            FREE(toc->ucdvar_names);
        }
    }

    if (toc->nvar > 0) {
        if (toc->var_names) {
            for (i = 0; i < toc->nvar; i++) {
                FREE(toc->var_names[i]);
            }
            FREE(toc->var_names);
        }
    }

    if (toc->nobj > 0) {
        if (toc->obj_names) {
            for (i = 0; i < toc->nobj; i++) {
                FREE(toc->obj_names[i]);
            }
            FREE(toc->obj_names);
        }
    }

    if (toc->ndir > 0) {
        if (toc->dir_names) {
            for (i = 0; i < toc->ndir; i++) {
                FREE(toc->dir_names[i]);
            }
            FREE(toc->dir_names);
        }
    }

    if (toc->narray > 0) {
        if (toc->array_names) {
            for (i = 0; i < toc->narray; i++) {
                FREE(toc->array_names[i]);
            }
            FREE(toc->array_names);
        }
    }

    if (toc->nmrgtree > 0) {
        if (toc->mrgtree_names) {
            for (i = 0; i < toc->nmrgtree; i++) {
                FREE(toc->mrgtree_names[i]);
            }
            FREE(toc->mrgtree_names);
        }
    }

    if (toc->ngroupelmap > 0) {
        if (toc->groupelmap_names) {
            for (i = 0; i < toc->ngroupelmap; i++) {
                FREE(toc->groupelmap_names[i]);
            }
            FREE(toc->groupelmap_names);
        }
    }

    if (toc->nmrgvar > 0) {
        if (toc->mrgvar_names) {
            for (i = 0; i < toc->nmrgvar; i++) {
                FREE(toc->mrgvar_names[i]);
            }
            FREE(toc->mrgvar_names);
        }
    }

    if (toc->nsymlink > 0)
    {
        if (toc->symlink_target_names) {
            for (i = 0; i < toc->nsymlink; i++)
                FREE(toc->symlink_target_names[i]);
            FREE(toc->symlink_target_names);
        }
#ifndef _WIN32
#warning WE SHOULD PROBABLY JUST EITHER MAKE THIS CONSISTENT OR PERHAPS COPY ALL CHARS INTO LINK@TARGET format
#endif
        /* Here we free just the list of pointers to names that are actually
           other names in the toc */
        FREE(toc->symlink_names);
    }

    FREE(dbfile->pub.toc);
    return 0;
}

/*----------------------------------------------------------------------
 *  Routine                                          silo_GetMachDataSize
 *
 *  Purpose
 *
 *      Return the byte length of the given data type ON THE CURRENT
 *      MACHINE.
 *
 *  Notes
 *
 *  Modified
 *    Robb Matzke, Thu Nov 10 12:33:44 EST 1994
 *    Added error mechanism.  An invalid `datatype' is an error.
 *
 *    Mark C. Miller, Mon Sep 21 15:17:08 PDT 2009
 *    Adding support for long long type.
 *
 *    Mark C. Miller, Mon Dec  7 09:50:19 PST 2009
 *    Conditionally compile long long support only when its
 *    different from long.
 *
 *    Mark C. Miller, Mon Jan 11 16:02:16 PST 2010
 *    Made long long support UNconditionally compiled.
 *--------------------------------------------------------------------*/
INTERNAL int
db_GetMachDataSize(int datatype)
{
    int            size;
    char          *me = "db_GetMachDataSize";

    switch (datatype)
    {
        case DB_CHAR:      size = sizeof(char); break;
        case DB_SHORT:     size = sizeof(short); break;
        case DB_INT:       size = sizeof(int); break;
        case DB_LONG:      size = sizeof(long); break;
        case DB_LONG_LONG: size = sizeof(long long); break;
        case DB_FLOAT:     size = sizeof(float); break;
        case DB_DOUBLE:    size = sizeof(double); break;
        default:           return db_perror("datatype", E_BADARGS, me);
    }
    return (size);
}

/*----------------------------------------------------------------------
 *  Routine                                         db_GetDatatypeID
 *
 *  Purpose
 *
 *      Return the SILO integer definition for the provided ascii datatype
 *      description. That is, convert "float" to SWAT_FLOAT (i.e., 19).
 *
 *  Notes
 *
 *  Modified
 *    Robb Matzke, Thu Nov 10 12:35:24 EST 1994
 *    Added error mechanism.  An invalid `dataname' is an error.
 *
 *    Mark C. Miller, Mon Sep 21 15:17:08 PDT 2009
 *    Adding support for long long type.
 *
 *    Mark C. Miller, Fri Nov 13 15:32:02 PST 2009
 *    Changed name of "long long" type to "longlong" as PDB is
 *    sensitive to spaces in type names.
 *
 *    Mark C. Miller, Tue Nov 17 22:30:30 PST 2009
 *    Changed name of long long datatype to match PDB proper.
 *
 *    Mark C. Miller, Mon Dec  7 09:50:19 PST 2009
 *    Conditionally compile long long support only when its
 *    different from long.
 *
 *    Mark C. Miller, Mon Jan 11 16:02:16 PST 2010
 *    Made long long support UNconditionally compiled.
 *--------------------------------------------------------------------*/
INTERNAL int
db_GetDatatypeID(char const * const dataname)
{
    int            size;
    char          *me = "db_GetDatatypeID";

    if (STR_BEGINSWITH(dataname, "integer"))
        size = DB_INT;
    else if (STR_BEGINSWITH(dataname, "int"))
        size = DB_INT;
    else if (STR_BEGINSWITH(dataname, "short"))
        size = DB_SHORT;
    else if (STR_BEGINSWITH(dataname, "long_long"))
        size = DB_LONG_LONG;
    else if (STR_BEGINSWITH(dataname, "long"))
        size = DB_LONG;
    else if (STR_BEGINSWITH(dataname, "float"))
        size = DB_FLOAT;
    else if (STR_BEGINSWITH(dataname, "double"))
        size = DB_DOUBLE;
    else if (STR_BEGINSWITH(dataname, "char"))
        size = DB_CHAR;
    else
        return db_perror("dataname", E_BADARGS, me);

    return (size);
}

/*----------------------------------------------------------------------
 *  Routine                                              DBGetObjtypeTag
 *
 *  Purpose
 *
 *      Return the tag (integer ID) for the given object type.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Parameters
 *
 *      type_name         {In}    {Name of object type to inquire about}
 *
 *  Notes
 *
 *      This function and it's counterpart, DBGetObjtypeName(), must
 *      stay in sync. Changes to this function must be mirrored in
 *      the other.
 *
 *  Modifications
 *    Al Leibee,Mon Aug  1 16:16:00 PDT 1994
 *    Added material species.
 *
 *    Robb Matzke, Tue Oct 25 08:57:56 PDT 1994
 *    Added compound array.
 *
 *    Robb Matzke, Wed Nov 9 14:04:23 EST 1994
 *    Added error mechanism.
 *
 *    Robb Matzke, Fri Dec 2 09:58:51 PST 1994
 *    Added `zonelist' and `facelist' and `edgelist'
 *
 *    Eric Brugger, Tue Feb  7 11:05:39 PST 1995
 *    I modified the routine to return DB_USERDEF if the type_name
 *    is not known.
 *
 *    Katherine Price, Thu May 25 10:00:50 PDT 1995
 *    Added multi-block material.
 *
 *    Jeremy Meredith, Sept 18 1998
 *    Added multi-block material species.
 *--------------------------------------------------------------------*/
PUBLIC int
DBGetObjtypeTag(char const *type_name)
{
    int            tag;
    char          *me = "DBGetObjtypeTag";

    if (!type_name || !*type_name)
        return db_perror("type name", E_BADARGS, me);

    if (type_name[0] == 'D' && type_name[1] == 'B')
        type_name += 2;

    if (STR_EQUAL(type_name, "multiblockmesh") ||
        STR_EQUAL(type_name, "multimesh"))
        tag = DB_MULTIMESH;

    else if (STR_EQUAL(type_name, "multimeshadj"))
        tag = DB_MULTIMESHADJ;

    else if (STR_EQUAL(type_name, "multiblockvar") ||
             STR_EQUAL(type_name, "multivar"))
        tag = DB_MULTIVAR;

    else if (STR_EQUAL(type_name, "multiblockmat") ||
             STR_EQUAL(type_name, "multimat"))
        tag = DB_MULTIMAT;

    else if (STR_EQUAL(type_name, "multimatspecies"))
        tag = DB_MULTIMATSPECIES;

    else if (STR_EQUAL(type_name, "quadmesh-rect"))
        tag = DB_QUAD_RECT;

    else if (STR_EQUAL(type_name, "quadmesh-curv"))
        tag = DB_QUAD_CURV;

    else if (STR_EQUAL(type_name, "csgmesh"))
        tag = DB_CSGMESH;

    else if (STR_EQUAL(type_name, "csgvar"))
        tag = DB_CSGVAR;

    else if (STR_EQUAL(type_name, "defvars"))
        tag = DB_DEFVARS;

    else if (STR_EQUAL(type_name, "quadmesh"))
        tag = DB_QUADMESH;

    else if (STR_EQUAL(type_name, "quadvar"))
        tag = DB_QUADVAR;

    else if (STR_EQUAL(type_name, "ucdmesh"))
        tag = DB_UCDMESH;

    else if (STR_EQUAL(type_name, "ucdvar"))
        tag = DB_UCDVAR;

    else if (STR_EQUAL(type_name, "pointmesh"))
        tag = DB_POINTMESH;

    else if (STR_EQUAL(type_name, "pointvar"))
        tag = DB_POINTVAR;

    else if (STR_EQUAL(type_name, "curve"))
        tag = DB_CURVE;

    else if (STR_EQUAL(type_name, "material"))
        tag = DB_MATERIAL;

    else if (STR_EQUAL(type_name, "matspecies"))
        tag = DB_MATSPECIES;

    else if (STR_EQUAL(type_name, "compoundarray"))
        tag = DB_ARRAY;

    else if (STR_EQUAL(type_name, "facelist"))
        tag = DB_FACELIST;

    else if (STR_EQUAL(type_name, "zonelist"))
        tag = DB_ZONELIST;

    else if (STR_EQUAL(type_name, "polyhedral-zonelist"))
        tag = DB_PHZONELIST;

    else if (STR_EQUAL(type_name, "csgzonelist"))
        tag = DB_CSGZONELIST;

    else if (STR_EQUAL(type_name, "edgelist"))
        tag = DB_EDGELIST;

    else if (STR_EQUAL(type_name, "mrgtree"))
        tag = DB_MRGTREE;

    else if (STR_EQUAL(type_name, "groupelmap"))
        tag = DB_GROUPELMAP;

    else if (STR_EQUAL(type_name, "mrgvar"))
        tag = DB_MRGVAR;

    else if (STR_EQUAL(type_name, "symlink"))
        tag = DB_SYMLINK;

    else
        tag = DB_USERDEF;

    return (tag);
}

/*----------------------------------------------------------------------
 *  Routine                                             DBGetObjtypeName
 *
 *  Purpose
 *
 *      Return the name associated with an object of the given type.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Parameters
 *
 *      type         {In}    {Type of object to inquire about}
 *
 *  Notes
 *
 *      The calling routine should NOT attempt to free the memory
 *      associated with the returned string.
 *
 *  Modifications
 *    Al Leibee, Tue Jul 26 08:44:01 PDT 1994
 *    Replaced composition by matspecies.
 *
 *    Robb Matzke, Fri Oct 21 15:23:18 EST 1994
 *    Added DB_ARRAY for Compound Array type.
 *
 *    Eric Brugger, Tue Feb  7 11:05:39 PST 1995
 *    I modified the routine to return "unknown" if the type is
 *    DB_USERDEF.
 *
 *    Katherine Price, Thu May 25 10:00:50 PDT 1995
 *    Added DB_MULTIMAT for Multi-Block Material type.
 *     
 *    Jeremy Meredith, Sept 18 1998
 *    Added DB_MULTIMATSPECIES for Multi-block Species.
 *--------------------------------------------------------------------*/
INTERNAL char *
DBGetObjtypeName(int type)
{
    char          *me = "DBGetObjtypeName";

    switch (type) {
        case DB_CSGMESH:
            return ("csgmesh");
        case DB_CSGVAR:
            return ("csgvar");
        case DB_CSGZONELIST:
            return ("csgzonelist");
        case DB_DEFVARS:
            return ("defvars");
        case DB_QUADMESH:
            return ("quadmesh");
        case DB_QUAD_RECT:
            return ("quadmesh-rect");
        case DB_QUAD_CURV:
            return ("quadmesh-curv");
        case DB_QUADVAR:
            return ("quadvar");
        case DB_UCDMESH:
            return ("ucdmesh");
        case DB_UCDVAR:
            return ("ucdvar");
        case DB_POINTMESH:
            return ("pointmesh");
        case DB_POINTVAR:
            return ("pointvar");
        case DB_MULTIMESH:
            return ("multiblockmesh");
        case DB_MULTIMESHADJ:
            return ("multimeshadj");
        case DB_MULTIVAR:
            return ("multiblockvar");
        case DB_MULTIMAT:
            return ("multiblockmat");
        case DB_MULTIMATSPECIES:
            return ("multimatspecies");
        case DB_MATERIAL:
            return ("material");
        case DB_MATSPECIES:
            return ("matspecies");
        case DB_FACELIST:
            return ("facelist");
        case DB_ZONELIST:
            return ("zonelist");
        case DB_PHZONELIST:
            return ("polyhedral-zonelist");
        case DB_EDGELIST:
            return ("edgelist");
        case DB_CURVE:
            return ("curve");
        case DB_ARRAY:
            return ("compoundarray");
        case DB_MRGTREE:
            return ("mrgtree");
        case DB_GROUPELMAP:
            return ("groupelmap");
        case DB_MRGVAR:
            return ("mrgvar");
        case DB_SYMLINK:
            return ("symlink");
        case DB_USERDEF:
            return ("unknown");
    }

    db_perror("type-number", E_BADARGS, me);
    return ("unknown");
}

/*-------------------------------------------------------------------------
 * Function:    DBLs
 *
 * Purpose:     Lists the contents of the given directories based on the
 *              listing options set in the `args' array.  Directory path
 *              arguments can be either absolute or relative.  The standard
 *              Unix directory syntax is understood: `..' is shorthand for
 *              the parent of te current directory and `.' is shorthand
 *              for the current directory.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Arguments:
 *      args          Argument array
 *      nargs         Number of arguments
 *      build_list    Sentinel: 1:build list instead of printing
 *      list          List of varnames matching request
 *      nlist         Returned length of list
 *
 * Programmer:  robb@cloud
 *              Tue Nov 15 15:07:26 EST 1994
 *
 * Modifications:
 *    Eric Brugger, Thu Feb  9 14:43:50 PST 1995
 *    I modified the routine to handle the obj in the table of contents.
 *
 *    Jeremy Meredith, Sept 18 1998
 *    Added multimatspecies to the toc
 *    
 *    Mark C. Miller, Thu Aug 20 14:20:25 PDT 2020
 *    Changed name from db_ListDir2 to DBLs and made it a public function.
 *    Macro-ized much of the internal logic.
 *    Changed list filtering logic. "-a -d" means everything *but* dirs
 *    whereas "-d" without the preceding "-a" means only dirs. Same behavior
 *    for all other options after a "-a".
 *-------------------------------------------------------------------------*/
PUBLIC int
DBLs(DBfile *_dbfile, char const *cl_args, char *list[], int *nlist)
{
    int            i, k, npaths, nopts;
    int            ls_mesh, ls_var, ls_mat, ls_curve, ls_multi, ls_dir;
    int            ls_low, ls_obj, ls_arr, ls_link, ls_mrg;
    int            count_list = FALSE, build_list = FALSE, print_list = FALSE;
    char           opts[256], cwd[256], orig_dir[256], *paths[64];
    DBtoc         *toc = NULL;
    int            left_margin, col_margin, line_width;
    char          *me = "DBLs";
    int            _nlist_orig, has_opt_a = FALSE, nargs;
    char         **args;

     /*----------------------------------------
      *  Parse input options and pathnames.
      *----------------------------------------*/

    if (!list && nlist)
        count_list = TRUE;
    else if (list && nlist)
        build_list = TRUE;
    else if (!list && !nlist)
        print_list = TRUE;
    else
        return -1;

    npaths = 0;
    nopts = 0;
    nargs = -1;
    args = db_StringListToStringArray(cl_args, &nargs, ' ', 0);
    for (i = 0; i < nargs; i++) {

        switch (args[i][0]) {
            case '-':

                strcpy(&opts[nopts], &args[i][1]);
                nopts += strlen(args[i]) - 1;
                break;

            default:

                paths[npaths++] = args[i];
                break;
        }
    }

     /*----------------------------------------
      *  Set listing options based on input.
      *----------------------------------------*/
    if (nopts > 0) {
        ls_mesh = ls_var = ls_mat = ls_curve = ls_multi = ls_dir = FALSE;
        ls_low = ls_obj = ls_arr = ls_link = ls_mrg = FALSE;
    }
    else {
        /* Default values */
        ls_mesh = ls_var = ls_multi = ls_dir = TRUE;
        ls_mat = ls_curve = FALSE;
        ls_low = ls_obj = ls_arr = ls_link = ls_mrg = FALSE;
    }

    for (i = 0; i < nopts; i++) {

        switch (opts[i]) {
            case 'a':
                ls_mesh = ls_var = ls_mat = ls_curve = ls_multi = ls_dir = TRUE;
                ls_low = ls_obj = ls_arr = ls_link = ls_mrg = TRUE;
                has_opt_a = TRUE;
                break;
            case 'A':
                ls_arr = has_opt_a ? FALSE : TRUE;
                break;
            case 'c':
                ls_curve = has_opt_a ? FALSE : TRUE;
                break;
            case 'd':
                ls_dir = has_opt_a ? FALSE : TRUE;
                break;
            case 'l':
                ls_link = has_opt_a ? FALSE : TRUE;
                break;
            case 'm':
                ls_mesh = has_opt_a ? FALSE : TRUE;
                break;
            case 'M':
                ls_multi = has_opt_a ? FALSE : TRUE;
                break;
            case 'o':
                ls_obj = has_opt_a ? FALSE : TRUE;
                break;
            case 'r':
                ls_mat = has_opt_a ? FALSE : TRUE;
                break;
            case 't':
                ls_mrg = has_opt_a ? FALSE : TRUE;
                break;
            case 'v':
                ls_var = has_opt_a ? FALSE : TRUE;
                break;
            case 'x':
                ls_low = has_opt_a ? FALSE : TRUE;
                break;
            default:
                return db_perror("invalid list option", E_BADARGS, me);
        }
    }

     /*----------------------------------------
      *  List all requested objects/dirs
      *----------------------------------------*/

    DBGetDir(_dbfile, orig_dir);

    if (npaths <= 0) {
        npaths = 1;
        paths[0] = ".";
    }

    left_margin = 10;
    col_margin = 5;
    line_width = 80;

    if (nlist)
    {
        _nlist_orig = *nlist;
        *nlist = 0;
    }

#define PROCESS_LIST(LS_VAR, CAT) \
if (LS_VAR && toc->n##CAT > 0) { \
    if (count_list) {\
        *nlist += toc->n##CAT; \
    } \
    else if (build_list) { \
        for (i = 0; i < toc->n##CAT && *nlist<_nlist_orig; i++) { \
            list[*nlist] = ALLOC_N(char, strlen(toc->CAT##_names[i]) + 1); \
            strcpy(list[(*nlist)++], toc->CAT##_names[i]); \
        } \
    } \
    else if (print_list) { \
        printf("%7d "#CAT"s:\n", toc->n##CAT); \
        _DBstrprint(stdout, toc->CAT##_names, toc->n##CAT, \
                    'c', left_margin, col_margin, line_width); \
        printf("\n"); \
    } \
}

    for (k = 0; k < npaths; k++) {

        DBGetDir(_dbfile, cwd);

        /* Change to requested directory, if necessary */
        if (!STR_EQUAL(".", paths[k]) &&
            !STR_EQUAL(cwd, paths[k]))
            DBSetDir(_dbfile, paths[k]);

        toc = DBGetToc(_dbfile);
        if (!toc)
            return db_perror("unable to get toc", E_INTERNAL, me);

        PROCESS_LIST(ls_curve, curve);
        PROCESS_LIST(ls_low, var); /* misc. vars */
        PROCESS_LIST(ls_mat, mat);
        PROCESS_LIST(ls_mat, matspecies);
        PROCESS_LIST(ls_arr, array); /* compound arrays */
        PROCESS_LIST(ls_dir, dir);
        PROCESS_LIST(ls_multi && ls_mesh, multimesh);
        PROCESS_LIST(ls_multi && ls_mesh, multimeshadj);
        PROCESS_LIST(ls_mesh, qmesh);
        PROCESS_LIST(ls_mesh, ucdmesh);
        PROCESS_LIST(ls_mesh, ptmesh);
        PROCESS_LIST(ls_mesh, csgmesh);
        PROCESS_LIST(ls_mrg, mrgtree);
        PROCESS_LIST(ls_mrg, groupelmap);
        PROCESS_LIST(ls_multi && ls_var, multivar);
        PROCESS_LIST(ls_multi && ls_mat, multimat);
        PROCESS_LIST(ls_multi && ls_mat, multimatspecies);
        PROCESS_LIST(ls_var, qvar);
        PROCESS_LIST(ls_var, ucdvar);
        PROCESS_LIST(ls_var, ptvar);
        PROCESS_LIST(ls_var, csgvar);
        PROCESS_LIST(ls_var, defvars);
        PROCESS_LIST(ls_var, mrgvar);
        PROCESS_LIST(ls_obj, obj); /* misc. objects */
        PROCESS_LIST(ls_link, symlink);

        /*
         * Return to original directory, since next path may
         * be relative to it.
         */
        DBSetDir(_dbfile, orig_dir);

    }

#ifndef _WIN32
#warning CLEAN UP IS LOST IN ABOVE EARLY RETURNS
#endif
    if (args)
        DBFreeStringArray(args, nargs);

    if (nlist && (*nlist >= _nlist_orig))
        return -1;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    context_switch
 *
 * Purpose:     Many of the DB...() functions take an object name as a
 *              parameter.  The old protocol didn't specify whether the
 *              name could include a path, so some drivers allow one and
 *              other don't.  To fix the problem, each of the API functions
 *              that allow a name will call this routine to change to
 *              the specified directory and will call context_restore()
 *              to change back.
 *
 * Bugs:        This function doesn't protect calls to the other API
 *              directory setting/retrieving functions and thus might
 *              leak memory if one of those calls fail with a non-local
 *              return.
 *
 * Return:      Success:        ptr to the previous context.  This ptr
 *                              should be passed to context_restore().
 *
 *              Failure:        NULL, db_perror called.
 *
 * Arguments:
 *      base         output parameter
 *
 * Programmer:  matzke@viper
 *              Sun Jan 29 11:42:47 PST 1995
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
INTERNAL context_t *
context_switch(DBfile *dbfile, char const *name, char const **base)
{
    char          const *me = "context_switch";
    char          s[256], *b;
    context_t     *old = ALLOC(context_t);

    /*
     * Save the old information.  If the name doesn't contain a `/' then
     * we don't have to do anything.  We will mark this case by storing
     * NULL as the context name.
     */
    *base = name;
    if (!strchr(name, '/')) {
        old->dirid = 0;
        old->name = NULL;
        return old;
    }
    if (DBGetDir(dbfile, s) < 0) {
        FREE(old);
        return NULL;
    }
    old->dirid = dbfile->pub.dirid;
    old->name = STRDUP(s);

    /*
     * Split the name into a path and a base name.  The base name
     * is the stuff after the last `/'.  If the base name is empty
     * then we should raise an E_NOTFOUND right away.
     */
    b = (char *)strrchr(name, '/');
    if (!b || !b[1]) {
        FREE(old->name);
        FREE(old);
        db_perror(name, E_NOTFOUND, me);
        return NULL;
    }
    *base = b + 1;
    if (b == name) {
        /*
         * This is the root directory.
         */
        if (DBSetDir(dbfile, "/") < 0) {
            FREE(old->name);
            FREE(old);
            return NULL;
        }
    }
    else {
        /*
         * The path is everything before (not including) the last
         * `/'.  We would like to just change that slash to a null
         * terminator temporarily, but the name might be a static
         * string in a read-only data section, so we have to do it
         * the long way.  We assume (like the rest of SILO and most
         * drivers) that the name will not be longer than 255
         * characters.
         */
        strncpy(s, name, b - name);
        s[b - name] = '\0';
        if (DBSetDir(dbfile, s) < 0) {
            FREE(old->name);
            FREE(old);
            return NULL;
        }
    }

    return old;
}

/*-------------------------------------------------------------------------
 * Function:    context_restore
 *
 * Purpose:     Restore a previously saved context.  If the driver can
 *              change directories based on a directory ID, we do that.
 *              Otherwise, we change directories based on the old directory
 *              name.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1, db_perror called.
 *
 * Programmer:  matzke@viper
 *              Sun Jan 29 12:01:21 PST 1995
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
INTERNAL int
context_restore(DBfile *dbfile, context_t *old)
{
    if (!dbfile || !old)
        return 0;
    if (!old->name) {
        FREE(old);
        return 0;
    }

    DBSetDir(dbfile, old->name);

    FREE(old->name);
    FREE(old);
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_get_fileid
 *
 * Purpose:     Obtain a file ID number which is unique with respect to
 *              all other open files.
 *
 * Return:      Success:        ID number, [0..DB_NFILES-1]
 *
 *              Failure:        -1, too many files are open.
 *
 * Programmer:  robb@cloud
 *              Tue Feb 28 11:00:04 EST 1995
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
PRIVATE int
db_get_fileid ( int flags )
{
    static int     vhand = 0;
    int            i;

    for (i = 0; i < DB_NFILES; i++) {
        if (!_db_fstatus[(vhand + i) % DB_NFILES]) {
            i = (vhand + i) % DB_NFILES;
            _db_fstatus[i] = flags | DB_ISOPEN;
            vhand = (i + 1) % DB_NFILES;
            return i;
        }
    }
    return -1;
}

/*-------------------------------------------------------------------------
  Function: bjhash 

  Purpose: Hash a variable length stream of bytes into a 32-bit value.

  Programmer: By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.

  You may use this code any way you wish, private, educational, or
  commercial.  It's free. However, do NOT use for cryptographic purposes.

  See http://burtleburtle.net/bob/hash/evahash.html
 *-------------------------------------------------------------------------*/

#define bjhash_mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

static unsigned int bjhash(register const unsigned char *k, register unsigned int length, register unsigned int initval)
{
   register unsigned int a,b,c,len;

   len = length;
   a = b = 0x9e3779b9;
   c = initval;

   while (len >= 12)
   {
      a += (k[0] +((unsigned int)k[1]<<8) +((unsigned int)k[2]<<16) +((unsigned int)k[3]<<24));
      b += (k[4] +((unsigned int)k[5]<<8) +((unsigned int)k[6]<<16) +((unsigned int)k[7]<<24));
      c += (k[8] +((unsigned int)k[9]<<8) +((unsigned int)k[10]<<16)+((unsigned int)k[11]<<24));
      bjhash_mix(a,b,c);
      k += 12; len -= 12;
   }

   c += length;

   switch(len)
   {
      case 11: c+=((unsigned int)k[10]<<24);
      case 10: c+=((unsigned int)k[9]<<16);
      case 9 : c+=((unsigned int)k[8]<<8);
      case 8 : b+=((unsigned int)k[7]<<24);
      case 7 : b+=((unsigned int)k[6]<<16);
      case 6 : b+=((unsigned int)k[5]<<8);
      case 5 : b+=k[4];
      case 4 : a+=((unsigned int)k[3]<<24);
      case 3 : a+=((unsigned int)k[2]<<16);
      case 2 : a+=((unsigned int)k[1]<<8);
      case 1 : a+=k[0];
   }

   bjhash_mix(a,b,c);

   return c;
}


/*-------------------------------------------------------------------------
 * Functions:   db_register_file, db_unregister_file, db_isregistered_file
 *
 * Purpose:     Maintain list of files returned by DBCreate/DBOpen as well
 *              as closed by DBClose in order to detect possible operation
 *              on closed files.
 *
 * Return:      -1 if file limit exceeded. Otherwise [0..DB_NFILES-1]
 *              representing position in fixed size list. 
 *
 * Programmer:  Mark C. Miller, Wed Jul 23 00:14:00 PDT 2008
 *
 * Modifications:
 *   Mark C. Miller, Wed Feb 25 23:46:44 PST 2009
 *   Replaced name of file which can be very different but still represent
 *   the same file stat structure. We wind up using st_dev/st_ino members
 *   of stat struct to identify a file. In fact, we use a bjhash of those
 *   members. This is not foolproof. Non posix filesystems can apparently
 *   result in st_dev/st_ino combinations which are the same but for
 *   different files.
 *
 *   Mark C. Miller, Fri Feb 12 08:20:03 PST 2010
 *   Replaced conditional compilation with SIZEOF_OFF64T with
 *   db_silo_stat_struct.
 *
 *   Mark C. Miller, Wed May 19 17:07:05 PDT 2010
 *   Added logic for _WIN32 form of the db_silo_stat_struct.
 *-------------------------------------------------------------------------*/
PRIVATE int 
db_register_file(DBfile *dbfile, const db_silo_stat_t *filestate, int writeable)
{
    int i;
    for (i = 0; i < DB_NFILES; i++)
    {
        if (_db_regstatus[i].f == 0)
        {
            unsigned int hval = 0;
#ifndef _WIN32
            hval = bjhash((unsigned char *) &(filestate->s.st_dev), sizeof(filestate->s.st_dev), hval);
            hval = bjhash((unsigned char *) &(filestate->s.st_ino), sizeof(filestate->s.st_ino), hval);
#else
            hval = bjhash((unsigned char *) &(filestate->fileindexlo), sizeof(filestate->fileindexlo), hval);
            hval = bjhash((unsigned char *) &(filestate->fileindexhi), sizeof(filestate->fileindexhi), hval);
#endif
            _db_regstatus[i].f = dbfile;
            _db_regstatus[i].n = hval; 
            _db_regstatus[i].w = writeable;
            return i;
        }
    }
    return -1;
}

PRIVATE int 
db_unregister_file(DBfile *dbfile)
{
    int i;
    for (i = 0; i < DB_NFILES; i++)
    {
        if (_db_regstatus[i].f == dbfile)
        {
            int j;
            _db_regstatus[i].f = 0;
            for (j = i; (j < DB_NFILES-1) && (_db_regstatus[j+1].f != 0); j++)
            {
                _db_regstatus[j].f = _db_regstatus[j+1].f;
                _db_regstatus[j].n = _db_regstatus[j+1].n;
                _db_regstatus[j].w = _db_regstatus[j+1].w;
            }
            _db_regstatus[j].f = 0;
            return i;
        }
    }
    return -1;
}

PRIVATE int
db_isregistered_file(DBfile *dbfile, const db_silo_stat_t *filestate)
{
    int i;
    if (dbfile)
    {
        for (i = 0; i < DB_NFILES; i++)
        {
            if (_db_regstatus[i].f == dbfile)
                return i;
        }
    }
    else if (filestate)
    {
        unsigned int hval = 0;
#ifndef _WIN32
        hval = bjhash((unsigned char *) &(filestate->s.st_dev), sizeof(filestate->s.st_dev), hval);
        hval = bjhash((unsigned char *) &(filestate->s.st_ino), sizeof(filestate->s.st_ino), hval);
#else
        hval = bjhash((unsigned char *) &(filestate->fileindexlo), sizeof(filestate->fileindexlo), hval);
        hval = bjhash((unsigned char *) &(filestate->fileindexhi), sizeof(filestate->fileindexhi), hval);
#endif
        for (i = 0; i < DB_NFILES; i++)
        {
            if (_db_regstatus[i].f != 0 &&
                _db_regstatus[i].n == hval)
                return i;
        }
    }
    return -1;
}

INTERNAL int
db_num_registered_files()
{
    int i;
    int cnt = 0;
    for (i = 0; i < DB_NFILES; i++)
    {
        if (_db_regstatus[i].f) cnt++;
    }
    return cnt;
}

/*-------------------------------------------------------------------------
 * Function:   db_silo_stat_one_file
 *
 * Purpose:    Better stat method for silo taking into account stat/stat64
 *             as well as windows-specific notion of an 'inode'.
 *
 * Programmer: Mark C. Miller
 *
 * Modifications:
 *   Adjusted the windows-specific logic to obtain fileindex information so
 *   that if that work fails, it still returns stat retval and errno of stat.
 *-------------------------------------------------------------------------*/
PRIVATE int
db_silo_stat_one_file(const char *name, db_silo_stat_t *statbuf)
{
    int retval;
    errno = 0;
    memset(&(statbuf->s), 0, sizeof(statbuf->s));

#if SIZEOF_OFF64_T > 4 && (defined(HAVE_STAT64) || !defined(HAVE_STAT))
    retval = stat64(name, &(statbuf->s));
#else
    retval = stat(name, &(statbuf->s));
#endif /* #if SIZEOF_OFF64_T > 4 */

#ifdef _WIN32
    if (retval == 0)
    {
        /* this logic was copied by and large from HDF5 sec2 VFD */
        int errnotmp = errno;
        int fd = open(name, O_RDONLY);
        if (fd != -1)
        {
            struct _BY_HANDLE_FILE_INFORMATION fileinfo;
            GetFileInformationByHandle((HANDLE)_get_osfhandle(fd), &fileinfo);
            statbuf->fileindexhi = fileinfo.nFileIndexHigh;
            statbuf->fileindexlo = fileinfo.nFileIndexLow;
            close(fd);
        }
        errno = errnotmp;
    }
#endif /* #ifdef _WIN32 */

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:   db_silo_stat
 *
 * Purpose:    Better stat method for silo taking into account stat/stat64
 *             as well as issues with filenames used for split vfds.
 *
 * Programmer: Mark C. Miller, Fri Feb 12 08:21:52 PST 2010
 *-------------------------------------------------------------------------*/
PRIVATE int
db_silo_stat(const char *name, db_silo_stat_t *statbuf, int opts_set_id)
{
    int retval;

    /* check for case where we're opening a buffer as a file */
    if (opts_set_id > DB_FILE_OPTS_LAST)
    {
        const DBoptlist *opts = SILO_Globals.fileOptionsSets[opts_set_id-NUM_DEFAULT_FILE_OPTIONS_SETS];
        void *p; int vfd = -1;
        if ((p = DBGetOption(opts, DBOPT_H5_VFD)))
            vfd = *((int*)p);
        if (vfd == DB_H5VFD_FIC)
        {
            static int n = 0;
            statbuf->s.st_mode = 0x0;
            statbuf->s.st_mode |= S_IRUSR;
            statbuf->s.st_mode |= S_IWUSR;
            statbuf->s.st_dev = (dev_t) n++;
            statbuf->s.st_ino = (ino_t) n++;
            return 0;
        }
    }
 
    retval = db_silo_stat_one_file(name, statbuf); 

    if (opts_set_id == -1 ||
        opts_set_id == DB_FILE_OPTS_H5_DEFAULT_SPLIT ||
        opts_set_id > DB_FILE_OPTS_LAST)
    {
        int i;
        int imin = opts_set_id == -1 ? 0 : opts_set_id;
        int imax = opts_set_id == -1 ? MAX_FILE_OPTIONS_SETS: opts_set_id;
        int tmperrno = errno;

        for (i = imin; i < imax; i++)
        {
            db_silo_stat_t tmpstatbuf;
            static char tmpname[4096];
            char *meta_ext="", *raw_ext="-raw";
            void *p; int vfd = -1;
            const DBoptlist *opts;

            if (opts_set_id == -1)
                opts = SILO_Globals.fileOptionsSets[i];
            else if (opts_set_id == DB_FILE_OPTS_H5_DEFAULT_SPLIT)
                opts = 0;
            else
                opts = SILO_Globals.fileOptionsSets[i-NUM_DEFAULT_FILE_OPTIONS_SETS];

            /* ignore if options set id does not yield a valid options set */
            if (opts)
            {
                /* ignore if options set unrelated to split vfds */
                if ((p = DBGetOption(opts, DBOPT_H5_VFD)))
                    vfd = *((int*)p);

                if (vfd != DB_H5VFD_SPLIT)
                    continue;

                /* ok, get meta/raw filenaming extension conventions */
                if ((p = DBGetOption(opts, DBOPT_H5_META_EXTENSION)))
                    meta_ext = (char *) p;
                if ((p = DBGetOption(opts, DBOPT_H5_RAW_EXTENSION)))
                    raw_ext = (char *) p;
            }

            /* try the raw file name, first */
            if (strstr(raw_ext,"%s"))
                sprintf(tmpname, raw_ext, name);
            else
                sprintf(tmpname, "%s%s", name, raw_ext);
            errno = 0;
            if (db_silo_stat_one_file(tmpname, &tmpstatbuf) != 0 || errno != 0)
                continue;

            /* try the meta file last and return its statbuf */
            if (strstr(meta_ext,"%s"))
                sprintf(tmpname, meta_ext, name);
            else
                sprintf(tmpname, "%s%s", name, meta_ext);
            memset(&tmpstatbuf, 0, sizeof(tmpstatbuf));
            if (db_silo_stat_one_file(tmpname, &tmpstatbuf) == 0 && errno == 0)
            {
                memcpy(statbuf, &tmpstatbuf, sizeof(tmpstatbuf));
                return 0;
            }
        }

        errno = tmperrno;
    }

    return retval;
}

PUBLIC int
DBStat(char const *fname)
{
    db_silo_stat_t dbstat;
    return db_silo_stat(fname, &dbstat, -1);
}

/*-------------------------------------------------------------------------
 * Function:    db_filter_install
 *
 * Purpose:     Install the database-requested filters, calling the filter
 *              `open' routine for each named filter and reporting errors
 *              for filters that can't be found.  Filters are requested
 *              through the `_filters' character variable which is
 *              optional.  This variable should contain a list of filter
 *              names separated by `;' (extra `;' may appear at the beginning
 *              or end of the string).  The first filter in the list is
 *              the one that will be installed closest to the device
 *              driver while the last filter is installed closest to the
 *              API.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Tue Feb 28 11:58:01 EST 1995
 *
 * Modifications:
 *    Eric Brugger, Fri Mar  7 15:26:29 PST 1997
 *    I modified the routine to copy the filter name string to a scratch
 *    so array so that a NULL character could be added without overwriting
 *    the last character in the string.
 *-------------------------------------------------------------------------*/
PRIVATE int
db_filter_install ( DBfile *dbfile )
{
    char          *me = "db_filter_install";
    int            len, i;
    char          *var, *var2, *s, *filter_name;
    static char    not_found[128];

    /*
     * There should be a miscellaneous variable called `_filters' in
     * the current (root) directory.  If not, then no filters are
     * requested.
     */
    if (!DBInqVarExists(dbfile,"_filters"))
        return(0);

    /*
     * Read the `_filters' variable and make sure it is a character
     * string.
     */
    if (DB_CHAR != DBGetVarType(dbfile, "_filters")) {
        db_perror("`_filters' is not a character variable",
                  E_NOTFILTER, me);
        return -1;
    }
    len = DBGetVarLength(dbfile, "_filters");
    if (len <= 0)
        return 0;               /*no filters requested */
    if (NULL == (var = (char*)DBGetVar(dbfile, "_filters")))
        return -1;

    /*
     * Copy the variable and add a terminating NULL character.
     */
    var2 = ALLOC_N (char, len+1);
    strncpy (var2, var, len);
    var2[len] = '\0';

    /*
     * Process each filter.  Names are separated from one another
     * by semicolons which may also appear at the beginning and end
     * of the string.  Be careful for things like `;;'.
     */
    not_found[0] = '\0';
    s = var2;
    while ((filter_name = strtok(s, ";\n\r"))) {
        s = NULL;
        if (!filter_name[0])
            continue;

        for (i = 0; i < DB_NFILTERS; i++) {
            if (_db_filter[i].name &&
                !strcmp(_db_filter[i].name, filter_name)) {
                break;
            }
        }

        /*
         * If the filter isn't found, tack the name onto the end
         * of a list of names that weren't found, being careful
         * not to overflow that buffer.  Each name should be
         * separated from the others by a semicolon as in the `_filters'
         * database variable.
         */
        if (i >= DB_NFILTERS) {
            int q = 0;
            len = strlen(not_found);
            if (len && len + 1 < sizeof(not_found)) {
                strcat(not_found, ";");
                len++;
            }
            while (len + 1 < sizeof(not_found))
                not_found[len++] = filter_name[q++];
            not_found[len] = '\0';
            continue;
        }

        /*
         * If the filter has an `open' routine, call it now.
         */
        if (_db_filter[i].open) {
            (void)(_db_filter[i].open) (dbfile, _db_filter[i].name);
        }
    }

    FREE (var2);

    /*
     * If we failed to find some filters, we should notify the user.
     * Should we return success or failure???  For now, we return
     * success so that failure to find a filter is a warning at
     * this level but an error if db_perror calls longjmp().  This
     * gives the application a little control.
     */
    if (not_found[0]) {
        db_perror(not_found, E_NOTFILTER, me);
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    DBFilterRegistration
 *
 * Purpose:     Manipulate the global filter table by adding, changing,
 *              or removing a filter.  `Name' specifies the filter that
 *              will be affected.  `Init' and `open' are filter functions
 *              that will be called when a database is opened.  `Init' is
 *              called for every database that is opend (just after opening;
 *              filters called in arbitrary order).  `Open' is called for
 *              each file which requests that filter.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1, table is full
 *
 * Programmer:  robb@cloud
 *              Tue Feb 28 11:26:07 EST 1995
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
PUBLIC int
DBFilterRegistration(const char *name, int(*init)(DBfile*, char*),
                     int(*open)(DBfile*, char*))
{
    int            i, j = -1;

    API_BEGIN("DBFilterRegistration", int, -1) {

        /*
         * Look for entry already in the table.  If found, simply change
         * the callbacks.
         */
        for (i = 0; i < DB_NFILTERS; i++) {
            if (_db_filter[i].name && !strcmp(_db_filter[i].name, name)) {
                break;
            }
            if (j < 0 && !_db_filter[i].name)
                j = i;
        }
        if (i < DB_NFILTERS) {
            if (!init && !open) {
                FREE(_db_filter[i].name);
                _db_filter[i].name = NULL;
            }
            else {
                _db_filter[i].init = init;
                _db_filter[i].open = open;
            }
            API_RETURN(0);
        }

        /*
         * This is a new filter definition.  Add it to the first free
         * slot.
         */
        if (init || open) {
            if (j < 0)
                API_ERROR((char *)name, E_MAXFILTERS);
            _db_filter[j].name = STRDUP(name);
            _db_filter[j].init = init;
            _db_filter[j].open = open;
        }
        API_RETURN(0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBUninstall
 *
 * Purpose:     Uninstalls the top-most filter if any.  This is similar
 *              to closing the file except the uninstall is not propogated
 *              down the filter stack.
 *
 *              If the `uninstall' callback is null, this routine
 *              doesn't do anything and then returns success.  This
 *              allows device drivers to omit the uninstall function.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Thu Mar 16 10:29:36 EST 1995
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
int
DBUninstall(DBfile *dbfile)
{
    int retval;

    API_BEGIN("DBUninstall", int, -1) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (!dbfile->pub.uninstall)
        {
            API_RETURN(0);
        }

        retval = (dbfile->pub.uninstall) (dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Purpose: Macro to build set/get global and per-file property
 * methods. These are designed so that a file's properties are
 * initially NOT set. So operations on the file will obey
 * whatever the *current* library global properties are. However,
 * if a given property is set specifically on the file with a
 * call to the DBSetXxxFile() method, then that property will
 * no longer obey *global* library settings and will instead
 * be forever more controlled only by calls to the DBSetxxxFile()
 * method. In this way, files "inheret" the old behavior of the
 * library global settings until they have been specifically set
 * otherwise. And, this is true, independently for each property.
 *--------------------------------------------------------------------*/
#define DB_SETGET(Typ,NM,nM,NotSet)                             \
PUBLIC Typ DBSet ## NM(Typ val)                                 \
{                                                               \
    Typ oldVal = SILO_Globals.nM;                               \
    SILO_Globals.nM = val;                                      \
    return oldVal;                                              \
}                                                               \
                                                                \
PUBLIC Typ DBGet ## NM(void)                                    \
{                                                               \
    return SILO_Globals.nM;                                     \
}                                                               \
                                                                \
static Typ db_SetGet ## NM ## File(DBfile *f, Typ val, int set) \
{                                                               \
    Typ retval;                                                 \
    API_BEGIN("DB(Set|Get)" #NM "File", Typ, -1) {              \
        if (!f)                                                 \
            API_ERROR("DBfile*", E_BADARGS);                    \
        retval = f->pub.file_scope_globals->nM;                 \
        if (set)                                                \
            f->pub.file_scope_globals->nM = val;                \
        if (retval == NotSet)                                   \
            retval = DBGet ## NM();                             \
        API_RETURN(retval);                                     \
    }                                                           \
    API_END_NOPOP;                                              \
}                                                               \
                                                                \
PUBLIC Typ DBSet ## NM ## File(DBfile *f, Typ val)              \
{                                                               \
    return db_SetGet ## NM ## File(f, val, 1);                  \
}                                                               \
                                                                \
PUBLIC Typ DBGet ## NM ## File(DBfile *f)                       \
{                                                               \
    return db_SetGet ## NM ## File(f, 0, 0);                    \
}

/* Define global and file-level set/get property methods */
DB_SETGET(int, AllowOverwrites, allowOverwrites, DB_INTBOOL_NOT_SET) 
DB_SETGET(int, AllowEmptyObjects, allowEmptyObjects, DB_INTBOOL_NOT_SET) 
DB_SETGET(int, EnableChecksums, enableChecksums, DB_INTBOOL_NOT_SET) 
DB_SETGET(int, FriendlyHDF5Names, enableFriendlyHDF5Names, DB_INTBOOL_NOT_SET) 
DB_SETGET(int, DeprecateWarnings, maxDeprecateWarnings, DB_INTBOOL_NOT_SET) 
/*DB_SETGET(int, EnableDarshan, darshanEnabled, DB_INTBOOL_NOT_SET) */
DB_SETGET(int, AllowLongStrComponents, allowLongStrComponents, DB_INTBOOL_NOT_SET) 
DB_SETGET(unsigned long long, DataReadMask2, dataReadMask, DB_MASK_NOT_SET) 
DB_SETGET(int, CompatibilityMode, compatibilityMode, DB_INTBOOL_NOT_SET)
#ifndef _WIN32
#warning WHAT ABOUT FORCESINGLE SHOWERRORS
#endif
DB_SETGET(int, EvalNameschemes, evalNameschemes, DB_INTBOOL_NOT_SET)

/* The compression stuff has some custom initialization */
static void _db_set_compression_params(char **dst, char const *s)
{
    if (s && *s == '\0') {
        if (*dst)
            FREE(*dst);
        *dst = ALLOC_N(char, 12);
        strcpy(*dst, "METHOD=GZIP");
    }   
    else if (s) {
        if (*dst)
            FREE(*dst);
        *dst = ALLOC_N(char,strlen(s)+1);
        strcpy(*dst, s);
    }
    else {
        if (*dst)
            FREE(*dst);
        *dst=0;
    }
}

/*----------------------------------------------------------------------
 * Routine:  DBSetCompression
 *
 * Purpose:  Set and return the enable Compression flags 
 *
 * Programmer:  Thomas R. Treadway, Wed Feb 28 11:36:34 PST 2007
 *
 * Description:  This routine enters the compression method information.
 *--------------------------------------------------------------------*/
PUBLIC void
DBSetCompression(const char *s)
{
    return _db_set_compression_params(&SILO_Globals.compressionParams, s);
}

PUBLIC char const * 
DBGetCompression()
{
    return SILO_Globals.compressionParams;
}

static char const *db_SetGetCompressionFile(DBfile *f, char const *val, int set)
{
    char const *retval;
    API_BEGIN("DB(Set|Get)CompressionFile", char const *, 0) {
        if (!f)
            API_ERROR("DBfile*", E_BADARGS);
        retval = f->pub.file_scope_globals->compressionParams;
        if (set)
            _db_set_compression_params(&(f->pub.file_scope_globals->compressionParams), val);
        if (retval == DB_CHAR_PTR_NOT_SET)
            retval = DBGetCompression();
        API_RETURN(retval);
    }
    API_END_NOPOP;
}

PUBLIC void
DBSetCompressionFile(DBfile *f, char const *s)
{
    db_SetGetCompressionFile(f, s, 1);
}

PUBLIC char const * 
DBGetCompressionFile(DBfile *f)
{
    return db_SetGetCompressionFile(f, 0, 0);
}

PUBLIC int
DBFreeCompressionResources(DBfile *dbfile, const char *meshname)
{
    int retval = 0;

    API_BEGIN2("DBFreeCompressionResources", int, -1, api_dummy) {

        if (!dbfile->pub.free_z)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        retval = ((dbfile->pub.free_z) (dbfile, meshname));

        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

#define CHECK_FOR_FRIENDLY(ON,SU)					\
    ntotal += toc->n ## ON;						\
    for (i = 0; i < toc->n ## ON; i++)					\
    {									\
        char tmp[1024];							\
        snprintf(tmp, sizeof(tmp), "%s_%s", toc->ON ## _names[i], SU);	\
        if (DBInqVarExists(f, tmp))					\
            nfriendly++;						\
    }

/*----------------------------------------------------------------------
 * Routine:  db_guess_has_friendly_HDF5_names_r 
 *
 * Purpose:  Recursive helper func for DBGuessHasFriendlyHDF5Names 
 *           names.
 *
 * Programmer: Mark C. Miller, Wed Sep  2 15:27:06 PDT 2009
 *
 *--------------------------------------------------------------------*/
PRIVATE int
db_guess_has_friendly_HDF5_names_r(DBfile *f)
{
    int i, ntotal = 0, nfriendly = 0;
    int retval;
    DBtoc *toc;

    toc = DBGetToc(f);

    if (!toc) return 0;

    CHECK_FOR_FRIENDLY(multimesh, "meshnames");
    CHECK_FOR_FRIENDLY(multivar, "varnames");
    CHECK_FOR_FRIENDLY(multimat, "matnames");
    CHECK_FOR_FRIENDLY(qmesh, "coord0");
    CHECK_FOR_FRIENDLY(qvar, "data");
    CHECK_FOR_FRIENDLY(ucdmesh, "coord0");
    CHECK_FOR_FRIENDLY(ucdvar, "data");
    CHECK_FOR_FRIENDLY(ptmesh, "coord0");
    CHECK_FOR_FRIENDLY(ptvar, "data");
    CHECK_FOR_FRIENDLY(csgmesh, "_coeffs");
    CHECK_FOR_FRIENDLY(csgvar, "data");
    CHECK_FOR_FRIENDLY(mat, "_matlist");
    CHECK_FOR_FRIENDLY(matspecies, "_speclist");
    CHECK_FOR_FRIENDLY(curve, "_yvals");
    CHECK_FOR_FRIENDLY(obj, "_nodelist");

    if (ntotal >= 3) /* arb. min of 3 objects */
    {
        if (nfriendly >= ntotal/2)
            return 1;
        else
            return 0;
    }

    retval = -1;
    for (i = 0; i < toc->ndir && retval == -1; i++)
    {
        DBSetDir(f, toc->dir_names[i]);
        retval = db_guess_has_friendly_HDF5_names_r(f);
        DBSetDir(f, "..");
    }

    return retval;
}

/*----------------------------------------------------------------------
 * Routine:  DBGuessHasFriendlyHDF5Names
 *
 * Purpose:  Determine if it looks like a given file has HDF5 friendly 
 *           names.
 *
 * Programmer: Mark C. Miller, Wed Sep  2 15:27:06 PDT 2009
 *
 *--------------------------------------------------------------------*/
PUBLIC int
DBGuessHasFriendlyHDF5Names(DBfile *f)
{
    char cwd[1024];
    int retval;

#ifdef DB_HDF5X
    if (DBGetDriverType(f) != DB_HDF5X)
        return 0;
#endif

    DBGetDir(f, cwd);
    retval = db_guess_has_friendly_HDF5_names_r(f);
    DBSetDir(f, cwd);    

    return retval;
}

/*----------------------------------------------------------------------
 * Routine:  DBSetUnknownDriverPriority
 *
 * Purpose:  Set priority order of drivers used by unknown driver. 
 *
 * Programmer:  Mark C. Miller, May 1, 2006 
 *
 * Description:  This routine sets the flag that controls whether
 *               checksums are computed on client data.
 *--------------------------------------------------------------------*/
PUBLIC int const * 
DBSetUnknownDriverPriorities(const int *priorities)
{
    int i = 0;
    static int oldPriorities[MAX_FILE_OPTIONS_SETS+DB_NFORMATS+1];
    memcpy(oldPriorities, SILO_Globals.unknownDriverPriorities, sizeof(oldPriorities));
    while (i < (MAX_FILE_OPTIONS_SETS+DB_NFORMATS+1) && priorities[i] >= 0)
    {
        SILO_Globals.unknownDriverPriorities[i] = priorities[i];
        i++;
    }
    if (i < (MAX_FILE_OPTIONS_SETS+DB_NFORMATS+1))
        SILO_Globals.unknownDriverPriorities[i] = -1;
    return oldPriorities;
}

PUBLIC int const *
DBGetUnknownDriverPriorities()
{
    static int priorities[MAX_FILE_OPTIONS_SETS+DB_NFORMATS+1];
    memcpy(priorities, SILO_Globals.unknownDriverPriorities, sizeof(priorities));
    return priorities;
}

PUBLIC int
DBRegisterFileOptionsSet(const DBoptlist *opts)
{
    int i;

    API_BEGIN("DBRegisterFileOptionsSet", int, -1) {
        for (i = 0; i < MAX_FILE_OPTIONS_SETS; i++)
        {
            if (SILO_Globals.fileOptionsSets[i] == 0)
            {
                SILO_Globals.fileOptionsSets[i] = opts;
                API_RETURN(i+NUM_DEFAULT_FILE_OPTIONS_SETS);
            }
        }
        API_ERROR("Silo library", E_MAXFILEOPTSETS);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC int
DBUnregisterFileOptionsSet(int opts_set_id)
{
    int _opts_set_id = opts_set_id-NUM_DEFAULT_FILE_OPTIONS_SETS;

    API_BEGIN("DBUnregisterFileOptionsSet", int, -1) {
        if (SILO_Globals.fileOptionsSets[_opts_set_id] == 0)
            API_ERROR("opts_set_id", E_BADARGS);
        SILO_Globals.fileOptionsSets[_opts_set_id] = 0;
        API_RETURN(0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC void
DBUnregisterAllFileOptionsSets()
{
    int i;

    for (i = 0; i < MAX_FILE_OPTIONS_SETS; i++)
        SILO_Globals.fileOptionsSets[i] = 0;
}

const int* db_get_used_file_options_sets_ids()
{
    int i,n;
    static int used_slots[MAX_FILE_OPTIONS_SETS+NUM_DEFAULT_FILE_OPTIONS_SETS+1];

   
    /* For the default cases, only return those that 'matter' in that
       they could possibly have an impact on Silo's ability to actually
       open the file. In addtion, put them in some kind of priority order */
    n = 0;
    used_slots[n++] = DB_FILE_OPTS_H5_DEFAULT_SILO;
    used_slots[n++] = DB_FILE_OPTS_H5_DEFAULT_SPLIT;
    used_slots[n++] = DB_FILE_OPTS_H5_DEFAULT_DIRECT;
    used_slots[n++] = DB_FILE_OPTS_H5_DEFAULT_FAMILY;
    for (i = n; i < MAX_FILE_OPTIONS_SETS+NUM_DEFAULT_FILE_OPTIONS_SETS+1; i++)
        used_slots[i] = -1;

    /* fill in with used options set slots */
    for (i = 0; i < MAX_FILE_OPTIONS_SETS; i++)
    {
        if (SILO_Globals.fileOptionsSets[i]==0)
            continue;
        used_slots[n++] = i+NUM_DEFAULT_FILE_OPTIONS_SETS;
    }

    return used_slots;
}

/*----------------------------------------------------------------------
 * Routine:  DBGrabDriver
 *
 * Purpose:  Set and return the low level driver file handle
 *
 * Programmer:  Thomas R. Treadway, Tue May 29 15:52:19 PDT 2007
 *
 * Description:  This routine returns a ponter to the driver-native
 * file handle.
 *
 * Modifications
 *   Mark C. Miller, Thu Oct 11 15:36:10 PDT 2007
 *   Record fact file was grabbed by adding var at top-level
 *--------------------------------------------------------------------*/
PUBLIC void * 
DBGrabDriver(DBfile *file)
{
    void *rtn = 0;
    if (file) {
       if (file->pub.GrabId > (void *) 0) {
          int grab_val = 1;
          DBWrite(file, "/_was_grabbed", &grab_val, &grab_val, 1, DB_INT);
#ifndef _WIN32
#warning FIX GLOBAL LOCK
#endif
          SILO_Globals.enableGrabDriver = TRUE;
          rtn = (void *) file->pub.GrabId;
       }
    }
    return rtn;
}
/*----------------------------------------------------------------------
 * Routine:  DBGetDriverType
 *
 * Purpose:  Return the drive type 
 *
 * Programmer:  Thomas R. Treadway, Thu Jun  7 13:19:48 PDT 2007
 *
 * Description:  This routine returns a the driver type
 *--------------------------------------------------------------------*/
PUBLIC int
DBGetDriverType(const DBfile *file)
{
    if (file) {
       return file->pub.type;
    }
    return DB_UNKNOWN;
}

/*----------------------------------------------------------------------
 * Routine:  DBGetDriverTypeFromPath
 *
 * Purpose:  Return the drive type 
 *
 * Programmer:  Thomas R. Treadway, Tue Jul  3 15:24:58 PDT 2007
 *
 * Description:  This routine returns a the driver type
 *
 * Modifications:
 *
 * Thomas R. Treadway, Thu Jul  5 11:57:03 PDT 2007
 * DB_HDR5 is conditional
 *
 * Mark C. Miller, Mon Nov 19 10:45:05 PST 2007
 * Removed conditional compilation on HDF5 driver
 *
 * Mark C. Miller, Mon Oct 25 16:12:49 PDT 2010
 * Initialize buf to ensure it will be null terminated no matter
 * what happens during open/read.
 *--------------------------------------------------------------------*/
PUBLIC int
DBGetDriverTypeFromPath(const char *path)
{
   char buf[9] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0'};
   int fd;
   int nbytes;
   int flags = O_RDONLY;
   if ((fd = open(path, flags)) < 0) {
      printf("cannot open `%s'\n", path);
      return -1;
   }
   if ((nbytes = read(fd, (char *)buf, 8)) == -1) {
      printf("cannot read `%s'\n", path);
      close(fd);
      return -1;
   }
   if (nbytes <= 5) {
      printf("cannot read `%s' buffer too small\n", path);
      close(fd);
      return -1;
   }
   (void) close(fd);
   if (strstr(buf, "PDB"))
      return DB_PDB;
#ifdef DB_HDF5X
   if (strstr(buf, "HDF"))
      return DB_HDF5X;
#endif
   return DB_UNKNOWN;
}

/*----------------------------------------------------------------------
 * Routine:  DBJoinPath
 *
 * Purpose:  Given paths with possible relative naming, combine them
 *           into a single absolute path. 
 *
 * Programmer:  Mark C. Miller, July 20, 2008 
 *--------------------------------------------------------------------*/
PUBLIC char * 
DBJoinPath(const char *first, const char *second)
{
    API_BEGIN("DBJoinPath", char *, NULL) {
        API_RETURN(db_join_path(first, second));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine:  DBUngrabDriver
 *
 * Purpose:  Return control of the low level driver
 *
 * Programmer:  Thomas R. Treadway, Thu Jun  7 13:19:48 PDT 2007
 *
 * Description:  This routine returns a the driver-native type
 *--------------------------------------------------------------------*/
PUBLIC int
DBUngrabDriver(DBfile *file, const void *driver_handle)
{
    if (file) {
       SILO_Globals.enableGrabDriver = FALSE;
       return file->pub.type;
    }
    return DB_UNKNOWN;
}

static int 
db_IncObjectComponentCount(DBobject *obj)
{
    int new_maxcomps = 0;
    char **new_comp_names = 0;
    char **new_pdb_names = 0;

    obj->ncomponents++;
    if (obj->ncomponents < obj->maxcomponents)
        return 1;

    new_maxcomps = obj->maxcomponents * 1.5 + 1; /* golden rule + 1 */
    new_comp_names = REALLOC_N(obj->comp_names, char *, new_maxcomps);
    if (!new_comp_names)
    {
        db_perror(0, E_NOMEM, "db_IncObjectComponentCount");
        return 0;
    }
    new_pdb_names = REALLOC_N(obj->pdb_names, char *, new_maxcomps);
    if (!new_pdb_names)
    {
        FREE(new_comp_names);
        db_perror(0, E_NOMEM, "db_IncObjectComponentCount");
        return 0;
    }

    obj->maxcomponents = new_maxcomps;
    obj->comp_names = new_comp_names;
    obj->pdb_names = new_pdb_names;

    return 1;
}

/*----------------------------------------------------------------------
 *  Routine                                                 DBMakeObject
 *
 *  Purpose
 *
 *      Allocate an object of the requested length and initialize it.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Modified
 *    Robb Matzke, Tue Nov 8 11:41:23 PST 1994
 *    Added error mechanism
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *--------------------------------------------------------------------*/
PUBLIC DBobject *
DBMakeObject(const char *name, int type, int maxcomps)
{
    DBobject      *object = NULL;

    API_BEGIN("DBMakeObject", DBobject *, NULL) {

        if (!name || !*name)
            API_ERROR("object name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("object name", E_INVALIDNAME);
        if (NULL == (object = ALLOC(DBobject)))
            API_ERROR(NULL, E_NOMEM);

        if (maxcomps <= 0) maxcomps = 30;
        object->name = STRDUP(name);
        object->type = STRDUP(DBGetObjtypeName(type));
        object->comp_names = ALLOC_N(char *, maxcomps);
        object->pdb_names = ALLOC_N(char *, maxcomps);

        if (!object->name || !object->type ||
            !object->comp_names || !object->pdb_names)
        {
            FREE(object->name);
            FREE(object->type);
            FREE(object->comp_names);
            FREE(object->pdb_names);
            API_ERROR(NULL, E_NOMEM);
        }

        object->ncomponents = 0;
        object->maxcomponents = maxcomps;

        API_RETURN(object);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 *  Routine                                                DBFreeObject
 *
 *  Purpose
 *
 *      Release the storage associated with the given object list.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 *  Modified
 *    Robb Matzke, Thu Nov 10 17:28:39 EST 1994
 *    Added error mechanism.
 *
 *    Robb Matzke, Fri Dec 2 13:14:18 PST 1994
 *    Removed all references to SCORE memory management.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *--------------------------------------------------------------------*/
PUBLIC int
DBFreeObject(DBobject *object)
{
    int            i;

    API_BEGIN("DBFreeObject", int, -1) {

        if (!object)
            API_ERROR("object pointer", E_BADARGS);
        if (object->ncomponents < 0) {
            API_ERROR("object ncomponents", E_BADARGS);
        }

        for (i = 0; i < object->ncomponents; i++) {
            FREE(object->comp_names[i]);
            FREE(object->pdb_names[i]);
        }
        for (i = 0; i < DB_MAX_H5_OBJ_VALS; i++) {
            FREE(object->h5_names[i]);
        }

        FREE(object->comp_names);
        FREE(object->pdb_names);
        FREE(object->name);
        FREE(object->type);
        FREE(object);
    }
    API_END;

    return(0);  /* Always succeeds by the time we get here */
}

/*----------------------------------------------------------------------
 *  Routine                                                DBClearObject
 *
 *  Purpose
 *
 *      Remove all components from the given object and reset counters.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 *  Modified
 *    Robb Matzke, Tue Nov 8 07:46:29 PST 1994
 *    Added error mechanism
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *--------------------------------------------------------------------*/
PUBLIC int
DBClearObject(DBobject *object)
{
    int            i;

    API_BEGIN("DBClearObject", int, -1) {
        if (!object)
            API_ERROR("object pointer", E_BADARGS);
        if (object->ncomponents < 0) {
            API_ERROR("object ncomponents", E_BADARGS);
        }

        /* Reset values, but do not free */
        for (i = 0; i < object->maxcomponents; i++) {
            object->comp_names[i] = NULL;
            object->pdb_names[i] = NULL;
        }

        object->name = NULL;
        object->type = NULL;
        object->ncomponents = 0;
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                            DBAddVarComponent
 *
 *  Purpose
 *
 *      Add a variable component to the given object structure.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 *  Modified
 *    Robb Matzke, Tue Nov 8 07:43:38 PST 1994
 *    Added error mechanism
 *
 *    Robb Matzke, Fri Dec 2 13:14:46 PST 1994
 *    Removed all references to SCORE memory management.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.  Correct spelling.
 *--------------------------------------------------------------------*/
PUBLIC int
DBAddVarComponent(DBobject *object, const char *compname, const char *pdbname)
{
    API_BEGIN("DBAddVarComponent", int, -1) {
        if (!object)
            API_ERROR("object pointer", E_BADARGS);
        if (!compname || !*compname)
            API_ERROR("component name", E_BADARGS);
        if (db_VariableNameValid(compname) == 0)
            API_ERROR("component name", E_INVALIDNAME);
        if (!pdbname || !*pdbname)
            API_ERROR("pdb name", E_BADARGS);
        if (object->ncomponents >= object->maxcomponents) {
            API_ERROR("object ncomponents", E_BADARGS);
        }

        if (NULL == (object->comp_names[object->ncomponents] =
                     STRDUP(compname)) ||
            NULL == (object->pdb_names[object->ncomponents] =
                     STRDUP(pdbname))) {
            FREE(object->comp_names[object->ncomponents]);
            API_ERROR(NULL, E_NOMEM);
        }

        if (!db_IncObjectComponentCount(object))
            API_ERROR(NULL, E_NOMEM);

    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                            DBAddIntComponent
 *
 *  Purpose
 *
 *      Add an integer literal component to the given object structure.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 *  Modifications
 *    Robb Matzke, Tue Nov 8 07:06:11 PST 1994
 *    Added error mechanism. Returns 0 on success, -1 on failure.
 *
 *    Robb Matzke, Fri Dec 2 13:15:06 PST 1994
 *    Removed all references to SCORE memory management.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 * Build string representation of literal component arrays safely.
 *----------------------------------------------------------------------*/
PRIVATE char *
db_mk_int_literal_str(int n, int const *ii)
{
    int i;
    size_t need = 0;
    size_t used = 0;
    char *buf;

    if (n < 1 || !ii)
        return NULL;

    need += (size_t) snprintf(NULL, 0, "'<i>%d", ii[0]);
    for (i = 1; i < n; i++)
        need += (size_t) snprintf(NULL, 0, ",%d", ii[i]);
    need += 2; /* trailing quote + NUL */

    buf = (char *) malloc(need);
    if (!buf)
        return NULL;

    used += (size_t) snprintf(buf + used, need - used, "'<i>%d", ii[0]);
    for (i = 1; i < n; i++)
        used += (size_t) snprintf(buf + used, need - used, ",%d", ii[i]);
    snprintf(buf + used, need - used, "'");

    return buf;
}

PRIVATE char *
db_mk_flt_literal_str(int n, double const *ff)
{
    int i;
    size_t need = 0;
    size_t used = 0;
    char *buf;

    if (n < 1 || !ff)
        return NULL;

    need += (size_t) snprintf(NULL, 0, "'<f>%g", ff[0]);
    for (i = 1; i < n; i++)
        need += (size_t) snprintf(NULL, 0, ",%g", ff[i]);
    need += 2; /* trailing quote + NUL */

    buf = (char *) malloc(need);
    if (!buf)
        return NULL;

    used += (size_t) snprintf(buf + used, need - used, "'<f>%g", ff[0]);
    for (i = 1; i < n; i++)
        used += (size_t) snprintf(buf + used, need - used, ",%g", ff[i]);
    snprintf(buf + used, need - used, "'");

    return buf;
}

PRIVATE char *
db_mk_dbl_literal_str(int n, double const *dd)
{
    int i;
    size_t need = 0;
    size_t used = 0;
    char *buf;

    if (n < 1 || !dd)
        return NULL;

    need += (size_t) snprintf(NULL, 0, "'<d>%.30g", dd[0]);
    for (i = 1; i < n; i++)
        need += (size_t) snprintf(NULL, 0, ",%.30g", dd[i]);
    need += 2; /* trailing quote + NUL */

    buf = (char *) malloc(need);
    if (!buf)
        return NULL;

    used += (size_t) snprintf(buf + used, need - used, "'<d>%.30g", dd[0]);
    for (i = 1; i < n; i++)
        used += (size_t) snprintf(buf + used, need - used, ",%.30g", dd[i]);
    snprintf(buf + used, need - used, "'");

    return buf;
}

PUBLIC int
DBAddIntComponent(DBobject *object, const char *compname, int ii)
{
    return DBAddIntNComponent(object, compname, 1, &ii);
}

PUBLIC int
DBAddIntNComponent(DBobject *object, const char *compname, int n, int const *ii)
{
    char           *tmp = NULL;

    API_BEGIN("DBAddIntNComponent", int, -1) {
        if (!object)
            API_ERROR("object pointer", E_BADARGS);
        if (!compname || !*compname)
            API_ERROR("component name", E_BADARGS);
        if (db_VariableNameValid(compname) == 0)
            API_ERROR("component name", E_INVALIDNAME);
        if (n < 1)
            API_ERROR("n", E_BADARGS);
        if (!ii)
            API_ERROR("ii array", E_BADARGS);
        if (object->ncomponents >= object->maxcomponents)
            API_ERROR("object ncomponents", E_BADARGS);

        tmp = db_mk_int_literal_str(n, ii);
        if (!tmp)
            API_ERROR(NULL, E_NOMEM);

        if (NULL == (object->comp_names[object->ncomponents] =
                     STRDUP(compname)) ||
            NULL == (object->pdb_names[object->ncomponents] =
                     STRDUP(tmp))) {
            FREE(object->comp_names[object->ncomponents]);
            FREE(tmp);
            API_ERROR(NULL, E_NOMEM);
        }
        FREE(tmp);
        tmp = NULL;

        if (!db_IncObjectComponentCount(object))
            API_ERROR(NULL, E_NOMEM);

    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                            DBAddFltComponent
 *
 *  Purpose
 *
 *      Add a floating point literal component to the given object
 *      structure.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 *  Modified:
 *    Robb Matzke, Tue Nov 8 07:04:15 PST 1994
 *    Added error mechanism.  Return -1 on failure, 0 on success.
 *
 *    Robb Matzke, Fri Dec 2 13:15:28 PST 1994
 *    Removed all references to SCORE memory management.
 *
 *    Eric Brugger, Tue Feb  7 09:06:58 PST 1995
 *    I modified the argument declarations to reflect argument promotions.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *--------------------------------------------------------------------*/
PUBLIC int
DBAddFltComponent(DBobject *object, const char *compname, double ff)
{
    return DBAddFltNComponent(object, compname, 1, &ff);
}

PUBLIC int
DBAddFltNComponent(DBobject *object, const char *compname, int n, double const *ff)
{
    char           *tmp = NULL;

    API_BEGIN("DBAddFltNComponent", int, -1) {
        if (!object)
            API_ERROR("object pointer", E_BADARGS);
        if (!compname || !*compname)
            API_ERROR("component name", E_BADARGS);
        if (n < 1)
            API_ERROR("n", E_BADARGS);
        if (!ff)
            API_ERROR("ff array", E_BADARGS);
        if (db_VariableNameValid(compname) == 0)
            API_ERROR("component name", E_INVALIDNAME);
        if (object->ncomponents >= object->maxcomponents) {
            API_ERROR("object ncomponents", E_BADARGS);
        }

        tmp = db_mk_flt_literal_str(n, ff);
        if (!tmp)
            API_ERROR(NULL, E_NOMEM);

        if (NULL == (object->comp_names[object->ncomponents] =
                     STRDUP(compname)) ||
            NULL == (object->pdb_names[object->ncomponents] =
                     STRDUP(tmp))) {
            FREE(object->comp_names[object->ncomponents]);
            FREE(tmp);
            API_ERROR(NULL, E_NOMEM);
        }
        FREE(tmp);
        tmp = NULL;
        if (!db_IncObjectComponentCount(object))
            API_ERROR(NULL, E_NOMEM);
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                            DBAddDblComponent
 *
 *  Purpose
 *
 *      Add a double precision floating point literal component to 
 *      the given object structure.
 *
 *  Programmer
 *
 *      Brad Whitlock, Thu Jan 20 09:43:13 PDT 2000
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 *  Modified:
 *
 *--------------------------------------------------------------------*/
PUBLIC int
DBAddDblComponent(DBobject *object, const char *compname, double dd)
{
    return DBAddDblNComponent(object, compname, 1, &dd);
}

PUBLIC int
DBAddDblNComponent(DBobject *object, const char *compname, int n, double const *dd)
{
    char           *tmp = NULL;

    API_BEGIN("DBAddDblNComponent", int, -1) {
        if (!object)
            API_ERROR("object pointer", E_BADARGS);
        if (!compname || !*compname)
            API_ERROR("component name", E_BADARGS);
        if (n < 1)
            API_ERROR("n", E_BADARGS);
        if (!dd)
            API_ERROR("dd array", E_BADARGS);
        if (db_VariableNameValid(compname) == 0)
            API_ERROR("component name", E_INVALIDNAME);
        if (object->ncomponents >= object->maxcomponents) {
            API_ERROR("object ncomponents", E_BADARGS);
        }

        tmp = db_mk_dbl_literal_str(n, dd);
        if (!tmp)
            API_ERROR(NULL, E_NOMEM);

        if (NULL == (object->comp_names[object->ncomponents] =
                     STRDUP(compname)) ||
            NULL == (object->pdb_names[object->ncomponents] =
                     STRDUP(tmp))) {
            FREE(object->comp_names[object->ncomponents]);
            FREE(tmp);
            API_ERROR(NULL, E_NOMEM);
        }
        FREE(tmp);
        tmp = NULL;
        if (!db_IncObjectComponentCount(object))
            API_ERROR(NULL, E_NOMEM);
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                            DBAddStrComponent
 *
 *  Purpose
 *
 *      Add a string literal component to the given object structure.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 *  Modified
 *    Robb Matzke, Tue Nov 8 07:08:33 PST 1994
 *    Added error mechanism.  Return 0 on success, -1 on failure.
 *
 *    Robb Matzke, Fri Dec 2 13:15:49 PST 1994
 *    Removed all references to SCORE memory management.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Mark C. Miller, Thu Apr  2 09:48:57 PDT 2015
 *    Added Al Nichols' enhancement for arb. length str components.
 *--------------------------------------------------------------------*/
PUBLIC int
DBAddStrComponent(DBobject *object, const char *compname, const char *ss)
{
    char *tmp = 0;

    API_BEGIN("DBAddStrComponent", int, -1) {
        if (!object)
            API_ERROR("object pointer", E_BADARGS);
        if (!compname || !*compname)
            API_ERROR("component name", E_BADARGS);
        if (db_VariableNameValid(compname) == 0)
            API_ERROR("component name", E_INVALIDNAME);
        if (object->ncomponents >= object->maxcomponents) {
            API_ERROR("object ncomponents", E_BADARGS);
        }
        if (!ss)
        {
            if (!DBGetAllowEmptyObjects())
                API_ERROR("string literal component", E_BADARGS);
            tmp = malloc(10);
            sprintf(tmp, "'<s>null'");
        }
        else
        {
            tmp = malloc(strlen(ss)+6);
            sprintf(tmp, "'<s>%s'", ss);
        }

        if (NULL == (object->comp_names[object->ncomponents] =
                     STRDUP(compname)) ||
            NULL == (object->pdb_names[object->ncomponents] =
                     STRDUP(tmp))) {
            FREE(tmp);
            FREE(object->comp_names[object->ncomponents]);
            API_ERROR(NULL, E_NOMEM);
        }
        if (!db_IncObjectComponentCount(object))
        {
            FREE(tmp);
            API_ERROR(NULL, E_NOMEM);
        }
        FREE(tmp);
    }
    API_END;

    return(0);
}

/*-------------------------------------------------------------------------
 * Function:    DBShowErrors
 *
 * Purpose:     Set the method by which errors are displayed.  The
 *              `level' parameter is one of the following:
 *
 *                 DB_ALL       -- Show all errors, beginning with the
 *                                 routine that first detected the error
 *                                 and continuing up the call stack to
 *                                 the application.
 *
 *                 DB_ABORT     -- Same as DB_ALL except abort() is called
 *                                 after the error message is printed.
 *
 *                 DB_TOP       -- (default) Only the top-level API functions
 *                                 issue error messages.
 *
 *                 DB_NONE      -- The library does not handle error messages.
 *                                 The application is responsible for
 *                                 checking the API return values and
 *                                 handling the error.
 *
 *                 DB_SUSPEND   -- This is used internally to temporarily
 *                                 suspend the issuance of error messages
 *                                 by changing the error level to DB_NONE.
 *
 *                 DB_RESTORE   -- This is used internally to restore the
 *                                 previous error level after a DB_SUSPEND.
 *
 *              The `func' parameter can point to an application-level
 *              error handling function that will be passed a string that
 *              is part of the error message (similar to the argument for
 *              perror()).  If the function pointer is null, then
 *              the library will issue error messages to the standard
 *              error stream.
 *
 *              The error text and erring function name can
 *              be obtained by calling DBErrString() or DBErrFunc().
 *
 * Return:      void
 *
 * Programmer:  matzke@viper
 *              Mon Nov  7 09:58:43 PST 1994
 *
 * Modifications:
 *    Robb Matzke, Mon Dec 12 14:25:04 EST 1994
 *    Added DB_SUSPEND and DB_RESUME in order to get
 *    db_unk_Open to work properly [the Open callback for
 *    the SILO-Unknown driver].
 *
 *    Eric Brugger, Tue Feb  7 09:06:58 PST 1995
 *    I modified the function declaration and changed the default error
 *    reporting level to DB_NONE.
 *
 *    Eric Brugger, Wed Mar  1 17:07:39 PST 1995
 *    I shrouded the prototypes for non-ansi compilers.
 * 
 *    Hank Childs, Thu Mar  2 13:34:35 PST 2000
 *    Add check to ensure that nested DBShowErrors to suspend error 
 *    messages would work correctly.
 *
 *-------------------------------------------------------------------------*/
#ifndef _WIN32
#warning ADD DBSHOWERRORSFILE
#endif
PUBLIC void
DBShowErrors(int level, void(*func)(char*))
{
    static int     old_level = DB_NONE;
    static int     old_level_drvr = DB_NONE;
    static int     nested_suspend = 0;

    SILO_Globals._db_err_level_drvr = DB_NONE;
    if (level == DB_ALL_AND_DRVR)
    {
        level = DB_ALL;
	SILO_Globals._db_err_level_drvr = DB_ALL;
    }

#ifndef _WIN32
#warning GET RID OF SUSPEND/RESUME STUFF
#endif
    switch (level) {
        case DB_SUSPEND:
            if (nested_suspend++ == 0)
            {
                old_level = SILO_Globals._db_err_level;
                old_level_drvr = SILO_Globals._db_err_level_drvr;
            }
            SILO_Globals._db_err_level = DB_NONE;
	    SILO_Globals._db_err_level_drvr = DB_NONE;
            break;
        case DB_RESUME:
            if (--nested_suspend == 0)
            {
                SILO_Globals._db_err_level = old_level;
	        SILO_Globals._db_err_level_drvr = old_level_drvr;
            }
            break;
        default:
            SILO_Globals._db_err_level = level;
            SILO_Globals._db_err_func = func;
            break;
    }
}

/*-------------------------------------------------------------------------
 * Function:    DBErrString
 *
 * Purpose:     Return the error message of the last error.
 *
 * Return:      Success:        ptr to static error message
 *
 *              Failure:        ptr to static message for db_errno=0
 *
 * Programmer:  robb@cloud
 *              Tue Feb 21 08:23:48 EST 1995
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
PUBLIC char const *
DBErrString(void)
{
    static char    s[128];

    if (db_errno < 0 || db_errno >= NELMTS(_db_err_list)) {
        sprintf(s, "Error %d", db_errno);
        return s;
    }

    return _db_err_list[db_errno];
}

PUBLIC int
DBErrno(void)
{
    return db_errno;
}

PUBLIC char const *
DBErrFuncname(void)
{
    return db_errfunc;
}

PUBLIC DBErrFunc_t
DBErrfunc(void)
{
    return SILO_Globals._db_err_func;
}

PUBLIC int
DBErrlvl(void)
{
    return SILO_Globals._db_err_level;
}

/*-------------------------------------------------------------------------
 * Function: db_parse_version_digits_from_string
 *
 * str: version string
 * sep: separator character (typically '.')
 * digits: array of digits to return
 * ndigits: size of digits array
 *
 * returns 0 on successful conversion, non-zero on failure
 *-----------------------------------------------------------------------*/
static int
db_parse_version_digits_from_string(char const *str, char sep, int *digits, int ndigits)
{
    int i, nseps, non_digits, retval = 0;
    char *p, *ostr;
    
    if (!str || !*str)
        return 1;

    ostr = strdup(str);
    p = ostr;

    /* Examine string for seperator chars and non-digits */
    nseps = 0;
    non_digits = 0;
    while (*p)
    {
        if (*p == sep)
        {
            *p = '\0';
            nseps++;
        }
        else if (!strncmp(p, "-pre", 4) ||
                 !strncmp(p, "-pos", 4))
        {
            *(p+0) = '\0';
            *(p+1) = '0';
            *(p+2) = '0';
            *(p+3) = '0';
            nseps++;
            p += 3;
        }
        else if (*p < '0' || *p > '9')
        {
            non_digits = 1;
        }
        p++;
    }
    nseps++;

    /* Make a second pass over string converting all the digits */
    if (!non_digits)
    {
        p = ostr;
        errno = 0;
        for (i = 0; i < ndigits; i++)
            digits[i] = 0;
        for (i = 0; i < nseps && ndigits && errno == 0; i++, ndigits--)
        {
            digits[i] = strtol(p, 0, 10);
            while (*p != '\0') p++;
            p++;
        }
        if (errno)
            retval = 1;
    }
    else
    {
        retval = 1;
    }

    free(ostr);

    return retval;
}

static int
db_compare_version_digits(int const *a_digits, int const *b_digits, int ndigits)
{
    int i;
    for (i = 0; i < ndigits; i++)
    {
        if (a_digits[i] < b_digits[i])
            return -1;
        else if (a_digits[i] > b_digits[i])
            return 1;
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    DBVersion
 *
 * Purpose:     Return the version number of the library as a string.
 *
 * Returns:     ptr to version number
 *
 * Programmer:  Hank Childs
 *              Tue Oct 17 14:08:45 PDT 2000
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Oct 24 12:39:31 PDT 2006
 *   Changed to use SILO_VSTRING
 *-------------------------------------------------------------------------*/
PUBLIC char const *
DBVersion(void)
{
    static char version[256];
    strcpy(version, SILO_VSTRING);

    return version;
}

PUBLIC int
DBVersionDigits(int *maj, int *min, int *pat, int *pre)
{
    int digits[4] = {0,0,0,0};

    if (!db_parse_version_digits_from_string(DBVersion(), '.',
             digits, sizeof(digits)/sizeof(digits[0])))
    {
        if (maj) *maj = digits[0];
        if (min) *min = digits[1];
        if (pat) *pat = digits[2];
        if (pre) *pre = digits[3];
        return 0;
    }
    return -1;
}

/*-------------------------------------------------------------------------
 * Function:    DBVersionGE
 *
 * Purpose:     Return whether or not the version of the library is greater
 *              than or equal to the version specified by Maj, Min, Pat.
 *              This is a run-time equiv. of the SILO_VERSION_GE macro.
 *
 * Returns:     integer indicating if true (1) or false (0) 
 *
 * Programmer:  Mark C. Miller, Mon Jan 12 20:59:30 PST 2009
 *-------------------------------------------------------------------------*/
PUBLIC int 
DBVersionGE(int Maj, int Min, int Pat)
{
    int a_digits[3] = {SILO_VERS_MAJ, SILO_VERS_MIN, SILO_VERS_PAT};
    int b_digits[3] = {Maj<0?0:Maj, Min<0?0:Min, Pat<0?0:Pat};
    return db_compare_version_digits(a_digits, b_digits, 3) >= 0;
}

PUBLIC int
DBVersionGEFileVersion(const DBfile *dbfile)
{
    int a_digits[3];
    int b_digits[3] = {4, 5, 0}; /* earliest version we have version info in file */
    DBVersionDigits(&a_digits[0], &a_digits[1], &a_digits[2], 0);
    DBFileVersionDigits(dbfile, &b_digits[0], &b_digits[1], &b_digits[2], 0);
    return db_compare_version_digits(a_digits, b_digits, 3) >= 0;
}

/*-------------------------------------------------------------------------
 * Function:    DBFileVersion
 *
 * Purpose:     Return the version number of the library that created the
 *              given file as a string.
 *
 * Returns:     ptr to version number
 *
 * Programmer:  Mark C. Miller, Mon Jan 12 20:59:30 PST 2009
 *-------------------------------------------------------------------------*/
PUBLIC char const *
DBFileVersion(const DBfile *dbfile)
{
    static char version[256];
    if (dbfile->pub.file_lib_version)
        strcpy(version, dbfile->pub.file_lib_version);
    else
        strcpy(version, "unknown; 4.5 or older");
    return version;
}

PUBLIC int
DBFileVersionDigits(const DBfile *dbfile, int *maj, int *min, int *pat, int *pre)
{
    int digits[4] = {0,0,0,0};
    if (!db_parse_version_digits_from_string(DBFileVersion(dbfile), '.',
             digits, sizeof(digits)/sizeof(digits[0])))
    {
        if (maj) *maj = digits[0];
        if (min) *min = digits[1];
        if (pat) *pat = digits[2];
        if (pre) *pre = digits[3];
        return 0;
    }
    return -1;
}

/*-------------------------------------------------------------------------
 * Function:    DBFileVersionGE
 *
 * Purpose:     Return whether or not the given file was created with a 
 *              version of the library greater than or equal to the
 *              version specified by Maj, Min, Pat 
 *
 * Returns:     1 if file version is greather than or equal to Maj/Min/Pat
 *              0 if file version is less than Maj/Min/Pat
 *             -1 if unable to determine.
 *
 * Programmer:  Mark C. Miller, Mon Jan 12 20:59:30 PST 2009
 *-------------------------------------------------------------------------*/
PUBLIC int
DBFileVersionGE(const DBfile *dbfile, int Maj, int Min, int Pat)
{
    int retval = -1;
    int unknown = 0;
    int a_digits[3];
    int b_digits[3] = {Maj<0?0:Maj, Min<0?0:Min, Pat<0?0:Pat};
    char *version = STRDUP(DBFileVersion(dbfile));

    if (strncmp(version, "unknown", 7) == 0)
    {
        /* We started maintaining library version information in the file
           in version 4.5.1. So, if it is 'unknown', we can return something
           useful ONLY if the version we're comparing against is 4.5.1 or
           greater. */
        a_digits[0] = 4;
        a_digits[1] = 5;
        a_digits[2] = 0;
        unknown = 1;
    }
    else
    {
        if (db_parse_version_digits_from_string(version, '.',
                 a_digits, sizeof(a_digits)/sizeof(a_digits[0])))
        {
            free(version);
            return -1;
        }
    }

    free(version);

    retval = db_compare_version_digits(a_digits, b_digits, 3) >= 0; 

    if (unknown)
    {
        if (retval)
            retval = -1;
    }

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    DBFileName
 *
 * Purpose:     Return the name of the associated file.
 *
 * Returns:     ptr to string holding the name of the file or "unknown".
 *
 * Programmer:  Mark C. Miller, Tue May 24 12:45:53 PDT 2016
 *-------------------------------------------------------------------------*/
PUBLIC char const *
DBFileName(const DBfile *dbfile)
{
    static char name[256];
    if (dbfile->pub.name)
        strcpy(name, dbfile->pub.name);
    else
        strcpy(name, "unknown");
    return name;
}

static void
db_InitFileGlobals(DBfile *dbfile, int mode)
{
    int i;

    dbfile->pub.file_scope_globals = (SILO_Globals_t*) malloc(sizeof(SILO_Globals_t));
    /* since most entries are ints and we want to init them to the NOT-SET value
       this call averts *some* mistakes where SILO_Globals_t might get updated with
       new int members but failed to be explicitly initialize here. The value of
       DB_INTBOOL_NOT_SET is accepted as int but is ultimately cast to unsigned char
       internally in memset(). Given that its integer value is -1 (0xFFFFFFFF), its
       conversion to unsigned char will be 0xFF and will have the effect of setting
       all int members of SILO_Globals_t to -1 or DB_INTBOOL_NOT_SET. */
    memset(dbfile->pub.file_scope_globals, DB_INTBOOL_NOT_SET, sizeof(SILO_Globals_t));

    /* Initialize to NOT-SET values so that, initially, a file is set to
       inheret *and* track all behaviors from Silo's globals. The first time
       any of these values are changed to a NOT_SET value, then forevermore, 
       that particular behavior is governed by the file */
    dbfile->pub.file_scope_globals->dataReadMask            = DB_MASK_NOT_SET;
    dbfile->pub.file_scope_globals->allowOverwrites         = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->allowEmptyObjects       = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->enableChecksums         = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->enableFriendlyHDF5Names = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->enableGrabDriver        = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->darshanEnabled          = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->allowLongStrComponents  = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->maxDeprecateWarnings    = DB_INTBOOL_NOT_SET;
#ifndef _WIN32
#warning ADD compressionMinSize
#endif
    dbfile->pub.file_scope_globals->compressionMinratio     = DB_FLOAT_NOT_SET;
#ifndef _WIN32
#warning CORRECT INITIALIZATION OF compressionErrmode
#endif
    dbfile->pub.file_scope_globals->compressionErrmode      = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->compatibilityMode       = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->compressionParams       = (char*) DB_CHAR_PTR_NOT_SET;
    dbfile->pub.file_scope_globals->_db_err_level           = DB_INTBOOL_NOT_SET;
    dbfile->pub.file_scope_globals->_db_err_func            = DB_VOID_PTR_NOT_SET;
    dbfile->pub.file_scope_globals->_db_err_level_drvr      = DB_INTBOOL_NOT_SET;

    /* Jstk only relevant for lib as a whole, not specific to a file */
    dbfile->pub.file_scope_globals->Jstk = 0;

    for (i = 0; i < MAX_FILE_OPTIONS_SETS; i++)
        dbfile->pub.file_scope_globals->fileOptionsSets[i] = 0;

    /* If compatibility is set in mode, then set it in file_scope_globals too */
    if (mode & 0x000000F0)
        dbfile->pub.file_scope_globals->compatibilityMode = mode & 0x000000F0;
}

/*-------------------------------------------------------------------------
 * Function:    DBOpen
 *
 * Purpose:     Open a data file.
 *
 * Return:      Success:        pointer to new file descriptor
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Mon Nov  7 10:25:08 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Robb Matzke, Tue Feb 28 10:51:19 EST 1995
 *    When a file is opened, it is given a unique ID number wrt all other
 *    open files.  The ID is a small integer [0..DB_NFILES-1].
 *
 *    Robb Matzke, Tue Feb 28 11:38:08 EST 1995
 *    For each registered filter, call the non-null `init' functions for
 *    every file that is opened.
 *
 *    Sean Ahern, Mon Jan  8 17:38:18 PST 1996
 *    Added the mode parameter.
 *
 *    Lisa J. Nafziger, Wed Mar  6 10:20:48 PST 1996
 *    Added code to check for file existence, to check if it is a
 *    directory and to check for read permission.  This allows more
 *    specific error messages to be returned.
 *
 *    Lisa J. Nafziger, Tue Mar 12 14:15:06 PST 1996
 *    Modified code to check file attributes so that stat() rather
 *    than access() is used.  The former is POSIX compliant.
 *
 *    Eric Brugger, Tue Jun 17 10:25:57 PDT 1997
 *    I modified the routine to only check file validity if the type
 *    is not an SDX connection.
 *
 *    Jeremy Meredith, Fri Jul 23 09:31:14 PDT 1999
 *    I added error reporting to the result of stat().
 *
 *    Jeremy Meredith, Mon Jul 26 10:39:49 PDT 1999
 *    Made stat() error reporting POSIX.1 compliant.
 *
 *    Sean Ahern, Wed Jul  5 15:35:48 PDT 2000
 *    Renamed the function to DBOpenReal.  Client code now calls a macro
 *    called DBOpen.
 *
 *    Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *    Added printing of error message from stat() with strerror
 *
 *    Mark C. Miller, Wed Feb 23 08:51:35 PST 2005
 *    Added code to reset _db_fstatus slot to 0 if open fails
 *
 *    Thomas R. Treadway, Tue Jun 27 13:59:21 PDT 2006
 *    Added HAVE_STRERROR wrappers
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Added code to register the returned file pointer
 *
 *    Mark C. Miller, Mon Jan 12 20:50:41 PST 2009
 *    Removed DB_SDX conditionally compiled code blocks.
 *
 *    Mark C. Miller, Wed Feb 25 23:50:06 PST 2009
 *    Moved call to db_isregistered_file to AFTER calls to stat the file
 *    add changed db_isregistered_file to accept stat struct instead of name.
 *    Changed call to db_register_file to accpet stat struct.
 *
 *    Mark C. Miller, Fri Feb 12 08:22:41 PST 2010
 *    Replaced stat/stat64 calls with db_silo_stat. Replaced conditional
 *    compilation logic for SIZEOF_OFF64_T with db_silo_stat_struct.
 *------------------------------------------------------------------------- */
PUBLIC DBfile *
DBOpenReal(const char *name, int type, int mode)
{
    char           ascii[16];
    DBfile        *dbfile;
    int            fileid, i;
    int            origtype = type;
    int            opts_set_id;
    db_silo_stat_t filestate;

    API_BEGIN("DBOpen", DBfile *, NULL) {
        if (DB_NOBJ_TYPES != _db_nobj_types)
            API_ERROR("Silo TOC not configured corretly", E_INTERNAL);

        if (!name)
            API_ERROR(NULL, E_NOFILE);

        /* deal with extended driver type specifications */
        db_DriverTypeAndFileOptionsSetId(origtype, &type, &opts_set_id);

        if (type < 0 || type >= DB_NFORMATS) {
            sprintf(ascii, "%d", type);
            API_ERROR(ascii, E_BADFTYPE);
        }
        if (((mode & 0x00000003) != DB_READ) && ((mode & 0x00000003) != DB_APPEND))
        {
            sprintf(ascii, "%d", mode);
            API_ERROR(ascii, E_BADARGS);
        }
        if (!DBOpenCB[type]) {
            sprintf(ascii, "%d", type);
            API_ERROR(ascii, E_NOTIMP);
        }

        /****************************************************/
        /* Check to make sure the file exists and has the   */
        /* correct permissions.                             */
        /****************************************************/
        if (db_silo_stat(name, &filestate, type==DB_UNKNOWN?-1:opts_set_id) != 0)
        {
            if( errno == ENOENT )
            {
                /********************************/
                /* File doesn't exist.          */
                /********************************/
                API_ERROR((char *)name, E_NOFILE);
            }
            else
            {
                /********************************/
                /* System level error occured.  */
                /********************************/
#if SIZEOF_OFF64_T > 4 && (defined(HAVE_STAT64) || !defined(HAVE_STAT))
                printf("stat64() failed with error: ");
#else
                printf("stat() failed with error: ");
#endif
                switch (errno)
                {
                  case EACCES:       printf("EACCES\n");       break;
                  case EBADF:        printf("EBADF\n");        break;
                  case ENAMETOOLONG: printf("ENAMETOOLONG\n"); break;
                  case ENOTDIR:      printf("ENOTDIR\n");      break;
#ifdef EOVERFLOW
                  case EOVERFLOW:    
#ifdef HAVE_STRERROR
                                     printf("EOVERFLOW: \"%s\"\n", 
                                        strerror(errno));
#else
                                     printf("EOVERFLOW: errno=%d\n", errno);
#endif
                                     printf("Silo may need to be re-compiled with "
                                            "Large File Support (LFS)\n");
                                     break;
#endif
                  default:           
#ifdef HAVE_STRERROR
                                     printf("\"%s\"\n",
                                        strerror(errno));
#else
                                     printf("errno=%d\n", errno);
#endif
                                     break;
                }
                API_ERROR((char *)name, E_SYSTEMERR);
            }
        }

        /* Check if file is already opened. If so, none can
           have it opened for write, including this new one */ 
        i = db_isregistered_file(0, &filestate);
        if (i != -1)
        {
            if (_db_regstatus[i].w != 0 || ((mode & 0x00000003) != DB_READ))
                API_ERROR(name, E_CONCURRENT);
        }

        if( ( filestate.s.st_mode & S_IFDIR ) != 0 )
        {
            /************************************/
            /* File is actually a directory.    */
            /************************************/
            API_ERROR((char *)name, E_FILEISDIR);
        }
        if( ( filestate.s.st_mode & S_IREAD ) == 0 )
        {
            /****************************************/
            /* File is missing read permissions.    */
            /****************************************/
            API_ERROR((char *)name, E_FILENOREAD);
        }
        if (DB_READ!=(mode&0x0000000F) && (filestate.s.st_mode & S_IWUSR) == 0)
        {
            /****************************************/
            /* File is open for write and missing write permission. */
            /****************************************/
            API_ERROR((char *)name, E_FILENOWRITE);
        }

        if ((fileid = db_get_fileid(DB_ISOPEN)) < 0)
            API_ERROR((char *)name, E_MAXOPEN);
        if (NULL == (dbfile = (DBOpenCB[type]) (name, mode, opts_set_id)))
        {
            _db_fstatus[fileid] = 0;
            API_RETURN(NULL);
        }
        dbfile->pub.fileid = fileid;
        db_InitFileGlobals(dbfile, mode);
        db_register_file(dbfile, &filestate, (((mode&0x00000003)!=DB_READ)));

        /*
         * Install filters.  First, all `init' filters, then the
         * specified filters.
         */
        for (i = 0; i < DB_NFILTERS; i++) {
            if (_db_filter[i].name && _db_filter[i].init) {
                (void)(_db_filter[i].init) (dbfile, _db_filter[i].name);
            }
        }
        db_filter_install(dbfile);
        if (DBInqVarExists(dbfile, SILO_VSTRING_NAME))
            dbfile->pub.file_lib_version = (char*)DBGetVar(dbfile, SILO_VSTRING_NAME);

        API_RETURN(dbfile);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBCreateReal
 *
 * Purpose:     Create a data file
 *
 * Return:      Success:        pointer to file descriptor
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Mon Nov  7 10:29:23 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Robb Matzke, Tue Feb 28 10:57:06 EST 1995
 *    A file ID is assigned to the new file.
 *
 *    Robb Matzke, 15 May 1996
 *    Removed the unused `statue' auto variable.
 *
 *    Sean Ahern, Wed Jul  5 15:35:48 PDT 2000
 *    Renamed the function to DBCreateReal.  Client code now calls a macro
 *    called DBCreate.
 *
 *    Mark C. Miller, Wed Feb 23 08:51:35 PST 2005
 *    Added code to reset _db_fstatus slot to 0 if create fails
 *
 *    Mark C. Miller, Wed Apr  5 10:17:31 PDT 2006
 *    Added code to output silo library version string to the file
 *
 *    Mark C. Miller, Mon Nov 19 10:45:05 PST 2007
 *    Added hdf5 driver warning.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Added code to register the returned file pointer
 *
 *    Mark C. Miller, Mon Nov 17 19:04:39 PST 2008
 *    Added code to check to see if name is a directory.
 *
 *    Mark C. Miller, Wed Feb 25 23:52:05 PST 2009
 *    Moved call to db_isregistered_file to after stat calls. Stat the
 *    file after its created so we can get information to register it.
 *
 *    Mark C. Miller, Fri Feb 12 08:22:41 PST 2010
 *    Replaced stat/stat64 calls with db_silo_stat. Replaced conditional
 *    compilation logic for SIZEOF_OFF64_T with db_silo_stat_struct.
 *
 *    Mark C. Miller, Thu Aug 30 17:41:24 PDT 2012
 *    Added logic to temporarily disable any compression settings prior
 *    to writing silo library info and then re-enabling it.
 *-------------------------------------------------------------------------*/
PUBLIC DBfile *
DBCreateReal(const char *name, int mode, int target, const char *info, int type)
{
    char           ascii[16];
    char           *tmpcs = 0;
    DBfile        *dbfile;
    int            fileid, i, n;
    int            origtype = type;
    int            opts_set_id = 0;
    db_silo_stat_t filestate;

    API_BEGIN("DBCreate", DBfile *, NULL) {
        if (DB_NOBJ_TYPES != _db_nobj_types)
            API_ERROR("Silo TOC not configured corretly", E_INTERNAL);

        if (!name)
            API_ERROR(NULL, E_NOFILE);

        /* deal with extended driver type specifications */
        db_DriverTypeAndFileOptionsSetId(origtype, &type, &opts_set_id);

        if (type < 0 || type >= DB_NFORMATS) {
            sprintf(ascii, "%d", type);
            API_ERROR(ascii, E_BADFTYPE);
        }

        if (db_silo_stat(name, &filestate, opts_set_id) == 0)  /* Success - File exists */
        {
            if ((mode & 0x0000000F) == DB_NOCLOBBER)
            {
                API_ERROR((char *)name, E_FEXIST);
            }
            if ((filestate.s.st_mode & S_IFDIR) != 0)
            {
                API_ERROR((char *)name, E_FILEISDIR);
            }

            /* Check if file is already opened. If so, none can
               have it opened for write, including this new one */
            i = db_isregistered_file(0, &filestate);
            if (i != -1)
            {
                API_ERROR(name, E_CONCURRENT);
            }
        }

        if (!DBCreateCB[type]) {
            sprintf(ascii, "%d", type);
            if (type == 7)
            {
                API_ERROR(ascii, E_NOHDF5);
            }
            else
            {
                API_ERROR(ascii, E_NOTIMP);
            }
        }

        if ((fileid = db_get_fileid(DB_ISOPEN)) < 0)
            API_ERROR((char *)name, E_MAXOPEN);
        dbfile = ((DBCreateCB[type]) (name, mode, target, opts_set_id,
                                      info));
        if (!dbfile)
        {
            _db_fstatus[fileid] = 0;
            API_RETURN(NULL);
        }
        dbfile->pub.fileid = fileid;
        db_InitFileGlobals(dbfile, mode);
        db_silo_stat(name, &filestate, opts_set_id);
        db_register_file(dbfile, &filestate, 1);

        /*
         * Install filters.  First all `init' routines, then the specified
         * `open' routines.
         */
        for (i = 0; i < DB_NFILTERS; i++) {
            if (_db_filter[i].name && _db_filter[i].init) {
                (void)(_db_filter[i].init) (dbfile, _db_filter[i].name);
            }
        }
        db_filter_install(dbfile);

        /* write silo library version information to the file */
        /* Temporarily turn off any compression settings and then re-enable */
        if (DBGetCompression())
        {
    
            n = strlen(DBGetCompression());
            tmpcs = ALLOC_N(char,n+1);
            strncpy(tmpcs, DBGetCompression(), n);
            tmpcs[n] = '\0';
            DBSetCompression(0);
        }
        n = strlen(SILO_VSTRING)+1;
        DBWrite(dbfile, SILO_VSTRING_NAME, SILO_VSTRING, &n, 1, DB_CHAR);
        dbfile->pub.file_lib_version = STRDUP(SILO_VSTRING);
        if (tmpcs)
        {
            DBSetCompression(tmpcs);
            FREE(tmpcs);
        }

        API_RETURN(dbfile);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBClose
 *
 * Purpose:     Close the specified data file and return NULL.
 *
 * Return:      Success:        NULL
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Mon Nov  7 10:31:41 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Eric Brugger, Mon Feb 27 15:03:01 PST 1995
 *    I changed the return value to be an integer instead of a pointer
 *    to a DBfile.
 *
 *    Robb Matzke, Tue Feb 28 10:57:57 EST 1995
 *    The file status slot is cleared so it can be reused.
 *
 *    Eric Brugger, Mon Jul 10 07:42:24 PDT 1995
 *    I moved the reseting of _db_fstatus to before the return statement,
 *    so that the instruction would get executed.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *    Added code to UNregister the given file pointer.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBClose(DBfile *dbfile)
{
    int            id;
    int            retval;
    SILO_Globals_t *tmp_file_scope_globals;

    API_BEGIN2("DBClose", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (NULL == dbfile->pub.close)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
#ifndef _WIN32
#warning IS ORDER OF OPS CORRECT HERE
#endif
        id = dbfile->pub.fileid;
        if (id >= 0 && id < DB_NFILES)
            _db_fstatus[id] = 0;

        if (dbfile->pub.file_lib_version)
            free(dbfile->pub.file_lib_version);
        db_unregister_file(dbfile);

	tmp_file_scope_globals = dbfile->pub.file_scope_globals; 
        retval = (dbfile->pub.close) (dbfile);
        free(tmp_file_scope_globals);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}


/*-------------------------------------------------------------------------
 * Function:    DBFlush
 *
 * Purpose:     Flush the specified file contents to disk.
 *
 * Return:      Success:        NULL
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller, Fri Nov  6 09:38:05 PST 2015
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
PUBLIC int
DBFlush(DBfile *dbfile)
{
    int            id;
    int            retval;

    API_BEGIN2("DBFlush", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (NULL == dbfile->pub.flush)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        retval = (dbfile->pub.flush) (dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine:  db_inq_file_has_silo_objects_r
 *
 * Purpose:  Recursive helper func for DBInqFileHasObjects 
 *
 * Programmer: Mark C. Miller, Wed Sep 23 11:34:01 PDT 2009
 *
 * Modifications:
 *   Mark C. Miller, Mon Nov 16 10:28:41 PST 2009
 *   Fixed dir recursion by copying dir-related toc entries. Removed
 *   misc. vars from count of silo objects.
 *--------------------------------------------------------------------*/

PRIVATE int
db_inq_file_has_silo_objects_r(DBfile *f)
{
    int i, ndir, retval = 0;
    char **dirnames;
    DBtoc *toc = DBGetToc(f);

    if (!toc)
        return -1;

    /* save dirnames so we don't loose 'em as we get new tocs */
    ndir = toc->ndir;
    dirnames = (char **) malloc(ndir * sizeof(char*));
    for (i = 0; i < ndir; i++)
        dirnames[i] = STRDUP(toc->dir_names[i]);
     
    /* We exclude dirs and misc. vars because a non-Silo file may
     * contain them. */
    retval = toc->ncurve + toc->ncsgmesh + toc->ncsgvar + toc->ndefvars +
        toc->nmultimesh + toc->nmultimeshadj + toc->nmultivar +
        toc->nmultimat + toc->nmultimatspecies + toc->nqmesh +
        toc->nqvar + toc->nucdmesh + toc->nucdvar + toc->nptmesh +
        toc->nptvar + toc->nmat + toc->nmatspecies +
        toc->nobj + toc->nmrgtree + toc->ngroupelmap +
        toc->nmrgvar + toc->narray;

    /* Recurse on directories. */
    for (i = 0; i < ndir && retval == 0; i++)
    {
        DBSetDir(f, dirnames[i]);
        retval += db_inq_file_has_silo_objects_r(f);
        DBSetDir(f, "..");
    }

    /* free the dirnames */
    for (i = 0; i < ndir; i++)
        free(dirnames[i]);
    free(dirnames);

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    DBInqFileHasObjects
 *
 * Purpose:     See if the file contains any silo objects, excluding
 *              directories in the search.
 *
 * Return:      Success:         >0 ==> yes, the file has silo objects.
 *                              ==0 ==> no, the file has no silo objects.
 *
 *              Failure:        -1 
 *
 * Programmer:  Mark C. Miller, Wed Sep 23 09:42:27 PDT 2009
 *
 * Modifications:
 *   Mark C. Miller, Mon Nov 16 10:29:36 PST 2009
 *   Added logic to test from some well known, tell-tale silo variables.
 *-------------------------------------------------------------------------*/

PUBLIC int
DBInqFileHasObjects(DBfile *f)
{
    char cwd[4096];
    int retval;

    if (f == 0)
        return -1;

    if (DBInqVarExists(f, "_silolibinfo"))
        return 1;
    if (DBInqVarExists(f, "_hdf5libinfo"))
        return 1;

    DBGetDir(f, cwd);
    retval = db_inq_file_has_silo_objects_r(f);
    DBSetDir(f, cwd);    

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    DBInqFileReal
 *
 * Purpose:     Determines if the filename is a Silo file.
 *
 * Return:      0  if filename is not a Silo file, 
 *              >0 if filename is a Silo file,
 *              <0 if an error occurred.
 *
 * Programmer:  Hank Childs
 *              Tue Feb 29 16:24:01 PST 2000
 *
 * Modifications:
 *    Sean Ahern, Wed Jul  5 15:35:48 PDT 2000
 *    Renamed the function to DBInqFileReal.  Client code now calls a macro
 *    called DBInqFile.
 *
 *    Mark C. Miller, Wed Sep 23 11:48:19 PDT 2009
 *    Added logic to confirm that indeed the successfully opened file has
 *    some silo objects in it.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBInqFileReal(const char *filename)
{
    DBfile *dbfile = NULL;
    int hasobjects = -1;

    API_BEGIN("DBInqFile", int, -1) {
        if (!filename || ! *filename)
            API_ERROR("filename", E_BADARGS);

        /* 
         * Turn the error handling off so user won't see errors, 
         * won't abort, etc.
         */
        DBShowErrors(DB_SUSPEND, NULL);

        /*
         * Must protect this code so that the error handling can be
         * restored afterwards.
         */
        PROTECT {
            dbfile = DBOpen(filename, DB_UNKNOWN, DB_READ);
            if (dbfile)
                hasobjects = DBInqFileHasObjects(dbfile);
        } CLEANUP {
            CANCEL_UNWIND;
        } END_PROTECT;

        /* 
         * Turn the error handling back on. 
         */
        DBShowErrors(DB_RESUME, NULL);

        if (dbfile != NULL)
        {
            DBClose(dbfile);
            API_RETURN(hasobjects);
        }
 
        API_RETURN(0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 *  Routine                                               DBInqVarExists
 *
 *  Purpose
 *
 *      Determine if the given variable object exists in the SILO file.
 *      Return non-zero if it does and 0 if it doesn't.
 *
 *  Programmer
 *
 *      Sean Ahern, Thu Jul 20 11:53:40 PDT 1995
 *
 *  Modifications
 *    Mon Aug 28 11:15:21 PDT 1995
 *    (ahern) Changed the API_BEGIN to API_BEGIN2.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *--------------------------------------------------------------------*/
PUBLIC int
DBInqVarExists(DBfile *dbfile, const char *varname)
{
    int retval;

    API_BEGIN2("DBInqVarExists", int, 0, varname) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (!varname || !*varname)
            API_ERROR("variable name", E_BADARGS);
        if (dbfile->pub.exist == NULL)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        retval = (dbfile->pub.exist) (dbfile, varname);
        API_RETURN(retval);
    }
    API_END_NOPOP; /* BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBForceSingle
 *
 * Purpose:     If 'status' is non-zero, then any 'datatype'd arrays are
 *              converted on read from whatever their native datatype is to
 *              float. A 'datatype'd array is an array that is part of some
 *              Silo object containing a 'datatype' member which indicates
 *              the type of data in the array. So, for example, a DBucdvar
 *              has a 'datatype' member to indicate the type of data in the
 *              var and mixvar arrays. Such arrays will be converted on read
 *              if 'status' here is non-zero. However, a DBmaterial object 
 *              is ALWAYS integer data. There is no 'datatype' member for
 *              such an object and so its data will NEVER be converted to
 *              float on read regardless of force single status set here.
 *
 *              I believe this function's original intention was to convert
 *              only double precision arrays to single precision. However,
 *              the PDB driver was apparently never designed that way and
 *              the PDB driver's behavior sort of established the defacto
 *              meaning of force single. So, now, as of Silo version 4.8
 *              the HDF5 driver obeys it as well. Though, in fact the HDF5
 *              driver was originally written to support the original
 *              intention of force single status and it worked in this
 *              ('buggy') fashion for many years before we started
 *              encountering real problems with it in VisIt.
 *
 * Return:      Success:        0 if all drivers succeeded or did not
 *                              implement this function.
 *
 *              Failure:        -1 if any driver returned failure.
 *
 * Programmer:  matzke@viper
 *              Tue Jan 10 11:01:24 PST 1995
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Mark C. Miller, Fri Jul 16 19:28:23 PDT 2010
 *    Updated 'Purpose' above to reflect current understanding of the
 *    meaning of force single.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBForceSingle(int status)
{
    int            i;

    API_BEGIN("DBForceSingle", int, -1) {
        for (i = 0; i < DB_NFORMATS; i++) {
            if (DBFSingleCB[i]) {
                if (((DBFSingleCB[i]) (status)) < 0) {
                    char           dname[32];

                    sprintf(dname, "driver-%d", i);
                    API_ERROR(dname, E_CALLFAIL);
                }
            }
        }
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                                DBMakeOptlist
 *
 *  Purpose
 *
 *      Allocate an option list of the requested length and initialize it.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Modified
 *    Robb Matzke, Tue Nov 8 06:58:04 PST 1994
 *    Added error mechanism
 *--------------------------------------------------------------------*/
PUBLIC DBoptlist *
DBMakeOptlist(int maxopts)
{
    DBoptlist     *optlist = NULL;

    API_BEGIN("DBMakeOptlist", DBoptlist *, NULL) {
        if (maxopts <= 0)
            API_ERROR("maxopts", E_BADARGS);
        optlist = ALLOC(DBoptlist);
        if (!optlist) API_ERROR(NULL, E_NOMEM);
        optlist->options = ALLOC_N(int, maxopts);
        optlist->values = ALLOC_N(void *, maxopts);

        if (!optlist->options || !optlist->values)
        {
            FREE(optlist->values);
            FREE(optlist->options);
            FREE(optlist);
            API_ERROR(NULL, E_NOMEM);
        }

        optlist->numopts = 0;
        optlist->maxopts = maxopts;

        API_RETURN(optlist);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 *  Routine                                                DBFreeOptlist
 *
 *  Purpose
 *
 *      Release the storage associated with the given optlist list.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns 0 on success, -1 on failure.
 *
 *  Modified
 *    Robb Matzke, Tue Nov 8 07:56:34 PST 1994
 *    Added error mechanism.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *--------------------------------------------------------------------*/
PUBLIC int
DBFreeOptlist(DBoptlist *optlist)
{
    API_BEGIN("DBFreeOptlist", int, -1) {
        if (!optlist || optlist->numopts < 0) {
            API_ERROR("optlist pointer", E_BADARGS);
        }
        FREE(optlist->options);
        FREE(optlist->values);
        FREE(optlist);
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                                DBClearOptlist
 *
 *  Purpose
 *
 *      Remove all options from the given optlist and reset counters.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 *  Modified
 *    Robb Matzke, Tue Nov 8 07:48:52 PST 1994
 *    Added error mechanism.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *--------------------------------------------------------------------*/
PUBLIC int
DBClearOptlist(DBoptlist *optlist)
{
    int            i;

    API_BEGIN("DBClearOptlist", int, -1) {
        if (!optlist || optlist->numopts < 0) {
            API_ERROR("optlist pointer", E_BADARGS);
        }

        /* Reset values, but do not free */
        for (i = 0; i < optlist->maxopts; i++) {
            optlist->options[i] = 0;
            optlist->values[i] = (void *)NULL;
        }

        optlist->numopts = 0;
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                                  DBAddOption
 *
 *  Purpose
 *
 *      Add an option to the given option list structure.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 *  Modified:
 *    Robb Matzke, Tue Nov 8 07:00:55 PST 1994
 *    Added error mechanism.  Returns -1 on failure, 0 on success.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *--------------------------------------------------------------------*/
PUBLIC int
DBAddOption(DBoptlist *optlist, int option, void *value)
{
    API_BEGIN("DBAddOption", int, -1) {
        if (!optlist)
            API_ERROR("optlist pointer", E_BADARGS);
        if (optlist->numopts >= optlist->maxopts)
            API_ERROR("optlist nopts", E_BADARGS);

        optlist->options[optlist->numopts] = option;
        optlist->values[optlist->numopts] = value;
        optlist->numopts++;

        if (optlist->numopts >= optlist->maxopts)
        {
            int new_maxopts = optlist->maxopts * 1.5 + 1; /* golden rule + 1 */
            int *new_options = REALLOC_N(optlist->options, int, new_maxopts);
            void **new_values = REALLOC_N(optlist->values, void*, new_maxopts);

            if (!new_options || !new_values)
            {
                FREE(new_options);
                FREE(new_values);
                API_ERROR(0, E_NOMEM);
            }

            optlist->maxopts = new_maxopts;
            optlist->options = new_options;
            optlist->values  = new_values;
        }

    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                                DBClearOption
 *
 *  Purpose
 *
 *      Remove a given option from the given optlist and re-order
 *      the remaining options.
 *
 *  Programmer
 *
 *      Mark C. Miller, August 18, 2005 
 *
 *--------------------------------------------------------------------*/
PUBLIC int
DBClearOption(DBoptlist *optlist, int option)
{
    int            i, j, foundit=0;

    API_BEGIN("DBClearOption", int, -1) {
        if (!optlist || optlist->numopts < 0) {
            API_ERROR("optlist pointer", E_BADARGS);
        }

        /* Shift values down in list by one entry */
        for (i = 0; i < optlist->numopts; i++) {
            if (optlist->options[i] == option) {
                foundit = 1;
                for (j = i; j < optlist->numopts-1; j++) {
                    optlist->options[j] = optlist->options[j+1];
                    optlist->values[j]  = optlist->values[j+1];
                }
                break;
            }
        }

        if (foundit) {
            optlist->numopts--;
            optlist->options[optlist->numopts] = 0;
            optlist->values[optlist->numopts]  = 0;
        }
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                                DBGetOption
 *
 *  Purpose
 *
 *      Return value set for a given option from the given optlist.
 *
 *  Programmer
 *
 *      Mark C. Miller, August 18, 2005 
 *
 *  Modifications:
 *
 *      Mark C. Miller, Wed Jul 14 20:35:50 PDT 2010
 *      Replaced 'return' with 'API_RETURN'
 *
 *      Mark C. Miller, Tue Aug 10 23:49:51 PDT 2010
 *      Removed API_BEGIN/END stuff so that function can be handed
 *      a null optlist and it will behave well.
 *--------------------------------------------------------------------*/
PUBLIC void * 
DBGetOption(const DBoptlist *optlist, int option)
{
    int            i;

    if (!optlist) return 0;

    /* find the given option in the optlist and return its value */
    for (i = 0; i < optlist->numopts; i++)
        if (optlist->options[i] == option)
            return optlist->values[i];

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    DBGetToc
 *
 * Purpose:     Return a pointer to table of contents of the file.  Note
 *              that the pointer is the same as the one in the DBfile
 *              so it should not be modified and may become invalid after
 *              calling the next silo routine.
 *
 * Return:      Success:        Pointer to the table of contents structure.
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Mon Nov  7 10:35:47 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Fri Jan 27 08:23:43 PST 1995
 *    I changed the interface and function of the routine.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *-------------------------------------------------------------------------*/
PUBLIC DBtoc  *
DBGetToc(DBfile *dbfile)
{
    API_BEGIN2("DBGetToc", DBtoc *, NULL, api_dummy) {
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("", E_GRABBED) ; 
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);

        DBNewToc(dbfile);
        API_RETURN(dbfile->pub.toc);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBInqVarType
 *
 * Purpose:     Return the DBObjectType for a given object name
 *
 * Return:      Success:        the ObjectType for the given object
 *
 *              Failure:        DB_INVALID_OBJECT
 *
 * Programmer:  Sean Ahern,
 *              Wed Oct 28 14:46:53 PST 1998
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *-------------------------------------------------------------------------*/
PUBLIC DBObjectType
DBInqVarType(DBfile *dbfile, char const *varname)
{
    DBObjectType retval;

    API_BEGIN2("DBInqVarType", DBObjectType, DB_INVALID_OBJECT, api_dummy) {
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("", E_GRABBED) ; 
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (!varname || !*varname)
            API_ERROR("variable name", E_BADARGS);
        if (!dbfile->pub.inqvartype)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.inqvartype) (dbfile, varname);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBNewToc
 *
 * Purpose:     Used to be called `DBGetToc', this function installs a
 *              new table of contents in the specified file from that
 *              file's current working directory.  The old table of contents
 *              is destroyed.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Tue Mar  7 10:26:23 EST 1995
 *
 * Modifications:
 *              Robb Matzke, 2000-05-23
 *              If nothing has changed then this function just returns
 *              success, leaving the original table of contents in place.
 *              Any function that potentially changes the table of
 *              contents should call db_FreeToc() on the file handle.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBNewToc(DBfile *dbfile)
{
    int retval;

    API_BEGIN("DBNewToc", int, -1) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("", E_GRABBED) ; 
        if (!dbfile->pub.newtoc)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        if (dbfile->pub.toc)
            API_RETURN(0);
        retval = (dbfile->pub.newtoc) (dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*
This logic is necessary to support callers accessing standard (e.g. non-DB_USERDEF)
objects via the generic interface to ensure logic for specially handled component
values is consistent with the standard object's DBGetXXX methods.
To ensure a zero for the component in the file means either not-present or
not-set, certain components, especially those for which a zero value is a valid
value for the data producer to use, special handling is required and that logic
is sprinkled about in the DBPutXXX and DBGetXXX methods of the drivers and so
to is it required here. Its a nasty maintenance issue.
*/

static int
db_IsComponentNameStandardWithSpecialHandling(char const *compname)
{
    if (!strcmp(compname, "missing_value")) return 1;
    if (!strcmp(compname, "topo_dim")) return 1;
    if (!strcmp(compname, "repr_block_idx")) return 1;
    return 0;
}

/* For use in the DBGetComponent call */
static void
db_AdjustSpeciallyHandledStandardObjectComponentValue(
    void *val_ptr, int obj_type, char const *comp_name)
{
    if (!val_ptr || !comp_name) return;

    if (!strcmp(comp_name, "missing_value") &&
        (obj_type == DB_UCDVAR || obj_type == DB_QUADVAR || obj_type == DB_CURVE ||
         obj_type == DB_POINTVAR || obj_type == DB_MULTIVAR))
    {
        double val_for_mem, val_from_file = *((double*)val_ptr);
        db_SetMissingValueForGet(val_for_mem, val_from_file);
        *((double*)val_ptr) = val_for_mem;
    }
    else if (!strcmp(comp_name, "repr_block_idx") &&
        (obj_type == DB_MULTIMESH || obj_type == DB_MULTIVAR ||
         obj_type == DB_MULTIMAT || obj_type == DB_MULTIMATSPECIES))
    {
        int val_for_mem, val_from_file = *((int*)val_ptr);
        val_for_mem = val_from_file - 1;
        *((int*)val_ptr) = val_for_mem;
    }
    else if (!strcmp(comp_name, "topo_dim") &&
        (obj_type == DB_MULTIMESH || obj_type == DB_UCDMESH))
    {
        int val_for_mem, val_from_file = *((int*)val_ptr);
        val_for_mem = val_from_file - 1;
        *((int*)val_ptr) = val_for_mem;
    }
}

/* For use in the DBGetObject call */
static void
db_AdjustSpeciallyHandledStandardObjectComponentValues(DBobject *obj)
{
    int i, obj_type;

    if (!obj) return;

    obj_type = DBGetObjtypeTag(obj->type);
    if (obj_type == DB_USERDEF) return;

    for (i = 0; i < obj->ncomponents; i++)
    {
        char tmp[256];

        if (!strcmp(obj->comp_names[i], "missing_value") &&
            (obj_type == DB_UCDVAR || obj_type == DB_QUADVAR || obj_type == DB_CURVE ||
             obj_type == DB_POINTVAR || obj_type == DB_MULTIVAR))
        {
            double val_for_mem, val_from_file = strtod(obj->pdb_names[i]+4,0);
            db_SetMissingValueForGet(val_for_mem, val_from_file);
            sprintf(tmp, "'<d>%.30g'", val_for_mem);
        }
        else if (!strcmp(obj->comp_names[i], "repr_block_idx") &&
            (obj_type == DB_MULTIMESH || obj_type == DB_MULTIVAR ||
             obj_type == DB_MULTIMAT || obj_type == DB_MULTIMATSPECIES))
        {
            int val_for_mem, val_from_file = (int) strtol(obj->pdb_names[i]+4,0,10);
            val_for_mem = val_from_file - 1;
            sprintf(tmp, "'<i>%d'", val_for_mem);
        }
        else if (!strcmp(obj->comp_names[i], "topo_dim") &&
            (obj_type == DB_MULTIMESH || obj_type == DB_UCDMESH))
        {
            int val_for_mem, val_from_file = (int) strtol(obj->pdb_names[i]+4,0,10);
            val_for_mem = val_from_file - 1;
            sprintf(tmp, "'<i>%d'", val_for_mem);
        }
        else
        {
            continue;
        }

        FREE(obj->pdb_names[i]);
        obj->pdb_names[i] = STRDUP(tmp);
    }
}

/*----------------------------------------------------------------------
 *  Routine                                                DBGetComponent
 *
 *  Purpose
 *
 *      Return the requested component value for the given object.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Parameters
 *
 *      dbfile           {In}    {Pointer to current file}
 *      objname          {In}    {Name of object to inquire about}
 *      compname         {In}    {Name of component to return}
 *
 *  Notes
 *
 *  Modified
 *    Robb Matzke, Tue Nov 8 08:22:42 PST 1994
 *    Added error mechanism
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Eric Brugger, Wed Mar 10 16:59:34 PST 1999
 *    Changed API_BEGIN2 to API_BEGIN so that Silo directory information
 *    would be processed at the driver level, since the pdb driver
 *    version of this routine handles silo directory paths as well as
 *    file system directory paths, which API_BEGIN2 does not.
 *
 *    Eric Brugger, Thu Mar 11 12:33:15 PST 1999
 *    I forgot to remove the fourth argument when I changed API_BEGIN2
 *    to API_BEGIN.  I did so now.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *--------------------------------------------------------------------*/
PUBLIC void   *
DBGetComponent(DBfile *dbfile, char const *objname, char const *compname)
{
    void *retval = NULL;

    API_BEGIN2("DBGetComponent", void *, NULL, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetComponent", E_GRABBED) ; 
        if (!objname || !*objname)
            API_ERROR("object name", E_BADARGS);
        if (!compname || !*compname)
            API_ERROR("component name", E_BADARGS);
        if (!dbfile->pub.g_comp)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_comp) (dbfile, objname, compname);

        if (db_IsComponentNameStandardWithSpecialHandling(compname))
            db_AdjustSpeciallyHandledStandardObjectComponentValue(retval,
                DBInqVarType(dbfile, objname), compname);

        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 *  Routine                                           DBGetComponentType
 *
 *  Purpose
 *
 *      Return the type of a component for the given object.
 *
 *  Programmer
 *
 *      Brad Whitlock, Thu Jan 20 11:54:54 PDT 2000
 *
 *  Parameters
 *
 *      dbfile           {In}    {Pointer to current file}
 *      objname          {In}    {Name of object to inquire about}
 *      compname         {In}    {Name of component to return}
 *
 *  Modified
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *--------------------------------------------------------------------*/

PUBLIC int
DBGetComponentType(DBfile *dbfile, char const *objname, char const *compname)
{
    int retval = DB_NOTYPE;

    API_BEGIN2("DBGetComponentType", int, DB_NOTYPE, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("", E_GRABBED) ; 
        if (!objname || !*objname)
            API_ERROR("object name", E_BADARGS);
        if (!compname || !*compname)
            API_ERROR("component name", E_BADARGS);
        if (!dbfile->pub.g_comptyp)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_comptyp) (dbfile, objname, compname);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 *  Routine                                                     DBGetDir
 *
 *  Purpose
 *
 *      Get the name of the current directory, return in space provided.
 *
 *  Modified
 *    Robb Matzke, Tue Nov 8 08:48:12 PST 1994
 *    Added error mechanism
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *--------------------------------------------------------------------*/
PUBLIC int
DBGetDir(DBfile *dbfile, char *path)
{
    int retval;

    API_BEGIN2("DBGetDir", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetDir", E_GRABBED) ; 
        if (!path)
            API_ERROR("path", E_BADARGS);
        if (!dbfile->pub.g_dir)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_dir) (dbfile, path);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBSetDir
 *
 * Purpose:     Sets the current directory within the database.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Wed Nov  9 13:09:23 EST 1994
 *
 * Modifications:
 *    Robb Matzke, Mon Nov 21 21:31:17 EST 1994
 *    Added error mechanism.
 *
 *    Robb Matzke, Fri Jan 6 07:34:29 PST 1995
 *    Checkes for changing to `.' since that is a no-op.
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBSetDir(DBfile *dbfile, const char *path)
{
    char           tmp[256];
    int retval;

    API_BEGIN2("DBSetDir", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBSetDir", E_GRABBED) ; 
        if (!path || !*path)
            API_ERROR("path", E_BADARGS);
        if (STR_EQUAL(path, "."))
        {
            API_RETURN(0);
        }
        if (DBGetDir(dbfile, tmp) < 0)
            API_ERROR("DBGetDir", E_CALLFAIL);
        if (STR_EQUAL(tmp, path))
        {
            API_RETURN(0);
        }
        if (!dbfile->pub.cd)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.cd) (dbfile, path);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBFilters
 *
 * Purpose:     List the names of filters installed for the specified
 *              file.  The list is sent to the specified stream.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Tue Mar  7 10:51:58 EST 1995
 *
 * Modifications:
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBFilters(DBfile *dbfile, FILE *stream)
{
    int retval;

    API_BEGIN2("DBFilters", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBFilters", E_GRABBED) ; 
        if (!stream)
            stream = stdout;
        if (!dbfile->pub.module)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.module) (dbfile, stream);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBMkDir & DBMkDirP
 *
 * Purpose:     Creates a new directory in the database. The P variant
 *              obeys unix `mkdir -p` semantics making parent directories
 *              as necessary as does so *above* any specific driver using
 *              calls to DBGetDir(), DBSetDir() and DBMkDir().
 *
 * Return:      Success:        directory ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:46:21 PST 1994
 *
 * Modifications:
 *    Robb Matzke, Mon Nov 21 21:36:44 EST 1994
 *    Added error mechanism.
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBMkDir(DBfile *dbfile, const char *name)
{
    int retval;

    API_BEGIN2("DBMkDir", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBMkDir", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("directory name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("directory name", E_INVALIDNAME);
        if (!dbfile->pub.mkdir)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.mkdir) (dbfile, name);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC int
DBMkDirP(DBfile *dbfile, const char *name)
{
    char        cwg[1024], *p;
    char       *abspath = 0;
    int         abslen = 0;
    int         retval = 0;

    API_BEGIN2("DBMkDirP", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBMkDir", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("directory name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("directory name", E_INVALIDNAME);

        /* Save cwg */
        cwg[0] = '\0';
        DBGetDir(dbfile, cwg);
        abspath = db_absoluteOf_path(cwg,name);
        abslen = strlen(abspath);

        /* iterate setting and making one dir */
        p = abspath+abslen;

        /* Walk up path finding nearest existing parent */
        while (p > abspath)
        {
            if (DBInqVarType(dbfile, abspath[0]=='\0'?"/":abspath) == DB_DIR) break;
            while (p>abspath && '/'!=*p) p--;
            *p = '\0';
        }

        /* Walk down path creating missing parents */
        while (!retval && p < abspath+abslen)
        {
            DBSetDir(dbfile, abspath[0]=='\0'?"/":abspath);
            retval = DBMkDir(dbfile, p+1);
            *p = '/';
            while (p<abspath+abslen && '\0'!=*p) p++;
        }

        /* restore cwg */
        DBSetDir(dbfile, cwg);
        free(abspath);

        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}


/*-------------------------------------------------------------------------
 * Function:    DBCpDir
 *
 * Purpose:     Copies a directory tree from one file to another
 *
 * Return:      Success:        directory ID
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller, Wed Aug  6 15:14:33 PDT 2008
 *
 *-------------------------------------------------------------------------*/
PUBLIC int
DBCpDir(DBfile *dbfile, char const *srcDir,
        DBfile *dstFile, char const *dstDir)
{
    int retval;

    API_DEPRECATE2("DBCpDir", int, -1, api_dummy, 4, 11, "DBCp") {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (!dstFile)
            API_ERROR(NULL, E_NOFILE);
        if (db_isregistered_file(dstFile,0)==-1)
            API_ERROR(NULL, E_NOTREG);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR(NULL, E_GRABBED) ; 
        if (!srcDir || !*srcDir)
            API_ERROR("source directory name", E_BADARGS);
        if (!dstDir || !*dstDir)
            API_ERROR("destination directory name", E_BADARGS);
        if (db_VariableNameValid(dstDir) == 0)
            API_ERROR("destination directory name", E_INVALIDNAME);
        if (!dbfile->pub.cpdir)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.cpdir) (dbfile, srcDir, dstFile, dstDir);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBMkSymlink
 *
 * Purpose:     Create symbolic link in silo file's CWD with name link and 
 *              referencing target. If target string contains ':', then
 *              chars before ':' are interpreted as another silo file
 *              and chars after ':' are interpreted as object within that
 *              file. If file part specifies a relative path, then path is
 *              relative to directory in which dbfile exists in
 *              file system.
 *
 * Return:      Success:        directory ID
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller, Mon Apr 16 14:52:05 PDT 2018
 *
 *-------------------------------------------------------------------------*/
PUBLIC int
DBMkSymlink(DBfile *dbfile, char const *target, char const *link)
{
    int retval;

    API_BEGIN2("DBMkSymlink", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBMkDir", E_GRABBED) ; 
        if (!target || !*target)
            API_ERROR("target", E_BADARGS);
        if (db_VariableNameValid(target) == 0)
            API_ERROR("target", E_INVALIDNAME);
        if (!link || !*link)
            API_ERROR("link", E_BADARGS);
        if (db_VariableNameValid(link) == 0)
            API_ERROR("link", E_INVALIDNAME);
        if (!dbfile->pub.mksymlink)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.mksymlink) (dbfile, target, link);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/* return 0 if the toc_entry is not a symlink or the target of the
 * symlink if it is */
PUBLIC char const *
DBIsSymlink(DBtoc const *toc, char const *toc_entry)
{
    int i;

    if (!toc) return 0;
    if (!toc_entry || !*toc_entry) return 0;

    /* note: could use pointer compare instead of string
       compare if could guarantee the toc_entry is one of
       the members from this toc because symlink_name entry
       is really just a copy of other toc entry's name
       pointers */

    for (i = 0; i < toc->nsymlink; i++)
    {
        if (!strcmp(toc->symlink_names[i], toc_entry))
            return toc->symlink_target_names[i];
    }

    return 0;
}

PUBLIC int
DBGetSymlink(DBfile *dbfile, char const *in_candidate_link, char *out_target)
{
    int retval;
    API_BEGIN2("DBGetSymlink", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetSymlink", E_GRABBED);
        if (!in_candidate_link || !*in_candidate_link)
            API_ERROR("in_candidate_link", E_BADARGS);
        if (!dbfile->pub.g_symlink)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_symlink) (dbfile, in_candidate_link, out_target);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PRIVATE void
CheckForComponentSeries(DBobject const *srcObj, int i,
    int *isser, int *nser, char **sernm, char **sertstr, void **serd)
{
    char *p = 0;
    int pass = 0;
    int dtyp = DB_NOTYPE;
    int i0 = i;
    int nd = (int) strcspn(srcObj->comp_names[i0], "0123456789");

    /* advance while component names match in first nd chars */
    i++;
    while (i < srcObj->ncomponents)
    {
        if (strncmp(srcObj->comp_names[i], srcObj->comp_names[i0], nd) ||
            strncmp(srcObj->pdb_names[i], "'<", 2) ||
           !strncmp(srcObj->pdb_names[i], "'<s>/.silo/#", 12))
            break;
        i++;
    }

    /* If the series is of length one (or less) its not a series */
    if ((i-i0) <= 1)
    {
        *isser = 0;
        return;
    }

    *isser = 1;
    *nser = i-i0;
    *sernm = db_strndup(srcObj->comp_names[i0], nd);

    /* first pass get size, second pass fill buffer */
    for (pass = 0; pass < 2; pass++)
    {
        int j;

        if (pass == 1)
        {
            p = (char *) malloc((size_t)p);
            *serd = p;
        }

        for (j = i0; j < i; j++)
        {
            if      (!strncmp(srcObj->pdb_names[j], "'<i>", 4))
            {
                if (pass == 1)
                    *((int*)p) = (int) strtol(srcObj->pdb_names[j]+4, NULL, 0);
                p += sizeof(int);
                if (dtyp == DB_NOTYPE) dtyp = DB_INT;
            }
            else if (!strncmp(srcObj->pdb_names[j], "'<f>", 4))
            {
                if (pass == 1)
                     *((float*)p) = (float) strtod(srcObj->pdb_names[j]+4, NULL);
                p += sizeof(float);
                if (dtyp == DB_NOTYPE) dtyp = DB_FLOAT;
            }
            else if (!strncmp(srcObj->pdb_names[j], "'<d>", 4))
            {
                if (pass == 1)
                    *((double*)p) = strtod(srcObj->pdb_names[j]+4, NULL);
                p += sizeof(double);
                if (dtyp == DB_NOTYPE) dtyp = DB_DOUBLE;
            }
            else if (!strncmp(srcObj->pdb_names[j], "'<s>", 4))
            {
                int len = (int) strlen(srcObj->pdb_names[j])-5;
                if (pass == 1)
                {
                     strcpy(p, srcObj->pdb_names[j]+4);
                     p[len] = '\0';
                }
                p += (len+1);
                if (dtyp == DB_NOTYPE) dtyp = DB_CHAR;
            }
            else
            {
                int len = (int) strlen(srcObj->pdb_names[j]);
                if (pass == 1)
                     strcpy(p, srcObj->pdb_names[j]);
                p += (len+1);
            }
        }
    }

    *sertstr = db_GetDatatypeString(dtyp);
}

static int
db_validate_copy_step(int pass, int recurse,
    char const *srcName, char const *srcAbsName, int srcType,
    char const *dstName, char const *dstAbsName, int dstType)
{
    /* We validate everything 1rst pass so on 2nd, just ignore */
    if (pass == 1) return 1;

    /* Validate source object first */
    if (srcType == DB_INVALID_OBJECT)
    {
        db_perror("src object does not exist", E_BADARGS, srcName);
        return 0;
    }

    if (srcType == DB_DIR && !recurse)
    {
        db_perror("src is a directory (not copied)", E_BADARGS, srcName);
        return 0;
    }

    /* If destination does not exist, that is fine */
    if (dstType == DB_INVALID_OBJECT)
        return 1;

    /* If pre-existing destination type isn't same as src, that is a problem */
    if (dstType != srcType)
    {
        db_perror("pre-existing dst object type mismatch with src", E_BADARGS,
            dstName?dstName:srcName);
        return 0;
    }

    /* If we get here, we're overwriting the destination object and that
       is ok only if AllowOverwrites is enabled *and* the src object
       can fit within the space used by the dst object, the latter piece
       of logic we cannot currently check. */
    if (!DBGetAllowOverwrites())
    {
#ifndef _WIN32
#warning ONLY IF SRC SIZE SMALLER THAN DST SIZE
#endif
        db_perror("overwrite of pre-existing dst prevented due "
            "to DBSetAllowOverwrites(0)", E_BADARGS, dstName?dstName:srcName);
        return 0;
    }

    return 1;
}

#define COPY_TOC_ENTRY(TOC, NM, NUM, NAM)                        \
    NUM = TOC->n ## NM;                                          \
    NAM = (char **) malloc(NUM * sizeof(char*));                 \
    for (int q = 0 ; q < NUM; q++)                               \
    {                                                            \
        int len = strlen(TOC->NM ## _names[q]);                  \
        NAM[q] = (char*) calloc(len+1,1);                        \
        strncpy(NAM[q], TOC->NM ## _names[q],len);               \
    }

#define FREE_COPIED_TOC_ENTRY(NUM, NAM)                          \
    for (int q = 0; q < NUM; q++)                                \
        FREE(NAM[q]);                                            \
    FREE(NAM);

/*-------------------------------------------------------------------------
 * Function: db_can_overwrite_dstobj_with_srcobj
 *
 * Purpose: Compare two objects, member-by-member, to ensure the source
 * object is smaller than (e.g. will fit into the same file space as) the
 * destination object. There is possible recursion if the object contains
 * sub-objects (e.g. a zonelist object within an ucd mesh object).
 *
 * Returns: 1 if source object matches destination object *and* fits in
 *            in same space destination object occupies.
 *          0 otherwise.
 *
 * Programmer:  Mark C. Miller, Wed Apr 18 09:23:55  PDT 2018
 *
 *-------------------------------------------------------------------------*/
static int
db_can_overwrite_dstobj_with_srcobj(
    DBfile *srcFile, char const *srcAbsName,
    DBfile *dstFile, char const *dstAbsName)
{
    int q;
    int retval = 0; /* assume it won't work */
    DBobject *srcObj = 0;
    DBobject *dstObj = 0;

    srcObj = DBGetObject(srcFile, srcAbsName);
    if (!srcObj) goto done;

    dstObj = DBGetObject(dstFile, dstAbsName);
    if (!dstObj) goto done;

    for (q = 0; q < srcObj->ncomponents; q++)
    {
        int r, dstr = -1;
        char const *curr_compname = srcObj->comp_names[q];

        for (r = 0; r < dstObj->ncomponents && dstr==-1; r++)
            if (!strcmp(curr_compname, dstObj->comp_names[r]))
                dstr = r;

        if (dstr == -1) goto done; /* no matching comp in dst */

        if (!strncmp(srcObj->pdb_names[q], "'<i>", 4))
            continue;
        else if (!strncmp(srcObj->pdb_names[q], "'<f>", 4))
            continue;
        else if (!strncmp(srcObj->pdb_names[q], "'<d>", 4))
            continue;
        else /* possible sub-object */
        {
            int srcLen, dstLen, can_overwrite;
            DBObjectType srcSubObjType, dstSubObjType;
            char *srcSubObjName, *srcSubObjDirName, *srcSubObjAbsName;
            char *dstSubObjName, *dstSubObjDirName, *dstSubObjAbsName;

            if (!strncmp(srcObj->pdb_names[q], "'<s>", 4))
                srcSubObjName = db_strndup(srcObj->pdb_names[q]+4, strlen(srcObj->pdb_names[q])-5);
            else
                srcSubObjName = db_strndup(srcObj->pdb_names[q], strlen(srcObj->pdb_names[q]));
            srcSubObjDirName = db_dirname(srcAbsName);
            srcSubObjAbsName = db_join_path(srcSubObjDirName, srcSubObjName);

            if (!strncmp(dstObj->pdb_names[dstr], "'<s>", 4))
                dstSubObjName = db_strndup(dstObj->pdb_names[dstr]+4, strlen(dstObj->pdb_names[dstr])-5);
            else
                dstSubObjName = db_strndup(dstObj->pdb_names[dstr], strlen(dstObj->pdb_names[dstr]));
            dstSubObjDirName = db_dirname(dstAbsName);
            dstSubObjAbsName = db_join_path(dstSubObjDirName, dstSubObjName);

            /* To make exiting and cleanup logic here a tad simpler, we do all
               the work we *might* need to with the strings and then free them. */
#ifndef _WIN32
#warning SHOULDNT WE USE FILE LENGTH FUNCTIONS HERE
#endif
            srcSubObjType = DBInqVarType(srcFile, srcSubObjAbsName);
            srcLen = DBGetVarByteLength(srcFile, srcSubObjAbsName);
            dstSubObjType = DBInqVarType(dstFile, dstSubObjAbsName);
            dstLen = DBGetVarByteLength(dstFile, dstSubObjAbsName);

            if (dstSubObjType == DB_VARIABLE)
                can_overwrite = 1;
            else if (dstSubObjType == DB_DIR)
                can_overwrite = 0;
#ifndef _WIN32
#warning DISALLOW SIMLINKS FOR NOW
#endif
            else if (dstSubObjType == DB_SYMLINK)
                can_overwrite = 0;
            else
                can_overwrite = db_can_overwrite_dstobj_with_srcobj(
                    srcFile, srcSubObjAbsName, dstFile, dstSubObjAbsName); /* recursive call */

            FREE(srcSubObjName);
            FREE(srcSubObjDirName);
            FREE(srcSubObjAbsName);
            FREE(dstSubObjName);
            FREE(dstSubObjDirName);
            FREE(dstSubObjAbsName);

            /* decide if its ok to proceed or not */
            if (srcSubObjType == DB_VARIABLE && dstSubObjType == DB_VARIABLE)
            {
                if (srcLen > dstLen) goto done; /* simple src var doesn't fit in dst */
            }
            else if (srcSubObjType == dstSubObjType)
            {
                if (!can_overwrite) goto done; /* src object doesn't fit in dst */
            }
            else
            {
                goto done; /* src & dst disagree on subobject's type */
            }
        }
    }

    /* indicate it will work */
    retval = 1;

done:

    if (srcObj) DBFreeObject(srcObj);
    if (dstObj) DBFreeObject(dstObj);

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:   db_copy_single_object_abspath
 *
 * Purpose: Copy a single source *object* to a destination object or
 * directory using the generic object routines. The source cannot be a
 * directory. If destination does not exist, ensure its parent is an
 * existing directory. If destination is a pre-existing object, overwrite
 * that object as per options and as long as it fits. If destination is a
 * pre-existing directory, copy the source object *into* that directory.
 *
 * Returns: 1 If the copy succeeds.
 *          0 Otherwise.
 *
 * Programmer:  Mark C. Miller, Wed Apr 18 09:23:55  PDT 2018
 *
 *-------------------------------------------------------------------------*/
static int
db_copy_single_object_abspath(char const *opts,
    DBfile *srcFile, char const *srcObjAbsName, DBObjectType srcType,
    DBfile *dstFile, char const *dstObjAbsName, DBObjectType dstType)
{
    int q;
    char *_dstObjAbsName;
    DBobject *dstObj, *srcObj;

#ifndef _WIN32
#warning CHECK ARGS HERE. ALLOW FOR NULL dstFile AND dstObjAbsName
#warning WHY CHECK dstType HERE
#endif

    /* Query type information if not already known */
    if (srcType == DB_INVALID_OBJECT)
        srcType = DBInqVarType(srcFile, srcObjAbsName);
    if (dstType == DB_INVALID_OBJECT)
        dstType = DBInqVarType(dstFile, dstObjAbsName);

    /* Source object cannot be a directory */
    if (srcType == DB_INVALID_OBJECT || srcType == DB_DIR)
        return 0;

    /* If destination is a directory, dstObjAbsName needs to be adjusted
       to append last component of srcObjAbsName */
    if (dstType == DB_DIR)
    {
        char *bname = db_basename(srcObjAbsName);
        _dstObjAbsName = db_join_path(dstObjAbsName, bname);
        FREE(bname);
    }
    /* If destination is a pre-existing non-directory object, this implies
       it will be overwritten by source. But, in that case, dstFile needs
       to allow overwrites and the source needs to be small enough to fit
       into the space the destination object occupies. */
    else if (dstType != DB_INVALID_OBJECT)
    {
        int srcSize, dstSize;
#ifndef _WIN32
#warning USE FILE-BASED FUNCTION WHEN AVAILABLE
#endif
        if (!DBGetAllowOverwrites())
        {
            db_perror("overwrite of pre-existing dst prevented due "
                "to DBSetAllowOverwrites(0)", E_BADARGS, dstObjAbsName);
            return 0;
        }
        if (!db_can_overwrite_dstobj_with_srcobj(
             srcFile, srcObjAbsName, dstFile, dstObjAbsName))
        {
            db_perror("overwrite of pre-existing dst prevented due "
                "to insufficient space for src object", E_BADARGS, dstObjAbsName);
            return 0;
        }
        _dstObjAbsName = STRDUP(dstObjAbsName);
    }
    else
    {
        _dstObjAbsName = STRDUP(dstObjAbsName);
    }

    /* Access the source object using generic interface */
    srcObj = DBGetObject(srcFile, srcObjAbsName);
    if (!srcObj)
    {
        db_perror("Unable to access source object for copy operation",
            E_NOTFOUND, srcObjAbsName);
        return 0;
    }

    /* Handle funniness with quadmesh type strings */
    if (srcType == DB_QUADMESH)
    {
        if (strstr(srcObj->type, "rect"))
            srcType = DB_QUAD_RECT;
        else if (strstr(srcObj->type, "curv"))
            srcType = DB_QUAD_CURV;
    }

    /* Ok, lets do what we came here to do...create the dst object and
       start populating it */
    dstObj = DBMakeObject(_dstObjAbsName, srcType, srcObj->ncomponents);
    for (q = 0; q < srcObj->ncomponents; q++)
    {
        int isser = 0, nser = 0;
        char *sernm = 0, *sertstr = 0;
        void *serd = 0;
        CheckForComponentSeries(srcObj, q, &isser, &nser, &sernm, &sertstr, &serd);
        if (isser)
        {
            long tmpn = (long) nser;
            DBWriteComponent(dstFile, dstObj, sernm,
                dstObj->name, sertstr, serd, 1, &tmpn);
            free(sernm);
            free(sertstr);
            free(serd);
            q += (nser-1);
        }
        else if (!strncmp(srcObj->pdb_names[q], "'<i>", 4))
            DBAddIntComponent(dstObj, srcObj->comp_names[q],
                (int) strtol(srcObj->pdb_names[q]+4, NULL, 0));
        else if (!strncmp(srcObj->pdb_names[q], "'<f>", 4))
            DBAddFltComponent(dstObj, srcObj->comp_names[q],
                (float) strtod(srcObj->pdb_names[q]+4, NULL));
        else if (!strncmp(srcObj->pdb_names[q], "'<d>", 4))
            DBAddDblComponent(dstObj, srcObj->comp_names[q],
                strtod(srcObj->pdb_names[q]+4, NULL));
        else
        {
            DBObjectType subObjType;
            char *subObjName;
            char *srcObjDirName, *srcSubObjAbsName;
             
            if (!strncmp(srcObj->pdb_names[q], "'<s>", 4))
                subObjName = db_strndup(srcObj->pdb_names[q]+4, strlen(srcObj->pdb_names[q])-5);
            else
                subObjName = db_strndup(srcObj->pdb_names[q], strlen(srcObj->pdb_names[q]));

            srcObjDirName = db_dirname(srcObjAbsName);
            srcSubObjAbsName = db_join_path(srcObjDirName, subObjName);

            subObjType = DBInqVarType(srcFile, srcSubObjAbsName);
            if (subObjType == DB_VARIABLE) /* raw data copy */
            {
                int j, dims[32];
                long ldims[32];
                int idtype = DBGetVarType(srcFile, srcSubObjAbsName);
                int ndims = DBGetVarDims(srcFile, srcSubObjAbsName, 32, dims);
                void *data = DBGetVar(srcFile, srcSubObjAbsName);
                char *dtype = db_GetDatatypeString(idtype);
                for (j = 0; j < ndims; ldims[j] = (long) dims[j], j++);
                DBWriteComponent(dstFile, dstObj, srcObj->comp_names[q],
                    dstObj->name, dtype, data, ndims, ldims);
                FREE(dtype);
                FREE(data);
            }
            else if (((srcType == DB_UCDMESH) && /* possible recurse on sub-object */
                      (subObjType == DB_FACELIST || subObjType == DB_EDGELIST ||
                       subObjType == DB_ZONELIST || subObjType == DB_PHZONELIST)) ||
                     (srcType == DB_CSGMESH && subObjType == DB_CSGZONELIST)) 
            {
                char *dstObjDirName = db_dirname(dstObjAbsName);
                char *dstSubObjAbsName = db_join_path(dstObjDirName, subObjName);

                db_copy_single_object_abspath(opts, /* recursive call */
                   srcFile, srcSubObjAbsName, subObjType,
                   dstFile, dstSubObjAbsName, DB_INVALID_OBJECT);

                DBAddStrComponent(dstObj, srcObj->comp_names[q], subObjName);

                FREE(dstObjDirName);
                FREE(dstSubObjAbsName);
            }
            free(subObjName);
            free(srcObjDirName);
            free(srcSubObjAbsName);
        }
    }

    DBWriteObject(dstFile, dstObj, SILO_Globals.allowOverwrites);
    DBFreeObject(srcObj);
    DBFreeObject(dstObj);
    FREE(_dstObjAbsName);

    return 1;
}

/*-------------------------------------------------------------------------
 * Function:    DBCp
 *
 * Purpose:     The Silo equivalent of a unix `cp` command obeying all unix
 *              semantics and applicable flags. In particular...
 *
 *    Single src object to single dst object
 *        DBCp(char const *opts, DBfile *srcFile, DBfile *dstFile,
 *            char const *srcPATH, char const *dstPATH, DB_EOA);
 *
 *    Multiple src objects to single dst (dir) object
 *        DBCp(char const *opts, DBfile *srcFile, DBfile *dstFile,
 *            char const *srcPATH1, char const *srcPATH2, ...,
 *            char const *dstDIR, DB_EOA);
 *
 *    Multiple src objects to multiple dst objects
 *        DBCp("-2...", DBfile *srcFile, DBfile *dstFile,
 *            char const *srcPATH1, char const *dstPATH1,
 *            char const *srcPATH2, char const *dstPATH2,
 *            char const *srcPATH3, char const *dstPATH3, ..., DB_EOA);
 *
 *    srcFile and dstFile may be the same Silo file. srcFile cannot be null.
 *    dstFile may be null in which case it is assumed same as srcFile.
 *    The argument list *must* be terminated by the DB_EOA sentinel. Just as
 *    for unix `cp`, the options and their meanings are...
 *
 *      -R/-r recurse on any directory objects
 *      -L/-P dereference links / never dereference links
 *      -d    preserve links
 *      -s/-l don't actually copy, just sym/hard link
 *            (only possible when srcFile==dstFile)
 *
 *    Other rules:
 *       * If any src is a dir, then it is an error without -R/-r.
 *       * If cooresponding dst exists and is a dir, src is copied
 *         into (e.g. becomes a child of) dst.
 *       * If cooresponding dst exists and is not a dir (e.g. is just a
 *         normal Silo object), then it is an error if src is not also the 
 *         same kind of Silo object. The copy overwrites (destructive) dst.
 *         However, if the file space dst object occupies is smaller than
 *         that needed to copy src, behavior is indeterminate but most 
 *         likely will result the dst file (not just the dst object) being
 *         corrupted.
 *
 *    The following options are specific to Silo's DBCp method and are
 *    mutually exclusive...
 *
 *      -2 treat varargs list of args as src/dst path pairs and where any
 *         null dst is inferred to have same path as associated src except
           that relative paths are interpreted relative to dst files cwg.
 *      -1 like -2 except caller passes only src paths. All dst paths are
 *         inferred to be same as associated src path. The dst file's cwg
 *         will then determine how any relative src paths are interpreted.
 *      -3 only 3 args follow the dstFile arg...
 *         int N                number of objects in the following lists
 *         DBCAS_t srcPathNames list of N source path names
 *         DBCAS_t dstPathNames list of N destination path names
 *      -4 Like -3, except 3rd arg is treated as a single dest. dir name
 *         int N                number of paths in srcPathNames
 *         DBCAS_t srcPathNames list of N source path names
 *         char const *dstDIR   pre-existing destination dir path name.
 *      -5 Internal use only...like -4 except used only internally when
 *         DBCp recursively calls itself.
 *
 *      If none of the preceding numeric arguments are specified, then
 *      the varags list of args is treated as (default) where the last
 *      is a pre-existing destination directory and all the others are
 *      the paths of source objects to be copied into that directory.
 *
 *      Relative path names are interpreted relative to the current working
 *      directory of the associated (src or dst) file when DBCp is invoked.
 *
 *      In all the different ways this function can be invoked, there are
 *      really just two fundamentally different interpretations of the list(s)
 *      of names. Either each source path is paired also with a destination
 *      path or all source paths go into a single destination path which, just
 *      as for linux cp, must then also be a directory already present in the
 *      destination.
 *
 * Return:      Success:        0
 *              Failure:       -1
 *
 * Programmer:  Mark C. Miller, Wed Apr 18 09:23:55  PDT 2018
 *
 *-------------------------------------------------------------------------*/
PUBLIC int
DBCp(char const *opts, DBfile *srcFile, DBfile *dstFile, ...)
{
#ifndef _WIN32
#warning WHAT ABOUT API MACROS. MAYBE NOT NEEDED SINCE NOT CALLING DOWN INTO DRIVERS
#warning WHAT ABOUT -f (force) OPTION
#endif
    char const *me = "DBCp";
    int i, j, pass, all_ok;
    int recurse_on_dirs = 0;
    int preserve_links = 0;
    int deref_links = 0;
    int never_deref_links = 0;
    int dont_copy_just_symlink = 0;
    int dont_copy_just_hardlink = 0;
    int src_dst_pairs = 0;
    int srcs_only = 0;
    int n_src_dst_triple = 0;
    int n_src_dir_triple = 0;
    int recursive_call = 0;
    char const *many_to_one_dir = 0;
    int N = 0;
    char const **srcPathNames = 0, **dstPathNames = 0;
    int srcPathNamesAlloc = 0, dstPathNamesAlloc = 0;
    char srcStartCwg[1024], dstStartCwg[1024];

#ifndef _WIN32
#warning WARN ABOUT CERTAIN OPTIONS NOT YET SUPPORTED
#endif
    /* process any options in the opts string */
    for (i = 0; opts && opts[i]; i++)
    {
        switch (opts[i])
        {
            case ' ':
            case '-': continue;
            case 'R':
            case 'r': recurse_on_dirs = 1; break;
            case 'd': preserve_links = 1; break;
            case 'L': deref_links = 1; break;
            case 'P': never_deref_links = 1; break;
            case 's': dont_copy_just_symlink = 1; break;
            case 'l': dont_copy_just_hardlink = 1; break;
            case '2': src_dst_pairs = 1; break;
            case '1': srcs_only = 1; break;
            case '3': n_src_dst_triple = 1; break;
            case '4': n_src_dir_triple = 1; break;
            case '5': n_src_dir_triple = 1; recursive_call = 1; break;
            default: return db_perror(&opts[i], E_BADARGS, me);
        }
    }

    /* Smoke checks */
    if (!dstFile)
        dstFile = srcFile;

    if (!srcFile)
        return db_perror("srcFile cannot be null", E_BADARGS, me);

    if ((dont_copy_just_symlink || dont_copy_just_hardlink) && srcFile != dstFile)
        return db_perror("srcFile and dstFile must be same for -s or -l", E_BADARGS, me);


    if (n_src_dst_triple || n_src_dir_triple) // -3 or -4
    {
        va_list ap;
        va_start(ap, dstFile);
        N = va_arg(ap, int);
        srcPathNames = va_arg(ap, char const **);
        if (n_src_dst_triple)
            dstPathNames = va_arg(ap, char const **);
        else
            many_to_one_dir = va_arg(ap, char const *);
        va_end(ap);
    }
    else
    {
        /* Process varargs in two passes. Pass 1, count them. Pass 2
           allocate space and capture them. */
        int pass;
        for (pass = 0; pass < 2; pass++)
        {
            int n = 0;
            char const *arg;
            va_list ap;
            va_start(ap, dstFile);
            if (pass == 1)
            {
                srcPathNames = (char const **) malloc(N * sizeof(char const *));
                srcPathNamesAlloc = 1;
                if (src_dst_pairs)
                {
                    dstPathNames = (char const **) malloc(N * sizeof(char const *));
                    dstPathNamesAlloc = 1;
                }
            }
            while ((arg = va_arg(ap, char const *)) != DB_EOA)
            {
                if (pass == 1)
                {
                    if (src_dst_pairs)
                    {
                        if (n%2) dstPathNames[n/2] = arg;
                        else     srcPathNames[n/2] = arg;
                    }
                    else
                    {
                        srcPathNames[n] = arg;
                    }
                }
                n++;
            }
            va_end(ap);
            N = src_dst_pairs?n/2:n;
        }
    }

    if (n_src_dir_triple && N == 0)
    {
        if (srcPathNamesAlloc) FREE(srcPathNames);
        if (dstPathNamesAlloc) FREE(dstPathNames);
        return 0;
    }
    if (!n_src_dir_triple && N < 2)
    {
        if (srcPathNamesAlloc) FREE(srcPathNames);
        if (dstPathNamesAlloc) FREE(dstPathNames);
        return db_perror("src or dst unspecified", E_BADARGS, me);
    }
    if (src_dst_pairs && N%2)
    {
        if (srcPathNamesAlloc) FREE(srcPathNames);
        if (dstPathNamesAlloc) FREE(dstPathNames);
        return db_perror("non-even arg count for -2 option", E_BADARGS, me);
    }

    /* target dir is at end of list of sources */
    if (!many_to_one_dir && !src_dst_pairs && !n_src_dst_triple && !n_src_dir_triple)
    {
        many_to_one_dir = srcPathNames[N-1];
        N--;
    }

    if (many_to_one_dir)
    {
        int q;
        dstPathNames = (char const **) malloc(N * sizeof(char const *));
        dstPathNamesAlloc = 1;
        for (q = 0; q < N; q++)
            dstPathNames[q] = many_to_one_dir; /* all point to same char* */
    }

    DBGetDir(srcFile, srcStartCwg);
    DBGetDir(dstFile, dstStartCwg);

    for (i = 0; i < N; i++)
    {
        DBObjectType srcType, dstType;
        char *srcObjAbsName, *dstObjAbsName;
        char savcwg[1024], srccwg[1024], dstcwg[1024];
        char opts2[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        char **dirItems=0, **otherItems;
        int  dirItemCount=0, otherItemCount;

        if (i == 0)
        {
            DBGetDir(srcFile, srccwg);
            DBGetDir(dstFile, dstcwg);
        }

        srcObjAbsName = db_join_path(srccwg, srcPathNames[i]);
        srcType = DBInqVarType(srcFile, srcObjAbsName);

        dstObjAbsName = db_join_path(dstcwg, dstPathNames[i]?dstPathNames[i]:srcPathNames[i]);
        dstType = DBInqVarType(dstFile, dstObjAbsName);

        if (srcType == DB_INVALID_OBJECT)
        {
            db_perror(DBSPrintf("\"%s\" invalid object", srcObjAbsName), E_BADARGS, me);
            goto endLoop;
        }

        if (srcType != DB_DIR)
        {
            if (!db_copy_single_object_abspath(opts, 
                    srcFile, srcObjAbsName, srcType,
                    dstFile, dstObjAbsName, dstType))
            {
                db_perror("Object copy failed", E_CALLFAIL, me);
                goto endLoop;
            }
            goto endLoop;
        }

        /* If we get this far into the loop body here, then src is a dir */
        if (!recurse_on_dirs)
        {
            db_perror(DBSPrintf("Cannot copy dir \"%s\" without -r flag",
                srcObjAbsName), E_BADARGS, me);
            goto endLoop;
        }

        if (dstType != DB_INVALID_OBJECT && dstType != DB_DIR)
        {
            db_perror(DBSPrintf("Cannot copy dir \"%s\" onto pre-existing non-dir \"%s\"",
                srcObjAbsName, dstObjAbsName), E_BADARGS, me);
            goto endLoop;
        }

#if 0
        /* get the contents of this dir in two lists; all the dirs, everything else */
        DBLs(srcFile, DBSPrintf("-d %s", srcObjAbsName), 0, &dirItemCount); /* just count it first */
        dirItems = ALLOC_N(char*, ++dirItemCount);
        DBLs(srcFile, DBSPrintf("-d %s", srcObjAbsName), dirItems, &dirItemCount);

        DBLs(srcFile, DBSPrintf("-a -d %s", srcObjAbsName), 0, &otherItemCount); /* just count it first */
        otherItems = ALLOC_N(char*, ++otherItemCount);
        DBLs(srcFile, DBSPrintf("-a -d %s", srcObjAbsName), otherItems, &otherItemCount);
#endif
        /* get the contents of this dir */
        DBSetDir(srcFile, srcObjAbsName);
        DBLs(srcFile, "-a -x", 0, &dirItemCount); /* just count it first */
        dirItems = ALLOC_N(char*, dirItemCount);
        DBLs(srcFile, "-a -x", dirItems, &dirItemCount);

        if (dstType == DB_INVALID_OBJECT)
        {
            DBMkDir(dstFile, dstObjAbsName);
            DBSetDir(dstFile, dstObjAbsName);
        }
        else if (dstType == DB_DIR)
        {
            char *srcDirBaseName = db_basename(srcObjAbsName);
            DBMkDir(dstFile, srcDirBaseName);
            DBSetDir(dstFile, srcDirBaseName);
            free(srcDirBaseName);
        }

        if (n_src_dir_triple)
            DBCp(opts, srcFile, dstFile, dirItemCount, dirItems, ".");
        else
            DBCp(DBSPrintf("%s -4", opts), srcFile, dstFile, dirItemCount, dirItems, ".");

        DBSetDir(srcFile, "..");
        DBSetDir(dstFile, "..");

endLoop:
        if (dirItems)
        {
            for (int k = 0; k < dirItemCount; k++)
                FREE(dirItems[k]);
            FREE(dirItems);
        }
        FREE(srcObjAbsName);
        FREE(dstObjAbsName);
    }

    if (srcPathNamesAlloc) FREE(srcPathNames);
    if (dstPathNamesAlloc) FREE(dstPathNames);

    DBSetDir(srcFile, srcStartCwg);
    DBSetDir(dstFile, dstStartCwg);

    return 0;
}

#if 1
PUBLIC int
DBCpListedObjects(int nobjs,
    DBfile *dbfile, char const * const *srcObjs,
    DBfile *dstFile, char const * const *dstObjs)
{
    int retval;

    API_BEGIN2("DBCpListedObjects", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (!dstFile)
            API_ERROR(NULL, E_NOFILE);
        if (db_isregistered_file(dstFile,0)==-1)
            API_ERROR(NULL, E_NOTREG);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR(NULL, E_GRABBED); 
        if (nobjs > 0)
        {
            if (!srcObjs)
                API_ERROR("source object names list", E_BADARGS);
            if (dstObjs)
            {
                int i;
                for (i = 0; i < nobjs; i++)
                {
                    if (dstObjs[i] == 0 || dstObjs[i][0] == '\0')
                        continue;
                    if (db_VariableNameValid(dstObjs[i]) == 0)
                        API_ERROR(dstObjs[i], E_INVALIDNAME);
                }
            }
        }
        if (!dbfile->pub.cpnobjs)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        retval = (dbfile->pub.cpnobjs) (nobjs, dbfile, srcObjs, dstFile, dstObjs);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}
#else
#warning REMOVE ABOVE IF STATEMENT
PUBLIC int
DBCpListedObjects(int nobjs,
    DBfile *dbfile, char const * const *srcObjs,
    DBfile *dstFile, char const * const *dstObjs)
{
}
#endif

/*-------------------------------------------------------------------------
 * Function:    DBChangeObject
 *
 * Purpose:     Overwrites an object with a new object.  This is usually
 *              the same function as called by DBWriteObject but with
 *              OVER_WRITE as the flag.  However, we keep it as a separate
 *              callback so existing drivers that don't support overwriting
 *              don't need to be changed and so that the silo API doesn't
 *              change by changing the meaning of the `freemem' argument
 *              to DBWriteObject.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Mar  7 1997
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Completely reformatted the code so a human can read it.  Made the error
 *    messages a little better.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBChangeObject (DBfile *dbfile, DBobject const *obj)
{
    int             retval;

    API_BEGIN2("DBChangeObject", int, -1, api_dummy)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBChangeObject", E_GRABBED) ; 
        if (!obj)
            API_ERROR("object pointer", E_BADARGS);
        if (!dbfile->pub.c_obj)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        retval = (dbfile->pub.c_obj) (dbfile, obj, OVER_WRITE);
        API_RETURN(retval);
    }
    API_END_NOPOP;                     /* BEWARE: If API_RETURN is removed 
                                        * use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBWriteObject
 *
 * Purpose:     Write an object into the data file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Mon Nov  7 10:45:14 PST 1994
 *
 * Modifications:
 *    Robb Matzke, Mon Nov 21 21:37:54 EST 1994
 *    Added error mechanism.
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Robb Matzke, 7 Mar 1997
 *    The freemem value passed to the driver is either FREE_MEM or zero
 *    so that drivers that overload this function with DBChangeObject
 *    are guaranteed to be able to tell the difference.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBWriteObject(DBfile *dbfile, DBobject const *obj, int freemem)
{
    int retval;

    API_BEGIN2("DBWriteObject", int, -1, api_dummy)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBWriteObject", E_GRABBED) ; 
        if (!obj)
            API_ERROR("object pointer", E_BADARGS);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, obj->name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (!dbfile->pub.w_obj)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.w_obj) (dbfile, obj, freemem?FREE_MEM:0);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC void *
DBGetPartialObject(DBfile *dbfile, char const *name, int nvals, int ndims,
    int index_mode, void const *indices, DBoptlist *options)
{
#ifndef _WIN32
#warning FIX THIS METHOD
#endif



    /* call directly down into driver */

    /* get type */
    DBObjectType otype = DBInqVarType(dbfile, name);

    /* set mask and read object header */

    /* get list of datasets */

    /* If dense, do partial I/O on the datasets */
    /* Else... do object specific partial I/O */
    /* Handle mixed values on variables */

    return 0;
}

PUBLIC DBmaterial *
DBGetPartialMaterial(DBfile *dbfile, char const *name, int mode, int nvals,
    int ndims, void const *indices, DBoptlist *options)
{
    return (DBmaterial *) DBGetPartialObject(dbfile, name, mode, nvals, ndims, indices, options);
}

/*-------------------------------------------------------------------------
 * Function:    DBGetObject
 *
 * Purpose:     Reads an object from a file.
 *
 * Return:      Success:        Ptr to the new object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  2 1996
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *-------------------------------------------------------------------------*/
PUBLIC DBobject *
DBGetObject (DBfile *dbfile, char const *objname)
{
    DBobject       *retval = NULL;

    API_BEGIN2("DBGetObject", DBobject *, NULL, api_dummy)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetObject", E_GRABBED) ; 
        if (!objname)
            API_ERROR("object name", E_BADARGS);
        if (!dbfile->pub.g_obj)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        retval = (dbfile->pub.g_obj) (dbfile, (char const *)objname);
        db_AdjustSpeciallyHandledStandardObjectComponentValues(retval);
        API_RETURN(retval);
    }
    API_END_NOPOP;                     /* BEWARE:  If API_RETURN above is
                                        * removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBWriteComponent
 *
 * Purpose:     Add a variable component to the given object structure, AND
 *              write out the associated data.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Mon Nov  7 10:47:29 PST 1994
 *
 * Modifications:
 *    Robb Matzke, Mon Nov 21 21:39:06 EST 1994
 *    Added error mechanism.
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Mar 31 17:16:24 PST 1998
 *    I added a check for zero-length data arrays.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Wed Jul 23 00:15:15 PDT 2008
 *    Changed to API_BEGIN2 to help detect attempted ops on closed files.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBWriteComponent(DBfile *dbfile, DBobject *obj, char const *comp_name,
                 char const *prefix, char const *datatype, void const *var, int nd,
                 long const *count)
{
    int retval;
    int nvals, i;

    API_BEGIN2("DBWriteComponent", int, -1, api_dummy) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBWriteComponent", E_GRABBED) ;
        if (!obj)
            API_ERROR("object pointer", E_BADARGS);
        if (DBGetAllowEmptyObjectsFile(dbfile))
        {
            if (nd<=0) API_RETURN(0);
            if (!count) API_RETURN(0);
            if (!var) API_RETURN(0);
            for(nvals=1,i=0;i<nd;i++)
                nvals *= count[i];
            if (nvals<=0) API_RETURN(0);
        }
        if (!comp_name || !*comp_name)
            API_ERROR("component name", E_BADARGS);
        if (db_VariableNameValid((char *)comp_name) == 0)
            API_ERROR("component name", E_INVALIDNAME);
#ifndef _MSC_VER
#warning FIXME
#endif
#if 0
        /* We don't know what name to pass to DBInqVarExists here because it
           is the driver that knows how to construct component names */
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, obj->name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
#endif
        if (!prefix || !*prefix)
            API_ERROR("prefix", E_BADARGS);
        if (db_VariableNameValid((char *)prefix) == 0)
            API_ERROR("prefix", E_INVALIDNAME);
        if (!datatype || !*datatype)
            API_ERROR("data type", E_BADARGS);
        if (!var)
            API_ERROR("var pointer", E_BADARGS);
        if (nd <= 0)
            API_ERROR("nd", E_BADARGS);
        if (!count && nd)
            API_ERROR("count", E_BADARGS);
        for(nvals=1,i=0;i<nd;i++)
        {
            nvals *= count[i];
        }
        if (nvals == 0) {
            API_ERROR("Zero-length write attempted", E_BADARGS);
        }
        if (!dbfile->pub.w_comp)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        /* Note that the work to add the variable component to the object is
           handled down in the drivers due to the fact that the drivers may
           use different rules to construct the object component names */

        retval = (dbfile->pub.w_comp) (dbfile, obj, comp_name, prefix,
                                       datatype, var, nd, count);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBWrite
 *
 * Purpose:     Writes a single variable into the database.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Wed Nov  9 13:19:49 EST 1994
 *
 * Modifications:
 *    Robb Matzke, Mon Nov 21 21:42:27 EST 1994
 *    Added error mecanism.
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Mar 31 17:17:44 PST 1998
 *    I added a check for zero-length arrays.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Mon Jan 11 17:42:51 PST 2010
 *    Allow special variable names in the magic /.silo dir for HDF5 files.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBWrite(DBfile *dbfile, char const *vname, void const *var, int const *dims,
    int ndims, int datatype)
{
    int retval;
    int nvals, i;

    API_BEGIN2("DBWrite", int, -1, vname) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBWrite", E_GRABBED) ; 
        if (!vname || !*vname)
            API_ERROR("variable name", E_BADARGS);
        if (strncmp("/.silo/#", vname, 8) != 0 &&
            db_VariableNameValid(vname) == 0)
            API_ERROR("variable name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, vname))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (ndims < 0)
            API_ERROR("ndims", E_BADARGS);
        if (ndims == 0 && !DBGetAllowEmptyObjectsFile(dbfile))
            API_ERROR("ndims==0", E_EMPTYOBJECT);
        if (!dims && ndims)
            API_ERROR("dims", E_BADARGS);
        for(nvals=ndims>0?1:0,i=0;i<ndims;i++)
            nvals *= dims[i];
        if (nvals == 0 && !DBGetAllowEmptyObjectsFile(dbfile))
            API_ERROR("Zero length write attempted", E_EMPTYOBJECT);
        if (db_FullyDeprecatedConvention(vname))
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        if (!dbfile->pub.write)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.write) (dbfile, vname, var, dims,
                                      ndims, datatype);

        /* diddle with retval if its an empty case */
        if ((ndims == 0 || nvals == 0) && retval == 1) retval = 0;

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBWriteSlice
 *
 * Purpose:     Similar to DBWrite except only part of the data is
 *              written.  If VNAME doesn't exist, space is reserved for
 *              the entire variable based on DIMS; otherwise we check
 *              that DIMS has the same value as originally.  Then we
 *              write the specified slice to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.com
 *              May  9, 1996
 *
 * Modifications:
 *    Sean Ahern, Tue Mar 31 17:19:38 PST 1998
 *    I added a check for zero-length writes.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBWriteSlice (DBfile *dbfile, const char *vname, void const *values, int dtype,
              int const *offset, int const *length, int const *stride, int const *dims,
              int ndims)
{
    int retval;
    int nvals,i;

    API_BEGIN2("DBWriteSlice", int, -1, vname)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBWriteSlice", E_GRABBED) ; 
        if (!vname || !*vname)
            API_ERROR("variable name", E_BADARGS);
        if (db_VariableNameValid(vname) == 0)
            API_ERROR("variable name", E_INVALIDNAME);
        if (!values)
            API_ERROR("values", E_BADARGS);
        if (!offset)
            API_ERROR("offset", E_BADARGS);
        if (!length)
            API_ERROR("length", E_BADARGS);
        if (!stride)
            API_ERROR("stride", E_BADARGS);
        if (!dims)
            API_ERROR("dims", E_BADARGS);
        if (ndims <= 0 || ndims > 3)
            API_ERROR("ndims", E_BADARGS);
        for(nvals=1,i=0;i<ndims;i++)
        {
            nvals *= length[i];
        }
        if (nvals == 0)
            API_ERROR("Zero-length write attempted", E_BADARGS);
        if (!dbfile->pub.writeslice)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.writeslice) (dbfile, vname, values,
                                           dtype, offset, length, stride,
                                           dims, ndims);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP;     /* BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetCompoundarray
 *
 * Purpose:     Read a compound array object from the file.
 *
 * Return:      Success:        pointer to fresh compound array obj.
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Mon Nov  7 10:50:29 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
PUBLIC DBcompoundarray *
DBGetCompoundarray(DBfile *dbfile, char const *name)
{
    DBcompoundarray *retval = NULL;

    API_BEGIN2("DBGetCompoundarray", DBcompoundarray *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetCompoundarray", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("array name", E_BADARGS);
        if (NULL == dbfile->pub.g_ca)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_ca) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetCurve
 *
 * Purpose:     Read a curve object from the file.
 *
 * Return:      Success:        pointer to fresh curve obj
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.com
 *              May 16, 1996
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
PUBLIC DBcurve *
DBGetCurve (DBfile *dbfile, char const *name)
{
    DBcurve *retval = NULL;

    API_BEGIN2("DBGetCurve", DBcurve *, NULL, name)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetCurve", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("curve name", E_BADARGS);
        if (NULL == dbfile->pub.g_cu)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_cu) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP;  /* BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetDefvars
 *
 * Purpose:     Read a defvars object from the file.
 *
 * Return:      Success:        pointer to fresh defvars obj
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller 
 *              August 8, 2005 
 *
 *-------------------------------------------------------------------------*/
PUBLIC DBdefvars *
DBGetDefvars (DBfile *dbfile, const char *name)
{
    DBdefvars *retval = NULL;

    API_BEGIN2("DBGetDefvars", DBdefvars *, NULL, name)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetDefvars", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("defvars name", E_BADARGS);
        if (NULL == dbfile->pub.g_defv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_defv) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP;  /* BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetMaterial
 *
 * Purpose:     Allocates a DBmaterial data structure, reads material data
 *              from the database, and returns a pointer to that struct.
 *
 * Return:      Success:        pointer to a new DBmaterial structure
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 09:32:17 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
PUBLIC DBmaterial *
DBGetMaterial(DBfile *dbfile, const char *name)
{
    DBmaterial *retval = NULL;

    API_BEGIN2("DBGetMaterial", DBmaterial *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetMaterial", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("material name", E_BADARGS);
        if (!dbfile->pub.g_ma)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_ma) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 *  Routine                                             DBGetMatspecies
 *
 *  Purpose
 *
 *      Read a matspecies-data structure from the given database.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Parameters
 *
 *      DBGetMatspecies {Out}    {Pointer to matspecies structure}
 *      dbfile           {In}    {Pointer to current file}
 *      name             {In}    {Name of matspecies-data to read}
 *
 *  Notes
 *
 *  Modifications
 *    Al Leibee, Tue Jul 26 08:44:01 PDT 1994
 *    Replaced composition by species.
 *
 *    Robb Matzke, Tue Nov 29 13:21:27 PST 1994
 *    Modified for device independence.  Added error mechanism.
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *--------------------------------------------------------------------*/
PUBLIC DBmatspecies *
DBGetMatspecies(DBfile *dbfile, const char *name)
{
    DBmatspecies *retval = NULL;

    API_BEGIN2("DBGetMatspecies", DBmatspecies *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetMatspecies", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("material species name", E_BADARGS);
        if (!dbfile->pub.g_ms)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_ms) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 *  Routine                                        DBGenerateMBBlockName
 *
 *  Purpose: Use multiblock object block and file nameschemes and empty
 *  list information to generate a multiblock object block name for a 
 *  given block index. The result is returned in a static local variable
 *  and should be *USED IMMEDIATELY*.
 *
 *  This code was taken entirely from VisIt's Silo plugin.
 *
 *  Mark C. Miller (copied from Cyrus Harrison), Wed Jul  2 PDT 2025
 *
 *  Modifications
 *--------------------------------------------------------------------*/
PUBLIC char
*DBGenerateMBBlockName(
    int idx,                     /* block index for name to be generated */
    DBnamescheme const *fileNS,  /* file path namescheme */
    DBnamescheme const *blockNS, /* block path namescheme */
    int emptyCnt,                /* optional empty list size */
    int const *emptyLst)         /* optional list of empty block numbers */
{
    static char res[4096];
    int avail = (int) sizeof(res)-1;

    strcpy(res, "EMPTY");
    if (emptyLst)
    {
        int bot = 0;
        int top = emptyCnt - 1;
        int mid;
        while (bot <= top)
        {
            mid = (bot + top) >> 1;

            if (idx > emptyLst[mid])
                bot = mid + 1;
            else if (idx < emptyLst[mid])
                top = mid - 1;
            else
                return res;
        }
    }

    // namescheme case
    res[0] = '\0';
    if (fileNS != 0)
    {
        const char *file_res = DBGetName(fileNS,idx);
        if (file_res != 0)
        {
            strncat(res, file_res, avail);
            avail -= strlen(file_res);
            strncat(res, ":", avail);
            avail--;
        }
    }

    if (blockNS != 0)
    {
        const char *block_res = DBGetName(blockNS,idx);
        if (block_res != 0)
            strncat(res, block_res, avail);
    }

    return res;
}

/*----------------------------------------------------------------------
 *  Routine                                 _dbEvalMultiblockNameschemes
 *
 *  Purpose: Workhorse to iterate over relevant members of a multiblock
 *  object with nameschemes to generate an explicit list of names and,
 *  where needed, block types.
 *
 *  Mark C. Miller, Wed Jul  2 10:33:39 PDT 2025
 *
 *  Modifications
 *--------------------------------------------------------------------*/
PRIVATE void
_dbEvalMultiblockNameschemes(
    DBfile *dbfile,         /* Silo file the associated MB object was obtained from */
    int nblocks,            /* number of names to generate */
    int block_type,         /* homogeneous block type of all blocks */
    char const *file_ns,    /* The file path namescheme */
    char const *block_ns,   /* The block path (within a file) namescheme */
    int empty_cnt,          /* Size of empty list */
    int const * empty_list, /* List of empty block indices */
    int **block_types,      /* Returned block types array */
    char ***block_names     /* Returned block names array */
)
{
    DBnamescheme *fileNS = DBMakeNamescheme(file_ns, 0, dbfile, 0);
    DBnamescheme *blockNS = DBMakeNamescheme(block_ns, 0, dbfile, 0);

    *block_names = (char **) malloc(nblocks * sizeof(char*));
    if (block_types)
        *block_types = (int *) malloc(nblocks * sizeof(int));

    for (int i = 0; i < nblocks; i++)
    {
        (*block_names)[i] = strdup(DBGenerateMBBlockName(i, fileNS, blockNS, empty_cnt, empty_list));
        if (block_types)
            (*block_types)[i] = block_type;
    }

    DBFreeNamescheme(fileNS);
    DBFreeNamescheme(blockNS);
}


/*----------------------------------------------------------------------
 *  Routine                                   DBEvalMultimeshNameschemes
 *
 *  Purpose: Convert a DBmultimesh object with nameschemes to an
 *  explicit list of names. The object is converted in place. It is
 *  harmless to call this method on an object that is not using
 *  nameschemes
 *
 *  Mark C. Miller, Wed Jul  2 10:33:39 PDT 2025
 *
 *  Modifications
 *--------------------------------------------------------------------*/
PUBLIC void
DBEvalMultimeshNameschemes(
    DBfile *dbfile,  /* The file the object was read from */
    DBmultimesh *mm) /* The object to convert */
{
    if (mm->meshnames) return;

    _dbEvalMultiblockNameschemes(dbfile, mm->nblocks, mm->block_type,
        mm->file_ns, mm->block_ns, mm->empty_cnt, mm->empty_list,
        &mm->meshtypes, &mm->meshnames);

    FREE(mm->file_ns);
    FREE(mm->block_ns);
}

/*----------------------------------------------------------------------
 *  Routine                                    DBEvalMultivarNameschemes
 *
 *  Purpose: Convert a DBmultivar object with nameschemes to an
 *  explicit list of names. The object is converted in place. It is
 *  harmless to call this method on an object that is not using
 *  nameschemes
 *
 *  Mark C. Miller, Wed Jul  2 10:33:39 PDT 2025
 *
 *  Modifications
 *--------------------------------------------------------------------*/
PUBLIC void
DBEvalMultivarNameschemes(
    DBfile *dbfile, /* The file the object was read from */
    DBmultivar *mv) /* The object to convert */
{
    if (mv->varnames) return;

    _dbEvalMultiblockNameschemes(dbfile, mv->nvars, mv->block_type,
        mv->file_ns, mv->block_ns, mv->empty_cnt, mv->empty_list,
        &mv->vartypes, &mv->varnames);

    FREE(mv->file_ns);
    FREE(mv->block_ns);
}

/*----------------------------------------------------------------------
 *  Routine                                    DBEvalMultimatNameschemes
 *
 *  Purpose: Convert a DBmultimat object with nameschemes to an
 *  explicit list of names.
 *
 *  Note: A DBmultimat has no member for the mesh or variable type of
 *  each block. So, those arguments to _dbEvalMultiblockNameschemes are
 *  NULL'd out. The object is converted in place. It is harmless to call
 *  this method on an object that is not using nameschemes.
 *
 *  Mark C. Miller, Wed Jul  2 10:33:39 PDT 2025
 *
 *  Modifications
 *--------------------------------------------------------------------*/
PUBLIC void
DBEvalMultimatNameschemes(
    DBfile *dbfile, /* The file the object was read from */
    DBmultimat *mm) /* The object to convert */
{
    if (mm->matnames) return;

    _dbEvalMultiblockNameschemes(dbfile, mm->nmats, 0 /*no mm->block_type */,
        mm->file_ns, mm->block_ns, mm->empty_cnt, mm->empty_list,
        0 /*no mm->mattypes */, &mm->matnames);

    FREE(mm->file_ns);
    FREE(mm->block_ns);
}

/*----------------------------------------------------------------------
 *  Routine                             DBEvalMultimatspeciesNameschemes
 *
 *  Purpose: Convert a DBmultimatspecies object with nameschemes to an
 *  explicit list of names.
 *
 *  Note: A DBmultimatspecies has no member for mesh or variable type of
 *  each block. So, those arguments to _dbEvalMultiblockNameschemes are
 *  NULL'd out. The object is converted in place. It is harmless to call
 *  this method on an object that is not using nameschemes.
 *
 *  Mark C. Miller, Wed Jul  2 10:33:39 PDT 2025
 *
 *  Modifications
 *--------------------------------------------------------------------*/
PUBLIC void
DBEvalMultimatspeciesNameschemes(
    DBfile *dbfile, /* The file the object was read from */
    DBmultimatspecies *ms) /* The object to convert */
{
    if (ms->specnames) return;

    _dbEvalMultiblockNameschemes(dbfile, ms->nspec, 0 /* no ms->block_type */,
        ms->file_ns, ms->block_ns, ms->empty_cnt, ms->empty_list,
        0 /* no ms->spectypes */, &ms->specnames);

    FREE(ms->file_ns);
    FREE(ms->block_ns);
}

/*-------------------------------------------------------------------------
 * Function:    DBGetMultimesh
 *
 * Purpose:     Allocates a DBmultimesh data structure, reads a multi-block
 *              mesh from the database, and returns a pointer to the
 *              new structure.
 *
 * Return:      Success:        pointer to the new DBmultimesh
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 09:35:38 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBmultimesh *
DBGetMultimesh(DBfile *dbfile, const char *name)
{
    DBmultimesh * retval = NULL;

    API_BEGIN2("DBGetMultimesh", DBmultimesh *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetMultimesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multimesh name", E_BADARGS);
        if (!dbfile->pub.g_mm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_mm) (dbfile, name);
        if (DBGetEvalNameschemesFile(dbfile))
          DBEvalMultimeshNameschemes(dbfile, retval);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetMultimeshadj
 *
 * Purpose:     Allocates a DBmultimeshdj data structure, reads a
 *              multi-block mesh adjacency object from the database, and
 *              returns a pointer to the new structure.
 *
 * Return:      Success:        pointer to the new DBmultimeshadj
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller 
 *              August 24, 2005 
 *
 *-------------------------------------------------------------------------*/
PUBLIC DBmultimeshadj *
DBGetMultimeshadj(DBfile *dbfile, const char *name, int nmesh,
   const int *block_map)
{
    DBmultimeshadj * retval = NULL;

    API_BEGIN2("DBGetMultimeshadj", DBmultimeshadj *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetMultimeshadj", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multimesh name", E_BADARGS);
        if (!dbfile->pub.g_mmadj)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_mmadj) (dbfile, name, nmesh,
                                        block_map);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}





/*-------------------------------------------------------------------------
 * Function:    DBGetMultivar
 *
 * Purpose:     Allocates a DBmultivar data structure, reads a multi-block
 *              variable from the database, and returns a pointer to the
 *              new structure.
 *
 * Return:      Success:        pointer to the new DBmultivar
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@cloud
 *              Tue Feb 21 11:27:46 EST 1995
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBmultivar *
DBGetMultivar(DBfile *dbfile, const char *name)
{
    DBmultivar * retval = NULL;

    API_BEGIN2("DBGetMultivar", DBmultivar *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetMultivar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multivar name", E_BADARGS);
        if (!dbfile->pub.g_mv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_mv) (dbfile, name);
        if (DBGetEvalNameschemesFile(dbfile))
          DBEvalMultivarNameschemes(dbfile, retval);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetMultimat
 *
 * Purpose:     Allocates a DBmultimat data structure, reads a multi-
 *              material from the database, and returns a pointer to the
 *              new structure.
 *
 * Return:      Success:        pointer to the new DBmultimat
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@cloud
 *              Tue Feb 21 11:27:46 EST 1995
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBmultimat *
DBGetMultimat(DBfile *dbfile, const char *name)
{
    DBmultimat * retval = NULL;

    API_BEGIN2("DBGetMultimat", DBmultimat *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetMultimat", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multimat name", E_BADARGS);
        if (!dbfile->pub.g_mt)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_mt) (dbfile, name);
        if (DBGetEvalNameschemesFile(dbfile))
          DBEvalMultimatNameschemes(dbfile, retval);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetMultimatspecies
 *
 * Purpose:     Allocates a DBmultimatspecies data structure, reads a
 *              multi-material-species from the database, and returns
 *              a pointer to the new structure.
 *
 * Return:      Success:        pointer to the new DBmultimatspecies
 *
 *              Failure:        NULL
 *
 * Programmer:  Jeremy S. Meredith  
 *              Sept 17 1998
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBmultimatspecies *
DBGetMultimatspecies(DBfile *dbfile, const char *name)
{
    DBmultimatspecies * retval = NULL;

    API_BEGIN2("DBGetMultimatspecies", DBmultimatspecies *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetMultimatspecies", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multimatspecies name", E_BADARGS);
        if (!dbfile->pub.g_mms)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_mms) (dbfile, name);
        if (DBGetEvalNameschemesFile(dbfile))
          DBEvalMultimatspeciesNameschemes(dbfile, retval);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetPointmesh
 *
 * Purpose:     Allocates a DBpointmesh data structure and reads a point
 *              mesh from the database.
 *
 * Return:      Success:        pointer to the new DBpointmesh struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 09:37:55 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBpointmesh *
DBGetPointmesh(DBfile *dbfile, const char *name)
{
    DBpointmesh * retval = NULL;

    API_BEGIN2("DBGetPointmesh", DBpointmesh *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetPointmesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("pointmesh name", E_BADARGS);
        if (!dbfile->pub.g_pm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_pm) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetPointvar
 *
 * Purpose:     Allocates a DBmeshvar data structure and reads a variable
 *              associated with a point mesh from the database.
 *
 * Return:      Success:        pointer to the new DBmeshvar struct
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 09:41:21 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBmeshvar *
DBGetPointvar(DBfile *dbfile, const char *name)
{
    DBmeshvar * retval = NULL;

    API_BEGIN2("DBGetPointvar", DBmeshvar *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetPointvar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("pointvar name", E_BADARGS);
        if (!dbfile->pub.g_pv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_pv) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetQuadmesh
 *
 * Purpose:     Allocates a DBquadmesh data structure and reads
 *              a quadrilateral mesh from the databas.
 *
 * Return:      Success:        pointer to the new DBquadmesh struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 09:44:26 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 * 
 *    Hank Childs, Fri Feb 25 09:48:40 PST 2000
 *    Initialized start_index and size_index.
 *
 *-------------------------------------------------------------------------*/
PUBLIC DBquadmesh *
DBGetQuadmesh(DBfile *dbfile, const char *name)
{
    DBquadmesh    *qm = NULL;
    int            i;

    API_BEGIN2("DBGetQuadmesh", DBquadmesh *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetQuadmesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("quadmesh name", E_BADARGS);
        if (!dbfile->pub.g_qm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        qm = (dbfile->pub.g_qm) (dbfile, name);
        if (!qm)
        {
            API_RETURN(NULL);
        }

        /*
         * Put in default axis labels if none supplied.
         */
        switch (qm->ndims) {
            case 3:
                if (qm->labels[2] == NULL) {
                    qm->labels[2] = ALLOC_N(char, 7);

                    strcpy(qm->labels[2], "Z Axis");
                }
                /* Fall through */
            case 2:
                if (qm->labels[1] == NULL) {
                    qm->labels[1] = ALLOC_N(char, 7);

                    strcpy(qm->labels[1], "Y Axis");
                }
                /* Fall through */
            case 1:
                if (qm->labels[0] == NULL) {
                    qm->labels[0] = ALLOC_N(char, 7);

                    strcpy(qm->labels[0], "X Axis");
                }
                break;
        }

        for (i = 0 ; i < 3 ; i++)
        {
            qm->start_index[i] = 0;
            qm->size_index[i]  = qm->dims[i];
        }

        API_RETURN(qm);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetQuadvar
 *
 * Purpose:     Allocates a DBquadvar data structure and reads a variable
 *              associated with a quadrilateral mesh from the database.
 *
 * Return:      Success:        Pointer to the new DBquadvar struct
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 09:46:57 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBquadvar *
DBGetQuadvar(DBfile *dbfile, const char *name)
{
    DBquadvar * retval = NULL;

    API_BEGIN2("DBGetQuadvar", DBquadvar *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetQuadvar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("quadvar name", E_BADARGS);
        if (!dbfile->pub.g_qv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_qv) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBAnnotateUcdmesh
 *
 * Purpose:     Walks a DBucdmesh data structure and adds shapetype
 *              info based on ndims and shapesize (spatial dimensions
 *              and node count).
 *
 * Return:      1:        One or more zones/shapes were annotated.
 *
 *              0:        No annotation was performed.
 *
 *              -1:       Annotation could not be performed (an
 *                        error condition such as exhaustion of
 *                        dynamic memory occurred).
 *
 * Programmer:  reus@viper
 *              Tue Oct  8 09:40:36 PDT 1996
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
PUBLIC int
DBAnnotateUcdmesh(DBucdmesh *m)
{
   if (m != NULL)
   {
      DBzonelist *z;

      if ((z=m->zones) != NULL)
         if (z->shapetype == NULL)
         {
            int N, dims;

            if (m->topo_dim == 0 || m->topo_dim == 1 || m->topo_dim == 2)
                dims = m->topo_dim;
            else
                dims = z->ndims;

            N = z->nshapes;
            if ((z->shapetype=(int *)malloc(N*sizeof(int))) != NULL)
            {
               int *numberOfNodes;

               if ((numberOfNodes=z->shapesize) != NULL)
               {
                  int i;

                  switch (dims)
                  {
                    case 1: for (i=0; i<N; ++i)
                               z->shapetype[i] = DB_ZONETYPE_BEAM;
                            break;
                    case 2: for (i=0; i<N; ++i)
                               switch (numberOfNodes[i])
                               {
                                 case 3:  z->shapetype[i] = DB_ZONETYPE_TRIANGLE;
                                          break;
                                 case 4:  z->shapetype[i] = DB_ZONETYPE_QUAD;
                                          break;
                                 default: z->shapetype[i] = DB_ZONETYPE_POLYGON;
                                          break;
                               }
                            break;
                    case 3: for (i=0; i<N; ++i)
                               switch (numberOfNodes[i])
                               {
                                 case 4:  z->shapetype[i] = DB_ZONETYPE_TET;
                                          break;
                                 case 5:  z->shapetype[i] = DB_ZONETYPE_PYRAMID;
                                          break;
                                 case 6:  z->shapetype[i] = DB_ZONETYPE_PRISM;
                                          break;
                                 case 8:  z->shapetype[i] = DB_ZONETYPE_HEX;
                                          break;
                                 default: z->shapetype[i] = DB_ZONETYPE_POLYHEDRON;
                                          break;
                               }
                            break;
                  }
                  return 1;
               }
               else
                  return 0;
            }
            else
               return -1;
         }
         else
            return 0;
      else
         return 0;
   }
   else
      return 0;
}

/*-------------------------------------------------------------------------
 * Function:    DBGetUcdmesh
 *
 * Purpose:     Allocates a DBucdmesh data structure and reads a UCD mesh
 *              from the data file.
 *
 * Return:      Success:        Pointer to the new DBucdmesh struct
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 09:57:59 PST 1994
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBucdmesh *
DBGetUcdmesh(DBfile *dbfile, const char *name)
{
    DBucdmesh     *um = NULL;

    API_BEGIN2("DBGetUcdmesh", DBucdmesh *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetUcdmesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("UCDmesh name", E_BADARGS);
        if (!dbfile->pub.g_um)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        um = ((dbfile->pub.g_um) (dbfile, name));
        if (!um)
        {
            API_RETURN(NULL);
        }

        /*
         * Put in default axis labels if none supplied.
         */
        switch (um->ndims) {
            case 3:
                if (um->labels[2] == NULL) {
                    um->labels[2] = ALLOC_N(char, 7);

                    if (!um->labels[2])
                        API_ERROR(NULL, E_NOMEM);
                    strcpy(um->labels[2], "Z Axis");
                }
                /*fall through */
            case 2:
                if (um->labels[1] == NULL) {
                    um->labels[1] = ALLOC_N(char, 7);

                    if (!um->labels[1])
                        API_ERROR(NULL, E_NOMEM);
                    strcpy(um->labels[1], "Y Axis");
                }
                /*fall through */
            case 1:
                if (um->labels[0] == NULL) {
                    um->labels[0] = ALLOC_N(char, 7);

                    if (!um->labels[0])
                        API_ERROR(NULL, E_NOMEM);
                    strcpy(um->labels[0], "X Axis");
                }
        }
        if (DBAnnotateUcdmesh(um) < 0)
           API_ERROR(NULL, E_NOMEM);

        API_RETURN(um);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetUcdvar
 *
 * Purpose:     Allocates a DBucdvar structure and reads a variable associated
 *              with a UCD mesh from the database.
 *
 * Return:      Success:        Pointer to the new DBucdvar struct
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:04:35 PST 1994
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBucdvar *
DBGetUcdvar(DBfile *dbfile, const char *name)
{
    DBucdvar * retval = NULL;

    API_BEGIN2("DBGetUcdvar", DBucdvar *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetUcdvar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("UCDvar name", E_BADARGS);
        if (!dbfile->pub.g_uv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_uv) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetFacelist
 *
 * Purpose:     Allocate and read a DBfacelist structure.
 *
 * Return:      Success:        ptr to new DBfacelist
 *
 *              Failure:        NULL
 *
 * Programmer:  robb@cloud
 *              Wed Dec 14 13:50:44 EST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBfacelist *
DBGetFacelist(DBfile *dbfile, const char *name)
{
    DBfacelist * retval = NULL;

    API_BEGIN2("DBGetFacelist", DBfacelist *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetFacelist", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("facelist name", E_BADARGS);
        if (!dbfile->pub.g_fl)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_fl) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetZonelist
 *
 * Purpose:     Allocate and read a DBzonelist structure.
 *
 * Return:      Success:        ptr to new DBzonelist.
 *
 *              Failure:        NULL
 *
 * Programmer:  robb@cloud
 *              Wed Dec 14 13:59:51 EST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBzonelist *
DBGetZonelist(DBfile *dbfile, const char *name)
{
    DBzonelist * retval = NULL;

    API_BEGIN2("DBGetZonelist", DBzonelist *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetZonelist", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("zonelist name", E_BADARGS);
        if (!dbfile->pub.g_zl)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_zl) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetPHZonelist
 *
 * Purpose:     Allocate and read a DBphzonelist structure.
 *
 * Return:      Success:        ptr to new DBphzonelist.
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller 
 *              July 28, 2004 
 *
 *-------------------------------------------------------------------------*/
PUBLIC DBphzonelist *
DBGetPHZonelist(DBfile *dbfile, const char *name)
{
    DBphzonelist * retval = NULL;

    API_BEGIN2("DBGetPHZonelist", DBphzonelist *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetPHZonelist", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("zonelist name", E_BADARGS);
        if (!dbfile->pub.g_phzl)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_phzl) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetVar
 *
 * Purpose:     Allocate space for a variable and read the variable from the
 *              database.
 *
 * Return:      Success:        Pointer to the variable value.
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:11:29 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
PUBLIC void   *
DBGetVar(DBfile *dbfile, const char *name)
{
    void   * retval = NULL;

    API_BEGIN2("DBGetVar", void *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetVar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("variable name", E_BADARGS);
        if (!dbfile->pub.g_var)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_var) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBReadVar
 *
 * Purpose:     Same as DBGetVar() except the user supplies the result
 *              memory instead of the function allocating it on the heap.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Wed Nov  9 13:00:07 EST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBReadVar(DBfile *dbfile, const char *name, void *result)
{
    int retval;

    API_BEGIN2("DBReadVar", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBReadVar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("variable name", E_BADARGS);
        if (!result)
            API_ERROR("result pointer", E_BADARGS);
        if (!dbfile->pub.r_var)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.r_var) (dbfile, name, result);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBReadVarSlice
 *
 * Purpose:     Same as DBReadVarSlice() except the user can read a only
 *              a slice of the variable.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  brugger@sgibird
 *              Thu Feb 16 08:25:47 PST 1995
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBReadVarSlice(DBfile *dbfile, const char *name, int const *offset, int const *length,
               int const *stride, int ndims, void *result)
{
    int retval;

    API_BEGIN2("DBReadVarSlice", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBReadVarSlice", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("variable name", E_BADARGS);
        if (!offset)
            API_ERROR("offset", E_BADARGS);
        if (!length)
            API_ERROR("length", E_BADARGS);
        if (!stride)
            API_ERROR("stride", E_BADARGS);
        if (ndims <= 0)
            API_ERROR("ndims", E_BADARGS);
        if (!result)
            API_ERROR("result pointer", E_BADARGS);
        if (!dbfile->pub.r_varslice)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.r_varslice) (dbfile, name, offset,
                                           length, stride, ndims, result);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}
/*-------------------------------------------------------------------------
 * Function:    DBReadVarVals
 *
 * Purpose:     Like DBReadVarSlice() except the user can read a specifc
 *              set of values of the variable.
 *
 *              mode can be either DB_PARTIO_POINTS or DB_PARTIO_HSLABS.
 *
 *              For DB_PARTIO_POINTS case, the indices array is treated as
 *              NVALS x NDIMS values, each specifying the logical
 *              coordinates of a single point in the array. A total of
 *              NVALS values are returned in the same order as their
 *              associated logical indices are specified.
 *
 *              For DB_PARTIO_HSLABS case, the indices array is treated as
 *              NVALS x NDIMS x 3. NVALS is the number of hyper slabs.
 *              NDIMS is the dimensionality of each hyper slab and 3 is
 *              for a <start,count,stride> 3-tuple for each dimension
 *              of a hyperslab. The number of returned values is the sum
 *              of the sizes of the hyperslabs. If hyperslabs overlap,
 *              values will be repeated in the results.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer : Mark C. Miller, Sat Jun  4 17:40:23 PDT 2016
 *-------------------------------------------------------------------------*/
PUBLIC int
DBReadVarVals(DBfile *dbfile, const char *name, int mode, int nvals,
    int ndims, void const *indices, void **result, int *ncomps, int *nitems)
{
    int retval;

    API_BEGIN2("DBReadVarVals", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBReadVarVals", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("variable name", E_BADARGS);
        if (nvals <= 0)
            API_ERROR("nvals", E_BADARGS);
        if (ndims <= 0)
            API_ERROR("ndims", E_BADARGS);
        if (!result)
            API_ERROR("result pointer", E_BADARGS);
        if (!dbfile->pub.r_varvals)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.r_varvals) (dbfile, name, mode,
            nvals, ndims, indices, result, ncomps, nitems);

        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetVarByteLength
 *
 * Purpose:     Returns the in-memory length of the requested variable,
 *              in bytes.
 *
 * Return:      Success:        length of variable in bytes.
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:18:35 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
static int _DBGetVarByteLength_r(DBfile *dbfile, const char *name);
#ifndef _WIN32
#warning DO WE NEED THIS METHOD
#endif
PRIVATE size_t
db_get_obj_byte_length(DBfile *dbfile, char const *name)
{
    int q;
    size_t sum = 0;
    DBobject *srcObj = DBGetObject(dbfile, name);
    if (!srcObj) return 0;

    for (q = 0; q < srcObj->ncomponents; q++)
    {
        if (!strncmp(srcObj->pdb_names[q], "'<i>", 4))
            sum += sizeof(int);
        else if (!strncmp(srcObj->pdb_names[q], "'<f>", 4))
            sum += sizeof(float);
        else if (!strncmp(srcObj->pdb_names[q], "'<d>", 4))
            sum += sizeof(double);
        else /* possible sub-object */
        {
            int bytlen;
            char *subObjName, *srcObjDirName, *srcSubObjAbsName;
             
            if (!strncmp(srcObj->pdb_names[q], "'<s>", 4))
                subObjName = db_strndup(srcObj->pdb_names[q]+4, strlen(srcObj->pdb_names[q])-5);
            else
                subObjName = db_strndup(srcObj->pdb_names[q], strlen(srcObj->pdb_names[q]));

            srcObjDirName = db_dirname(name);
            srcSubObjAbsName = db_join_path(srcObjDirName, subObjName);
            free(subObjName);
            free(srcObjDirName);

            bytlen = _DBGetVarByteLength_r(dbfile, srcSubObjAbsName); /* recursion */
            free(srcSubObjAbsName);
            if (bytlen > 0) sum += bytlen;
        }
    }
    DBFreeObject(srcObj);
    return sum;
}

PRIVATE int
_DBGetVarByteLength_r(DBfile *dbfile, const char *name)
{
    int retval = (dbfile->pub.g_varbl) (dbfile, name);
    if (retval < 0)
        retval = (int) db_get_obj_byte_length(dbfile, name);
    return retval;
}

PUBLIC int
DBGetVarByteLength(DBfile *dbfile, const char *name)
{
    int retval;

    API_BEGIN2("DBGetVarByteLength", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetVarByteLength", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("variable name", E_BADARGS);
        if (!dbfile->pub.g_varbl)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = _DBGetVarByteLength_r(dbfile, name);

        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetVarByteLengthInFile
 *
 * Purpose:     Returns the in-file length of the requested variable,
 *              in bytes.
 *
 * Return:      Success:        length of variable in bytes.
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:18:35 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBGetVarByteLengthInFile(DBfile *dbfile, const char *name)
{
    int retval;

    API_BEGIN2("DBGetVarByteLengthInFile", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetVarByteLengthInFile", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("variable name", E_BADARGS);
        if (!dbfile->pub.g_varblf)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_varblf) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetVarLength
 *
 * Purpose:     Returns the number of elements in the given variable.
 *
 * Return:      Success:        number of elements
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:23:20 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBGetVarLength(DBfile *dbfile, const char *name)
{
    int retval;

    API_BEGIN2("DBGetVarLength", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetVarLength", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("variable name", E_BADARGS);
        if (!dbfile->pub.g_varlen)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_varlen) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetVarDims
 *
 * Purpose:     Returns information about the dimensions of a variable.
 *              The user passes a buffer to hold the dimension sizes
 *              and indicates the size of that buffer.
 *
 * Return:      Success:        The number of dimensions not exceeding
 *                              MAXDIMS.  The dimension sizes are returned
 *                              through DIMS.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Mar  6 1997
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Reformatted the code so that a human can read it.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBGetVarDims(DBfile *dbfile, const char *name, int maxdims, int *dims)
{
    int             retval = -1;

    API_BEGIN2("DBGetVarDims", int, -1, name)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetVarDims", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("variable name", E_BADARGS);
        if (maxdims <= 0)
            API_ERROR("max dims", E_BADARGS);
        if (!dims)
            API_ERROR("dimension buffer pointer", E_BADARGS);
        if (!dbfile->pub.g_vardims)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_vardims) (dbfile, name, maxdims, dims);
        API_RETURN(retval);
    }
    API_END_NOPOP;                     /* BEWARE: If API_RETURN above is
                                        * removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetVarType
 *
 * Purpose:     Returns the data type of a variable.
 *
 * Return:      Success:        type, such as DB_INT, DB_FLOAT, etc.
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Thu Dec 22 08:54:20 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBGetVarType(DBfile *dbfile, const char *name)
{
    int retval;

    API_BEGIN2("DBGetVarType", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetVarType", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("variable name", E_BADARGS);
        if (!dbfile->pub.g_vartype)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_vartype) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBInqMeshname
 *
 * Purpose:     Returns the name of a mesh associated with a mesh
 *              variable.
 *
 * Return:      Success:        0, name returned via `mname'
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:27:15 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBInqMeshname(DBfile *dbfile, const char *vname, char *mname)
{
    int retval;

    API_BEGIN2("DBInqMeshname", int, -1, vname) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBInqMeshname", E_GRABBED) ; 
        if (!vname || !*vname)
            API_ERROR("variable name", E_BADARGS);
        if (!mname)
            API_ERROR("mesh name pointer", E_BADARGS);
        if (!dbfile->pub.i_meshname)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.i_meshname) (dbfile, vname, mname);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBInqMeshtype
 *
 * Purpose:     Returns the mesh type for the specified mesh.
 *
 * Return:      Success:        mesh type constant
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:31:56 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBInqMeshtype(DBfile *dbfile, const char *name)
{
    int retval;

    API_BEGIN2("DBInqMeshtype", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBInqMeshtype", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("mesh name", E_BADARGS);
        if (!dbfile->pub.i_meshtype)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.i_meshtype) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutCompoundarray
 *
 * Purpose:     Write compoundarray information to a file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Mon Nov  7 10:54:46 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutCompoundarray(
    DBfile *dbfile,
    char const *name,
    char const * const *elemnames,
    int const *elemlengths,
    int nelems,
    void const *values,
    int nvalues,
    int datatype,
    DBoptlist const *opts
)
{
    int retval;

    API_BEGIN2("DBPutCompoundarray", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutCompoundarray", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("array name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("array name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nelems < 0)
            API_ERROR("nelems<0", E_BADARGS);
        if (nelems)
        {
            if (!nelems)
                API_ERROR("nelems=0", E_BADARGS);
            if (!elemnames)
                API_ERROR("elemnames=0", E_BADARGS);
            if (nvalues <= 0)
                API_ERROR("nvalues=0", E_BADARGS);
            if (!values)
                API_ERROR("values=0", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nelems=0", E_EMPTYOBJECT);
        }
        else
        {
            nvalues = 0;
        }

        if (NULL == dbfile->pub.p_ca)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_ca) (dbfile, name, elemnames,
                                     elemlengths, nelems, values, nvalues,
                                     datatype, opts);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutCurve
 *
 * Purpose:     Writes curve information to a file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.com
 *              May 15, 1996
 *
 * Modifications:
 *    Robb Matzke, 16 May 1996
 *    Don't check for existence of XVALS and YVALS since the OPTS can
 *    specify a PDB variable name which has already been added to the
 *    file and which contains the necessary x or y values.  However,
 *    the driver should check.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutCurve(
    DBfile *dbfile,
    char const *name,
    void const *xvals,
    void const *yvals,
    int datatype,
    int npts,
    DBoptlist const *opts
)
{
    int retval;

    API_BEGIN2("DBPutCurve", int, -1, name)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutCurve", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("curve name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("curve name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (npts < 0)
            API_ERROR("npts<0", E_BADARGS);
        if (npts)
        {
            if (DBGetOption(opts, DBOPT_REFERENCE))
            {
                if (yvals || xvals)
                    API_ERROR("xvals & yvals must be null when using DBOPT_REFERENCE", E_BADARGS);
            }
            else
            {
                if (!xvals && !DBGetOption(opts, DBOPT_XVARNAME))
                    API_ERROR("xvals=0 || DBOPT_XVARNAME", E_BADARGS);
                if (!yvals && !DBGetOption(opts, DBOPT_YVARNAME))
                    API_ERROR("yvals=0 || DBOPT_YVARNAME", E_BADARGS);
#ifndef DB_HDF5X
                if (xvals && DBGetOption(opts, DBOPT_XVARNAME))
                    API_ERROR("xvals!=0 && DBOPT_XVARNAME", E_BADARGS);
                if (yvals && DBGetOption(opts, DBOPT_YVARNAME))
                    API_ERROR("yvals!=0 && DBOPT_YVARNAME", E_BADARGS);
#else
                if (DBGetDriverType(dbfile) != DB_HDF5X && xvals && DBGetOption(opts, DBOPT_XVARNAME))
                    API_ERROR("xvals!=0 && DBOPT_XVARNAME", E_BADARGS);
                if (DBGetDriverType(dbfile) != DB_HDF5X && yvals && DBGetOption(opts, DBOPT_YVARNAME))
                    API_ERROR("yvals!=0 && DBOPT_YVARNAME", E_BADARGS);
#endif
            }
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile) &&
                 !DBGetOption(opts, DBOPT_REFERENCE))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("npts=0", E_EMPTYOBJECT);
        }

        if (NULL == dbfile->pub.p_cu)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_cu) (dbfile, name, (void*)xvals, (void*)yvals,
                                     datatype, npts, opts);

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /* BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutDefvars
 *
 * Purpose:     Writes a Defvars object to a file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller 
 *              August 8, 2005
 *
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutDefvars(
    DBfile *dbfile,
    const char *name,
    int ndefs,
    char const * const *names,
    int const *types,
    char const * const *defns,
    DBoptlist const * const *opts
)
{
    int retval;

    API_BEGIN2("DBPutDefvars", int, -1, name)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutDefvars", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("defvars name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("defvars name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (ndefs < 0)
            API_ERROR("ndefs", E_BADARGS);
        if (ndefs)
        {
            if (!names)
                API_ERROR("names=0", E_BADARGS);
            if (!types)
                API_ERROR("types=0", E_BADARGS);
            if (!defns)
                API_ERROR("defns=0", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("ndefs=0", E_EMPTYOBJECT);
        }

        if (NULL == dbfile->pub.p_defv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_defv) (dbfile, name, ndefs, names,
                                       types, defns, opts);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /* BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutFacelist
 *
 * Purpose:     Writes a facelist object into the open database file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:52:25 PST 1994
 *
 * Modifications:
 *    Robb Matzke, Thu Dec 1 10:34:09 PST 1994
 *    The `zoneno' parameter is optional even if
 *    the number of faces is zero.
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutFacelist(DBfile *dbfile, const char *name, int nfaces, int ndims,
              int const *nodelist, int lnodelist, int origin, int const *zoneno,
              int const *shapesize, int const *shapecnt, int nshapes, int const *types,
              int const *typelist, int ntypes)
{
    int retval;

    API_BEGIN2("DBPutFacelist", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutFacelist", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("facelist name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("facelist name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nfaces < 0)
            API_ERROR("nfaces<0", E_BADARGS);
        if (nfaces)
        {
            if (nfaces <= 0)
                API_ERROR("nfaces<=0", E_BADARGS);
            if (ndims <= 0)
                API_ERROR("ndims<=0", E_BADARGS);
            if (lnodelist <= 0)
                API_ERROR("lnodelist<0", E_BADARGS);
            if (nshapes <= 0)
                API_ERROR("nshapes<0", E_BADARGS);
            if (!nodelist)
                API_ERROR("nodelist==0", E_BADARGS);
            if (!shapesize)
                API_ERROR("shapesize==0", E_BADARGS);
            if (!shapecnt)
                API_ERROR("shapecnt==0", E_BADARGS);
            if (ntypes < 0)
                API_ERROR("ntypes<0", E_BADARGS);
            if (origin != 0 && origin != 1)
                API_ERROR("origin", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nfaces=0", E_EMPTYOBJECT);
        }
        else
        {
            nfaces = 0;
            lnodelist = 0;
            nshapes = 0;
            ntypes = 0;
        }

        if (!dbfile->pub.p_fl)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_fl) (dbfile, name, nfaces, ndims,
                                     nodelist, lnodelist, origin, zoneno,
                                     shapesize, shapecnt, nshapes, types,
                                     typelist, ntypes);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutMaterial
 *
 * Purpose:     Writes a material data object into the current open
 *              database.  The minimum required information for a material
 *              data object is supplied via the standard arguments to the
 *              function.  The `optlist' argument must be used for
 *              supplying any information not requested through the
 *              standard arguments.
 *
 * Return:      Success:        object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 12:05:23 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Sean Ahern, Thu Jun  8 12:08:05 PDT 2000
 *    Removed an unnecessary check on mix_zone.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutMaterial(
    DBfile *dbfile,
    char const *name,
    char const *meshname,
    int nmat,
    int const *matnos,
    int const *matlist,
    int const *dims,
    int ndims,
    int const *mix_next,
    int const *mix_mat,
    int const *mix_zone,
    DBVCP1_t mix_vf,
    int mixlen,
    int datatype,
    DBoptlist const *optlist
)
{
    int i, retval, is_empty = 1;
    int const zdims[10] = {0,0,0,0,0,0,0,0,0,0};

    API_BEGIN2("DBPutMaterial", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutMaterial", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("material name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("material name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nmat < 0)
            API_ERROR("nmat<0", E_BADARGS);
        if (ndims < 0)
            API_ERROR("ndims<0", E_BADARGS);
        if (!dims)
            API_ERROR("dims=0", E_BADARGS);
        for (i = 0; i < ndims; i++)
        {
            if (dims[i] != 0)
            {
                is_empty = 0;
                break;
            }
        }
        if (!is_empty)
        {
            for (i = 0; i < ndims && dims!=zdims; i++)
                if (dims[i] == 0) dims = zdims;
            if (!matnos) API_ERROR("matnos=0", E_BADARGS);
            if (!matlist) API_ERROR("matlist=0", E_BADARGS);
            if (!meshname || !*meshname) API_ERROR("mesh name", E_BADARGS);
            if (!db_VariableNameValid(meshname)) API_ERROR("meshname", E_INVALIDNAME);
            if (mixlen < 0)
                API_ERROR("mixlen<0", E_BADARGS);
            if (mixlen)
            {
                if (!mix_next)
                    API_ERROR("mix_next=0", E_BADARGS);
                if (!mix_mat)
                    API_ERROR("mix_mat=0", E_BADARGS);
                if (!mix_vf)
                    API_ERROR("mix_vf=0", E_BADARGS);
            }
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            API_ERROR("nmat=0", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_ma)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_ma) (dbfile, name, meshname,
                                     nmat, matnos, matlist, dims, ndims,
                                     mix_next, mix_mat, mix_zone, mix_vf,
                                     mixlen, datatype, optlist);
#ifndef _WIN32
#warning BETTER PLACE TO NULL THESE
#endif
        /* Zero out the _ma._matnames pointer so we can't accidentially use it
         * again. Likewise for matcolors. */
        _ma._matnames = NULL;
        _ma._matcolors = NULL;

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 *  Routine                                                DBPutMatspecies
 *
 *  Purpose
 *
 *      Write a material species object into the open file.
 *
 *  Programmer
 *
 *      Al Leibee, B-DSAD
 *
 *  Notes
 *
 *      One zonal array ('speclist') is used which contains either:
 *      1) an index into the 'species_mf' array of the species mass fractions
 *         of a clean zone's material.
 *
 *                                  OR
 *      2) an index into the 'mix_speclist' array which contains an index
 *         into the 'species_mf' of the species mass fractions of a mixed
 *         zone's materials.
 *
 *  Modified
 *    Robb Matzke, Mon Nov 28 15:19:50 EST 1994
 *    Added device independence stuff.
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *--------------------------------------------------------------------*/
PUBLIC int
DBPutMatspecies(DBfile *dbfile, const char *name, const char *matname,
                int nmat, int const *nmatspec, int const *speclist, int const *dims,
                int ndims, int nspecies_mf, DBVCP1_t species_mf,
                int const *mix_speclist, int mixlen, int datatype,
                DBoptlist const *optlist)
{
    int i, retval, is_empty=1;

    API_BEGIN2("DBPutMatspecies", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutMatspecies", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("matspecies name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("matspecies name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nmat < 0)
            API_ERROR("nmat<0", E_BADARGS);
        if (ndims < 0)
            API_ERROR("ndims<0", E_BADARGS);
        if (!dims)
            API_ERROR("dims=0", E_BADARGS);
        if (nspecies_mf < 0)
            API_ERROR("nspecies_mf<0", E_BADARGS);
        for (i = 0; i < ndims; i++)
        {
            if (dims[i] != 0)
            {
                is_empty = 0;
                break;
            }
        }
        if (!is_empty && nspecies_mf > 0)
        {
            if (!matname || !*matname)
                API_ERROR("material name", E_BADARGS);
            if (db_VariableNameValid(matname) == 0)
                API_ERROR("material name", E_INVALIDNAME);
            if (ndims == 1 && dims[0] <= 0) API_ERROR("dims[0]<=0", E_BADARGS);
            if (ndims == 2 && dims[1] <= 0) API_ERROR("dims[1]<=0", E_BADARGS);
            if (ndims == 3 && dims[2] <= 0) API_ERROR("dims[2]<=0", E_BADARGS);
            if (!nmatspec) API_ERROR("nmatspec=0", E_BADARGS);
            if (!speclist) API_ERROR("speclist=0", E_BADARGS);
            if (!species_mf) API_ERROR("species_mf=0", E_BADARGS);
            if (mixlen < 0)
                API_ERROR("mixlen", E_BADARGS);
            if (!mix_speclist && mixlen)
                API_ERROR("mix_speclist", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            API_ERROR("dims[i]==0 for all i || nspecies_mf==0", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_ms)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        retval = (dbfile->pub.p_ms) (dbfile, name, matname,
                                     nmat, nmatspec, speclist, dims, ndims,
                                     nspecies_mf, species_mf, mix_speclist,
                                     mixlen, datatype, optlist);

        /* Zero out the _ma._matnames pointer so we can't accidentially use it
         * again. Likewise for matcolors. */
        _ms._specnames = NULL;
        _ms._speccolors = NULL;

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutMultimesh
 *
 * Purpose:     Writes a multi-bloc kmesh object into the open database.
 *              It accepts as input descriptions of the various sub-meshes
 *              (blocks) which are part of this mesh.
 *
 * Return:      Success:        object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 12:17:57 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Wed Jul 14 20:36:23 PDT 2010
 *    Added support for nameschemes on multi-block objects. This meant
 *    adjusting smoke checks for args as some can be null now.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutMultimesh(DBfile *dbfile, char const *name, int nmesh,
    char const * const *meshnames, int const *meshtypes,
    DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutMultimesh", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutMultimesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multimesh name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("multimesh name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nmesh < 0)
            API_ERROR("nmesh", E_BADARGS);
        if (nmesh)
        {
            if (!meshnames && (!optlist || 
                (!DBGetOption(optlist, DBOPT_MB_FILE_NS) &&
                 !DBGetOption(optlist, DBOPT_MB_BLOCK_NS))))
                API_ERROR("mesh names", E_BADARGS);
            if (!meshtypes && (!optlist ||
                 !DBGetOption(optlist, DBOPT_MB_BLOCK_TYPE)))
                API_ERROR("mesh types", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nmesh==0", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_mm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_mm) (dbfile, name, nmesh, meshnames,
                                     meshtypes, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutMultimeshadj
 *
 * Purpose:     Writes a multi-mesh adjacency object into the
 *              open database.
 *
 * Return:      Success:        object ID
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller, August 23, 2005 
 *
 * Notes:       This function is designed to support repeated calls where
 *              Different parts of the same object are written at different
 *              times.
 *
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutMultimeshadj(DBfile *dbfile, char const *name, int nmesh,
                  int const *meshtypes, int const *nneighbors,
                  int const *neighbors, int const *back,
                  int const *lnodelists, int const * const *nodelists,
                  int const *lzonelists, int const * const *zonelists,
                  DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutMultimeshadj", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutMultimeshadj", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multimeshadj name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("multimeshadj name", E_INVALIDNAME);
        if (nmesh < 0)
            API_ERROR("nmesh", E_BADARGS);
        if (!meshtypes && nmesh)
            API_ERROR("mesh types", E_BADARGS);
        if (!nneighbors && nmesh)
            API_ERROR("nneighbors", E_BADARGS);
        if (!neighbors && nmesh)
            API_ERROR("neighbors", E_BADARGS);
        if (lnodelists == NULL && nodelists != NULL)
            API_ERROR("non-NULL nodelists", E_BADARGS);
        if (lzonelists == NULL && zonelists != NULL)
            API_ERROR("non-NULL zonelists", E_BADARGS);
        if (!dbfile->pub.p_mmadj)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_mmadj) (dbfile, name, nmesh, meshtypes,
                                        nneighbors, neighbors, back,
                                        lnodelists, nodelists, lzonelists, zonelists,
                                        optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutMultivar
 *
 * Purpose:     Writes a multi-block variable object to the database.
 *
 * Return:      Success:        object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 12:26:30 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Wed Jul 14 20:36:23 PDT 2010
 *    Added support for nameschemes on multi-block objects. This meant
 *    adjusting smoke checks for args as some can be null now.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutMultivar(DBfile *dbfile, const char *name, int nvar,
              char const * const *varnames, int const *vartypes, DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutMultivar", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutMultivar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multivar name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("multivar name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nvar < 0)
            API_ERROR("nvar", E_BADARGS);
        if (nvar)
        {
            if (!varnames && (!optlist ||
                 (!DBGetOption(optlist, DBOPT_MB_FILE_NS) &&
                  !DBGetOption(optlist, DBOPT_MB_BLOCK_NS))))
                API_ERROR("varnames", E_BADARGS);
            if (!vartypes && (!optlist ||
                 !DBGetOption(optlist, DBOPT_MB_BLOCK_TYPE)))
                API_ERROR("vartypes", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nvar==0", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_mv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_mv) (dbfile, name, nvar, varnames,
                                     vartypes, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutMultimat
 *
 * Purpose:     Writes a multi-material object to the database.
 *
 * Return:      Success:        object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Feb 21 12:35:10 EST 1995
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Wed Jul 14 20:36:23 PDT 2010
 *    Added support for nameschemes on multi-block objects. This meant
 *    adjusting smoke checks for args as some can be null now.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutMultimat(DBfile *dbfile, const char *name, int nmats,
              char const * const *matnames, DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutMultimat", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutMultimat", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multimat name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("multimat name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nmats < 0)
            API_ERROR("nmats", E_BADARGS);
        if (nmats)
        {
            if (!matnames && (!optlist ||
                 (!DBGetOption(optlist, DBOPT_MB_FILE_NS) && 
                  !DBGetOption(optlist, DBOPT_MB_BLOCK_NS))))
                API_ERROR("material-names", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nmats==0", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_mt)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_mt) (dbfile, name, nmats, matnames,
                                     optlist);
#ifndef _WIN32
#warning BETTER PLACE TO NULL THESE
#endif
        /* Zero out the _mm._matnames pointer so we can't accidentially use it
         * again. Likewise for matcolors. */
        _mm._matnames = NULL;
        _mm._matcolors = NULL;

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutMultimatspecies
 *
 * Purpose:     Writes a multi-material object to the database.
 *
 * Return:      Success:        object ID
 *
 *              Failure:        -1
 *
 * Programmer:  Jeremy S. Meredith
 *              Sept 17 1998
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Wed Jul 14 20:36:23 PDT 2010
 *    Added support for nameschemes on multi-block objects. This meant
 *    adjusting smoke checks for args as some can be null now.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutMultimatspecies(DBfile *dbfile, const char *name, int nspec,
                     char const * const *specnames, DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutMultimatspecies", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutMultimatspecies", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("multimatspecies name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("multimatspecies name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nspec < 0)
            API_ERROR("nspec", E_BADARGS);
        if (nspec)
        {
            if (!specnames && (!optlist ||
                 (!DBGetOption(optlist, DBOPT_MB_FILE_NS) && 
                  !DBGetOption(optlist, DBOPT_MB_BLOCK_NS))))
                API_ERROR("species-names", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nspec==0", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_mms)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_mms) (dbfile, name, nspec, specnames, optlist);

        /* Zero out the _ma._matnames pointer so we can't accidentially use it
         * again. Likewise for matcolors. */
        _mm._specnames = NULL;
        _mm._speccolors = NULL;

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutPoinmesh
 *
 * Purpose:     Accepts pointers to the coordinate arrays and writes the
 *              mesh into the database.
 *
 * Return:      Success:        object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 12:32:43 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutPointmesh(DBfile *dbfile, const char *name, int ndims, DBVCP2_t coords,
               int nels, int datatype, DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutPointmesh", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutPointmesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("pointmesh name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("pointmesh name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nels < 0)
            API_ERROR("nels<0", E_BADARGS);
        if (nels > 0)
        {
            int i;
            void **coords2 = (void**) coords;
            if (ndims < 1 || ndims > 3)
                API_ERROR("ndims < 1 || ndims > 3", E_BADARGS);
            for (i = 0; i < ndims && coords; i++)
                if (coords2[i] == 0) coords = 0;
            if (!coords)
                API_ERROR("coords=0 || coords[i]=0 for some i", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            API_ERROR("nels==0", E_EMPTYOBJECT);
        }

        if (!dbfile->pub.p_pm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_pm) (dbfile, name, ndims, coords, nels,
                                     datatype, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutPointvar
 *
 * Purpose:     Accepts pointers to the value arrays and writes the
 *              variables into a point-variable object in the database.
 *
 * Return:      Success:        object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 12:38:22 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutPointvar(DBfile *dbfile, const char *vname, const char *mname, int nvars,
              DBVCP2_t vars, int nels, int datatype, DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutPointvar", int, -1, vname) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutPointvar", E_GRABBED) ; 
        if (!vname || !*vname)
            API_ERROR("pointvar name", E_BADARGS);
        if (db_VariableNameValid(vname) == 0)
            API_ERROR("pointvar name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, vname))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nels < 0)
            API_ERROR("nels<0", E_BADARGS);
        if (nels)
        {
            int i;
            void **vars2 = (void**) vars;
            if (nvars <= 0)
                API_ERROR("nvars<=0", E_BADARGS);
            for (i = 0; i < nvars && vars; i++)
                if (!vars2[i]) vars = 0;
            if (!vars) API_ERROR("vars==0 || vars[i]==0", E_BADARGS);
            if (!mname || !*mname)
                API_ERROR("pointmesh name", E_BADARGS);
            if (db_VariableNameValid(mname) == 0)
                API_ERROR("pointmesh name", E_INVALIDNAME);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            API_ERROR("nels=0", E_EMPTYOBJECT);
        }

        if (!dbfile->pub.p_pv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_pv) (dbfile, vname, mname,
                                     nvars, vars, nels, datatype, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutPointvar1
 *
 * Purpose:     Same as DBPutPointvar except only one variable at a time.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 12:46:01 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutPointvar1(DBfile *dbfile, const char *vname, const char *mname,
               DBVCP1_t var, int nels, int datatype, DBoptlist const *optlist)
{
    DBVCP1_t vars[1] = {var};
    int retval;

    API_BEGIN2("DBPutPointvar1", int, -1, vname)
    {
        retval = DBPutPointvar(dbfile, vname, mname, 1, vars,
                               nels, datatype, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutQuadmesh
 *
 * Purpose:     Accepts pointers to the coordinate arrays and writes the
 *              mesh into a quad-mesh object in the database.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 12:50:40 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Mon Nov  2 17:51:55 PST 1998
 *    Removed the requirement for a non-NULL coordnames parameter.
 *
 *    Sean Ahern, Wed Mar 17 10:12:39 PST 1999
 *    Added a check for the coordtype.  It must be DB_COLLINEAR or
 *    DB_NONCOLLINEAR.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutQuadmesh(DBfile *dbfile, const char *name, char const * const *coordnames,
              DBVCP2_t coords, int const *dims, int ndims, int datatype,
              int coordtype, DBoptlist const *optlist)
{
    int i, retval, is_empty=1;

    API_BEGIN2("DBPutQuadmesh", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutQuadmesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("quadmesh name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("quadmesh name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if ((coordtype != DB_COLLINEAR) && (coordtype != DB_NONCOLLINEAR))
            API_ERROR("coordtype must be DB_COLLINEAR or DB_NONCOLLINEAR", E_BADARGS);
        if (ndims < 0)
            API_ERROR("ndims", E_BADARGS);
        if (!dims)
            API_ERROR("dims==0", E_BADARGS);
        for (i = 0; i < ndims; i++)
        {
            if (dims[i] != 0)
            {
                is_empty = 0;
                break;
            }
        }
        if (!is_empty)
        {
            void **coords2 = (void**) coords;
            for (i = 0; i < ndims && coords; i++)
                if (!coords2[i]) coords = 0;
            if (!coords)
                API_ERROR("coords==0 || coords[i]==0", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't think it was intentional */
            API_ERROR("dims[i]==0 for all i", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_qm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_qm) (dbfile, name, coordnames, coords,
                                     dims, ndims, datatype, coordtype,
                                     optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutQuadvar
 *
 * Purpose:     Writes a variable associated with a quad mesh into a
 *              database. Variables will be either node-centered or zone-
 *              centered.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 12:57:08 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Wed Nov 11 09:19:20 PST 2009
 *    Added check for valid centering.
 *
 *    Mark C. Miller, Thu Feb  4 11:29:35 PST 2010
 *    Removed upper bound restriction for nvars.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutQuadvar(DBfile *dbfile, const char *vname, const char *mname, int nvars,
             char const * const *varnames, DBVCP2_t vars, int const *dims, int ndims,
             DBVCP2_t mixvars, int mixlen, int datatype, int centering,
             DBoptlist const *optlist)
{
    int i, retval, is_empty = 1;

    API_BEGIN2("DBPutQuadvar", int, -1, vname) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutQuadvar", E_GRABBED) ; 
        if (!vname || !*vname)
            API_ERROR("quadvar name", E_BADARGS);
        if (db_VariableNameValid(vname) == 0)
            API_ERROR("quadvar name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, vname))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (!mname || !*mname)
            API_ERROR("quadmesh name", E_BADARGS);
        if (db_VariableNameValid(mname) == 0)
            API_ERROR("quadmesh name", E_INVALIDNAME);
        if (ndims < 0)
            API_ERROR("ndims", E_BADARGS);
        if (!dims)
            API_ERROR("dims==0", E_BADARGS);
        for (i = 0; i < ndims; i++)
        {
            if (dims[i] != 0)
            {
                is_empty = 0;
                break;
            }
        }
        if (!is_empty)
        {
            int i;
            void **vars2 = (void**) vars;
            for (i = 0; i < ndims && dims; i++)
                if (!dims[i]) dims = 0;
            if (!dims)
                API_ERROR("dims=0 || dims[i]=0", E_BADARGS);
            if (nvars < 1)
                API_ERROR("nvars<1", E_BADARGS);
            for (i = 0; i < nvars && vars; i++)
                if (!vars2[i]) vars = 0;
            vars2 = (void**) mixvars;
            for (i = 0; i < nvars && mixvars; i++)
                if (!vars2[i]) mixvars = 0;
            for (i = 0; i < nvars && varnames; i++)
                if (!varnames[i] && !*varnames[i]) varnames = 0;
            if (!vars)
                API_ERROR("vars=0 || vars[i]=0", E_BADARGS);
            if (!varnames)
                API_ERROR("varnames=0 || varnames[i]=0||\"\"", E_BADARGS);
            if (mixlen < 0)
                API_ERROR("mixlen", E_BADARGS);
            if (!mixvars && mixlen)
                API_ERROR("mixvars", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't think it was intentional */
            API_ERROR("ndims=0", E_EMPTYOBJECT);
        }
        if (centering != DB_NODECENT && centering != DB_ZONECENT &&
            centering != DB_FACECENT && centering != DB_BNDCENT &&
            centering != DB_EDGECENT && centering != DB_BLOCKCENT)
            API_ERROR("centering", E_BADARGS);
        if (!dbfile->pub.p_qv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);


        retval = (dbfile->pub.p_qv) (dbfile, vname, mname,
                                     nvars, varnames, vars, dims, ndims,
                                     mixvars, mixlen, datatype, centering,
                                     optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutQuadvar1
 *
 * Purpose:     Same as DBPutQuadvar except for scalar variables.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 13:07:45 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutQuadvar1(DBfile *dbfile, const char *vname, const char *mname, void const *var,
              int const *dims, int ndims, void const *mixvar, int mixlen, int datatype,
              int centering, DBoptlist const *optlist)
{
    char const *varnames[1] = {vname};
    void const *vars[1] = {var};
    void const *mixvars[1] = {mixvar};
    int retval;

    API_BEGIN2("DBPutQuadvar1", int, -1, vname) {

        retval = DBPutQuadvar(dbfile, vname, mname, 1,
                              varnames, vars, dims, ndims,
                              mixvars, mixlen,
                              datatype, centering, optlist);

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutUcdmesh
 *
 * Purpose:     Accepts pointers to the coordinate arrays and writes
 *              the mesh into a UCD-mesh object in the database.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 13:13:46 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Mon Nov  2 17:51:55 PST 1998
 *    Removed the requirement for a non-NULL coordnames parameter.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Thu Mar 26 12:40:20 PDT 2015
 *    Add logic to support Kerbel's funky empty ucd mesh with a single
 *    node and no zones.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutUcdmesh(DBfile *dbfile, const char *name, int ndims,
             char const * const *coordnames, DBVCP2_t coords, int nnodes,
             int nzones, const char *zonel_name, const char *facel_name,
             int datatype, DBoptlist const *optlist)
{
    int retval;
    char *zl_name;

    API_BEGIN2("DBPutUcdmesh", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutUcdmesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("UCDmesh name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("UCDmesh name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (ndims < 0)
            API_ERROR("ndims<0", E_BADARGS);
        if (nnodes < 0)
            API_ERROR("nnodes<0", E_BADARGS);
        if (nnodes)
        {
            int i;
            void **coords2 = (void**) coords;
            if (nnodes > 1 && nzones <= 0)
                API_ERROR("nzones<=0", E_BADARGS);
            for (i = 0; i < ndims && coords; i++)
                if (coords2[i] == 0) coords = 0;;
            if (!coords)
                API_ERROR("coords==0 || coords[i]==0", E_BADARGS);
            if ((zl_name = (char*)DBGetOption(optlist, DBOPT_PHZONELIST)))
            {
                if (!zl_name || !*zl_name)
                    API_ERROR("zonelist name specified with DBOPT_PHZONELIST is null or \"\"", E_BADARGS);
                if (db_VariableNameValid(zl_name) == 0)
                    API_ERROR("zonelist name specified with DBOPT_PHZONELIST", E_INVALIDNAME);
            }
            else if (zonel_name)
            {
                if (!*zonel_name)
                    API_ERROR("zonel_name==\"\"", E_BADARGS);
                if (db_VariableNameValid(zonel_name) == 0)
                    API_ERROR("zonel_name", E_INVALIDNAME);
            }
            else if (facel_name)
            {
                if (!*facel_name)
                    API_ERROR("facel_name==\"\"", E_BADARGS);
                if (db_VariableNameValid(facel_name) == 0)
                    API_ERROR("facel_name", E_INVALIDNAME);
            }
            else if (nzones > 0)
            {
                API_ERROR("no zonelist or facelist specified", E_BADARGS);
            }
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("ndims==0 || nnodes==0", E_EMPTYOBJECT);
        }

        if (!dbfile->pub.p_um)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_um) (dbfile, name, ndims, coordnames,
                                     coords, nnodes, nzones,
                                     zonel_name, facel_name,
                                     datatype, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutUcdsubmesh
 *
 * Purpose:     Accepts names of parent mesh with coordinate arrays and writes
 *              the mesh into a UCD-mesh object in the database.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  reus@ferret
 *              Wed Dec  9 15:17:00 PST 1998
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutUcdsubmesh(DBfile *dbfile, const char *name, const char *parentmesh,
                int nzones, const char *zonel_name, const char *facel_name,
                DBoptlist const *optlist)
{
    int retval;

    API_DEPRECATE2("DBPutUcdsubmesh", int, -1, name, 4, 6, "MRG Trees") {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutUcdsubmesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("mesh name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("mesh name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (!parentmesh || !*parentmesh)
            API_ERROR("parent mesh name", E_BADARGS);
        if (db_VariableNameValid(parentmesh) == 0)
            API_ERROR("parent mesh name", E_INVALIDNAME);
        if (nzones < 0)
            API_ERROR("nzones", E_BADARGS);
        if (!dbfile->pub.p_sm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);


        retval = (dbfile->pub.p_sm) (dbfile, name, parentmesh,
                                     nzones, zonel_name,
                                     facel_name, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutUcdvar
 *
 * Purpose:     Writes a variable associated with a UCD mesh into the
 *              database.  Note that variables will be either node-centered
 *              or zone-centered.  A UCD-var object contains the variable
 *              values, plus the object ID of the associated UCD mesh.  Other
 *              information can also be included.  This function is useful
 *              for writing vector and tensor fields, wereas the companion
 *              function, DBPutUcdvar1(), is appropriate for writing
 *              scalar fields.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Wed Nov  9 12:02:56 EST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *
 *    Mark C. Miller, Wed Nov 11 09:19:20 PST 2009
 *    Added check for valid centering.
 *
 *    Mark C. Miller, Thu Feb  4 11:28:55 PST 2010
 *    Removed upper bound restriction on nvars.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutUcdvar(DBfile *dbfile, const char *vname, const char *mname, int nvars,
            char const * const *varnames, DBVCP2_t vars, int nels, DBVCP2_t mixvars,
            int mixlen, int datatype, int centering, DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutUcdvar", int, -1, vname) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutUcdvar", E_GRABBED) ; 
        if (!vname || !*vname)
            API_ERROR("UCDvar name", E_BADARGS);
        if (db_VariableNameValid(vname) == 0)
            API_ERROR("UCDvar name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, vname))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (!mname || !*mname)
            API_ERROR("UCDmesh name", E_BADARGS);
        if (db_VariableNameValid(mname) == 0)
            API_ERROR("UCDmesh name", E_INVALIDNAME);
        if (nels < 0)
            API_ERROR("nels<0", E_BADARGS);
        if (nels)
        {
            int i;
            void **vars2 = (void**) vars;
            if (nvars <= 0)
                API_ERROR("nvars<0", E_BADARGS);
            for (i = 0; i < nvars && vars; i++)
                if (vars2[i] == 0) vars = 0;
            for (i = 0; i < nvars && varnames; i++)
                if (!varnames[i] && !*varnames[i]) varnames = 0;
            if (mixlen < 0)
                API_ERROR("mixlen", E_BADARGS);
            if (mixvars)
            {
                vars2 = (void**) mixvars;
                for (i = 0; i < nvars && mixvars; i++)
                    if (vars2[i] == 0) mixvars = 0;
            }
            if (!vars)
                API_ERROR("vars=0 || vars[i]=0", E_BADARGS);
            if (!varnames)
                API_ERROR("varnames=0 || varnames[i]=0", E_BADARGS);
            if (mixlen && !mixvars)
                API_ERROR("mixvars=0 || mixvars[i]=0", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nvars=0 || nels==0", E_EMPTYOBJECT);
        }
        if (centering != DB_NODECENT && centering != DB_ZONECENT &&
            centering != DB_FACECENT && centering != DB_BNDCENT &&
            centering != DB_EDGECENT && centering != DB_BLOCKCENT)
            API_ERROR("centering", E_BADARGS);
        if (!dbfile->pub.p_uv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_uv) (dbfile, vname, mname,
                                     nvars, varnames, vars, nels, mixvars,
                                     mixlen, datatype, centering, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutUcdvar1
 *
 * Purpose:     Same as DBPutUcdvar() except for scalar variables.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Wed Nov  9 12:14:28 EST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutUcdvar1(DBfile *dbfile, const char *vname, const char *mname, void const *var,
             int nels, void const *mixvar, int mixlen, int datatype, int centering,
             DBoptlist const *optlist)
{
    void const *vars[1] = {var};
    void const *mixvars[1] = {mixvar};
    char const *varnames[1] = {vname};
    int            retval;

    API_BEGIN2("DBPutUcdvar1", int, -1, vname)
    {

        retval = DBPutUcdvar(dbfile, vname, mname, 1, varnames, vars,
                     nels, mixvars, mixlen, datatype, centering, optlist);

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutZonelist
 *
 * Purpose:     Writes a zonelist object into the open database.  The name
 *              assigned to this object can in turn be used as the
 *              `zonel_name' parameter to the DBPutUcdmesh() function.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Wed Nov  9 12:20:22 EST 1994
 *
 * Modifications:
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutZonelist(DBfile *dbfile, const char *name, int nzones, int ndims,
              int const *nodelist, int lnodelist, int origin, int const *shapesize,
              int const *shapecnt, int nshapes)
{
    int retval;

    API_DEPRECATE2("DBPutZonelist", int, -1, name, 4, 6, "DBPutZonelist2()") {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutZonelist", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("zonelist name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("zonelist name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nzones < 0)
            API_ERROR("nzones", E_BADARGS);
        if (nzones)
        {
            if (ndims <= 0)
                API_ERROR("ndims<=0", E_BADARGS);
            if (lnodelist <= 0)
                API_ERROR("lnodelist<=", E_BADARGS);
            if (!nodelist)
                API_ERROR("nodelist=0", E_BADARGS);
            if (0 != origin && 1 != origin)
                API_ERROR("origin!=0||1", E_BADARGS);
            if (nshapes <= 0)
                API_ERROR("nshapes<=0", E_BADARGS);
            if (!shapesize)
                API_ERROR("shapesize=0", E_BADARGS);
            if (!shapecnt)
                API_ERROR("shapecnt=0", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nzones=0", E_EMPTYOBJECT);
        }
        else
        {
            lnodelist = 0;
            nshapes = 0;
        }
        if (!dbfile->pub.p_zl)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_zl) (dbfile, name, nzones, ndims,
                                     nodelist, lnodelist, origin, shapesize,
                                     shapecnt, nshapes);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutZonelist2
 *
 * Purpose:     Writes a zonelist object into the open database.  The name
 *              assigned to this object can in turn be used as the
 *              `zonel_name' parameter to the DBPutUcdmesh() function.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  brugger@kickit
 *              Tue Mar 30 10:41:12 PST 1999
 *
 * Modifications:
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added an option list to the arguments and to the call to p_zl2().
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *
 *    Robb Matzke, 2000-05-23
 *    The old table of contents is discarded if the directory changes.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutZonelist2(DBfile *dbfile, const char *name, int nzones, int ndims,
               int const *nodelist, int lnodelist, int origin, int lo_offset,
               int hi_offset, int const *shapetype, int const *shapesize, int const *shapecnt,
               int nshapes, DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutZonelist2", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutZonelist2", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("zonelist name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("zonelist name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nzones < 0)
            API_ERROR("nzones", E_BADARGS);
        if (nzones)
        {
            if (ndims <= 0)
                API_ERROR("ndims<=0", E_BADARGS);
            if (lnodelist <= 0)
                API_ERROR("lnodelist<=0", E_BADARGS);
            if (nshapes <= 0)
                API_ERROR("nshapes<=0", E_BADARGS);
            if (!nodelist)
                API_ERROR("nodelist=0", E_BADARGS);
            if (!shapetype)
                API_ERROR("shapetype=0", E_BADARGS);
            if (!shapesize)
                API_ERROR("shapesize=0", E_BADARGS);
            if (!shapecnt)
                API_ERROR("shapecnt=0", E_BADARGS);
            if (0 != origin && 1 != origin)
                API_ERROR("origin!=0||1", E_BADARGS);
            if (lo_offset < 0)
                API_ERROR("lo_offset<0", E_BADARGS);
            if (hi_offset < 0)
                API_ERROR("hi_offset<0", E_BADARGS);
#ifndef _MSC_VER
#warning HI AND LO OFFSET NOT VALID IN PRESENCE OF EXPLICIT GHOST LABELS
#endif
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nzones=0", E_EMPTYOBJECT);
        }
        else
        {
            lnodelist = 0;
            nshapes = 0;
        }
        if (!dbfile->pub.p_zl2)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        retval = (dbfile->pub.p_zl2) (dbfile, name, nzones, ndims,
                                      nodelist, lnodelist, origin, lo_offset,
                                      hi_offset, shapetype, shapesize,
                                      shapecnt, nshapes, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutPHZonelist 
 *
 * Purpose:     Writes a polyhedral zonelist object into the open database.
 *              The name assigned to this object can in turn be used as the
 *              parameter to a DBOPT_PHZONELIST option in the optlist passed to
 *              the DBPutUcdmesh() function.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller
 *              Tuesday, July 26, 2004 
 *
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutPHZonelist(DBfile *dbfile, const char *name,
    int nfaces, int const *nodecnt, int lnodelist, int const *nodelist,
    const char *extface, int nzones, int const *facecnt, int lfacelist,
    int const *facelist, int origin, int lo_offset, int hi_offset,
    DBoptlist const *optlist) 
{
    int retval;

    API_BEGIN2("DBPutPHZonelist", int, -1, name) {

        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutPHZonelist", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("zonelist name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("zonelist name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nfaces < 0)
            API_ERROR("nfaces<0", E_BADARGS);
        if (nfaces)
        {
            if (0 != origin && 1 != origin)
                API_ERROR("origin", E_BADARGS);
            if (!nodecnt)
                API_ERROR("nodecnt==0", E_BADARGS);
            if (!lnodelist)
                API_ERROR("lnodelist==0", E_BADARGS);
            if (!nodelist)
                API_ERROR("nodelist==0", E_BADARGS);
            if (nzones < 0)
                API_ERROR("nzones<0", E_BADARGS);
            if (nzones)
            {
                if ((lo_offset < 0) || (lo_offset >= nzones))
                    API_ERROR("lo_offset", E_BADARGS);
                if ((hi_offset < 0) || (hi_offset >= nzones))
                    API_ERROR("hi_offset", E_BADARGS);
                if (lo_offset > hi_offset)
                    API_ERROR("hi_offset", E_BADARGS);
                if (!facecnt)
                    API_ERROR("facecnt==0", E_BADARGS);
                if (!lfacelist)
                    API_ERROR("lfacelist==0", E_BADARGS);
                if (!facelist)
                    API_ERROR("facelist==0", E_BADARGS);
            }
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nfaces==0", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_phzl)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_phzl) (dbfile, name, nfaces, nodecnt,
                                       lnodelist, nodelist, extface,
                                       nzones, facecnt, lfacelist, facelist,
                                       origin, lo_offset, hi_offset,
                                       optlist);

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutCsgmesh
 *
 * Purpose:     Writes a CSG (Constructive Solid Geometry) mesh object to
 *              a silo database.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller
 *              Wed Jul 27 14:22:03 PDT 2005 
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutCsgmesh(DBfile *dbfile, const char *name, int ndims,
             int nbounds,
             const int *typeflags, const int *bndids/*optional*/,
             const void *coeffs, int lcoeffs, int datatype,
             const double *extents, const char *zonel_name, DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutCsgmesh", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutCsgmesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("CSGmesh name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("CSGmesh name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nbounds < 0)
            API_ERROR("nbounds<0", E_BADARGS);
        if (nbounds)
        {
            if (ndims < 0)
                API_ERROR("ndims<0", E_BADARGS);
            if (lcoeffs < 0)
                API_ERROR("lcoeffs<0", E_BADARGS);
            if (!(ndims == 2 || ndims == 3))
                API_ERROR("ndims must be either 2 or 3", E_BADARGS);
            if (!typeflags) API_ERROR("typeflags==0", E_BADARGS);
            if (!coeffs) API_ERROR("coeffs==0", E_BADARGS);
            if (!extents) API_ERROR("extents==0", E_BADARGS);
            if (!zonel_name || !*zonel_name) API_ERROR("zonel_name", E_BADARGS);
            if (db_VariableNameValid(zonel_name) == 0)
                API_ERROR("zonelist name", E_INVALIDNAME);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            API_ERROR("nbounds==0 || ndims==0 || lcoeffs==0", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_csgm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_csgm) (dbfile, name, ndims,
                                     nbounds, typeflags, bndids, coeffs,
                                     lcoeffs, datatype, extents, zonel_name,
                                     optlist);

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetCsgmesh
 *
 * Purpose:     Allocates a DBcsgmesh data structure and reads a CSG mesh
 *              from the data file.
 *
 * Return:      Success:        Pointer to the new DBcsgmesh struct
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller 
 *              Wed Jul 27 14:22:03 PDT 2005 
 *
 *-------------------------------------------------------------------------*/
PUBLIC DBcsgmesh *
DBGetCsgmesh(DBfile *dbfile, const char *name)
{
    DBcsgmesh     *csgm = NULL;

    API_BEGIN2("DBGetCsgmesh", DBcsgmesh *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetCsgmesh", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("CSGmesh name", E_BADARGS);
        if (!dbfile->pub.g_csgm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        csgm = ((dbfile->pub.g_csgm) (dbfile, name));
        if (!csgm)
        {
            API_RETURN(NULL);
        }
        API_RETURN(csgm);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutCSGZonelist
 *
 * Purpose:     Writes a CSG zonelist object into the open database.
 *              The name assigned to this object can in turn be used as the
 *              `zonel_name' parameter to the DBPutCsgmesh() function.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller 
 *              Wed Jul 27 14:22:03 PDT 2005
 *-------------------------------------------------------------------------*/

PUBLIC int
DBPutCSGZonelist(DBfile *dbfile, const char *name, int nregs,
                 const int *typeflags,
                 const int *leftids, const int *rightids,
                 const void *xforms, int lxforms, int datatype,
                 int nzones, const int *zonelist, DBoptlist const *optlist)
{
    int retval;

    API_BEGIN2("DBPutCSGZonelist", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutCSGZonelist", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("zonelist name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("zonelist name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nzones)
        {
            if (nregs < 0)
                API_ERROR("nregs", E_BADARGS);
            if (nzones < 0)
                API_ERROR("nzones", E_BADARGS);
            if (!typeflags)
                API_ERROR("typeflags", E_BADARGS);
            if (!leftids)
                API_ERROR("leftids", E_BADARGS);
            if (!rightids)
                API_ERROR("rightids", E_BADARGS);
            if (!zonelist)
                API_ERROR("zonelist", E_BADARGS);
            if ((xforms && lxforms <= 0) || (!xforms && lxforms > 0))
                API_ERROR("xforms and lxforms", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            /* this is an empty object but we don't know if it was intentional */
            API_ERROR("nregs==0 || nzones==0", E_EMPTYOBJECT);
        }

        if (!dbfile->pub.p_csgzl)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_csgzl) (dbfile, name, nregs,
                                        typeflags, leftids, rightids,
                                        xforms, lxforms, datatype, 
                                        nzones, zonelist, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetCSGZonelist
 *
 * Purpose:     Allocate and read a DBcsgzonelist structure.
 *
 * Return:      Success:        ptr to new DBcsgzonelist.
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller 
 *              Wed Jul 27 14:22:03 PDT 2005
 *
 *-------------------------------------------------------------------------*/
PUBLIC DBcsgzonelist*
DBGetCSGZonelist(DBfile *dbfile, const char *name)
{
    DBcsgzonelist * retval = NULL;

    API_BEGIN2("DBGetCSGZonelist", DBcsgzonelist *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetCSGZonelist", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("CSG zonelist name", E_BADARGS);
        if (!dbfile->pub.g_csgzl)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_csgzl) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPutCsgvar
 *
 * Purpose:     Writes a variable associated with a CSG mesh into the
 *              database.  Note that variables will be either
 *              boundary-centered or zone-centered. A CSG-var object
 *              contains the variable values, plus the object ID of the
 *              associated CSG mesh.  Other information can also be included.
 *
 * Return:      Success:        Object ID
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller 
 *              Wed Jul 27 14:22:03 PDT 2005 
 *
 *-------------------------------------------------------------------------*/
PUBLIC int
DBPutCsgvar(DBfile *dbfile, const char *vname, const char *meshname,
            int nvars, char const * const *varnames, DBVCP2_t _vars,
            int nvals, int datatype, int centering, DBoptlist const *optlist)
{
    int retval;
    void const * const *vars = (void const * const *) _vars;

    API_BEGIN2("DBPutCsgvar", int, -1, vname) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutCsgvar", E_GRABBED) ; 
        if (!vname || !*vname)
            API_ERROR("CSGvar name", E_BADARGS);
        if (db_VariableNameValid(vname) == 0)
            API_ERROR("CSGvar name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, vname))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nvals < 0)
            API_ERROR("nvals<0", E_BADARGS);
        if (nvals)
        {
            int i;
            if (!meshname || !*meshname)
                API_ERROR("CSGmesh name", E_BADARGS);
            if (nvars <= 0)
                API_ERROR("nvars<0", E_BADARGS);
            if (db_VariableNameValid(meshname) == 0)
                API_ERROR("CSGmesh name", E_INVALIDNAME);
            for (i = 0; i < nvars && vars; i++)
                if (!vars[i]) vars = 0;
            for (i = 0; i < nvars && varnames; i++)
                if (!varnames[i] && !*varnames[i]) varnames = 0;
            if (!vars) API_ERROR("vars==0 || vars[i]==0", E_BADARGS);
            if (!varnames) API_ERROR("varnames==0 || varnames[i]==0", E_BADARGS);
            if (!(centering == DB_ZONECENT || centering == DB_BNDCENT)) 
                API_ERROR("centering", E_BADARGS);
        }
        else if (!DBGetAllowEmptyObjectsFile(dbfile))
        {
            API_ERROR("nvars=0 || nvals=0", E_EMPTYOBJECT);
        }
        if (!dbfile->pub.p_csgv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_csgv) (dbfile, vname, meshname,
                                     nvars, varnames, vars, nvals,
                                     datatype, centering, optlist);
        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBGetCsgvar
 *
 * Purpose:     Allocates a DBcsgvar structure and reads a variable associated
 *              with a CSG mesh from the database.
 *
 * Return:      Success:        Pointer to the new DBucdvar struct
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 11:04:35 PST 1994
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *-------------------------------------------------------------------------*/
PUBLIC DBcsgvar *
DBGetCsgvar(DBfile *dbfile, const char *name)
{
    DBcsgvar * retval = NULL;

    API_BEGIN2("DBGetCsgvar", DBcsgvar *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetCsgvar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("CSGvar name", E_BADARGS);
        if (!dbfile->pub.g_csgv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_csgv) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}


/*----------------------------------------------------------------------
 *  Routine                                                  _DBstrprint
 *
 *  Purpose
 *
 *      This function prints an array of strings in either column- or
 *      row-major order.
 *
 *  Programmer
 *
 *      Jeff Long
 *  Arguments:
 *      fp             File pointer for output of printing
 *      strs           Array of character strings to print
 *      nstrs          Number of character strings in 'strs'
 *      order          Printing order: 'c' for by-column, 'r' for by-row
 *      left_margin    Width of left margin (in chars)
 *      col_margin     Width of empty spaces between columns (in chars)
 *      line_width     Width of entire output line.
 *
 * Modified
 *    Robb Matzke, Wed Jan 11 06:41:23 PST 1995
 *    Changed name from strprint since that conflicted with MeshTV.
 *
 *    Eric Brugger, Tue Feb  7 09:06:58 PST 1995
 *    I modified the argument declaration to reflect argument promotions.
 *---------------------------------------------------------------------*/
INTERNAL int
_DBstrprint(FILE *fp, char *strs[], int nstrs, int order, int left_margin,
            int col_margin, int line_width)
{
    char         **sorted_strs = NULL;
    int            i, j, index;
    int            maxwidth;
    int            nrows, ncols;
    double         dtmp;
    char          *me = "_DBstrprint";

    if (nstrs <= 0)
        return db_perror("nstrs", E_BADARGS, me);
    if (left_margin < 0 || left_margin > line_width) {
        return db_perror("left margin", E_BADARGS, me);
    }

     /*----------------------------------------
      *  Sort strings into alphabetical order.
      *---------------------------------------*/
    sorted_strs = ALLOC_N(char *, nstrs);
    for (i = 0; i < nstrs; i++)
        sorted_strs[i] = strs[i];

    _DBsort_list(sorted_strs, nstrs);

     /*----------------------------------------
      *  Find maximum string width.
      *---------------------------------------*/
    maxwidth = strlen(sorted_strs[0]);

    for (i = 1; i < nstrs; i++) {
        maxwidth = MAX(maxwidth, (int)strlen(sorted_strs[i]));
    }

     /*----------------------------------------
      *  Determine number of columns and rows.
      *---------------------------------------*/
    ncols = (line_width - left_margin) / (maxwidth + col_margin);
    if (ncols <= 0) {
        FREE(sorted_strs);
        return (OOPS);
    }

    dtmp = (double)nstrs / (double)ncols;
    nrows = (int)ceil(dtmp);
    if (nrows <= 0) {
        FREE(sorted_strs);
        return -1;
    }

     /*----------------------------------------
      *  Print strings in requested order.
      *---------------------------------------*/

    if (order == 'c') {
        /*------------------------------
         *  Print by column
         *-----------------------------*/

        for (i = 0; i < nrows; i++) {
            index = i;

            fprintf(fp, "%*s", left_margin, " ");

            for (j = 0; j < ncols; j++) {

                fprintf(fp, "%-*s%*s", maxwidth, sorted_strs[index],
                        col_margin, " ");

                index += nrows;
                if (index >= nstrs)
                    break;
            }
            fprintf(fp, "\n");
        }

    }
    else {
        /*------------------------------
         *  Print by row
         *-----------------------------*/

        for (i = 0; i < nrows; i++) {
            index = i * ncols;

            fprintf(fp, "%*s", left_margin, " ");

            for (j = 0; j < ncols; j++) {

                fprintf(fp, "%-*s%*s", maxwidth, sorted_strs[index],
                        col_margin, " ");

                index++;
                if (index >= nstrs)
                    break;
            }
            fprintf(fp, "\n");
        }

    }

    FREE(sorted_strs);
    return 0;
}

static int
qsort_strcmp(const void *str1, const void *str2)
{
   return(strcmp(*((const char **)str1), *((const char **)str2)));
}

/*----------------------------------------------------------------------
 *  Function                                                _DBsort_list
 *
 *  Purpose
 *
 *      Sort a list of character strings. Algorithm taken from
 *      _SX_sort_lists() -- courtesy of Stewart Brown.
 *
 * Modified
 *    Robb Matzke, Wed Jan 11 06:39:16 PST 1995
 *    Changed name from sort_list because it conflicts with MeshTV.
 *
 *    Sean Ahern, Fri Mar  2 09:45:05 PST 2001
 *    Changed this sort to a qsort, as suggested by Dan Schikore.
 *---------------------------------------------------------------------*/
INTERNAL void
_DBsort_list(char **ss, int n)
{
    qsort(ss, n, sizeof(char *), qsort_strcmp);
}

/*---------------------------------------------------------------------------
 * arrminmax - Return the min and max value of the given float array.
 *
 * Modified
 *    Robb Matzke, Wed Jan 11 06:43:08 PST 1995
 *    Changed name from arrminmax since that conflicted with MeshTV.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *---------------------------------------------------------------------------*/
INTERNAL int
_DBarrminmax(float arr[], int len, float *arr_min, float *arr_max)
{
    int             i;
    char           *me = "_DBarrminmax";

    if (!arr)
        return db_perror("arr pointer", E_BADARGS, me);
    if (len <= 0)
        return db_perror("len", E_BADARGS, me);

    *arr_min = arr[0];
    *arr_max = arr[0];

    for (i = 1; i < len; i++)
    {
        *arr_min = MIN(*arr_min, arr[i]);
        *arr_max = MAX(*arr_max, arr[i]);
    }

    return 0;
}

/*---------------------------------------------------------------------------
 * iarrminmax - Return the min and max value of the given int array.
 *
 * Modified:
 *    Robb Matzke, Wed Jan 11 06:43:42 PST 1995
 *    Changed name from iarrminmax since that conflicted with MeshTV.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *---------------------------------------------------------------------------*/
INTERNAL int
_DBiarrminmax(int arr[], int len, int *arr_min, int *arr_max)
{
    int             i;
    char           *me = "_DBiarrminmax";

    if (!arr)
        return db_perror("arr pointer", E_BADARGS, me);
    if (len <= 0)
        return db_perror("len", E_BADARGS, me);

    *arr_min = arr[0];
    *arr_max = arr[0];

    for (i = 1; i < len; i++)
    {
        *arr_min = MIN(*arr_min, arr[i]);
        *arr_max = MAX(*arr_max, arr[i]);
    }

    return 0;
}

/*---------------------------------------------------------------------------
 * darrminmax - Return the min and max value of the given double array.
 *
 * Modified:
 *    Robb Matzke, Wed Jan 11 06:44:16 PST 1995
 *    Changed name from darrminmax since that conflicted with MeshTV.
 *
 *    Sean Ahern, Tue Sep 28 11:00:13 PDT 1999
 *    Made the error messages a little better.
 *---------------------------------------------------------------------------*/
INTERNAL int
_DBdarrminmax(double arr[], int len, double *arr_min, double *arr_max)
{
    int             i;
    char           *me = "_DBdarrminmax";

    if (!arr)
        return db_perror("arr pointer", E_BADARGS, me);
    if (len <= 0)
        return db_perror("len", E_BADARGS, me);

    *arr_min = arr[0];
    *arr_max = arr[0];

    for (i = 1; i < len; i++)
    {
        *arr_min = MIN(*arr_min, arr[i]);
        *arr_max = MAX(*arr_max, arr[i]);
    }

    return 0;
}

static int
include_point(int ptidx, int ndims, int const *dims,
    int const *minidx, int const *maxidx)
{
    if (dims == 0) return 1;

    int i = ndims>0 ? (ptidx)                   % dims[0] : 0;
    int j = ndims>1 ? (ptidx/dims[0])           % dims[1] : 0;
    int k = ndims>2 ? (ptidx/(dims[1]*dims[0])) % dims[2] : 0;

    if (ndims > 0 && i < minidx[0]) return 0;
    if (ndims > 0 && i > maxidx[0]) return 0;
    if (ndims > 1 && j < minidx[1]) return 0;
    if (ndims > 1 && j > maxidx[1]) return 0;
    if (ndims > 2 && k < minidx[2]) return 0;
    if (ndims > 2 && k > maxidx[2]) return 0;

    return 1;
}

/*----------------------------------------------------------------------
 * Routine                                               _CalcExtents
 *
 * Purpose: Return min/max of each dimension of coordinate array data
 *
 * Modifications:
 *      Sean Ahern, Wed Oct 21 10:55:21 PDT 1998
 *      Changed the function so that the min_extents and max_extents are 
 *      passed in as void* variables.
 *
 *      Mark C. Miller, Mon May 20 12:25:25 PDT 2024
 *      Adjusted to avoid strict pointer aliasing optimization issues.
 *--------------------------------------------------------------------*/
static int
_CalcExtents(DBVCP2_t coord_arrays, int datatype, int ndims, int npts,
             void *min_extents, void *max_extents,
             int const *dims, int const *minidx, int const *maxidx)
{
    int i, j, first;

    if (npts <= 0) return 0;

    if (datatype == DB_DOUBLE) {

        double *dmin_extents = (double *) min_extents;
        double *dmax_extents = (double *) max_extents;
        double const * const * _coord_arrays = (double const * const *) coord_arrays;
        double const *dcoord_arrays[3] = {
            ndims>0?_coord_arrays[0]:0,
            ndims>1?_coord_arrays[1]:0,
            ndims>2?_coord_arrays[2]:0};

        for (i = 0; i < ndims; i++)
        {
            for (j = 0, first = 1; j < npts; j++)
            {
                if (include_point(j, ndims, dims, minidx, maxidx))
                {
                    if (first)
                    {
                        first = 0;
                        dmin_extents[i] = dcoord_arrays[i][j];
                        dmax_extents[i] = dcoord_arrays[i][j];
                    }
                    else
                    {
                        dmin_extents[i] = MIN(dmin_extents[i], dcoord_arrays[i][j]);
                        dmax_extents[i] = MAX(dmax_extents[i], dcoord_arrays[i][j]);
                    }
                }
            }
        }
    }
    else
    {
        float *fmin_extents = (float *) min_extents;
        float *fmax_extents = (float *) max_extents;
        float  const * const * _coord_arrays = (float const * const *) coord_arrays;
        float const *fcoord_arrays[3] = {
            ndims>0?_coord_arrays[0]:0,
            ndims>1?_coord_arrays[1]:0,
            ndims>2?_coord_arrays[2]:0};

        for (i = 0; i < ndims; i++)
        {
            for (j = 0, first = 1; j < npts; j++)
            {
                if (include_point(j, ndims, dims, minidx, maxidx))
                {
                    if (first)
                    {
                        first = 0;
                        fmin_extents[i] = fcoord_arrays[i][j];
                        fmax_extents[i] = fcoord_arrays[i][j];
                    }
                    else
                    {
                        fmin_extents[i] = MIN(fmin_extents[i], fcoord_arrays[i][j]);
                        fmax_extents[i] = MAX(fmax_extents[i], fcoord_arrays[i][j]);
                    }
                }
            }
        }
    }

    return 0;
}

/*----------------------------------------------------------------------
 *  Routine                                               _DBQMCalcExtents
 *
 *  Purpose
 *
 *      Return the extents of the given quad mesh.
 *
 *      Works for 1D, 2D and 3D meshes, collinear or non-collinear.
 *
 *  Modification History
 *    Jeff Long, 11/16/92
 *    Modified handling of double precision coords so that extents
 *    are returned as floats.
 *
 *    Robb Matzke, Wed Jan 11 06:34:09 PST 1995
 *    Changed name from QM_CalcExtents because it conflicts with MeshTV.
 *
 *    Eric Brugger, Wed Feb 15 08:12:43 PST 1995
 *    I removed the error message that precision had been lost.
 *    Their is no loss of precision because all the casts from
 *    double to float were done on pointers.  Casting of a pointer
 *    just makes the compiler happy and has no impact on the value
 *    pointed to.
 *
 *    Sean Ahern, Mon Oct 19 18:17:10 PDT 1998
 *    Added the ability to have the extents returned either as float or
 *    double.
 *--------------------------------------------------------------------*/
INTERNAL int
_DBQMCalcExtents(DBVCP2_t coord_arrays, int datatype, int const *min_index,
                 int const *max_index, int const *dims, int ndims, int coordtype,
                 void *min_extents, void *max_extents)
{
    int i, npts;

    npts = 1;
    for (i = 0; i < ndims; i++)
        npts *= dims[i];

    if (npts <= 0) return 0;

    if (coordtype == DB_COLLINEAR)
    {
        if (datatype == DB_DOUBLE)
        {
            double *dmin_extents = (double *) min_extents;
            double *dmax_extents = (double *) max_extents;
            double const * const * _coord_arrays = (double const * const *) coord_arrays;
            double const *dcoord_arrays[3] = {
                ndims>0?_coord_arrays[0]:0,
                ndims>1?_coord_arrays[1]:0,
                ndims>2?_coord_arrays[2]:0};
            for (i = 0; i < ndims; i++)
                _CalcExtents(&dcoord_arrays[i], datatype, 1, dims[i], &dmin_extents[i], &dmax_extents[i],
                    &dims[i], &min_index[i], &max_index[i]);
        }
        else
        {
            float *fmin_extents = (float *) min_extents;
            float *fmax_extents = (float *) max_extents;
            float const * const * _coord_arrays = (float const * const *) coord_arrays;
            float const *fcoord_arrays[3] = {
                ndims>0?_coord_arrays[0]:0,
                ndims>1?_coord_arrays[1]:0,
                ndims>2?_coord_arrays[2]:0};
            for (i = 0; i < ndims; i++)
                _CalcExtents(&fcoord_arrays[i], datatype, 1, dims[i], &fmin_extents[i], &fmax_extents[i],
                    &dims[i], &min_index[i], &max_index[i]);
        }
    }
    else
    {
        _CalcExtents(coord_arrays, datatype, ndims, npts, min_extents, max_extents,
            dims, min_index, max_index);
    }

    return 0;
}

INTERNAL int
UM_CalcExtents(DBVCP2_t coord_arrays, int datatype, int ndims, int npts,
               void *min_extents, void *max_extents)
{
    return _CalcExtents(coord_arrays, datatype, ndims, npts, min_extents, max_extents,0,0,0);
}

/*----------------------------------------------------------------------
 * Routine                                               CSGM_CalcExtents
 *
 * Purpose
 *
 *      Return the extents of the given csg mesh.
 *
 *--------------------------------------------------------------------*/
INTERNAL int
CSGM_CalcExtents(int datatype, int ndims, int nbounds,
               const int *typeflags, const void *coeffs,
               double *min_extents, double *max_extents)
{
    min_extents[0] = -DBL_MAX;
    min_extents[1] = -DBL_MAX;
    min_extents[2] = -DBL_MAX;
    max_extents[0] = DBL_MAX;
    max_extents[1] = DBL_MAX;
    max_extents[2] = DBL_MAX;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_ProcessOptlist
 *
 * Purpose:     Process the options list for an object and initializes the
 *              object's global data.  Each object type has its own global data
 *              so that `cycle' for one type of object is different from
 *              `cycle' for another type of object.  This results form trying
 *              to stay compatible with the previous version, where each
 *              object had its own source file with global variables
 *              declared `static'.
 *
 *              Some objects share the same global data.  They are:
 *                      DB_MULTIMESH    and DB_MULTIVAR (_mm)
 *                      DB_POINTMESH    and DB_POINTVAR (_pm)
 *                      DB_QUADMESH     and DB_QUADVAR  (_qm)
 *                      DB_UCDMESH      and DB_UCDVAR   (_um)
 *
 *
 * Return:      Success:        0, no options or all options processed.
 *
 *              Failure:        >0, number of unrecognized options.
 *                              -1, bad objtype
 *
 * Programmer:  matzke@viper
 *              Wed Dec 14 13:36:04 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Fri Jan 12 18:36:56 PST 1996
 *    I added the case for DB_MULTIMESH.
 *
 *    Robb Matzke, 18 Jun 1997
 *    Added DB_ASCII_LABEL for DB_QUADMESH and DB_QUADVAR.
 *
 *    Eric Brugger, Wed Oct 15 15:37:22 PDT 1997
 *    I added DBOPT_HI_OFFSET and DBOPT_LO_OFFSET to DB_UCDVAR.
 *
 *    Eric Brugger, Thu Oct 16 10:31:26 PDT 1997
 *    I added DBOPT_MATNOS and DBOPT_NMATNOS to DB_MULTIMAT (which
 *    is covered by the DB_MULTIMESH case).
 *
 *    Jeremy Meredith, Sept 18 1998
 *    Added options DBOPT_MATNAME, DBOPT_NMAT, and DBOPT_NMATSPEC
 *    to DB_MULTIMATSPECIES (covered by DB_MULTIMESH case).
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added DBOPT_GROUPNUM to the point, quad, and ucd meshes.
 *    Added DBOPT_BASEINDEX to the quad mesh; set it from the origin if needed.
 *    Added DBOPT_NODENUM to the ucd mesh.
 *    Added a DB_ZONELIST type.
 *    Added DBOPT_ZONENUM to the ucd zonelist.
 *    Added DBOPT_BLOCKORIGIN, _GROUPORIGIN, and _NGROUPS to the multimesh.
 *
 *    Jeremy Meredith, Wed Jul  7 12:15:31 PDT 1999
 *    I removed the DBOPT_ORIGIN from the species object.
 *
 *    Sean Ahern, Tue Feb  5 10:22:25 PST 2002
 *    Added names for materials.
 *
 *    Brad Whitlock, Wed Jan 18 15:36:55 PST 2006
 *    Added ascii_labels for ucdvars.
 *
 *    Thomas R. Treadway, Wed Jun 28 10:31:45 PDT 2006
 *    Added topo_dim to ucdmesh.
 *
 *    Thomas R. Treadway, Thu Jul  6 17:05:24 PDT 2006
 *    Added reference to curve options.
 *
 *    Thomas R. Treadway, Thu Jul 20 11:06:27 PDT 2006
 *    Added lgroupings, groupings, and groupnames to multimesh options.
 *
 *    Mark C. Miller, Mon Aug  7 17:03:51 PDT 2006
 *    Added DBOPT_MATCOLORS, DBOPT_MATNAMES options to multimesh
 *
 *    Thomas R. Treadway, Tue Aug 15 14:05:59 PDT 2006
 *    Added DBOPT_ALLOWMAT0
 *
 *    Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *    Added names and colors for species.
 *
 *    Mark C. Miller, Wed Sep 23 11:49:34 PDT 2009
 *    Added DBOPT_LLONGNZNUM for long long global node/zone numbers
 *    to pointmeshes, ucdmeshes, zonelists.
 *
 *    Mark C. Miller, Thu Nov  5 16:14:12 PST 2009
 *    Added conserved/extensive options to all var objects.
 *
 *    Mark C. Miller, Fri Nov 13 15:33:02 PST 2009
 *    Add DBOPT_LLONGNZNUM to polyhedral zonelist object.
 *
 *    Mark C. Miller, Wed Jul 14 20:36:23 PDT 2010
 *    Added support for nameschemes options on multi-block objects.
 *-------------------------------------------------------------------------*/
INTERNAL int
db_ProcessOptlist(int objtype, DBoptlist const * const optlist)
{
    int             i, j, *ip = NULL, unused = 0;
    char           *me = "db_ProcessOptlist";

    if (!optlist || optlist->numopts < 0)
        return 0;

    for (i = 0; i < optlist->numopts; i++)
    {
        if (optlist->options[i] >= DBOPT_FIRST &&
            optlist->options[i] <= DBOPT_LAST)
            continue;
        return db_perror(NULL, E_BADOPTCLASS, me);
    }

    switch (objtype)
    {
        case DB_CSGMESH:
        case DB_CSGVAR:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_TIME:
                        _csgm._time = DEREF(float, optlist->values[i]);
                        _csgm._time_set = TRUE;
                        break;

                    case DBOPT_DTIME:
                        _csgm._dtime = DEREF(double, optlist->values[i]);
                        _csgm._dtime_set = TRUE;
                        break;

                    case DBOPT_CYCLE:
                        _csgm._cycle = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_LABEL:
                        _csgm._label = (char *)optlist->values[i];
                        break;

                    case DBOPT_XLABEL:
                        _csgm._labels[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YLABEL:
                        _csgm._labels[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_ZLABEL:
                        _csgm._labels[2] = (char *)optlist->values[i];
                        break;

                    case DBOPT_UNITS:
                        _csgm._unit = (char *)optlist->values[i];
                        break;

                    case DBOPT_XUNITS:
                        _csgm._units[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YUNITS:
                        _csgm._units[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_ZUNITS:
                        _csgm._units[2] = (char *)optlist->values[i];
                        break;

                    case DBOPT_USESPECMF:
                        _csgm._use_specmf = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_GROUPNUM:
                        DEPRECATE_MSG("DBOPT_GROUPNUM",4,6,"MRG Trees")
                        _csgm._group_no = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_ORIGIN:
                        _csgm._origin = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_BNDNAMES:
                        _csgm._bndnames = (char **)optlist->values[i];
                        break;

                    case DBOPT_HIDE_FROM_GUI:
                        _csgm._guihide = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MRGTREE_NAME:
                        _csgm._mrgtree_name = (char *)optlist->values[i];
                        break;

                    case DBOPT_REGION_PNAMES:
                        _csgm._region_pnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_TV_CONNECTIVITY:
                        _csgm._tv_connectivity = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_DISJOINT_MODE:
                        _csgm._disjoint_mode = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_CONSERVED:
                        _csgm._conserved = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_EXTENSIVE:
                        _csgm._extensive = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MISSING_VALUE:
                        _csgm._missing_value = DEREF(double, optlist->values[i]);
                        break;

                    case DBOPT_ALT_NODENUM_VARS:
                        _csgm._alt_nodenum_vars = (char **) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_MATERIAL:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_MAJORORDER:
                        _ma._majororder = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_ORIGIN:
                        _ma._origin = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MATNAMES:
                        _ma._matnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_MATCOLORS:
                        _ma._matcolors = (char **) optlist->values[i];
                        break;

                    case DBOPT_ALLOWMAT0:
                        _ma._allowmat0 = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_HIDE_FROM_GUI:
                        _ma._guihide = DEREF(int, optlist->values[i]);
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_MATSPECIES:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_MAJORORDER:
                        _ms._majororder = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_HIDE_FROM_GUI:
                        _ms._guihide = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_SPECNAMES:
                        _ms._specnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_SPECCOLORS:
                        _ms._speccolors = (char **) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_POINTMESH:
        case DB_POINTVAR:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_TIME:
                        _pm._time = DEREF(float, optlist->values[i]);
                        _pm._time_set = 1;
                        break;

                    case DBOPT_DTIME:
                        _pm._dtime = DEREF(double, optlist->values[i]);
                        _pm._dtime_set = 1;
                        break;

                    case DBOPT_CYCLE:
                        _pm._cycle = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_NSPACE:
                        _pm._nspace = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_ORIGIN:
                        _pm._origin = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_HI_OFFSET:
                        _pm._hi_offset = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_LO_OFFSET:
                        _pm._lo_offset = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_LABEL:
                        _pm._label = (char *)optlist->values[i];
                        break;

                    case DBOPT_XLABEL:
                        _pm._labels[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YLABEL:
                        _pm._labels[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_ZLABEL:
                        _pm._labels[2] = (char *)optlist->values[i];
                        break;

                    case DBOPT_UNITS:
                        _pm._unit = (char *)optlist->values[i];
                        break;

                    case DBOPT_XUNITS:
                        _pm._units[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YUNITS:
                        _pm._units[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_ZUNITS:
                        _pm._units[2] = (char *)optlist->values[i];
                        break;

                    case DBOPT_GROUPNUM:
                        DEPRECATE_MSG("DBOPT_GROUPNUM",4,6,"MRG Trees")
                        _pm._group_no = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_HIDE_FROM_GUI:
                        _pm._guihide = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_ASCII_LABEL:
                        _pm._ascii_labels = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_NODENUM:
                        _pm._gnodeno = (int*)optlist->values[i];
                        break;

                    case DBOPT_MRGTREE_NAME:
                        _pm._mrgtree_name = (char *)optlist->values[i];
                        break;

                    case DBOPT_REGION_PNAMES:
                        _pm._region_pnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_LLONGNZNUM:
                        _pm._llong_gnodeno = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_CONSERVED:
                        _pm._conserved = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_EXTENSIVE:
                        _pm._extensive = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MISSING_VALUE:
                        _pm._missing_value = DEREF(double, optlist->values[i]);
                        break;

                    case DBOPT_GHOST_NODE_LABELS:
                        _pm._ghost_node_labels = (char *)optlist->values[i];
                        break;

                    case DBOPT_ALT_NODENUM_VARS:
                        _pm._alt_nodenum_vars = (char **) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_QUADMESH:
        case DB_QUADVAR:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_TIME:
                        _qm._time = DEREF(float, optlist->values[i]);
                        _qm._time_set = TRUE;
                        break;

                    case DBOPT_DTIME:
                        _qm._dtime = DEREF(double, optlist->values[i]);
                        _qm._dtime_set = TRUE;
                        break;

                    case DBOPT_CYCLE:
                        _qm._cycle = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_COORDSYS:
                        _qm._coord_sys = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_FACETYPE:
                        _qm._facetype = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MAJORORDER:
                        _qm._majororder = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_NSPACE:
                        _qm._nspace = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_ORIGIN:
                        _qm._origin = DEREF(int, optlist->values[i]);
                        if (! _qm._baseindex_set)
                        {
                            for (j = 0; j < _qm._ndims; j++)
                                _qm._baseindex[j] = _qm._origin;
                        }
                        break;

                    case DBOPT_PLANAR:
                        _qm._planar = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_HI_OFFSET:
                        ip = (int *)optlist->values[i];
                        for (j = 0; j < _qm._ndims; j++)
                            _qm._hi_offset[j] = ip[j];
                        break;

                    case DBOPT_LO_OFFSET:
                        ip = (int *)optlist->values[i];
                        for (j = 0; j < _qm._ndims; j++)
                            _qm._lo_offset[j] = ip[j];
                        break;

                    case DBOPT_LABEL:
                        _qm._label = (char *)optlist->values[i];
                        break;

                    case DBOPT_XLABEL:
                        _qm._labels[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YLABEL:
                        _qm._labels[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_ZLABEL:
                        _qm._labels[2] = (char *)optlist->values[i];
                        break;

                    case DBOPT_UNITS:
                        _qm._unit = (char *)optlist->values[i];
                        break;

                    case DBOPT_XUNITS:
                        _qm._units[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YUNITS:
                        _qm._units[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_ZUNITS:
                        _qm._units[2] = (char *)optlist->values[i];
                        break;

                    case DBOPT_USESPECMF:
                        _qm._use_specmf = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_ASCII_LABEL:
                        _qm._ascii_labels = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_GROUPNUM:
                        DEPRECATE_MSG("DBOPT_GROUPNUM",4,6,"MRG Trees")
                        _qm._group_no = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_BASEINDEX:
                        ip = (int *)optlist->values[i];
                        for (j = 0; j < _qm._ndims; j++)
                            _qm._baseindex[j] = ip[j];
                        _qm._baseindex_set = TRUE;
                        break;                        

                    case DBOPT_HIDE_FROM_GUI:
                        _qm._guihide = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MRGTREE_NAME:
                        _qm._mrgtree_name = (char *)optlist->values[i];
                        break;

                    case DBOPT_REGION_PNAMES:
                        _qm._region_pnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_CONSERVED:
                        _qm._conserved = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_EXTENSIVE:
                        _qm._extensive = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MISSING_VALUE:
                        _qm._missing_value = DEREF(double, optlist->values[i]);
                        break;

                    case DBOPT_GHOST_NODE_LABELS:
                        _qm._ghost_node_labels = (char *)optlist->values[i];
                        break;

                    case DBOPT_GHOST_ZONE_LABELS:
                        _qm._ghost_zone_labels = (char *)optlist->values[i];
                        break;

                    case DBOPT_ALT_NODENUM_VARS:
                        _qm._alt_nodenum_vars = (char **) optlist->values[i];
                        break;

                    case DBOPT_ALT_ZONENUM_VARS:
                        _qm._alt_zonenum_vars = (char **) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_UCDMESH:
        case DB_UCDVAR:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_TIME:
                        _um._time = DEREF(float, optlist->values[i]);
                        _um._time_set = TRUE;
                        break;

                    case DBOPT_DTIME:
                        _um._dtime = DEREF(double, optlist->values[i]);
                        _um._dtime_set = TRUE;
                        break;

                    case DBOPT_CYCLE:
                        _um._cycle = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_COORDSYS:
                        _um._coord_sys = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_TOPO_DIM:
                        /* The value of '_topo_dim' member is designed such
                           that a value of zero (which can be a valid topological
                           dimension specified by a caller) represents the
                           NOT SET value. So, we always add 1 to whatever the
                           caller gives us. */
                        _um._topo_dim = DEREF(int, optlist->values[i])+1;
                        break;

                    case DBOPT_FACETYPE:
                        _um._facetype = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_ORIGIN:
                        _um._origin = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_PLANAR:
                        _um._planar = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_LABEL:
                        _um._label = (char *)optlist->values[i];
                        break;

                    case DBOPT_XLABEL:
                        _um._labels[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YLABEL:
                        _um._labels[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_ZLABEL:
                        _um._labels[2] = (char *)optlist->values[i];
                        break;

                    case DBOPT_UNITS:
                        _um._unit = (char *)optlist->values[i];
                        break;

                    case DBOPT_XUNITS:
                        _um._units[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YUNITS:
                        _um._units[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_ZUNITS:
                        _um._units[2] = (char *)optlist->values[i];
                        break;

                    case DBOPT_USESPECMF:
                        _um._use_specmf = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_ASCII_LABEL:
                        _um._ascii_labels = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_HI_OFFSET:
                        _um._hi_offset = DEREF(int, optlist->values[i]);
                        _um._hi_offset_set = TRUE;
                        break;

                    case DBOPT_LO_OFFSET:
                        _um._lo_offset = DEREF(int, optlist->values[i]);
                        _um._lo_offset_set = TRUE;
                        break;

                    case DBOPT_GROUPNUM:
                        DEPRECATE_MSG("DBOPT_GROUPNUM",4,6,"MRG Trees")
                        _um._group_no = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_NODENUM:
                        _um._gnodeno = (int*)optlist->values[i];
                        break;

                    case DBOPT_PHZONELIST:
                        _um._phzl_name = (char *)optlist->values[i];
                        break;

                    case DBOPT_HIDE_FROM_GUI:
                        _um._guihide = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MRGTREE_NAME:
                        _um._mrgtree_name = (char *)optlist->values[i];
                        break;

                    case DBOPT_REGION_PNAMES:
                        _um._region_pnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_TV_CONNECTIVITY:
                        _um._tv_connectivity = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_DISJOINT_MODE:
                        _um._disjoint_mode = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_LLONGNZNUM:
                        _um._llong_gnodeno = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_CONSERVED:
                        _um._conserved = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_EXTENSIVE:
                        _um._extensive = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MISSING_VALUE:
                        _um._missing_value = DEREF(double, optlist->values[i]);
                        break;

                    case DBOPT_GHOST_NODE_LABELS:
                        _um._ghost_node_labels = (char *)optlist->values[i];
                        break;

                    case DBOPT_ALT_NODENUM_VARS:
                        _um._alt_nodenum_vars = (char **) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_ZONELIST:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_ZONENUM:
                        _uzl._gzoneno = (int*)optlist->values[i];
                        break;

                    case DBOPT_LLONGNZNUM:
                        _uzl._llong_gzoneno = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_GHOST_ZONE_LABELS:
                        _uzl._ghost_zone_labels = (char *)optlist->values[i];
                        break;

                    case DBOPT_ALT_ZONENUM_VARS:
                        _uzl._alt_zonenum_vars = (char **) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_PHZONELIST:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_ZONENUM:
                        _phzl._gzoneno = (int*)optlist->values[i];
                        break;

                    case DBOPT_LLONGNZNUM:
                        _phzl._llong_gzoneno = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_GHOST_ZONE_LABELS:
                        _phzl._ghost_zone_labels = (char *)optlist->values[i];
                        break;

                    case DBOPT_ALT_ZONENUM_VARS:
                        _phzl._alt_zonenum_vars = (char **) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_CSGZONELIST:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_REGNAMES:
                        _csgzl._regnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_ZONENAMES:
                        _csgzl._zonenames = (char **) optlist->values[i];
                        break;

                    case DBOPT_ALT_ZONENUM_VARS:
                        _csgzl._alt_zonenum_vars = (char **) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_MULTIMESH:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_TIME:
                        _mm._time = DEREF(float, optlist->values[i]);
                        _mm._time_set = TRUE;
                        break;

                    case DBOPT_DTIME:
                        _mm._dtime = DEREF(double, optlist->values[i]);
                        _mm._dtime_set = TRUE;
                        break;

                    case DBOPT_CYCLE:
                        _mm._cycle = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MATNOS:
                        _mm._matnos = (int *) optlist->values[i];
                        break;

                    case DBOPT_NMATNOS:
                        _mm._nmatnos = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MATNAME:
                        _mm._matname = (char *) optlist->values[i];
                        break;

                    case DBOPT_NMAT:
                        _mm._nmat = DEREF(int,optlist->values[i]);
                        break;

                    case DBOPT_NMATSPEC:
                        _mm._nmatspec = (int *) optlist->values[i];
                        break;

                    case DBOPT_BLOCKORIGIN:
                        _mm._blockorigin = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_GROUPORIGIN:
                        DEPRECATE_MSG("DBOPT_GROUPORIGIN",4,6,"MRG Trees")
                        _mm._grouporigin = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_NGROUPS:
                        DEPRECATE_MSG("DBOPT_NGROUPS",4,6,"MRG Trees")
                        _mm._ngroups = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_EXTENTS_SIZE:
                        _mm._extentssize = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_EXTENTS:
                        _mm._extents = (double *) optlist->values[i];
                        break;

                    case DBOPT_ZONECOUNTS:
                        _mm._zonecounts = (int *) optlist->values[i];
                        break;

                    case DBOPT_MIXLENS:
                        _mm._mixlens = (int *) optlist->values[i];
                        break;

                    case DBOPT_MATCOUNTS:
                        _mm._matcounts = (int *) optlist->values[i];
                        break;

                    case DBOPT_MATLISTS:
                        _mm._matlists = (int *) optlist->values[i];
                        break;

                    case DBOPT_HAS_EXTERNAL_ZONES:
                        _mm._has_external_zones = (int *) optlist->values[i];
                        break;

                    case DBOPT_HIDE_FROM_GUI:
                        _mm._guihide = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_GROUPINGS_SIZE:
                        DEPRECATE_MSG("DBOPT_GROUPINGS_SIZE",4,6,"MRG Trees")
                        _mm._lgroupings = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_GROUPINGS:
                        DEPRECATE_MSG("DBOPT_GROUPINGS",4,6,"MRG Trees")
                        _mm._groupings = (int *) optlist->values[i];
                        break;

                    case DBOPT_GROUPINGNAMES:
                        DEPRECATE_MSG("DBOPT_GROUPINGNAMES",4,6,"MRG Trees")
                        _mm._groupnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_MATCOLORS:
                        _mm._matcolors = (char **) optlist->values[i];
                        break;

                    case DBOPT_MATNAMES:
                        _mm._matnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_ALLOWMAT0:
                        _mm._allowmat0 = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MRGTREE_NAME:
                        _mm._mrgtree_name = (char *)optlist->values[i];
                        break;

                    case DBOPT_REGION_PNAMES:
                        _mm._region_pnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_MMESH_NAME:
                        _mm._mmesh_name = (char *)optlist->values[i];
                        break;

                    case DBOPT_TENSOR_RANK:
                        _mm._tensor_rank = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_TV_CONNECTIVITY:
                        _mm._tv_connectivity = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_DISJOINT_MODE:
                        _mm._disjoint_mode = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_TOPO_DIM:
                        /* The value of '_topo_dim' member is designed such
                           that a value of zero (which can be a valid topological
                           dimension specified by a caller) represents the
                           NOT SET value. So, we always add 1 to whatever the
                           caller gives us. */
                        _mm._topo_dim = DEREF(int, optlist->values[i])+1;
                        break;

                    case DBOPT_SPECNAMES:
                        _mm._specnames = (char **) optlist->values[i];
                        break;

                    case DBOPT_SPECCOLORS:
                        _mm._speccolors = (char **) optlist->values[i];
                        break;

                    case DBOPT_CONSERVED:
                        _mm._conserved = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_EXTENSIVE:
                        _mm._extensive = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MB_FILE_NS:
                        _mm._file_ns = (char *) optlist->values[i];
                        break;

                    case DBOPT_MB_BLOCK_NS:
                        _mm._block_ns = (char *) optlist->values[i];
                        break;

                    case DBOPT_MB_BLOCK_TYPE:
                        _mm._block_type = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MB_EMPTY_LIST:
                        _mm._empty_list = (int *) optlist->values[i];
                        break;

                    case DBOPT_MB_EMPTY_COUNT:
                        _mm._empty_cnt = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MB_REPR_BLOCK_IDX:
                        /* +1 to support zero value indicating NOT SET */
                        _mm._repr_block_idx = DEREF(int,optlist->values[i])+1;
                        break;

                    case DBOPT_MISSING_VALUE:
                        _mm._missing_value = DEREF(double, optlist->values[i]);
                        break;

                    case DBOPT_ALT_NODENUM_VARS:
                        _mm._alt_nodenum_vars = (char**) optlist->values[i];
                        break;

                    case DBOPT_ALT_ZONENUM_VARS:
                        _mm._alt_zonenum_vars = (char**) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_CURVE:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_LABEL:
                        _cu._label = (char *)optlist->values[i];
                        break;

                    case DBOPT_XLABEL:
                        _cu._labels[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YLABEL:
                        _cu._labels[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_XUNITS:
                        _cu._units[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YUNITS:
                        _cu._units[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_XVARNAME:
                        _cu._varname[0] = (char *)optlist->values[i];
                        break;

                    case DBOPT_YVARNAME:
                        _cu._varname[1] = (char *)optlist->values[i];
                        break;

                    case DBOPT_HIDE_FROM_GUI:
                        _cu._guihide = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_REFERENCE:
                        _cu._reference = (char *)optlist->values[i];
                        break;

                    case DBOPT_COORDSYS:
                        _cu._coord_sys = DEREF(int, optlist->values[i]);
                        break;

                    case DBOPT_MISSING_VALUE:
                        _cu._missing_value = DEREF(double, optlist->values[i]);
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_DEFVARS:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_HIDE_FROM_GUI:
                        _dv._guihide = DEREF(int, optlist->values[i]);
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        case DB_MRGTREE:
            for (i = 0; i < optlist->numopts; i++)
            {
                switch (optlist->options[i])
                {
                    case DBOPT_MRGV_ONAMES:
                        _mrgt._mrgvar_onames = (char **) optlist->values[i];
                        break;

                    case DBOPT_MRGV_RNAMES:
                        _mrgt._mrgvar_rnames = (char **) optlist->values[i];
                        break;

                    default:
                        unused++;
                        break;
                }
            }
            break;

        default:
            return db_perror(NULL, E_NOTIMP, me);
    }

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    DBInqCompoundarray
 *
 * Purpose:     Inquire compound array attributes
 *
 * Return:      Success:        OKAY
 *
 *              Failure:        OOPS
 *
 * Arguments:
 *      dbfile         ptr to data file
 *      array_name     array name
 *
 *                Output args
 *      elemnames      simple array names
 *      elemlengths    simple array sizes
 *      nelems         number of simple arrys
 *      nvalues        number of values
 *      datatype       value data type
 *
 * Programmer:  matzke@viper
 *              Tue Oct 25 13:58:53 PDT 1994
 *
 * Modifications:
 *    matzke@viper, Mon Oct 31 13:39:10 PST 1994
 *    No longer calls DBGetCompoundarray.
 *
 *    Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *    I replaced API_END with API_END_NOPOP.
 *
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *-------------------------------------------------------------------------*/
PUBLIC int
DBInqCompoundarray(DBfile *dbfile, const char *array_name,
                   char **elemnames[], int **elemlengths, int *nelems,
                   int *nvalues, int *datatype)

{
    DBcompoundarray *ca = NULL;

    API_BEGIN2("DBInqCompoundarray", int, -1, array_name) {
        if (!array_name || !*array_name)
            API_ERROR("array name", E_BADARGS);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBInqCompoundarray", E_GRABBED) ; 
        if (elemnames)
            *elemnames = NULL;
        if (elemlengths)
            *elemlengths = NULL;
        if (nelems)
            *nelems = 0;
        if (nvalues)
            *nvalues = 0;
        if (datatype)
            *datatype = 0;

        if (!dbfile->pub.g_ca)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        ca = DBGetCompoundarray(dbfile, array_name);
        if (!ca)
            API_ERROR("DBGetCompoundarray", E_CALLFAIL);

        if (elemnames) {
            *elemnames = ca->elemnames;
            ca->elemnames = NULL;  /*so we don't free it... */
        }
        if (elemlengths) {
            *elemlengths = ca->elemlengths;
            ca->elemlengths = NULL;
        }
        if (nelems)
            *nelems = ca->nelems;
        if (nvalues)
            *nvalues = ca->nvalues;
        if (datatype)
            *datatype = ca->datatype;

        DBFreeCompoundarray(ca);
    }
    API_END;

    return(0);
}

/*-------------------------------------------------------------------------
 * Function:    DBGetComponentNames
 *
 * Purpose:     Returns the component names for the specified object.
 *              Each component name also has a variable name under which
 *              the component value is stored in the data file.  The
 *              COMP_NAMES and FILE_NAMES output arguments will point to
 *              an array of pointers to names.  Each name as well as the
 *              two arrays will be allocated with `malloc'.
 *
 * Return:      Success:        Number of components found for the
 *                              specified object.
 *
 *              Failure:        zero.
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.mdn.com
 *              May 20, 1996
 *
 * Modifications:
 *    Sean Ahern, Tue Sep 28 10:48:06 PDT 1999
 *    Added a check for variable name validity.
 *
 *    Mark C. Miller, Tue Sep  6 10:57:55 PDT 2005
 *    Deprecated this function
 *-------------------------------------------------------------------------*/
PUBLIC int
DBGetComponentNames(DBfile *dbfile, const char *objname,
                    char ***comp_names, char ***file_names)
{
    int retval;

    API_DEPRECATE2("DBGetComponentNames", int, -1, objname, 4,6,"")
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (!dbfile->pub.g_compnames)
            API_ERROR(dbfile->pub.name, E_NOTIMP);
        if (!objname || !*objname)
            API_ERROR("object name", E_BADARGS);

        retval = (dbfile->pub.g_compnames) (dbfile, objname,
                                            comp_names, file_names);
        API_RETURN(retval);
    }
    API_END_NOPOP;  /* If API_RETURN above is removed, use API_END instead */
}

/*----------------------------------------------------------------------
 * Routine                                             db_SplitShapelist
 *
 * Purpose
 *
 *    Split the shapecnts in the zone list so that an entry in the
 *    shapecnt array will either refer to all real zones or all ghost
 *    zones.
 *
 * Programmer
 *
 *    Eric Brugger, January 22, 1999
 *
 * Notes
 *
 * Modifications
 *    Eric Brugger, Wed Mar 31 11:36:42 PST 1999
 *    Modify the routine to handle polyhedra.  This turned out to be
 *    a significant rewrite of the routine.
 *
 *    Eric Brugger, Tue Apr 20 09:24:51 PDT 1999
 *    Correct a bug where the nshapes field was always zero when the
 *    shapetype field was NULL.
 *
 *    Jeremy Meredith, Fri Aug 13 13:53:57 PDT 1999
 *    Corrected a bug where nshapes was still not incremented enough if
 *    shapetype was NULL.  This was causing ghost zones to disappear.
 *
 *    Mark C. Miller, Mon Jun 21 18:06:36 PDT 2004
 *    Moved from silo_pdb.c to public place where any driver can call
 *--------------------------------------------------------------------*/
INTERNAL int
db_SplitShapelist (DBucdmesh *um)
{
    int       *shapecnt=NULL, *shapesize=NULL, *shapetype=NULL, nshapes;
    int       *zonelist=NULL, nzones;
    int        min_index, max_index;
    int       *shapecnt2=NULL, *shapesize2=NULL, *shapetype2=NULL, nshapes2;
    int        i, iz, izl, deltaiz;
    int        isplit, splits[3];

    shapecnt  = um->zones->shapecnt;
    shapesize = um->zones->shapesize;
    shapetype = um->zones->shapetype;
    nshapes   = um->zones->nshapes;
    zonelist  = um->zones->nodelist;
    min_index = um->zones->min_index;
    max_index = um->zones->max_index;
    nzones    = um->zones->nzones;

    nshapes2   = 0;
    shapecnt2  = ALLOC_N (int, nshapes+2);
    shapesize2 = ALLOC_N (int, nshapes+2);
    if (shapetype != NULL)
    {
        shapetype2 = ALLOC_N (int, nshapes+2);
    }

    if (min_index > 0)
    {
        splits[0] = min_index;
        splits[1] = max_index + 1;
        splits[2] = nzones;
    }
    else
    {
        splits[0] = max_index + 1;
        splits[1] = nzones;
        splits[2] = 0;
    }

    isplit = 0;
    i = 0;
    iz = 0;
    izl = 0;
    while (iz < nzones)
    {
        if (splits[isplit] - iz >= shapecnt[i])
        {
            shapecnt2 [nshapes2]   = shapecnt[i];
            shapesize2[nshapes2]   = shapesize[i];
            if (shapetype != NULL)
            {
                shapetype2[nshapes2] = shapetype[i];
            }
            nshapes2++;
            isplit += (splits[isplit] - iz == shapecnt[i]) ? 1 : 0;
            iz += shapecnt[i];
            if (shapetype != NULL && shapetype[i] == DB_ZONETYPE_POLYHEDRON)
            {
                izl += shapesize[i];
            }
            else
            {
                izl += shapesize[i] * shapecnt[i];
            }
            i++;
        }
        else
        {
            deltaiz = splits[isplit] - iz;
            shapecnt2[nshapes2] = deltaiz;
            if (shapetype != NULL && shapetype[i] == DB_ZONETYPE_POLYHEDRON)
            {
                int       j, k;
                int       izlInit, nFaces;

                izlInit = izl;
                for (j = 0; j < deltaiz; j++)
                {
                    nFaces = zonelist[izl++];
                    for (k = 0; k < nFaces; k++)
                    {
                        izl += zonelist[izl] + 1;
                    }
                }
                shapesize2[nshapes2] = izl - izlInit;
                shapesize[i] -= izl - izlInit;
            }
            else
            {
                izl += shapesize[i] * deltaiz;
                shapesize2[nshapes2] = shapesize[i];
            }
            if (shapetype != NULL)
            {
                shapetype2[nshapes2] = shapetype[i];
            }
            nshapes2++;
            shapecnt[i] -= deltaiz;
            isplit++;
            iz += deltaiz;
        }
    }

    FREE (shapecnt);
    FREE (shapesize);
    FREE (shapetype);
    um->zones->shapecnt  = shapecnt2;
    um->zones->shapesize = shapesize2;
    um->zones->shapetype = shapetype2;
    um->zones->nshapes   = nshapes2;

    return 0;
}

/*----------------------------------------------------------------------
 *  Routine                                   db_ResetGlobalData_Csgmesh
 *
 *  Purpose
 *
 *      Reset global data to default values. For internal use only.
 *
 *  Programmer
 *
 *    Mark C. Miller, Wed Aug  3 14:39:03 PDT 2005
 *
 *  Modifications:
 *    Mark C. Miller, Mon Jan 12 16:29:19 PST 2009
 *    Removed explicit setting of data members already handled
 *    correctly by memset to zero.
 *--------------------------------------------------------------------*/
INTERNAL int
db_ResetGlobalData_Csgmesh () {

   memset(&_csgm, 0, sizeof(_csgm));
   _csgm._use_specmf = DB_OFF;
   _csgm._group_no = -1;
   _csgm._missing_value = DB_MISSING_VALUE_NOT_SET;

   return 0;
}
/*----------------------------------------------------------------------
 *  Routine                                 db_ResetGlobalData_PointMesh
 *
 *  Purpose
 *
 *      Reset global data to default values. For internal use only.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *      It is assumed that _ndims has a valid value before this
 *      function is invoked. (It is assigned to _nspace.)
 *
 *  Modifications
 *
 *      Al Leibee, Mon Apr 18 07:45:58 PDT 1994
 *      Added _dtime.
 *
 *      Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *      Init group_no to -1.
 *
 *    Mark C. Miller, Mon Jun 21 18:06:36 PDT 2004
 *    Moved from silo_pdb.c to public place where any driver can call
 *
 *    Mark C. Miller, Mon Jan 12 16:29:19 PST 2009
 *    Removed explicit setting of data members already handled
 *    correctly by memset to zero.
 *--------------------------------------------------------------------*/
INTERNAL int
db_ResetGlobalData_PointMesh (int ndims) {

   memset(&_pm, 0, sizeof(_pm));
   _pm._ndims = ndims;
   _pm._nspace = ndims;
   _pm._group_no = -1;
   _pm._missing_value = DB_MISSING_VALUE_NOT_SET;
   return 0;
}

/*----------------------------------------------------------------------
 *  Routine                                  db_ResetGlobalData_QuadMesh
 *
 *  Purpose
 *
 *      Reset global data to default values. For internal use only.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *      It is assumed that _ndims has a valid value before this
 *      function is invoked. (It is assigned to _nspace.)
 *
 *  Modifications
 *
 *     Al Leibee, Wed Aug  3 16:57:38 PDT 1994
 *     Added _use_specmf.
 *
 *     Al Leibee, Sun Apr 17 07:54:25 PDT 1994
 *     Added dtime.
 *
 *     Robb Matzke, 18 Jun 1997
 *     Initialize ascii_labels field to FALSE.
 *
 *     Eric Brugger, Mon Oct  6 15:11:26 PDT 1997
 *     I modified the routine to initialize lo_offset and hi_offset.
 *
 *     Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *     Init group_no to -1.  Init baseindex and baseindex_set.
 *
 *    Mark C. Miller, Mon Jun 21 18:06:36 PDT 2004
 *    Moved from silo_pdb.c to public place where any driver can call
 *
 *    Mark C. Miller, Mon Jan 12 16:29:19 PST 2009
 *    Removed explicit setting of data members already handled
 *    correctly by memset to zero.
 *--------------------------------------------------------------------*/
INTERNAL int
db_ResetGlobalData_QuadMesh (int ndims) {

   memset(&_qm, 0, sizeof(_qm));

   _qm._coord_sys = DB_OTHER;
   _qm._facetype = DB_RECTILINEAR;
   _qm._ndims = ndims;
   _qm._nspace = ndims;
   _qm._planar = DB_AREA;
   _qm._use_specmf = DB_OFF;
   _qm._group_no = -1;
   _qm._missing_value = DB_MISSING_VALUE_NOT_SET;

   return 0;
}


/*-------------------------------------------------------------------------
 * Function:    db_ResetGlobalData_Curve
 *
 * Purpose:     Reset global data to default values.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.com
 *              May 16, 1996
 *
 * Modifications:
 *
 *    Mark C. Miller, Mon Jun 21 18:06:36 PDT 2004
 *    Moved from silo_pdb.c to public place where any driver can call
 *-------------------------------------------------------------------------*/
INTERNAL void
db_ResetGlobalData_Curve (void) {

   memset (&_cu, 0, sizeof(_cu)) ;
   _cu._missing_value = DB_MISSING_VALUE_NOT_SET;
}

/*----------------------------------------------------------------------
 *  Routine                                   db_ResetGlobalData_Ucdmesh
 *
 *  Purpose
 *
 *      Reset global data to default values. For internal use only.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *      It is assumed that _ndims has a valid value before this
 *      function is invoked. (It is assigned to _nspace.)
 *
 *  Modifications
 *     Al Leibee, Wed Aug  3 16:57:38 PDT 1994
 *     Added _use_specmf.
 *
 *     Al Leibee, Mon Apr 18 07:45:58 PDT 1994
 *     Added _dtime.
 *
 *     Eric Brugger, Wed Oct 15 14:45:47 PDT 1997
 *     Added _hi_offset and _lo_offset.
 *
 *     Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *     Init group_no to -1.
 *
 *     Mark C. Miller, Mon Jun 21 18:06:36 PDT 2004
 *     Moved from silo_pdb.c to public place where any driver can call
 *
 *     Brad Whitlock, Wed Jan 18 15:38:39 PST 2006
 *     Added _ascii_labels.
 *
 *     Thomas R. Treadway, Wed Jun 28 10:31:45 PDT 2006
 *     Added _topo_dim..
 *
 *     Mark C. Miller, Tue Jan  6 22:12:43 PST 2009
 *     Made default value for topo_dim to be NOT SET (-1).
 *
 *     Mark C. Miller, Mon Jan 12 16:26:08 PST 2009
 *     Replaced 'topo_dim' with 'tdim_plus1', removed it from being
 *     explicitly set. Likewise, removed explicit setting of other
 *     entries that are already correctly handled by memset to zero.
 *--------------------------------------------------------------------*/
INTERNAL int
db_ResetGlobalData_Ucdmesh (int ndims, int nnodes, int nzones) {

   memset(&_um, 0, sizeof(_um));
   _um._coord_sys = DB_OTHER;
   _um._facetype = DB_RECTILINEAR;
   _um._ndims = ndims;
   _um._nnodes = nnodes;
   _um._nzones = nzones;
   _um._planar = DB_OTHER;
   _um._use_specmf = DB_OFF;
   _um._group_no = -1;
   _um._missing_value = DB_MISSING_VALUE_NOT_SET;

   return 0;
}

/*----------------------------------------------------------------------
 *  Routine                               db_ResetGlobalData_Ucdzonelist
 *
 *  Purpose
 *
 *      Reset global data to default values. For internal use only.
 *
 *  Programmer
 *
 *      Jeremy Meredith, May 21 1999
 *
 *  Notes
 *
 *  Modifications
 *
 *      Hank Childs, Thu Jan  6 16:10:03 PST 2000
 *      Added void to function signature to avoid compiler warning.
 *
 *    Mark C. Miller, Mon Jun 21 18:06:36 PDT 2004
 *    Moved from silo_pdb.c to public place where any driver can call
 *--------------------------------------------------------------------*/
INTERNAL int
db_ResetGlobalData_Ucdzonelist (void) {

   memset(&_uzl, 0, sizeof(_uzl));

   return 0;
}

/*----------------------------------------------------------------------
 * Routine                                  db_ResetGlobalData_MultiMesh
 *
 * Purpose
 *
 *    Reset global data to default values. For internal use only.
 *
 * Programmer
 *
 *    Eric Brugger, January 12, 1996
 *
 * Notes
 *
 * Modifications
 *    Eric Brugger, Thu Oct 16 10:40:00 PDT 1997
 *    I added the options DBOPT_MATNOS and DBOPT_NMATNOS.
 *
 *    Jeremy Meredith Sept 18 1998
 *    Added options DBOPT_MATNAME, DBOPT_NMAT, and DBOPT_NMATSPEC.
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added _blockorigin, _grouporigin, and _ngroups.
 *
 *    Mark C. Miller, Mon Jun 21 18:06:36 PDT 2004
 *    Moved from silo_pdb.c to public place where any driver can call
 *
 *    Thomas R. Treadway, Thu Jul 20 11:06:27 PDT 2006
 *    Added _lgroupings, _groupings, and _groupnames.
 *
 *    Mark C. Miller, Mon Jan 12 16:28:18 PST 2009
 *    Removed explicit setting of members already correctly handled
 *    by memset to zero.
 *
 *    Mark C. Miller, Thu Aug 30 17:55:43 PDT 2012
 *    Removed setting nmatnos to -1.
 *--------------------------------------------------------------------*/
INTERNAL int
db_ResetGlobalData_MultiMesh (void) {
   memset(&_mm, 0, sizeof(_mm));
   _mm._nmat = -1;
   _mm._blockorigin = 1;
   _mm._grouporigin = 1;
   _mm._missing_value = DB_MISSING_VALUE_NOT_SET;
   return 0;
}

/*----------------------------------------------------------------------
 * Routine                                  db_ResetGlobalData_Defvars
 *
 * Purpose
 *
 *    Reset global data to default values. For internal use only.
 *
 * Programmer:
 *
 *    Mark C. Miller, March 22, 2006
 *--------------------------------------------------------------------*/
INTERNAL int
db_ResetGlobalData_Defvars (void) {
   memset(&_dv, 0, sizeof(_dv));
   return 0;
}

INTERNAL int
db_ResetGlobalData_Mrgtree (void) {
   memset(&_mrgt, 0, sizeof(_mrgt));
   return 0;
}

/*----------------------------------------------------------------------
 * Routine                                  db_FullName2BaseName
 *
 * Purpose
 *
 *    Given a the full path name of an object in the db, return
 *    the object's basename.
 *
 * Programmer
 *
 *    Mark C. Miller, June 22, 2004 
 *
 * Modifications:
 *    Mark C. Miller, Thu Sep  7 10:50:55 PDT 2006
 *    Made it just use Jim Reus' new basename routine.
 *--------------------------------------------------------------------*/
INTERNAL char *
db_FullName2BaseName(const char *path)
{
   return db_basename(path);
}

/*----------------------------------------------------------------------
 * Purpose
 *
 *    catenate an array of strings into a single, semicolon seperated
 *    string list
 *
 * Programmer
 *
 *    Mark C. Miller, July 20, 2005 
 *
 * Modifications:
 *    Mark C. Miller, Wed Oct  3 21:51:42 PDT 2007
 *    Made it handle null string as no chars output and empty string
 *    ("") as '\n' output so during readback, we can construct either
 *    null ptrs or emtpy strings correctly.
 *    Made it handle a variable length list where n is unspecified.
 *
 *    Mark C. Miller, Wed Jul 14 20:38:46 PDT 2010
 *    Made this function public, replacing 'db_' with 'DB' in name.
 *--------------------------------------------------------------------*/
PUBLIC void 
DBStringArrayToStringList(
    char const * const *strArray,
    int n,
    char **strList,
    int *m
)
{
    int i, len;
    char *s = NULL;

    /* if n is unspecified, determine it by counting forward until
       we get a null pointer */
    if (n < 0)
    {
        n = 0;
        while (strArray[n] != 0)
            n++;
    }

    /*
     * Create a string which is a semi-colon separated list of strings
     */
     for (i=len=0; i<n; i++)
     {
         if (strArray[i])
             len += strlen(strArray[i])+1;
         else
             len += 2;
     }
     s = (char*)malloc(len+1);
#ifndef _WIN32
#warning TEST THIS LOGIC
#endif
     for (i=len=0; i<n; i++) {
         char const *p = strArray[i]?strArray[i]:"\n";
         if (i) s[len++] = ';';
         strcpy(s+len, p);
         len += strlen(p);
     }
     len++; /*count last null*/

     *strList = s;
     *m = len;
}

/*----------------------------------------------------------------------
 * Purpose
 *
 *    Decompose a single, sep-char seperated string list into an array
 *    of strings
 *
 * Programmer
 *
 *    Mark C. Miller, July 20, 2005 
 *
 * Modfications:
 *
 *    Mark C. Miller, Fri Jul 14 23:39:32 PDT 2006
 *    Fixed problem with empty strings in the input list being skipped
 *
 *    Mark C. Miller, Wed Oct  3 21:54:35 PDT 2007
 *    Made it return empty or null strings depending on input
 *    Made it handle a variable length list where n is unspecified
 *
 *    Mark C. Miller, Mon Nov  9 12:10:47 PST 2009
 *    Added logic to handle swapping of slash character between 
 *    windows/linux. Note that swapping of slash character only 
 *    makes sense in certain context and only when it appears in
 *    a string BEFORE a colon character. We try to minimize the
 *    amount of work we do looking for a colon character by
 *    remembering where we find it in the last substring.
 *
 *    Mark C. Miller, Thu Dec 17 17:09:27 PST 2009
 *    Fixed UMR on strLen when n>=0.
 *
 *    Mark C. Miller, Wed Jul 14 20:38:46 PDT 2010
 *    Made this function public, replacing 'db_' with 'DB' in name.
 *    Merged fixes from 4.7.3 patches to fix problems with swaping
 *    the slash character.
 *
 *    Mark C. Miller, Wed Jun 30 16:01:17 PDT 2010
 *    Made logic for handling slash swap more sane. Now, swapping is
 *    performed AFTER the list of strings has been broken out into
 *    separate arrays.
 *
 *    Mark C. Miller, Fri Oct 12 22:57:23 PDT 2012
 *    Changed interface to return value for number of strings as well
 *    as accept an input value or nothing at all.
 *--------------------------------------------------------------------*/
INTERNAL char **
db_StringListToStringArray(char const *strList, int *_n, char sep, int skipSepAtIndexZero)
{
    int i, l, n, add1 = 0;
    char **retval;

    /* handle null case */
    if (!strList || strList[0] == '\0')
    {
        if (_n && *_n < 0) *_n = 0;
        return 0;
    }

    /* if n is unspecified (<0), compute it by counting sep chars */
    if (_n == 0 || *_n < 0)
    {
        add1 = 1;
        n = 1;
        i = (skipSepAtIndexZero&&strList[0]==sep)?1:0;
        while (strList[i] != '\0')
        {
            if (strList[i] == sep)
                n++;
            i++;
        }
    }
    else
    {
        n = *_n;
    }

    if (n <= 0)
    {
        if (_n && *_n < 0) *_n = n;
        return 0;
    }

    retval = (char**) calloc(n+add1, sizeof(char*));
    for (i=0, l=(skipSepAtIndexZero&&strList[0]==sep)?1:0; i<n; i++)
    {
        if (strList[l] == sep)
        {
            retval[i] = STRDUP(""); 
            l += 1;
        }
        else if (strList[l] == '\n')
        {
            retval[i] = 0; 
            l += 2;
        }
        else
        {
            int len, lstart = l;
            while (strList[l] != sep && strList[l] != '\0')
                l++;
            len = l-lstart;
            retval[i] = (char *) malloc(len+1);
            memcpy(retval[i],&strList[lstart],len);
            retval[i][len] = '\0';
            l++;
        }
    }
    if (add1) retval[i] = 0;

    /* Return value of n computed if requested */
    if (_n && *_n < 0) *_n = n;

    return retval;
}

PUBLIC char **
DBStringListToStringArray(char const *strList, int *_n, int skipSemicolonAtIndexZero)
{
    return db_StringListToStringArray(strList, _n, ';', skipSemicolonAtIndexZero);
}

PUBLIC void
DBFreeStringArray(char **strArray, int n)
{
    int i;
    if (n < 0)
    {
        for (i = 0; strArray[i]; i++)
            FREE(strArray[i]);
    }
    else
    {
        for (i = 0; i < n; i++)
            FREE(strArray[i]);
    }
    FREE(strArray);
}

INTERNAL int 
db_StringListToStringArrayMBOpt(char *strList, char ***retArray, char **alloc_flag, int nblocks)
{
    int i=0, s=0, n=0, hasColon=0, nearlyDone = 0, completelyDone = 0, slashCharsToSwap[128];
    char **strArray;
    static char const *me = "DBStringListToStringArrayMBOpt";

    if (!strList) return 0;

    if (nblocks <= 0)
        return db_perror("nblocks", E_BADARGS, me);

    strArray = (char **) malloc(nblocks * sizeof(char*));
    if (strList[0] == ';')
        i = 1;
    strArray[n++] = &strList[i];
    while (!completelyDone)
    {
        switch (strList[i])
        {
            case '\0':
                completelyDone = 1; // note fall-through to next case
            case ';':
            {
                strList[i] = '\0';
                if (!completelyDone) i++;
                if (strList[i] != '\0')
                    strArray[n++] = &strList[i];
                if (hasColon)
                {
                    int j;
                    for (j = 0; j < hasColon; j++)
#if !defined(_WIN32)
                        strList[slashCharsToSwap[j]] = '/';
#else
                        strList[slashCharsToSwap[j]] = '\\';
#endif
                }
                s = 0;
                hasColon = 0;
                break;
            }
#if !defined(_WIN32) /* linux case */
            case '\\':
#else                /* windows case */
            case '/':
#endif 
            {
                if (hasColon)
                    break;
                slashCharsToSwap[s++] = i;
                if (s == sizeof(slashCharsToSwap)/sizeof(slashCharsToSwap[0]))
                {
                    free(strList);
                    free(strArray);
                    return db_perror("exceeded slashCharsToSwap size", E_INTERNAL, me);
                }
                break;
            }
            case ':':
            {
                hasColon = s;
                break;
            }
        }
        if (!completelyDone)
        {
            if (strList[i] != '\0')
                i++;
            if (strList[i] == '\0')
                nearlyDone = 1;
        }
    }

    if (n != nblocks)
    {
        free(strArray);
        return db_perror("incorrect number of block names", E_INTERNAL, me);
    }

    /* ensure we store the originally allocated pointer for later free's */
    *alloc_flag = strList;

    *retArray = strArray;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    DBSortObjectsByOffset
 *
 * Purpose:     Determines the offset within the Silo file of each object
 *              in the list of objects passed in and returns an array
 *              and returns an integer array indicating their ordering. 
 *
 * Return:      Success:        Non-zero. 
 *              Failure:        zero.
 *
 * Programmer:  Mark C. Miller, Thu Jul 15 06:40:27 PDT 2010
 *-------------------------------------------------------------------------*/
PUBLIC int
DBSortObjectsByOffset(DBfile *dbfile, int nobjs, 
    const char *const *const names, int *ordering)
{
    int retval;

    API_BEGIN2("DBSortObjectsByOffset", int, -1, api_dummy);
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (nobjs <= 0)
            API_ERROR("nobjs", E_BADARGS);
        if (!names)
            API_ERROR("names", E_BADARGS);
        if (!ordering)
            API_ERROR("ordering", E_BADARGS);
        if (!dbfile->pub.sort_obo)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.sort_obo) (dbfile, nobjs, names, ordering); 

        API_RETURN(retval);
    }
    API_END_NOPOP;  /* If API_RETURN above is removed, use API_END instead */
}

/*----------------------------------------------------------------------
 * Purpose
 *
 *    Break an extend driver id into type and subtype 
 *
 * Programmer
 *
 *    Mark C. Miller, July 31, 2006 
 *
 * Modifications:
 *  Mark C. Miller, Mon Aug 21 23:14:29 PDT 2006
 *  Made code that references DB_HDF5 conditionally compiled
 *
 *  Mark C. Miller, Thu Feb 11 09:51:28 PST 2010
 *  Changed logic for how subtype is handled.
 *
 *  Mark C. Miller, Thu Feb 25 19:00:09 PST 2010
 *  Versions of silo 4.7.2 and earlier used a bit of a brain dead way
 *  to specify alternative HDF5 vfds by manipulating the high order
 *  bits in the integer 'type' arg to DBCreate/DBOpen. For example, the
 *  default HDF5 driver was '7' while HDF5 w/STDIO vfd was '0x200'. This
 *  was inflexible and unable to handle the large variety of options
 *  available in HDF5.
 *
 *  Versions of silo newer than 4.7.2 use a global array of options
 *  sets registered and stored in the SILO_Globals structure. So,
 *  a particular set of HDF5 vfd options is identified by a single
 *  integer indexing into this global list of options. It is this integer
 *  index that is shifted left by 11 bits to make space for the primary
 *  Silo driver id (e.g. DB_PDB or DB_HDF5) and obsoleted HDF5 vfd
 *  specifications and then OR'd into the integer 'type' arg in the
 *  DBCreate/DBOpen calls to specify HDF5 vfd options.
 *
 *  In the initial implementation of this new approach using a global
 *  array of options sets, we allowed for a total of 32 (5 bits)
 *  options sets plus another 10 default options sets for convenience.
 *  But, we don't actually store the 10 default options set and use
 *  only the integer identifer between 0 and 9 to identify them.
 *  So, the identifier for a given options set ranges from 0...41
 *  requiring a total of 6 bits. Those 6 bits are 0x1F800.
 *--------------------------------------------------------------------*/
INTERNAL void 
db_DriverTypeAndFileOptionsSetId(int driver, int *type, int *_opts_set_id)
{
    int theType = driver&0xF; 
    int opts_set_id = 0;

    if (driver > DB_NFORMATS)
    {
        opts_set_id = (driver&0x1F800)>>11;
#ifdef DB_HDF5X
        if (theType == DB_HDF5X)
        {
            int obsolete_subType = driver&0x700;
            switch (obsolete_subType)
            {
                case DB_HDF5_SEC2_OBSOLETE:
                    opts_set_id = DB_FILE_OPTS_H5_DEFAULT_SEC2;
                    break;
                case DB_HDF5_STDIO_OBSOLETE:
                    opts_set_id = DB_FILE_OPTS_H5_DEFAULT_STDIO;
                    break;
                case DB_HDF5_CORE_OBSOLETE:
                    opts_set_id = DB_FILE_OPTS_H5_DEFAULT_CORE;
                    break;
                case DB_HDF5_MPIO_OBSOLETE:
                    opts_set_id = DB_FILE_OPTS_H5_DEFAULT_MPIO;
                    break;
                case DB_HDF5_MPIOP_OBSOLETE:
                    opts_set_id = DB_FILE_OPTS_H5_DEFAULT_MPIP;
                    break;
                default:
                    break;
            }
        }
#endif
    }

    if (type) *type = theType;
    if (_opts_set_id) *_opts_set_id = opts_set_id;
}

/*
 *
 * The following data structures and functions for manipulating
 * character strings representing pathnames was originally written
 * by James F. Reus as part of the DSL Library. It was extracted from
 * DSL and adapted, slightly, for use in the Silo library by Mark C. Miller
 *
 * BEGIN CODE FROM JIM REUS' DSL {
 *
 */

char *db_absoluteOf_path (const char *cwg,
                          const char *pathname)
{  char *result;

   if (pathname && strlen(pathname))
   {  if (db_isAbsolute_path(pathname))
          result = db_normalize_path(pathname);
      else if (cwg && strlen(cwg))
          result = db_join_path(cwg,pathname);
      else
          result = _db_safe_strdup("");
   }
   else
   {
      result = _db_safe_strdup("");
   }
   return result;
}

/*-------------------------------------------------------------------------- - -
|
|   Description: This function returns a string representing the basename part
|                of the given pathname (stripping off the parent path).  Note
|                that special cases arise...
|
|                    pathname == 0            Return value is 0.
|                    pathname == "/"            Return value is "/".
|                    pathname == "/base"        Return value is "base".
|                    pathname == "base"         Return value is "base".
|                    pathname == "path/base"    Return value is "base".
|
|   Return:      A pointer to a NULL-terminated string is returned when this
|                function is successful.  Note that this string is constructed
|                from allocated dynamic memory, it is up to the caller to
|                release this string when it is no longer needed.  A 0
|                pointer is returned on error.
|
+-----------------------------------------------------------------------------*/

char *db_basename ( const char *pathname )
{  char *result;

   result = 0;
   {  if (0 < strlen(pathname))
      {  if (strcmp(pathname,"/") == 0)
            result = STRDUP("/");
         else
         {  int i;

            for (i=(int)strlen(pathname)-1; 0<=i; --i)
               if (pathname[i] == '/')
               {  result = STRDUP(&(pathname[i+1]));
                  goto theExit;
               }
            result = STRDUP(pathname);
         }
      }
   }
theExit:
   return result;
}

/*-------------------------------------------------------------------------- - -
|
|   Description: This function is used to release all storage associated
|                with the given pathname component list.  Such a pathname
|                component list is typically derived from a NULL-terminated
|                string using the db_split_path() function.
|
|   Return:      A 0 pointer is always returned.
|
+-----------------------------------------------------------------------------*/

db_Pathname *db_cleanup_path ( db_Pathname *p )
{  
   {  if (p != 0)
      {  while (p->firstComponent != 0)
         {  db_PathnameComponent *c;

            c                                  = p->firstComponent;
            p->firstComponent                  = c->nextComponent;
            if (c->nextComponent == 0)
               p->lastComponent                = 0;
            else
               c->nextComponent->prevComponent = 0;
            if (c->name != 0)
            {  free(c->name);
               c->name                         = 0;
            }
            c->prevComponent                   = 0;
            c->nextComponent                   = 0;
            free(c);
         }
         free(p);
         p = 0;
      }
   }
   return p;
}

/*-------------------------------------------------------------------------- - -
|
|   Description: This function returns a string representing the dirname part
|                of the given pathname (stripping off the parent path).  Note
|                that special cases arise...
|
|                    pathname == 0            Return value is 0.
|                    pathname == "/"            Return value is "/".
|                    pathname == "/base"        Return value is "/".
|                    pathname == "base"         Return value is ".".
|                    pathname == "path/base"    Return value is "path".
|
|   Return:      A pointer to a NULL-terminated string is returned when this
|                function is successful.  Note that this string is constructed
|                from allocated dynamic memory, it is up to the caller to
|                release this string when it is no longer needed.  A 0
|                pointer is returned on error.
|
+-----------------------------------------------------------------------------*/

char *db_dirname ( const char *pathname )
{  char *result;
   char *tmp_pathname = db_normalize_path(pathname);

   result = 0;
   {  if (0 < strlen(tmp_pathname))
      {  if (strcmp(tmp_pathname,"/") == 0)
            result = STRDUP("/");
         else
         {  int  i;
            char tmp[32767];

            strcpy(tmp,tmp_pathname);
            for (i=(int)strlen(tmp)-1; 0<=i; --i)
               if (tmp[i] == '/')
               {  if (i == 0)
                     tmp[1] = '\0';
                  else
                     tmp[i] = '\0';
                  result = STRDUP(tmp);
                  goto theExit;
               }
            result = STRDUP(".");
         }
      }
   }
theExit:
   free(tmp_pathname);
   return result;
}

/*-------------------------------------------------------------------------- - -
|
|   Description: This function is used to determine if the given pathname
|                is an absolute pathname.  Note that this is really just a
|                test for a leading '/'.
|
|   Return:      A value of TRUE is returned when the function is
|                successful, otherwise a value of FALSE is returned.
|
+-----------------------------------------------------------------------------*/

int db_isAbsolute_path ( const char *pathname )
{  int result;

   result = FALSE;
   if (0 < strlen(pathname))
      if (pathname[0] == '/')
         result = TRUE;
   return result;
}

/*-------------------------------------------------------------------------- - -
|
|   Description: This function is used to determine if the given pathname is an
|                relative pathname.  Note that this is really just a test for a
|                leading '/'.
|
|   Return:      A value of TRUE is returned when the function is
|                successful, otherwise a value of FALSE is returned.
+-----------------------------------------------------------------------------*/

int db_isRelative_path ( const char *pathname )
{  int result;

   result = FALSE;
   if (0 < strlen(pathname))
      if (pathname[0] != '/')
         result = TRUE;
   return result;
}

/*-------------------------------------------------------------------------- - -
|
|   Description: This function joins the two given pathname components to form
|                a new pathname.  The result is normalized to deal properly
|                with absolute pathnames, `.' and `..' components.  For
|                example: joining "abc/def" and "../xyz/123" would result
|                in "abc/xyz/123".  Note that joining "abc/123" and "/xyz"
|                will yield "/xyz" since the second part is an absolute
|                path. Note that the first operand, a, is treated as the
|                "root" for any '.' or '..' in operand b.
|
|   Return:      A pointer to a NULL-terminated string is returned when this
|                function is successful.  Note that this string is constructed
|                from allocated dynamic memory, it is up to the caller to
|                release this string when it is no longer needed.  A 0
|                pointer is returned on error.
|
|   Modifications:
|
|     Mark C. Miller, Wed Oct 18 08:41:33 PDT 2006
|     Fixed bug where result was set at top of function but then uninitialized
|     tmp was tested at end causing result to be set to zero
+-----------------------------------------------------------------------------*/

char *db_join_path ( const char *a,
                     const char *b )
{  char       *result;
   char *tmp;

   if (strlen(b) == 0)
      return db_normalize_path(a);
   else if (strlen(a) == 0)
      return db_normalize_path(b);
   else if (db_isAbsolute_path(b))
      return db_normalize_path(b);
   else
   {  db_Pathname *Pa;

      tmp = 0;
      if ((Pa=db_split_path(a)) != 0)
      {  db_Pathname *Pb;

         if ((Pb=db_split_path(b)) != 0)
         {  db_Pathname *t;

            if ((t=(db_Pathname *)malloc(sizeof(db_Pathname))) != 0)
            {  db_PathnameComponent *c;
               int          ok;

               t->firstComponent = 0;
               t->lastComponent  = 0;
               ok                = TRUE;
               c                 = Pa->firstComponent;
               while (c != 0)
               {  db_PathnameComponent *k;

                  if ((k=(db_PathnameComponent *)malloc(sizeof(db_PathnameComponent))) != 0)
                  {  if (c->name != 0)
                        k->name                         = STRDUP(c->name);
                     else
                        k->name                         = 0;
                     k->prevComponent                   = t->lastComponent;
                     k->nextComponent                   = 0;
                     if (t->lastComponent == 0)
                        t->firstComponent               = k;
                     else
                        t->lastComponent->nextComponent = k;
                     t->lastComponent                   = k;
                  }
                  else
                  {  ok = FALSE;
                     break;
                  }
                  c = c->nextComponent;
               }
               if (ok)
               {  c = Pb->firstComponent;
                  while (c != 0)
                  {  db_PathnameComponent *k;

                     if ((k=(db_PathnameComponent *)malloc(sizeof(db_PathnameComponent))) != 0)
                     {  if (c->name != 0)
                           k->name                         = STRDUP(c->name);
                        else
                           k->name                         = 0;
                        k->prevComponent                   = t->lastComponent;
                        k->nextComponent                   = 0;
                        if (t->lastComponent == 0)
                           t->firstComponent               = k;
                        else
                           t->lastComponent->nextComponent = k;
                        t->lastComponent                   = k;
                     }
                     else
                     {  ok = FALSE;
                        break;
                     }
                     c = c->nextComponent;
                  }
                  if (ok)
                     tmp = db_unsplit_path(t);
               }
               db_cleanup_path(t);
            }
            db_cleanup_path(Pb);
         }
         db_cleanup_path(Pa);
      }
   }
   if (tmp != 0)
   {
      result = db_normalize_path(tmp);
      free(tmp);
   }
   else
      result = 0;
   return result;
}

/*-------------------------------------------------------------------------- - -
|
|   Description: This function is used to normalize the given pathname, dealing
|                with dots, double-dots and such.
|
|                This function resolves:
|
|                    - double slashes (such as abc//123)
|                    - trailing slashes (such as abc/)
|                    - embedded single dots (such as abc/./123) except for
|                      some leading dots.
|                    - name-double dot sets (such as abc/../123)
|
|   Return:      A pointer to a NULL-terminated string is returned when this
|                function is successful.  Note that this string is constructed
|                from allocated dynamic memory, it is up to the caller to
|                release this string when it is no longer needed.  A 0
|                pointer is returned on error.
|
+-----------------------------------------------------------------------------*/

char *db_normalize_path ( const char *pathname )
{  char *result;

   result = 0;
   if (0 < strlen(pathname))
   {  db_Pathname *p;

        /*--------------------------------------
        |
        |   Break into separate components...
        |
        +-------------------------------------*/

      if ((p=(db_split_path(pathname))) != 0)
      {  db_PathnameComponent *c;

        /*--------------------------------------
        |
        |   Eliminate . components
        |
        +-------------------------------------*/

         c = p->firstComponent;
         while (c != 0)
         {  if (c != p->firstComponent)
            {  if (c->name && (strcmp(c->name,".") == 0))
               {  db_PathnameComponent *cc;

                  cc                                 = c->nextComponent;
                  if (c->prevComponent == 0)
                     p->firstComponent               = c->nextComponent;
                  else
                     c->prevComponent->nextComponent = c->nextComponent;
                  if (c->nextComponent == 0)
                     p->lastComponent                = c->prevComponent;
                  else
                     c->nextComponent->prevComponent = c->prevComponent;
                  free(c->name);
                  c->name                            = 0;
                  c->prevComponent                   = 0;
                  c->nextComponent                   = 0;
                  free(c);
                  c                                  = cc;
               }
               else
                  c = c->nextComponent;
            }
            else
               c = c->nextComponent;
         }

        /*--------------------------------------
        |
        |   Eliminate .. components, note
        |   that this process is a little
        |   tougher then the . case, this
        |   is due to things like ../../a/b
        |
        +-------------------------------------*/

tryAgain:c = p->firstComponent;
         while (c != 0)
         {  if (c->name && (strcmp(c->name,"..") == 0))
            {  db_PathnameComponent *k;

               if ((k=c->prevComponent) != 0)
               {  if (k->name != 0 && strcmp(k->name,"..") != 0)
                  {
                     if (k->prevComponent == 0)
                        p->firstComponent                   = k->nextComponent;
                     else
                        k->prevComponent->nextComponent     = k->nextComponent;
                     if (k->nextComponent == 0)
                        p->lastComponent                    = k->prevComponent;
                     else
                        k->nextComponent->prevComponent     = k->prevComponent;
                     if (k->name != 0)
                        free(k->name);
                     k->name                                = 0;
                     k->prevComponent                       = 0;
                     k->nextComponent                       = 0;
                     free(k);
                     k                                      = 0;
                     if (c->prevComponent == 0)
                        p->firstComponent                   = c->nextComponent;
                     else
                        c->prevComponent->nextComponent     = c->nextComponent;
                     if (c->nextComponent == 0)
                        p->lastComponent                    = c->prevComponent;
                     else
                        c->nextComponent->prevComponent     = c->prevComponent;
                     if (c->name != 0)
                        free(c->name);
                     c->name                                = 0;
                     c->prevComponent                       = 0;
                     c->nextComponent                       = 0;
                     free(c);
                     c                                      = 0;
                     goto tryAgain;
                  }
               }
            }
            c = c->nextComponent;
         }

        /*--------------------------------------
        |
        |   Rejoin components into a string...
        |
        +-------------------------------------*/

         result = db_unsplit_path(p);
         db_cleanup_path(p);
      }
   }
   return result;
}

static db_Pathname *makePathname ( void )
{  db_Pathname *p;

   if ((p=(db_Pathname *)malloc(sizeof(db_Pathname))) != 0)
   {  p->firstComponent = 0;
      p->lastComponent  = 0;
   }
   return p;
}

static db_Pathname *appendComponent ( db_Pathname *p, char *s )
{  if (p == 0)
      p = makePathname();
   if (p != 0)
   {  db_PathnameComponent *c;

      if ((c=(db_PathnameComponent *)malloc(sizeof(db_PathnameComponent))) != 0)
      {  c->name                            = STRDUP(s);
         c->prevComponent                   = p->lastComponent;
         c->nextComponent                   = 0;
         if (p->lastComponent == 0)
            p->firstComponent               = c;
         else
            p->lastComponent->nextComponent = c;
         p->lastComponent                   = c;
      }
   }
   return p;
}

/*-------------------------------------------------------------------------- - -
|
|   Description: This function splits a given pathname into its components.
|                The split is generally made at the embedded slashes (/),
|                forming a linked list of pathname components.  For example
|                the pathname "abc/def/123/xyz" has four components: "abc",
|                "def", "123", and "xyz".  Note that the list of components
|                returned by this function is formed using allocated dynamic
|                memory and should be released using the db_cleanup_path()
|                function when it is no longer needed.  This function is
|                intended for internal use only.
|
|   Return:      A pointer to the first component of a list of pathname
|                components is returned when this function succeeds, otherwise
|                a 0 pointer is returned.
|
+-----------------------------------------------------------------------------*/

db_Pathname *db_split_path ( const char *pathname )
{  db_Pathname *result;

   result = 0;
   if (0 < strlen(pathname))
   {  if ((result=makePathname()) != 0)
      {  int  L;
         int  state;
         char tmp[32767];

         L      = 0;
         tmp[L] = '\0';
         state  = 0;
         for (;;)
         {  char c;

            c = *pathname;
            switch (state)
            { case 0: switch (c)
                      { case '\0': goto done;
                        case '/':  result        = appendComponent(result,0);
                                   state         = 1;
                                   break;
                        default:   L             = 0;
                                   tmp[L]        = c;
                                   L            += 1;
                                   tmp[L]        = '\0';
                                   state         = 2;
                                   break;
                      }
                      break;
              case 1: switch (c)
                      { case '\0': goto done;
                        case '/':  state         = 1;
                                   break;
                        default:   L             = 0;
                                   tmp[L]        = c;
                                   L            += 1;
                                   tmp[L]        = '\0';
                                   state         = 2;
                                   break;
                      }
                      break;
              case 2: switch (c)
                      { case '\0': result        = appendComponent(result,tmp);
                                   goto done;
                        case '/':  result        = appendComponent(result,tmp);
                                   state         = 1;
                                   break;
                        default:   tmp[L]        = c;
                                   L            += 1;
                                   tmp[L]        = '\0';
                                   state         = 2;
                                   break;
                      }
                      break;
            }
            pathname += 1;
         }
done:    ;
      }
   }
   return result;
}

/*------------------------------------------------------------------------------
|
|   Description: This function forms a pathname string from a linked set of
|                pathname components.  Note that this function is intended for
|                internal use only.
|
|   Modifications:
|     Mark C. Miller, Wed Jul 11 17:01:09 PDT 2012
|     Initialize tmp buffer to the empty string with tmp[0] = '\0'
+-----------------------------------------------------------------------------*/

char *db_unsplit_path ( const db_Pathname *p )
{  char *result;

   result = 0;
   if (p != 0)
   {  db_PathnameComponent *c;
      int          first;
      int          slashed;
      static char  tmp[4096];

      tmp[0] = '\0';
      first   = TRUE;
      slashed = FALSE;
      c       = p->firstComponent;
      while (c != 0)
      {
         if ((c->name == 0) || (strlen(c->name) == 0))
         {  strcpy(tmp,"/");
            slashed = TRUE;
         }
         else
         {  if ((!slashed) && (!first))
            {  strcat(tmp,"/");
               slashed = TRUE;
            }
            strcat(tmp,c->name);
            slashed = FALSE;
         }
         first = FALSE;
         c     = c->nextComponent;
      }
      result = STRDUP(tmp);
   }
   return result;
}

/*
 * END CODE FROM JIM REUS' DSL }
 */

static void
DBFreeMrgnode(DBmrgtnode *tnode, int walk_order, void *data)
{
    if (tnode == 0)
        return;
    FREE(tnode->name);
    if (tnode->narray > 0)
    {
        if (strchr(tnode->names[0], '%') == 0)
        {
            int i;
            for (i = 0; i < tnode->narray; i++)
                FREE(tnode->names[i]);
            FREE(tnode->names);
        }
        else
        {
            FREE(tnode->names[0]);
            FREE(tnode->names);
        }
    }
    FREE(tnode->maps_name);
    FREE(tnode->seg_ids);
    FREE(tnode->seg_lens);
    FREE(tnode->seg_types);
    FREE(tnode->children);
    FREE(tnode);
}

void DBFreeMrgtree(DBmrgtree *tree)
{
    if (tree == 0)
        return;
    DBWalkMrgtree(tree, (DBmrgwalkcb) DBFreeMrgnode, 0, DB_POSTORDER);
    FREE(tree->name);
    FREE(tree->src_mesh_name);
    if (tree->mrgvar_onames)
    {
        int i = 0;
        while (tree->mrgvar_onames[i] != 0)
        {
            FREE(tree->mrgvar_onames[i]);
            i++;
        }
        FREE(tree->mrgvar_onames);
    }
    if (tree->mrgvar_rnames)
    {
        int i = 0;
        while (tree->mrgvar_rnames[i] != 0)
        {
            FREE(tree->mrgvar_rnames[i]);
            i++;
        }
        FREE(tree->mrgvar_rnames);
    }
    FREE(tree);
}

void DBPrintMrgtree(DBmrgtnode *tnode, int walk_order, void *data)
{
    FILE *f = (FILE *) data;
    int level = -1;
    DBmrgtnode *tmp = tnode;

    /* walk to top to determine level of indentation */
    while (tmp != 0)
    {
        tmp = tmp->parent;
        level++;
    }
    level *= 3;

    if (f == 0)
        f = stdout;

    /* print this node using special '*' field width specifier */
    fprintf(f, "%*s name = \"%s\" {\n", level, "", tnode->name);
    fprintf(f, "%*s     walk_order = %d\n", level, "", tnode->walk_order);
    fprintf(f, "%*s         parent = \"%s\"\n", level, "", tnode->parent?tnode->parent->name:"");
    fprintf(f, "%*s         narray = %d\n", level, "", tnode->narray);
    if (tnode->narray > 0)
    {
        if (strchr(tnode->names[0], '%') == 0)
        {
            int j;
            fprintf(f, "%*s          names = ...\n", level, "");
            for (j = 0; j < tnode->narray; j++)
                fprintf(f, "%*s                  \"%s\"\n", level, "", tnode->names[j]);
        }
        else
        {
            fprintf(f, "%*s          names = \"%s\"\n", level, "", tnode->names[0]);
        }
    }
    fprintf(f, "%*s type_info_bits = %d\n", level, "", tnode->type_info_bits);
    fprintf(f, "%*s   max_children = %d\n", level, "", tnode->max_children);
    fprintf(f, "%*s      maps_name = \"%s\"\n", level, "", tnode->maps_name?tnode->maps_name:"");
    fprintf(f, "%*s          nsegs = %d\n", level, "", tnode->nsegs);
    if (tnode->nsegs > 0)
    {
        int j;
        fprintf(f, "%*s       segments =     ids   |   lens   |   types\n", level, "");
        for (j = 0; j < tnode->nsegs*(tnode->narray?tnode->narray:1); j++)
            fprintf(f, "%*s                  %.10d|%.10d|%.10d\n", level, "",
                tnode->seg_ids[j], tnode->seg_lens[j], tnode->seg_types[j]);

    }
    fprintf(f, "%*s   num_children = %d\n", level, "", tnode->num_children);
    if (tnode->num_children > 0)
    {
        int j;
        for (j = 0; j < tnode->num_children && tnode->children[j] != 0; j++)
            fprintf(f, "%*s              \"%s\"\n", level, "", tnode->children[j]->name);
    }
    fprintf(f, "%*s} \"%s\"\n", level, "", tnode->name);
}

void
DBLinearizeMrgtree(DBmrgtnode *tnode, int walk_order, void *data)
{
    DBmrgtnode **ltree = (DBmrgtnode **) data;
    ltree[walk_order] = tnode;
    tnode->walk_order = walk_order;
}

static void
DBWalkMrgtree_r(DBmrgtnode const *node, int *walk_order, DBmrgwalkcb wcb, void *wdata,
    int traversal_flags)
{
    if (node == 0)
        return;

    /* if we're at a terminal node, issue the callback */
    if (node->children == 0)
    {
       wcb(node, *walk_order, wdata);
       (*walk_order)++;
    }
    else
    {
        int i;

        /* issue callback first if in pre-order mode */
        if (traversal_flags & DB_PREORDER)
        {
            wcb(node, *walk_order, wdata);
            (*walk_order)++;
        }

        /* recurse on all the children */
        for (i = 0; i < node->num_children && node->children[i] != 0; i++)
            DBWalkMrgtree_r(node->children[i], walk_order, wcb, wdata,
                traversal_flags);

        /* issue callback last if in post-order mode */
        if (traversal_flags & DB_POSTORDER)
        {
            wcb(node, *walk_order, wdata);
            (*walk_order)++;
        }
    }
}

void
DBWalkMrgtree(DBmrgtree const *tree, DBmrgwalkcb cb, void *wdata, int traversal_flags)
{
    int walk_order = 0;
    DBmrgtnode *start = tree->root;

    if (cb == 0)
        return;

    if (traversal_flags & DB_FROMCWR)
        start = tree->cwr;

    DBWalkMrgtree_r(start, &walk_order, cb, wdata, traversal_flags);
}

PUBLIC DBmrgtree *
DBMakeMrgtree(int source_mesh_type, int type_info_bits,
    int max_root_descendents, DBoptlist *opts)
{
    DBmrgtree *tree = NULL;
    DBmrgtnode *root = NULL;

    API_BEGIN("DBMakeMrgtree", DBmrgtree *, NULL) {
        if (!(source_mesh_type == DB_MULTIMESH ||
              source_mesh_type == DB_QUADMESH ||
              source_mesh_type == DB_UCDMESH ||
              source_mesh_type == DB_POINTMESH ||
              source_mesh_type == DB_CSGMESH ||
              source_mesh_type == DB_CURVE))
            API_ERROR("source_mesh_type", E_BADARGS);
        if (type_info_bits != 0)
            API_ERROR("type_info_bits", E_BADARGS);
        if (max_root_descendents <= 0)
            API_ERROR("max_root_descendents", E_BADARGS);
        tree = ALLOC(DBmrgtree);
        if (!tree) API_ERROR(NULL, E_NOMEM);
        memset(tree, 0, sizeof(DBmrgtree));
        root = ALLOC(DBmrgtnode);
        if (!root) {
            FREE(tree);
            API_ERROR(NULL, E_NOMEM);
        }
        memset(root, 0, sizeof(DBmrgtnode));
        root->children = ALLOC_N(DBmrgtnode*, max_root_descendents);
        if (!root->children) {
            FREE(root);
            FREE(tree);
            API_ERROR(NULL, E_NOMEM);
        }

        /* fill in the tree header */
        tree->type_info_bits = type_info_bits;
        tree->src_mesh_type = source_mesh_type;
        tree->src_mesh_name = 0;
        tree->name = 0;

        /* update internal node info */
        root->walk_order = -1;
        root->parent = 0;

        /* update client data data */
        root->name = STRDUP("whole");
        root->narray = 0;
        root->names = 0;
        root->type_info_bits = 0;
        root->num_children = 0;
        root->max_children = max_root_descendents;
        root->maps_name = 0;
        root->nsegs = 0;
        root->seg_ids = 0;
        root->seg_lens = 0;
        root->seg_types = 0;

        /* add the new tnode to the tree */
        tree->root = root;
        tree->cwr = root;
        tree->num_nodes = 1;

        API_RETURN(tree);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC int
DBAddRegion(DBmrgtree *tree, const char *region_name,
    int type_info_bits, int max_descendents, 
    const char *maps_name, int nsegs, int const *seg_ids,
    int const *seg_lens, int const *seg_types, DBoptlist const *opts)
{
    DBmrgtnode *tnode = NULL;

    API_BEGIN("DBAddRegion", int, -1) {

        if (!tree)
            API_ERROR("tree pointer", E_BADARGS);
        if (!region_name || !*region_name)
            API_ERROR("region_name", E_BADARGS);
        if (type_info_bits != 0)
            API_ERROR("type_info_bits", E_BADARGS);
        if (max_descendents < 0)
            API_ERROR("max_descendents", E_BADARGS);
        if (tree->cwr->num_children >= tree->cwr->max_children) {
            API_ERROR("exceeded max_descendents", E_BADARGS);
        }
        if (nsegs > 0)
        {
            if (seg_ids == 0)
                API_ERROR("seg_ids", E_BADARGS);
            if (seg_lens == 0)
                API_ERROR("seg_lens", E_BADARGS);
            if (seg_types == 0)
                API_ERROR("seg_types", E_BADARGS);
        }
        if (NULL == (tnode = ALLOC(DBmrgtnode)))
            API_ERROR(NULL, E_NOMEM);
        memset(tnode, 0, sizeof(DBmrgtnode));
        if (NULL == (tnode->children = ALLOC_N(DBmrgtnode*, max_descendents)) &&
            max_descendents) {
            FREE(tnode);
            API_ERROR(NULL, E_NOMEM);
        }

        /* update internal node info */
        tnode->walk_order = -1;
        tnode->parent = tree->cwr;

        /* update client data data */
        tnode->name = STRDUP(region_name); 
        tnode->narray = 0;
        tnode->names = 0;
        tnode->type_info_bits = type_info_bits;
        tnode->num_children = 0;
        tnode->max_children = max_descendents;
        tnode->maps_name = STRDUP(maps_name);
        tnode->nsegs = nsegs;
        if (nsegs > 0)
        {
            int i;

            tnode->seg_ids = ALLOC_N(int, nsegs);
            tnode->seg_lens = ALLOC_N(int, nsegs);
            tnode->seg_types = ALLOC_N(int, nsegs);
 
            if (!tnode->seg_ids || !tnode->seg_lens || !tnode->seg_types)
            {
                FREE(tnode->seg_types);
                FREE(tnode->seg_lens);
                FREE(tnode->seg_ids);
                FREE(tnode->maps_name);
                FREE(tnode->name);
                FREE(tnode->children);
                FREE(tnode);
                API_ERROR(NULL, E_NOMEM);
            }

            for (i = 0; i < nsegs; i++)
            {
                tnode->seg_ids[i] = seg_ids[i];
                tnode->seg_lens[i] = seg_lens[i]; 
                tnode->seg_types[i] = seg_types[i];
            }
        }
        else
        {
            tnode->seg_ids = 0;
            tnode->seg_lens = 0;
            tnode->seg_types = 0;
        }


        /* add the new tnode to the tree */
        tree->cwr->children[tree->cwr->num_children] = tnode;
        tree->cwr->num_children++;
        tree->num_nodes++;

    }
    API_END;

    return(tree->cwr->num_children-1);
}

PUBLIC int
DBAddRegionArray(DBmrgtree *tree, int nregns,
    char const * const *regn_names, int type_info_bits,
    char const *maps_name, int nsegs, int const *seg_ids,
    int const *seg_lens, int const *seg_types, DBoptlist const *opts)
{
    DBmrgtnode *tnode = NULL;
    int i;

    API_BEGIN("DBAddRegionArray", int, -1) {

        if (!tree)
            API_ERROR("tree pointer", E_BADARGS);
        if (nregns <= 0)
            API_ERROR("nregns", E_BADARGS);
        if (tree->cwr->num_children + nregns > tree->cwr->max_children) {
            API_ERROR("exceeded max_descendents", E_BADARGS);
        }
        if (NULL == (tnode = ALLOC(DBmrgtnode)))
            API_ERROR(NULL, E_NOMEM);
        memset(tnode, 0, sizeof(DBmrgtnode));
        if (nsegs > 0)
        {
            if (seg_ids == 0)
                API_ERROR("seg_ids", E_BADARGS);
            if (seg_lens == 0)
                API_ERROR("seg_lens", E_BADARGS);
            if (seg_types == 0)
                API_ERROR("seg_types", E_BADARGS);
        }

        /* update internal node info */
        tnode->walk_order = -1;
        tnode->parent = tree->cwr;

        /* update client data data */
        tnode->name = 0;
        tnode->narray = nregns;
        if (strchr(regn_names[0], '%') != 0)
        {
            if (NULL == (tnode->names = ALLOC_N(char*, 1))) {
                FREE(tnode);
                API_ERROR(NULL, E_NOMEM);
            }
            tnode->names[0] = STRDUP(regn_names[0]);
        }
        else
        {
            if (NULL == (tnode->names = ALLOC_N(char*, nregns))) {
                FREE(tnode);
                API_ERROR(NULL, E_NOMEM);
            }
            for (i = 0; i < nregns; i++)
                tnode->names[i] = STRDUP(regn_names[i]);
        }
        tnode->type_info_bits = type_info_bits;
        tnode->num_children = 0;
        tnode->max_children = 0;
        tnode->children = 0;
        tnode->maps_name = STRDUP(maps_name);
        tnode->nsegs = nsegs;
        if (nsegs > 0)
        {
            tnode->seg_ids = ALLOC_N(int, nsegs*nregns);
            tnode->seg_lens = ALLOC_N(int, nsegs*nregns);
            tnode->seg_types = ALLOC_N(int, nsegs*nregns);

            if (!tnode->seg_ids || !tnode->seg_lens || !tnode->seg_types) {
                FREE(tnode->seg_types);
                FREE(tnode->seg_lens);
                FREE(tnode->seg_ids);
                if (strchr(regn_names[0], '%') != 0) {
                    FREE(tnode->names[0]);
                    FREE(tnode->names);
                } else {
                    for (i = 0; i < nregns; i++)
                        FREE(tnode->names[i]);
                    FREE(tnode->names);
                }
                FREE(tnode);
                API_ERROR(NULL, E_NOMEM);
            }

            for (i = 0; i < nsegs*nregns; i++)
            {
                tnode->seg_ids[i] = seg_ids[i];
                tnode->seg_lens[i] = seg_lens[i]; 
                tnode->seg_types[i] = seg_types[i];
            }
        }
        else
        {
            tnode->seg_ids = 0;
            tnode->seg_lens = 0;
            tnode->seg_types = 0;
        }

        /* add the new tnode to the tree */
        tree->cwr->children[tree->cwr->num_children] = tnode;
        tree->cwr->num_children++;
        tree->num_nodes++;

    }
    API_END;

    return(tree->cwr->num_children-1);
}

PUBLIC int
DBSetCwr(DBmrgtree *tree, char const *path)
{
    int retval = -1;

    API_BEGIN("DBSetCwr", int, -1)
    {
        if (tree == 0)
            API_ERROR("tree", E_BADARGS);
        if (!path || !*path)
            API_ERROR("path", E_BADARGS);
        if (path[0] == '.' && path[1] == '.')
        {
            DBmrgtnode *tnode = tree->cwr;
            if (tnode != tree->root)
            {
                tree->cwr = tnode->parent;
                retval = 1;
            }
        }
        else
        {
            DBmrgtnode *tnode = tree->cwr;
            int i = 0;
            while (i < tnode->num_children)
            {
                if (strcmp(tnode->children[i]->name, path) == 0)
                {
                    tree->cwr = tnode->children[i];
                    break;
                }
                i++;
            }
            if (i < tnode->num_children)
                retval = i;
        }
        API_RETURN(retval);
    }
    API_END_NOPOP;  /* BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC char const *
DBGetCwr(DBmrgtree *tree)
{
    const char *retval = NULL;

    API_BEGIN("DBGetCwr", const char *, NULL)
    {
        if (tree == 0)
            API_ERROR("tree", E_BADARGS);

        retval = tree->cwr->name; 
        API_RETURN(retval);
    }
    API_END_NOPOP;  /* BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC int
DBPutMrgtree(DBfile *dbfile, char const *name, char const *mesh_name,
    DBmrgtree const *tree, DBoptlist const *opts)
{
    int retval;

    API_BEGIN2("DBPutMrgtree", int, -1, name)
    {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutMrgtree", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("mrgtree name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("mrgtree name", E_INVALIDNAME);
        if (!mesh_name || !*mesh_name)
            API_ERROR("mesh_name", E_BADARGS);
        if (db_VariableNameValid(mesh_name) == 0)
            API_ERROR("mesh_name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (NULL == dbfile->pub.p_mrgt)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_mrgt) (dbfile, name, mesh_name,
                                       tree, opts); 

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /* BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC DBmrgtree *
DBGetMrgtree(DBfile *dbfile, const char *name)
{
    DBmrgtree *retval = NULL;

    API_BEGIN2("DBGetMrgtree", DBmrgtree *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetMrgtree", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("mrgtree name", E_BADARGS);
        if (!dbfile->pub.g_mrgt)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_mrgt) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC int
DBPutGroupelmap(DBfile *dbfile, const char *name,
    int num_segments, int const *groupel_types, int const *segment_lengths,
    int const *segment_ids, int const * const *segment_data, DBVCP2_t segment_fracs,
    int fracs_data_type, DBoptlist const *opts)
{
    int retval;

    API_BEGIN2("DBGroupelmap", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutGroupelmap", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("groupel map name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("groupel map name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (num_segments < 0)
            API_ERROR("num_segments", E_BADARGS);
        if (!groupel_types)
            API_ERROR("groupel_types", E_BADARGS);
        if (!segment_lengths)
            API_ERROR("segment_lengths", E_BADARGS);
        if (!segment_data)
            API_ERROR("segment_data", E_BADARGS);
        if (!dbfile->pub.p_grplm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_grplm) (dbfile, name,
            num_segments, groupel_types, segment_lengths, segment_ids,
            segment_data, (void const * const *) segment_fracs, fracs_data_type, opts);

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC DBgroupelmap *
DBGetGroupelmap(DBfile *dbfile, const char *name)
{
    DBgroupelmap *retval = NULL;

    API_BEGIN2("DBGetGroupelmap", DBgroupelmap *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetGroupelmap", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("groupel map name", E_BADARGS);
        if (!dbfile->pub.g_grplm)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_grplm) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC int
DBPutMrgvar(DBfile *dbfile, const char *name, const char *mrgt_name,
    int ncomps, char const * const *compnames, int nregns, char const * const *reg_pnames,
    int datatype, DBVCP2_t data, DBoptlist const *opts)
{
    int retval;

    API_BEGIN2("DBPutMrgvar", int, -1, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBPutMrgvar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("mrgvar name", E_BADARGS);
        if (db_VariableNameValid(name) == 0)
            API_ERROR("mrgvar name", E_INVALIDNAME);
        if (!mrgt_name || !*mrgt_name)
            API_ERROR("mrgt_name", E_BADARGS);
        if (db_VariableNameValid(mrgt_name) == 0)
            API_ERROR("mrgt_name", E_INVALIDNAME);
        if (!DBGetAllowOverwritesFile(dbfile) && DBInqVarExists(dbfile, name))
            API_ERROR("overwrite not allowed", E_NOOVERWRITE);
        if (nregns < 0)
            API_ERROR("nregns", E_BADARGS);
        if (ncomps < 0)
            API_ERROR("ncomps", E_BADARGS);
        if (!reg_pnames)
            API_ERROR("reg_pnames", E_BADARGS);
        if (!data)
            API_ERROR("data", E_BADARGS);
        if (!dbfile->pub.p_mrgv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.p_mrgv) (dbfile, name, mrgt_name, ncomps,
            compnames, nregns, reg_pnames, datatype, (void const * const *) data, opts);

        db_FreeToc(dbfile);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

PUBLIC DBmrgvar *
DBGetMrgvar(DBfile *dbfile, const char *name)
{
    DBmrgvar *retval = NULL;

    API_BEGIN2("DBGetMrgvar", DBmrgvar *, NULL, name) {
        if (!dbfile)
            API_ERROR(NULL, E_NOFILE);
        if (SILO_Globals.enableGrabDriver == TRUE)
            API_ERROR("DBGetMrgvar", E_GRABBED) ; 
        if (!name || !*name)
            API_ERROR("mrgvar name", E_BADARGS);
        if (!dbfile->pub.g_mrgv)
            API_ERROR(dbfile->pub.name, E_NOTIMP);

        retval = (dbfile->pub.g_mrgv) (dbfile, name);
        API_RETURN(retval);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}
/**********************************************************************
 *
 * Purpose: Provide a strdup command which correctly handles
 *          a NULL string.
 *
 * Programmer: Sean Ahern
 * Date: April 1999
 *
 * Input arguments:                                               
 *    s             The string to copy.
 *  
 * Global variables:
 *    None
 *      
 * Local variables:
 *    retval        The new string, with memory allocated.
 *      
 * Assumptions and Comments:
 *
 * Modifications:
 *
 *    Lisa J. Roberts, Tue Jul 27 12:44:57 PDT 1999
 *    Modified the function so it returns the string.
 *  
 *    Jeremy Meredith, Tue Aug 31 13:41:29 PDT 1999
 *    Made it handle 0-length strings correctly.
 *  
 *       Thomas R. Treadway, Wed Nov 28 15:25:53 PST 2007
 *       Moved from src/swat/sw_string.c
 *
 ***********************************************************************/
char *  
_db_safe_strdup(const char *s)
{
    char *retval = NULL;
    int n;
    
    if (!s) 
        return NULL;
            
    n = strlen(s);
    retval = (char*)malloc(sizeof(char)*(n+1));
    memcpy(retval, s, n);
    retval[n] = '\0';

    return(retval);
}

/*-------------------------------------------------------------------------
 * Function:    different
 *
 * Purpose:     Determines if A and B are same or different based on an
 *              absolute tolerance and relative tolerance.  A and B differ
 *              if and only if
 *
 *                      | A-B | > ABSTOL
 *
 *              or
 *
 *                      2 | A-B |
 *                      ---------  > RELTOL
 *                       | A+B |
 *
 *              If ABSTOL or RELTOL is negative then the corresponding
 *              test is not performed.  If both are negative then this
 *              function degenerates to a `!=' operator.
 *
 * Return:      Success:        0 if same, 1 if different.
 *
 *              Failure:        never fails
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Feb  6 1997
 *
 * Modifications:
 *
 *  Mark C. Miller, Wed Nov 11 22:18:17 PST 2009
 *  Added suppot for alternate relative diff option.
 *
 * Mark C. Miller, Tue Feb  7 15:18:38 PST 2012
 * Made reltol_eps diff mutually exclusive with abstol || reltol diff.
 *
 * Mark C. Miller, Mon Oct 24, 23:00:00 PDT 2022
 * Handle nans properly
 *-------------------------------------------------------------------------
 */
int DBIsDifferentDouble(double a, double b, double abstol, double reltol, double reltol_eps)
{
   double       num, den;

   /* handle possible NaNs first */
   if (isnan(a))
   {
       if (isnan(b)) return 0;
       return 1;
   }
   else if (isnan(b))
   {
       return 1;
   }

   /*
    * First, see if we should use the alternate diff.
    * check |A-B|/(|A|+|B|+EPS) in a way that won't overflow.
    */
   if (reltol_eps >= 0 && reltol > 0)
   {
      if ((a<0 && b>0) || (b<0 && a>0)) {
         num = fabs (a/2 - b/2);
         den = fabs (a/2) + fabs(b/2) + reltol_eps/2;
         reltol /= 2;
      } else {
         num = fabs (a - b);
         den = fabs (a) + fabs(b) + reltol_eps;
      }
      if (0.0==den && num) return 1;
      if (num/den > reltol) return 1;
      return 0;
   }
   else /* use the old Abs|Rel difference test */
   {
      /*
       * Now the |A-B| but make sure it doesn't overflow which can only
       * happen if one is negative and the other is positive.
       */
      if (abstol>0) {
         if ((a<0 && b>0) || (b<0 && a>0)) {
            if (fabs (a/2 - b/2) > abstol/2) return 1;
         } else {
            if (fabs(a-b) > abstol) return 1;
         }
      }

      /*
       * Now check 2|A-B|/|A+B| in a way that won't overflow.
       */
      if (reltol>0) {
         if ((a<0 && b>0) || (b<0 && a>0)) {
            num = fabs (a/2 - b/2);
            den = fabs (a/2 + b/2);
            reltol /= 2;
         } else {
            num = fabs (a - b);
            den = fabs (a/2 + b/2);
         }
         if (0.0==den && num) return 1;
         if (num/den > reltol) return 1;
      }

      if (abstol>0 || reltol>0) return 0;
   }

   /*
    * Otherwise do a normal exact comparison.
    */
   return a!=b;
}

/*-------------------------------------------------------------------------
 * Function:    differentll
 *
 * Purpose:     Implement above difference function for long long type. 
 *
 * Programmer:  Mark C. Miller, Mon Dec  7 07:05:39 PST 2009
 *
 * Modifications:
 *   Mark C. Miller, Mon Dec  7 09:50:19 PST 2009
 *   Change conditional compilation logic to compile this routine
 *   whenever a double is NOT sufficient to hold full precision of long
 *   or long long.
 *
 *   Mark C. Miller, Mon Jan 11 16:20:16 PST 2010
 *   Made it compiled UNconditionally.
 *-------------------------------------------------------------------------
 */
#define FABS(A) ((A)<0?-(A):(A))
int DBIsDifferentLongLong(long long a, long long b, double abstol, double reltol, double reltol_eps)
{

   long long num, den;

   /*
    * First, see if we should use the alternate diff.
    * check |A-B|/(|A|+|B|+EPS) in a way that won't overflow.
    */
   if (reltol_eps >= 0 && reltol > 0)
   {
      if ((a<0 && b>0) || (b<0 && a>0)) {
         num = FABS (a/2 - b/2);
         den = FABS (a/2) + FABS(b/2) + reltol_eps/2;
         reltol /= 2;
      } else {
         num = FABS (a - b);
         den = FABS (a) + FABS(b) + reltol_eps;
      }
      if (0.0==den && num) return 1;
      if (num/den > reltol) return 1;
      return 0;
   }
   else
   {
      /*
       * Now the |A-B| but make sure it doesn't overflow which can only
       * happen if one is negative and the other is positive.
       */
      if (abstol>0) {
         if ((a<0 && b>0) || (b<0 && a>0)) {
            if (FABS(a/2 - b/2) > abstol/2) return 1;
         } else {
            if (FABS(a-b) > abstol) return 1;
         }
      }

      /*
       * Now check 2|A-B|/|A+B| in a way that won't overflow.
       */
      if (reltol>0) {
         if ((a<0 && b>0) || (b<0 && a>0)) {
            num = FABS (a/2 - b/2);
            den = FABS (a/2 + b/2);
            reltol /= 2;
         } else {
            num = FABS (a - b);
            den = FABS (a/2 + b/2);
         }
         if (0.0==den && num) return 1;
         if (num/den > reltol) return 1;

         if (abstol>0 || reltol>0) return 0;
      }
   }

   /*
    * Otherwise do a normal exact comparison.
    */
   return a!=b;
}

PUBLIC char *
DBGetDatatypeString(int dt)
{
    return db_GetDatatypeString(dt);
}

PUBLIC int
db_fix_obsolete_centering(int ndims, float const *align, int carfm)
{
   int centering = carfm;
   if (carfm== DB_NOTCENT && align)
   {
       if (ndims == 1)
       {
           if (align[0] == 0)
               centering = DB_NODECENT;
           else if (align[0] == 0.5)
               centering = DB_ZONECENT;
       }
       else if (ndims == 2)
       {
           if (align[0] == 0 && align[1] == 0)
               centering = DB_NODECENT;
           else if (align[0] == 0.5 && align[1] == 0.5)
               centering = DB_ZONECENT;
       }
       else if (ndims == 3)
       {
           if (align[0] == 0 && align[1] == 0 && align[2] == 0)
               centering = DB_NODECENT;
           else if (align[0] == 0.5 && align[1] == 0.5 && align[2] == 0.5)
               centering = DB_ZONECENT;
       }
   }
   return centering;
}
