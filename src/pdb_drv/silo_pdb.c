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
 * silo-pdb.c   -- The PDB functions.
 */

#define NEED_SCORE_MM
#include "silo_pdb_private.h"
#include <float.h>

/* The code between BEGIN/END monikers used to reside in a separate
 * file, 'pjjacket.c' but was moved here to reduce polution of the
 * global namespace with PJ symbols that are relevant only to the
 * PDB driver as well as to make it easier to re-map the code here
 * for th PDB proper driver */
/* BEGIN pjjacket.c */
/*
  Module Name                                                pjjacket.c

  Purpose

       This module contains the 'jacket' functions which map the
       special directory PDB functions (prefixed with 'PJ') using
       variable names into valid PDB functions.

  Programmer

       Jeffery Long, NSSD/B

  Contents

     Jacket Routines
     ---------------

       (int)  PJ_read()
       (int)  PJ_read_as()
       (int)  PJ_read_alt()
       (int)  PJ_read_as_alt()
*/

static char   *pj_fixname(PDBfile *, char const *);

/* Bounded name construction helper to avoid overflowing stack/static
 * buffers at call sites. Most call sites pass fixed-size arrays, so
 * sizeof(out) in the macro below resolves to the destination capacity. */
#ifdef PDB_WRITE
PRIVATE void db_mkname_sized(PDBfile *pdb, char const *name, char const *suffix,
    char *out, size_t outsz);
#define db_mkname(pdb, name, suffix, out) \
    db_mkname_sized((pdb), (name), (suffix), (out), sizeof(out))
#endif

/*-------------------------------------------------------------------------
 * Function:    PJ_read
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
INTERNAL int
PJ_read (PDBfile *file, char const *name, void *var) {

   char          *newname = pj_fixname(file, name);

   return (lite_PD_read(file, newname, var));
}

/*-------------------------------------------------------------------------
 * Function:    PJ_read_alt
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
INTERNAL int
PJ_read_alt (PDBfile *file, char *name, void *var, long *ind) {

   char          *newname = pj_fixname(file, name);

   return (lite_PD_read_alt(file, newname, var, ind));
}

/*-------------------------------------------------------------------------
 * Function:    PJ_read_as
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
INTERNAL int
PJ_read_as (PDBfile *file, char *name, char *type, void *var) {

   char          *newname = pj_fixname(file, name);

   return (lite_PD_read_as(file, newname, type, var));
}

/*-------------------------------------------------------------------------
 * Function:    PJ_read_as_alt
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
INTERNAL int
PJ_read_as_alt (PDBfile *file, char *name, char *type, void *var, long *ind) {

   char          *newname = pj_fixname(file, name);

   return (lite_PD_read_as_alt(file, newname, type, var, ind));
}


/*-----------------------------------------------------------*
 * Modifications:
 *
 *    Al Leibee, Mon Aug  9 10:30:54 PDT 1993
 *    Converted to new PDBlib interface for PD_inquire_entry.
 *    Note that the new args 'flag' and 'full path' are not
 *    used since pj_fixname is always invoked to get the full
 *    path name.
 *-----------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function:    PJ_inquire_entry
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
INTERNAL syment *
PJ_inquire_entry (PDBfile *file, char const *name) {

   char          *newname = pj_fixname(file, name);

   return (lite_PD_inquire_entry(file, newname, FALSE, NULL));
}

/*======================================================================
  ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  ======================================================================*/

/*
 * Modifications
 *
 * Al Leibee, Wed Jul  7 08:00:00 PDT 1993
 * SCALLOC_N to ALLOC_N to be not SCORE-dependent.
 */

/*-------------------------------------------------------------------------
 * Function:    pj_fixname
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *      Robb Matzke, Sun Dec 18 17:33:35 EST 1994
 *      _outname is static allocated instead of on the heap.  Otherwise
 *      it shows up as a memory leak since we always forget to free it
 *      at the end of the program.
 *
 *      Mark C. Miller, Mon Jan 30 19:43:02 PST 2006
 *      Increased size of _outname to something more appropriate. Added
 *      code to avoid over-writes
 *
 *-------------------------------------------------------------------------
 */
INTERNAL char *
pj_fixname (PDBfile *file, char const *inname) {

   static char    _outname[MAXLINE];

   /*
    *  If requested name begins with a '/', just use it.
    *  Otherwise, form a name with an absolute path.
    */
   if (inname[0] == '/') {
      strncpy(_outname, inname, sizeof(_outname));
      _outname[sizeof(_outname)-1] = '\0';
   }
   else {
      PJ_get_fullpath(lite_PD_pwd(file), inname, _outname);
   }

   return (_outname);
}


/*-------------------------------------------------------------------------
 * Function:    PJ_write
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
INTERNAL int
PJ_write (PDBfile *file, char *name, char *type, void *var) {

   char          *newname = pj_fixname(file, name);

   return (lite_PD_write(file, newname, type, var));
}
#endif /* PDB_WRITE */


/*-------------------------------------------------------------------------
 * Function:    PJ_write_alt
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
INTERNAL int
PJ_write_alt (PDBfile *file, char const *name, char const *type,
    void const *var, int nd, long const *ind) {

   char          *newname = pj_fixname(file, name);

   return (lite_PD_write_alt(file, newname, (char *) type,
              (void *) var, nd, (long *) ind));
}
#endif /* PDB_WRITE */


/*-------------------------------------------------------------------------
 * Function:    PJ_write_len
 *
 * Purpose:     Alternate form of PD_write_alt, which accepts length rather
 *              than pairs of min/max indices.
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
INTERNAL int
PJ_write_len (PDBfile *file, char const *name, char const *type, void const *var, int nd,
              long const *len) {

   long           ind[15], i;

   char          *newname = pj_fixname(file, name);

   for (i = 0; i < nd; i++) {
      ind[i * 3] = 0;
      ind[i * 3 + 1] = len[i] - 1;
      ind[i * 3 + 2] = 1;
   }

   return (lite_PD_write_alt(file, newname, (char*) type, (void*) var, nd, ind));
}
#endif /* PDB_WRITE */
/* END pjjacket.c */

/* The code between BEGIN/END monikers used to reside in a separate
 * file, 'pjobj.c' but was moved here to reduce polution of the
 * global namespace with PJ symbols that are relevant only to the
 * PDB driver as well as to make it easier to re-map the code here
 * for th PDB proper driver */
/* BEGIN pjobj.c */
/*-------------------------------------------------------------------------
 * Global private variables.
 *-------------------------------------------------------------------------
 */
static int     _pj_force_single = FALSE;

/*======================================================================
  ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  ======================================================================

  Module Name                                                 pjobject.c

  Purpose

        This module contains functions for easily reading PDB objects.

  Programmer

        Jeffery Long, NSSD/B

  Description

        The principal function here is PJ_GetObject. It operates on a
        PJcomplist data structure, which is simply a set of names, pointers,
        and data types which describe the components of a PDB object.
        PJ_GetObject interprets the object description and reads the object
        components into the pointers provided.

        There are three macros defined in pjgroup.h for simplifying the use
        of PJ_GetObject. They are PJ_INIT_OBJ, PJ_DEFINE_OBJ and
        PJ_DEFALL_OBJ. PJ_INIT_OBJ takes a single argument, the address of
        a PJ_Object to manipulate. PJ_DEFINE_OBJ & PJ_DEFALL_OBJ take four
        arguments: the component name, the address of where to store the
        component, the component data type, and a sentinel indicating
        whether or not the component is a scalar.

   Contents

        int   PJ_GetObject (file, objname, object, ret_type)
        void *PJ_GetComponent (file, objname, compname)

*/

/* Definition of global variables (bleah!) */

/* The variable use_PJgroup_cache is used in the PJ_GetObject function call.
 * That function will cache any PJgroup that it retrieves, if possible.
 * Sometimes, however, we've changed the state of the file (like changing
 * directories) and must ignore any cache that we already have.  The
 * "use_PJgroup_cache" variable is a boolean value that allows us to ignore
 * the cached value when we need to.  Note that, even if this variable is set
 * to 0 here, it will be reset to 1 after the first group is read.
 */
static int use_PJgroup_cache = 1;

/* To cache the PJgroup, we need to keep the group, and we need to keep
 * the "definition" of which object it is.  This information can be
 * captured by storing the name of the object, the name of the file it
 * came from, and the directory from which it came.
 */
static PJgroup *cached_group = NULL;
static char    *cached_obj_name = NULL;
static char    *cached_file_name = NULL;

PRIVATE int db_pdb_ParseVDBSpec (char const *mvdbspec, char **varname,
                                 char **filename);
PRIVATE int pj_GetVarDatatypeID (PDBfile *file, char *varname);
PRIVATE void reduce_path(char *path, char *npath);

/*----------------------------------------------------------------------
 *  Routine                                               PJ_ForceSingle
 *
 *  Purpose
 *
 *      Set the force single flag.
 *
 *  Programmer
 *
 *      Eric Brugger, February 15, 1995
 *
 *  Notes
 *
 *  Modified
 *
 *--------------------------------------------------------------------
 */
INTERNAL int
PJ_ForceSingle (int flag) {

   _pj_force_single = flag ? 1 : 0;

   return (0);
}

/*----------------------------------------------------------------------
 *  Routine                                            PJ_InqForceSingle
 *
 *  Purpose
 *
 *      Inquire the status of the force single flag.
 *
 *  Programmer
 *
 *      Eric Brugger, February 15, 1995
 *
 *  Notes
 *
 *  Modified
 *
 *--------------------------------------------------------------------
 */
INTERNAL int
PJ_InqForceSingle(void)
{
    return (_pj_force_single);
}

/*----------------------------------------------------------------------
 *  Routine:                                                PJ_NoCache
 *
 *  Purpose:
 *      Turn off caching of PJgroups until the next PJ_GetObject call.
 *
 *  Programmer:
 *      Sean Ahern, Mon Jul  1 08:23:41 PDT 1996
 *
 *  Notes:
 *     See comment at the top of this file about the use_PJgroup_cache
 *     global variable.
 *
 *  Modifications:
 *
 *     Jim Reus, 23 Apr 97
 *     Provide void in parameter list (to agree with header file).
 *
 *--------------------------------------------------------------------
 */
INTERNAL void
PJ_NoCache ( void )
{
    use_PJgroup_cache = 0;
}


/*----------------------------------------------------------------------
 *  Routine                                                 PJ_GetObject
 *
 *  Purpose
 *
 *      Read the contents of an object and store each entity into the
 *      address provided in the given component list structure.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 08:30:58 PST 1994
 *      Added error mechanism
 *
 *      Sean Ahern, Wed Jun 26 16:26:08 PDT 1996
 *      Added caching of the PJgroup.
 *
 *      Eric Brugger, Wed Dec 11 12:41:47 PST 1996
 *      I swapped the order of the loops over group component names
 *      and list component names.  This can be made slightly more
 *      efficient since the inner loop can be made to terminate
 *      earlier if a match is found.  This also closes a memory leak
 *      in the case where a component name is in the group twice and
 *      the memory for it should be allocated.  This has the other
 *      side affect of changing the behaviour such that if an entry
 *      shows up more than once it will take the first one, not the
 *      last one.
 *
 *      Sean Ahern, Mon Nov 23 17:03:59 PST 1998
 *      Removed the memory associated with the group when we're replacing the
 *      cached one.  Moved the memory management of the cache to a better
 *      place in the file.
 *
 *      Eric Brugger, Tue Dec 15 14:41:20 PST 1998
 *      I modified the routine to handle file names in the object name.
 *
 *      Sean Ahern, Thu Mar 30 11:01:56 PST 2000
 *      Fixed a memory leak.
 *
 *      Sean Ahern, Tue Jun 13 17:25:25 PDT 2000
 *      Made the function return the type of the object, if requested.
 *      Beefed up the error message if the object isn't found.
 *
 *      Mark C. Miller, Wed Sep 12 09:08:32 PDT 2012
 *      Replaced returned 'ret_type' argument with input expected_dbtype
 *      argument and cause it to fail if type that is read doesn't
 *      match the expected_dbtype.
 *--------------------------------------------------------------------*/
INTERNAL int
PJ_GetObject(PDBfile *file_in, char const *objname_in, PJcomplist *tobj, int expected_dbtype)
{
    int             i, j, error;
    char           *varname=NULL, *filename=NULL;
    char const     *objname=NULL;
    PDBfile        *file=NULL;
    char           *me = "PJ_GetObject";

    if (!file_in)
        return db_perror(NULL, E_NOFILE, me);
    if (!objname_in || !*objname_in)
        return db_perror("objname", E_BADARGS, me);

    /*
     * If the object name has a filename in it then open the file
     */
    if (db_pdb_ParseVDBSpec(objname_in, &varname, &filename) < 0)
    {
        FREE(varname);
        return db_perror("objname", E_BADARGS, me);
    }

    if (filename != NULL)
    {
        objname = varname;
        if ((file = lite_PD_open((char*)filename, "r")) == NULL)
        {
            FREE (varname);
            FREE (filename);
            return db_perror("objname", E_BADARGS, me);
        }
#ifdef USING_PDB_PROPER
        PD_set_track_pointers(file, FALSE);
#endif
    }
    else
    {
        objname = objname_in;
        file = file_in;
    }

    /* Read object description if we don't have it cached.
     *
     * There are three things that we have to check:
     *     use_PJgroup_cache is true
     *     cache_obj_name is the same as the passed-in name
     *     cache_file_name is the same as the passed-in file's name
     *
     * If any of these are false, we can't use the cached PJgroup and must
     * instead read a new one from the PDBfile.
     */
    if ((use_PJgroup_cache == 0) ||
        (cached_obj_name == NULL) || (strcmp(cached_obj_name, objname) != 0) ||
        (cached_file_name == NULL) || (strcmp(cached_file_name, file->name)))
    {
        PJ_ClearCache();
        error = !PJ_get_group(file, objname, &cached_group);
        if (error || cached_group == NULL)
        {
            char err_str[256];
            sprintf(err_str,"PJ_get_group: Probably no such object \"%s\".",objname);
            FREE(varname);
            FREE(filename);
            db_perror(err_str, E_CALLFAIL, me);
            return -1;
        }

        /* Check object type before we do any allocations */
        if (expected_dbtype > 0)
        {
            int matched = 1;
            if (expected_dbtype == DB_QUADMESH)
            {
                if ((strcmp(cached_group->type, DBGetObjtypeName(DB_QUAD_RECT)) != 0) &&
                    (strcmp(cached_group->type, DBGetObjtypeName(DB_QUAD_CURV)) != 0))
                    matched = 0;
            }  
            else if (strcmp(cached_group->type, DBGetObjtypeName(expected_dbtype)) != 0)
            {
                matched = 0;
            }
            if (!matched)
            {
                char error[256];
                sprintf(error,"Requested %s object \"%s\" is not a %s.",
                    cached_group->type, objname_in, DBGetObjtypeName(expected_dbtype));
                FREE(varname);
                FREE(filename);
                db_perror(error, E_NOTFOUND, me);
                return -1;
            }
        }

        /* We've gotten a new group, remember which one it is. */
        cached_obj_name = STRDUP(objname);
        cached_file_name = STRDUP(file->name);

        /* Now that we've cached a group, turn caching back on. */
        use_PJgroup_cache = 1;
    }

    /* Check object type */
    if (expected_dbtype > 0)
    {
        int matched = 1;
        if (expected_dbtype == DB_QUADMESH)
        {
            if ((strcmp(cached_group->type, DBGetObjtypeName(DB_QUAD_RECT)) != 0) &&
                (strcmp(cached_group->type, DBGetObjtypeName(DB_QUAD_CURV)) != 0))
                matched = 0;
        }  
        else if (strcmp(cached_group->type, DBGetObjtypeName(expected_dbtype)) != 0)
        {
            matched = 0;
        }
        if (!matched)
        {
            char error[256];
            FREE(varname);
            FREE(filename);
            sprintf(error,"Requested %s object \"%s\" is not a %s.",
                cached_group->type, objname_in, DBGetObjtypeName(expected_dbtype));
            db_perror(error, E_NOTFOUND, me);
            return -1;
        }
    }

    /* Walk through the object, putting the data into the appropriate memory
     * locations.  */
    for (i = 0; i < tobj->num; i++)
    {
        for (j = 0; j < cached_group->ncomponents; j++)
        {
            if (tobj->ptr[i] != NULL &&
                STR_EQUAL(cached_group->comp_names[j], tobj->name[i]))
            {

                /*
                 *  For alloced arrays, pass the address of the
                 *  pointer (i.e., ptr[i]). If not alloced, address
                 *  is already in the ptr[i] element.
                 */
                PJ_ReadVariable(file, cached_group->pdb_names[j],
                                tobj->type[i], (int)tobj->alloced[i],
                                (tobj->alloced[i]) ?
                                (char **)&tobj->ptr[i] :
                                (char **)tobj->ptr[i]);

            }
        }
    }

    /*
     * If the variable was from another file the file.
     */
    if (filename != NULL)
    {
        FREE (filename);
        lite_PD_close (file);
    }

    FREE (varname);

    return 0;
}

/*----------------------------------------------------------------------
 * Function:                                             PJ_ClearCache
 *
 * Purpose:     Frees up the storage associated with the cache.
 *
 * Programmer:  Sean Ahern, Mon Nov 23 17:19:17 PST 1998
 *
 * Modifications:
 *
 *    Lisa J. Roberts, Tue Jul 27 12:46:30 PDT 1999
 *    Added a return statement at the end of the function.
 *
 *    Brad Whitlock, Thu Jan 20 15:32:27 PST 2000
 *    Added the void to the argument list to preserve the prototype.
 *
 *--------------------------------------------------------------------*/
INTERNAL int
PJ_ClearCache(void)
{
    int error;
    static char *me = "PJ_ClearCache";

    /* Free up the old group. */
    if (cached_group)
    {
        error = !PJ_rel_group(cached_group);
        if (error)
        {
            db_perror("PJ_rel_group", E_CALLFAIL, me);
            return -1;
        }
        cached_group = NULL;
    }
    FREE(cached_obj_name);
    FREE(cached_file_name);

    return 0;
}


/*----------------------------------------------------------------------
 *  Routine                                              PJ_GetComponent
 *
 *  Purpose
 *
 *      Read the contents of an object and store each entity into the
 *      address provided in the given component list structure.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 08:39:56 PST 1994
 *      Added error mechanism
 *--------------------------------------------------------------------
 */
INTERNAL void *
PJ_GetComponent (PDBfile *file, char const *objname, char const *compname) {

   char          *result = NULL;
   PJcomplist     tmp_obj;
   char          *me = "PJ_GetComponent";
   PJcomplist    *_tcl;

   /* Read just the requested component of the given object */
   INIT_OBJ(&tmp_obj);
   DEFALL_OBJ(compname, &result, DB_NOTYPE);

   if (PJ_GetObject(file, objname, &tmp_obj, 0) < 0) {
      db_perror("PJ_GetObject", E_CALLFAIL, me);
      return NULL;
   }

   return ((void *)result);
}

/*----------------------------------------------------------------------
 *  Routine                                          PJ_GetComponentType
 *
 *  Purpose
 *
 *      Reads the contents of an object if it is not already cached and
 *      looks in the component list to determine the component type.
 *
 *  Programmer
 *
 *      Brad Whitlock, Thu Jan 20 15:26:51 PST 2000
 *
 *  Notes
 *
 *  Modified
 *
 *    Mark C. Miller, Thu Aug 24 22:41:23 PDT 2023
 *    Add use_PJcache_group==0 to conditions controlling whether a new
 *    call to PJ_GetObject is made.
 *--------------------------------------------------------------------
 */
INTERNAL int
PJ_GetComponentType (PDBfile *file, char const *objname, char const *compname)
{
   int  retval = DB_NOTYPE;
   char *me = "PJ_GetComponentType";
   PJcomplist    *_tcl;

   /* If there is no cached group, or we are interested in a
    * different object, get the one we want.  */
   if(use_PJgroup_cache == 0 || cached_group == NULL || cached_obj_name == NULL ||
      (!STR_EQUAL(cached_obj_name, objname)))
   {
       char       *result = NULL;
       PJcomplist tmp_obj;

       /* Read just the requested component of the given object */
       INIT_OBJ(&tmp_obj);
       DEFALL_OBJ(compname, &result, DB_NOTYPE);

       if (PJ_GetObject(file, objname, &tmp_obj, 0) < 0) {
          db_perror("PJ_GetObject", E_CALLFAIL, me);
          return DB_NOTYPE;
       }
       FREE(result);
   }

   /* If there is now cached group information (and there should be)
    * then look for the component in the group and determine its type.  */
   if(use_PJgroup_cache && cached_group)
   {
       int i, index, found = 0;

       /* Look through the cached group's component list to find
        * the appropriate index.  */
        for(i = 0; i < cached_group->ncomponents; i++)
        {
            if(strcmp(compname, cached_group->comp_names[i]) == 0)
            {
                found = 1;
                index = i;
                break;
            }
        }

        /* If the component name was in the list then determine
         * the type. If it does not have a prefix, then it must be
         * a variable since components can only be int,float,double,
         * string, or variable.  */
        if(found)
        {
            if(strncmp(cached_group->pdb_names[index], "'<i>", 4) == 0)
                retval = DB_INT;
            else if(strncmp(cached_group->pdb_names[index], "'<f>", 4) == 0)
                retval = DB_FLOAT;
            else if(strncmp(cached_group->pdb_names[index], "'<d>", 4) == 0)
                retval = DB_DOUBLE;
            else if(strncmp(cached_group->pdb_names[index], "'<s>", 4) == 0)
                retval = DB_CHAR;
            else
                retval = DB_VARIABLE;
        }
   }

   return retval;
}

/*----------------------------------------------------------------------
 *  Routine                                             PJ_ReadVariable
 *
 *  Purpose
 *
 *      Read an entity of arbitrary type from a PDB file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *      If the requested entity is either a scalar or a literal, the
 *      result will be stored directly into the space pointed to by
 *      var. Otherwise, space will be allocated and var will be
 *      assigned the new address.
 *
 *  Modifications
 *      Al Leibee, Thu Feb 17 13:04:47 PST 1994
 *      Removed Kludge re use of db_pdb_getvarinfo by using
 *      pdb_getvarinfo.
 *
 *      Robb Matzke, Fri Dec 2 13:51:28 PST 1994
 *      If this function allocates space for `var' we use the C
 *      libraray--not SCORE.
 *
 *      Eric Brugger, Mon Oct 23 12:17:03 PDT 1995
 *      I corrected a bug with the forcing of values.  If the value
 *      was forced, new storage was allocated for the space to put the
 *      results.  But this was a bug since if storage was passed to the
 *      routine, then the data had to be put in that space.
 *
 *      Eric Brugger, Thu Aug 20 11:46:54 PDT 1998
 *      I modified the routine to not perform forcing of values to
 *      single if the required type was DB_NOTYPE.  I think that the
 *      correct behavior is to only do forcing if the required data
 *      type is DB_FLOAT, but this routine gets used a lot and to
 *      check that it was always being called correctly is a lot of
 *      work.
 *
 *      Brad Whitlock, Thu Jan 20 17:33:20 PST 2000
 *      Added code for double conversion.
 *
 *      Eric Brugger, Fri Sep  1 14:53:59 PDT 2000
 *      Added code to compress out "../" from the variable name.
 *
 *      Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *      Add support for long long data type.
 *
 *      Mark C. Miller, Mon Dec  7 09:50:19 PST 2009
 *      Conditionally compile long long support only when its
 *      different from long.
 *
 *      Mark C. Miller, Mon Jan 11 16:02:16 PST 2010
 *      Made long long support UNconditionally compiled.
 *
 *      Mark C. MIller Mon Dec 10 09:54:05 PST 2012
 *      Fix possible ABR when tname is size zero.
 *--------------------------------------------------------------------*/
INTERNAL int
PJ_ReadVariable(PDBfile *file,
                char    *name_in,     /*Name of variable to read */
                int     req_datatype, /*Requested datatype for variable */
                int     alloced,      /*has space already been allocated? */
                char    **var)        /*Address of ptr to store data into */
{
   int            num, size, i, okay;
   int            act_datatype, forcing;
   int           *iptr;
   char           tname[256], *lit;
   float         *local_f;
   char          *local_c;
   float         *fptr;
   double        *dptr;
   char          *name=0;

   okay = TRUE;

   name = ALLOC_N(char, strlen(name_in)+1);
   reduce_path(name_in, name);

   /* Determine actual datatype of variable or literal */
   act_datatype = pj_GetVarDatatypeID(file, name);

   /* Set sentinel if will be forcing doubles to floats */
   if (req_datatype != DB_NOTYPE && req_datatype != act_datatype &&
       _pj_force_single)
      forcing = 1;
   else
      forcing = 0;

   /*--------------------------------------------------
    *  If name is enclosed in single quotes, take it as
    *  a literal value.
    *-------------------------------------------------*/

   if (name[0] == '\'') {
      int tnmlen;

      /*
       * No forcing of literals.  Since literals return an
       * an actual data type of -1, they will never get forced
       * since this is not a valid type.  This is a case of two
       * wrongs making a right since if the actual data type had
       * been a valid value, this routine would have tried to
       * convert it which would have been an error.
       */
      forcing = 0;

      /*--------------------------------------------------
       *  Component is a literal. Convert from ascii
       *  to type specified in string (default is int).
       *-------------------------------------------------*/

      strcpy(tname, &name[1]);
      tnmlen = strlen(tname);
      tnmlen = tnmlen>0?tnmlen:1;
      tname[tnmlen - 1] = '\0';

      if (name[1] == '<')
         lit = &tname[3];
      else
         lit = tname;

      if (STR_BEGINSWITH(tname, "<i>")) {

         if (alloced)
            iptr = (int *)*var;
         else
            iptr = ALLOC(int);

         *iptr = atoi(lit);
         *var = (char *)iptr;

      }
      else if (STR_BEGINSWITH(tname, "<f>")) {

         if (alloced)
            fptr = (float *)*var;
         else
            fptr = ALLOC(float);

         *fptr = (float)atof(lit);
         *var = (char *)fptr;

      }
      else if (STR_BEGINSWITH(tname, "<d>")) {

         if (alloced)
            dptr = (double *)*var;
         else
            dptr = ALLOC(double);

         *dptr = (double)atof(lit);
         *var = (char *)dptr;

      }
      else if (STR_BEGINSWITH(tname, "<s>")) {

         if (alloced)
            strcpy(*var, lit);
         else {
            *var = ALLOC_N(char, strlen(lit) + 1);

            strcpy(*var, lit);
         }

      }
      else {

         if (alloced)
            iptr = (int *)*var;
         else
            iptr = ALLOC(int);

         *iptr = atoi(lit);
         *var = (char *)iptr;
      }
   }
   else {
      /*--------------------------------------------------
       *  Component is not a literal. See if it was
       *  alloced. If so, read directly into 'var' space.
       *  Otherwise, let PDB assign address to our ptr.
       *  NOTE -- if num is returned '-1', name is a
       *  pointered array.
       *-------------------------------------------------*/

      (void)pdb_getvarinfo(file, name, tname, &num, &size, 0);

      /* If not already allocated, and is not a pointered var, allocate */
      if (!alloced && num > 0) {
         if (forcing)
            *var = ALLOC_N (char, num * sizeof(float));
         else
         {
            if (act_datatype == DB_CHAR)
            {
                *var = ALLOC_N (char, (num+1) * size);
                (*var)[num] = '\0';
            }
            else
            {
                *var = ALLOC_N (char, num * size);
            }
         }

         alloced = 1;
      }

      /*
       * If we are forcing the values to float, then read the
       * values into a buffer the size of the data as it exists
       * in the file.
       */
      if (forcing) {
         local_f = (float *) *var;

         *var = ALLOC_N (char, num * size);
      }

      if (alloced)
         /*------------------------------
          *  Space already allocated by
          *  caller, so use that.
          *-----------------------------*/
         okay = PJ_read(file, name, *var);
      else
         /*------------------------------
          *  Let PDBlib allocate new
          *  space. Just set pointer.
          *-----------------------------*/
         okay = PJ_read(file, name, var);

      /*
       *  PDB cannot query length of pointered vars in file, so
       *  num is set to -1. Now that it has been read, can query
       *  length.
       */
      if (num < 0) {
         num = lite_SC_arrlen(*var) / size;

         local_c = ALLOC_N(char, num * size);

         memcpy(local_c, *var, num * size);

         SCFREE(*var);
         *var = local_c;
      }
   }

   /*--------------------------------------------------
    *  Map values to float if
    *  force-single flag is TRUE.
    *--------------------------------------------------*/
   if (okay && forcing) {
      char          *c_conv = NULL;
      double        *d_conv = NULL;
      long          *l_conv = NULL;
      long long     *ll_conv = NULL;
      short         *s_conv = NULL;
      int           *i_conv = NULL;

      switch (act_datatype) {
      case DB_INT:
         i_conv = (int *)(*var);

         for (i = 0; i < num; i++)
            local_f[i] = (float)i_conv[i];
         break;
      case DB_SHORT:
         s_conv = (short *)(*var);

         for (i = 0; i < num; i++)
            local_f[i] = (float)s_conv[i];
         break;
      case DB_LONG:
         l_conv = (long *)(*var);

         for (i = 0; i < num; i++)
            local_f[i] = (float)l_conv[i];
         break;
      case DB_LONG_LONG:
         ll_conv = (long long *)(*var);

         for (i = 0; i < num; i++)
            local_f[i] = (float)ll_conv[i];
         break;
      case DB_DOUBLE:
         d_conv = (double *)(*var);

         for (i = 0; i < num; i++)
            local_f[i] = (float)d_conv[i];
         break;
      case DB_CHAR:
         c_conv = (char *)(*var);

         for (i = 0; i < num; i++)
            local_f[i] = (float)c_conv[i];
         break;
      default:
         break;
      }
   }

   if (forcing) {
      FREE(*var);
      *var = (char *)local_f;
   }

   FREE(name);

   return (okay);
}

/*----------------------------------------------------------------------
 *  Routine                                          pj_GetVarDatatypeID
 *
 *  Purpose
 *
 *      Return the datatype of the given variable.
 *
 *  Notes
 *
 *  Modifications
 *
 *      Al Leibee, Wed Aug 18 15:59:26 PDT 1993
 *      Convert to new PJ_inquire_entry interface.
 *
 *      Sean Ahern, Wed Apr 12 11:14:38 PDT 2000
 *      Removed the last two parameters to PJ_inquire_entry because they
 *      weren't being used.
 *--------------------------------------------------------------------*/
PRIVATE int
pj_GetVarDatatypeID (PDBfile *file, char *varname) {

   syment        *ep;

   if (varname[0] == '<')
      return DB_CHAR;

   ep = PJ_inquire_entry(file, varname);
   if (ep == NULL)
      return (OOPS);

   return (SW_GetDatatypeID(ep->type));
}

/*----------------------------------------------------------------------
 *  Routine                                         db_pdb_ParseVDBSpec
 *
 *  Purpose
 *
 *      Parse a variable database specification.
 *
 *  Notes
 *
 *  Modifications
 *
 *--------------------------------------------------------------------*/
PRIVATE int
db_pdb_ParseVDBSpec (char const *mvdbspec, char **varname, char **filename)
{
    int len_filename, len_varname;

    /*
     * Split spec into SILO name and SILO directory/variable name.
     */
    if (strchr (mvdbspec, ':') != NULL)
    {
        len_filename = strcspn (mvdbspec, ":");

        *filename = ALLOC_N (char, len_filename+1);

        strncpy (*filename, mvdbspec, len_filename);

        len_varname = strlen(mvdbspec) - (len_filename+1);
        if (len_varname <= 0)
        {
            FREE (*filename);
            return (OOPS);
        }

        /*
         * If a / does not exist at the beginning of the
         * path, insert it.
         */
        if (mvdbspec[len_filename+1] == '/')
        {
            *varname = ALLOC_N (char, len_varname+1);
            strncpy (*varname, &mvdbspec[len_filename+1], len_varname);
        }
        else
        {
            *varname = ALLOC_N (char, len_varname+2);
            (*varname) [0] = '/';
            strncpy (&((*varname)[1]), &mvdbspec[len_filename+1], len_varname);
            len_varname++;
        }
     }
     else
     {
        *filename = NULL;

        *varname = ALLOC_N (char, strlen(mvdbspec)+1);
        strcpy (*varname, mvdbspec);
    }

    return (OKAY);
}

/*----------------------------------------------------------------------
 *  Routine                                                 reduce_path
 *
 *  Purpose
 *
 *      Compress out the "../" from a path.
 *
 *  Notes
 *
 *  Programmer   Eric Brugger
 *  Date         September 1, 2000
 *
 *  Modifications
 *
 *--------------------------------------------------------------------*/
PRIVATE void
reduce_path(char *path, char *npath)
{
    int      i, j;
    int      lpath;

     npath [0] = '/' ;
     npath [1] = '\0' ;
     j = 0 ;
     lpath = strlen (path) ;
     for (i = 0; i < lpath; i++) {
          while (path [i] == '/' && path [i+1] == '/')
               i++ ;
          if (path [i] == '/' && path [i+1] == '.' && path [i+2] == '.' &&
                (path [i+3] == '/' || path [i+3] == '\0')) {
               if (j > 0)
                     j-- ;
               while (npath [j] != '/' && j > 0)
                    j-- ;
               i += 2 ;
               }
          else {
               npath [j++] = path [i] ;
          }
     }
     npath [j] =  '\0' ;

     /*
      * Check that the path is valid.
      */
     if (j == 0) {
          path [0] = '/';
          path [1] = '\0';
     }
}
/* END pjobj.c */

/* The code between BEGIN/END monikers used to reside in a separate
 * file, 'pjgroup.c' but was moved here to reduce polution of the
 * global namespace with PJ symbols that are relevant only to the
 * PDB driver as well as to make it easier to re-map the code here
 * for th PDB proper driver */
/* BEGIN pjgroup.c */
/*======================================================================
  ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  ======================================================================

  Module Name                                                  pjgroup.c

  Purpose

        This module provides a capability for reading and writing
        group descriptions in a PDB file.

  Programmer

        Jeffery Long, NSSD/B


  Description


  Contents

        User-Callable Functions (C)

                int      PJ_put_group (file, group, overwrite)
                int      PJ_get_group (file, name, PJgroup **group)
                PJgroup *PJ_make_group (name, type, comp_names, pdb_names, num)
                int      PJ_rel_group (group)

        User-Callable Functions (Fortran)


        Internal-Use-Only Functions




  ======================================================================
  ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  ======================================================================*/

/*
 **********************
 * Modification History
 **********************
 * $Id: pjgroup.c,v 1.4 1995/08/30 00:21:38 ahern Exp $
 * $Header: /SRC/pact/repository/src/silo/pdb/pjgroup.c,v 1.4 1995/08/30 00:21:38 ahern Exp $
 * $Log: pjgroup.c,v $
 * Revision 1.4  1995/08/30  00:21:38  ahern
 *  < ahern | D95_08_29_17_20_15 > Reformatted whitespace and tabs. You'll find that most changes are in comments.
 *
 * Revision 1.3  1995/08/28  21:53:40  ahern
 *  < ahern | D95_08_28_14_49_09 > Ran indent on silo.
 *
 * Revision 1.2  1995/04/21  16:16:34  brugger
 *  < brugger | D95_04_21_09_10_55 > I merged in Robbs silo filters enhancements into silo.
 *
 * Revision 1.1  1995/02/01  00:54:03  brugger
 *  < brugger | D95_01_31_16_47_08 > I incorporated robbs new device independent silo.
 *
 * Revision 1.2  1994/10/17  16:17:29  brugger
 *  < brugger | D94_10_17_09_09_09 > I restructured silo
 *
 * Revision 1.1.1.1  1994/04/13  22:55:17  sabrown
 * Reconfigure SILO
 *
 * Revision 1.2  1993/09/27  16:30:37  leibee
 *  < leibee | Mon Sep 27 09:25:46 PDT 1993 > Commit of v2_3 of SILO
 *
 *
 * Al Leibee, Wed Jul 21 09:01:32 PDT 1993
 * Ensure that PJ_make_group input strings are
 * SCORE-allocated.
 *
 * Revision 1.2  1993/07/13  16:57:51  leibee
 * Updated slide, pdbext, silo, swat modules: made allocate/free usage consistant re SCORE and system memory managers; reduced dependence on SCORE memory manager; removed check for SCORE-allocated array within FREE; added configman files.
 *
 *
 * Al Leibee, Wed Jul  7 08:00:00 PDT 1993
 * Made allocation/free usage consistant.
 */

/*=============================
 * Global Data for this Module
 *=============================*/

/*--------------------------------------------------------------------
 *  Routine                                              PJ_get_group
 *
 *  Purpose
 *
 *      Read a group description from the given PDB file.
 *
 *  Notes
 *
 *  Modified
 *
 * 	Robb Matzke, 5 Feb 1997
 *	This function returns 0 instead of dumping core if NAME is
 *	not a group.
 *
 *    Eric Brugger, Mon Dec  7 11:03:09 PST 1998
 *    Removed call to lite_PD_reset_ptr_list since it was removed.
 *
 *--------------------------------------------------------------------
 */
INTERNAL int
PJ_get_group(
   PDBfile       *file,        /* PDB file pointer */
   char const    *name,        /* Name of group desc to read */
   PJgroup      **group)       /* Variable to write into */
{
   syment	*ep;
   
   /*
    * Make sure the thing we're looking up is really a group.
    */
   ep = lite_PD_inquire_entry (file, (char *) name, TRUE, NULL);
   if (!ep || strcmp(PD_entry_type(ep), "Group *")) return 0;


   return ((int)PJ_read(file, name, group));

}

/*--------------------------------------------------------------------
 *  Routine                                              PJ_make_group
 *
 *  Purpose
 *
 *      Return a group description variable constructed from given
 *      elements.
 *
 *  Notes
 *
 *  Modifications
 *
 *      Al Leibee, Wed Jul 21 09:01:32 PDT 1993
 *      Ensure that inputted strings are SCORE-allocated.
 *
 *      Al Leibee, Wed Jul  7 08:00:00 PDT 1993
 *      Character strings known to have been allocated by SCORE.
 *
 *      Jeremy Meredith, Fri Nov 19 10:04:54 PST 1999
 *      Changed SC_strdup to safe_strdup.
 *
 *      Jeremy Meredith, Tue Nov 30 09:12:15 PST 1999
 *      I changed safe_strdup back to SC_strdup.  Not allocating the
 *      memory with SCORE's memory management routines caused severe
 *      problems because the memory block was not tagged correctly.
 *
 *--------------------------------------------------------------------
 */
INTERNAL PJgroup *
PJ_make_group (
   char          *name,        /* Name of this group */
   char          *type,        /* Type of this group */
   char         **comp_names,  /* Array of component names (num) */
   char         **pdb_names,   /* Array of internal (PDB) names (num) */
   int            num)         /* Number of components provided */
{
   PJgroup       *group;
   char          *sc_type, *sc_name;
   char         **sc_comp_nms;
   char         **sc_int_nms;
   int            i;

   if (num <= 0 || name == NULL || type == NULL ||
       comp_names == NULL || pdb_names == NULL)
      return (NULL);

   group = SCALLOC(PJgroup);

   /*----------------------------------------
    *  Ensure strings allocated by SCORE
    *  by making duplicates.
    *----------------------------------------*/
   sc_comp_nms = SCALLOC_N(char *, num);
   sc_int_nms = SCALLOC_N(char *, num);

   for (i = 0; i < num; i++) {
      sc_comp_nms[i] = SC_strdup(comp_names[i]);
      sc_int_nms[i] = SC_strdup(pdb_names[i]);
   }
   sc_type = SC_strdup(type);
   sc_name = SC_strdup(name);

   /*----------------------------------------
    *  Store new pointers in group variable.
    *----------------------------------------*/
   group->name = sc_name;
   group->type = sc_type;
   group->ncomponents = num;
   group->comp_names = sc_comp_nms;
   group->pdb_names = sc_int_nms;

   return (group);
}

/*--------------------------------------------------------------------
 *  Routine                                              PJ_rel_group
 *
 *  Purpose
 *
 *      Release the space associated with the given group.
 *
 *  Notes
 *
 *  Modifications
 *
 *      Al Leibee, Wed Jul  7 08:00:00 PDT 1993
 *      FREE to SCFREE to be consistant with allocation.
 *
 *      Sean Ahern, Mon Nov 23 17:02:49 PST 1998
 *      Made the return value meaningful.
 *--------------------------------------------------------------------*/
INTERNAL int
PJ_rel_group (PJgroup *group) {

   int            i;

   if (group == NULL || group->ncomponents <= 0)
      return (FALSE);

   for (i = 0; i < group->ncomponents; i++) {
      SCFREE(group->comp_names[i]);
      SCFREE(group->pdb_names[i]);
   }

   SCFREE(group->name);
   SCFREE(group->type);
   SCFREE(group->comp_names);
   SCFREE(group->pdb_names);
   SCFREE(group);

   return (TRUE);
}

/*--------------------------------------------------------------------
 *  Routine                                              PJ_print_group
 *
 *  Purpose
 *
 *      Print the given group.
 *
 *  Notes
 *
 *--------------------------------------------------------------------
 */
INTERNAL int
PJ_print_group (PJgroup *group, FILE *fp) {

   int            i;

   if (group == NULL || group->ncomponents <= 0)
      return (FALSE);

   if (fp == NULL)
      fp = stdout;

   fprintf(fp, "Group: %s is of type %s and has %d components.\n",
	   group->name, group->type, group->ncomponents);

   for (i = 0; i < group->ncomponents; i++) {
      fprintf(fp, "Component [%d] = %s  ==> %s\n", i,
	      group->comp_names[i], group->pdb_names[i]);
   }
   return 0;
}

/*--------------------------------------------------------------------
 *  Routine                                             PJ_put_group
 *
 *  Purpose
 *
 *      Write a group description into the given PDB file.
 *
 *  Return:	Success:	non-zero
 *
 * 		Failute:	zero
 *
 *  Notes
 *
 *
 *  Modifications
 *
 *     Al Leibee, Wed Jul  7 08:00:00 PDT 1993
 *     FREE to SCFREE to be consistant with allocation.
 *
 *     Robb Matzke, 7 Mar 1997
 *     Added the OVERWRITE argument.  If non-zero, then we can overwrite
 *     any existing group with the same name.
 *
 *    Eric Brugger, Mon Dec  7 11:03:09 PST 1998
 *    Removed call to lite_PD_reset_ptr_list since it was removed.
 *
 *--------------------------------------------------------------------
 */
#ifdef PDB_WRITE
INTERNAL int
PJ_put_group (
   PDBfile       *file,        /* PDB file pointer */
   PJgroup       *group,       /* Group variable to write */
   int		 overwrite)
{

   char         **varlist;
   char           name[MAXNAME];

   if (file == NULL || group == NULL)
      return (FALSE);

   /*----------------------------------------
    *  Define the group struct, if it hasn't
    *  been already.
    *----------------------------------------*/

   if (PD_inquire_type(file, "Group") == NULL) {

      if ((lite_PD_defstr(file, "Group",
			  "char    *name",
			  "char    *type",
			  "char    **comp_names",
			  "char    **pdb_names",
			  "integer ncomponents",
			  lite_LAST)) == NULL)
	 printf("PJ_put_group -- Error defining Group structure.\n");
   }

   /*----------------------------------------
    *  Build an absolute pathname.
    *---------------------------------------*/
   PJ_get_fullpath(lite_PD_pwd(file), group->name, name);

   /*----------------------------------------
    *  Make sure this group description hasn't
    *  already been written.
    *----------------------------------------*/
   if (!overwrite) {
#ifdef USING_PDB_PROPER
      varlist = SC_hasharr_dump(file->symtab, name, 0, 0);
#else
      varlist = lite_SC_hash_dump(file->symtab, name);
#endif

      if (varlist != NULL && varlist[0] != NULL)
	 return (FALSE);

      /*FREE(varlist); */
      SCFREE(varlist);
   }

   /*----------------------------------------
    *  Write the group description variable.
    *----------------------------------------*/
   return ((int)PJ_write(file, name, "Group *", &group));
}
#endif /* PDB_WRITE */

/* END pjgroup.c */

/* File-wide modifications:
 *      Sean Ahern, Fri Aug  1 13:23:38 PDT 1997
 *      Reformatted tabs to spaces.
 */

static char   *_valstr[10] =
{"value0", "value1", "value2",
 "value3", "value4", "value5",
 "value6", "value7", "value8",
 "value9"};

static char   *_mixvalstr[10] =
{"mixed_value0", "mixed_value1",
 "mixed_value2", "mixed_value3",
 "mixed_value4", "mixed_value5",
 "mixed_value6", "mixed_value7",
 "mixed_value8", "mixed_value9"};

static char   *_ptvalstr[10] =
{"0_data", "1_data", "2_data",
 "3_data", "4_data", "5_data",
 "6_data", "7_data", "8_data",
 "9_data"};

/* Symbolic constants used in calls to db_StringListToStringArray
   to indicate behavior. A '!' in front means to not perform the
   associated action. For PDB driver, we handle the slash swap
   on the 'names' member of multi-block objects only and we
   skip first semicolon ONLY for those string arrays that were
   added to PDB driver prior to db_StringListToStringArray
   coming into existence. This is to ensure diffs on files
   before and after this change don't vary due to leading
   semicolon. */
static int const skipFirstSemicolon = 1;

/*----------------------------------------------------------------------
 *  Routine                                       db_pdb_GetVarDatatype
 *
 *  Purpose
 *
 *      Return the datatype of the given variable.
 *
 *  Notes
 *
 *  Modifications
 *
 *      Al Leibee, Wed Aug 18 15:59:26 PDT 1993
 *      Convert to new PJ_inquire_interface.
 *
 *      Sean Ahern, Wed Apr 12 11:14:38 PDT 2000
 *      Removed the last two parameters to PJ_inquire_entry because they
 *      weren't being used.
 *
 *      Mark C. Miller, Thu May 13 19:02:58 PDT 2010
 *      Moved here from pjobj.c so it could be made static (private).
 *--------------------------------------------------------------------*/
PRIVATE int
db_pdb_GetVarDatatype (PDBfile *pdb, char const *varname) {

   syment        *ep;
   int            datatype;

   ep = PJ_inquire_entry(pdb, varname);
   if (!ep)
      return -1;

   datatype = SW_GetDatatypeID(ep->type);

   return datatype;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_InitCallbacks
 *
 * Purpose:     Initialize the callbacks in a DBfile structure.
 *
 * Programmer:  matzke@viper
 *              Tue Nov 29 13:26:58 PST 1994
 *
 * Modifications:
 *
 *    Eric Brugger, Thu Feb 16 08:45:00 PST 1995
 *    I added the DBReadVarSlice callback.
 *
 *    Jim Reus, 23 Apr 97
 *    I changed this to prototype form.
 *
 *    Jeremy Meredith, Sept 18 1998
 *    I added multi-material-species
 *
 *    Eric Brugger, Tue Mar 30 10:49:56 PST 1999
 *    I added the DBPutZonelist2 callback.
 *
 *    Robb Matzke, 2000-01-14
 *    I organized callbacks by category for easier comparison with other
 *    drivers.
 *
 *    Brad Whitlock, Thu Jan 20 11:59:11 PDT 2000
 *    I added the DBGetComponentType callback.
 *-------------------------------------------------------------------------*/
PRIVATE void
db_pdb_InitCallbacks ( DBfile *dbfile )
{
    /* Properties of the driver */
    dbfile->pub.pathok = FALSE;         /*driver doesn't handle paths well*/

    /* File operations */
    dbfile->pub.close = db_pdb_close;
    dbfile->pub.module = db_pdb_Filters;
    dbfile->pub.flush = db_pdb_flush;

    /* Directory operations */
    dbfile->pub.cd = db_pdb_SetDir;
    dbfile->pub.g_dir = db_pdb_GetDir;
    dbfile->pub.newtoc = db_pdb_NewToc;
#ifdef PDB_WRITE
    dbfile->pub.mkdir = db_pdb_MkDir;
#endif

    /* Variable inquiries */
    dbfile->pub.exist = db_pdb_InqVarExists;
    dbfile->pub.g_varlen = db_pdb_GetVarLength;
    dbfile->pub.g_varbl = db_pdb_GetVarByteLength;
    dbfile->pub.g_vartype = db_pdb_GetVarType;
    dbfile->pub.g_vardims = db_pdb_GetVarDims;

    /* Variable I/O operations */
    dbfile->pub.g_var = db_pdb_GetVar;
    dbfile->pub.r_var = db_pdb_ReadVar;
    dbfile->pub.r_varslice = db_pdb_ReadVarSlice;
    dbfile->pub.r_varvals = db_pdb_ReadVarVals;
#ifdef PDB_WRITE
    dbfile->pub.write = db_pdb_Write;
    dbfile->pub.writeslice = db_pdb_WriteSlice;
#endif

    /* Low-level object functions */
    dbfile->pub.g_obj = db_pdb_GetObject;
    dbfile->pub.inqvartype = db_pdb_InqVarType;
    dbfile->pub.i_meshtype = db_pdb_InqMeshtype;
    dbfile->pub.i_meshname = db_pdb_InqMeshname;
    dbfile->pub.g_comp = db_pdb_GetComponent;
    dbfile->pub.g_comptyp = db_pdb_GetComponentType;
    dbfile->pub.g_compnames = db_pdb_GetComponentNames;
#ifdef PDB_WRITE
    dbfile->pub.c_obj = db_pdb_WriteObject;      /*overloaded with w_obj*/
    dbfile->pub.w_obj = db_pdb_WriteObject;      /*overloaded with c_obj*/
    dbfile->pub.w_comp = db_pdb_WriteComponent;
#endif

    /* Curve functions */
    dbfile->pub.g_cu = db_pdb_GetCurve;
#ifdef PDB_WRITE
    dbfile->pub.p_cu = db_pdb_PutCurve;
#endif

    /* Defvar functions */
    dbfile->pub.g_defv = db_pdb_GetDefvars;
#ifdef PDB_WRITE
    dbfile->pub.p_defv = db_pdb_PutDefvars;
#endif

    /* Csgmesh functions */
    dbfile->pub.g_csgm = db_pdb_GetCsgmesh;
    dbfile->pub.g_csgv = db_pdb_GetCsgvar;
    dbfile->pub.g_csgzl = db_pdb_GetCSGZonelist;
#ifdef PDB_WRITE
    dbfile->pub.p_csgm = db_pdb_PutCsgmesh;
    dbfile->pub.p_csgv = db_pdb_PutCsgvar;
    dbfile->pub.p_csgzl = db_pdb_PutCSGZonelist;
#endif

    /* Quadmesh functions */
    dbfile->pub.g_qm = db_pdb_GetQuadmesh;
    dbfile->pub.g_qv = db_pdb_GetQuadvar;
#ifdef PDB_WRITE
    dbfile->pub.p_qm = db_pdb_PutQuadmesh;
    dbfile->pub.p_qv = db_pdb_PutQuadvar;
#endif

    /* Unstructured mesh functions */
    dbfile->pub.g_um = db_pdb_GetUcdmesh;
    dbfile->pub.g_uv = db_pdb_GetUcdvar;
    dbfile->pub.g_fl = db_pdb_GetFacelist;
    dbfile->pub.g_zl = db_pdb_GetZonelist;
    dbfile->pub.g_phzl = db_pdb_GetPHZonelist;
#ifdef PDB_WRITE
    dbfile->pub.p_um = db_pdb_PutUcdmesh;
    dbfile->pub.p_sm = db_pdb_PutUcdsubmesh;
    dbfile->pub.p_uv = db_pdb_PutUcdvar;
    dbfile->pub.p_fl = db_pdb_PutFacelist;
    dbfile->pub.p_zl = db_pdb_PutZonelist;
    dbfile->pub.p_zl2= db_pdb_PutZonelist2;
    dbfile->pub.p_phzl= db_pdb_PutPHZonelist;
#endif

    /* Material functions */
    dbfile->pub.g_ma = db_pdb_GetMaterial;
    dbfile->pub.g_ms = db_pdb_GetMatspecies;
#ifdef PDB_WRITE
    dbfile->pub.p_ma = db_pdb_PutMaterial;
    dbfile->pub.p_ms = db_pdb_PutMatspecies;
#endif

    /* Pointmesh functions */
    dbfile->pub.g_pm = db_pdb_GetPointmesh;
    dbfile->pub.g_pv = db_pdb_GetPointvar;
#ifdef PDB_WRITE
    dbfile->pub.p_pm = db_pdb_PutPointmesh;
    dbfile->pub.p_pv = db_pdb_PutPointvar;
#endif

    /* Multiblock functions */
    dbfile->pub.g_mm = db_pdb_GetMultimesh;
    dbfile->pub.g_mmadj = db_pdb_GetMultimeshadj;
    dbfile->pub.g_mv = db_pdb_GetMultivar;
    dbfile->pub.g_mt = db_pdb_GetMultimat;
    dbfile->pub.g_mms= db_pdb_GetMultimatspecies;
#ifdef PDB_WRITE
    dbfile->pub.p_mm = db_pdb_PutMultimesh;
    dbfile->pub.p_mmadj = db_pdb_PutMultimeshadj;
    dbfile->pub.p_mv = db_pdb_PutMultivar;
    dbfile->pub.p_mt = db_pdb_PutMultimat;
    dbfile->pub.p_mms= db_pdb_PutMultimatspecies;
#endif

    /* Compound arrays */
    dbfile->pub.g_ca = db_pdb_GetCompoundarray;
#ifdef PDB_WRITE
    dbfile->pub.p_ca = db_pdb_PutCompoundarray;
#endif

    /* MRG trees and Groupel maps */
    dbfile->pub.g_mrgt = db_pdb_GetMrgtree;
    dbfile->pub.g_grplm = db_pdb_GetGroupelmap;
    dbfile->pub.g_mrgv = db_pdb_GetMrgvar;
#ifdef PDB_WRITE
    dbfile->pub.p_mrgt = db_pdb_PutMrgtree;
    dbfile->pub.p_grplm = db_pdb_PutGroupelmap;
    dbfile->pub.p_mrgv = db_pdb_PutMrgvar;
#endif

    dbfile->pub.free_z = db_pdb_FreeCompressionResources;

    dbfile->pub.sort_obo = db_pdb_SortObjectsByOffset;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_close
 *
 * Purpose:     Closes a PDB file and free the memory associated with
 *              the file.
 *
 * Return:      Success:        NULL, usually assigned to the file
 *                              pointer being closed as in:
 *                                dbfile = db_pdb_close (dbfile) ;
 *
 *              Failure:        Never fails.
 *
 * Programmer:  matzke@viper
 *              Wed Nov  2 13:42:25 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Mon Feb 27 15:55:01 PST 1995
 *    I changed the return value to be an integer instead of a pointer
 *    to a DBfile.
 *
 *    Sean Ahern, Mon Jul  1 14:05:38 PDT 1996
 *    Turned off the PJgroup cache when we close files.
 *
 *    Sean Ahern, Tue Oct 20 17:21:18 PDT 1998
 *    Reformatted whitespace.
 *
 *    Sean Ahern, Mon Nov 23 17:29:17 PST 1998
 *    Added clearing of the object cache when the file is closed.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_close(DBfile *_dbfile)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;

   if (dbfile)
   {
      /*
       * Free the private parts of the file.
       */
      lite_PD_close(dbfile->pdb);
      dbfile->pdb = NULL;

      /* We've closed a file, so we can't cache PJgroups any more */
      PJ_NoCache();

      /*
       * Free the public parts of the file.
       */
      silo_db_close(_dbfile);

      /* Clear the current object cache. */
      PJ_ClearCache();
   }
   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_flush
 *
 * Purpose:     Flushes a PDB file to disk.
 *
 * Return:      Success:        0
 *              Fail:          -1
 *
 * Programmer: Mark C. Miller, Fri Aug 14 11:47:56 PDT 2015
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_flush(DBfile *_dbfile)
{
   int retval = -1;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;

   if (!dbfile)
      return retval;

   if (lite_PD_flush(dbfile->pdb) == TRUE)
      retval = 0;

   return retval;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_Open
 *
 * Purpose:     Opens a PDB file that already exists.
 *
 * Return:      Success:        ptr to the file structure
 *
 *              Failure:        NULL, db_errno set.
 *
 * Programmer:  matzke@viper
 *              Wed Nov  2 13:15:16 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Fri Jan 27 08:27:46 PST 1995
 *    I changed the call DBGetToc to db_pdb_GetToc.
 *
 *    Robb Matzke, Tue Mar 7 10:36:13 EST 1995
 *    I changed the call db_pdb_GetToc to DBNewToc.
 *
 *    Sean Ahern, Sun Oct  1 03:05:08 PDT 1995
 *    Made "me" static.
 *
 *    Sean Ahern, Sun Oct  1 06:09:25 PDT 1995
 *    Fixed a parameter type problem.
 *
 *    Sean Ahern, Mon Jan  8 17:37:47 PST 1996
 *    Added the mode parameter and accompanying logic.
 *
 *    Eric Brugger, Fri Jan 19 10:00:41 PST 1996
 *    I checked to see if it is a netcdf flavor of pdb file.
 *    If it is, then it returns NULL.
 *
 *    Sean Ahern, Fri Oct 16 17:46:57 PDT 1998
 *    Reformatted whitespace.
 *
 *    Mark C. Miller, Wed Feb 25 09:37:48 PST 2009
 *    Changed error code for failure to open to E_DRVRCANTOPEN
 *-------------------------------------------------------------------------*/
INTERNAL DBfile *
db_pdb_Open(char const *name, int mode, int opts_set_id)
{
    PDBfile        *pdb;
    DBfile_pdb     *dbfile;
    static char    *me = "db_pdb_Open";

    if (!SW_file_exists(name))
    {
        db_perror(name, E_NOFILE, me);
        return NULL;
    } else if (!SW_file_readable(name))
    {
        db_perror("not readable", E_NOFILE, me);
        return NULL;
    }
    if (mode == DB_READ)
    {
        if (NULL == (pdb = lite_PD_open((char*)name, "r")))
        {
            db_perror(NULL, E_DRVRCANTOPEN, me);
            return NULL;
        }
    } else if (mode == DB_APPEND)
    {
        if (NULL == (pdb = lite_PD_open((char*)name, "a")))
        {
            db_perror(NULL, E_DRVRCANTOPEN, me);
            return NULL;
        }
    } else
    {
        db_perror("mode", E_INTERNAL, me);
        return (NULL);
    }

    /*
     * If it is the netcdf flavor of pdb, then return NULL.
     */
#ifdef USING_PDB_PROPER
    PD_set_track_pointers(pdb, FALSE);
    if (NULL != SC_hasharr_lookup(pdb->symtab, "_whatami"))
#else
    if (NULL != lite_SC_lookup("_whatami", pdb->symtab))
#endif
    {
        lite_PD_close(pdb);
        return (NULL);
    }

    dbfile = ALLOC(DBfile_pdb);
    memset(dbfile, 0, sizeof(DBfile_pdb));
    dbfile->pub.name = STRDUP(name);
    dbfile->pub.type = DB_PDB;
#ifdef USING_PDB_PROPER
    dbfile->pub.type = DB_PDBP;
#endif
    dbfile->pdb = pdb;
    db_pdb_InitCallbacks((DBfile *) dbfile);
    return (DBfile *) dbfile;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_Create
 *
 * Purpose:     Creates a PDB file and begins the process of writing
 *              mesh and mesh-related data into that file.
 *
 *              NOTE: The `mode' parameter is unused.
 *                    But it's caught at the DBCreate call.
 *
 * Return:      Success:        pointer to a new file
 *
 *              Failure:        NULL, db_errno set
 *
 * Programmer:  matzke@viper
 *              Thu Nov  3 14:46:38 PST 1994
 *
 * Modifications:
 *    Eric Brugger, Fri Jan 27 08:27:46 PST 1995
 *    I changed the call DBGetToc to db_pdb_GetToc.
 *
 *    Robb Matzke, Tue Mar 7 10:37:07 EST 1995
 *    I changed the call db_pdb_GetToc to DBNewToc.
 *
 *    Sean Ahern, Sun Oct  1 03:05:32 PDT 1995
 *    Made "me" static.
 *
 *    Sean Ahern, Wed Oct  4 17:13:16 PDT 1995
 *    Fixed a parameter type problem.
 *
 *    Sean Ahern, Fri Oct 16 17:47:22 PDT 1998
 *    Reformatted whitespace.
 *
 *    Hank Childs, Thu Jan  6 13:41:38 PST 2000
 *    Put in cast to long of strlen to remove compiler warning.
 *
 *    Jeremy Meredith, Wed Oct 25 16:16:59 PDT 2000
 *    Added DB_INTEL so we had a little-endian target.
 *
 *    Thomas R. Treadway, Wed Feb 28 11:36:34 PST 2007
 *    Checked for compression option.
 *-------------------------------------------------------------------------*/
/* ARGSUSED */
INTERNAL DBfile *
db_pdb_Create (char const *name, int mode, int target, int opts_set_id, char const *finfo)
{
    DBfile_pdb     *dbfile;
    static char    *me = "db_pdb_create";

    if (SILO_Globals.enableChecksums)
    {
        db_perror(name, E_NOTIMP, "no checksums in PDB driver");
        return NULL;
    }
    if (SILO_Globals.compressionParams)
    {
        db_perror(name, E_NOTIMP, "no compression in PDB driver");
        return NULL;
    }


    /*
     * The target type.
     */
    switch (target)
    {
    case DB_LOCAL:
        break;
    case DB_SUN3:
        lite_PD_target(&lite_IEEEA_STD, &lite_M68000_ALIGNMENT);
        break;
    case DB_SUN4:
        lite_PD_target(&lite_IEEEA_STD, &lite_SPARC_ALIGNMENT);
        break;
    case DB_SGI:
        lite_PD_target(&lite_IEEEA_STD, &lite_MIPS_ALIGNMENT);
        break;
    case DB_RS6000:
        lite_PD_target(&lite_IEEEA_STD, &lite_RS6000_ALIGNMENT);
        break;
    case DB_CRAY:
        lite_PD_target(&lite_CRAY_STD, &lite_UNICOS_ALIGNMENT);
        break;
    case DB_INTEL:
        lite_PD_target(&lite_IEEEA_STD, &lite_INTELA_ALIGNMENT);
        break;
    default:
        db_perror("target", E_BADARGS, me);
        return NULL;
    }

    if (NULL == (dbfile = ALLOC(DBfile_pdb)))
    {
        db_perror(name, E_NOMEM, me);
        return NULL;
    }
    dbfile->pub.name = STRDUP(name);
    dbfile->pub.type = DB_PDB;
#ifdef USING_PDB_PROPER
    dbfile->pub.type = DB_PDBP;
#endif
    db_pdb_InitCallbacks((DBfile *) dbfile);

    if (NULL == (dbfile->pdb = lite_PD_open((char*)name, "w")))
    {
        FREE(dbfile->pub.name);
        FREE(dbfile);
        db_perror(name, E_NOFILE, me);
        return NULL;
    }
#ifdef USING_PDB_PROPER
    PD_set_track_pointers(dbfile->pdb, FALSE);
#endif
    DBNewToc((DBfile *) dbfile);
    if (finfo)
    {
        long    count = (long) strlen(finfo) + 1;

        PJ_write_len(dbfile->pdb, "_fileinfo", "char", finfo, 1, &count);
    }
#ifdef USING_PDB_PROPER
    {
        long count = 1; 
        int version_val = PDB_SYSTEM_VERSION;
        PJ_write_len(dbfile->pdb, "_pdblibinfo", "integer", &version_val, 1, &count);
    }
#endif
    return (DBfile *) dbfile;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_ForceSingle
 *
 * Purpose:     If status is non-zero, then force all double-precision
 *              floating-point values to single precision.
 *
 * Return:      Success:        0
 *
 *              Failure:        never fails
 *
 * Programmer:  matzke@viper
 *              Tue Jan 10 10:50:49 PST 1995
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
INTERNAL int
db_pdb_ForceSingle (int status)
{
   PJ_ForceSingle(status);
   return 0;
}

/*--------------------------------------------------------------------
 *  Routine                                                      PJ_ls
 *
 *  Purpose
 *
 *      Return a list of all variables of the specified type in the
 *      specified directory.
 *
 *  Notes
 *
 *      Providing a NULL string for 'type' will cause the type check
 *      to be avoided. Providing a NULL string for the directory path
 *      will cause only the root directory to be searched.
 *
 *  Modifications
 *
 *      Robb Matzke, Fri Feb 24 10:37:43 EST 1995
 *      This function just calls PD_ls().  But we must make sure that
 *      the return value is allocated with the C memory management.
 *
 *      Robb Matzke, Fri Dec 2 13:20:38 PST 1994
 *      Removed references to SCORE memory management.  The varlist
 *      vector and the strings in that vector are allocated with
 *      SCORE by lite_SC_hash_dump()--actually, the strings pointed to
 *      by this vector should not be freed because they are in the
 *      hash table.
 *
 *      Al Leibee, Mon Aug  9 10:30:54 PDT 1993
 *      Convert to new PD_inquire_entry interface.
 *
 *      Al Leibee, Thu Jul  8 08:05:09 PDT 1993
 *      FREE to SCFREE to be consistant with allocation.
 *--------------------------------------------------------------------*/
INTERNAL char **
PJ_ls (PDBfile       *file, /* PDB file pointer */
       char          *path, /* Path of dir to search (if NULL use '/') */
       char          *type, /* Variable type to search for (else NULL) */
       int           *num)  /* Returned number of matches */
{
   char         **score_out, **malloc_out;

   score_out = lite_PD_ls(file, path, type, num);
   if (*num == 0)
   {
      lite_SC_free(score_out);
      return 0;
   }
   malloc_out = ALLOC_N(char *, *num + 1);
   memcpy(malloc_out, score_out, *num * sizeof(char *));

   malloc_out[*num] = NULL;
   lite_SC_free(score_out);
   return malloc_out;
}

/*--------------------------------------------------------------------
 *  Routine                                            PJ_get_fullpath
 *
 *  Purpose
 *
 *      Generate a full pathname from the current working directory
 *      and the given pathname (abs/rel)
 *
 *  Notes
 *
 *      This routine is for internal use only. Not for user-level.
 *
 *      Robb Matzke, Fri Feb 24 10:40:39 EST 1995
 *      Removed the call to PJ_file_has_dirs() and we assume that all
 *      files support directories.
 *
 *      Sean Ahern, Fri Oct 16 17:47:44 PDT 1998
 *      Reformatted whitespace.
 *--------------------------------------------------------------------*/
INTERNAL int
PJ_get_fullpath(
   char          *cwd,         /* Current working directory */
   char const    *path,        /* Pathname (abs or rel) to traverse */
   char          *name)        /* Returned adjusted name */
{
    char *abspath;

    if (cwd == NULL || path == NULL || name == NULL)
        return (FALSE);

    abspath = db_absoluteOf_path(cwd, path);
    strcpy(name, abspath);
    free(abspath);

    return (TRUE);
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_getobjinfo
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *    Al Leibee, Fri Sep  3 10:54:47 PDT 1993
 *    Error check on PJ_read.
 *
 *    Eric Brugger, Thu Feb  9 12:54:37 PST 1995
 *    I modified the routine to read elements of the group structure
 *    as "abc->type" instead of "abc.type".
 *
 *    Sean Ahern, Sun Oct  1 03:05:56 PDT 1995
 *    Made "me" static.
 *
 *    Eric Brugger, Fri Dec  4 12:45:22 PST 1998
 *    Added code to free ctype to eliminate a memory leak.
 *-------------------------------------------------------------------------*/
PRIVATE int
db_pdb_getobjinfo (PDBfile       *pdb,
                   char const    *name, /* Name of object to inquire about */
                   char          *type, /* Returned object type of 'name'  */
                   int           *num)  /* Returned number of elements */
{
   char           newname[MAXNAME];
   char          *ctype;
   static char   *me = "db_pdb_getobjinfo";

   if (!pdb)
      return db_perror(NULL, E_NOFILE, me);
   if (!name || !*name)
      return db_perror("name", E_BADARGS, me);

   *num = *type = 0;

   sprintf(newname, "%s->type", name);
   if (PJ_read(pdb, newname, &ctype) == FALSE) {
      return db_perror("PJ_read", E_CALLFAIL, me);
   }
   strcpy(type, ctype);
   SCFREE(ctype);

   sprintf(newname, "%s->ncomponents", name);
   PJ_read(pdb, newname, num);

   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_getvarinfo
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *      Al Leibee, Wed Aug 18 15:59:26 PDT 1993
 *      Convert to new PJ_inquire_entry interface.
 *
 *      Al Leibee, Mon Aug 30 15:13:50 PDT 1993
 *      Use PD_inquire_host_type to get true data type size.
 *
 *      Sean Ahern, Sun Oct  1 03:06:20 PDT 1995
 *      Made "me" static.
 *
 *      Sean Ahern, Wed Apr 12 11:14:38 PDT 2000
 *      Removed the last two parameters to PJ_inquire_entry because they
 *      weren't being used.
 *-------------------------------------------------------------------------*/
PRIVATE int
db_pdb_getvarinfo (PDBfile       *pdb,
                   char const    *name,     /*Name of var to inquire about */
                   char          *type,     /*Returned datatype of 'name' */
                   int           *num,      /*Returned number of elements */
                   int           *size,     /*Returned element size */
                   int            verbose)  /*Sentinel: 1=print error msgs*/
{
   int            is_ptr;
   char           lastchar;
   char          *s;
   defstr        *dp;
   syment        *ep;
   static char   *me = "db_pdb_getvarinfo";

   *num = *size = 0;
   if (type)
      type[0] = '\0';

   /*
    *  Get symbol table entry for requested variable.
    */
   ep = PJ_inquire_entry(pdb, name);
   if (ep == NULL)
      return db_perror("PJ_inquire_entry", E_CALLFAIL, me);

   /* Assign values */
   if (type)
      strcpy(type, ep->type);

   lastchar = STR_LASTCHAR(ep->type);
   is_ptr = (lastchar == '*');

   if (is_ptr) {
      s = ALLOC_N(char, strlen(ep->type) + 1);

      strcpy(s, ep->type);
      s[strcspn(s, " *")] = '\0';

      /* Get data size of primitive (non-pointer) unit */
      dp = PD_inquire_host_type(pdb, s);
      *size = dp->size;
      *num = -1;              /* lite_SC_arrlen(values) / dp->size; */
      if (verbose)
         printf("Cannot query length of pointered variable.\n");

      FREE(s);

   }
   else {
      dp = PD_inquire_host_type(pdb, ep->type);
      if (dp == NULL) {
         if (verbose)
            printf("Don't know about data of type: %s\n", ep->type);
         return db_perror("PD_inquire_host_type", E_CALLFAIL, me);
      }

      /* Assign values */
      *size = dp->size;
      *num = ep->number;
   }

   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetDir
 *
 * Purpose:     Return the name of the current directory by copying the
 *              name to the output buffer supplied.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 08:52:38 PST 1994
 *
 * Modifications:
 *    Sean Ahern, Sun Oct  1 03:06:52 PDT 1995
 *    Made "me" static.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_GetDir (DBfile *_dbfile, char *result)
{
   char          *p;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   static char   *me = "db_pdb_GetDir";

   if (!result)
      return db_perror("result", E_BADARGS, me);
   p = lite_PD_pwd(dbfile->pdb);
   if (!p || !*p) {
      db_perror("PD_pwd", E_CALLFAIL, me);
      result[0] = '\0';
      return -1;
   }
   strcpy(result, p);
   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_NewToc
 *
 * Purpose:     Read the table of contents from the current directory
 *              and make it the current table of contents for the file
 *              pointer, freeing any previously existing table of contents.
 *
 * Notes
 *
 *              It is assumed that scalar values within the TOC have been
 *              initialized to zero.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1, db_errno set
 *
 * Programmer:  matzke@viper
 *              Thu Nov  3 14:34:31 PST 1994
 *
 * Modifications
 *    Al Leibee, Wed Jul  7 08:00:00 PDT 1993
 *    Changed FREE to SCFREE for consistant allocate/free usage.
 *
 *    Al Leibee, Wed Aug 18 15:59:26 PDT 1993
 *    Convert to new PJ_inquire_entry interface.
 *
 *    Al Leibee, Mon Aug  1 16:16:00 PDT 1994
 *    Added material species.
 *
 *    Robb Matzke, Tue Oct 25 09:46:48 PDT 1994
 *    added compound arrays.
 *
 *    Robb Matzke, Fri Dec 2 14:05:57 PST 1994
 *    Removed all references to SCORE memory management.  Local variable
 *    `name' is now allocated on the stack instead of by memory management.
 *
 *    Eric Brugger, Fri Jan 27 08:27:46 PST 1995
 *    I made it into an internal routine.
 *
 *    Eric Brugger, Thu Feb  9 12:54:37 PST 1995
 *    I modified the routine to read elements of the group structure
 *    as "abc->type" instead of "abc.type".
 *
 *    Eric Brugger, Thu Feb  9 15:07:29 PST 1995
 *    I modified the routine to handle the obj in the table of contents.
 *
 *    Robb Matzke, Tue Feb 21 16:20:58 EST 1995
 *    Removed references to the `id' fields of the DBtoc.
 *
 *    Robb Matzke, Tue Mar 7 10:38:59 EST 1995
 *    Changed the name from db_pdb_GetToc to db_pdb_NewToc.
 *
 *    Robb Matzke, Tue Mar 7 11:21:21 EST 1995
 *    Changed this to a CALLBACK.
 *
 *    Katherine Price, Thu May 25 14:44:42 PDT 1995
 *    Added multi-block materials.
 *
 *    Eric Brugger, Thu Jun 29 09:44:47 PDT 1995
 *    I modified the routine so that directories would show up in the
 *    table of contents.
 *
 *    Sean Ahern, Sun Oct  1 03:07:17 PDT 1995
 *    Made "me" static.
 *
 *    Eric Brugger, Mon Oct 23 08:00:16 PDT 1995
 *    I corrected a bug where the last character of a directory name
 *    was always eliminated.  This was done to get rid to the terminating
 *    '/' character in a directory name.  Well it turns out that not
 *    all silo files have a '/' at the end of a directory name.
 *
 *    Jeremy Meredith, Sept 18 1998
 *    Added multi-block species.
 *
 *    Jeremy Meredith, Nov 17 1998
 *    Added initialization of imultimatspecies.
 *
 *    Eric Brugger, Fri Dec  4 12:45:22 PST 1998
 *    Added code to free ctype to eliminate a memory leak.
 *
 *    Hank Childs, Thu Jan  6 13:33:21 PST 2000
 *    Removed print statement that appears to be a debugging relic.
 *
 *    Robb Matzke, Fri May 19 13:18:36 EDT 2000
 *    Avoid malloc(0)/calloc(0) since the behavior is undefined by Posix.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_NewToc (DBfile *_dbfile)
{
#define PJDIR  -10
#define PJVAR  -11

   int            i, lstr, num;
   int           *types=NULL;
   int            ivar, iqmesh, iqvar, iumesh, iuvar, icurve, idir, iarray,
      imat, imatspecies, imultimesh, imultivar, imultimat, imultimatspecies,
      ipmesh, iptvar, iobj, icsgmesh, icsgvar, idefvars, imultimeshadj,
      imrgtree, igroupelmap, imrgvar;

   DBtoc         *toc;
   char          *ctype;
   char         **list;        /* Names of everything in current directory */
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PDBfile       *file;        /* PDB file pointer  */
   char           name[128];
   static char   *me = "db_pdb_NewToc";

   db_FreeToc(_dbfile);
   dbfile->pub.toc = toc = db_AllocToc();

   file = dbfile->pdb;

   /*------------------------------------------------------------
    *  Get count of each entity (var, dir, curve, etc.)
    *  Start by getting list of everything in current dir,
    *  then count occurences of each type. Build an array of types
    *  which correspond to each item in list.
    *  (list is allocated by malloc().  The strings it points
    *  to should never be free since they are the strings stored in
    *  the SCORE hash table used by PDB.
    *------------------------------------------------------------*/
   list = PJ_ls(file, ".", NULL, &num);
   if (num) types = ALLOC_N(int, num);

   for (i = 0; i < num; i++) {

      syment        *ep;

      ep = lite_PD_inquire_entry(file, list[i], TRUE, NULL);

      if (ep == NULL) {
         types[i] = 999999;
         continue;
      }

      /*----------------------------------------
       *  Directory
       *----------------------------------------*/
      if (STR_BEGINSWITH(ep->type, "Directory")) {
         toc->ndir++;
         types[i] = PJDIR;

         /*----------------------------------------
          *  Group (object)
          *----------------------------------------*/
      }
      else if (STR_BEGINSWITH(ep->type, "GroupDataShadow")) {
          toc->nobj++;
          continue;
      }
      else if (STR_BEGINSWITH(ep->type, "Group")) {

         /*
          * Read the type field of each object and increment
          * the appropriate count.
          */
         sprintf(name, "%s->type", list[i]);
         if (!PJ_read(file, name, &ctype)) {
            sprintf(name, "%s.type", list[i]);
            if (!PJ_read(file, name, &ctype)) {
               FREE(types);
               FREE(list);
               return db_perror("PJ_read", E_CALLFAIL, me);
            }
         }

         types[i] = DBGetObjtypeTag(ctype);
         SCFREE(ctype);

         switch (types[i]) {
         case DB_MULTIMESH:
            toc->nmultimesh++;
            break;
         case DB_MULTIMESHADJ:
            toc->nmultimeshadj++;
            break;
         case DB_MULTIVAR:
            toc->nmultivar++;
            break;
         case DB_MULTIMAT:
            toc->nmultimat++;
            break;
         case DB_MULTIMATSPECIES:
            toc->nmultimatspecies++;
            break;
         case DB_CSGMESH:
            toc->ncsgmesh++;
            break;
         case DB_CSGVAR:
            toc->ncsgvar++;
            break;
         case DB_DEFVARS:
            toc->ndefvars++;
            break;
         case DB_QUADMESH:
         case DB_QUAD_RECT:
         case DB_QUAD_CURV:
            toc->nqmesh++;
            break;
         case DB_QUADVAR:
            toc->nqvar++;
            break;
         case DB_UCDMESH:
            toc->nucdmesh++;
            break;
         case DB_UCDVAR:
            toc->nucdvar++;
            break;
         case DB_POINTMESH:
            toc->nptmesh++;
            break;
         case DB_POINTVAR:
            toc->nptvar++;
            break;
         case DB_CURVE:
            toc->ncurve++;
            break;
         case DB_MATERIAL:
            toc->nmat++;
            break;
         case DB_MATSPECIES:
            toc->nmatspecies++;
            break;
         case DB_ARRAY:
            toc->narray++;
            break;
         case DB_MRGTREE:
            toc->nmrgtree++;
            break;
         case DB_GROUPELMAP:
            toc->ngroupelmap++;
            break;
         case DB_MRGVAR:
            toc->nmrgvar++;
            break;
         default:
            toc->nobj++;
            break;
         }

         /*----------------------------------------
          *  Other (variable?)
          *----------------------------------------*/
      }
      else {
         types[i] = PJVAR;
         toc->nvar++;
      }
   }

   /*----------------------------------------------------------------------
    *  Now all the counts have been made; allocate space.
    *---------------------------------------------------------------------*/
   if (toc->nvar > 0) {
      toc->var_names = ALLOC_N(char *, toc->nvar);
   }

   if (toc->nobj > 0) {
      toc->obj_names = ALLOC_N(char *, toc->nobj);
   }

   if (toc->ndir > 0) {
      toc->dir_names = ALLOC_N(char *, toc->ndir);
   }

   if (toc->ncurve > 0) {
      toc->curve_names = ALLOC_N(char *, toc->ncurve);
   }

   if (toc->ndefvars > 0) {
      toc->defvars_names = ALLOC_N(char *, toc->ndefvars);
   }

   if (toc->nmultimesh > 0) {
      toc->multimesh_names = ALLOC_N(char *, toc->nmultimesh);
   }

   if (toc->nmultimeshadj > 0) {
      toc->multimeshadj_names = ALLOC_N(char *, toc->nmultimeshadj);
   }

   if (toc->nmultivar > 0) {
      toc->multivar_names = ALLOC_N(char *, toc->nmultivar);
   }

   if (toc->nmultimat > 0) {
      toc->multimat_names = ALLOC_N(char *, toc->nmultimat);
   }

   if (toc->nmultimatspecies > 0) {
      toc->multimatspecies_names = ALLOC_N(char *, toc->nmultimatspecies);
   }

   if (toc->ncsgmesh > 0) {
      toc->csgmesh_names = ALLOC_N(char *, toc->ncsgmesh);
   }

   if (toc->ncsgvar > 0) {
      toc->csgvar_names = ALLOC_N(char *, toc->ncsgvar);
   }

   if (toc->nqmesh > 0) {
      toc->qmesh_names = ALLOC_N(char *, toc->nqmesh);
   }

   if (toc->nqvar > 0) {
      toc->qvar_names = ALLOC_N(char *, toc->nqvar);
   }

   if (toc->nucdmesh > 0) {
      toc->ucdmesh_names = ALLOC_N(char *, toc->nucdmesh);
   }

   if (toc->nucdvar > 0) {
      toc->ucdvar_names = ALLOC_N(char *, toc->nucdvar);
   }

   if (toc->nptmesh > 0) {
      toc->ptmesh_names = ALLOC_N(char *, toc->nptmesh);
   }

   if (toc->nptvar > 0) {
      toc->ptvar_names = ALLOC_N(char *, toc->nptvar);
   }

   if (toc->nmat > 0) {
      toc->mat_names = ALLOC_N(char *, toc->nmat);
   }

   if (toc->nmatspecies > 0) {
      toc->matspecies_names = ALLOC_N(char *, toc->nmatspecies);
   }

   if (toc->narray > 0) {
      toc->array_names = ALLOC_N(char *, toc->narray);
   }

   if (toc->nmrgtree > 0) {
      toc->mrgtree_names = ALLOC_N(char *, toc->nmrgtree);
   }

   if (toc->ngroupelmap > 0) {
      toc->groupelmap_names = ALLOC_N(char *, toc->ngroupelmap);
   }

   if (toc->nmrgvar > 0) {
      toc->mrgvar_names = ALLOC_N(char *, toc->nmrgvar);
   }
   /*----------------------------------------------------------------------
    *  Now loop over all the items in the directory and store the
    *  names and ID's
    *---------------------------------------------------------------------*/
   icurve = ivar = iqmesh = iqvar = iumesh = iuvar = idir = iarray = 0;
   imultimesh = imultivar = imultimat = imat = imatspecies = ipmesh = 0;
   iptvar = iobj = imultimatspecies = icsgmesh = icsgvar = idefvars = 0;
   imultimeshadj = imrgtree = igroupelmap = imrgvar = 0 ;

   for (i = 0; i < num; i++) {

      switch (types[i]) {

      case 999999:
         /*The call to PD_inquire_entry() failed above. */
         break;

      case PJDIR:
         /*
          * After copying the directory name, eliminate
          * the terminating '/' if one is present.
          */
         toc->dir_names[idir] = STRDUP(list[i]);
         lstr = strlen(list[i]);
         if (toc->dir_names[idir][lstr-1] == '/')
            toc->dir_names[idir][lstr-1] = '\0';
         idir++;
         break;

      case PJVAR:
         toc->var_names[ivar] = STRDUP(list[i]);
         ivar++;
         break;

      case DB_MULTIMESH:
         toc->multimesh_names[imultimesh] = STRDUP(list[i]);
         imultimesh++;
         break;

      case DB_MULTIMESHADJ:
         toc->multimeshadj_names[imultimeshadj] = STRDUP(list[i]);
         imultimeshadj++;
         break;

      case DB_MULTIVAR:
         toc->multivar_names[imultivar] = STRDUP(list[i]);
         imultivar++;
         break;

      case DB_MULTIMAT:
         toc->multimat_names[imultimat] = STRDUP(list[i]);
         imultimat++;
         break;

      case DB_MULTIMATSPECIES:
         toc->multimatspecies_names[imultimatspecies] = STRDUP(list[i]);
         imultimatspecies++;
         break;

      case DB_CSGMESH:
         toc->csgmesh_names[icsgmesh] = STRDUP(list[i]);
         icsgmesh++;
         break;

      case DB_CSGVAR:
         toc->csgvar_names[icsgvar] = STRDUP(list[i]);
         icsgvar++;
         break;

      case DB_DEFVARS:
         toc->defvars_names[idefvars] = STRDUP(list[i]);
         idefvars++;
         break;

      case DB_QUAD_RECT:
      case DB_QUAD_CURV:
      case DB_QUADMESH:
         toc->qmesh_names[iqmesh] = STRDUP(list[i]);
         iqmesh++;
         break;

      case DB_QUADVAR:
         toc->qvar_names[iqvar] = STRDUP(list[i]);
         iqvar++;
         break;

      case DB_UCDMESH:
         toc->ucdmesh_names[iumesh] = STRDUP(list[i]);
         iumesh++;
         break;

      case DB_UCDVAR:
         toc->ucdvar_names[iuvar] = STRDUP(list[i]);
         iuvar++;
         break;

      case DB_POINTMESH:
         toc->ptmesh_names[ipmesh] = STRDUP(list[i]);
         ipmesh++;
         break;

      case DB_POINTVAR:
         toc->ptvar_names[iptvar] = STRDUP(list[i]);
         iptvar++;
         break;

      case DB_CURVE:
         toc->curve_names[icurve] = STRDUP(list[i]);
         icurve++;
         break;

      case DB_MATERIAL:
         toc->mat_names[imat] = STRDUP(list[i]);
         imat++;
         break;

      case DB_MATSPECIES:
         toc->matspecies_names[imatspecies] = STRDUP(list[i]);
         imatspecies++;
         break;

      case DB_ARRAY:
         toc->array_names[iarray] = STRDUP(list[i]);
         iarray++;
         break;

      case DB_MRGTREE:
         toc->mrgtree_names[imrgtree] = STRDUP(list[i]);
         imrgtree++;
         break;

      case DB_GROUPELMAP:
         toc->groupelmap_names[igroupelmap] = STRDUP(list[i]);
         igroupelmap++;
         break;

      case DB_MRGVAR:
         toc->mrgvar_names[imrgvar] = STRDUP(list[i]);
         imrgvar++;
         break;

      default:
         toc->obj_names[iobj] = STRDUP(list[i]);
         iobj++;
         break;
      }
   }

   FREE(list);
   FREE(types);
   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_InqVarType
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
 *    Eric Brugger, Fri Dec  4 12:45:22 PST 1998
 *    Added code to free char_type to eliminate a memory leak.
 *
 *    Sean Ahern, Wed Dec  9 16:21:52 PST 1998
 *    Added some checks of the return value of DBGetObjtypeTag to make sure
 *    that we're returning a DBObjectType.
 *
 *    Sean Ahern, Thu Apr 29 16:24:16 PDT 1999
 *    Added two checks, one is if the variable exists at all.  The other is to
 *    add some logic to help PDB understand directories.
 *
 *    Lisa J. Roberts, Tue Nov 23 09:49:42 PST 1999
 *    Removed type, which was unused.
 *
 *    Hank Childs, Thu Jan  6 13:46:02 PST 2000
 *    Put in cast to remove compiler warning.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBObjectType
db_pdb_InqVarType(DBfile *_dbfile, char const *varname)
{
    DBfile_pdb     *dbfile = (DBfile_pdb *) _dbfile;
    PDBfile        *file = dbfile->pdb;
    char           *char_type = NULL;
    char            name[256];
    syment         *entry;
    int             typetag;

    /* First, check to see if it exists */
    entry = lite_PD_inquire_entry(file, (char *)varname, TRUE, NULL);
    if (entry == NULL)
    {
        /* This could be a directory.  Add a "/" to the end and try again. */
        char *newname = (char*)malloc(strlen(varname)+2);
        sprintf(newname,"%s/",varname);
        entry = lite_PD_inquire_entry(file, newname, TRUE, NULL);
        free(newname);

        /* If it's NULL now, we really have an invalid object.  Otherwise,
         * just drop out of the if, and continue. */
        if (entry == NULL)
            return(DB_INVALID_OBJECT);
    }

    /* Then, check to see if it's a directory. */
    if (STR_BEGINSWITH(entry->type, "Directory"))
        return(DB_DIR);
    else if (STR_BEGINSWITH(entry->type, "Group"))
    {
        /* It's not a directory.  Ask PDB what the type is. */
        sprintf(name, "%s.type", varname);
        if (!PJ_read(file, name, &char_type))
        {
            sprintf(name, "%s->type", varname);
            if (!PJ_read(file, name, &char_type))
                return(DB_INVALID_OBJECT);
        }
        typetag = DBGetObjtypeTag(char_type);
        SCFREE(char_type);
        if ((typetag == DB_QUAD_RECT) || (typetag == DB_QUAD_CURV))
            typetag = DB_QUADMESH;
        return( (DBObjectType) typetag);
    } else
    {
        return(DB_VARIABLE);
    }

    /* For stupid compilers */
    /* NOTREACHED */
    return(DB_INVALID_OBJECT);
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetObject
 *
 * Purpose:     Reads a group from a PDB file.
 *
 * Return:      Success:        Ptr to the new group, all memory allocated
 *                              by malloc().
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.mdn.com
 *              Jan 24, 1997
 *
 * Modifications:
 *
 *    Lisa J. Roberts, Tue Nov 23 09:39:49 PST 1999
 *    Changed strdup to safe_strdup.
 *
 *    Mark C. Miller, Wed Jan 21 13:52:10 PST 2009
 *    Removed #if 0 conditional compilation of PJ_rel_group call to fix
 *    a leak.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBobject *
db_pdb_GetObject (DBfile *_file, char const *name)
{
   PJgroup      *group=NULL;
   PDBfile      *file = ((DBfile_pdb *)_file)->pdb;
   DBobject     *obj=NULL;
   int          i;

   if (!PJ_get_group (file, name, &group)) return NULL;

   /*
    * We build our own DBobject here instead of calling DBMakeObject
    * because we have a character string type name instead of an
    * integer type constant.
    */
   obj = (DBobject *)calloc (1, sizeof (DBobject));
   obj->name = _db_safe_strdup (group->name);
   obj->type = _db_safe_strdup (group->type);
   obj->ncomponents = obj->maxcomponents = group->ncomponents;
   obj->comp_names = (char **)malloc (obj->maxcomponents * sizeof(char*));
   obj->pdb_names  = (char **)malloc (obj->maxcomponents * sizeof(char*));

   for (i=0; i<group->ncomponents; i++) {
      obj->comp_names[i] = _db_safe_strdup (group->comp_names[i]);
      obj->pdb_names[i] = _db_safe_strdup (group->pdb_names[i]);
   }

   PJ_rel_group (group);

   return obj;
}

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_GetMaterial
 *
 *  Purpose
 *
 *      Read a material-data object from a SILO file and return the
 *      SILO structure for this type.
 *
 *  Modified
 *
 *      Robb Matzke, Mon Nov 14 14:30:12 EST 1994
 *      Device independence rewrite.
 *
 *      Sean Ahern, Sun Oct  1 03:07:59 PDT 1995
 *      Made "me" static.
 *
 *      Robb Matzke, 26 Aug 1997
 *      The datatype is set from DB_DOUBLE to DB_FLOAT only if the
 *      force-single flag is set.
 *
 *      Sean Ahern, Wed Jun 14 10:53:56 PDT 2000
 *      Added a check to make sure the object we're reading is the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Sean Ahern, Tue Feb  5 13:35:49 PST 2002
 *      Added support for material names.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBmaterial *
db_pdb_GetMaterial(DBfile *_dbfile,     /*DB file pointer */
                   char   const *name)        /*Name of material object to return*/
{
    DBmaterial *mm = NULL;
    DBfile_pdb *dbfile = (DBfile_pdb *) _dbfile;
    static char *me = "db_pdb_GetMaterial";
    PJcomplist tmp_obj;
    char *tmpnames = NULL;
    char *tmpcolors = NULL;
    DBmaterial tmpmm;
    PJcomplist *_tcl;

    /* Comp. Name        Comp. Address     Data Type     */
    memset(&tmpmm, 0, sizeof(DBmaterial));
    INIT_OBJ(&tmp_obj);

    DEFINE_OBJ("ndims",       &tmpmm.ndims,       DB_INT);
    DEFINE_OBJ("dims",         tmpmm.dims,        DB_INT);
    DEFINE_OBJ("major_order", &tmpmm.major_order, DB_INT);
    DEFINE_OBJ("origin",      &tmpmm.origin,      DB_INT);
    DEFALL_OBJ("meshid",      &tmpmm.meshname,    DB_CHAR);
    DEFINE_OBJ("allowmat0",   &tmpmm.allowmat0,   DB_INT);
    DEFINE_OBJ("guihide",     &tmpmm.guihide,     DB_INT);

    DEFINE_OBJ("nmat",        &tmpmm.nmat,        DB_INT);
    DEFINE_OBJ("mixlen",      &tmpmm.mixlen,      DB_INT);
    DEFINE_OBJ("datatype",    &tmpmm.datatype,    DB_INT);

    if (DBGetDataReadMask2File(_dbfile) & DBMatMatnos)
        DEFALL_OBJ("matnos",      &tmpmm.matnos,      DB_INT);
    if (DBGetDataReadMask2File(_dbfile) & DBMatMatnames)
        DEFALL_OBJ("matnames",    &tmpnames,        DB_CHAR);
    if (DBGetDataReadMask2File(_dbfile) & DBMatMatcolors)
        DEFALL_OBJ("matcolors",    &tmpcolors,        DB_CHAR);
    if (DBGetDataReadMask2File(_dbfile) & DBMatMatlist)
        DEFALL_OBJ("matlist",     &tmpmm.matlist,     DB_INT);
    if (DBGetDataReadMask2File(_dbfile) & DBMatMixList)
    {
        DEFALL_OBJ("mix_mat",     &tmpmm.mix_mat,     DB_INT);
        DEFALL_OBJ("mix_next",    &tmpmm.mix_next,    DB_INT);
        DEFALL_OBJ("mix_zone",    &tmpmm.mix_zone,    DB_INT);
        DEFALL_OBJ("mix_vf",      &tmpmm.mix_vf,      DB_FLOAT);
    }

    if (PJ_GetObject(dbfile->pdb, name, &tmp_obj, DB_MATERIAL) < 0)
        return NULL;

    if (NULL == (mm = DBAllocMaterial()))
    {
        db_perror("DBAllocMaterial", E_CALLFAIL, me);
        return NULL;
    }
    *mm = tmpmm;

    _DBQQCalcStride(mm->stride, mm->dims, mm->ndims, mm->major_order);

    /* If we have material names, restore it to an array of names.  In the
     * file, it's stored as one string, with individual names separated by
     * semicolons. */
    if ((tmpnames != NULL) && (mm->nmat > 0))
    {
        char *s, *name;
        int i;
        char error[256];

        mm->matnames = ALLOC_N(char *, mm->nmat);

        s = &tmpnames[0];
        name = (char *)strtok(s, ";");

        for (i = 0; i < mm->nmat; i++)
        {
            mm->matnames[i] = STRDUP(name);

            if (i + 1 < mm->nmat)
            {
                name = (char *)strtok(NULL, ";");
                if (name == NULL)
                {
                    sprintf(error, "(%s) Not enough material names found\n", me);
                    db_perror(error, E_INTERNAL, me);
                }
            }
        }
        FREE(tmpnames);
    }
    if ((tmpcolors != NULL) && (mm->nmat > 0))
    {
        mm->matcolors = DBStringListToStringArray(tmpcolors, &(mm->nmat), !skipFirstSemicolon);
        FREE(tmpcolors);
    }

    mm->id = 0;
    mm->name = STRDUP(name);
    if (0 >= mm->mixlen)
    {
        mm->datatype = DB_NOTYPE;
    }
    if (DB_DOUBLE == mm->datatype && PJ_InqForceSingle())
    {
        mm->datatype = DB_FLOAT;
    }

    return (mm);
}

/*----------------------------------------------------------------------
 *  Routine                                       db_pdb_GetMatspecies
 *
 *  Purpose
 *
 *      Read a matspecies-data object from a SILO file and return the
 *      SILO structure for this type.
 *
 *  Modifications
 *
 *      Robb Matzke, Tue Nov 29 13:29:57 PST 1994
 *      Changed for device independence.
 *
 *      Al Leibee, Tue Jul 26 08:44:01 PDT 1994
 *      Replaced composition by species.
 *
 *      Sean Ahern, Sun Oct  1 03:08:19 PDT 1995
 *      Made "me" static.
 *
 *      Jeremy Meredith, Wed Jul  7 12:15:31 PDT 1999
 *      I removed the origin value from the species object.
 *
 *      Sean Ahern, Wed Jun 14 17:19:12 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *      Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *      Added names and colors for species.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBmatspecies *
db_pdb_GetMatspecies (DBfile *_dbfile,   /*DB file pointer */
                      char   const *objname)   /*Name of matspecies obj to return */
{
   DBmatspecies  *mm;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   static char   *me = "db_pdb_GetMatspecies";
   char *tmpnames = NULL;
   char *tmpcolors = NULL;
   char           tmpstr[256];
   PJcomplist     tmp_obj;
   DBmatspecies   tmpmm;
   int            i, nstrs = 0;
   PJcomplist    *_tcl;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmpmm, 0, sizeof(DBmatspecies));
   INIT_OBJ(&tmp_obj);

   DEFALL_OBJ("matname", &tmpmm.matname, DB_CHAR);
   DEFINE_OBJ("ndims", &tmpmm.ndims, DB_INT);
   DEFINE_OBJ("dims", tmpmm.dims, DB_INT);
   DEFINE_OBJ("major_order", &tmpmm.major_order, DB_INT);
   DEFINE_OBJ("datatype", &tmpmm.datatype, DB_INT);
   DEFINE_OBJ("nmat", &tmpmm.nmat, DB_INT);
   DEFALL_OBJ("nmatspec", &tmpmm.nmatspec, DB_INT);
   DEFINE_OBJ("nspecies_mf", &tmpmm.nspecies_mf, DB_INT);
   DEFALL_OBJ("speclist", &tmpmm.speclist, DB_INT);
   DEFINE_OBJ("mixlen", &tmpmm.mixlen, DB_INT);
   DEFALL_OBJ("mix_speclist", &tmpmm.mix_speclist, DB_INT);
   DEFINE_OBJ("guihide", &tmpmm.guihide, DB_INT);
   if (DBGetDataReadMask2File(_dbfile) & DBMatMatnames)
       DEFALL_OBJ("species_names",    &tmpnames,        DB_CHAR);
   if (DBGetDataReadMask2File(_dbfile) & DBMatMatcolors)
       DEFALL_OBJ("speccolors",    &tmpcolors,        DB_CHAR);

   if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_MATSPECIES) < 0)
      return NULL;

   if (NULL == (mm = DBAllocMatspecies())) {
      db_perror("DBAllocMatspecies", E_CALLFAIL, me);
      return NULL;
   }
   *mm = tmpmm;

   /* Now read in species_mf per its datatype. */
   INIT_OBJ(&tmp_obj);

   if (mm->datatype == 0) {
      strcpy(tmpstr, objname);
      strcat(tmpstr, "_data");
      mm->datatype = db_pdb_GetVarDatatype(dbfile->pdb, tmpstr);

      if (mm->datatype < 0)
         mm->datatype = DB_FLOAT;
   }

   if (mm->datatype == DB_DOUBLE && PJ_InqForceSingle())
      mm->datatype = DB_FLOAT;

   DEFALL_OBJ("species_mf", &mm->species_mf, mm->datatype);
   PJ_GetObject(dbfile->pdb, objname, &tmp_obj, 0);

   _DBQQCalcStride(mm->stride, mm->dims, mm->ndims, mm->major_order);

   mm->id = 0;
   mm->name = STRDUP(objname);

   for (i=0; i < mm->nmat; i++)
       nstrs += mm->nmatspec[i];
   if (tmpnames != NULL)
   {
       if (nstrs > 0)
          mm->specnames = DBStringListToStringArray(tmpnames, &nstrs, !skipFirstSemicolon);
       FREE(tmpnames);
   }
   if (tmpcolors != NULL)
   {
       if (nstrs > 0)
          mm->speccolors = DBStringListToStringArray(tmpcolors, &nstrs, !skipFirstSemicolon);
       FREE(tmpcolors);
   }

   return (mm);
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetCompoundarray
 *
 * Purpose:     Read a compound array object from a PDB data file.
 *
 * Return:      Success:        pointer to the new object
 *
 *              Failure:        NULL, db_errno set
 *
 * Programmer:  matzke@viper
 *              Wed Nov  2 14:43:05 PST 1994
 *
 * Modifications:
 *     Sean Ahern, Sun Oct  1 03:09:15 PDT 1995
 *     Made "me" static.
 *
 *     Sean Ahern, Wed Jun 14 17:19:34 PDT 2000
 *     Added a check to make sure the object is the right type.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBcompoundarray *
db_pdb_GetCompoundarray (DBfile *_dbfile, char const *array_name)
{
   int            i;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   DBcompoundarray *ca = NULL;
   char          *s, delim[2], *name_vector = NULL;
   static char   *me = "db_pdb_GetCompoundarray";
   PJcomplist     tmp_obj;
   DBcompoundarray tmpca;
   PJcomplist    *_tcl;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmpca, 0, sizeof(DBcompoundarray));
   INIT_OBJ(&tmp_obj);
   DEFINE_OBJ("nelems", &tmpca.nelems, DB_INT);
   DEFINE_OBJ("nvalues", &tmpca.nvalues, DB_INT);
   DEFINE_OBJ("datatype", &tmpca.datatype, DB_INT);
   DEFALL_OBJ("elemnames", &name_vector, DB_CHAR);
   DEFALL_OBJ("elemlengths", &tmpca.elemlengths, DB_INT);
   if (PJ_GetObject(dbfile->pdb, array_name, &tmp_obj, DB_ARRAY) < 0)
       return NULL;
   if (NULL == (ca = DBAllocCompoundarray()))
      return NULL;
   *ca = tmpca;

   if (ca->nelems <= 0 || ca->nvalues <= 0 || ca->datatype < 0 ||
       !name_vector) {
      DBFreeCompoundarray(ca);
      db_perror(array_name, E_NOTFOUND, me);
      return NULL;
   }

   /*
    *  Internally, the meshnames are stored in a single character
    *  string as a delimited set of names. Here we break them into
    *  separate names.  The delimiter is the first character of the
    *  name vector.
    */
   if (name_vector && ca->nelems > 0) {
      ca->elemnames = ALLOC_N(char *, ca->nelems);

      delim[0] = name_vector[0];
      delim[1] = '\0';
      for (i = 0; i < ca->nelems; i++) {
         s = strtok(i ? NULL : (name_vector + 1), delim);
         ca->elemnames[i] = STRDUP(s);
      }
      FREE(name_vector);
   }

   /*
    * Read the rest of the object--the vector of values, per datatype.
    */
   INIT_OBJ(&tmp_obj);
   if (DB_DOUBLE == ca->datatype && PJ_InqForceSingle()) {
      ca->datatype = DB_FLOAT;
   }

   DEFALL_OBJ("values", &ca->values, ca->datatype);
   PJ_GetObject(dbfile->pdb, array_name, &tmp_obj, 0);

   ca->id = 0;
   ca->name = STRDUP(array_name);

   return ca;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetCurve
 *
 * Purpose:     Read a curve object from a PDB data file.
 *
 * Return:      Success:        pointer to the new object
 *
 *              Failure:        NULL, db_errno set
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.com
 *              May 16, 1996
 *
 * Modifications:
 *
 *      Sean Ahern, Wed Jun 14 17:19:43 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *      Thomas R. Treadway, Fri Jul  7 11:43:41 PDT 2006
 *      Added DBOPT_REFERENCE support.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBcurve *
db_pdb_GetCurve (DBfile *_dbfile, char const *name)
{
   DBfile_pdb   *dbfile = (DBfile_pdb *) _dbfile ;
   DBcurve      *cu ;
   static char  *me = "db_pdb_GetCurve" ;
   PJcomplist   tmp_obj ;
   DBcurve tmpcu;
   PJcomplist  *_tcl;

   memset(&tmpcu, 0, sizeof(DBcurve));
   INIT_OBJ (&tmp_obj) ;
   DEFINE_OBJ ("npts", &tmpcu.npts, DB_INT) ;
   DEFINE_OBJ ("datatype", &tmpcu.datatype, DB_INT) ;
   DEFALL_OBJ ("label",    &tmpcu.title,    DB_CHAR) ;
   DEFALL_OBJ ("xvarname", &tmpcu.xvarname, DB_CHAR) ;
   DEFALL_OBJ ("yvarname", &tmpcu.yvarname, DB_CHAR) ;
   DEFALL_OBJ ("xlabel",   &tmpcu.xlabel,   DB_CHAR) ;
   DEFALL_OBJ ("ylabel",   &tmpcu.ylabel,   DB_CHAR) ;
   DEFALL_OBJ ("xunits",   &tmpcu.xunits,   DB_CHAR) ;
   DEFALL_OBJ ("yunits",   &tmpcu.yunits,   DB_CHAR) ;
   DEFALL_OBJ ("reference",&tmpcu.reference,DB_CHAR) ;
   DEFINE_OBJ ("guihide",  &tmpcu.guihide,  DB_INT) ;
   DEFINE_OBJ ("coord_sys",  &tmpcu.coord_sys,  DB_INT) ;
   DEFINE_OBJ ("missing_value", &tmpcu.missing_value, DB_DOUBLE);
   if (PJ_GetObject (dbfile->pdb, name, &tmp_obj, DB_CURVE)<0)
       return NULL ;
   if (NULL == (cu = DBAllocCurve ())) return NULL ;
   *cu = tmpcu;

   if (DB_DOUBLE == cu->datatype && PJ_InqForceSingle())
      cu->datatype = DB_FLOAT ;

   /*
    * Read the x and y arrays.
    */
   if (DBGetDataReadMask2File(_dbfile) & DBCurveArrays)
   {
      if (cu->reference && (cu->x || cu->y)) {
         db_perror ("x and y not NULL", E_BADARGS, me) ;
         return NULL ;
      } else if (cu->reference) {
         cu->x = NULL;
         cu->y = NULL;
      } else {
         INIT_OBJ (&tmp_obj) ;
         DEFALL_OBJ ("xvals", &cu->x, cu->datatype) ;
         DEFALL_OBJ ("yvals", &cu->y, cu->datatype) ;
         PJ_GetObject (dbfile->pdb, name, &tmp_obj, 0) ;
      }
   }

   if      (cu->missing_value == DB_MISSING_VALUE_NOT_SET)
       cu->missing_value = 0.0;
   else if (cu->missing_value == 0.0)
       cu->missing_value = DB_MISSING_VALUE_NOT_SET;

   cu->id = 0 ;

   return cu ;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetComponent
 *
 * Purpose:     Read a component value from the data file.
 *
 * Return:      Success:        pointer to component.
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 08:26:55 PST 1994
 *
 * Modifications:
 *     Sean Ahern, Sun Oct  1 03:10:32 PDT 1995
 *     Made "me" static.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK void *
db_pdb_GetComponent (DBfile *_dbfile, char const *objname, char const *compname)
{
   void          *result;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   static char   *me = "db_pdb_GetComponent";

   result = PJ_GetComponent(dbfile->pdb, objname, compname);
   if (!result) {
      db_perror("PJ_GetComponent", E_CALLFAIL, me);
      return NULL;
   }

   return result;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetComponentType
 *
 * Purpose:     Read a component type from the data file.
 *
 * Return:      Success:        integer representing component type.
 *
 *              Failure:        DB_NOTYPE
 *
 * Programmer:  Brad Whitlock
 *              Thu Jan 20 12:02:29 PDT 2000
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_GetComponentType (DBfile *_dbfile, char const *objname, char const *compname)
{
   int           retval = DB_NOTYPE;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;

   retval = PJ_GetComponentType(dbfile->pdb, objname, compname);
   return retval;
}

/*----------------------------------------------------------------------
 *  Routine                                         db_pdb_GetDefvars
 *
 *  Purpose
 *
 *      Read a defvars structure from the given database.
 *
 *  Programmger
 *
 *      Mark C. Miller,
 *      August 8, 2005
 *
 *  Modifications:
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *
 *   Mark C. Miller, Thu Jan  6 17:01:28 PST 2011
 *   Fix name of guihide array (removed the extra 's').
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBdefvars *
db_pdb_GetDefvars(DBfile *_dbfile, char const *objname)
{
   DBdefvars     *defv = NULL;
   int            ncomps, type;
   char          *tmpnames, *tmpdefns, tmp[256];
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   static char   *me = "db_pdb_GetDefvars";

   db_pdb_getobjinfo(dbfile->pdb, (char *) objname, tmp, &ncomps);
   type = DBGetObjtypeTag(tmp);

   /* Read multi-block object */
   if (type == DB_DEFVARS)
   {
       DBdefvars tmpdefv;
       PJcomplist *_tcl;

       memset(&tmpdefv, 0, sizeof(DBdefvars));
       INIT_OBJ(&tmp_obj);
       DEFINE_OBJ("ndefs", &tmpdefv.ndefs, DB_INT);
       DEFALL_OBJ("types", &tmpdefv.types, DB_INT);
       DEFALL_OBJ("guihide", &tmpdefv.guihides, DB_INT);
       DEFALL_OBJ("names", &tmpnames, DB_CHAR);
       DEFALL_OBJ("defns", &tmpdefns, DB_CHAR);

       if (PJ_GetObject(dbfile->pdb, (char *) objname, &tmp_obj, DB_DEFVARS) < 0)
           return NULL;
       if ((defv = DBAllocDefvars(0)) == NULL)
           return NULL;
       *defv = tmpdefv;

       if ((tmpnames != NULL) && (defv->ndefs > 0))
       {
           defv->names = DBStringListToStringArray(tmpnames, &(defv->ndefs), !skipFirstSemicolon);
           FREE(tmpnames);
       }

       if ((tmpdefns != NULL) && (defv->ndefs > 0))
       {
           defv->defns = DBStringListToStringArray(tmpdefns, &(defv->ndefs), !skipFirstSemicolon);
           FREE(tmpdefns);
       }
   }

   return (defv);
}

/*-------------------------------------------------------------------------
 * Function:    pdb_getvarinfo
 *
 * Purpose:
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *      Sean Ahern, Wed Apr 12 11:14:38 PDT 2000
 *      Removed the last two parameters to PJ_inquire_entry because they
 *      weren't being used.
 *-------------------------------------------------------------------------*/
INTERNAL int
pdb_getvarinfo (PDBfile *pdbfile,
                char    *name,    /*Name of variable to inquire about */
                char    *type,    /*Returned datatype of 'name' */
                int     *num,     /*Returned number of elements */
                int     *size,    /*Returned element size */
                int     verbose)  /*Sentinel: 1==print error msgs; 0==quiet*/
{
   int            is_ptr;
   char           lastchar;
   char          *s;
   defstr        *dp;
   syment        *ep;

   *num = *size = 0;
   if (type)
      type[0] = '\0';

   /*
    *  Get symbol table entry for requested variable.
    */
   ep = PJ_inquire_entry(pdbfile, name);
   if (ep == NULL)
      return (OOPS);

   /* Assign values */
   if (type)
      strcpy(type, ep->type);

   lastchar = STR_LASTCHAR(ep->type);
   if (lastchar == '*')
      is_ptr = 1;
   else
      is_ptr = 0;

   if (is_ptr) {
      s = STRDUP(ep->type);
      s[strcspn(s, " *")] = '\0';

      /* Get data size of primitive (non-pointer) unit */
      dp = PD_inquire_host_type(pdbfile, s);
      *size = dp->size;
      *num = -1;              /* lite_SC_arrlen(values) / dp->size; */
      if (verbose)
         printf("Cannot query length of pointered variable.\n");

      FREE(s);

   }
   else {
      dp = PD_inquire_host_type(pdbfile, ep->type);
      if (dp == NULL) {
         if (verbose)
            printf("Don't know about data of type: %s\n", ep->type);
         return (OOPS);
      }

      /* Assign values */
      *size = dp->size;
      *num = ep->number;
   }

   return (OKAY);
}

/*----------------------------------------------------------------------
 *  Routine                                         db_pdb_GetMultimesh
 *
 *  Purpose
 *
 *      Read a multi-block-mesh structure from the given database. If the
 *      requested object is not multi-block, return the information for
 *      that one mesh only.
 *
 * Modifications
 *    Al Leibee, Wed Jul  7 08:00:00 PDT 1993
 *    Change SCALLOC_N to ALLOC_N and SC_strsave to STR_SAVE to be less
 *    SCORE dependent.  Change FREE to SCFREE to be consistant with
 *    allocation.
 *
 *    Robb Matzke, Mon Nov 14 14:37:33 EST 1994
 *    Device independence rewrite.
 *
 *    Robb Matzke, Fri Dec 2 14:11:57 PST 1994
 *    Removed references to SCORE memory management: `tmpnames'
 *
 *    Eric Brugger, Fri Jan 27 09:33:44 PST 1995
 *    I modified the routine to return NULL unless the variable is
 *    a multimesh.
 *
 *    Sean Ahern, Sun Oct  1 03:11:55 PDT 1995
 *    Made "me" static.
 *
 *    Eric Brugger, Wed Jul  2 13:30:05 PDT 1997
 *    Modify the call to DBAllocMultimesh to not allocate any arrays
 *    in the multimesh structure.
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added ngroups, blockorigin, and grouporigin.
 *
 *    Jeremy Meredith, Fri Jul 23 09:08:09 PDT 1999
 *    Added a check to stop strtok from occurring past the end of the
 *    array.  This was crashing the code intermittently.
 *
 *    Sean Ahern, Wed Jun 14 17:19:47 PDT 2000
 *    Added a check to make sure the object is the right type.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *    Thomas R. Treadway, Thu Jul 20 13:34:57 PDT 2006
 *    Added lgroupings, groupings, and groupnames options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Replaced strtok-loop over ...names member with call to
 *   db_StringListToStringArray. Added logic to control behavior of
 *   slash character swapping for windows/linux and skipping of first
 *   semicolon in calls to db_StringListToStringArray.
 *
 *   Mark C. Miller, Wed Jul 14 20:40:55 PDT 2010
 *   Added support for namescheme/empty list options.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBmultimesh *
db_pdb_GetMultimesh (DBfile *_dbfile, char const *objname)
{
   DBmultimesh   *mm = NULL;
   int            ncomps, type;
   char          *tmpnames=0, tmp[256];
   char          *tmpgnames = 0;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   static char   *me = "db_pdb_GetMultimesh";
   PJcomplist    *_tcl;

   db_pdb_getobjinfo(dbfile->pdb, objname, tmp, &ncomps);
   type = DBGetObjtypeTag(tmp);

   if (type == DB_MULTIMESH) {

      DBmultimesh tmpmm;
      memset(&tmpmm, 0, sizeof(DBmultimesh));

      /* Read multi-block object */
      INIT_OBJ(&tmp_obj);
      DEFINE_OBJ("nblocks", &tmpmm.nblocks, DB_INT);
      DEFINE_OBJ("ngroups", &tmpmm.ngroups, DB_INT);
      DEFINE_OBJ("blockorigin", &tmpmm.blockorigin, DB_INT);
      DEFINE_OBJ("grouporigin", &tmpmm.grouporigin, DB_INT);
      DEFINE_OBJ("guihide", &tmpmm.guihide, DB_INT);
      DEFALL_OBJ("meshids", &tmpmm.meshids, DB_INT);
      if (DBGetDataReadMask2File(_dbfile) & DBMBNamesAndTypes)
      {
          DEFALL_OBJ("meshtypes", &tmpmm.meshtypes, DB_INT);
          DEFALL_OBJ("meshnames", &tmpnames, DB_CHAR);
      }
      DEFALL_OBJ("meshdirs", &tmpmm.dirids, DB_INT);
      if (DBGetDataReadMask2File(_dbfile) & DBMBOptions)
      {
          DEFINE_OBJ("extentssize", &tmpmm.extentssize, DB_INT);
          DEFALL_OBJ("extents", &tmpmm.extents, DB_DOUBLE);
          DEFALL_OBJ("zonecounts", &tmpmm.zonecounts, DB_INT);
          DEFALL_OBJ("has_external_zones", &tmpmm.has_external_zones, DB_INT);
      }
      DEFINE_OBJ("lgroupings", &tmpmm.lgroupings, DB_INT);
      DEFALL_OBJ("groupings", &tmpmm.groupings, DB_INT);
      DEFALL_OBJ("groupnames", &tmpgnames, DB_CHAR);
      DEFALL_OBJ("mrgtree_name", &tmpmm.mrgtree_name, DB_CHAR);
      DEFINE_OBJ("tv_connectivity", &tmpmm.tv_connectivity, DB_INT);
      DEFINE_OBJ("disjoint_mode", &tmpmm.disjoint_mode, DB_INT);
      DEFINE_OBJ("topo_dim", &tmpmm.topo_dim, DB_INT);
      DEFALL_OBJ("file_ns", &tmpmm.file_ns, DB_CHAR);
      DEFALL_OBJ("block_ns", &tmpmm.block_ns, DB_CHAR);
      DEFINE_OBJ("block_type", &tmpmm.block_type, DB_INT);
      DEFALL_OBJ("empty_list", &tmpmm.empty_list, DB_INT);
      DEFINE_OBJ("empty_cnt", &tmpmm.empty_cnt, DB_INT);
      DEFINE_OBJ("repr_block_idx", &tmpmm.repr_block_idx, DB_INT);

      if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_MULTIMESH) < 0)
         return NULL;
      if ((mm = DBAllocMultimesh(0)) == NULL)
         return NULL;
      *mm = tmpmm;

      /* The value we store to the file for 'topo_dim' member is
         designed such that zero indicates a value that was NOT
         specified in the file. Since zero is a valid topological
         dimension, when we store topo_dim to a file, we always
         add 1. So, we have to subtract it here. This was implemented
         for multimeshes in 4.7 and so is handled correctly for
         them in all cases. Likewise for repr_block_idx. */
       mm->topo_dim = mm->topo_dim - 1;
       mm->repr_block_idx = mm->repr_block_idx-1;

      /*----------------------------------------
       *  Internally, the meshnames and groupings 
       *  iare stored in a single character string 
       *  as a delimited set of names. Here we break
       *  them into separate names.
       *----------------------------------------*/

      if (tmpnames != NULL) {
         if (mm->nblocks > 0)
             db_StringListToStringArrayMBOpt(tmpnames, &(mm->meshnames), &(mm->meshnames_alloc), mm->nblocks);
       /*FREE(tmpnames); We don't free this here because the MBOpt routine creates pointers into it. */
      }
      if ((tmpgnames != NULL) && (mm->lgroupings > 0)) {
         mm->groupnames = DBStringListToStringArray(tmpgnames, &(mm->lgroupings), !skipFirstSemicolon);
         FREE(tmpgnames);
      }
   }

   return (mm);
}

/*----------------------------------------------------------------------
 *  Routine                                      db_pdb_GetMultimeshadj
 *
 *  Purpose
 *
 *      Read some or all of a multi-block-mesh adjacency object from
 *      the given database.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBmultimeshadj *
db_pdb_GetMultimeshadj (DBfile *_dbfile, char const *objname, int nmesh,
    int const *block_map)
{
   DBmultimeshadj   *mmadj = NULL;
   int            ncomps, type;
   int            i, j, tmpnmesh;
   char          tmp[256], *nlsname = 0, zlsname = 0;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   static char   *me = "db_pdb_GetMultimeshadj";
   char           tmpn[256];
   int           *offsetmap, *offsetmapn=0, *offsetmapz=0, lneighbors, tmpoff;
   PJcomplist    *_tcl;

   db_pdb_getobjinfo(dbfile->pdb, (char*)objname, tmp, &ncomps);
   type = DBGetObjtypeTag(tmp);

   if (type == DB_MULTIMESHADJ) {

      DBmultimeshadj tmpmmadj;
      memset(&tmpmmadj, 0, sizeof(DBmultimeshadj));

      /* Read multi-block object */
      INIT_OBJ(&tmp_obj);
      DEFINE_OBJ("nblocks", &tmpmmadj.nblocks, DB_INT);
      DEFINE_OBJ("lneighbors", &tmpmmadj.lneighbors, DB_INT);
      DEFINE_OBJ("totlnodelists", &tmpmmadj.totlnodelists, DB_INT);
      DEFINE_OBJ("totlzonelists", &tmpmmadj.totlzonelists, DB_INT);
      DEFINE_OBJ("blockorigin", &tmpmmadj.blockorigin, DB_INT);
      DEFALL_OBJ("meshtypes", &tmpmmadj.meshtypes, DB_INT);
      DEFALL_OBJ("nneighbors", &tmpmmadj.nneighbors, DB_INT);
      DEFALL_OBJ("neighbors", &tmpmmadj.neighbors, DB_INT);
      DEFALL_OBJ("back", &tmpmmadj.back, DB_INT);
      DEFALL_OBJ("lnodelists", &tmpmmadj.lnodelists, DB_INT);
      DEFALL_OBJ("lzonelists", &tmpmmadj.lzonelists, DB_INT);

      if (PJ_GetObject(dbfile->pdb, (char*)objname, &tmp_obj, DB_MULTIMESHADJ) < 0)
         return NULL;
      if ((mmadj = DBAllocMultimeshadj(0)) == NULL)
         return NULL;
      *mmadj = tmpmmadj;

       offsetmap = ALLOC_N(int, mmadj->nblocks);
       lneighbors = 0;
       for (i = 0; i < mmadj->nblocks; i++)
       {
           offsetmap[i] = lneighbors;
           lneighbors += mmadj->nneighbors[i];
       }

       if (mmadj->lnodelists && (DBGetDataReadMask2File(_dbfile) & DBMMADJNodelists))
       {
          mmadj->nodelists = ALLOC_N(int *, lneighbors); 
          offsetmapn = ALLOC_N(int, mmadj->nblocks);
          tmpoff = 0;
          for (i = 0; i < mmadj->nblocks; i++)
          {
              offsetmapn[i] = tmpoff;
              for (j = 0; j < mmadj->nneighbors[i]; j++)
                 tmpoff += mmadj->lnodelists[offsetmap[i]+j];
          }
       }

       if (mmadj->lzonelists && (DBGetDataReadMask2File(_dbfile) & DBMMADJZonelists))
       {
          mmadj->zonelists = ALLOC_N(int *, lneighbors); 
          offsetmapz = ALLOC_N(int, mmadj->nblocks);
          tmpoff = 0;
          for (i = 0; i < mmadj->nblocks; i++)
          {
              offsetmapz[i] = tmpoff;
              for (j = 0; j < mmadj->nneighbors[i]; j++)
                 tmpoff += mmadj->lzonelists[offsetmap[i]+j];
          }
       }
       
       tmpnmesh = nmesh;
       if (nmesh <= 0 || !block_map)
           tmpnmesh = mmadj->nblocks;

       /* This loop could be optimized w.r.t. number of I/O requests
          it makes. The nodelists and/or zonelists could be read in
          a single call. But then we'd have to split it into separate
          arrays duplicating memory */
       for (i = 0; (i < tmpnmesh) &&
                   (DBGetDataReadMask2File(_dbfile) & (DBMMADJNodelists|DBMMADJZonelists)); i++)
       {
          int blockno = block_map ? block_map[i] : i;

          if (mmadj->lnodelists && (DBGetDataReadMask2File(_dbfile) & DBMMADJNodelists))
          {
             tmpoff = offsetmapn[blockno];
             for (j = 0; j < mmadj->nneighbors[blockno]; j++)
             {
                long ind[3];
                int len = mmadj->lnodelists[offsetmap[blockno]+j];
                int *nlist = ALLOC_N(int, len);

                ind[0] = tmpoff;
                ind[1] = tmpoff + len - 1;
                ind[2] = 1;
                db_mkname(dbfile->pdb, (char*)objname, "nodelists", tmpn);
                if (!PJ_read_alt(dbfile->pdb, tmpn, nlist, ind)) {
                   FREE(offsetmap);
                   FREE(offsetmapn);
                   FREE(offsetmapz);
                   db_perror("PJ_read_alt", E_CALLFAIL, me);
                }

                mmadj->nodelists[offsetmap[blockno]+j] = nlist;
                tmpoff += len;
             }
          }

          if (mmadj->lzonelists && (DBGetDataReadMask2File(_dbfile) & DBMMADJZonelists))
          {
             tmpoff = offsetmapz[blockno];
             for (j = 0; j < mmadj->nneighbors[blockno]; j++)
             {
                long ind[3];
                int len = mmadj->lzonelists[offsetmap[blockno]+j];
                int *zlist = ALLOC_N(int, len);

                ind[0] = tmpoff;
                ind[1] = tmpoff + len - 1;
                ind[2] = 1;
                db_mkname(dbfile->pdb, (char*)objname, "zonelists", tmpn);
                if (!PJ_read_alt(dbfile->pdb, tmpn, zlist, ind)) {
                   FREE(offsetmap);
                   FREE(offsetmapn);
                   FREE(offsetmapz);
                   db_perror("PJ_read_alt", E_CALLFAIL, me);
                }

                mmadj->zonelists[offsetmap[blockno]+j] = zlist;
                tmpoff += len;
             }
          }
       }

       FREE(offsetmap);
       FREE(offsetmapn);
       FREE(offsetmapz);
   }

   return (mmadj);
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetMultivar
 *
 * Purpose:     Read a multi-block-var structure from the given database.
 *
 * Return:      Success:        ptr to a new structure
 *
 *              Failure:        NULL
 *
 * Programmer:  robb@cloud
 *              Tue Feb 21 11:04:39 EST 1995
 *
 * Modifications:
 *    Sean Ahern, Sun Oct  1 03:12:16 PDT 1995
 *    Made "me" static.
 *
 *    Eric Brugger, Wed Jul  2 13:30:05 PDT 1997
 *    Modify the call to DBAllocMultivar to not allocate any arrays
 *    in the multivar structure.
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added ngroups, blockorigin, and grouporigin.
 *
 *    Jeremy Meredith, Fri Jul 23 09:08:09 PDT 1999
 *    Added a check to stop strtok from occurring past the end of the
 *    array.  This was crashing the code intermittently.
 *
 *    Sean Ahern, Wed Jun 14 17:20:24 PDT 2000
 *    Added a check to make sure the object is the right type.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *    Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *    Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Replaced strtok-loop over ...names member with call to
 *   db_StringListToStringArray. Added logic to control behavior of
 *   slash character swapping for windows/linux and skipping of first
 *   semicolon in calls to db_StringListToStringArray.
 *
 *   Mark C. Miller, Wed Jul 14 20:40:55 PDT 2010
 *   Added support for namescheme/empty list options.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBmultivar *
db_pdb_GetMultivar (DBfile *_dbfile, char const *objname)
{
   DBmultivar    *mv = NULL;
   int            ncomps, type;
   char          *tmpnames=0, tmp[256];
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   static char   *me = "db_pdb_GetMultivar";
   char          *rpnames = NULL;
   PJcomplist    *_tcl;

   db_pdb_getobjinfo(dbfile->pdb, objname, tmp, &ncomps);
   type = DBGetObjtypeTag(tmp);

   if (type == DB_MULTIVAR) {

      DBmultivar tmpmv;
      memset(&tmpmv, 0, sizeof(DBmultivar));

      /* Read multi-block object */
      INIT_OBJ(&tmp_obj);
      DEFINE_OBJ("nvars", &tmpmv.nvars, DB_INT);
      if (DBGetDataReadMask2File(_dbfile) & DBMBNamesAndTypes)
      {
          DEFALL_OBJ("vartypes", &tmpmv.vartypes, DB_INT);
          DEFALL_OBJ("varnames", &tmpnames, DB_CHAR);
      }
      DEFINE_OBJ("ngroups", &tmpmv.ngroups, DB_INT);
      DEFINE_OBJ("blockorigin", &tmpmv.blockorigin, DB_INT);
      DEFINE_OBJ("grouporigin", &tmpmv.grouporigin, DB_INT);
      if (DBGetDataReadMask2File(_dbfile) & DBMBOptions)
      {
          DEFINE_OBJ("extentssize", &tmpmv.extentssize, DB_INT);
          DEFALL_OBJ("extents", &tmpmv.extents, DB_DOUBLE);
      }
      DEFINE_OBJ("guihide", &tmpmv.guihide, DB_INT);
      DEFALL_OBJ("region_pnames", &rpnames, DB_CHAR);
      DEFINE_OBJ("tensor_rank", &tmpmv.tensor_rank, DB_INT);
      DEFALL_OBJ("mmesh_name", &tmpmv.mmesh_name, DB_CHAR);
      DEFINE_OBJ("conserved", &tmpmv.conserved, DB_INT);
      DEFINE_OBJ("extensive", &tmpmv.extensive, DB_INT);
      DEFALL_OBJ("file_ns", &tmpmv.file_ns, DB_CHAR);
      DEFALL_OBJ("block_ns", &tmpmv.block_ns, DB_CHAR);
      DEFINE_OBJ("block_type", &tmpmv.block_type, DB_INT);
      DEFALL_OBJ("empty_list", &tmpmv.empty_list, DB_INT);
      DEFINE_OBJ("empty_cnt", &tmpmv.empty_cnt, DB_INT);
      DEFINE_OBJ("repr_block_idx", &tmpmv.repr_block_idx, DB_INT);
      DEFINE_OBJ("missing_value", &tmpmv.missing_value, DB_DOUBLE);

      if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_MULTIVAR) < 0)
         return NULL;
      if ((mv = DBAllocMultivar(0)) == NULL)
         return NULL;
      *mv = tmpmv;

      /* -1 to support zero value indicating NOT SET */
      mv->repr_block_idx = mv->repr_block_idx - 1;

      /*----------------------------------------
       *  Internally, the varnames are stored
       *  in a single character string as a
       *  delimited set of names. Here we break
       *  them into separate names.
       *----------------------------------------*/

      if (tmpnames != NULL) {
         if (mv->nvars > 0)
             db_StringListToStringArrayMBOpt(tmpnames, &(mv->varnames), &(mv->varnames_alloc), mv->nvars);
       /*FREE(tmpnames); We don't free this here because the MBOpt routine creates pointers into it. */
      }

      if (rpnames != NULL)
      {
         mv->region_pnames = DBStringListToStringArray(rpnames, 0, !skipFirstSemicolon);
         FREE(rpnames);
      }

      if      (mv->missing_value == DB_MISSING_VALUE_NOT_SET)
          mv->missing_value = 0.0;
      else if (mv->missing_value == 0.0)
          mv->missing_value = DB_MISSING_VALUE_NOT_SET;

   }

   return (mv);
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetMultimat
 *
 * Purpose:     Read a multi-material structure from the given database.
 *
 * Return:      Success:        ptr to a new structure
 *
 *              Failure:        NULL
 *
 * Programmer:  robb@cloud
 *              Tue Feb 21 12:39:51 EST 1995
 *
 * Modifications:
 *    Sean Ahern, Sun Oct  1 03:12:39 PDT 1995
 *    Made "me" static.
 *
 *    Sean Ahern, Mon Jun 24 14:44:44 PDT 1996
 *    Added better error checking to keep strtok from running off the
 *    end of the tmpnames array,
 *
 *    Sean Ahern, Mon Jun 24 15:03:43 PDT 1996
 *    Fixed a memory leak where mt->matnames was being allocated twice,
 *    once in DBAllocMultimat, and once here.
 *
 *    Sean Ahern, Tue Jun 25 13:41:23 PDT 1996
 *    Since we know that material names are delimited by ';', we don't
 *    need the delim array.  Just use ";" instead.
 *
 *    Eric Brugger, Wed Jul  2 13:30:05 PDT 1997
 *    Modify the call to DBAllocMultimat to not allocate any arrays
 *    in the multimat structure.
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added ngroups, blockorigin, and grouporigin.
 *
 *    Sean Ahern, Wed Jun 14 17:20:55 PDT 2000
 *    Added a check to make sure the object is the right type.
 *
 *    Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *    Moved DBAlloc call to after PJ_GetObject. Added automatic
 *    var for PJ_GetObject to read into. Added check for return
 *    value of PJ_GetObject.
 *
 *    Mark C. Miller, Mon Aug  7 17:03:51 PDT 2006
 *    Added material_names and matcolors as well as nmatnos and matnos
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Replaced strtok-loop over ...names member with call to
 *   db_StringListToStringArray. Added logic to control behavior of
 *   slash character swapping for windows/linux and skipping of first
 *   semicolon in calls to db_StringListToStringArray.
 *
 *   Mark C. Miller, Thu Feb  4 11:14:20 PST 2010
 *   Added missing logic for setting allowmat0.
 *
 *   Mark C. Miller, Wed Jul 14 20:40:55 PDT 2010
 *   Added support for namescheme/empty list options.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBmultimat *
db_pdb_GetMultimat (DBfile *_dbfile, char const *objname)
{
   DBmultimat    *mt = NULL;
   int            ncomps, type;
   char          *tmpnames=NULL, *s=NULL, *name=NULL, tmp[256];
   char          *tmpmatcolors=NULL, *tmpmaterial_names=NULL;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   static char   *me = "db_pdb_GetMultimat";
   PJcomplist    *_tcl;

   db_pdb_getobjinfo(dbfile->pdb, objname, tmp, &ncomps);
   type = DBGetObjtypeTag(tmp);

   if (type == DB_MULTIMAT) {

      DBmultimat tmpmt;
      memset(&tmpmt, 0, sizeof(DBmultimat));

      /* Read multi-block object */
      INIT_OBJ(&tmp_obj);
      DEFINE_OBJ("nmats", &tmpmt.nmats, DB_INT);
      if (DBGetDataReadMask2File(_dbfile) & DBMBNamesAndTypes)
          DEFALL_OBJ("matnames", &tmpnames, DB_CHAR);
      DEFINE_OBJ("ngroups", &tmpmt.ngroups, DB_INT);
      DEFINE_OBJ("blockorigin", &tmpmt.blockorigin, DB_INT);
      DEFINE_OBJ("grouporigin", &tmpmt.grouporigin, DB_INT);
      DEFINE_OBJ("nmatnos", &tmpmt.nmatnos, DB_INT);
      if (DBGetDataReadMask2File(_dbfile) & DBMBOptions)
      {
          DEFALL_OBJ("matnos", &tmpmt.matnos, DB_INT);
          DEFALL_OBJ("mixlens", &tmpmt.mixlens, DB_INT);
          DEFALL_OBJ("matcounts", &tmpmt.matcounts, DB_INT);
          DEFALL_OBJ("matlists", &tmpmt.matlists, DB_INT);
      }
      DEFINE_OBJ("guihide", &tmpmt.guihide, DB_INT);
      DEFINE_OBJ("allowmat0", &tmpmt.allowmat0, DB_INT);
      DEFALL_OBJ("material_names", &tmpmaterial_names, DB_CHAR);
      DEFALL_OBJ("matcolors", &tmpmatcolors, DB_CHAR);
      DEFALL_OBJ("mmesh_name", &tmpmt.mmesh_name, DB_CHAR);
      DEFALL_OBJ("file_ns", &tmpmt.file_ns, DB_CHAR);
      DEFALL_OBJ("block_ns", &tmpmt.block_ns, DB_CHAR);
      DEFALL_OBJ("empty_list", &tmpmt.empty_list, DB_INT);
      DEFINE_OBJ("empty_cnt", &tmpmt.empty_cnt, DB_INT);
      DEFINE_OBJ("repr_block_idx", &tmpmt.repr_block_idx, DB_INT);

      if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_MULTIMAT) < 0)
         return NULL;
      if ((mt = DBAllocMultimat(0)) == NULL)
         return NULL;
      *mt = tmpmt;

      /* -1 to support zero value indicating NOT SET */
      mt->repr_block_idx = mt->repr_block_idx - 1;

      /*----------------------------------------
       *  Internally, the material names are stored
       *  in a single character string as a
       *  delimited set of names. Here we break
       *  them into separate names.
       *----------------------------------------*/

      if (tmpnames != NULL) {
          if (mt->nmats > 0)
              db_StringListToStringArrayMBOpt(tmpnames, &(mt->matnames), &(mt->matnames_alloc), mt->nmats);
        /*FREE(tmpnames); We don't free this here because the MBOpt routine creates pointers into it. */
      }

      if (tmpmaterial_names && mt->nmatnos > 0)
      {
          mt->material_names = DBStringListToStringArray(tmpmaterial_names,
              &(mt->nmatnos), !skipFirstSemicolon);
          FREE(tmpmaterial_names);
      }
      if (tmpmatcolors && mt->nmatnos > 0)
      {
          mt->matcolors = DBStringListToStringArray(tmpmatcolors,
              &(mt->nmatnos), !skipFirstSemicolon);
          FREE(tmpmatcolors);
      }
   }

   return (mt);
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetMultimatspecies
 *
 * Purpose:     Read a multi-species structure from the given database.
 *
 * Return:      Success:        ptr to a new structure
 *
 *              Failure:        NULL
 *
 * Programmer:  Jeremy S. Meredith
 *              Sept 17 1998
 *
 * Modifications:
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added ngroups, blockorigin, and grouporigin.
 *
 *    Sean Ahern, Wed Jun 14 17:21:18 PDT 2000
 *    Added a check to make sure the object is the right type.
 *
 *    Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *    Moved DBAlloc call to after PJ_GetObject. Added automatic
 *    var for PJ_GetObject to read into. Added check for return
 *    value of PJ_GetObject.
 *
 *    Mark C. Miller, Mon Aug  7 17:03:51 PDT 2006
 *    Added nmat and nmatspec
 *
 *    Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *    Added names and colors for species.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Replaced strtok-loop over ...names member with call to
 *   db_StringListToStringArray. Added logic to control behavior of
 *   slash character swapping for windows/linux and skipping of first
 *   semicolon in calls to db_StringListToStringArray.
 *
 *   Mark C. Miller, Wed Jul 14 20:40:55 PDT 2010
 *   Added support for namescheme/empty list options.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBmultimatspecies *
db_pdb_GetMultimatspecies (DBfile *_dbfile, char const *objname)
{
   DBmultimatspecies *mms = NULL;
   int                ncomps, type;
   int                i, nstrs = 0;
   char              *tmpnames=NULL, *s=NULL, *name=NULL, tmp[256];
   char              *tmpspecnames=NULL, *tmpcolors=NULL;
   DBfile_pdb        *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist         tmp_obj;
   static char       *me = "db_pdb_GetMultimatspecies";
   PJcomplist    *_tcl;

   db_pdb_getobjinfo(dbfile->pdb, objname, tmp, &ncomps);
   type = DBGetObjtypeTag(tmp);

   if (type == DB_MULTIMATSPECIES) {

      DBmultimatspecies tmpmms;
      memset(&tmpmms, 0, sizeof(DBmultimatspecies));

      /* Read multi-block object */
      INIT_OBJ(&tmp_obj);
      DEFINE_OBJ("nspec", &tmpmms.nspec, DB_INT);
      if (DBGetDataReadMask2File(_dbfile) & DBMBNamesAndTypes)
          DEFALL_OBJ("specnames", &tmpnames, DB_CHAR);
      DEFINE_OBJ("ngroups", &tmpmms.ngroups, DB_INT);
      DEFINE_OBJ("blockorigin", &tmpmms.blockorigin, DB_INT);
      DEFINE_OBJ("grouporigin", &tmpmms.grouporigin, DB_INT);
      DEFINE_OBJ("guihide", &tmpmms.guihide, DB_INT);
      DEFINE_OBJ("nmat", &tmpmms.nmat, DB_INT);
      DEFALL_OBJ("nmatspec", &tmpmms.nmatspec, DB_INT);
      if (DBGetDataReadMask2File(_dbfile) & DBMatMatnames)
         DEFALL_OBJ("species_names", &tmpspecnames, DB_CHAR);
      if (DBGetDataReadMask2File(_dbfile) & DBMatMatcolors)
         DEFALL_OBJ("speccolors", &tmpcolors, DB_CHAR);
      DEFALL_OBJ("file_ns", &tmpmms.file_ns, DB_CHAR);
      DEFALL_OBJ("block_ns", &tmpmms.block_ns, DB_CHAR);
      DEFALL_OBJ("empty_list", &tmpmms.empty_list, DB_INT);
      DEFINE_OBJ("empty_cnt", &tmpmms.empty_cnt, DB_INT);
      DEFINE_OBJ("repr_block_idx", &tmpmms.repr_block_idx, DB_INT);
      DEFALL_OBJ("matname", &tmpmms.matname, DB_CHAR);

      if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_MULTIMATSPECIES) < 0)
         return NULL;
      if ((mms = DBAllocMultimatspecies(0)) == NULL)
         return NULL;
      *mms = tmpmms;

      /* -1 to support zero value indicating NOT SET */
      mms->repr_block_idx = mms->repr_block_idx - 1;

      /*----------------------------------------
       *  Internally, the material species names
       *  are stored in a single character string
       *  as a delimited set of names. Here we
       *  break them into separate names.
       *----------------------------------------*/

      if (tmpnames != NULL)
      {
          if (mms->nspec > 0)
              db_StringListToStringArrayMBOpt(tmpnames, &(mms->specnames), &(mms->specnames_alloc), mms->nspec);
          /*FREE(tmpnames); We don't free this here because the MBOpt routine creates pointers into it. */
      }

      if (tmpspecnames != NULL)
      {
          for (i = 0; i < mms->nmat; i++)
              nstrs += mms->nmatspec[i];
          if (nstrs > 0)
              mms->species_names = DBStringListToStringArray(tmpspecnames, &nstrs, !skipFirstSemicolon);
          FREE(tmpspecnames);
      }
      if (tmpcolors != NULL)
      {
          if (nstrs == 0)
          {
              for (i = 0; i < mms->nmat; i++)
                  nstrs += mms->nmatspec[i];
          }
          if (nstrs > 0)
              mms->speccolors = DBStringListToStringArray(tmpcolors, &nstrs, !skipFirstSemicolon);
          FREE(tmpcolors);
      }
   }

   return (mms);
}

/*----------------------------------------------------------------------
 *  Routine                                         db_pdb_GetPointmesh
 *
 *  Purpose
 *
 *      Read a point-mesh object from a SILO file and return the
 *      SILO structure for this type.
 *
 *  Modifications
 *
 *      Al Leibee, Tue Apr 19 08:56:11 PDT 1994
 *      Added dtime.
 *
 *      Robb Matzke, Mon Nov 14 15:49:15 EST 1994
 *      Device independence rewrite
 *
 *      Robb Matzke, 27 Aug 1997
 *      The datatype is changed to DB_FLOAT only if it was DB_DOUBLE
 *      and force_single is turned on.
 *
 *      Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *      Added group_no.
 *
 *      Sean Ahern, Wed Jun 14 17:21:36 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *      Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *      Add support for long long global node/zone numbers.
 *
 *      Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *      Changed how long long global node/zone numbers are supported
 *      from a int (bool), "llong_gnode|zoneno" to an int holding
 *      the actual datatype. The type is assumed int if it its
 *      value is zero or it does not exist. Otherwise, the type is
 *      is whatever is stored in gnznodtype member. 
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBpointmesh *
db_pdb_GetPointmesh (DBfile *_dbfile, char const *objname)
{
   DBpointmesh   *pm = NULL;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   static char *me = "db_pdb_GetPointmesh";
   DBpointmesh tmppm;
   PJcomplist    *_tcl;
   char          *tmpannums = 0;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmppm, 0, sizeof(DBpointmesh));
   INIT_OBJ(&tmp_obj);

   DEFINE_OBJ("block_no", &tmppm.block_no, DB_INT);
   DEFINE_OBJ("group_no", &tmppm.group_no, DB_INT);
   DEFINE_OBJ("cycle", &tmppm.cycle, DB_INT);
   DEFINE_OBJ("time", &tmppm.time, DB_FLOAT);
   DEFINE_OBJ("dtime", &tmppm.dtime, DB_DOUBLE);
   DEFINE_OBJ("datatype", &tmppm.datatype, DB_INT);
   DEFINE_OBJ("ndims", &tmppm.ndims, DB_INT);
   DEFINE_OBJ("nels", &tmppm.nels, DB_INT);
   DEFINE_OBJ("origin", &tmppm.origin, DB_INT);
   DEFINE_OBJ("gnznodtype", &tmppm.gnznodtype, DB_INT);

   DEFINE_OBJ("min_extents", tmppm.min_extents, DB_FLOAT);
   DEFINE_OBJ("max_extents", tmppm.max_extents, DB_FLOAT);

   DEFINE_OBJ("guihide", &tmppm.guihide, DB_INT);
   DEFALL_OBJ("mrgtree_name", &tmppm.mrgtree_name, DB_CHAR);

   if (DBGetDataReadMask2File(_dbfile) & DBPMCoords)
   {
       DEFALL_OBJ("coord0", &tmppm.coords[0], DB_FLOAT);
       DEFALL_OBJ("coord1", &tmppm.coords[1], DB_FLOAT);
       DEFALL_OBJ("coord2", &tmppm.coords[2], DB_FLOAT);
   }

   DEFALL_OBJ("label0", &tmppm.labels[0], DB_CHAR);
   DEFALL_OBJ("label1", &tmppm.labels[1], DB_CHAR);
   DEFALL_OBJ("label2", &tmppm.labels[2], DB_CHAR);
   DEFALL_OBJ("units0", &tmppm.units[0], DB_CHAR);
   DEFALL_OBJ("units1", &tmppm.units[1], DB_CHAR);
   DEFALL_OBJ("units2", &tmppm.units[2], DB_CHAR);

   if (DBGetDataReadMask2File(_dbfile) & DBPMGhostNodeLabels)
       DEFALL_OBJ("ghost_node_labels", &tmppm.ghost_node_labels, DB_CHAR);
   DEFALL_OBJ("alt_nodenum_vars",  &tmpannums,  DB_CHAR);

   if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_POINTMESH) < 0)
      return NULL;
   if ((pm = DBAllocPointmesh()) == NULL)
      return NULL;
   *pm = tmppm;

   if (tmpannums != NULL)
   {
       pm->alt_nodenum_vars = DBStringListToStringArray(tmpannums, 0, !skipFirstSemicolon);
       FREE(tmpannums);
   }

   /*
    *  Read the remainder of the object: loop over all values
    *  associated with this variable.
    */
   pm->gnznodtype = tmppm.gnznodtype?tmppm.gnznodtype:DB_INT;
   if (DBGetDataReadMask2File(_dbfile) & DBPMGlobNodeNo) {
      INIT_OBJ(&tmp_obj);
      DEFALL_OBJ("gnodeno", &tmppm.gnodeno, pm->gnznodtype);
      pm->gnodeno = 0;
      if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, 0)>=0)
          pm->gnodeno = tmppm.gnodeno;
   }

   pm->id = 0;
   pm->name = STRDUP(objname);
   if (DB_DOUBLE==pm->datatype && PJ_InqForceSingle()) {
      pm->datatype = DB_FLOAT;
   }

   return (pm);
}

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_GetPointvar
 *
 *  Purpose
 *
 *      Read a point-var object from a SILO file and return the
 *      SILO structure for this type.
 *
 *  Modifications
 *
 *      Al Leibee, Tue Apr 19 08:56:11 PDT 1994
 *      Added dtime.
 *
 *      Robb Matzke, Mon Nov 14 15:58:36 EST 1994
 *      Device independence rewrite.
 *
 *      Sean Ahern, Fri May 24 17:35:58 PDT 1996
 *      Added smarts for figuring out the datatype, similar to
 *      db_pdb_GetUcdvar and db_pdb_GetQuadvar.
 *
 *      Sean Ahern, Fri May 24 17:36:31 PDT 1996
 *      I corrected a possible bug where, if force single were on,
 *      the datatype would only be set to DB_FLOAT if the datatype
 *      had previously been DB_DOUBLE.  This ignored things like
 *      DB_INT.
 *
 *      Eric Brugger, Fri Sep  6 08:12:40 PDT 1996
 *      I removed the reading of "meshid" since it is not used and
 *      is actually stored as a string in the silo files not an
 *      integer.  This caused a memory overwrite which only hurt
 *      things if the string was more than 104 characters.
 *
 *      Sam Wookey, Wed Jul  1 16:21:09 PDT 1998
 *      Corrected the way we retrieve variables with nvals > 1.
 *
 *      Sean Ahern, Wed Jun 14 17:22:54 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Eric Brugger, Tue Mar  2 16:21:15 PST 2004
 *      Modified the routine to use _ptvalstr for the names of the
 *      components when the variable has more than one component.
 *      Each component name needs to be a seperate string for things
 *      to work properly, since a pointer to the string is stored and
 *      not a copy of the string.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *      Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *      Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBmeshvar *
db_pdb_GetPointvar (DBfile *_dbfile, char const *objname)
{
   DBmeshvar     *mv = NULL;
   int            i;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   char           tmp[256];
   static char *me = "db_pdb_GetPointvar";
   char          *rpnames = NULL;
   DBmeshvar tmpmv;
   PJcomplist    *_tcl;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmpmv, 0, sizeof(DBmeshvar));
   INIT_OBJ(&tmp_obj);

   DEFINE_OBJ("cycle", &tmpmv.cycle, DB_INT);
   DEFINE_OBJ("time", &tmpmv.time, DB_FLOAT);
   DEFINE_OBJ("dtime", &tmpmv.dtime, DB_DOUBLE);
   DEFINE_OBJ("datatype", &tmpmv.datatype, DB_INT);
   DEFINE_OBJ("ndims", &tmpmv.ndims, DB_INT);
   DEFINE_OBJ("nels", &tmpmv.nels, DB_INT);
   DEFINE_OBJ("nvals", &tmpmv.nvals, DB_INT);
   DEFINE_OBJ("origin", &tmpmv.origin, DB_INT);

   DEFALL_OBJ("label", &tmpmv.label, DB_CHAR);
   DEFALL_OBJ("units", &tmpmv.units, DB_CHAR);
   DEFALL_OBJ("meshid",&tmpmv.meshname, DB_CHAR);
   DEFINE_OBJ("guihide", &tmpmv.guihide, DB_INT);
   DEFALL_OBJ("region_pnames", &rpnames, DB_CHAR);
   DEFINE_OBJ("conserved", &tmpmv.conserved, DB_INT);
   DEFINE_OBJ("extensive", &tmpmv.extensive, DB_INT);
   DEFINE_OBJ("missing_value", &tmpmv.missing_value, DB_DOUBLE);

   if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_POINTVAR) < 0)
      return NULL;
   if ((mv = DBAllocMeshvar()) == NULL)
      return NULL;
   *mv = tmpmv;

   /*
    *  Read the remainder of the object: loop over all values
    *  associated with this variable.
    */

   if (mv->ndims>0 && mv->nels>0 && mv->nvals>0 && (DBGetDataReadMask2File(_dbfile) & DBPVData)) {
      INIT_OBJ(&tmp_obj);

      mv->vals = ALLOC_N(void*, mv->nvals);

      if (mv->datatype == 0) {
          if(mv->nvals == 1)
          {
              sprintf(tmp, "%s_data", objname);
          }
          else
          {
              sprintf(tmp, "%s_0_data", objname);
          }
          if ((mv->datatype = db_pdb_GetVarDatatype(dbfile->pdb, tmp)) < 0) {
              /* Not found. Assume float. */
              mv->datatype = DB_FLOAT;
          }
      }

      if (PJ_InqForceSingle())
          mv->datatype = DB_FLOAT;

      if(mv->nvals == 1)
      {
          DEFALL_OBJ("_data", &mv->vals[0], DB_FLOAT);
      }
      else
      {
          for (i = 0; i < mv->nvals; i++)
          {
              DEFALL_OBJ(_ptvalstr[i], &mv->vals[i], DB_FLOAT);
          }
      }

      PJ_GetObject(dbfile->pdb, objname, &tmp_obj, 0);
   }

   if (rpnames != NULL)
   {
      mv->region_pnames = DBStringListToStringArray(rpnames, 0, !skipFirstSemicolon);
      FREE(rpnames);
   }

   if      (mv->missing_value == DB_MISSING_VALUE_NOT_SET)
       mv->missing_value = 0.0;
   else if (mv->missing_value == 0.0)
       mv->missing_value = DB_MISSING_VALUE_NOT_SET;

   mv->id = 0;
   mv->name = STRDUP(objname);

   return (mv);
}

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_GetQuadmesh
 *
 *  Purpose
 *
 *      Read a quad-mesh object from a SILO file and return the
 *      SILO structure for this type.
 *
 *  Modifications
 *
 *      Al Leibee, Tue Apr 19 08:56:11 PDT 1994
 *      Added dtime.
 *
 *      Robb Matzke, Mon Nov 14 16:03:55 EST 1994
 *      Device independence rewrite.
 *
 *      Sean Ahern, Fri Aug  1 13:24:28 PDT 1997
 *      Reformatted code through "indent".
 *
 *      Sean Ahern, Fri Aug  1 13:24:45 PDT 1997
 *      Changed how the datatype is computed.  It used to be unconditionally
 *      set to DB_FLOAT.  Now we only do that if we're doing a force single.
 *
 *      Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *      Added group_no and baseindex[].
 *
 *      Eric Brugger, Thu Aug 26 09:05:20 PDT 1999
 *      I modified the routine to set the base_index field base on
 *      the origin field when the base_index field is not defined in
 *      the file.
 *
 *      Sean Ahern, Wed Jun 14 17:36:42 PDT 2000
 *      Added checks to make sure the object has the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBquadmesh *
db_pdb_GetQuadmesh (DBfile *_dbfile, char const *objname)
{
    DBquadmesh     *qm = NULL;
    PJcomplist      tmp_obj;
    DBfile_pdb     *dbfile = (DBfile_pdb *) _dbfile;
    static char *me = "db_pdb_GetQuadmesh";
    DBquadmesh tmpqm;
    PJcomplist    *_tcl;
    char           *tmpannum = 0, *tmpaznum = 0;

    memset(&tmpqm, 0, sizeof(DBquadmesh));
    tmpqm.base_index[0] = -99999;

    /*------------------------------------------------------------*
     * Comp. Name        Comp. Address     Data Type              *
     *------------------------------------------------------------*/
    INIT_OBJ(&tmp_obj);

    DEFINE_OBJ("block_no", &tmpqm.block_no, DB_INT);
    DEFINE_OBJ("group_no", &tmpqm.group_no, DB_INT);
    DEFINE_OBJ("cycle", &tmpqm.cycle, DB_INT);
    DEFINE_OBJ("time", &tmpqm.time, DB_FLOAT);
    DEFINE_OBJ("dtime", &tmpqm.dtime, DB_DOUBLE);
    DEFINE_OBJ("datatype", &tmpqm.datatype, DB_INT);
    DEFINE_OBJ("coord_sys", &tmpqm.coord_sys, DB_INT);
    DEFINE_OBJ("coordtype", &tmpqm.coordtype, DB_INT);
    DEFINE_OBJ("facetype", &tmpqm.facetype, DB_INT);
    DEFINE_OBJ("planar", &tmpqm.planar, DB_INT);
    DEFINE_OBJ("ndims", &tmpqm.ndims, DB_INT);
    DEFINE_OBJ("nspace", &tmpqm.nspace, DB_INT);
    DEFINE_OBJ("nnodes", &tmpqm.nnodes, DB_INT);
    DEFINE_OBJ("major_order", &tmpqm.major_order, DB_INT);
    DEFINE_OBJ("origin", &tmpqm.origin, DB_INT);

    if (DBGetDataReadMask2File(_dbfile) & DBQMCoords)
    {
        DEFALL_OBJ("coord0", &tmpqm.coords[0], DB_FLOAT);
        DEFALL_OBJ("coord1", &tmpqm.coords[1], DB_FLOAT);
        DEFALL_OBJ("coord2", &tmpqm.coords[2], DB_FLOAT);
    }
    DEFALL_OBJ("label0", &tmpqm.labels[0], DB_CHAR);
    DEFALL_OBJ("label1", &tmpqm.labels[1], DB_CHAR);
    DEFALL_OBJ("label2", &tmpqm.labels[2], DB_CHAR);
    DEFALL_OBJ("units0", &tmpqm.units[0], DB_CHAR);
    DEFALL_OBJ("units1", &tmpqm.units[1], DB_CHAR);
    DEFALL_OBJ("units2", &tmpqm.units[2], DB_CHAR);

    DEFINE_OBJ("dims", tmpqm.dims, DB_INT);
    DEFINE_OBJ("min_index", tmpqm.min_index, DB_INT);
    DEFINE_OBJ("max_index", tmpqm.max_index, DB_INT);
    DEFINE_OBJ("min_extents", tmpqm.min_extents, DB_FLOAT);
    DEFINE_OBJ("max_extents", tmpqm.max_extents, DB_FLOAT);
    DEFINE_OBJ("baseindex", tmpqm.base_index, DB_INT);
    DEFINE_OBJ("guihide", &tmpqm.guihide, DB_INT);
    DEFALL_OBJ("mrgtree_name", &tmpqm.mrgtree_name, DB_CHAR);

    if (DBGetDataReadMask2File(_dbfile) & DBQMGhostNodeLabels)
        DEFALL_OBJ("ghost_node_labels", &tmpqm.ghost_node_labels, DB_CHAR);
    if (DBGetDataReadMask2File(_dbfile) & DBQMGhostZoneLabels)
        DEFALL_OBJ("ghost_zone_labels", &tmpqm.ghost_zone_labels, DB_CHAR);
    DEFALL_OBJ("alt_nodenum_vars", &tmpannum, DB_CHAR);
    DEFALL_OBJ("alt_zonenum_vars", &tmpaznum, DB_CHAR);

    /* The type passed here to PJ_GetObject is NULL because quadmeshes can have
     * one of two types:  DB_COLLINEAR or DB_NONCOLLINEAR.  */
    if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_QUADMESH) < 0)
       return NULL;
    if ((qm = DBAllocQuadmesh()) == NULL)
       return NULL;
    *qm = tmpqm;

    if (tmpannum)
    {
        qm->alt_nodenum_vars = DBStringListToStringArray(tmpannum, 0, !skipFirstSemicolon);
        FREE(tmpannum);
    }
    if (tmpaznum)
    {
        qm->alt_zonenum_vars = DBStringListToStringArray(tmpaznum, 0, !skipFirstSemicolon);
        FREE(tmpaznum);
    }

    /*
     * If base_index was not set with the PJ_GetObject call, then
     * set it based on origin.
     */
    if (qm->base_index[0] == -99999)
    {
        int       i;
        qm->base_index[0] = 0;
        for (i = 0; i < qm->ndims; i++)
        {
            qm->base_index[i] = qm->origin;
        }
    }

    qm->id = 0;
    qm->name = STRDUP(objname);

    if (PJ_InqForceSingle())
        qm->datatype = DB_FLOAT;

    _DBQMSetStride(qm);

    return (qm);
}

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_GetQuadvar
 *
 *  Purpose
 *
 *      Read a quad-var object from a SILO file and return the
 *      SILO structure for this type.
 *
 *  Modifications
 *
 *      Al Leibee, Wed Jul 20 07:56:37 PDT 1994
 *      Added get of mixed zone data arrays.
 *
 *      Al Leibee, Tue Apr 19 08:56:11 PDT 1994
 *      Added dtime.
 *
 *      Robb Matzke, Mon Nov 14 17:02:44 EST 1994
 *      Device independence rewrite.
 *
 *      Sean Ahern, Fri Sep 29 18:39:08 PDT 1995
 *      Requested DB_FLOATS instead of qv->datatype.
 *
 *      Sean Ahern, Fri May 24 17:20:00 PDT 1996
 *      I corrected a possible bug where, if force single were on,
 *      the datatype would only be set to DB_FLOAT if the datatype
 *      had previously been DB_DOUBLE.  This ignored things like
 *      DB_INT.
 *
 *      Robb Matzke, 19 Jun 1997
 *      Reads the optional `ascii_labels' field.
 *
 *      Sean Ahern, Wed Jun 14 17:23:49 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Brad Whitlock, Wed Jul 21 12:05:56 PDT 2004
 *      I fixed a coding error that prevented PJ_GetObject from
 *      reading in labels and units for the quadvar.
 *      value of PJ_GetObject.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *      Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *      Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBquadvar *
db_pdb_GetQuadvar (DBfile *_dbfile, char const *objname)
{
   DBquadvar     *qv = NULL;
   int            i;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   char           tmp[256];
   static char *me = "db_pdb_GetQuadvar";
   char          *rpnames = NULL;
   DBquadvar tmpqv;
   PJcomplist    *_tcl;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmpqv, 0, sizeof(DBquadvar));
   INIT_OBJ(&tmp_obj);

   /* Scalars */
   DEFINE_OBJ("cycle", &tmpqv.cycle, DB_INT);
   DEFINE_OBJ("time", &tmpqv.time, DB_FLOAT);
   DEFINE_OBJ("dtime", &tmpqv.dtime, DB_DOUBLE);
   DEFINE_OBJ("datatype", &tmpqv.datatype, DB_INT);
   DEFINE_OBJ("centering", &tmpqv.centering, DB_INT);
   DEFINE_OBJ("ndims", &tmpqv.ndims, DB_INT);
   DEFINE_OBJ("major_order", &tmpqv.major_order, DB_INT);
   DEFINE_OBJ("nels", &tmpqv.nels, DB_INT);
   DEFINE_OBJ("nvals", &tmpqv.nvals, DB_INT);
   DEFINE_OBJ("origin", &tmpqv.origin, DB_INT);
   DEFINE_OBJ("mixlen", &tmpqv.mixlen, DB_INT);
   DEFINE_OBJ("use_specmf", &tmpqv.use_specmf, DB_INT);
   DEFINE_OBJ("ascii_labels", &tmpqv.ascii_labels, DB_INT);
   DEFALL_OBJ("meshid", &tmpqv.meshname, DB_CHAR);
   DEFINE_OBJ("guihide", &tmpqv.guihide, DB_INT);
   DEFINE_OBJ("conserved", &tmpqv.conserved, DB_INT);
   DEFINE_OBJ("conserved", &tmpqv.extensive, DB_INT);
   DEFINE_OBJ("missing_value", &tmpqv.missing_value, DB_DOUBLE);

   /* Arrays */
   DEFINE_OBJ("min_index", tmpqv.min_index, DB_INT);
   DEFINE_OBJ("max_index", tmpqv.max_index, DB_INT);
   DEFINE_OBJ("dims", tmpqv.dims, DB_INT);
   DEFINE_OBJ("align", tmpqv.align, DB_FLOAT);
   DEFALL_OBJ("region_pnames", &rpnames, DB_CHAR);

   /* Arrays that PJ_GetObject must allocate. */
   DEFALL_OBJ("label", &tmpqv.label, DB_CHAR);
   DEFALL_OBJ("units", &tmpqv.units, DB_CHAR);

   if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_QUADVAR) < 0)
      return NULL;

   /* Patch up "centering" if it wasn't present in file but align is.
      Align only worked for node/zone centering. */
   tmpqv.centering = db_fix_obsolete_centering(tmpqv.ndims, tmpqv.align, tmpqv.centering);

   if ((qv = DBAllocQuadvar()) == NULL)
      return NULL;
   *qv = tmpqv;

   /*
    *  Read the remainder of the object: loop over all values
    *  associated with this variable.
    */

   if ((qv->ndims>0) && (qv->nvals > 0) && (DBGetDataReadMask2File(_dbfile) & DBQVData)) {
      INIT_OBJ(&tmp_obj);

      qv->vals = ALLOC_N(void*, qv->nvals);

      if (qv->mixlen > 0) {
         qv->mixvals = ALLOC_N(void*, qv->nvals);
      }

      if (qv->datatype == 0) {
         strcpy(tmp, objname);
         strcat(tmp, "_data");
         if ((qv->datatype = db_pdb_GetVarDatatype(dbfile->pdb, tmp)) < 0) {
            /* Not found. Assume float. */
            qv->datatype = DB_FLOAT;
         }
      }

      if (PJ_InqForceSingle())
         qv->datatype = DB_FLOAT;

      for (i = 0; i < qv->nvals; i++) {
         DEFALL_OBJ(_valstr[i], &qv->vals[i], DB_FLOAT);

         if (qv->mixlen > 0) {
            DEFALL_OBJ(_mixvalstr[i], &qv->mixvals[i], DB_FLOAT);
         }
      }

      PJ_GetObject(dbfile->pdb, objname, &tmp_obj, 0);
   }

   if (rpnames != NULL)
   {
      qv->region_pnames = DBStringListToStringArray(rpnames, 0, !skipFirstSemicolon);
      FREE(rpnames);
   }

   if      (qv->missing_value == DB_MISSING_VALUE_NOT_SET)
       qv->missing_value = 0.0;
   else if (qv->missing_value == 0.0)
       qv->missing_value = DB_MISSING_VALUE_NOT_SET;

   qv->id = 0;
   qv->name = STRDUP(objname);

   _DBQQCalcStride(qv->stride, qv->dims, qv->ndims, qv->major_order);

   return (qv);
}

/*----------------------------------------------------------------------
 *  Routine                                                DBGetUcdmesh
 *
 *  Purpose
 *
 *      Read a ucd-mesh structure from the given database.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Parameters
 *
 *      DBGetUcdmesh  {Out}   {Pointer to DBucdmesh structure}
 *      dbfile        {In}    {Pointer to current file}
 *      meshname      {In}    {Name of ucdmesh to read}
 *
 *  Notes
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 15 10:54:15 EST 1994
 *      Device independence rewrite.
 *
 *      Eric Brugger, Wed Dec 11 13:01:52 PST 1996
 *      I plugged some memory leaks by freeing flname, zlname and
 *      elname at the end.
 *
 *      Eric Brugger, Tue Oct 20 12:46:53 PDT 1998
 *      Modify the routine to read in the min_index and max_index
 *      fields of the zonelist.
 *
 *      Eric Brugger, Tue Mar 30 11:17:58 PST 1999
 *      Modify the routine to read in the shapetype field of the
 *      zonelist.  This field is optional.
 *
 *      Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *      Added group_no, gnodeno, and gzoneno.
 *
 *      Sean Ahern, Wed Jun 14 17:24:07 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc calls to after PJ_GetObject. Added automatic
 *      vars for PJ_GetObject to read into. Added checks for return
 *      value of PJ_GetObject calls.
 *
 *      Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *      Add support for long long global node/zone numbers. Protect
 *      call to db_SplitShapelist if data read mask did not include
 *      zonelist info.
 *
 *      Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *      Changed how long long global node/zone numbers are supported
 *      from a int (bool), "llong_gnode|zoneno" to an int holding
 *      the actual datatype. The type is assumed int if it its
 *      value is zero or it does not exist. Otherwise, the type is
 *      is whatever is stored in gnznodtype member. 
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBucdmesh *
db_pdb_GetUcdmesh (DBfile *_dbfile, char const *meshname)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   DBucdmesh     *um = NULL;
   char          *flname = NULL, *zlname = NULL, *elname = NULL,
                 *phzlname = NULL;
   PJcomplist     tmp_obj;
   int            lo_offset, hi_offset;
   static char *me = "db_pdb_GetUcdmesh";
   DBucdmesh tmpum;
   PJcomplist    *_tcl;
   char          *tmpannum = 0;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmpum, 0, sizeof(DBucdmesh));
   INIT_OBJ(&tmp_obj);

   DEFINE_OBJ("block_no", &tmpum.block_no, DB_INT);
   DEFINE_OBJ("group_no", &tmpum.group_no, DB_INT);
   DEFINE_OBJ("cycle", &tmpum.cycle, DB_INT);
   DEFINE_OBJ("time", &tmpum.time, DB_FLOAT);
   DEFINE_OBJ("dtime", &tmpum.dtime, DB_DOUBLE);
   DEFINE_OBJ("datatype", &tmpum.datatype, DB_INT);
   DEFINE_OBJ("coord_sys", &tmpum.coord_sys, DB_INT);
   DEFINE_OBJ("topo_dim", &tmpum.topo_dim, DB_INT);
   DEFINE_OBJ("ndims", &tmpum.ndims, DB_INT);
   DEFINE_OBJ("nnodes", &tmpum.nnodes, DB_INT);
   DEFINE_OBJ("origin", &tmpum.origin, DB_INT);

   DEFINE_OBJ("min_extents", tmpum.min_extents, DB_FLOAT);
   DEFINE_OBJ("max_extents", tmpum.max_extents, DB_FLOAT);

   if (DBGetDataReadMask2File(_dbfile) & DBUMCoords)
   {
       DEFALL_OBJ("coord0", &tmpum.coords[0], DB_FLOAT);
       DEFALL_OBJ("coord1", &tmpum.coords[1], DB_FLOAT);
       DEFALL_OBJ("coord2", &tmpum.coords[2], DB_FLOAT);
   }
   DEFALL_OBJ("label0", &tmpum.labels[0], DB_CHAR);
   DEFALL_OBJ("label1", &tmpum.labels[1], DB_CHAR);
   DEFALL_OBJ("label2", &tmpum.labels[2], DB_CHAR);
   DEFALL_OBJ("units0", &tmpum.units[0], DB_CHAR);
   DEFALL_OBJ("units1", &tmpum.units[1], DB_CHAR);
   DEFALL_OBJ("units2", &tmpum.units[2], DB_CHAR);
   DEFINE_OBJ("guihide", &tmpum.guihide, DB_INT);
   DEFINE_OBJ("gnznodtype", &tmpum.gnznodtype, DB_INT);

   /* Get SILO ID's for other UCD mesh components */
   DEFALL_OBJ("facelist", &flname, DB_CHAR);
   DEFALL_OBJ("zonelist", &zlname, DB_CHAR);
   DEFALL_OBJ("edgelist", &elname, DB_CHAR);

   /* Optional polyhedral zonelist */
   DEFALL_OBJ("phzonelist", &phzlname, DB_CHAR);

   DEFALL_OBJ("mrgtree_name", &tmpum.mrgtree_name, DB_CHAR);
   DEFINE_OBJ("tv_connectivity", &tmpum.tv_connectivity, DB_INT);
   DEFINE_OBJ("disjoint_mode", &tmpum.disjoint_mode, DB_INT);

   if (DBGetDataReadMask2File(_dbfile) & DBUMGhostNodeLabels)
       DEFALL_OBJ("ghost_node_labels", &tmpum.ghost_node_labels, DB_CHAR);
   DEFALL_OBJ("alt_nodenum_vars", &tmpannum, DB_CHAR);

   if (PJ_GetObject(dbfile->pdb, meshname, &tmp_obj, DB_UCDMESH) < 0)
      return NULL;
   if ((um = DBAllocUcdmesh()) == NULL)
      return NULL;
   *um = tmpum;

   if (tmpannum)
   {
       um->alt_nodenum_vars = DBStringListToStringArray(tmpannum, 0, !skipFirstSemicolon);
       FREE(tmpannum);
   }

   if (PJ_InqForceSingle() == TRUE)
      um->datatype = DB_FLOAT;

   um->id = 0;
   um->name = STRDUP(meshname);

   /* The value we store to the file for 'topo_dim' member is
      designed such that zero indicates a value that was NOT
      specified in the file. Since zero is a valid topological
      dimension, when we store topo_dim to a file, we always
      add 1. So, we have to subtract it here. However, this
      data member was not being handled correctly in files
      versions before 4.7. So, for older files, if topo_dim
      is non-zero we just pass it without alteration. */
   if (!DBFileVersionGE(_dbfile,4,5,1) || DBFileVersionGE(_dbfile, 4,7,0))
       um->topo_dim = um->topo_dim - 1;

   /* Optional global node number */
   um->gnznodtype = um->gnznodtype?um->gnznodtype:DB_INT;
   if (um->nnodes>0 && (DBGetDataReadMask2File(_dbfile) & DBUMGlobNodeNo)) {
      INIT_OBJ(&tmp_obj);
      DEFALL_OBJ("gnodeno", &tmpum.gnodeno, um->gnznodtype);
      um->gnodeno = 0;
      if (PJ_GetObject(dbfile->pdb, meshname, &tmp_obj, 0)>=0)
          um->gnodeno = tmpum.gnodeno;
   }

   /* Read facelist, zonelist, and edgelist */

   if (um->nnodes>0 && (flname != NULL && strlen(flname) > 0)
       && (DBGetDataReadMask2File(_dbfile) & DBUMFacelist))
   {
      DBfacelist tmpfaces;
      memset(&tmpfaces, 0, sizeof(DBfacelist));


      /*------------------------------------------------------------*/
      /*          Comp. Name        Comp. Address     Data Type     */
      /*------------------------------------------------------------*/
      INIT_OBJ(&tmp_obj);

      DEFINE_OBJ("ndims", &tmpfaces.ndims, DB_INT);
      DEFINE_OBJ("nfaces", &tmpfaces.nfaces, DB_INT);
      DEFINE_OBJ("lnodelist", &tmpfaces.lnodelist, DB_INT);
      DEFINE_OBJ("nshapes", &tmpfaces.nshapes, DB_INT);
      DEFINE_OBJ("ntypes", &tmpfaces.ntypes, DB_INT);
      DEFINE_OBJ("origin", &tmpfaces.origin, DB_INT);

      DEFALL_OBJ("nodelist", &tmpfaces.nodelist, DB_INT);
      DEFALL_OBJ("shapesize", &tmpfaces.shapesize, DB_INT);
      DEFALL_OBJ("shapecnt", &tmpfaces.shapecnt, DB_INT);
      DEFALL_OBJ("typelist", &tmpfaces.typelist, DB_INT);
      DEFALL_OBJ("types", &tmpfaces.types, DB_INT);
      DEFALL_OBJ("zoneno", &tmpfaces.zoneno, DB_INT);

      if (PJ_GetObject(dbfile->pdb, flname, &tmp_obj, 0) < 0)
      {
         DBFreeUcdmesh(um);
         return NULL;
      }
      if ((um->faces = DBAllocFacelist()) == NULL)
      {
         DBFreeUcdmesh(um);
         return NULL;
      }
      *(um->faces) = tmpfaces;

   }

   if (um->nnodes>0 && (zlname != NULL && strlen(zlname) > 0)
       && (DBGetDataReadMask2File(_dbfile) & DBUMZonelist))
   {
      DBzonelist tmpzones;
      memset(&tmpzones, 0, sizeof(DBzonelist));

      /*------------------------------------------------------------*/
      /*          Comp. Name        Comp. Address     Data Type     */
      /*------------------------------------------------------------*/
      INIT_OBJ(&tmp_obj);

      DEFINE_OBJ("ndims", &tmpzones.ndims, DB_INT);
      DEFINE_OBJ("nzones", &tmpzones.nzones, DB_INT);
      DEFINE_OBJ("nshapes", &tmpzones.nshapes, DB_INT);
      DEFINE_OBJ("lnodelist", &tmpzones.lnodelist, DB_INT);
      DEFINE_OBJ("origin", &tmpzones.origin, DB_INT);

      DEFALL_OBJ("nodelist", &tmpzones.nodelist, DB_INT);
      DEFALL_OBJ("shapetype", &tmpzones.shapetype, DB_INT);
      DEFALL_OBJ("shapesize", &tmpzones.shapesize, DB_INT);
      DEFALL_OBJ("shapecnt", &tmpzones.shapecnt, DB_INT);
      DEFINE_OBJ("gnznodtype", &tmpzones.gnznodtype, DB_INT);

      /*----------------------------------------------------------*/
      /* These are optional so set them to their default values   */
      /*----------------------------------------------------------*/
      lo_offset = 0;
      hi_offset = 0;
      DEFINE_OBJ("lo_offset", &lo_offset, DB_INT);
      DEFINE_OBJ("hi_offset", &hi_offset, DB_INT);
 
      if (DBGetDataReadMask2File(_dbfile) & DBZonelistGhostZoneLabels)
         DEFALL_OBJ("ghost_zone_labels", &tmpzones.ghost_zone_labels, DB_CHAR);

      if (PJ_GetObject(dbfile->pdb, zlname, &tmp_obj, DB_ZONELIST) < 0)
      {
         DBFreeUcdmesh(um);
         return NULL;
      }
      if ((um->zones = DBAllocZonelist()) == NULL)
      {
         DBFreeUcdmesh(um);
         return NULL;
      }
      *(um->zones) = tmpzones;

      um->zones->min_index = lo_offset;
      um->zones->max_index = um->zones->nzones - hi_offset - 1;

      /*----------------------------------------------------------*/
      /* If we have ghost zones, split any group of shapecnt so   */
      /* all the shapecnt refer to all real zones or all ghost    */
      /* zones.  This will make dealing with ghost zones easier   */
      /* for applications.                                        */
      /*----------------------------------------------------------*/
      if ((lo_offset != 0 || hi_offset != 0) &&
          DBGetDataReadMask2File(_dbfile) & DBZonelistInfo)
      {
          db_SplitShapelist (um);
      }

      /* Read optional global zone numbers */
      um->zones->gnznodtype = um->zones->gnznodtype?um->zones->gnznodtype:DB_INT;
      if (DBGetDataReadMask2File(_dbfile) & DBZonelistGlobZoneNo) {
          INIT_OBJ(&tmp_obj);
          DEFALL_OBJ("gzoneno", &tmpzones.gzoneno, um->zones->gnznodtype);
          um->zones->gzoneno = 0;
          if (PJ_GetObject(dbfile->pdb, zlname, &tmp_obj, 0)>=0)
              um->zones->gzoneno = tmpzones.gzoneno;
      }

   }

   if (um->nnodes>0 && elname && *elname) {
      DBedgelist tmpedges;
      memset(&tmpedges, 0, sizeof(DBedgelist));


      /*------------------------------------------------------------*/
      /*          Comp. Name        Comp. Address     Data Type     */
      /*------------------------------------------------------------*/
      INIT_OBJ(&tmp_obj);

      DEFINE_OBJ("ndims", &tmpedges.ndims, DB_INT);
      DEFINE_OBJ("nedges", &tmpedges.nedges, DB_INT);
      DEFINE_OBJ("origin", &tmpedges.origin, DB_INT);

      DEFALL_OBJ("edge_beg", &tmpedges.edge_beg, DB_INT);
      DEFALL_OBJ("edge_end", &tmpedges.edge_end, DB_INT);

      if (PJ_GetObject(dbfile->pdb, elname, &tmp_obj, 0) < 0)
      {
         DBFreeUcdmesh(um);
         return NULL;
      }
      if ((um->edges = DBAllocEdgelist()) == NULL)
      {
         DBFreeUcdmesh(um);
         return NULL;
      }
      *(um->edges) = tmpedges;
   }

   if (um->nnodes>0 && phzlname && *phzlname && (DBGetDataReadMask2File(_dbfile) & DBUMZonelist)) {
      um->phzones = db_pdb_GetPHZonelist(_dbfile, phzlname);
   }

   FREE (zlname);
   FREE (flname);
   FREE (elname);
   FREE (phzlname);

   return (um);

}

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_GetUcdvar
 *
 *  Purpose
 *
 *      Read a ucd-var object from a SILO file and return the
 *      SILO structure for this type.
 *
 *  Modifications
 *
 *      Al Leibee, Tue Apr 19 08:56:11 PDT 1994
 *      Added dtime.
 *
 *      Robb Matzke, Tue Nov 15 11:05:44 EST 1994
 *      Device independence rewrite.
 *
 *      Sean Ahern, Tue Dec  5 16:38:55 PST 1995
 *      Added PDB logic to determine datatype
 *      if it's not in the SILO file.
 *
 *      Eric Brugger, Thu May 23 10:02:33 PDT 1996
 *      I corrected a bug where the datatype was being incorrectly set
 *      to DOUBLE, if force single were on and the datatype was
 *      specified in the silo file.
 *
 *      Sean Ahern, Fri May 24 17:20:00 PDT 1996
 *      I corrected a possible bug where, if force single were on,
 *      the datatype would only be set to DB_FLOAT if the datatype
 *      had previously been DB_DOUBLE.  This ignored things like
 *      DB_INT.
 *
 *      Eric Brugger, Fri Sep  6 08:12:40 PDT 1996
 *      I removed the reading of "meshid" since it is not used and
 *      is actually stored as a string in the silo files not an
 *      integer.  This caused a memory overwrite which only hurt
 *      things if the string was more than 48 characters.
 *
 *      Sean Ahern, Wed Jun 14 17:24:41 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *      Brad Whitlock, Wed Jan 18 15:07:57 PST 2006
 *      Added optional ascii_labels.
 *
 *      Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *      Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBucdvar *
db_pdb_GetUcdvar (DBfile *_dbfile, char const *objname)
{
   DBucdvar      *uv = NULL;
   int            i;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   char           tmp[256];
   static char *me = "db_pdb_GetUcdvar";
   char          *rpnames = NULL;
   DBucdvar tmpuv;
   PJcomplist    *_tcl;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmpuv, 0, sizeof(DBucdvar));
   INIT_OBJ(&tmp_obj);

   DEFINE_OBJ("cycle", &tmpuv.cycle, DB_INT);
   DEFINE_OBJ("time", &tmpuv.time, DB_FLOAT);
   DEFINE_OBJ("dtime", &tmpuv.dtime, DB_DOUBLE);
   DEFINE_OBJ("datatype", &tmpuv.datatype, DB_INT);
   DEFINE_OBJ("centering", &tmpuv.centering, DB_INT);
   DEFINE_OBJ("ndims", &tmpuv.ndims, DB_INT);
   DEFINE_OBJ("nels", &tmpuv.nels, DB_INT);
   DEFINE_OBJ("nvals", &tmpuv.nvals, DB_INT);
   DEFINE_OBJ("origin", &tmpuv.origin, DB_INT);
   DEFINE_OBJ("mixlen", &tmpuv.mixlen, DB_INT);
   DEFINE_OBJ("use_specmf", &tmpuv.use_specmf, DB_INT);
   DEFINE_OBJ("ascii_labels", &tmpuv.ascii_labels, DB_INT);

   DEFALL_OBJ("label", &tmpuv.label, DB_CHAR);
   DEFALL_OBJ("units", &tmpuv.units, DB_CHAR);
   DEFALL_OBJ("meshid",&tmpuv.meshname, DB_CHAR);
   DEFINE_OBJ("guihide", &tmpuv.guihide, DB_INT);
   DEFALL_OBJ("region_pnames", &rpnames, DB_CHAR);
   DEFINE_OBJ("conserved", &tmpuv.conserved, DB_INT);
   DEFINE_OBJ("extensive", &tmpuv.extensive, DB_INT);
   DEFINE_OBJ("missing_value", &tmpuv.missing_value, DB_DOUBLE);

   if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_UCDVAR) < 0)
      return NULL;
   if ((uv = DBAllocUcdvar()) == NULL)
      return NULL;
   *uv = tmpuv;

   /*
    *  Read the remainder of the object: loop over all values
    *  associated with this variable.
    */
   if ((uv->nvals > 0) && (DBGetDataReadMask2File(_dbfile) & DBUVData)) {
      INIT_OBJ(&tmp_obj);

      uv->vals = ALLOC_N(void*, uv->nvals);

      if (uv->mixlen > 0) {
         uv->mixvals = ALLOC_N(void*, uv->nvals);
      }

      if (uv->datatype == 0) {
          strcpy(tmp, objname);
          strcat(tmp, "_data");
          if ((uv->datatype = db_pdb_GetVarDatatype(dbfile->pdb, tmp)) < 0) {
              /* Not found. Assume float. */
              uv->datatype = DB_FLOAT;
          }
      }

      if (PJ_InqForceSingle())
          uv->datatype = DB_FLOAT;

      for (i = 0; i < uv->nvals; i++) {
         DEFALL_OBJ(_valstr[i], &uv->vals[i], DB_FLOAT);

         if (uv->mixlen > 0) {
            DEFALL_OBJ(_mixvalstr[i], &uv->mixvals[i], DB_FLOAT);
         }
      }

      PJ_GetObject(dbfile->pdb, objname, &tmp_obj, 0);
   }

   if (rpnames != NULL)
   {
      uv->region_pnames = DBStringListToStringArray(rpnames, 0, !skipFirstSemicolon);
      FREE(rpnames);
   }

   if      (uv->missing_value == DB_MISSING_VALUE_NOT_SET)
       uv->missing_value = 0.0;
   else if (uv->missing_value == 0.0)
       uv->missing_value = DB_MISSING_VALUE_NOT_SET;

   uv->id = 0;
   uv->name = STRDUP(objname);

   return (uv);
}

/*----------------------------------------------------------------------
 *  Routine                                                DBGetCsgmesh
 *
 *  Purpose
 *
 *      Read a CSG mesh structure from the given database.
 *
 *  Programmer
 *
 *      Mark C. Miller
 *      August 9, 2005
 *
 *  Modifications:
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBcsgmesh *
db_pdb_GetCsgmesh (DBfile *_dbfile, char const *meshname)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   DBcsgmesh     *csgm = NULL;
   char          *zlname = NULL, *tmpbndnames = NULL;
   PJcomplist     tmp_obj;
   static char *me = "db_pdb_GetCsgmesh";
   DBcsgmesh tmpcsgm;
   PJcomplist    *_tcl;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmpcsgm, 0, sizeof(DBcsgmesh));
   INIT_OBJ(&tmp_obj);

   DEFINE_OBJ("block_no", &tmpcsgm.block_no, DB_INT);
   DEFINE_OBJ("group_no", &tmpcsgm.group_no, DB_INT);
   DEFINE_OBJ("cycle", &tmpcsgm.cycle, DB_INT);
   DEFINE_OBJ("time", &tmpcsgm.time, DB_FLOAT);
   DEFINE_OBJ("dtime", &tmpcsgm.dtime, DB_DOUBLE);
   DEFINE_OBJ("lcoeffs", &tmpcsgm.lcoeffs, DB_INT);
   DEFINE_OBJ("datatype", &tmpcsgm.datatype, DB_INT);
   DEFINE_OBJ("ndims", &tmpcsgm.ndims, DB_INT);
   DEFINE_OBJ("nbounds", &tmpcsgm.nbounds, DB_INT);
   DEFINE_OBJ("origin", &tmpcsgm.origin, DB_INT);
   DEFINE_OBJ("min_extents", tmpcsgm.min_extents, DB_DOUBLE);
   DEFINE_OBJ("max_extents", tmpcsgm.max_extents, DB_DOUBLE);
   DEFALL_OBJ("label0", &tmpcsgm.labels[0], DB_CHAR);
   DEFALL_OBJ("label1", &tmpcsgm.labels[1], DB_CHAR);
   DEFALL_OBJ("label2", &tmpcsgm.labels[2], DB_CHAR);
   DEFALL_OBJ("units0", &tmpcsgm.units[0], DB_CHAR);
   DEFALL_OBJ("units1", &tmpcsgm.units[1], DB_CHAR);
   DEFALL_OBJ("units2", &tmpcsgm.units[2], DB_CHAR);
   DEFALL_OBJ("csgzonelist", &zlname, DB_CHAR);
   DEFINE_OBJ("guihide", &tmpcsgm.guihide, DB_INT);
   DEFALL_OBJ("mrgtree_name", &tmpcsgm.mrgtree_name, DB_CHAR);
   DEFINE_OBJ("tv_connectivity", &tmpcsgm.tv_connectivity, DB_INT);
   DEFINE_OBJ("disjoint_mode", &tmpcsgm.disjoint_mode, DB_INT);

   if (DBGetDataReadMask2File(_dbfile) & DBCSGMBoundaryInfo)
   {
       DEFALL_OBJ("typeflags", &tmpcsgm.typeflags, DB_INT);
       DEFALL_OBJ("bndids", &tmpcsgm.bndids, DB_INT);
   }

   /* Optional boundary names */
   if (DBGetDataReadMask2File(_dbfile) & DBCSGMBoundaryNames)
       DEFALL_OBJ("bndnames", &tmpbndnames, DB_CHAR);

   if (PJ_GetObject(dbfile->pdb, (char*) meshname, &tmp_obj, DB_CSGMESH) < 0)
      return NULL;

    /* now that we know the object's data type, we can correctly
       read the coeffs */
    if ((DBGetDataReadMask2File(_dbfile) & DBCSGMBoundaryInfo) && (tmpcsgm.lcoeffs > 0))
    {
        INIT_OBJ(&tmp_obj);
        if (DB_DOUBLE == tmpcsgm.datatype && PJ_InqForceSingle()) {
           tmpcsgm.datatype = DB_FLOAT;
        }

        DEFALL_OBJ("coeffs", &tmpcsgm.coeffs, tmpcsgm.datatype);
        PJ_GetObject(dbfile->pdb, (char*) meshname, &tmp_obj, 0);
    }

    if ((tmpbndnames != NULL) && (tmpcsgm.nbounds > 0))
    {
        tmpcsgm.bndnames = DBStringListToStringArray(tmpbndnames, &tmpcsgm.nbounds, !skipFirstSemicolon);
        FREE(tmpbndnames);
    }

   tmpcsgm.name = STRDUP(meshname);

   if ((tmpcsgm.nbounds > 0 && zlname && *zlname &&
        (DBGetDataReadMask2File(_dbfile) & DBCSGMZonelist)))
      tmpcsgm.zones = db_pdb_GetCSGZonelist(_dbfile, zlname);
  
   if ((csgm = DBAllocCsgmesh()) == NULL)
      return NULL;
   *csgm = tmpcsgm;

   FREE (zlname);

   return (csgm);

}

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_GetCsgvar
 *
 *  Purpose
 *
 *      Read a csg-var object from a SILO file and return the
 *      SILO structure for this type.
 *
 *  Programmer
 *
 *      Mark C. Miller, August 10, 2005
 *
 *  Modifications:
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBcsgvar *
db_pdb_GetCsgvar (DBfile *_dbfile, char const *objname)
{
   DBcsgvar      *csgv = NULL;
   int            i;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   char           tmp[256];
   static char *me = "db_pdb_GetCsgvar";
   char          *rpnames = NULL;
   DBcsgvar tmpcsgv;
   PJcomplist    *_tcl;

   memset(&tmpcsgv, 0, sizeof(DBcsgvar));
   INIT_OBJ(&tmp_obj);

   DEFINE_OBJ("cycle", &tmpcsgv.cycle, DB_INT);
   DEFINE_OBJ("time", &tmpcsgv.time, DB_FLOAT);
   DEFINE_OBJ("dtime", &tmpcsgv.dtime, DB_DOUBLE);
   DEFINE_OBJ("datatype", &tmpcsgv.datatype, DB_INT);
   DEFINE_OBJ("centering", &tmpcsgv.centering, DB_INT);
   DEFINE_OBJ("nels", &tmpcsgv.nels, DB_INT);
   DEFINE_OBJ("nvals", &tmpcsgv.nvals, DB_INT);
   DEFINE_OBJ("ascii_labels", &tmpcsgv.ascii_labels, DB_INT);
   DEFALL_OBJ("label", &tmpcsgv.label, DB_CHAR);
   DEFALL_OBJ("units", &tmpcsgv.units, DB_CHAR);
   DEFALL_OBJ("meshid",&tmpcsgv.meshname, DB_CHAR);
   DEFINE_OBJ("guihide", &tmpcsgv.guihide, DB_INT);
   DEFALL_OBJ("region_pnames", &rpnames, DB_CHAR);
   DEFINE_OBJ("missing_value", &tmpcsgv.missing_value, DB_DOUBLE);

   if (PJ_GetObject(dbfile->pdb, (char*) objname, &tmp_obj, DB_CSGVAR) < 0)
      return NULL;

   /*
    *  Read the remainder of the object: loop over all values
    *  associated with this variable.
    */
   if ((tmpcsgv.nvals > 0) && (DBGetDataReadMask2File(_dbfile) & DBCSGVData)) {
      INIT_OBJ(&tmp_obj);

      tmpcsgv.vals = ALLOC_N(void *, tmpcsgv.nvals);

      if (tmpcsgv.datatype == 0) {
          strcpy(tmp, objname);
          strcat(tmp, "_data");
          if ((tmpcsgv.datatype = db_pdb_GetVarDatatype(dbfile->pdb, tmp)) < 0) {
              /* Not found. Assume float. */
              tmpcsgv.datatype = DB_FLOAT;
          }
      }

      if ((tmpcsgv.datatype == DB_DOUBLE) && PJ_InqForceSingle())
          tmpcsgv.datatype = DB_FLOAT;

      for (i = 0; i < tmpcsgv.nvals; i++) {
         DEFALL_OBJ(_valstr[i], &tmpcsgv.vals[i], tmpcsgv.datatype);
      }

      PJ_GetObject(dbfile->pdb, (char*) objname, &tmp_obj, 0);
   }

   if (rpnames != NULL)
   {
      tmpcsgv.region_pnames = DBStringListToStringArray(rpnames, 0, !skipFirstSemicolon);
      FREE(rpnames);
   }

   if      (tmpcsgv.missing_value == DB_MISSING_VALUE_NOT_SET)
       tmpcsgv.missing_value = 0.0;
   else if (tmpcsgv.missing_value == 0.0)
       tmpcsgv.missing_value = DB_MISSING_VALUE_NOT_SET;

   if ((csgv = DBAllocCsgvar()) == NULL)
      return NULL;

   tmpcsgv.name = STRDUP(objname);
   *csgv = tmpcsgv;

   return (csgv);
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetFacelist
 *
 * Purpose:     Reads a facelist object from a SILO file.
 *
 * Return:      Success:        A pointer to a new facelist structure.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, January 14, 2000
 *
 * Modifications:
 *
 *      Sean Ahern, Wed Jun 14 17:24:58 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBfacelist *
db_pdb_GetFacelist(DBfile *_dbfile, char const *objname)
{
    DBfacelist          *fl=NULL;
    PJcomplist          tmp_obj;
    DBfile_pdb          *dbfile = (DBfile_pdb*)_dbfile;
    static char *me = "db_pdb_GetFacelist";
    DBfacelist           tmpfl;
    PJcomplist         *_tcl;

    /*------------------------------------------------------------*/
    /*          Comp. Name        Comp. Address     Data Type     */
    /*------------------------------------------------------------*/
    memset(&tmpfl, 0, sizeof(DBfacelist));
    INIT_OBJ(&tmp_obj);

    DEFINE_OBJ("ndims", &tmpfl.ndims, DB_INT);
    DEFINE_OBJ("nfaces", &tmpfl.nfaces, DB_INT);
    DEFINE_OBJ("origin", &tmpfl.origin, DB_INT);
    DEFINE_OBJ("lnodelist", &tmpfl.lnodelist, DB_INT);
    DEFINE_OBJ("nshapes", &tmpfl.nshapes, DB_INT);
    DEFINE_OBJ("ntypes", &tmpfl.ntypes, DB_INT);

    if (DBGetDataReadMask2File(_dbfile) & DBFacelistInfo)
    {
        DEFALL_OBJ("nodelist", &tmpfl.nodelist, DB_INT);
        DEFALL_OBJ("shapecnt", &tmpfl.shapecnt, DB_INT);
        DEFALL_OBJ("shapesize", &tmpfl.shapesize, DB_INT);
        DEFALL_OBJ("typelist", &tmpfl.typelist, DB_INT);
        DEFALL_OBJ("types", &tmpfl.types, DB_INT);
        DEFALL_OBJ("nodeno", &tmpfl.nodeno, DB_INT);
        DEFALL_OBJ("zoneno", &tmpfl.zoneno, DB_INT);
    }

    if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_FACELIST) < 0)
       return NULL;
    if ((fl = DBAllocFacelist()) == NULL)
       return NULL;
    *fl = tmpfl;

    return fl;
}


/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetZonelist
 *
 * Purpose:     Reads a zonelist object from a SILO file.
 *
 * Return:      Success:        A pointer to a new facelist structure.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, January 14, 2000
 *
 * Modifications:
 *
 *      Sean Ahern, Wed Jun 14 17:25:16 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *      Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *      Add support for long long global node/zone numbers.
 *
 *      Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *      Changed how long long global node/zone numbers are supported
 *      from a int (bool), "llong_gnode|zoneno" to an int holding
 *      the actual datatype. The type is assumed int if it its
 *      value is zero or it does not exist. Otherwise, the type is
 *      is whatever is stored in gnznodtype member. 
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBzonelist *
db_pdb_GetZonelist(DBfile *_dbfile, char const *objname)
{
    DBzonelist          *zl = NULL;
    PJcomplist          tmp_obj;
    DBfile_pdb          *dbfile = (DBfile_pdb*)_dbfile;
    static char         *me = "db_pdb_GetZonelist";
    DBzonelist           tmpzl;
    PJcomplist          *_tcl;
    char                *tmpaznum = 0;

    /*------------------------------------------------------------*/
    /*          Comp. Name        Comp. Address     Data Type     */
    /*------------------------------------------------------------*/
    memset(&tmpzl, 0, sizeof(DBzonelist));
    INIT_OBJ(&tmp_obj);

    DEFINE_OBJ("ndims", &tmpzl.ndims, DB_INT);
    DEFINE_OBJ("nzones", &tmpzl.nzones, DB_INT);
    DEFINE_OBJ("origin", &tmpzl.origin, DB_INT);
    DEFINE_OBJ("lnodelist", &tmpzl.lnodelist, DB_INT);
    DEFINE_OBJ("nshapes", &tmpzl.nshapes, DB_INT);
    DEFINE_OBJ("min_index", &tmpzl.min_index, DB_INT);
    DEFINE_OBJ("max_index", &tmpzl.max_index, DB_INT);
    DEFINE_OBJ("gnznodtype", &tmpzl.gnznodtype, DB_INT);

    if (DBGetDataReadMask2File(_dbfile) & DBZonelistInfo)
    {
        DEFALL_OBJ("shapecnt", &tmpzl.shapecnt, DB_INT);
        DEFALL_OBJ("shapesize", &tmpzl.shapesize, DB_INT);
        DEFALL_OBJ("shapetype", &tmpzl.shapetype, DB_INT);
        DEFALL_OBJ("nodelist", &tmpzl.nodelist, DB_INT);
        DEFALL_OBJ("zoneno", &tmpzl.zoneno, DB_INT);
    }

    if (DBGetDataReadMask2File(_dbfile) & DBZonelistGhostZoneLabels)
        DEFALL_OBJ("ghost_zone_labels", &tmpzl.ghost_zone_labels, DB_CHAR);
    DEFALL_OBJ("alt_zonenum_vars", &tmpaznum, DB_CHAR);

    if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_ZONELIST) < 0)
       return NULL;
    if ((zl = DBAllocZonelist()) == NULL)
       return NULL;
    *zl = tmpzl;

    if (tmpaznum)
    {
        zl->alt_zonenum_vars = DBStringListToStringArray(tmpaznum, 0, !skipFirstSemicolon);
        FREE(tmpaznum);
    }

    /* optional global zone numbers */
    zl->gnznodtype = zl->gnznodtype?zl->gnznodtype:DB_INT;
    if (DBGetDataReadMask2File(_dbfile) & DBZonelistGlobZoneNo) {
       INIT_OBJ(&tmp_obj);
       DEFALL_OBJ("gzoneno", &tmpzl.gzoneno, zl->gnznodtype);
       zl->gzoneno = 0;
       if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, 0)>=0)
           zl->gzoneno = tmpzl.gzoneno; 
    }

    return zl;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetPHZonelist
 *
 * Purpose:     Reads a DBphzonelist object from a SILO file.
 *
 * Return:      Success:        A pointer to a new facelist structure.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, January 14, 2000
 *
 * Modifications:
 *
 *      Sean Ahern, Wed Jun 14 17:25:16 PDT 2000
 *      Added a check to make sure the object is the right type.
 *
 *      Sean Ahern, Thu Mar  1 12:28:07 PST 2001
 *      Added support for the dataReadMask stuff.
 *
 *      Mark C. Miller, Wed Feb  2 07:59:53 PST 2005
 *      Moved DBAlloc call to after PJ_GetObject. Added automatic
 *      var for PJ_GetObject to read into. Added check for return
 *      value of PJ_GetObject.
 *
 *      Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *      Add support for long long global node/zone numbers.
 *
 *      Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *      Changed how long long global node/zone numbers are supported
 *      from a int (bool), "llong_gnode|zoneno" to an int holding
 *      the actual datatype. The type is assumed int if it its
 *      value is zero or it does not exist. Otherwise, the type is
 *      is whatever is stored in gnznodtype member. 
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBphzonelist *
db_pdb_GetPHZonelist(DBfile *_dbfile, char const *objname)
{
    DBphzonelist          *phzl = NULL;
    PJcomplist          tmp_obj;
    DBfile_pdb          *dbfile = (DBfile_pdb*)_dbfile;
    static char         *me = "db_pdb_GetPHZonelist";
    DBphzonelist         tmpphzl;
    PJcomplist          *_tcl;
    char                *tmpaznum = 0;

    /*------------------------------------------------------------*/
    /*          Comp. Name        Comp. Address     Data Type     */
    /*------------------------------------------------------------*/
    memset(&tmpphzl, 0, sizeof(DBphzonelist));
    INIT_OBJ(&tmp_obj);

    DEFINE_OBJ("nfaces", &tmpphzl.nfaces, DB_INT);
    DEFINE_OBJ("lnodelist", &tmpphzl.lnodelist, DB_INT);
    DEFINE_OBJ("nzones", &tmpphzl.nzones, DB_INT);
    DEFINE_OBJ("lfacelist", &tmpphzl.lfacelist, DB_INT);
    DEFINE_OBJ("origin", &tmpphzl.origin, DB_INT);
    DEFINE_OBJ("lo_offset", &tmpphzl.lo_offset, DB_INT);
    DEFINE_OBJ("hi_offset", &tmpphzl.hi_offset, DB_INT);
    DEFINE_OBJ("gnznodtype", &tmpphzl.gnznodtype, DB_INT);

    if (DBGetDataReadMask2File(_dbfile) & DBZonelistInfo)
    {
        DEFALL_OBJ("nodecnt", &tmpphzl.nodecnt, DB_INT);
        DEFALL_OBJ("nodelist", &tmpphzl.nodelist, DB_INT);
        DEFALL_OBJ("extface", &tmpphzl.extface, DB_CHAR);
        DEFALL_OBJ("facecnt", &tmpphzl.facecnt, DB_INT);
        DEFALL_OBJ("facelist", &tmpphzl.facelist, DB_INT);
        DEFALL_OBJ("zoneno", &tmpphzl.zoneno, DB_INT);
    }

    if (DBGetDataReadMask2File(_dbfile) & DBZonelistGhostZoneLabels)
       DEFALL_OBJ("ghost_zone_labels", &tmpphzl.ghost_zone_labels, DB_CHAR);
    DEFALL_OBJ("alt_zonenum_vars", &tmpaznum, DB_CHAR);

    if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, DB_PHZONELIST) < 0)
       return NULL;
    if ((phzl = DBAllocPHZonelist()) == NULL)
       return NULL;
    *phzl = tmpphzl;

    if (tmpaznum)
    {
        phzl->alt_zonenum_vars = DBStringListToStringArray(tmpaznum, 0, !skipFirstSemicolon);
        FREE(tmpaznum);
    }

    /* optional global zone numbers */
    phzl->gnznodtype = phzl->gnznodtype?phzl->gnznodtype:DB_INT;
    if (DBGetDataReadMask2File(_dbfile) & DBZonelistGlobZoneNo) {
       INIT_OBJ(&tmp_obj);
       DEFALL_OBJ("gzoneno", &tmpphzl.gzoneno, phzl->gnznodtype);
       phzl->gzoneno = 0;
       if (PJ_GetObject(dbfile->pdb, objname, &tmp_obj, 0)>=0)
           phzl->gzoneno = tmpphzl.gzoneno;
    }

    return phzl;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetCSGZonelist
 *
 * Purpose:     Reads a CSG zonelist object from a SILO file.
 *
 * Return:      Success:        A pointer to a new facelist structure.
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller 
 *              August 9, 2005 
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK DBcsgzonelist *
db_pdb_GetCSGZonelist(DBfile *_dbfile, char const *objname)
{
    char *tmprnames = 0, *tmpznames = 0, *tmpaznums = 0;
    DBcsgzonelist      *zl = NULL;
    PJcomplist          tmp_obj;
    DBfile_pdb         *dbfile = (DBfile_pdb*)_dbfile;
    static char        *me = "db_pdb_GetCSGZonelist";
    DBcsgzonelist       tmpzl;
    PJcomplist         *_tcl;

    /*------------------------------------------------------------*/
    /*          Comp. Name        Comp. Address     Data Type     */
    /*------------------------------------------------------------*/
    memset(&tmpzl, 0, sizeof(DBcsgzonelist));
    INIT_OBJ(&tmp_obj);

    DEFINE_OBJ("nregs", &tmpzl.nregs, DB_INT);
    DEFINE_OBJ("origin", &tmpzl.origin, DB_INT);
    DEFINE_OBJ("lxform", &tmpzl.lxform, DB_INT);
    DEFINE_OBJ("datatype", &tmpzl.datatype, DB_INT);
    DEFINE_OBJ("nzones", &tmpzl.nzones, DB_INT);
    DEFINE_OBJ("min_index", &tmpzl.min_index, DB_INT);
    DEFINE_OBJ("max_index", &tmpzl.max_index, DB_INT);

    if (DBGetDataReadMask2File(_dbfile) & DBZonelistInfo)
    {
        DEFALL_OBJ("typeflags", &tmpzl.typeflags, DB_INT);
        DEFALL_OBJ("leftids", &tmpzl.leftids, DB_INT);
        DEFALL_OBJ("rightids", &tmpzl.rightids, DB_INT);
        DEFALL_OBJ("zonelist", &tmpzl.zonelist, DB_INT);
    }
    if (DBGetDataReadMask2File(_dbfile) & DBCSGZonelistRegNames)
        DEFALL_OBJ("regnames", &tmprnames, DB_CHAR);
    if (DBGetDataReadMask2File(_dbfile) & DBCSGZonelistZoneNames)
        DEFALL_OBJ("zonenames", &tmpznames, DB_CHAR);
    DEFALL_OBJ("alt_zonenum_vars", &tmpaznums, DB_CHAR);

    if (PJ_GetObject(dbfile->pdb, (char*) objname, &tmp_obj, DB_CSGZONELIST) < 0)
       return NULL;

    /* now that we know the object's data type, we can correctly
       read the xforms */
    if ((DBGetDataReadMask2File(_dbfile) & DBZonelistInfo) && (tmpzl.lxform > 0))
    {
        INIT_OBJ(&tmp_obj);
        if (DB_DOUBLE == tmpzl.datatype && PJ_InqForceSingle()) {
           tmpzl.datatype = DB_FLOAT;
        }

        DEFALL_OBJ("xform", &tmpzl.xform, tmpzl.datatype);
        PJ_GetObject(dbfile->pdb, (char*) objname, &tmp_obj, 0);
    }

    if ((tmprnames != NULL) && (tmpzl.nregs > 0))
    {
        tmpzl.regnames = DBStringListToStringArray(tmprnames, &tmpzl.nregs, !skipFirstSemicolon);
        FREE(tmprnames);
    }

    if ((tmpznames != NULL) && (tmpzl.nzones > 0))
    {
        tmpzl.zonenames = DBStringListToStringArray(tmpznames, &tmpzl.nzones, !skipFirstSemicolon);
        FREE(tmpznames);
    }

    if (tmpaznums != NULL)
    {
        tmpzl.alt_zonenum_vars = DBStringListToStringArray(tmpaznums, 0, !skipFirstSemicolon);
        FREE(tmpaznums);
    }

    if ((zl = DBAllocCSGZonelist()) == NULL)
       return NULL;
    *zl = tmpzl;

    return zl;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetVar
 *
 * Purpose:     Allocates space for a variable and reads the variable
 *              from the database.
 *
 * Return:      Success:        Pointer to variable data
 *
 *              Failure:        NULL
 *
 * Programmer:  robb@cloud
 *              Tue Nov 15 11:12:23 EST 1994
 *
 * Modifications:
 *     Sean Ahern, Sun Oct  1 03:13:19 PDT 1995
 *     Made "me" static.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK void *
db_pdb_GetVar (DBfile *_dbfile, char const *name)
{
   static char   *me = "db_pdb_GetVar";
   char          *data;
   int            n;

   /* Find out how long (in bytes) requested variable is. */
   if (0 == (n = DBGetVarByteLength(_dbfile, name))) {
      db_perror(name, E_NOTFOUND, me);
      return NULL;
   }

   /* Read it and return. On the off chance this is a string we're reading,
      we'll allocate 1 more than needed for the null terminator. */
   data = ALLOC_N(char, n+1);
   if (DBReadVar(_dbfile, name, data) < 0) {
      db_perror("DBReadVar", E_CALLFAIL, me);
      FREE(data);
      return (NULL);
   }

   return data;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetVarByteLength
 *
 * Purpose:     Returns the in-memory length of the given variable in bytes.
 *
 * Return:      Success:        length of variable in bytes
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Tue Nov 15 11:45:42 EST 1994
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_GetVarByteLength (DBfile *_dbfile, char const *varname)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   int            number, size;

   db_pdb_getvarinfo(dbfile->pdb, varname, NULL, &number, &size, 0);
   return number * size;
}


/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetVarDims
 *
 * Purpose:     Returns the size of each dimension.
 *
 * Return:      Success:        Number of dimensions not to exceed MAXDIMS.
 *                              The dimension sizes are written to DIMS.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Mar  6 1997
 *
 * Modifications:
 *      Sean Ahern, Wed Apr 12 11:14:38 PDT 2000
 *      Removed the last two parameters to PJ_inquire_entry because they
 *      weren't being used.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_GetVarDims (DBfile *_dbfile, char const *varname, int maxdims, int *dims)
{
   DBfile_pdb   *dbfile = (DBfile_pdb *) _dbfile;
   static char  *me = "db_pdb_GetVarDims";
   syment       *ep;
   dimdes       *dd;
   int          i;

   ep = PJ_inquire_entry (dbfile->pdb, varname);
   if (!ep) return db_perror ("PJ_inquire_entry", E_CALLFAIL, me);

   for (i=0, dd=ep->dimensions; i<maxdims && dd; i++, dd=dd->next) {
      dims[i] = dd->number;
   }
   return i;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetVarType
 *
 * Purpose:     Returns the data type ID (DB_INT, DB_FLOAT, etc) of the
 *              variable.
 *
 * Return:      Success:        type ID
 *
 *              Failure:        -1
 *
 * Programmer:  matzke@viper
 *              Thu Dec 22 09:00:39 PST 1994
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_GetVarType (DBfile *_dbfile, char const *varname)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   int            number, size;
   char           type_name[256];

   db_pdb_getvarinfo(dbfile->pdb, varname, type_name, &number, &size, 0);
   return db_GetDatatypeID(type_name);
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetVarLength
 *
 * Purpose:     Returns the number of elements in the requested variable.
 *
 * Return:      Success:        number of elements
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Tue Nov 15 12:06:36 EST 1994
 *
 * Modifications:
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_GetVarLength (DBfile *_dbfile, char const *varname)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   int            number, size;

   db_pdb_getvarinfo(dbfile->pdb, varname, NULL, &number, &size, 0);
   return number;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_InqMeshname
 *
 * Purpose:     Returns the name of a mesh associated with a mesh-variable.
 *              Caller must allocate space for mesh name.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Tue Nov 15 12:10:59 EST 1994
 *
 * Modifications:
 *      Katherine Price, Fri Jun  2 08:58:54 PDT 1995
 *      Added error return code.
 *
 *      Sean Ahern, Mon Jun 24 13:30:53 PDT 1996
 *      Fixed a memory leak.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_InqMeshname (DBfile *_dbfile, char const *vname, char *mname)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   void          *v;

   if ((v = PJ_GetComponent(dbfile->pdb, vname, "meshid"))) {
      if (mname)
         strcpy(mname, (char *)v);
      FREE(v);
      return 0;
   }
   FREE(v);
   return -1;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_InqMeshtype
 *
 * Purpose:     returns the mesh type.
 *
 * Return:      Success:        mesh type
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Tue Nov 15 12:16:12 EST 1994
 *
 * Modifications:
 *    Eric Brugger, Thu Feb  9 12:54:37 PST 1995
 *    I modified the routine to read elements of the group structure
 *    as "abc->type" instead of "abc.type".
 *
 *    Sean Ahern, Sun Oct  1 03:13:44 PDT 1995
 *    Made "me" static.
 *
 *    Eric Brugger, Fri Dec  4 12:45:22 PST 1998
 *    Added code to free ctype to eliminate a memory leak.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_InqMeshtype (DBfile *_dbfile, char const *mname)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   char           tmp[256], *ctype;
   int            type;
   static char   *me = "db_pdb_InqMeshtype";

   sprintf(tmp, "%s->type", mname);
   if (!PJ_read(dbfile->pdb, tmp, &ctype)) {
      return db_perror("PJ_read", E_CALLFAIL, me);
   }

   type = DBGetObjtypeTag(ctype);
   SCFREE(ctype);

   return type;
}

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_InqVarExists
 *
 *  Purpose
 *      Check whether the variable exists and return non-zero if so,
 *      and 0 if not
 *
 *  Programmer
 *      Sean Ahern, Thu Jul 20 12:04:39 PDT 1995
 *
 *  Modifications
 *      Sean Ahern, Sun Oct  1 03:14:03 PDT 1995
 *      Made "me" static.
 *
 *      Sean Ahern, Wed Apr 12 11:14:38 PDT 2000
 *      Removed the last two parameters to PJ_inquire_entry because they
 *      weren't being used.
 *
 *      Mark C. Miller, Wed Dec  7 16:37:11 PST 2022
 *      Adjusted to behave similarly to InqVarType and, in particular,
 *      work also for names of directories.
 *--------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_InqVarExists (DBfile *_dbfile, char const *varname)
{
   char          *newname;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   syment        *ep;

    /* First, check to see if it exists */
    ep = lite_PD_inquire_entry(dbfile->pdb, (char *)varname, TRUE, NULL);
    if (ep) return 1;

    /* This could be a directory.  Add a "/" to the end and try again. */
    newname = (char*)malloc(strlen(varname)+2);
    sprintf(newname,"%s/",varname);
    ep = lite_PD_inquire_entry(dbfile->pdb, newname, TRUE, NULL);
    free(newname);

    /* If it's NULL now, we really cannot find the object. */
    if (ep) return 1;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_ReadVar
 *
 * Purpose:     Reads a variable into the given space.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Mon Nov 21 21:05:07 EST 1994
 *
 * Modifications:
 *     Sean Ahern, Sun Oct  1 03:18:45 PDT 1995
 *     Made "me" static.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_ReadVar (DBfile *_dbfile, char const *vname, void *result)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   static char   *me = "db_pdb_ReadVar";

   if (!PJ_read(dbfile->pdb, vname, result)) {
      return db_perror("PJ_read", E_CALLFAIL, me);
   }
   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_ReadVarSlice
 *
 * Purpose:     Reads a slice of a variable into the given space.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  brugger@sgibird
 *              Thu Feb 16 08:45:00 PST 1995
 *
 * Modifications:
 *     Sean Ahern, Sun Oct  1 03:19:03 PDT 1995
 *     Made "me" static.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_ReadVarSlice (DBfile *_dbfile, char const *vname, int const *offset, int const *length,
                     int const *stride, int ndims, void *result)
{
   int            i;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   long           ind[3 * MAXDIMS_VARWRITE];
   static char   *me = "db_pdb_ReadVarSlice";

   for (i = 0; i < ndims && i < MAXDIMS_VARWRITE; i++) {
      ind[3 * i] = offset[i];
      ind[3 * i + 1] = offset[i] + length[i] - 1;
      ind[3 * i + 2] = stride[i];
   }

   if (!PJ_read_alt(dbfile->pdb, (char*)vname, result, ind)) {
      return db_perror("PJ_read_alt", E_CALLFAIL, me);
   }
   return 0;
}

#define FSTRS1(A) ={#A}
#define FSTRS2(A,B) ={#A,#B}
#define FSTRS3(A,B,C) ={#A,#B,#C}
#define FSTRS4(A,B,C,D) ={#A,#B,#C,#D}
#define FSTRS5(A,B,C,D,E) ={#A,#B,#C,#D,#E}
#define DSNAMES_CASE(OTYPE, FMTSTRS) \
        case OTYPE: \
        { \
            int i, j, pass; \
            char const *fmtstrs[] FMTSTRS; \
            PJgroup *group=NULL; \
            if (!PJ_get_group(pdbfile, name, &group)) \
            { \
                PJ_rel_group(group); \
                return 0; \
            } \
            for (pass = 0; pass < 2; pass++) \
            { \
                if (pass == 1) \
                { \
                    _dsnames = (char **) malloc(n * sizeof(char*)); \
                    n = 0; \
                } \
                for (i=0; i<group->ncomponents; i++) \
                { \
                    for (j=0; j < sizeof(fmtstrs)/sizeof(fmtstrs[0]); j++) \
                    { \
                        char tmp[32]; \
                        snprintf(tmp, sizeof(tmp), fmtstrs[j], n); \
                        if (!strcmp(group->comp_names[i], tmp)) \
                        { \
                            if (pass == 1) \
                                _dsnames[n] = _db_safe_strdup(group->pdb_names[i]); \
                            n++; \
                        } \
                    } \
                } \
                if (n == 0) \
                { \
                    PJ_rel_group(group); \
                    return 0; \
                } \
            } \
            PJ_rel_group(group); \
            break; \
        }


PRIVATE int
db_pdb_get_obj_dsnames(DBfile *_dbfile, char const *name, DBObjectType *otype,
    int *dscount, char ***dsnames)
{
    int n = 0;
    char **_dsnames = 0;
    static char const *me = "db_pdb_get_obj_dsnames";
    DBObjectType _otype = db_pdb_InqVarType(_dbfile, name);
    PDBfile *pdbfile = ((DBfile_pdb *)_dbfile)->pdb;

    *otype = _otype;
    switch (_otype)
    {
        DSNAMES_CASE(DB_CURVE,           FSTRS2(xvals,yvals));
        DSNAMES_CASE(DB_QUADVAR,         FSTRS2(value%d,mixed_value%d));
        DSNAMES_CASE(DB_QUADMESH,        FSTRS1(coord%d));
        DSNAMES_CASE(DB_QUADCURV,        FSTRS1(coord%d));
        DSNAMES_CASE(DB_QUADRECT,        FSTRS1(coord%d));
        DSNAMES_CASE(DB_UCDVAR,          FSTRS1(value%d));
        DSNAMES_CASE(DB_UCDMESH,         FSTRS1(coord%d));
        DSNAMES_CASE(DB_POINTMESH,       FSTRS1(coord%d));
        DSNAMES_CASE(DB_POINTVAR,        FSTRS2(_data,%d_data));
        DSNAMES_CASE(DB_MULTIMESH,       FSTRS4(meshtypes,meshnames,extents,zonecounts));
        DSNAMES_CASE(DB_MULTIVAR,        FSTRS3(vartypes,varnames,extents));
        DSNAMES_CASE(DB_MULTIMAT,        FSTRS3(matnames,mixlens,matcounts));
        DSNAMES_CASE(DB_MULTIMATSPECIES, FSTRS1(specnames));
        DSNAMES_CASE(DB_MATERIAL,        FSTRS5(matlist,mix_next,mix_vf,mix_mat,mix_zone));
        DSNAMES_CASE(DB_MATSPECIES,      FSTRS4(speclist,species_mf,nmatspec,mix_speclist));
        DSNAMES_CASE(DB_ZONELIST,        FSTRS4(nodelist,shapecnt,shapesize,shapetype));
        DSNAMES_CASE(DB_FACELIST,        FSTRS4(nodelist,shapecnt,shapesize,zoneno));
        DSNAMES_CASE(DB_ARRAY,           FSTRS3(elemnames,elemlengths,values));
        case DB_VARIABLE:
        {
            n = 1;
            _dsnames = (char**) malloc(n*sizeof(char*));
            _dsnames[0] = _db_safe_strdup(name);
            break;
        }
    }
    *dscount = n;
    *dsnames = _dsnames;
    return 1;
}

PRIVATE int
db_pdb_ReadDenseArrayVals(DBfile *_dbfile, char const *vname, int objtype,
    int dscount, char const * const *dsnames, int nvals, int ndims, int const *indices,
    void **result, int *ncomps, int *nitems)
{
    static char const *me = "db_pdb_ReadDenseArrayVals";
    DBfile_pdb *dbfile = (DBfile_pdb *) _dbfile;
    int db_type, db_type_size;
    int i, j, k;
    char *p;

    db_type = db_pdb_GetVarType(_dbfile, dsnames[0]);
    if (db_type < 0)
        return db_perror(vname, E_CALLFAIL, me);
    db_type_size = db_GetMachDataSize(db_type);

    /* If result space isn't already allocated, allocate it */
    if (!*result)
    {
        *result = malloc(nvals*dscount*db_type_size);
        if (!*result)
            return db_perror(vname, E_NOMEM, me);
    }

    /* Loop to read the equivalent value(s) from each dataset */
    p = (char *) *result;
    for (j = 0; j < nvals; j++)
    {
        for (i = 0; i < dscount; i++)
        {
            if (objtype == DB_QUADRECT) /* really dscount sep. 1D arrays */
            {
                long ind[3];
                ind[0] = indices[j*ndims+i];
                ind[1] = indices[j*ndims+i];
                ind[2] = 1;
                if (!PJ_read_alt(dbfile->pdb, (char*)dsnames[i], p, ind))
                    return db_perror("PJ_read_alt", E_CALLFAIL, me);
            }
            else
            {
                long ind[3 * MAXDIMS_VARWRITE];
                for (k = 0; k < ndims && k < MAXDIMS_VARWRITE; k++)
                {
                    ind[3 * k    ] = indices[j*ndims+k];
                    ind[3 * k + 1] = indices[j*ndims+k];
                    ind[3 * k + 2] = 1;
                }
                if (!PJ_read_alt(dbfile->pdb, (char*)dsnames[i], p, ind))
                   return db_perror("PJ_read_alt", E_CALLFAIL, me);
            }
            p += db_type_size;
        }
    }
    if (ncomps) *ncomps = dscount;
    if (nitems) *nitems = nvals;
    return 0;
}

PRIVATE int
db_pdb_ReadMatVals(DBfile *_dbfile, char const *vname, int objtype,
    int dscount, char const * const *dsnames, int nvals, int ndims, int const *indices,
    void **result, int *ncomps, int *nitems)
{
    static char const *me = "db_pdb_ReadMatVals";
    DBfile_pdb *dbfile = (DBfile_pdb *) _dbfile;
    int i, have_mixed = 0;
    int *matlist_result = 0;
    DBmaterial *origmat = 0, *mat = 0;
    unsigned long long oldmask;

    /*        0       1        2      3       4           */
    /* FSTRS5(matlist,mix_next,mix_vf,mix_mat,mix_zone)); */
    oldmask = DBSetDataReadMask2File(_dbfile, DBNone);
    origmat = db_pdb_GetMaterial(_dbfile, vname);
    DBSetDataReadMask2File(_dbfile, oldmask);

    /* Do partial I/O on matlist */
    if (db_pdb_ReadDenseArrayVals(_dbfile, vname, objtype, 1,
            (char const * const *) &dsnames[0],
            nvals, ndims, indices, (void**) &matlist_result, 0, nitems) < 0)
        return db_perror(dsnames[0], E_CALLFAIL, me);

    if (!*result)
    {
        *result = DBAllocMaterial();
        if (!*result)
            return db_perror(vname, E_NOMEM, me);
    }
    mat = *result;

    mat->matlist = matlist_result;
    mat->ndims = 1;
    mat->dims[0] = nvals;

    /* If none are negative values, we're done */
    for (i = 0; i < *nitems && !have_mixed; i++)
        have_mixed = matlist_result[i] < 0;
    if (!have_mixed)
    {
        if (ncomps) *ncomps = 1;
        if (nitems) *nitems = 1;
        return 0;
    }

    /* For negative matlist entries, need to follow the links for mix_next, mix_mat, mix_vf */
    for (i = 0; i < *nitems; i++)
    {
        long ind[3];
        int mixidx;
        int mix_next, mix_mat, mix_zone;
        double const dbl_max = DBL_MAX;
        int const halfn = (int const) sizeof(double)/2;
        double mix_vf = dbl_max;
        float *fp = (float*) &mix_vf;
        void const *halfa = (void*) &mix_vf;
        void const *halfb = (void*) ((char*) &mix_vf + halfn);
        void const *halfc = (void*) &dbl_max;
        void const *halfd = (void*) ((char*) &dbl_max + halfn);

        if (mat->matlist[i] >= 0) continue;

        mixidx = -(mat->matlist[i]);

        while (mixidx != 0)
        {
            ind[0] = mixidx-1; ind[1] = mixidx-1; ind[2] = 1;
            PJ_read_alt(dbfile->pdb, (char*)dsnames[2], &mix_next, ind);
            PJ_read_alt(dbfile->pdb, (char*)dsnames[1], &mix_vf, ind);
            if (memcmp(halfa,halfc,halfn) == 0 || memcmp(halfb,halfd,halfn) == 0)
            {
                printf("looks like float, *fp=%f\n",*fp);
                mix_vf = (double) *fp;
            }
            PJ_read_alt(dbfile->pdb, (char*)dsnames[3], &mix_mat, ind);
            mixidx = mix_next;
            printf("got mat=%d, vf = %f, mixidx=%d\n", mix_mat, mix_vf, mixidx);
            mix_vf = dbl_max;
#if 0
        if (dscount > 4)
            PJ_read_alt(dbfile->pdb, (char*)dsnames[4], &mix_zone, ind);
#endif
        }
    }

    return db_perror(vname, E_CALLFAIL, me);
}

SILO_CALLBACK int
db_pdb_ReadVarVals(DBfile *_dbfile, char const *vname, int mode,
    int nvals, int ndims, void const *_indices, void **result,
    int *ncomps, int *nitems)
{
    DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
    static char const *me = "db_pdb_ReadVarVals";
    DBObjectType objtype;
    int dscount = 0;
    char **dsnames = 0;
    int db_type, db_type_size;
    int i, j, k;
    char *p;
    int const *indices = (int const *) _indices;

    if (db_pdb_get_obj_dsnames(_dbfile, vname, &objtype, &dscount, &dsnames) < 0)
        return db_perror(vname, E_CALLFAIL, me);

    if (dscount <= 0)
        return db_perror(vname, E_CALLFAIL, me);

    /* There are two basic cases here. One is a simple (dense) array.
       The other is some non-dense "object" (e.g. the coordinates of
       a rectilinear mesh, a zonelist, a material, etc.) These latter
       objects cannot be handled by PDB's simple partial I/O interface
       and require special handling. All the special cases we handle
       in the switch. */
    switch (objtype)
    {
        case DB_MATERIAL: return db_pdb_ReadMatVals(_dbfile, vname, objtype, dscount,
                                     (char const * const *) dsnames,
                                     nvals, ndims, indices, result, ncomps, nitems);
#if 0
        case DB_ZONELIST: return db_pdb_ReadZonelistVals(_dbfile, vname, dscount, dsnames,
                                     nvals, ndims, indices, result, ncomps, nitems);
#endif
        default:     return db_pdb_ReadDenseArrayVals(_dbfile, vname, objtype, dscount,
                            (char const * const *) dsnames, nvals, ndims, indices,
                            result, ncomps, nitems);
    }

    return db_perror(vname, E_CALLFAIL, me);
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_SetDir
 *
 * Purpose:     Sets the current directory within the database.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Mon Nov 21 21:10:37 EST 1994
 *
 * Modifications:
 *    Eric Brugger, Fri Jan 27 08:27:46 PST 1995
 *    I changed the call DBGetToc to db_pdb_GetToc.
 *
 *    Robb Matzke, Fri Feb 24 10:15:54 EST 1995
 *    The directory ID (pub.dirid) is no longer set since PJ_pwd_id()
 *    has gone away.
 *
 *    Robb Matzke, Tue Mar 7 10:40:02 EST 1995
 *    I changed the call db_pdb_GetToc to DBNewToc.
 *
 *    Sean Ahern, Sun Oct  1 03:19:19 PDT 1995
 *    Made "me" static.
 *
 *    Sean Ahern, Mon Jul  1 14:06:08 PDT 1996
 *    Turned off the PJgroup cache when we change directories.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_SetDir (DBfile *_dbfile, char const *path)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   static char   *me = "db_pdb_SetDir";
   char           error_message[256];

   if (1 == lite_PD_cd(dbfile->pdb, (char*) path)) {
      dbfile->pub.dirid = 0;

      /* We've changed directories, so we can't cache PJgroups any more */
      PJ_NoCache();

      /* Must make new table-of-contents since dir has changed */
      db_FreeToc(_dbfile);
   }
   else {
      sprintf(error_message,"\"%s\" ***%s***",path,lite_PD_err);
      return db_perror(error_message, E_NOTDIR, me);
   }

   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_pdb_Filters
 *
 * Purpose:     Output the name of this device driver to the specified
 *              stream.
 *
 * Return:      Success:        0
 *
 *              Failure:        never fails
 *
 * Programmer:  robb@cloud
 *              Tue Mar  7 11:11:59 EST 1995
 *
 * Modifications:
 *
 *    Hank Childs, Thu Jan  6 13:48:40 PST 2000
 *    Put in lint directive for unused arguments.
 *-------------------------------------------------------------------------*/
/* ARGSUSED */
SILO_CALLBACK int
db_pdb_Filters (DBfile *dbfile, FILE *stream)
{
   fprintf(stream, "PDB Device Driver\n");
   return 0;
}


/*-------------------------------------------------------------------------
 * Function:    db_pdb_GetComponentNames
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
 *
 *    Lisa J. Roberts, Tue Nov 23 09:39:49 PST 1999
 *    Changed strdup to safe_strdup.
 *-------------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_GetComponentNames (DBfile *_dbfile, char const *objname,
                          char ***comp_names, char ***file_names)
{
   PJgroup      *group = NULL ;
   DBfile_pdb   *dbfile = (DBfile_pdb *) _dbfile ;
   int          i ;
   syment       *ep ;

   if (comp_names) *comp_names = NULL ;
   if (file_names) *file_names = NULL ;

   /*
    * First make sure that the specified object is type "Group *", otherwise
    * we'll get a segmentation fault deep within the PDB library!
    */
   ep = lite_PD_inquire_entry (dbfile->pdb, (char*)objname, TRUE, NULL) ;
   if (!ep || strcmp(ep->type, "Group *")) return 0 ;

   /*
    * OK, now we can go ahead and get the group, but watch out
    * for the empty group.
    */
   if (!PJ_get_group (dbfile->pdb, objname, &group)) return 0 ;
   if (!group) return 0 ;
   if (group->ncomponents<=0) return 0 ;

   /*
    * Copy the group component names and pdb names into the
    * output arrays after allocating them.
    */
   if (comp_names)
       *comp_names = (char**)malloc(group->ncomponents * sizeof(char *)) ;
   if (file_names)
       *file_names = (char**)malloc(group->ncomponents * sizeof(char *)) ;
   for (i=0; i<group->ncomponents; i++) {
      if (comp_names) (*comp_names)[i] = _db_safe_strdup (group->comp_names[i]) ;
      if (file_names) (*file_names)[i] = _db_safe_strdup (group->pdb_names[i]) ;
   }

   return group->ncomponents ;
}

/*----------------------------------------------------------------------
 *  Routine                                                 DBGetMrgtree
 *
 *  Purpose
 *
 *      Read mrg tree structure from the given database.
 *
 *  Programmer
 *
 *      Mark C. Miller, Wed Oct 10 13:08:36 PDT 2007
 *
 *  Modifications:
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBmrgtree *
db_pdb_GetMrgtree(DBfile *_dbfile, char const *mrgtree_name)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   DBmrgtree     *tree = NULL;
   PJcomplist     tmp_obj;
   static char   *me = "db_pdb_GetMrgtree";
   DBmrgtree      tmptree;
   DBmrgtnode   **ltree;
   int            root, num_nodes, i, j, n;
   int           *intArray = 0;
   char          *s, **strArray = 0;
   char          *mrgv_onames = 0, *mrgv_rnames = 0;
   PJcomplist    *_tcl;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmptree, 0, sizeof(DBmrgtree));
   INIT_OBJ(&tmp_obj);

   /* read header and scalars data only first */
   DEFINE_OBJ("src_mesh_type", &tmptree.src_mesh_type, DB_INT);
   DEFINE_OBJ("type_info_bits", &tmptree.type_info_bits, DB_INT);
   DEFINE_OBJ("num_nodes", &tmptree.num_nodes, DB_INT);
   DEFINE_OBJ("root", &root, DB_FLOAT);
   DEFALL_OBJ("src_mesh_name", &tmptree.src_mesh_name, DB_CHAR);
   DEFALL_OBJ("scalars", &intArray, DB_INT);
   DEFALL_OBJ("mrgvar_onames", &mrgv_onames, DB_CHAR);
   DEFALL_OBJ("mrgvar_rnames", &mrgv_rnames, DB_CHAR);

   if (PJ_GetObject(dbfile->pdb, (char*)mrgtree_name, &tmp_obj, DB_MRGTREE) < 0)
      return NULL;

   tree = (DBmrgtree *) calloc(1,sizeof(DBmrgtree));
   memset(tree, 0, sizeof(DBmrgtree));
   *tree = tmptree;
   tree->root = 0;
   tree->cwr = 0;
   num_nodes = tree->num_nodes;

   if (num_nodes <= 0) return (tree);

   /* allocate all the nodes of the tree and a linear list of pointers
   to them */
   ltree = (DBmrgtnode **) malloc(num_nodes * sizeof(DBmrgtnode*));
   for (i = 0; i < num_nodes; i++)
       ltree[i] = (DBmrgtnode *) calloc(1,sizeof(DBmrgtnode));

   /* Read the nodal scalar data */
   for (i = 0; i < num_nodes; i++)
   {
       ltree[i]->narray           = intArray[i*6+0];
       ltree[i]->type_info_bits   = intArray[i*6+1];
       ltree[i]->max_children     = intArray[i*6+2];
       ltree[i]->nsegs            = intArray[i*6+3];
       ltree[i]->num_children     = intArray[i*6+4];
       ltree[i]->parent           = intArray[i*6+5] >= 0 ? ltree[intArray[i*6+5]] : 0;
   }
   FREE(intArray);

   /* read the node 'name' member */
   INIT_OBJ(&tmp_obj);
   DEFALL_OBJ("name", &s, DB_CHAR);
   PJ_GetObject(dbfile->pdb, (char*)mrgtree_name, &tmp_obj, 0);
   if (s)
   {
       strArray = DBStringListToStringArray(s, &num_nodes, !skipFirstSemicolon);
       for (i = 0; i < num_nodes; i++)
           ltree[i]->name = strArray[i];
       FREE(s);
       FREE(strArray); /* free only top-level array of pointers */
   }

   /* read the node 'names' member */
   INIT_OBJ(&tmp_obj);
   DEFALL_OBJ("names", &s, DB_CHAR);
   PJ_GetObject(dbfile->pdb, (char*)mrgtree_name, &tmp_obj, 0);
   if (s)
   {
       strArray = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
       n = 0;
       for (i = 0; i < num_nodes; i++)
       {
           if (ltree[i]->narray == 0)
               continue;

           if (strchr(strArray[n], '%') == 0)
           {
               ltree[i]->names = (char**) malloc(ltree[i]->narray * sizeof(char*));
               for (j = 0; j < ltree[i]->narray; j++, n++)
                   ltree[i]->names[j] = strArray[n];
           }
           else
           {
               ltree[i]->names = (char**) malloc(1 * sizeof(char*));
               ltree[i]->names[0] = strArray[n];
               n++;
           }
       }
       FREE(s);
       FREE(strArray); /* free only top-level array of pointers */
   }

   /* read the maps_name data */
   INIT_OBJ(&tmp_obj);
   DEFALL_OBJ("maps_name", &s, DB_CHAR);
   PJ_GetObject(dbfile->pdb, (char*)mrgtree_name, &tmp_obj, 0);
   if (s)
   {
       strArray = DBStringListToStringArray(s, &num_nodes, !skipFirstSemicolon);
       for (i = 0; i < num_nodes; i++)
           ltree[i]->maps_name = strArray[i];
       FREE(s);
       FREE(strArray); /* free only top-level array of pointers */
   }

   /* read the map segment id data */
   INIT_OBJ(&tmp_obj);
   DEFALL_OBJ("seg_ids", &intArray, DB_INT);
   PJ_GetObject(dbfile->pdb, (char*)mrgtree_name, &tmp_obj, 0);
   n = 0;
   for (i = 0; i < num_nodes; i++)
   {
       int ns = ltree[i]->nsegs*(ltree[i]->narray?ltree[i]->narray:1);
       if (ns > 0)
       {
           ltree[i]->seg_ids = (int*) malloc(ns * sizeof(int));
           for (j = 0; j < ns; j++)
               ltree[i]->seg_ids[j] = intArray[n++];
       }
   }
   FREE(intArray);

   /* read the map segment len data */
   INIT_OBJ(&tmp_obj);
   DEFALL_OBJ("seg_lens", &intArray, DB_INT);
   PJ_GetObject(dbfile->pdb, (char*)mrgtree_name, &tmp_obj, 0);
   n = 0;
   for (i = 0; i < num_nodes; i++)
   {
       int ns = ltree[i]->nsegs*(ltree[i]->narray?ltree[i]->narray:1);
       if (ns > 0)
       {
           ltree[i]->seg_lens = (int*) malloc(ns * sizeof(int));
           for (j = 0; j < ns; j++)
               ltree[i]->seg_lens[j] = intArray[n++];
       }
   }
   FREE(intArray);

   /* read the map segment type data */
   INIT_OBJ(&tmp_obj);
   DEFALL_OBJ("seg_types", &intArray, DB_INT);
   PJ_GetObject(dbfile->pdb, (char*)mrgtree_name, &tmp_obj, 0);
   n = 0;
   for (i = 0; i < num_nodes; i++)
   {
       int ns = ltree[i]->nsegs*(ltree[i]->narray?ltree[i]->narray:1);
       if (ns > 0)
       {
           ltree[i]->seg_types = (int*) malloc(ns * sizeof(int));
           for (j = 0; j < ns; j++)
               ltree[i]->seg_types[j] = intArray[n++];
       }
   }
   FREE(intArray);

   /* read the child ids */
   INIT_OBJ(&tmp_obj);
   DEFALL_OBJ("children", &intArray, DB_INT);
   PJ_GetObject(dbfile->pdb, (char*)mrgtree_name, &tmp_obj, 0);
   n = 0;
   for (i = 0; i < num_nodes; i++)
   {
       int nc = ltree[i]->num_children;
       if (nc > 0)
       {
           ltree[i]->children = (DBmrgtnode**) malloc(nc * sizeof(DBmrgtnode*));
           for (j = 0; j < nc; j++)
               ltree[i]->children[j] = ltree[intArray[n++]];
       }
   }
   FREE(intArray);

   if (mrgv_onames)
   {
      tree->mrgvar_onames = DBStringListToStringArray(mrgv_onames, 0, !skipFirstSemicolon);
      FREE(mrgv_onames);
   }
   if (mrgv_rnames)
   {
      tree->mrgvar_rnames = DBStringListToStringArray(mrgv_rnames, 0, !skipFirstSemicolon);
      FREE(mrgv_rnames);
   }

   tree->root = ltree[root];
   tree->cwr = tree->root;

   FREE(ltree);

   return (tree);

}

/*----------------------------------------------------------------------
 *  Routine                                              DBGetGroupelmap
 *
 *  Purpose
 *
 *      Read groupel map object from the given database.
 *
 *  Programmer
 *
 *      Mark C. Miller, Wed Oct 10 13:08:36 PDT 2007
 *
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBgroupelmap*
db_pdb_GetGroupelmap(DBfile *_dbfile, char const *name)
{
    int i, j, n;
    DBgroupelmap *gm = NULL;
    DBfile_pdb *dbfile = (DBfile_pdb *) _dbfile;
    static char *me = "db_pdb_GetGroupelmap";
    PJcomplist tmp_obj;
    int *segData = NULL;
    int *fracLengths = NULL;
    void *fracsArray = NULL;
    DBgroupelmap tmpgm;
    PJcomplist *_tcl;

    memset(&tmpgm, 0, sizeof(DBgroupelmap));
    INIT_OBJ(&tmp_obj);
    DEFINE_OBJ("num_segments",    &tmpgm.num_segments, DB_INT);
    DEFINE_OBJ("fracs_data_type", &tmpgm.fracs_data_type, DB_INT);
    DEFALL_OBJ("groupel_types",   &tmpgm.groupel_types, DB_INT);
    DEFALL_OBJ("segment_lengths", &tmpgm.segment_lengths, DB_INT);
    DEFALL_OBJ("segment_ids",     &tmpgm.segment_ids, DB_INT);
    DEFALL_OBJ("segment_data",    &segData, DB_INT);
    DEFALL_OBJ("frac_lengths",    &fracLengths, DB_INT);
    DEFALL_OBJ("segment_fracs",   &fracsArray, DB_FLOAT);

    if (PJ_GetObject(dbfile->pdb, (char*)name, &tmp_obj, DB_GROUPELMAP) < 0)
        return NULL;

    gm = (DBgroupelmap*) calloc(1,sizeof(DBgroupelmap));
    *gm = tmpgm;

    /* unflatten the segment data */
    gm->segment_data = (int **) malloc(gm->num_segments * sizeof(int*));
    n = 0;
    for (i = 0; i < gm->num_segments; i++)
    {
        int sl = gm->segment_lengths[i];
        gm->segment_data[i] = 0;
        if (sl > 0)
        {
            gm->segment_data[i] = (int*) malloc(sl * sizeof(int));
            for (j = 0; j < sl; j++)
                gm->segment_data[i][j] = segData[n++];
        }
    }
    FREE(segData);

    /* unflatten frac data if we have it */
    if (fracLengths != NULL)
    {
        gm->segment_fracs = (void **)malloc(gm->num_segments * sizeof(void*));
        n = 0;
        for (i = 0; i < gm->num_segments; i++)
        {
            int len = fracLengths[i];

            if (len <= 0)
            {
                gm->segment_fracs[i] = 0;
                continue;
            }

            gm->segment_fracs[i] = malloc(len * ((gm->fracs_data_type==DB_FLOAT)?sizeof(float):sizeof(double)));
            for (j = 0; j < len; j++)
            {
                if (gm->fracs_data_type == DB_FLOAT)
                {
                    float *pfa = (float *) fracsArray;
                    float *psf = (float *) (gm->segment_fracs[i]);
                    psf[j] = pfa[n++];
                }
                else
                {
                    double *pfa = (double *) fracsArray;
                    double *psf = (double *) (gm->segment_fracs[i]);
                    psf[j] = pfa[n++];
                }
            }
        }
    }
    else
    {
        gm->segment_fracs = 0;
    }
    FREE(fracLengths);
    FREE(fracsArray);

    gm->name = STRDUP(name);
    if (DB_DOUBLE == gm->fracs_data_type && PJ_InqForceSingle())
        gm->fracs_data_type = DB_FLOAT;

    return gm;
}

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_GetMrgvar
 *
 *  Purpose
 *
 *      Read a mrgvar object from a SILO file and return the
 *      SILO structure for this type.
 *
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Replaced strtok-loop over ...names member with call to
 *   db_StringListToStringArray. Added logic to control behavior of
 *   slash character swapping for windows/linux and skipping of first
 *   semicolon in calls to db_StringListToStringArray.
 *--------------------------------------------------------------------*/
SILO_CALLBACK DBmrgvar *
db_pdb_GetMrgvar(DBfile *_dbfile, char const *objname)
{
   DBmrgvar      *mrgv = NULL;
   int            i;
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   PJcomplist     tmp_obj;
   char           tmp[256];
   static char   *me = "db_pdb_GetMrgvar";
   char          *rpnames = NULL;
   char          *cnames = NULL;
   DBmrgvar       tmpmrgv;
   PJcomplist    *_tcl;

   /*------------------------------------------------------------*/
   /*          Comp. Name        Comp. Address     Data Type     */
   /*------------------------------------------------------------*/
   memset(&tmpmrgv, 0, sizeof(DBmrgvar));
   INIT_OBJ(&tmp_obj);

   DEFINE_OBJ("ncomps", &tmpmrgv.ncomps, DB_INT);
   DEFINE_OBJ("nregns", &tmpmrgv.nregns, DB_INT);
   DEFINE_OBJ("datatype", &tmpmrgv.datatype, DB_INT);
   DEFALL_OBJ("mrgt_name",&tmpmrgv.mrgt_name, DB_CHAR);
   DEFALL_OBJ("compnames", &cnames, DB_CHAR);
   DEFALL_OBJ("reg_pnames", &rpnames, DB_CHAR);

   if (PJ_GetObject(dbfile->pdb, (char*)objname, &tmp_obj, DB_MRGVAR) < 0)
      return NULL;
   mrgv = (DBmrgvar *) calloc(1, sizeof(DBmrgvar));
   *mrgv = tmpmrgv;

   INIT_OBJ(&tmp_obj);

   mrgv->data = ALLOC_N(void *, mrgv->ncomps);

   if (mrgv->datatype == 0) {
      strcpy(tmp, objname);
      strcat(tmp, "_data");
      if ((mrgv->datatype = db_pdb_GetVarDatatype(dbfile->pdb, tmp)) < 0) {
         /* Not found. Assume float. */
         mrgv->datatype = DB_FLOAT;
      }
   }

   if (PJ_InqForceSingle())
      mrgv->datatype = DB_FLOAT;

   for (i = 0; i < mrgv->ncomps; i++) {
      DEFALL_OBJ(_valstr[i], &mrgv->data[i], DB_FLOAT);
   }

   PJ_GetObject(dbfile->pdb, (char*)objname, &tmp_obj, 0);

   if (cnames != NULL)
   {
      mrgv->compnames = DBStringListToStringArray(cnames, &(mrgv->ncomps), !skipFirstSemicolon);
      FREE(cnames);
   }

   if (rpnames != NULL)
   {
      mrgv->reg_pnames = DBStringListToStringArray(rpnames, 0, !skipFirstSemicolon);
      FREE(rpnames);
   }

   mrgv->name = STRDUP(objname);

   return (mrgv);
}

/*----------------------------------------------------------------------
 *  Routine                              db_pdb_FreeCompressionResources 
 *
 *  Purpose: Implement this method to quite error messages from top
 *           top-level api. This is better than just NOT generating
 *           those messages at the top-level because that would make
 *           the choice for all drivers.
 *
 *  Programmer: Mark C. Miller, Tue Dec  2 09:59:20 PST 2008
 *--------------------------------------------------------------------*/
SILO_CALLBACK int
db_pdb_FreeCompressionResources(DBfile *_dbfile, char const *meshname)
{
    return 0;
}

/*----------------------------------------------------------------------
 *  Routine                                         db_pdb_WriteObject
 *
 *  Purpose
 *
 *      Write an object into the given file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *
 * Modifications:
 *
 *      Robb Matzke, 7 Mar 1997
 *      Returns a failure status if PJ_put_group fails.
 *
 *      Robb Matzke, 7 Mar 1997
 *      The FREEMEM argument was replaced with a FLAGS argument that
 *      has the value 0, FREE_MEM, or OVER_WRITE depending on how
 *      this function got called (FREE_MEM still doesn't do anything).
 *
 *      Mark C. Miller, 10May06
 *      Passed value for SILO_Globals.allowOverwrites to PJ_put_group
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_WriteObject(DBfile   *_file,    /*File to write into */
                   DBobject const *obj,/*Object description to write out */
                   int      flags)     /*Sentinel: 1=free associated memory */
{
   PJgroup       *group;
   PDBfile       *file;
   static char   *me = "db_pdb_WriteObject";

   if (!obj || !_file)
      return (OOPS);
   file = ((DBfile_pdb *) _file)->pdb;

   group = PJ_make_group(obj->name, obj->type, obj->comp_names,
                         obj->pdb_names, obj->ncomponents);
   if (!PJ_put_group(file, group, (OVER_WRITE==flags?1:0) ||
                                  DBGetAllowOverwritesFile(_file))) {
      PJ_rel_group (group);
      return db_perror ("PJ_put_group", E_CALLFAIL, me);
   }
   PJ_rel_group(group);
   return (OKAY);
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                       db_pdb_WriteComponent
 *
 *  Purpose
 *
 *      Add a variable component to the given object structure, AND
 *      write out the associated data.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Returns
 *
 *      Returns OKAY on success, OOPS on failure.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_WriteComponent (DBfile *_file, DBobject *obj, char const *compname,
                       char const *prefix, char const *datatype, void const *var,
                       int nd, long const *count)
{
   PDBfile       *file;
   char           tmp[256];

   file = ((DBfile_pdb *) _file)->pdb;
   db_mkname(file, prefix, compname, tmp);

   PJ_write_len(file, tmp, datatype, var, nd, count);
   DBAddVarComponent(obj, compname, tmp);
   return 0;
}
#endif /* PDB_WRITE */


/*-------------------------------------------------------------------------
 * Function:    db_pdb_Write
 *
 * Purpose:     Writes a single variable into a file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Mon Nov 21 21:17:50 EST 1994
 *
 * Modifications:
 *
 *      Robb Matzke, Sun Dec 18 17:14:18 EST 1994
 *      Changed SW_GetDatatypeString to db_GetDatatypeString.
 *
 *      Sean Ahern, Sun Oct  1 03:19:43 PDT 1995
 *      Made "me" static.
 *-------------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_Write (DBfile *_dbfile, char const *vname, void const *var,
              int const *dims, int ndims, int datatype)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   char          *idata;
   int            i;
   long           ind[3 * MAXDIMS_VARWRITE];
   static char   *me = "db_pdb_Write";

   for (i = 0; i < 3 * MAXDIMS_VARWRITE; i++)
      ind[i] = 0;
   for (i = 0; i < ndims; i++) {
      ind[3 * i] = 0;
      ind[3 * i + 1] = dims[i] - 1;
      ind[3 * i + 2] = 1;
   }

   idata = db_GetDatatypeString(datatype);
   if (!PJ_write_alt(dbfile->pdb, vname, idata,
                     var, ndims, ind)) {
      return db_perror("PJ_write_alt", E_CALLFAIL, me);
   }
   FREE(idata);
   return 0;
}
#endif /* PDB_WRITE */


/*-------------------------------------------------------------------------
 * Function:    db_pdb_WriteSlice
 *
 * Purpose:     Similar to db_pdb_Write except only part of the data is
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
 *      Sean Ahern, Tue Mar 31 11:15:03 PST 1998
 *      Freed dtype_s.
 *-------------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_WriteSlice(
    DBfile *_dbfile,
    char const *vname,
    void const *values,
    int dtype,
    int const *offset,
    int const *length,
    int const *stride,
    int const *dims,
    int ndims
)
{
   char         *dtype_s ;
   long         dim_extents[9] ;
   int          i ;
   DBfile_pdb   *dbfile = (DBfile_pdb*)_dbfile ;
   syment       *ep ;
   dimdes       *dimensions ;
   static char  *me = "db_pdb_WriteSlice" ;

   if (NULL==(dtype_s=db_GetDatatypeString (dtype))) {
      return db_perror ("db_GetDatatypeString", E_CALLFAIL, me) ;
   }

   if ((ep=lite_PD_inquire_entry (dbfile->pdb, (char*) vname, TRUE, NULL))) {
      /*
       * Variable already exists.  Make sure the supplied dimensions
       * are the same as what was originally used when space was reserved.
       */
      for (i=0, dimensions=ep->dimensions;
           i<ndims && dimensions;
           i++, dimensions=dimensions->next) {
         if (0!=dimensions->index_min) {
            FREE(dtype_s);
            return db_perror ("index_min!=0", E_BADARGS, me) ;
         }
         if (dimensions->number!=dims[i]) {
            FREE(dtype_s);
            return db_perror ("dims", E_BADARGS, me) ;
         }
      }
      if (i!=ndims) {
         FREE(dtype_s);
         return db_perror ("ndims", E_BADARGS, me) ;
      }
   } else {
      /*
       * Variable doesn't exist yet.  Reserve space for the variable
       * in the database and enter it's name in the symbol table.
       */
      for (i=0; i<3 && i<ndims; i++) {
         dim_extents[i*2+0] = 0 ;               /*minimum index*/
         dim_extents[i*2+1] = dims[i]-1 ;       /*maximum index*/
      }
      if (!lite_PD_defent_alt (dbfile->pdb, (char*) vname, dtype_s, ndims,
                               dim_extents)) {
         FREE(dtype_s);
         return db_perror ("PD_defent_alt", E_CALLFAIL, me) ;
      }
   }

   /*
    * Verify that offset and length are compatible with
    * the supplied dimensions.
    */
   for (i=0; i<ndims; i++) {
      if (offset[i]<0 || offset[i]>=dims[i]) {
         FREE(dtype_s);
         return db_perror ("offset", E_BADARGS, me) ;
      }
      if (length[i]<=0 || length[i]>dims[i]) {
         FREE(dtype_s);
         return db_perror ("length", E_BADARGS, me) ;
      }
      if (offset[i]+length[i]>dims[i]) {
         FREE(dtype_s);
         return db_perror ("offset+length", E_BADARGS, me) ;
      }
   }


   /*
    * Write the specified chunk of the values to the file.
    */
   for (i=0; i<3 && i<ndims; i++) {
      dim_extents[i*3+0] = offset[i] ;
      dim_extents[i*3+1] = offset[i] + length[i] - 1 ;
      dim_extents[i*3+2] = stride[i] ;
   }
   PJ_write_alt (dbfile->pdb, vname, dtype_s, values, ndims, dim_extents);
   FREE(dtype_s);

   return 0 ;
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    db_pdb_MkDir
 *
 * Purpose:     Creates a new directory in the open database file.
 *
 * Return:      Success:        directory ID
 *
 *              Failure:        -1
 *
 * Programmer:  robb@cloud
 *              Tue Nov 15 15:13:44 EST 1994
 *
 * Modifications:
 *     Sean Ahern, Sun Oct  1 03:14:25 PDT 1995
 *     Made "me" static.
 *-------------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_MkDir (DBfile *_dbfile, char const *name)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   static char   *me = "db_pdb_MkDir";

   if (!lite_PD_mkdir(dbfile->pdb, (char*) name)) {
      return db_perror("PD_mkdir", E_CALLFAIL, me);
   }
   return 0;
}
#endif /* PDB_WRITE */


/*-------------------------------------------------------------------------
 * Function:    db_pdb_PutCompoundarray
 *
 * Purpose:     Put a compound array object into the PDB data file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1, db_errno set
 *
 * Programmer:  matzke@viper
 *              Thu Nov  3 11:04:22 PST 1994
 *
 * Modifications:
 *
 *      Robb Matzke, Thu Nov 3 14:21:25 PST 1994
 *      Sets db_errno on failure.
 *
 *      Robb Matzke, Fri Dec 2 14:08:24 PST 1994
 *      Remove all references to SCORE memory management.
 *
 *      Robb Matzke, Sun Dec 18 17:04:29 EST 1994
 *      Changed SW_GetDatatypeString to db_GetDatatypeString and
 *      removed associated memory leak.
 *
 *      Sean Ahern, Sun Oct  1 03:09:48 PDT 1995
 *      Added "me" and made it static.
 *
 *      Hank Childs, Thu Jan  6 13:48:40 PST 2000
 *      Casted strlen to long to remove compiler warning.
 *      Put in lint directive for unused arguments.
 *
 *      Hank Childs, Wed Apr 11 08:05:24 PDT 2001
 *      Concatenate strings more intelligently [HYPer02535].
 *
 *      Hank Childs, Mon May 14 14:27:29 PDT 2001
 *      Fixed bug where there was an assumption that the string is
 *      NULL terminated.
 *
 *      Eric Brugger, Tue Apr 23 10:14:46 PDT 2002
 *      I modified the routine to add a ';' delimiter to the end of the
 *      name string so that GetCompoundarray would work properly since it
 *      makes that assumption.
 *
 *      Eric Brugger, Mon Sep 16 15:40:20 PDT 2002
 *      I corrected a bug where the routine would write out the string
 *      containing the component names one too large.  It still writes out
 *      the same number of characters, but now the array is one larger
 *      and the last character is set to a NULL character.  This seemed
 *      the safest thing to do.
 *
 *-------------------------------------------------------------------------*/
#ifdef PDB_WRITE
/* ARGSUSED */
SILO_CALLBACK int
db_pdb_PutCompoundarray (
    DBfile *_dbfile,               /*pointer to open file  */
    char const *array_name,        /*name of array object  */
    char const * const *elemnames, /*simple array names  */
    int const *elemlengths,        /*lengths of simple arrays */
    int nelems,                    /*number of simple arrays */
    void const *values,            /*vector of simple values */
    int nvalues,                   /*num of values (redundant) */
    int datatype,                  /*type of simple all values */
    DBoptlist const *optlist       /*option list   */
)
{
   DBobject      *obj;
   char          *tmp, *cur;
   int            i, acc, len;
   long           count[3];

   /*
    * Build the list of simple array names in the format:
    *  `;name1;name2;...;nameN;'  The string must have a ';' at
    * the end for GetCompoundarray to work properly.
    */
   for (i = 0, acc = 1; i < nelems; i++)
      acc += strlen(elemnames[i]) + 1;
   acc++;
   tmp = ALLOC_N(char, acc);

   tmp[0] = '\0';
   cur = tmp;
   for (i = 0; i < nelems; i++) {
      strncpy(cur, ";", 1);
      cur += 1;
      len = strlen(elemnames[i]);
      strncpy(cur, elemnames[i], len);
      cur += len;
   }
   cur[0] = ';';
   cur++;
   cur[0] = '\0';

#if 0                           /*No global options available at this time */
   db_ProcessOptlist(DB_ARRAY, optlist);
#endif
   obj = DBMakeObject(array_name, DB_ARRAY, 25);

   /*
    * Write the compound array components to the database.
    */
   count[0] = (long) (cur - tmp) + 1;
   DBWriteComponent(_dbfile, obj, "elemnames", array_name,
                    "char", tmp, 1, count);
   FREE(tmp);

   count[0] = nelems;
   DBWriteComponent(_dbfile, obj, "elemlengths", array_name,
                    "integer", elemlengths, 1, count);

   DBAddIntComponent(obj, "nelems", nelems);

   count[0] = nvalues;
   tmp = db_GetDatatypeString(datatype);
   DBWriteComponent(_dbfile, obj, "values", array_name,
                    tmp, values, 1, count);
   FREE(tmp);

   DBAddIntComponent(obj, "nvalues", nvalues);
   DBAddIntComponent(obj, "datatype", datatype);

   DBWriteObject(_dbfile, obj, TRUE);
   DBFreeObject(obj);

   return OKAY;
}
#endif /* PDB_WRITE */


/*-------------------------------------------------------------------------
 * Function:    db_pdb_PutCurve
 *
 * Purpose:     Put a curve object into the PDB data file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1, db_errno set
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.com
 *              May 15, 1996
 *
 * Modifications:
 *
 *      Thomas R. Treadway, Fri Jul  7 11:43:41 PDT 2006
 *      Added DBOPT_REFERENCE support.
 *-------------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutCurve (DBfile *_dbfile, char const *name, void const *xvals, void const *yvals,
                 int dtype, int npts, DBoptlist const *opts)
{
   DBobject     *obj ;
   char         *dtype_s ;
   long         lnpts = npts ;
   static char  *me = "db_pdb_PutCurve" ;

   /*
    * Set global curve options to default values, then initialize
    * them with any values specified in the options list.
    */
   db_InitCurve (opts) ;
   obj = DBMakeObject (name, DB_CURVE, 19) ;

   /*
    * Write the X and Y arrays.  If the user specified a variable
    * name with the OPTS for the X or Y arrays then we assume that
    * the arrays have already been stored in the file.  This allows
    * us to share X values (or Y values) among several curves.
    * If a variable name is specified, then the corresponding X or
    * Y values array must be the null pointer!
    */
   dtype_s = db_GetDatatypeString (dtype) ;
   if (_cu._varname[0]) {
      DBAddVarComponent (obj, "xvals", _cu._varname[0]) ;
   } else {
      if (npts && xvals && !_cu._reference) {
         DBWriteComponent (_dbfile, obj, "xvals", name, dtype_s,
                        xvals, 1, &lnpts);
      }
   }
   if (_cu._varname[1]) {
      DBAddVarComponent (obj, "yvals", _cu._varname[1]) ;
   } else {
      if (npts && yvals && !_cu._reference) {
         DBWriteComponent (_dbfile, obj, "yvals", name, dtype_s,
                        yvals, 1, &lnpts);
      }
   }
   FREE (dtype_s) ;

   /*
    * Now output the other values of the curve.
    */
   DBAddIntComponent (obj, "npts", npts) ;
   DBAddIntComponent (obj, "datatype", dtype) ;
   if (_cu._label)      DBAddStrComponent (obj, "label",    _cu._label) ;
   if (_cu._varname[0]) DBAddStrComponent (obj, "xvarname", _cu._varname[0]) ;
   if (_cu._labels[0])  DBAddStrComponent (obj, "xlabel",   _cu._labels[0]) ;
   if (_cu._units[0])   DBAddStrComponent (obj, "xunits",   _cu._units[0]) ;
   if (_cu._varname[1]) DBAddStrComponent (obj, "yvarname", _cu._varname[1]) ;
   if (_cu._labels[1])  DBAddStrComponent (obj, "ylabel",   _cu._labels[1]) ;
   if (_cu._units[1])   DBAddStrComponent (obj, "yunits",   _cu._units[1]) ;
   if (_cu._reference)  DBAddStrComponent (obj, "reference",_cu._reference) ;
   if (_cu._guihide)    DBAddIntComponent (obj, "guihide",  _cu._guihide) ;
   if (_cu._coord_sys)  DBAddIntComponent (obj, "coord_sys", _cu._coord_sys) ;
   if (_cu._missing_value != DB_MISSING_VALUE_NOT_SET)
   {
      if (_cu._missing_value == 0.0)
          DBAddDblComponent(obj, "missing_value", DB_MISSING_VALUE_NOT_SET);
      else
          DBAddDblComponent(obj, "missing_value", _cu._missing_value);
   }

   DBWriteObject (_dbfile, obj, TRUE) ;
   DBFreeObject(obj);

   return 0 ;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 * Routine                                           db_pdb_PutDefvars
 *
 * Purpose
 *
 *    Write a defvars object into the open SILO file.
 *
 * Programmer
 *
 *    Mark C. Miller
 *    August 8, 2005
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutDefvars(
    DBfile *dbfile,
    char const *name,
    int ndefs,
    char const * const *names,
    int const *types,
    char const * const *defns,
    DBoptlist const * const *optlists
)
{
   int            i, len;
   long           count[1];
   DBobject      *obj;
   char          *tmp = NULL, *cur = NULL;
   int           *guihide = NULL;

   /*
    * Optlists are a little funky for this object because we were
    * concerned about possibly handling things like units, etc. So,
    * we really have an array of optlists that needs to get serialized.
    */
   if (optlists)
   {
       for (i = 0; i < ndefs; i++)
       {
           db_InitDefvars(optlists[i]);
           if (_dv._guihide)
           {
               if (guihide == NULL)
                   guihide = (int* ) calloc(ndefs, sizeof(int));
               guihide[i] = _dv._guihide;
           }
       }
   }

   /*-------------------------------------------------------------
    *  Build object description from literals and var-id's
    *-------------------------------------------------------------*/
   obj = DBMakeObject(name, DB_DEFVARS, 10);
   DBAddIntComponent(obj, "ndefs", ndefs);

   /*-------------------------------------------------------------
    *  Define and write variables types
    *-------------------------------------------------------------*/
   count[0] = ndefs;
   DBWriteComponent(dbfile, obj, "types", name, "integer",
                    (int*) types, 1, count);

   /*-------------------------------------------------------------
    *  Define and write variable names 
    *-------------------------------------------------------------*/
   DBStringArrayToStringList((char const * const *)names, ndefs, &tmp, &len);
   count[0] = len;  
   DBWriteComponent(dbfile, obj, "names", name, "char",
                    tmp, 1, count);
   FREE(tmp);
   tmp = NULL;

   /*-------------------------------------------------------------
    *  Define and write variable definitions 
    *-------------------------------------------------------------*/
   DBStringArrayToStringList((char const * const *)defns, ndefs, &tmp, &len);
   count[0] = len;  
   DBWriteComponent(dbfile, obj, "defns", name, "char",
                    tmp, 1, count);
   FREE(tmp);
   tmp = NULL;

   if (guihide != NULL) {

      count[0] = ndefs;
      DBWriteComponent(dbfile, obj, "guihide", name, "integer", guihide,
                       1, count);
      free(guihide);
   }

   /*-------------------------------------------------------------
    *  Write defvars object to SILO file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_PutFacelist
 *
 *  Purpose
 *
 *      Write a ucd facelist object into the open output file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modified
 *
 *      Robb Matzke, Wed Nov 16 11:39:19 EST 1994
 *      Added device independence.
 *
 *      Robb Matzke, Fri Dec 2 14:12:48 PST 1994
 *      Changed SCFREE(obj) to DBFreeObject(obj)
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutFacelist(
    DBfile *dbfile,
    char const *name,
    int nfaces,
    int ndims,
    int const *nodelist,
    int lnodelist,
    int origin,
    int const *zoneno,
    int const *shapesize,
    int const *shapecnt,
    int nshapes,
    int const *types,
    int const *typelist,
    int ntypes
)
{
   long           count[5];
   DBobject      *obj;

   /*--------------------------------------------------
    *  Build up object description by defining literals
    *  and defining/writing arrays.
    *-------------------------------------------------*/
   obj = DBMakeObject(name, DB_FACELIST, 15);

   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nfaces", nfaces);
   DBAddIntComponent(obj, "nshapes", nshapes);
   DBAddIntComponent(obj, "ntypes", ntypes);
   DBAddIntComponent(obj, "lnodelist", lnodelist);
   DBAddIntComponent(obj, "origin", origin);

   count[0] = lnodelist;

   DBWriteComponent(dbfile, obj, "nodelist", name, "integer",
                    nodelist, 1, count);

   if (ndims == 2 || ndims == 3) {
      count[0] = nshapes;

      DBWriteComponent(dbfile, obj, "shapecnt", name, "integer",
                       shapecnt, 1, count);

      DBWriteComponent(dbfile, obj, "shapesize", name, "integer",
                       shapesize, 1, count);
   }

   if (ntypes > 0 && typelist != NULL) {
      count[0] = ntypes;
      DBWriteComponent(dbfile, obj, "typelist", name, "integer",
                       typelist, 1, count);
   }

   if (ntypes > 0 && types != NULL) {
      count[0] = nfaces;
      DBWriteComponent(dbfile, obj, "types", name, "integer",
                       types, 1, count);
   }

   if (zoneno != NULL) {
      count[0] = nfaces;
      DBWriteComponent(dbfile, obj, "zoneno", name, "integer",
                       zoneno, 1, count);
   }

   /*-------------------------------------------------------------
    *  Write object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_PutMaterial
 *
 *  Purpose
 *
 *      Write a material object into the open SILO file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *      See the prologue for DBPutMaterial for a description of this
 *      data.
 *
 *  Modifications
 *
 *      Robb Matzke, Sun Dec 18 17:05:39 EST 1994
 *      Changed SW_GetDatatypeString to db_GetDatatypeString and removed
 *      associated memory leak.
 *
 *      Robb Matzke, Fri Dec 2 14:13:55 PST 1994
 *      Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *      Robb Matzke, Wed Nov 16 11:45:08 EST 1994
 *      Added device independence.
 *
 *      Al Leibee, Tue Jul 13 17:14:31 PDT 1993
 *      FREE of obj to SCFREE for consistant MemMan usage.
 *
 *      Sean Ahern, Wed Jan 17 17:02:31 PST 1996
 *      Added writing of the datatype parameter.
 *
 *      Sean Ahern, Tue Feb  5 10:19:53 PST 2002
 *      Added naming of Silo materials.
 *
 *      Mark C. Miller, Thu Feb 11 09:40:10 PST 2010
 *      Set global values in _ma to zero after use.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutMaterial(
    DBfile *dbfile,
    char const *name,
    char const *mname,
    int nmat,
    int const *matnos,
    int const *matlist,
    int const *dims,
    int ndims,
    int const *mix_next,
    int const *mix_mat,
    int const *mix_zone,
    void const *mix_vf,
    int mixlen,
    int datatype,
    DBoptlist const *optlist
)
{
   int            i, nels;
   long           count[3];
   DBobject      *obj;
   char          *datatype_str;
   int            is_empty = 1;

   for (i = 0; i < ndims; i++)
   {
       if (dims[i] > 0)
       {
           is_empty = 0;
           break;
       }
   }

   /*-------------------------------------------------------------
    *  Process option list; build object description.
    *-------------------------------------------------------------*/
   db_ProcessOptlist(DB_MATERIAL, optlist);
   obj = DBMakeObject(name, DB_MATERIAL, 26);

   /*-------------------------------------------------------------
    * Define literals used by material data.
    *-------------------------------------------------------------*/

   DBAddStrComponent(obj, "meshid", mname);
   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nmat", nmat);
   DBAddIntComponent(obj, "mixlen", mixlen);
   DBAddIntComponent(obj, "origin", _ma._origin);
   DBAddIntComponent(obj, "major_order", _ma._majororder);
   DBAddIntComponent(obj, "datatype", datatype);
   if (_ma._allowmat0)
      DBAddIntComponent(obj, "allowmat0", _ma._allowmat0);
   if (_ma._guihide)
      DBAddIntComponent(obj, "guihide", _ma._guihide);

   /*-------------------------------------------------------------
    * Define variables, write them into object description.
    *-------------------------------------------------------------*/
   count[0] = ndims;
   DBWriteComponent(dbfile, obj, "dims", name, "integer", dims, 1, count);

   /* Do zonal material ID array */
   nels = ndims==0?0:1;
   for (i = 0; i < ndims; i++)
      nels *= dims[i];

   count[0] = nels;
   if (!is_empty)
       DBWriteComponent(dbfile, obj, "matlist", name, "integer", matlist, 1, count);

   /* Do material numbers list */
   count[0] = nmat;
   DBWriteComponent(dbfile, obj, "matnos", name, "integer", matnos, 1, count);

   /* Now do mixed data arrays (mix_zone is optional) */
   if (!is_empty && mixlen > 0) {

      datatype_str = db_GetDatatypeString(datatype);
      count[0] = mixlen;
      DBWriteComponent(dbfile, obj, "mix_vf", name, datatype_str,
                       mix_vf, 1, count);
      FREE(datatype_str);

      DBWriteComponent(dbfile, obj, "mix_next", name, "integer",
                       mix_next, 1, count);
      DBWriteComponent(dbfile, obj, "mix_mat", name, "integer",
                       mix_mat, 1, count);

      if (mix_zone != NULL) {
         DBWriteComponent(dbfile, obj, "mix_zone", name, "integer",
                          mix_zone, 1, count);
      }
   }

   /* If we have material names, write them out */
   if (_ma._matnames != NULL)
   {
      int len; long llen; char *tmpstr = 0;
      DBStringArrayToStringList((char const * const *)_ma._matnames, nmat, &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "matnames", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
      _ma._matnames = NULL;
   }
   if (_ma._matcolors != NULL)
   {
      int len; long llen; char *tmpstr = 0;
      DBStringArrayToStringList((char const * const *)_ma._matcolors, nmat, &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "matcolors", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
      _ma._matcolors = NULL;
   }


   /*-------------------------------------------------------------
    *  Write material object to output file. Request that underlying
    *  memory be freed (the 'TRUE' argument.)
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_PutMatspecies
 *
 *  Purpose
 *
 *      Write a material species object into the open SILO file.
 *
 *  Programmer
 *
 *      Al Leibee, DSAD-B
 *
 *  Notes
 *
 *      See the prologue for DBPutMatspecies for a description of this
 *      data.
 *
 *  Modifications
 *
 *      Robb Matzke, Fri Dec 2 14:14:27 PST 1994
 *      Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *      Robb Matzke, Sun Dec 18 17:06:28 EST 1994
 *      Changed SW_GetDatatypeString to db_GetDatatypeString and removed
 *      associated memory leak.
 *
 *      Jeremy Meredith, Wed Jul  7 12:15:31 PDT 1999
 *      I removed the origin value from the species object.
 *
 *      Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *      Added names and colors for species.
 *
 *      Mark C. Miller, Thu Feb 11 09:40:10 PST 2010
 *      Set global values in _ma to zero after use.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutMatspecies (DBfile *dbfile, char const *name, char const *matname,
                      int nmat, int const *nmatspec, int const *speclist,
                      int const *dims, int ndims, int nspecies_mf,
                      void const *species_mf, int const *mix_speclist,
                      int mixlen, int datatype, DBoptlist const *optlist) {

   long           count[3];
   int            i, nels, nstrs;
   DBobject      *obj;
   char          *datatype_str;
   int            is_empty = 1;

   for (i = 0; i < ndims; i++)
   {
       if (dims[i] > 0)
       {
           is_empty = 0;
           break;
       }
   }

   /*-------------------------------------------------------------
    *  Process option list; build object description.
    *-------------------------------------------------------------*/
   db_ProcessOptlist(DB_MATSPECIES, optlist);
   obj = DBMakeObject(name, DB_MATSPECIES, 15);

   /*-------------------------------------------------------------
    * Define literals used by material species data.
    *-------------------------------------------------------------*/
   DBAddStrComponent(obj, "matname", matname);
   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nmat", nmat);
   DBAddIntComponent(obj, "nspecies_mf", nspecies_mf);
   DBAddIntComponent(obj, "mixlen", mixlen);
   DBAddIntComponent(obj, "datatype", datatype);
   DBAddIntComponent(obj, "major_order", _ms._majororder);
   if (_ms._guihide)
      DBAddIntComponent(obj, "guihide", _ms._guihide);

   /*-------------------------------------------------------------
    * Define variables, write them into object description.
    *-------------------------------------------------------------*/
   count[0] = ndims;
   DBWriteComponent(dbfile, obj, "dims", name, "integer", dims, 1, count);

   /* Do zonal material species ID array */
   nels = ndims==0?0:1;
   for (i = 0; i < ndims; i++)
      nels *= dims[i];

   count[0] = nels;
   if (!is_empty)
       DBWriteComponent(dbfile, obj, "speclist", name, "integer", speclist, 1, count);

   /* Do material species count per material list */
   count[0] = nmat;
   DBWriteComponent(dbfile, obj, "nmatspec", name, "integer", nmatspec, 1, count);

   /* Do material species mass fractions */
   datatype_str = db_GetDatatypeString(datatype);
   count[0] = nspecies_mf;
   if (!is_empty)
       DBWriteComponent(dbfile, obj, "species_mf", name, datatype_str, species_mf, 1, count);
   FREE(datatype_str);

   /* Now do mixed data arrays */
   if (!is_empty && mixlen > 0) {
      count[0] = mixlen;
      DBWriteComponent(dbfile, obj, "mix_speclist", name, "integer",
                       mix_speclist, 1, count);
   }

   /* If we have species names or colors, write them out */
   nstrs = 0;
   if (_ms._specnames != NULL)
   {
      int len; long llen; char *tmpstr = 0;

      /* count how many names we have */
      for (i=0; i < nmat; i++)
          nstrs += nmatspec[i];
      DBStringArrayToStringList((char const * const *)_ms._specnames, nstrs, &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "species_names", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
      _ms._specnames = NULL;
   }
   if (_ms._speccolors != NULL)
   {
      int len; long llen; char *tmpstr = 0;

      /* count how many names we have */
      if (nstrs == 0)
      {
          for (i=0; i < nmat; i++)
              nstrs += nmatspec[i];
      }
      DBStringArrayToStringList((char const * const *)_ms._speccolors, nstrs, &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "speccolors", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
      _ms._speccolors = NULL;
   }

   /*-------------------------------------------------------------
    *  Write material object to output file. Request that underlying
    *  memory be freed (the 'TRUE' argument.)
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 * Routine                                           db_pdb_PutMultimesh
 *
 * Purpose
 *
 *    Write a multi-block mesh object into the open SILO file.
 *
 * Programmer
 *
 *    Jeffery W. Long, NSSD-B
 *
 * Notes
 *
 * Modified
 *    Robb Matzke, Fri Dec 2 14:15:31 PST 1994
 *    Changed  SCFREE(obj) to DBFreeObject(obj)
 *
 *    Eric Brugger, Fri Jan 12 17:42:55 PST 1996
 *    I added cycle, time and dtime as options.
 *
 *    Sean Ahern, Thu Aug 15 11:16:18 PDT 1996
 *    Allowed the mesh names to be any length, rather than hardcoded.
 *
 *    Eric Brugger, Fri Oct 17 09:11:58 PDT 1997
 *    I corrected the outputing of the cyle, time and dtime options to
 *    use the values from the global _mm instead of _pm.  A cut
 *    and paste error.
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added ngroups, blockorigin, and grouporigin.
 *
 *    Hank Childs, Thu Jan  6 13:51:22 PST 2000
 *    Casted a strlen to long to remove compiler warning.
 *
 *    Hank Childs, Wed Apr 11 08:05:24 PDT 2001
 *    Concatenate strings more intelligently [HYPer02535].
 *
 *    Hank Childs, Mon May 14 14:27:29 PDT 2001
 *    Fixed bug where there was an assumption that the string is
 *    NULL terminated.
 *
 *    Thomas R. Treadway, Thu Jul 20 13:34:57 PDT 2006
 *    Added lgroupings, groupings, and groupnames options.
 *
 *   Mark C. Miller, Wed Jul 14 20:40:55 PDT 2010
 *   Added support for namescheme/empty list options.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutMultimesh (DBfile *dbfile, char const *name, int nmesh,
                     char const * const *meshnames, int const *meshtypes,
                     DBoptlist const *optlist) {

   int            i, len;
   long           count[3];
   DBobject      *obj;
   char          *tmp = NULL, *cur = NULL;
   char          *gtmp = NULL;

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   db_InitMulti(dbfile, optlist);

   /*-------------------------------------------------------------
    *  Build object description from literals and var-id's
    *-------------------------------------------------------------*/
   obj = DBMakeObject(name, DB_MULTIMESH, 31);
   DBAddIntComponent(obj, "nblocks", nmesh);
   DBAddIntComponent(obj, "ngroups", _mm._ngroups);
   DBAddIntComponent(obj, "blockorigin", _mm._blockorigin);
   DBAddIntComponent(obj, "grouporigin", _mm._grouporigin);
   if (_mm._guihide)
      DBAddIntComponent(obj, "guihide", _mm._guihide);
   if (_mm._mrgtree_name)
      DBAddStrComponent(obj, "mrgtree_name", _mm._mrgtree_name);
   if (_mm._tv_connectivity)
      DBAddIntComponent(obj, "tv_connectivity", _mm._tv_connectivity);
   if (_mm._disjoint_mode)
      DBAddIntComponent(obj, "disjoint_mode", _mm._disjoint_mode);
   if (_mm._topo_dim > 0)
      DBAddIntComponent(obj, "topo_dim", _mm._topo_dim);

   /*-------------------------------------------------------------
    *  Define and write variables before adding them to object.
    *-------------------------------------------------------------*/
   count[0] = nmesh;
   if (meshtypes)
       DBWriteComponent(dbfile, obj, "meshtypes", name, "integer",
                        meshtypes, 1, count);

   if (meshnames && nmesh>0)
   {
       /* Compute size needed for string of concatenated block names.
        *
        * Note that we start with 2 so that we have one `;' and the NULL
        * terminator.  Also, the +1 in the "len +=" line is for the `;'
        * character.
        */
       len = 2;
       for(i=0; i<nmesh; i++)
       {
           len += strlen(meshnames[i]) + 1;
       }
       tmp = ALLOC_N(char,len);

      /* Build 1-D character string from 2-D mesh-name array */
      tmp[0] = ';';
      tmp[1] = '\0';

      cur = tmp+1;
      for (i = 0; i < nmesh; i++) {
         int len2;
         len2 = strlen(meshnames[i]);
         strncpy(cur, meshnames[i], len2);
         cur += len2;
         strncpy(cur, ";", 1);
         cur += 1;
      }

      count[0] = (long) (cur - tmp);
      DBWriteComponent(dbfile, obj, "meshnames", name, "char",
                       tmp, 1, count);
   }

   /*-------------------------------------------------------------
    *  Define and write the time and cycle.
    *-------------------------------------------------------------*/
   DBAddIntComponent(obj, "cycle", _mm._cycle);

   if (_mm._time_set == TRUE)
      DBAddVarComponent(obj, "time", _mm._nm_time);
   if (_mm._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _mm._nm_dtime);

   /*-------------------------------------------------------------
    *  Add the DBOPT_EXTENTS_SIZE and DBOPT_EXTENTS options if present.
    *-------------------------------------------------------------*/
   if (_mm._extents != NULL && _mm._extentssize > 0) {
      DBAddIntComponent(obj, "extentssize", _mm._extentssize);

      count[0] = _mm._extentssize * nmesh;
      DBWriteComponent(dbfile, obj, "extents", name, "double", _mm._extents,
                       1, count);
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_ZONECOUNTS option if present.
    *-------------------------------------------------------------*/
   if (_mm._zonecounts != NULL) {

      count[0] = nmesh;
      DBWriteComponent(dbfile, obj, "zonecounts", name, "integer", 
                       _mm._zonecounts, 1, count);
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_HAS_EXTERNAL_ZONES option if present.
    *-------------------------------------------------------------*/
   if (_mm._has_external_zones != NULL) {

      count[0] = nmesh;
      DBWriteComponent(dbfile, obj, "has_external_zones", name, "integer", 
                       _mm._has_external_zones, 1, count);
   }
   /*-------------------------------------------------------------
    *  Add the DBOPT_GROUP* options if present.
    *-------------------------------------------------------------*/
   if (_mm._lgroupings > 0)
      DBAddIntComponent(obj, "lgroupings", _mm._lgroupings);
   if ((_mm._lgroupings  > 0) && (_mm._groupnames != NULL)) {
      DBStringArrayToStringList((char const * const *)_mm._groupnames, 
                    _mm._lgroupings, &gtmp, &len);

      count[0] = len;
      DBWriteComponent(dbfile, obj, "groupnames", name, "char",
                    gtmp, 1, count);
      FREE(gtmp);
   }
   if ((_mm._lgroupings  > 0) && (_mm._groupings != NULL)) {
      count[0] = _mm._lgroupings;
      DBWriteComponent(dbfile, obj, "groupings", name, "integer",
                    _mm._groupings, 1, count);
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_MB_... options if present.
    *-------------------------------------------------------------*/
   if (_mm._file_ns)
   { 
      count[0] = strlen(_mm._file_ns)+1;
      DBWriteComponent(dbfile, obj, "file_ns", name, "char",
                    _mm._file_ns, 1, count);
   }
   if (_mm._block_ns)
   { 
      count[0] = strlen(_mm._block_ns)+1;
      DBWriteComponent(dbfile, obj, "block_ns", name, "char",
                    _mm._block_ns, 1, count);
   }
   if (_mm._block_type)
      DBAddIntComponent(obj, "block_type", _mm._block_type);
   if (_mm._empty_list && _mm._empty_cnt>0)
   {
      DBAddIntComponent(obj, "empty_cnt", _mm._empty_cnt);
      count[0] = _mm._empty_cnt;
      DBWriteComponent(dbfile, obj, "empty_list", name, "integer", 
                       _mm._empty_list, 1, count);
   }
   if (_mm._repr_block_idx > 0)
      DBAddIntComponent(obj, "repr_block_idx", _mm._repr_block_idx);

   /*-------------------------------------------------------------
    *  Write multi-mesh object to SILO file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   FREE(tmp);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 * Routine                                        db_pdb_PutMultimeshadj
 *
 * Purpose
 *
 *    Write some or all of a multi-block mesh adjacency object into the
 *    open SILO file. This routine is designed to permit multiple
 *    writes to the same object with different pieces being written
 *    each time.
 *
 * Programmer
 *
 *    Mark C. Miller, August 23, 2005 
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutMultimeshadj (DBfile *_dbfile, char const *name, int nmesh,
                  int const *meshtypes, int const *nneighbors,
                  int const *neighbors, int const *back,
                  int const *lnodelists, int const * const *nodelists,
                  int const *lzonelists, int const * const *zonelists,
                  DBoptlist const *optlist) {


   long         count[2];
   int          i, len, noff, zoff, lneighbors;
   DBfile_pdb   *dbfile = (DBfile_pdb*)_dbfile;
   syment       *ep;
   dimdes       *dimensions;
   static char  *me = "db_pdb_PutMultimeshadj";
   char tmpn[256];

   if ((ep=lite_PD_inquire_entry (dbfile->pdb, (char*)name, TRUE, NULL))) {
      /*
       * Object already exists. Do whatever sanity checking we can do without
       * reading back problem-sized data. Basically, this means checking
       * existence and size of various components.
       */

      /* meshtypes should *always* exist and be of size nmesh */
      db_mkname(dbfile->pdb, (char*)name, "meshtypes", tmpn);
      if ((ep=lite_PD_inquire_entry(dbfile->pdb, tmpn, TRUE, NULL)))
      {
         len = 0;
         for (dimensions=ep->dimensions; dimensions; dimensions=dimensions->next)
            len += dimensions->number;

         if (len != nmesh)
            return db_perror("inconsistent meshtypes", E_BADARGS, me);
      }
      else
      {
         return db_perror("not a DBmultimeshadj object", E_BADARGS, me);
      }

      /* nneirhbors should *always* exist and be of size nmesh */
      db_mkname(dbfile->pdb, (char*)name, "nneighbors", tmpn);
      if ((ep=lite_PD_inquire_entry(dbfile->pdb, tmpn, TRUE, NULL)))
      {
         len = 0;
         for (dimensions=ep->dimensions; dimensions; dimensions=dimensions->next)
            len += dimensions->number;

         if (len != nmesh)
            return db_perror("inconsistent nneighbors", E_BADARGS, me);
      }
      else
      {
         return db_perror("not a DBmultimeshadj object", E_BADARGS, me);
      }

      /* compute expected size of neighbors array */
      lneighbors = 0;
      for (i = 0; i < nmesh; i++)
          lneighbors += nneighbors[i];

      /* neighbors should always exist and be of size lneighbors */
      db_mkname(dbfile->pdb, (char*)name, "neighbors", tmpn);
      if ((ep=lite_PD_inquire_entry(dbfile->pdb, tmpn, TRUE, NULL)))
      {
         len = 0;
         for (dimensions=ep->dimensions; dimensions; dimensions=dimensions->next)
            len += dimensions->number;

         if (len != lneighbors)
            return db_perror("inconsistent neighbors", E_BADARGS, me);
      }
      else
      {
         return db_perror("not a DBmultimeshadj object", E_BADARGS, me);
      }

      /* if lnodelists exists, it should be of size lneighbors AND it
         should be non-NULL in the argument list. Otherwise, it should be NULL */
      db_mkname(dbfile->pdb, (char*)name, "lnodelists", tmpn);
      if ((ep=lite_PD_inquire_entry(dbfile->pdb, tmpn, TRUE, NULL)))
      {
         if (lnodelists == 0)
            return db_perror("inconsistent lnodelists", E_BADARGS, me);
         len = 0;
         for (dimensions=ep->dimensions; dimensions; dimensions=dimensions->next)
            len += dimensions->number;
         if (len != lneighbors)
            return db_perror("inconsistent lnodelists", E_BADARGS, me);
      }
      else
      {
         if (lnodelists != 0)
            return db_perror("inconsistent lnodelists", E_BADARGS, me);
      }

      /* if lzonelists exists, it should be of size lneighbors AND it
         should be non-NULL in the argument list. Otherwise, it should be NULL */
      db_mkname(dbfile->pdb, (char*)name, "lzonelists", tmpn);
      if ((ep=lite_PD_inquire_entry(dbfile->pdb, tmpn, TRUE, NULL)))
      {
         if (lzonelists == 0)
            return db_perror("inconsistent lzonelists", E_BADARGS, me);
         len = 0;
         for (dimensions=ep->dimensions; dimensions; dimensions=dimensions->next)
            len += dimensions->number;
         if (len != lneighbors)
            return db_perror("inconsistent lzonelists", E_BADARGS, me);
      }
      else
      {
         if (lzonelists != 0)
            return db_perror("inconsistent lzonelists", E_BADARGS, me);
      }

   } else {
      /*
       * Object doesn't exist yet.  Write all the object's invariant
       * components and reserve space for the nodelists and/or
       * zonelists and enter names in the symbol table.
       */
      DBobject *obj;

      /* compute length of neighbors, back, lnodelists, nodelists,
         lzonelists, zonelists arrays */
      lneighbors = 0;
      for (i = 0; i < nmesh; i++)
          lneighbors += nneighbors[i];

      db_InitMulti(_dbfile, optlist);
      obj = DBMakeObject(name, DB_MULTIMESHADJ, 13);

      DBAddIntComponent(obj, "nblocks", nmesh);
      DBAddIntComponent(obj, "blockorigin", _mm._blockorigin);
      DBAddIntComponent(obj, "lneighbors", lneighbors);

      count[0] = nmesh;
      DBWriteComponent(_dbfile, obj, "meshtypes", name, "integer",
                       meshtypes, 1, count);
      DBWriteComponent(_dbfile, obj, "nneighbors", name, "integer",
                       nneighbors, 1, count);

      count[0] = lneighbors;
      if (count[0] > 0) {
          DBWriteComponent(_dbfile, obj, "neighbors", name, "integer",
                           neighbors, 1, count);
          if (back) {
              DBWriteComponent(_dbfile, obj, "back", name, "integer",
                               back, 1, count);
          }
          if (lnodelists) {
              DBWriteComponent(_dbfile, obj, "lnodelists", name, "integer",
                               lnodelists, 1, count);
          }
          if (lzonelists) {
              DBWriteComponent(_dbfile, obj, "lzonelists", name, "integer",
                               lzonelists, 1, count);
          }
      }

      /* All object components up to here are invariant and *should*
         be identical in repeated calls. Now, handle the parts of the
         object that can vary from call to call. Reserve space for
         the entire nodelists and/or zonelists arrays */

      if (nodelists) {

          /* compute total length of nodelists array */
          len = 0;
          for (i = 0; i < lneighbors; i++)
              len += lnodelists[i];

          if (len > 0) {
              DBAddIntComponent(obj, "totlnodelists", len);

              /* reserve space for the nodelists array in the file */
              count[0] = 0;
              count[1] = len - 1;
              db_mkname(dbfile->pdb, (char*)name, "nodelists", tmpn);
              if (!lite_PD_defent_alt (dbfile->pdb, tmpn, "integer", 1, count)) {
                 return db_perror ("PD_defent_alt", E_CALLFAIL, me) ;
              }

              /* add the nodelists array to this object */
              DBAddVarComponent(obj, "nodelists", tmpn);
          }
      }

      if (zonelists) {

          /* compute total length of nodelists array */
          len = 0;
          for (i = 0; i < lneighbors; i++)
              len += lzonelists[i];

          if (len > 0) {
              DBAddIntComponent(obj, "totlzonelists", len);

              /* reserve space for the nodelists array in the file */
              count[0] = 0;
              count[1] = len - 1;
              db_mkname(dbfile->pdb, (char*)name, "zonelists", tmpn);
              if (!lite_PD_defent_alt (dbfile->pdb, tmpn, "integer", 1, count)) {
                 return db_perror ("PD_defent_alt", E_CALLFAIL, me) ;
              }

              /* add the nodelists array to this object */
              DBAddVarComponent(obj, "zonelists", tmpn);
          }
      }

      /* Ok, finally, create the object in the file */
      DBWriteObject(_dbfile, obj, TRUE);
      DBFreeObject(obj);
   }

   /* Ok, now write contents of nodelists and/or zonelists */
   noff = 0;
   zoff = 0;
   for (i = 0; i < lneighbors; i++)
   {
      long dim_extents[3];

      if (nodelists)
      {
         if (nodelists[i])
         {
            dim_extents[0] = noff;
            dim_extents[1] = noff + lnodelists[i] - 1;
            dim_extents[2] = 1;
            db_mkname(dbfile->pdb, (char*)name, "nodelists", tmpn);
            PJ_write_alt (dbfile->pdb, tmpn, "integer", (int*) nodelists[i], 1, dim_extents);
         }
         noff += lnodelists[i];
      }

      if (zonelists)
      {
         if (zonelists[i])
         {
            dim_extents[0] = zoff;
            dim_extents[1] = zoff + lzonelists[i] - 1;
            dim_extents[2] = 1;
            db_mkname(dbfile->pdb, (char*)name, "zonelists", tmpn);
            PJ_write_alt (dbfile->pdb, tmpn, "integer", (int*) zonelists[i], 1, dim_extents);
         }
         zoff += lzonelists[i];
      }
   }

   return 0;
}
#endif


/*----------------------------------------------------------------------
 * Routine                                            db_pdb_PutMultivar
 *
 * Purpose
 *
 *    Write a multi-block variable object into the open SILO file.
 *
 * Programmer
 *
 *    Jeffery W. Long, NSSD-B
 *
 * Notes
 *
 * Modified
 *    Robb Matzke, Fri Dec 2 14:16:09 PST 1994
 *    Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *    Eric Brugger, Fri Jan 12 17:42:55 PST 1996
 *    I added cycle, time and dtime as options.
 *
 *    Sean Ahern, Thu Aug 15 11:16:18 PDT 1996
 *    Allowed the mesh names to be any length, rather than hardcoded.
 *
 *    Eric Brugger, Fri Oct 17 09:11:58 PDT 1997
 *    I corrected the outputing of the cyle, time and dtime options to
 *    use the values from the global _mm instead of _pm.  A cut
 *    and paste error.
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added ngroups, blockorigin, and grouporigin.
 *
 *    Hank Childs, Thu Jan  6 13:51:22 PST 2000
 *    Casted a strlen to long to remove a compiler warning.
 *
 *    Hank Childs, Wed Apr 11 08:05:24 PDT 2001
 *    Concatenate strings more intelligently [HYPer02535].
 *
 *    Hank Childs, Mon May 14 14:27:29 PDT 2001
 *    Fixed bug where there was an assumption that the string is
 *    NULL terminated.
 *
 *    Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *    Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Wed Jul 14 20:40:55 PDT 2010
 *   Added support for namescheme/empty list options.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutMultivar (DBfile *dbfile, char const *name, int nvars,
                    char const * const *varnames, int const *vartypes, DBoptlist const *optlist) {

   int            i, len;
   long           count[3];
   char          *tmp = NULL, *cur = NULL;
   DBobject      *obj;

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   db_InitMulti(dbfile, optlist);

   /*-------------------------------------------------------------
    *  Build object description from literals and var-id's
    *-------------------------------------------------------------*/
   obj = DBMakeObject(name, DB_MULTIVAR, 32);
   DBAddIntComponent(obj, "nvars", nvars);
   DBAddIntComponent(obj, "ngroups", _mm._ngroups);
   DBAddIntComponent(obj, "blockorigin", _mm._blockorigin);
   DBAddIntComponent(obj, "grouporigin", _mm._grouporigin);
   if (_mm._guihide)
      DBAddIntComponent(obj, "guihide", _mm._guihide);
   if (_mm._region_pnames != NULL)
   {
        char *s=0; int len=0; long llen;
        DBStringArrayToStringList((char const * const *)_mm._region_pnames, -1, &s, &len);
        llen = len;
        DBWriteComponent(dbfile, obj, "region_pnames", name, "char", s, 1, &llen);
        FREE(s);
   }
   if (_mm._tensor_rank)
      DBAddIntComponent(obj, "tensor_rank", _mm._tensor_rank);
   if (_mm._mmesh_name != NULL)
      DBAddStrComponent(obj, "mmesh_name", _mm._mmesh_name);
   if (_mm._conserved)
      DBAddIntComponent(obj, "conserved", _mm._conserved);
   if (_mm._extensive)
      DBAddIntComponent(obj, "extensive", _mm._extensive);

   /*-------------------------------------------------------------
    *  Define and write variables before adding them to object.
    *-------------------------------------------------------------*/
   count[0] = nvars;
   if (vartypes)
       DBWriteComponent(dbfile, obj, "vartypes", name, "integer",
                        vartypes, 1, count);

    /* Compute size needed for string of concatenated block names.
     *
     * Note that we start with 2 so that we have one `;' and the NULL
     * terminator.  Also, the +1 in the "len +=" line is for the `;'
     * character.
     */
    if (varnames && nvars>0)
    {
        len = 2;
        for(i=0; i<nvars; i++)
        {
            len += strlen(varnames[i]) + 1;
        }
        tmp = ALLOC_N(char,len);

        /* Build 1-D character string from 2-D mesh-name array */
        tmp[0] = ';';
        tmp[1] = '\0';

        cur = tmp+1;
        for (i = 0; i < nvars; i++) {
           int len2;
           len2 = strlen(varnames[i]);
           strncpy(cur, varnames[i], len2);
           cur += len2;
           strncpy(cur, ";", 1);
           cur += 1;
        }

        count[0] = (long) (cur - tmp);
        DBWriteComponent(dbfile, obj, "varnames", name, "char",
                    tmp, 1, count);
    }

   /*-------------------------------------------------------------
    *  Define and write the time and cycle.
    *-------------------------------------------------------------*/
   DBAddIntComponent(obj, "cycle", _mm._cycle);

   if (_mm._time_set == TRUE)
      DBAddVarComponent(obj, "time", _mm._nm_time);
   if (_mm._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _mm._nm_dtime);

   /*-------------------------------------------------------------
    *  Add the DBOPT_EXTENTS_SIZE and DBOPT_EXTENTS options if present.
    *-------------------------------------------------------------*/
   if (_mm._extents != NULL && _mm._extentssize > 0) {
      DBAddIntComponent(obj, "extentssize", _mm._extentssize);

      count[0] = _mm._extentssize * nvars;
      DBWriteComponent(dbfile, obj, "extents", name, "double", _mm._extents,
                       1, count);
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_MB_... options if present.
    *-------------------------------------------------------------*/
   if (_mm._file_ns)
   { 
      count[0] = strlen(_mm._file_ns)+1;
      DBWriteComponent(dbfile, obj, "file_ns", name, "char",
                    _mm._file_ns, 1, count);
   }
   if (_mm._block_ns)
   { 
      count[0] = strlen(_mm._block_ns)+1;
      DBWriteComponent(dbfile, obj, "block_ns", name, "char",
                    _mm._block_ns, 1, count);
   }
   if (_mm._block_type)
      DBAddIntComponent(obj, "block_type", _mm._block_type);
   if (_mm._empty_list && _mm._empty_cnt>0) {
      DBAddIntComponent(obj, "empty_cnt", _mm._empty_cnt);
      count[0] = _mm._empty_cnt;
      DBWriteComponent(dbfile, obj, "empty_list", name, "integer", 
                       _mm._empty_list, 1, count);
   }
   if (_mm._repr_block_idx)
      DBAddIntComponent(obj, "repr_block_idx", _mm._repr_block_idx);

   if (_mm._missing_value != DB_MISSING_VALUE_NOT_SET)
   {
      if (_mm._missing_value == 0.0)
          DBAddDblComponent(obj, "missing_value", DB_MISSING_VALUE_NOT_SET);
      else
          DBAddDblComponent(obj, "missing_value", _mm._missing_value);
   }

   /*-------------------------------------------------------------
    *  Write multi-var object to SILO file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   FREE(tmp);

   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 * Routine                                            db_pdb_PutMultimat
 *
 * Purpose
 *
 *    Write a multi-material object into the open SILO file.
 *
 * Programmer
 *
 *    robb@cloud
 *    Tue Feb 21 12:42:07 EST 1995
 *
 * Notes
 *
 * Modified
 *    Eric Brugger, Fri Jan 12 17:42:55 PST 1996
 *    I added cycle, time and dtime as options.
 *
 *    Sean Ahern, Thu Aug 15 11:16:18 PDT 1996
 *    Allowed the mesh names to be any length, rather than hardcoded.
 *
 *    Eric Brugger, Fri Oct 17 09:11:58 PDT 1997
 *    I corrected the outputing of the cyle, time and dtime options to
 *    use the values from the global _mm instead of _pm.  A cut
 *    and paste error.  I added code to write out the DPOPT_NMATNOS
 *    and DPOPT_MATNOS options if provided.
 *
 *    Jeremy Meredith, Sept 23 1998
 *    Corrected a bug where 'browser' wouldn't read "matnos": changed
 *    type used when writing "matnos" from "int" to "integer".
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added ngroups, blockorigin, and grouporigin.
 *
 *    Hank Childs, Thu Jan  6 13:51:22 PST 2000
 *    Cast a strlen to long to avoid a compiler warning.
 *
 *    Hank Childs, Wed Apr 11 08:05:24 PDT 2001
 *    Concatenate strings more intelligently [HYPer02535].
 *
 *    Hank Childs, Mon May 14 14:27:29 PDT 2001
 *    Fixed bug where there was an assumption that the string is
 *    NULL terminated.
 *
 *    Mark C. Miller, Mon Aug  7 17:03:51 PDT 2006
 *    Added matnames and matcolors options
 *
 *   Mark C. Miller, Wed Jul 14 20:40:55 PDT 2010
 *   Added support for namescheme/empty list options.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutMultimat (DBfile *dbfile, char const *name, int nmats,
                    char const * const *matnames, DBoptlist const *optlist) {

   int            i, len;
   long           count[3];
   char          *tmp = NULL, *cur = NULL;
   DBobject      *obj;

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   db_InitMulti(dbfile, optlist);

   /*-------------------------------------------------------------
    *  Build object description from literals and var-id's
    *-------------------------------------------------------------*/
   obj = DBMakeObject(name, DB_MULTIMAT, 31);
   DBAddIntComponent(obj, "nmats", nmats);
   DBAddIntComponent(obj, "ngroups", _mm._ngroups);
   DBAddIntComponent(obj, "blockorigin", _mm._blockorigin);
   DBAddIntComponent(obj, "grouporigin", _mm._grouporigin);
   if (_mm._allowmat0)
      DBAddIntComponent(obj, "allowmat0", _mm._allowmat0);
   if (_mm._guihide)
      DBAddIntComponent(obj, "guihide", _mm._guihide);
   if (_mm._mmesh_name != NULL)
      DBAddStrComponent(obj, "mmesh_name", _mm._mmesh_name);

   /*-------------------------------------------------------------
    *  Define and write materials before adding them to object.
    *-------------------------------------------------------------*/

    /* Compute size needed for string of concatenated block names.
     *
     * Note that we start with 2 so that we have one `;' and the NULL
     * terminator.  Also, the +1 in the "len +=" line is for the `;'
     * character.
     */
    if (matnames && nmats>0)
    {
        len = 2;
        for(i=0; i<nmats; i++)
        {
            len += strlen(matnames[i]) + 1;
        }
        tmp = ALLOC_N(char,len);

       /* Build 1-D character string from 2-D mesh-name array */
       tmp[0] = ';';
       tmp[1] = '\0';

       cur = tmp+1;
       for (i = 0; i < nmats; i++) {
          int len2;
          len2 = strlen(matnames[i]);
          strncpy(cur, matnames[i], len2);
          cur += len2;
          strncpy(cur, ";", 1);
          cur += 1;
       }

       count[0] = (long) (cur - tmp);
       DBWriteComponent(dbfile, obj, "matnames", name, "char",
                    tmp, 1, count);
    }

   /*-------------------------------------------------------------
    *  Define and write the time and cycle.
    *-------------------------------------------------------------*/
   DBAddIntComponent(obj, "cycle", _mm._cycle);

   if (_mm._time_set == TRUE)
      DBAddVarComponent(obj, "time", _mm._nm_time);
   if (_mm._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _mm._nm_dtime);

   /*-------------------------------------------------------------
    *  Add the DBOPT_MATNOS and DBOPT_NMATNOS options if present.
    *-------------------------------------------------------------*/
   if (_mm._matnos != NULL && _mm._nmatnos > 0) {
      DBAddIntComponent(obj, "nmatnos", _mm._nmatnos);

      count[0] = _mm._nmatnos;
      DBWriteComponent(dbfile, obj, "matnos", name, "integer", _mm._matnos,
                       1, count);
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_MIXLENS option if present.
    *-------------------------------------------------------------*/
   if (_mm._mixlens != NULL) {

      count[0] = nmats; 
      DBWriteComponent(dbfile, obj, "mixlens", name, "integer", _mm._mixlens,
                       1, count);
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_MATCOUNTS and DBOPT_MATLISTS options if present.
    *-------------------------------------------------------------*/
   if (_mm._matcounts != NULL && _mm._matlists != NULL) {
      long tot = 0;
      for (i = 0; i < nmats; i++)
         tot += _mm._matcounts[i];

      if (tot) {
         count[0] = nmats; 
         DBWriteComponent(dbfile, obj, "matcounts", name, "integer", _mm._matcounts, 1, count);
         DBWriteComponent(dbfile, obj, "matlists", name, "integer", _mm._matlists, 1, &tot);
      }
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_MATNAMES option if present
    *-------------------------------------------------------------*/
   if (_mm._matnames && _mm._nmatnos > 0) {
      int len; long llen; char *tmpstr = 0;
      DBStringArrayToStringList((char const * const *)_mm._matnames, _mm._nmatnos,
          &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "material_names", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
      _mm._matnames = 0;
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_MATCOLORS option if present
    *-------------------------------------------------------------*/
   if (_mm._matcolors && _mm._nmatnos > 0) {
      int len; long llen; char *tmpstr = 0;
      DBStringArrayToStringList((char const * const *)_mm._matcolors, _mm._nmatnos,
          &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "matcolors", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
      _mm._matcolors = 0;
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_MB_... options if present.
    *-------------------------------------------------------------*/
   if (_mm._file_ns)
   { 
      count[0] = strlen(_mm._file_ns)+1;
      DBWriteComponent(dbfile, obj, "file_ns", name, "char",
                    _mm._file_ns, 1, count);
   }
   if (_mm._block_ns)
   { 
      count[0] = strlen(_mm._block_ns)+1;
      DBWriteComponent(dbfile, obj, "block_ns", name, "char",
                    _mm._block_ns, 1, count);
   }
   if (_mm._empty_list && _mm._empty_cnt>0) {
      DBAddIntComponent(obj, "empty_cnt", _mm._empty_cnt);
      count[0] = _mm._empty_cnt;
      DBWriteComponent(dbfile, obj, "empty_list", name, "integer", 
                       _mm._empty_list, 1, count);
   }
   if (_mm._repr_block_idx)
      DBAddIntComponent(obj, "repr_block_idx", _mm._repr_block_idx);

   /*-------------------------------------------------------------
    *  Write multi-material object to SILO file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   FREE(tmp);

   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 * Routine                                     db_pdb_PutMultimatspecies
 *
 * Purpose
 *
 *    Write a multi-species object into the open SILO file.
 *
 * Programmer
 *    Jeremy S. Meredith
 *    Sept 17 1998
 *
 * Notes
 *
 * Modified
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added ngroups, blockorigin, and grouporigin.
 *
 *    Hank Childs, Thu Jan  6 13:51:22 PST 2000
 *    Cast a strlen to long to avoid a compiler warning.
 *
 *    Jeremy Meredith, Thu Feb 10 16:00:53 PST 2000
 *    Fixed an off-by-one error on the max number of components.
 *
 *    Hank Childs, Wed Apr 11 08:05:24 PDT 2001
 *    Concatenate strings more intelligently [HYPer02535].
 *
 *    Hank Childs, Mon May 14 14:27:29 PDT 2001
 *    Fixed bug where there was an assumption that the string is
 *    NULL terminated.
 *
 *    Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *    Added names and colors for species.
 *
 *   Mark C. Miller, Wed Jul 14 20:40:55 PDT 2010
 *   Added support for namescheme/empty list options.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutMultimatspecies (DBfile *dbfile, char const *name, int nspec,
                    char const * const *specnames, DBoptlist const *optlist) {

   int            i, len, nstrs;
   long           count[3];
   char          *tmp = NULL, *cur = NULL;
   DBobject      *obj;

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   db_InitMulti(dbfile, optlist);

   /*-------------------------------------------------------------
    *  Build object description from literals and var-id's
    *-------------------------------------------------------------*/
   obj = DBMakeObject(name, DB_MULTIMATSPECIES, 31);
   DBAddIntComponent(obj, "nspec", nspec);
   DBAddIntComponent(obj, "ngroups", _mm._ngroups);
   DBAddIntComponent(obj, "blockorigin", _mm._blockorigin);
   DBAddIntComponent(obj, "grouporigin", _mm._grouporigin);
   if (_mm._guihide)
      DBAddIntComponent(obj, "guihide", _mm._guihide);

   /*-------------------------------------------------------------
    *  Define and write species before adding them to object.
    *-------------------------------------------------------------*/

    /* Compute size needed for string of concatenated block names.
     *
     * Note that we start with 2 so that we have one `;' and the NULL
     * terminator.  Also, the +1 in the "len +=" line is for the `;'
     * character.
     */
    if (specnames && nspec>0)
    {
        len = 2;
        for(i=0; i<nspec; i++)
        {
            len += strlen(specnames[i]) + 1;
        }
        tmp = ALLOC_N(char,len);

       /* Build 1-D character string from 2-D mesh-name array */
       tmp[0] = ';';
       tmp[1] = '\0';

       cur = tmp+1;
       for (i = 0; i < nspec; i++) {
          int len2;
          len2 = strlen(specnames[i]);
          strncpy(cur, specnames[i], len2);
          cur += len2;
          strncpy(cur, ";", 1);
          cur += 1;
       }

       count[0] = (long) (cur - tmp);
       DBWriteComponent(dbfile, obj, "specnames", name, "char",
                    tmp, 1, count);
    }

   /*-------------------------------------------------------------
    *  Define and write the time and cycle.
    *-------------------------------------------------------------*/
   DBAddIntComponent(obj, "cycle", _mm._cycle);

   if (_mm._time_set == TRUE)
      DBAddVarComponent(obj, "time", _mm._nm_time);
   if (_mm._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _mm._nm_dtime);

   /*-------------------------------------------------------------
    *  Write the DBOPT_MATNAME, _NMAT, and _NMATSPEC to the file.
    *-------------------------------------------------------------*/
   if (_mm._matname != NULL)
      DBAddStrComponent(obj, "matname", _mm._matname);

   if (_mm._nmat > 0 && _mm._nmatspec != NULL) {
      DBAddIntComponent(obj, "nmat", _mm._nmat);

      count[0]=_mm._nmat;
      DBWriteComponent(dbfile, obj, "nmatspec", name, "integer", _mm._nmatspec,
                       1, count);

      /* If we have species names or colors, write them out */
      nstrs = 0;
      if (_mm._specnames != NULL)
      {
         int len; long llen; char *tmpstr = 0;

         /* count how many names we have */
         for (i=0; i < _mm._nmat; i++)
             nstrs += _mm._nmatspec[i];
         DBStringArrayToStringList((char const * const *)_mm._specnames, nstrs, &tmpstr, &len);
         llen = (long) len;
         DBWriteComponent(dbfile, obj, "species_names", name, "char", tmpstr, 1, &llen);
         FREE(tmpstr);
      }
      if (_mm._speccolors != NULL)
      {
         int len; long llen; char *tmpstr = 0;

         /* count how many names we have */
         if (nstrs == 0)
         {
             for (i=0; i < _mm._nmat; i++)
                 nstrs += _mm._nmatspec[i];
         }
         DBStringArrayToStringList((char const * const *)_mm._speccolors, nstrs, &tmpstr, &len);
         llen = (long) len;
         DBWriteComponent(dbfile, obj, "speccolors", name, "char", tmpstr, 1, &llen);
         FREE(tmpstr);
      }
   }

   /*-------------------------------------------------------------
    *  Add the DBOPT_MB_... options if present.
    *-------------------------------------------------------------*/
   if (_mm._file_ns)
   { 
      count[0] = strlen(_mm._file_ns)+1;
      DBWriteComponent(dbfile, obj, "file_ns", name, "char",
                    _mm._file_ns, 1, count);
   }
   if (_mm._block_ns)
   { 
      count[0] = strlen(_mm._block_ns)+1;
      DBWriteComponent(dbfile, obj, "block_ns", name, "char",
                    _mm._block_ns, 1, count);
   }
   if (_mm._empty_list && _mm._empty_cnt>0) {
      DBAddIntComponent(obj, "empty_cnt", _mm._empty_cnt);
      count[0] = _mm._empty_cnt;
      DBWriteComponent(dbfile, obj, "empty_list", name, "integer", 
                       _mm._empty_list, 1, count);
   }
   if (_mm._repr_block_idx)
      DBAddIntComponent(obj, "repr_block_idx", _mm._repr_block_idx);

   /*-------------------------------------------------------------
    *  Write multi-species object to SILO file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   FREE(tmp);

   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 *  Routine                                         db_pdb_PutPointmesh
 *
 *  Purpose
 *
 *      Write a point mesh object into the open output file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modifications
 *
 *      Al Leibee, Mon Apr 18 07:45:58 PDT 1994
 *      Added _dtime.
 *
 *      Robb Matzke, Fri Dec 2 14:16:41 PST 1994
 *      Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *      Robb Matzke, Sun Dec 18 17:07:34 EST 1994
 *      Changed SW_GetDatatypeString to db_GetDatatypeString and
 *      removed associated memory leak.
 *
 *      Sean Ahern, Sun Oct  1 03:15:18 PDT 1995
 *      Made "me" static.
 *
 *      Robb Matzke, 11 Nov 1997
 *      Added `datatype' to the file.
 *
 *      Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *      Added group_no.
 *
 *      Eric Brugger, Fri Mar  8 12:23:35 PST 2002
 *      I modified the routine to output min and max extents as either
 *      float or doubles, depending on the datatype of the coordinates.
 *
 *      Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *      Add support for long long global node/zone numbers.
 *
 *      Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *      Changed how long long global node/zone numbers are supported
 *      from a int (bool), "llong_gnode|zoneno" to an int holding
 *      the actual datatype. The type is assumed int if it its
 *      value is zero or it does not exist. Otherwise, the type is
 *      is whatever is stored in gnznodtype member. 
 *
 *      Mark C. Miller, Tue Nov 17 22:22:09 PST 2009
 *      Changed name of long long datatype to match what PDB proper
 *      would call it.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutPointmesh (DBfile *dbfile, char const *name, int ndims, DBVCP2_t _coords,
                     int nels, int datatype, DBoptlist const *optlist) {

   int            i;
   long           count[3];
   DBobject      *obj;
   char          *datatype_str, tmp[1024];
   float          fmin_extents[3], fmax_extents[3];
   double         dmin_extents[3], dmax_extents[3];
   static char   *me = "db_pdb_PutPointmesh";
   void const * const *coords = (void const * const *) _coords;

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   db_InitPoint(dbfile, optlist, ndims, nels);
   obj = DBMakeObject(name, DB_POINTMESH, 31);

   /*-------------------------------------------------------------
    *  Write coordinate arrays.
    *-------------------------------------------------------------*/
   datatype_str = db_GetDatatypeString(datatype);
   count[0] = nels;
   for (i = 0; i < ndims && nels; i++) {
      sprintf(tmp, "coord%d", i);
      DBWriteComponent(dbfile, obj, tmp, name, datatype_str,
                       coords[i], 1, count);
   }
   FREE(datatype_str);

   /*-------------------------------------------------------------
    *  Find the mesh extents from the coordinate arrays. Write
    *  them out to output file.
    *-------------------------------------------------------------*/
   count[0] = ndims;
   if (ndims && nels) {
      switch (datatype) {
      case DB_FLOAT:
         switch (ndims) {
         case 3:
            _DBarrminmax((float*)coords[2], nels, &fmin_extents[2], &fmax_extents[2]);
         case 2:
            _DBarrminmax((float*)coords[1], nels, &fmin_extents[1], &fmax_extents[1]);
         case 1:
            _DBarrminmax((float*)coords[0], nels, &fmin_extents[0], &fmax_extents[0]);
            break;
         default:
            return db_perror("ndims", E_BADARGS, me);
         }

         DBWriteComponent(dbfile, obj, "min_extents", name, "float",
                          fmin_extents, 1, count);
         DBWriteComponent(dbfile, obj, "max_extents", name, "float",
                          fmax_extents, 1, count);

         break;
      case DB_DOUBLE:
         switch (ndims) {
         case 3:
            _DBdarrminmax((double *)coords[2], nels,
                          &dmin_extents[2], &dmax_extents[2]);
         case 2:
            _DBdarrminmax((double *)coords[1], nels,
                          &dmin_extents[1], &dmax_extents[1]);
         case 1:
            _DBdarrminmax((double *)coords[0], nels,
                          &dmin_extents[0], &dmax_extents[0]);
            break;
         default:
            return db_perror("ndims", E_BADARGS, me);
         }

         DBWriteComponent(dbfile, obj, "min_extents", name, "double",
                          dmin_extents, 1, count);
         DBWriteComponent(dbfile, obj, "max_extents", name, "double",
                          dmax_extents, 1, count);

         break;
      default:
         return db_perror("type not supported", E_NOTIMP, me);
      }
   }

   if (nels>0 && _pm._gnodeno)
   {
       count[0] = nels;
       if (_pm._llong_gnodeno)
           DBWriteComponent(dbfile, obj, "gnodeno", name, "long_long",
               _pm._gnodeno, 1, count);
       else
           DBWriteComponent(dbfile, obj, "gnodeno", name, "integer",
               _pm._gnodeno, 1, count);
   }

   /*-------------------------------------------------------------
    *  Build a SILO object definition for a point mesh. The minimum
    *  required information for a point mesh is the coordinate arrays,
    *  the number of dimensions, and the type (RECT/CURV). Process
    *  the provided options to complete the definition.
    *
    *  The SILO object definition is composed of a string of delimited
    *  component names plus an array of SILO *identifiers* of
    *  previously defined variables.
    *-------------------------------------------------------------*/
   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nspace", _pm._nspace);
   DBAddIntComponent(obj, "nels", _pm._nels);
   DBAddIntComponent(obj, "cycle", _pm._cycle);
   DBAddIntComponent(obj, "origin", _pm._origin);
   DBAddIntComponent(obj, "min_index", _pm._minindex);
   DBAddIntComponent(obj, "max_index", _pm._maxindex);
   DBAddIntComponent(obj, "datatype", datatype);
   if (_pm._llong_gnodeno)
       DBAddIntComponent(obj, "gnznodtype", DB_LONG_LONG);
   if (_pm._guihide)
       DBAddIntComponent(obj, "guihide", _pm._guihide);
   if (_pm._group_no >= 0)
       DBAddIntComponent(obj, "group_no", _pm._group_no);
   if (_pm._time_set == TRUE)
      DBAddVarComponent(obj, "time", _pm._nm_time);
   if (_pm._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _pm._nm_dtime);

   /*-------------------------------------------------------------
    *  Process character strings: labels & units for x, y, &/or z,
    *-------------------------------------------------------------*/
   if (_pm._labels[0] != NULL)
      DBAddStrComponent(obj, "label0", _pm._labels[0]);

   if (_pm._labels[1] != NULL)
      DBAddStrComponent(obj, "label1", _pm._labels[1]);

   if (_pm._labels[2] != NULL)
      DBAddStrComponent(obj, "label2", _pm._labels[2]);

   if (_pm._units[0] != NULL)
      DBAddStrComponent(obj, "units0", _pm._units[0]);

   if (_pm._units[1] != NULL)
      DBAddStrComponent(obj, "units1", _pm._units[1]);

   if (_pm._units[2] != NULL)
      DBAddStrComponent(obj, "units2", _pm._units[2]);

   if (_pm._mrgtree_name != NULL)
      DBAddStrComponent(obj, "mrgtree_name", _pm._mrgtree_name);

   if (nels>0 && _pm._ghost_node_labels != NULL)
   {
      count[0] = nels;
      DBWriteComponent(dbfile, obj, "ghost_node_labels", name, "char",
          _pm._ghost_node_labels, 1, count);
   }

   if (nels>0 && _pm._alt_nodenum_vars)
   {
      int nvars=-1, len; long llen; char *tmpstr = 0;
      DBStringArrayToStringList((char const * const *)_pm._alt_nodenum_vars, nvars, &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "alt_nodenum_vars", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
   }

   /*-------------------------------------------------------------
    *  Write point-mesh object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return (OKAY);
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_PutPointvar
 *
 *  Purpose
 *
 *      Write a point variable object into the open output file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modifications
 *
 *      Robb Matzke, Sun Dec 18 17:08:42 EST 1994
 *      Changed SW_GetDatatypeString to db_GetDatatypeString and removed
 *      associated memory leak.
 *
 *      Robb Matzke, Fri Dec 2 14:17:15 PST 1994
 *      Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *      Al Leibee, Mon Apr 18 07:45:58 PDT 1994
 *      Added _dtime.
 *
 *      Sean Ahern, Tue Mar 24 17:23:04 PST 1998
 *      Bumped up the min number of attributes so that we can write out 3D
 *      point meshes.
 *
 *      Mark C. Miller, Tue Sep  6 11:05:58 PDT 2005
 *      Removed duplicate DBAddStr call for "meshid"
 *
 *      Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *      Added support for conserved/extensive options.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutPointvar (DBfile *dbfile, char const *name, char const *meshname, int nvars,
                    DBVCP2_t _vars, int nels, int datatype,
                    DBoptlist const *optlist) {

   int            i;
   long           count[3];
   DBobject      *obj;
   char          *datatype_str;
   char           tmp[1024];
   void const * const *vars = (void const * const *) _vars;

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   db_InitPoint(dbfile, optlist, _pm._ndims, nels);

   obj = DBMakeObject(name, DB_POINTVAR, nvars+20);

   /*-------------------------------------------------------------
    *  Write variable arrays.
    *  Set index variables and counters.
    *-----------------------------------------------------------*/
   if (nels>0 && nvars>0)
   {
      datatype_str = db_GetDatatypeString(datatype);
      count[0] = nels;
      if (nvars == 1) {
         DBWriteComponent(dbfile, obj, "_data", name, datatype_str,
                          vars[0], 1, count);
      }
      else {
         for (i = 0; i < nvars; i++) {
            sprintf(tmp, "%d_data", i);
            DBWriteComponent(dbfile, obj, tmp, name, datatype_str,
                             vars[i], 1, count);
         }
      }
      FREE(datatype_str);
   }

   /*-------------------------------------------------------------
    *  Build a SILO object definition for a point var. The
    *  minimum required information for a point var is the variable
    *  itself, plus the ID for the associated point mesh object.
    *  Process any additional options to complete the definition.
    *-------------------------------------------------------------*/

   DBAddStrComponent(obj, "meshid", meshname);
   if (_pm._time_set == TRUE)
      DBAddVarComponent(obj, "time", _pm._nm_time);
   if (_pm._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _pm._nm_dtime);

   DBAddIntComponent(obj, "nvals", nvars);
   DBAddIntComponent(obj, "nels", nels);
   DBAddIntComponent(obj, "ndims", _pm._ndims);
   DBAddIntComponent(obj, "datatype", datatype);
   DBAddIntComponent(obj, "nspace", _pm._nspace);
   DBAddIntComponent(obj, "origin", _pm._origin);
   DBAddIntComponent(obj, "cycle", _pm._cycle);
   DBAddIntComponent(obj, "min_index", _pm._minindex);
   DBAddIntComponent(obj, "max_index", _pm._maxindex);
   if (_pm._guihide)
      DBAddIntComponent(obj, "guihide", _pm._guihide);
   if (_pm._ascii_labels)
      DBAddIntComponent(obj, "ascii_labels", _pm._ascii_labels);

   /*-------------------------------------------------------------
    *  Process character strings: labels & units for variable.
    *-------------------------------------------------------------*/
   if (_pm._label != NULL)
      DBAddStrComponent(obj, "label", _pm._label);

   if (_pm._unit != NULL)
      DBAddStrComponent(obj, "units", _pm._unit);

   if (_pm._region_pnames != NULL)
   {
        char *s=0; int len=0; long llen;
        DBStringArrayToStringList((char const * const *)_pm._region_pnames, -1, &s, &len);
        llen = len;
        DBWriteComponent(dbfile, obj, "region_pnames", name, "char", s, 1, &llen);
        FREE(s);
   }

   if (_pm._conserved)
      DBAddIntComponent(obj, "conserved", _pm._conserved);

   if (_pm._extensive)
      DBAddIntComponent(obj, "extensive", _pm._extensive);

   if (_pm._missing_value != DB_MISSING_VALUE_NOT_SET)
   {
      if (_pm._missing_value == 0.0)
          DBAddDblComponent(obj, "missing_value", DB_MISSING_VALUE_NOT_SET);
      else
          DBAddDblComponent(obj, "missing_value", _pm._missing_value);
   }

   /*-------------------------------------------------------------
    *  Write point-mesh object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, 0);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 *  Routine                                           db_pdb_PutQuadmesh
 *
 *  Purpose
 *
 *      Write a quad mesh object into the open SILO file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modifications
 *
 *     Robb Matzke, Sun Dec 18 17:10:00 EST 1994
 *     Changed SW_GetDatatypeString to db_GetDatatypeString and removed
 *     associated memory leak.
 *
 *     Robb Matzke, Fri Dec 2 14:17:56 PST 1994
 *     Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *     Robb Matzke, Wed Nov 23 12:00:20 EST 1994
 *     Increased third argument of DBMakeObject from 25 to 40 because
 *     we were running out of room.  Thanks to the new error mechanism,
 *     we actually got a message for this!
 *
 *     Al Leibee, Sun Apr 17 07:54:25 PDT 1994
 *     Added dtime.
 *
 *     Sean Ahern, Fri Oct 16 17:48:23 PDT 1998
 *     Reformatted whitespace.
 *
 *     Sean Ahern, Tue Oct 20 17:22:58 PDT 1998
 *     Changed the extents so that they are written out in the datatype that
 *     is passed to the call.
 *
 *     Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *     Added group_no and base_index[].
 *
 *     Hank Childs, Thu Jan  6 13:48:40 PST 2000
 *     Put in lint directive for unused argument.
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
/* ARGSUSED */
SILO_CALLBACK int
db_pdb_PutQuadmesh (DBfile *dbfile, char const *name, char const * const *coordnames,
                    DBVCP2_t _coords, int const *dims, int ndims, int datatype,
                    int coordtype, DBoptlist const *optlist)
{
    int             i;
    long            count[3];
    int             nd;
    char           *datatype_str;
    DBobject       *obj;
    char            tmp[1024];
    void const * const *coords = (void const * const *) _coords;
    int             is_empty = 1;

    /* The following is declared as double for worst case. */
    double         min_extents[3]={0,0,0}, max_extents[3]={0,0,0};

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
    db_InitQuad(dbfile, name, optlist, dims, ndims);
    obj = DBMakeObject(name, coordtype, 44);

   /*-------------------------------------------------------------
    *  Write coordinate arrays.
    *-------------------------------------------------------------*/
    for (i = 0; i < ndims; i++)
    {
        if (dims[i] > 0)
        {
            is_empty = 0;
            break;
        }
    }

    for (i = 0; i < ndims; i++)
        count[i] = dims[i];
    if (coordtype == DB_COLLINEAR)
        nd = 1;
    else
        nd = ndims;

    datatype_str = db_GetDatatypeString(datatype);
    for (i = 0; (i < ndims) && !is_empty; i++)
    {
        if (coordtype == DB_COLLINEAR)
            count[0] = dims[i];

        /* Do coordinate array, in form: coordN, where n = 0, 1, ... */
        sprintf(tmp, "coord%d", i);
        DBWriteComponent(dbfile, obj, tmp, name, datatype_str,
                         coords[i], nd, count);
    }

   /*-------------------------------------------------------------
    *  Find the mesh extents from the coordinate arrays. Write
    *  them out to SILO file.
    *-------------------------------------------------------------*/

    _DBQMCalcExtents(coords, datatype, _qm._minindex, _qm._maxindex_n, dims,
                     ndims, coordtype, min_extents, max_extents);

    count[0] = ndims;
    DBWriteComponent(dbfile, obj, "min_extents", name, datatype_str,
                     min_extents, 1, count);

    DBWriteComponent(dbfile, obj, "max_extents", name, datatype_str,
                     max_extents, 1, count);
    FREE(datatype_str);

   /*-------------------------------------------------------------
    *  Build a SILO object definition for a quad mesh. The minimum
    *  required information for a quad mesh is the coordinate arrays,
    *  the number of dimensions, and the type (RECT/CURV). Process
    *  the provided options to complete the definition.
    *
    *  The SILO object definition is composed of a string of delimited
    *  component names plus an array of internal PDB variable names.
    *-------------------------------------------------------------*/

    DBAddIntComponent(obj, "ndims", ndims);
    DBAddIntComponent(obj, "coordtype", coordtype);
    DBAddIntComponent(obj, "datatype", datatype);
    DBAddIntComponent(obj, "nspace", _qm._nspace);
    DBAddIntComponent(obj, "nnodes", _qm._nnodes);
    DBAddIntComponent(obj, "facetype", _qm._facetype);
    DBAddIntComponent(obj, "major_order", _qm._majororder);
    DBAddIntComponent(obj, "cycle", _qm._cycle);
    DBAddIntComponent(obj, "coord_sys", _qm._coord_sys);
    DBAddIntComponent(obj, "planar", _qm._planar);
    DBAddIntComponent(obj, "origin", _qm._origin);

   if (_qm._group_no >= 0)
       DBAddIntComponent(obj, "group_no", _qm._group_no);

    DBAddVarComponent(obj, "dims", _qm._nm_dims);
    DBAddVarComponent(obj, "min_index", _qm._nm_minindex);
    DBAddVarComponent(obj, "max_index", _qm._nm_maxindex_n);
    DBAddVarComponent(obj, "baseindex", _qm._nm_baseindex);

    if (_qm._time_set == TRUE)
        DBAddVarComponent(obj, "time", _qm._nm_time);
    if (_qm._dtime_set == TRUE)
        DBAddVarComponent(obj, "dtime", _qm._nm_dtime);

   /*-------------------------------------------------------------
    *  Process character strings: labels & units for x, y, &/or z,
    *-------------------------------------------------------------*/
    if (_qm._labels[0] != NULL)
        DBAddStrComponent(obj, "label0", _qm._labels[0]);

    if (_qm._labels[1] != NULL)
        DBAddStrComponent(obj, "label1", _qm._labels[1]);

    if (_qm._labels[2] != NULL)
        DBAddStrComponent(obj, "label2", _qm._labels[2]);

    if (_qm._units[0] != NULL)
        DBAddStrComponent(obj, "units0", _qm._units[0]);

    if (_qm._units[1] != NULL)
        DBAddStrComponent(obj, "units1", _qm._units[1]);

    if (_qm._units[2] != NULL)
        DBAddStrComponent(obj, "units2", _qm._units[2]);

    if (_qm._guihide)
        DBAddIntComponent(obj, "guihide", _qm._guihide);

    if (_qm._mrgtree_name != NULL)
        DBAddStrComponent(obj, "mrgtree_name", _qm._mrgtree_name);

    if (!is_empty && _qm._ghost_node_labels != NULL)
    {
        for (i = 0; i < ndims; i++)
            count[i] = dims[i];
        DBWriteComponent(dbfile, obj, "ghost_node_labels", name, "char",
                         _qm._ghost_node_labels, ndims, count);
    }

    if (!is_empty && _qm._ghost_zone_labels != NULL)
    {
        for (i = 0; i < ndims; i++)
            count[i] = dims[i]-1;
        DBWriteComponent(dbfile, obj, "ghost_zone_labels", name, "char",
                         _qm._ghost_zone_labels, ndims, count);
    }

    if (!is_empty && _qm._alt_nodenum_vars)
    {
       int nvars=-1, len; long llen; char *tmpstr = 0;
       DBStringArrayToStringList((char const * const *)_qm._alt_nodenum_vars, nvars, &tmpstr, &len);
       llen = (long) len;
       DBWriteComponent(dbfile, obj, "alt_nodenum_vars", name, "char", tmpstr, 1, &llen);
       FREE(tmpstr);
    }

    if (!is_empty && _qm._alt_zonenum_vars)
    {
       int nvars=-1, len; long llen; char *tmpstr = 0;
       DBStringArrayToStringList((char const * const *)_qm._alt_zonenum_vars, nvars, &tmpstr, &len);
       llen = (long) len;
       DBWriteComponent(dbfile, obj, "alt_zonenum_vars", name, "char", tmpstr, 1, &llen);
       FREE(tmpstr);
    }

   /*-------------------------------------------------------------
    *  Write quad-mesh object to SILO file.
    *-------------------------------------------------------------*/
    DBWriteObject(dbfile, obj, TRUE);
    DBFreeObject(obj);

    return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 *  Routine                                            db_pdb_PutQuadvar
 *
 *  Purpose
 *
 *      Write a quad variable object into the open SILO file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modifications
 *
 *      Al Leibee, Sun Apr 17 07:54:25 PDT 1994
 *      Added dtime.
 *
 *      Al Leibee, Wed Jul 20 07:56:37 PDT 1994
 *      Added write of mixlen component.
 *
 *      Al Leibee, Wed Aug  3 16:57:38 PDT 1994
 *      Added _use_specmf.
 *
 *      Robb Matzke, Fri Dec 2 14:18:34 PST 1994
 *      Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *      Robb Matzke, Sun Dec 18 17:11:12 EST 1994
 *      Changed SW_GetDatatypeString to db_GetDatatypeString and
 *      removed associated memory leak.
 *
 *      Sean Ahern, Sun Oct  1 03:16:19 PDT 1995
 *      Made "me" static.
 *
 *      Robb Matzke, 1 May 1996
 *      The `dims' is actually used.  Before we just used the
 *      dimension variable whose name was based on the centering.  Now
 *      we store the dimensions for every quadvar.
 *
 *      Robb Matzke, 19 Jun 1997
 *      Added the `ascii_labels' optional field.  The default is
 *      false, so we write `ascii_labels' to the file only if it isn't
 *      false.
 *
 *      Eric Brugger, Mon Oct  6 15:11:26 PDT 1997
 *      I modified the routine to output the maximum index properly.
 *
 *      Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *      Added support for conserved/extensive options. Also, split
 *      out the logic for centering and alignment to make clearer.
 *      Added logic to support edge/face centering -- need to
 *      multiply by ndims for each of i-,j- and k- associated edges
 *      or faces. In addition, needed to add a 'centering' member
 *      to quadvars.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutQuadvar (DBfile *_dbfile, char const *name, char const *meshname, int nvars,
                   char const * const *varnames, DBVCP2_t _vars, int const *dims, int ndims,
                   DBVCP2_t _mixvars, int mixlen, int datatype, int centering,
                   DBoptlist const *optlist) {

   DBfile_pdb   *dbfile = (DBfile_pdb *) _dbfile;
   int          i, nels;
   long         count[4], mcount[1];
   char         *suffix, *datatype_str, tmp1[1024], tmp2[1024];
   static char  *me = "db_pdb_PutQuadvar";
   DBobject     *obj;
   int          maxindex[3] ;
   void const * const    *vars = (void const * const *) _vars;
   void const * const *mixvars = (void const * const *) _mixvars;
   int          is_empty = 1;

   for (i = 0; i < ndims; i++)
   {
       if (dims[i] > 0)
       {
           is_empty = 0;
           break;
       }
   }

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   db_InitQuad(_dbfile, meshname, optlist, dims, ndims);

   obj = DBMakeObject(name, DB_QUADVAR, (1+(mixlen!=0))*nvars + 30);

   DBAddStrComponent(obj, "meshid", meshname);

   /*-------------------------------------------------------------
    *  Write variable arrays.
    *  Set index variables and counters.
    *-----------------------------------------------------------*/
   nels = !is_empty?1:0;
   for (i = 0; i < ndims; i++) {
      count[i] = dims[i];
      nels *= dims[i];
   }
   if ((ndims > 1 && centering == DB_EDGECENT) ||
       (ndims > 2 && centering == DB_FACECENT))
      nels *= ndims;

   switch (centering) {
   case DB_NODECENT:
      DBAddVarComponent(obj, "align", _qm._nm_alignn);
      break;

   case DB_EDGECENT:
      if (ndims == 1) /* edge centering on 1D mesh is like zone centering */
          DBAddVarComponent(obj, "align", _qm._nm_alignz);
      break;

   case DB_FACECENT:
      if (ndims == 2) /* face centering on 2D mesh is like zone centering */
          DBAddVarComponent(obj, "align", _qm._nm_alignz);
      break;

   case DB_ZONECENT:
      DBAddVarComponent(obj, "align", _qm._nm_alignz);
      break;

   default:
      return db_perror("centering", E_BADARGS, me);
   }

   /*
    * Dimensions
    */
   db_mkname (dbfile->pdb, name, "dims", tmp2) ;
   mcount[0] = ndims ;
   if (mcount[0])
   {
       PJ_write_len (dbfile->pdb, tmp2, "integer", dims, 1, mcount) ;
       DBAddVarComponent (obj, "dims", tmp2) ;
   }

   /*
    * Max indices
    */
   for (i=0; i<ndims; i++) maxindex[i] = dims[i] - _qm._hi_offset[i] - 1 ;
   db_mkname (dbfile->pdb, name, "maxindex", tmp2) ;
   mcount[0] = ndims ;
   if (mcount[0])
   {
       PJ_write_len (dbfile->pdb, tmp2, "integer", maxindex, 1, mcount) ;
       DBAddVarComponent (obj, "max_index", tmp2) ;
   }

   /*-------------------------------------------------------------
    *  We first will write the given variables to SILO, then
    *  we'll define a Quadvar object in SILO composed of the
    *  variables plus the given options.
    *-------------------------------------------------------------*/

   suffix = "data";

   datatype_str = db_GetDatatypeString(datatype);
   for (i = 0; i < nvars && !is_empty; i++) {

      db_mkname(dbfile->pdb, varnames[i], suffix, tmp2);
      if ((ndims > 1 && centering == DB_EDGECENT) ||
          (ndims > 2 && centering == DB_FACECENT))
      {
          int j, tmpndims = ndims+1;
          long tmpcnt[4] = {0,0,0,0};
          for (j = ndims; j > 0; j--)
              tmpcnt[j] = count[j-1];
          tmpcnt[0] = ndims;
          PJ_write_len(dbfile->pdb, tmp2, datatype_str, vars[i],
                       tmpndims, tmpcnt);
      }
      else
      {
          int k; long n=ndims==0?0:1;
          for (k = 0; k < ndims; n *= count[k++]);
          if (n)
              PJ_write_len(dbfile->pdb, tmp2, datatype_str, vars[i], ndims, count);
      }

      sprintf(tmp1, "value%d", i);
      DBAddVarComponent(obj, tmp1, tmp2);

      /* Write the mixed data component if present */
      if (mixvars != NULL && mixvars[i] != NULL && mixlen > 0) {
         mcount[0] = mixlen;

         db_mkname(dbfile->pdb, varnames[i], "mix", tmp2);
         PJ_write_len(dbfile->pdb, tmp2, datatype_str, mixvars[i],
                      1, mcount);

         sprintf(tmp1, "mixed_value%d", i);
         DBAddVarComponent(obj, tmp1, tmp2);
      }
   }
   FREE(datatype_str);

   /*-------------------------------------------------------------
    *  Build a SILO object definition for a quad mesh var. The
    *  minimum required information for a quad var is the variable
    *  itself, plus the ID for the associated quad mesh object.
    *  Process any additional options to complete the definition.
    *-------------------------------------------------------------*/

   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nvals", nvars);
   DBAddIntComponent(obj, "nels", nels);
   DBAddIntComponent(obj, "origin", _qm._origin);
   DBAddIntComponent(obj, "datatype", datatype);
   DBAddIntComponent(obj, "centering", centering);
   DBAddIntComponent(obj, "mixlen", mixlen);

   /*-------------------------------------------------------------
    * Add 'recommended' optional components.
    *-------------------------------------------------------------*/
   DBAddIntComponent(obj, "major_order", _qm._majororder);
   DBAddIntComponent(obj, "cycle", _qm._cycle);

   if (_qm._time_set == TRUE)
      DBAddVarComponent(obj, "time", _qm._nm_time);
   if (_qm._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _qm._nm_dtime);

   DBAddVarComponent(obj, "min_index", _qm._nm_minindex);
   DBAddIntComponent(obj, "use_specmf", _qm._use_specmf);
   if (_qm._ascii_labels) {
      DBAddIntComponent(obj, "ascii_labels", _qm._ascii_labels);
   }

   if (_qm._guihide)
      DBAddIntComponent(obj, "guihide", _qm._guihide);

   /*-------------------------------------------------------------
    *  Process character strings: labels & units for variable.
    *-------------------------------------------------------------*/
   if (_qm._label != NULL)
      DBAddStrComponent(obj, "label", _qm._label);

   if (_qm._unit != NULL)
      DBAddStrComponent(obj, "units", _qm._unit);

   if (_qm._region_pnames != NULL)
   {
        char *s=0; int len=0; long llen;
        DBStringArrayToStringList((char const * const *)_qm._region_pnames, -1, &s, &len);
        llen = len;
        DBWriteComponent(_dbfile, obj, "region_pnames", name, "char", s, 1, &llen);
        FREE(s);
   }

   if (_qm._conserved)
      DBAddIntComponent(obj, "conserved", _qm._conserved);

   if (_qm._extensive)
      DBAddIntComponent(obj, "extensive", _qm._extensive);


   if (_qm._missing_value != DB_MISSING_VALUE_NOT_SET)
   {
      if (_qm._missing_value == 0.0)
          DBAddDblComponent(obj, "missing_value", DB_MISSING_VALUE_NOT_SET);
      else
          DBAddDblComponent(obj, "missing_value", _qm._missing_value);
   }

   /*-------------------------------------------------------------
    *  Write quad-var object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(_dbfile, obj, 0);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                           db_pdb_PutCsgmesh
 *
 *  Purpose
 *
 *      Write a csg mesh object into the open output file.
 *
 *  Programmer
 *
 *      Mark C. Miller , Tue Aug  2 19:15:39 PDT 2005
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
/* ARGSUSED */
SILO_CALLBACK int
db_pdb_PutCsgmesh (DBfile *dbfile, char const *name, int ndims,
                   int nbounds,
                   int const *typeflags, int const *bndids,
                   void const *coeffs, int lcoeffs, int datatype,
                   double const *extents, char const *zlname,
                   DBoptlist const *optlist) {

   long           count[3];
   DBobject      *obj;
   char          *datatype_str;
   double         min_extents[3], max_extents[3];

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   db_InitCsg(dbfile, name, optlist);

   obj = DBMakeObject(name, DB_CSGMESH, 34);

   count[0] = nbounds;

   if (nbounds)
       DBWriteComponent(dbfile, obj, "typeflags", name, "integer", typeflags, 1, count);

   if (nbounds && bndids)
       DBWriteComponent(dbfile, obj, "bndids", name, "integer", bndids, 1, count);

   datatype_str = db_GetDatatypeString(datatype);
   count[0] = lcoeffs;
   if (nbounds)
       DBWriteComponent(dbfile, obj, "coeffs", name, datatype_str, coeffs, 1, count);
   FREE(datatype_str);

   if (extents)
   {
      if (ndims == 3)
      {
          min_extents[0] = extents[0];
          min_extents[1] = extents[1];
          min_extents[2] = extents[2];
          max_extents[0] = extents[3];
          max_extents[1] = extents[4];
          max_extents[2] = extents[5];
      }
      else if (ndims == 2)
      {
          min_extents[0] = extents[0];
          min_extents[1] = extents[1];
          max_extents[0] = extents[2];
          max_extents[1] = extents[3];
      }
      else if (ndims == 1)
      {
          min_extents[0] = extents[0];
          max_extents[0] = extents[1];
      }

      count[0] = ndims;
      DBWriteComponent(dbfile, obj, "min_extents", name, "double",
                       min_extents, 1, count);
      DBWriteComponent(dbfile, obj, "max_extents", name, "double",
                       max_extents, 1, count);
   }

   /*-------------------------------------------------------------
    *  Build a output object definition for a ucd mesh. The minimum
    *  required information for a ucd mesh is the coordinate arrays,
    *  the number of dimensions, and the type (RECT/CURV). Process
    *  the provided options to complete the definition.
    *
    *  The output object definition is composed of a string of delimited
    *  component names plus an array of output *identifiers* of
    *  previously defined variables.
    *-------------------------------------------------------------*/

   if (zlname)
      DBAddStrComponent(obj, "csgzonelist", zlname);

   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nbounds", nbounds);
   DBAddIntComponent(obj, "cycle", _csgm._cycle);
   DBAddIntComponent(obj, "datatype", datatype);
   DBAddIntComponent(obj, "lcoeffs", lcoeffs);
   if (_csgm._guihide)
       DBAddIntComponent(obj, "guihide", _csgm._guihide);

   if (_csgm._group_no >= 0)
       DBAddIntComponent(obj, "group_no", _csgm._group_no);

   if (_csgm._time_set == TRUE)
      DBAddVarComponent(obj, "time", _csgm._nm_time);
   if (_csgm._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _csgm._nm_dtime);

   /*-------------------------------------------------------------
    *  Process character strings: labels & units for x, y, &/or z,
    *-------------------------------------------------------------*/
   if (_csgm._labels[0] != NULL)
      DBAddStrComponent(obj, "label0", _csgm._labels[0]);

   if (_csgm._labels[1] != NULL)
      DBAddStrComponent(obj, "label1", _csgm._labels[1]);

   if (_csgm._labels[2] != NULL)
      DBAddStrComponent(obj, "label2", _csgm._labels[2]);

   if (_csgm._units[0] != NULL)
      DBAddStrComponent(obj, "units0", _csgm._units[0]);

   if (_csgm._units[1] != NULL)
      DBAddStrComponent(obj, "units1", _csgm._units[1]);

   if (_csgm._units[2] != NULL)
      DBAddStrComponent(obj, "units2", _csgm._units[2]);

   if (_csgm._mrgtree_name != NULL)
      DBAddStrComponent(obj, "mrgtree_name", _csgm._mrgtree_name);

   if (_csgm._tv_connectivity)
       DBAddIntComponent(obj, "tv_connectivity", _csgm._tv_connectivity);

   if (_csgm._disjoint_mode)
       DBAddIntComponent(obj, "disjoint_mode", _csgm._disjoint_mode);

   /*-------------------------------------------------------------
    *  Write csg-mesh object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                            db_pdb_PutCsgvar
 *
 *  Purpose
 *
 *      Write a csg variable object into the open output file.
 *
 *  Programmer
 *
 *      Mark C. Miller, August 10. 2005 
 *
 *  Modifications:
 *      Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *      Added support for conserved/extensive options.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutCsgvar (DBfile *_dbfile, char const *name, char const *meshname,
                  int nvars, char const * const *varnames, void const * const *vars,
                  int nels, int datatype, int centering,
                  DBoptlist const *optlist) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   int            i;
   long           count[3];
   DBobject      *obj;
   char          *suffix, *datatype_str, tmp1[256], tmp2[256];
   static char   *me = "db_pdb_PutCsgvar";

   db_InitCsg(_dbfile, name, optlist);

   obj = DBMakeObject(name, DB_CSGVAR, nvars+19);

   DBAddStrComponent(obj, "meshid", meshname);

   /*-------------------------------------------------------------
    *  Write variable arrays.
    *  Set index variables and counters.
    *-----------------------------------------------------------*/

   count[0] = nels;

   switch (centering) {
   case DB_NODECENT:
   case DB_ZONECENT:
   case DB_FACECENT:
      break;

   default:
      return db_perror("centering", E_BADARGS, me);
   }

   /*-------------------------------------------------------------
    *  We first will write the given variables to output, then
    *  we'll define a Ucdvar object in output composed of the
    *  variables plus the given options.
    *-------------------------------------------------------------*/

   suffix = "data";
   datatype_str = db_GetDatatypeString(datatype);

   for (i = 0; i < nvars && nels; i++) {

      db_mkname(dbfile->pdb, (char*) varnames[i], suffix, tmp2);
      PJ_write_len(dbfile->pdb, tmp2, datatype_str, vars[i],
                   1, count);

      sprintf(tmp1, "value%d", i);
      DBAddVarComponent(obj, tmp1, tmp2);

   }
   FREE(datatype_str);

   /*-------------------------------------------------------------
    *  Build a output object definition for a ucd mesh var. The
    *  minimum required information for a ucd var is the variable
    *  itself, plus the ID for the associated ucd mesh object.
    *  Process any additional options to complete the definition.
    *-------------------------------------------------------------*/

   DBAddIntComponent(obj, "nvals", nvars);
   DBAddIntComponent(obj, "nels", nels);
   DBAddIntComponent(obj, "centering", centering);
   DBAddIntComponent(obj, "datatype", datatype);
   if (_csgm._guihide)
      DBAddIntComponent(obj, "guihide", _csgm._guihide);

   /*-------------------------------------------------------------
    * Add 'recommended' optional components.
    *-------------------------------------------------------------*/
   if (_csgm._time_set == TRUE)
      DBAddVarComponent(obj, "time", _csgm._nm_time);
   if (_csgm._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _csgm._nm_dtime);

   if (centering == DB_ZONECENT)
   {
      if (_csgm._hi_offset_set == TRUE)
         DBAddIntComponent(obj, "hi_offset", _csgm._hi_offset);
      if (_csgm._lo_offset_set == TRUE)
         DBAddIntComponent(obj, "lo_offset", _csgm._lo_offset);
   }

   DBAddIntComponent(obj, "cycle", _csgm._cycle);
   DBAddIntComponent(obj, "use_specmf", _csgm._use_specmf);
   if (_csgm._ascii_labels) {
      DBAddIntComponent(obj, "ascii_labels", _csgm._ascii_labels);
   }

   /*-------------------------------------------------------------
    *  Process character strings: labels & units for variable.
    *-------------------------------------------------------------*/
   if (_csgm._label != NULL)
      DBAddStrComponent(obj, "label", _csgm._label);

   if (_csgm._unit != NULL)
      DBAddStrComponent(obj, "units", _csgm._unit);

   if (_csgm._region_pnames != NULL)
   {
        char *s=0; int len=0; long llen;
        DBStringArrayToStringList((char const * const *)_csgm._region_pnames, -1, &s, &len);
        llen = len;
        DBWriteComponent(_dbfile, obj, "region_pnames", name, "char", s, 1, &llen);
        FREE(s);
   }

   if (_csgm._conserved)
      DBAddIntComponent(obj, "conserved", _csgm._conserved);

   if (_csgm._extensive)
      DBAddIntComponent(obj, "extensive", _csgm._extensive);

   if (_csgm._missing_value != DB_MISSING_VALUE_NOT_SET)
   {
      if (_csgm._missing_value == 0.0)
          DBAddDblComponent(obj, "missing_value", DB_MISSING_VALUE_NOT_SET);
      else
          DBAddDblComponent(obj, "missing_value", _csgm._missing_value);
   }

   /*-------------------------------------------------------------
    *  Write ucd-mesh object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(_dbfile, obj, 0);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                        db_pdb_PutCSGZonelist
 *
 *  Purpose
 *
 *      Write a csg zonelist object into the open output file.
 *
 *  Programmer
 *
 *      Mark C. Miller 
 *      August 9, 2005 
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutCSGZonelist (DBfile *dbfile, char const *name, int nregs,
                 int const *typeflags,
                 int const *leftids, int const *rightids,
                 void const *xforms, int lxforms, int datatype,
                 int nzones, int const *zonelist, DBoptlist const *optlist) {

   long           count[1];
   DBobject      *obj;

   memset(&_csgzl, 0, sizeof(_csgzl));
   db_ProcessOptlist(DB_CSGZONELIST, optlist);

   /*--------------------------------------------------
    *  Build up object description by defining literals
    *  and defining/writing arrays.
    *-------------------------------------------------*/
   obj = DBMakeObject(name, DB_CSGZONELIST, 15);

   DBAddIntComponent(obj, "nregs", nregs);
   DBAddIntComponent(obj, "datatype", datatype);
   DBAddIntComponent(obj, "nzones", nzones);

   count[0] = nregs;
   DBWriteComponent(dbfile, obj, "typeflags", name, "integer",
                    typeflags, 1, count);
   DBWriteComponent(dbfile, obj, "leftids", name, "integer",
                    leftids, 1, count);
   DBWriteComponent(dbfile, obj, "rightids", name, "integer",
                    rightids, 1, count);
   count[0] = nzones;
   DBWriteComponent(dbfile, obj, "zonelist", name, "integer",
                    zonelist, 1, count);

   if (xforms && lxforms > 0)
   {
       char *datatype_str = db_GetDatatypeString(datatype);
       count[0] = lxforms;
       DBWriteComponent(dbfile, obj, "xforms", name, datatype_str, xforms, 1, count);
   }

   if (_csgzl._regnames)
   {
       int len; char *tmp;
       DBStringArrayToStringList((char const * const *)_csgzl._regnames, nregs, &tmp, &len);
       count[0] = len;
       DBWriteComponent(dbfile, obj, "regnames", name, "char", tmp, 1, count);
       FREE(tmp);
   }

   if (_csgzl._zonenames)
   {
       int len; char *tmp;
       DBStringArrayToStringList((char const * const *)_csgzl._zonenames, nzones, &tmp, &len);
       count[0] = len;
       DBWriteComponent(dbfile, obj, "zonenames", name, "char", tmp, 1, count);
       FREE(tmp);
   }

   if (_csgzl._alt_zonenum_vars)
   {
       int nvars=-1, len; char *tmp;
       DBStringArrayToStringList((char const * const *)_csgzl._alt_zonenum_vars, nvars, &tmp, &len);
       count[0] = len;
       DBWriteComponent(dbfile, obj, "alt_zonenum_vars", name, "char", tmp, 1, count);
       FREE(tmp);
   }

   /*-------------------------------------------------------------
    *  Write object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                           db_pdb_PutUcdmesh
 *
 *  Purpose
 *
 *      Write a ucd mesh object into the open output file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modifications
 *     Robb Matzke, Sun Dec 18 17:12:16 EST 1994
 *     Changed SW_GetDatatypeString to db_GetDatatypeString and
 *     removed associated memory leak.
 *
 *     Robb Matzke, Fri Dec 2 14:19:04 PST 1994
 *     Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *     Al Leibee, Mon Apr 18 07:45:58 PDT 1994
 *     Added _dtime.
 *
 *     Sean Ahern, Wed Oct 21 09:22:30 PDT 1998
 *     Changed how we pass extents data to the UM_CalcExtents function.
 *
 *     Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *     Added group_no and gnodeno.
 *
 *     Hank Childs, Thu Jan  6 13:51:22 PST 2000
 *     Put in lint directive for unused arguments.
 *
 *     Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *     Add support for long long global node/zone numbers.
 *
 *      Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *      Changed how long long global node/zone numbers are supported
 *      from a int (bool), "llong_gnode|zoneno" to an int holding
 *      the actual datatype. The type is assumed int if it its
 *      value is zero or it does not exist. Otherwise, the type is
 *      is whatever is stored in gnznodtype member. 
 *
 *      Mark C. Miller, Tue Nov 17 22:22:09 PST 2009
 *      Changed name of long long datatype to match what PDB proper
 *      would call it.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
/* ARGSUSED */
SILO_CALLBACK int
db_pdb_PutUcdmesh (DBfile *dbfile, char const *name, int ndims, char const * const *coordnames,
                   DBVCP2_t _coords, int nnodes, int nzones, char const *zlname,
                   char const *flname, int datatype, DBoptlist const *optlist) {

   int            i;
   long           count[3];
   DBobject      *obj;
   char          *datatype_str;
   char           tmp[256];
   void const * const *coords = (void const * const *) _coords;

   /* Following is declared as double for worst case */
   double         min_extents[3]={0,0,0}, max_extents[3]={0,0,0};

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   db_InitUcd(dbfile, name, optlist, ndims, nnodes, nzones);

   obj = DBMakeObject(name, DB_UCDMESH, 33);

   /*-------------------------------------------------------------
    *  We first will write the given coordinate arrays to output,
    *  then we'll define a UCD-mesh object in output composed of the
    *  coordinates plus the given options.
    *-------------------------------------------------------------*/

   datatype_str = db_GetDatatypeString(datatype);
   count[0] = nnodes;

   for (i = 0; (i < ndims) && (nnodes > 0); i++) {

      sprintf(tmp, "coord%d", i);

      DBWriteComponent(dbfile, obj, tmp, name, datatype_str,
                       coords[i], 1, count);
   }

   /*-------------------------------------------------------------
    *  Find the mesh extents from the coordinate arrays. Write
    *  them out to output file.
    *-------------------------------------------------------------*/

   UM_CalcExtents(coords, datatype, ndims, nnodes, min_extents, max_extents);

   count[0] = ndims;
   DBWriteComponent(dbfile, obj, "min_extents", name, datatype_str,
                    min_extents, 1, count);

   DBWriteComponent(dbfile, obj, "max_extents", name, datatype_str,
                    max_extents, 1, count);
   FREE(datatype_str);

   /*-------------------------------------------------------------
    *  Build a output object definition for a ucd mesh. The minimum
    *  required information for a ucd mesh is the coordinate arrays,
    *  the number of dimensions, and the type (RECT/CURV). Process
    *  the provided options to complete the definition.
    *
    *  The output object definition is composed of a string of delimited
    *  component names plus an array of output *identifiers* of
    *  previously defined variables.
    *-------------------------------------------------------------*/

   if (flname)
      DBAddStrComponent(obj, "facelist", flname);
   if (zlname)
      DBAddStrComponent(obj, "zonelist", zlname);

   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nnodes", nnodes);
   DBAddIntComponent(obj, "nzones", nzones);
   DBAddIntComponent(obj, "facetype", _um._facetype);
   DBAddIntComponent(obj, "cycle", _um._cycle);
   DBAddIntComponent(obj, "coord_sys", _um._coord_sys);
   if (_um._topo_dim > 0)
      DBAddIntComponent(obj, "topo_dim", _um._topo_dim);
   DBAddIntComponent(obj, "planar", _um._planar);
   DBAddIntComponent(obj, "origin", _um._origin);
   DBAddIntComponent(obj, "datatype", datatype);
   if (_um._llong_gnodeno)
       DBAddIntComponent(obj, "gnznodtype", DB_LONG_LONG);

   if (_um._gnodeno)
   {
       count[0] = nnodes;
       if (_um._llong_gnodeno)
           DBWriteComponent(dbfile, obj, "gnodeno", name, "long_long",
               _um._gnodeno, 1, count);
       else
           DBWriteComponent(dbfile, obj, "gnodeno", name, "integer",
               _um._gnodeno, 1, count);
   }

   if (_um._group_no >= 0)
       DBAddIntComponent(obj, "group_no", _um._group_no);

   if (_um._time_set == TRUE)
      DBAddVarComponent(obj, "time", _um._nm_time);
   if (_um._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _um._nm_dtime);

   /*-------------------------------------------------------------
    *  Process character strings: labels & units for x, y, &/or z,
    *-------------------------------------------------------------*/
   if (_um._labels[0] != NULL)
      DBAddStrComponent(obj, "label0", _um._labels[0]);

   if (_um._labels[1] != NULL)
      DBAddStrComponent(obj, "label1", _um._labels[1]);

   if (_um._labels[2] != NULL)
      DBAddStrComponent(obj, "label2", _um._labels[2]);

   if (_um._units[0] != NULL)
      DBAddStrComponent(obj, "units0", _um._units[0]);

   if (_um._units[1] != NULL)
      DBAddStrComponent(obj, "units1", _um._units[1]);

   if (_um._units[2] != NULL)
      DBAddStrComponent(obj, "units2", _um._units[2]);

   if (_um._guihide)
      DBAddIntComponent(obj, "guihide", _um._guihide);

   /*-------------------------------------------------------------
    *  Optional polyhedral zonelist 
    *-------------------------------------------------------------*/
   if (_um._phzl_name != NULL)
      DBAddStrComponent(obj, "phzonelist", _um._phzl_name);

   if (_um._mrgtree_name != NULL)
      DBAddStrComponent(obj, "mrgtree_name", _um._mrgtree_name);

   if (_um._tv_connectivity)
      DBAddIntComponent(obj, "tv_connectivity", _um._tv_connectivity);

   if (_um._disjoint_mode)
      DBAddIntComponent(obj, "disjoint_mode", _um._disjoint_mode);

   if (nnodes>0 && _um._ghost_node_labels != NULL)
   {
      count[0] = nnodes;
      DBWriteComponent(dbfile, obj, "ghost_node_labels", name, "char",
          _um._ghost_node_labels, 1, count);
   }

   if (nnodes>0 && _um._alt_nodenum_vars)
   {
      int nvars=-1, len; long llen; char *tmpstr = 0;
      DBStringArrayToStringList((char const * const *)_um._alt_nodenum_vars, nvars, &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "alt_nodenum_vars", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
   }

   /*-------------------------------------------------------------
    *  Write ucd-mesh object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 *  Routine                                         db_pdb_PutUcdsubmesh
 *
 *  Purpose
 *
 *      Write a subset of a ucd mesh object into the open output file.
 *
 *  Programmer
 *
 *      Jim Reus
 *
 *  Notes
 *
 *  Modifications
 *     none yet.
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutUcdsubmesh (DBfile *dbfile, char const *name, char const *parentmesh,
                      int nzones, char const *zlname, char const *flname,
                      DBoptlist const *optlist) {

   int            i;
   DBobject      *obj;
   int           *Pdatatype,datatype;
   int           *Pndims,ndims;
   int           *Pnnodes,nnodes;

   /*-------------------------------------------------------------
    *  We first retreive certain attributes from the parent mesh.
    *  Of course this had better succeed...
    *-------------------------------------------------------------*/

   Pndims       = (int *)DBGetComponent(dbfile,parentmesh,"ndims");
          ndims = *Pndims;
   Pnnodes      = (int *)DBGetComponent(dbfile,parentmesh,"nnodes");
         nnodes = *Pnnodes;
   Pdatatype    = (int *)DBGetComponent(dbfile,parentmesh,"datatype");
       datatype = *Pdatatype;

   /*-------------------------------------------------------------
    *  Now we can initialize global data, and process options.
    *-------------------------------------------------------------*/

   db_InitUcd(dbfile, name, optlist, ndims, nnodes, nzones);

   obj = DBMakeObject(name, DB_UCDMESH, 28);

   /*-------------------------------------------------------------
    *  Then we will add references to the coordinate arrays of the
    *  parent mesh...
    *-------------------------------------------------------------*/

   for (i = 0; i < ndims; i++) {
      char           myComponName[256];
      char           parentComponName[256];

      sprintf(myComponName, "coord%d", i);
      sprintf(parentComponName, "%s_coord%d", parentmesh, i);
      DBAddVarComponent(obj,myComponName,parentComponName);
   }

   /*-------------------------------------------------------------
    *  For now we'll simply refer to the extents provided for the
    *  parent mesh.  It would be be better to compute extents for
    *  the cordinates actually used, but then we'd have to retrieve
    *  the coords...
    *-------------------------------------------------------------*/

   {  char           myComponName[256];
      char           parentComponName[256];

      sprintf(myComponName, "min_extents");
      sprintf(parentComponName, "%s_min_extents", parentmesh);
      DBAddVarComponent(obj,myComponName,parentComponName);

      sprintf(myComponName, "max_extents");
      sprintf(parentComponName, "%s_max_extents", parentmesh);
      DBAddVarComponent(obj,myComponName,parentComponName);
   }

   /*-------------------------------------------------------------
    *  Build a output object definition for a ucd mesh. The minimum
    *  required information for a ucd mesh is the coordinate arrays,
    *  the number of dimensions, and the type (RECT/CURV). Process
    *  the provided options to complete the definition.
    *
    *  The output object definition is composed of a string of delimited
    *  component names plus an array of output *identifiers* of
    *  previously defined variables.
    *-------------------------------------------------------------*/

   if (flname)
      DBAddStrComponent(obj, "facelist", flname);
   if (zlname)
      DBAddStrComponent(obj, "zonelist", zlname);

   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nnodes", nnodes);
   DBAddIntComponent(obj, "nzones", nzones);
   DBAddIntComponent(obj, "facetype", _um._facetype);
   DBAddIntComponent(obj, "cycle", _um._cycle);
   DBAddIntComponent(obj, "coord_sys", _um._coord_sys);
   if (_um._topo_dim > 0)
      DBAddIntComponent(obj, "topo_dim", _um._topo_dim);
   DBAddIntComponent(obj, "planar", _um._planar);
   DBAddIntComponent(obj, "origin", _um._origin);
   DBAddIntComponent(obj, "datatype", datatype);

   if (_um._time_set == TRUE)
      DBAddVarComponent(obj, "time", _um._nm_time);
   if (_um._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _um._nm_dtime);

   /*-------------------------------------------------------------
    *  Process character strings: labels & units for x, y, &/or z,
    *-------------------------------------------------------------*/
   if (_um._labels[0] != NULL)
      DBAddStrComponent(obj, "label0", _um._labels[0]);

   if (_um._labels[1] != NULL)
      DBAddStrComponent(obj, "label1", _um._labels[1]);

   if (_um._labels[2] != NULL)
      DBAddStrComponent(obj, "label2", _um._labels[2]);

   if (_um._units[0] != NULL)
      DBAddStrComponent(obj, "units0", _um._units[0]);

   if (_um._units[1] != NULL)
      DBAddStrComponent(obj, "units1", _um._units[1]);

   if (_um._units[2] != NULL)
      DBAddStrComponent(obj, "units2", _um._units[2]);

   if (_um._guihide)
      DBAddIntComponent(obj, "guihide", _um._guihide);

   if (_um._tv_connectivity)
      DBAddIntComponent(obj, "tv_connectivity", _um._tv_connectivity);

   if (_um._disjoint_mode)
      DBAddIntComponent(obj, "disjoint_mode", _um._disjoint_mode);


   /*-------------------------------------------------------------
    *  Write ucd-mesh object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);

   FREE(Pdatatype);
   FREE(Pnnodes);
   FREE(Pndims);

   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 *  Routine                                            db_pdb_PutUcdvar
 *
 *  Purpose
 *
 *      Write a ucd variable object into the open output file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modifications
 *     Al Leibee, Mon Apr 18 07:45:58 PDT 1994
 *     Added _dtime.
 *
 *     Al Leibee, Mon Jul 11 12:00:00 PDT 1994
 *     Use mixlen, not _mixlen.
 *
 *     Al Leibee, Wed Jul 20 07:56:37 PDT 1994
 *     Write mixlen component.
 *
 *     Al Leibee, Wed Aug  3 16:57:38 PDT 1994
 *     Added _use_specmf.
 *
 *     Robb Matzke, Fri Dec 2 14:19:42 PST 1994
 *     Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *     Robb Matzke, Sun Dec 18 17:13:17 EST 1994
 *     Changed SW_GetDatatypeString to db_GetDatatypeString and
 *     removed associated memory leak.
 *
 *     Sean Ahern, Sun Oct  1 03:17:35 PDT 1995
 *     Made "me" static.
 *
 *     Eric Brugger, Wed Oct 15 14:45:47 PDT 1997
 *     Added _hi_offset and _lo_offset.
 *
 *     Hank Childs, Thu Jan  6 13:51:22 PST 2000
 *     Removed unused variable nm_cent.
 *
 *     Robb Matzke, 2000-05-23
 *     Removed the duplicate `meshid' field.
 *
 *     Brad Whitlock, Wed Jan 18 15:09:57 PST 2006
 *     Added ascii_labels.
 *
 *     Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *     Added support for conserved/extensive options.
 *
 *     Mark C. Miller, Wed Nov 11 22:17:40 PST 2009
 *     Removed unnecessary code checking centering.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutUcdvar (DBfile *_dbfile, char const *name, char const *meshname, int nvars,
                  char const * const *varnames, DBVCP2_t _vars, int nels, DBVCP2_t _mixvars,
                  int mixlen, int datatype, int centering,
                  DBoptlist const *optlist) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   int            i;
   long           count[3], mcount[1];
   DBobject      *obj;
   char          *suffix, *datatype_str, tmp1[256], tmp2[256];
   static char   *me = "db_pdb_PutUcdvar";
   void const * const    *vars = (void const * const *) _vars;
   void const * const *mixvars = (void const * const *) _mixvars;

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
#if 1                       /*which way is right?  (1st one is the original) */
   db_InitUcd(_dbfile, meshname, optlist, _um._ndims,
              _um._nnodes, _um._nzones);
#else
   db_InitUcd(_dbfile, meshname, optlist, ndims, nnodes, nzones);
#endif

   obj = DBMakeObject(name, DB_UCDVAR, (1+(mixlen!=0)) * nvars + 22);

   DBAddStrComponent(obj, "meshid", meshname);

   /*-------------------------------------------------------------
    *  Write variable arrays.
    *  Set index variables and counters.
    *-----------------------------------------------------------*/

   count[0] = nels;

   /*-------------------------------------------------------------
    *  We first will write the given variables to output, then
    *  we'll define a Ucdvar object in output composed of the
    *  variables plus the given options.
    *-------------------------------------------------------------*/

   suffix = "data";
   datatype_str = db_GetDatatypeString(datatype);

   for (i = 0; i < nvars && nels; i++) {

      db_mkname(dbfile->pdb, varnames[i], suffix, tmp2);
      PJ_write_len(dbfile->pdb, tmp2, datatype_str, vars[i],
                   1, count);

      sprintf(tmp1, "value%d", i);
      DBAddVarComponent(obj, tmp1, tmp2);

      /* Write the mixed data component if present */
      if (mixvars != NULL && mixvars[i] != NULL && mixlen > 0) {
         mcount[0] = mixlen;

         db_mkname(dbfile->pdb, varnames[i], "mix", tmp2);
         PJ_write_len(dbfile->pdb, tmp2, datatype_str, mixvars[i],
                      1, mcount);

         sprintf(tmp1, "mixed_value%d", i);
         DBAddVarComponent(obj, tmp1, tmp2);
      }
   }
   FREE(datatype_str);

   /*-------------------------------------------------------------
    *  Build a output object definition for a ucd mesh var. The
    *  minimum required information for a ucd var is the variable
    *  itself, plus the ID for the associated ucd mesh object.
    *  Process any additional options to complete the definition.
    *-------------------------------------------------------------*/

   DBAddIntComponent(obj, "ndims", _um._ndims);
   DBAddIntComponent(obj, "nvals", nvars);
   DBAddIntComponent(obj, "nels", nels);
   DBAddIntComponent(obj, "centering", centering);
   DBAddIntComponent(obj, "origin", _um._origin);
   DBAddIntComponent(obj, "mixlen", mixlen);
   DBAddIntComponent(obj, "datatype", datatype);

   /*-------------------------------------------------------------
    * Add 'recommended' optional components.
    *-------------------------------------------------------------*/
   if (_um._time_set == TRUE)
      DBAddVarComponent(obj, "time", _um._nm_time);
   if (_um._dtime_set == TRUE)
      DBAddVarComponent(obj, "dtime", _um._nm_dtime);

   if (centering == DB_ZONECENT)
   {
      if (_um._hi_offset_set == TRUE)
         DBAddIntComponent(obj, "hi_offset", _um._hi_offset);
      if (_um._lo_offset_set == TRUE)
         DBAddIntComponent(obj, "lo_offset", _um._lo_offset);
   }

   DBAddIntComponent(obj, "cycle", _um._cycle);
   DBAddIntComponent(obj, "use_specmf", _um._use_specmf);
   if (_um._ascii_labels) {
      DBAddIntComponent(obj, "ascii_labels", _um._ascii_labels);
   }

   /*-------------------------------------------------------------
    *  Process character strings: labels & units for variable.
    *-------------------------------------------------------------*/
   if (_um._label != NULL)
      DBAddStrComponent(obj, "label", _um._label);

   if (_um._unit != NULL)
      DBAddStrComponent(obj, "units", _um._unit);

   if (_um._guihide)
      DBAddIntComponent(obj, "guihide", _um._guihide);

   if (_um._region_pnames != NULL)
   {
        char *s=0; int len=0; long llen;
        DBStringArrayToStringList((char const * const *)_um._region_pnames, -1, &s, &len);
        llen = len;
        DBWriteComponent(_dbfile, obj, "region_pnames", name, "char", s, 1, &llen);
        FREE(s);
   }

   if (_um._conserved)
      DBAddIntComponent(obj, "conserved", _um._conserved);

   if (_um._extensive)
      DBAddIntComponent(obj, "extensive", _um._extensive);

   if (_um._missing_value != DB_MISSING_VALUE_NOT_SET)
   {
      if (_um._missing_value == 0.0)
          DBAddDblComponent(obj, "missing_value", DB_MISSING_VALUE_NOT_SET);
      else
          DBAddDblComponent(obj, "missing_value", _um._missing_value);
   }

   /*-------------------------------------------------------------
    *  Write ucd-mesh object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(_dbfile, obj, 0);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                           db_pdb_PutZonelist
 *
 *  Purpose
 *
 *      Write a ucd zonelist object into the open output file.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modified
 *     Robb Matzke, Fri Dec 2 14:20:10 PST 1994
 *     Changed SCFREE(obj) to DBFreeObject(obj)
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutZonelist (DBfile *dbfile, char const *name, int nzones, int ndims,
                    int const *nodelist, int lnodelist, int origin, int const *shapesize,
                    int const *shapecnt, int nshapes) {

   long           count[5];
   DBobject      *obj;

   /*--------------------------------------------------
    *  Build up object description by defining literals
    *  and defining/writing arrays.
    *-------------------------------------------------*/
   obj = DBMakeObject(name, DB_ZONELIST, 15);

   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nzones", nzones);
   DBAddIntComponent(obj, "nshapes", nshapes);
   DBAddIntComponent(obj, "lnodelist", lnodelist);
   DBAddIntComponent(obj, "origin", origin);

   count[0] = lnodelist;

   DBWriteComponent(dbfile, obj, "nodelist", name, "integer",
                    nodelist, 1, count);

   count[0] = nshapes;

   DBWriteComponent(dbfile, obj, "shapecnt", name, "integer",
                    shapecnt, 1, count);

   DBWriteComponent(dbfile, obj, "shapesize", name, "integer",
                    shapesize, 1, count);

   /*-------------------------------------------------------------
    *  Write object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                          db_pdb_PutZonelist2
 *
 *  Purpose
 *
 *      Write a ucd zonelist object into the open output file.
 *
 *  Programmer
 *
 *      Eric Brugger
 *      March 30, 1999
 *
 *  Notes
 *
 *  Modified
 *
 *    Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *    Added an option list, a call to initialize it, and gzoneno.
 *
 *    Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *    Add support for long long global node/zone numbers.
 *
 *    Mark C. Miller, Tue Nov 17 22:22:09 PST 2009
 *    Changed name of long long datatype to match what PDB proper
 *    would call it.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutZonelist2 (DBfile *dbfile, char const *name, int nzones, int ndims,
                     int const *nodelist, int lnodelist, int origin,
                     int lo_offset, int hi_offset, int const *shapetype,
                     int const *shapesize, int const *shapecnt, int nshapes,
                     DBoptlist const *optlist) {

   long           count[5];
   DBobject      *obj;

   db_InitZonelist(dbfile, optlist);

   /*--------------------------------------------------
    *  Build up object description by defining literals
    *  and defining/writing arrays.
    *-------------------------------------------------*/
   obj = DBMakeObject(name, DB_ZONELIST, 16);

   DBAddIntComponent(obj, "ndims", ndims);
   DBAddIntComponent(obj, "nzones", nzones);
   DBAddIntComponent(obj, "nshapes", nshapes);
   DBAddIntComponent(obj, "lnodelist", lnodelist);
   DBAddIntComponent(obj, "origin", origin);
   DBAddIntComponent(obj, "lo_offset", lo_offset);
   DBAddIntComponent(obj, "hi_offset", hi_offset);
   if (_uzl._llong_gzoneno)
       DBAddIntComponent(obj, "gnznodtype", DB_LONG_LONG);

   count[0] = lnodelist;

   DBWriteComponent(dbfile, obj, "nodelist", name, "integer",
                    nodelist, 1, count);

   count[0] = nshapes;

   DBWriteComponent(dbfile, obj, "shapecnt", name, "integer",
                    shapecnt, 1, count);

   DBWriteComponent(dbfile, obj, "shapesize", name, "integer",
                    shapesize, 1, count);

   DBWriteComponent(dbfile, obj, "shapetype", name, "integer",
                    shapetype, 1, count);

   if (nzones>0 && _uzl._gzoneno)
   {
       count[0] = nzones;
       if (_uzl._llong_gzoneno)
           DBWriteComponent(dbfile, obj, "gzoneno", name, "long_long",
               _uzl._gzoneno, 1, count);
       else
           DBWriteComponent(dbfile, obj, "gzoneno", name, "integer",
               _uzl._gzoneno, 1, count);
   }

   if (nzones>0 && _uzl._ghost_zone_labels)
   {
       count[0] = nzones;
       DBWriteComponent(dbfile, obj, "ghost_zone_labels", name, "char",
               _uzl._ghost_zone_labels, 1, count);
   }

   if (nzones>0 && _uzl._alt_zonenum_vars)
   {
      int nvars=-1, len; long llen; char *tmpstr = 0;
      DBStringArrayToStringList((char const * const *)_uzl._alt_zonenum_vars, nvars, &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "alt_zonenum_vars", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
   }

   /*-------------------------------------------------------------
    *  Write object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                 db_pdb_PutPHZonelist
 *
 *  Purpose
 *
 *      Write a polyhedral zonelist object into the open output file.
 *
 *  Programmer
 *
 *      Mark C. Miller 
 *      July 27, 2004
 *
 *  Modifications:
 *      Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *      Add support for long long global node/zone numbers.
 *
 *      Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *      Changed how long long global node/zone numbers are supported
 *      from a int (bool), "llong_gnode|zoneno" to an int holding
 *      the actual datatype. The type is assumed int if it its
 *      value is zero or it does not exist. Otherwise, the type is
 *      is whatever is stored in gnznodtype member. 
 *
 *      Mark C. Miller, Tue Nov 17 22:22:09 PST 2009
 *      Changed name of long long datatype to match what PDB proper
 *      would call it.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutPHZonelist (DBfile *dbfile, char const *name,
   int nfaces, int const *nodecnt, int lnodelist, int const *nodelist, char const *extface,
   int nzones, int const *facecnt, int lfacelist, int const *facelist,
   int origin, int lo_offset, int hi_offset,
   DBoptlist const *optlist) {

   long           count[1];
   DBobject      *obj;

   db_InitPHZonelist(dbfile, optlist);

   /*--------------------------------------------------
    *  Build up object description by defining literals
    *  and defining/writing arrays.
    *-------------------------------------------------*/
   obj = DBMakeObject(name, DB_PHZONELIST, 16);

   DBAddIntComponent(obj, "nfaces", nfaces);
   DBAddIntComponent(obj, "lnodelist", lnodelist);
   DBAddIntComponent(obj, "nzones", nzones);
   DBAddIntComponent(obj, "lfacelist", lfacelist);
   DBAddIntComponent(obj, "origin", origin);
   DBAddIntComponent(obj, "lo_offset", lo_offset);
   DBAddIntComponent(obj, "hi_offset", hi_offset);
   if (_phzl._llong_gzoneno)
       DBAddIntComponent(obj, "gnznodtype", DB_LONG_LONG);

   count[0] = nfaces;

   DBWriteComponent(dbfile, obj, "nodecnt", name, "integer",
                    nodecnt, 1, count);

   count[0] = lnodelist;

   DBWriteComponent(dbfile, obj, "nodelist", name, "integer",
                    nodelist, 1, count);

   if (facecnt)
   {
       count[0] = nzones;

       DBWriteComponent(dbfile, obj, "facecnt", name, "integer",
                        facecnt, 1, count);
   }

   if (facelist)
   {
       count[0] = lfacelist;

       DBWriteComponent(dbfile, obj, "facelist", name, "integer",
                        facelist, 1, count);

   }

   if (extface)
   {
       count[0] = nfaces;

       DBWriteComponent(dbfile, obj, "extface", name, "char",
                        extface, 1, count);
   }

   if (nzones>0 && _phzl._gzoneno)
   {
       count[0] = nzones;
       if (_phzl._llong_gzoneno)
           DBWriteComponent(dbfile, obj, "gzoneno", name, "long_long",
               _phzl._gzoneno, 1, count);
       else
           DBWriteComponent(dbfile, obj, "gzoneno", name, "integer",
               _phzl._gzoneno, 1, count);
   }

   if (nzones>0 && _phzl._ghost_zone_labels)
   {
       count[0] = nzones;
       DBWriteComponent(dbfile, obj, "ghost_zone_labels", name, "char",
               _phzl._ghost_zone_labels, 1, count);
   }

   if (nzones>0 && _phzl._alt_zonenum_vars)
   {
      int nvars=-1, len; long llen; char *tmpstr = 0;
      DBStringArrayToStringList((char const * const *)_phzl._alt_zonenum_vars, nvars, &tmpstr, &len);
      llen = (long) len;
      DBWriteComponent(dbfile, obj, "alt_zonenum_vars", name, "char", tmpstr, 1, &llen);
      FREE(tmpstr);
   }

   /*-------------------------------------------------------------
    *  Write object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                            db_pdb_PutMrgtree
 *
 *  Purpose
 *
 *      Write mrg tree object into the open output file.
 *
 *  Programmer
 *
 *      Mark C. Miller, Wed Oct 10 10:25:53 PDT 2007
 *
 *  Modifications:
 *
 *      Mark C. Miller, Mon Nov 17 19:02:58 PST 2008
 *      Fixed output of src_mesh_name
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
/* ARGSUSED */
SILO_CALLBACK int
db_pdb_PutMrgtree(DBfile *dbfile, char const *name,
    char const *mesh_name, DBmrgtree const *tree, DBoptlist const *optlist)
{
    int                 i, j, n, len, pass, num_nodes = tree->num_nodes;
    int                 tot_segs, tot_children;
    long                count;
    char               *s = 0;
    DBmrgtnode        **ltree = 0;
    char              **strArray = 0;
    int                *intArray = 0;
    DBobject           *obj;

    obj = DBMakeObject(name, DB_MRGTREE, 17);

    /* Set global options */
    db_ResetGlobalData_Mrgtree();
    db_ProcessOptlist(DB_MRGTREE, optlist);

    /* allocate an emtpy, linearized list of tree node pointers */
    ltree = (DBmrgtnode **) malloc(num_nodes * sizeof(DBmrgtnode*));

    /* walk tree to populate the linearized list of nodes ptrs */
    DBWalkMrgtree(tree, (DBmrgwalkcb) DBLinearizeMrgtree, ltree, DB_POSTORDER);

    /* form an array of the integer, scalar data at each node */
    intArray = (int *) malloc(num_nodes * sizeof(int) * 6);
    for (i = 0; i < num_nodes; i++)
    {
        intArray[i*6+0] = ltree[i]->narray;
        intArray[i*6+1] = ltree[i]->type_info_bits;
        intArray[i*6+2] = ltree[i]->max_children;
        intArray[i*6+3] = ltree[i]->nsegs;
        intArray[i*6+4] = ltree[i]->num_children;
        intArray[i*6+5] = ltree[i] != tree->root ? ltree[i]->parent->walk_order : -1;
    }
    count = num_nodes * 6;
    DBWriteComponent(dbfile, obj, "scalars", name, "integer", intArray, 1, &count);
    FREE(intArray);

    /* form an array of strings of the name data member */
    strArray = (char **) malloc(num_nodes * sizeof(char *));
    for (i = 0; i < num_nodes; i++)
        strArray[i] = ltree[i]->name;

    /* output all the node names as one long dataset */
    s = 0;
    DBStringArrayToStringList((char const * const *)strArray, num_nodes, &s, &len);
    count = len;
    DBWriteComponent(dbfile, obj, "name", name, "char", s, 1, &count);
    FREE(s);
    FREE(strArray);

    /* form an array of strings of names data member */
    /* requires 2 passes; first pass only counts, 2nd pass builds the array */
    for (pass = 0; pass < 2; pass ++)
    {
        if (pass == 1)
        {
            if (n == 0)
                break;
            strArray = (char **) malloc(n * sizeof(char *));
        }
        n = 0;
        for (i = 0; i < num_nodes; i++)
        {
            if (ltree[i]->narray > 0)
            {
                if (strchr(ltree[i]->names[0], '%') == 0)
                {
                    for (j = 0; j < ltree[i]->narray; j++)
                    {
                        if (pass == 1)
                            strArray[n] = ltree[i]->names[j];
                        n++;
                    }
                }
                else
                {
                    if (pass == 1)
                        strArray[n] = ltree[i]->names[0];
                    n++;
                }
            }
        }
    }
    if (n > 0)
    {
        s = 0;
        DBStringArrayToStringList((char const * const *)strArray, n, &s, &len);
        count = len;
        DBWriteComponent(dbfile, obj, "names", name, "char", s, 1, &count);
        FREE(s);
    }
    FREE(strArray);

    /* linearize and output map name data */
    strArray = (char **) malloc(num_nodes * sizeof(char*));
    for (i = 0; i < num_nodes; i++)
        strArray[i] = ltree[i]->maps_name;
    s = 0;
    len = 0;
    DBStringArrayToStringList((char const * const *)strArray, num_nodes, &s, &len);
    count = len;
    DBWriteComponent(dbfile, obj, "maps_name", name, "char", s, 1, &count);
    FREE(s);
    FREE(strArray);

    tot_segs = 0;
    for (i = 0; i < num_nodes; i++)
        tot_segs += ltree[i]->nsegs * (ltree[i]->narray?ltree[i]->narray:1);
    count = tot_segs;

    if (tot_segs > 0)
    {
        /* linearize and output map segment id data */
        intArray = (int *) malloc(tot_segs * sizeof(int));
        n = 0;
        for (i = 0; i < num_nodes; i++)
            for (j = 0; j < ltree[i]->nsegs * (ltree[i]->narray?ltree[i]->narray:1); j++)
                intArray[n++] = ltree[i]->seg_ids[j];
        DBWriteComponent(dbfile, obj, "seg_ids", name, "integer", intArray, 1, &count);
        FREE(intArray);

        /* linearize and output seg len type data */
        intArray = (int *) malloc(tot_segs * sizeof(int));
        n = 0;
        for (i = 0; i < num_nodes; i++)
            for (j = 0; j < ltree[i]->nsegs * (ltree[i]->narray?ltree[i]->narray:1); j++)
                intArray[n++] = ltree[i]->seg_lens[j];
        DBWriteComponent(dbfile, obj, "seg_lens", name, "integer", intArray, 1, &count);
        FREE(intArray);

        /* linearize and output seg type data */
        intArray = (int *) malloc(tot_segs * sizeof(int));
        n = 0;
        for (i = 0; i < num_nodes; i++)
            for (j = 0; j < ltree[i]->nsegs * (ltree[i]->narray?ltree[i]->narray:1); j++)
                intArray[n++] = ltree[i]->seg_types[j];
        DBWriteComponent(dbfile, obj, "seg_types", name, "integer", intArray, 1, &count);
        FREE(intArray);
    }

    /* form integer array for children data */
    tot_children = 0;
    for (i = 0; i < num_nodes; i++)
        tot_children += ltree[i]->num_children;
    if (tot_children)
    {
        count = tot_children;
        intArray = (int *) malloc(tot_children * sizeof(int));
        n = 0;
        for (i = 0; i < num_nodes; i++)
            for (j = 0; j < ltree[i]->num_children; j++)
                intArray[n++] = ltree[i]->children[j]->walk_order;
        DBWriteComponent(dbfile, obj, "children", name, "integer", intArray, 1, &count);
        FREE(intArray);
    }

    FREE(ltree);

    if (_mrgt._mrgvar_onames)
    {
        s = 0;
        len = 0;
        DBStringArrayToStringList((char const * const *)_mrgt._mrgvar_onames, -1, &s, &len);
        count = len;
        DBWriteComponent(dbfile, obj, "mrgvar_onames", name, "char", s, 1, &count);
        FREE(s);
    }

    if (_mrgt._mrgvar_rnames)
    {
        s = 0;
        len = 0;
        DBStringArrayToStringList((char const * const *)_mrgt._mrgvar_rnames, -1, &s, &len);
        count = len;
        DBWriteComponent(dbfile, obj, "mrgvar_rnames", name, "char", s, 1, &count);
        FREE(s);
    }

    DBAddIntComponent(obj, "src_mesh_type", tree->src_mesh_type);
    DBAddStrComponent(obj, "src_mesh_name", mesh_name);
    DBAddIntComponent(obj, "type_info_bits", tree->type_info_bits);
    DBAddIntComponent(obj, "num_nodes", tree->num_nodes);
    DBAddIntComponent(obj, "root", tree->root->walk_order);

   /*-------------------------------------------------------------
    *  Write object to output file.
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);
   return 0;
}
#endif /* PDB_WRITE */


/*----------------------------------------------------------------------
 *  Routine                                         db_pdb_PutGroupelmap
 *
 *  Purpose
 *
 *      Write groupel map object into the open output file.
 *
 *  Programmer
 *
 *      Mark C. Miller, Wed Oct 10 10:25:53 PDT 2007
 *
 *  Modifications:
 *    Mark C. Miller, Wed Aug 18 20:55:42 PDT 2010
 *    Fix bug setting correct size for frac_lengths array.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
/* ARGSUSED */
SILO_CALLBACK int
db_pdb_PutGroupelmap(DBfile *dbfile, char const *name,
    int num_segments, int const *groupel_types, int const *segment_lengths,
    int const *segment_ids, int const * const *segment_data, void const * const *segment_fracs,
    int fracs_data_type, DBoptlist const *opts)
{
   int            i, j, tot_len;
   int           *intArray;
   long           count;
   DBobject      *obj;

   /*-------------------------------------------------------------
    *  Process option list; build object description.
    *-------------------------------------------------------------*/
   db_ProcessOptlist(DB_GROUPELMAP, opts);
   obj = DBMakeObject(name, DB_GROUPELMAP, 10);

   DBAddIntComponent(obj, "num_segments", num_segments);
   DBAddIntComponent(obj, "fracs_data_type", fracs_data_type);

   /* Write raw data arrays */
   count = num_segments;
   DBWriteComponent(dbfile, obj, "groupel_types", name, "integer",
                    groupel_types, 1, &count);
   DBWriteComponent(dbfile, obj, "segment_lengths", name, "integer",
                    segment_lengths, 1, &count);
   if (segment_ids)
       DBWriteComponent(dbfile, obj, "segment_ids", name, "integer",
                        segment_ids, 1, &count);

   tot_len = 0;
   for (i = 0; i < num_segments; i++)
       tot_len += segment_lengths[i];
   if (tot_len)
   {
       intArray = (int *) malloc(tot_len * sizeof(int));
       tot_len = 0;
       for (i = 0; i < num_segments; i++)
           for (j = 0; j < segment_lengths[i]; j++)
               intArray[tot_len++] = segment_data[i][j];
       count = tot_len;
       DBWriteComponent(dbfile, obj, "segment_data", name, "integer",
                        intArray, 1, &count);
       FREE(intArray);
   }

   /* write out fractional data if we have it */
   if (segment_fracs)
   {
       void *fracsArray;
       char *datatype_str;

       /* write array of frac lengths */
       tot_len = 0;
       intArray = (int *) malloc(num_segments * sizeof(int));
       for (i = 0; i < num_segments; i++)
       {
           int len = segment_fracs[i] == 0 ? 0 : segment_lengths[i];
           intArray[i] = len;
           tot_len += len;
       }
       count = num_segments;
       DBWriteComponent(dbfile, obj, "frac_lengths", name, "integer",
                        intArray, 1, &count);
       FREE(intArray);

       /* build and write out fractional data array */
       if (tot_len)
       {
           fracsArray = (void *) malloc(tot_len * ((fracs_data_type==DB_FLOAT)?sizeof(float):sizeof(double)));
           tot_len = 0;
           for (i = 0; i < num_segments; i++)
           {
               if (segment_fracs[i] == 0)
                   continue;

               for (j = 0; j < segment_lengths[i]; j++)
               {
                   if (fracs_data_type == DB_FLOAT)
                   {
                       float *pfa = (float *) fracsArray;
                       float *psf = (float *) segment_fracs[i];
                       pfa[tot_len++] = psf[j];
                   }
                   else
                   {
                       double *pfa = (double *) fracsArray;
                       double *psf = (double *) segment_fracs[i];
                       pfa[tot_len++] = psf[j];
                   }
               }
           }
           count = tot_len;
           datatype_str = db_GetDatatypeString(fracs_data_type);
           DBWriteComponent(dbfile, obj, "segment_fracs", name, datatype_str, 
                            fracsArray, 1, &count);
           FREE(fracsArray);
           FREE(datatype_str);
       }
   }

   /*-------------------------------------------------------------
    *  Write material object to output file. Request that underlying
    *  memory be freed (the 'TRUE' argument.)
    *-------------------------------------------------------------*/
   DBWriteObject(dbfile, obj, TRUE);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                            db_pdb_PutMrgvar
 *
 *  Purpose
 *
 *      Write a mrg variable object into the open output file.
 *
 *  Programmer
 *
 *      Mark C. Miller, Thu Oct 18 09:35:47 PDT 2007
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
SILO_CALLBACK int
db_pdb_PutMrgvar(DBfile *_dbfile, char const *name, char const *mrgt_name,
    int ncomps, char const * const *compnames,
    int nregns, char const * const *reg_pnames,
    int datatype, void const * const *data, DBoptlist const *optlist)
{
   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   int            i, nstrs;
   char *s=0; int len=0; long llen;
   DBobject      *obj;
   char          *suffix, *datatype_str, tmp1[256], tmp2[256];
   static char   *me = "db_pdb_PutMrgvar";

   /*-------------------------------------------------------------
    *  Initialize global data, and process options.
    *-------------------------------------------------------------*/
   obj = DBMakeObject(name, DB_MRGVAR, 20);

   DBAddStrComponent(obj, "mrgt_name", mrgt_name);

   suffix = "data";
   datatype_str = db_GetDatatypeString(datatype);

   llen = nregns;
   for (i = 0; i < ncomps; i++) {
      char tmp3[256];

      if (compnames && compnames[i])
      {
          sprintf(tmp3, "%s_%s", name, compnames[i]);
          db_mkname(dbfile->pdb, tmp3, suffix, tmp2);
      }
      else
      {
          sprintf(tmp3, "%s_comp%d", name, i);
          db_mkname(dbfile->pdb, tmp3, suffix, tmp2);
      }
      PJ_write_len(dbfile->pdb, tmp2, datatype_str, data[i],
                   1, &llen);

      sprintf(tmp1, "value%d", i);
      DBAddVarComponent(obj, tmp1, tmp2);
   }
   FREE(datatype_str);

   DBAddIntComponent(obj, "ncomps", ncomps);
   DBAddIntComponent(obj, "nregns", nregns);
   DBAddIntComponent(obj, "datatype", datatype);

   if (compnames)
   {
       DBStringArrayToStringList((char const * const *)compnames, ncomps, &s, &len);
       llen = len;
       DBWriteComponent(_dbfile, obj, "compnames", name, "char", s, 1, &llen);
       FREE(s);
   }

   nstrs = nregns;
   if (strchr(reg_pnames[0], '%') != 0)
       nstrs = 1;
   DBStringArrayToStringList((char const * const *)reg_pnames, nstrs, &s, &len);
   llen = len;
   DBWriteComponent(_dbfile, obj, "reg_pnames", name, "char", s, 1, &llen);
   FREE(s);

   DBWriteObject(_dbfile, obj, 0);
   DBFreeObject(obj);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                   db_pdb_SortObjectsByOffset
 *
 *  Purpose: Returned array of sorted indices for a list of object names
 *           sorted by offset within the file.
 *
 *  Programmer: Mark C. Miller, Thu Jul 15 08:06:27 PDT 2010
 *
 *--------------------------------------------------------------------*/
/* Support type for db_hdf5_SortObjectsByOffset */
typedef struct _index_offset_pair_t {
    int index;
    long offset;
} index_offset_pair_t;

/* Support function for db_hdf5_SortObjectsByOffset */
static int compare_index_offset_pair(void const *a1, void const *a2)
{
    index_offset_pair_t *p1 = (index_offset_pair_t*) a1;
    index_offset_pair_t *p2 = (index_offset_pair_t*) a2;
    if (p1->offset < p2->offset) return -1;
    else if (p1->offset > p2->offset) return 1;
    else return 0;
}

SILO_CALLBACK int
db_pdb_SortObjectsByOffset(DBfile *_dbfile, int nobjs,
    char const *const *const names, int *ordering)
{
   DBfile_pdb *dbfile = (DBfile_pdb *) _dbfile;
   static char *me = "db_pdb_SortObjectsByOffset";
   index_offset_pair_t *iop = (index_offset_pair_t*)
       malloc(nobjs * sizeof(index_offset_pair_t));
   int i;

   for (i = 0; i < nobjs; i++)
   {
       iop[i].index = i;
       if (strchr(names[i], ':')) iop[i].offset = LONG_MAX;
       else
       {
           syment *ep = lite_PD_inquire_entry (dbfile->pdb, (char*)names[i], TRUE, NULL);
           if (!ep) iop[i].offset = LONG_MAX;
           else iop[i].offset = PD_entry_address(ep); 
        }
    }

    /* Ok, sort the index/offset pairs */
    qsort(iop, nobjs, sizeof(index_offset_pair_t), compare_index_offset_pair);

    /* Populate ordering array */
    for (i = 0; i < nobjs; i++)
        ordering[i] = iop[i].index;

    free(iop);

    return 0;
}

/*----------------------------------------------------------------------
 *  Routine                                                  db_InitCsg
 *
 *  Purpose
 *
 *      Initialize the csg output package. This involves
 *      setting global data (vars, dim-ids, var-ids), writing standard
 *      dimensions and variables to the output file, and processing the
 *      option list provided by the caller.
 *
 *  Programmer
 *
 *      Mark C. Miller, Wed Aug  3 14:39:03 PDT 2005
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE int
db_InitCsg(DBfile *_dbfile, char const *meshname, DBoptlist const *optlist) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   long           count[3];
   char           tmp[256];

   db_mkname(dbfile->pdb, meshname, "typeflags", tmp);
   if (lite_PD_inquire_entry(dbfile->pdb, tmp, FALSE, NULL) != NULL) {
      db_ResetGlobalData_Csgmesh();
      db_ProcessOptlist(DB_CSGMESH, optlist);
      db_build_shared_names_csgmesh(_dbfile, meshname);
      return 0;
   }

   /*--------------------------------------------------
    *  Process the given option list (this function
    *  modifies the global variable data based on
    *  the option values.)
    *--------------------------------------------------*/
   db_ResetGlobalData_Csgmesh();
   db_ProcessOptlist(DB_CSGMESH, optlist);
   db_build_shared_names_csgmesh(_dbfile, meshname);

   /*  Write some scalars */
   count[0] = 1L;
   if (_csgm._time_set == TRUE)
      PJ_write_len(dbfile->pdb, _csgm._nm_time, "float", &(_csgm._time), 1, count);
   if (_csgm._dtime_set == TRUE)
      PJ_write_len(dbfile->pdb, _csgm._nm_dtime, "double",
                   &(_csgm._dtime), 1, count);

   PJ_write_len(dbfile->pdb, _csgm._nm_cycle, "integer", &(_csgm._cycle),
                1, count);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                                  db_InitPoint
 *
 *  Purpose
 *
 *      Initialize the point output package. This involves
 *      setting global data (vars, dim-ids, var-ids), writing standard
 *      dimensions and variables to the output file, and processing the
 *      option list provided by the caller.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modifications
 *
 *      Al Leibee, Mon Apr 18 07:45:58 PDT 1994
 *      Added _dtime.
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE int
db_InitPoint (DBfile *_dbfile, DBoptlist const *optlist, int ndims, int nels) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   long           count[3];

   /*--------------------------------------------------
    *  Process the given option list (this function
    *  modifies the global variable data based on
    *  the option values.)
    *--------------------------------------------------*/
   db_ResetGlobalData_PointMesh(ndims);
   db_ProcessOptlist(DB_POINTMESH, optlist);

   /*--------------------------------------------------
    *  Assign values to global data.
    *--------------------------------------------------*/
   _pm._nels = nels;

   _pm._minindex = _pm._lo_offset;
   _pm._maxindex = nels - _pm._hi_offset - 1;

   _pm._coordnames[0] = "xpt_data";
   _pm._coordnames[1] = "ypt_data";
   _pm._coordnames[2] = "zpt_data";

   /*------------------------------------------------------
    * Define all attribute arrays and scalars which are
    * likely to be needed. Some require a zonal AND nodal
    * version.
    *-----------------------------------------------------*/

   /*  Write some scalars */
   count[0] = 1L;
   if (_pm._time_set == TRUE) {
      db_mkname(dbfile->pdb, NULL, "time", _pm._nm_time);
      PJ_write_len(dbfile->pdb, _pm._nm_time, "float", &(_pm._time),
                   1, count);
   }

   if (_pm._dtime_set == TRUE) {
      db_mkname(dbfile->pdb, NULL, "dtime", _pm._nm_dtime);
      PJ_write_len(dbfile->pdb, _pm._nm_dtime, "double", &(_pm._dtime),
                   1, count);
   }

   db_mkname(dbfile->pdb, NULL, "cycle", _pm._nm_cycle);
   PJ_write_len(dbfile->pdb, _pm._nm_cycle, "integer", &(_pm._cycle),
                1, count);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                                  db_InitQuad
 *
 *  Purpose
 *
 *      Initialize the quadrilateral output package. This involves
 *      setting global data (vars, dim-ids, var-ids), writing standard
 *      dimensions and variables to the SILO file, and processing the
 *      option list provided by the caller.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modifications
 *
 *  Modifications
 *
 *     Al Leibee, Sun Apr 17 07:54:25 PDT 1994
 *     Added dtime.
 *      Al Leibee, Mon Aug  9 10:30:54 PDT 1993
 *      Converted to new PD_inquire_entry interface.
 *
 *     Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *     Added baseindex[].
 *
 *     Hank Childs, Thu Jan  6 13:51:22 PST 2000
 *     Removed unused variable error.
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE int
db_InitQuad (DBfile *_dbfile, char const *meshname, DBoptlist const *optlist,
             int const *dims, int ndims) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   int            i, is_empty = 1;
   int            nzones, nnodes;
   long           count[3];
   float          a[3];
   char           tmp[1024];
   char          *p=NULL;

   /*--------------------------------------------------
    *  Define number of zones and nodes.
    *--------------------------------------------------*/
   nzones = nnodes = ndims?1:0;

   for (i = 0; i < ndims; i++) {
      nzones *= (dims[i] - 1);
      nnodes *= dims[i];
      if (dims[i] > 0)
          is_empty = 0;
   }

   /*--------------------------------------------------
    *  Process the given option list (this function
    *  modifies the global variable data based on
    *  the option values.)
    *--------------------------------------------------*/

   db_ResetGlobalData_QuadMesh(ndims);
   db_ProcessOptlist(DB_QUADMESH, optlist);
   db_build_shared_names_quadmesh(_dbfile, meshname);

   /*--------------------------------------------------
    *  Determine if this directory has already been
    *  initialized for the given mesh. If so, set
    *  the names of the shared variables (e.g., 'dims')
    *  and return. If not, write out the dimensions
    *  and set the names.
    *--------------------------------------------------*/

   db_mkname(dbfile->pdb, meshname, "dims", tmp);

   if (lite_PD_inquire_entry(dbfile->pdb, tmp, FALSE, NULL) != NULL) {
      return 0;
   }

   /*--------------------------------------------------
    *  Assign values to global data.
    *--------------------------------------------------*/
   _qm._nzones = nzones;
   _qm._nnodes = nnodes;

   for (i = 0; i < ndims; i++) {
      _qm._dims[i] = dims[i];
      _qm._zones[i] = dims[i] - 1;

      _qm._minindex[i] = _qm._lo_offset[i];
      _qm._maxindex_n[i] = dims[i] - _qm._hi_offset[i] - 1;
      _qm._maxindex_z[i] = _qm._maxindex_n[i] - 1;
   }

   /*--------------------------------------------------
    * This directory is uninitialized.
    *
    * Define all attribute arrays and scalars which are
    * likely to be needed. Some require a zonal AND nodal
    * version.
    *-----------------------------------------------------*/

   count[0] = ndims;
   /*  File name contained within meshname */
   p = (char *)strchr(meshname, ':');
   if (p == NULL) {
      PJ_write_len(dbfile->pdb, _qm._nm_dims, "integer", dims, 1, count);
      PJ_write_len(dbfile->pdb, _qm._nm_zones, "integer", _qm._zones, 1, count);
      PJ_write_len(dbfile->pdb, _qm._nm_maxindex_n, "integer", _qm._maxindex_n, 1, count);
      PJ_write_len(dbfile->pdb, _qm._nm_maxindex_z, "integer", _qm._maxindex_z, 1, count);
      PJ_write_len(dbfile->pdb, _qm._nm_minindex, "integer", _qm._minindex, 1, count);
      PJ_write_len(dbfile->pdb, _qm._nm_baseindex, "integer", _qm._baseindex, 1, count);
      a[0] = a[1] = a[2] = 0.5;
      PJ_write_len(dbfile->pdb, _qm._nm_alignz, "float", a, 1, count);
      a[0] = a[1] = a[2] = 0.0;
      PJ_write_len(dbfile->pdb, _qm._nm_alignn, "float", a, 1, count);
   }

   /*  Write some scalars */
   count[0] = 1L;

   if (_qm._time_set == TRUE) {
      PJ_write_len(dbfile->pdb, _qm._nm_time, "float", &(_qm._time), 1, count);
   }
   if (_qm._dtime_set == TRUE) {
      PJ_write_len(dbfile->pdb, _qm._nm_dtime, "double",
                   &(_qm._dtime), 1, count);
   }
   PJ_write_len(dbfile->pdb, _qm._nm_cycle, "integer",
                &(_qm._cycle), 1, count);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                                  db_InitUcd
 *
 *  Purpose
 *
 *      Initialize the uc output package. This involves
 *      setting global data (vars, dim-ids, var-ids), writing standard
 *      dimensions and variables to the output file, and processing the
 *      option list provided by the caller.
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Notes
 *
 *  Modifications
 *
 *      Al Leibee, Mon Aug  9 10:30:54 PDT 1993
 *      Converted to new PD_inquire_entry interface.
 *
 *      Hank Childs, Thu Jan  6 13:51:22 PST 2000
 *      Removed unused variable error.
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE int
db_InitUcd (DBfile *_dbfile, char const *meshname, DBoptlist const *optlist,
            int ndims, int nnodes, int nzones) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   long           count[3];
   float          a[3];
   char           tmp[256];
   char          *p=NULL;

   /*--------------------------------------------------
    *  Process the given option list (this function
    *  modifies the global variable data based on
    *  the option values.)
    *--------------------------------------------------*/
   db_ResetGlobalData_Ucdmesh(ndims, nnodes, nzones);
   db_ProcessOptlist(DB_UCDMESH, optlist);
   db_build_shared_names_ucdmesh(_dbfile, meshname);

   /*--------------------------------------------------
    *  Determine if this directory has already been
    *  initialized. Return if it has.
    *--------------------------------------------------*/

   db_mkname(dbfile->pdb, meshname, "align_zonal", tmp);

   if (lite_PD_inquire_entry(dbfile->pdb, tmp, FALSE, NULL) != NULL) {
      return 0;
   }

   /*--------------------------------------------------
    *  Assign values to global data.
    *--------------------------------------------------*/
   _um._nzones = nzones;
   _um._nnodes = nnodes;

   /*------------------------------------------------------
    * Define all attribute arrays and scalars which are
    * likely to be needed. Some require a zonal AND nodal
    * version.
    *-----------------------------------------------------*/
   count[0] = ndims;
   if (count[0] <= 0)
      return 0;

   /*------------------------------------------------------
    * Assume that if there is a file in the meshname, that 
    * there is possibily another  mesh that may be pointing to,
    * so we'll use that meshes discriptions.
    *-----------------------------------------------------*/
   p = (char *)strchr(meshname, ':');
   if (p == NULL && count[0]) {
      a[0] = a[1] = a[2] = 0.5;
      PJ_write_len(dbfile->pdb, _um._nm_alignz, "float", a, 1, count);
   
      a[0] = a[1] = a[2] = 0.0;
      PJ_write_len(dbfile->pdb, _um._nm_alignn, "float", a, 1, count);
   }

   /*  Write some scalars */
   count[0] = 1L;
   if (_um._time_set == TRUE)
      PJ_write_len(dbfile->pdb, _um._nm_time, "float", &(_um._time), 1, count);
   if (_um._dtime_set == TRUE)
      PJ_write_len(dbfile->pdb, _um._nm_dtime, "double",
                   &(_um._dtime), 1, count);

   PJ_write_len(dbfile->pdb, _um._nm_cycle, "integer", &(_um._cycle),
                1, count);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                              db_InitZonelist
 *
 *  Purpose
 *
 *      Initialize the ucd zonelist output package.  This involves
 *      setting global data and processing the option list provided
 *      by the caller.
 *
 *  Programmer
 *
 *      Jeremy Meredith, May 21 1999
 *
 *  Notes
 *
 *  Modifications
 *
 *     Hank Childs, Thu Jan  6 13:51:22 PST 2000
 *     Put in lint directive for unused arguments. Removed unused
 *     argument dbfile.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
/* ARGSUSED */
PRIVATE int
db_InitZonelist (DBfile *_dbfile, DBoptlist const *optlist)
{
   /*--------------------------------------------------
    *  Process the given option list (this function
    *  modifies the global variable data based on
    *  the option values.)
    *--------------------------------------------------*/
   db_ResetGlobalData_Ucdzonelist();
   db_ProcessOptlist(DB_ZONELIST, optlist);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                          db_InitPHZonelist
 *
 *  Purpose
 *
 *      Initialize the polyhedral zonelist output package.  This involves
 *      setting global data and processing the option list provided
 *      by the caller.
 *
 *  Programmer
 *
 *      Mark C. Miller, July 27, 2004
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
/* ARGSUSED */
PRIVATE int
db_InitPHZonelist (DBfile *_dbfile, DBoptlist const *optlist)
{
   /*--------------------------------------------------
    *  Process the given option list (this function
    *  modifies the global variable data based on
    *  the option values.)
    *--------------------------------------------------*/
   db_ResetGlobalData_phzonelist();
   db_ProcessOptlist(DB_PHZONELIST, optlist);

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                db_build_shared_names_csgmesh
 *
 *  Purpose
 *
 *      Build the names of the shared variables, based on the name
 *      of the current mesh. Store the names in the global variables
 *      (which all begin with the prefix "_nm_".
 *
 *  Programmer
 *
 *      Mark C. Miller, Wed Aug  3 14:39:03 PDT 2005 
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE void
db_build_shared_names_csgmesh(DBfile *_dbfile, char const *meshname) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;

   if (_csgm._time_set == TRUE)
      db_mkname(dbfile->pdb, NULL, "time", _csgm._nm_time);
   if (_csgm._dtime_set == TRUE)
      db_mkname(dbfile->pdb, NULL, "dtime", _csgm._nm_dtime);

   db_mkname(dbfile->pdb, NULL, "cycle", _csgm._nm_cycle);
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                  db_build_shared_names_quadmesh
 *
 *  Purpose
 *
 *      Build the names of the shared variables, based on the name
 *      of the current mesh. Store the names in the global variables
 *      (which all begin with the prefix "_nm_").
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Modifications
 *
 *     Al Leibee, Sun Apr 17 07:54:25 PDT 1994
 *     Added dtime.
 *
 *     Jeremy Meredith, Fri May 21 10:04:25 PDT 1999
 *     Added _nm_baseindex.
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE void
db_build_shared_names_quadmesh (DBfile *_dbfile, char const *meshname) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;

   db_mkname(dbfile->pdb, meshname, "dims", _qm._nm_dims);
   db_mkname(dbfile->pdb, meshname, "zonedims", _qm._nm_zones);
   db_mkname(dbfile->pdb, meshname, "max_index_n", _qm._nm_maxindex_n);
   db_mkname(dbfile->pdb, meshname, "max_index_z", _qm._nm_maxindex_z);
   db_mkname(dbfile->pdb, meshname, "min_index", _qm._nm_minindex);
   db_mkname(dbfile->pdb, meshname, "align_zonal", _qm._nm_alignz);
   db_mkname(dbfile->pdb, meshname, "align_nodal", _qm._nm_alignn);
   db_mkname(dbfile->pdb, meshname, "baseindex", _qm._nm_baseindex);

   if (_qm._time_set == TRUE)
      db_mkname(dbfile->pdb, NULL, "time", _qm._nm_time);
   if (_qm._dtime_set == TRUE)
      db_mkname(dbfile->pdb, NULL, "dtime", _qm._nm_dtime);

   db_mkname(dbfile->pdb, NULL, "cycle", _qm._nm_cycle);
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                              db_ResetGlobalData_phzonelist
 *
 *  Purpose
 *
 *      Reset global data to default values. For internal use only.
 *
 *  Programmer
 *
 *      Mark C. Miller, July 27, 2004 
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE int
db_ResetGlobalData_phzonelist (void) {

   memset(&_phzl, 0, sizeof(_phzl));

   return 0;
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                db_build_shared_names_ucdmesh
 *
 *  Purpose
 *
 *      Build the names of the shared variables, based on the name
 *      of the current mesh. Store the names in the global variables
 *      (which all begin with the prefix "_nm_".
 *
 *  Programmer
 *
 *      Jeffery W. Long, NSSD-B
 *
 *  Modifications
 *
 *     Al Leibee, Mon Apr 18 07:45:58 PDT 1994
 *     Added _dtime.
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE void
db_build_shared_names_ucdmesh (DBfile *_dbfile, char const *meshname) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;

   db_mkname(dbfile->pdb, meshname, "align_zonal", _um._nm_alignz);
   db_mkname(dbfile->pdb, meshname, "align_nodal", _um._nm_alignn);
   if (_um._time_set == TRUE)
      db_mkname(dbfile->pdb, NULL, "time", _um._nm_time);
   if (_um._dtime_set == TRUE)
      db_mkname(dbfile->pdb, NULL, "dtime", _um._nm_dtime);

   db_mkname(dbfile->pdb, NULL, "cycle", _um._nm_cycle);
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 *  Routine                                                    db_mkname
 *
 *  Purpose
 *
 *      Generate a name based on current directory, given name, and
 *      the given suffix.
 *
 *  Modifications:
 *      Sean Ahern, Mon Dec 18 17:48:30 PST 2000
 *      If the "name" starts with "/", then we don't need to determine our
 *      current working directory.  I added this logic path.
 *
 *      Eric Brugger, Fri Apr  6 11:15:37 PDT 2001
 *      I changed a comparison between an integer and NULL to an integer
 *      and 0.
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE void
db_mkname_sized(PDBfile *pdb, char const *name, char const *suffix,
    char *out, size_t outsz)
{
    char   *cwd;
    int     n;
    size_t  used = 0;

    if (!out || outsz == 0)
        return;

    out[0] = '\0';

    /* Check if the "name" starts with "/", indicating an absolute
     * pathname. */
    if (!name || (strncmp(name, "/", 1) != 0))
    {
        /* Get the directory name. */
        if ((cwd = lite_PD_pwd(pdb)))
        {
            n = snprintf(out + used, outsz - used, "%s%s", cwd,
                STR_EQUAL("/", cwd) ? "" : "/");
        } else
        {
            n = snprintf(out + used, outsz - used, "/");
        }

        if (n < 0 || (size_t)n >= outsz - used)
            return;
        used += (size_t)n;
    }

    /* And tack on the file name and extension if supplied. */
    if (name)
    {
        n = snprintf(out + used, outsz - used, "%s", name);
        if (n < 0 || (size_t)n >= outsz - used)
            return;
        used += (size_t)n;
    }
    if (suffix)
    {
        n = snprintf(out + used, outsz - used, "%s%s",
            name ? "_" : "", suffix);
        if (n < 0 || (size_t)n >= outsz - used)
            return;
    }
}
#endif /* PDB_WRITE */

/*----------------------------------------------------------------------
 * Routine                                                  db_InitMulti
 *
 * Purpose
 *
 *    Initialize the quadrilateral output package. This involves
 *    setting global data (vars, dim-ids, var-ids), writing standard
 *    dimensions and variables to the SILO file, and processing the
 *    option list provided by the caller.
 *
 * Programmer
 *
 *    Eric Brugger, January 12, 1996
 *
 * Notes
 *
 * Modifications
 *
 *--------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE int
db_InitMulti (DBfile *_dbfile, DBoptlist const *const optlist) {

   DBfile_pdb    *dbfile = (DBfile_pdb *) _dbfile;
   long           count[3];

   /*--------------------------------------------------
    *  Process the given option list (this function
    *  modifies the global variable data based on
    *  the option values.)
    *--------------------------------------------------*/
   db_ResetGlobalData_MultiMesh();
   db_ProcessOptlist(DB_MULTIMESH, optlist);

   /*------------------------------------------------------
    * Define all attribute arrays and scalars which are
    * likely to be needed. Some require a zonal AND nodal
    * version.
    *-----------------------------------------------------*/

   /*  Write some scalars */
   count[0] = 1L;
   if (_mm._time_set == TRUE) {
      db_mkname(dbfile->pdb, NULL, "time", _mm._nm_time);
      PJ_write_len(dbfile->pdb, _mm._nm_time, "float", &(_mm._time),
                   1, count);
   }

   if (_mm._dtime_set == TRUE) {
      db_mkname(dbfile->pdb, NULL, "dtime", _mm._nm_dtime);
      PJ_write_len(dbfile->pdb, _mm._nm_dtime, "double", &(_mm._dtime),
                   1, count);
   }

   db_mkname(dbfile->pdb, NULL, "cycle", _mm._nm_cycle);
   PJ_write_len(dbfile->pdb, _mm._nm_cycle, "integer", &(_mm._cycle),
                1, count);

   return 0;
}
#endif /* PDB_WRITE */


/*-------------------------------------------------------------------------
 * Function:    db_InitCurve
 *
 * Purpose:     Inititalize global curve data.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.com
 *              May 16, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE void
db_InitCurve (DBoptlist const * const opts) {

   db_ResetGlobalData_Curve () ;
   db_ProcessOptlist (DB_CURVE, opts) ;
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    db_InitDefvars
 *
 * Purpose:     Inititalize global defvars data.
 *
 * Return:      void
 *
 * Programmer:  Mark C. Miller 
 *              March 22, 2006
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------*/
#ifdef PDB_WRITE
PRIVATE void
db_InitDefvars (DBoptlist const *opts) {

   db_ResetGlobalData_Defvars () ;
   db_ProcessOptlist (DB_DEFVARS, opts) ;
}
#endif /* PDB_WRITE */
