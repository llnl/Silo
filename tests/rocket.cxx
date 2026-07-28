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

#define HAVE_SILO
//#define STATIC_PLUGINS

#ifdef __sgi    /* IRIX C++ bug */
#include <math.h>
#else
#include <cmath>
#endif
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>

#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#ifndef STATIC_PLUGINS
#include <dlfcn.h>
#endif

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

//
// The following arrays of data describe a single, 2D, 'layer'
// of the rocket body. They are used in the build_body() functions to
// build several layers of the 3D rocket body.
//
const float cX = (float) sqrt(2.0);
float layerXVals[] =
{
//   Internal 9 nodes (0 is the mid-line node)
//   0     1     2     3     4     5     6     7     8
    0.0, -1.0, -1.0,  0.0,  1.0,  1.0,  1.0,  0.0, -1.0,

//   External 8 nodes
//   9    10    11    12    13    14    15    16
   -2.0, -cX,   0.0,  cX,   2.0,  cX,   0.0, -cX
};
const int layerNNodes = sizeof(layerXVals) / sizeof(layerXVals[0]);
float layerYVals[] =
{
//   Internal 9 nodes (0 is the mid-line node)
//   0     1     2     3     4     5     6     7     8
    0.0,  0.0,  1.0,  1.0,  1.0,  0.0, -1.0, -1.0, -1.0,

//   External 8 nodes
//   9    10    11    12    13    14    15    16
    0.0,   cX,  2.0,  cX,   0.0, -cX,  -2.0, -cX
};
vector<float> xvals_g, yvals_g, zvals_g;

int layerNodelist[] =
{
//  Internal 4 zones
//     0             1             2             3 
    0,3,2,1,      0,5,4,3,      7,6,5,0,      8,7,0,1,

//  External 8 zones
//     4             5             6             7 
    1,2,10,9,     2,3,11,10,    3,4,12,11,    4,5,13,12,

//     8             9             10            11 
    5,6,14,13,    6,7,15,14,    7,8,16,15,    8,1,9,16
};
const int layerNZones = sizeof(layerNodelist) / (4*sizeof(layerNodelist[0]));
vector<int> nodelist_g, nodecnts_g, nodestarts_g;

//
// Material names and numbers
// 
int matnos[] = {1,2,3,4,5};
char const *matNames[] =
{
    "High Explosive",
    "Solid Propellant",
    "Liquid Propellant",
    "Electronics",
    "Body"
};
char const *matColors[] =
{
    "Red",
    "Magenta",
    "Green",
    "Black",
    "Gray"
};
map<string, int> matMap;
vector<int> matlist_g;

EntityClassifier nodeClasses, zoneClasses;

static int AddNode(float x, float y, float z, int level)
{
    xvals_g.push_back(x);
    yvals_g.push_back(y);
    zvals_g.push_back(z);
    stringstream tmp;
    tmp << level;
    string lname = "Level" + tmp.str();
    int idx = nodeClasses.AddEntity();
    nodeClasses.MakeEntityMemberOf(idx, lname);
    return idx;
}

static int AddZone(string ztype, const vector<int>& nlist, int level)
{
    assert((ztype=="Hex" && nlist.size()==8) ||
           (ztype=="Wedge" && nlist.size()==6) ||
           (ztype=="Prism" && nlist.size()==6) ||
           (ztype=="Pyramid" && nlist.size()==5) ||
           (ztype=="Tet" && nlist.size()==4));
    nodestarts_g.push_back(nodelist_g.size());
    nodecnts_g.push_back(nlist.size());
    for (int i = 0; i < nlist.size(); i++)
    {
        assert(nlist[i] < nodeClasses.GetNumEntities());
        nodelist_g.push_back(nlist[i]);
    }
    int idx = zoneClasses.AddEntity();
    zoneClasses.MakeEntityMemberOf(idx, ztype);
    stringstream tmp;
    tmp << level;
    string lname = "Level" + tmp.str();
    zoneClasses.MakeEntityMemberOf(idx, lname);
    return idx;
}

