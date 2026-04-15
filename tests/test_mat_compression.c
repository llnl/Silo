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
#include "silo.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include "config.h"

#include "std.c"

#define ASSERT(PRED) if(!(PRED)){fprintf(stderr,"Assertion \"%s\" at line %d failed\n",#PRED,__LINE__);abort();}
/* ERROR is a macro on WINDOWS, so rename for this file */
#define tmcERROR(ARGS) { __line__ = __LINE__; print_error ARGS; exit(1); }
#define FREE(x) if ( (x) != NULL) {free(x);(x)=NULL;}

static int __line__ = 0;
static void print_error(char *msg, ...)
{
    va_list ap;
    fprintf(stderr, "Error at line %d: ", __line__);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

static int calc_material_object_size(DBmaterial const *mat)
{
    int i;
    int retval = 0;
    int nzones = 1;

    for (i = 0; i < mat->ndims; i++)
        nzones *= mat->dims[i];

    /* for matlist */
    retval += nzones * sizeof(int);

    /* for mix_mat, mix_zone, mix_next */
    retval += mat->mixlen * (3 * sizeof(int));

    /* for mix_vf */
    retval += mat->mixlen * ((mat->datatype == DB_FLOAT)?sizeof(float):sizeof(double));

    return retval;
}

/******************************************************************************
 * Test various approaches to compressing material data.
 *
 * This test takes as input a filename and a material object name in that
 * file. It will then extract that material object and write it to a new
 * file using any one of a variety of options.
 *
 * Usage and Examples
 *
 * ./test_mat_compression in=<file>:<obj> out=<file>:<obj> extra=<file> \
 *     dense=<int> compress=<string> <driver>
 *
 * in= specifies Silo file name and material object name sep by ':'
 * out= specifies output Silo file name and material object name sep by ':'
 * compress= specify the compression algorithm and parameters string as
 *     input to DBSetCompression().
 * extra= specifies output Silo file with a number of extra objects to
 *     facilitate analysis. This includes the original mesh object, the
 *     unaltered dense volume fractions as mesh variables, the re-constructed
 *     dense volume fractions as mesh variables and the material object
 *     formed from the reconstructed dense volume fractions. This material
 *     object represents what VisIt would 'see' after compression and
 *     decompression.
 * dense=<int> specifies whether to output dense material fractions (def=0)
 * driver a Silo file driver such as DB_HDF5 (default) or DB_PDB or other
 *     HDF5 variants. Note, compression works *only* with HDF5 variants.
 *
 * To extract a material object and put it in its own file...
 *
 *     ./test_mat_compression in=foo.pdb:Material out=bar.h5:Material
 *
 * To extract a material object and store it as dense volume fractions
 * to its own file (no compression)...
 *
 *     ./test_mat_compression in=foo.pdb:Material out=bar.h5:Material dense=1
 * 
 * To extract a material object and store it as dense volume fractions
 * to its own file using GZIP compression level 3...
 *
 *     ./test_mat_compression in=foo.pdb:Material out=bar.h5:Material dense=1\
 *     compress="METHOD=GZIP LEVEL=3"
 *
 * Note the quotes around the space-separated arg to compress option
 *
 * To extract a material object and store it as dense volume fractions
 * to its own file using ZFP ACCURACY mode of 0.001
 *
 *     ./test_mat_compression in=foo.pdb:Material out=bar.h5:Material dense=1\
 *     compress="METHOD=ZFP ACCURACY=0.001"
 *
 * Be aware that only Silo Quadmesh objects support multi-dimensional dense
 * material volume fraction arrays. Ucdmesh objects will be only 1D.
 *
 *****************************************************************************/

int
main(int argc, char *argv[])
{
    int             i;
    DBfile         *infile, *outfile;
    int             driver = DB_HDF5;
    char            ifile[256], imat[256];
    char            ofile[256], omat[256];
    char            efile[256];
    int             ifileSet = 0, ofileSet = 0, efileSet = 0;
    int             outputDense = 0;
    int             show_all_errors = 1;
    DBoptlist      *mat_opts = DBMakeOptlist(10);
    DBmaterial     *mat;
    int             narrs = 0;
    void          **vfracs = 0;
    char          **vfrac_varnames = 0;

    /* Parse command-line */
    for (i=1; i<argc; i++) {
	if (!strncmp(argv[i], "DB_", 3)) {
	    driver = StringToDriver(argv[i]);
        } else if (!strcmp(argv[i], "no-show-all-errors")) {
            show_all_errors = 0;
        } else if (!strncmp(argv[i], "in=", 3)) {
            char *fname = argv[i]+3;
            char *mname = strchr(fname, ':');
	    if (!mname) tmcERROR(("no path to material object in \"%s\"",argv[i]));
            *mname = '\0';
            strncpy(ifile, fname, sizeof(ifile));
            strncpy(imat, mname+1, sizeof(imat));
            ifile[sizeof(ifile)-1] = '\0';
            imat[sizeof(imat)-1] = '\0';
            ifileSet = 1;
        } else if (!strncmp(argv[i], "out=", 4)) {
            char *fname = argv[i]+4;
            char *mname = strchr(fname, ':');
	    if (!mname) tmcERROR(("no path to material object in \"%s\"",argv[i]));
            *mname = '\0';
            strncpy(ofile, fname, sizeof(ofile));
            strncpy(omat, mname+1, sizeof(omat));
            ofile[sizeof(ofile)-1] = '\0';
            omat[sizeof(omat)-1] = '\0';
            ofileSet = 1;
        } else if (!strncmp(argv[i], "extra=", 6)) {
            char *fname = argv[i]+6;
            strncpy(efile, fname, sizeof(efile));
            efileSet = 1;
            outputDense = 1;
        } else if (!strncmp(argv[i], "dense=", 6)) {
            outputDense = (int) strtol(argv[i]+6,0,10);
        } else if (!strncmp(argv[i], "compress=", 9)) {
            DBSetCompression(argv[i]+9);
	} else if (argv[i][0] != '\0') {
	    fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
	}
    }

    if (!ifileSet) tmcERROR(("input file and material object path not specified"));
    if (!ofileSet) tmcERROR(("output file and material object path not specified"));

    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_NONE, NULL);

    infile = DBOpen(ifile, DB_UNKNOWN, DB_READ);
    if (!infile) tmcERROR(("unable to open file \"%s\"\n", ifile));

    mat = DBGetMaterial(infile, imat);
    if (!mat) tmcERROR(("unable to read material object at path \"%s\" in file \"%s\"", imat, ifile));
    if (mat->matcolors)
        DBAddOption(mat_opts, DBOPT_MATCOLORS, mat->matcolors);
    if (mat->matnames)
        DBAddOption(mat_opts, DBOPT_MATNAMES, mat->matnames);

    outfile = DBOpen(ofile, DB_UNKNOWN, DB_APPEND);
    if (!outfile)
    {
        fprintf(stderr, "Appending to \"%s\" failed. Creating anew...\n", ofile);
        outfile = DBCreate(ofile, DB_CLOBBER, DB_LOCAL, "Material compression test", driver);
        if (!outfile) tmcERROR(("unable to open/create file \"%s\"\n", ofile));
    }

    if (outputDense)
    {
        int total_file_bytes = 0;
        int total_mem_bytes = 0;
        int mat_bytes = calc_material_object_size(mat);

        DBCalcDenseArraysFromMaterial(mat, mat->datatype, &narrs, &vfracs);
        ASSERT(narrs = mat->nmat);
        vfrac_varnames = (char **) malloc(narrs * sizeof(char*));
        for (i = 0; i < narrs; i++)
        {
            int nchars;
            vfrac_varnames[i] = (char *) malloc(256*sizeof(char));
            if (mat->matnames && mat->matnames[i])
                nchars = snprintf(vfrac_varnames[i], 256, "%.163s_%.64s_%d_vfracs", omat, mat->matnames[i], mat->matnos[i]); 
            else
                nchars = snprintf(vfrac_varnames[i], 256, "%.227s_%0d_vfracs", omat, mat->matnos[i]); 
            assert(nchars > 0 && nchars < 256);
            DBWrite(outfile, vfrac_varnames[i], vfracs[i], mat->dims, mat->ndims, mat->datatype);
            total_file_bytes += DBGetVarByteLengthInFile(outfile, vfrac_varnames[i]);
            total_mem_bytes += DBGetVarByteLength(outfile, vfrac_varnames[i]);
        }
        printf("Dense Volume Fractions = %d bytes, %d bytes in mem, compression = %g:1\n",
            total_file_bytes, total_mem_bytes, (double) total_mem_bytes / total_file_bytes);
        printf("Dense Volume Fractions = %d bytes; Original material object = %d bytes, compression = %g:1\n",
            total_file_bytes, mat_bytes, (double) mat_bytes / total_file_bytes);
    }
    else
    {
        DBPutMaterial(outfile, omat, "unknown", mat->nmat, mat->matnos, mat->matlist, mat->dims, mat->ndims,
            mat->mix_next, mat->mix_mat, mat->mix_zone, mat->mix_vf, mat->mixlen, mat->datatype, mat_opts);
    }

    /* Output an extra file with mesh vfracs as mesh variables 
     * and reconstructed material object */
    DBSetCompression(0);
    if (efileSet)
    {
        int mtype;
        char imat_copy[256];
        char *pslash;

        DBfile *extrafile = DBOpen(efile, DB_UNKNOWN, DB_APPEND);
        if (!extrafile )
        {
            fprintf(stderr, "Appending to \"%s\" failed. Creating anew...\n", efile);
            extrafile  = DBCreate(efile, DB_CLOBBER, DB_LOCAL, "Material compression test; reconstructed data", driver);
            if (!extrafile ) tmcERROR(("unable to open/create file \"%s\"\n", efile));
        }

        /* copy mesh from input file to extra file */
        memcpy(imat_copy, imat, strlen(imat));
        pslash = strrchr(imat_copy, '/');
        if (pslash)
        {
            *pslash = '\0';
             DBSetDir(infile, imat_copy);
        }
        mtype = DBInqMeshtype(infile, mat->meshname);
        if (mtype == DB_QUADRECT || mtype == DB_QUADCURV)
        {
            char const *coordnames[] = {"X","Y","Z"};
            char meshname[256];
            DBmaterial *cmat;

            DBquadmesh *qm = DBGetQuadmesh(infile, mat->meshname);
            if (!qm) tmcERROR(("unable to read mesh \"%s\" from file \"%s\"", mat->meshname, ifile));

            /* copy the original mesh over */
            DBPutQuadmesh(extrafile, mat->meshname, coordnames, qm->coords,
                qm->dims, qm->nspace, qm->datatype, qm->coordtype, 0);

            /* output original dense data as mesh variables */
            snprintf(meshname, sizeof(meshname), "/%s", mat->meshname);
            DBMkDir(extrafile, "original_dense");
            DBSetDir(extrafile, "original_dense");
            for (i = 0; i < narrs; i++)
            {
                DBPutQuadvar1(extrafile, vfrac_varnames[i], meshname, vfracs[i],
                    mat->dims, mat->ndims, 0, 0, mat->datatype, DB_ZONECENT, 0);
            }
            DBSetDir(extrafile, "..");

            /* read compressed dense and output as material object */
            DBMkDir(extrafile, "compressed_dense");
            DBSetDir(extrafile, "compressed_dense");
            for (i = 0; i < narrs; i++)
            {
                free(vfracs[i]);
                vfracs[i] = DBGetVar(outfile, vfrac_varnames[i]);
                assert(vfracs[i]);
                DBPutQuadvar1(extrafile, vfrac_varnames[i], meshname, vfracs[i],
                    mat->dims, mat->ndims, 0, 0, mat->datatype, DB_ZONECENT, 0);
            }
            DBSetDir(extrafile, "..");

            /* output compressed dense data as material object */
            cmat = DBCalcMaterialFromDenseArrays(narrs, mat->ndims, mat->dims, mat->matnos,
                mat->datatype, vfracs);
            DBPutMaterial(extrafile, omat, mat->meshname, cmat->nmat, cmat->matnos, cmat->matlist,
                cmat->dims, cmat->ndims, cmat->mix_next, cmat->mix_mat, cmat->mix_zone, cmat->mix_vf,
                cmat->mixlen, cmat->datatype, mat_opts);
            DBFreeMaterial(cmat);
        }
        else if (mtype == DB_UCDMESH)
        {
            char const *coordnames[] = {"X","Y","Z"};
            char meshname[256];
            DBmaterial *cmat;
            int nzones = 0;
            DBzonelist *zl = 0;

            DBucdmesh *um = DBGetUcdmesh(infile, mat->meshname);
            if (!um) tmcERROR(("unable to read mesh \"%s\" from file \"%s\"", mat->meshname, ifile));

            zl = um->zones?um->zones:0;
            nzones = zl?zl->nzones:0;
            DBPutUcdmesh(extrafile, mat->meshname, um->ndims, coordnames, um->coords, um->nnodes,
                nzones, "zonelist", 0, um->datatype, 0);

            if (zl)
                DBPutZonelist2(extrafile, "zonelist", nzones, zl->ndims, zl->nodelist, zl->lnodelist,
                    0, 0, 0, zl->shapetype, zl->shapesize, zl->shapecnt, zl->nshapes, 0);

            /* output original dense data as mesh variables */
            snprintf(meshname, sizeof(meshname), "/%s", mat->meshname);
            DBMkDir(extrafile, "original_dense");
            DBSetDir(extrafile, "original_dense");
            for (i = 0; i < narrs; i++)
            {
                DBPutUcdvar1(extrafile, vfrac_varnames[i], meshname, vfracs[i],
                    nzones, 0, 0, mat->datatype, DB_ZONECENT, 0);
            }
            DBSetDir(extrafile, "..");

            /* read compressed dense data and output as mesh variables */
            DBMkDir(extrafile, "compressed_dense");
            DBSetDir(extrafile, "compressed_dense");
            for (i = 0; i < narrs; i++)
            {
                free(vfracs[i]);
                vfracs[i] = DBGetVar(outfile, vfrac_varnames[i]);
                DBPutUcdvar1(extrafile, vfrac_varnames[i], meshname, vfracs[i],
                    nzones, 0, 0, mat->datatype, DB_ZONECENT, 0);
            }
            DBSetDir(extrafile, "..");

            /* output compressed dense data as material object */
            cmat = DBCalcMaterialFromDenseArrays(narrs, mat->ndims, mat->dims, mat->matnos,
                mat->datatype, vfracs);
            DBPutMaterial(extrafile, omat, mat->meshname, cmat->nmat, cmat->matnos, cmat->matlist,
                cmat->dims, cmat->ndims, cmat->mix_next, cmat->mix_mat, cmat->mix_zone, cmat->mix_vf,
                cmat->mixlen, cmat->datatype, mat_opts);
            DBFreeMaterial(cmat);
        }
        else
        {
            tmcERROR(("Don't know how to deal with mesh type %d\n", mtype));
        }
        DBClose(extrafile);
    }

    DBClose(infile);
    DBClose(outfile);

    for (i = 0; i < narrs; i++)
    {
        FREE(vfrac_varnames[i]);
        FREE(vfracs[i]);
    }
    FREE(vfrac_varnames);
    FREE(vfracs);

    DBFreeMaterial(mat);
    DBFreeOptlist(mat_opts);
    CleanupDriverStuff();

    return 0;
}
