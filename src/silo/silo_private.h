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

/*
 * SILO Private header file.
 *
 * This file contains the private parts of SILO and is included
 * by every SILO source file.
 */
#ifndef SILO_PRIVATE_H
#define SILO_PRIVATE_H

#include "config.h" /*silo configuration settings */
#if HAVE_STDLIB_H
#include <stdlib.h> /*for malloc,calloc,realloc, etc */
#endif
#include <setjmp.h> /*for unwind_protect(), etc */
#if HAVE_STRING_H
#include <string.h> /*for STR_BEGINSWITH, etc */
#endif
#if defined(_WIN32)
#include <silo_win32_compatibility.h>
#else
#if HAVE_UNISTD_H
#include <unistd.h> /*for access() F_OK, R_OK */
#endif
#endif
#include "silo.h"

/*
 * The error mechanism....
 *
 * All application programming interface (API) function bodies are
 * surrounded by an `API_BEGIN' and an `API_END.'  Inside this construct,
 * the `return' statement should not be used is it would result in
 * leaving an invalid jump structure on the jump stack, Jstk.
 * If an error occurs which causes db_perror() to longjump back to the
 * inside of the API_BEGIN macro, the API_BEGIN macro will remove the invalid
 * entries from the jump stack.  But just to be safe, all occurances of
 * a `return' statement inside the API_BEGIN/API_END construct should be
 * coded as an `API_RETURN(x)' macro call.  As a convenience, returning
 * the failure value as registered with API_BEGIN may be done by calling
 * API_ERROR().  API_BEGIN and API_BEGIN2 cannot be used in routines that
 * are recursive.  This is because there are two local variables that are
 * declared static so that setjmp and longjmp work properly.
 *
 * Synopsis:
 *
 *  API_BEGIN (name, type, failure) {
 *
 *    if (...) API_ERROR(string, errno) ;
 *
 *    API_RETURN (retval) ;
 *
 *  } API_END ;
 *
 * Where:
 *   `name' is a string that is the name of the API function.
 *
 *   `type' is the API function return type.
 *
 *   `failure' is the API function failure return value.
 *
 *   `string' is an error message auxilliary string--1st arg to db_perror()
 *
 *   `errno' is the error number--one of the E_... constants.
 *
 *   `retval' is a return value, which can be a function call and the
 *   function call is protected by the API_BEGIN/API_END.
 *
 * Example:
 *
 * > This is an application program interface function that calls
 * > one of the functions in the `DBOpenCB' callback vector.  The
 * > callback might fail by calling db_perror() in which case this
 * > API function might be responsible for issuing the error message.
 *
 *  PUBLIC DBfile *DBOpen (char *name, int type, int mode) {
 *
 *    API_BEGIN ("DBOpen", DBfile*, NULL) {
 *       if (!name || !*name) API_ERROR ("name", E_BADARG) ;
 *       if (!DBOpenCB[type]) API_ERROR (NULL, E_NOTIMP) ;
 *       API_RETURN ((DBOpenCB[type])(name, mode)) ;
 *    } API_END ;
 * }
 *
 *
 * > This is an example `open' callback function for the PDB device
 * > driver.  The function allocates some memory on the heap and then
 * > calls functions that might fail via db_perror().  The failures
 * > cause the clean-up body to be executed.  The clean-up code is
 * > activated in one of three ways:
 * >   Implicitly by calling db_perror() which may longjmp to the
 * >     cleanup section.
 * >   Implicitly by calling a function that might call db_perror()
 * >     that might longjmp to the cleanup section (possibly after
 * >     going through other clean-up sections on the jump stack)
 * >   Explicitly through the UNWIND() macro.  A `return' statement
 * > should never appear in a PROTECT/CLEANUP/END_PROTECT construct
 * > as this would bypass the END_PROTECT.
 * >
 * > SILO_CALLBACK int db_pdb_open (char *name, int mode)
 * > {
 * >     char  *path=NULL, *fullname=NULL;
 * >     int  fd;
 * >     
 * >     PROTECT {
 * >         path = search_paths (name);
 * >         fullname = MALLOC (strlen(path)+strlen(name)+2);
 * >         if (!fullname) {
 * >             db_perror (NULL, E_NOMEM, "db_pdb_open");
 * >             UNWIND();
 * >         }
 * >         sprintf (fullname, "%s/%s", path, name);
 * >         fd = db_pdb_open_file (fullname);
 * >         if (fd<0)
 * >            UNWIND();
 * >     } CLEANUP {
 * >             FREE (path);
 * >             FREE (fullname);
 * >     } END_PROTECT;
 * >     return fd;
 * > }
 * 
 *
 * API_END_NOPOP should be used in place of API_END if an API_RETURN is
 * used in the body and will always be executed.  This is to eliminate
 * warning messages from some compilers that the API_END coding will never
 * be reached.
 *
 * Why is it needed/How does it work?
 *
 * SILO has a function, DBShowErrors, that sets a flag to indicate what
 * level of error reporting should be used.  The DB_TOP level indicates
 * that when an error occurs deep within SILO or a device driver, that
 * the error should actually be reported by the API function called by
 * the application.  The jump stack is the mechanism used to transfer
 * the error signal to the top-level API function without the need for
 * device driver developers to constantly worry about the error handling
 * mechanism.
 *
 * Each time an API function is called and the global jump stack, Jstk, is
 * empty (as it should be at the application level) the API function will
 * call setjmp() and add the resulting jump buffer to the jump stack.  The
 * API_END macro conditionally removes this item from the jump stack.  When
 * an error is detected in the API function or in any SILO or device driver
 * function in the call stack below the API function, and db_perror is
 * called as a result of the error, and the error reporting level is
 * set to DB_TOP, a call to longjmp() will occur with the contents of the
 * jump buffer at the top of the jump stack.  This causes the corresponding
 * setjmp() call to return again, and results in db_perror() being called
 * by the API_BEGIN macro whose setjmp() returned.  The top (only) item from
 * the jump stack is removed and the API function returns an error status.
 *
 * A jump `stack' is used instead of a scalar jump variable so that any
 * function can register cleanup code to be executed as we are longjmp'ing
 * back to the top-level API.  This typically includes freeing memory and
 * continuing the jumping.  However, this mechanism can also be used
 * to tentatively execute a piece of code without issuing error messages
 * and then, based on whether an error actually occured, execute some other
 * chunk of code instead.
 *
 * The `CANCEL_UNWIND' statement can be used in the CLEANUP body to
 * indicate that the cleanup code has handled the error and we shouldn't
 * continue the unwind process.  This can be used for a piece of code
 * we expect to fail as in:
 *
 *  PROTECT {
 *    int status = expected_to_fail();
 * } CLEANUP {
 *    fix_the_failure();
 *    CANCEL_UNWIND;
 * } END_PROTECT;
 *      -- we continue here regardless of status --
 */