void GetRocketBounds(
    float& xmin, float& ymin, float& zmin,
    float& xmax, float& ymax, float& zmax)
{
    xmin = xmax = xvals_g[0];
    ymin = ymax = yvals_g[0];
    zmin = zmax = zvals_g[0];
    for (int i = 1; i < xvals_g.size(); i++)
    {
        if (xvals_g[i] < xmin) xmin = xvals_g[i];
        if (xvals_g[i] > xmax) xmax = xvals_g[i];
        if (yvals_g[i] < ymin) ymin = yvals_g[i];
        if (yvals_g[i] > ymax) ymax = yvals_g[i];
        if (zvals_g[i] < zmin) zmin = zvals_g[i];
        if (zvals_g[i] > zmax) zmax = zvals_g[i];
    }
}

void GetZoneBounds(int idx,
    float& xmin, float& ymin, float& zmin,
    float& xmax, float& ymax, float& zmax)
{
    assert(idx < nodestarts_g.size());
    assert(idx < zoneClasses.GetNumEntities());
    int i0 = nodestarts_g[idx];
    int n0 = nodelist_g[i0];
    assert(n0 < xvals_g.size());
    xmin = xmax = xvals_g[n0];
    ymin = ymax = yvals_g[n0];
    zmin = zmax = zvals_g[n0];
    int cnt = nodecnts_g[idx];
    for (int i = 1; i < cnt; i++)
    {
        int n = nodelist_g[i0+i];
        assert(n < xvals_g.size());
        if (xvals_g[n] < xmin) xmin = xvals_g[n];
        if (xvals_g[n] > xmax) xmax = xvals_g[n];
        if (yvals_g[n] < ymin) ymin = yvals_g[n];
        if (yvals_g[n] > ymax) ymax = yvals_g[n];
        if (zvals_g[n] < zmin) zmin = zvals_g[n];
        if (zvals_g[n] > zmax) zmax = zvals_g[n];
    }
}

void GetUpDownMapsForZoneClass(const string& zcname,
    vector<int>& upnodes, vector<int>& dnnodes,
    vector<int>& upzones, vector<int>& dnzones)
{
    // Get list of zones in this class. Thats the upzones map
    vector<string> classes;
    classes.push_back(zcname);
    zoneClasses.GetEntitiesInClasses(classes, upzones);

    // Compute down zones map from upzones
    dnzones.assign(zoneClasses.GetNumEntities(),-1);
    for (int i = 0; i < upzones.size(); i++)
        dnzones[upzones[i]] = i;

    // Build set of unique nodes.
    set<int> uniq;
    for (int i = 0; i < upzones.size(); i++)
    {
        int j0 = nodestarts_g[upzones[i]];
        int cnt = nodecnts_g[upzones[i]];
        for (int j = 0; j < cnt; j++)
            uniq.insert(nodelist_g[j0+j]); // set ensures uniqueness
    }
    for (set<int>::const_iterator it = uniq.begin(); it != uniq.end(); it++)
        upnodes.push_back(*it);

    // Compute down nodes map from upnodes
    dnnodes.assign(nodeClasses.GetNumEntities(),-1);
    for (int i = 0; i < upnodes.size(); i++)
        dnnodes[upnodes[i]] = i;
}

//
// Makes the bottom layer of nodes at z=0
//
static void make_base_layer()
{
    for (int i = 0; i < layerNNodes; i++)
    {
        int id = AddNode(layerXVals[i], layerYVals[i], 0.0, 0);
        if (i==0) nodeClasses.MakeEntityMemberOf(id, "Midline");
        if (i<9) nodeClasses.MakeEntityMemberOf(id, "Internal");
        else nodeClasses.MakeEntityMemberOf(id, "External");
        nodeClasses.MakeEntityMemberOf(id, "Stage1");
    }
}

