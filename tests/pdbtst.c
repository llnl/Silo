/*
Copyright (C) 1994-2016 Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory
Written by Stewart Brown (brown50@llnl.gov).
CODE-422942.
All rights reserved.

This file was adapted for PDB Lite from the 11_09_21 version of
PACT by Mark Miller (miller86@llnl.gov). For details on PACT, see
pact.llnl.gov

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

/*
Mark C. Miller, Wed Dec 19 20:06:37 PST 2012
This file was taken from the 11_09_21 version of PACT and adpated to
be used as a test of PDB Lite, part of the Silo library. To the
extent possible, the adaption here was to retain the original code 
and intentions of tests as much as possible and use the CPP to re-map
any methods from the original code to their equivalents in PDB Lite.
In cases where there were no equivalents, a few different strategies
were employed...

   * Existing functionality was disabled; this works fine for cosmetic
     things such as memory utilization and performance tracking as
     similar data can be obtained from tools like valgrind.
   * Customized alternative methods were coded here in this file.
   * The logic of the original test was altered slightly.
   * Some tests were completely disabled.

The only symbols with "lite_" pre-pending them are the numeric format
symbols that are unique to PDB Lite. Otherwise, any symbol names here
retain their original (PDB Proper) names becasue part of the intention
of this test is to test the exported lite_pdb.h and lite_score.h header
files. Those files include CPP macros to remap PDB Proper symbol names
to their PDB Lite equivalents.

In some tests, we leak memory for certain data ('ca' in test 2 and
'tab4_r' in test 4.

Note that in PDB Lite, by default, we compile it with a reduced SCORE
memory header to save memory useage for some libraries. Nonetheless,
this test will pass with these reduced SCORE headers. If you are
running into problems with this test, you can always re-configure to
--enable-normal-sclite-mem-headers and see if that fixes the problems.
*/
#ifdef PDB_LITE

#include <config.h>

#include <lite_pdb.h>
#include <config.h>

#include <assert.h>
#ifndef _WIN32
#include <dirent.h>
#endif
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#ifndef _WIN32
#include <sys/time.h>
#endif
#endif
#include <sys/types.h>
#ifndef _WIN32
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

/* Constant and macro definitions taken from PDB proper */
#define SC_HA_NAME_KEY "by-name"
#define N_PRIMITIVE_FP    2 /* float and double but not long double */
#define PD_MD5_FILE    1
#define TOLERANCE 1.0e-10
#define SMALL 1.0e-100
#define ABS fabs
#define min(a, b) ((a) < (b) ? (a) : (b))

/* Typename differences between PDB proper and PDB Lite */
#define hasharr HASHTAB
#define hasharrstr "hashtab"
#define haelem hashel
#define haelemstr "hashel"

/* Methods mapped to PDB Lite / SCORE Lite (or system) equivalents */
#define SC_hasharr_dump(A,B,C,D) SC_dump_hash(A,B,D)
#define SC_hasharr_get_n(A) (A->nelements)
#define SC_hasharr_def_lookup(A,B) SC_def_lookup(B,A)
#define SC_make_hasharr(A, B, C, D) SC_make_hash_table(A,B)
#define SC_hasharr_install(A, B, C, D, E, F) SC_install(B, C, D, A)
#define SC_free_hasharr(A, B, C) SC_rl_hash_table(A)
#define PD_target_n_platforms() NSTD
#define _PD_lookup_size _lite_PD_lookup_size
#define SC_ASSERT assert
#define SC_signal signal
#define SC_VA_START(A) va_list ap; va_start(ap, A);
#define SC_VSNPRINTF(A,B,C) vsnprintf(A,B,C,ap);
#define SC_VA_END va_end(ap);
#define SC_sleep sleep
#define STDOUT stdout
#define CSTRSAVE(A) SC_strsavef(A,foo_str())
#define CFREE SFREE
#ifndef _WIN32
#define REMOVE unlink
#define CHDIR  chdir
#define RMDIR  rmdir
#else
#define REMOVE _unlink
#define CHDIR  _chdir
#define RMDIR  _rmdir
#endif
#define POW pow
#ifdef PRINT
#undef PRINT
#define PRINT fprintf
#endif
#define CMAKE(A) FMAKE(A,foo_str())
#define CMAKE_N(A,B) FMAKE_N(A,B,foo_str())

/* Methods mapped to customized equivalents coded here in this file */
#define SC_free_strings free_strings
#define SC_wall_clock_time wall_clock_time
#define PD_fp_toler fp_toler
#define PD_def_hash_types(A,B) def_hash_types(A)
#define PD_target_platform target_platform
#define PD_target_platform_n target_platform_n
#define PD_target_platform_name target_platform_name

/* Methods completely disabled here */
#define SC_bf_set_hooks() /*void*/
#define SC_zero_space_n(A,B) /*void*/
#define SC_mem_stats(A,B,C,D) /*void*/
#define SC_mem_monitor(A,B,C,D) /*void*/0
#define PD_init_threads(A,B) /*void*/
#define PD_read_as_dwim(A,B,C,D,E) /*void*/1
#define PD_activate_cksum(A,B) /*void*/
#define PD_verify(A) /*void*/0
#define PD_verify_writes(A) /*void*/
#define PD_set_io_hooks(A) /*void*/
#define PD_open_vif(A) /*void*/
#define PD_set_fmt_version(A) /*void*/
#define PD_set_track_pointers(A,B) /*void*/
#undef PD_set_buffer_size
#define PD_set_buffer_size(A) /*void*/
#define SC_hasharr_rekey(A,B) /*void*/

static int last = 0; 

/* ensures the tag names used in SCORE allocations are unique */
static char *foo_str()
{
    static char retval[32];
    static int i = 0;
    sprintf(retval, "foo%d", i++);
    return retval;
}

static void free_strings(char **strs)
{
    int i = 0;
    if (!strs) return;
    SFREE(strs);
}

static double wall_clock_time()
{
#ifndef _WIN32
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (double) tv.tv_sec + (double) tv.tv_usec / 1e+6;
#else
    return 0;
#endif
}

static int target_platform_n(int i) {return 1;};

static char *target_platform_name(int i)
{
    switch (i)
    {
        case 1: return "IEEE_32_64";
        case 2: return "INTEL_X86";
        case 3: return "CRAY_64";
        case 4: return "VAX_11";
        case 6: return "IEEE_32_96";
        default: return "default";
    }
}

static int target_platform(const char *tgt)
{
    if (!strcmp(tgt, "IEEE_32_64"))
       return PD_target(&lite_IEEEA_STD, &lite_M68000_ALIGNMENT);
    else if (!strcmp(tgt, "IEEE_32_96"))
       return PD_target(&lite_IEEEB_STD, &lite_M68000_ALIGNMENT);
    else if (!strcmp(tgt, "INTEL_X86"))
       return PD_target(&lite_INTELA_STD, &lite_INTELA_ALIGNMENT);
    else if (!strcmp(tgt, "CRAY_64"))
       return PD_target(&lite_CRAY_STD, &lite_UNICOS_ALIGNMENT);
    else if (!strcmp(tgt, "VAX_11"))
       return PD_target(&lite_VAX_STD, &lite_DEF_ALIGNMENT);
    else
       return PD_target(&lite_DEF_STD, &lite_DEF_ALIGNMENT);
}

static void fp_toler(PDBfile *file, long double *fptol)
   {int i, fpmn[N_PRIMITIVE_FP];
    data_standard *fstd, *hstd;
    long *fformat, *hformat;

    fstd = file->std;
    hstd = file->host_std;

    for (i = 0; i < N_PRIMITIVE_FP; i++)
    {
        switch (i)
        {
            case 0:
                fformat = fstd->float_format;
                hformat = hstd->float_format;
                break;
            case 1:
                fformat = fstd->double_format;
                hformat = hstd->double_format;
                break;
        }
        
        {fpmn[i]  = min(fformat[2], hformat[2]);
         fptol[i] = powl(2.0, -((long double) fpmn[i]));};
    }

    return;}

/* taken directly from PDB in PACT December, 2012 */
static int PM_value_compare(double x1, double x2, double tol)
   {int rv;
    double dx;

    if (tol < 0.0)
       tol = TOLERANCE;

    dx = (x1 - x2)/(ABS(x1) + ABS(x2) + SMALL);
    if (dx < -tol)
       rv = -1;
    else if (tol < dx)
       rv = 1;
    else
       rv = 0;

    return(rv);}

static void def_hash_types(PDBfile *file)
{
    PD_defstr(file, haelemstr,
                    "char *name",
                    "char *type",
                    "char *def",
                    "hashel *next",
                    LAST);

    PD_defstr(file, hasharrstr,
                    "int size",
                    "int nelements",
                    "int docp",
                    "hashel **table",
                    LAST);
}


#else

#include <pdb.h>
#define hasharrstr "hasharr"
#define haelemstr "hashel"

#endif

#define DATDIR "pdbtst-data"
#define DATFILE "nat"

#define N_DOUBLE 3
#define N_INT    5
#define N_CHAR  10
#define N_FLOAT  4

#define FLOAT_EQUAL(d1, d2)  (PM_value_compare(d1, d2, fptol[0]) == 0)
#define DOUBLE_EQUAL(d1, d2) (PM_value_compare(d1, d2, fptol[1]) == 0)

struct s_l_frame
   {float x_min;
    float x_max;
    float y_min;
    float y_max;};

typedef struct s_l_frame l_frame;

struct s_plot
   {float x_axis[N_CHAR];
    float y_axis[N_CHAR];
    int npts;
    char *label;
    l_frame view;};

typedef struct s_plot plot;

struct s_lev2
   {char **s;
    int type;};

typedef struct s_lev2 lev2;

struct s_lev1
   {int *a;
    double *b;
    lev2 *c;};

typedef struct s_lev1 lev1;

struct s_st3
   {char a;
    short b;
    char c[2];
    int d;
    char e[3];
    float f;
    char g[4];
    double h;
    char i[5];
    char *j;
    char k[6];};

typedef struct s_st3 st3;

struct s_st4
   {short a;
    char  b;};

typedef struct s_st4 st4;

struct s_st61
   {int n;
    double a[10];};

typedef struct s_st61 st61;

struct s_st62
   {int n;
    double *a;};

typedef struct s_st62 st62;

typedef int (*PFTest)(char *base, char *tgt, int n);

extern long _PD_lookup_size(char *s, hasharr *tab);

static st61
 *d61_w = NULL;

static st62
 *d62_w = NULL;

static st62
 d71_w,
 d71_w_save,
 d72_w,
 d71_r,
 d72_r;

static st4
 *vr4_w,
 *vr4_r;

static st3
 vr1_w,
 vr1_r;

static lev1
 tar5_t[4],
 tar5_r[4],
 *tar_r,
 *tar_w;

static hasharr 
 *tab4_r,
 *tab4_w;

static char
 *CHAR_S,
 *SHORT_S,
 *INT_S,
 *LONG_S,
 *FLOAT_S,
 *DOUBLE_S,
 *HASHEL_S,
 cs_w,
 cs_r,
 ca_w[N_CHAR],
 ca_r[N_CHAR],
 *cap_w[N_DOUBLE],
 *cap_r[N_DOUBLE];

static short
 ss_w,
 ss_r,
 sa_w[N_INT],
 sa_r[N_INT];

static int
 debug_mode  = FALSE,
 native_only = FALSE,
 read_only   = FALSE,
 is_w,
 is_r,
 ia_w[N_INT],
 ia_r[N_INT],
 do_r,
 do_w,
 p_w[N_INT],
 p_r[N_INT],
 len;

static float
 d61_a[10],
 d62_a[10],
 d62_s[8],
 fs_w,
 fs_r,
 fs_app_w,
 fs_app_r,
 fs_p1_r,
 fs_p2_r,
 fs_p3_r,
 fa1_r[N_FLOAT],
 fa2_w[N_FLOAT][N_DOUBLE],
 fa2_r[N_FLOAT][N_DOUBLE],
 fa2_app_w[N_FLOAT][N_DOUBLE],
 fa2_app_r[N_FLOAT][N_DOUBLE];

static double
 ds_w,
 ds_r,
 da_w[N_FLOAT],
 da_r[N_FLOAT],
 *d8_w,
 *d8a_r,
 *d8b_r,
 *d8c_r,
 *d8d_r;

#ifdef HAVE_ANSI_FLOAT16

static long double
 qs_w,
 qs_r,
 qa_w[N_FLOAT],
 qa_r[N_FLOAT];

#endif

static plot
 graph_w,
 graph_r;

static l_frame
 view_w,
 view_r;

/* variables for partial and member reads in test #2 */

static int
 *ap1,
 *ap2,
 aa[4];

static double
 *bp1,
 *bp2,
 ba[4];

static lev2
 *cp1,
 *cp2,
 ca[4];

static char
 **sp1,
 **sp2,
 **sp3,
 **sp4,
 *tp1,
 *tp2,
 *tp3,
 *tp4,
 *tp5,
 *tp6,
 *tp7,
 *tp8,
 ta[8];

/*--------------------------------------------------------------------------*/

/*                         GENERAL PURPOSE ROUTINES                         */

/*--------------------------------------------------------------------------*/

/* TEST_TARGET - set up the target for the data file */

