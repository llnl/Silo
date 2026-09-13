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

#include "silo_private.h"

/*======================================================================
 *||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
 *======================================================================
 *  File                                                         alloc.c
 *
 *  Purpose
 *
 *     Source file for allocating and freeing DB data structures.
 *
 *  Author
 *
 *      Jeff Long
 *
 *  Routine Summary
 *
 *      mmesh   = DBAllocMultimesh (num)<< Multi-block mesh >>
 *      mvar    = DBAllocMultivar (num) << Multi-block var >>
 *      qmesh   = DBAllocQuadmesh ()    << Quad mesh >>
 *      umesh   = DBAllocUcdmesh ()     << UCD mesh >>
 *      pmesh   = DBAllocPointmesh ()   << Point mesh >>
 *      qvar    = DBAllocQuadvar ()     << Quad var >>
 *      uvar    = DBAllocUcdvar ()      << UCD var >>
 *      var     = V_Alloc ()            << Mesh var >>
 *      elist   = DBAllocEdgelist ()    << Edgelist >>
 *      flist   = DBAllocFacelist ()    << Facelist >>
 *      zlist   = DBAllocZonelist ()    << Zonelist >>
 *      mat     = DBAllocMaterial()     << Material >>
 *      species = DBAllocMatspecies()   << Matspecies >>
 *      array   = DBAllocCompoundarray()<< Compoundarray >>
 *
 *      Also, corresponding routines for 'free'.
 *
 *======================================================================
 *||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
 *=====================================================================*/

/*
 * Modification History
 *
 * Robb Matzke, Tue Oct 25 08:13:04 PDT 1994
 * Added routines for Compound Arrays.
 *
 * Robb Matzke, Thu Nov 3 15:33:48 PST 1994
 * Restructured for device independence.
 *
 * Al Leibee, Tue Jul 26 08:44:01 PDT 1994
 * Replaced composition by species.
 *
 */

/*----------------------------------------------------------------------
 *  Function                                            DBAllocDefvars
 *
 *  Purpose
 *
 *     Allocate and initialize a defvars object.
 *
 *  Programmer:
 *     Mark C. Miller
 *     August 8, 2005
 *
 *----------------------------------------------------------------------*/
