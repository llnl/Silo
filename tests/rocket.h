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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//
// The following static arrays of data describe a single, 2D, 'layer'
// of the rocket body. They are used in the build_body() functions to
// build several layers of the 3D rocket body.
//
extern const float cX;
extern float layerXVals[];
extern const int layerNNodes;
extern float layerYVals[];
extern std::vector<float> xvals_g;
extern std::vector<float> yvals_g;
extern std::vector<float> zvals_g;

extern int layerNodelist[];
extern const int layerNZones;
extern std::vector<int> nodelist_g, nodecnts_g, nodestarts_g;

//
// Material names and numbers
// 
extern int matnos[];
extern char const *matNames[];
extern char const *matColors[];
extern std::map<std::string, int> matMap;
extern std::vector<int> matlist_g;

class EntityClassifier
{
    public:
        void AddClass(const std::string& cname)
        {
            int nclasses = class_masks.size();
            assert(nclasses < sizeof(unsigned long long)*8);
            assert(class_masks.find(cname) == class_masks.end());
            unsigned long long mask = ((unsigned long long) 1)<<(nclasses+1);
            class_masks[cname] = mask;
        }
        int AddEntity()
        {
            entity_masks.push_back(0x0);
            return entity_masks.size()-1;
        }
        void MakeEntityMemberOf(int idx, const std::string& cname)
        {
            assert(class_masks.find(cname) != class_masks.end());
            assert(idx < entity_masks.size());
            unsigned long long mask = class_masks[cname];
            entity_masks[idx] |= mask; 
        }
        bool IsEntityMemberOf(int idx, const std::string& cname) const
        {
            if (class_masks.find(cname) == class_masks.end()) return false;
            assert(idx < entity_masks.size());
            const unsigned long long mask = class_masks.at(cname);
            return entity_masks[idx] & mask;
        }
        bool IsEntityMemberOf(int idx, const std::vector<std::string>& cnames, int& ci) const
        {
            for (int i = 0; i < (int) cnames.size(); i++)
            {
                if (IsEntityMemberOf(idx, cnames[i]))
                {
                    ci = i;
                    return true;
                }
            }
            return false;
        }
        int GetNumEntities() const
        {
            return (int) entity_masks.size();
        }
        const std::string ZoneType(int idx) const
        {
            static const char* zt_classes[] = {
                "Hex", "Tet", "Wedge", "Prism", "Pyramid", "Polyhedron", // 3D
                "Quad", "Tri", "Polygon",                                // 2D
                "Beam",                                                  // 1D
                "Point"};                                                // 0D
            for (int i = 0; i < sizeof(zt_classes)/sizeof(zt_classes[0]); i++)
                if (IsEntityMemberOf(idx, zt_classes[i])) return zt_classes[i];
            return "Unknown"; 
        }
        // Gets list of entities in all specified classes
        void GetEntitiesInClasses(const std::vector<std::string>& cnames, std::vector<int>& elist) const
        {
            elist.clear();
            for (int i = 0; i < entity_masks.size(); i++)
            {
                bool inAllClasses = true;
                for (int j = 0; j < cnames.size() && inAllClasses; j++)
                    inAllClasses = IsEntityMemberOf(i, cnames[j]);
                if (inAllClasses) elist.push_back(i);
            }
        }
        // asserts classes define a 'partition' by initializing colors to -1
        void GetEntitiesPartitionedByClasses(const std::vector<std::string>& classes, std::vector<int>& colors) const
        {
            colors.resize(GetNumEntities());
            for (int i = 0; i < colors.size(); i++) colors[i] = -1;
            for (int i = 0; i < classes.size(); i++)
            {
                for (int j = 0; j < GetNumEntities(); j++)
                {
                    if (IsEntityMemberOf(j, classes[i]))
                    {
                        assert(colors[j] == -1);
                        colors[j] = i;
                    }
                }
            }
        }
        void PrintEntitiesInClasses(std::ostream& str) const
        {
            std::map<std::string, unsigned long long>::const_iterator cit;
            for (cit = class_masks.begin(); cit != class_masks.end(); cit++)
            {
                std::vector<int> ents;
                std::vector<std::string> ckclasses;
                ckclasses.push_back(cit->first);
                GetEntitiesInClasses(ckclasses, ents);
                if (!ents.size()) continue;
                str << "Entities in class \"" << cit->first << "\" [" << ents.size() << "]" << std::endl;
                for (int i = 0; i < ents.size(); i++)
                {
                    str << ents[i] << ((i==(ents.size()-1))?"":", ");
                    if (i && !((i*5)%80)) str << std::endl;
                }
                str << std::endl << std::endl;
            }
        }
        
    private:
        std::map<std::string, unsigned long long> class_masks;
        std::vector<unsigned long long> entity_masks;
};

extern EntityClassifier nodeClasses, zoneClasses;

extern void GetRocketBounds(
    float& xmin, float& ymin, float& zmin,
    float& xmax, float& ymax, float& zmax);
extern void GetZoneBounds(int idx,
    float& xmin, float& ymin, float& zmin,
    float& xmax, float& ymax, float& zmax);

extern void GetUpDownMapsForZoneClass(const std::string& zcname,
    std::vector<int>& upnodes, std::vector<int>& dnnodes,
    std::vector<int>& upzones, std::vector<int>& dnzones);

template <typename T> inline
void MapArray(const std::vector<int>& themap, const std::vector<T>& inarr,
    std::vector<T>& outarr, T fill)
{
    assert(inarr.size());
    outarr.assign(themap.size(), fill);
    for (int i = 0; i < themap.size(); i++)
    {
        if (themap[i] == -1) continue;
        assert(themap[i]<inarr.size());
        outarr[i] = inarr[themap[i]];
    }
}

typedef int (*FormatWriteFunc)(int argc, const char *const *const argv);
