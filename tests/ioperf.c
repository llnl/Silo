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
#include <dlfcn.h>
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <unistd.h>

#include <ioperf.h>

#ifdef PARALLEL
#include <mpi.h>
#endif

/*-------------------------------------------------------------------------
  Function: bjhash 

  Purpose: Hash a variable length stream of bytes into a 32-bit value.

  Programmer: By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.

  You may use this code any way you wish, private, educational, or
  commercial.  It's free. However, do NOT use for cryptographic purposes.

  See http://burtleburtle.net/bob/hash/evahash.html
 *-------------------------------------------------------------------------*/

#define bjhash_mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

static unsigned int bjhash(register const unsigned char *k, register unsigned int length, register unsigned int initval)
{
   register unsigned int a,b,c,len;

   len = length;
   a = b = 0x9e3779b9;
   c = initval;

   while (len >= 12)
   {
      a += (k[0] +((unsigned int)k[1]<<8) +((unsigned int)k[2]<<16) +((unsigned int)k[3]<<24));
      b += (k[4] +((unsigned int)k[5]<<8) +((unsigned int)k[6]<<16) +((unsigned int)k[7]<<24));
      c += (k[8] +((unsigned int)k[9]<<8) +((unsigned int)k[10]<<16)+((unsigned int)k[11]<<24));
      bjhash_mix(a,b,c);
      k += 12; len -= 12;
   }

   c += length;

   switch(len)
   {
      case 11: c+=((unsigned int)k[10]<<24);
      case 10: c+=((unsigned int)k[9]<<16);
      case 9 : c+=((unsigned int)k[8]<<8);
      case 8 : b+=((unsigned int)k[7]<<24);
      case 7 : b+=((unsigned int)k[6]<<16);
      case 6 : b+=((unsigned int)k[5]<<8);
      case 5 : b+=k[4];
      case 4 : a+=((unsigned int)k[3]<<24);
      case 3 : a+=((unsigned int)k[2]<<16);
      case 2 : a+=((unsigned int)k[1]<<8);
      case 1 : a+=k[0];
   }

   bjhash_mix(a,b,c);

   return c;
}

typedef struct _timeinfo_t
{
    ioop_t op;
    size_t size;
    double t0; /* time started */
    double t1; /* time completed */
} timeinfo_t;

static int GetSizeFromModifierChar(char c)
{
    int n=1;
    switch (c)
    {
        case 'b': case 'B': n=1; break;
        case 'k': case 'K': n=1024; break;
        case 'm': case 'M': n=1024*1024; break;
        case 'g': case 'G': n=1024*1024*1024; break;
        default:  n=1; break;
    }
    return n;
}

static char* GetUniqueString(const options_t *opts)
{
    struct timeval tv0;
    unsigned int hval;
    long hid, rnum;
    char rstate[128];
    static char retval[128];

    if (!opts->no_mpi)
    {
        sprintf(retval, "%08d", opts->mpi_rank);
	return retval;
    }

    /* ok, try to build a random string */
#ifndef _WIN32
    gettimeofday(&tv0, 0);
    hval = bjhash((unsigned char *) &tv0, sizeof(tv0), 0);
#endif

    hid = gethostid();
    hval = bjhash((unsigned char *) &hid, sizeof(hid), hval);

    initstate(hval, rstate, 128);
    rnum = random();
    hval = bjhash((unsigned char *) &rnum, sizeof(rnum), hval);

    sprintf(retval, "%08d", hval);
    return retval;
}


/*
 * This is a default timer, with microsecond accuracy or thereabouts.
 * IO Interface plugins can override this default timer if they wish.
 * First call initializes time zero. Succeeding calls return time since
 * first initialized.
 */
static double GetTime()
{
    static double t0 = -1;
    double t1;
    struct timeval tv1;

    if (t0<0)
    {
        struct timeval tv0;
#ifndef _WIN32
        gettimeofday(&tv0, 0);
        t0 = (double)tv0.tv_sec*1e+6+(double)tv0.tv_usec;
#endif
        return 0;
    }

#ifndef _WIN32
    gettimeofday(&tv1, 0);
    t1 = (double)tv1.tv_sec*1e+6+(double)tv1.tv_usec;
#endif

    return t1-t0;
}