typedef struct jstk_t {
    struct jstk_t *prev;
    jmp_buf        jbuf;
} jstk_t;

typedef struct context_t {
    int            dirid;
    char          *name;
} context_t;

#define jstk_push()     {jstk_t*jt=ALLOC(jstk_t);jt->prev=SILO_Globals.Jstk;SILO_Globals.Jstk=jt;}
#define jstk_pop()      if(SILO_Globals.Jstk){jstk_t*jt=SILO_Globals.Jstk;SILO_Globals.Jstk=SILO_Globals.Jstk->prev;FREE(jt);}

#define DEPRECATE_MSG(M,Maj,Min,Alt)                                          \
{                                                                             \
   static int ncalls = 0;                                                     \
   if (ncalls < SILO_Globals.maxDeprecateWarnings) {                          \
      fprintf(stderr, "Silo warning %d of %d: \"%s\" was deprecated in version %d.%d.\n", \
          ncalls+1, SILO_Globals.maxDeprecateWarnings, M,Maj,Min);            \
      if (strlen(Alt) > 0)                                                                \
          fprintf(stderr, "Use \"%s\" instead\n", Alt);                       \
      fprintf(stderr, "Use DBSetDeprecateWarnings(0) to disable this message.\n");    \
      fflush(stderr);                                                         \
   }                                                                          \
   ncalls++;                                                                  \
}                                                                             \