static void test_target(char *tgt, char *base, int n,
		        char *fname, char *datfile)
   {int rv;

    if (tgt != NULL)
       {rv = PD_target_platform(tgt);
	SC_ASSERT(rv == TRUE);

        sprintf(fname, "%s-%s.rs%d", base, tgt, n);
        sprintf(datfile, "%s-%s.db%d", base, tgt, n);}

    else
       {sprintf(fname, "%s-nat.rs%d", base, n);
        sprintf(datfile, "%s-nat.db%d", base, n);};

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* DUMP_TEST_SYMBOL_TABLE - dump the symbol table */

static void dump_test_symbol_table(FILE *fp, hasharr *tab, int n)
   {int i, ne;
    char **names;

    PRINT(fp, "\nTest %d Symbol table:\n", n);

    ne    = SC_hasharr_get_n(tab);
    names = SC_hasharr_dump(tab, NULL, NULL, FALSE);
    for (i = 0; i < ne; i++)
        PRINT(fp, "%s\n", names[i]);
    SC_free_strings(names);

    PRINT(fp, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* ERROR - get out on an error */

static void error(int n, FILE *fp, char *fmt, ...)
   {char t[MAXLINE];
        
    SC_VA_START(fmt);
    SC_VSNPRINTF(t, MAXLINE, fmt);
    SC_VA_END;

    io_printf(fp, "%s", t);

    exit(EXIT_SUCCESS);

    return;}

/*--------------------------------------------------------------------------*/

/*                            TEST #0 ROUTINES                              */

/*--------------------------------------------------------------------------*/

/* PRINT_TEST_0_DATA - print it out to STDOUT */

static void print_test_0_data(PDBfile *strm, FILE *fp)
   {int nbp;

/* print scalars */
    nbp = _PD_lookup_size("char", strm->chart);
    PRINT(fp, "size(char) = %d\n", nbp);

    nbp = _PD_lookup_size("short", strm->chart);
    PRINT(fp, "size(short) = %d\n", nbp);

    nbp = _PD_lookup_size("int", strm->chart);
    PRINT(fp, "size(int) = %d\n", nbp);

    nbp = _PD_lookup_size("long", strm->chart);
    PRINT(fp, "size(long) = %d\n", nbp);

    nbp = _PD_lookup_size("long_long", strm->chart);
    PRINT(fp, "size(long_long) = %d\n", nbp);

    nbp = _PD_lookup_size("float", strm->chart);
    PRINT(fp, "size(float) = %d\n", nbp);

    nbp = _PD_lookup_size("double", strm->chart);
    PRINT(fp, "size(double) = %d\n", nbp);

#ifdef HAVE_ANSI_FLOAT16
    nbp = _PD_lookup_size("long_double", strm->chart);
    PRINT(fp, "size(long double) = %d\n", nbp);
#endif

    nbp = _PD_lookup_size("*", strm->chart);
    PRINT(fp, "size(char *) = %d\n", nbp);

    PRINT(fp, "\n");

/* whole struct test */
    nbp = _PD_lookup_size("l_frame", strm->chart);
    PRINT(fp, "size(l_frame) = %d\n", nbp);

    nbp = _PD_lookup_size("plot", strm->chart);
    PRINT(fp, "size(plot) = %d\n", nbp);

    PRINT(fp, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_0_DATA - compare the test data */

static int compare_test_0_data(PDBfile *strm, FILE *fp)
   {int nbp, nbc, err, err_tot;

    err     = TRUE;
    err_tot = TRUE;

/* compare primitive types */
    nbp  = _PD_lookup_size("char", strm->host_chart);
    nbc  = sizeof(char);
    err &= (nbp == nbc);

    nbp  = _PD_lookup_size("short", strm->host_chart);
    nbc  = sizeof(short);
    err &= (nbp == nbc);

    nbp  = _PD_lookup_size("int", strm->host_chart);
    nbc  = sizeof(int);
    err &= (nbp == nbc);

    nbp  = _PD_lookup_size("long", strm->host_chart);
    nbc  = sizeof(long);
    err &= (nbp == nbc);

    nbp  = _PD_lookup_size("long_long", strm->host_chart);
    nbc  = sizeof(int64_t);
    err &= (nbp == nbc);

    nbp  = _PD_lookup_size("float", strm->host_chart);
    nbc  = sizeof(float);
    err &= (nbp == nbc);

    nbp  = _PD_lookup_size("double", strm->host_chart);
    nbc  = sizeof(double);
    err &= (nbp == nbc);

#ifdef HAVE_ANSI_FLOAT16
    nbp  = _PD_lookup_size("long_double", strm->host_chart);
    nbc  = sizeof(long double);
    err &= (nbp == nbc);
#endif

    nbp  = _PD_lookup_size("*", strm->host_chart);
    nbc  = sizeof(char *);
    err &= (nbp == nbc);

    if (err)
       PRINT(fp, "Primitive types compare\n");
    else
       PRINT(fp, "Primitive types differ\n");
    err_tot &= err;

/* compare structures */
    nbp  = _PD_lookup_size("l_frame", strm->host_chart);
    nbc  = sizeof(l_frame);
    err &= (nbp == nbc);

    nbp  = _PD_lookup_size("plot", strm->host_chart);
    nbc  = sizeof(plot);
    err &= (nbp == nbc);

    if (err)
       PRINT(fp, "Structs compare\n");
    else
       PRINT(fp, "Structs differ\n");
    err_tot &= err;

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_0 - test PDB calculation of type and struct sizes */

static int test_0(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file as asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

/* create the named file */
    strm = PD_create(datfile);
    if (strm == NULL)
       error(1, fp, "Test couldn't create file %s\r\n", datfile);

    PRINT(fp, "File %s created\n", datfile);

/* make a few defstructs */
    PD_defstr(strm, "l_frame",
              "float x_min",
              "float x_max",
              "float y_min",
              "float y_max",
              LAST);

    PD_defstr(strm, "plot",
              "float x_axis(10)",
              "float y_axis(10)",
              "integer npts", 
              "char * label",
              "l_frame view",
               LAST);

    {
         char *members[] = {"plot foo(3)", "int a(5)", "double b", "long long bar"};
         int nmemb = sizeof(members)/sizeof(members[0]);
         PD_defstr_alt(strm, "foobar", nmemb, members);
    }

/* compare the original data with that read in */
    err = compare_test_0_data(strm, fp);

/* print it out to STDOUT */
    print_test_0_data(strm, fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(1, fp, "Test couldn't close file %s\r\n", datfile);

    PRINT(fp, "File %s closed\n", datfile);

    io_close(fp);

    return(err);}

/*--------------------------------------------------------------------------*/

/*                            TEST #1 ROUTINES                              */

/*--------------------------------------------------------------------------*/

/* PREP_TEST_1_DATA - prepare the test data */

static void prep_test_1_data(void)
   {int i, k;

/* set scalars */
    cs_r    = 0;
    ss_r    = 0;
    is_r    = 0;
    fs_r    = 0.0;
    fs_p1_r = 0.0;
    fs_p2_r = 0.0;
    fs_p3_r = 0.0;
    ds_r    = 0.0;

    cs_w     = 'Q';
    ss_w     = -514;
    is_w     =  10;
    fs_w     =  3.14159;
    fs_app_w = -3.14159;
    ds_w     =  exp(1.0);

/* set char array */
    for (i = 0; i < N_CHAR; i++)
        {ca_w[i] = '\0';
         ca_r[i] = '\0';};

/* set short array */
    for (i = 0; i < N_INT; i++)
        {sa_w[i] = 2 - i;
         sa_r[i] = 0;};

/* set int array */
    for (i = 0; i < N_INT; i++)
        {ia_w[i] = i - 2;
         ia_r[i] = 0;};

/* set float array */
    for (i = 0; i < N_FLOAT; i++)
        for (k = 0; k < N_DOUBLE; k++)
            {fa2_w[i][k]     = POW((double) (i+1), (double) (k+1));
             fa2_app_w[i][k] = POW((double) (k+1), (double) (i+1));
             fa2_r[i][k]     = 0.0;
             fa2_app_r[i][k] = 0.0;};

/* set double array */
    for (i = 0; i < N_FLOAT; i++)
        {da_w[i] = POW(ds_w, (double) (i+1));
         da_r[i] = 0.0;};

/* set strings */
    strcpy(ca_w, "Hi there!");
    len = strlen(ca_w) + 1;

    cap_w[0] = CSTRSAVE("lev1");
    cap_w[1] = CSTRSAVE("lev2");
    cap_w[2] = CSTRSAVE("tar fu blat");
    cap_r[0] = NULL;
    cap_r[1] = NULL;
    cap_r[2] = NULL;

/* set structures */
    for (i = 0; i < N_CHAR; i++)
        {graph_w.x_axis[i] = ((float) i)/10.0;
         graph_w.y_axis[i] = 0.5 - graph_w.x_axis[i];
         graph_r.x_axis[i] = -1000.0;
         graph_r.y_axis[i] = -1000.0;};

    view_w.x_min = 0.1;
    view_w.x_max = 1.0;
    view_w.y_min = -0.5;
    view_w.y_max =  0.5;

    view_r.x_min = -1.e-10;
    view_r.x_max = -1.e-10;
    view_r.y_min = -1.e-10;
    view_r.y_max = -1.e-10;

    graph_w.npts  = N_CHAR;
    graph_w.label = CSTRSAVE("test graph");
    graph_w.view  = view_w;

    graph_r.npts  = 0;
    graph_r.label = NULL;

    graph_r.view.x_min = -1.e-10;
    graph_r.view.x_max = -1.e-10;
    graph_r.view.y_min = -1.e-10;
    graph_r.view.y_max = -1.e-10;

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* CLEANUP_TEST_1 - free all known test data memory */

static void cleanup_test_1(void)
   {

    CFREE(cap_w[0]);
    CFREE(cap_w[1]);
    CFREE(cap_w[2]);

    CFREE(cap_r[0]);
    CFREE(cap_r[1]);
    CFREE(cap_r[2]);

    CFREE(graph_w.label);
    CFREE(graph_r.label);

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* WRITE_TEST_1_DATA - write out the data into the PDB file */

static void write_test_1_data(PDBfile *strm)
   {long ind[6];

/* write scalars into the file */
    if (PD_write(strm, "cs", "char",    &cs_w) == 0)
       error(1, STDOUT, "CS WRITE FAILED - WRITE_TEST_1_DATA\n");
    if (PD_write(strm, "ss", "short",   &ss_w) == 0)
       error(1, STDOUT, "SS WRITE FAILED - WRITE_TEST_1_DATA\n");
    if (PD_write(strm, "is", "integer", &is_w) == 0)
       error(1, STDOUT, "IS WRITE FAILED - WRITE_TEST_1_DATA\n");
    if (PD_write(strm, "fs", "float",   &fs_w) == 0)
       error(1, STDOUT, "FS WRITE FAILED - WRITE_TEST_1_DATA\n");
    if (PD_write(strm, "ds", "double",  &ds_w) == 0)
       error(1, STDOUT, "DS WRITE FAILED - WRITE_TEST_1_DATA\n");

/* write primitive arrays into the file */
    ind[0] = 0L;
    ind[1] = len - 1;
    ind[2] = 1L;
    if (PD_write_alt(strm, "ca", "char", ca_w, 1, ind) == 0)
       error(1, STDOUT, "CA WRITE FAILED - WRITE_TEST_1_DATA\n");
    if (PD_write(strm, "sa(5)", "short", sa_w) == 0)
       error(1, STDOUT, "SA WRITE FAILED - WRITE_TEST_1_DATA\n");
    if (PD_write(strm, "ia(5)", "integer", ia_w) == 0)
       error(1, STDOUT, "IA WRITE FAILED - WRITE_TEST_1_DATA\n");

    ind[0] = 0L;
    ind[1] = N_FLOAT - 1;
    ind[2] = 1L;
    ind[3] = 0L;
    ind[4] = N_DOUBLE - 1;
    ind[5] = 1L;
    if (PD_write_alt(strm, "fa2", "float", fa2_w, 2, ind) == 0)
       error(1, STDOUT, "FA2 WRITE FAILED - WRITE_TEST_1_DATA\n");

    ind[0] = 0L;
    ind[1] = N_FLOAT - 1;
    ind[2] = 1L;
    if (PD_write_alt(strm, "da", "double", da_w, 1, ind) == 0)
       error(1, STDOUT, "DA WRITE FAILED - WRITE_TEST_1_DATA\n");

    ind[0] = 0L;
    ind[1] = N_DOUBLE - 1;
    ind[2] = 1L;
    if (PD_write_alt(strm, "cap", "char *", cap_w, 1, ind) == 0)
       error(1, STDOUT, "CAP WRITE FAILED - WRITE_TEST_1_DATA\n");

/* write structures into the file */
    if (PD_write(strm, "view", "l_frame", &view_w) == 0)
       error(1, STDOUT, "VIEW WRITE FAILED - WRITE_TEST_1_DATA\n");

    if (PD_write(strm, "graph", "plot", &graph_w) == 0)
       error(1, STDOUT, "GRAPH WRITE FAILED - WRITE_TEST_1_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* APPEND_TEST_1_DATA - append some test data to the file */

static void append_test_1_data(PDBfile *strm)
   {long ind[6];

    if (PD_write(strm, "fs_app", "float", &fs_app_w) == 0)
       error(1, STDOUT, "FS_APP WRITE FAILED - APPEND_TEST_1_DATA\n");

    ind[0] = 0L;
    ind[1] = N_FLOAT - 1;
    ind[2] = 1L;
    ind[3] = 0L;
    ind[4] = N_DOUBLE - 1;
    ind[5] = 1L;
    if (PD_write_alt(strm, "fa2_app", "float", fa2_app_w, 2, ind) == 0)
       error(1, STDOUT, "FA2_APP WRITE FAILED - APPEND_TEST_1_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* READ_TEST_1_DATA - read the test data from the file */

static int read_test_1_data(PDBfile *strm)
   {int err;
    long ind[6];

/* read the scalar data from the file */
    err = PD_read(strm, "cs", &cs_r);
    err = PD_read(strm, "ss", &ss_r);
    err = PD_read(strm, "is", &is_r);
    err = PD_read(strm, "fs", &fs_r);
    err = PD_read(strm, "ds", &ds_r);

/* read the primitive arrays from the file */
    err = PD_read(strm, "ca",  ca_r);
    err = PD_read(strm, "sa",  sa_r);
    err = PD_read(strm, "ia",  ia_r);
    err = PD_read(strm, "fa2", fa2_r);
    err = PD_read(strm, "da",  da_r);
    err = PD_read(strm, "cap", cap_r);

/* read the entire structures from the file */
    err = PD_read(strm, "view",  &view_r);
    err = PD_read(strm, "graph", &graph_r);

/* read the appended data from the file */
    err = PD_read(strm, "fs_app",  &fs_app_r);
    err = PD_read(strm, "fa2_app", fa2_app_r);

/* partial array read */
    ind[0] = 1;
    ind[1] = 2;
    ind[2] = 1;
    ind[3] = 0;
    ind[4] = 1;
    ind[5] = 1;
    err = PD_read_alt(strm, "fa2", fa1_r, ind);

/* struct member test */
    ind[0] = 8;
    ind[1] = 8;
    ind[2] = 1;
    err = PD_read_alt(strm, "graph.x_axis",     &fs_p1_r, ind);
    err = PD_read(    strm, "graph.view.x_max", &fs_p2_r);
    err = PD_read(    strm, "view.y_max",       &fs_p3_r);

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_TEST_1_DATA - print it out to STDOUT */

static void print_test_1_data(FILE *fp)
   {int i, k;

/* print scalars */
    PRINT(fp, "short scalar:        ss = %d\n", ss_r);
    PRINT(fp, "integer scalar:      is = %d\n", is_r);
    PRINT(fp, "float scalar:        fs = %14.6e\n", fs_r);
    PRINT(fp, "double scalar:       ds = %14.6e\n", ds_r);

    PRINT(fp, "\n");

/* print char array */
    PRINT(fp, "static char array:\n  ca = %s\n", ca_r);

/* print short array */
    PRINT(fp, "short array:\n");
    for (i = 0; i < N_INT; i++)
        PRINT(fp, "  sa[%d] = %d\n", i, sa_r[i]);

/* print int array */
    PRINT(fp, "integer array:\n");
    for (i = 0; i < N_INT; i++)
        PRINT(fp, "  ia[%d] = %d\n", i, ia_r[i]);

/* print float array */
    PRINT(fp, "float array:\n");
    for (i = 0; i < N_FLOAT; i++)
        for (k = 0; k < N_DOUBLE; k++)
            PRINT(fp, "  fa2[%d][%d] = %14.6e\n", i, k, fa2_r[i][k]);

/* print double array */
    PRINT(fp, "double array:\n");
    for (i = 0; i < N_FLOAT; i++)
        PRINT(fp, "  da[%d] = %14.6e\n", i, da_r[i]);

/* print character pointer array */
    PRINT(fp, "string array:\n");
    for (i = 0; i < N_DOUBLE; i++)
        PRINT(fp, "  cap[%d] = %s\n", i, cap_r[i]);

    PRINT(fp, "\n");

/* print appended scalars */
    PRINT(fp, "appended float scalar: fs_app = %14.6e\n", fs_app_r);

/* print float array */
    PRINT(fp, "appended float array:\n");
    for (i = 0; i < N_FLOAT; i++)
        for (k = 0; k < N_DOUBLE; k++)
            PRINT(fp, "  fa2_app[%d][%d] = %14.6e\n",
                    i, k, fa2_app_r[i][k]);

    PRINT(fp, "\n");

/* whole struct test */
    PRINT(fp, "struct view:\n");
    PRINT(fp, "  x-min = %14.6e\n", view_r.x_min);
    PRINT(fp, "  x-max = %14.6e\n", view_r.x_max);
    PRINT(fp, "  y-min = %14.6e\n", view_r.y_min);
    PRINT(fp, "  y-max = %14.6e\n", view_r.y_max);

    PRINT(fp, "\n");

    PRINT(fp, "struct graph:\n");
    PRINT(fp, "  #pts  = %d\n",     graph_r.npts);
    PRINT(fp, "  x-min = %14.6e\n", graph_r.view.x_min);
    PRINT(fp, "  x-max = %14.6e\n", graph_r.view.x_max);
    PRINT(fp, "  y-min = %14.6e\n", graph_r.view.y_min);
    PRINT(fp, "  y-max = %14.6e\n", graph_r.view.y_max);

    PRINT(fp, "      x values             y values\n");
    for (i = 0; i < N_CHAR; i++)
        PRINT(fp, "   %14.6e       %14.6e\n",
              graph_r.x_axis[i], graph_r.y_axis[i]);

    PRINT(fp, "\n");

/* partial read single elements */
    PRINT(fp, "\npartial read scalars:\n");
    PRINT(fp, "  graph.x_axis[8]  = %14.6e\n", fs_p1_r);
    PRINT(fp, "  graph.view.x_max = %14.6e\n", fs_p2_r);
    PRINT(fp, "  view.y_max       = %14.6e\n", fs_p3_r);

    PRINT(fp, "\n");

/* partial read arrays */
    PRINT(fp, "partial read float array:\n");
    for (i = 0; i < N_FLOAT; i++)
        PRINT(fp, "  fa2_p[%d] = %14.6e\n", i, fa1_r[i]);

    PRINT(fp, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_1_DATA - compare the test data */

static int compare_test_1_data(PDBfile *strm, FILE *fp)
   {int i, k, err, err_tot;
    long double fptol[N_PRIMITIVE_FP];

    PD_fp_toler(strm, fptol);

    err_tot = TRUE;

/* compare scalars */
    err  = TRUE;
    err &= (cs_r == cs_w);
    err &= (ss_r == ss_w);
    err &= (is_r == is_w);
    err &= FLOAT_EQUAL(fs_r, fs_w);
    err &= FLOAT_EQUAL(fs_app_r, fs_app_w);
    err &= DOUBLE_EQUAL(ds_r, ds_w);

    if (err)
       PRINT(fp, "Scalars compare\n");
    else
       PRINT(fp, "Scalars differ\n");
    err_tot &= err;

/* compare char array */
    err = TRUE;
    for (i = 0; i < N_CHAR; i++)
        err &= (ca_r[i] == ca_w[i]);

/* compare short array */
    for (i = 0; i < N_INT; i++)
        err &= (sa_r[i] == sa_w[i]);

/* compare int array */
    for (i = 0; i < N_INT; i++)
        err &= (ia_r[i] == ia_w[i]);

/* compare float array */
    for (i = 0; i < N_FLOAT; i++)
        for (k = 0; k < N_DOUBLE; k++)
            {err &= FLOAT_EQUAL(fa2_r[i][k], fa2_w[i][k]);
             err &= FLOAT_EQUAL(fa2_app_r[i][k], fa2_app_w[i][k]);}

/* compare double array */
    for (i = 0; i < N_FLOAT; i++)
        err &= DOUBLE_EQUAL(da_r[i], da_w[i]);

    if (err)
       PRINT(fp, "Arrays compare\n");
    else
       PRINT(fp, "Arrays differ\n");
    err_tot &= err;

/* compare strings */
    err = TRUE;
    for (i = 0; i < N_DOUBLE; i++)
        err &= (strcmp(cap_r[i], cap_w[i]) == 0);

    if (err)
       PRINT(fp, "Strings compare\n");
    else
       PRINT(fp, "Strings differ\n");
    err_tot &= err;

/* compare structures */
    err = TRUE;
    for (i = 0; i < N_CHAR; i++)
        {err &= FLOAT_EQUAL(graph_r.x_axis[i], graph_w.x_axis[i]);
         err &= FLOAT_EQUAL(graph_r.y_axis[i], graph_w.y_axis[i]);};

    err &= FLOAT_EQUAL(view_r.x_min, view_w.x_min);
    err &= FLOAT_EQUAL(view_r.x_max, view_w.x_max);
    err &= FLOAT_EQUAL(view_r.y_min, view_w.y_min);
    err &= FLOAT_EQUAL(view_r.y_max, view_w.y_max);

    err &= (graph_r.npts == graph_w.npts);
    err &= (strcmp(graph_r.label, graph_w.label) == 0);

    err &= FLOAT_EQUAL(graph_r.view.x_min, graph_w.view.x_min);
    err &= FLOAT_EQUAL(graph_r.view.x_max, graph_w.view.x_max);
    err &= FLOAT_EQUAL(graph_r.view.y_min, graph_w.view.y_min);
    err &= FLOAT_EQUAL(graph_r.view.y_max, graph_w.view.y_max);

    if (err)
       PRINT(fp, "Structs compare\n");
    else
       PRINT(fp, "Structs differ\n");
    err_tot &= err;

/* compare partial read results */
    err  = TRUE;
    err &= FLOAT_EQUAL(fs_p1_r, graph_w.x_axis[8]);
    err &= FLOAT_EQUAL(fs_p2_r, graph_w.view.x_max);
    err &= FLOAT_EQUAL(fs_p3_r, view_w.y_max);

    if (err)
       PRINT(fp, "Partial reads compare\n");
    else
       PRINT(fp, "Partial reads differ\n");
    err_tot &= err;

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_1 - test the fundamental PDBLib functions
 *        -
 *        - read and write scalars of all primitive types
 *        - read and write structures with no indirections
 *        - append data to a file and read it back
 *        - read structure members
 *        - read parts of arrays
 *        -
 *        - tests can be targeted
 */

static int test_1(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file as asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

    prep_test_1_data();

    if (read_only == FALSE)

/* create the named file */
       {strm = PD_create(datfile);
	if (strm == NULL)
	   error(1, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

/* make a few defstructs */
	PD_defstr(strm, "l_frame",
		  " float x_min",
		  "float x_max",
		  "float y_min",
		  "float y_max",
		  LAST);
	PD_defstr(strm, "plot",
		  "float x_axis(10)",
		  "float y_axis(10)",
		  "integer npts", 
		  "char * label",
		  "l_frame view",
		  LAST);

/* write the test data */
	write_test_1_data(strm);

/* close the file */
	if (PD_close(strm) == FALSE)
	   error(1, fp, "Test couldn't close file %s\r\n", datfile);
	PRINT(fp, "File %s closed\n", datfile);

/* reopen the file to append */
	strm = PD_open(datfile, "a");
	if (strm == NULL)
	   error(1, fp, "Test couldn't open file %s to append\r\n", datfile);
	PRINT(fp, "File %s opened to append\n", datfile);

	append_test_1_data(strm);

/* close the file after append */
	if (PD_close(strm) == FALSE)
	   error(1, fp, "Test couldn't close file %s after append\r\n",
		 datfile);
	PRINT(fp, "File %s closed after append\n", datfile);};

/* reopen the file */
    strm = PD_open(datfile, "r");
    if (strm == NULL)
       error(1, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s opened\n", datfile);

/* dump the symbol table */
    dump_test_symbol_table(fp, strm->symtab, 1);

/* read the data from the file */
    read_test_1_data(strm);

/* compare the original data with that read in */
    err = compare_test_1_data(strm, fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(1, fp, "Test couldn't close file %s\r\n", datfile);
    PRINT(fp, "File %s closed\n", datfile);

/* print it out to STDOUT */
    print_test_1_data(fp);

/* free known test data memory */
    cleanup_test_1();

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/

/*                            TEST #2 ROUTINES                              */

/*--------------------------------------------------------------------------*/

/* PREP_TEST_2_DATA - prepare the test data */

static void prep_test_2_data(void)
   {int i, *p1, *p2;
    double *p3, *p4;

/*    do_w = -1; */

    for (i = 0; i < N_INT; i++)
        {p_w[i] = i;
         p_r[i] = 0;};

    tar_w = CMAKE_N(lev1, 2);

    p1 = tar_w[0].a = CMAKE_N(int, N_INT);
    p2 = tar_w[1].a = CMAKE_N(int, N_INT);
    for (i = 0; i < N_INT; i++)
        {p1[i] = i;
         p2[i] = i + 10;};

    p3 = tar_w[0].b = CMAKE_N(double, N_DOUBLE);
    p4 = tar_w[1].b = CMAKE_N(double, N_DOUBLE);
    for (i = 0; i < N_DOUBLE; i++)
        {p3[i] = exp((double) i);
         p4[i] = log(1.0 + (double) i);};

    tar_w[0].c = CMAKE_N(lev2, 2);
    tar_w[1].c = CMAKE_N(lev2, 2);

    tar_w[0].c[0].s    = CMAKE_N(char *, 2);
    tar_w[0].c[0].s[0] = CSTRSAVE("Hello");
    tar_w[0].c[0].s[1] = CSTRSAVE(" ");
    tar_w[0].c[1].s    = CMAKE_N(char *, 2);
    tar_w[0].c[1].s[0] = CSTRSAVE("world");
    tar_w[0].c[1].s[1] = CSTRSAVE("!");

    tar_w[1].c[0].s    = CMAKE_N(char *, 2);
    tar_w[1].c[0].s[0] = CSTRSAVE("Foo");
    tar_w[1].c[0].s[1] = CSTRSAVE(" ");
    tar_w[1].c[1].s    = CMAKE_N(char *, 2);
    tar_w[1].c[1].s[0] = CSTRSAVE("Bar");
    tar_w[1].c[1].s[1] = CSTRSAVE("!!!");

    tar_w[0].c[0].type = 1;
    tar_w[0].c[1].type = 2;
    tar_w[1].c[0].type = 3;
    tar_w[1].c[1].type = 4;

    tar_r = NULL;

    ap1 = NULL;
    ap2 = NULL;
    for (i = 0; i < 4; i++)
        aa[i] = 0;

    bp1 = NULL;
    bp2 = NULL;
    for (i = 0; i < 4; i++)
        ba[i] = 0.0;

    cp1 = NULL;
    cp2 = NULL;
    for (i = 0; i < 4; i++)
        {ca[i].s    = NULL;
         ca[i].type = 0;};

    sp1 = NULL;
    sp2 = NULL;
    sp3 = NULL;
    sp4 = NULL;

    tp1 = NULL;
    tp2 = NULL;
    tp3 = NULL;
    tp4 = NULL;
    tp5 = NULL;
    tp6 = NULL;
    tp7 = NULL;
    tp8 = NULL;
    for (i = 0; i < 4; i++)
        ta[i] = 0;

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* CLEANUP_TEST_2 - free all know test data memory */

static void cleanup_test_2(void)
   {int i;

    if (tar_w != NULL)
       {CFREE(tar_w[0].c[0].s[0]);
        CFREE(tar_w[0].c[0].s[1]);
        CFREE(tar_w[0].c[1].s[0]);
        CFREE(tar_w[0].c[1].s[1]);

        CFREE(tar_w[1].c[0].s[0]);
        CFREE(tar_w[1].c[0].s[1]);
        CFREE(tar_w[1].c[1].s[0]);
        CFREE(tar_w[1].c[1].s[1]);

        CFREE(tar_w[0].c[0].s);
        CFREE(tar_w[0].c[1].s);
        CFREE(tar_w[1].c[0].s);
        CFREE(tar_w[1].c[1].s);

        CFREE(tar_w[0].c);
        CFREE(tar_w[1].c);

        CFREE(tar_w[0].a);
        CFREE(tar_w[1].a);

        CFREE(tar_w[0].b);
        CFREE(tar_w[1].b);

        CFREE(tar_w);};

    if (tar_r != NULL)
       {CFREE(tar_r[0].c[0].s[0]);
        CFREE(tar_r[0].c[0].s[1]);
        CFREE(tar_r[0].c[1].s[0]);
        CFREE(tar_r[0].c[1].s[1]);

        CFREE(tar_r[1].c[0].s[0]);
        CFREE(tar_r[1].c[0].s[1]);
        CFREE(tar_r[1].c[1].s[0]);
        CFREE(tar_r[1].c[1].s[1]);

        CFREE(tar_r[0].c[0].s);
        CFREE(tar_r[0].c[1].s);
        CFREE(tar_r[1].c[0].s);
        CFREE(tar_r[1].c[1].s);

        CFREE(tar_r[0].c);
        CFREE(tar_r[1].c);

        CFREE(tar_r[0].a);
        CFREE(tar_r[1].a);

        CFREE(tar_r[0].b);
        CFREE(tar_r[1].b);

        CFREE(tar_r);};

    CFREE(ap1);
    CFREE(ap2);

    CFREE(bp1);
    CFREE(bp2);

    if (cp1 != NULL)
       {if (cp1[0].s != NULL)
           {CFREE(cp1[0].s[0]);
            CFREE(cp1[0].s[1]);
            CFREE(cp1[0].s);};

        if (cp1[1].s != NULL)
           {CFREE(cp1[1].s[0]);
            CFREE(cp1[1].s[1]);
            CFREE(cp1[1].s);};

        CFREE(cp1);};

    if (cp2 != NULL)
       {if (cp2[0].s != NULL)
           {CFREE(cp2[0].s[0]);
            CFREE(cp2[0].s[1]);
            CFREE(cp2[0].s);};

        if (cp2[1].s != NULL)
           {CFREE(cp2[1].s[0]);
            CFREE(cp2[1].s[1]);
            CFREE(cp2[1].s);};

        CFREE(cp2);};

#ifndef PDB_LITE /* we'll leak these with PDB Lite */
    for (i = 0; i < 4; i++)
        {CFREE(ca[i].s[0]);
	 CFREE(ca[i].s[1]);
	 CFREE(ca[i].s);};
#endif

    if (sp1 != NULL)
       {CFREE(sp1[0]);
        CFREE(sp1[1]);
        CFREE(sp1);};

    if (sp2 != NULL)
       {CFREE(sp2[0]);
        CFREE(sp2[1]);
        CFREE(sp2);};

    if (sp3 != NULL)
       {CFREE(sp3[0]);
        CFREE(sp3[1]);
        CFREE(sp3);};

    if (sp4 != NULL)
       {CFREE(sp4[0]);
        CFREE(sp4[1]);
        CFREE(sp4);};

    CFREE(tp1);
    CFREE(tp2);
    CFREE(tp3);
    CFREE(tp4);
    CFREE(tp5);
    CFREE(tp6);
    CFREE(tp7);
    CFREE(tp8);

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* WRITE_TEST_2_DATA - write out the data into the PDB file */

static void write_test_2_data(PDBfile *strm)
   {long ind[3];

    ind[0] = 1L;
    ind[1] = N_INT;
    ind[2] = 1L;
    if (PD_write_alt(strm, "p", "integer", p_w, 1, ind) == 0)
       error(1, STDOUT, "P WRITE FAILED - WRITE_TEST_2_DATA\n");

    if (PD_write(strm, "tar", "lev1 *",  &tar_w) == 0)
       error(1, STDOUT, "TAR WRITE FAILED - WRITE_TEST_2_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* READ_TEST_2_DATA - read the test data from the file */

static int read_test_2_data(PDBfile *strm)
   {int err;

    do_r = strm->default_offset;

    err = PD_read(strm, "tar", &tar_r);
    err = PD_read(strm, "p", p_r);

    err = PD_read(strm, "tar(0).a", &ap1);
    err = PD_read(strm, "tar(1).a", &ap2);

    err = PD_read(strm, "tar(0).a(0)", &aa[0]);
    err = PD_read(strm, "tar(0).a(1)", &aa[1]);
    err = PD_read(strm, "tar(1).a(0)", &aa[2]);
    err = PD_read(strm, "tar(1).a(1)", &aa[3]);

    err = PD_read(strm, "tar(0).b", &bp1);
    err = PD_read(strm, "tar(1).b", &bp2);

    err = PD_read(strm, "tar(0).b(0)", &ba[0]);
    err = PD_read(strm, "tar(0).b(1)", &ba[1]);
    err = PD_read(strm, "tar(1).b(0)", &ba[2]);
    err = PD_read(strm, "tar(1).b(1)", &ba[3]);

    err = PD_read(strm, "tar(0).c", &cp1);
    err = PD_read(strm, "tar(1).c", &cp2);

    err = PD_read(strm, "tar(0).c(0)", &ca[0]);
    err = PD_read(strm, "tar(0).c(1)", &ca[1]);
    err = PD_read(strm, "tar(1).c(0)", &ca[2]);
    err = PD_read(strm, "tar(1).c(1)", &ca[3]);

    err = PD_read(strm, "tar(0).c(0).s", &sp1);
    err = PD_read(strm, "tar(0).c(1).s", &sp2);
    err = PD_read(strm, "tar(1).c(0).s", &sp3);
    err = PD_read(strm, "tar(1).c(1).s", &sp4);

    err = PD_read(strm, "tar(0).c(0).s(0)", &tp1);
    err = PD_read(strm, "tar(0).c(0).s(1)", &tp2);
    err = PD_read(strm, "tar(0).c(1).s(0)", &tp3);
    err = PD_read(strm, "tar(0).c(1).s(1)", &tp4);

    err = PD_read(strm, "tar(1).c(0).s(0)", &tp5);
    err = PD_read(strm, "tar(1).c(0).s(1)", &tp6);
    err = PD_read(strm, "tar(1).c(1).s(0)", &tp7);
    err = PD_read(strm, "tar(1).c(1).s(1)", &tp8);

    err = PD_read(strm, "tar(0).c(0).s(0)(2)", &ta[0]);
    err = PD_read(strm, "tar(0).c(0).s(1)(1)", &ta[1]);
    err = PD_read(strm, "tar(0).c(1).s(0)(3)", &ta[2]);
    err = PD_read(strm, "tar(0).c(1).s(1)(2)", &ta[3]);

    err = PD_read(strm, "tar(1).c(0).s(0)(2)", &ta[4]);
    err = PD_read(strm, "tar(1).c(0).s(1)(1)", &ta[5]);
    err = PD_read(strm, "tar(1).c(1).s(0)(3)", &ta[6]);
    err = PD_read(strm, "tar(1).c(1).s(1)(2)", &ta[7]);

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_TEST_2_DATA - print it out to the file */

static void print_test_2_data(FILE *fp)
   {int i;
    int *p1, *p2;
    double *p3, *p4;

    PRINT(fp, "\n");

    p1 = tar_r[0].a;
    p2 = tar_r[1].a;
    PRINT(fp, "TAR struct member A:\n");
    PRINT(fp, "    TAR[0].A    TAR[1].A\n");
    for (i = 0; i < N_INT; i++)
        PRINT(fp, "        %d          %d\n", p1[i], p2[i]);
    PRINT(fp, "\n");

    p3 = tar_r[0].b;
    p4 = tar_r[1].b;
    PRINT(fp, "TAR struct member B:\n");
    PRINT(fp, "    TAR[0].B    TAR[1].B\n");
    for (i = 0; i < N_DOUBLE; i++)
        PRINT(fp, "    %f    %f\n", p3[i], p4[i]);
    PRINT(fp, "\n");


    PRINT(fp, "TAR struct member C:\n");
    PRINT(fp, "    TAR[0].C[0]         TAR[0].C[1]\n");
    PRINT(fp, "   S[0]    S[1]        S[0]    S[1]\n");
    PRINT(fp, "  %s      %s        %s      %s\n",
                tar_r[0].c[0].s[0], tar_r[0].c[0].s[1],
                tar_r[0].c[1].s[0], tar_r[0].c[1].s[1]);
    PRINT(fp, "\n       TYPE\n");
    PRINT(fp, "     %d       %d\n",
                tar_r[0].c[0].type, tar_r[0].c[1].type);


    PRINT(fp, "\n    TAR[1].C[0]         TAR[1].C[1]\n");
    PRINT(fp, "   S[0]    S[1]        S[0]    S[1]\n");
    PRINT(fp, "   %s     %s           %s      %s\n",
                tar_r[1].c[0].s[0], tar_r[1].c[0].s[1],
                tar_r[1].c[1].s[0], tar_r[1].c[1].s[1]);

    PRINT(fp, "       TYPE\n");
    PRINT(fp, "     %d       %d\n",
                tar_r[1].c[0].type, tar_r[1].c[1].type);

    PRINT(fp, "\n");

/* print the offset */
    PRINT(fp, "file array index offset: %d\n\n", do_r);

    PRINT(fp, "\n");

/* read the an array which has offsets */
    PRINT(fp, "array p: \n");
    for (i = 0; i < N_INT; i++)
        PRINT(fp, "  %d\n", p_r[i]);

    PRINT(fp, "\n");

/* print the partial and member read stuff */
    if (ap1 != NULL)
       {PRINT(fp, "member read    TAR[0].A    TAR[1].A\n");
        for (i = 0; i < N_INT; i++)
            PRINT(fp, "                  %d          %d\n", ap1[i], ap2[i]);
        PRINT(fp, "\n");};
        
    if (bp1 != NULL)
       {PRINT(fp, "member read    TAR[0].B    TAR[1].B\n");
        for (i = 0; i < N_DOUBLE; i++)
            PRINT(fp, "               %f    %f\n", bp1[i], bp2[i]);
        PRINT(fp, "\n");};

    if (cp1 != NULL)
       {PRINT(fp, "member read TAR[0].C:\n");
        PRINT(fp, "    TAR[0].C[0]         TAR[0].C[1]\n");
        PRINT(fp, "   S[0]    S[1]        S[0]    S[1]\n");
        PRINT(fp, "  %s      %s        %s      %s\n",
                    cp1[0].s[0], cp1[0].s[1],
                    cp1[1].s[0], cp1[1].s[1]);
        PRINT(fp, "\n       TYPE\n");
        PRINT(fp, "     %d       %d\n",
                    cp1[0].type, cp1[1].type);

        PRINT(fp, "\nmember read TAR[1].C:\n");
        PRINT(fp, "    TAR[1].C[0]         TAR[1].C[1]\n");
        PRINT(fp, "   S[0]    S[1]        S[0]    S[1]\n");
        PRINT(fp, "   %s     %s           %s      %s\n",
                    cp2[0].s[0], cp2[0].s[1],
                    cp2[1].s[0], cp2[1].s[1]);
        PRINT(fp, "\n       TYPE\n");
        PRINT(fp, "     %d       %d\n",
                    cp2[0].type, cp2[1].type);};

/*
    PRINT(fp, "member read TAR[0].C[0]:\n");
    PRINT(fp, "   S[0]    S[1]\n");
    PRINT(fp, "  %s      %s\n", ca[0].s[0], ca[0].s[1]);

    PRINT(fp, "member read TAR[0].C[1]:\n");
    PRINT(fp, "   S[0]    S[1]\n");
    PRINT(fp, "  %s      %s\n", ca[1].s[0], ca[1].s[1]);

    PRINT(fp, "member read TAR[1].C[1]:\n");
    PRINT(fp, "   S[0]    S[1]\n");
    PRINT(fp, "  %s      %s\n", ca[2].s[0], ca[2].s[1]);

    PRINT(fp, "member read TAR[1].C[1]:\n");
    PRINT(fp, "   S[0]    S[1]\n");
    PRINT(fp, "  %s      %s\n", ca[3].s[0], ca[3].s[1]);
*/
/* TAR.C.S read */

    if (sp1 != NULL)
       {PRINT(fp, "\nmember read TAR[0].C[0].S:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", sp1[0], sp1[1]);

        PRINT(fp, "\nmember read TAR[0].C[1].S:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", sp2[0], sp2[1]);

        PRINT(fp, "\nmember read TAR[1].C[0].S:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", sp3[0], sp3[1]);

        PRINT(fp, "\nmember read TAR[1].C[1].S:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", sp4[0], sp4[1]);};

/* TAR.C.S[i] read */

    if (tp1 != NULL)
       {PRINT(fp, "\nmember read TAR[0].C[0].S[0]:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", tp1, tp1);

        PRINT(fp, "\nmember read TAR[0].C[1].S[1]:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", tp2, tp2);

        PRINT(fp, "\nmember read TAR[0].C[1].S[0]:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", tp3, tp3);

        PRINT(fp, "\nmember read TAR[0].C[1].S[1]:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", tp4, tp4);

        PRINT(fp, "\nmember read TAR[1].C[0].S[0]:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", tp5, tp5);

        PRINT(fp, "\nmember read TAR[1].C[0].S[1]:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", tp6, tp6);

        PRINT(fp, "\nmember read TAR[1].C[1].S[0]:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", tp7, tp7);

        PRINT(fp, "\nmember read TAR[1].C[1].S[1]:\n");
        PRINT(fp, "   S[0]    S[1]\n");
        PRINT(fp, "  %s      %s\n", tp8, tp8);

/* TAR.C.S[i][j] read */

        PRINT(fp, "\nmember read from TAR[0]:\n");
        PRINT(fp, "C[0].S[0][2]  C[0].S[1][1]  C[1].S[0][3]  C[1].S[1][2]\n");
        PRINT(fp, "     %c             %c           %c             %c\n",
                    ta[0], ta[1], ta[2], ta[3]);};

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_2_DATA - compare the test data */

static int compare_test_2_data(PDBfile *strm, FILE *fp)
   {int i, err, err_tot;
    int *p1w, *p2w, *p1r, *p2r;
    double *p3w, *p4w, *p3r, *p4r;
    long double fptol[N_PRIMITIVE_FP];

    PD_fp_toler(strm, fptol);

    err_tot = TRUE;

/* compare offset and array */
    err  = TRUE;
    err &= (do_r == do_w);

    for (i = 0; i < N_INT; i++)
        err &= (p_w[i] == p_r[i]);

    if (err)
       PRINT(fp, "Offset compare\n");
    else
       PRINT(fp, "Offset differ\n");
    err_tot &= err;

    err = TRUE;
    p1w = tar_w[0].a;
    p2w = tar_w[1].a;
    p1r = tar_r[0].a;
    p2r = tar_r[1].a;
    for (i = 0; i < N_INT; i++)
        {err &= (p1w[i] == p1r[i]);
         err &= (p2w[i] == p2r[i]);};

    p3w = tar_w[0].b;
    p4w = tar_w[1].b;
    p3r = tar_r[0].b;
    p4r = tar_r[1].b;
    for (i = 0; i < N_DOUBLE; i++)
        {err &= DOUBLE_EQUAL(p3w[i], p3r[i]);
         err &= DOUBLE_EQUAL(p4w[i], p4r[i]);};

    err &= (strcmp(tar_w[0].c[0].s[0], tar_r[0].c[0].s[0]) == 0);
    err &= (strcmp(tar_w[0].c[0].s[1], tar_r[0].c[0].s[1]) == 0);
    err &= (strcmp(tar_w[0].c[1].s[0], tar_r[0].c[1].s[0]) == 0);
    err &= (strcmp(tar_w[0].c[1].s[1], tar_r[0].c[1].s[1]) == 0);

    err &= (strcmp(tar_w[1].c[0].s[0], tar_r[1].c[0].s[0]) == 0);
    err &= (strcmp(tar_w[1].c[0].s[1], tar_r[1].c[0].s[1]) == 0);
    err &= (strcmp(tar_w[1].c[1].s[0], tar_r[1].c[1].s[0]) == 0);
    err &= (strcmp(tar_w[1].c[1].s[1], tar_r[1].c[1].s[1]) == 0);

    err &= (tar_w[0].c[0].type == tar_r[0].c[0].type);
    err &= (tar_w[0].c[1].type == tar_r[0].c[1].type);
    err &= (tar_w[1].c[0].type == tar_r[1].c[0].type);
    err &= (tar_w[1].c[1].type == tar_r[1].c[1].type);

    if (err)
       PRINT(fp, "Indirects compare\n");
    else
       PRINT(fp, "Indirects differ\n");
    err_tot &= err;

    PRINT(fp, "\n");

    if (err)
       PRINT(fp, "Offset compare\n");
    else
       PRINT(fp, "Offset differ\n");
    err_tot &= err;

    err = TRUE;

    if (ap1 != NULL)
       {p1w = tar_w[0].a;
        p2w = tar_w[1].a;
        for (i = 0; i < N_INT; i++)
            {err &= (p1w[i] == ap1[i]);
             err &= (p2w[i] == ap2[i]);};};

    if (bp1 != NULL)
       {p3w = tar_w[0].b;
        p4w = tar_w[1].b;
        for (i = 0; i < N_DOUBLE; i++)
            {err &= DOUBLE_EQUAL(p3w[i], bp1[i]);
             err &= DOUBLE_EQUAL(p4w[i], bp2[i]);};};

    if (cp1 != NULL)
       {err &= (strcmp(tar_w[0].c[0].s[0], cp1[0].s[0]) == 0);
        err &= (strcmp(tar_w[0].c[0].s[1], cp1[0].s[1]) == 0);
        err &= (strcmp(tar_w[0].c[1].s[0], cp1[1].s[0]) == 0);
        err &= (strcmp(tar_w[0].c[1].s[1], cp1[1].s[1]) == 0);

        err &= (strcmp(tar_w[1].c[0].s[0], cp2[0].s[0]) == 0);
        err &= (strcmp(tar_w[1].c[0].s[1], cp2[0].s[1]) == 0);
        err &= (strcmp(tar_w[1].c[1].s[0], cp2[1].s[0]) == 0);
        err &= (strcmp(tar_w[1].c[1].s[1], cp2[1].s[1]) == 0);

        err &= (tar_w[0].c[0].type == cp1[0].type);
        err &= (tar_w[0].c[1].type == cp1[1].type);
        err &= (tar_w[1].c[0].type == cp2[0].type);
        err &= (tar_w[1].c[1].type == cp2[1].type);};
/*
    err &= (strcmp(tar_w[0].c[0].s[0], ca[0].s[0]) == 0);
    err &= (strcmp(tar_w[0].c[0].s[1], ca[0].s[1]) == 0);
    err &= (strcmp(tar_w[0].c[1].s[0], ca[1].s[0]) == 0);
    err &= (strcmp(tar_w[0].c[1].s[1], ca[1].s[1]) == 0);

    err &= (strcmp(tar_w[1].c[0].s[0], ca[2].s[0]) == 0);
    err &= (strcmp(tar_w[1].c[0].s[1], ca[2].s[1]) == 0);
    err &= (strcmp(tar_w[1].c[1].s[0], ca[3].s[0]) == 0);
    err &= (strcmp(tar_w[1].c[1].s[1], ca[3].s[1]) == 0);
*/
    if (sp1 != NULL)
       {err &= (strcmp(tar_w[0].c[0].s[0], sp1[0]) == 0);
        err &= (strcmp(tar_w[0].c[0].s[1], sp1[1]) == 0);
        err &= (strcmp(tar_w[0].c[1].s[0], sp2[0]) == 0);
        err &= (strcmp(tar_w[0].c[1].s[1], sp2[1]) == 0);

        err &= (strcmp(tar_w[1].c[0].s[0], sp3[0]) == 0);
        err &= (strcmp(tar_w[1].c[0].s[1], sp3[1]) == 0);
        err &= (strcmp(tar_w[1].c[1].s[0], sp4[0]) == 0);
        err &= (strcmp(tar_w[1].c[1].s[1], sp4[1]) == 0);};

    if (tp1 != NULL)
       {err &= (strcmp(tar_w[0].c[0].s[0], tp1) == 0);
        err &= (strcmp(tar_w[0].c[0].s[1], tp2) == 0);
        err &= (strcmp(tar_w[0].c[1].s[0], tp3) == 0);
        err &= (strcmp(tar_w[0].c[1].s[1], tp4) == 0);

        err &= (strcmp(tar_w[1].c[0].s[0], tp5) == 0);
        err &= (strcmp(tar_w[1].c[0].s[1], tp6) == 0);
        err &= (strcmp(tar_w[1].c[1].s[0], tp7) == 0);
        err &= (strcmp(tar_w[1].c[1].s[1], tp8) == 0);

        err &= (tar_w[0].c[0].s[0][2] == ta[0]);
        err &= (tar_w[0].c[0].s[1][1] == ta[1]);
        err &= (tar_w[0].c[1].s[0][3] == ta[2]);
        err &= (tar_w[0].c[1].s[1][2] == ta[3]);};

    if (err)
       PRINT(fp, "Indirects compare\n");
    else
       PRINT(fp, "Indirects differ\n");
    err_tot &= err;

    PRINT(fp, "\n");

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_2 - test the PDBLib functions handling indirections
 *        -
 *        - read and write structures with pointers
 *        - set default array base indexes
 *        - read structure members
 *        - read parts of arrays
 *        -
 *        - tests can be targeted
 */

static int test_2(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file is asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

    prep_test_2_data();

    if (read_only == FALSE)

/* create the named file */
       {strm = PD_create(datfile);
	if (strm == NULL)
	   error(2, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

/* set the default offset */
	strm->default_offset = do_w;

/* make a few defstructs */
	PD_defstr(strm, "lev2",
		  "char **s",
		  "integer type",
		  LAST);

	PD_defstr(strm, "lev1",
		  "integer *a",
		  "double *b",
		  "lev2 *c",
		  LAST);

/* write the test data */
	write_test_2_data(strm);

/* close the file */
	if (PD_close(strm) == FALSE)
	   error(2, fp, "Test couldn't close file %s\r\n", datfile);
	PRINT(fp, "File %s closed\n", datfile);};

/* reopen the file */
    strm = PD_open(datfile, "r");
    if (strm == NULL)
       error(2, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s opened\n", datfile);

/* dump the symbol table */
    dump_test_symbol_table(fp, strm->symtab, 2);

/* read the structs */
    read_test_2_data(strm);

/* compare the original data with that read in */
    err = compare_test_2_data(strm, fp);

/* print out the results */
    print_test_2_data(fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(2, fp, "Test couldn't close file %s\r\n", datfile);
    PRINT(fp, "File %s closed\n", datfile);

/* cleanup test data memory */
    cleanup_test_2();

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/

/*                            TEST #3 ROUTINES                              */

/*--------------------------------------------------------------------------*/

/* PREP_TEST_3_DATA - prepare the test data */

static void prep_test_3_data(void)
   {

    vr1_w.a    = 'A';
    vr1_w.b    = -10;
    vr1_w.c[0] = 'B';
    vr1_w.c[1] = 'a';
    vr1_w.d    = 32589;
    vr1_w.e[0] = 'C';
    vr1_w.e[1] = 'a';
    vr1_w.e[2] = 'b';
    vr1_w.f    = 1.523e-19;
    vr1_w.g[0] = 'D';
    vr1_w.g[1] = 'a';
    vr1_w.g[2] = 'b';
    vr1_w.g[3] = 'c';
    vr1_w.h    = 4.2782918323832554e30;
    vr1_w.i[0] = 'E';
    vr1_w.i[1] = 'a';
    vr1_w.i[2] = 'b';
    vr1_w.i[3] = 'c';
    vr1_w.i[4] = 'd';
    vr1_w.j    = CSTRSAVE("whoa there big fella!");
    vr1_w.k[0] = 'F';
    vr1_w.k[1] = 'a';
    vr1_w.k[2] = 'b';
    vr1_w.k[3] = 'c';
    vr1_w.k[4] = 'd';
    vr1_w.k[5] = 'e';

    vr1_r.a    = '\0';
    vr1_r.b    = 0;
    vr1_r.c[0] = '\0';
    vr1_r.c[1] = '\0';
    vr1_r.d    = 0;
    vr1_r.e[0] = '\0';
    vr1_r.e[1] = '\0';
    vr1_r.e[2] = '\0';
    vr1_r.f    = 0.0;
    vr1_r.g[0] = '\0';
    vr1_r.g[1] = '\0';
    vr1_r.g[2] = '\0';
    vr1_r.g[3] = '\0';
    vr1_r.h    = 0.0;
    vr1_r.i[0] = '\0';
    vr1_r.i[1] = '\0';
    vr1_r.i[2] = '\0';
    vr1_r.i[3] = '\0';
    vr1_r.i[4] = '\0';
    vr1_r.j    = NULL;
    vr1_r.k[0] = '\0';
    vr1_r.k[1] = '\0';
    vr1_r.k[2] = '\0';
    vr1_r.k[3] = '\0';
    vr1_r.k[4] = '\0';
    vr1_r.k[5] = '\0';

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* CLEANUP_TEST_3 - free all known test data memory */

static void cleanup_test_3(void)
   {

    CFREE(vr1_w.j);
    CFREE(vr1_r.j);

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* WRITE_TEST_3_DATA - write out the data into the PDB file */

static void write_test_3_data(PDBfile *strm)
   {

    if (PD_write(strm, "vr1", "st3", &vr1_w) == 0)
       error(1, STDOUT, "VR1 WRITE FAILED - WRITE_TEST_3_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* READ_TEST_3_DATA - read the test data from the file */

static int read_test_3_data(PDBfile *strm)
   {int err;

    err = PD_read(strm, "vr1", &vr1_r);

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_TEST_3_DATA - print it out to the file */

static void print_test_3_data(FILE *fp)
   {

    PRINT(fp, "struct vr1:\n");
    PRINT(fp, "   vr1.a    = %c\n", vr1_r.a);
    PRINT(fp, "   vr1.b    = %d\n", vr1_r.b);
    PRINT(fp, "   vr1.c[0] = %c\n", vr1_r.c[0]);
    PRINT(fp, "   vr1.c[1] = %c\n", vr1_r.c[1]);
    PRINT(fp, "   vr1.d    = %d\n", vr1_r.d);
    PRINT(fp, "   vr1.e[0] = %c\n", vr1_r.e[0]);
    PRINT(fp, "   vr1.e[1] = %c\n", vr1_r.e[1]);
    PRINT(fp, "   vr1.e[2] = %c\n", vr1_r.e[2]);
    PRINT(fp, "   vr1.f    = %14.7e\n", vr1_r.f);
    PRINT(fp, "   vr1.g[0] = %c\n", vr1_r.g[0]);
    PRINT(fp, "   vr1.g[1] = %c\n", vr1_r.g[1]);
    PRINT(fp, "   vr1.g[2] = %c\n", vr1_r.g[2]);
    PRINT(fp, "   vr1.g[3] = %c\n", vr1_r.g[3]);
    PRINT(fp, "   vr1.h    = %20.13e\n", vr1_r.h);
    PRINT(fp, "   vr1.i[0] = %c\n", vr1_r.i[0]);
    PRINT(fp, "   vr1.i[1] = %c\n", vr1_r.i[1]);
    PRINT(fp, "   vr1.i[2] = %c\n", vr1_r.i[2]);
    PRINT(fp, "   vr1.i[3] = %c\n", vr1_r.i[3]);
    PRINT(fp, "   vr1.i[4] = %c\n", vr1_r.i[4]);
    PRINT(fp, "   vr1.j    = %s\n", vr1_r.j);
    PRINT(fp, "   vr1.k[0] = %c\n", vr1_r.k[0]);
    PRINT(fp, "   vr1.k[1] = %c\n", vr1_r.k[1]);
    PRINT(fp, "   vr1.k[2] = %c\n", vr1_r.k[2]);
    PRINT(fp, "   vr1.k[3] = %c\n", vr1_r.k[3]);
    PRINT(fp, "   vr1.k[4] = %c\n", vr1_r.k[4]);
    PRINT(fp, "   vr1.k[5] = %c\n", vr1_r.k[5]);

    PRINT(fp, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_3_DATA - compare the test data */

static int compare_test_3_data(PDBfile *strm, FILE *fp)
   {int err, err_tot;
    long double fptol[N_PRIMITIVE_FP];

    PD_fp_toler(strm, fptol);

    err_tot = TRUE;

/* compare offset and array */
    err  = TRUE;
    err &= (vr1_w.a == vr1_r.a);
    err &= (vr1_w.b == vr1_r.b);
    err &= (vr1_w.c[0] == vr1_r.c[0]);
    err &= (vr1_w.c[1] == vr1_r.c[1]);
    err &= (vr1_w.d == vr1_r.d);
    err &= (vr1_w.e[0] == vr1_r.e[0]);
    err &= (vr1_w.e[1] == vr1_r.e[1]);
    err &= (vr1_w.e[2] == vr1_r.e[2]);
    err &= FLOAT_EQUAL(vr1_w.f, vr1_r.f);
    err &= (vr1_w.g[0] == vr1_r.g[0]);
    err &= (vr1_w.g[1] == vr1_r.g[1]);
    err &= (vr1_w.g[2] == vr1_r.g[2]);
    err &= (vr1_w.g[3] == vr1_r.g[3]);
    err &= DOUBLE_EQUAL(vr1_w.h, vr1_r.h);
    err &= (vr1_w.i[0] == vr1_r.i[0]);
    err &= (vr1_w.i[1] == vr1_r.i[1]);
    err &= (vr1_w.i[2] == vr1_r.i[2]);
    err &= (vr1_w.i[3] == vr1_r.i[3]);
    err &= (vr1_w.i[4] == vr1_r.i[4]);
    err &= (strcmp(vr1_w.j, vr1_r.j) == 0);
    err &= (vr1_w.k[0] == vr1_r.k[0]);
    err &= (vr1_w.k[1] == vr1_r.k[1]);
    err &= (vr1_w.k[2] == vr1_r.k[2]);
    err &= (vr1_w.k[3] == vr1_r.k[3]);
    err &= (vr1_w.k[4] == vr1_r.k[4]);
    err &= (vr1_w.k[5] == vr1_r.k[5]);

    if (err)
       PRINT(fp, "Alignments compare\n");
    else
       PRINT(fp, "Alignments differ\n");
    err_tot &= err;

    PRINT(fp, "\n");

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_3 - test the PDBLib functions handling indirections
 *        - read and write structures with alignment difficulties
 *        - tests can be targeted
 */

static int test_3(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file is asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

    prep_test_3_data();

    if (read_only == FALSE)

/* create the named file */
       {if ((strm = PD_create(datfile)) == NULL)
	   error(2, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

/* make a few defstructs */
	PD_defstr(strm, "st3",
		  "char a",
		  "short b",
		  "char c[2]",
		  "integer d",
		  "char e[3]",
		  "float f",
		  "char g[4]",
		  "double h",
		  "char i[5]",
		  "char *j",
		  "char k[6]",
		  LAST);

/* write the test data */
	write_test_3_data(strm);

/* close the file */
	if (PD_close(strm) == FALSE)
	   error(2, fp, "Test couldn't close file %s\r\n", datfile);
	PRINT(fp, "File %s closed\n", datfile);

#ifdef LINUX
/* GOTCHA: Without this delay, the test fails about half the time,
 *         iff the files are being accessed across the network.
 */
	SC_sleep(10);
#endif
	};

/* reopen the file */
    if ((strm = PD_open(datfile, "r")) == NULL)
       error(2, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s opened\n", datfile);

/* dump the symbol table */
    dump_test_symbol_table(fp, strm->symtab, 3);

/* read the structs */
    read_test_3_data(strm);

/* compare the original data with that read in */
    err = compare_test_3_data(strm, fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(2, fp, "Test couldn't close file %s\r\n", datfile);
    PRINT(fp, "File %s closed\n", datfile);

/* print out the results */
    print_test_3_data(fp);

/* clean up test data memory */
    cleanup_test_3();

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/

/*                            TEST #4 ROUTINES                              */

/*--------------------------------------------------------------------------*/

/* PREP_TEST_4_DATA - prepare the test data */

static void prep_test_4_data(void)
   {int *pi;
    short *ps;
    long *pl;
    float *pf;
    double *pd;
    char *pc;
    haelem *hp;

    tab4_w = SC_make_hasharr(3, NODOC, SC_HA_NAME_KEY, 0);

    CHAR_S   = CSTRSAVE("char *");
    SHORT_S  = CSTRSAVE("short *");
    INT_S    = CSTRSAVE("integer *");
    LONG_S   = CSTRSAVE("long *");
    FLOAT_S  = CSTRSAVE("float *");
    DOUBLE_S = CSTRSAVE("double *");
    HASHEL_S = CSTRSAVE(haelemstr " *");

    pc  = CMAKE(char);
    *pc = 'A';   
    SC_hasharr_install(tab4_w, "pc", pc, CHAR_S, TRUE, TRUE);

    ps  = CMAKE(short);
    *ps = -1024;
    SC_hasharr_install(tab4_w, "ps", ps, SHORT_S, TRUE, TRUE);

    pi  = CMAKE(int);
    *pi = 16384;
    SC_hasharr_install(tab4_w, "pi", pi, INT_S, TRUE, TRUE);

    pl  = CMAKE(long);
    *pl = -1048576;
    SC_hasharr_install(tab4_w, "pl", pl, LONG_S, TRUE, TRUE);

    pf  = CMAKE(float);
    *pf = 3.141596;
    SC_hasharr_install(tab4_w, "pf", pf, FLOAT_S, TRUE, TRUE);

    pd  = CMAKE(double);
    *pd = -1.0e-30;
    hp = SC_hasharr_install(tab4_w, "pd", pd, DOUBLE_S, TRUE, TRUE);

    SC_hasharr_install(tab4_w, "ph", hp, HASHEL_S, TRUE, TRUE);

    tab4_r = NULL;

    vr4_w = CMAKE_N(st4, 3);

    vr4_w[0].a =  2048;
    vr4_w[0].b =  'G';
    vr4_w[1].a = -2048;
    vr4_w[1].b =  'H';
    vr4_w[2].a =  4096;
    vr4_w[2].b =  'I';

    vr4_r = NULL;

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* CLEANUP_HA_TEST_4 - free the haelem types which were read in not constant */

static int cleanup_ha_test_4(haelem *hp, void *a)
   {

    CFREE(hp->type);

    return(TRUE);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* CLEANUP_TEST_4 - free all known test data memory */

static void cleanup_test_4(void)
   {
        /* PDB Lite's hash table isn't smart enough to deal with interdependencies
           of 'pd' and 'hp'. So, break that linkage now by null'ing hp's reference
           to the hashel for 'pd' */
#ifdef PDB_LITE
        haelem *hp = SC_lookup("ph", tab4_w);
        hp->def = 0;
#endif
    SC_free_hasharr(tab4_w, NULL, NULL);

    if (tab4_r != NULL)
       SC_free_hasharr(tab4_r, cleanup_ha_test_4, NULL);

    CFREE(CHAR_S);
    CFREE(SHORT_S);
    CFREE(INT_S);
    CFREE(LONG_S);
    CFREE(FLOAT_S);
    CFREE(DOUBLE_S);
    CFREE(HASHEL_S);

    CFREE(vr4_w);
    CFREE(vr4_r);

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* WRITE_TEST_4_DATA - write out the data into the PDB file */

static void write_test_4_data(PDBfile *strm)
   {

    if (PD_write(strm, "tab4", hasharrstr " *", &tab4_w) == 0)
       error(1, STDOUT, "TAB4 WRITE FAILED - WRITE_TEST_4_DATA\n");

    if (PD_write(strm, "vr4", "st4 *", &vr4_w) == 0)
       error(1, STDOUT, "VR4 WRITE FAILED - WRITE_TEST_4_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* READ_TEST_4_DATA - read the test data from the file */

static int read_test_4_data(PDBfile *strm)
   {int err;

    err = PD_read(strm, "tab4", &tab4_r);
    SC_hasharr_rekey(tab4_r, SC_HA_NAME_KEY);

    err = PD_read(strm, "vr4", &vr4_r);

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_TEST_4_DATA - print it out to the file */

static void print_test_4_data(FILE *fp)
   {int i, ne, *pi;
    long *pl;
    short *ps;
    float *pf;
    double *pd;
    char *pc, **names;
    haelem *ph;

    ne    = SC_hasharr_get_n(tab4_r);
    names = SC_hasharr_dump(tab4_r, NULL, NULL, FALSE);
    for (i = 0; i < ne; i++)
        PRINT(fp, "%s\n", names[i]);
    SC_free_strings(names);

    PRINT(fp, "\n");

    pc = (char *) SC_hasharr_def_lookup(tab4_r, "pc");
    ps = (short *) SC_hasharr_def_lookup(tab4_r, "ps");
    pi = (int *) SC_hasharr_def_lookup(tab4_r, "pi");
    pl = (long *) SC_hasharr_def_lookup(tab4_r, "pl");
    pf = (float *) SC_hasharr_def_lookup(tab4_r, "pf");
    pd = (double *) SC_hasharr_def_lookup(tab4_r, "pd");
    ph = (haelem *) SC_hasharr_def_lookup(tab4_r, "ph");

    PRINT(fp, "Table values:\n");
    PRINT(fp, "   pc = %c %s\n", *pc, CHAR_S);
    PRINT(fp, "   ps = %d %s\n", *ps, SHORT_S);
    PRINT(fp, "   pi = %d %s\n", *pi, INT_S);
    PRINT(fp, "   pl = %ld %s\n", *pl, LONG_S);
    PRINT(fp, "   pf = %f %s\n", *pf, FLOAT_S);
    PRINT(fp, "   pd = %e %s\n", *pd, DOUBLE_S);

    PRINT(fp, "\n   ph : %s %s\n\n", ph->name, ph->type);

    PRINT(fp, "VR4[0]:\n");
    PRINT(fp, "VR4[0].A = %d\n", vr4_r[0].a);
    PRINT(fp, "VR4[0].B = %c\n", vr4_r[0].b);
    PRINT(fp, "VR4[1].A = %d\n", vr4_r[1].a);
    PRINT(fp, "VR4[1].B = %c\n", vr4_r[1].b);
    PRINT(fp, "VR4[2].A = %d\n", vr4_r[2].a);
    PRINT(fp, "VR4[2].B = %c\n", vr4_r[2].b);

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_4_DATA - compare the test data */

static int compare_test_4_data(PDBfile *strm, FILE *fp)
   {int err, err_tot;
    long double fptol[N_PRIMITIVE_FP];
    char *pc_w, *pc_r;
    short *ps_w, *ps_r;
    int *pi_w, *pi_r;
    long *pl_w, *pl_r;
    float *pf_w, *pf_r;
    double *pd_w, *pd_r;
    haelem *ph_w, *ph_r;

    PD_fp_toler(strm, fptol);

    err_tot = TRUE;

    pc_r = (char *) SC_hasharr_def_lookup(tab4_r, "pc");
    ps_r = (short *) SC_hasharr_def_lookup(tab4_r, "ps");
    pi_r = (int *) SC_hasharr_def_lookup(tab4_r, "pi");
    pl_r = (long *) SC_hasharr_def_lookup(tab4_r, "pl");
    pf_r = (float *) SC_hasharr_def_lookup(tab4_r, "pf");
    pd_r = (double *) SC_hasharr_def_lookup(tab4_r, "pd");
    ph_r = (haelem *) SC_hasharr_def_lookup(tab4_r, "ph");

    pc_w = (char *) SC_hasharr_def_lookup(tab4_w, "pc");
    ps_w = (short *) SC_hasharr_def_lookup(tab4_w, "ps");
    pi_w = (int *) SC_hasharr_def_lookup(tab4_w, "pi");
    pl_w = (long *) SC_hasharr_def_lookup(tab4_w, "pl");
    pf_w = (float *) SC_hasharr_def_lookup(tab4_w, "pf");
    pd_w = (double *) SC_hasharr_def_lookup(tab4_w, "pd");
    ph_w = (haelem *) SC_hasharr_def_lookup(tab4_w, "ph");

/* compare offset and array */
    err  = TRUE;
    err &= (*pc_r == *pc_w);
    err &= (*ps_r == *ps_w);
    err &= (*pi_r == *pi_w);
    err &= (*pl_r == *pl_w);
    err &= FLOAT_EQUAL(*pf_r, *pf_w);
    err &= DOUBLE_EQUAL(*pd_r, *pd_w);

    if (err)
       PRINT(fp, "Hash tables compare\n");
    else
       PRINT(fp, "Hash tables differ\n");
    err_tot &= err;

/* compare haelem info */
    err  = TRUE;
    err &= (strcmp(ph_r->name, ph_w->name) == 0);
    err &= (strcmp(ph_r->type, ph_w->type) == 0);
    if (err)
       PRINT(fp, "Hashels compare\n");
    else
       PRINT(fp, "Hashels differ\n");
    err_tot &= err;

/* check the new alignments */
    err  = TRUE;
    err &= (vr4_w[0].a == vr4_r[0].a);
    err &= (vr4_w[0].b == vr4_r[0].b);
    err &= (vr4_w[1].a == vr4_r[1].a);
    err &= (vr4_w[1].b == vr4_r[1].b);
    err &= (vr4_w[2].a == vr4_r[2].a);
    err &= (vr4_w[2].b == vr4_r[2].b);

    if (err)
       PRINT(fp, "Alignments compare\n");
    else
       PRINT(fp, "Alignments differ\n");
    err_tot &= err;

    PRINT(fp, "\n");

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_4 - test the PDBLib functions handling indirections
 *        - read and write structures with alignment difficulties
 *        - tests can be targeted
 */

static int test_4(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file as asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

    prep_test_4_data();

    if (read_only == FALSE)

/* create the named file */
       {strm = PD_create(datfile);
	if (strm == NULL)
	   error(2, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

	strm->previous_file = CSTRSAVE(base);

/* make a few defstructs */
	PD_def_hash_types(strm, 3);

	PD_defstr(strm, "st4",
		  "short a",
		  "char b",
		  LAST);

/* write the test data */
	write_test_4_data(strm);

/* close the file */
	if (PD_close(strm) == FALSE)
	   error(2, fp, "Test couldn't close file %s\r\n", datfile);
	PRINT(fp, "File %s closed\n", datfile);};

/* reopen the file */
    strm = PD_open(datfile, "r");
    if (strm == NULL)
       error(2, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s opened\n", datfile);

/* dump the symbol table */
    dump_test_symbol_table(fp, strm->symtab, 4);

/* read the structs */
    read_test_4_data(strm);

/* compare the original data with that read in */
    err = compare_test_4_data(strm, fp);

/* print out the results */
    print_test_4_data(fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(2, fp, "Test couldn't close file %s\r\n", datfile);
    PRINT(fp, "File %s closed\n", datfile);

/* clean up test data memory */
    cleanup_test_4();

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/

/*                            TEST #5 ROUTINES                              */

/*--------------------------------------------------------------------------*/

/* PREP_TEST_5_DATA - prepare the test data */

static void prep_test_5_data(void)
   {int i, *p1, *p2;
    double *p3, *p4;

    for (i = 0; i < N_INT; i++)
        {p_w[i] = i;
         p_r[i] = 0;};

    tar_w = CMAKE_N(lev1, 2);

    p1 = tar_w[0].a = CMAKE_N(int, N_INT);
    p2 = tar_w[1].a = CMAKE_N(int, N_INT);
    for (i = 0; i < N_INT; i++)
        {p1[i] = i;
         p2[i] = i + 10;};

    p3 = tar_w[0].b = CMAKE_N(double, N_DOUBLE);
    p4 = tar_w[1].b = CMAKE_N(double, N_DOUBLE);
    for (i = 0; i < N_DOUBLE; i++)
        {p3[i] = exp((double) i);
         p4[i] = log(1.0 + (double) i);};

    tar_w[0].c = NULL;
    tar_w[1].c = NULL;

    memset(tar5_t, 0, 4*sizeof(lev1));
    memset(tar5_r, 0, 4*sizeof(lev1));

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* CLEANUP_TEST_5 - free all known test data memory */

static void cleanup_test_5(void)
   {int l;

    if (tar_w != NULL)
       {CFREE(tar_w[0].a);
        CFREE(tar_w[1].a);

        CFREE(tar_w[0].b);
        CFREE(tar_w[1].b);

        CFREE(tar_w);};

    for (l = 0; l < 4; l++)
       {CFREE(tar5_t[l].a);
        CFREE(tar5_t[l].b);

        CFREE(tar5_r[l].a);
        CFREE(tar5_r[l].b);};

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* WRITE_TEST_5_DATA - write out the data into the PDB file */

static void write_test_5_data(PDBfile *strm)
   {

    if (PD_write(strm, "tar(2)", "lev1", tar_w) == 0)
       error(1, STDOUT, "TAR WRITE FAILED - WRITE_TEST_5_DATA\n");

    if (PD_append(strm, "tar(2:3)", tar_w) == 0)
       error(1, STDOUT, "TAR APPEND FAILED - WRITE_TEST_5_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* READ_TEST_5_DATA - read the test data from the file */

static int read_test_5_data(PDBfile *strm)
   {int err; long ind[3];

    err = (PD_read(strm, "tar",    tar5_t) != 4);

#ifdef PDB_LITE
    err = (PD_read(strm, "tar",    tar5_r) != 4);
    err = (PD_read(strm, "tar(0)", tar5_r+0) != 1);
    err = (PD_read(strm, "tar(1)", tar5_r+1) != 1);
#else
    err = (PD_read(strm, "tar(0)", tar5_r+0) != 1);
    err = (PD_read(strm, "tar(1)", tar5_r+1) != 1);
    err = (PD_read(strm, "tar(2)", tar5_r+2) != 1);
    err = (PD_read(strm, "tar(3)", tar5_r+3) != 1);
#endif

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_TEST_5_DATA - print it out to the file */

static void print_test_5_data(FILE *fp)
   {int i;
    int *p0, *p1, *p2, *p3;
    double *p4, *p5, *p6, *p7;

    PRINT(fp, "\n");

    p0 = tar5_t[0].a;
    p1 = tar5_t[1].a;
    p2 = tar5_t[2].a;
    p3 = tar5_t[3].a;
    PRINT(fp, "TAR struct member A:\n");
    PRINT(fp, "    TAR[0].A    TAR[1].A    TAR[2].A    TAR[3].A\n");
    for (i = 0; i < N_INT; i++)
        PRINT(fp, "        %d          %d        %d          %d\n",
	      p0[i], p1[i], p2[i], p3[i]);
    PRINT(fp, "\n");

    p4 = tar5_t[0].b;
    p5 = tar5_t[1].b;
    p6 = tar5_t[2].b;
    p7 = tar5_t[3].b;
    PRINT(fp, "TAR struct member B:\n");
    PRINT(fp, "    TAR[0].B    TAR[1].B    TAR[2].B    TAR[3].B\n");
    for (i = 0; i < N_DOUBLE; i++)
        PRINT(fp, "    %f    %f    %f    %f\n",
	      p4[i], p5[i], p6[i], p7[i]);
    PRINT(fp, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_5_DATA - compare the test data */

static int compare_test_5_data(PDBfile *strm, FILE *fp)
   {int i, l, *p1w, *p2w, *p1r, *p2r;
    int err, err_tot;
    double *p3w, *p4w, *p3r, *p4r;
    long double fptol[N_PRIMITIVE_FP];

    PD_fp_toler(strm, fptol);

    err_tot = TRUE;

    err = TRUE;
    p1w = tar_w[0].a;
    p2w = tar_w[1].a;
    p3w = tar_w[0].b;
    p4w = tar_w[1].b;

    for (l = 0; l < 2; l++)
        {p1r = tar5_t[2*l].a;
	 p2r = tar5_t[2*l+1].a;
	 for (i = 0; i < N_INT; i++)
	     {err &= (p1w[i] == p1r[i]);
	      err &= (p2w[i] == p2r[i]);};

	 p3r = tar5_t[2*l].b;
	 p4r = tar5_t[2*l+1].b;
	 for (i = 0; i < N_DOUBLE; i++)
	     {err &= DOUBLE_EQUAL(p3w[i], p3r[i]);
	      err &= DOUBLE_EQUAL(p4w[i], p4r[i]);};};

    for (l = 0; l < 2; l++)
        {p1r = tar5_r[2*l].a;
	 p2r = tar5_r[2*l+1].a;
	 for (i = 0; i < N_INT; i++)
	     {err &= (p1w[i] == p1r[i]);
	      err &= (p2w[i] == p2r[i]);};

	 p3r = tar5_r[2*l].b;
	 p4r = tar5_r[2*l+1].b;
	 for (i = 0; i < N_DOUBLE; i++)
	     {err &= DOUBLE_EQUAL(p3w[i], p3r[i]);
	      err &= DOUBLE_EQUAL(p4w[i], p4r[i]);};};

    PRINT(fp, "\n");

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_5 - test the PDBLib functions handling indirections and appends
 *        - read and write structures with alignment difficulties
 *        - tests can be targeted
 */

static int test_5(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file as asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

    prep_test_5_data();

    if (read_only == FALSE)

/* create the named file */
       {strm = PD_create(datfile);
	if (strm == NULL)
	   error(2, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

	strm->previous_file = CSTRSAVE(base);

/* make a few defstructs */
	PD_defstr(strm, "lev1",
		  "integer *a",
		  "double *b",
		  "char *c",
		  LAST);

/* write the test data */
	write_test_5_data(strm);

/* close the file */
	if (PD_close(strm) == FALSE)
	   error(2, fp, "Test couldn't close file %s\r\n", datfile);
	PRINT(fp, "File %s closed\n", datfile);};

/* reopen the file */
    strm = PD_open(datfile, "r");
    if (strm == NULL)
       error(2, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s opened\n", datfile);

/* read the structs */
    read_test_5_data(strm);

/* compare the original data with that read in */
    err = compare_test_5_data(strm, fp);

/* print out the results */
    print_test_5_data(fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(2, fp, "Test couldn't close file %s\r\n", datfile);
    PRINT(fp, "File %s closed\n", datfile);

/* clean up test data memory */
    cleanup_test_5();

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/

/*                            TEST #6 ROUTINES                              */

/*--------------------------------------------------------------------------*/

/* PREP_TEST_6_DATA - prepare the test data */

static void prep_test_6_data(void)
   {int i, n;
    double *p1, *p2;

    n = 10;

    d61_w    = CMAKE(st61);
    d61_w->n = n;

    d62_w    = CMAKE(st62);
    d62_w->n = n;
    d62_w->a = CMAKE_N(double, n);

    p1 = d61_w->a;
    p2 = d62_w->a;
    for (i = 0; i < n; i++)
        {p1[i] = 0.1*((double) i);
         p2[i] = -0.1*((double) i);};

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* CLEANUP_TEST_6 - free all known test data memory */

static void cleanup_test_6(void)
   {

    if (d61_w != NULL)
       CFREE(d61_w);

    if (d62_w != NULL)
       {CFREE(d62_w->a);
	CFREE(d62_w);};

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* WRITE_TEST_6_DATA - write out the data into the PDB file */

static void write_test_6_data(PDBfile *strm)
   {

    if (PD_write(strm, "d61", "st61", d61_w) == 0)
       error(1, STDOUT, "D61 WRITE FAILED - WRITE_TEST_6_DATA\n");

    if (PD_write(strm, "d62", "st62", d62_w) == 0)
       error(1, STDOUT, "D62 WRITE FAILED - WRITE_TEST_6_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* READ_TEST_6_DATA - read the test data from the file */

static int read_test_6_data(PDBfile *strm)
   {int err;

    err = PD_read_as_dwim(strm, "d61.a", "float", 10, d61_a);
    err = PD_read_as_dwim(strm, "d62.a", "float", 10, d62_a);
    err = PD_read_as_dwim(strm, "d62.a", "float",  8, d62_s);

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_TEST_6_DATA - print it out to the file */

static void print_test_6_data(FILE *fp)
   {int i, n;

    n = d61_w->n;

    PRINT(fp, "\n");

    PRINT(fp, "D61 struct member A:\n");
    for (i = 0; i < n; i++)
        PRINT(fp, "%3d %5.2f\n", i, d61_a[i]);
    PRINT(fp, "\n");

    PRINT(fp, "D62 struct member A:\n");
    for (i = 0; i < n; i++)
        PRINT(fp, "%3d %5.2f\n", i, d62_a[i]);
    PRINT(fp, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_6_DATA - compare the test data */

static int compare_test_6_data(PDBfile *strm, FILE *fp)
   {int i, n;
    int err, err_tot;
    float fc1, fc2;
    double *p1w, *p2w;
    long double fptol[N_PRIMITIVE_FP];

    PD_fp_toler(strm, fptol);

    err_tot = TRUE;

    err = TRUE;
    p1w = d61_w->a;
    p2w = d62_w->a;

    n = d61_w->n;

    for (i = 0; i < n; i++)
        {fc1 = p1w[i];
	 fc2 = p2w[i];
	 err &= FLOAT_EQUAL(fc1, d61_a[i]);
	 err &= FLOAT_EQUAL(fc2, d62_a[i]);};

    err_tot &= err;

    n -= 2;
    for (i = 0; i < n; i++)
        {fc2 = p2w[i];
	 err &= FLOAT_EQUAL(fc2, d62_s[i]);};

    err_tot &= err;

    PRINT(fp, "\n");

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_6 - test the PDBLib function PD_read_as_dwim */

static int test_6(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file as asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

    prep_test_6_data();

    if (read_only == FALSE)

/* create the named file */
       {strm = PD_create(datfile);
	if (strm == NULL)
	   error(2, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

/* make a few defstructs */
	PD_defstr(strm, "st61",
		  "int n",
		  "double a[10]",
		  LAST);

	PD_defstr(strm, "st62",
		  "int n",
		  "double *a",
		  LAST);

/* write the test data */
	write_test_6_data(strm);

/* close the file */
	if (PD_close(strm) == FALSE)
	   error(2, fp, "Test couldn't close file %s\r\n", datfile);
	PRINT(fp, "File %s closed\n", datfile);};

/* reopen the file */
    strm = PD_open(datfile, "r");
    if (strm == NULL)
       error(2, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s opened\n", datfile);

/* read the structs */
    read_test_6_data(strm);

/* compare the original data with that read in */
    err = compare_test_6_data(strm, fp);

/* print out the results */
    print_test_6_data(fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(2, fp, "Test couldn't close file %s\r\n", datfile);
    PRINT(fp, "File %s closed\n", datfile);

/* clean up test data memory */
    cleanup_test_6();

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/

/*                            TEST #7 ROUTINES                              */

/*--------------------------------------------------------------------------*/

/* PREP_TEST_7_DATA - prepare the test data */

static void prep_test_7_data(void)
   {int i, n;

    n = 10;

    d71_w.a      = CMAKE_N(double, n);
    d71_w_save.a = CMAKE_N(double, n);
    d72_w.a      = CMAKE_N(double, n);
  
    d71_w.n      = n;
    d71_w_save.n = n;
    d72_w.n      = n;

    for (i = 0; i < n; i++)
        {d71_w.a[i] = d71_w_save.a[i] = (double)i;
         d72_w.a[i] = (double)i * 10.0;}

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* CLEANUP_TEST_7 - free all known test data memory */

static void cleanup_test_7(void)
   {

    CFREE(d71_w.a);
    CFREE(d71_w_save.a);
    CFREE(d72_w.a);
    CFREE(d71_r.a);
    CFREE(d72_r.a);

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* WRITE_TEST_7_DATA - write out the data into the PDB file */

static void write_test_7_data(PDBfile *strm)
   {int i;

    if (PD_write(strm, "d71", "st62", &d71_w) == 0)
       error(1, STDOUT, "D71 WRITE FAILED - WRITE_TEST_7_DATA\n");

/* reuse an area of memory to test PD_TRACK_POINTERS */
    for (i = 0; i < d71_w.n; i++)
        d71_w.a[i] = (double) i * 10.0;

    if (PD_write(strm, "d72", "st62", &d71_w) == 0)
       error(1, STDOUT, "D72 WRITE FAILED - WRITE_TEST_7_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* READ_TEST_7_DATA - read the test data from the file */

static int read_test_7_data(PDBfile *strm)
   {int err;

    err = PD_read(strm, "d71", &d71_r);
    err = PD_read(strm, "d72", &d72_r);

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_TEST_7_DATA - print it out to the file */

static void print_test_7_data(FILE *fp)
   {int i, n;

    n = d71_w.n;

    PRINT(fp, "\n");

    PRINT(fp, "D71 struct member A:\n");
    for (i = 0; i < n; i++)
        PRINT(fp, "%3d %5.2f\n", i, d71_r.a[i]);
    PRINT(fp, "\n");

    PRINT(fp, "D72 struct member A:\n");
    for (i = 0; i < n; i++)
        PRINT(fp, "%3d %5.2f\n", i, d72_r.a[i]);
    PRINT(fp, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_7_DATA - compare the test data */

static int compare_test_7_data(PDBfile *strm, FILE *fp)
   {int i, n;
    int err, err_tot;
    float fc1, fc2;
    double *p1w, *p2w;
    long double fptol[N_PRIMITIVE_FP];

    PD_fp_toler(strm, fptol);

    err_tot = TRUE;

    err = TRUE;
    p1w = d71_w_save.a;
    p2w = d72_w.a;

    n = d71_w.n;

    for (i = 0; i < n; i++)
        {fc1 = p1w[i];
	 fc2 = p2w[i];
	 err &= FLOAT_EQUAL(fc1, d71_r.a[i]);
	 err &= FLOAT_EQUAL(fc2, d72_r.a[i]);};

    err_tot &= err;

    PRINT(fp, "\n");

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_7 - test the PDBLib pointer tracking */

static int test_7(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file as asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

    prep_test_7_data();

    if (read_only == FALSE)

/* create the named file */
       {strm = PD_create(datfile);
	if (strm == NULL)
	   error(2, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

/* make a few defstructs */
	PD_defstr(strm, "st62",
		  "int n",
		  "double *a",
		  LAST);

/* turn off pointer tracking */
	PD_set_track_pointers(strm, FALSE);

/* write the test data */
	write_test_7_data(strm);

/* close the file */
	if (PD_close(strm) == FALSE)
	   error(2, fp, "Test couldn't close file %s\r\n", datfile);
	PRINT(fp, "File %s closed\n", datfile);};

/* reopen the file */
    strm = PD_open(datfile, "r");
    if (strm == NULL)
       error(2, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s opened\n", datfile);

/* read the structs */
    read_test_7_data(strm);

/* compare the original data with that read in */
    err = compare_test_7_data(strm, fp);

/* print out the results */
    print_test_7_data(fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(2, fp, "Test couldn't close file %s\r\n", datfile);
    PRINT(fp, "File %s closed\n", datfile);

/* test isfile method */
    if (!SC_isfile(datfile))
       error(2, fp, "SC_isfile failed for %s\r\n", datfile);

    if (SC_isfile("/.foobar-gorfo"))
       error(2, fp, "SC_isfile succeeded for /.foobar-gorfo but should have failed.");

/* clean up test data memory */
    cleanup_test_7();

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/

/*                            TEST #8 ROUTINES                              */

/*--------------------------------------------------------------------------*/

/* PREP_TEST_8_DATA - prepare the test data */

static void prep_test_8_data(void)
   {int i, j, n, m;

    n = 10;
    m = 2*n;

    d8_w  = CMAKE_N(double, n);
    d8a_r = CMAKE_N(double, m);
    d8b_r = CMAKE_N(double, m);
    d8c_r = CMAKE_N(double, m);
    d8d_r = CMAKE_N(double, m);

    for (i = 0; i < n; i++)
        {j = i + 1;
	 d8_w[i] = (double) (j * j);};

    for (i = 0; i < m; i++)
        {d8a_r[i] = 0.0;
         d8b_r[i] = 0.0;
         d8c_r[i] = 0.0;
         d8d_r[i] = 0.0;};

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* CLEANUP_TEST_8 - free all known test data memory */

static void cleanup_test_8(void)
   {

    CFREE(d8_w);
    CFREE(d8a_r);
    CFREE(d8b_r);
    CFREE(d8c_r);
    CFREE(d8d_r);

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* WRITE_TEST_8_DATA - write out the data into the PDB file */

static void write_test_8_data(PDBfile *strm)
   {long ind[4];

    if (PD_write(strm, "d8a[10]", "double", d8_w) == 0)
       error(1, STDOUT, "D8A WRITE FAILED - WRITE_TEST_8_DATA\n");

#ifdef PDB_LITE
    ind[0] = 1L;
    ind[1] = 10L;
    if (PD_defent_alt(strm, "d8b", "double", 1, ind) == 0)
#else
    if (PD_defent(strm, "d8b[1,10]", "double") == 0)
#endif
       error(1, STDOUT, "D8B DEFENT FAILED - WRITE_TEST_8_DATA\n");

    if (PD_write(strm, "d8b[0,0:9]", "double", d8_w) == 0)
       error(1, STDOUT, "D8B WRITE FAILED - WRITE_TEST_8_DATA\n");

#ifdef PDB_LITE
    ind[0] = 0L;
    ind[1] = 10L;
    if (PD_defent_alt(strm, "d8c", "double", 1, ind) == 0)
#else
    if (PD_defent(strm, "d8c[0,10]", "double") == 0)
#endif
       error(1, STDOUT, "D8C DEFENT FAILED - WRITE_TEST_8_DATA\n");

    ind[0] = 1L;
    ind[1] = 0L;
    ind[2] = 0L;
    ind[3] = 9L;
    if (PD_defent_alt(strm, "d8d", "double", 2, ind) == 0)
       error(1, STDOUT, "D8D DEFENT FAILED - WRITE_TEST_8_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* APPEND_TEST_8_DATA - append to the data in the PDB file */

static void append_test_8_data(PDBfile *strm)
   {

    if (PD_append(strm, "d8a[10:19]", d8_w) == 0)
       error(1, STDOUT, "D8A APPEND FAILED - WRITE_TEST_8_DATA\n");

#ifndef PDB_LITE
    if (PD_append(strm, "d8b[1:1,0:9]", d8_w) == 0)
       error(1, STDOUT, "D8B APPEND FAILED - WRITE_TEST_8_DATA\n");
#endif

    if (PD_append(strm, "d8c[0:0,0:9]", d8_w) == 0)
       error(1, STDOUT, "D8C APPEND FAILED - WRITE_TEST_8_DATA\n");

    if (PD_append(strm, "d8d[0:0,0:9]", d8_w) == 0)
       error(1, STDOUT, "D8D APPEND FAILED - WRITE_TEST_8_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* READ_TEST_8_DATA - read the test data from the file */

static int read_test_8_data(PDBfile *strm)
   {int err;

    err = PD_read(strm, "d8a", d8a_r);
    err = PD_read(strm, "d8b", d8b_r);
    err = PD_read(strm, "d8c", d8c_r);
    err = PD_read(strm, "d8d", d8d_r);

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_TEST_8_DATA - print it out to the file */

static void print_test_8_data(FILE *fp)
   {int i, n;

    n = 20;

    PRINT(fp, "\n");

    PRINT(fp, "D8A:\n");
    for (i = 0; i < n; i++)
        PRINT(fp, "%3d  %11.4e %11.4e %11.4e\n",
	      i, d8a_r[i], d8b_r[i], d8c_r[i]);
    PRINT(fp, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_8_DATA - compare the test data */

static int compare_test_8_data(PDBfile *strm, FILE *fp)
   {int i, n;
    int err, err_tot;
    double wc, arc, brc, crc, drc;
    long double fptol[N_PRIMITIVE_FP];

    PD_fp_toler(strm, fptol);

    err_tot = TRUE;
    err     = TRUE;
    n       = 10;

    for (i = 0; i < n; i++)
        {wc  = d8_w[i];
	 arc = d8a_r[i];
	 brc = d8b_r[i];
	 crc = d8c_r[i];
	 drc = d8d_r[i];

	 err &= DOUBLE_EQUAL(wc, arc);
	 err &= DOUBLE_EQUAL(wc, brc);
	 err &= DOUBLE_EQUAL(wc, crc);
	 err &= DOUBLE_EQUAL(wc, drc);};

    for (i = 0; i < n; i++)
        {wc  = d8_w[i];
	 arc = d8a_r[n+i];
	 brc = d8b_r[n+i];

	 err &= DOUBLE_EQUAL(wc, arc);
	 err &= DOUBLE_EQUAL(wc, brc);};

    err_tot &= err;

    PRINT(fp, "\n");

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_8 - test the PDBLib defent and append logic */

static int test_8(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file as asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

    prep_test_8_data();

    if (read_only == FALSE)

/* create the named file */
       {strm = PD_create(datfile);
	if (strm == NULL)
	   error(2, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

/* write the test data */
	write_test_8_data(strm);

/* append the test data */
	append_test_8_data(strm);

/* close the file */
	if (PD_close(strm) == FALSE)
	   error(2, fp, "Test couldn't close file %s\r\n", datfile);
	PRINT(fp, "File %s closed\n", datfile);};

/* reopen the file */
    strm = PD_open(datfile, "r");
    if (strm == NULL)
       error(2, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s opened\n", datfile);

/* read the structs */
    read_test_8_data(strm);

/* compare the original data with that read in */
    err = compare_test_8_data(strm, fp);

/* print out the results */
    print_test_8_data(fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(2, fp, "Test couldn't close file %s\r\n", datfile);
    PRINT(fp, "File %s closed\n", datfile);

/* clean up test data memory */
    cleanup_test_8();

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_9 - test the PDBLib checksum logic */

static int test_9(char *base, char *tgt, int n)
   {int err, status, var;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *file;
    FILE *fp;

    err = TRUE;
    var = 42;
  
/* target the file as asked */
    test_target(tgt, base, n, fname, datfile);

    fp = io_open(fname, "w");

    if (tgt != NULL)
       snprintf(datfile, MAXLINE, "%s-%s.db9a", base, tgt);
    else
       snprintf(datfile, MAXLINE, "%s-nat.db9a", base);

    if (read_only == FALSE)

/* create the named file */
       {file = PD_create(datfile);
	if (file == NULL)
	   error(2, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

/* turn on the checksum logic */
	PD_activate_cksum(file, PD_MD5_FILE);
	PD_write(file, "test", "int", &var);
	PD_close(file);};

/* reopen the file and see if the checksum is valid */
    file = PD_open(datfile, "a");
    if (file == NULL)
       error(2, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s reopened\n", datfile);

    PD_activate_cksum(file, PD_MD5_FILE);

/* make sure the checksum checks out */
    status = PD_verify(file);
   
    if (status != TRUE)
       err = FALSE;
    else
       PRINT(fp, "File %s verified\n", datfile);

    PD_close(file);

    if (tgt != NULL)
       snprintf(datfile, MAXLINE, "%s-%s.db9b", base, tgt);
    else
       snprintf(datfile, MAXLINE, "%s-nat.db9b", base);

    if (read_only == FALSE)

/* do it again, except this time, don't activate the checksum
 * create the named file
 */
       {file = PD_create(datfile);
	if (file == NULL)
	   error(2, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

/* leave the checksum logic off */
	PD_write(file, "test", "int", &var);
	PD_close(file);};

/* reopen the file and see if the checksum is valid */
    file = PD_open(datfile, "a");
    if (file == NULL)
       error(2, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s reopened\n", datfile);

    PD_activate_cksum(file, PD_MD5_FILE);

/* make sure it returns -1 meaning integrity cannot be determined */
    status = PD_verify(file);
   
    if (status != -1)
       err = FALSE;
    else
       PRINT(fp, "File %s cannot be verified (as expected)\n", datfile);

    PD_close(file);

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/

/*                            TEST #10 ROUTINES                             */

#ifdef HAVE_ANSI_FLOAT16

#define QUAD_EQUAL(q1, q2) (PM_qvalue_compare(q1, q2, fptol[2]) == 0)

/*--------------------------------------------------------------------------*/

/* PREP_TEST_10_DATA - prepare the test data */

static void prep_test_10_data(void)
   {int i;

/* set scalars */
    qs_r = 0.0L;
    qs_w = expl(1.0);
    qs_w = 2.0L;

/* set long double array */
    for (i = 0; i < N_FLOAT; i++)
        {qa_w[i] = POWL(qs_w, (long double) (i+1));
         qa_r[i] = 0.0L;};

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* WRITE_TEST_10_DATA - write out the data into the PDB file */

static void write_test_10_data(PDBfile *strm)
   {long ind[6];

/* write scalars into the file */
    if (PD_write(strm, "qs", "long_double",  &qs_w) == 0)
       error(1, STDOUT, "QS WRITE FAILED - WRITE_TEST_10_DATA\n");

/* write primitive arrays into the file */
    ind[0] = 0L;
    ind[1] = N_FLOAT - 1;
    ind[2] = 1L;
    if (PD_write_alt(strm, "qa", "long_double", qa_w, 1, ind) == 0)
       error(1, STDOUT, "DA WRITE FAILED - WRITE_TEST_10_DATA\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* READ_TEST_10_DATA - read the test data from the file */

static int read_test_10_data(PDBfile *strm)
   {int err;

/* read the scalar data from the file */
    err = PD_read(strm, "qs", &qs_r);

/* read the primitive arrays from the file */
    err = PD_read(strm, "qa",  qa_r);

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_TEST_10_DATA - print it out to STDOUT */

static void print_test_10_data(FILE *fp)
   {int i;

    PRINT(fp, "long double scalar:  qs = %14.6Le\n", qs_r);

/* print long double array */
    PRINT(fp, "long double array:\n");
    for (i = 0; i < N_FLOAT; i++)
        PRINT(fp, "  qa[%d] = %14.6Le\n", i, qa_r[i]);

    PRINT(fp, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* COMPARE_TEST_10_DATA - compare the test data */

static int compare_test_10_data(PDBfile *strm, FILE *fp)
   {int i, err, err_tot;
    long double fptol[N_PRIMITIVE_FP];

    PD_fp_toler(strm, fptol);

    err_tot = TRUE;

/* compare scalars */
    err  = TRUE;

    err &= QUAD_EQUAL(qs_r, qs_w);

    if (err)
       PRINT(fp, "Scalars compare\n");
    else
       PRINT(fp, "Scalars differ\n");
    err_tot &= err;

/* compare long double array */
    for (i = 0; i < N_FLOAT; i++)
        err &= QUAD_EQUAL(qa_r[i], qa_w[i]);

    if (err)
       PRINT(fp, "Arrays compare\n");
    else
       PRINT(fp, "Arrays differ\n");
    err_tot &= err;

    return(err_tot);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* TEST_10 - test the long double support in PDBLib
 *         -
 *         - read and write scalars and arrays of long doubles
 *         -
 *         - tests can be targeted
 */

static int test_10(char *base, char *tgt, int n)
   {int err;
    char datfile[MAXLINE], fname[MAXLINE];
    PDBfile *strm;
    FILE *fp;

/* target the file as asked */
    test_target(tgt, base, 100+n, fname, datfile);

    fp = io_open(fname, "w");

    prep_test_10_data();

    if (read_only == FALSE)

/* create the named file */
       {strm = PD_create(datfile);
	if (strm == NULL)
	   error(1, fp, "Test couldn't create file %s\r\n", datfile);
	PRINT(fp, "File %s created\n", datfile);

/* write the test data */
	write_test_10_data(strm);

/* close the file */
	if (PD_close(strm) == FALSE)
	   error(1, fp, "Test couldn't close file %s\r\n", datfile);
	PRINT(fp, "File %s closed\n", datfile);};

/* reopen the file */
    strm = PD_open(datfile, "r");
    if (strm == NULL)
       error(1, fp, "Test couldn't open file %s\r\n", datfile);
    PRINT(fp, "File %s opened\n", datfile);

/* dump the symbol table */
    dump_test_symbol_table(fp, strm->symtab, 1);

/* read the data from the file */
    read_test_10_data(strm);

/* compare the original data with that read in */
    err = compare_test_10_data(strm, fp);

/* close the file */
    if (PD_close(strm) == FALSE)
       error(1, fp, "Test couldn't close file %s\r\n", datfile);
    PRINT(fp, "File %s closed\n", datfile);

/* print it out to STDOUT */
    print_test_10_data(fp);

    io_close(fp);
    if (err)
       REMOVE(fname);

    return(err);}

/*--------------------------------------------------------------------------*/

#else

/*--------------------------------------------------------------------------*/

/* TEST_10 - stub for when there is no long double support */

static int test_10(char *base, char *tgt, int n)
   {

    return(TRUE);}

/*--------------------------------------------------------------------------*/

#endif

/*                              DRIVER ROUTINES                             */

/*--------------------------------------------------------------------------*/

/* RUN_TEST - run a particular test through all targeting modes
 *          - return the number of tests that fail
 */

static int run_test(PFTest test, int n, char *host, int native)
   {int i, m, rv, cs, fail;
    int64_t bytaa=0, bytfa=0, bytab=0, bytfb=0;
    char msg[MAXLINE];
    char *nm;
    double time;
    static int dbg = 0;

    if (debug_mode)
       dbg = 2;

/* NOTE: under the debugger set dbg to 1 or 2 for additional
 *       memory leak monitoring
 */
    cs = SC_mem_monitor(-1, dbg, "B", msg);

    SC_mem_stats(&bytab, &bytfb, NULL, NULL);

    time = SC_wall_clock_time();

    fail = 0;

    if (native == FALSE)
       {m = PD_target_n_platforms();
	for (i = 0; i < m; i++)
	    {rv = PD_target_platform_n(i);
	     SC_ASSERT(rv == TRUE);

	     nm = PD_target_platform_name(i);
	     if ((*test)(host, nm, n) == FALSE)
	        {PRINT(STDOUT, "Test #%d %s failed\n", n, nm);
		 fail++;};};};

/* native test */
    if ((*test)(host, NULL, n) == FALSE)
       {PRINT(STDOUT, "Test #%d native failed\n", n);
	fail++;};

    SC_mem_stats(&bytaa, &bytfa, NULL, NULL);

    bytaa -= bytab;
    bytfa -= bytfb;
    time   = SC_wall_clock_time() - time;

    cs = SC_mem_monitor(cs, dbg, "B", msg);

#if defined PRId64
    PRINT(STDOUT,
          "\t\t     %3d    %8" PRId64 "  %8" PRI64d "   %7" PRI64d "     %.2g\n",
          n, bytaa, bytfa, bytaa - bytfa, time);
#else
    PRINT(STDOUT,
          "\t\t     %3d    %8lld  %8lld   %7lld     %.2g\n",
          n, (long long) bytaa, (long long) bytfa, (long long) bytaa - bytfa, time);
#endif

    return(fail);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* PRINT_HELP - print a help message */

static void print_help(void)
   {

    PRINT(STDOUT, "\nPDBTST - run basic PDB test suite\n\n");
    PRINT(STDOUT, "Usage: pdbtst [-b #] [-c] [-d] [-h] [-n] [-r] [-v #] [-0] [-1] [-2] [-3] [-4] [-5] [-6] [-7] [-8] [-9] [-10]\n");
    PRINT(STDOUT, "\n");
    PRINT(STDOUT, "       b  - set buffer size (default no buffering)\n");
    PRINT(STDOUT, "       c  - verify low level writes\n");
    PRINT(STDOUT, "       d  - turn on debug mode to display memory maps\n");
    PRINT(STDOUT, "       h  - print this help message and exit\n");
    PRINT(STDOUT, "       n  - run native mode test only\n");
    PRINT(STDOUT, "       r  - read only (assuming files from other run exist)\n");
    PRINT(STDOUT, "       v  - use format version # (default is 2)\n");
    PRINT(STDOUT, "       0  - do NOT run test #0\n");
    PRINT(STDOUT, "       1  - do NOT run test #1\n");
    PRINT(STDOUT, "       2  - do NOT run test #2\n");
    PRINT(STDOUT, "       3  - do NOT run test #3\n");
    PRINT(STDOUT, "       4  - do NOT run test #4\n");
    PRINT(STDOUT, "       5  - do NOT run test #5\n");
    PRINT(STDOUT, "       6  - do NOT run test #6\n");
    PRINT(STDOUT, "       7  - do NOT run test #7\n");
    PRINT(STDOUT, "       8  - do NOT run test #8\n");
    PRINT(STDOUT, "       9  - do NOT run test #9\n");
    PRINT(STDOUT, "       10 - do NOT run test #10\n");
    PRINT(STDOUT, "\n");

    return;}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* MAIN - test the PDB Library system */

int main(int c, char **v)
   {int i, err;
    int test_zero, test_one, test_two, test_three;
    int test_four, test_five, test_six, test_seven;
    int test_eight, test_nine, test_ten;
    int use_mapped_files, check_writes;
    int64_t bfsz;

    setvbuf(STDOUT, 0, _IONBF, 0);

    if (chdir(DATDIR) == 0)
    {
        DIR *ddir;
        struct dirent *dent;

        ddir = opendir(".");
        while ((dent = readdir(ddir)))
        {
            if (dent->d_name[0] == '.') continue;
            SC_ASSERT(REMOVE(dent->d_name)==0);
        }
        closedir(ddir);
        SC_ASSERT(chdir("..")==0);
        SC_ASSERT(rmdir(DATDIR)==0);
    }
    SC_ASSERT(mkdir(DATDIR,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)==0);
    SC_ASSERT(chdir(DATDIR)==0);

    PD_init_threads(0, NULL);

    SC_bf_set_hooks();
    SC_zero_space_n(1, -2);

    bfsz             = -1;
    bfsz             = 100000;
    check_writes     = FALSE;
    debug_mode       = FALSE;
    native_only      = FALSE;
    read_only        = FALSE;
    use_mapped_files = FALSE;
    test_zero        = TRUE;
    test_one         = TRUE;
    test_two         = TRUE;
    test_three       = TRUE;
    test_four        = TRUE;
    test_five        = TRUE;
    test_six         = TRUE;
    test_seven       = TRUE;
    test_eight       = TRUE;
    test_nine        = TRUE;
    test_ten         = TRUE;
#ifdef PDB_LITE
    test_six         = FALSE; /* PDB Lite has no read_as_dwim method */
    test_eight       = FALSE; /* PDB Lite can't handle the append modes */
    test_nine        = FALSE; /* PDB Lite doesn't have checksums */
#endif
    for (i = 1; i < c; i++)
        {if (v[i][0] == '-')
            {switch (v[i][1])
                {case 'b' :
		      bfsz = SC_stoi(v[++i]);
		      break;
                 case 'c' :
		      check_writes = TRUE;
		      break;
		 case 'd' :
		      debug_mode  = TRUE;
		      break;
                 case 'h' :
		      print_help();
		      return(1);
                 case 'm' :
		      use_mapped_files = TRUE;
		      break;
                 case 'n' :
		      native_only = TRUE;
		      break;
                 case 'r' :
		      read_only = TRUE;
		      break;
                 case 'v' :
                      PD_set_fmt_version(SC_stoi(v[++i]));
		      break;
                 case '0' :
		      test_zero = FALSE;
		      break;
                 case '1' :
		      if (v[i][2] == '0')
			 test_ten = FALSE;
		      else
			 test_one = FALSE;
		      break;
                 case '2' :
		      test_two = FALSE;
		      break;
                 case '3' :
		      test_three = FALSE;
		      break;
                 case '4' :
		      test_four = FALSE;
		      break;
                 case '5' :
		      test_five = FALSE;
		      break;
                 case '6' :
		      test_six = FALSE;
		      break;
                 case '7' :
		      test_seven = FALSE;
		      break;
                 case '8' :
		      test_eight = FALSE;
		      break;
                 case '9' :
		      test_nine = FALSE;
		      break;};}
         else
            break;};

    PD_set_io_hooks(use_mapped_files);
    PD_verify_writes(check_writes);

    PD_set_buffer_size(bfsz);

    SC_signal(SIGINT, SIG_DFL);

/* force allocation of permanent memory outside of memory monitors */
    PD_open_vif("foo");

    PRINT(STDOUT, "\n");
    PRINT(STDOUT, "\t\t                      Memory                Time\n");
    PRINT(STDOUT, "\t\t                     (bytes)               (secs)\n");
    PRINT(STDOUT, "\t\t     Test  Allocated     Freed      Diff\n");

    err = 0;

    if (test_zero)
       err += run_test(test_0, 0, DATFILE, TRUE);
    if (test_one)
       err += run_test(test_1, 1, DATFILE, native_only);
    if (test_two)
       err += run_test(test_2, 2, DATFILE, native_only);
    if (test_three)
       err += run_test(test_3, 3, DATFILE, native_only);
    if (test_four)
       err += run_test(test_4, 4, DATFILE, native_only);
    if (test_five)
       err += run_test(test_5, 5, DATFILE, native_only);
    if (test_six)
       err += run_test(test_6, 6, DATFILE, native_only);
    if (test_seven)
       err += run_test(test_7, 7, DATFILE, native_only);
    if (test_eight)
       err += run_test(test_8, 8, DATFILE, native_only);
    if (test_nine)
       err += run_test(test_9, 9, DATFILE, native_only);
    if (test_ten)
       err += run_test(test_10, 10, DATFILE, native_only);

    PRINT(STDOUT, "\n");

    return(err);}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