//
// Adds a layer of nodes and then connects them to
// the previous layer making hex elements
//
static void add_layer(int stage, int zval)
{
    int i;

    stringstream tmp;
    tmp << stage;
    string stage_name = "Stage"+tmp.str();

    for (i = 0; i < layerNNodes; i++)
    {
        int nid = AddNode(layerXVals[i], layerYVals[i], zval, 0);
        if (i==0) nodeClasses.MakeEntityMemberOf(nid, "Midline");
        if (i<9) nodeClasses.MakeEntityMemberOf(nid, "Internal");
        else nodeClasses.MakeEntityMemberOf(nid, "External");
        nodeClasses.MakeEntityMemberOf(nid, stage_name);
    }
    for (i = 0; i < layerNZones; i++)
    {
        int j;
        vector<int> nl;
        for (j = 0; j < 4; j++)
            nl.push_back(layerNodelist[i*4+j] + (zval-1)*layerNNodes);
        for (j = 0; j < 4; j++)
            nl.push_back(layerNodelist[i*4+j] + zval*layerNNodes);
        int zid = AddZone("Hex", nl, 0);
        if (i<4)
        {
            zoneClasses.MakeEntityMemberOf(zid, "Internal");
            zoneClasses.MakeEntityMemberOf(zid, "Solid Propellant");
        }
        else
        {
            zoneClasses.MakeEntityMemberOf(zid, "External");
            zoneClasses.MakeEntityMemberOf(zid, "Body");
        }
        zoneClasses.MakeEntityMemberOf(zid, stage_name);
    }
}

//
// Adds pyramid elements that taper to a single apex which is
// the nose of the rocket.
//
void add_nose(int stage, int zval)
{
    int i,j;

    stringstream tmp;
    tmp << stage;
    string stage_name = "Stage" + tmp.str();

    // add central core nodes at this zval
    int layer1NoseStart = xvals_g.size();
    for (i = 0; i < 9; i++)
    {
        int nid = AddNode(layerXVals[i], layerYVals[i], zval, 0);
        if (i==0)
        {
            nodeClasses.MakeEntityMemberOf(nid, "Midline");
            nodeClasses.MakeEntityMemberOf(nid, "Internal");
        }
        else nodeClasses.MakeEntityMemberOf(nid, "External");
        nodeClasses.MakeEntityMemberOf(nid, stage_name);
    }

    // add nose end node at zval + 1
    int noseApex = xvals_g.size();
    int nid2 = AddNode(0.0, 0.0, (float)zval+1.0, 0);
    nodeClasses.MakeEntityMemberOf(nid2, "Midline");
    nodeClasses.MakeEntityMemberOf(nid2, "External");
    nodeClasses.MakeEntityMemberOf(nid2, stage_name);

    // add central core hexes
    for (i = 0; i < 4; i++)
    {
        vector<int> nl;
        for (j = 0; j < 4; j++)
            nl.push_back(layerNodelist[i*4+j] + (zval-1)*layerNNodes);
        for (j = 0; j < 4; j++)
            nl.push_back(layerNodelist[i*4+j] + zval*layerNNodes);
        int zid = AddZone("Hex", nl, 0);
        zoneClasses.MakeEntityMemberOf(zid, "Internal");
        zoneClasses.MakeEntityMemberOf(zid, "Nose");
        zoneClasses.MakeEntityMemberOf(zid, "Guidance");
        zoneClasses.MakeEntityMemberOf(zid, stage_name);
        zoneClasses.MakeEntityMemberOf(zid, "Liquid Propellant");
    }

    // add external wedges
    int permute[] = {1, 0, 3, 2}; // wedge base is permuted from hex
    int k = layer1NoseStart + 1;
    for (i = 4; i < layerNZones; i++) // just the outer layer of hex bases
    {
        vector<int> nl;
        for (j = 0; j < 4; j++)
            nl.push_back(layerNodelist[i*4+permute[j]] + (zval-1)*layerNNodes);
        if (i == layerNZones - 1)
        {
            nl.push_back(layer1NoseStart+1);
            nl.push_back(k);
        }
        else
        {
            nl.push_back(k+1);
            nl.push_back(k);
        }
        k++;
        int zid = AddZone("Wedge", nl, 0);
        zoneClasses.MakeEntityMemberOf(zid, "External");
        zoneClasses.MakeEntityMemberOf(zid, "Nose");
        zoneClasses.MakeEntityMemberOf(zid, "Ring");
        zoneClasses.MakeEntityMemberOf(zid, stage_name);
        zoneClasses.MakeEntityMemberOf(zid, i%2 ? "Electronics" : "Body");
    }

    // add top-level pyramids
    for (i = 0; i < 4; i++)
    {
        vector<int> nl;
        for (j = 0; j < 4; j++)
            nl.push_back(layerNodelist[i*4+permute[j]] + layer1NoseStart);
        nl.push_back(noseApex);
        int zid = AddZone("Pyramid", nl, 0);
        zoneClasses.MakeEntityMemberOf(zid, "External");
        zoneClasses.MakeEntityMemberOf(zid, "Nose");
        zoneClasses.MakeEntityMemberOf(zid, "Mirvs");
        stringstream tmp;
        tmp << i+1;
        zoneClasses.MakeEntityMemberOf(zid, "Mirv"+tmp.str());
        zoneClasses.MakeEntityMemberOf(zid, stage_name);
        zoneClasses.MakeEntityMemberOf(zid, "High Explosive");
    }
}

