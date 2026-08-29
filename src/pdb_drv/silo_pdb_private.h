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
Contract  No.   DE-AC52-07NA27344 with  the  DOE.  Neither the  United
States Government  nor Lawrence  Livermore National Security,  LLC nor
any of  their employees,  makes any warranty,  express or  implied, or
assumes   any   liability   or   responsibility  for   the   accuracy,
completeness, or usefulness of any information, apparatus, product, or
process  disclosed, or  represents  that its  use  would not  infringe
privately-owned   rights.  Any  reference   herein  to   any  specific
commercial products,  process, or  services by trade  name, trademark,
manufacturer or otherwise does not necessarily constitute or imply its
endorsement,  recommendation,   or  favoring  by   the  United  States
Government or Lawrence Livermore National Security, LLC. The views and
opinions  of authors  expressed  herein do  not  necessarily state  or
reflect those  of the United  States Government or  Lawrence Livermore
National  Security, LLC,  and shall  not  be used  for advertising  or
product endorsement purposes.
*/
#ifndef SILO_PDB_PRIVATE_H
#define SILO_PDB_PRIVATE_H

#include "pdb.h"
/* 'VERSION' is defined in PDB proper and Silo's config.h */
#ifdef USING_PDB_PROPER
#    ifdef VERSION
#    undef VERSION
#    endif
#endif
#include "silo_private.h"

/*
 * The private version of the DBfile structure is defined here.
 */
typedef struct DBfile_pdb {
    DBfile_pub     pub;
    PDBfile       *pdb;
} DBfile_pdb;

typedef struct {
    char          *name;
    char          *type;        /* Type of group/object */
    char         **comp_names;  /* Array of component names */
    char         **pdb_names;   /* Array of internal (PDB) variable names */
    int            ncomponents; /* Number of components */
} PJgroup;

/*
 * Generally, nothing uses SCORE memory management any more.  However,
 * sometimes we need it when talking to SCORE or PDB at the low levels.
 * Those few source files that need SCORE memory management should
 * define NEED_SCORE_MM before including this file.
 */
#ifdef NEED_SCORE_MM
#define SCALLOC(T)	((T*)lite_SC_alloc(1L,(long)sizeof(T),NULL))
#define SCALLOC_N(T,N)  ((T*)lite_SC_alloc((long)(N),(long)sizeof(T),NULL))
#define SCFREE(M)       if(M){lite_SC_free((void*)M);(M)=NULL;}
#define SC_strdup(S)    (strcpy(SCALLOC_N(char,strlen((S))+1),(S)))
#endif

typedef struct {
    int            id;
} PJdir;

typedef struct {
    char const    *name[80];    /* Component name */
    void          *ptr[80];     /* Address of component value */
    int            type[80];    /* Datatype of component */
    unsigned char  alloced[80]; /* Sentinel: 1 == space already alloc'd */
    int            nelmts[80];  /* Size of allocation in # of elements */
    int            num;         /* Number of components */
} PJcomplist;

#ifndef SILO_NO_CALLBACKS
SILO_CALLBACK int db_pdb_close (DBfile *);
SILO_CALLBACK int db_pdb_flush (DBfile *);
SILO_CALLBACK int db_pdb_InqVarExists (DBfile *, char const *);
SILO_CALLBACK void *db_pdb_GetComponent (DBfile *, char const *, char const *);
SILO_CALLBACK int db_pdb_GetComponentType (DBfile *, char const *, char const *);
SILO_CALLBACK int db_pdb_GetDir (DBfile *, char *);

SILO_CALLBACK DBobject *db_pdb_GetObject (DBfile*, char const *);
SILO_CALLBACK DBcompoundarray *db_pdb_GetCompoundarray (DBfile *, char const *);
SILO_CALLBACK DBcurve *db_pdb_GetCurve (DBfile *, char const *);
SILO_CALLBACK DBdefvars *db_pdb_GetDefvars (DBfile *, char const *);
SILO_CALLBACK DBmaterial *db_pdb_GetMaterial (DBfile *, char const *);
SILO_CALLBACK DBmatspecies *db_pdb_GetMatspecies (DBfile *, char const *);
SILO_CALLBACK DBmultimesh *db_pdb_GetMultimesh (DBfile *, char const *);
SILO_CALLBACK DBmultimeshadj *db_pdb_GetMultimeshadj (DBfile *, char const *,
                            int, int const *);
