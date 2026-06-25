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

#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#if !defined(_WIN32)
#include <sys/time.h>
#include <unistd.h>
#else
#define random rand
#define srandom srand
#endif
#include <stdlib.h>

#define GNU_AUTOTEST_SKIP_CODE 77
#define ONE_MEG 1048576
#define ITERATE 50

#include <std.c>

/*-------------------------------------------------------------------------
 * Function:        main
 *
 * Purpose:        
 *
 * Return:        0
 *
 * Programmer:        
 *      Thomas R. Treadway, Mon Mar 12 14:13:51 PDT 2007
 *      Test of HDF5 compression.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
    
    int            nerrors = 0;
    int            verbose = 0;
    int            usefloat = 0;
    int            readonly = 0;
    int            i, j, ndims=1;
    int            fdims[]={ONE_MEG/sizeof(float)};
    int            ddims[]={ONE_MEG/sizeof(double)};
    float          *fval;
    float          *frval;
    double         *dval;
    double         *drval;
    int            driver=DB_HDF5;
    char          *filename="compression.h5";
    char          *ptr;
    char           tmpname[64];
    DBfile        *dbfile;
#if !defined(_WIN32)
    struct         timeval tim;
    double         t1, t2;
#endif
    struct stat    buffer;
    off_t          fsize;
    int            has_loss = 0;
    int            show_errors = DB_TOP;
    double         noise = 0.3;
    int            compat = DB_COMPAT_OVER_PERF;

    /* Parse command-line */
    for (i=1; i<argc; i++) {
       if (!strncmp(argv[i], "DB_PDB",6)) {
          fprintf(stderr, "This test only supported on HDF5 driver\n");
          exit(EXIT_SUCCESS);
       } else if (!strncmp(argv[i], "DB_HDF5", 7)) {
          driver = StringToDriver(argv[i]);
          filename = "compression.h5";
       } else if (!strcmp(argv[i], "compress")) {
          if ((i+1<argc) && ((ptr=strstr(argv[i+1], "METHOD=")) != NULL))
          {
            DBSetCompression(argv[i+1]);
            i++;
          }
          else
            DBSetCompression("METHOD=GZIP");
       } else if (!strcmp(argv[i], "szip")) {
          DBSetCompression("METHOD=SZIP");
       } else if (!strcmp(argv[i], "gzip")) {
          DBSetCompression("METHOD=GZIP");
       } else if (!strcmp(argv[i], "gzip9")) {
          DBSetCompression("METHOD=GZIP LEVEL=9");
       } else if (!strcmp(argv[i], "fpzip")) {
          DBSetCompression("METHOD=FPZIP");
       } else if (!strcmp(argv[i], "zfp")) {
          DBSetCompression("METHOD=ZFP RATE=8.5");
          has_loss = 1;
       } else if (!strcmp(argv[i], "single")) {
          usefloat = 1;
       } else if (!strcmp(argv[i], "verbose")) {
          verbose = 1;
       } else if (!strcmp(argv[i], "lossy1")) {
          DBSetCompression("METHOD=FPZIP LOSS=1");
          has_loss = 1;
       } else if (!strcmp(argv[i], "lossy2")) {
          DBSetCompression("METHOD=FPZIP LOSS=2");
          has_loss = 1;
       } else if (!strcmp(argv[i], "lossy3")) {
          DBSetCompression("METHOD=FPZIP LOSS=3");
          has_loss = 1;
       } else if (!strcmp(argv[i], "generic")) {
          DBSetCompression("ERRMODE=FAIL METHOD=HDF5_PLUGIN ID=1 NAME=deflate CDVALS=8");
       /* These test fail/fallback behavior. We target a compresion ratio
          of 1000:1 and FAIL or FALLBACK if we can't hit...which we can't. */
       } else if (!strcmp(argv[i], "minratio1000")) {
          DBSetCompression("ERRMODE=FAIL MINRATIO=1000 METHOD=FPZIP");
       } else if (!strcmp(argv[i], "minratio1001")) {
          DBSetCompression("ERRMODE=FALLBACK MINRATIO=1000 METHOD=FPZIP");
       } else if (!strcmp(argv[i], "readonly")) {
          readonly = 1;
       } else if (!strcmp(argv[i], "help")) {
          printf("Usage: %s [compress [\"METHOD=...\"]|single|verbose|readonly]\n",argv[0]);
          printf("Where: compress - enables compression, followed by compression information string\n");
          printf("                  default is compress \"METHOD=GZIP LEVEL=1\"\n");
          printf("       single   - writes data as floats not doubles\n");
          printf("       verbose  - displays more feedback\n");
          printf("       readonly - checks an existing file (used for cross platform test)\n");
          printf("       DB_HDF5  - enable HDF5 driver, the default\n");
          return (0);
       } else if (!strcmp(argv[i], "perf-over-compat")) {
           compat = DB_PERF_OVER_COMPAT;
       } else if (!strcmp(argv[i], "show-all-errors")) {
          show_errors = DB_ALL_AND_DRVR;
       } else if (argv[i][0] != '\0') {
          fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
       }
    }

    /* get some temporary memory */
    fval = (float*) malloc(ONE_MEG);
    frval = (float*) malloc(ONE_MEG);
    dval = (double*) malloc(ONE_MEG);
    drval = (double*) malloc(ONE_MEG);

    DBSetCompatibilityMode(compat);
    DBShowErrors(show_errors, 0);

    if (!readonly)
    {
      /*
       * Create a file that contains a simple variables.
       */
      if (verbose)
         printf("Creating file: `%s'\n", filename);
      dbfile = DBCreate(filename, 0, DB_LOCAL, "Compression Test", driver);

#if !defined(_WIN32)
      gettimeofday(&tim, NULL);
      t1=tim.tv_sec+(tim.tv_usec/1000000.0);
#endif
      srandom(0xDeadBeef);
      if (usefloat)
      {
         for (j = 0; j < ITERATE; j++)
         {
            if (verbose)
             if (j % 100 == 0)
               printf("Iterations %04d to %04d of %04d\n", j,j+100-1,ITERATE);

            sprintf(tmpname, "compression_%04d", j);

            for (i = 0; i < fdims[0]; i++)
            {
                double x = 2 * M_PI * (double) i / (double) (fdims[0]-1);
                double n = noise * ((double) random() / ((double)((long long)1<<31)-1) - 0.5);
                fval[i] = (float) ((j+1) * (1 + sin(x)) + n);
            }

            if (DBWrite(dbfile, tmpname, fval, fdims, ndims, DB_FLOAT) < 0)
            {
                if (DBErrno() == E_COMPRESSION)
                {
                    free(fval);
                    free(frval);
                    free(dval);
                    free(drval);
                    return GNU_AUTOTEST_SKIP_CODE; /* compression requested not supported in this build */
                }
                nerrors++;
                break;
            }
         }
      }
      else
      {
         for (j = 0; j < ITERATE; j++)
         {
            if (verbose)
               if (j % 100 == 0)
                 printf("Iterations %04d to %04d of %04d\n",j,j+100-1,ITERATE);

            sprintf(tmpname, "compression_%04d", j);

            for (i = 0; i < ddims[0]; i++)
            {
                double x = 2 * M_PI * (double) i / (double) (ddims[0]-1);
                double n = noise * ((double) random() / ((double)((long long)1<<31)-1) - 0.5);
                dval[i] = (double) ((j+1) * (1 + sin(x)) + n);
            }

            if (DBWrite(dbfile, tmpname, dval, ddims, ndims, DB_DOUBLE) < 0)
            {
                if (DBErrno() == E_COMPRESSION)
                {
                    free(fval);
                    free(frval);
                    free(dval);
                    free(drval);
                    return GNU_AUTOTEST_SKIP_CODE; /* compression requested not supported in this build */
                }
                nerrors++;
                break;
            }
         }
      }
#if !defined(_WIN32)
      gettimeofday(&tim, NULL);
      t2=tim.tv_sec+(tim.tv_usec/1000000.0);
      stat(filename, &buffer);
      fsize = buffer.st_size;
      printf("Write took %.6lf seconds and %.6g bytes/second\n", 
         t2-t1,fsize/(t2-t1));
#endif

      DBClose(dbfile);
    }
    else
    {
      stat(filename, &buffer);
      fsize = buffer.st_size;
    }

    if (nerrors)
    {
        free(fval);
        free(frval);
        free(dval);
        free(drval);
        return nerrors;
    }

    /*
     * Now try opening the file again and verify the simple
     * variable.
     */
    if (verbose)
        printf("Reopening `%s'\n", filename);
    dbfile = DBOpen(filename, driver, DB_READ);

    if (dbfile == 0)
    {
        printf("Unable to Open file for reading\n");
        exit(EXIT_SUCCESS);
    }

