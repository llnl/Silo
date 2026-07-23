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

#ifdef __sgi    /* IRIX C++ bug */
#include <math.h>
#else
#include <cmath>
#endif
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>

#include <assert.h>
#include <sys/types.h>

#include <silo.h>
#include <std.c>

#include <rocket.h>

using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::map;
using std::set;
using std::string;
using std::stringstream;
using std::vector;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int SiloZoneType(const string& ztstr)
{
    // 3D zone types
    if (ztstr == "Hex") return DB_ZONETYPE_HEX;
    if (ztstr == "Tet") return DB_ZONETYPE_TET;
    if (ztstr == "Wedge") return DB_ZONETYPE_PRISM;
    if (ztstr == "Prism") return DB_ZONETYPE_PRISM;
    if (ztstr == "Pyramid") return DB_ZONETYPE_PYRAMID;
    if (ztstr == "Polyhedron") return DB_ZONETYPE_POLYHEDRON;

    // 2D zone types
    if (ztstr == "Quad") return DB_ZONETYPE_QUAD;
    if (ztstr == "Tri") return DB_ZONETYPE_TRIANGLE;
    if (ztstr == "Polygon") return DB_ZONETYPE_POLYGON;

    // 1D zone types
    if (ztstr == "Line") return DB_ZONETYPE_BEAM;

    // 0D zone types
    if (ztstr == "Point") return 0; // no silo zone-type for this

    return -1; 
}

static int SiloZoneShapeSize(int ztype)
{
    // 3D zone types
    if (ztype == DB_ZONETYPE_HEX) return 8;
    if (ztype == DB_ZONETYPE_TET) return 4;
    if (ztype == DB_ZONETYPE_PRISM) return 6;
    if (ztype == DB_ZONETYPE_PYRAMID) return 5;
    if (ztype == DB_ZONETYPE_POLYHEDRON) return -1;

    // 2D zone types
    if (ztype == DB_ZONETYPE_QUAD) return 4;
    if (ztype == DB_ZONETYPE_TRIANGLE) return 3;
    if (ztype == DB_ZONETYPE_POLYGON) return -1;

    // 1D zone types
    if (ztype == DB_ZONETYPE_BEAM) return 2;

    return 0; 
}

static void SiloZonelistStuff(const string& restrict_to_class,
    vector<int>& shptype, vector<int>& shpsz, vector<int>& shpcnt)
{
    shptype.clear();
    shpsz.clear();
    shpcnt.clear();

    int i, zid, zt;
    int shapecnt = 0;
    int lastzt;

    int nzones;
    vector<int> zlist;
    if (restrict_to_class == "")
    {
        nzones = zoneClasses.GetNumEntities();
        lastzt = SiloZoneType(zoneClasses.ZoneType(0));
    }
    else
    {
        vector<string> r2c;
        r2c.push_back(restrict_to_class);
        zoneClasses.GetEntitiesInClasses(r2c, zlist);
        nzones = zlist.size();
        lastzt = SiloZoneType(zoneClasses.ZoneType(zlist[0]));
    }


    for (i = 0; i < nzones; i++)
    {
        zid = (restrict_to_class == "") ? i : zlist[i];
        zt = SiloZoneType(zoneClasses.ZoneType(zid));
        assert(zt != DB_ZONETYPE_POLYHEDRON);
        assert(zt != DB_ZONETYPE_POLYGON);
        if (zt != lastzt)
        {
            shptype.push_back(lastzt);
            shpsz.push_back(SiloZoneShapeSize(lastzt));
            shpcnt.push_back(shapecnt);
            shapecnt = 0;
            lastzt = zt;
        }
        shapecnt++;
    }
    zid = (restrict_to_class == "") ? i-1 : zlist[i-1];
    zt = SiloZoneType(zoneClasses.ZoneType(zid));
    assert(zt != DB_ZONETYPE_POLYHEDRON);
    assert(zt != DB_ZONETYPE_POLYGON);
    shptype.push_back(zt);
    shpsz.push_back(SiloZoneShapeSize(zt));
    shpcnt.push_back(shapecnt);
}

static int silo_driver = -1;
static int ProcessArgsForSilo(const int argc, const char *const *const argv)
{
    int show_all_errors = FALSE;
    int j, i = 1;

    silo_driver = DB_PDB;
    while (i < argc)
    {
        if (strncmp(argv[i], "DB_HDF5", 7) == 0)
        {
            silo_driver = StringToDriver(argv[i]);
        }
        else if (strncmp(argv[i], "DB_PDB", 6) == 0)
        {
            silo_driver = StringToDriver(argv[i]);
        }
        else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
        }
        i++;
    }

    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ABORT, NULL);
    return 0;
}

