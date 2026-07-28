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
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Tuesday, February 9, 1999
 *
 * Purpose:     This header file is included by all silo-hdf5 source files
 *              and contains constants and prototypes that should be visible
 *              to the SILO/HDF5 driver but not to the application.
 *
 * Note:        This file can be included even if HDF5 is not available.
 */
#ifndef SILO_HDF5_PRIVATE_H
#define SILO_HDF5_PRIVATE_H
#include "config.h"
#include "silo_private.h"
#include "silo_drivers.h"
#if defined(HAVE_HDF5_H) && defined(HAVE_LIBHDF5)

#include "hdf5.h"

#define NDSETTAB        30

/* The private version of the DBfile structure is defined here */
typedef struct DBfile_hdf5 {
    DBfile_pub  pub;                    /*public stuff                  */
    hid_t       fid;                    /*hdf5 file identifier          */
    hid_t       cwg;                    /*current working group         */
    char        *cwg_name;              /*full name of cwg or NULL      */
    hid_t       link;                   /*link group                    */
    char        *dsettab[NDSETTAB];     /*circular buffer of datasets   */
    char        compname[NDSETTAB][32]; /*component names for datasets  */
    int         dsettab_ins;            /*next insert location          */
    int         dsettab_rem;            /*next remove location          */
    hid_t       T_char;                 /*target DB_CHAR type           */
    hid_t       T_short;                /*target DB_SHORT type          */
    hid_t       T_int;                  /*target DB_INT type            */
    hid_t       T_long;                 /*target DB_LONG type           */
    hid_t       T_llong;                /*target DB_LONG_LONG type      */
    hid_t       T_float;                /*target DB_FLOAT type          */
    hid_t       T_double;               /*target DB_DOUBLE type         */
    hid_t       T_str256;               /*target 256-char string        */
    hid_t       (*T_str)(char*);        /*target character string       */
} DBfile_hdf5;

#ifndef SILO_NO_CALLBACKS

/* File operations */
SILO_CALLBACK int db_hdf5_Close (DBfile *);
SILO_CALLBACK int db_hdf5_Filters(DBfile *_dbfile, FILE *stream);
SILO_CALLBACK int db_hdf5_Flush (DBfile *);