static int ProcessCommandLine(int argc, char *argv[], options_t *opts)
{
    char plugin_opts_delim[256];
    int i,n,nerrors=0;
    for (i=1; i<argc; i++)
    {
        errno=0;
        if (!strcmp(argv[i], "--io-interface"))
        {
            i++;
            opts->io_interface = strdup(argv[i]);
            sprintf(plugin_opts_delim, "--%s-args", opts->io_interface);
            if (errno) goto fail;
        }
        else if (!strcmp(argv[i], "--request-size"))
        {
            i++;
            n=strlen(argv[i])-1;
            n=GetSizeFromModifierChar(argv[i][n]);
            opts->request_size_in_bytes = strtol(argv[i], (char **)NULL, 10)*n;
            if (errno) goto fail;
        }
        else if (!strcmp(argv[i], "--num-requests"))
        {
            i++;
            opts->num_requests = strtol(argv[i], (char **)NULL, 10);
            if (errno) goto fail;
        }
        else if (!strcmp(argv[i], "--seek-noise"))
        {
            i++;
            opts->seek_noise = strtol(argv[i], (char **)NULL, 10);
            if (errno) goto fail;
        }
        else if (!strcmp(argv[i], "--size-noise"))
        {
            i++;
            opts->size_noise = strtol(argv[i], (char **)NULL, 10);
            if (errno) goto fail;
        }
        else if (!strcmp(argv[i], "--initial-size"))
        {
            i++;
            n=strlen(argv[i])-1;
            n=GetSizeFromModifierChar(argv[i][n]);
            opts->initial_file_size = strtol(argv[i], (char **)NULL, 10)*n;
            if (errno) goto fail;
        }
        else if (!strcmp(argv[i], "--alignment"))
        {
            i++;
            n=strlen(argv[i])-1;
            n=GetSizeFromModifierChar(argv[i][n]);
            opts->alignment = strtol(argv[i], (char **)NULL, 10)*n;
            if (errno) goto fail;
        }
        else if (!strcmp(argv[i], "--rand-file-name"))
        {
            opts->rand_file_name = 1;
        }
        else if (!strcmp(argv[i], "--no-mpi"))
        {
            opts->no_mpi = 1;
        }
        else if (!strcmp(argv[i], "--test-read"))
        {
            opts->flags = IO_READ;
        }
        else if (!strcmp(argv[i], "--print-details"))
        {
            opts->print_details = 1;
        }
        else if (!strcmp(argv[i], plugin_opts_delim))
        {
            break;
        }
	else if (argv[i][0] != '\0')
        {
fail:
	    fprintf(stderr, "%s: bad argument `%s' (\"%s\")\n", argv[0], argv[i],
                errno?strerror(errno):"");
            exit(EXIT_SUCCESS);
	}
    }

    /* sanity check some values */
    if (opts->io_interface == 0)
        fprintf(stderr, "%d: no io-interface specified\n", nerrors++);
    if (nerrors)
        exit(nerrors);

    return i+1;
}

#ifdef STATIC_PLUGINS
extern iointerface_t* CreateInterface_silo(int argi, int argc, char *argv[],
    const char *filename, const options_t *opts);
extern iointerface_t* CreateInterface_hdf5(int argi, int argc, char *argv[],
    const char *filename, const options_t *opts);
extern iointerface_t* CreateInterface_stdio(int argi, int argc, char *argv[],
    const char *filename, const options_t *opts);
extern iointerface_t* CreateInterface_sec2(int argi, int argc, char *argv[],
    const char *filename, const options_t *opts);
extern iointerface_t* CreateInterface_pdb(int argi, int argc, char *argv[],
    const char *filename, const options_t *opts);
#endif

static iointerface_t* GetIOInterface(int argi, int argc, char *argv[], const options_t *opts)
{
    char testfilename[256];
    char ifacename[256];
    void *dlhandle=0;
    iointerface_t *retval=0;

    /* First, get rid of the old data file */
    strcpy(ifacename, opts->io_interface);
    sprintf(testfilename, "iop_test_%s%s.dat", ifacename, 
        opts->rand_file_name?GetUniqueString(opts):"");
    unlink(testfilename);

    /* First, attempt to create interface using static approach, if that
       is enabled. */
#ifdef STATIC_PLUGINS
    if (!strcmp(ifacename, "silo"))
        retval = CreateInterface_silo(argi, argc, argv, testfilename, opts);
    else if (!strcmp(ifacename, "hdf5"))
        retval = CreateInterface_hdf5(argi, argc, argv, testfilename, opts);
    else if (!strcmp(ifacename, "stdio"))
        retval = CreateInterface_stdio(argi, argc, argv, testfilename, opts);
    else if (!strcmp(ifacename, "sec2"))
        retval = CreateInterface_sec2(argi, argc, argv, testfilename, opts);
    else if (!strcmp(ifacename, "pdb"))
        retval = CreateInterface_pdb(argi, argc, argv, testfilename, opts);
#else
    /* Fall back to dynamic approach */
    if (!retval)
    {
        int d, foundIt = 0;
        char *dirs[] = {".", "../..", ".libs", "../../.libs"};
        for (d = 0; d < sizeof(dirs)/sizeof(dirs[0]) && !foundIt; d++)
        {
            char libfilename[256];
            CreateInterfaceFunc createFunc;
            sprintf(libfilename, "%s/ioperf_%s.so", dirs[d], ifacename);
            dlhandle = dlopen(libfilename, RTLD_LAZY);
            if (!dlhandle) continue;

            createFunc = (CreateInterfaceFunc) dlsym(dlhandle, "CreateInterface");
            if (!createFunc) continue;

            /* we allow the io-interface plugin to process command line args too */
            printf("Using plugin file \"%s\n\n", libfilename);
            retval = createFunc(argi, argc, argv, testfilename, opts);
        }
    }
#endif

    if (!retval)
    {
        fprintf(stderr,"Encountered error instantiating IO interface\n");
        exit(EXIT_SUCCESS);
    }

    /* store off the handle to this plugin so we can close it later */
    retval->dlhandle = dlhandle;

    /* slip in the default time function */
    if (!retval->Time)
        retval->Time = GetTime;

    return retval;
}