static void AddRegions(DBfile *dbfile, DBmrgtree *mrgt,
    const char *className, const char *const *const regnNames,
    int nreg, bool addMaps = false, int remapForProci = -1)
{
    DBAddRegion(mrgt, className, 0, nreg, 0, 0, 0, 0, 0, 0);
    DBSetCwr(mrgt, className);

    int seg_types[1], seg_lens[1], seg_ids[1];
    int *seg_data[1];
    seg_types[0] = DB_NODECENT;
    seg_ids[0] = 0;

    for (int i = 0; i < nreg; i++)
    {
        if (!addMaps) // no maps, so just define the region
        {
            DBAddRegion(mrgt, regnNames[i], 0, 0, 0, 0, 0, 0, 0, 0);
        }
        else // Do it as one map per region. Could do one map for all regions too.
        {
            vector<string> cnames;
            cnames.push_back(regnNames[i]);
            vector<int> map_data;
            nodeClasses.GetEntitiesInClasses(cnames, map_data);

            if (remapForProci >= 0)
            {
                stringstream procClassName;
                procClassName << "Domain" << remapForProci;
                vector<int> upn, dnn, upz, dnz;
                GetUpDownMapsForZoneClass(procClassName.str(), upn, dnn, upz, dnz);
                vector<int> map_data2;
                for (int j = 0; j < map_data.size(); j++)
                {
                    int gnid = map_data[j];
                    int lnid = dnn[gnid];
                    if (lnid != -1)
                        map_data2.push_back(lnid);
                }
                map_data = map_data2;
            }

            if (map_data.size() == 0)
            {
                DBAddRegion(mrgt, regnNames[i], 0, 0, 0, 0, 0, 0, 0, 0);
            }
            else
            {
                seg_lens[0] = map_data.size();
                seg_data[0] = &map_data[0];
                string mapnm = string(regnNames[i]) + "_map";
                DBAddRegion(mrgt, regnNames[i], 0, 0, mapnm.c_str(), 1, seg_ids, seg_lens, seg_types, 0);
                DBPutGroupelmap(dbfile, mapnm.c_str(), 1, seg_types, seg_lens, seg_ids, (int**)seg_data, 0, DB_NOTYPE, 0);
            }
        }
    }

    DBSetCwr(mrgt, "..");
}

static void WriteSiloSingleMesh(DBfile *dbfile, DBoptlist *ol,
    const vector<string>& dom_classes)
{
    float *coords[3];
    coords[0] = &xvals_g[0];
    coords[1] = &yvals_g[0];
    coords[2] = &zvals_g[0];
    char const *coordnames[3] = {"X", "Y", "Z"};
    int nnodes = nodeClasses.GetNumEntities();
    int nzones = zoneClasses.GetNumEntities();
    int nmats = 5;

    DBAddOption(ol, DBOPT_MRGTREE_NAME, (void*) "mrgtree");
    DBPutUcdmesh(dbfile, "mesh", 3, coordnames, coords, nnodes, nzones, "zl", 0, DB_FLOAT, ol);
    DBClearOptlist(ol);

    vector<int> shapetyp, shapesize, shapecnt;
    SiloZonelistStuff("", shapetyp, shapesize, shapecnt);
    DBPutZonelist2(dbfile, "zl", nzones, 3, &nodelist_g[0], (int) nodelist_g.size(), 0, 0, 0,
        &shapetyp[0], &shapesize[0], &shapecnt[0], (int) shapetyp.size(), 0);

    // Create a zone-centered variable showing domain-decomp for debug purposes
    int ndoms = dom_classes.size();
    vector<int> dom_map;
    zoneClasses.GetEntitiesPartitionedByClasses(dom_classes, dom_map);
    DBPutUcdvar1(dbfile, "proc_map", "mesh", (float*) &dom_map[0], nzones, 0, 0, DB_INT, DB_ZONECENT, 0);

    vector<int> matlist;
    vector<string> mat_classes(matNames, matNames + nmats);
    zoneClasses.GetEntitiesPartitionedByClasses(mat_classes, matlist);
    for (int i = 0; i < (int) matlist.size(); i++)
        matlist[i] = matlist[i] + 1;
    DBClearOptlist(ol);
    DBAddOption(ol, DBOPT_NMATNOS, &nmats);
    DBAddOption(ol, DBOPT_MATNOS, matnos);
    DBAddOption(ol, DBOPT_MATNAMES, matNames);
    DBAddOption(ol, DBOPT_MATCOLORS, matColors);
    DBPutMaterial(dbfile, "materials", "mesh", 5, matnos,
        &matlist[0], &nzones, 1, 0, 0, 0, 0, 0, DB_FLOAT, ol);

    // Create MRG Tree 
    DBmrgtree *mrgt = DBMakeMrgtree(DB_UCDMESH, 0x0, 4, 0);

    // Add nodeset regions to MRG Tree
    const char *ns_names[] = {"HighQ", "HoldDownPoints", "TowerContact"};
    const int nns = sizeof(ns_names)/sizeof(ns_names[0]);
    const bool addMaps = true;
    AddRegions(dbfile, mrgt, "nodesets", ns_names, nns, addMaps);

    // Add faceset regions to MRG Tree
    const char *fs_names[] = {"PeakPressure", "ControlSurfaces"};
    const int nfs = sizeof(fs_names)/sizeof(fs_names[0]);
    AddRegions(dbfile, mrgt, "facesets", fs_names, nfs, addMaps);

    // Up until now, only map data has been written to file. Now,
    // write the MRG Tree too.
    DBPutMrgtree(dbfile, "mrgtree", "mesh", mrgt, 0);
    DBFreeMrgtree(mrgt);
}