SILO_CALLBACK DBmultivar *db_pdb_GetMultivar (DBfile *, char const *);
SILO_CALLBACK DBmultimat *db_pdb_GetMultimat (DBfile *, char const *);
SILO_CALLBACK DBmultimatspecies *db_pdb_GetMultimatspecies (DBfile *, char const *);
SILO_CALLBACK DBpointmesh *db_pdb_GetPointmesh (DBfile *, char const *);
SILO_CALLBACK DBmeshvar *db_pdb_GetPointvar (DBfile *, char const *);
SILO_CALLBACK DBquadmesh *db_pdb_GetQuadmesh (DBfile *, char const *);
SILO_CALLBACK DBquadvar *db_pdb_GetQuadvar (DBfile *, char const *);
SILO_CALLBACK DBucdmesh *db_pdb_GetUcdmesh (DBfile *, char const *);
SILO_CALLBACK DBucdvar *db_pdb_GetUcdvar (DBfile *, char const *);
SILO_CALLBACK DBcsgmesh *db_pdb_GetCsgmesh (DBfile *, char const *);
SILO_CALLBACK DBcsgvar *db_pdb_GetCsgvar (DBfile *, char const *);
SILO_CALLBACK DBfacelist *db_pdb_GetFacelist(DBfile*, char const *);
SILO_CALLBACK DBzonelist *db_pdb_GetZonelist(DBfile*, char const *);
SILO_CALLBACK DBphzonelist *db_pdb_GetPHZonelist(DBfile*, char const *);
SILO_CALLBACK DBcsgzonelist *db_pdb_GetCSGZonelist(DBfile*, char const *);
SILO_CALLBACK DBmrgtree *db_pdb_GetMrgtree(DBfile *_dbfile, char const *name);
SILO_CALLBACK DBmrgvar *db_pdb_GetMrgvar(DBfile *dbfile, char const *name);
SILO_CALLBACK DBgroupelmap *db_pdb_GetGroupelmap(DBfile *dbfile, char const *name);
SILO_CALLBACK void *db_pdb_GetVar (DBfile *, char const *);
SILO_CALLBACK int db_pdb_GetVarByteLength (DBfile *, char const *);
SILO_CALLBACK int db_pdb_GetVarLength (DBfile *, char const *);
SILO_CALLBACK int db_pdb_GetVarDims (DBfile*, char const *, int, int*);
SILO_CALLBACK int db_pdb_GetVarType (DBfile *, char const *);
SILO_CALLBACK DBObjectType db_pdb_InqVarType (DBfile *, char const *);
SILO_CALLBACK int db_pdb_InqMeshname (DBfile *, char const *, char *);
SILO_CALLBACK int db_pdb_InqMeshtype (DBfile *, char const *);
SILO_CALLBACK int db_pdb_ReadVar (DBfile *, char const *, void *);
SILO_CALLBACK int db_pdb_ReadVarSlice (DBfile *, char const *, int const *, int const *, int const *, int, void *);
SILO_CALLBACK int db_pdb_ReadVarVals(DBfile *_dbfile, char const *vname, int mode, int nvals, int ndims,
                      void const *indices, void **result, int *ncomps, int *nitems);
SILO_CALLBACK int db_pdb_SetDir (DBfile *, char const *);
SILO_CALLBACK int db_pdb_Filters (DBfile *, FILE *);
SILO_CALLBACK int db_pdb_NewToc (DBfile *);
SILO_CALLBACK int db_pdb_GetComponentNames (DBfile *, char const *, char ***, char ***);

SILO_CALLBACK int db_pdb_FreeCompressionResources(DBfile *_dbfile, char const *meshname);

PRIVATE int db_pdb_getobjinfo (PDBfile *, char const *, char *, int *);
PRIVATE int db_pdb_getvarinfo (PDBfile *, char const *, char *, int *, int *, int);

#ifdef PDB_WRITE
SILO_CALLBACK int db_pdb_WriteObject (DBfile *, DBobject const *, int);
SILO_CALLBACK int db_pdb_WriteComponent (DBfile *, DBobject *, char const *,
				    char const *, char const *, void const *, int, long const *);