#if !defined(_WIN32)
    gettimeofday(&tim, NULL);
    t1=tim.tv_sec+(tim.tv_usec/1000000.0);
#endif
    srandom(0xDeadBeef);
    if (usefloat)
    {
       for (j = 0; j < ITERATE; j++)
       {
          if (verbose)
             if (j % 100 == 0)
                printf("Iterations %04d to %04d of %04d\n", j,j+100-1,ITERATE);

          sprintf(tmpname, "compression_%04d", j);

          if (DBReadVar(dbfile, tmpname, frval) < 0)
          {
             if (!has_loss) nerrors++;
             if (!has_loss && nerrors <= 10) printf("DBReadVar for \"%s\" failed\n", tmpname);
             if (!has_loss && nerrors == 10) printf("Further errors will be suppressed\n");
          }
          for (i = 0; i < fdims[0]; i++)
          {
             double x = 2 * M_PI * (double) i / (double) (fdims[0]-1);
             double n = noise * ((double) random() / ((double)((long long)1<<31)-1) - 0.5);
             fval[i] = (float) ((j+1) * (1 + sin(x)) + n);
             if (fval[i] != frval[i])
             {
                if (!strcmp(DBGetCompression(), "METHOD=ZFP RATE=8.5"))
                {
                    double rel_err = 0;
                    if (fval[i] != 0) rel_err = (fval[i] - frval[i]) / fval[i];
                    if (rel_err < 0) rel_err = -rel_err;
                    if (rel_err > 0.001 && nerrors <= 10)
                    {
                        printf("Read error above tolerance in \"%s\" at position %04d. "
                            "Expected %f, got %f, err = %f\n", tmpname, i, fval[i], frval[i], rel_err);
                        if (nerrors == 10) printf("Further errors will be suppressed\n");
                        nerrors++;
                    }
                }
                if (!has_loss) nerrors++;
                if (!has_loss && nerrors <= 10) 
                    printf("Read error in \"%s\" at position %04d. Expected %f, got %f\n",
                    tmpname, i, fval[i], frval[i]);
                if (!has_loss && nerrors == 10) printf("Further errors will be suppressed\n");
                break;
             }
          }
       }
    }
    else
    {
       for (j = 0; j < ITERATE; j++)
       {
          if (verbose)
             if (j % 100 == 0)
                printf("Iterations %04d to %04d of %04d\n",j,j+100-1,ITERATE);

          sprintf(tmpname, "compression_%04d", j);

          if (DBReadVar(dbfile, tmpname, drval) < 0)
          {
             if (!has_loss) nerrors++;
             if (!has_loss && nerrors <= 10) printf("DBReadVar for \"%s\" failed\n", tmpname);
             if (!has_loss && nerrors == 10) printf("Further errors will be suppressed\n");
          }
          for (i = 0; i < ddims[0]; i++)
          {
             double x = 2 * M_PI * (double) i / (double) (ddims[0]-1);
             double n = noise * ((double) random() / ((double)((long long)1<<31)-1) - 0.5);
             dval[i] = (double) ((j+1) * (1 + sin(x)) + n);
             if (dval[i] != drval[i])
             {
                if (!has_loss) nerrors++;
                if (!has_loss && nerrors <= 10) 
                    printf("Read error in \"%s\" at position %04d. Expected %f, got %f\n",
                    tmpname, i, dval[i], drval[i]);
                if (!has_loss && nerrors == 10) printf("Further errors will be suppressed\n");
                break;
             }
          }
       }
    }
#if !defined(_WIN32)
    gettimeofday(&tim, NULL);
    t2=tim.tv_sec+(tim.tv_usec/1000000.0);
    printf("Read took %.6lf seconds and %.6g bytes/second\n", 
       t2-t1,fsize/(t2-t1));
#endif
    DBClose(dbfile);

    free(fval);
    free(frval);
    free(dval);
    free(drval);

    DBSetCompression(0);
    CleanupDriverStuff();
    return nerrors;
}
