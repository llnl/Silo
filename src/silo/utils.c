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

#include <assert.h>
#include <limits.h>

#include "silo_private.h"

static double get_frac(int m, int i, int dtype, DBVCP2_t const vfracs)
{
    assert(dtype==DB_FLOAT || dtype==DB_DOUBLE);
    if (dtype == DB_FLOAT)
    {
        float const **ffracs = (float const **) vfracs;
        if (!ffracs[m]) return 0;
        return ffracs[m][i];
    }
    else if (dtype == DB_DOUBLE)
    {
        double const **dfracs = (double const **) vfracs;
        if (!dfracs[m]) return 0;
        return dfracs[m][i];
    }
    return 0;
}

static void put_frac(void *mix_vf, int mixlen, int dtype, double vf)
{
    assert(mix_vf);
    assert(dtype==DB_FLOAT || dtype==DB_DOUBLE);
    assert(0 <= vf && vf <= 1);
    if (dtype == DB_FLOAT)
    {
        float *fmix_vf = (float *) mix_vf;
        fmix_vf[mixlen] = (float) vf;
    }
    else if (dtype == DB_DOUBLE)
    {
        double *dmix_vf = (double *) mix_vf;
        dmix_vf[mixlen] = vf;
    }
}

static
DBmaterial *db_CalcMaterialFromDenseArrays(int narrs, int ndims, int const *dims,
    int const *matnos, int dtype, DBVCP2_t const vfracs)
{
    const int notSet = -INT_MAX;
    int i,m,z,nzones = 1;
    int *matlist = 0, mixlen = 0, *mix_mat = 0;
    int *mix_zone = 0, *mix_next = 0;
    int *matnos_copy = 0;
    void *mix_vf = 0;
    DBmaterial *mat = 0;

    /* copy matnos (we'll need it for returned material object anyways) */
    matnos_copy = (int *) malloc(narrs * sizeof(int));
    if (!matnos_copy) goto cleanup;
    memcpy(matnos_copy, matnos, narrs * sizeof(int));

    /* compute number of zones (and array length) */
    for (i = 0; i < ndims; i++)
        nzones *= dims[i];

    /* allocate and fill matlist array with the 'notSet' value */
    matlist = (int *) malloc(nzones * sizeof(int)); 
    if (!matlist) goto cleanup;

    for (i = 0; i < nzones; i++)
        matlist[i] = notSet;

    /* pre-compute size of mix arrays */
    for (m = 0; m < narrs; m++)
    {
        for (z = 0; z < nzones; z++)
        {
            double vf = get_frac(m, z, dtype, vfracs);
            if (0 < vf && vf < 1)
                mixlen++;
        }
    }

    /* allocate mix arrays */
    mix_vf   = malloc(mixlen * (dtype==DB_FLOAT?sizeof(float):sizeof(double))); 
    if (!mix_vf) goto cleanup;
    mix_mat  = (int *) malloc(mixlen * sizeof(int));
    if (!mix_mat) goto cleanup;
    mix_zone = (int *) malloc(mixlen * sizeof(int));
    if (!mix_zone) goto cleanup;
    mix_next = (int *) malloc(mixlen * sizeof(int));
    if (!mix_next) goto cleanup;

    /* Loop over zones, then materials to ensure this operation
       will faithfully invert the companion operation. */
    mixlen = 0;
    for (z = 0; z < nzones; z++)
    {
        int nmixing = 0;
        for (m = 0; m < narrs; m++)
        {
            double vf = get_frac(m, z, dtype, vfracs);

            if (vf >= 1.0)
            {
                assert(matlist[z] == notSet);
                matlist[z] = matnos[m];
            }
            else if (vf > 0.0)
            {
                nmixing++;
                if (matlist[z] == notSet)
                {
                    /* put the first entry in the list for this zone */
                    matlist[z] = -(mixlen+1); 
                    mix_mat [mixlen] = matnos[m];
                    put_frac(mix_vf, mixlen, dtype, vf);
                    mix_zone[mixlen] = z+1; /* one origin */
                    mix_next[mixlen] = 0;
                }
                else if (matlist[z] < 0)
                {
                    /* walk forward through the list for this zone */
                    int j = -(matlist[z]+1);
                    while (mix_next[j]!=0)
                        j = mix_next[j]-1;

                    /* link up last entry in list with the new entry */
                    mix_next[j] = mixlen+1;

                    /* put in the new entry */
                    mix_mat [mixlen] = matnos[m];
                    put_frac(mix_vf, mixlen, dtype, vf);
                    mix_zone[mixlen] = z+1; /* one origin */
                    mix_next[mixlen] = 0;
                }
                else
                {
                    /* We've encountered a zone with a frac>=1 *AND* another
                       frac > 0. We ignore the frac>0 since we already have
                       marked the zone clean for the frac>=1. We effectively
                       put a placeholder entry into the mix arrays here. */
                    mix_mat [mixlen] = matnos[m];
                    put_frac(mix_vf, mixlen, dtype, vf);
                    mix_zone[mixlen] = z+1; /* one origin */
                    mix_next[mixlen] = 0;
                }
                mixlen++;
            }
        }
        assert(nmixing==0 || nmixing>=2);
    }

    /* create material object to return */
    mat = DBAllocMaterial();
    mat->origin = 0;
    mat->ndims = ndims;
    for (i = 0; i < ndims; i++)
        mat->dims[i] = dims[i];
    mat->nmat = narrs;
    mat->matnos = matnos_copy;
    mat->matlist = matlist;
    mat->datatype = dtype;
    mat->mix_vf = mix_vf;
    mat->mix_next = mix_next;
    mat->mix_mat = mix_mat;
    mat->mix_zone = mix_zone;
    mat->mixlen = mixlen;

    return(mat);

cleanup:

    FREE(matnos_copy);
    FREE(matlist); 
    FREE(mix_vf);
    FREE(mix_mat);
    FREE(mix_zone);
    FREE(mix_next);

    return 0;
}

