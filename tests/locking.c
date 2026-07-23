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

#ifdef _WIN32
#include <process.h>
#include <stdint.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>

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
        if (!db)
        {
            if (DBErrno() == E_FILELOCKING) return 5;
            return 6;
        }
        DBClose(db);
        return 0;
    }

    /* if we're here, we're in the create phase */
    db = DBCreate("locktest.silo", DB_CLOBBER, DB_LOCAL, "file locking test", DB_HDF5);
    DBFlush(db);

#ifdef _WIN32
    {
        char const *new_argv[] = {
            argv[0],
            "open-phase",
            NULL
        };

        intptr_t result = _spawnv(_P_WAIT, argv[0], new_argv);

        if (result == -1)
            return errno;

        status = (int) result;
    }
#else
    {
        pid_t pid;
        int wait_status;

        pid = fork();

        if (pid == -1)
            return errno;

        if (pid == 0)
        {
            char *new_argv[] = {
                argv[0],
                "open-phase",
                NULL
            };

            execv(argv[0], new_argv);
            return errno;
        }

        waitpid(pid, &wait_status, 0);

        if (!WIFEXITED(wait_status))
            return 1;

        status = WEXITSTATUS(wait_status);
    }
#endif

    DBClose(db);

    if (status == 6)
    {
        printf("second DBOpen failed for some reason unrelated to locking\n");
        return 1;
    }
    if (status == 5)
    {
        printf("second DBOpen failed: HDF5 locking incorrectly appears enabled\n");
        return 1;
    }
    if (status == 0)
    {
        printf("second DBOpen succeeded: HDF5 locking correctly appears disabled\n");
        return 0;
    }

    printf("There was some kind of failure testing whether file locking is disabled\n");
    return 1;
}