static void AddTimingInfo(ioop_t op, size_t size, double t0, double t1)
{
    static timeinfo_t *tinfo=0;
    static int i=0;
    static int max=100;

    if (op == OP_OUTPUT_TIMINGS || op == OP_OUTPUT_SUMMARY)
    {
        int j;
        const char *opnms[] = {"WRITE", "READ", "OPEN", "CLOSE", "SEEK", "ERROR"};
        double tottime=0, totwrtime=0, totrdtime=0, toterrtime=0;
        size_t totwrbytes=0, totrdbytes=0;
        double wrfastest=0, wrslowest=DBL_MAX, rdfastest=0, rdslowest=DBL_MAX;
        int wrfastesti=-1, wrslowesti=-1, rdfastesti=-1, rdslowesti=-1;
        double wravg, rdavg;

        if (op == OP_OUTPUT_TIMINGS)
            fprintf(stdout, "i\top\tt0\t\tt1\t\tsize\n");
        for (j=0; j<i; j++)
        {
            if (op == OP_OUTPUT_TIMINGS)
            {
                fprintf(stdout, "%d\t%s\t%f\t%f\t%zd\n",
                    j,opnms[tinfo[j].op],tinfo[j].t0,tinfo[j].t1,tinfo[j].size);
            }

            if (tinfo[j].op != OP_ERROR)
            {
                tottime += (tinfo[j].t1-tinfo[j].t0);
            }
            else
            {
                toterrtime += (tinfo[j].t1-tinfo[j].t0);
            }

            if (tinfo[j].op == OP_WRITE)
            {
                double wrtime = tinfo[j].t1-tinfo[j].t0;
                double wrspeed = wrtime?(tinfo[j].size/wrtime):0;
                totwrtime += wrtime;
                totwrbytes += tinfo[j].size;
                if (wrspeed > wrfastest)
                {
                    wrfastest = wrspeed;
                    wrfastesti = j;
                }
                if (wrspeed < wrslowest)
                {
                    wrslowest = wrspeed;
                    wrslowesti = j;
                }
            }
            else if (tinfo[j].op == OP_READ)
            {
                double rdtime = tinfo[j].t1-tinfo[j].t0;
                double rdspeed = rdtime?(tinfo[j].size/rdtime):0;
                totrdtime += rdtime;
                totrdbytes += tinfo[j].size;
                if (rdspeed > rdfastest)
                {
                    rdfastest = rdspeed;
                    rdfastesti = j;
                }
                if (rdspeed < rdslowest)
                {
                    rdslowest = rdspeed;
                    rdslowesti = j;
                }
            }
        }

        fprintf(stdout, "=============================================================\n");
        fprintf(stdout, "==========================Summary============================\n");
        fprintf(stdout, "=============================================================\n");
        if (totwrbytes)
        {
            fprintf(stdout, "**************************Writes****************************\n");
            fprintf(stdout, "Total:   %zd bytes in %f seconds = %f Mb/s\n",
                totwrbytes, tottime*1e-6, totwrbytes/(tottime*1e-6)/(1<<20));
            fprintf(stdout, "Average: %zd bytes in %f seconds = %f Mb/s\n",
                totwrbytes, totwrtime*1e-6, totwrbytes/(totwrtime*1e-6)/(1<<20));
            if (wrfastesti>=0)
                fprintf(stdout, "Fastest: %zd bytes in %f seconds = %f Mb/s (iter=%d)\n",
                tinfo[wrfastesti].size, (tinfo[wrfastesti].t1-tinfo[wrfastesti].t0)*1e-6,
                wrfastest*1e+6/(1<<20), wrfastesti);
            if (wrslowesti>=0)
                fprintf(stdout, "Slowest: %zd bytes in %f seconds = %f Mb/s (iter=%d)\n",
                tinfo[wrslowesti].size, (tinfo[wrslowesti].t1-tinfo[wrslowesti].t0)*1e-6,
                wrslowest*1e+6/(1<<20), wrslowesti);
        }
        if (totrdbytes)
        {
            fprintf(stdout, "**************************Reads*****************************\n");
            fprintf(stdout, "Total:   %zd bytes in %f seconds = %f Mb/s\n",
                totrdbytes, tottime*1e-6, totrdbytes/(tottime*1e-6)/(1<<20));
            fprintf(stdout, "Average: %zd bytes in %f seconds = %f Mb/s\n",
                totrdbytes, totrdtime*1e-6, totrdbytes/(totrdtime*1e-6)/(1<<20));
            if (rdfastesti>=0)
                fprintf(stdout, "Fastest: %zd bytes in %f seconds = %f Mb/s (iter=%d)\n",
                tinfo[rdfastesti].size, (tinfo[rdfastesti].t1-tinfo[rdfastesti].t0)*1e-6,
                rdfastest*1e+6/(1<<20),rdfastesti);
            if (rdslowesti>=0)
                fprintf(stdout, "Slowest: %zd bytes in %f seconds = %f Mb/s (iter=%d)\n",
                tinfo[rdslowesti].size, (tinfo[rdslowesti].t1-tinfo[rdslowesti].t0)*1e-6,
                rdslowest*1e+6/(1<<20), rdslowesti);
        } 

        return;
    }

    if (tinfo==0 || i==max-1)
    {
        max = max*1.5;
        tinfo = (timeinfo_t*) realloc(tinfo, max*sizeof(timeinfo_t));
    }

    tinfo[i].op = op;
    tinfo[i].t0 = t0;
    tinfo[i].t1 = t1;
    tinfo[i].size = size;
    i++;
}