static void WriteSiloMultiMesh(DBfile *dbfile, DBoptlist *ol,
    const vector<string>& dom_classes)
{
    float *coords[3];
    char const *coordnames[3] = {"X", "Y", "Z"};

    int nmats = 5;
    int ndoms = dom_classes.size();

    for (int i = 0; i < ndoms; i++)
    {
        DBMkDir(dbfile, dom_classes[i].c_str());
        DBSetDir(dbfile, dom_classes[i].c_str());

        vector<int> upz, upn, dnz, dnn;
        GetUpDownMapsForZoneClass(dom_classes[i], upn, dnn, upz, dnz);

        vector<float> x, y, z;
        MapArray(upn, xvals_g, x, (float) -1);
        MapArray(upn, yvals_g, y, (float) -1);
        MapArray(upn, zvals_g, z, (float) -1);
        int nnodes2 = upn.size();
        int nzones2 = upz.size();

        // Map the Zonelist object's nodelist
        vector<int> nl;
        vector<int> matlist;
        for (int j = 0; j < upz.size(); j++)
        {
            int gzid = upz[j];
            for (int k = 0; k < 5; k++)
            {
                if (zoneClasses.IsEntityMemberOf(gzid, matNames[k]))
                {
                    matlist.push_back(k+1);
                    break;
                }
            }
            int k0 = nodestarts_g[gzid];
            int cnt = nodecnts_g[gzid];
            for (int k = 0; k < cnt; k++)
            {
                int gnid = nodelist_g[k0+k];
                int lnid = dnn[gnid];
                assert(lnid>=0 && lnid<nnodes2);
                nl.push_back(lnid);
            }
        }

        coords[0] = &x[0];
        coords[1] = &y[0];
        coords[2] = &z[0];
        DBAddOption(ol, DBOPT_MRGTREE_NAME, (void*) "mrgtree");
        DBPutUcdmesh(dbfile, "mesh", 3, coordnames, coords, nnodes2, nzones2, "zl", 0, DB_FLOAT, ol);
        DBClearOptlist(ol);

        vector<int> shapetyp, shapesize, shapecnt;
        SiloZonelistStuff(dom_classes[i], shapetyp, shapesize, shapecnt);
        DBPutZonelist2(dbfile, "zl", nzones2, 3, &nl[0], (int) nl.size(), 0, 0, 0,
            &shapetyp[0], &shapesize[0], &shapecnt[0], (int) shapetyp.size(), 0);

        DBClearOptlist(ol);
        DBAddOption(ol, DBOPT_NMATNOS, &nmats);
        DBAddOption(ol, DBOPT_MATNOS, matnos);
        DBAddOption(ol, DBOPT_MATNAMES, matNames);
        DBAddOption(ol, DBOPT_MATCOLORS, matColors);
        DBPutMaterial(dbfile, "materials", "mesh", 5, matnos,
            &matlist[0], &nzones2, 1, 0, 0, 0, 0, 0, DB_FLOAT, ol);

        // Create MRG Tree 
        DBmrgtree *mrgt = DBMakeMrgtree(DB_UCDMESH, 0x0, 4, 0);

        // Add nodeset regions to MRG Tree
        const char *ns_names[] = {"HighQ", "HoldDownPoints", "TowerContact"};
        const int nns = sizeof(ns_names)/sizeof(ns_names[0]);
        const bool addMaps = true;
        AddRegions(dbfile, mrgt, "nodesets", ns_names, nns, addMaps, i);

        // Add faceset regions to MRG Tree
        const char *fs_names[] = {"PeakPressure", "ControlSurfaces"};
        const int nfs = sizeof(fs_names)/sizeof(fs_names[0]);
        AddRegions(dbfile, mrgt, "facesets", fs_names, nfs, addMaps, i);

        // Up until now, only map data has been written to file. Now,
        // write the MRG Tree too.
        DBPutMrgtree(dbfile, "mrgtree", "mesh", mrgt, 0);
        DBFreeMrgtree(mrgt);

        DBSetDir(dbfile, "..");
    }

    int mtypes[ndoms];
    char *mnames[ndoms];
    char *matnames[ndoms];
    for (int i = 0; i < ndoms; i++)
    {
        mtypes[i] = DB_UCDMESH;
        string mname = "/UseCaseB/"+dom_classes[i]+"/mesh";
        mnames[i] = strdup(mname.c_str());
        matnames[i] = strdup(string("/UseCaseB/"+dom_classes[i]+"/materials").c_str());
    }

    DBAddOption(ol, DBOPT_MRGTREE_NAME, (void*) "mrgtree");
    DBPutMultimesh(dbfile, "mmesh", ndoms, mnames, mtypes, ol);
    DBClearOptlist(ol);
    DBAddOption(ol, DBOPT_NMATNOS, &nmats);
    DBAddOption(ol, DBOPT_MATNOS, matnos);
    DBAddOption(ol, DBOPT_MATNAMES, matNames);
    DBAddOption(ol, DBOPT_MATCOLORS, matColors);
    DBPutMultimat(dbfile, "mmat", 5, matnames, ol);
    DBClearOptlist(ol);

    // Create MRG Tree 
    DBmrgtree *mrgt = DBMakeMrgtree(DB_UCDMESH, 0x0, 4, 0);

    // Add nodeset regions to MRG Tree
    const char *ns_names[] = {"HighQ", "HoldDownPoints", "TowerContact"};
    const int nns = sizeof(ns_names)/sizeof(ns_names[0]);
    const bool addMaps = false;
    AddRegions(dbfile, mrgt, "nodesets", ns_names, nns, addMaps);

    // Add faceset regions to MRG Tree
    const char *fs_names[] = {"PeakPressure", "ControlSurfaces"};
    const int nfs = sizeof(fs_names)/sizeof(fs_names[0]);
    AddRegions(dbfile, mrgt, "facesets", fs_names, nfs, addMaps);

    // Up until now, only map data has been written to file. Now,
    // write the MRG Tree too.
    //DBPrintMrgtree(mrgt->root, DB_PREORDER, stdout);
    DBPutMrgtree(dbfile, "mrgtree", "mmesh", mrgt, 0);
    DBFreeMrgtree(mrgt);

    for (int i = 0; i < ndoms; i++)
    {
        free(mnames[i]);
        free(matnames[i]);
    }
}