#define API_DEPRECATE(M,T,R,Maj,Min,Alt)                                      \
   DEPRECATE_MSG(M,Maj,Min,Alt)                                               \
   API_BEGIN(M,T,R)

#define API_BEGIN(M,T,R) {                                                    \
                        char    *me = M ;                                     \
                        static int     jstat ;                                \
                        static context_t *jold ;                              \
                        DBfile  *jdbfile = NULL ;                             \
                        T jrv = R ;                                           \
                        jstat = 0 ;                                           \
                        jold = NULL ;                                         \
                        if (DBDebugAPI>0) {                                   \
                           size_t _nbyt;                                      \
                           _nbyt = write (DBDebugAPI, M, strlen(M));          \
                           _nbyt = write (DBDebugAPI, "\n", 1);               \
                        }                                                     \
                        if (!SILO_Globals.Jstk){                              \
                           jstk_push() ;                                      \
                           if (setjmp(SILO_Globals.Jstk->jbuf)) {             \
                              while (SILO_Globals.Jstk) jstk_pop () ;         \
                              db_perror ("", db_errno, me) ;                  \
                              return R ;                                      \
                           }                                                  \
                           jstat = 1 ;                                        \
                        }

#define API_DEPRECATE2(M,T,R,NM,Maj,Min,Alt)                                  \
   DEPRECATE_MSG(M,Maj,Min,Alt)                                               \
   API_BEGIN2(M,T,R,NM)

#define API_BEGIN2(M,T,R,NM) {                                                \
                        char    *me = M ;                                     \
                        static int     jstat ;                                \
                        static context_t *jold ;                              \
                        DBfile  *jdbfile = dbfile ;                           \
                        T jrv = R ;                                           \
                        jstat = 0 ;                                           \
                        jold = NULL ;                                         \
                        if (db_isregistered_file(dbfile,0) == -1)             \
                        {                                                     \
                            db_perror("", E_NOTREG, me);                      \
                            return R;                                         \
                        }                                                     \
                        if (DBDebugAPI>0) {                                   \
                           size_t _nbyt;                                      \
                           _nbyt = write (DBDebugAPI, M, strlen(M));          \
                           _nbyt = write (DBDebugAPI, "\n", 1);               \
                        }                                                     \
                        if (!SILO_Globals.Jstk){                              \
                           jstk_push() ;                                      \
                           if (setjmp(SILO_Globals.Jstk->jbuf)) {             \
                              if (jold) {                                     \
                                 context_restore (jdbfile, jold) ;            \
                              }                                               \
                              while (SILO_Globals.Jstk) jstk_pop () ;         \
                              db_perror ("", db_errno, me) ;                  \
                              return R ;                                      \
                           }                                                  \
                           jstat = 1 ;                                        \
                           if (NM && jdbfile && !jdbfile->pub.pathok) {       \
                              char const *jr ;                                \
                              jold = context_switch (jdbfile,NM,&jr) ;        \
                              if (!jold) longjmp (SILO_Globals.Jstk->jbuf, -1) ;\
                              NM = jr ;                                       \
                           }                                                  \
                        }

#define API_END         if (jold) context_restore (jdbfile, jold) ;     \
                        if (jstat) jstk_pop() ;                         \
                     }                        /*API_BEGIN or API_BEGIN2 */

#define API_END_NOPOP   }         /*API_BEGIN or API_BEGIN2 */

#define API_ERROR(S,N)  {                                               \
                           db_perror (S,N,me) ; /*might never return*/  \
                           if (jold) context_restore (jdbfile, jold) ;  \
                           if (jstat) jstk_pop() ;                      \
                           return jrv ;                                 \
                        }

#define API_RETURN(R)   {                                               \
                           jrv = R ; /*might be a calculation*/         \
                           if (jold) context_restore (jdbfile, jold) ;  \
                           if (jstat) jstk_pop() ;                      \
                           return jrv ;                                 \
                        }