static void TestWrites(iointerface_t *ioiface, const options_t *opts)
{
    int i,n;
    double t0,t1;
    double *buf;
    int num_doubles;

    /* allocate and initialize a buffer of data to write */
    num_doubles = opts->request_size_in_bytes / sizeof(double);
    buf = (double*) calloc(opts->request_size_in_bytes,1);
    for (i=0; i<num_doubles;i++)
        buf[i] = i;

    for (i=0; i<opts->num_requests; i++)
    {
        /* Add some request noise */
        if (opts->size_noise && i && (i%(opts->size_noise))==0)
        {
            t0 = ioiface->Time();
            n = ioiface->Write(buf, 8);
            t1 = ioiface->Time();
            AddTimingInfo(n==8?OP_WRITE:OP_ERROR, n, t0, t1);
        }

        /* Ok, do a write of prescribed size */
        t0 = ioiface->Time();
        n = ioiface->Write(buf, opts->request_size_in_bytes);
        t1 = ioiface->Time();
        AddTimingInfo(n==opts->request_size_in_bytes?OP_WRITE:OP_ERROR, n, t0, t1);
    }
}

int
main(int argc, char *argv[])
{
    options_t      options;
    iointerface_t  *ioiface;
    double         t0,t1;
    int            plugin_argi;

    /* setup default options */
    memset(&options, 0, sizeof(options));
    options.request_size_in_bytes = 4096;
    options.num_requests = 100;
    options.flags = IO_WRITE|IO_TRUNCATE;

    plugin_argi = ProcessCommandLine(argc, argv, &options);

#ifdef PARALLEL
    if (!options.no_mpi)
    {
        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &options.mpi_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &options.mpi_rank);
    }
#endif

    /* GetIOInterface either exits or returns valid pointer */
    ioiface = GetIOInterface(plugin_argi, argc, argv, &options);

    /* First call initializes timer */
    t0 = ioiface->Time();

    /* open the file */
    if (!ioiface->Open(0))
    {
        fprintf(stderr, "Problem opening file\n");
        exit(EXIT_SUCCESS);
    }
    t1 = ioiface->Time();
    AddTimingInfo(OP_OPEN, 0, t0, t1);

    if (options.flags&IO_WRITE)
        TestWrites(ioiface, &options);

    /* close the file */
    t0 = ioiface->Time();
    if (!ioiface->Close())
    {
        fprintf(stderr, "Problem closing file\n");
    }
    t1 = ioiface->Time();
    AddTimingInfo(OP_CLOSE, 0, t0, t1);

/*
    if (options.flags&IO_READ)
        TestReads(ioiface, &options);
*/
        

#ifndef STATIC_PLUGINS
    /* close the interface */
    if (ioiface->dlhandle)
        dlclose(ioiface->dlhandle);
#endif
    
    /* output timing info */
    if (options.print_details)
        AddTimingInfo(OP_OUTPUT_TIMINGS, 0, 0, 0);
    else
        AddTimingInfo(OP_OUTPUT_SUMMARY, 0, 0, 0);

#ifdef PARALLEL
    if (!options.no_mpi)
        MPI_Finalize();
#endif

    return (0);
}
