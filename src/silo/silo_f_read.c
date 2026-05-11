/*
 * Experimental Fortran read-side wrappers for Silo.
 *
 * Intended to be compiled alongside src/silo/silo_f.c and to use the same
 * DBFortranAccessPointer()/FCD_DB/FORTRAN wrapper machinery.
 *
 * This file deliberately mirrors the existing write-side Fortran convention:
 *   - dbid is a Fortran integer handle to DBfile *
 *   - names are Fortran strings plus explicit lengths
 *   - caller allocates all output buffers
 *   - wrapper copies C-allocated DBGetXxx() object contents into buffers
 *     only when the corresponding Fortran argument is not DBF77_NULL/NULL
 *   - wrapper uses Silo's data read mask to avoid bulk reads when no
 *     problem-sized arrays were requested
 *   - wrapper frees DBGetXxx() compound object before returning
 *
 * Coverage here is for the common write-side object families whose get-side
 * wrappers are absent or thin in the historical Fortran interface:
 *   DBGetPointmesh, DBGetPointvar
 *   DBGetQuadmesh, DBGetQuadvar (multi-component replacement/superset of DBGETQV1)
 *   DBGetUcdmesh, DBGetUcdvar
 *   DBGetMaterial, DBGetMatspecies
 *   DBGetZonelist, DBGetFacelist
 *
 * Important limitations:
 *   1. String-valued optional fields are not copied here.
 *   2. Multi-component variable wrappers assume component arrays are packed
 *      contiguously in the Fortran buffer, exactly like DBPUTQV_FC does.
 *   3. Caller must provide sufficiently large buffers for any non-null
 *      output arrays. Passing DBF77_NULL/NULL suppresses copying and, when
 *      all bulk arrays are null, suppresses problem-sized I/O via DBNone.
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "silo_private.h"
#include "silo_f.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#define DB_FC_NELTS_3(d, nd) \
    ((d)[0] * (((nd) > 1) ? (d)[1] : 1) * (((nd) > 2) ? (d)[2] : 1))

#define DB_FC_CHECK_SIZE(requested, caller_cap, required_cap, func) \
    do { \
        if ((requested) && (caller_cap) < (required_cap)) \
            API_ERROR(func, E_BADARGS); \
    } while (0)

/* These are defined in silo_f.c. */
extern void *DBFortranAccessPointer(int value);

static char *
db_fstrdup(FCD_DB s, int len)
{
#ifdef CRAY
    char *cp = _fcdtocp(s);
#else
    char *cp = (char *)s;
#endif
    if (len <= 0 || !cp || !strcmp(cp, DB_F77NULLSTRING))
        return NULL;
    return SW_strndup(cp, len);
}

static int
db_type_size(int datatype)
{
    return db_GetMachDataSize(datatype);
}

static int
db_is_f77_null_ptr(const void *p)
{
    /*
     * The historical Silo Fortran interface uses DBF77_NULL as the caller
     * side convention for optional/unused arguments. Depending on compiler
     * and binding details this may arrive as a true NULL-equivalent pointer,
     * or as an address-like sentinel. Prefer the real macro when visible,
     * but always treat C NULL as null too.
     */
    if (p == NULL) return 1;
#ifdef DBF77_NULL
    if (p == (const void *)(intptr_t) DBF77_NULL) return 1;
#endif
#ifdef DB_F77NULL
    if (p == (const void *)(intptr_t) DB_F77NULL) return 1;
#endif
    return 0;
}

static void
db_copy_ints(int *dst, const int *src, int n)
{
    if (!db_is_f77_null_ptr(dst) && src && n > 0)
        memcpy(dst, src, (size_t)n * sizeof(int));
}

static void
db_copy_data(void *dst, const void *src, int datatype, int n)
{
    int sz = db_type_size(datatype);
    if (!db_is_f77_null_ptr(dst) && src && sz > 0 && n > 0)
        memcpy(dst, src, (size_t)n * (size_t)sz);
}

static int
db_any_requested3(const void *a, const void *b, const void *c)
{
    return !db_is_f77_null_ptr(a) || !db_is_f77_null_ptr(b) || !db_is_f77_null_ptr(c);
}

