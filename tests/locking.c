/*
Copyright (C) 1994-2026 Lawrence Livermore National Security, LLC.
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

#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "silo.h"

int main(int argc, char **argv)
{
    int i;
    int const create_phase = 1, open_phase = 2;
    int phase = create_phase;
    int status;
    char *new_argv[] = {"open-phase", NULL};
    pid_t pid;
    DBfile *db;

    for (i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "open-phase"))
            phase = open_phase;
    }

    if (phase == open_phase)
    {
        /* try opening a file already opened by another process in previous phase */
        DBShowErrors(DB_ALL_AND_DRVR,NULL);
        db = DBOpen("locktest.silo", DB_HDF5, DB_APPEND);
        if (!db) return 5;
        DBClose(db);
        return 0;
    }

    /* if we're here, we in the create phase*/
    db = DBCreate("locktest.silo", DB_CLOBBER, DB_LOCAL, "file locking test", DB_HDF5);
    DBFlush(db);

    pid = fork();
    if (pid == -1) return errno;

    if (pid == 0)
    {
        /* child is an execv of this same executable with a new arg list */
        execv(argv[0], new_argv);
        return errno; /* should never hit */
    }

    waitpid(pid, &status, 0);

    DBClose(db);

    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == 5)
        {
            printf("second DBOpen failed: HDF5 locking appears enabled\n");
            return 1;
        }
        if (WEXITSTATUS(status) == 0)
        {
            printf("second DBOpen succeeded: HDF5 locking appears disabled\n");
            return 0;
        }
    }

    printf("There was some kind of failure testing file locking is disabled\n");
    return 1;
}
