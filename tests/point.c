/* Standalone Silo -> self-contained HZIP meshlets HDF5 demonstration.
 * Prototype: fixed-capacity meshlets; float32 coordinates/variables.
 * See README.md for limitations and dependencies.
 */
#include <silo.h>
#include <hdf5.h>
#include <hzip.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#define FILTER_ID 32042
#define MAGIC 0x4d5a4831u
#define VERSION 1u
#define PERM2 0x00002130u
#define PERM3 0x65217430u
#define FAIL(...) do {fprintf(stderr,"error: " __VA_ARGS__); fputc('\n',stderr); goto fail;} while(0)
typedef struct
{
    unsigned char *p;
    size_t n, cap;
}
Bytes;
static int grow(Bytes *b, size_t extra)
{
    if (extra>SIZE_MAX-b->n) return -1;
    size_t need=b->n+extra;
    if (need<=b->cap) return 0;
    size_t cap=b->cap?b->cap:256;
    while (cap<need)
    {
        if (cap>SIZE_MAX/2)
        {
            cap=need;
            break;
        }
        cap*=2;
    }
    void *p=realloc(b->p, cap);
    if (!p) return -1;
    b->p=p;
    b->cap=cap;
    return 0;
}
static int append(Bytes *b, const void *p, size_t n)
{
    if (grow(b, n)) return -1;
    memcpy(b->p+b->n, p, n);
    b->n+=n;
    return 0;
}
static void put32(unsigned char *p, uint32_t v)
{
    for (int i=0; i<4; i++) p[i]=(unsigned char)(v>>(8*i));
}
static uint32_t get32(const unsigned char *p)
{
    return (uint32_t) p[0]|((uint32_t) p[1]<<8) |((uint32_t) p[2]<<16) |((uint32_t) p[3]<<24);
}
static int add32(Bytes *b, uint32_t v)
{
    unsigned char x[4];
    put32(x, v);
    return append(b, x, 4);
}
static int product(size_t a, size_t b, size_t *out)
{
    if (a&&b>SIZE_MAX/a) return -1;
    *out=a*b;
    return 0;
}
typedef struct
{
    unsigned dim, max_nodes, max_zones, fields;
    size_t record_size, off_ids, off_conn, off_vals;
}
Layout;
static int layout_init(Layout *l, unsigned dim, unsigned nodes, unsigned zones, unsigned fields)
{
    size_t a, b, c;
    if ((dim!=2&&dim!=3) || !nodes||!zones||!fields||nodes>INT_MAX||zones>INT_MAX||fields>1024) return -1;
    l->dim=dim;
    l->max_nodes=nodes;
    l->max_zones=zones;
    l->fields=fields;
    l->off_ids=8;
    if (product(nodes, 4, &a) || a>SIZE_MAX-l->off_ids) return -1;
    l->off_conn=l->off_ids+a;
    if (product(zones,(size_t)(1u<<dim) *4, &b) || b>SIZE_MAX-l->off_conn) return -1;
    l->off_vals=l->off_conn+b;
    if (product(fields, nodes, &c) || product(c, 4, &c) || c>SIZE_MAX-l->off_vals) return -1;
    l->record_size=l->off_vals+c;
    return l->record_size<=UINT_MAX?0:-1;
}
static uint32_t field_u32(const unsigned char *p, size_t off)
{
    return get32(p+off);
}
static void field_set(unsigned char *p, size_t off, uint32_t v)
{
    put32(p+off, v);
}
static float field_float(const unsigned char *p, size_t off)
{
    uint32_t x=get32(p+off);
    float v;
    memcpy(&v, &x, 4);
    return v;
}
static void set_float(unsigned char *p, size_t off, float v)
{
    uint32_t x;
    memcpy(&x, &v, 4);
    put32(p+off, x);
}
/* HZIP compressed substream helpers. We grow the destination and retry if
 * the encoder cannot fit. A failed encoder is always closed before retrying. */
