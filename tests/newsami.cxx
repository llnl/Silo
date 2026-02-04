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

#include <silo.h>
#include <std.c>

#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

using std::map;
using std::vector;

static void move_random_dir(int ndirs, int& i, int& j, int& k)
{
    int mod;

    if (ndirs == 0)      /* face connected */
        mod = 6;
    else if (ndirs == 1) /* edge or face connected */
        mod = 18;
    else                 /* edge, face or vert connected */
        mod = 26;

#ifdef _WIN32
    int dir = rand() % mod;
#else
    int dir = random() % mod;
#endif
    switch (dir)
    {
        /* face connected cases */
         case 0: i--; break;
         case 1: i++; break;
         case 2: j--; break;
         case 3: j++; break;
         case 4: k--; break;
         case 5: k++; break;

        /* edge connected cases */
         case 6: i++; k++; break;
         case 7: i++; k--; break;
         case 8: i++; j++; break;
         case 9: i++; j--; break;
        case 10: i--; k++; break;
        case 11: i--; k--; break;
        case 12: i--; j++; break;
        case 13: i--; j--; break;
        case 14: j++; k++; break;
        case 15: j++; k--; break;
        case 16: j--; k++; break;
        case 17: j--; k--; break;

        /* vertex connected cases */
        case 18: i++; j++; k++; break;
        case 19: i++; j++; k--; break;
        case 20: i++; j--; k--; break;
        case 21: i++; j--; k++; break;
        case 22: i--; j++; k++; break;
        case 23: i--; j++; k--; break;
        case 24: i--; j--; k--; break;
        case 25: i--; j--; k++; break;
    }
}

class coord_t
{
  public:
    coord_t(double _c[3]) {c[0]=_c[0]; c[1]=_c[1]; c[2]=_c[2];};
    coord_t(int _c[3]) {c[0]=_c[0]; c[1]=_c[1]; c[2]=_c[2];};
    double operator[](int i) const {return c[i];};
    bool operator==(const coord_t& _c) const { return _c[0]==c[0] && _c[1]==c[1] && _c[2]==c[2]; };
    coord_t operator=(const coord_t& _c) { c[0]=_c[0]; c[1]=_c[1]; c[2]=_c[2]; return *this; };
    bool operator<(const coord_t& _c) const { 
      if (c[0] < _c[0]) return true;
      if (c[0] > _c[0]) return false;
      if (c[1] < _c[1]) return true;
      if (c[1] > _c[1]) return false;
      if (c[2] < _c[2]) return true;
      if (c[2] > _c[2]) return false;
      return false;
    };
  private:
    double c[3];
};

static void add_vert(map<coord_t, int>& coord_map, coord_t c, int& id, bool& new_vert)
{
    map<coord_t, int>::iterator it = coord_map.find(c);

    if (it != coord_map.end())
    {
        id = it->second;
        new_vert = false;
        return;
    }

    id = (int) coord_map.size();
    coord_map[c] = id;
    new_vert = true;
}

static void add_hex(int i, int j, int k, int mat, map<coord_t, int>& coord_map,
    vector<int> (&nodeids)[8], vector<int>& mats, bool& at_least_one_new_vert)
{
    static int const hex_vert_offsets[3][8] = {
        0, 0, 1, 1, 0, 0, 1, 1,
        0, 0, 0, 0, 1, 1, 1, 1,
        0, 1, 1, 0, 0, 1, 1, 0};

    int ids[8];
    at_least_one_new_vert = false;
    for (int n = 0; n < 8; n++)
    {
        bool new_vert;
        int c[3] = {i + hex_vert_offsets[0][n],
                    j + hex_vert_offsets[1][n],
                    k + hex_vert_offsets[2][n]};

        add_vert(coord_map, c, ids[n], new_vert);
        if (new_vert) at_least_one_new_vert = true;
    }

    if (at_least_one_new_vert)
    {
        for (int i = 0; i < 8; i++)
            nodeids[i].push_back(ids[i]);
        mats.push_back(mat);
    }
}

static void random_walk_hexes(int seed, int nhexes, int nmats, int ndirs,
    vector<double> (&coords)[3], vector<int> (&nodeids)[8], vector<int>& mats)
{
    int i=0, j=0, k=0, h=0, mat=1;
    map<coord_t, int> coord_map;
#ifdef _WIN32
    srand(seed);
#else
    srandom(seed);
#endif
    while (h < nhexes)
    {
        bool did_add = false;
        add_hex(i, j, k, nmats * h / nhexes + 1, coord_map, nodeids, mats, did_add);
        move_random_dir(ndirs, i, j, k);
        if (did_add) h++;
    }

    int nnodes = coord_map.size();
    coords[0].resize(nnodes);
    coords[1].resize(nnodes);
    coords[2].resize(nnodes);
    for (map<coord_t, int>::iterator it = coord_map.begin(); it != coord_map.end(); it++)
    {
        coords[0][it->second] = it->first[0];
        coords[1][it->second] = it->first[1];
        coords[2][it->second] = it->first[2];
    }
}

