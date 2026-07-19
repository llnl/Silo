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

#include <stdlib.h>     /* For exit()   */
#include <string.h>     /* For strcmp() */
#include "silo.h"
#include <std.c>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

static char cwd[4096];

static int VersionNumberGE(int aMaj, int aMin, int aPat,
                           int bMaj, int bMin, int bPat)
{
    if (aMaj < 0) aMaj = 0;
    if (aMin < 0) aMin = 0;
    if (aPat < 0) aPat = 0;

    if (bMaj < 0) bMaj = 0;
    if (bMin < 0) bMin = 0;
    if (bPat < 0) bPat = 0;

    if (aMaj > bMaj)
        return 1;
    else if (aMaj < bMaj)
        return 0;
    else
    {
        if (aMin > bMin)
            return 1;
        else if (aMin < bMin)
            return 0;
        else
        {
            if (aPat >= bPat)
                return 1;
            else
                return 0;
        }
    }
}

int
main(int argc, char *argv[])
{
    DBfile        *dbfile;
    int           i, pass, driver=DB_PDB;
    static char   *filename="version.pdb";
    int            show_all_errors = FALSE;
    int           vnos[4];
    char          voverwrite[5] = "99.9";

    for (i=1; i<argc; i++) {
        if (!strncmp(argv[i], "DB_PDB", 6)) {
            driver = StringToDriver(argv[i]);
            filename = "version.pdb";
        } else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
            filename = "version.h5";
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
	} else if (argv[i][0] != '\0') {
            fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
        }
    }

    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ALL, NULL);

    /* test version macro with 1 digit version number */
#if SILO_VERSION_GE(4,0,0)
    printf("This version of the Silo library is greater than or equal to 4\n");
#else
    printf("This version of the Silo library is NOT greater than or equal to 4\n");
#endif
#if SILO_VERSION_GE(9,0,0)
    printf("This version of the Silo library is greater than or equal to 9\n");
#else
    printf("This version of the Silo library is NOT greater than or equal to 9\n");
#endif

    /* test version macro with 2 digit version number */
#if SILO_VERSION_GE(4,6,0)
    printf("This version of the Silo library is greater than or equal to 4.6\n");
#else
    printf("This version of the Silo library is NOT greater than or equal to 4.6\n");
#endif

    /* test version macro with 3 digit version number */
#if SILO_VERSION_GE(90,5,2)
    printf("This version of the Silo library is greater than or equal to 90.5.2\n");
#else
    printf("This version of the Silo library is NOT greater than or equal to 90.5.2\n");
#endif

    /* test run-time version methods for lib */
    printf("DBVersion() returns \"%s\"\n", DBVersion());
    printf("DBVersionGE(4,6,0) returns %d\n", DBVersionGE(4,6,0));
    printf("DBVersionGE(90,5,2) returns %d\n", DBVersionGE(90,5,2));
    
    dbfile = DBCreate(filename, 0, DB_LOCAL,
                      "version test file", driver);
    
    /* test run-time version methods for files */
    printf("On file handle returned from DBCreate...\n");
    printf("    DBFileVersion() returns \"%s\"\n", DBFileVersion(dbfile));
    printf("    DBFileVersionGE(4,6,0) returns %d\n", DBFileVersionGE(dbfile,4,6,0));
    printf("    DBFileVersionGE(90,5,2) returns %d\n", DBFileVersionGE(dbfile,90,5,2));
    DBClose(dbfile);

    dbfile = DBOpen(filename, driver, DB_READ);
    printf("On file handle returned from DBOpen...\n");
    printf("    DBFileVersion() returns \"%s\"\n", DBFileVersion(dbfile));
    printf("    DBFileVersionGE(4,6,0) returns %d\n", DBFileVersionGE(dbfile,4,6,0));
    printf("    DBFileVersionGE(90,5,2) returns %d\n", DBFileVersionGE(dbfile,90,5,2));
    DBClose(dbfile);


if (getcwd(cwd, sizeof(cwd)))
    printf("dbversion cwd = \"%s\"\n", cwd);