static unsigned long long
db_set_read_mask_for_bulk_request(DBfile *dbfile, int bulk_requested)
{
    unsigned long long oldmask = DBGetDataReadMask2File(dbfile);
    DBSetDataReadMask2File(dbfile, bulk_requested ? DBAll : DBNone);
    return oldmask;
}

static void
db_restore_read_mask(DBfile *dbfile, unsigned long long oldmask)
{
    DBSetDataReadMask2File(dbfile, oldmask);
}

/* -------------------------------------------------------------------------
 * Lightweight inquiry routines. These let Fortran callers ask how large the
 * buffers must be before calling the copying DBGET* wrappers below.
 * ------------------------------------------------------------------------- */

SILO_API FORTRAN
DBINQPM_FC(int *dbid, FCD_DB name, int *lname,
           int *ndims, int *nels, int *datatype)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBpointmesh *pm = NULL;
    API_BEGIN("dbinqpm", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        if (NULL == (pm = DBGetPointmesh(dbfile, nm))) API_ERROR("DBGetPointmesh", E_CALLFAIL);
        *ndims = pm->ndims;
        *nels = pm->nels;
        *datatype = pm->datatype;
        DBFreePointmesh(pm);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBINQQM_FC(int *dbid, FCD_DB name, int *lname,
           int *ndims, int *nspace, int *nnodes, int *dims,
           int *datatype, int *coordtype)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBquadmesh *qm = NULL;
    int i;
    API_BEGIN("dbinqqm", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        if (NULL == (qm = DBGetQuadmesh(dbfile, nm))) API_ERROR("DBGetQuadmesh", E_CALLFAIL);
        *ndims = qm->ndims;
        *nspace = qm->nspace;
        *nnodes = qm->nnodes;
        *datatype = qm->datatype;
        *coordtype = qm->coordtype;
        for (i = 0; i < qm->ndims && i < 3; i++) dims[i] = qm->dims[i];
        DBFreeQuadmesh(qm);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBINQUM_FC(int *dbid, FCD_DB name, int *lname,
           int *ndims, int *nnodes, int *nzones, int *datatype,
           int *has_zones, int *has_faces)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBucdmesh *um = NULL;
    API_BEGIN("dbinqum", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        if (NULL == (um = DBGetUcdmesh(dbfile, nm))) API_ERROR("DBGetUcdmesh", E_CALLFAIL);
        *ndims = um->ndims;
        *nnodes = um->nnodes;
        *nzones = um->zones ? um->zones->nzones : 0;
        *datatype = um->datatype;
        *has_zones = um->zones ? 1 : 0;
        *has_faces = um->faces ? 1 : 0;
        DBFreeUcdmesh(um);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBINQVAR_FC(int *dbid, FCD_DB name, int *lname,
            int *nels, int *nvals, int *ndims, int *dims,
            int *mixlen, int *datatype, int *centering)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    int vtype, i;
    API_BEGIN("dbinqvar", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        vtype = DBInqVarType(dbfile, nm);
        *nels = *nvals = *ndims = *mixlen = *datatype = *centering = 0;
        for (i = 0; i < 3; i++) dims[i] = 0;
        if (vtype == DB_QUADVAR) {
            DBquadvar *qv = DBGetQuadvar(dbfile, nm);
            if (!qv) API_ERROR("DBGetQuadvar", E_CALLFAIL);
            *nels = qv->nels; *nvals = qv->nvals; *ndims = qv->ndims;
            *datatype = qv->datatype; *centering = qv->centering;
            for (i = 0; i < qv->ndims && i < 3; i++) dims[i] = qv->dims[i];
            DBFreeQuadvar(qv);
        } else if (vtype == DB_UCDVAR) {
            DBucdvar *uv = DBGetUcdvar(dbfile, nm);
            if (!uv) API_ERROR("DBGetUcdvar", E_CALLFAIL);
            *nels = uv->nels; *nvals = uv->nvals; *ndims = uv->ndims;
            *mixlen = uv->mixlen; *datatype = uv->datatype; *centering = uv->centering;
            DBFreeUcdvar(uv);
        } else if (vtype == DB_POINTVAR) {
            DBmeshvar *pv = DBGetPointvar(dbfile, nm);
            if (!pv) API_ERROR("DBGetPointvar", E_CALLFAIL);
            *nels = pv->nels; *nvals = pv->nvals; *ndims = pv->ndims;
            *datatype = pv->datatype; *centering = DB_NODECENT;
            DBFreeMeshvar(pv);
        } else {
            API_ERROR("unsupported variable type", E_BADARGS);
        }
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBINQZL_FC(int *dbid, FCD_DB name, int *lname,
           int *nzones, int *ndims, int *lnodelist,
           int *origin, int *nshapes)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBzonelist *zl = NULL;
    API_BEGIN("dbinqzl", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        if (NULL == (zl = DBGetZonelist(dbfile, nm))) API_ERROR("DBGetZonelist", E_CALLFAIL);
        *nzones = zl->nzones; *ndims = zl->ndims; *lnodelist = zl->lnodelist;
        *origin = zl->origin; *nshapes = zl->nshapes;
        DBFreeZonelist(zl);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBINQFL_FC(int *dbid, FCD_DB name, int *lname,
           int *nfaces, int *ndims, int *lnodelist,
           int *origin, int *nshapes, int *ntypes)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBfacelist *fl = NULL;
    API_BEGIN("dbinqfl", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        if (NULL == (fl = DBGetFacelist(dbfile, nm))) API_ERROR("DBGetFacelist", E_CALLFAIL);
        *nfaces = fl->nfaces; *ndims = fl->ndims; *lnodelist = fl->lnodelist;
        *origin = fl->origin; *nshapes = fl->nshapes; *ntypes = fl->ntypes;
        DBFreeFacelist(fl);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBINQMAT_FC(int *dbid, FCD_DB name, int *lname,
            int *nmat, int *mixlen, int *ndims, int *dims,
            int *datatype)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBmaterial *mat = NULL;
    int i;
    API_BEGIN("dbinqmat", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        if (NULL == (mat = DBGetMaterial(dbfile, nm))) API_ERROR("DBGetMaterial", E_CALLFAIL);
        *nmat = mat->nmat; *mixlen = mat->mixlen; *ndims = mat->ndims;
        *datatype = mat->datatype;
        for (i = 0; i < mat->ndims && i < 3; i++) dims[i] = mat->dims[i];
        DBFreeMaterial(mat);
        FREE(nm);
    } API_END;
    return 0;
}

/* -------------------------------------------------------------------------
 * Copying read wrappers.
 * ------------------------------------------------------------------------- */

SILO_API FORTRAN
DBGETPM_FC(int *dbid, FCD_DB name, int *lname, int *ndims,
           void *x, void *y, void *z, int *nels, int *datatype)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBpointmesh *pm = NULL;
    unsigned long long oldmask;
    int bulk_requested;
    API_BEGIN("dbgetpm", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = db_any_requested3(x, y, z);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        pm = DBGetPointmesh(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == pm) API_ERROR("DBGetPointmesh", E_CALLFAIL);
        DB_FC_CHECK_SIZE(bulk_requested, *nels, pm->nels, "DBGetPointmesh");
        *ndims = pm->ndims; *nels = pm->nels; *datatype = pm->datatype;
        if (pm->ndims > 0) db_copy_data(x, pm->coords[0], pm->datatype, pm->nels);
        if (pm->ndims > 1) db_copy_data(y, pm->coords[1], pm->datatype, pm->nels);
        if (pm->ndims > 2) db_copy_data(z, pm->coords[2], pm->datatype, pm->nels);
        DBFreePointmesh(pm);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBGETPV_FC(int *dbid, FCD_DB name, int *lname,
           int *nvars, void *vars, int *nels, int *datatype)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBmeshvar *pv = NULL;
    unsigned long long oldmask;
    int i, nbytes, bulk_requested;
    API_BEGIN("dbgetpv", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = !db_is_f77_null_ptr(vars);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        pv = DBGetPointvar(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == pv) API_ERROR("DBGetPointvar", E_CALLFAIL);
        DB_FC_CHECK_SIZE(bulk_requested, *nvars, pv->nvals, "DBGetPointvar");
        DB_FC_CHECK_SIZE(bulk_requested, *nels, pv->nels, "DBGetPointvar");
        *nvars = pv->nvals; *nels = pv->nels; *datatype = pv->datatype;
        nbytes = pv->nels * db_type_size(pv->datatype);
        if (!db_is_f77_null_ptr(vars) && pv->vals)
            for (i = 0; i < pv->nvals; i++)
                if (pv->vals[i]) memcpy((char *)vars + (size_t)i * (size_t)nbytes, pv->vals[i], (size_t)nbytes);
        DBFreeMeshvar(pv);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBGETPV1_FC(int *dbid, FCD_DB name, int *lname,
            void *var, int *nels, int *datatype)
{
    int nvars = 1;
    return DBGETPV_FC(dbid, name, lname, &nvars, var, nels, datatype);
}

SILO_API FORTRAN
DBGETQM_FC(int *dbid, FCD_DB name, int *lname,
           void *x, void *y, void *z, int *dims, int *ndims,
           int *datatype, int *coordtype)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBquadmesh *qm = NULL;
    unsigned long long oldmask;
    int i, nelts, bulk_requested, capdims[3], capnelts;
    API_BEGIN("dbgetqm", int, -1) {
        capdims[0] = dims ? dims[0] : 0; capdims[1] = dims ? dims[1] : 0; capdims[2] = dims ? dims[2] : 0;
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = db_any_requested3(x, y, z);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        qm = DBGetQuadmesh(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == qm) API_ERROR("DBGetQuadmesh", E_CALLFAIL);
        if (qm->coordtype == DB_COLLINEAR) {
            if (qm->ndims > 0) DB_FC_CHECK_SIZE(bulk_requested, capdims[0], qm->dims[0], "DBGetQuadmesh");
            if (qm->ndims > 1) DB_FC_CHECK_SIZE(bulk_requested, capdims[1], qm->dims[1], "DBGetQuadmesh");
            if (qm->ndims > 2) DB_FC_CHECK_SIZE(bulk_requested, capdims[2], qm->dims[2], "DBGetQuadmesh");
        } else {
            capnelts = DB_FC_NELTS_3(capdims, qm->ndims);
            DB_FC_CHECK_SIZE(bulk_requested, capnelts, qm->nnodes, "DBGetQuadmesh");
        }
        *ndims = qm->ndims; *datatype = qm->datatype; *coordtype = qm->coordtype;
        for (i = 0; i < qm->ndims && i < 3; i++) dims[i] = qm->dims[i];
        if (qm->coordtype == DB_COLLINEAR) {
            if (qm->ndims > 0) db_copy_data(x, qm->coords[0], qm->datatype, qm->dims[0]);
            if (qm->ndims > 1) db_copy_data(y, qm->coords[1], qm->datatype, qm->dims[1]);
            if (qm->ndims > 2) db_copy_data(z, qm->coords[2], qm->datatype, qm->dims[2]);
        } else {
            nelts = qm->nnodes;
            if (qm->ndims > 0) db_copy_data(x, qm->coords[0], qm->datatype, nelts);
            if (qm->ndims > 1) db_copy_data(y, qm->coords[1], qm->datatype, nelts);
            if (qm->ndims > 2) db_copy_data(z, qm->coords[2], qm->datatype, nelts);
        }
        DBFreeQuadmesh(qm);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBGETQV_FC(int *dbid, FCD_DB name, int *lname,
           int *nvars, void *vars, int *dims, int *ndims,
           void *mixvars, int *mixlen, int *datatype, int *centering)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBquadvar *qv = NULL;
    unsigned long long oldmask;
    int i, nbytes, mnbytes, bulk_requested, capdims[3], capnelts;
    API_BEGIN("dbgetqv", int, -1) {
        capdims[0] = dims ? dims[0] : 0; capdims[1] = dims ? dims[1] : 0; capdims[2] = dims ? dims[2] : 0;
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = !db_is_f77_null_ptr(vars) || !db_is_f77_null_ptr(mixvars);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        qv = DBGetQuadvar(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == qv) API_ERROR("DBGetQuadvar", E_CALLFAIL);
        capnelts = DB_FC_NELTS_3(capdims, qv->ndims);
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(vars), *nvars, qv->nvals, "DBGetQuadvar");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(vars), capnelts, qv->nels, "DBGetQuadvar");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(mixvars), *nvars, qv->nvals, "DBGetQuadvar");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(mixvars), *mixlen, qv->mixlen, "DBGetQuadvar");
        *nvars = qv->nvals; *ndims = qv->ndims; *datatype = qv->datatype;
        *centering = qv->centering;
        for (i = 0; i < qv->ndims && i < 3; i++) dims[i] = qv->dims[i];
        nbytes = qv->nels * db_type_size(qv->datatype);
        if (!db_is_f77_null_ptr(vars) && qv->vals)
            for (i = 0; i < qv->nvals; i++)
                if (qv->vals[i]) memcpy((char *)vars + (size_t)i * (size_t)nbytes, qv->vals[i], (size_t)nbytes);
        *mixlen = qv->mixlen;
        if (!db_is_f77_null_ptr(mixvars) && qv->mixvals && qv->mixlen > 0) {
            mnbytes = qv->mixlen * db_type_size(qv->datatype);
            for (i = 0; i < qv->nvals; i++)
                if (qv->mixvals[i]) memcpy((char *)mixvars + (size_t)i * (size_t)mnbytes, qv->mixvals[i], (size_t)mnbytes);
        }
        DBFreeQuadvar(qv);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBGETUM_FC(int *dbid, FCD_DB name, int *lname, int *ndims,
           void *x, void *y, void *z, int *datatype,
           int *nnodes, int *nzones)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBucdmesh *um = NULL;
    unsigned long long oldmask;
    int bulk_requested;
    API_BEGIN("dbgetum", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = db_any_requested3(x, y, z);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        um = DBGetUcdmesh(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == um) API_ERROR("DBGetUcdmesh", E_CALLFAIL);
        DB_FC_CHECK_SIZE(bulk_requested, *nnodes, um->nnodes, "DBGetUcdmesh");
        *ndims = um->ndims; *datatype = um->datatype; *nnodes = um->nnodes;
        *nzones = um->zones ? um->zones->nzones : 0;
        if (um->ndims > 0) db_copy_data(x, um->coords[0], um->datatype, um->nnodes);
        if (um->ndims > 1) db_copy_data(y, um->coords[1], um->datatype, um->nnodes);
        if (um->ndims > 2) db_copy_data(z, um->coords[2], um->datatype, um->nnodes);
        DBFreeUcdmesh(um);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBGETUV_FC(int *dbid, FCD_DB name, int *lname,
           int *nvars, void *vars, int *nels, void *mixvars,
           int *mixlen, int *datatype, int *centering)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBucdvar *uv = NULL;
    unsigned long long oldmask;
    int i, nbytes, mnbytes, bulk_requested;
    API_BEGIN("dbgetuv", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = !db_is_f77_null_ptr(vars) || !db_is_f77_null_ptr(mixvars);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        uv = DBGetUcdvar(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == uv) API_ERROR("DBGetUcdvar", E_CALLFAIL);
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(vars), *nvars, uv->nvals, "DBGetUcdvar");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(vars), *nels, uv->nels, "DBGetUcdvar");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(mixvars), *nvars, uv->nvals, "DBGetUcdvar");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(mixvars), *mixlen, uv->mixlen, "DBGetUcdvar");
        *nvars = uv->nvals; *nels = uv->nels; *datatype = uv->datatype;
        *centering = uv->centering; *mixlen = uv->mixlen;
        nbytes = uv->nels * db_type_size(uv->datatype);
        if (!db_is_f77_null_ptr(vars) && uv->vals)
            for (i = 0; i < uv->nvals; i++)
                if (uv->vals[i]) memcpy((char *)vars + (size_t)i * (size_t)nbytes, uv->vals[i], (size_t)nbytes);
        if (!db_is_f77_null_ptr(mixvars) && uv->mixvals && uv->mixlen > 0) {
            mnbytes = uv->mixlen * db_type_size(uv->datatype);
            for (i = 0; i < uv->nvals; i++)
                if (uv->mixvals[i]) memcpy((char *)mixvars + (size_t)i * (size_t)mnbytes, uv->mixvals[i], (size_t)mnbytes);
        }
        DBFreeUcdvar(uv);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBGETUV1_FC(int *dbid, FCD_DB name, int *lname,
            void *var, int *nels, void *mixvar,
            int *mixlen, int *datatype, int *centering)
{
    int nvars = 1;
    return DBGETUV_FC(dbid, name, lname, &nvars, var, nels, mixvar, mixlen, datatype, centering);
}

SILO_API FORTRAN
DBGETZL_FC(int *dbid, FCD_DB name, int *lname,
           int *nzones, int *ndims, int *nodelist, int *lnodelist,
           int *origin, int *shapesize, int *shapecnt, int *nshapes)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBzonelist *zl = NULL;
    unsigned long long oldmask;
    int bulk_requested;
    API_BEGIN("dbgetzl", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = !db_is_f77_null_ptr(nodelist) || !db_is_f77_null_ptr(shapesize) || !db_is_f77_null_ptr(shapecnt);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        zl = DBGetZonelist(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == zl) API_ERROR("DBGetZonelist", E_CALLFAIL);
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(nodelist), *lnodelist, zl->lnodelist, "DBGetZonelist");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(shapesize) || !db_is_f77_null_ptr(shapecnt), *nshapes, zl->nshapes, "DBGetZonelist");
        *nzones = zl->nzones; *ndims = zl->ndims; *lnodelist = zl->lnodelist;
        *origin = zl->origin; *nshapes = zl->nshapes;
        db_copy_ints(nodelist, zl->nodelist, zl->lnodelist);
        db_copy_ints(shapesize, zl->shapesize, zl->nshapes);
        db_copy_ints(shapecnt, zl->shapecnt, zl->nshapes);
        DBFreeZonelist(zl);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBGETFL_FC(int *dbid, FCD_DB name, int *lname,
           int *nfaces, int *ndims, int *nodelist, int *lnodelist,
           int *origin, int *zoneno, int *shapesize, int *shapecnt,
           int *nshapes, int *types, int *typelist, int *ntypes)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBfacelist *fl = NULL;
    unsigned long long oldmask;
    int bulk_requested;
    API_BEGIN("dbgetfl", int, -1) {
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = !db_is_f77_null_ptr(nodelist) || !db_is_f77_null_ptr(zoneno) || !db_is_f77_null_ptr(shapesize) ||
                         !db_is_f77_null_ptr(shapecnt) || !db_is_f77_null_ptr(types) || !db_is_f77_null_ptr(typelist);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        fl = DBGetFacelist(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == fl) API_ERROR("DBGetFacelist", E_CALLFAIL);
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(nodelist), *lnodelist, fl->lnodelist, "DBGetFacelist");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(zoneno) || !db_is_f77_null_ptr(types), *nfaces, fl->nfaces, "DBGetFacelist");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(shapesize) || !db_is_f77_null_ptr(shapecnt), *nshapes, fl->nshapes, "DBGetFacelist");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(typelist), *ntypes, fl->ntypes, "DBGetFacelist");
        *nfaces = fl->nfaces; *ndims = fl->ndims; *lnodelist = fl->lnodelist;
        *origin = fl->origin; *nshapes = fl->nshapes; *ntypes = fl->ntypes;
        db_copy_ints(nodelist, fl->nodelist, fl->lnodelist);
        db_copy_ints(zoneno, fl->zoneno, fl->nfaces);
        db_copy_ints(shapesize, fl->shapesize, fl->nshapes);
        db_copy_ints(shapecnt, fl->shapecnt, fl->nshapes);
        db_copy_ints(types, fl->types, fl->nfaces);
        db_copy_ints(typelist, fl->typelist, fl->ntypes);
        DBFreeFacelist(fl);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBGETMAT_FC(int *dbid, FCD_DB name, int *lname, FCD_DB meshname,
            int *lmeshname, int *nmat, int *matnos, int *matlist,
            int *dims, int *ndims, int *mix_next, int *mix_mat,
            int *mix_zone, void *mix_vf, int *mixlen, int *datatype)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBmaterial *mat = NULL;
    unsigned long long oldmask;
    int i, bulk_requested, capdims[3], capzones;
    (void)meshname; (void)lmeshname; /* returned meshname string not copied in this version */
    API_BEGIN("dbgetmat", int, -1) {
        capdims[0] = dims ? dims[0] : 0; capdims[1] = dims ? dims[1] : 0; capdims[2] = dims ? dims[2] : 0;
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = !db_is_f77_null_ptr(matnos) || !db_is_f77_null_ptr(matlist) ||
                         !db_is_f77_null_ptr(mix_next) || !db_is_f77_null_ptr(mix_mat) ||
                         !db_is_f77_null_ptr(mix_zone) || !db_is_f77_null_ptr(mix_vf);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        mat = DBGetMaterial(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == mat) API_ERROR("DBGetMaterial", E_CALLFAIL);
        capzones = DB_FC_NELTS_3(capdims, mat->ndims);
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(matnos), *nmat, mat->nmat, "DBGetMaterial");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(matlist), capzones, DB_FC_NELTS_3(mat->dims, mat->ndims), "DBGetMaterial");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(mix_next) || !db_is_f77_null_ptr(mix_mat) ||
                         !db_is_f77_null_ptr(mix_zone) || !db_is_f77_null_ptr(mix_vf),
                         *mixlen, mat->mixlen, "DBGetMaterial");
        *nmat = mat->nmat; *ndims = mat->ndims; *mixlen = mat->mixlen; *datatype = mat->datatype;
        for (i = 0; i < mat->ndims && i < 3; i++) dims[i] = mat->dims[i];
        db_copy_ints(matnos, mat->matnos, mat->nmat);
        db_copy_ints(matlist, mat->matlist, mat->dims[0] * (mat->ndims > 1 ? mat->dims[1] : 1) * (mat->ndims > 2 ? mat->dims[2] : 1));
        db_copy_ints(mix_next, mat->mix_next, mat->mixlen);
        db_copy_ints(mix_mat, mat->mix_mat, mat->mixlen);
        db_copy_ints(mix_zone, mat->mix_zone, mat->mixlen);
        db_copy_data(mix_vf, mat->mix_vf, mat->datatype, mat->mixlen);
        DBFreeMaterial(mat);
        FREE(nm);
    } API_END;
    return 0;
}