#define PROTECT         {jstk_push();if(!setjmp(SILO_Globals.Jstk->jbuf)){
#define UNWIND()        longjmp(SILO_Globals.Jstk->jbuf,-1)
#define CLEANUP         jstk_pop();}else{int jcan=0;
#define END_PROTECT     jstk_pop();if(!jcan&&SILO_Globals.Jstk)longjmp(SILO_Globals.Jstk->jbuf,-1);}}
#define CANCEL_UNWIND   jcan=1

/*
 * Some convenience macros.
 * We define some new function return types:
 *    PRIVATE -- the function is used internally by defining file only.
 *    INTERNAL -- the function is public, but not part of the API.
 *    SILO_CALLBACK -- the function is a callback--never called directly
 *    PUBLIC -- the function can be called publicly
 *    FORTRAN -- the function is part of the fortran interface.
 */
#define PRIVATE         static
#define INTERNAL                /*semi-private */
#ifndef CALLBACK
#define SILO_CALLBACK        static
#endif
#define PUBLIC                  /*public */
#define FORTRAN         int

#define OOPS            -1                /*DONT CHANGE THIS */
#define OKAY            0                 /*DONT CHANGE THIS */
#define MAXDIMS_VARWRITE 7
#define OVER_WRITE      0x0001            /*overwrite DBobject */
#define FREE_MEM        0x0002            /*free DBobject memory */
#define NELMTS(X)       ((int)(sizeof(X)/sizeof((X)[0])))  /*Number of elements */

#define STR_EQUAL(S1,S2) (!strcmp((S1),(S2)))
#define STR_BEGINSWITH(S,P) ((strstr((S),(P))==(S))?1:0)
#define STR_LASTCHAR(S) ((S)[strlen((S))-1])
#define STR_HASCHAR(S,C) (strchr((S),(C))?1:0)
#define copy_var(FROM,TO,N) (memmove((TO),(FROM),(N)))

/*
 * Memory management macros.  All memory allocated by device
 * drivers which is visible to any other device driver or
 * SILO proper must be allocated with the C library memory
 * management (malloc, calloc, realloc, strdup, free...).
 */
#define ALLOC(T)                ((T*)calloc((size_t)1,sizeof(T)))
#define ALLOC_N(T,N)            ((T*)((N)>0?calloc((size_t)(N),sizeof(T)):0))
#define REALLOC(P,T,N)  REALLOC_N((P),(T),(N))
#define REALLOC_N(P,T,N)        ((T*)((N)>0?realloc((P),(size_t)((N)*sizeof(T))):0))
#define FREE(M)         if(M){free(M);(M)=NULL;}
#define STRDUP(S)               _db_safe_strdup((S))
#define STRNDUP(S,N)            db_strndup((S),(N))

#define SW_strndup(S,N) db_strndup((S),(N))
#define SW_GetDatatypeString(N) db_GetDatatypeString((N))
#define SW_GetDatatypeID(S) db_GetDatatypeID((S))
#define SW_file_exists(S)       (access((S),F_OK)>=0?1:0)
#define SW_file_readable(S)     (access((S),R_OK)>=0?1:0)
#define INDEX(col,row,ncol) (((row)*(ncol))+(col))  /* Zero - origin ! */
#define INDEX3(i,j,k,nx,nxy) ((k)*(nxy)+(j)*(nx)+(i))  /* Zero - origin ! */

#ifndef MAX
#define MAX(X,Y)        ((X)>(Y)?(X):(Y))
#define MIN(X,Y)        ((X)<(Y)?(X):(Y))
#endif

#ifdef   DEREF
#undef   DEREF
#endif
#define  DEREF(type,x)       (* ((type *) (x)))

/*
 * File status, maintained by DBOpen, DBCreate, and DBClose.
 */
#define DB_ISOPEN       0x01          /*database is open; ID is in use */

#define db_SetMissingValueForGet(DST, SRC) \
{                                          \
    DST = SRC;                             \
    if (DST == DB_MISSING_VALUE_NOT_SET)   \
        DST = 0.0;                         \
    else if (DST == 0.0)                   \
        DST = DB_MISSING_VALUE_NOT_SET;    \
}