else
    perror("getcwd");

    /* The second open attempt is to allow this test to run correctly
       under autotest. Autotest runs its tests two levels down. */
    dbfile = DBOpen("pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("../../pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("./bin/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("./all_tests/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("/a/Silo/Silo/build/all_tests/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("D:/a/Silo/Silo/build/all_tests/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("/a/Silo/Silo/tests/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("D:/a/Silo/Silo/tests/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile)
    {
        printf("On old silo file handle returned from DBOpen...\n");
        printf("    DBFileVersion() returns \"%s\"\n", DBFileVersion(dbfile));
        printf("    DBFileVersionGE(3,0,0) returns %d\n", DBFileVersionGE(dbfile,3,0,0));
        printf("    DBFileVersionGE(4,6,0) returns %d\n", DBFileVersionGE(dbfile,4,6,0));
        printf("    DBFileVersionGE(90,5,2) returns %d\n", DBFileVersionGE(dbfile,90,5,2));
        DBClose(dbfile);
    }

    /* Test scanning through a set of version numbers on both old and
       new files to ensure comparisons work as expected */
    for (pass = 0; pass < 2; pass++)
    {
        struct {int maj; int min; int pat; int result_old; int result_new;}
            vnos_to_sample[] =
            {
                { 3, 0, 0, -1, 1},
                { 3, 0, 1, -1, 1},
                { 3, 1, 0, -1, 1},
                { 3, 1, 1, -1, 1},
                { 4, 5, 0, -1, 1},
                { 4, 5, 1,  0, 1},
                { 4, 5, 2,  0, 1},
                {90, 0, 0,  0, 0},
                {90, 0, 1,  0, 0},
                {90, 1, 0,  0, 0},
                {90, 1, 1,  0, 0}
            };

        /* Get a file handle for this pass */
        if (pass == 0)
        {
            /* The second open attempt is to allow this test to run correctly
               under autotest. Autotest runs its tests two levels down. */
            dbfile = DBOpen("pion0244.silo", DB_UNKNOWN, DB_READ);
            if (dbfile == 0)
                dbfile = DBOpen("../../pion0244.silo", DB_UNKNOWN, DB_READ);
            if (dbfile == 0)
                dbfile = DBOpen("./bin/pion0244.silo", DB_UNKNOWN, DB_READ);
            if (dbfile == 0)
                dbfile = DBOpen("./all_tests/pion0244.silo", DB_UNKNOWN, DB_READ);
            if (dbfile == 0)
                dbfile = DBOpen("/a/Silo/Silo/build/all_tests/pion0244.silo", DB_UNKNOWN, DB_READ);
            if (dbfile == 0)
                dbfile = DBOpen("D:/a/Silo/Silo/build/all_tests/pion0244.silo", DB_UNKNOWN, DB_READ);
            if (dbfile == 0)
                dbfile = DBOpen("/a/Silo/Silo/tests/pion0244.silo", DB_UNKNOWN, DB_READ);
            if (dbfile == 0)
                dbfile = DBOpen("D:/a/Silo/Silo/tests/pion0244.silo", DB_UNKNOWN, DB_READ);
        }
        else
        {
            dbfile = DBOpen(filename, driver, DB_READ);
        }

        if (dbfile == 0)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }
        
        /* loop over all version number tests */
        for (i = 0; i < sizeof(vnos_to_sample)/sizeof(vnos_to_sample[0]); i++)
        {
            int ck = DBFileVersionGE(dbfile, vnos_to_sample[i].maj, 
                                             vnos_to_sample[i].min,
                                             vnos_to_sample[i].pat);
            int expected = pass==0 ? vnos_to_sample[i].result_old : vnos_to_sample[i].result_new;

            if (ck != expected)
            {
                fprintf(stderr, "Version check failed for file version \"%s\" against %d.%d.%d, expected = %d, result = %d\n",
                    DBFileVersion(dbfile), vnos_to_sample[i].maj, vnos_to_sample[i].min, vnos_to_sample[i].pat, expected, ck);
                return 1;
            }
        }

        DBClose(dbfile);
    }

    /* Test getting version digits from file handle */
    dbfile = DBOpen(filename, driver, DB_READ);
    if (dbfile == 0)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }
    if (DBFileVersionDigits(dbfile, &vnos[0], &vnos[1], &vnos[2], &vnos[3]))
    {
        fprintf(stderr, "Error getting file version digits\n");
        return 1;
    }
    if (vnos[0] != SILO_VERS_MAJ || vnos[1] != SILO_VERS_MIN || vnos[2] != SILO_VERS_PAT)
    {
        fprintf(stderr, "Error in version digits returned from DBFileVersionDigits\n");
        return 1;
    }
    DBClose(dbfile);

    /* Test getting version digits from library */
    if (DBVersionDigits(&vnos[0], &vnos[1], &vnos[2], 0))
    {
        fprintf(stderr, "Error getting library version digits\n");
        return 1;
    }
    if (vnos[0] != SILO_VERS_MAJ || vnos[1] != SILO_VERS_MIN || vnos[2] != SILO_VERS_PAT)
    {
        fprintf(stderr, "Error in version digits returned from DBVersionDigits\n");
        return 1;
    }

    dbfile = DBOpen("pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("../../pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("./bin/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("./all_tests/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("/a/Silo/Silo/build/all_tests/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("D:/a/Silo/Silo/build/all_tests/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("/a/Silo/Silo/tests/pion0244.silo", DB_UNKNOWN, DB_READ);
    if (dbfile == 0)
        dbfile = DBOpen("D:/a/Silo/Silo/tests/pion0244.silo", DB_UNKNOWN, DB_READ);

    if (dbfile == 0)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }
    if (!DBVersionGEFileVersion(dbfile))
    {
        fprintf(stderr, "Error comparing library version to OLD file version.\n");
        return 1;
    }
    DBClose(dbfile);

    dbfile = DBOpen(filename, driver, DB_APPEND);
    if (dbfile == 0)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }
    if (!DBVersionGEFileVersion(dbfile))
    {
        fprintf(stderr, "Error comparing library version to CURRENT file version.\n");
        return 1;
    }

    /* Ok, now overwrite version info in the file to a version number way in the
       future and test again */
    DBSetAllowOverwrites(1);
    i = 5;
    DBWrite(dbfile, SILO_VSTRING_NAME, voverwrite, &i, 1, DB_CHAR);
    DBClose(dbfile);
    DBSetAllowOverwrites(0);

    dbfile = DBOpen(filename, driver, DB_READ);
    if (dbfile == 0)
    {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }
    if (DBVersionGEFileVersion(dbfile))
    {
        fprintf(stderr, "Error comparing library version to FUTURE file version.\n");
        return 1;
    }
    DBClose(dbfile);

    CleanupDriverStuff();
    return 0;
}