//
// Adds the fins of the rocket around the bottom two layers
//
void add_fins()
{
    int i;
    float finX = 4 * cos(22.5 / 180.0 * M_PI);
    float finY = 4 * sin(22.5 / 180.0 * M_PI);

    // add layer 0,1 and 2 fin tip nodes
    int finNodesStart = xvals_g.size();
    for (i = 0; i < 3; i++)
    {
        int nid;
        nid = AddNode(-finX, finY, (float)i, 0);
        nodeClasses.MakeEntityMemberOf(nid, "Fin1");
        nodeClasses.MakeEntityMemberOf(nid, "Fins");

        nid = AddNode(finY, finX, (float)i, 0);
        nodeClasses.MakeEntityMemberOf(nid, "Fin2");
        nodeClasses.MakeEntityMemberOf(nid, "Fins");

        nid = AddNode(finX, -finY, (float)i, 0);
        nodeClasses.MakeEntityMemberOf(nid, "Fin3");
        nodeClasses.MakeEntityMemberOf(nid, "Fins");

        nid = AddNode(-finY, -finX, (float)i, 0);
        nodeClasses.MakeEntityMemberOf(nid, "Fin4");
        nodeClasses.MakeEntityMemberOf(nid, "Fins");
    }

    // add fin bottoms (wedges) on layer 0 and 1
    int startNodes[] = {9, 11, 13, 15};
    for (i = 0; i < 8; i++)
    {
        int n = startNodes[i%4] + (i/4)*layerNNodes;
        vector<int> nl;
        nl.push_back(n+1);
        nl.push_back(n+1+layerNNodes);
        nl.push_back(n+layerNNodes);
        nl.push_back(n);
        nl.push_back(finNodesStart+i);
        nl.push_back(finNodesStart+i+4);
        int zid = AddZone("Wedge", nl, 0);
        zoneClasses.MakeEntityMemberOf(zid, "Fins");
        stringstream tmp;
        tmp << i%4+1;
        zoneClasses.MakeEntityMemberOf(zid, "Fin"+tmp.str());
        zoneClasses.MakeEntityMemberOf(zid, "Body");
    }

    // add fin tops (pyramids) on layer 2
    for (i = 0; i < 4; i++)
    {
        int n = startNodes[i]+2*layerNNodes;
        vector<int> nl;
        nl.push_back(n+1);
        nl.push_back(n+1+layerNNodes);
        nl.push_back(n+layerNNodes);
        nl.push_back(n);
        nl.push_back(finNodesStart+i+8);
        int zid = AddZone("Pyramid", nl, 0);
        zoneClasses.MakeEntityMemberOf(zid, "Fins");
        stringstream tmp;
        tmp << i+1;
        zoneClasses.MakeEntityMemberOf(zid, "Fin"+tmp.str());
        zoneClasses.MakeEntityMemberOf(zid, "Body");
    }
}

//
// Build the rocket from the simple 2D description of a
// single layer of nodal positions.
//
static void build_rocket(const vector<int>& layers_per_stage)
{
    int j, k = 1;
    make_base_layer();
    for (j = 0; j < layers_per_stage.size(); j++)
    {
        for (int i = 0; i < layers_per_stage[j]; i++, k++)
            add_layer(j+1, k);
    }
    add_nose(j+1,k);
    add_fins();
}

