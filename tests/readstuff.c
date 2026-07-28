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

#include "silo.h"               /*include public silo           */
#include "std.c"

#define VERVAL(MAJ, MIN, PAT) ((size_t)MAJ*1000000+(size_t)MIN*1000+(size_t)PAT)

/*-------------------------------------------------------------------------
 * Function:	main
 *
 * Purpose: Test various read operations.	
 *
 * Return:	0
 *
 * Programmer:Mark C. Miller, Thu Jul 15 08:23:56 PDT 2010
 *-------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
    int            driver = DB_PDB, driverType = DB_PDB;
    int            i;
    DBfile        *dbfile;
    int            show_all_errors = FALSE;
    char           filename[256];
    char          *obj_names[13];
    int            ordering[13];

    /* Parse command-line */
    for (i=1; i<argc; i++) {
        if (!strncmp(argv[i], "DB_PDB", 6)) {
            driver = StringToDriver(argv[i]);
            driverType = DB_PDB;
        } else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
            driverType = DB_HDF5;
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
	} else if (argv[i][0] != '\0') {
	    fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
	}
    }
    
    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_TOP, NULL);
    DBForceSingle(1);

    sprintf(filename, "multi_rect2d.%s", driverType==DB_PDB?"pdb":"h5");
    dbfile = DBOpen(filename, DB_UNKNOWN, DB_READ);
    DBSetDir(dbfile, "block7");

    obj_names[0] = "cycle";
    obj_names[1] = "d";
    obj_names[2] = "../_fileinfo";
    obj_names[3] = "otherfile:block7/u";
    obj_names[4] = "v";
    obj_names[5] = "u";
    obj_names[6] = "/.silo/#000005";
    obj_names[7] = "../block7/d";
    obj_names[8] = "../block9/d";
    obj_names[9] = "../block4/d";
    obj_names[10] = "../mesh1_hidden";
    obj_names[11] = "../mesh1";
    obj_names[12] = "../block11/u";

    if (DBSortObjectsByOffset(dbfile, 13, (DBCAS_t) obj_names, ordering) < 0)
    {
        int maj, min, pat;
        DBDriverVersion(driverType, &maj, &min, &pat);
        if (driverType == DB_HDF5 && VERVAL(maj,min,pat) < VERVAL(1,12,0))
        {
            fprintf(stderr, "Object sorting not testable on this version of HDF5\n");
            return 0;
        }
        else
        {
            fprintf(stderr, "Object sorting failed: DBErrno = %d, \"%s\"\n", DBErrno(), DBErrString());
            return 1;
        }
    }
    
    printf("UNsorted objects...\n");
    for (i = 0; i < 13; i++)
        printf("\t\"%s\"\n", obj_names[i]);
    printf("Sorted objects...\n");
    for (i = 0; i < 13; i++)
        printf("\t\"%s\"\n", obj_names[ordering[i]]);

    DBClose(dbfile);

    return 0;
}