SILO_API FORTRAN
DBGETMSP_FC(int *dbid, FCD_DB name, int *lname, FCD_DB matname,
            int *lmatname, int *nmat, int *nmatspec, int *speclist,
            int *dims, int *ndims, int *nspecies_mf, void *species_mf,
            int *mix_speclist, int *mixlen, int *datatype)
{
    char *nm = NULL;
    DBfile *dbfile = NULL;
    DBmatspecies *msp = NULL;
    unsigned long long oldmask;
    int i, nzones, bulk_requested, capdims[3], capzones;
    (void)matname; (void)lmatname; /* returned matname string not copied in this version */
    API_BEGIN("dbgetmsp", int, -1) {
        capdims[0] = dims ? dims[0] : 0; capdims[1] = dims ? dims[1] : 0; capdims[2] = dims ? dims[2] : 0;
        nm = db_fstrdup(name, *lname);
        dbfile = (DBfile *) DBFortranAccessPointer(*dbid);
        bulk_requested = !db_is_f77_null_ptr(nmatspec) || !db_is_f77_null_ptr(speclist) ||
                         !db_is_f77_null_ptr(species_mf) || !db_is_f77_null_ptr(mix_speclist);
        oldmask = db_set_read_mask_for_bulk_request(dbfile, bulk_requested);
        msp = DBGetMatspecies(dbfile, nm);
        db_restore_read_mask(dbfile, oldmask);
        if (NULL == msp) API_ERROR("DBGetMatspecies", E_CALLFAIL);
        capzones = DB_FC_NELTS_3(capdims, msp->ndims);
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(nmatspec), *nmat, msp->nmat, "DBGetMatspecies");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(speclist), capzones, DB_FC_NELTS_3(msp->dims, msp->ndims), "DBGetMatspecies");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(species_mf), *nspecies_mf, msp->nspecies_mf, "DBGetMatspecies");
        DB_FC_CHECK_SIZE(!db_is_f77_null_ptr(mix_speclist), *mixlen, msp->mixlen, "DBGetMatspecies");
        *nmat = msp->nmat; *ndims = msp->ndims; *nspecies_mf = msp->nspecies_mf;
        *mixlen = msp->mixlen; *datatype = msp->datatype;
        for (i = 0; i < msp->ndims && i < 3; i++) dims[i] = msp->dims[i];
        nzones = msp->dims[0] * (msp->ndims > 1 ? msp->dims[1] : 1) * (msp->ndims > 2 ? msp->dims[2] : 1);
        db_copy_ints(nmatspec, msp->nmatspec, msp->nmat);
        db_copy_ints(speclist, msp->speclist, nzones);
        db_copy_data(species_mf, msp->species_mf, msp->datatype, msp->nspecies_mf);
        db_copy_ints(mix_speclist, msp->mix_speclist, msp->mixlen);
        DBFreeMatspecies(msp);
        FREE(nm);
    } API_END;
    return 0;
}