PUBLIC
DBmaterial *DBCalcMaterialFromDenseArrays(int narrs, int ndims, int const *dims,
    int const *matnos, int dtype, DBVCP2_t const vfracs)
{
    DBmaterial *retval = 0;

    API_BEGIN("DBCalcMaterialFromDenseArrays", DBmaterial*, 0) {
        if (narrs<=0)
            API_ERROR("narrs<=0", E_BADARGS);
        if (ndims<=0)
            API_ERROR("ndims<=0", E_BADARGS);
        if (!dims)
            API_ERROR("dims==0", E_BADARGS);
        if (!matnos)
            API_ERROR("matnos==0", E_BADARGS);
        if (!vfracs)
            API_ERROR("vfracs==0", E_BADARGS);
        retval = db_CalcMaterialFromDenseArrays(narrs, ndims, dims, matnos, dtype, vfracs);
        API_RETURN(retval);
    }
    API_END_NOPOP;
}

PRIVATE
int compar_ints(void const *ia, void const *ib)
{
    if (*((int const *)ia) < *((int const *)ib)) return -1;
    if (*((int const *)ia) > *((int const *)ib)) return  1;
    return 0;
}

/* Binary search for a given material number in the sorted
   list of material numbers. */
PRIVATE
int mat_index(int nmat_nums, int const *mat_nums, int mat_num)
{
    int bot = 0;
    int top = nmat_nums - 1;
    int mid;
    static int last_mat_num = -1;
    static int last_mat_idx = 0;

    /* quick acclerator (see below for reset logic) */
    if (mat_num == last_mat_num)
        return last_mat_idx;

    while (bot <= top)
    {
        mid = (bot + top) >> 1;

        if (mat_num > mat_nums[mid])
            bot = mid + 1;
        else if (mat_num < mat_nums[mid])
            top = mid - 1;
        else
        {
            last_mat_num = mat_num;
            last_mat_idx = mid;
            return mid;
        }
    }

    /* Any real inputs should never get here. But,
       an input to force reset of last num/idx will.
       We want this logic here instead of at top to
       avoid having to test whether to execut it *every*
       lookup. A call of mat_index(0,0,-1) is sufficient
       to cause this reset code to execute. */
    assert(nmat_nums==0 && mat_nums==0 && mat_num==-1);
    last_mat_num = -1;
    last_mat_idx = 0;
    return -1;
}