#define db_SetMissingValueForPut(DST, SRC) \
{                                          \
    if (SRC != DB_MISSING_VALUE_NOT_SET)   \
    {                                      \
        if (SRC == 0)                      \
            DST = DB_MISSING_VALUE_NOT_SET;\
        else                               \
            DST = SRC;                     \
    }                                      \
}

/*
 * Global data for Material
 */
struct _ma {
    int            _origin;
    int            _majororder;
    char         **_matnames;
    char         **_matcolors;
    int            _allowmat0;
    int            _guihide;
};

/*
 * Global data for Material Species.
 */
struct _ms {
    int            _majororder;
    int            _guihide;
    char         **_specnames;
    char         **_speccolors;
};

/*
 * Global data for PointMesh and PointVar objects.
 */
struct _pm {
    float          _time;
    int            _time_set;
    double         _dtime;
    int            _dtime_set;
    int            _cycle;
    int            _hi_offset;
    int            _lo_offset;
    int            _ndims;
    int            _nspace;
    int            _nels;
    int            _origin;
    int            _minindex;
    int            _maxindex;
    char          *_label;
    char          *_unit;
    char          *_labels[3];
    char          *_units[3];
    char          *_coordnames[3];
    char           _nm_time[64];
    char           _nm_dtime[64];
    char           _nm_cycle[64];
    int            _group_no;
    int            _guihide;
    int            _ascii_labels;
    int           *_gnodeno;
    char          *_mrgtree_name;
    char         **_region_pnames;
    int            _llong_gnodeno;
    int            _conserved;
    int            _extensive;
    double         _missing_value;
    char          *_ghost_node_labels;
    char         **_alt_nodenum_vars;

    /*These used only by NetCDF driver */
    int            _dim_ndims;
    int            _dim_nels;
    int            _dim_scalar;
    int            _dim_2Xndims;
    int            _dim_nspace;
    int            _id_time;
    int            _id_dtime;
    int            _id_cycle;
};

/*
 * Global data for Quadmesh and Quadvar objects.
 */
struct _qm {
    float          _time;
    int            _time_set;
    double         _dtime;
    int            _dtime_set;
    float          _align[3];
    int            _cycle;
    int            _coord_sys;
    int            _facetype;
    int            _hi_offset[3];
    int            _lo_offset[3];
    int            _majororder;
    int            _ndims;
    int            _nspace;
    int            _nnodes;
    int            _nzones;
    int            _origin;
    int            _planar;
    int            _dims[3];
    int            _zones[3];
    int            _minindex[3];
    int            _maxindex_n[3];
    int            _maxindex_z[3];
    int            _nmat;
    int            _use_specmf;
    int            _ascii_labels;
    char          *_label;
    char          *_unit;
    char          *_labels[3];
    char          *_units[3];
    int            _baseindex[3];
    int            _baseindex_set;
    int            _group_no;
    int            _guihide;
    char          *_mrgtree_name;
    char         **_region_pnames;
    int            _conserved;
    int            _extensive;
    double         _missing_value;
    char          *_ghost_node_labels;
    char          *_ghost_zone_labels;
    char         **_alt_nodenum_vars;
    char         **_alt_zonenum_vars;

    /* These are probably only used by the pdb driver */
    char           _nm_dims[64];
    char           _nm_zones[64];
    char           _nm_alignz[64];
    char           _nm_alignn[64];
    char           _nm_time[64];
    char           _nm_dtime[64];
    char           _nm_cycle[64];
    char           _nm_minindex[64];
    char           _nm_maxindex_n[64];
    char           _nm_maxindex_z[64];
    char           _nm_baseindex[64];

    /*These are used only by the NetCDF driver */
    int            _dim_nnode[3];
    int            _dim_nzone[3];
    int            _dim_ndims;
    int            _id_dims;
    int            _id_minindex;
    int            _id_maxindex_n;
    int            _id_maxindex_z;
    int            _id_time;
    int            _id_dtime;
    int            _id_alignn;
    int            _id_alignz;
    int            _id_zones;

};