//
// Decomposes the rocket such that lower half is split lengthwise
// into two pieces while the upper half is split crosswise into three
// pieces for a total of 5 domains. All this function does is 
// assign zones to one of the 5 domain classes.
//
static void ParallelDecompose()
{
    float dummy, zmin, zmax;
    GetRocketBounds(dummy, dummy, zmin, dummy, dummy, zmax);
    float zhalf = (zmin + zmax) / 2.0;
    float z3 = (zmax - zmin) / 2.0 / 3.0;

    for (int i = 0; i < zoneClasses.GetNumEntities(); i++)
    {
        float xmin, xmax, ymin, ymax;
        GetZoneBounds(i, xmin, ymin, zmin, xmax, ymax, zmax);
        float zcenter = (zmin + zmax) / 2.0;
        if (zcenter < zhalf)
        {
            float ycenter = (ymin + ymax) / 2.0;
            if (ycenter < 0) zoneClasses.MakeEntityMemberOf(i, "Domain0");
            else zoneClasses.MakeEntityMemberOf(i, "Domain1");
        }
        else
        {
            if (zcenter < (zhalf + z3)) zoneClasses.MakeEntityMemberOf(i, "Domain2");
            else if (zcenter < (zhalf + 2*z3)) zoneClasses.MakeEntityMemberOf(i, "Domain3");
            else zoneClasses.MakeEntityMemberOf(i, "Domain4");
        }
    }
}

static void DefineNodesets()
{
    nodeClasses.AddClass("HighQ");
    nodeClasses.AddClass("HoldDownPoints");
    nodeClasses.AddClass("TowerContact");

    // HighQ nodes
    for (int i = 290; i<= 298; i++)
        nodeClasses.MakeEntityMemberOf(i, "HighQ");
    for (int i = 307; i<= 310; i++)
        nodeClasses.MakeEntityMemberOf(i, "HighQ");

    // Hold down nodes
    for (int i = 1; i<= 8; i++)
        nodeClasses.MakeEntityMemberOf(i, "HoldDownPoints");
    for (int i = 10; i<= 16; i+=2)
        nodeClasses.MakeEntityMemberOf(i, "HoldDownPoints");

    // Tower contact nodes
    for (int i = 9; i<= 281; i += 17)
        nodeClasses.MakeEntityMemberOf(i, "TowerContact");
}

#define MakeFaceFromNodesEntityMemberOf(c, n0, n1, n2, n3) \
{ \
    nodeClasses.MakeEntityMemberOf(n0, c); \
    nodeClasses.MakeEntityMemberOf(n1, c); \
    nodeClasses.MakeEntityMemberOf(n2, c); \
    if (n3 != -1) nodeClasses.MakeEntityMemberOf(n3, c); \
}

// for now, we'll do this in terms of nodes of the faces
static void DefineFacesets()
{
    nodeClasses.AddClass("PeakPressure");
    nodeClasses.AddClass("ControlSurfaces");

    MakeFaceFromNodesEntityMemberOf("PeakPressure", 66, 67, 310, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 64, 65, 309, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 62, 63, 308, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 60, 61, 307, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 298, 290, 297, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 298, 297, 296, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 298, 296, 295, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 298, 295, 294, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 298, 294, 293, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 298, 293, 292, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 298, 292, 291, -1);
    MakeFaceFromNodesEntityMemberOf("PeakPressure", 298, 291, 290, -1);

    MakeFaceFromNodesEntityMemberOf("ControlSurfaces", 16, 302, 306, 33);
    MakeFaceFromNodesEntityMemberOf("ControlSurfaces", 302, 15, 32, 306);
    MakeFaceFromNodesEntityMemberOf("ControlSurfaces", 14, 301, 305, 31);
    MakeFaceFromNodesEntityMemberOf("ControlSurfaces", 301, 13, 30, 305);
    MakeFaceFromNodesEntityMemberOf("ControlSurfaces", 12, 300, 304, 29);
    MakeFaceFromNodesEntityMemberOf("ControlSurfaces", 300, 11, 28, 304);
    MakeFaceFromNodesEntityMemberOf("ControlSurfaces", 10, 299, 303, 27);
    MakeFaceFromNodesEntityMemberOf("ControlSurfaces", 299, 9, 26, 303);
}