PRIVATE
int db_CalcDenseArraysFromMaterial(DBmaterial const *mat, int datatype, int *narrs, void ***vfracs)
{
    static char const *me = "db_CalcDenseArraysFromMaterial";
    int i;
    int nzones = 1;
    size_t nzones_sz = 1;
    int etag = E_NOMEM;
    int typesz = (int) sizeof(float);
    int *matnos_sorted=0;
    void **matarrs=0, **matarrs_fixed=0;
    float *pflt;
    double *pdbl;
#ifndef NDEBUG
    float *check_fracs = 0;
#endif

    if (datatype == DB_DOUBLE)
        typesz = (int) sizeof(double);

    if (mat->nmat < 0)
    {
        etag = E_BADARGS;
        goto cleanup;
    }

    matarrs = (void **) calloc(mat->nmat,sizeof(void *));
    if (!matarrs) goto cleanup;
    matarrs_fixed = (void **) calloc(mat->nmat,sizeof(void *));
    if (!matarrs_fixed) goto cleanup;

    for (i = 0; i < mat->ndims; i++)
    {
        if (mat->dims[i] < 0)
        {
            etag = E_BADARGS;
            goto cleanup;
        }
        if ((size_t) mat->dims[i] != 0 && nzones_sz > ((size_t) INT_MAX) / (size_t) mat->dims[i])
        {
            etag = E_BADARGS;
            goto cleanup;
        }
        nzones_sz *= (size_t) mat->dims[i];
    }
    nzones = (int) nzones_sz;

    /* use calloc so vfrac arrays are initialized with zeros */
    for (i = 0; i < mat->nmat; i++)
    {
        matarrs[i] = (void *) calloc(nzones, typesz);
        if (!matarrs[i]) goto cleanup;
    }
#ifndef NDEBUG
    check_fracs = (float *) calloc(nzones, sizeof(float));
    if (!check_fracs) goto cleanup;
#endif

    /* make a copy of matnos and sort it for binary search */
    if ((size_t) mat->nmat > SIZE_MAX / sizeof(int))
    {
        etag = E_BADARGS;
        goto cleanup;
    }
    matnos_sorted = (int *) malloc((size_t) mat->nmat * sizeof(int));
    if (!matnos_sorted) goto cleanup;
    memcpy(matnos_sorted, mat->matnos, (size_t) mat->nmat * sizeof(int));
    qsort(matnos_sorted, mat->nmat, sizeof(int), compar_ints);

    /* reset binary searcher last num/idx state */
    mat_index(0,0,-1);

    for (i = 0; i < nzones; i++)
    {
        if (mat->matlist[i] >= 0) /* clean case */
        {
            int idx = mat_index(mat->nmat, matnos_sorted, mat->matlist[i]);
            switch (datatype)
            {
                case DB_FLOAT: ((float*)(matarrs[idx]))[i] = 1.0; break;
                case DB_DOUBLE: ((double*)(matarrs[idx]))[i] = 1.0; break;
            }
#ifndef NDEBUG
            check_fracs[i] += 1.0;
#endif
        }
        else /* mixing case */
        {
            int mix_idx = -mat->matlist[i] - 1;
            while(mix_idx >= 0)
            {
                int matno = mat->mix_mat[mix_idx];
                int idx = mat_index(mat->nmat, matnos_sorted, matno);
                switch (datatype)
                {
                    case DB_FLOAT:
                        ((float*)matarrs[idx])[i] = ((float*)mat->mix_vf)[mix_idx];
#ifndef NDEBUG
                        check_fracs[i] += ((float*)matarrs[idx])[i];
#endif
                        break;
                    case DB_DOUBLE:
                        ((double*)matarrs[idx])[i] = ((double*)mat->mix_vf)[mix_idx];
#ifndef NDEBUG
                        check_fracs[i] += ((double*)matarrs[idx])[i];
#endif
                        break;
                }
                mix_idx = mat->mix_next[mix_idx] - 1;
            }
        }
    }

#ifndef NDEBUG
    /* sanity check */
    for (i = 0; i < nzones; i++)
        assert(0.999 <= check_fracs[i] && check_fracs[i] < 1.001);
    FREE(check_fracs);
#endif

    /* The matarr arrays are in the wrong order because
       we pre-sorted matnos. We need to undue that now. */
    mat_index(0,0,-1);
    for (i = 0; i < mat->nmat; i++)
        matarrs_fixed[i] = matarrs[mat_index(mat->nmat, matnos_sorted, mat->matnos[i])];

    free(matarrs);
    free(matnos_sorted);

    *narrs = mat->nmat;
    *vfracs = matarrs_fixed; 

    return 0;

cleanup:

    if (matarrs)
    {
        for (i = 0; i < mat->nmat; i++)
            FREE(matarrs[i]);
        FREE(matarrs);
    }
    FREE(matarrs_fixed);
    FREE(matnos_sorted);
#ifndef NDEBUG
    FREE(check_fracs);
#endif
    db_perror(NULL, etag, me);

    return -1;
}

PUBLIC
int DBCalcDenseArraysFromMaterial(DBmaterial const *mat, int datatype, int *narrs, void ***vfracs)
{
    int retval;

    API_BEGIN("DBCalcDenseArraysFromMaterial", int, -1) {
        if (!mat)
            API_ERROR("mat pointer", E_BADARGS);
        if (DBIsEmptyMaterial(mat))
            API_ERROR("Empty DBmaterial object", E_BADARGS);
        if (!((datatype == DB_FLOAT) || (datatype == DB_DOUBLE)))
            API_ERROR("datatype must be DB_FLOAT or DB_DOUBLE", E_BADARGS);
        if (!narrs)
            API_ERROR("narrs pointer", E_BADARGS);
        if (!vfracs)
            API_ERROR("vfracs pointer", E_BADARGS);
        retval = db_CalcDenseArraysFromMaterial(mat, datatype, narrs, vfracs);
        API_RETURN(retval);
    }
    API_END_NOPOP;
}