/* Directory operations */
SILO_CALLBACK int db_hdf5_MkDir(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_SetDir(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_GetDir(DBfile *_dbfile, char *name/*out*/);
SILO_CALLBACK int db_hdf5_CpDir(DBfile *_dbfile, char const *srcDir,
                           DBfile *dstFile, char const *dstDir);
#ifndef _MSC_VER
#warning REMOVE db_hdf5_CpListedObjects
#endif
SILO_CALLBACK int db_hdf5_CpListedObjects(int nobjs,
                           DBfile *_dbfile, char const * const *srcObjs,
                           DBfile *dstFile, char const * const *dstObjs);
SILO_CALLBACK int db_hdf5_MkSymlink(DBfile *_dbfile,
                          char const *target, char const *link);
SILO_CALLBACK int db_hdf5_GetSymlink(DBfile *_dbfile,
                          char const *in_candidate_link, char *out_target);
SILO_CALLBACK int db_hdf5_NewToc(DBfile *_dbfile);

/* Variable inquiries */
SILO_CALLBACK int db_hdf5_InqVarExists (DBfile *_dbfile, char const *varname);
SILO_CALLBACK int db_hdf5_GetVarLength(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_GetVarByteLength(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_GetVarByteLengthInFile(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_GetVarType(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_GetVarDims(DBfile *_dbfile, char const *varname, int maxdims,
                                int *dims/*out*/);

/* Variable I/O */
SILO_CALLBACK void *db_hdf5_GetVar(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_ReadVar (DBfile *, char const *, void *);
SILO_CALLBACK int db_hdf5_ReadVarSlice (DBfile *, char const *, int const *, int const *, int const *,
                                   int, void *);
SILO_CALLBACK int db_hdf5_ReadVarVals (DBfile *, char const *, int, int, int, void const *, void **, int*, int*);

SILO_CALLBACK int db_hdf5_Write(
    DBfile *dbfile,
    char const *array_name,
    void const *data,
    int const *dims,
    int ndims,
    int datatype
);
SILO_CALLBACK int db_hdf5_WriteSlice(
    DBfile *dbfile,
    char const *array_name,
    void const *data,
    int datatype,
    int const *offsets,
    int const *lengths,
    int const *strides,
    int const *dims,
    int ndims
);

/* Low-level object functions */
SILO_CALLBACK DBobject *db_hdf5_GetObject(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_GetComponentNames(DBfile *_dbfile, char const *objname,
                                       char ***comp_names, char ***file_names);
SILO_CALLBACK int db_hdf5_WriteObject(DBfile *_dbfile, DBobject const *obj, int flags);
SILO_CALLBACK int db_hdf5_WriteComponent(DBfile *_dbfile, DBobject *obj,
                                    char const *compname, char const *prefix,
                                    char const *datatype, void const *data, int rank,
                                    long const _size[]);
SILO_CALLBACK int db_hdf5_GetComponentType(DBfile *_dbfile, char const *objname,
                                    char const *compname);
SILO_CALLBACK void *db_hdf5_GetComponent(DBfile *_dbfile, char const *objname,
                                    char const *compname);
SILO_CALLBACK void *db_hdf5_GetComponentStuff(DBfile *_dbfile, char const *objname,
                                    char const *compname, int *just_get_datatype, int *nvals);
SILO_CALLBACK DBObjectType db_hdf5_InqVarType(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_InqMeshName(DBfile *_dbfile, char const *name,
                                 char *meshname/*out*/);

/* Curves */
SILO_CALLBACK int db_hdf5_PutCurve(
    DBfile *_dbfile,
    char const *name,
    void const *xvals,
    void const *yvals,
    int dtype,
    int npts,
    DBoptlist const *opts
);
SILO_CALLBACK DBcurve *db_hdf5_GetCurve(DBfile *_dbfile, char const *name);

/* Csgmeshes */
SILO_CALLBACK int db_hdf5_PutCsgmesh(DBfile *_dbfile, char const *name, int ndims,
                                int nbounds, int const *typeflags,
                                int const *bndids/*optional*/,
                                void const *coeffs, int lcoeffs, int datatype,
                                double const *extents, char const *zonel_name,
                                DBoptlist const *optlist);
SILO_CALLBACK DBcsgmesh *db_hdf5_GetCsgmesh(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutCsgvar(DBfile *_dbfile, char const *vname, char const *meshname,
                               int nvars, char const * const *varnames, void const * const *vars,
                               int nvals, int datatype, int centering, DBoptlist const *optlist);
SILO_CALLBACK DBcsgvar *db_hdf5_GetCsgvar(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutCSGZonelist(DBfile *_dbfile, char const *name, int nregs,
                                    int const *typeflags,
                                    int const *leftids, int const *rightids,
                                    void const *xforms, int lxforms, int datatype,
                                    int nzones, int const *zonelist, DBoptlist const *optlist);
SILO_CALLBACK DBcsgzonelist *db_hdf5_GetCSGZonelist(DBfile *_dbfile, char const *name);

/* Defvars */
SILO_CALLBACK int db_hdf5_PutDefvars(
    DBfile *dbfile, char const *name,
    int ndefs,
    char const * const *names,
    int const *types,
    char const * const *defns,
    DBoptlist const * const *opts
);
SILO_CALLBACK DBdefvars *db_hdf5_GetDefvars(DBfile *_dbfile, char const *name);

/* Quadmeshes */
SILO_CALLBACK int db_hdf5_PutQuadmesh(DBfile *_dbfile, char const *name,
                                 char const * const *coordnames, DBVCP2_t coords,
                                 int const *dims, int ndims, int datatype,
                                 int coordtype, DBoptlist const *optlist);
SILO_CALLBACK DBquadmesh *db_hdf5_GetQuadmesh (DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutQuadvar(DBfile *_dbfile, char const *name, char const *meshname,
                                int nvars, char const * const *varnames,
                                DBVCP2_t vars, int const *dims,
                                int ndims, DBVCP2_t mixvars,
                                int mixlen, int datatype, int centering,
                                DBoptlist const *optlist);
SILO_CALLBACK DBquadvar *db_hdf5_GetQuadvar(DBfile *_dbfile, char const *name);

/* Unstructured meshes */
SILO_CALLBACK int db_hdf5_PutUcdmesh(DBfile *_dbfile, char const *name, int ndims,
                                char const * const *coordnames,
                                DBVCP2_t coords, int nnodes,
                                int nzones, char const *zlname, char const *flname,
                                int datatype, DBoptlist const *optlist);
SILO_CALLBACK int db_hdf5_PutUcdsubmesh(DBfile *_dbfile, char const *name,
                                   char const *parentmesh, int nzones, char const *zlname,
                                   char const *flname, DBoptlist const *optlist);
SILO_CALLBACK DBucdmesh *db_hdf5_GetUcdmesh(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutUcdvar(DBfile *_dbfile, char const *name, char const *meshname,
                               int nvars, char const * const *varnames,
                               DBVCP2_t vars, int nels,
                               DBVCP2_t mixvars, int mixlen,
                               int datatype, int centering,
                               DBoptlist const *optlist);
SILO_CALLBACK DBucdvar *db_hdf5_GetUcdvar(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutFacelist(
    DBfile *_dbfile,
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
);
SILO_CALLBACK DBfacelist *db_hdf5_GetFacelist(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutZonelist(DBfile *_dbfile, char const *name, int nzones,
                                 int ndims, int const *nodelist, int lnodelist,
                                 int origin, int const *shapesize, int const *shapecnt,
                                 int nshapes);
SILO_CALLBACK int db_hdf5_PutZonelist2(DBfile *_dbfile, char const *name, int nzones,
                                  int ndims, int const *nodelist, int lnodelist,
                                  int origin, int lo_offset, int hi_offset,
                                  int const *shapetype, int const *shapesize,
                                  int const *shapecnt, int nshapes,
                                  DBoptlist const *optlist);
SILO_CALLBACK int db_hdf5_PutPHZonelist(DBfile *_dbfile, char const *name,
                                  int nfaces, int const *nodecnt, int lnodelist, int const *nodelist,
                                  char const *extface,
                                  int nzones, int const *facecnt, int lfacelist, int const *facelist,
                                  int origin, int lo_offset, int hi_offset,
                                  DBoptlist const *optlist);
SILO_CALLBACK DBzonelist *db_hdf5_GetZonelist(DBfile *_dbfile, char const *name);
SILO_CALLBACK DBphzonelist *db_hdf5_GetPHZonelist(DBfile *_dbfile, char const *name);

/* Materials */
SILO_CALLBACK int db_hdf5_PutMaterial(
    DBfile *_dbfile,
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
);
SILO_CALLBACK DBmaterial *db_hdf5_GetMaterial(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutMatspecies(DBfile *_dbfile, char const *name, char const *matname,
                                   int nmat, int const *nmatspec, int const *speclist,
                                   int const *dims, int ndims, int nspecies_mf,
                                   void const *species_mf, int const *mix_speclist,
                                   int mixlen, int datatype,
                                   DBoptlist const *optlist);
SILO_CALLBACK DBmatspecies *db_hdf5_GetMatspecies(DBfile *_dbfile, char const *name);

/* Point meshes */
SILO_CALLBACK int db_hdf5_PutPointmesh(DBfile *_dbfile, char const *name, int ndims,
                                  DBVCP2_t coords, int nels, int datatype,
                                  DBoptlist const *optlist);
SILO_CALLBACK DBpointmesh *db_hdf5_GetPointmesh(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutPointvar(DBfile *_dbfile, char const *name, char const *meshname,
                                 int nvars, DBVCP2_t vars, int nels,
                                 int datatype, DBoptlist const *optlist);
SILO_CALLBACK DBmeshvar *db_hdf5_GetPointvar(DBfile *_dbfile, char const *name);

/* Multiblock meshes */
SILO_CALLBACK int db_hdf5_PutMultimesh(DBfile *_dbfile, char const *name, int nmesh,
                                  char const *const *meshnames, int const *meshtypes,
                                  DBoptlist const *optlist);
SILO_CALLBACK int db_hdf5_PutMultimeshadj(
    DBfile *_dbfile,
    char const *name,
    int nmesh,
    int const *meshtypes,
    int const *nneighbors,
    int const *neighbors,
    int const *back,
    int const *lnodelists,
    int const * const *nodelists,
    int const *lzonelists,
    int const * const *zonelists,
    DBoptlist const *optlist
);
SILO_CALLBACK DBmultimesh *db_hdf5_GetMultimesh(DBfile *_dbfile, char const *name);
SILO_CALLBACK DBmultimeshadj *db_hdf5_GetMultimeshadj(DBfile *_dbfile, char const *name, int nmesh,
                                                 int const *block_map);
SILO_CALLBACK int db_hdf5_PutMultivar(DBfile *_dbfile, char const *name, int nvars,
                                 char const * const *varnames, int const *vartypes,
                                 DBoptlist const *optlist);
SILO_CALLBACK DBmultivar *db_hdf5_GetMultivar(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutMultimat(DBfile *_dbfile, char const *name, int nmats,
                                 char const * const *matnames, DBoptlist const *optlist);
SILO_CALLBACK DBmultimat *db_hdf5_GetMultimat(DBfile *_dbfile, char const *name);
SILO_CALLBACK int db_hdf5_PutMultimatspecies(DBfile *_dbfile, char const *name, int nspec,
                                        char const * const *specnames, DBoptlist const *optlist);
SILO_CALLBACK DBmultimatspecies *db_hdf5_GetMultimatspecies(DBfile *_dbfile,
                                                       char const *name);

/* Compound arrays */
SILO_CALLBACK int db_hdf5_PutCompoundarray(
    DBfile *_dbfile,
    char const *name,
    char const * const *elmtnames,
    int const *elmtlen,
    int nelmts,
    void const *values,
    int nvalues,
    int datatype,
    DBoptlist const *optlist
);
SILO_CALLBACK DBcompoundarray *db_hdf5_GetCompoundarray(
    DBfile *_dbfile,
    char const *name
);

/* Mrgtree objects */
SILO_CALLBACK int db_hdf5_PutMrgtree(DBfile *_dbfile, char const *name, char const *mesh_name,
    DBmrgtree const *tree, DBoptlist const *optlist);
SILO_CALLBACK DBmrgtree *db_hdf5_GetMrgtree(DBfile *_dbfile, char const *name);

/* groupel maps */
SILO_CALLBACK int db_hdf5_PutGroupelmap(DBfile *_dbfile, char const *map_name,
    int num_segments, int const *groupel_types, int const *segment_lengths,
    int const *segment_ids, int const * const *segment_data, void const * const *segment_fracs,
    int fracs_data_type, DBoptlist const *opts);
SILO_CALLBACK DBgroupelmap *db_hdf5_GetGroupelmap(DBfile *dbfile, char const *name);

/* mrgvars */
SILO_CALLBACK int db_hdf5_PutMrgvar(DBfile *dbfile, char const *name,
                             char const *mrgt_name,
                             int ncomps, char const * const *compnames,
                             int nregns, char const * const *reg_pnames,
                             int datatype, void const * const *data, DBoptlist const *opts);
SILO_CALLBACK DBmrgvar *db_hdf5_GetMrgvar(DBfile *dbfile, char const *name);

SILO_CALLBACK int db_hdf5_FreeCompressionResources(DBfile *_dbfile,
                 char const *meshname);

SILO_CALLBACK int db_hdf5_SortObjectsByOffset(DBfile *_dbfile, int nobjs,
                 char const *const *const names, int *ordering);

#endif /* !SILO_NO_CALLBACKS */

#endif /* defined(HAVE_HDF5_H) && defined(HAVE_LIBHDF5) */
#endif /* !SILO_HDF5_PRIVATE_H */