#if 0
static void GetAvgNodalCoords(int idx,
    int n, const int *const offsets,
    float& xavg, float& yavg, float& zavg)
{
    assert(idx<nodestarts_g.size());
    int i0 = nodestarts_g[idx];
    int cnt = nodecnt_g[idx];
    assert(n>0&&n<=cnt);
    xavg = yavg = zavg = 0.0;
    for (int i = 0; i < n; i++)
    {
        int nid = nodelist_g[i0+offsets[i]];
        xavg += xvals_g[nid];
        yavg += yvals_g[nid];
        zavg += zvals_g[nid];
    }
    xavg /= cnt;
    yavg /= cnt;
    zavg /= cnt;
}

static void RefineHex(int idx)
{
    assert(idx<nodestarts_g.size());
    int *poffs;

    float x, y, z;

    // Add Center node
    for (int i = 0; i < 8; i++)
        offs[i] = i;
    GetAvgNodalCoords(idx, offs, x, y, z);
    int cid = AddNode(x, y, z, 1);

    // Add center face nodes


    // Add center edge nodes

    // Build hex zones.

}

static void RefineWedge()
{
}

static void RefinePyramid()
{
}

static void AMRRefine()
{
    for (int i = 0; i < zoneClasses.GetNumEntities(); i++)
    {
        string ztn = zoneClasses.ZoneType(i);
        if (ztn == "Hex")
    }
}
#endif

static void DefineNodeClasses()
{
    nodeClasses.AddClass("Point");

    nodeClasses.AddClass("Midline");
    nodeClasses.AddClass("Internal");
    nodeClasses.AddClass("External");
    nodeClasses.AddClass("Nose");
    nodeClasses.AddClass("Fins");
    nodeClasses.AddClass("Fin1");
    nodeClasses.AddClass("Fin2");
    nodeClasses.AddClass("Fin3");
    nodeClasses.AddClass("Fin4");
    nodeClasses.AddClass("Stage1");
    nodeClasses.AddClass("Stage2");
    nodeClasses.AddClass("Stage3");
    nodeClasses.AddClass("Stage4");
    nodeClasses.AddClass("Stage5");

    nodeClasses.AddClass("Level0");
    nodeClasses.AddClass("Level1");
    nodeClasses.AddClass("Level2");
}

static void DefineZoneClasses()
{
    zoneClasses.AddClass("Hex");
    zoneClasses.AddClass("Wedge");
    zoneClasses.AddClass("Pyramid");
    zoneClasses.AddClass("Tet");

    zoneClasses.AddClass("Domain0");
    zoneClasses.AddClass("Domain1");
    zoneClasses.AddClass("Domain2");
    zoneClasses.AddClass("Domain3");
    zoneClasses.AddClass("Domain4");

    zoneClasses.AddClass("Internal");
    zoneClasses.AddClass("External");

    zoneClasses.AddClass("Stage1");
    zoneClasses.AddClass("Stage2");
    zoneClasses.AddClass("Stage3");
    zoneClasses.AddClass("Stage4");
    zoneClasses.AddClass("Stage5");

    zoneClasses.AddClass("Nose");
    zoneClasses.AddClass("Ring");
    zoneClasses.AddClass("Fins");
    zoneClasses.AddClass("Fin1");
    zoneClasses.AddClass("Fin2");
    zoneClasses.AddClass("Fin3");
    zoneClasses.AddClass("Fin4");
    zoneClasses.AddClass("Mirvs");
    zoneClasses.AddClass("Mirv1");
    zoneClasses.AddClass("Mirv2");
    zoneClasses.AddClass("Mirv3");
    zoneClasses.AddClass("Mirv4");
    zoneClasses.AddClass("Guidance");

    zoneClasses.AddClass("Level0");
    zoneClasses.AddClass("Level1");
    zoneClasses.AddClass("Level2");

    zoneClasses.AddClass("High Explosive");
    zoneClasses.AddClass("Solid Propellant");
    zoneClasses.AddClass("Liquid Propellant");
    zoneClasses.AddClass("Electronics");
    zoneClasses.AddClass("Body");
}

//
// Purpose: Build a simple, 3D mesh with a lot of interesting subsets
// to serve as a talking point for VisIt's subsetting functionality.
//
// Modifications:
//    Mark C. Miller, Wed Jul 14 15:33:02 PDT 2010
//    Added show-all-errors option.
//