SILO_CALLBACK int db_pdb_MkDir (DBfile *, char const *);
SILO_CALLBACK int db_pdb_PutCompoundarray(
    DBfile * dbfile,
    char const *name,
    char const * const *elemnames,
    int const *elemlens,
    int nelems,
    void const *values,
    int nvalues,
    int datatype,
    DBoptlist const *
);
SILO_CALLBACK int db_pdb_PutCurve(
    DBfile *dbfile,
    char const *name,
    void const *xvals,
    void const *yvals,
    int npts,
    int datatype,
    DBoptlist const *
);
SILO_CALLBACK int db_pdb_PutDefvars(
    DBfile *dbfile,
    char const *name,
    int,
    char const * const *names,
    int const *types,
    char const * const *defns,
    DBoptlist const * const *opts
);
SILO_CALLBACK int db_pdb_PutFacelist(
    DBfile *dbfile,
    char const * name,
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
SILO_CALLBACK int db_pdb_PutMaterial(
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
    void const *mix_vf,
    int mixlen,
    int datatype,
    DBoptlist const *opts
);
SILO_CALLBACK int db_pdb_PutMatspecies (struct DBfile *, char const *, char const *, int,
				   int const *, int const *, int const *, int, int, void const *,
				   int const *, int, int, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutMultimesh (DBfile *, char const *, int, char const * const *,
                                  int const *, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutMultimeshadj (DBfile *, char const *, int, int const *,
                               int const *, int const *, int const *, int const *,
                               int const * const *, int const *, int const * const *,
                               DBoptlist const *optlist);
SILO_CALLBACK int db_pdb_PutMultivar (DBfile *, char const *, int, char const * const *, int const *,
				 DBoptlist const *);
SILO_CALLBACK int db_pdb_PutMultimat (DBfile *, char const *, int, char const * const *,
				 DBoptlist const *);
SILO_CALLBACK int db_pdb_PutMultimatspecies (DBfile *, char const *, int, char const * const *,
					DBoptlist const *);
SILO_CALLBACK int db_pdb_PutPointmesh (DBfile *, char const *, int, DBVCP2_t, int,
				  int, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutPointvar (DBfile *, char const *, char const *, int, DBVCP2_t,
				 int, int, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutQuadmesh (DBfile *, char const *, char const * const *, DBVCP2_t,
				 int const *, int, int, int, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutQuadvar (DBfile *, char const *, char const *, int, char const * const *,
				DBVCP2_t, int const *, int, DBVCP2_t, int, int,
				int, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutUcdmesh (DBfile *, char const *, int, char const * const *, DBVCP2_t,
				int, int, char const *, char const *, int, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutUcdsubmesh (DBfile *, char const *, char const *,
				int, char const *, char const *, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutUcdvar (DBfile *, char const *, char const *, int, char const * const *,
			       DBVCP2_t, int, DBVCP2_t, int, int, int, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutCsgmesh (DBfile *, char const *, int, int,
                                int const *, int const *, void const *,int, int,
                                double const *, char const *, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutCsgvar (DBfile *, char const *, char const *, int,
                               char const * const *varnames, void const * const *vars,
                               int, int, int, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutZonelist (DBfile *, char const *, int, int, int const *, int,
				 int, int const *, int const *, int);
SILO_CALLBACK int db_pdb_PutZonelist2 (DBfile *, char const *, int, int, int const *, int,
				  int, int, int, int const *, int const *, int const *, int,
                                  DBoptlist const *);
SILO_CALLBACK int db_pdb_PutPHZonelist(DBfile *, char const *,
                                  int, int const *, int, int const *, char const *,
                                  int, int const *, int, int const *,
                                  int, int, int, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutCSGZonelist (DBfile *, char const *, int,
                                    int const *, int const *, int const *,
                                    void const *, int, int,
                                    int, int const *, DBoptlist const *);
SILO_CALLBACK int db_pdb_PutMrgtree(DBfile *_dbfile, char const *name, char const *mesh_name,
                                    DBmrgtree const *tree, DBoptlist const *optlist);
SILO_CALLBACK int db_pdb_PutGroupelmap(DBfile *_dbfile, char const *map_name,
                                  int num_segments, int const *groupel_types,
                                  int const *segment_lengths, int const *segment_ids,
                                  int const * const *segment_data, void const * const *segment_fracs,
                                  int fracs_data_type, DBoptlist const *opts);
SILO_CALLBACK int db_pdb_PutMrgvar(DBfile *dbfile, char const *name,
                             char const *mrgt_name,
                             int ncomps, char const * const *compnames,
                             int nregns, char  const * const *reg_pnames,
                             int datatype, void const * const *data, DBoptlist const *opts);

SILO_CALLBACK int db_pdb_Write (DBfile *, char const *, void const *, int const *, int, int);
SILO_CALLBACK int db_pdb_WriteSlice(
    DBfile*,
    char const *array_name,
    void const *data,
    int datatype,
    int const *offsets,
    int const *lengths,
    int const *strides,
    int const *dims,
    int ndims
);

SILO_CALLBACK int db_pdb_SortObjectsByOffset(DBfile *_dbfile, int nobjs,
    char const *const *const names, int *ordering);

PRIVATE int db_InitCsg (DBfile *, char const *, DBoptlist const *);
PRIVATE int db_InitPoint (DBfile *, DBoptlist const *, int, int);
PRIVATE int db_InitQuad (DBfile *, char const *, DBoptlist const *, int const *, int);
PRIVATE void db_InitCurve (DBoptlist const * const);
PRIVATE void db_build_shared_names_csgmesh (DBfile *, char const *);
PRIVATE void db_build_shared_names_quadmesh (DBfile *, char const *);
PRIVATE int db_InitUcd (DBfile *, char const *, DBoptlist const *, int, int, int);
PRIVATE int db_InitZonelist (DBfile *, DBoptlist const *);
PRIVATE int db_ResetGlobalData_phzonelist (void);
PRIVATE int db_InitPHZonelist (DBfile *, DBoptlist const *);
PRIVATE void db_build_shared_names_ucdmesh (DBfile *, char const *);
PRIVATE void db_mkname (PDBfile*, char const *, char const *, char*);
PRIVATE int db_InitMulti (DBfile*, DBoptlist const *const);
PRIVATE void db_InitDefvars (DBoptlist const *);
#endif /* PDB_WRITE */
#endif /* !SILO_NO_CALLBACKS */

/*-------------------------------------------------------------------------
 * Macros...
 *-------------------------------------------------------------------------
 */
#define MAXNAME         256
#define SZA(A)          ((int)(sizeof(A)/sizeof((A)[0])))
#define INIT_OBJ(A)     (_tcl=(A),_tcl->num=0)
#define DEFINE_OBJ(NM,PP,TYP) DEF_OBJ(NM,PP,TYP,1,1)
#define DEFALL_OBJ(NM,PP,TYP) DEF_OBJ(NM,PP,TYP,0,0)
#define DEFINE_OBN(NM,PP,TYP,N) DEF_OBJ(NM,PP,TYP,1,N)
#define DEF_OBJ(NM,PP,TYP,AL,N) {                                       \
     (_tcl->name[_tcl->num]=(NM),                                       \
      _tcl->ptr[_tcl->num]=(void*)(PP),                                 \
      _tcl->type[_tcl->num]=(TYP),                                      \
      _tcl->alloced[_tcl->num]=(AL),                                    \
      _tcl->nelmts[_tcl->num]=(N));                                     \
      _tcl->num++;}

/*-------------------------------------------------------------------------
 * Private functions.
 *-------------------------------------------------------------------------
 */
PRIVATE char **PJ_ls (PDBfile *, char *, char *, int *);
PRIVATE int PJ_get_fullpath (char *, char const *, char *);

PRIVATE int PJ_read (PDBfile *, char const *, void *);
PRIVATE int PJ_read_alt (PDBfile *, char *, void *, long *);
PRIVATE int PJ_read_as (PDBfile *, char *, char *, void *);
PRIVATE int PJ_read_as_alt (PDBfile *, char *, char *, void *, long *);
PRIVATE syment *PJ_inquire_entry (PDBfile *, char const *);
PRIVATE int pdb_getvarinfo (PDBfile *, char *, char *, int *, int *, int);

PRIVATE int PJ_ForceSingle (int);
PRIVATE int PJ_GetObject (PDBfile *, char const *, PJcomplist *, int expected_dbtype);
PRIVATE int PJ_ClearCache(void);
PRIVATE int PJ_InqForceSingle (void);
PRIVATE void PJ_NoCache ( void );
PRIVATE void *PJ_GetComponent (PDBfile *, char const *, char const *);
PRIVATE int PJ_GetComponentType (PDBfile *, char const *, char const *);
PRIVATE int PJ_ReadVariable (PDBfile *, char *, int, int, int, char **);

PRIVATE int PJ_get_group (PDBfile *, char const *, PJgroup **);
PRIVATE PJgroup *PJ_make_group (char *, char *, char **, char **, int);
PRIVATE int PJ_rel_group (PJgroup *);
PRIVATE int PJ_print_group (PJgroup *, FILE *);

#ifdef PDB_WRITE
PRIVATE int PJ_put_group (PDBfile*,PJgroup*, int);
PRIVATE int PJ_write (PDBfile*,char*,char*,void*) ;
PRIVATE int PJ_write_len (PDBfile*,char const *,char const *,void const *,int,long const *);
PRIVATE int PJ_write_alt (PDBfile*,char const *,char const *,void const *,int,long const *);
#endif /* PDB_WRITE */

#endif /* !SILO_PDB_PRIVATE_H */