PUBLIC DBdefvars *
DBAllocDefvars(int num)
{
    DBdefvars *defv;

    API_BEGIN("DBAllocDefvars", DBdefvars *, NULL) {
        if (NULL == (defv = ALLOC(DBdefvars)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(defv, 0, sizeof(DBdefvars));

        defv->ndefs = num;

        /* Allocate sub-arrays of requested lengths */
        if (num > 0) {
            defv->names = ALLOC_N(char *, num);
            defv->types = ALLOC_N(int, num);
            defv->defns = ALLOC_N(char *, num);

            if (!defv->names || !defv->types || !defv->defns)
            {
                DBFreeDefvars(defv);
                API_ERROR(NULL, E_NOMEM);
            }
        }
    }
    API_END;

    return (defv);
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocMultimesh
 *
 *  Purpose
 *
 *     Allocate and initialize a multi-block mesh object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:12:09 PST 1994
 *      Added error mechanism.
 *
 *      Eric Brugger, Wed Jul  2 13:19:07 PDT 1997
 *      Added code to allocate the dirids array.
 *
 *      Jeremy Meredith, Fri May 21 09:58:34 PDT 1999
 *      Added code to initialize the block and group origins to 1.
 *
 *      Mark C. Miller, Wed Jul 27 19:04:00 PDT 2005
 *      Initialized nblocks member 
 *
 *----------------------------------------------------------------------*/
PUBLIC DBmultimesh *
DBAllocMultimesh(int num)
{
    DBmultimesh   *msh;

    API_BEGIN("DBAllocMultimesh", DBmultimesh *, NULL) {
        if (NULL == (msh = ALLOC(DBmultimesh)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(msh, 0, sizeof(DBmultimesh));

        msh->nblocks = num;
        msh->blockorigin = 1;
        msh->grouporigin = 1;

        /* Allocate sub-arrays of requested lengths */
        if (num > 0) {
            msh->meshids = ALLOC_N(int, num);
            msh->meshnames = ALLOC_N(char *, num);
            msh->meshtypes = ALLOC_N(int, num);
            msh->dirids = ALLOC_N(int, num);

            if (!msh->meshids || !msh->meshtypes || !msh->meshnames ||
                !msh->dirids) {
                DBFreeMultimesh(msh);
                API_ERROR(NULL, E_NOMEM);
            }
        }
    }
    API_END;

    return (msh);
}

/*----------------------------------------------------------------------
 *  Function                                         DBAllocMultimeshadj
 *
 *  Purpose
 *
 *     Allocate and initialize a multi-block mesh adjacency object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:12:09 PST 1994
 *      Added error mechanism.
 *
 *      Eric Brugger, Wed Jul  2 13:19:07 PDT 1997
 *      Added code to allocate the dirids array.
 *
 *      Jeremy Meredith, Fri May 21 09:58:34 PDT 1999
 *      Added code to initialize the block and group origins to 1.
 *
 *      Mark C. Miller, Wed Jul 27 19:04:00 PDT 2005
 *      Initialized nblocks member 
 *
 *----------------------------------------------------------------------*/
PUBLIC DBmultimeshadj *
DBAllocMultimeshadj(int num)
{
    DBmultimeshadj   *mshadj;

    API_BEGIN("DBAllocMultimeshadj", DBmultimeshadj *, NULL) {
        if (NULL == (mshadj = ALLOC(DBmultimeshadj)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(mshadj, 0, sizeof(DBmultimeshadj));

        mshadj->nblocks = num;
        mshadj->blockorigin = 1;

        /* Allocate sub-arrays of requested lengths */
        if (num > 0) {
            mshadj->meshtypes = ALLOC_N(int, num);
            mshadj->nneighbors = ALLOC_N(int, num);

            if (!mshadj->meshtypes || !mshadj->nneighbors) {
                DBFreeMultimeshadj(mshadj);
                API_ERROR(NULL, E_NOMEM);
            }
        }
    }
    API_END;

    return (mshadj);
}
/*----------------------------------------------------------------------
 *  Function                                            DBAllocMultivar
 *
 *  Purpose
 *
 *     Allocate and initialize a multi-block mesh object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:13:34 PST 1994
 *      Added error mechanism
 *
 *      Jeremy Meredith, Fri May 21 09:58:34 PDT 1999
 *      Added code to initialize the block and group origins to 1.
 *
 *      Mark C. Miller, Wed Jul 27 19:04:00 PDT 2005
 *      Initialized nvars member 
 *----------------------------------------------------------------------*/
PUBLIC DBmultivar *
DBAllocMultivar(int num)
{
    DBmultivar    *var;

    API_BEGIN("DBAllocMultivar", DBmultivar *, NULL) {
        if (NULL == (var = ALLOC(DBmultivar)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(var, 0, sizeof(DBmultivar));

        var->nvars = num;
        var->blockorigin = 1;
        var->grouporigin = 1;

        /* Allocate sub-arrays of requested lengths */
        if (num > 0) {
            var->varnames = ALLOC_N(char *, num);
            var->vartypes = ALLOC_N(int, num);

            if (!var->varnames || !var->vartypes) {
                DBFreeMultivar(var);
                API_ERROR(NULL, E_NOMEM);
            }
        }
    }
    API_END;

    return (var);
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocMultimat
 *
 *  Purpose
 *
 *     Allocate and initialize a multi-material object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:13:34 PST 1994
 *      Added error mechanism
 *
 *      Jeremy Meredith, Fri May 21 09:58:34 PDT 1999
 *      Added code to initialize the block and group origins to 1.
 *
 *      Mark C. Miller, Wed Jul 27 19:04:00 PDT 2005
 *      Initialized nmats member 
 *----------------------------------------------------------------------*/
PUBLIC DBmultimat *
DBAllocMultimat(int num)
{
    DBmultimat    *mat;

    API_BEGIN("DBAllocMultimat", DBmultimat *, NULL) {
        if (NULL == (mat = ALLOC(DBmultimat)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(mat, 0, sizeof(DBmultimat));

        mat->nmats = num;
        mat->blockorigin = 1;
        mat->grouporigin = 1;

        /* Allocate sub-arrays of requested lengths */
        if (num > 0) {
            mat->matnames = ALLOC_N(char *, num);

            if (!mat->matnames) {
                DBFreeMultimat(mat);
                API_ERROR(NULL, E_NOMEM);
            }
        }
    }
    API_END;

    return (mat);
}

/*----------------------------------------------------------------------
 *  Function                                      DBAllocMultimatspecies
 *
 *  Programmer
 *     Jeremy Meredith, Sept 18 1998
 *
 *  Purpose
 *
 *     Allocate and initialize a multi-species object.
 *
 *  Modified
 *
 *      Jeremy Meredith, Fri May 21 09:58:34 PDT 1999
 *      Added code to initialize the block and group origins to 1.
 *
 *      Mark C. Miller, Wed Jul 27 19:04:00 PDT 2005
 *      Initialized nspec member 
 *----------------------------------------------------------------------*/
PUBLIC DBmultimatspecies *
DBAllocMultimatspecies(int num)
{
    DBmultimatspecies    *spec;

    API_BEGIN("DBAllocMultimatspecies", DBmultimatspecies *, NULL) {
        if (NULL == (spec = ALLOC(DBmultimatspecies)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(spec, 0, sizeof(DBmultimatspecies));

        spec->nspec = num;
        spec->blockorigin = 1;
        spec->grouporigin = 1;

        /* Allocate sub-arrays of requested lengths */
        if (num > 0) {
            spec->specnames = ALLOC_N(char *, num);

            if (!spec->specnames) {
                DBFreeMultimatspecies(spec);
                API_ERROR(NULL, E_NOMEM);
            }
        }
    }
    API_END;

    return (spec);
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeDefvars
 *
 *  Purpose
 *
 *     Free the space used by the given defvars object.  Also frees
 *     items pointed to by the structure.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeDefvars(DBdefvars *defv)
{
    int            i;

    if (defv == NULL)
        return;

    for (i = 0; i < defv->ndefs; i++) {
        FREE(defv->names[i]);
        FREE(defv->defns[i]);
    }

    FREE(defv->names);
    FREE(defv->types);
    FREE(defv->defns);
    FREE(defv->guihides);
    FREE(defv);
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeMultimesh
 *
 *  Purpose
 *
 *     Free the space used by the given multi mesh object.  Also frees
 *     items pointed to by the structure.
 *
 *  Modificaitions:
 *     Eric Brugger, Wed Jul  2 13:19:07 PDT 1997
 *     I added code to free msh->meshnames to close a memory leak.
 *
 *     Mark C. Miller, Wed Jul 14 20:26:09 PDT 2010
 *     Added support for namescheme options on multi-block objects.
 *     When these options are in use, the ...names member can be null.
 *     So, modified to only delete the ...names member if it is non-null.
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeMultimesh(DBmultimesh *msh)
{
    int            i;

    if (msh == NULL)
        return;

    if (msh->meshnames_alloc)
    {
        FREE(msh->meshnames_alloc);
        FREE(msh->meshnames);
    }
    else if (msh->meshnames)
    {
        for (i = 0; i < msh->nblocks; i++) {
            FREE(msh->meshnames[i]);
        }
        FREE(msh->meshnames);
    }

    if (msh->groupnames)
    {
        for(i=0;i<msh->lgroupings;i++)
            FREE(msh->groupnames[i]);
        FREE(msh->groupnames);
    }
    if (msh->groupings)
        FREE(msh->groupings);

    FREE(msh->extents);
    FREE(msh->zonecounts);
    FREE(msh->has_external_zones);
    FREE(msh->meshids);
    FREE(msh->meshtypes);
    FREE(msh->dirids);
    FREE(msh->mrgtree_name);
    FREE(msh->file_ns);
    FREE(msh->block_ns);
    FREE(msh->empty_list);
    FREE(msh);
}

PUBLIC int
DBIsEmptyMultimesh(DBmultimesh const *mmsh)
{
    if (mmsh && mmsh->nblocks!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                          DBFreeMultimeshadj
 *
 *  Purpose
 *
 *     Free the space used by the given multi mesh adjacency object. Also
 *     frees items pointed to by the structure.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeMultimeshadj(DBmultimeshadj *mshadj)
{
    int            i;
    int lneighbors = 0;

    if (mshadj == NULL)
        return;

    /* compute how long various arrays are */
    for (i = 0; i < mshadj->nblocks; i++)
        lneighbors += mshadj->nneighbors[i];

    if (mshadj->nodelists) {
       for (i = 0; i < lneighbors; i++)
          FREE(mshadj->nodelists[i]);
       FREE(mshadj->nodelists);
    }

    if (mshadj->zonelists) {
       for (i = 0; i < lneighbors; i++)
          FREE(mshadj->zonelists[i]);
       FREE(mshadj->zonelists);
    }

    FREE(mshadj->meshtypes);
    FREE(mshadj->nneighbors);
    FREE(mshadj->neighbors);
    FREE(mshadj->back);
    FREE(mshadj->lnodelists);
    FREE(mshadj->lzonelists);
    FREE(mshadj);
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeMultivar
 *
 *  Purpose
 *
 *     Free the space used by the given multi variable object.  Also frees
 *     items pointed to by the structure.
 *
 *  Modificaitions:
 *     Eric Brugger, Wed Jul  2 13:19:07 PDT 1997
 *     I added code to free mv->varnames to close a memory leak.
 *
 *     Mark C. Miller, Thu Oct 29 15:55:34 PDT 2009
 *     Added free for mmesh_name
 *
 *     Mark C. Miller, Wed Jul 14 20:26:09 PDT 2010
 *     Added support for namescheme options on multi-block objects.
 *     When these options are in use, the ...names member can be null.
 *     So, modified to only delete the ...names member if it is non-null.
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeMultivar (DBmultivar *mv)
{
     int   i;

     if (mv == NULL)
          return;

     if (mv->varnames_alloc)
     {
         FREE(mv->varnames_alloc);
         FREE(mv->varnames);
     }
     else if (mv->varnames)
     {
         for (i = 0; i < mv->nvars; i++) {
              FREE(mv->varnames[i]);
         }
         FREE(mv->varnames);
     }

     if (mv->region_pnames)
     {
         for (i = 0; mv->region_pnames[i]; i++)
             FREE(mv->region_pnames[i]);
         FREE(mv->region_pnames);
     }

     FREE(mv->vartypes);
     FREE(mv->mmesh_name);
     FREE(mv->extents);
     FREE(mv->file_ns);
     FREE(mv->block_ns);
     FREE(mv->empty_list);
     FREE(mv);
}

PUBLIC int
DBIsEmptyMultivar(DBmultivar const *mvar)
{
    if (mvar && mvar->nvars!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeMultimat
 *
 *  Purpose
 *
 *     Free the space used by the given multi material object.  Also frees
 *     items pointed to by the structure.
 *
 *  Modifications
 *     Sean Ahern, Fri Jun 21 10:56:49 PDT 1996
 *     Freed a pointer we were forgetting about.
 *
 *     Mark C. Miller, Mon Aug  7 17:03:51 PDT 2006
 *     Added code to deal with material_names, matcolors and other
 *     stuff that has been added in past several years
 *
 *     Mark C. Miller, Thu Oct 29 15:55:34 PDT 2009
 *     Added free for mmesh_name
 *
 *     Mark C. Miller, Wed Jul 14 20:26:09 PDT 2010
 *     Added support for namescheme options on multi-block objects.
 *     When these options are in use, the ...names member can be null.
 *     So, modified to only delete the ...names member if it is non-null.
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeMultimat (DBmultimat *mat)
{
     int   i;

     if (mat == NULL)
          return;

     if (mat->matnames_alloc)
     {
         FREE(mat->matnames_alloc);
         FREE(mat->matnames);
     }
     else if (mat->matnames)
     {
         for (i = 0; i < mat->nmats; i++) {
              FREE(mat->matnames[i]);
         }
         FREE(mat->matnames);
     }
     if (mat->material_names)
     {
         for (i = 0; i < mat->nmatnos; i++)
             FREE(mat->material_names[i]);
         FREE(mat->material_names);
     }
     if (mat->matcolors)
     {
         for (i = 0; i < mat->nmatnos; i++)
             FREE(mat->matcolors[i]);
         FREE(mat->matcolors);
     }

     FREE(mat->mixlens);
     FREE(mat->matcounts);
     FREE(mat->matlists);
     FREE(mat->matnos);
     FREE(mat->mmesh_name);
     FREE(mat->file_ns);
     FREE(mat->block_ns);
     FREE(mat->empty_list);
     FREE(mat);
}

PUBLIC int
DBIsEmptyMultimat(DBmultimat const *mmat)
{
    if (mmat && mmat->nmats!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                       DBFreeMultimatspecies
 *
 *  Programmer
 *     Jeremy Meredith, Sept 18 1998
 *
 *  Purpose
 *
 *     Free the space used by the given multi material species object.  
 *     Also frees items pointed to by the structure.
 *
 *  Modifications
 *
 *    Mark C. Miller, Mon Aug  7 17:03:51 PDT 2006
 *    Added code to free nmatspec
 *
 *    Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *    Added names and colors for species.
 *
 *     Mark C. Miller, Wed Jul 14 20:26:09 PDT 2010
 *     Added support for namescheme options on multi-block objects.
 *     When these options are in use, the ...names member can be null.
 *     So, modified to only delete the ...names member if it is non-null.
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeMultimatspecies (DBmultimatspecies *spec)
{
     int   i, j, k;

     if (spec == NULL)
          return;

     if (spec->species_names)
     {
         for(i=0,k=0;i<spec->nmat;i++)
         {
             for(j=0;j<spec->nmatspec[i];j++,k++)
                 FREE(spec->species_names[k]);
         }
         FREE(spec->species_names);
     }

     if (spec->speccolors)
     {
         for(i=0,k=0;i<spec->nmat;i++)
         {
             for(j=0;j<spec->nmatspec[i];j++,k++)
                 FREE(spec->speccolors[k]);
         }
         FREE(spec->speccolors);
     }

     if (spec->specnames_alloc)
     {
         FREE(spec->specnames_alloc);
         FREE(spec->specnames);
     }
     else if (spec->specnames)
     {
         for (i = 0; i < spec->nspec; i++) {
              FREE(spec->specnames[i]);
         }
         FREE(spec->specnames);
     }

     FREE(spec->nmatspec);
     FREE(spec->file_ns);
     FREE(spec->block_ns);
     FREE(spec->empty_list);
     FREE(spec);
}

PUBLIC int
DBIsEmptyMultimatspecies(DBmultimatspecies const *mspec)
{
    if (mspec && mspec->nspec!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                              DBAllocCsgmesh
 *
 *  Purpose
 *
 *     Allocate and initialize a CSG mesh object.
 *
 * Programmer:  Mark C. Miller
 *              Wed Jul 27 14:22:03 PDT 2005
 *----------------------------------------------------------------------*/
PUBLIC DBcsgmesh *
DBAllocCsgmesh(void)
{
    DBcsgmesh    *msh;

    API_BEGIN("DBAllocCsgmesh", DBcsgmesh *, NULL) {
        if (NULL == (msh = ALLOC(DBcsgmesh)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(msh, 0, sizeof(DBcsgmesh));

        msh->block_no = -1;
        msh->group_no = -1;
    }
    API_END;

    return (msh);
}

/*----------------------------------------------------------------------
 *  Function                                               DBFreeCsgmesh
 *
 *  Purpose
 *
 *     Free the space used by the given CSG mesh object.  Also frees
 *     items pointed to by the structure.
 *
 *  Programmer:  Mark C. Miller
 *               Wed Jul 27 14:22:03 PDT 2005
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeCsgmesh(DBcsgmesh *msh)
{
    int i;

    if (msh == NULL)
        return;

    for (i = 0; i < msh->ndims; i++) {
        FREE(msh->labels[i]);
        FREE(msh->units[i]);
    }

    if (msh->bndnames && msh->nbounds)
    {
        for (i = 0; i < msh->nbounds; i++)
            FREE(msh->bndnames[i]);
    }

    FREE(msh->typeflags);
    FREE(msh->bndids);
    FREE(msh->coeffs);
    FREE(msh->coeffidx);
    FREE(msh->bndnames);
    FREE(msh->name);
    FREE(msh->mrgtree_name);

    DBFreeCSGZonelist(msh->zones);

    FREE(msh);
}

PUBLIC int
DBIsEmptyCsgmesh(DBcsgmesh const *msh)
{
    if (msh && msh->nbounds!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocQuadmesh
 *
 *  Purpose
 *
 *     Allocate and initialize a quad mesh object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:14:44 PST 1994
 *      Added error mechanism
 *
 *      Jeremy Meredith, Fri May 21 09:58:34 PDT 1999
 *      Added code to initialize the block and group numbers to -1.
 *
 *----------------------------------------------------------------------*/
PUBLIC DBquadmesh *
DBAllocQuadmesh(void)
{
    DBquadmesh    *msh;

    API_BEGIN("DBAllocQuadmesh", DBquadmesh *, NULL) {
        if (NULL == (msh = ALLOC(DBquadmesh)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(msh, 0, sizeof(DBquadmesh));

        msh->block_no = -1;
        msh->group_no = -1;
    }
    API_END;

    return (msh);
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeQuadmesh
 *
 *  Purpose
 *
 *     Free the space used by the given quad mesh object.  Also frees
 *     items pointed to by the structure.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeQuadmesh(DBquadmesh *msh)
{
    int            i;

    if (msh == NULL)
        return;

    for (i = 0; i < 3; i++) {
        FREE(msh->coords[i]);
        FREE(msh->labels[i]);
        FREE(msh->units[i]);
    }

    FREE(msh->name);
    FREE(msh->mrgtree_name);
    FREE(msh);
}

PUBLIC int
DBIsEmptyQuadmesh(DBquadmesh const *msh)
{
#ifndef _WIN32
#warning CHECK THIS LOGIC. IF ANY DIM>0, IT IS NOT EMPTY
#endif
    int i, is_empty = 1;

    for (i = 0; msh && i < msh->ndims; i++)
    {
        if (msh->dims[i] > 0)
        {
            is_empty = 0;
            break;
        }
    }
    return is_empty;
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocPointmesh
 *
 *  Purpose
 *
 *     Allocate and initialize a point mesh object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:16:44 PST 1994
 *      Added error mechanism
 *
 *      Jeremy Meredith, Fri May 21 09:58:34 PDT 1999
 *      Added code to initialize the block and group numbers to -1.
 *
 *----------------------------------------------------------------------*/
PUBLIC DBpointmesh *
DBAllocPointmesh(void)
{
    DBpointmesh   *msh;

    API_BEGIN("DBAllocPointmesh", DBpointmesh *, NULL) {
        if (NULL == (msh = ALLOC(DBpointmesh)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(msh, 0, sizeof(DBpointmesh));

        msh->block_no = -1;
        msh->group_no = -1;
    }
    API_END;

    return (msh);
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreePointmesh
 *
 *  Purpose
 *
 *     Free the space used by the given point mesh object.  Also frees
 *     items pointed to by the structure.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreePointmesh(DBpointmesh *msh)
{
    int            i;

    if (msh == NULL)
        return;

    for (i = 0; i < 3; i++) {
        FREE(msh->coords[i]);
        FREE(msh->labels[i]);
        FREE(msh->units[i]);
    }

    FREE(msh->gnodeno);
    FREE(msh->name);
    FREE(msh->title);
    FREE(msh->mrgtree_name);
    FREE(msh);
}

PUBLIC int
DBIsEmptyPointmesh(DBpointmesh const *msh)
{
    if (msh && msh->nels!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                              DBAllocMeshvar
 *
 *  Purpose
 *
 *     Allocate and initialize a generic mesh variable object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:18:20 PST 1994
 *      Added error mechanism
 *----------------------------------------------------------------------*/
PUBLIC DBmeshvar *
DBAllocMeshvar(void)
{
    DBmeshvar     *var;

    API_BEGIN("DBAllocMeshvar", DBmeshvar *, NULL) {
        if (NULL == (var = ALLOC(DBmeshvar)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(var, 0, sizeof(DBmeshvar));
    }
    API_END;

    return (var);
}

/*----------------------------------------------------------------------
 *  Function                                               DBFreeMeshvar
 *
 *  Purpose
 *
 *     Free the space used by the given mesh var object.  Also frees
 *     items pointed to by the structure.
 *
 *
 * Modifications:
 *      Sean Ahern, Fri Aug  3 12:53:04 PDT 2001
 *      Fixed a problem with freeing a partially-retried object.
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeMeshvar(DBmeshvar *var)
{
    int            i;

    if (var == NULL)
        return;

    if (var->vals != NULL)
    {
        for (i = 0; i < var->nvals; i++) {
            FREE(var->vals[i]);
        }
    }

    if (var->region_pnames)
    {
        for (i = 0; var->region_pnames[i]; i++)
            FREE(var->region_pnames[i]);
        FREE(var->region_pnames);
    }

    FREE(var->vals);
    FREE(var->name);
    FREE(var->units);
    FREE(var->label);
    FREE(var->meshname);
    FREE(var);
}

PUBLIC int
DBIsEmptyMeshvar(DBmeshvar const *var)
{
    if (var && var->nels!=0) return 0;
    return 1;
}

PUBLIC DBpointvar *
DBAllocPointvar()
{
    return DBAllocMeshvar();
}

PUBLIC void
DBFreePointvar(DBpointvar *var)
{
    DBFreeMeshvar(var);
}

PUBLIC int
DBIsEmptyPointvar(DBpointvar const *pv)
{
    return DBIsEmptyMeshvar(pv);
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocUcdmesh
 *
 *  Purpose
 *
 *     Allocate and initialize a ucd mesh object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:19:21 PST 1994
 *      Added error mechanism.
 *
 *      Jeremy Meredith, Fri May 21 09:58:34 PDT 1999
 *      Added code to initialize the block and group numbers to -1.
 *
 *----------------------------------------------------------------------*/
PUBLIC DBucdmesh *
DBAllocUcdmesh(void)
{
    DBucdmesh     *msh;

    API_BEGIN("DBAllocUcdmesh", DBucdmesh *, NULL) {
        if (NULL == (msh = ALLOC(DBucdmesh)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(msh, 0, sizeof(DBucdmesh));

        msh->block_no = -1;
        msh->group_no = -1;
    }
    API_END;

    return (msh);
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeUcdmesh
 *
 *  Purpose
 *
 *     Free the space used by the given ucd mesh object.  Also frees
 *     items pointed to by the structure.
 *
 *  Modifications:
 *
 *    Lisa J. Roberts, Tue Mar 30 17:10:11 PST 1999
 *    I added code to free the new nodeno array.
 *
 *    Mark C. Miller, Wed Jul 28 11:09:42 PDT 2004
 *    Added missing call to free optional gnodeno array
 *    Added call to free new, optional, polyhedral zonelist
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeUcdmesh(DBucdmesh *msh)
{
    int            i;

    if (msh == NULL)
        return;

    for (i = 0; i < 3; i++) {
        FREE(msh->coords[i]);
        FREE(msh->labels[i]);
        FREE(msh->units[i]);
    }

    DBFreeFacelist(msh->faces);
    DBFreeZonelist(msh->zones);
    DBFreeEdgelist(msh->edges);
    DBFreePHZonelist(msh->phzones);

    FREE(msh->nodeno);
    FREE(msh->gnodeno);
    FREE(msh->name);
    FREE(msh->mrgtree_name);
    FREE(msh);
}

PUBLIC int
DBIsEmptyUcdmesh(DBucdmesh const *msh)
{
    if (msh)
    {
        /* This logic supports Kerbel's funky empty ucdmesh */
        if (msh->nnodes == 1 && ((msh->zones == 0 || msh->zones->nzones == 0))) return 1;
        if (msh->nnodes > 0) return 0;
    }
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                               DBAllocCsgvar
 *
 *  Purpose
 *
 *     Allocate and initialize a CSG var object.
 *
 *  Programmer:  Mark C. Miller
 *               Wed Jul 27 14:22:03 PDT 2005
 *----------------------------------------------------------------------*/
PUBLIC DBcsgvar *
DBAllocCsgvar(void)
{
    DBcsgvar     *csgvar;

    API_BEGIN("DBAllocCsgvar", DBcsgvar *, NULL) {
        if (NULL == (csgvar = ALLOC(DBcsgvar)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(csgvar, 0, sizeof(DBcsgvar));
    }
    API_END;

    return (csgvar);
}

/*----------------------------------------------------------------------
 *  Function                                                DBFreeCsgvar
 *
 *  Purpose
 *
 *     Free the space used by the given CSG var object.
 *
 *  Programmer:  Mark C. Miller
 *               Wed Jul 27 14:22:03 PDT 2005
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeCsgvar(DBcsgvar *var)
{
    int            i;

    if (var == NULL)
        return;

    if (var->vals != NULL) {
        for (i = 0; i < var->nvals; i++) {
            FREE(var->vals[i]);
        }
    }

    if (var->region_pnames)
    {
        for (i = 0; var->region_pnames[i]; i++)
            FREE(var->region_pnames[i]);
        FREE(var->region_pnames);
    }

    FREE(var->vals);
    FREE(var->name);
    FREE(var->label);
    FREE(var->units);
    FREE(var->meshname);
    FREE(var);
}

PUBLIC int
DBIsEmptyCsgvar(DBcsgvar const *var)
{
    if (var && var->nels!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocQuadvar
 *
 *  Purpose
 *
 *     Allocate and initialize a quad var object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:20:11 PST 1994
 *      Added error mechanism
 *----------------------------------------------------------------------*/
PUBLIC DBquadvar *
DBAllocQuadvar(void)
{
    DBquadvar     *qvar;

    API_BEGIN("DBAllocQuadvar", DBquadvar *, NULL) {
        if (NULL == (qvar = ALLOC(DBquadvar)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(qvar, 0, sizeof(DBquadvar));
    }
    API_END;

    return (qvar);
}

/*----------------------------------------------------------------------
 *  Function                                              DBResetQuadvar
 *
 *  Purpose
 *
 *     Reset a quad variable.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBResetQuadvar(DBquadvar *qv)
{

    memset(qv, 0, sizeof(DBquadvar));
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeQuadvar
 *
 *  Purpose
 *
 *     Free the space used by the given quad mesh object.  Also frees
 *     items pointed to by the structure.
 *
 *  Modifications:
 *      Sean Ahern, Thu Jul  9 16:18:51 PDT 1998
 *      Freed the mixvals, fixing a memory leak.
 *
 *      Sean Ahern, Thu Aug  2 12:51:36 PDT 2001
 *      Fixed the case where the variable might only be partially filled,
 *      due to the read mask.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeQuadvar(DBquadvar *var)
{
    int            i;

    if (var == NULL)
        return;

    if (var->vals != NULL) {
        for (i = 0; i < var->nvals; i++) {
            FREE(var->vals[i]);
            if (var->mixvals != NULL)
                FREE(var->mixvals[i]);
        }
    }

    if (var->region_pnames)
    {
        for (i = 0; var->region_pnames[i]; i++)
            FREE(var->region_pnames[i]);
        FREE(var->region_pnames);
    }

    FREE(var->vals);
    FREE(var->mixvals);
    FREE(var->name);
    FREE(var->label);
    FREE(var->units);
    FREE(var->meshname);
    FREE(var);
}

PUBLIC int
DBIsEmptyQuadvar(DBquadvar const *var)
{
#ifndef _WIN32
#warning CHECK THIS LOGIC
#endif
    int i, is_empty = 1;

    for (i = 0; var && i < var->ndims; i++)
    {
        if (var->dims[i] > 0)
        {
            is_empty = 0;
            break;
        }
    }
    return is_empty;
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocUcdvar
 *
 *  Purpose
 *
 *     Allocate and initialize a ucd variable object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:21:11 PST 1994
 *      Added error mechanism
 *----------------------------------------------------------------------*/
PUBLIC DBucdvar *
DBAllocUcdvar(void)
{
    DBucdvar      *uvar;

    API_BEGIN("DBAllocUcdvar", DBucdvar *, NULL) {
        if (NULL == (uvar = ALLOC(DBucdvar)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        DBResetUcdvar(uvar);
    }
    API_END;

    return (uvar);
}

/*----------------------------------------------------------------------
 *  Function                                               DBResetUcdvar
 *
 *  Purpose
 *
 *     Reset a ucd variable.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBResetUcdvar(DBucdvar *uv)
{

    memset(uv, 0, sizeof(DBucdvar));
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeUcdvar
 *
 *  Purpose
 *
 *     Free the space used by the given ucd var object.  Also frees
 *     items pointed to by the structure.
 *
 *  Modifications
 *     Eric Brugger, Wed Sep  1 11:35:24 PDT 1999
 *     Freed the mixvals, fixing a memory leak.
 *
 *     Sean Ahern, Fri Aug  3 12:53:31 PDT 2001
 *     Fixed a problem with freeing a partially-read object.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeUcdvar(DBucdvar *var)
{
    int            i;

    if (var == NULL)
        return;

    if (var->vals != NULL)
    {
        for (i = 0; i < var->nvals; i++) {
            FREE(var->vals[i]);
            if (var->mixvals != NULL)
                FREE(var->mixvals[i]);
        }
    }

    if (var->region_pnames)
    {
        for (i = 0; var->region_pnames[i]; i++)
            FREE(var->region_pnames[i]);
        FREE(var->region_pnames);
    }

    FREE(var->vals);
    FREE(var->mixvals);
    FREE(var->name);
    FREE(var->label);
    FREE(var->units);
    FREE(var->meshname);
    FREE(var);
}

PUBLIC int
DBIsEmptyUcdvar(DBucdvar const *var)
{
    if (var && var->nels!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocZonelist
 *
 *  Purpose
 *
 *     Allocate and initialize a zonelist object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:22:06 PST 1994
 *      Added error mechanism
 *----------------------------------------------------------------------*/
PUBLIC DBzonelist *
DBAllocZonelist(void)
{
    DBzonelist    *zl;

    API_BEGIN("DBAllocZonelist", DBzonelist *, NULL) {
        if (NULL == (zl = ALLOC(DBzonelist)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(zl, 0, sizeof(DBzonelist));
    }
    API_END;

    return (zl);
}

PUBLIC int
DBIsEmptyZonelist(DBzonelist const *zl)
{
    if (zl && zl->nzones!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocPHZonelist
 *
 *  Purpose
 *
 *     Allocate and initialize a DBphzonelist object.
 *
 *----------------------------------------------------------------------*/
PUBLIC DBphzonelist *
DBAllocPHZonelist(void)
{
    DBphzonelist    *phzl;

    API_BEGIN("DBAllocPHZonelist", DBphzonelist *, NULL) {
        if (NULL == (phzl = ALLOC(DBphzonelist)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(phzl, 0, sizeof(DBphzonelist));
    }
    API_END;

    return (phzl);
}

PUBLIC int
DBIsEmptyPHZonelist(DBphzonelist const *zl)
{
    if (zl && zl->nfaces!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                          DBAllocCSGZonelist
 *
 *  Purpose
 *
 *     Allocate and initialize a DBcsgzonelist object.
 *
 *----------------------------------------------------------------------*/
PUBLIC DBcsgzonelist *
DBAllocCSGZonelist(void)
{
    DBcsgzonelist    *csgzl;

    API_BEGIN("DBAllocCSGZonelist", DBcsgzonelist *, NULL) {
        if (NULL == (csgzl = ALLOC(DBcsgzonelist)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(csgzl, 0, sizeof(DBcsgzonelist));
    }
    API_END;

    return (csgzl);
}

PUBLIC int
DBIsEmptyCSGZonelist(DBcsgzonelist const *zl)
{
    if (zl && zl->nzones!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeZonelist
 *
 *  Purpose
 *
 *     Release all storage associated with the given zonelist.
 *
 *  Modified
 *
 *    Mark C. Miller, Wed Jul 28 10:53:35 PDT 2004
 *    Added missing free for gzoneno
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeZonelist(DBzonelist *list)
{
    if (list == NULL)
        return;

    FREE(list->shapecnt);
    FREE(list->shapesize);
    FREE(list->shapetype);
    FREE(list->nodelist);
    FREE(list->zoneno);
    FREE(list->gzoneno);
    FREE(list);
}

/*----------------------------------------------------------------------
 *  Function                                            DBFreePHZonelist
 *
 *  Purpose
 *
 *     Release all storage associated with the given DBphzonelist.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreePHZonelist(DBphzonelist *list)
{
    if (list == NULL)
        return;

    FREE(list->nodecnt);
    FREE(list->nodelist);
    FREE(list->extface);
    FREE(list->facecnt);
    FREE(list->facelist);
    FREE(list->zoneno);
    FREE(list->gzoneno);
    FREE(list);
}

/*----------------------------------------------------------------------
 *  Function                                           DBFreeCSGZonelist
 *
 *  Purpose
 *
 *     Release all storage associated with the given DBcsgzonelist.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeCSGZonelist(DBcsgzonelist *list)
{
    if (list == NULL)
        return;

    if (list->zonenames && list->nzones)
    {
        int i;
        for (i = 0; i < list->nzones; i++)
            FREE(list->zonenames[i]);
    }

    if (list->regnames && list->nregs)
    {
        int i;
        for (i = 0; i < list->nregs; i++)
            FREE(list->regnames[i]);
    }

    FREE(list->typeflags);
    FREE(list->leftids);
    FREE(list->rightids);
    FREE(list->xform);
    FREE(list->zonelist);
    FREE(list->zonenames);
    FREE(list->regnames);

    FREE(list);
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocEdgelist
 *
 *  Purpose
 *
 *     Allocate and initialize a edgelist object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:22:55 PST 1994
 *      Added error mechanism
 *----------------------------------------------------------------------*/
PUBLIC DBedgelist *
DBAllocEdgelist(void)
{
    DBedgelist    *el;

    API_BEGIN("DBAllocEdgelist", DBedgelist *, NULL) {
        if (NULL == (el = ALLOC(DBedgelist)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(el, 0, sizeof(DBedgelist));
    }
    API_END;

    return (el);
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeEdgelist
 *
 *  Purpose
 *
 *     Release all storage associated with the given edgelist.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeEdgelist(DBedgelist *list)
{
    if (list == NULL)
        return;

    FREE(list->edge_beg);
    FREE(list->edge_end);
    FREE(list);
}

/*----------------------------------------------------------------------
 *  Function                                            DBAllocFacelist
 *
 *  Purpose
 *
 *     Allocate and initialize a facelist object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:23:59 PST 1994
 *      Added error mechanism
 *----------------------------------------------------------------------*/
PUBLIC DBfacelist *
DBAllocFacelist(void)
{
    DBfacelist    *fl;

    API_BEGIN("DBAllocFacelist", DBfacelist *, NULL) {
        if (NULL == (fl = ALLOC(DBfacelist)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(fl, 0, sizeof(DBfacelist));
    }
    API_END;

    return (fl);
}

PUBLIC int
DBIsEmptyFacelist(DBfacelist const *fl)
{
    if (fl && fl->nfaces!=0) return 0;
    return 1;
}

/*----------------------------------------------------------------------
 *  Function                                             DBFreeFacelist
 *
 *  Purpose
 *
 *     Release all storage associated with the given facelist.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeFacelist(DBfacelist *list)
{
    if (list == NULL)
        return;

    FREE(list->shapecnt);
    FREE(list->shapesize);
    FREE(list->nodelist);
    FREE(list->types);
    FREE(list->typelist);
    FREE(list->nodeno);
    FREE(list->zoneno);
    FREE(list);
}

/*----------------------------------------------------------------------
 *  Function                                           DBAllocMaterial
 *
 *  Purpose
 *
 *     Allocate and initialize a material-data object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:24:51 PST 1994
 *      Added error mechanism
 *----------------------------------------------------------------------*/
PUBLIC DBmaterial *
DBAllocMaterial(void)
{
    DBmaterial    *mats;

    API_BEGIN("DBAllocMaterial", DBmaterial *, NULL) {
        if (NULL == (mats = ALLOC(DBmaterial)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(mats, 0, sizeof(DBmaterial));
    }
    API_END;

    return (mats);
}
/*----------------------------------------------------------------------
 *  Function                                            DBFreeMaterial
 *
 *  Purpose
 *
 *     Release all storage associated with the given material object.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeMaterial(DBmaterial *mats)
{
    int i;
    if (mats == NULL)
        return;

    if (mats->matnames)
    {
        for(i=0;i<mats->nmat;i++)
            FREE(mats->matnames[i]);
        FREE(mats->matnames);
    }

    if (mats->matcolors)
    {
        for(i=0;i<mats->nmat;i++)
            FREE(mats->matcolors[i]);
        FREE(mats->matcolors);
    }

    FREE(mats->name);
    FREE(mats->matnos);
    FREE(mats->matlist);
    FREE(mats->mix_vf);
    FREE(mats->mix_next);
    FREE(mats->mix_zone);
    FREE(mats->mix_mat);
    FREE(mats->meshname);
    FREE(mats);
}

PUBLIC int
DBIsEmptyMaterial(DBmaterial const *mats)
{
    int i, is_empty = 1;

    for (i = 0; mats && i < mats->ndims; i++)
    {
        if (mats->dims[i] > 0)
        {
            is_empty = 0;
            break;
        }
    }
    return is_empty;
}

/*----------------------------------------------------------------------
 *  Function                                          DBAllocMatspecies
 *
 *  Purpose
 *
 *     Allocate and initialize a matspecies-data object.
 *
 *  Modified
 *
 *      Robb Matzke, Tue Nov 8 07:25:42 PST 1994
 *      Added error mechanism
 *----------------------------------------------------------------------*/
PUBLIC DBmatspecies *
DBAllocMatspecies(void)
{
    DBmatspecies  *species;

    API_BEGIN("DBAllocMatspecies", DBmatspecies *, NULL) {
        if (NULL == (species = ALLOC(DBmatspecies)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(species, 0, sizeof(DBmatspecies));
    }
    API_END;

    return (species);
}

/*----------------------------------------------------------------------
 *  Function                                           DBFreeMatspecies
 *
 *  Purpose
 *
 *     Release all storage associated with the given matspecies object.
 *
 *  Modifications:
 *
 *   Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *   Added names and colors for species.
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeMatspecies(DBmatspecies *species)
{
    int i, j, k;

    if (species == NULL)
        return;

    if (species->specnames)
    {
        for(i=0,k=0;i<species->nmat;i++)
        {
            for(j=0;j<species->nmatspec[i];j++,k++)
                FREE(species->specnames[k]);
        }
        FREE(species->specnames);
    }

    if (species->speccolors)
    {
        for(i=0,k=0;i<species->nmat;i++)
        {
            for(j=0;j<species->nmatspec[i];j++,k++)
                FREE(species->speccolors[k]);
        }
        FREE(species->speccolors);
    }

    FREE(species->name);
    FREE(species->matname);
    FREE(species->nmatspec);
    FREE(species->species_mf);
    FREE(species->speclist);
    FREE(species->mix_speclist);
    FREE(species);
}

PUBLIC int
DBIsEmptyMatspecies(DBmatspecies const *species)
{
    int i, is_empty = 1;

    if (species->nspecies_mf == 0)
        return 1;

    for (i = 0; species && i < species->ndims; i++)
    {
        if (species->dims[i] > 0)
        {
            is_empty = 0;
            break;
        }
    }
    return is_empty;
}

/*-------------------------------------------------------------------------
 * Function:    DBAllocCompoundarray
 *
 * Purpose:     Allocate a compound array object
 *
 * Return:      Success:        pointer to object
 *
 *              Failure:        NULL
 *
 * Programmer:  matzke@viper
 *              Tue Nov  8 07:26:44 PST 1994
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PUBLIC DBcompoundarray *
DBAllocCompoundarray(void)
{

    DBcompoundarray *array;

    API_BEGIN("DBAllocCompoundarray", DBcompoundarray *, NULL) {
        if (NULL == (array = ALLOC(DBcompoundarray)))
            API_ERROR(NULL, E_NOMEM);

        memset(array, 0, sizeof(DBcompoundarray));
    }
    API_END;

    return array;
}

/*----------------------------------------------------------------------
 *  Function                                           DBFreeCompoundarray
 *
 *  Purpose
 *
 *     Release all storage associated with the given compound array.
 *
 *----------------------------------------------------------------------*/
PUBLIC void
DBFreeCompoundarray(DBcompoundarray *array)
{

    int            i;

    if (array) {
        FREE(array->name);
        if (array->elemnames) {
            for (i = 0; i < array->nelems; i++)
                FREE(array->elemnames[i]);
            FREE(array->elemnames);
        }
        FREE(array->elemlengths);
        FREE(array->values);
        FREE(array);
    }
}

/*-------------------------------------------------------------------------
 * Function:	DBAllocCurve
 *
 * Purpose:	Allocate a curve array object.
 *
 * Return:	Success:	
 *
 *		Failure:	
 *
 * Programmer:	Robb Matzke
 *		robb@callisto.nuance.com
 *		May 16, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PUBLIC DBcurve *
DBAllocCurve (void)
{

   DBcurve	*cu ;

   API_BEGIN ("DBAllocCurve", DBcurve *, NULL) {
      if (NULL==(cu=ALLOC(DBcurve)))
	 API_ERROR (NULL, E_NOMEM) ;
      memset (cu, 0, sizeof(DBcurve)) ;
   } API_END ;

   return cu ;
}

PUBLIC int
DBIsEmptyCurve(DBcurve const *crv)
{
    if (crv && crv->npts!=0) return 0;
    return 1;
}

/*-------------------------------------------------------------------------
 * Function:	DBFreeCurve
 *
 * Purpose:	Release all storage associated with the given curve object.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		robb@callisto.nuance.com
 *		May 16, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PUBLIC void
DBFreeCurve (DBcurve *cu)
{

   if (cu) {
      FREE (cu->title) ;
      FREE (cu->xvarname) ;
      FREE (cu->yvarname) ;
      FREE (cu->xlabel) ;
      FREE (cu->ylabel) ;
      FREE (cu->xunits) ;
      FREE (cu->yunits) ;
      FREE (cu->x) ;
      FREE (cu->y) ;
      FREE (cu) ;
   }
}

PUBLIC void
DBFreeGroupelmap(DBgroupelmap *map)
{
    int i;

    if (map == 0)
        return;

    FREE(map->name);
    FREE(map->groupel_types);
    FREE(map->segment_lengths);
    FREE(map->segment_ids);

    for (i = 0; i < map->num_segments; i++)
        FREE(map->segment_data[i]);
    FREE(map->segment_data);

    if (map->segment_fracs)
    {
        for (i = 0; i < map->num_segments; i++)
            FREE(map->segment_fracs[i]);
        FREE(map->segment_fracs);
    }
    FREE(map);
}

PUBLIC void
DBFreeMrgvar(DBmrgvar *mrgv)
{
    int i;

    if (mrgv == 0)
        return;

    if (mrgv->compnames)
    {
        for (i = 0; i < mrgv->ncomps; i++)
            FREE(mrgv->compnames[i]);
        FREE(mrgv->compnames);
    }

    if (mrgv->reg_pnames)
    {
        if (strchr(mrgv->reg_pnames[0], '%') == 0)
        {
            for (i = 0; i < mrgv->nregns; i++)
                FREE(mrgv->reg_pnames[i]);
        }
        else
        {
            FREE(mrgv->reg_pnames[0]);
        }
        FREE(mrgv->reg_pnames);
    }

    if (mrgv->data)
    {
        for (i = 0; i < mrgv->ncomps; i++)
            FREE(mrgv->data[i]);
        FREE(mrgv->data);
    }

    FREE(mrgv);
}

PUBLIC void
DBFreeNamescheme(DBnamescheme *ns)
{
    int i,k;

    if (ns->arralloc)
    {
        for (i = 0, k = 0; i < ns->narrefs; i++)
        {
            while (ns->fmt[k] != '\0' && ns->fmt[k] != '$' && ns->fmt[k] != '#') k++;
            if (ns->fmt[k] == '#')
            {
                FREE(ns->arrvals[i]);
            }
            else
            {
                int j;
                for (j = 0; j < ns->arrsizes[i]; j++)
                {
                    FREE(((char **)(ns->arrvals[i]))[j]);
                }
                FREE(ns->arrvals[i]);
            }
        }
    }
    FREE(ns->arrvals);
    for (i = 0; i < ns->narrefs; i++)
        FREE(ns->arrnames[i]);
    FREE(ns->arrnames);
    FREE(ns->arrsizes);
    FREE(ns->fmt);
    FREE(ns->fmtptrs);
    for (i = 0; i < DB_MAX_EXPNS; i++)
    {
        if (ns->embedns[i])
            DBFreeNamescheme(ns->embedns[i]);
    }
    for (i = 0; i < ns->ncspecs; i++)
        FREE(ns->exprstrs[i]);
    FREE(ns->exprstrs);
    FREE(ns);
}

PUBLIC DBnamescheme *
DBAllocNamescheme()
{
    DBnamescheme *ns;

    API_BEGIN("DBAllocNamescheme", DBnamescheme*, NULL) {
        if (NULL == (ns = ALLOC(DBnamescheme)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(ns, 0, sizeof(DBnamescheme));

    }
    API_END;

    return (ns);
}

PUBLIC DBgroupelmap *
DBAllocGroupelmap(int num_segs, DBdatatype frac_type)
{
    DBgroupelmap *gm;

    API_BEGIN("DBAllocGroupelmap", DBgroupelmap*, NULL) {
        if (NULL == (gm = ALLOC(DBgroupelmap)))
            API_ERROR(NULL, E_NOMEM);

        /* Initialize all memory to zero. */
        memset(gm, 0, sizeof(DBgroupelmap));

        /* initialize all the arrays */
        gm->num_segments = num_segs;
        gm->groupel_types = ALLOC_N(int, num_segs);
        gm->segment_lengths = ALLOC_N(int, num_segs);
        gm->segment_ids = ALLOC_N(int, num_segs);
        gm->segment_data = ALLOC_N(int *, num_segs);
        switch (frac_type)
        {
            case DB_CHAR:
                gm->segment_fracs = (void**) ALLOC_N(char*, num_segs);
                break;
            case DB_INT:
                gm->segment_fracs = (void**) ALLOC_N(int*, num_segs);
                break;
            case DB_SHORT:
                gm->segment_fracs = (void**) ALLOC_N(short*, num_segs);
                break;
            case DB_LONG:
                gm->segment_fracs = (void**) ALLOC_N(long*, num_segs);
                break;
            case DB_LONG_LONG:
                gm->segment_fracs = (void**) ALLOC_N(long long*, num_segs);
                break;
            case DB_FLOAT:
                gm->segment_fracs = (void**) ALLOC_N(float*, num_segs);
                break;
            case DB_DOUBLE:
                gm->segment_fracs = (void**) ALLOC_N(double*, num_segs);
                break;
            default:
                gm->segment_fracs = 0;
                break;
        }

        if (!gm->groupel_types || ! gm->segment_lengths ||
            !gm->segment_ids || !gm->segment_data ||
            (frac_type != DB_NOTYPE && !gm->segment_fracs))
        {
            DBFreeGroupelmap(gm);
            API_ERROR(NULL, E_NOMEM);
        }
    }

    API_END;

    return (gm);
}