static int WriteFormatReal(int argc, const char *const *const argv)
{
    ProcessArgsForSilo(argc, argv);

    if (silo_driver == -1)
    {
        cerr << "Silo driver not set" << endl;
        return 1;
    }

    char const *dclasses[] = {"Domain0", "Domain1", "Domain2", "Domain3", "Domain4"};
    int ndoms = sizeof(dclasses) / sizeof(dclasses[0]);
    vector<string> dom_classes(dclasses, dclasses + ndoms);
    int nmats = 5;

    DBfile *dbfile = DBCreate("rocket.silo", DB_CLOBBER, DB_LOCAL,
                      "3D mesh with many interesting subsets", silo_driver);
    assert(dbfile);

    DBoptlist *ol = DBMakeOptlist(10);

    DBMkDir(dbfile, "UseCaseA");
    DBSetDir(dbfile, "UseCaseA");

    WriteSiloSingleMesh(dbfile, ol, dom_classes);

    DBSetDir(dbfile, "..");
    DBMkDir(dbfile, "UseCaseB");
    DBSetDir(dbfile, "UseCaseB");

    WriteSiloMultiMesh(dbfile, ol, dom_classes);

    DBFreeOptlist(ol);

    DBClose(dbfile);

    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

#ifdef STATIC_PLUGINS
int WriteFormat_silo(int argc, const char *const *const argv)
{
    return WriteFormatReal(argc, argv);
}
#else
int WriteFormat(int argc, const char *const *const argv)
{
    return WriteFormatReal(argc, argv);
}
#endif

#ifdef __cplusplus
}
#endif
