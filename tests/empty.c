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
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <silo.h>

#include <std.c>

static DBfile *dbfile = NULL;

#define ASSERT(FUNCCALL,RETPRED0,RETPRED1)		\
{   int retval, i=0; 					\
    char nm[256];					\
    strncpy(nm, #FUNCCALL, sizeof(nm)); 		\
    while (nm[i] != '(') i++;				\
    if (nm[i-1]=='1'||nm[i-1]=='2') nm[i-1] = '\0';	\
    nm[i] = '\0';					\
							\
    if (pass == 0)                                      \
    {                                                   \
        DBMkDir(dbfile, nm);				\
        DBSetDir(dbfile, nm);				\
        retval = FUNCCALL;                              \
        if(!(RETPRED0)){fprintf(stderr,"On empty object " \
            "rejection pass, assertion \"%s\" at line %d" \
            " failed\n",#RETPRED0,__LINE__);abort();}   \
        DBSetDir(dbfile, "..");				\
    }                                                   \
    else                                                \
    {                                                   \
        DBSetDir(dbfile, nm);				\
        retval = FUNCCALL;                              \
        if(!(RETPRED1)){fprintf(stderr,"On empty object "\
            "allow pass, assertion \"%s\" at line %d "	\
            "failed\n",#RETPRED1,__LINE__);abort();}    \
        DBSetDir(dbfile, "..");				\
    }                                                   \
}

static DBoptlist *OL(DBoptlist *ol)
{
    double *dtime = (double*) DBGetOption(ol, DBOPT_DTIME);
    int *hfg = (int*) DBGetOption(ol, DBOPT_HIDE_FROM_GUI);
    *dtime+=5;
    *hfg+=1;
    return ol;
}

int
main(int argc, char *argv[])
{
    int		    driver = DB_PDB;
    char 	    *filename = "empty.silo";
    int             show_all_errors = FALSE;
    int             i, pass;
    char const * const cnames[3] = {"x","y","z"};
    char const * const mnames[3] = {"dom000/mesh","dom001/mesh","dom002/mesh"};
    void           *coords[3] = {(void*)1,(void*)2,(void*)3}; /* really funky dummy pointers */
    float           zero = 0.0;
    void           *coordsK[3] = {(void*)&zero,(void*)&zero,(void*)&zero}; /* funky pointers for Kerbel case */
    void           *vars[3] = {(void*)1,(void*)2,(void*)3}; /* really funky dummy pointers */
    void const * const vvars[3] = {(void*)1,(void*)2,(void*)3}; /* really funky dummy pointers */
    void           *var = (void*)1;
    int             iarr[3] = {1,1,1}; /* dummy int array */
    int             iarr2[3] = {0,0,0}; /* dummy int array */
    int             ZDIMS[3] = {0,0,0};
    double          exts[6] = {0,0,0,0,0,0};
    DBoptlist      *ol = 0;
    double          dtime = 0.0;
    int             hide_from_gui=0;
    int            *gnodeno = 0;
    int            *gzoneno = 0;
    char           *ghostn = 0;
    char           *ghostz = 0;

    /* Parse command-line */
    for (i=1; i<argc; i++) {
	if (!strncmp(argv[i], "DB_", 3)) {
	    driver = StringToDriver(argv[i]);
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
	} else if (argv[i][0] != '\0') {
	    fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
	}
    }
    
    DBSetDeprecateWarnings(0);
    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_NONE, NULL);
    printf("Creating test file \"%s\".\n", filename);
    dbfile = DBCreate(filename, DB_CLOBBER, DB_LOCAL, "test empty silo objects", driver);

    ol = DBMakeOptlist(10);
    DBAddOption(ol, DBOPT_DTIME, &dtime);
    DBAddOption(ol, DBOPT_HIDE_FROM_GUI, &hide_from_gui);
    DBAddOption(ol, DBOPT_NODENUM, gnodeno);
    DBAddOption(ol, DBOPT_ZONENUM, gzoneno);
    DBAddOption(ol, DBOPT_GHOST_NODE_LABELS, ghostn);
    DBAddOption(ol, DBOPT_GHOST_ZONE_LABELS, ghostz);

    /* first pass confirms we catch bad arguments; second pass confirms we permit empty objects */
    for (pass = 0; pass < 2; pass++)
    {
        const int dt = DB_FLOAT;
        const int ct = DB_ZONECENT;
        const int ZZ = 0; /* Used for sole arg causing emptiness */
        if (pass) DBSetAllowEmptyObjects(1);

        /* Because references to the following objects will not ever appear in a
           multi-xxx object, we do not currently test for support of empties...
               DBPutUcdsubmesh, DBPutMrgtree, DBPutMrgvar, DBPutGroupelmap

           Note: 'ZZ' or 'ZDIMS' is the key argument in each call that triggers an empty
        */

        /* empty compound array objects */
        ASSERT(DBPutCompoundarray(dbfile, "empty_compoundarraya", cnames, iarr2, 3, exts, ZZ, DB_DOUBLE, 0),retval<0,retval==0);
        ASSERT(DBPutCompoundarray(dbfile, "empty_compoundarrayb", cnames, iarr2, 0, exts, ZZ, DB_DOUBLE, 0),retval<0,retval==0);

        /* empty curve objects */
        ASSERT(DBPutCurve(dbfile,"empty_curvea",coords[0],coords[0],dt,ZZ,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCurve(dbfile,"empty_curveb",        0,coords[0],dt,ZZ,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCurve(dbfile,"empty_curvec",coords[0],        0,dt,ZZ,OL(ol)),retval<0,retval==0);

        /* empty point meshes and vars */
        ASSERT(DBPutPointmesh(dbfile,"empty_pointmesha",1,coords,ZZ,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPointmesh(dbfile,"empty_pointmeshb",3,     0,ZZ,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPointvar(dbfile,"pva","empty_pointmesha",1,vars,ZZ,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPointvar(dbfile,"pvb","empty_pointmesha",3,   0,ZZ,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPointvar1(dbfile,"pv1a","empty_pointmesha",var,ZZ,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPointvar1(dbfile,"pv1b","empty_pointmesha",  0,ZZ,dt,OL(ol)),retval<0,retval==0);

        /* empty quad meshes and vars (ZDIMS is the magic zero'ing arg) */
        ASSERT(DBPutQuadmesh(dbfile,"empty_quadmesha",     0,coords,ZDIMS,1,dt,DB_COLLINEAR,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutQuadmesh(dbfile,"empty_quadmeshb",cnames,     0,ZDIMS,2,dt,DB_COLLINEAR,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutQuadmesh(dbfile,"empty_quadmeshc",cnames,coords,ZDIMS,3,dt,DB_COLLINEAR,OL(ol)),retval<0,retval==0);

        ASSERT(DBPutQuadvar(dbfile,"qva","empty_quadmesha",2,     0,vars,ZDIMS,2,0,0,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutQuadvar(dbfile,"qvb","empty_quadmesha",3,cnames,   0,ZDIMS,3,0,0,dt,ct,OL(ol)),retval<0,retval==0);

        ASSERT(DBPutQuadvar1(dbfile,"qv1a","empty_quadmesha",  0,ZDIMS,ZZ,var,0,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutQuadvar1(dbfile,"qv1b","empty_quadmesha",var,ZDIMS,ZZ,  0,0,dt,ct,OL(ol)),retval<0,retval==0);

        /* empty ucd meshes, facelists, zonelists and vars */
        ASSERT(DBPutUcdmesh(dbfile,"empty_ucdmesh1",3,cnames,coords,ZZ,1,"foo","bar",dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdmesh(dbfile,"empty_ucdmesh2",1,     0,coords,ZZ,1,"foo","bar",dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdmesh(dbfile,"empty_ucdmesh3",2,cnames,     0,ZZ,0,"foo","bar",dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdmesh(dbfile,"empty_ucdmesh4",0,     0,     0,ZZ,0,    0,    0,dt,OL(ol)),retval<0,retval==0);
        /* Test funky Kerbel case */
        ASSERT(DBPutUcdmesh(dbfile,"empty_ucdmesh5",3,cnames,coordsK,1,ZZ,0,0,dt,OL(ol)),retval>=0,retval==0);

        ASSERT(DBPutFacelist(dbfile,"empty_facelista",ZZ,0,iarr,1,1,iarr,iarr,iarr,1,iarr,iarr,1),retval<0,retval==0);
        ASSERT(DBPutFacelist(dbfile,"empty_facelistb",ZZ,0,   0,1,1,iarr,iarr,iarr,1,iarr,iarr,1),retval<0,retval==0);
        ASSERT(DBPutFacelist(dbfile,"empty_facelistc",ZZ,0,iarr,1,1,   0,iarr,iarr,1,iarr,iarr,1),retval<0,retval==0);
        ASSERT(DBPutFacelist(dbfile,"empty_facelistd",ZZ,0,iarr,1,1,iarr,   0,iarr,1,iarr,iarr,1),retval<0,retval==0);
        ASSERT(DBPutFacelist(dbfile,"empty_faceliste",ZZ,0,iarr,1,1,iarr,iarr,   0,1,iarr,iarr,1),retval<0,retval==0);
        ASSERT(DBPutFacelist(dbfile,"empty_facelistf",ZZ,0,iarr,1,1,iarr,iarr,iarr,1,   0,iarr,1),retval<0,retval==0);
        ASSERT(DBPutFacelist(dbfile,"empty_facelistg",ZZ,0,iarr,1,1,iarr,iarr,iarr,1,iarr,   0,1),retval<0,retval==0);

        ASSERT(DBPutZonelist(dbfile,"empty_zonelista",ZZ,1,iarr,10,0,iarr,iarr,10),retval<0,retval==0);
        ASSERT(DBPutZonelist(dbfile,"empty_zonelistb",ZZ,1,   0,10,0,iarr,iarr,10),retval<0,retval==0);
        ASSERT(DBPutZonelist(dbfile,"empty_zonelistc",ZZ,1,iarr,10,0,   0,iarr,10),retval<0,retval==0);
        ASSERT(DBPutZonelist(dbfile,"empty_zonelistd",ZZ,1,iarr,10,0,iarr,    0,10),retval<0,retval==0);

        ASSERT(DBPutZonelist2(dbfile,"empty_zonelist2a",ZZ,1,iarr,1,0,0,0,iarr,iarr,iarr,1,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutZonelist2(dbfile,"empty_zonelist2b",ZZ,0,   0,1,3,3,3,iarr,iarr,iarr,1,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutZonelist2(dbfile,"empty_zonelist2c",ZZ,1,iarr,0,3,3,3,   0,iarr,iarr,1,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutZonelist2(dbfile,"empty_zonelist2d",ZZ,1,iarr,1,0,3,0,iarr,   0,iarr,0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutZonelist2(dbfile,"empty_zonelist2e",ZZ,1,iarr,1,3,0,0,iarr,iarr,   0,1,OL(ol)),retval<0,retval==0);

        ASSERT(DBPutPHZonelist(dbfile,"empty_phzonelista",ZZ,iarr,1,iarr,cnames[0],1,iarr,1,iarr,0,0,0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPHZonelist(dbfile,"empty_phzonelistb",ZZ,   0,1,iarr,cnames[0],1,iarr,1,iarr,0,0,0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPHZonelist(dbfile,"empty_phzonelistc",ZZ,iarr,1,   0,cnames[0],1,iarr,1,iarr,0,0,0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPHZonelist(dbfile,"empty_phzonelistd",ZZ,iarr,1,iarr,        0,1,iarr,1,iarr,0,0,0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPHZonelist(dbfile,"empty_phzoneliste",ZZ,iarr,1,iarr,cnames[0],1,   0,1,iarr,0,0,0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutPHZonelist(dbfile,"empty_phzonelistf",ZZ,iarr,1,iarr,cnames[0],1,iarr,1,   0,0,0,0,OL(ol)),retval<0,retval==0);

        ASSERT(DBPutUcdvar(dbfile,"uva","empty_ucdmesh1",0,cnames,vars,ZZ,vars,1,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdvar(dbfile,"uvb","empty_ucdmesh1",1,     0,vars,ZZ,vars,1,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdvar(dbfile,"uvc","empty_ucdmesh1",2,cnames,   0,ZZ,vars,1,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdvar(dbfile,"uvd","empty_ucdmesh1",3,cnames,vars,ZZ,   0,1,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdvar(dbfile,"uve","empty_ucdmesh1",3,     0,   0,ZZ,   0,1,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdvar1(dbfile,"uv1a","empty_ucdmesh1",var,ZZ,vars[0],1,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdvar1(dbfile,"uv1b","empty_ucdmesh1",  0,ZZ,vars[0],1,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdvar1(dbfile,"uv1c","empty_ucdmesh1",var,ZZ,      0,1,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutUcdvar1(dbfile,"uv1d","empty_ucdmesh1",  0,ZZ,      0,1,dt,ct,OL(ol)),retval<0,retval==0);

        /* csg meshes and vars */
        ASSERT(DBPutCsgmesh(dbfile,"empty_csgmesh1",2,ZZ,   0,iarr,var,1,dt,exts,"foo",OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCsgmesh(dbfile,"empty_csgmesh2",2,ZZ,iarr,   0,var,2,dt,exts,"foo",OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCsgmesh(dbfile,"empty_csgmesh3",3,ZZ,iarr,iarr,  0,3,dt,exts,"foo",OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCsgmesh(dbfile,"empty_csgmesh4",3,ZZ,iarr,iarr,var,0,dt,exts,"foo",OL(ol)),retval<0,retval==0);

        ASSERT(DBPutCSGZonelist(dbfile,"empty_csgzonelista",0,iarr,iarr,iarr,0,0,dt,ZZ,iarr,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCSGZonelist(dbfile,"empty_csgzonelistb",1,   0,iarr,iarr,0,0,dt,ZZ,iarr,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCSGZonelist(dbfile,"empty_csgzonelistc",1,iarr,   0,iarr,0,0,dt,ZZ,iarr,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCSGZonelist(dbfile,"empty_csgzonelistd",1,iarr,iarr,   0,0,0,dt,ZZ,iarr,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCSGZonelist(dbfile,"empty_csgzoneliste",1,iarr,iarr,iarr,0,0,dt,ZZ,   0,OL(ol)),retval<0,retval==0);

        ASSERT(DBPutCsgvar(dbfile,"csgva","empty_csgmesh1",0,cnames,vvars,ZZ,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCsgvar(dbfile,"csgvb","empty_csgmesh1",1,     0,vvars,ZZ,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCsgvar(dbfile,"csgvc","empty_csgmesh1",1,cnames,   0, ZZ,dt,ct,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutCsgvar(dbfile,"csgvd","empty_csgmesh1",1,cnames,vvars,ZZ,dt,ct,OL(ol)),retval<0,retval==0);

        /* empty materials and species */
        ASSERT(DBPutMaterial(dbfile,"empty_mata","foo",1,iarr,iarr,ZDIMS,1,iarr,iarr,iarr,vars[0],1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMaterial(dbfile,"empty_matb","foo",1,   0,iarr,ZDIMS,1,iarr,iarr,iarr,vars[0],1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMaterial(dbfile,"empty_matc","foo",1,iarr,   0,ZDIMS,1,iarr,iarr,iarr,vars[0],1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMaterial(dbfile,"empty_matd","foo",1,iarr,iarr,ZDIMS,1,   0,iarr,iarr,vars[0],1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMaterial(dbfile,"empty_mate","foo",1,iarr,iarr,ZDIMS,1,iarr,   0,iarr,vars[0],1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMaterial(dbfile,"empty_matf","foo",1,iarr,iarr,ZDIMS,1,iarr,iarr,   0,vars[0],1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMaterial(dbfile,"empty_matg","foo",1,iarr,iarr,ZDIMS,1,iarr,iarr,iarr,   0,1,dt,OL(ol)),retval<0,retval==0);

        /* empty matspecies via dims[i] == 0 */
        ASSERT(DBPutMatspecies(dbfile,"empty_speca","empty_mata",1,iarr,iarr,ZDIMS,1,1,var,iarr,1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMatspecies(dbfile,"empty_specb","empty_mata",1,iarr,iarr,ZDIMS,2,1,var,iarr,1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMatspecies(dbfile,"empty_specc","empty_mata",1,iarr,iarr,ZDIMS,3,1,var,iarr,1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMatspecies(dbfile,"empty_specd","empty_mata",1,iarr,iarr,ZDIMS,1,1,  0,iarr,1,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMatspecies(dbfile,"empty_spece","empty_mata",1,iarr,iarr,ZDIMS,1,1,var,   0,0,dt,OL(ol)),retval<0,retval==0);

        /* empty matspeces via nspecies_mf==0 */
        { int nd=2, d[2]={3,2}, slm[6]={0,0,-1,-3,0,0}, ml=4, msl[4]={1,2,1,2}, slc[6]={0,0,0,0,0,0};
        ASSERT(DBPutMatspecies(dbfile,"empty_specf","empty_mata",1,iarr,slc,d,nd,ZZ,var,  0, 0,dt,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMatspecies(dbfile,"empty_specg","empty_mata",1,iarr,slm,d,nd,ZZ,var,msl,ml,dt,OL(ol)),retval<0,retval==0);
        }

        /* empty multi-block objects */
        ASSERT(DBPutMultimesh(dbfile,"empty_mmesha",ZZ,mnames,iarr,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultimesh(dbfile,"empty_mmeshb",ZZ,     0,iarr,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultimesh(dbfile,"empty_mmeshc",ZZ,mnames,     0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultimesh(dbfile,"empty_mmeshd",ZZ,     0,     0,OL(ol)),retval<0,retval==0);

        ASSERT(DBPutMultivar(dbfile,"empty_mvara",ZZ,mnames,iarr,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultivar(dbfile,"empty_mvarb",ZZ,     0,iarr,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultivar(dbfile,"empty_mvarc",ZZ,mnames,     0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultivar(dbfile,"empty_mvard",ZZ,     0,     0,OL(ol)),retval<0,retval==0);

        ASSERT(DBPutMultimat(dbfile,"empty_mmata",ZZ,mnames,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultimat(dbfile,"empty_mmatb",ZZ,     0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultimat(dbfile,"empty_mmatc",ZZ,mnames,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultimat(dbfile,"empty_mmatd",ZZ,     0,OL(ol)),retval<0,retval==0);

        ASSERT(DBPutMultimatspecies(dbfile,"empty_mmatspeca",ZZ,mnames,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultimatspecies(dbfile,"empty_mmatspecb",ZZ,     0,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultimatspecies(dbfile,"empty_mmatspecc",ZZ,mnames,OL(ol)),retval<0,retval==0);
        ASSERT(DBPutMultimatspecies(dbfile,"empty_mmatspecd",ZZ,     0,OL(ol)),retval<0,retval==0);

        ASSERT(DBWrite(dbfile,"empty_writea",0,ZDIMS, 3,DB_FLOAT),retval<0,retval==0);
        /*ASSERT(DBWrite(dbfile,"empty_writeb",0,    0,ZZ,DB_FLOAT),retval<0,retval==0);*/
    }

    DBFreeOptlist(ol);
    DBClose(dbfile);
    dbfile = 0;

    /* Ok, now try to read each empty object to make sure we get what we expect and nothing fails */
    dbfile = DBOpen(filename, DB_UNKNOWN, DB_READ);

    /* test read back of empty compound array objects */
    {   int i=0; char *canames[] = {"empty_compoundarraya", "empty_compoundarrayb", 0};
        DBSetDir(dbfile, "DBPutCompoundarray");
        while (canames[i])
        {
            DBcompoundarray *arr = DBGetCompoundarray(dbfile, canames[i++]);
            assert(DBIsEmptyCompoundarray(arr));
            DBFreeCompoundarray(arr);
        }
        DBSetDir(dbfile, "..");
    }

    /* test read back of empty curves */
    {   int i=0; char *cnames[] = {"empty_curvea", "empty_curveb", "empty_curvec", 0};
        DBSetDir(dbfile, "DBPutCurve");
        while (cnames[i])
        {
            DBcurve *curve = DBGetCurve(dbfile, cnames[i++]);
            assert(DBIsEmptyCurve(curve));
            DBFreeCurve(curve);
        }
        DBSetDir(dbfile, "..");
    }

    /* test read back of empty point meshes and vars */
    {   int i=0; char *pmnames[] = {"empty_pointmesha", "empty_pointmeshb", 0};
        DBSetDir(dbfile, "DBPutPointmesh");
        while (pmnames[i])
        {
            DBpointmesh *pointmesh = DBGetPointmesh(dbfile, pmnames[i++]);
            assert(DBIsEmptyPointmesh(pointmesh));
            DBFreePointmesh(pointmesh);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *vnames[] = {"pva", "pvb", "pv1a", "pv1b", 0};
        DBSetDir(dbfile, "DBPutPointvar");
        while (vnames[i])
        {
            DBpointvar *pointvar = DBGetPointvar(dbfile, vnames[i++]);
            assert(DBIsEmptyPointvar(pointvar));
            DBFreePointvar(pointvar);
        }
        DBSetDir(dbfile, "..");
    }

    /* test read back of empty quad meshes and vars */
    {   int i=0; char *qmnames[] = {"empty_quadmesha", "empty_quadmeshb", "empty_quadmeshc", 0};
        DBSetDir(dbfile, "DBPutQuadmesh");
        while (qmnames[i])
        {
            DBquadmesh *quadmesh = DBGetQuadmesh(dbfile, qmnames[i++]);
            assert(DBIsEmptyQuadmesh(quadmesh));
            DBFreeQuadmesh(quadmesh);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *vnames[] = {"qva" , "qvb", "qv1a", "qv1b",  0};
        DBSetDir(dbfile, "DBPutQuadvar");
        while (vnames[i])
        {
            DBquadvar *quadvar = DBGetQuadvar(dbfile, vnames[i++]);
            assert(DBIsEmptyQuadvar(quadvar));
            DBFreeQuadvar(quadvar);
        }
        DBSetDir(dbfile, "..");
    }

    /* test read back of empty ucd meshes, zonelists and vars */
    {   int i=0; char *mnames[] = {"empty_ucdmesh1", "empty_ucdmesh2", "empty_ucdmesh3",
                                   "empty_ucdmesh4", "empty_ucdmesh5", 0};
        DBSetDir(dbfile, "DBPutUcdmesh");
        while (mnames[i])
        {
            DBucdmesh *ucdmesh = DBGetUcdmesh(dbfile, mnames[i++]);
            assert(DBIsEmptyUcdmesh(ucdmesh));
            DBFreeUcdmesh(ucdmesh);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *flnames[] = {"empty_facelista", "empty_facelistb",
                                    "empty_facelistc", "empty_facelistd",
                                    "empty_faceliste", "empty_facelistf",
                                    "empty_facelistg", 0};
        DBSetDir(dbfile, "DBPutFacelist");
        while (flnames[i])
        {
            DBfacelist *fl = DBGetFacelist(dbfile, flnames[i++]);
            assert(DBIsEmptyFacelist(fl));
            DBFreeFacelist(fl);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *zlnames[] = {"empty_zonelista", "empty_zonelistb",
                                    "empty_zonelistc", "empty_zonelistd",
                                    "empty_zonelist2a", "empty_zonelist2b",
                                    "empty_zonelist2c", "empty_zonelist2d",
                                    "empty_zonelist2e", 0};
        DBSetDir(dbfile, "DBPutZonelist");
        while (zlnames[i])
        {
            DBzonelist *zl = DBGetZonelist(dbfile, zlnames[i++]);
            assert(DBIsEmptyZonelist(zl));
            DBFreeZonelist(zl);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *zlnames[] = {"empty_phzonelista", "empty_phzonelistb",
                                    "empty_phzonelistc", "empty_phzonelistd",
                                    "empty_phzoneliste", "empty_phzonelistf", 0};
        DBSetDir(dbfile, "DBPutPHZonelist");
        while (zlnames[i])
        {
            DBphzonelist *zl = DBGetPHZonelist(dbfile, zlnames[i++]);
            assert(DBIsEmptyPHZonelist(zl));
            DBFreePHZonelist(zl);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *vnames[] = { "uva",  "uvb",  "uvc",
                                    "uvd",  "uve",  "uv1a",
                                    "uv1b", "uv1c", "uv1d", 0};
        DBSetDir(dbfile, "DBPutUcdvar");
        while (vnames[i])
        {
            DBucdvar *ucdvar = DBGetUcdvar(dbfile, vnames[i++]);
            assert(DBIsEmptyUcdvar(ucdvar));
            DBFreeUcdvar(ucdvar);
        }
        DBSetDir(dbfile, "..");
    }

    /* test read back of empty csg meshes and vars */
    {   int i=0; char *mnames[] = {"empty_csgmesh1", "empty_csgmesh2", "empty_csgmesh3", 0};
        DBSetDir(dbfile, "DBPutCsgmesh");
        while (mnames[i])
        {
            DBcsgmesh *csgmesh = DBGetCsgmesh(dbfile, mnames[i++]);
            assert(DBIsEmptyCsgmesh(csgmesh));
            DBFreeCsgmesh(csgmesh);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *zlnames[] = {"empty_csgzonelista", "empty_csgzonelistb",
                                    "empty_csgzonelistc", "empty_csgzonelistd", 
                                    "empty_csgzoneliste", 0};
        DBSetDir(dbfile, "DBPutCSGZonelist");
        while (zlnames[i])
        {
            DBcsgzonelist *zl = DBGetCSGZonelist(dbfile, zlnames[i++]);
            assert(DBIsEmptyCSGZonelist(zl));
            DBFreeCSGZonelist(zl);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *vnames[] = {"csgva", "csgvb", "csgvc", "csgvd", 0};
        DBSetDir(dbfile, "DBPutCsgvar");
        while (vnames[i])
        {
            DBcsgvar *csgvar = DBGetCsgvar(dbfile, vnames[i++]);
            assert(DBIsEmptyCsgvar(csgvar));
            DBFreeCsgvar(csgvar);
        }
        DBSetDir(dbfile, "..");
    }

    /* test read back of empty materials and matspecies */
    {   int i=0; char *vnames[] = {"empty_mata", "empty_matb", "empty_matc", "empty_matd",
                                   "empty_mate", "empty_matf", "empty_matg", 0};
        DBSetDir(dbfile, "DBPutMaterial");
        while (vnames[i])
        {
            DBmaterial *mat = DBGetMaterial(dbfile, vnames[i++]);
            assert(DBIsEmptyMaterial(mat));
            DBFreeMaterial(mat);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *vnames[] = {"empty_speca", "empty_specb", "empty_specc",
                                   "empty_specd", "empty_spece", "empty_specf", 
                                   "empty_specg", 0};
        DBSetDir(dbfile, "DBPutMatspecies");
        while (vnames[i])
        {
            DBmatspecies *spec = DBGetMatspecies(dbfile, vnames[i++]);
            assert(DBIsEmptyMatspecies(spec));
            DBFreeMatspecies(spec);
        }
        DBSetDir(dbfile, "..");
    }

    /* test read back of empty multi-block objects */
    {   int i=0; char *vnames[] = {"empty_mmesha", "empty_mmeshb", "empty_mmeshc", "empty_mmeshd", 0};
        DBSetDir(dbfile, "DBPutMultimesh");
        while (vnames[i])
        {
            DBmultimesh *mmesh = DBGetMultimesh(dbfile, vnames[i++]);
            assert(DBIsEmptyMultimesh(mmesh));
            DBFreeMultimesh(mmesh);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *vnames[] = {"empty_mvara", "empty_mvarb", "empty_mvarc", "empty_mvard", 0};
        DBSetDir(dbfile, "DBPutMultivar");
        while (vnames[i])
        {
            DBmultivar *mvar = DBGetMultivar(dbfile, vnames[i++]);
            assert(DBIsEmptyMultivar(mvar));
            DBFreeMultivar(mvar);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *vnames[] = {"empty_mmata", "empty_mmatb", "empty_mmatc", "empty_mmatd", 0};
        DBSetDir(dbfile, "DBPutMultimat");
        while (vnames[i])
        {
            DBmultimat *mmat = DBGetMultimat(dbfile, vnames[i++]);
            assert(DBIsEmptyMultimat(mmat));
            DBFreeMultimat(mmat);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *vnames[] = {"empty_mmatspeca", "empty_mmatspecb", "empty_mmatspecc", "empty_mmatspecd", 0};
        DBSetDir(dbfile, "DBPutMultimatspecies");
        while (vnames[i])
        {
            DBmultimatspecies *mmatspecies = DBGetMultimatspecies(dbfile, vnames[i++]);
            assert(DBIsEmptyMultimatspecies(mmatspecies));
            DBFreeMultimatspecies(mmatspecies);
        }
        DBSetDir(dbfile, "..");
    }
    {   int i=0; char *vnames[] = {"empty_writea", 0};
        DBSetDir(dbfile, "DBWrite");
        while (vnames[i])
        {
            void *data = DBGetVar(dbfile, vnames[i++]);
            assert(data==0);
        }
        DBSetDir(dbfile, "..");
    }

    DBClose(dbfile);

    CleanupDriverStuff();

    return 0;
}