/*
 * Global data for Ucdmesh
 */
struct _um {
    float          _time;
    int            _time_set;
    double         _dtime;
    int            _dtime_set;
    float          _align[3];
    int            _cycle;
    int            _hi_offset;
    int            _lo_offset;
    int            _hi_offset_set;
    int            _lo_offset_set;
    int            _coord_sys;
    int            _topo_dim;
    int            _facetype;
    int            _ndims;
    int            _nspace;
    int            _nnodes;
    int            _nzones;
    int            _origin;
    int            _planar;
    int            _dims[3];
    int            _zones[3];
    int            _nmat;
    int            _use_specmf;
    int            _ascii_labels;
    char          *_label;
    char          *_unit;
    char          *_labels[3];
    char          *_units[3];
    char           _nm_dims[64];
    char           _nm_zones[64];
    char           _nm_alignz[64];
    char           _nm_alignn[64];
    char           _nm_time[64];
    char           _nm_dtime[64];
    char           _nm_cycle[64];
    int           *_gnodeno;
    int            _group_no;
    char          *_phzl_name;
    int            _guihide;
    char          *_mrgtree_name;
    char         **_region_pnames;
    int            _tv_connectivity;
    int            _disjoint_mode;
    int            _llong_gnodeno;
    int            _conserved;
    int            _extensive;
    double         _missing_value;
    char          *_ghost_node_labels;
    char         **_alt_nodenum_vars;
};

/*
 * Global data for Csgmesh
 */
struct _csgm {
    float          _time;
    int            _time_set;
    double         _dtime;
    int            _dtime_set;
    int            _cycle;
    int            _ndims;
    int            _nbounds;
    int            _use_specmf;
    int            _hi_offset;
    int            _lo_offset;
    int            _hi_offset_set;
    int            _lo_offset_set;
    int            _ascii_labels;
    char          *_label;
    char          *_unit;
    char          *_labels[3];
    char          *_units[3];
    char           _nm_time[64];
    char           _nm_dtime[64];
    char           _nm_cycle[64];
    int            _group_no;
    int            _origin;
    char          *_csgzl_name;
    char         **_bndnames;
    int            _guihide;
    char          *_mrgtree_name;
    char         **_region_pnames;
    int            _tv_connectivity;
    int            _disjoint_mode;
    int            _conserved;
    int            _extensive;
    double         _missing_value;
    char         **_alt_nodenum_vars;
};

/*
 * Global data for UCD Zonelist
 */
struct _uzl {
    int           *_gzoneno;
    int            _llong_gzoneno;
    char          *_ghost_zone_labels;
    char         **_alt_zonenum_vars;
};

/*
 * Global data for Poly Zonelist
 */
struct _phzl {
    int           *_gzoneno;
    int            _llong_gzoneno;
    char          *_ghost_zone_labels;
    char         **_alt_zonenum_vars;
};

/*
 * Global data for CSG Zonelist
 */
struct _csgzl {
    char         **_regnames;
    char         **_zonenames;
    char         **_alt_zonenum_vars;
};

/*
 * Global data for Mulimesh, Multimat, Multivar, and Multimatspecies
 */
struct _mm {
    float          _time;
    int            _time_set;
    double         _dtime;
    int            _dtime_set;
    int            _cycle;
    char           _nm_time[64];
    char           _nm_dtime[64];
    char           _nm_cycle[64];
    int            *_matnos;
    int            _nmatnos;
    char           *_matname;
    int            _nmat;
    int            *_nmatspec;
    int            _blockorigin;
    int            _grouporigin;
    int            _ngroups;
    int            _extentssize;
    double         *_extents;
    int            *_zonecounts;
    int            *_mixlens;
    int            *_matcounts;
    int            *_matlists;
    int            *_has_external_zones;
    int            _allowmat0;
    int            _guihide;
    int            _lgroupings;
    int            *_groupings;
    char           **_groupnames;
    char           **_matcolors;
    char           **_matnames;
    char           *_mrgtree_name;
    char          **_region_pnames;
    char           *_mmesh_name;
    int             _tensor_rank;
    int             _tv_connectivity;
    int             _disjoint_mode;
    int             _topo_dim;
    char          **_specnames;
    char          **_speccolors;
    int            _conserved;
    int            _extensive;
    char          *_file_ns;
    char          *_block_ns;
    int            _block_type;
    int           *_empty_list;
    int            _empty_cnt;
    int            _repr_block_idx;
    double         _missing_value;
    char         **_alt_zonenum_vars;
    char         **_alt_nodenum_vars;
};

