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

#include "silo.h"
#include <math.h>
#include <stdlib.h>
#ifdef _WIN32
#include <string.h>
#endif
#include <std.c>

static void build_objs(DBfile *dbfile);

int main(int argc, char **argv)
{  
    DBfile        *dbfile;
    int         i, driver = DB_PDB;
    char        *filename = "buf_overrun_objs.pdb";
    int          show_all_errors = FALSE;

    for (i=1; i<argc; i++) {
        if (!strncmp(argv[i], "DB_PDB", 6)) {
            driver = StringToDriver(argv[i]);
            filename = "buf_overrun_objs.pdb";
        } else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
            filename = "buf_overrun_objs.h5";
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = TRUE;
	} else if (argv[i][0] != '\0') {
            fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
        }
    }

    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ALL, NULL);

    dbfile = DBCreate(filename, 0, DB_LOCAL, "objects causing buf overruns if read", driver);
    printf("Creating file: '%s'...\n", filename);
    build_objs(dbfile);
    DBClose(dbfile);

    CleanupDriverStuff();
    return 0;
}

void
build_objs(DBfile *dbfile)
{
    if (!dbfile) return;

    /* Write wild material with ndims of 1000. Use this object to ensure 
       attempts to read it won't buf overrun */
    {
        int dims[1000]; for (int i=0;i<1000;i++) dims[i]=1;
        int matnos[1] = {5}, matlist[1] = {7};
        DBPutMaterial(dbfile,"mat","mesh",1,matnos,matlist,dims,1000,
              NULL,NULL,NULL,NULL,0,DB_FLOAT,NULL);   /* returns 0 */

    }

    /* Write groupel map */
    {
        int i;
        int numsegs = 5;
        int *segTypes = (int *) malloc(numsegs * sizeof(int));
        int *segLens = (int *) malloc(numsegs * sizeof(int));
        int **segData = (int **) malloc(numsegs * sizeof(int*));

        segLens[0] = 1;
        segLens[1] = 1;
        segLens[2] = -100;
        segLens[3] = 1;
        segLens[4] = 100;

        for (i = 0; i < numsegs; i++)
        {
            segTypes[i] = DB_BLOCKCENT;
            if (segLens[i] > 0)
                segData[i] = (int *) calloc(segLens[i], sizeof(int));
            else
                segData[i] = 0;
        }

        DBPutGroupelmap(dbfile, "glmap", numsegs, segTypes, segLens,
            0, (int const * const *) segData, 0, 0, 0);

        for (i = 0; i < numsegs; i++)
        {
            if (segData[i]) free(segData[i]);
        }

        free(segTypes);
        free(segLens);
        free(segData);
    }
}
