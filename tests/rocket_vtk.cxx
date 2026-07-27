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

#include <vtkCellData.h>
#include <vtkCellTypes.h>
#include <vtkDataSetWriter.h>
#include <vtkIntArray.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

#include "rocket.h"

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

static int myVTKCellType(const string& ztstr)
{
    // 3D zone types
    if (ztstr == "Hex") return VTK_HEXAHEDRON;
    if (ztstr == "Tet") return VTK_TETRA;
    if (ztstr == "Wedge") return VTK_WEDGE;
    if (ztstr == "Prism") return VTK_WEDGE;
    if (ztstr == "Pyramid") return VTK_PYRAMID;
    if (ztstr == "Polyhedron") return VTK_POLYHEDRON;

    // 2D zone types
    if (ztstr == "Quad") return VTK_QUAD;
    if (ztstr == "Tri") return VTK_TRIANGLE;
    if (ztstr == "Polygon") return VTK_POLYGON;

    // 1D zone types
    if (ztstr == "Line") return VTK_LINE;

    // 0D zone types
    if (ztstr == "Point") return VTK_VERTEX;

    return -1; 
}

static int VTKCellShapeSize(int ztype)
{
    // 3D zone types
    if (ztype == VTK_HEXAHEDRON) return 8;
    if (ztype == VTK_TETRA) return 4;
    if (ztype == VTK_WEDGE) return 6;
    if (ztype == VTK_PYRAMID) return 5;
    if (ztype == VTK_POLYHEDRON) return -1;

    // 2D zone types
    if (ztype == VTK_QUAD) return 4;
    if (ztype == VTK_TRIANGLE) return 3;
    if (ztype == VTK_POLYGON) return -1;

    // 1D zone types
    if (ztype == VTK_LINE) return 2;

    // 0D zone types
    if (ztype == VTK_VERTEX) return 1;

    return 0; 
}

static int vtk_binary = 0;
static int vtk_api = 1;
static int ProcessArgsForVTK(const int argc, const char *const *const argv)
{
    int show_all_errors = 0;
    int j, i = 1;

    while (i < argc)
    {
        if (strncmp(argv[i], "vtk-binary", 6) == 0)
        {
            vtk_binary = 1;
        }
        else if (strncmp(argv[i], "vtk-fmt", 6) == 0)
        {
            vtk_api = 0;
        }
        i++;
    }

    return 0;
}

static void WriteVTKApiSingleMesh(char const *vtkFileName, const vector<string>& dom_classes)
{
    float *coords[3];
    coords[0] = &xvals_g[0];
    coords[1] = &yvals_g[0];
    coords[2] = &zvals_g[0];
    char const *coordnames[3];
    coordnames[0] = "X";
    coordnames[1] = "Y";
    coordnames[2] = "Z";
    int nnodes = nodeClasses.GetNumEntities();
    int nzones = zoneClasses.GetNumEntities();
    int nmats = 5;

    vtkPoints *points = vtkPoints::New();
    for (int i = 0; i < nnodes; i++)
        points->InsertNextPoint(coords[0][i], coords[1][i], coords[2][i]);

    vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::New();
    ugrid->SetPoints(points);
    points->Delete();

    for (int k = 0, i = 0; i < nzones; i++)
    {
        int zt = myVTKCellType(zoneClasses.ZoneType(i));
        int sz = VTKCellShapeSize(zt);
        vtkIdType nl[10];
        for (int j = 0; j < sz; j++)
            nl[j] = (vtkIdType) nodelist_g[k+j];
        ugrid->InsertNextCell(zt, sz, nl);
        k += sz;
    }

    // Create a zone-centered variable showing domain-decomp for debug purposes
    int ndoms = dom_classes.size();
    vector<int> dom_map;
    zoneClasses.GetEntitiesPartitionedByClasses(dom_classes, dom_map);
    vtkIntArray *proc_map = vtkIntArray::New();
    proc_map->SetName("proc_map");
    proc_map->SetNumberOfComponents(1);
    proc_map->SetVoidArray(&dom_map[0], dom_map.size(), 1);
    ugrid->GetCellData()->AddArray(proc_map);

    vector<int> matlist;
    vector<string> mat_classes(matNames, matNames + nmats);
    zoneClasses.GetEntitiesPartitionedByClasses(mat_classes, matlist);
    for (int i = 0; i < (int) matlist.size(); i++)
        matlist[i] = matlist[i] + 1;
    vtkIntArray *mats = vtkIntArray::New();
    proc_map->SetName("materials");
    proc_map->SetNumberOfComponents(1);
    proc_map->SetVoidArray(&matlist[0], matlist.size(), 1);
    ugrid->GetCellData()->AddArray(mats);

    vtkDataSetWriter *wrtr = vtkDataSetWriter::New();
    if (vtk_binary)
        wrtr->SetFileTypeToBinary();
    wrtr->SetInputData(ugrid);
    wrtr->SetFileName(vtkFileName);
    wrtr->Write();
    wrtr->Delete();
    ugrid->Delete();
}

static void WriteVTKApiMultiMesh(char const *root, const vector<string>& dom_classes)
{
    float *coords[3];
    char const *coordnames[3];
    coordnames[0] = "X";
    coordnames[1] = "Y";
    coordnames[2] = "Z";

    int nmats = 5;
    int ndoms = dom_classes.size();

    for (int i = 0; i < ndoms; i++)
    {
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
    }
}

static int WriteFormatReal(int argc, const char *const *const argv)
{
    ProcessArgsForVTK(argc, argv);

    char const *dclasses[] = {"Domain0", "Domain1", "Domain2", "Domain3", "Domain4"};
    int ndoms = sizeof(dclasses) / sizeof(dclasses[0]);
    vector<string> dom_classes(dclasses, dclasses + ndoms);
    int nmats = 5;

    WriteVTKApiSingleMesh("rocket.vtk", dom_classes);

/*
    WriteVTKApiMultiMesh("rocketm.vtk", dom_classes);
*/

    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

#ifdef STATIC_PLUGINS
int WriteFormat_vtk(int argc, const char *const *const argv)
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