static int encode_mesh(const int *conn, unsigned dim, unsigned nz, Bytes *out)
{
    size_t cap=(size_t) nz*(1u<<dim) *4+1024;
    for (int attempt=0; attempt<8; attempt++)
    {
        unsigned char *tmp=malloc(cap);
        if (!tmp) return -1;
        HZMstream *s=hzip_mesh_create_mem(tmp, cap, dim==2?PERM2:PERM3,(int) dim,(int) nz, 0, 0);
        int wr=s?hzip_mesh_write(s, conn,(int) nz) :-1;
        int count=s?hzip_mesh_close(s) :-1;
        if (wr>=0&&count>=0&&(size_t) count<=cap)
        {
            int ok=append(out, tmp,(size_t) count);
            free(tmp);
            return ok;
        }
        free(tmp);
        if (cap>SIZE_MAX/2) return -1;
        cap*=2;
    }
    return -1;
}
static int encode_nodes(const float *vals, const int *conn, unsigned dim, unsigned nn, unsigned nz, Bytes *out)
{
    size_t cap=(size_t) nn*4+1024;
    for (int attempt=0; attempt<8; attempt++)
    {
        unsigned char *tmp=malloc(cap);
        if (!tmp) return -1;
        HZNstream *s=hzip_node_create_mem(tmp, cap, dim==2?PERM2:PERM3, 0,(int) dim,(int) nn, hzFLOAT, 0, 0);
        int wr=s?hzip_node_write(s, vals, conn,(int) nz) :-1;
        int count=s?hzip_node_close(s) :-1;
        if (wr>=0&&count>=0&&(size_t) count<=cap)
        {
            int ok=append(out, tmp,(size_t) count);
            free(tmp);
            return ok;
        }
        free(tmp);
        if (cap>SIZE_MAX/2) return -1;
        cap*=2;
    }
    return -1;
}
static int decode_mesh(const unsigned char *p, size_t n, unsigned dim, unsigned nz, int *conn)
{
    HZMstream *s=hzip_mesh_open_mem(p, n, dim==2?PERM2:PERM3);
    if (!s) return -1;
    int nr=hzip_mesh_read(s, conn,(int) nz);
    hzip_mesh_close(s);
    return nr==(int) nz?0:-1;
}
static int decode_nodes(const unsigned char *p, size_t n, unsigned dim, unsigned nn, unsigned nz, const int *conn, float *vals)
{
    HZNstream *s=hzip_node_open_mem(p, n, dim==2?PERM2:PERM3, 0);
    if (!s) return -1;
    if (hzip_node_count(s) != nn||hzip_node_dimensions(s) !=(int) dim||hzip_node_type(s) != hzFLOAT)
    {
        hzip_node_close(s);
        return -1;
    }
    int nr=hzip_node_read(s, vals, conn,(int) nz);
    hzip_node_close(s);
    return nr==(int) nn?0:-1;
}
/* Each chunk is a sequence of fixed-size meshlet records. On disk, one
 * compressed chunk holds a versioned sequence of self-contained meshlets.
 * Every meshlet carries its global IDs, connectivity, and all nodal fields.
 */
