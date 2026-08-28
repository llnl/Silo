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
 * SILO FORTRAN Interface.
 */
#include <stdio.h>
#include "silo_private.h"
#include "silo_f.h"

/* Array of pointers that Fortran accesses */
static void  **DBFortranPointers = NULL;
static int     DBMaxFortranPointer = 0;
static int     DBFortranEmptyPointerSpaces = 0;
static int     fortran2DStrLen = 32;

/*----------------------------------------------------------------------
 * Routine                                           DBFortranAccessPointer
 *
 * Programmer
 *     Sean Ahern
 *     Mon Apr 10 19:21:46 PDT 1995
 *
 * Purpose
 *     Access an entry in the DBFortranPointers array.
 *
 * Notes
 *     The fortran value starts at 1.
 *
 * Returns
 *     Returns a void* from the DBFortranPointers array.
 *
 * Modifications
 *
 *     Jim Reus, 23 Apr 97
 *     Changed to prototype form.
 *
 *     Eric Brugger, Tue Jun 17 11:12:38 PDT 1997
 *     I made the routine externally accessable.
 *
 *--------------------------------------------------------------------*/
void *
DBFortranAccessPointer (int value)
{
    static char    *me = "DBFortranAccessPointer";

    if (value == DB_F77NULL)
    {
        return (NULL);
    }
    if ((value < 1) || (value > DBMaxFortranPointer))
    {
        db_perror(NULL, E_BADARGS, me);
        return (NULL);
    } else
        return (DBFortranPointers[value - 1]);
}

/*----------------------------------------------------------------------
 * Routine                                            DBFortranAllocPointer
 *
 * Programmer
 *     Sean Ahern
 *     Mon Apr 10 19:21:46 PDT 1995
 *
 * Purpose
 *     Add an entry to the DBFortranPointers array, allocating memory
 *     if needed.
 *
 * Notes
 *     The first value (array element 0) is given to Fortran as 1
 *
 * Returns
 *     Returns an integer index into the DBFortranPointer array.
 *     Returns -1 on error (NULL is passed is one example).
 *
 * Modifications:
 *
 *     Jim Reus, 23 Apr 97
 *     Changed to prototype form.
 *
 *     Sean Ahern, Tue Feb  1 15:52:31 PST 2000
 *     Made this function publically accessible.
 *
 *--------------------------------------------------------------------*/
int
DBFortranAllocPointer(void *pointer)
{
    int            i;
    int            found = -1;
    static char   *me = "DBFortranAllocPointer";

    if (pointer == NULL)
        return (DB_F77NULL);

    if (DBFortranPointers == NULL) {
        DBFortranPointers = ALLOC(void *);

        if (DBFortranPointers == NULL) {
            db_perror(NULL, E_NOMEM, me);
            return (DB_F77NULL);
        }
        DBFortranPointers[0] = pointer;
        DBMaxFortranPointer = 1;
        return (1);
    }
    else {
        /* Check for NULL space */
        if (DBFortranEmptyPointerSpaces > 0) {
            for (i = 0; i < DBMaxFortranPointer; i++)
                if (DBFortranPointers[i] == NULL)
                    found = i;
        }

        /* Found a NULL space.  Use it */
        if (found != -1) {
            DBFortranPointers[found] = pointer;
            DBFortranEmptyPointerSpaces--;
            return (found + 1);
        }
        else {
            /* No NULL space found.  Make room. */
            /* I am using realloc instead of REALLOC because the REALLOC macro
             * hates allocating pointers */
            DBFortranPointers = (void **)realloc(DBFortranPointers,
                             sizeof(void *) * (DBMaxFortranPointer + 1));

            if (DBFortranPointers == NULL) {
                db_perror(NULL, E_NOMEM, me);
                return (DB_F77NULL);
            }
            DBFortranPointers[DBMaxFortranPointer] = pointer;
            DBMaxFortranPointer++;
            return (DBMaxFortranPointer);
        }
    }
}

/*----------------------------------------------------------------------
 * Routine                                        DBFortranRemovePointer
 *
 * Programmer
 *     Sean Ahern
 *     Mon Apr 10 19:21:46 PDT 1995
 *
 * Purpose
 *     Remove an entry from the DBFortranPointers array.
 *
 * Returns
 *     Returns nothing.
 *
 * Modifications
 *
 *     Robb Matzke, 15 May 1996
 *     Removed the unused `i' auto variable.
 *
 *     Jim Reus, 23 Apr 97
 *     Changed to prototype form.
 *
 *     Sean Ahern, Mon Nov 22 12:43:22 PST 1999
 *     Moved the test of the value range up before we use the value.  We 
 *     wouldn't want to walk into bad memory.
 *
 *     Sean Ahern, Tue Feb  1 15:53:01 PST 2000
 *     Made this function publically accessible.
 *
 *--------------------------------------------------------------------*/
void
DBFortranRemovePointer (int value)
{
    static char   *me = "DBFortranRemovePointer";

    if ((value > DBMaxFortranPointer) || (value < 1)) {
        db_perror(NULL, E_BADARGS, me);
        return;
    }

    if (DBFortranPointers[value - 1] == NULL) {
        db_perror(NULL, E_BADARGS, me);
        return;
    }

    DBFortranPointers[value-1] = NULL;
    DBFortranEmptyPointerSpaces++;
}

SILO_API FORTRAN
DBMKPTR_FC (void *p)
{
    return DBFortranAllocPointer(p);
}

SILO_API FORTRAN
DBRMPTR_FC (int *pid)
{
    DBFortranRemovePointer(*pid);
    return 0;
}

/*----------------------------------------------------------------------
 * Routine                                                  DBADDIOPT_FC
 *
 * Purpose
 *     Add an integer option to the given option list.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDIOPT_FC (int *optlist_id, int *option, int *ivalue)
{
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbaddiopt", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        if (!optlist)
            API_ERROR("optlist_id", E_BADARGS);
        if (optlist->numopts >= optlist->maxopts)
            API_ERROR("optlist numopts", E_BADARGS);

        optlist->options[optlist->numopts] = *option;
        optlist->values[optlist->numopts] = ivalue;
        optlist->numopts++;
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 * Routine                                                  DBADDROPT_FC
 *
 * Purpose
 *     Add a real option to the given option list.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0.  On error, it calles API_ERROR.
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDROPT_FC (int *optlist_id, int *option, float *rvalue)
{
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbaddropt", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        if (!optlist)
            API_ERROR("optlist_id", E_BADARGS);
        if (optlist->numopts >= optlist->maxopts)
            API_ERROR("optlist numopts", E_BADARGS);

        optlist->options[optlist->numopts] = *option;
        optlist->values[optlist->numopts] = rvalue;
        optlist->numopts++;
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 * Routine                                                  DBADDDOPT_FC
 *
 * Purpose
 *     Add a double precision option to the given option list.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDDOPT_FC (int *optlist_id, int *option, double *dvalue)
{
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbadddopt", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        if (!optlist)
            API_ERROR("optlist_id", E_BADARGS);
        if (optlist->numopts >= optlist->maxopts)
            API_ERROR("optlist numopts", E_BADARGS);

        optlist->options[optlist->numopts] = *option;
        optlist->values[optlist->numopts] = dvalue;
        optlist->numopts++;
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 * Routine                                                  DBADDCOPT_FC
 *
 * Purpose
 *     Add a character option to the given option list.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Warning
 *     THIS FUNCTION CAUSES A MEMORY LEAK. The temporary string,
 *     cval, is never freed.  It is put in the optlist instead.
 *
 * Returns
 *
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Eric Brugger, Thu Mar 14 11:06:07 PST 1996
 *     I corrected a bug where the copy of cvalue was always freed,
 *     causing the option value to always be NULL, causing the routine
 *     to never work.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDCOPT_FC (int *optlist_id, int *option, FCD_DB cvalue, int *lcvalue)
{
    char          *cval = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbaddcopt", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        if (!optlist)
            API_ERROR("optlist_id", E_BADARGS);
        if (*lcvalue <= 0)
            API_ERROR("lcvalue", E_BADARGS);
        if (optlist->numopts >= optlist->maxopts)
            API_ERROR("optlist numopts", E_BADARGS);

        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
#ifdef CRAY
        if (strcmp(_fcdtocp(cvalue), DB_F77NULLSTRING) == 0)
            API_ERROR("cvalue", E_BADARGS);
        cval = SW_strndup(_fcdtocp(cvalue), *lcvalue);
#else
        if (strcmp(cvalue, DB_F77NULLSTRING) == 0)
            API_ERROR("cvalue", E_BADARGS);
        cval = SW_strndup(cvalue, *lcvalue);
#endif

        optlist->options[optlist->numopts] = *option;
        optlist->values[optlist->numopts] = cval;
        optlist->numopts++;
    }
    API_END;

    return(0);
}
/*-------------------------------------------------------------------------
 * Routine                                                    DBADDCAOPT_FC
 * 
 * Purpose
 *     Add a character array option to the given option list.
 *     
 * Notes
 *     This function was built to be called from Fortran.
 *     Start of "#ifdef CRAY" deprecation.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Thomas R. Treadway
 *     Tue Jul 25 11:04:37 PDT 2006
 *
 * Modifications
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Thu Feb  4 10:10:28 PST 2010
 *     Added patch by Olivier Cessenat to allow variable length strings.
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDCAOPT_FC (int *optlist_id, int *option,
             int *nval, FCD_DB cvalue, int *lcvalue)
{
    char          **cval = NULL;
    char          *names = NULL;
    DBoptlist     *optlist = NULL;
    int           i;
    long long     indx;

    API_BEGIN("dbaddaopt", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        if (!optlist)
            API_ERROR("optlist_id", E_BADARGS);

        if (strcmp(cvalue, DB_F77NULLSTRING) == 0)
            names = NULL;
        else
            names = cvalue;

        if (*nval <= 0)
            API_ERROR("nval", E_BADARGS);
        cval = ALLOC_N(char *, *nval);

        for (indx = 0, i = 0; i < *nval; i++) {
            if (lcvalue[i] < 0)
                API_ERROR("lcvalue", E_BADARGS);
            cval[i] = SW_strndup(&names[indx], lcvalue[i]);
            if (fortran2DStrLen > 0)
                indx += fortran2DStrLen;
            else
                indx += lcvalue[i];
        }
        optlist->options[optlist->numopts] = *option;
        optlist->values[optlist->numopts] = cval;
        optlist->numopts++;

	/* memory allocated above must be freed with
	   dbfreecaopt */
    }
    API_END;

    return(0);
}