int main(int argc, char *argv[])
{
    DBfile		*dbfile;
    int			i, nerrors=0,  driver=DB_PDB;
    char const		*filename="newsami-test.sami";
    int                 show_all_errors = FALSE;
    int                 arch_g = DB_LOCAL;
    int                 seed = 0;
    int                 nhexes = 1000;
    int                 nmats = 5;
    int                 ndirs = 2;
    vector<double>      coords[3];
    vector<int>         nodeids[8];
    vector<int>         mats;
    
    for (i=1; i<argc; i++) {
	if (!strcmp(argv[i], "DB_LOCAL")) {
	    arch_g = DB_LOCAL;
	} else if (!strcmp(argv[i], "DB_SUN3")) {
	    arch_g = DB_SUN3;
	} else if (!strcmp(argv[i], "DB_SUN4")) {
	    arch_g = DB_SUN4;
	} else if (!strcmp(argv[i], "DB_SGI")) {
	    arch_g = DB_SGI;
	} else if (!strcmp(argv[i], "DB_RS6000")) {
	    arch_g = DB_RS6000;
	} else if (!strcmp(argv[i], "DB_CRAY")) {
	    arch_g = DB_CRAY;
	} else if (!strncmp(argv[i], "DB_",3)) {
            driver = StringToDriver(argv[i]);
	} else if (!strncmp(argv[i], "nhexes=", 7)) {
            nhexes = (int) strtol(argv[i]+7,0,10);
	} else if (!strncmp(argv[i], "nmats=", 6)) {
            nmats = (int) strtol(argv[i]+6,0,10);
	} else if (!strncmp(argv[i], "ndirs=", 6)) {
            ndirs = (int) strtol(argv[i]+6,0,10);
	} else if (!strncmp(argv[i], "seed=", 5)) {
            seed = (int) strtol(argv[i]+5,0,10);
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
	} else if (argv[i][0] != '\0') {
	    fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
	}
    }

    /* Create random walk of hexahedral elements */
    random_walk_hexes(seed, nhexes, nmats, ndirs, coords, nodeids, mats);
    
    if (show_all_errors) DBShowErrors(DB_ALL_AND_DRVR, 0);

    /* turn of deprecate warnings */
    DBSetDeprecateWarnings(0);

    /*
     * Create a new file (clobbering any old file), write some variables to
     * the file, then read them and compare with what was written.
     */
    puts("=== Creating file ===");
    if (NULL==(dbfile=DBCreate(filename, DB_CLOBBER, arch_g,
			       "testing SAMI HDF5 silo driver", driver))) {
	puts("DBCreate() failed");
	nerrors++;
    }

    /* Write the coordinate data */
    int nnodes = coords[0].size();
    char const *cnames[3] = {"x", "y", "z"};
    for (i = 0; i < 3; i++)
        DBWrite(dbfile, cnames[i], &coords[i][0], &nnodes, 1, DB_DOUBLE);

    /* Write the nodelist data */
    int nzones = nodeids[0].size();
    for (i = 0; i < 8; i++)
    {
        char tmpname[32];
        snprintf(tmpname, sizeof(tmpname), "brick_nd%d", i);
        DBWrite(dbfile, tmpname, &nodeids[i][0], &nzones, 1, DB_INT);
    }

    /* Write the material data */
    DBWrite(dbfile, "brick_material", &mats[0], &nzones, 1, DB_INT);

                       /* #zones, #shells, #beams, #nodes,  #mats, #slides, index-origin, ndims */
    int mesh_data[8] = {  nzones,       0,      0, nnodes,  nmats,       0,            0,     3};
    int len = 8;
    DBWrite(dbfile, "mesh_data", mesh_data, &len, 1, DB_INT);

    if (DBClose(dbfile)<0) {
	puts("DBClose() failed");
	nerrors++;
    }

    if (nerrors) {
	printf("*** %d error%s detected ***\n", nerrors, 1==nerrors?"":"s");
    } else {
	puts("All sami tests passed.");
    }
    CleanupDriverStuff();
    return nerrors?1:0;
}