static size_t meshlet_filter(unsigned int flags, size_t ncd, const unsigned int cd[], size_t nbytes, size_t *buf_size, void **buf)
{
    if (ncd<6||cd[0]!=VERSION||!buf||!*buf) return 0;
    Layout l;
    if (layout_init(&l, cd[1], cd[2], cd[3], cd[4]) || l.record_size!=cd[5]) return 0;
    unsigned arity=1u<<l.dim;
    if (!(flags&H5Z_FLAG_REVERSE))
    {
        if (nbytes%l.record_size) return 0;
        size_t nrec=nbytes/l.record_size;
        if (nrec>UINT_MAX) return 0;
        Bytes output=
        {
            0
        }
        ;
        if (add32(&output, MAGIC) || add32(&output, VERSION) || add32(&output,(uint32_t) nrec)) goto write_fail;
        for (size_t r=0; r<nrec; r++)
        {
            const unsigned char *src=(const unsigned char *) *buf+r*l.record_size;
            unsigned nn=field_u32(src, 0), nz=field_u32(src, 4);
            if (!nn||!nz||nn>l.max_nodes||nz>l.max_zones) goto write_fail;
            int *conn=malloc((size_t) nz*arity*sizeof (int));
            float *vals=malloc((size_t) nn*sizeof (float));
            if (!conn||!vals)
            {
                free(conn);
                free(vals);
                goto write_fail;
            }
            Bytes mesh=
            {
                0
            }
            ;
            int ok=0;
            for (unsigned z=0; z<nz&&!ok; z++) for (unsigned j=0; j<arity; j++)
            {
                uint32_t v=field_u32(src, l.off_conn+((size_t) z*arity+j) *4);
                if (v>=nn)
                {
                    ok=-1;
                    break;
                }
                conn[(size_t) z*arity+j]=(int) v;
            }
            if (!ok) ok=encode_mesh(conn, l.dim, nz, &mesh);
            if (!ok)
            {
                if (add32(&output, nn) || add32(&output, nz) || add32(&output,(uint32_t) mesh.n)) ok=-1;
                if (!ok&&append(&output, src+l.off_ids,(size_t) nn*4)) ok=-1;
                if (!ok&&append(&output, mesh.p, mesh.n)) ok=-1;
            }
            free(mesh.p);
            for (unsigned f=0; f<l.fields&&!ok; f++)
            {
                Bytes stream=
                {
                    0
                }
                ;
                for (unsigned i=0; i<nn; i++) vals[i]=field_float(src, l.off_vals+((size_t) f*l.max_nodes+i) *4);
                if (encode_nodes(vals, conn, l.dim, nn, nz, &stream) || stream.n>UINT_MAX||add32(&output,(uint32_t) stream.n) || append(&output, stream.p, stream.n)) ok=-1;
                free(stream.p);
            }
            free(conn);
            free(vals);
            if (ok) goto write_fail;
        }
        free(*buf);
        *buf=output.p;
        *buf_size=output.cap;
        return output.n;
        write_fail:free(output.p);
        return 0;
    }
    else
    {
        const unsigned char *src=(const unsigned char *) *buf;
        if (nbytes<12||get32(src) != MAGIC||get32(src+4) != VERSION) return 0;
        size_t nrec=get32(src+8), pos=12, decoded;
        if (product(nrec, l.record_size, &decoded) || !decoded) return 0;
        unsigned char *out=calloc(1, decoded);
        if (!out) return 0;
        for (size_t r=0; r<nrec; r++)
        {
            if (pos>nbytes||nbytes-pos<12) goto read_fail;
            unsigned nn=get32(src+pos), nz=get32(src+pos+4), msize=get32(src+pos+8);
            pos+=12;
            if (!nn||!nz||nn>l.max_nodes||nz>l.max_zones||nbytes-pos<(size_t) nn*4) goto read_fail;
            unsigned char *dst=out+r*l.record_size;
            field_set(dst, 0, nn);
            field_set(dst, 4, nz);
            memcpy(dst+l.off_ids, src+pos,(size_t) nn*4);
            pos+=(size_t) nn*4;
            if (msize>nbytes-pos) goto read_fail;
            int *conn=malloc((size_t) nz*arity*sizeof (int));
            float *vals=malloc((size_t) nn*sizeof (float));
            if (!conn||!vals)
            {
                free(conn);
                free(vals);
                goto read_fail;
            }
            int ok=decode_mesh(src+pos, msize, l.dim, nz, conn);
            pos+=msize;
            for (unsigned z=0; z<nz&&!ok; z++) for (unsigned j=0; j<arity; j++)
            {
                int v=conn[(size_t) z*arity+j];
                if (v<0||(unsigned) v>=nn)
                {
                    ok=-1;
                    break;
                }
                field_set(dst, l.off_conn+((size_t) z*arity+j) *4,(uint32_t) v);
            }
            for (unsigned f=0; f<l.fields&&!ok; f++)
            {
                if (nbytes-pos<4)
                {
                    ok=-1;
                    break;
                }
                size_t count=get32(src+pos);
                pos+=4;
                if (count>nbytes-pos)
                {
                    ok=-1;
                    break;
                }
                if (decode_nodes(src+pos, count, l.dim, nn, nz, conn, vals)) ok=-1;
                pos+=count;
                for (unsigned i=0; i<nn&&!ok; i++) set_float(dst, l.off_vals+((size_t) f*l.max_nodes+i) *4, vals[i]);
            }
            free(conn);
            free(vals);
            if (ok) goto read_fail;
        }
        if (pos!=nbytes) goto read_fail;
        free(*buf);
        *buf=out;
        *buf_size=decoded;
        return decoded;
        read_fail:free(out);
        return 0;
    }
}
static htri_t can_apply(hid_t dcpl, hid_t type, hid_t space)
{
    (void) dcpl;
    (void) space;
    return H5Tget_class(type) == H5T_COMPOUND?1:0;
}
static herr_t set_local(hid_t dcpl, hid_t type, hid_t space)
{
    (void) space;
    unsigned int cd[6];
    size_t n=6;
    unsigned flags=0;
    if (H5Pget_filter_by_id2(dcpl, FILTER_ID, &flags, &n, cd, 0, NULL, NULL) <0||n!=6) return -1;
    Layout l;
    if (layout_init(&l, cd[1], cd[2], cd[3], cd[4]) || H5Tget_size(type) != l.record_size) return -1;
    cd[5]=(unsigned) l.record_size;
    return H5Pmodify_filter(dcpl, FILTER_ID, flags, 6, cd);
}
static const H5Z_class2_t cls=
{
    H5Z_CLASS_T_VERS, FILTER_ID, 1, 1, "hzip-selfcontained-meshlet", can_apply, set_local, meshlet_filter
}
;
static int shape_kind(int t, int sz, int dim)
{
    if (dim==2)
    {
        if (t==DB_ZONETYPE_TRIANGLE||(!t&&sz==3)) return 3;
        if (t==DB_ZONETYPE_QUAD||(!t&&sz==4)) return 4;
    }
    else
    {
        if (t==DB_ZONETYPE_TET||(!t&&sz==4)) return 4;
        if (t==DB_ZONETYPE_PYRAMID||(!t&&sz==5)) return 5;
        if (t==DB_ZONETYPE_PRISM||(!t&&sz==6)) return 6;
        if (t==DB_ZONETYPE_HEX||(!t&&sz==8)) return 8;
    }
    return -1;
}
static int normalize(const DBucdmesh *mesh, int **out)
{
    const DBzonelist *zl=mesh->zones;
    if (!zl||!zl->nodelist||zl->nzones<1||zl->nshapes<1) return -1;
    unsigned arity=1u<<mesh->ndims;
    int *result=malloc((size_t) zl->nzones*arity*sizeof (int));
    if (!result) return -1;
    size_t off=0;
    int zone=0;
    for (int s=0; s<zl->nshapes; s++)
    {
        int sz=zl->shapesize[s], n=zl->shapecnt[s], kind=shape_kind(zl->shapetype?zl->shapetype[s]:0, sz, mesh->ndims);
        if (kind<0||kind!=sz||n<0)
        {
            free(result);
            return -1;
        }
        for (int z=0; z<n; z++)
        {
            if (zone>=zl->nzones||off+(size_t) sz>(size_t) zl->lnodelist)
            {
                free(result);
                return -1;
            }
            int a[8]=
            {
                0
            }
            ;
            for (int j=0; j<sz; j++)
            {
                long long id=(long long) zl->nodelist[off+j]-zl->origin;
                if (id<0||id>=mesh->nnodes)
                {
                    free(result);
                    return -1;
                }
                a[j]=(int) id;
            }
            off+=(size_t) sz;
            int *dst=result+(size_t) zone*arity;
            if (mesh->ndims==2)
            {
                dst[0]=a[0];
                dst[1]=a[1];
                dst[2]=a[2];
                dst[3]=a[kind==3?2:3];
            }
            else if (kind==4)
            {
                int p[]=
                {
                    0, 1, 2, 2, 3, 3, 3, 3
                }
                ;
                for (int j=0; j<8; j++) dst[j]=a[p[j]];
            }
            else if (kind==5)
            {
                int p[]=
                {
                    0, 1, 2, 3, 4, 4, 4, 4
                }
                ;
                for (int j=0; j<8; j++) dst[j]=a[p[j]];
            }
            else if (kind==6)
            {
                int p[]=
                {
                    0, 1, 2, 3, 4, 5, 5, 4
                }
                ;
                for (int j=0; j<8; j++) dst[j]=a[p[j]];
            }
            else memcpy(dst, a, 8*sizeof (int));
            zone++;
        }
    }
    if (zone!=zl->nzones||off!=(size_t) zl->lnodelist)
    {
        free(result);
        return -1;
    }
    *out=result;
    return 0;
}
static double number(const void *a, int type, size_t i)
{
    switch (type)
    {
        case DB_FLOAT:return ((const float*) a)[i];
        case DB_DOUBLE:return ((const double*) a)[i];
        case DB_INT:return ((const int*) a)[i];
        case DB_SHORT:return ((const short*) a)[i];
        case DB_CHAR:return ((const char*) a)[i];
        default:return NAN;
    }
}
static int supported_type(int t)
{
    return t==DB_FLOAT||t==DB_DOUBLE||t==DB_INT||t==DB_SHORT||t==DB_CHAR;
}
/* Arithmetic incident-zone averaging (equal weights), not volume weighting. */
static float *nodal_component(const DBucdvar *v, int component, const DBucdmesh *m, const int *conn)
{
    if (!supported_type(v->datatype) || !v->vals||!v->vals[component]) return NULL;
    float *out=calloc((size_t) m->nnodes, sizeof (float));
    if (!out) return NULL;
    if (v->centering==DB_NODECENT)
    {
        if (v->nels!=m->nnodes)
        {
            free(out);
            return NULL;
        }
        for (int i=0; i<m->nnodes; i++) out[i]=(float) number(v->vals[component], v->datatype,(size_t) i);
    }
    else if (v->centering==DB_ZONECENT)
    {
        if (v->nels!=m->zones->nzones)
        {
            free(out);
            return NULL;
        }
        double *sum=calloc((size_t) m->nnodes, sizeof (double));
        unsigned *count=calloc((size_t) m->nnodes, sizeof (unsigned));
        if (!sum||!count)
        {
            free(sum);
            free(count);
            free(out);
            return NULL;
        }
        unsigned arity=1u<<m->ndims;
        for (int z=0; z<m->zones->nzones; z++)
        {
            double x=number(v->vals[component], v->datatype,(size_t) z);
            /* Each unique node in a degenerate zone receives one contribution. */
            for (unsigned j=0; j<arity; j++)
            {
                int id=conn[(size_t) z*arity+j];
                int seen=0;
                for (unsigned k=0; k<j; k++) if (conn[(size_t) z*arity+k]==id)
                {
                    seen=1;
                    break;
                }
                if (!seen)
                {
                    sum[id]+=x;
                    count[id]++;
                }
            }
        }
        for (int i=0; i<m->nnodes; i++) out[i]=count[i]?(float)(sum[i]/count[i]) :NAN;
        free(sum);
        free(count);
    }
    else
    {
        free(out);
        return NULL;
    }
    return out;
}
static hid_t make_type(const Layout *l)
{
    hid_t t=-1, arr=-1;
    hsize_t dims[2];
    t=H5Tcreate(H5T_COMPOUND, l->record_size);
    if (t<0) return -1;
    if (H5Tinsert(t, "nnodes", 0, H5T_STD_U32LE) <0||H5Tinsert(t, "nzones", 4, H5T_STD_U32LE) <0) goto error;
    dims[0]=l->max_nodes;
    arr=H5Tarray_create2(H5T_STD_U32LE, 1, dims);
    if (arr<0||H5Tinsert(t, "global_node_ids", l->off_ids, arr) <0) goto error;
    H5Tclose(arr);
    arr=-1;
    dims[0]=l->max_zones;
    dims[1]=1u<<l->dim;
    arr=H5Tarray_create2(H5T_STD_U32LE, 2, dims);
    if (arr<0||H5Tinsert(t, "connectivity", l->off_conn, arr) <0) goto error;
    H5Tclose(arr);
    arr=-1;
    dims[0]=l->fields;
    dims[1]=l->max_nodes;
    arr=H5Tarray_create2(H5T_IEEE_F32LE, 2, dims);
    if (arr<0||H5Tinsert(t, "fields", l->off_vals, arr) <0) goto error;
    H5Tclose(arr);
    return t;
    error:if (arr>=0) H5Tclose(arr);
    H5Tclose(t);
    return -1;
}
/* Greedy sequential partition: no zone split, local IDs in order of first use. */
static int partition(const Layout *l, const DBucdmesh *m, const int *conn, float **fields, unsigned char **records, size_t *count)
{
    size_t cap=32, n=0;
    unsigned char *out=malloc(cap*l->record_size);
    if (!out) return -1;
    int *local=malloc((size_t) m->nnodes*sizeof (int));
    if (!local)
    {
        free(out);
        return -1;
    }
    for (int i=0; i<m->nnodes; i++) local[i]=-1;
    unsigned arity=1u<<l->dim;
    int z=0;
    while (z<m->zones->nzones)
    {
        if (n==cap)
        {
            if (cap>SIZE_MAX/2/l->record_size)
            {
                free(local);
                free(out);
                return -1;
            }
            cap*=2;
            void *p=realloc(out, cap*l->record_size);
            if (!p)
            {
                free(local);
                free(out);
                return -1;
            }
            out=p;
        }
        unsigned char *rec=out+n*l->record_size;
        memset(rec, 0, l->record_size);
        unsigned nn=0, nz=0;
        while (z<m->zones->nzones&&nz<l->max_zones)
        {
            unsigned extra=0;
            for (unsigned j=0; j<arity; j++)
            {
                int g=conn[(size_t) z*arity+j];
                int already=0;
                for (unsigned k=0; k<j; k++) if (conn[(size_t) z*arity+k]==g)
                {
                    already=1;
                    break;
                }
                if (local[g]<0&&!already) extra++;
            }
            if (nn+extra>l->max_nodes) break;
            for (unsigned j=0; j<arity; j++)
            {
                int g=conn[(size_t) z*arity+j];
                if (local[g]<0)
                {
                    local[g]=(int) nn;
                    field_set(rec, l->off_ids+(size_t) nn*4,(uint32_t) g);
                    nn++;
                }
                field_set(rec, l->off_conn+((size_t) nz*arity+j) *4,(uint32_t) local[g]);
            }
            z++;
            nz++;
        }
        if (!nz)
        {
            free(local);
            free(out);
            return -1;
        }
        field_set(rec, 0, nn);
        field_set(rec, 4, nz);
        for (unsigned i=0; i<nn; i++)
        {
            int g=(int) field_u32(rec, l->off_ids+(size_t) i*4);
            for (unsigned f=0; f<l->fields; f++) set_float(rec, l->off_vals+((size_t) f*l->max_nodes+i) *4, fields[f][g]);
            local[g]=-1;
        }
        n++;
    }
    free(local);
    *records=out;
    *count=n;
    return 0;
}
int main(int argc, char **argv)
{
    DBfile *db=NULL;
    DBucdmesh *mesh=NULL;
    DBucdvar **vars=NULL;
    int *conn=NULL;
    float **fields=NULL;
    unsigned char *records=NULL;
    hid_t file=-1, type=-1, space=-1, dcpl=-1, dset=-1;
    int result=1;
    size_t nrec=0;
    if (argc<5)
    {
        fprintf(stderr, "usage: %s input.silo meshname output.h5 var1 [var2 ...]\n", argv[0]);
        return 2;
    }
    db=DBOpen(argv[1], DB_UNKNOWN, DB_READ);
    if (!db) FAIL("cannot open Silo file");
    mesh=DBGetUcdmesh(db, argv[2]);
    if (!mesh) FAIL("mesh not found or not UCD");
    if ((mesh->ndims!=2&&mesh->ndims!=3) || mesh->nnodes<=0||!mesh->zones||mesh->phzones) FAIL("requires 2D/3D UCD zoo-type zonelist");
    if (!supported_type(mesh->datatype)) FAIL("unsupported coordinate datatype");
    if (normalize(mesh, &conn)) FAIL("unsupported/invalid shapes or connectivity");
    int nv=argc-4;
    vars=calloc((size_t) nv, sizeof (*vars));
    if (!vars) FAIL("out of memory");
    unsigned nfields=(unsigned) mesh->ndims;
    for (int v=0; v<nv; v++)
    {
        vars[v]=DBGetUcdvar(db, argv[v+4]);
        if (!vars[v]) FAIL("cannot read variable %s", argv[v+4]);
        if (vars[v]->nvals<1||vars[v]->nvals>32||nfields>UINT_MAX-(unsigned) vars[v]->nvals) FAIL("unsupported variable components");
        nfields+=(unsigned) vars[v]->nvals;
    }
    /* Fixed capacities; override via environment variables for experiments. */
    unsigned maxnodes=512, maxzones=256, chunkmeshlets=8;
    const char *e=getenv("MESHLET_NODES");
    if (e) maxnodes=(unsigned) strtoul(e, NULL, 10);
    e=getenv("MESHLET_ZONES");
    if (e) maxzones=(unsigned) strtoul(e, NULL, 10);
    e=getenv("MESHLET_CHUNK");
    if (e) chunkmeshlets=(unsigned) strtoul(e, NULL, 10);
    Layout l;
    if (layout_init(&l,(unsigned) mesh->ndims, maxnodes, maxzones, nfields) || !chunkmeshlets) FAIL("invalid meshlet capacities");
    fields=calloc(nfields, sizeof (*fields));
    if (!fields) FAIL("out of memory");
    unsigned f=0;
    for (int d=0; d<mesh->ndims; d++)
    {
        if (!mesh->coords[d]) FAIL("missing coordinate array");
        fields[f]=malloc((size_t) mesh->nnodes*sizeof (float));
        if (!fields[f]) FAIL("out of memory");
        for (int i=0; i<mesh->nnodes; i++) fields[f][i]=(float) number(mesh->coords[d], mesh->datatype,(size_t) i);
        f++;
    }
    for (int v=0; v<nv; v++) for (int c=0; c<vars[v]->nvals; c++)
    {
        fields[f]=nodal_component(vars[v], c, mesh, conn);
        if (!fields[f]) FAIL("cannot convert variable %s component %d", argv[v+4], c);
        f++;
    }
    if (partition(&l, mesh, conn, fields, &records, &nrec) || !nrec) FAIL("meshlet partition failed; try increasing capacities");
    if (H5Zregister(&cls) <0) FAIL("HZIP filter registration failed");
    file=H5Fcreate(argv[3], H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file<0) FAIL("H5Fcreate failed");
    type=make_type(&l);
    if (type<0) FAIL("cannot create compound datatype");
    char typename[128];
    snprintf(typename, sizeof (typename), "meshlet_%u_%u_%u_%u", l.dim, l.max_nodes, l.max_zones, l.fields);
    if (H5Tcommit2(file, typename, type, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) <0) FAIL("H5Tcommit2 failed");
    hsize_t dims[]=
    {
        nrec
    }
    , chunks[]=
    {
        chunkmeshlets
    }
    ;
    if (chunks[0]>dims[0]) chunks[0]=dims[0];
    space=H5Screate_simple(1, dims, NULL);
    dcpl=H5Pcreate(H5P_DATASET_CREATE);
    if (space<0||dcpl<0||H5Pset_chunk(dcpl, 1, chunks) <0) FAIL("cannot configure chunks");
    unsigned cd[]=
    {
        VERSION, l.dim, l.max_nodes, l.max_zones, l.fields,(unsigned) l.record_size
    }
    ;
    if (H5Pset_filter(dcpl, FILTER_ID, H5Z_FLAG_MANDATORY, 6, cd) <0) FAIL("H5Pset_filter failed");
    dset=H5Dcreate2(file, "meshlets", type, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
    if (dset<0) FAIL("H5Dcreate2 failed");
    /* Records are serialized in LE form; on LE hosts this is the same layout
         * as the compound file type. Use the file type as memory type only on LE. */
    {
        const uint16_t one=1;
        if (*(const unsigned char *) &one!=1) FAIL("prototype currently requires little-endian host");
    }
    if (H5Dwrite(dset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, records) <0) FAIL("H5Dwrite failed");
    /* Force filter execution before releasing the source records. */
    if (H5Fflush(file, H5F_SCOPE_GLOBAL) <0) FAIL("flush failed");
    /* Read-back verification exercises the reverse HZIP filter. */
    {
        size_t total;
        if (product(nrec, l.record_size, &total)) FAIL("readback size overflow");
        unsigned char *check=malloc(total);
        if (!check) FAIL("readback allocation failed");
        if (H5Dread(dset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, check) <0 || memcmp(check, records, total) != 0)
        {
            free(check);
            FAIL("readback mismatch");
        }
        free(check);
    }
    printf("wrote %zu meshlets (%u nodes / %u zones max, %u fields, %llu bytes/record, %llu meshlets/chunk) to %s\n", nrec, l.max_nodes, l.max_zones, l.fields,(unsigned long long) l.record_size,(unsigned long long) chunks[0], argv[3]);
    result=0;
    fail: if (dset>=0) H5Dclose(dset);
    if (dcpl>=0) H5Pclose(dcpl);
    if (space>=0) H5Sclose(space);
    if (type>=0) H5Tclose(type);
    if (file>=0) H5Fclose(file);
    free(records);
    free(conn);
    if (fields)
    {
        unsigned n=(mesh&&mesh->ndims>0) ?(unsigned) mesh->ndims:0;
        if (vars) for (int v=0; v<argc-4; v++) if (vars[v]) n+=(unsigned) vars[v]->nvals;
        for (unsigned i=0; i<n; i++) free(fields[i]);
        free(fields);
    }
    if (vars)
    {
        for (int v=0; v<argc-4; v++) if (vars[v]) DBFreeUcdvar(vars[v]);
        free(vars);
    }
    if (mesh) DBFreeUcdmesh(mesh);
    if (db) DBClose(db);
    return result;
}