/*-------------------------------------------------------------------------
 * Routine                                                   DBFREECAOPT_FC
 * 
 * Purpose
 *     Free a character array option to the given option list.
 *     Should be called before DBFREEOPTLIST to avoid memory leaks
 *     since DBADDCAOPT makes a copy of the Fortran array.
 *     
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Olivier G. Cessenat
 *     Thu Feb 11 8:34:01 MET 2022
 *
 * Modifications
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBFREECAOPT_FC (int *optlist_id, int *option, int *nval)
{
    char          **cval = NULL;
    DBoptlist     *optlist = NULL;
    int           i, j;

    API_BEGIN("dbfreecaopt", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        if (!optlist)
            API_ERROR("optlist_id", E_BADARGS);

        if (*nval <= 0)
            API_ERROR("nval", E_BADARGS);
        for (i = 0; i < optlist->numopts; i++) {
            if (optlist->options[i] == *option) {
                cval = optlist->values[i];
                for (j = 0; j < *nval; j++)
                    FREE(cval[j]);
                FREE(cval);
                API_RETURN(0);
            }
        }
    }

    API_END;
    return(-1);
}

/*-------------------------------------------------------------------------
 * Routine                                                    DBADDIAOPT_FC
 * 
 * Purpose
 *     Add an integer array option to the given option list.
 *     
 * Programmer
 *     Neil Hodge
 *     Tue Sep 13 11:16:43 PDT 2011
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDIAOPT_FC (int *optlist_id, int *option,
               int *nval, int *ivalues)
{
    int *ival = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbaddiaopt", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        if (!optlist)
            API_ERROR("optlist_id", E_BADARGS);

        if (*nval <= 0)
            API_ERROR("nval", E_BADARGS);
        ival = ALLOC_N(int, *nval);

        ival = ivalues;

        optlist->options[optlist->numopts] = *option;
        optlist->values[optlist->numopts] = ival;
        optlist->numopts++;
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 * Routine                                                DBMKOPTLIST_FC
 *
 * Purpose
 *     Create an option list and return its identifier.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBMKOPTLIST_FC (int *maxopts, int *optlist_id)
{
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbmkoptlist", int, -1) {
        if (*maxopts <= 0)
            API_ERROR("maxopts", E_BADARGS);

        optlist = DBMakeOptlist(*maxopts);
        *optlist_id = DBFortranAllocPointer(optlist);

        API_RETURN(optlist ? 0 : (-1));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                              DBFREEOPTLIST_FC
 *
 * Purpose
 *     Free an option list.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBFREEOPTLIST_FC (int *optlist_id)
{
    int            err;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbfreeoptlist", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        err = DBFreeOptlist(optlist);
        DBFortranRemovePointer(*optlist_id);
        *optlist_id = -1;
        API_RETURN(err);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                   DBPUTMAT_FC
 *
 * Purpose
 *     Write a material data object into the given SILO file.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Sean Ahern, Wed Apr 12 12:09:20 PDT 1995
 *     Added a DBoptlist lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTMAT_FC (int *dbid, FCD_DB name,
           int *lname, FCD_DB meshname, int *lmeshname,
           int *nmat, int *matnos, int *matlist, int *dims, int *ndims,
           int *mix_next, int *mix_mat, int *mix_zone, void const *mix_vf,
           int *mixlen, int *datatype, int *optlist_id, int *status)
{
    char          *nm = NULL, *mnm = NULL;
    DBfile        *dbfile = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputmat", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);

        if (strcmp(_fcdtocp(meshname), DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(_fcdtocp(meshname), *lmeshname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);

        if (strcmp(meshname, DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(meshname, *lmeshname);
#endif

        *status = DBPutMaterial(dbfile, nm, mnm, *nmat, matnos, matlist, dims, *ndims,
                      FPTR(mix_next), FPTR(mix_mat), FPTR(mix_zone), FPTR(mix_vf),
                      *mixlen, *datatype, optlist);

        FREE(nm);
        FREE(mnm);

        API_RETURN((*status >= 0) ? 0 : (-1));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                   DBPUTMSP_FC
 *
 * Purpose
 *     Write a material species data object into the given SILO file.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTMSP_FC (int *dbid, FCD_DB name, int *lname, FCD_DB matname, int *lmatname,
           int *nmat, int *nmatspec, int *speclist, int *dims, int *ndims,
           int *nspecies_mf, void const *species_mf, int *mix_speclist, int *mixlen,
           int *datatype, int *optlist_id, int *status)
{
    char          *nm = NULL, *mnm = NULL;
    DBfile        *dbfile = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputmsp", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);

        if (strcmp(_fcdtocp(matname), DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(_fcdtocp(matname), *lmatname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);

        if (strcmp(matname, DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(matname, *lmatname);
#endif

        *status = DBPutMatspecies(dbfile, nm, mnm, *nmat, nmatspec,
                                  speclist, dims, *ndims, *nspecies_mf,
                                  FPTR(species_mf), FPTR(mix_speclist), *mixlen,
                                  *datatype, optlist);

        FREE(nm);
        FREE(mnm);
        API_RETURN((*status >= 0) ? 0 : (-1));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                   DBCALCFL_FC
 *
 * Programmer
 *     Jeffery W. Long, NSSD-B
 *
 * Notes
 *     This is the Fortran-callable version of DBCalcExternalFacelist.  It
 *     creates an external facelist, and it returns the pointer to the
 *     facelist data structure.  This pointer can be written into a SILO
 *     file with the function DBWRTFL.
 *
 *     If the boundary method is > 0, then certain faces on material
 *     boundaries should be treated as external. The method for
 *     determining this is defined as follows:
 *
 *     0 =  Material boundaries should not be used in
 *          computing external faces.
 *     1 =  Material boundaries occuring between two
 *          clean zones should be marked as external.
 *     2 =  Material boundaries occuring between two clean
 *          zones, or between one clean and one mixed zone,
 *          should be marked as external (but only the face
 *          from the clean zone will be so marked, the face
 *          from the mixed zone will be marked as internal)
 *     4 =  Same as 2, except both faces will always be
 *          marked as external.
 *
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Fri Apr 21 14:00:55 PDT 1995
 *     Added a call to DBFortranAllocPointer to store
 *     the facelist pointer in the global DBFortranPointers
 *     array.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBCALCFL_FC (int *znodelist, int *nnodes, int *origin, int *zshapesize,
           int *zshapecnt, int *nzshapes, int *matlist, int *bnd_method,
           int *object_id)
{
    DBfacelist    *fl = NULL;

    API_BEGIN("dbcalcfl", int, -1) {
        fl = DBCalcExternalFacelist(znodelist, *nnodes, *origin, zshapesize,
                                    zshapecnt, *nzshapes, FPTR(matlist),
                                    *bnd_method);
        *object_id = DBFortranAllocPointer(fl);

        API_RETURN((fl == NULL) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBCLOSE_FC
 *
 * Purpose
 *     Close a database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 12:41:32 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Mon Mar 20 14:59:08 PST 1995
 *     I added a status return value.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995 Added a
 *     DBfile lookup in a global array using
 *     DBFortranAccessPointer.  Also added a call to
 *     DBFortranRemovePointer to clear a DBfile* from the
 *     global array.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Thu Nov  5 08:58:42 PST 2009
 *     Added *dbid = -1
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBCLOSE_FC (int *dbid)
{
    int            status;
    DBfile        *dbfile = NULL;

    API_BEGIN("dbclose", int, -1) {
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        status = DBClose(dbfile);
        DBFortranRemovePointer(*dbid);
        *dbid = -1;
        API_RETURN(status);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBCREATE_FC
 *
 * Purpose
 *     Create a new database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 12:43:57 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:08:16 PDT 1995
 *     Added a DBFortranAllocPointer call to put the DBfile*
 *     in a global array.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBCREATE_FC (FCD_DB pathname, int *lpathname, int *mode, int *target,
           FCD_DB fileinfo, int *lfileinfo, int *filetype, int *dbid)
{
    char          *path = NULL, *finfo = NULL;
    DBfile        *dbfile = NULL;

    API_BEGIN("dbcreate", int, -1) {
        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
#ifdef CRAY
        if (strcmp(_fcdtocp(pathname), DB_F77NULLSTRING) == 0)
            path = NULL;
        else
            path = SW_strndup(_fcdtocp(pathname), *lpathname);

        if (strcmp(_fcdtocp(fileinfo), DB_F77NULLSTRING) == 0)
            finfo = NULL;
        else
            finfo = SW_strndup(_fcdtocp(fileinfo), *lfileinfo);
#else
        if (strcmp(pathname, DB_F77NULLSTRING) == 0)
            path = NULL;
        else
            path = SW_strndup(pathname, *lpathname);

        if (strcmp(fileinfo, DB_F77NULLSTRING) == 0)
            finfo = NULL;
        else
            finfo = SW_strndup(fileinfo, *lfileinfo);
#endif

        /*------------------------------
         *  Create file and assign dbid.
         *-----------------------------*/
        dbfile = DBCreate(path, *mode, *target, finfo, *filetype);

        FREE(path);
        FREE(finfo);

        *dbid = DBFortranAllocPointer(dbfile);

        API_RETURN(dbfile ? 0 : (-1));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBINQLEN_FC
 *
 * Purpose
 *     Returns the number of elements of the requested variable.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 12:46:42 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBINQINT_FC (int *dbid, FCD_DB varname, int *lvarname, void *tag, int *theInt)
{
    char          *varnm = NULL;
    DBfile        *dbfile = NULL;

    API_BEGIN("dbinqint", int, -1) {
        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lvarname <= 0)
            API_ERROR("lvarname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(varname), DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(_fcdtocp(varname), *lvarname);
#else
        if (strcmp(varname, DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(varname, *lvarname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        if (tag == (void*) DBGetVarLength)
            *theInt = DBGetVarLength(dbfile, varnm);
        else if (tag == (void*) DBGetVarByteLength)
            *theInt = DBGetVarByteLength(dbfile, varnm);
        else if (tag == (void*) DBGetVarType)
            *theInt = DBGetVarType(dbfile, varnm);
        else if (tag == (void*) DBInqVarType)
            *theInt = DBInqVarType(dbfile, varnm);
        else if (tag == (void*) DBInqMeshtype)
            *theInt = DBInqMeshtype(dbfile, varnm);
        else
            *theInt = -1;

        FREE(varnm);

        if (*theInt < 0) {
            *theInt = 0;
            API_RETURN(-1);
        }
        else
            API_RETURN(0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

SILO_API FORTRAN
DBINQLEN_FC(int *dbid, FCD_DB varname, int *lvarname, int *len)
{
    return DBINQINT_FC(dbid, varname, lvarname, DBGetVarLength, len);
}

SILO_API FORTRAN
DBINQBLEN_FC(int *dbid, FCD_DB varname, int *lvarname, int *blen)
{
    return DBINQINT_FC(dbid, varname, lvarname, DBGetVarByteLength, blen);
}

SILO_API FORTRAN
DBINQDTYP_FC(int *dbid, FCD_DB varname, int *lvarname, int *dtyp)
{
    return DBINQINT_FC(dbid, varname, lvarname, DBGetVarType, dtyp);
}

SILO_API FORTRAN
DBINQVTYP_FC(int *dbid, FCD_DB varname, int *lvarname, int *vtyp)
{
    return DBINQINT_FC(dbid, varname, lvarname, DBInqVarType, vtyp);
}

SILO_API FORTRAN
DBINQMTYP_FC(int *dbid, FCD_DB varname, int *lvarname, int *mtyp)
{
    return DBINQINT_FC(dbid, varname, lvarname, DBInqMeshtype, mtyp);
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBMKDIR_FC
 *
 * Purpose
 *     Creates a new directory.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 12:49:25 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBMKDIR_FC (int *dbid, FCD_DB dirname, int *ldirname, int *status)
{
    char          *dir = NULL;
    DBfile        *dbfile = NULL;

    API_BEGIN("dbmkdir", int, -1) {
        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*ldirname <= 0)
            API_ERROR("ldirname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(dirname), DB_F77NULLSTRING) == 0)
            dir = NULL;
        else
            dir = SW_strndup(_fcdtocp(dirname), *ldirname);
#else
        if (strcmp(dirname, DB_F77NULLSTRING) == 0)
            dir = NULL;
        else
            dir = SW_strndup(dirname, *ldirname);
#endif

        /*------------------------------
         *  Create directory.
         *-----------------------------*/
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        *status = DBMkDir(dbfile, dir);

        FREE(dir);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                        DBOPEN_FC
 *
 * Purpose
 *     Open an existing database
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 12:53:00 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:08:16 PDT 1995
 *     Added a DBFortranAllocPointer call to put the DBfile*
 *     in a global array.
 *
 *     Sean Ahern, Wed Jan 17 17:06:07 PST 1996
 *     Added the mode parameter.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBOPEN_FC (FCD_DB pathname, int *lpathname, int *type, int *mode, int *dbid)
{
    char          *path = NULL;
    DBfile        *dbfile = NULL;

    API_BEGIN("dbopen", int, -1) {
        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lpathname <= 0)
            API_ERROR("lpathname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(pathname), DB_F77NULLSTRING) == 0)
            path = NULL;
        else
            path = SW_strndup(_fcdtocp(pathname), *lpathname);
#else
        if (strcmp(pathname, DB_F77NULLSTRING) == 0)
            path = NULL;
        else
            path = SW_strndup(pathname, *lpathname);
#endif

        /*------------------------------
         *  Open file and assign dbid.
         *-----------------------------*/
        dbfile = DBOpen(path, *type, *mode);

        FREE(path);

        *dbid = DBFortranAllocPointer(dbfile);
        API_RETURN((dbfile == NULL) ? -1 : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBPUTFL_FC
 *
 * Purpose
 *     Write a facelist object into the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 12:55:31 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Mon Mar 20 14:59:57 PST 1995
 *     Added assignment to *status.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Wed Apr 13 18:14:27 PDT 2022
 *     
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTFL_FC (int *dbid, FCD_DB name, int *lname, int *nfaces, int *ndims,
          int *nodelist, int *lnodelist, int *origin, int *zoneno,
          int *shapesize, int *shapecnt, int *nshapes, int *types,
          int *typelist, int *ntypes, int *status)
{
    DBfile        *dbfile = NULL;
    char          *nm = NULL;

    API_BEGIN("dbputfl", int, -1) {
        /*------------------------------
         *  Initializations.
         *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        /*---------------------------------------------
         *  Check for "null" arrays. The convention is
         *  that a -1 indicates a null array.
         *--------------------------------------------*/

        *status = DBPutFacelist(dbfile, nm, *nfaces, *ndims,
                                nodelist, *lnodelist, *origin,
                                FPTR(zoneno),
                                shapesize, shapecnt, *nshapes,
                                FPTR(types), FPTR(typelist), *ntypes);

        FREE(nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBWRTFL_FC
 *
 * Purpose
 *     Write a facelist object into the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     brugger@aria
 *     Wed Sep 13 08:02:04 PDT 1995
 *
 * Modifications
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBWRTFL_FC (int *dbid, FCD_DB name, int *lname, int *object_id, int *status)
{
    DBfile        *dbfile = NULL;
    DBfacelist    *fl = NULL;
    char          *nm = NULL;

    API_BEGIN("dbputfl", int, -1) {
        /*------------------------------
         *  Initializations.
         *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        fl = (DBfacelist *) DBFortranAccessPointer(*object_id);

        /*---------------------------------------------
         *  Check for "null" arrays. The convention is
         *  that a -1 indicates a null array.
         *--------------------------------------------*/

        *status = DBPutFacelist(dbfile, nm, fl->nfaces, fl->ndims,
                                fl->nodelist, fl->lnodelist, fl->origin,
                                fl->zoneno, fl->shapesize, fl->shapecnt,
                                fl->nshapes, fl->types, fl->typelist,
                                fl->ntypes);

        FREE(nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                    DBPUTMMESH_FC
 *
 * Purpose
 *     Writes a multi-block mesh object into the open database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 13:00:17 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Thu Feb  4 10:10:28 PST 2010
 *     Added patch by Olivier Cessenat to allow variable length strings.
 *
 *     Mark C. Miller, Sun Oct 25 14:15:00 CET 2020
 *     Added patch by Olivier Cessenat to allow nameschemes.
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTMMESH_FC (int *dbid, FCD_DB name, int *lname, int *nmesh, FCD_DB meshnames,
             int *lmeshnames, int *meshtypes, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char         **meshnms = NULL;
    char          *nm = NULL;
    char          *realmeshnames = NULL;
    int            i;
    long long      indx;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputmmesh", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

#ifdef CRAY
        if (strcmp(_fcdtocp(meshnames), DB_F77NULLSTRING) == 0)
            realmeshnames = NULL;
        else
            realmeshnames = _fcdtocp(meshnames);
#else
        if (strcmp(meshnames, DB_F77NULLSTRING) == 0)
            realmeshnames = NULL;
        else
            realmeshnames = meshnames;
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        if (*nmesh <= 0)
            API_ERROR("nmesh", E_BADARGS);

        if (realmeshnames == NULL)
        {
            /*----------------------------------------
             *  Invoke the C function to do the work : using namescheme probably !
             *---------------------------------------*/
            *status = DBPutMultimesh(dbfile, nm, *nmesh,
                                     NULL, NULL, optlist);
            
        }
        else
        {
            meshnms = ALLOC_N(char *, *nmesh);
            
            for (indx = 0, i = 0; i < *nmesh; i++) {
                if (lmeshnames[i] < 0)
                    API_ERROR("lmeshnames", E_BADARGS);
                meshnms[i] = (char *) SW_strndup(&realmeshnames[indx], lmeshnames[i]);
                if (fortran2DStrLen > 0)
                    indx += fortran2DStrLen;
                else
                    indx += lmeshnames[i];
            }

            /*----------------------------------------
             *  Invoke the C function to do the work.
             *---------------------------------------*/
            *status = DBPutMultimesh(dbfile, nm, *nmesh, (char const * const *) meshnms,
                         (int const *) FPTR(meshtypes), optlist);
            
            for (i = 0; i < *nmesh; i++)
                FREE(meshnms[i]);
            FREE(meshnms);

        }
        FREE(nm);
        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                  DBPUTDEFVARS_FC
 *
 * Purpose
 *     Writes a defvars object into the open database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller 
 *     October 12, 2005
 *
 * Modifications:
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Thu Feb  4 10:10:28 PST 2010
 *     Added patch by Olivier Cessenat to allow variable length strings.
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTDEFVARS_FC (int *dbid, FCD_DB name, int *lname, int *ndefs, FCD_DB names,
             int *lnames, int *types, FCD_DB defns, int *ldefns,
             int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char         **nms = NULL, **defs = NULL, *nm = NULL;
    char          *realnms = NULL, *realdefs = NULL;
    int            i;
    long long      indx;
    DBoptlist     **optlists = NULL;

    API_BEGIN("dbputdefvars", int, -1) {
        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
        if (*ndefs <= 0)
            API_ERROR("ndefs", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

#ifdef CRAY
        if (strcmp(_fcdtocp(names), DB_F77NULLSTRING) == 0)
            realnms = NULL;
        else
            realnms = _fcdtocp(names);
#else
        if (strcmp(names, DB_F77NULLSTRING) == 0)
            realnms = NULL;
        else
            realnms = names;
#endif

#ifdef CRAY
        if (strcmp(_fcdtocp(defns), DB_F77NULLSTRING) == 0)
            realdefs = NULL;
        else
            realdefs = _fcdtocp(defns);
#else
        if (strcmp(defns, DB_F77NULLSTRING) == 0)
            realdefs = NULL;
        else
            realdefs = defns;
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        nms = ALLOC_N(char *, *ndefs);
        for (indx = 0, i = 0; i < *ndefs; i++) {
            if (lnames[i] < 0)
                API_ERROR("lnames", E_BADARGS);
            nms[i] = SW_strndup(&realnms[indx], lnames[i]);
            if (fortran2DStrLen > 0)
               indx += fortran2DStrLen;
            else
                indx += lnames[i] ;
        }
        defs = ALLOC_N(char *, *ndefs);
        for (indx = 0, i = 0; i < *ndefs; i++) {
            if (ldefns[i] < 0)
                API_ERROR("ldefns", E_BADARGS);
            defs[i] = SW_strndup(&realdefs[indx], ldefns[i]);
            if (fortran2DStrLen > 0)
                indx += fortran2DStrLen;
            else
                indx += ldefns[i] ;
        }
        optlists = ALLOC_N(DBoptlist*, *ndefs);
        for (i = 0; i < *ndefs; i++) {
            optlists[i] = (DBoptlist *) DBFortranAccessPointer(optlist_id[i]);
        }

        /*----------------------------------------
         *  Invoke the C function to do the work.
         *---------------------------------------*/
        *status = DBPutDefvars(dbfile, nm, *ndefs, (char const * const *) nms,
                      types, (char const * const *) defs,
                      (DBoptlist const * const *) optlists);

        for (i = 0; i < *ndefs; i++)
        {
            FREE(nms[i]);
            FREE(defs[i]);
        }
        FREE(nms);
        FREE(defs);
        FREE(nm);
        FREE(optlists);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                     DBPUTMVAR_FC
 *
 * Purpose
 *     Writes a multi-block variable object into the open database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 13:05:12 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Sean Ahern, Wed Apr 15 11:37:28 PDT 1998
 *     Made the routine work.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Thu Feb  4 10:10:28 PST 2010
 *     Added patch by Olivier Cessenat to allow variable length strings.
 *
 *     Mark C. Miller, Sun Oct 25 14:15:00 CET 2020
 *     Added patch by Olivier Cessenat to allow nameschemes.
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTMVAR_FC (int *dbid, FCD_DB name, int *lname, int *nvar, FCD_DB varnames,
            int *lvarnames, int *vartypes, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char         **varnms = NULL, *nm = NULL;
    char          *realvarnames = NULL;
    int            i;
    long long      indx;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputmvar", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

        /* Duplicate all ASCII strings. */
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

#ifdef CRAY
        if (strcmp(_fcdtocp(varnames), DB_F77NULLSTRING) == 0)
            realvarnames = NULL;
        else
            realvarnames = _fcdtocp(varnames);
#else
        if (strcmp(varnames, DB_F77NULLSTRING) == 0)
            realvarnames = NULL;
        else
            realvarnames = varnames;
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        if (*nvar <= 0)
            API_ERROR("nvar", E_BADARGS);

        if (realvarnames == NULL)
        {
            /* Invoke the C function to do the work : using namescheme probably. */
            *status = DBPutMultivar(dbfile, nm, *nvar, NULL, NULL, optlist);
        }
        else
        {
            varnms = ALLOC_N(char *, *nvar);

            for(indx = 0, i = 0; i< *nvar; i++) {
                if (lvarnames[i] < 0)
                    API_ERROR("lvarnames", E_BADARGS);
                varnms[i] = SW_strndup(&realvarnames[indx], lvarnames[i]);
                if (fortran2DStrLen > 0)
                    indx += fortran2DStrLen;
                else
                    indx += lvarnames[i];
            }

            /* Invoke the C function to do the work. */
            *status = DBPutMultivar(dbfile, nm, *nvar, (char const * const *) varnms,
                      (int const *) FPTR(vartypes), optlist);

            for(i=0;i<*nvar;i++)
                FREE(varnms[i]);
            FREE(varnms);
        }
        FREE(nm);
        API_RETURN(*status < 0 ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */ ;
}

/*-------------------------------------------------------------------------
 * Routine                                                     DBPUTMMAT_FC
 *
 * Purpose
 *     Writes a multi-block material object into the open file.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Sean Ahern (ahern@llnl.gov)
 *     Thu Apr 16 13:12:02 PDT 1998
 *
 * Modifications
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Thu Feb  4 10:10:28 PST 2010
 *     Added patch by Olivier Cessenat to allow variable length strings.
 *
 *     Mark C. Miller, Sun Oct 25 14:15:00 CET 2020
 *     Added patch by Olivier Cessenat to allow nameschemes.
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTMMAT_FC (int *dbid, FCD_DB name, int *lname, int *nmat, FCD_DB matnames,
            int *lmatnames, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char         **matnms = NULL, *nm = NULL;
    char          *realmatnames = NULL;
    int            i;
    long long      indx;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputmmesh", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

#ifdef CRAY
        if (strcmp(_fcdtocp(matnames), DB_F77NULLSTRING) == 0)
            realmatnames = NULL;
        else
            realmatnames = _fcdtocp(matnames);
#else
        if (strcmp(matnames, DB_F77NULLSTRING) == 0)
            realmatnames = NULL;
        else
            realmatnames = matnames;
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        if (*nmat <= 0)
            API_ERROR("nmat", E_BADARGS);

        if (realmatnames == NULL)
        {
            /*----------------------------------------
             *  Invoke the C function to do the work : using namescheme probably.
             *---------------------------------------*/
            *status = DBPutMultimat(dbfile, nm, *nmat, NULL, optlist);
        }
        else
        {
            matnms = ALLOC_N(char *, *nmat);

            for (indx = 0, i = 0; i < *nmat; i++) {
                if (lmatnames[i] < 0)
                    API_ERROR("lmatnames", E_BADARGS);
                matnms[i] = SW_strndup(&realmatnames[indx], lmatnames[i]);
                if (fortran2DStrLen > 0)
                    indx += fortran2DStrLen;
                else
                    indx += lmatnames[i];
            }
            
            /*----------------------------------------
             *  Invoke the C function to do the work.
             *---------------------------------------*/
            *status = DBPutMultimat(dbfile, nm, *nmat, (char const * const *) matnms, optlist);
            
            for (i = 0; i < *nmat; i++)
                FREE(matnms[i]);
            FREE(matnms);
        }
        FREE(nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                     DBPUTMMATSPECIES_FC
 *
 * Purpose
 *     Write a multi-block species object into a Silo file.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Olivier Cessenat, Sun Oct 25 12:51:41 CET 2020
 *     Added the Fortran equivalent to the C DBPutMultimatspecies with
 *     namescheme.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTMMATSPECIES_FC (int *dbid, FCD_DB name, int *lname, int *nspec, FCD_DB specnames,
            int *lspecnames, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char         **specnms = NULL, *nm = NULL;
    char          *realspecnames = NULL;
    int            i;
    long long      indx;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputmmesh", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

#ifdef CRAY
        if (strcmp(_fcdtocp(specnames), DB_F77NULLSTRING) == 0)
            realspecnames = NULL;
        else
            realspecnames = _fcdtocp(specnames);
#else
        if (strcmp(specnames, DB_F77NULLSTRING) == 0)
            realspecnames = NULL;
        else
            realspecnames = specnames;
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        if (*nspec <= 0)
            API_ERROR("nspec", E_BADARGS);

        if (realspecnames == NULL)
        {
            /*----------------------------------------
             *  Invoke the C function to do the work : using namescheme probably.
             *---------------------------------------*/
            *status = DBPutMultimatspecies(dbfile, nm, *nspec, NULL, optlist);
        }
        else
        {
            specnms = ALLOC_N(char *, *nspec);

            for (indx = 0, i = 0; i < *nspec; i++) {
                if (lspecnames[i] < 0)
                    API_ERROR("lspecnames", E_BADARGS);
                specnms[i] = SW_strndup(&realspecnames[indx], lspecnames[i]);
                if (fortran2DStrLen > 0)
                    indx += fortran2DStrLen;
                else
                    indx += lspecnames[i];
            }
            
            /*----------------------------------------
             *  Invoke the C function to do the work.
             *---------------------------------------*/
            *status = DBPutMultimatspecies(dbfile, nm, *nspec, (char const * const *) specnms, optlist);
            
            for (i = 0; i < *nspec; i++)
                FREE(specnms[i]);
            FREE(specnms);
        }
        FREE(nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBPUTPM_FC
 *
 * Purpose
 *     Writes a point-mesh object to the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 13:10:04 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTPM_FC (int *dbid, FCD_DB name, int *lname, int *ndims, void const *x, void const *y,
          void const *z, int *nels, int *datatype, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char          *nm = NULL;
    void const *coords[3];
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputpm", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        coords[0] = x;
        coords[1] = y;
        coords[2] = z;

        *status = DBPutPointmesh(dbfile, nm, *ndims, coords, *nels,
                                 *datatype, optlist);

        FREE(nm);
        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBPUTPV1_FC
 *
 * Purpose
 *     Write a scalar point variable object into the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 13:26:42 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTPV1_FC (int *dbid, FCD_DB name,
           int *lname, FCD_DB meshname, int *lmeshname,
           void const *var, int *nels, int *datatype, int *optlist_id, int *status)
{
    char          *nm = NULL, *mnm = NULL;
    DBfile        *dbfile = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputpv1", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

        if (*lmeshname <= 0)
            API_ERROR("lmeshname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(meshname), DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(_fcdtocp(meshname), *lmeshname);
#else
        if (strcmp(meshname, DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(meshname, *lmeshname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        *status = DBPutPointvar1(dbfile, nm, mnm, var, *nels,
                                 *datatype, optlist);

        FREE(nm);
        FREE(mnm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBPUTQM_FC
 *
 * Purpose
 *     Write a quadmesh object into the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 13:44:28 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTQM_FC (int *dbid, FCD_DB name, int *lname, FCD_DB xname, int *lxname,
          FCD_DB yname, int *lyname, FCD_DB zname, int *lzname, void const *x,
          void const *y, void const *z, int *dims, int *ndims, int *datatype,
          int *coordtype, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    int            i;
    void const *coords[3];
    char          *coordnames[3], *nm = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputqm", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        for (i = 0; i < 3; i++) {
            coords[i] = NULL;
            coordnames[i] = NULL;
        }

        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        switch (*ndims) {
            case 3:
                if (*lzname <= 0)
                    API_ERROR("lzname", E_BADARGS);
#ifdef CRAY
                if (strcmp(_fcdtocp(zname), DB_F77NULLSTRING) == 0)
                    coordnames[2] = NULL;
                else
                    coordnames[2] = SW_strndup(_fcdtocp(zname), *lzname);
#else
                if (strcmp(zname, DB_F77NULLSTRING) == 0)
                    coordnames[2] = NULL;
                else
                    coordnames[2] = SW_strndup(zname, *lzname);
#endif
                coords[2] = z;
                /*fall through */
            case 2:
                if (*lyname <= 0)
                    API_ERROR("lyname", E_BADARGS);
#ifdef CRAY
                if (strcmp(_fcdtocp(yname), DB_F77NULLSTRING) == 0)
                    coordnames[1] = NULL;
                else
                    coordnames[1] = SW_strndup(_fcdtocp(yname), *lyname);
#else
                if (strcmp(yname, DB_F77NULLSTRING) == 0)
                    coordnames[1] = NULL;
                else
                    coordnames[1] = SW_strndup(yname, *lyname);
#endif
                coords[1] = y;
                /*fall through */
            case 1:
                if (*lxname <= 0)
                    API_ERROR("lxname", E_BADARGS);
#ifdef CRAY
                if (strcmp(_fcdtocp(xname), DB_F77NULLSTRING) == 0)
                    coordnames[0] = NULL;
                else
                    coordnames[0] = SW_strndup(_fcdtocp(xname), *lxname);
#else
                if (strcmp(xname, DB_F77NULLSTRING) == 0)
                    coordnames[0] = NULL;
                else
                    coordnames[0] = SW_strndup(xname, *lxname);
#endif
                coords[0] = x;
                break;
            default:
                API_ERROR("ndims", E_BADARGS);
        }

        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        *status = DBPutQuadmesh(dbfile, nm, (char const * const *) coordnames,
                      coords, dims, *ndims, *datatype, *coordtype, optlist);

        FREE(nm);
        FREE(coordnames[0]);
        FREE(coordnames[1]);
        FREE(coordnames[2]);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBPUTQV1_FC
 *
 * Purpose
 *     Write a single quad variable object to the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 13:52:23 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTQV1_FC (int *dbid, FCD_DB name,
           int *lname, FCD_DB meshname, int *lmeshname,
           void const *var, int *dims, int *ndims, void const *mixvar, int *mixlen,
           int *datatype, int *centering, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char          *nm = NULL, *mnm = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputqv1", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif
        if (*lmeshname <= 0)
            API_ERROR("lmeshname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(meshname), DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(_fcdtocp(meshname), *lmeshname);
#else
        if (strcmp(meshname, DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(meshname, *lmeshname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        *status = DBPutQuadvar1(dbfile, nm, mnm, var, dims, *ndims, FPTR(mixvar),
                                *mixlen, *datatype, *centering, optlist);

        FREE(nm);
        FREE(mnm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBPUTQV_FC
 *
 * Purpose
 *     Write a vector quad field object to the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Olivier Cessenat
 *     Sat Feb  6 18:19:43 CET 2010
 *
 * Modifications
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTQV_FC (int *dbid, FCD_DB vname, int *lvname, FCD_DB mname, int *lmname,
	    int *nvars, FCD_DB varnames, int* lvarnames,
	    void const *vars, int *dims, int *ndims, void const *mixvar, int *mixlen,
	    int *datatype, int *centering, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char          *cvname = NULL, *cmname = NULL;
    DBoptlist     *optlist = NULL;
    char          **cvarnames = NULL;
    char          *names = NULL;
    int           i, j;
    long long     indx;
    void        **cvars = NULL ;
    void        **cmixvar = NULL ;

    API_BEGIN("dbputqv", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lvname <= 0)
            API_ERROR("lvname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(vname), DB_F77NULLSTRING) == 0)
            cvname = NULL;
        else
            cvname = SW_strndup(_fcdtocp(vname), *lvname);
#else
        if (strcmp(vname, DB_F77NULLSTRING) == 0)
            cvname = NULL;
        else
            cvname = SW_strndup(vname, *lvname);
#endif
        if (*lmname <= 0)
            API_ERROR("lmname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(mname), DB_F77NULLSTRING) == 0)
            cmname = NULL;
        else
            cmname = SW_strndup(_fcdtocp(mname), *lmname);
#else
        if (strcmp(mname, DB_F77NULLSTRING) == 0)
            cmname = NULL;
        else
            cmname = SW_strndup(mname, *lmname);
#endif

        /*------------------------------
         *  Create the char ** array to variable names as in dbaddcaopt
         *-----------------------------*/
        if (strcmp(varnames, DB_F77NULLSTRING) == 0)
            names = NULL;
        else
            names = varnames;

        if (*nvars <= 0)
            API_ERROR("nvars", E_BADARGS);

        cvarnames = ALLOC_N(char *, *nvars);
        for (indx = 0, i = 0; i < *nvars; i++) {
	  if (lvarnames[i] < 0)
	    API_ERROR("lvarnames", E_BADARGS);

	  cvarnames[i] = SW_strndup(&names[indx], lvarnames[i]);
	  /* Allow for a matrix or array form */
	  if (fortran2DStrLen > 0) {
	    /* Matrix API */
	    indx += fortran2DStrLen;
	  } else {
	    /* More General Vector API */
	    indx += lvarnames[i];
	  }
        }

        /*------------------------------
         *  Create the variable data pointers addresses:
         *-----------------------------*/
	/* Compute the size of the arrays in bytes */
	indx = dims[0] ;
	for (j=1; j<*ndims; j++ ) {
	  indx *= dims[j] ;
	}
        indx *= db_GetMachDataSize(*datatype);

	/* Now convert the Fortran data arrays into C arrays of data */

	if ((*(int *)vars) != DB_F77NULL) {
	  cvars = (void **)malloc(sizeof(void*) * (*nvars));
	  /* Make pointers to Fortran address in vars array */
	  for (i=0;i<*nvars;i++) {
	    cvars[i] = (void*)((char*) vars + i*indx);
	  }
	} else {
	  API_ERROR("vars", E_BADARGS);
	}
	if ((*(int *)mixvar) != DB_F77NULL) {
	  /* Now convert the Fortran data array into a C array of data */
	  cmixvar = (void **)malloc(sizeof(void*) * (*nvars));
	  /* Make pointers to Fortran address in vars array */
	  for (i=0;i<*nvars;i++) {
	    cmixvar[i] = (void*)((char*) mixvar + i*indx);
	  }
	}

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        *status = DBPutQuadvar(dbfile, cvname, cmname, *nvars,
                      (char const * const *) cvarnames, cvars, dims, *ndims,
                      cmixvar, *mixlen, *datatype, *centering, optlist);

	/* Remove pointers to pointers arrays */
	if (cmixvar != NULL) 
	  FREE(cmixvar)
	if (cvars != NULL) 
	  FREE(cvars)

	/* Remove strings copies */ 
        for (i = 0; i < *nvars; i++)
	    FREE(cvarnames[i]);
        FREE(cvarnames);
        FREE(cmname);
        FREE(cvname);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBPUTUM_FC
 *
 * Purpose
 *     Write a UCD mesh object into the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 13:58:25 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTUM_FC (int *dbid, FCD_DB name, int *lname, int *ndims, void const *x, void const *y,
          void const *z, FCD_DB xname, int *lxname, FCD_DB yname, int *lyname,
          FCD_DB zname, int *lzname, int *datatype, int *nnodes, int *nzones,
          FCD_DB zlname, int *lzlname, FCD_DB flname, int *lflname,
          int *optlist_id, int *status)
{
    char          *nm = NULL, *zlnm = NULL, *flnm = NULL, *coordnames[3];
    void const    *coords[3];
    DBfile        *dbfile = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputum", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        memset(coords, 0, sizeof(coords));
        memset(coordnames, 0, sizeof(coordnames));

        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        switch (*ndims) {
            case 3:
                if (*lzname <= 0)
                    API_ERROR("lzname", E_BADARGS);
#ifdef CRAY
                if (strcmp(_fcdtocp(zname), DB_F77NULLSTRING) == 0)
                    coordnames[2] = NULL;
                else
                    coordnames[2] = SW_strndup(_fcdtocp(zname), *lzname);
#else
                if (strcmp(zname, DB_F77NULLSTRING) == 0)
                    coordnames[2] = NULL;
                else
                    coordnames[2] = SW_strndup(zname, *lzname);
#endif
                coords[2] = z;
                /*fall through */
            case 2:
                if (*lyname <= 0)
                    API_ERROR("lyname", E_BADARGS);
#ifdef CRAY
                if (strcmp(_fcdtocp(yname), DB_F77NULLSTRING) == 0)
                    coordnames[1] = NULL;
                else
                    coordnames[1] = SW_strndup(_fcdtocp(yname), *lyname);
#else
                if (strcmp(yname, DB_F77NULLSTRING) == 0)
                    coordnames[1] = NULL;
                else
                    coordnames[1] = SW_strndup(yname, *lyname);
#endif
                coords[1] = y;
                /*fall through */
            case 1:
                if (*lxname <= 0)
                    API_ERROR("lxname", E_BADARGS);
#ifdef CRAY
                if (strcmp(_fcdtocp(xname), DB_F77NULLSTRING) == 0)
                    coordnames[0] = NULL;
                else
                    coordnames[0] = SW_strndup(_fcdtocp(xname), *lxname);
#else
                if (strcmp(xname, DB_F77NULLSTRING) == 0)
                    coordnames[0] = NULL;
                else
                    coordnames[0] = SW_strndup(xname, *lxname);
#endif
                coords[0] = x;
                break;
            default:
                API_ERROR("ndims", E_BADARGS);
        }

        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

        if (*lflname <= 0)
            flnm = NULL;
        else
#ifdef CRAY
        if (strcmp(_fcdtocp(flname), DB_F77NULLSTRING) == 0)
            flnm = NULL;
        else
            flnm = SW_strndup(_fcdtocp(flname), *lflname);
#else
        if (strcmp(flname, DB_F77NULLSTRING) == 0)
            flnm = NULL;
        else
            flnm = SW_strndup(flname, *lflname);
#endif

        if (*lzlname <= 0)
            zlnm = NULL;
        else
#ifdef CRAY
        if (strcmp(_fcdtocp(zlname), DB_F77NULLSTRING) == 0)
            zlnm = NULL;
        else
            zlnm = SW_strndup(_fcdtocp(zlname), *lzlname);
#else
        if (strcmp(zlname, DB_F77NULLSTRING) == 0)
            zlnm = NULL;
        else
            zlnm = SW_strndup(zlname, *lzlname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        *status = DBPutUcdmesh(dbfile, nm, *ndims, (char const * const *) coordnames,
                               coords, *nnodes, *nzones, zlnm, flnm,
                               *datatype, optlist);

        FREE(nm);
        FREE(zlnm);
        FREE(flnm);
        FREE(coordnames[0]);
        FREE(coordnames[1]);
        FREE(coordnames[2]);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBPUTUV1_FC
 *
 * Purpose
 *     Write one UCD variable object to the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 14:07:14 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTUV1_FC (int *dbid, FCD_DB name,
           int *lname, FCD_DB meshname, int *lmeshname,
           void const *var, int *nels, void const *mixvar, int *mixlen, int *datatype,
           int *centering, int *optlist_id, int *status)
{
    char          *nm = NULL, *mnm = NULL;
    DBfile        *dbfile = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputuv1", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif
        if (*lmeshname <= 0)
            API_ERROR("lmeshname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(meshname), DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(_fcdtocp(meshname), *lmeshname);
#else
        if (strcmp(meshname, DB_F77NULLSTRING) == 0)
            mnm = NULL;
        else
            mnm = SW_strndup(meshname, *lmeshname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        *status = DBPutUcdvar1(dbfile, nm, mnm, var, *nels, FPTR(mixvar), *mixlen,
                               *datatype, *centering, optlist);

        FREE(nm);
        FREE(mnm);
        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBPUTZL_FC
 *
 * Purpose
 *     Write a zonelist object do the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 14:15:03 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTZL_FC (int *dbid, FCD_DB name, int *lname, int *nzones, int *ndims,
          int *nodelist, int *lnodelist, int *origin, int *shapesize,
          int *shapecnt, int *nshapes, int *status)
{
    DBfile        *dbfile = NULL;
    char          *nm = NULL;

    API_BEGIN("dbputzl", int, -1) {
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        *status = DBPutZonelist(dbfile, nm, *nzones, *ndims,
                                nodelist, *lnodelist, *origin,
                                shapesize, shapecnt, *nshapes);

        FREE(nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBPUTZL2_FC
 *
 * Purpose
 *     Write a zonelist object do the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller
 *     July 19, 2005
 *
 * Modifications:
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/

SILO_API FORTRAN
DBPUTZL2_FC (int *dbid, FCD_DB name, int *lname, int *nzones, int *ndims,
          int *nodelist, int *lnodelist, int *origin, int *lo_offset,
          int *hi_offset, int *shapetype, int *shapesize,
          int *shapecnt, int *nshapes, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char          *nm = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputzl2", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        *status = DBPutZonelist2(dbfile, nm, *nzones, *ndims,
                                nodelist, *lnodelist, *origin,
                               *lo_offset, *hi_offset, shapetype,
                                shapesize, shapecnt, *nshapes, optlist);

        FREE(nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBRDVAR_FC
 *
 * Purpose
 *     Read a simple variable from the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 14:21:54 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBRDVAR_FC (int *dbid, FCD_DB varname, int *lvarname, void *ptr)
{
    int            iret;
    char          *varnm = NULL;
    DBfile        *dbfile = NULL;

    API_BEGIN("dbrdvar", int, -1) {
      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
        if (*lvarname <= 0)
            API_ERROR("lvarname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(varname), DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(_fcdtocp(varname), *lvarname);
#else
        if (strcmp(varname, DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(varname, *lvarname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        iret = DBReadVar(dbfile, varnm, ptr);
        FREE(varnm);
        API_RETURN(iret);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                  DBRDVARSLICE_FC
 *
 * Purpose
 *     Read a slice of a simple variable from the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     brugger@sgibird
 *     Thu Feb 16 08:34:21 PST 1995
 *
 * Modifications
 *      Sean Ahern Tue Feb 21 14:43:42 PST 1995
 *      Changed offset to offset2 in the DBReadVarSlice call.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBRDVARSLICE_FC (int *dbid, FCD_DB varname, int *lvarname, int *offset,
               int *length, int *stride, int *ndims, void *ptr)
{
    int            i;
    int            iret;
    char          *varnm = NULL;
    int            offset2[MAXDIMS_VARWRITE];
    DBfile        *dbfile = NULL;

    API_BEGIN("dbrdvarslice", int, -1) {
      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
        if (*lvarname <= 0)
            API_ERROR("lvarname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(varname), DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(_fcdtocp(varname), *lvarname);
#else
        if (strcmp(varname, DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(varname, *lvarname);
#endif

        for (i = 0; i < *ndims && i < MAXDIMS_VARWRITE; i++)
            offset2[i] = offset[i] - 1;

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        iret = DBReadVarSlice(dbfile, varnm, offset2, length,
                              stride, *ndims, ptr);
        FREE(varnm);
        API_RETURN(iret);
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
 * Programmer:  Sean Ahern
 *              ahern@llnl.gov
 *              Tue Jan  7 16:37:29 PST 1997
 *
 * Modifications:
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *------------------------------------------------------------------------- */
SILO_API FORTRAN
DBWRITESLICE_FC (int *dbid, FCD_DB varname, int *lvarname, void *values,
               int *datatype, int *offset, int *length, int *stride,
               int *dims, int *ndims)
{
    int             iret;
    DBfile         *dbfile = NULL;
    char           *varnm = NULL;
    int             i;
    int             offset2[MAXDIMS_VARWRITE];

    API_BEGIN("dbwriteslice", int, -1)
    {
        /* Duplicate all ASCII strings. */
        if (*lvarname <= 0)
            API_ERROR("lvarname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(varname), DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(_fcdtocp(varname), *lvarname);
#else
        if (strcmp(varname, DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(varname, *lvarname);
#endif

        for (i = 0; i < *ndims && i < MAXDIMS_VARWRITE; i++)
            offset2[i] = offset[i] - 1;

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        iret = DBWriteSlice(dbfile, varnm, values, *datatype, offset2,
                            length, stride, dims, *ndims);

        FREE(varnm);
        API_RETURN(iret);
    }
    API_END_NOPOP;                     /* BEWARE: If API_RETURN above is
                                        * removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBSETDIR_FC
 *
 * Purpose
 *     Set the current directory within the database
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 14:26:23 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBSETDIR_FC (int *dbid, FCD_DB pathname, int *lpathname)
{
    int            err;
    char          *path = NULL;
    DBfile        *dbfile = NULL;

    API_BEGIN("dbsetdir", int, -1) {
      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
        if (*lpathname <= 0)
            API_ERROR("lpathname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(pathname), DB_F77NULLSTRING) == 0)
            path = NULL;
        else
            path = SW_strndup(_fcdtocp(pathname), *lpathname);
#else
        if (strcmp(pathname, DB_F77NULLSTRING) == 0)
            path = NULL;
        else
            path = SW_strndup(pathname, *lpathname);
#endif

      /*------------------------------
       *  Set directory.
       *-----------------------------*/

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        err = DBSetDir(dbfile, path);
        FREE(path);
        API_RETURN(err);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                  DBSHOWERRORS_FC
 *
 * Purpose
 *     Same as DBShowErrors except function pointers cannot be
 *     passed in Fortran, so the second argument is always NULL
 *     causing the default error handler to be used.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 14:30:19 EST 1994
 *
 * Modifications
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBSHOWERRORS_FC (int *mode)
{
    DBShowErrors(*mode, NULL);

    return 0;
}

SILO_API FORTRAN
DBERRNO_FC()
{
    API_BEGIN("dberrno", int, -1) {
        API_RETURN(DBErrno());
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

SILO_API FORTRAN
DBERRLVL_FC()
{
    API_BEGIN("dberrlvl", int, -1) {
        API_RETURN(DBErrlvl());
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBWRITE_FC
 *
 * Purpose
 *     Write a simple variable to the database
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     robb@cloud
 *     Mon Nov 28 14:33:36 EST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBWRITE_FC (int *dbid, FCD_DB varname, int *lvarname, void *var, int *dims,
          int *ndims, int *datatype)
{
    char          *varnm = NULL;
    DBfile        *dbfile = NULL;

    API_BEGIN("dbwrite", int, -1) {
      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
        if (*lvarname <= 0)
            API_ERROR("lvarname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(varname), DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(_fcdtocp(varname), *lvarname);
#else
        if (strcmp(varname, DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(varname, *lvarname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        (void) DBWrite(dbfile, varnm, var, dims, *ndims, *datatype);
        FREE(varnm);
    }
    API_END;

    return(0);
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBPUTCA_FC
 *
 * Purpose
 *     Write a compound array object into the given SILO file.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Return
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     matzke@viper
 *     Tue Oct 25 14:35:37 PDT 1994
 *
 *  Modifications
 *     Robb Matzke, Fri Dec 2 12:57:04 PST 1994
 *     Removed SCORE memory management.
 *
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTCA_FC (int *dbid, FCD_DB _name, int *lname, FCD_DB enames, int *width,
          int *elengths, int *nelems, void *values, int *datatype,
          int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    int            i, j, nvalues;
    char          *array_name = NULL, **elemnames = NULL;
    char          *realenames = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbputca", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        /*
         * Duplicate all ascii strings and count the total number
         * of values.
         */
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(_name), DB_F77NULLSTRING) == 0)
            array_name = NULL;
        else
            array_name = SW_strndup(_fcdtocp(_name), *lname);
#else
        if (strcmp(_name, DB_F77NULLSTRING) == 0)
            array_name = NULL;
        else
            array_name = SW_strndup(_name, *lname);
#endif

#ifdef CRAY
        realenames = _fcdtocp(enames);
#else
        realenames = enames;
#endif
        if (*nelems <= 0)
            API_ERROR("nelems", E_BADARGS);
        elemnames = ALLOC_N(char *, *nelems);

        for (i = nvalues = 0; i < *nelems; i++) {
            j = *width;
            while (j > 0 && ' ' == realenames[j - 1])
                --j;
            elemnames[i] = SW_strndup(realenames, j);
            realenames += *width;
            nvalues += elengths[i];
        }

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        *status = DBPutCompoundarray(dbfile, array_name, (char const * const *) elemnames,
                      elengths, *nelems, values, nvalues, *datatype, optlist);

        /*
         * Free all temporary memory.
         */
        FREE(array_name);
        for (i = 0; i < *nelems; i++)
            FREE(elemnames[i]);
        FREE(elemnames);
        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBGETCA_FC
 *
 * Purpose
 *     Retrieve a compound array object from the file. The
 *     caller should have already allocated the following output
 *     parameters: enames, lenames, elengths, and values.
 *
 * Notes
 *     This function was designed to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     matzke@viper
 *     Wed Oct 26 10:53:38 PDT 1994
 *
 * Modifications
 *     matzke@viper, Wed Oct 26 11:15:02 PDT 1994
 *     The following parameters may be null pointers, in which case
 *     this function doesn't do anything with them.  This is used by
 *     the DBFGETCAFC and DBINQCA_FC functions.
 *
 *         enames, lenames, elengths, datatype
 *
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBGETCA_FC (int *dbid, FCD_DB _name, int *lname, int *width, FCD_DB enames,
          int *elengths, int *nelems, void *values, int *nvalues,
          int *datatype)
{
    DBfile        *dbfile = NULL;
    DBcompoundarray *ca = NULL;
    char          *array_name = NULL;
    char          *realenames = NULL;
    int            to_copy, i;

    API_BEGIN("dbgetca", int, -1) {
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(_name), DB_F77NULLSTRING) == 0)
            array_name = NULL;
        else
            array_name = SW_strndup(_fcdtocp(_name), *lname);
#else
        if (strcmp(_name, DB_F77NULLSTRING) == 0)
            array_name = NULL;
        else
            array_name = SW_strndup(_name, *lname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        if (NULL == (ca = DBGetCompoundarray(dbfile, array_name))) {
            API_ERROR("DBGetCompoundarray", E_CALLFAIL);
        }

#ifdef CRAY
        if ((_fcdtocp(enames) == NULL) ||
            (strcmp(_fcdtocp(enames), DB_F77NULLSTRING) == 0)
           )
            realenames = NULL;
        else
            realenames = _fcdtocp(enames);
#else
        if ((enames == NULL) || (strcmp(enames, DB_F77NULLSTRING) == 0))
            realenames = NULL;
        else
            realenames = enames;
#endif

        /*
         * Copy fields of `ca' to the output parameters.
         */
        if (realenames)
            memset(realenames, ' ', *width * (ca->nelems));
        for (i = 0; i < ca->nelems; i++) {
            if (realenames) {
                to_copy = strlen(ca->elemnames[i]);
                if (to_copy > *width)
                    to_copy = *width;
                memcpy(realenames, ca->elemnames[i], to_copy);
                realenames += *width;
            }
            if (elengths)
                elengths[i] = ca->elemlengths[i];
        }
        if (nelems)
            *nelems = ca->nelems;
        if (nvalues)
            *nvalues = ca->nvalues;
        if (datatype)
            *datatype = ca->datatype;

        if (values) {
            memcpy(values, ca->values,
                   ca->nvalues * db_GetMachDataSize(ca->datatype));
        }

        FREE(array_name);
        DBFreeCompoundarray(ca);
    }
    API_END;

    return(0);
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBFGETCA_FC
 *
 * Purpose
 *     Same as DBGETCA_FC except not so many parameters.  Similar
 *     to DBFgetCompoundarray, except return vector is through
 *     a previously allocated parameter.
 *
 * Notes
 *     This function is designed to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     matzke@viper
 *     Wed Oct 26 11:10:35 PDT 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBFGETCA_FC (int *dbid, FCD_DB _name, int *lname, void *values, int *nvalues)
{
    API_BEGIN("dbfgetca", int, -1) {
#if CRAY
        _fcd           fstring;
        
        fstring = _cptofcd(DB_F77NULLSTRING, strlen(DB_F77NULLSTRING));

        API_RETURN(DBGETCA_FC (dbid, _name, lname,
                             NULL, fstring, NULL, NULL,
                             values, nvalues, NULL));
#else
        API_RETURN(DBGETCA_FC (dbid, _name, lname,
                             NULL, NULL, NULL, NULL,
                             values, nvalues, NULL));
#endif
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Function:    DBPUTCURVE_FC
 *
 * Purpose: Write a curve object into the given PDB file.
 *
 * Return:  Success:    0
 *
 *      Failure:    -1
 *
 * Programmer:  Robb Matzke
 *      robb@callisto.nuance.com
 *      May 17, 1996
 *
 * Modifications:
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *------------------------------------------------------------------------- */
SILO_API FORTRAN
DBPUTCURVE_FC (int *dbid, FCD_DB _name, int *lname, void const *xvals, void const *yvals,
              int *datatype, int *npts, int *optlist_id, int *status)
{

   DBfile   *dbfile ;
   DBoptlist    *optlist ;
   char     *name ;

   API_BEGIN ("dbputcurve", int, -1) {
      if (*lname<=0)
     API_ERROR ("lname", E_BADARGS) ;

      optlist = (DBoptlist *) DBFortranAccessPointer (*optlist_id) ;

#ifdef CRAY
      if (!strcmp(_fcdtocp(_name), DB_F77NULLSTRING)) {
     name = NULL ;
      } else {
     name = SW_strndup (_fcdtocp(_name), *lname) ;
      }
#else
      if (!strcmp(_name, DB_F77NULLSTRING)) {
     name = NULL ;
      } else {
     name = SW_strndup (_name, *lname) ;
      }
#endif

      dbfile = (DBfile *) DBFortranAccessPointer (*dbid) ;
      *status = DBPutCurve (dbfile, name, xvals, yvals, *datatype,
                *npts, optlist) ;
      FREE(name);
   } API_END;
   return 0 ;
}

/*-------------------------------------------------------------------------
 * Function:    DBGETCURVE_FC
 *
 * Purpose: Reads a curve from a PDB file.  The caller supplies the
 *      output buffers for the x and y values but specifies how
 *          many values they're ready to accept.  The total number
 *      of points in the curve is returned with the NPTS output
 *      argument.
 *
 * Return:  Success:    0
 *
 *      Failure:    -1
 *
 * Programmer:  Robb Matzke
 *      robb@callisto.nuance.com
 *      May 17, 1996
 *
 * Modifications:
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *------------------------------------------------------------------------ */
SILO_API FORTRAN
DBGETCURVE_FC (int *dbid, FCD_DB _name, int *lname, int *maxpts, void *xvals,
              void *yvals, int *datatype, int *npts)
{

   DBfile   *dbfile ;
   char     *name ;
   DBcurve  *cu ;
   int      elem_size, total_size ;

   API_BEGIN ("dbgetcurve", int, -1) {

      /*
       * Check for obvious errors.
       */
      if (*lname <= 0) API_ERROR ("lname", E_BADARGS) ;
      if (*maxpts < 0) API_ERROR ("maxpts", E_BADARGS) ;

      /*
       * Convert arguments from Fortran to C.
       */
#ifdef CRAY
      if (!strcmp (_fcdtocp(_name), DB_F77NULLSTRING)) {
     name = NULL ;
      } else {
     name = SW_strndup (_fcdtocp (_name), *lname) ;
      }
#else
      if (!strcmp (_name, DB_F77NULLSTRING)) {
     name = NULL ;
      } else {
     name = SW_strndup (_name, *lname) ;
      }
#endif
      dbfile = (DBfile *) DBFortranAccessPointer (*dbid) ;

      /*
       * Get the curve.
       */
      if (NULL==(cu=DBGetCurve(dbfile, name))) {
     API_ERROR ("DBGetCurve", E_CALLFAIL) ;
      }

      /*
       * Copy values to the output arguments.
       */
      *datatype = cu->datatype ;
      *npts = cu->npts ;
      elem_size = db_GetMachDataSize (cu->datatype) ;
      total_size = elem_size * MIN(*maxpts, cu->npts) ;
      memcpy (xvals, cu->x, total_size) ;
      memcpy (yvals, cu->y, total_size) ;
      DBFreeCurve (cu) ;
      if (name) free(name);
   } API_END ;
   return 0 ;
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBINQCA_FC
 *
 * Purpose
 *     Inquire compound array object attributes.
 *
 * Notes
 *     This function is designed to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     matzke@viper
 *     Wed Oct 26 11:19:51 PDT 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBINQCA_FC (int *dbid, FCD_DB _name, int *lname, int *maxwidth, int *nelems,
          int *nvalues, int *datatype)
{
    DBfile        *dbfile = NULL;
    char          *array_name = NULL, **elemnames = NULL;
    int            i, status, len;

    API_BEGIN("dbinqca", int, -1) {
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(_name), DB_F77NULLSTRING) == 0)
            array_name = NULL;
        else
            array_name = SW_strndup(_fcdtocp(_name), *lname);
#else
        if (strcmp(_name, DB_F77NULLSTRING) == 0)
            array_name = NULL;
        else
            array_name = SW_strndup(_name, *lname);
#endif
        *maxwidth = 0;

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        status = DBInqCompoundarray(dbfile, array_name,
                                    &elemnames, NULL, nelems,
                                    nvalues, datatype);
        if (status < 0)
            API_RETURN(-1);

        for (i = 0; i < *nelems; i++) {
            len = strlen(elemnames[i]);
            if (len > *maxwidth)
                *maxwidth = len;
            FREE(elemnames[i]);
        }
        FREE(elemnames);
        FREE(array_name);
    }
    API_END;

    return(0);
}

/*----------------------------------------------------------------------
 * Routine                                                     DBADDICMP
 *
 * Purpose
 *     Add an integer component to the given component list.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Robb Matzke, Thu Nov 10 17:46:06 EST 1994
 *     Added error mechanism.
 *
 *     Robb Matzke, Thu Nov 10 17:50:49 EST 1994
 *     Changed third parameter to be an integer.  If this function
 *     ever worked, it was pure luck!
 *
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDICMP_FC (int *obj_id, char *compname, int *val)
{
    DBobject      *object = NULL;

    API_BEGIN("dbaddicmp", int, -1) {
        object = (DBobject *) DBFortranAccessPointer(*obj_id);
        API_RETURN(DBAddIntComponent(object, compname, *val));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                  DBADDRCMP_FC
 *
 * Purpose
 *     Add a real compname to the given compname list.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Robb Matzke, Thu Nov 10 17:55:43 EST 1994
 *     Fixed third parameter--it should be a `float*' instead of
 *     a `char*'.
 *
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDRCMP_FC (int *object_id, char *compname, float *val)
{
    DBobject      *object = NULL;

    API_BEGIN("dbaddrcmp", int, -1) {
        object = (DBobject *) DBFortranAccessPointer(*object_id);
        API_RETURN(DBAddFltComponent(object, compname, (double) *val));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                  DBADDCCMP_FC
 *
 * Purpose
 *     Add a character component to the given comp list.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Warning
 *     THIS FUNCTION CAUSES A MEMORY LEAK. The temporary string,
 *     cval, is never freed.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Robb Matzke, Thu Nov 10 18:00:17 EST 1994
 *     Added error mecanism.
 *
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Eric Brugger, Wed Jun 18 14:18:16 PDT 1997
 *     I removed the return statement at the end of the routine which
 *     can never be reached.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDCCMP_FC (int *object_id, char *compname, char *val)
{
    DBobject      *object = NULL;

    API_BEGIN("dbaddccmp", int, -1) {
        object = (DBobject *) DBFortranAccessPointer(*object_id);
        API_RETURN(DBAddStrComponent(object, compname, val));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */ ;
}

/*----------------------------------------------------------------------
 * Routine                                                 DBMKOBJECT_FC
 *
 * Purpose
 *     Create an component list and return its identifier.
 *
 * Notes
 *    This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Robb Matzke, Thu Nov 10 18:02:04 EST 1994
 *     Added error mechanism.
 *
 *     Robb Matzke, Thu Nov 10 18:04:51 EST 1994
 *     The interface for this function needs to change!  The underlying
 *     C-function, DBMakeObject, also requires an object name and
 *     object type, parameters that are not supplied for this Fortran
 *     version.  For now, we will call this an error since we aren't
 *     changing the interface on this pass...
 *
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBMKOBJECT_FC (int *maxcomponents, int *object_id)
{
    API_BEGIN("dbmkobject", int, -1) {
        if (*maxcomponents <= 0)
            API_ERROR("max components", E_BADARGS);
        *object_id = -1;

        API_ERROR("INTERFACE WILL CHANGE", E_NOTIMP);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                               DBFREEOBJECT_FC
 *
 * Purpose
 *     Free an object list.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Modifications
 *     Robb Matzke, Thu Nov 10 18:08:32 EST 1994
 *     Added error mechanism.
 *
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Eric Brugger, Wed Mar  8 09:11:28 PST 1995
 *     I corrected the name from DBFREEOBJECT to DBFREEOBJECT_FC.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBFREEOBJECT_FC (int *object_id)
{
    DBobject      *obj = NULL;

    API_BEGIN("dbfreeobject", int, -1) {
        obj = (DBobject *) DBFortranAccessPointer(*object_id);
        *object_id = -1;
        API_RETURN(DBFreeObject(obj));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                      DBGETQV1_FC
 *
 * Purpose
 *     Read a quadrilateral variable from the database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     matzke@viper
 *     Tue Nov 29 13:50:16 PST 1994
 *
 * Modifications
 *     Eric Brugger, Tue Feb  7 08:09:26 PST 1995
 *     I replaced API_END with API_END_NOPOP.
 *
 *     Sean Ahern, Tue Mar 21 17:12:07 PST 1995
 *     I added support for the _fcd fortran character
 *     array descriptor.  This is only a Cray thing.
 *
 *     Sean Ahern, Mon Apr 10 20:04:23 PDT 1995
 *     Added a DBfile lookup in a global array using
 *     DBFortranAccessPointer.
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBGETQV1_FC (int *dbid, FCD_DB varname, int *lvarname, void *var, int *dims,
           int *ndims, void *mixvar, int *mixlen, int *datatype, int *centering)
{
    char           tmpstr[64];
    char          *varnm = NULL;
    int            nbytes, i;
    DBquadvar     *qv = NULL;
    DBfile        *dbfile = NULL;

    API_BEGIN("dbgetqv1", int, -1) {
        if (*lvarname <= 0)
            API_ERROR("lvarname", E_BADARGS);
#ifdef CRAY
        if (strcmp(_fcdtocp(varname), DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(_fcdtocp(varname), *lvarname);
#else
        if (strcmp(varname, DB_F77NULLSTRING) == 0)
            varnm = NULL;
        else
            varnm = SW_strndup(varname, *lvarname);
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        if (NULL == (qv = DBGetQuadvar(dbfile, varnm))) {
            API_ERROR("DBGetQuadvar", E_CALLFAIL);
        }

      /*------------------------------
       *  Copy the quad var into the
       *  supplied space. Assign the
       *  misc. attributes.
       *-----------------------------*/
        nbytes = qv->nels * db_GetMachDataSize(qv->datatype);
        memcpy(var, qv->vals[0], nbytes);

        *ndims = qv->ndims;
        *centering = (qv->align[0] == 0.) ? DB_NODECENT : DB_ZONECENT;
        *datatype = qv->datatype;
        for (i = 0; i < qv->ndims; i++)
            dims[i] = qv->dims[i];

      /*------------------------------------------------------------
       *  If there was mixed data, copy that too.
       *
       *  Kludge city!!! I'm assuming name of mixed component is
       *  'varname_mix'. This info should be kept in DBquadvar
       *  somewhere.
       *-----------------------------------------------------------*/
        if ((*(int *)mixvar) != DB_F77NULL) {
            strcpy(tmpstr, varnm);
            strcat(tmpstr, "_mix");

            *mixlen = DBGetVarLength(dbfile, tmpstr);
            DBReadVar(dbfile, tmpstr, mixvar);
        }

        DBFreeQuadvar(qv);
        FREE(varnm);
    }
    API_END;

    return(0);
}

/*-------------------------------------------------------------------------
 * Routine                                                     DBINQFILE_FC
 *
 * Purpose
 *     Tests to see if the filename specified is a Silo file.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns zero on success, -1 on failure.
 *
 * Programmer
 *     Hank Childs
 *     Tue Feb 29 17:12:47 PST 2000
 *
 * Modifications
 *
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBINQFILE_FC (FCD_DB pathname, int *lpathname, int *is_file)
{
    char          *path = NULL;

    API_BEGIN("dbinqfile", int, -1) {
      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
        if (*lpathname <= 0)
            API_ERROR("lpathname", E_BADARGS);
        if (strcmp(pathname, DB_F77NULLSTRING) == 0)
            path = NULL;
        else
            path = SW_strndup(pathname, *lpathname);

      /*----------------------------------------------------
       *  Make call to inquire if file is a valid Silo file.
       *----------------------------------------------------*/
        *is_file = DBInqFile(path);
        FREE(path);
        API_RETURN(0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                 DBSET2DSTRLEN_FC
 *
 * Purpose
 *     Set the 2D string length used for a 2D arrays of strings in fortran
 *     interface
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller 
 *     Thu Jul 28 10:41:14 PDT 2005 
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBSET2DSTRLEN_FC (int *len)
{
    int old = fortran2DStrLen;
    API_BEGIN("dbset2dstrlen", int, -1) {
        fortran2DStrLen = *len;
        API_RETURN(old);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                 DBGET2DSTRLEN_FC
 *
 * Purpose
 *     Get the 2D string length used for a 2D arrays of strings in fortran
 *     interface
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller 
 *     Thu Jul 28 10:41:14 PDT 2005 
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBGET2DSTRLEN_FC ()
{
    API_BEGIN("dbget2dstrlen", int, -1) {
        API_RETURN(fortran2DStrLen);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                   DBSETOVRWRT_FC
 *
 * Purpose: set value for flag controlling overwrite
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller 
 *     Mon May  1 15:50:09 PDT 2006 
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBSETOVRWRT_FC (int *allow)
{
    API_BEGIN("dbsetovrwrt", int, -1) {
        API_RETURN(DBSetAllowOverwrites(*allow));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                   DBGETOVRWRT_FC
 *
 * Purpose: Get value for flag controlling overwrite
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller 
 *     Mon May  1 15:50:09 PDT 2006 
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBGETOVRWRT_FC ()
{
    API_BEGIN("dbgetovrwrt", int, -1) {
        API_RETURN(DBGetAllowOverwrites());
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                   DBSETCKSUMS_FC
 *
 * Purpose: set value for flag controlling checksums 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller 
 *     Mon May  1 15:50:09 PDT 2006 
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBSETCKSUMS_FC (int *enable)
{
    API_BEGIN("dbsetcksums", int, -1) {
        API_RETURN(DBSetEnableChecksums(*enable));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                   DBGETCKSUMS_FC
 *
 * Purpose: Get value for flag controlling checksums
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller 
 *     Mon May  1 15:50:09 PDT 2006 
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBGETCKSUMS_FC ()
{
    API_BEGIN("dbgetcksums", int, -1) {
        API_RETURN(DBGetEnableChecksums());
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}
/*----------------------------------------------------------------------
 * Routine                                              DBSETCOMPRESS_FC
 *
 * Purpose
 *     Set the compression information.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Warning
 *     THIS FUNCTION CAUSES A MEMORY LEAK. The temporary string,
 *     cval, is never freed.  It is put in the optlist instead.
 *
 * Returns
 *
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Thomas R. Treadway, Tue Feb 27 14:10:03 PST 2007
 *
 *
 *
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Mon Dec  6 17:36:57 PST 2010
 *     Patched as per Olivier Cessenat suggestion.
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBSETCOMPRESS_FC (FCD_DB cvalue, int *lcvalue)
{
    char          *cval = NULL;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbsetcompress", int, -1) {
        if (strcmp(cvalue, DB_F77NULLSTRING) < 0 && *lcvalue > 0)
            cval = SW_strndup(cvalue, *lcvalue);

        DBSetCompression(cval);
        FREE(cval);
    }
    API_END;

    return(0);
}
/*----------------------------------------------------------------------
 * Routine                                              DBGETCOMPRESS_FC
 *
 * Purpose
 *     Get the compression information.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Warning
 *     THIS FUNCTION CAUSES A MEMORY LEAK. The temporary string,
 *     cval, is never freed.  It is put in the optlist instead.
 *
 * Returns
 *
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Thomas R. Treadway, Tue Feb 27 14:10:03 PST 2007
 *
 *
 *
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBGETCOMPRESS_FC (FCD_DB cvalue, int *lcvalue)
{
    char const *cval = NULL;

    API_BEGIN("dbgetcompression", int, -1) {
        if (*lcvalue <= 0)
            API_ERROR("lcvalue", E_BADARGS);

        cval = DBGetCompression();
        cvalue = SW_strndup(cval, *lcvalue);

        API_RETURN(0);
    }
    API_END;

    return(0);
}

/*-------------------------------------------------------------------------
 * Routine                                                   DBSETHDFNMS_FC
 *
 * Purpose: set value for flag controlling friendly hdf5 names 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Thu Apr 19 19:26:11 PDT 2007 
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBSETHDFNMS_FC (int *enable)
{
    API_BEGIN("dbsethdfnms", int, -1) {
        API_RETURN(DBSetFriendlyHDF5Names(*enable));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                   DBGETHDFNMS_FC
 *
 * Purpose: Get value for flag controlling friendly hdf5 names 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Thu Apr 19 19:26:11 PDT 2007 
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBGETHDFNMS_FC ()
{
    API_BEGIN("dbgethdfnms", int, -1) {
        API_RETURN(DBGetFriendlyHDF5Names());
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                  DBSETDEPWARN_FC
 *
 * Purpose: set maximum deprecation warnings issued by silo lib 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Thu Oct 11 20:56:41 PDT 2007
 *
 * Modifications:
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBSETDEPWARN_FC (int *max_count)
{
    API_BEGIN("dbsetdepwarn", int, -1) {
        API_RETURN(DBSetDeprecateWarnings(*max_count));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                  DBGETDEPWARN_FC
 *
 * Purpose: Get max. number of deprecation warnings issued by Silo lib 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Thu Oct 11 20:56:41 PDT 2007
 *
 * Modifications:
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBGETDEPWARN_FC ()
{
    API_BEGIN("dbgetdepwarn", int, -1) {
        API_RETURN(DBGetDeprecateWarnings());
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

SILO_API FORTRAN
DBREGFOPTS_FC ( int *optlist_id )
{
    DBoptlist *optlist = NULL;

    API_BEGIN("dbregfopts", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        API_RETURN(DBRegisterFileOptionsSet(optlist));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

SILO_API FORTRAN
DBUNREGFOPTS_FC ( int *opts_set_id )
{
    API_BEGIN("dbunregfopts", int, -1) {
        API_RETURN(DBUnregisterFileOptionsSet(*opts_set_id));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

SILO_API FORTRAN
DBSETEMPTYOK_FC(int *ok)
{
    API_BEGIN("dbsetemptyok", int, -1) {
        API_RETURN(DBSetAllowEmptyObjects(*ok));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

SILO_API FORTRAN
DBGETEMPTYOK_FC()
{
    API_BEGIN("dbgetemptyok", int, -1) {
        API_RETURN(DBGetAllowEmptyObjects());
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

SILO_API FORTRAN
DBUNREGAFOPTS_FC ()
{
    API_BEGIN("dbunregafopts", int, -1) {
        DBUnregisterAllFileOptionsSets();
        API_RETURN(0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                DBMKMRGTREE_FC
 *
 * Purpose
 *     Create an mrgtree and return its identifier.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBMKMRGTREE_FC (int *source_mesh_type, int *type_info_bits,
    int *max_root_descendents, int *optlist_id, int *tree_id)
{
    DBmrgtree *tree = NULL;
    DBoptlist *optlist = NULL;

    API_BEGIN("dbmkmrgtree", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

        tree = DBMakeMrgtree(*source_mesh_type, *type_info_bits,
            *max_root_descendents, optlist);
        *tree_id = DBFortranAllocPointer(tree);

        API_RETURN(tree ? 0 : (-1));
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                              DBFREEMRGTREE_FC
 *
 * Purpose
 *     Free an mrg tree.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBFREEMRGTREE_FC (int *tree_id)
{
    DBmrgtree     *tree= NULL;

    API_BEGIN("dbfreemrgtree", int, -1) {
        tree = (DBmrgtree *) DBFortranAccessPointer(*tree_id);
        DBFreeMrgtree(tree);
        DBFortranRemovePointer(*tree_id);
        *tree_id = -1;
        API_RETURN(0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                DBADDREGION_FC
 *
 * Purpose
 *     Add a region to an mrg tree 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Neil Hodge, Tue Sep 13 11:17:41 PDT 2011
 *     Allow null maps_name.
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDREGION_FC (int *tree_id, FCD_DB region_name, int *lregion_name,
    int *type_info_bits, int *max_descendents, FCD_DB maps_name,
    int *lmaps_name, int *nsegs, int *seg_ids, int *seg_sizes,
    int *seg_types, int *optlist_id, int *status)
{
    DBmrgtree *tree = NULL;
    DBoptlist *optlist = NULL;
    char *region_nm = NULL, *maps_nm = NULL;

    API_BEGIN("dbaddregion", int, -1) {
        if (*lregion_name<=0)
            API_ERROR ("lregion_name", E_BADARGS) ;
        if (*lmaps_name<0)
            API_ERROR ("lmaps_name", E_BADARGS) ;

        tree = (DBmrgtree*) DBFortranAccessPointer(*tree_id);
        optlist = (DBoptlist*) DBFortranAccessPointer(*optlist_id);

      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
#ifdef CRAY
        if (!strcmp(_fcdtocp(region_name), DB_F77NULLSTRING))
            region_nm = NULL ;
        else
            region_nm = SW_strndup(_fcdtocp(region_name), *lregion_name);

        if (!strcmp(_fcdtocp(maps_name), DB_F77NULLSTRING))
            maps_nm = NULL ;
        else
            maps_nm = SW_strndup(_fcdtocp(maps_name), *lmaps_name);
#else
        if (!strcmp(region_name, DB_F77NULLSTRING))
            region_nm = NULL ;
        else
            region_nm = SW_strndup(region_name, *lregion_name);

        if (!strcmp(maps_name, DB_F77NULLSTRING))
            maps_nm = NULL ;
        else
            maps_nm = SW_strndup(maps_name, *lmaps_name);
#endif

        *status = DBAddRegion(tree, region_nm, *type_info_bits,
             *max_descendents, maps_nm, *nsegs, seg_ids, seg_sizes,
              seg_types, optlist);

        FREE(region_nm);
        FREE(maps_nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                               DBADDREGIONA_FC
 *
 * Purpose
 *     Add a region to an mrg tree 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Thu Feb  4 10:10:28 PST 2010
 *     Added patch by Olivier Cessenat to allow variable length strings.
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBADDREGIONA_FC (int *tree_id, int *nregn, FCD_DB regn_names, int *lregn_names,
    int *type_info_bits, FCD_DB maps_name, int *lmaps_name, int *nsegs,
    int *seg_ids, int *seg_sizes, int *seg_types, int *optlist_id, int *status)
{
    DBmrgtree *tree = NULL;
    DBoptlist *optlist = NULL;
    char **regn_nms = NULL, *maps_nm = NULL;
    char *realregn_names = NULL;
    int       i;
    long long indx;

    API_BEGIN("dbaddregiona", int, -1) {
        if (*lmaps_name<=0)
            API_ERROR ("lmaps_name", E_BADARGS) ;

        tree = (DBmrgtree*) DBFortranAccessPointer(*tree_id);
        optlist = (DBoptlist*) DBFortranAccessPointer(*optlist_id);

      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
#ifdef CRAY
        if (strcmp(_fcdtocp(regn_names), DB_F77NULLSTRING) == 0)
            realregn_names = NULL;
        else
            realregn_names = _fcdtocp(regn_names);

        if (!strcmp(_fcdtocp(maps_name), DB_F77NULLSTRING))
            maps_nm = NULL ;
        else
            maps_nm = SW_strndup(_fcdtocp(maps_name), *lmaps_name);
#else
        if (strcmp(regn_names, DB_F77NULLSTRING) == 0)
            realregn_names = NULL;
        else
            realregn_names = regn_names;

        if (!strcmp(maps_name, DB_F77NULLSTRING))
            maps_nm = NULL ;
        else
            maps_nm = SW_strndup(maps_name, *lmaps_name);
#endif

        if (*nregn <= 0)
            API_ERROR("nmesh", E_BADARGS);
        regn_nms = ALLOC_N(char *, *nregn);

        for (indx = 0, i = 0; i < *nregn; i++) {
            if (lregn_names[i] < 0)
                API_ERROR("lregn_names", E_BADARGS);
            regn_nms[i] = SW_strndup(&realregn_names[indx], lregn_names[i]);
            if (fortran2DStrLen > 0)
                indx += fortran2DStrLen;
            else
                indx += lregn_names[i] ;
        }

        *status = DBAddRegionArray(tree, *nregn, (char const * const *) regn_nms,
            *type_info_bits, maps_nm, *nsegs, seg_ids, seg_sizes,
             seg_types, optlist);

        for (i = 0; i < *nregn; i++)
            FREE(regn_nms[i]);
        FREE(maps_nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                   DBSETCWR_FC
 *
 * Purpose
 *     Set current working region of an mrg tree 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBSETCWR_FC (int *tree_id, FCD_DB path, int *lpath)
{
    DBmrgtree *tree = NULL;
    char *path_nm = NULL;
    int err;

    API_BEGIN("dbsetcwr", int, -1) {
        if (*lpath<=0)
            API_ERROR ("lpath", E_BADARGS) ;

        tree = (DBmrgtree*) DBFortranAccessPointer(*tree_id);

      /*------------------------------
       *  Duplicate all ascii strings.
       *-----------------------------*/
#ifdef CRAY
        if (!strcmp(_fcdtocp(path), DB_F77NULLSTRING))
            path_nm = NULL ;
        else
            path_nm = SW_strndup(_fcdtocp(path), *lpath);
#else
        if (!strcmp(path, DB_F77NULLSTRING))
            path_nm = NULL ;
        else
            path_nm = SW_strndup(path, *lpath);
#endif

        err = DBSetCwr(tree, path_nm);
        FREE(path_nm);
        API_RETURN(err);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                               DBPUTMRGTREE_FC
 *
 * Purpose
 *     Write mrg tree to a file 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTMRGTREE_FC (int *dbid, FCD_DB mrg_tree_name, int *lmrg_tree_name,
    FCD_DB mesh_name, int *lmesh_name, int *tree_id, int *optlist_id,
    int *status)
{
    DBfile     *dbfile ;
    DBoptlist  *optlist ;
    DBmrgtree  *tree;
    char     *mrg_tree_nm = NULL, *mesh_nm = NULL;

    API_BEGIN ("dbputmrgtree", int, -1) {
        if (*lmrg_tree_name<=0)
            API_ERROR ("lmrg_tree_name", E_BADARGS) ;
        if (*lmesh_name<=0)
            API_ERROR ("lmesh_name", E_BADARGS) ;

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        tree = (DBmrgtree *) DBFortranAccessPointer(*tree_id);
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

#ifdef CRAY
        if (!strcmp(_fcdtocp(mrg_tree_name), DB_F77NULLSTRING))
            mrg_tree_nm  = NULL;
        else
            mrg_tree_nm  = SW_strndup(_fcdtocp(mrg_tree_name), *lmrg_tree_name);

        if (!strcmp(_fcdtocp(mesh_name), DB_F77NULLSTRING))
            mesh_nm  = NULL;
        else
            mesh_nm  = SW_strndup(_fcdtocp(mesh_name), *lmesh_name);
#else
        if (!strcmp(mrg_tree_name, DB_F77NULLSTRING))
            mrg_tree_nm  = NULL;
        else
            mrg_tree_nm  = SW_strndup(mrg_tree_name, *lmrg_tree_name);

        if (!strcmp(mesh_name, DB_F77NULLSTRING))
            mesh_nm  = NULL;
        else
            mesh_nm  = SW_strndup(mesh_name, *lmesh_name);
#endif

        *status = DBPutMrgtree(dbfile, mrg_tree_nm, mesh_nm, tree, optlist);

        FREE(mrg_tree_nm);
        FREE(mesh_nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                               DBPUTGRPLMAP_FC
 *
 * Purpose
 *     Set current working region of an mrg tree 
 *
 * Notes
 *     This function was built to be called from Fortran.
 *     To populate the segment_data_ids and, if needed, segment_fracs_ids
 *     the fortran client will need to use DBMKPTR to register one of
 *     its pointers as an integer id and then use the resulting id at
 *     the appropriate entry in segment_data_ids. The fortran client
 *     should use DBRMPTR to remove the allocated pointer from the
 *     table after this call is completed.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications
 *     Thomas R. Treadway, Thu Oct 11 15:21:03 PDT 2007
 *     Using AC_FC_WRAPPERS for name-mangling
 *
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Neil Hodge, Tue Sep 13 11:18:30 PDT 2011
 *     Test derefernced segement_frac_ids for non-null
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTGRPLMAP_FC (int *dbid, FCD_DB map_name, int *lmap_name,
    int *num_segments, int *groupel_types, int *segment_lengths,
    int *segment_ids, int *segment_data_ids, int *segment_fracs_ids,
    int *fracs_data_type, int *optlist_id, int *status)
{
    DBfile     *dbfile ;
    DBoptlist  *optlist ;
    char       *map_nm = NULL;
    int       **segment_data = 0;
    void      **segment_fracs = 0;
    int         i;

    API_BEGIN ("dbputgrplmap", int, -1) {
        if (*map_name<=0)
            API_ERROR ("lmap_name", E_BADARGS) ;

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

#ifdef CRAY
        if (!strcmp(_fcdtocp(map_name), DB_F77NULLSTRING))
            map_nm  = NULL;
        else
            map_nm  = SW_strndup(_fcdtocp(map_name), *lmap_name);
#else
        if (!strcmp(map_name, DB_F77NULLSTRING))
            map_nm  = NULL;
        else
            map_nm  = SW_strndup(map_name, *lmap_name);
#endif

        /* convert array of segment data ids to their pointers */
        segment_data = (int**) malloc(*num_segments * sizeof(int*));
        for (i = 0; i < *num_segments; i++)
            segment_data[i] = (int *)DBFortranAccessPointer(segment_data_ids[i]);

        /* convert array of segment fracs ids to their pointers */
        if (*segment_fracs_ids)
        {
            segment_fracs = (void**) malloc(*num_segments * sizeof(void*));
            for (i = 0; i < *num_segments; i++)
                segment_fracs[i] = DBFortranAccessPointer(segment_fracs_ids[i]);
        }

        *status = DBPutGroupelmap(dbfile, map_nm, *num_segments,
            groupel_types, segment_lengths, segment_ids,
            (int const * const *) segment_data, (void const * const *) segment_fracs,
            *fracs_data_type, optlist);

        FREE(segment_data);
        FREE(segment_fracs);
        FREE(map_nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                  DBPUTCSGM_FC
 *
 * Purpose: Write csg mesh to the silo file
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications:
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTCSGM_FC (int *dbid, FCD_DB name, int *lname, int *ndims, int *nbounds,
    const int *typeflags, const int *bndids, const void *coeffs,
    int *lcoeffs, int *datatype, const double *extents, FCD_DB zlname,
    int *lzlname, int *optlist_id, int *status) 
{
    DBfile     *dbfile ;
    DBoptlist  *optlist ;
    char       *nm = NULL;
    char       *zl_nm = NULL;

    API_BEGIN ("dbputcsgm", int, -1) {
        if (*name<=0)
            API_ERROR ("name", E_BADARGS) ;
        if (*zlname<=0)
            API_ERROR ("zlname", E_BADARGS) ;

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

#ifdef CRAY
        if (!strcmp(_fcdtocp(name), DB_F77NULLSTRING))
            nm  = NULL;
        else
            nm  = SW_strndup(_fcdtocp(name), *lname);
        if (!strcmp(_fcdtocp(zlname), DB_F77NULLSTRING))
            zl_nm  = NULL;
        else
            zl_nm  = SW_strndup(_fcdtocp(zlname), *lzlname);
#else
        if (!strcmp(name, DB_F77NULLSTRING))
            nm  = NULL;
        else
            nm  = SW_strndup(name, *lname);
        if (!strcmp(zlname, DB_F77NULLSTRING))
            zl_nm  = NULL;
        else
            zl_nm  = SW_strndup(zlname, *lzlname);
#endif

        *status = DBPutCsgmesh(dbfile, nm, *ndims, *nbounds, typeflags,
            FPTR(bndids), coeffs, *lcoeffs, *datatype,
            extents, zl_nm, optlist);

        FREE(nm);
        FREE(zl_nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                 DBPUTCSGV_FC
 *
 * Purpose: Write csg variable to the silo file
 *
 * Notes
 *     This function was built to be called from Fortran.
 *     To populate the data_ids
 *     the fortran client will need to use DBMKPTR to register 
 *     its pointers as integer ids and then use the resulting id at
 *     the appropriate entry in data_ids. The fortran client
 *     should use DBRMPTR to remove the allocated pointer from the
 *     table after this call is completed.
 *
 * Returns
 *
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications:
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTCSGV_FC (int *dbid, FCD_DB name, int *lname, FCD_DB meshname,
    int *lmeshname, int *data_ids, int *nvals, int *datatype,
    int *centering, int *optlist_id, int *status)
{
    DBfile     *dbfile ;
    DBoptlist  *optlist ;
    char       *nm = NULL;
    char       *m_nm = NULL;
    void      **data = NULL;
    int         i;

    API_BEGIN ("dbputcsgv", int, -1) {
        if (*name<=0)
            API_ERROR ("name", E_BADARGS) ;
        if (*meshname<=0)
            API_ERROR ("meshname", E_BADARGS) ;

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

#ifdef CRAY
        if (!strcmp(_fcdtocp(name), DB_F77NULLSTRING))
            nm  = NULL;
        else
            nm  = SW_strndup(_fcdtocp(name), *lname);
        if (!strcmp(_fcdtocp(meshname), DB_F77NULLSTRING))
            m_nm  = NULL;
        else
            m_nm  = SW_strndup(_fcdtocp(meshname), *lmeshname);
#else
        if (!strcmp(name, DB_F77NULLSTRING))
            nm  = NULL;
        else
            nm  = SW_strndup(name, *lname);
        if (!strcmp(meshname, DB_F77NULLSTRING))
            m_nm  = NULL;
        else
            m_nm  = SW_strndup(meshname, *lmeshname);
#endif

        data = (void**) malloc(*nvals * sizeof(void*));
        for (i = 0; i < *nvals; i++)
            data[i] = DBFortranAccessPointer(data_ids[i]);

        *status = DBPutCsgvar(dbfile, nm, m_nm, 1, (char const * const *) &nm,
            (void const * const *) data, *nvals, *datatype, *centering, optlist);

        FREE(nm);
        FREE(m_nm);
        FREE(data);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*----------------------------------------------------------------------
 * Routine                                                 DBPUTCSGZL_FC
 *
 * Purpose: Write csg zonelist to the silo file
 *
 * Notes
 *     This function was built to be called from Fortran.
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Tue Oct  9 22:25:20 PDT 2007
 *
 * Modifications:
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *--------------------------------------------------------------------*/
SILO_API FORTRAN
DBPUTCSGZL_FC (int *dbid, FCD_DB name, int *lname, int *nregs,
    const int *typeflags, const int *leftids, const int *rightids,
    const void *xforms, int *lxforms, int *datatype, int *nzones,
    const int *zonelist, int *optlist_id, int *status) 
{
    DBfile     *dbfile ;
    DBoptlist  *optlist ;
    char       *nm = NULL;

    API_BEGIN ("dbputcsgzl", int, -1) {
        if (*name<=0)
            API_ERROR ("name", E_BADARGS) ;

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);

#ifdef CRAY
        if (!strcmp(_fcdtocp(name), DB_F77NULLSTRING))
            nm  = NULL;
        else
            nm  = SW_strndup(_fcdtocp(name), *lname);
#else
        if (!strcmp(name, DB_F77NULLSTRING))
            nm  = NULL;
        else
            nm  = SW_strndup(name, *lname);
#endif

        *status = DBPutCSGZonelist(dbfile, nm, *nregs, typeflags, leftids,
            rightids, *lxforms>0?xforms:0, *lxforms>0?*lxforms:0, *datatype,
            *nzones, zonelist, optlist); 

        FREE(nm);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

/*-------------------------------------------------------------------------
 * Routine                                                       DBPMRGV_FC
 *
 * Purpose
 *     Writes a mrg variable object into the open database.
 *
 * Notes
 *     This function was built to be called from Fortran.
 *     To populate the data_ids
 *     the fortran client will need to use DBMKPTR to register 
 *     its pointers as integer ids and then use the resulting id at
 *     the appropriate entry in data_ids. The fortran client
 *     should use DBRMPTR to remove the allocated pointer from the
 *     table after this call is completed.
 *
 * Returns
 *
 * Returns
 *     Returns 0 on success, -1 on failure.
 *
 * Programmer
 *     Mark C. Miller, Wed Oct 24 15:28:51 PDT 2007
 *
 * Modifications:
 *     Kathleen Bonnell, Wed Sep 2 15:31:26 PDT 2009
 *     Added SILO_API so symbols are correctly exported on windows.
 *
 *     Mark C. Miller, Thu Feb  4 10:10:28 PST 2010
 *     Added patch by Olivier Cessanat to allow variable length strings.
 *-------------------------------------------------------------------------*/
SILO_API FORTRAN
DBPMRGV_FC (int *dbid, FCD_DB name, int *lname, FCD_DB tname, int *ltname,
    int *ncomps, FCD_DB compnames, int *lcompnames,
    int *nregns, FCD_DB regnnames, int *lregnnames,
    int *datatype, int *data_ids, int *optlist_id, int *status)
{
    DBfile        *dbfile = NULL;
    char         **compnms = NULL, *nm = NULL;
    char         **regnnms = NULL, *tnm = NULL;
    char          *realcompnames = NULL;
    char          *realregnnames = NULL;
    void         **data = NULL;
    int            i;
    long long      indx;
    DBoptlist     *optlist = NULL;

    API_BEGIN("dbpmrgv", int, -1) {
        optlist = (DBoptlist *) DBFortranAccessPointer(*optlist_id);
        /*------------------------------
         *  Duplicate all ascii strings.
         *-----------------------------*/
        if (*lname <= 0)
            API_ERROR("lname", E_BADARGS);

#ifdef CRAY
        if (strcmp(_fcdtocp(name), DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(_fcdtocp(name), *lname);
        if (strcmp(_fcdtocp(tname), DB_F77NULLSTRING) == 0)
            tnm = NULL;
        else
            tnm = SW_strndup(_fcdtocp(tname), *ltname);
#else
        if (strcmp(name, DB_F77NULLSTRING) == 0)
            nm = NULL;
        else
            nm = SW_strndup(name, *lname);
        if (strcmp(tname, DB_F77NULLSTRING) == 0)
            tnm = NULL;
        else
            tnm = SW_strndup(tname, *ltname);
#endif

#ifdef CRAY
        if (strcmp(_fcdtocp(compnames), DB_F77NULLSTRING) == 0)
            realcompnames = NULL;
        else
            realcompnames = _fcdtocp(compnames);
        if (strcmp(_fcdtocp(regnnames), DB_F77NULLSTRING) == 0)
            realregnnames = NULL;
        else
            realregnnames = _fcdtocp(regnnames);
#else
        if (strcmp(compnames, DB_F77NULLSTRING) == 0)
            realcompnames = NULL;
        else
            realcompnames = compnames;
        if (strcmp(regnnames, DB_F77NULLSTRING) == 0)
            realregnnames = NULL;
        else
            realregnnames = regnnames;
#endif

        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);

        if (*ncomps <= 0)
            API_ERROR("ncomps", E_BADARGS);
        compnms = ALLOC_N(char *, *ncomps);
        for (indx = 0, i = 0; i < *ncomps; i++) {
            if (lcompnames[i] < 0)
                API_ERROR("lcompnames", E_BADARGS);
            compnms[i] = SW_strndup(&realcompnames[indx], lcompnames[i]);
            if (fortran2DStrLen > 0)
                indx += fortran2DStrLen;
            else
                indx += lcompnames[i] ;
        }

        if (*nregns <= 0)
            API_ERROR("nregns", E_BADARGS);
        regnnms = ALLOC_N(char *, *nregns);
        for (indx = 0, i = 0; i < *nregns; i++) {
            if (lregnnames[i] < 0)
                API_ERROR("lregnnames", E_BADARGS);
            regnnms[i] = SW_strndup(&realregnnames[indx], lregnnames[i]);
            if (fortran2DStrLen > 0)
                indx += fortran2DStrLen;
            else
                indx += lregnnames[i] ;
        }

        data = (void**) malloc(*nregns * sizeof(void*));
        for (i = 0; i < *nregns; i++)
            data[i] = DBFortranAccessPointer(data_ids[i]);

        /*----------------------------------------
         *  Invoke the C function to do the work.
         *---------------------------------------*/
        *status = DBPutMrgvar(dbfile, nm, tnm, *ncomps, (char const * const *) compnms,
            *nregns, (char const * const *) regnnms, *datatype, (void const * const *) data, optlist);

        for (i = 0; i < *ncomps; i++)
            FREE(compnms[i]);
        FREE(compnms);
        for (i = 0; i < *nregns; i++)
            FREE(regnnms[i]);
        FREE(regnnms);
        FREE(nm);
        FREE(tnm);
        FREE(data);

        API_RETURN((*status < 0) ? (-1) : 0);
    }
    API_END_NOPOP; /*BEWARE: If API_RETURN above is removed use API_END */
}