/*
 * Global data for curves.
 */
struct _cu {
   char         *_label ;
   char         *_varname[2] ;
   char         *_labels[2] ;
   char         *_units[2] ;
    int          _guihide;
   char         *_reference ;
    int          _coord_sys ;
    double       _missing_value;
};

/*
 * Global data for defvars
 */
struct _dv {
    int         _guihide; /* for this object type, its an array */
};

/*
 * Global data for mrgtree 
 */
struct _mrgt {
    char      **_mrgvar_onames;
    char      **_mrgvar_rnames;
};

extern struct _ma _ma;
extern struct _ms _ms;
extern struct _csgm _csgm;
extern struct _pm _pm;
extern struct _qm _qm;
extern struct _um _um;
extern struct _uzl _uzl;
extern struct _phzl _phzl;
extern struct _csgzl _csgzl;
extern struct _mm _mm;
extern struct _cu _cu;
extern struct _dv _dv;
extern struct _mrgt _mrgt;

/*-------------------------------------------------------------------------
 * Filter Name Table.  Filters are modules inserted between the API and the
 * device driver that are able to perform preprocessing of the parameters
 * and postprocessing of the return values.  This table associates the
 * filter name (which must be unique) with a filter init routine
 * which is called just after a database is opened whether the database
 * requests the filter or not.  There is also a filter open routine which
 * is called after the init routine, but only if the database requests the
 * filter.  Either function can be null; if both are null then the entry
 * is removed from the table.
 *-------------------------------------------------------------------------
 */
typedef struct filter_t {
    char          *name;        /*filter name    */
    int            (*init) (DBfile *, char *);
    int            (*open) (DBfile *, char *);
} filter_t;

#define MAX_FILE_OPTIONS_SETS 32
#define NUM_DEFAULT_FILE_OPTIONS_SETS (DB_FILE_OPTS_LAST+1)
#define DEFAULT_DRIVER_PRIORITIES \
   {      /* unknown driver priorities */ \
        1, 2, 7, 0, 3, 6, -1, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0 \
    }

/* Namespace struct for Silo's global variables */
typedef struct SILO_Globals_t {
    unsigned long long dataReadMask;
    int allowOverwrites;
    int allowEmptyObjects;
    int enableChecksums;
    int enableFriendlyHDF5Names;
    int enableGrabDriver;
    int darshanEnabled;
    int allowLongStrComponents;
    int maxDeprecateWarnings;
    char *compressionParams;
    float compressionMinratio;
    int compressionErrmode;
    int compatibilityMode;
    int evalNameschemes;
    const DBoptlist *fileOptionsSets[MAX_FILE_OPTIONS_SETS];
    int _db_err_level;
    void  (*_db_err_func)(char *);
    int _db_err_level_drvr;
    jstk_t *Jstk;   /*error jump stack  */
    int unknownDriverPriorities[MAX_FILE_OPTIONS_SETS+10+1];
} SILO_Globals_t;
extern SILO_Globals_t SILO_Globals;

struct db_PathnameComponentTag
{  char                            *name;
   struct db_PathnameComponentTag *prevComponent;
   struct db_PathnameComponentTag *nextComponent;
};

typedef struct db_PathnameComponentTag   db_PathnameComponent;

struct db_PathnameTag
{  db_PathnameComponent *firstComponent;
   db_PathnameComponent *lastComponent;
};

typedef struct db_PathnameTag            db_Pathname;

/*
 * Private functions that need to be shared among compilation modules.
 */