#ifndef STATIC_PLUGINS
typedef struct _func_and_handle_t
{
    FormatWriteFunc func;
    void *dlhandle;
} func_and_handle_t;

static int
WriteAllFormats(int argc, char **argv)
{
    int nerrors = 0;
    int d, foundOne = 0;
    map<string, func_and_handle_t> formatMap;

    // Accomodate all possible places autotools may wind up building the plugins
    char const *dirs[] = {".", "./lib", "../..", ".libs", "../../.libs"};
    for (d = 0; d < sizeof(dirs)/sizeof(dirs[0]) && !foundOne; d++)
    {
        DIR *cwdir = opendir(dirs[d]);
        if (!cwdir) continue;
    
        struct dirent* dent;
        while ((dent = readdir(cwdir)))
        {
            string dname = string(dent->d_name);

            if (dname.find("rocket_") == string::npos)
                continue;

            string fmtname, pname;
            if (dname.rfind(".dylib") != string::npos)
            {
                fmtname = dname.substr(7,dname.size()-13);
                pname = string(dirs[d])+"/"+dname;
            }
            else if (dname.rfind(".dll") != string::npos)
            {
                fmtname = dname.substr(7,dname.size()-11);
                pname = string(dirs[d])+"/"+dname;
            }
            else if (dname.rfind(".so") != string::npos)
            {
                fmtname = dname.substr(7,dname.size()-10);
                pname = string(dirs[d])+"/"+dname;
            }
            else if (dname.rfind(".o") != string::npos)
            {
                fmtname = dname.substr(7,dname.size()-9);
                pname = string(dirs[d])+"/"+dname;
            }
            else
            {
                continue;
            }

            void *dlhandle = dlopen(pname.c_str(), RTLD_LAZY);
            if (!dlhandle)
            {
                cerr << "Encountered dlopen error \"" << dlerror() << "\" for file \""
                     << pname << "\" Skipping it." << endl;
                continue;
            }
    
            FormatWriteFunc writeFunc = (FormatWriteFunc) dlsym(dlhandle, "WriteFormat");
            if (!writeFunc)
            {
                cerr << "Encountered dlsym error \"" << dlerror() << "\" for format \""
                     << fmtname << "\". Skipping it." << endl;
                dlclose(dlhandle);
                continue;
            }
    
            foundOne = 1;
            cout << "Using \"" << dirs[d] << "\" as plugin dir." << endl;
            formatMap[fmtname].func = writeFunc;
            formatMap[fmtname].dlhandle = dlhandle;
        }

        closedir(cwdir);
    }

    if (!foundOne)
    {
        fprintf(stderr, "Unable to find suitable plugin directory\n");
        exit(1);
    }
    
    cout << "Found " << formatMap.size() << " format writers." << endl;
    
    for (map<string, func_and_handle_t>::iterator it = formatMap.begin(); it != formatMap.end(); it++)
    {
        cout << "Processing format \"" << it->first << "\"..." << endl;
        nerrors += ((it->second).func)(argc, argv);
        dlclose((it->second).dlhandle);
    }

    return nerrors + !formatMap.size();
}
#endif

#ifdef STATIC_PLUGINS

#ifdef HAVE_SILO
extern int WriteFormat_silo(int argc, const char *const *const argv);
#endif
#ifdef HAVE_VTK
extern int WriteFormat_vtk(int argc, const char *const *const argv);
#endif

#endif

int
main(int argc, char **argv)
{
    int i, nerrors = 0;

    DefineNodeClasses();
    DefineZoneClasses();

    /* build the monolithic, single domain rocket geometry, etc */
    const int layer_sizes[] = {8, 5, 3};
    vector<int> layers_per_stage(layer_sizes, layer_sizes + sizeof(layer_sizes)/sizeof(layer_sizes[0]));
    build_rocket(layers_per_stage);

    DefineNodesets();
    DefineFacesets();

    ParallelDecompose();

#ifdef STATIC_PLUGINS

#ifdef HAVE_SILO
    nerrors += WriteFormat_silo(argc, argv);
#endif
#ifdef HAVE_VTK
    nerrors += WriteFormat_vtk(argc, argv);
#endif

#else // STATIC_PLUGINS

    nerrors += WriteAllFormats(argc, argv);

#endif

    return nerrors;
}