INTERNAL context_t *context_switch (DBfile *, char const *, char const **);
INTERNAL int context_restore (DBfile *, context_t *);
INTERNAL DBfile *silo_db_close (DBfile *);
INTERNAL int db_num_registered_files();
INTERNAL DBtoc *db_AllocToc (void);
INTERNAL int db_FreeToc (DBfile *);
INTERNAL int db_GetMachDataSize (int);
INTERNAL char *DBGetObjtypeName (int);
INTERNAL char *db_strndup (const char *, int);
INTERNAL char *db_GetDatatypeString (int);
INTERNAL int db_GetDatatypeID (char const * const);
INTERNAL int db_perror (char const *, int, char const *);
INTERNAL void _DBQQCalcStride (int *, int *, int, int);
INTERNAL void _DBQMSetStride (DBquadmesh *);
INTERNAL int _DBstrprint (FILE *, char **, int, int, int, int, int);
INTERNAL void _DBsort_list (char **, int);
INTERNAL int _DBarrminmax (float *, int, float *, float *);
INTERNAL int _DBiarrminmax (int *, int, int *, int *);
INTERNAL int _DBdarrminmax (double *, int, double *, double *);
INTERNAL char *db_strerror (int);
INTERNAL int db_ListDir2 (DBfile *, char **, int, int, char **,
                              int *);
INTERNAL int CSGM_CalcExtents (int, int, int, const int*,
                                 const void *, double *, double *);
INTERNAL int _DBQMCalcExtents (DBVCP2_t, int, int const *, int const *, int const *, int,
                                   int, void *, void *);
INTERNAL int UM_CalcExtents (DBVCP2_t, int, int, int, void *,
                                 void *);
INTERNAL int db_ProcessOptlist (int, DBoptlist const * const);
INTERNAL int db_VariableNameValid(char const *);
INTERNAL int db_SplitShapelist (DBucdmesh *um);
INTERNAL int db_ResetGlobalData_Csgmesh ();
INTERNAL int db_ResetGlobalData_Mrgtree();
INTERNAL int db_ResetGlobalData_PointMesh (int ndims);
INTERNAL int db_ResetGlobalData_QuadMesh (int ndims);
INTERNAL void db_ResetGlobalData_Curve (void);
INTERNAL int db_ResetGlobalData_Ucdmesh (int ndims, int nnodes, int nzones);
INTERNAL int db_ResetGlobalData_Ucdzonelist (void);
INTERNAL int db_ResetGlobalData_MultiMesh (void);
INTERNAL int db_ResetGlobalData_Defvars(void);
INTERNAL char *db_FullName2BaseName(const char *);
INTERNAL void db_StringArrayToStringList(char**, int, char **, int*);
INTERNAL char ** db_StringListToStringArray(char const *, int *, char, int);
INTERNAL void db_DriverTypeAndFileOptionsSetId(int driver, int *type,
                                               int *opts_set_id);
INTERNAL char *db_absoluteOf_path ( const char *cwg, const char *pathname );
INTERNAL char *db_basename ( const char *pathname );
INTERNAL char *db_dirname ( const char *pathname );
INTERNAL int   db_isAbsolute_path ( const char *pathname );
INTERNAL int   db_isRelative_path ( const char *pathname );
INTERNAL char *db_join_path ( const char *a, const char *b );
INTERNAL char *db_normalize_path ( const char *p );
INTERNAL int   db_relative_path ( char *pathname );
INTERNAL char *db_unsplit_path ( const db_Pathname *p );
INTERNAL db_Pathname *db_split_path ( const char *pathname );
INTERNAL const int *db_get_used_file_options_sets_ids();
//char   *_db_safe_strdup (const char *);
#undef strdup /*prevent a warning for the following definition*/
#define strdup(s) _db_safe_strdup(s)

INTERNAL int db_StringListToStringArrayMBOpt(char *strList, char ***strArray, char **alloc_flag, int nblocks);
INTERNAL int db_fix_obsolete_centering(int ndims, float const *align, int carfm);

#endif /* !SILO_PRIVATE_H */
