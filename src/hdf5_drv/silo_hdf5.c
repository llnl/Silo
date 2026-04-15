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
#include <hdf5.h>

#include <errno.h>
#include <assert.h>
#include <limits.h>
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h> /*missing from silo header files*/
#endif
#include <stdio.h>
#include "silo_hdf5_private.h"
#include "H5FDsilo.h"
#if defined(HAVE_HDF5_H) && defined(HAVE_LIBHDF5) /* [ */

/* HZIP node order permuation vector construction.
   To construct a permutation vector, work backwards
   from the highest numbered node on the canonical
   element, finding its cooresponding node number on
   the source element. Put the resulting number in
   the highest nibble of the unsigned word for the
   permutation vector. Then, move to the next highest
   numbered node on the canonical element, repeating
   all these steps to arrive at a value for the next
   highest order nibble in the word. 3D example...
  
       3-------2       2-------3
      /|      /|      /|      /|
     4-------5 |     6-------7 |
     | |     | | --> | |     | |
     | 0-----|-1     | 0-----|-1
     |/      |/      |/      |/
     7-------6       4-------5
  
     source data     canonical
     ordering        ordering

             0x54672310

   2D example...
       (Note the zone on the left is also Silo/VisIt
        node order for a quad)

       1-------2       2-------3
       |       |       |       |
       |       |       |       |
       |       | -->   |       |
       0-------3       0-------1

            0x00002130

  HZIP Compression integration overview...

  HZIP is a unique among compression algorithms in that it is 
  capabable of exploiting mesh structure in order to improve
  performance. However, this means that the compressor needs 
  to maintain information on the mesh connectivity in order to
  compress any variables defined on the mesh. Other compressors
  operate either on raw byte streams or, maybe, multidimensional
  integer and floating point arrays. FPZIP does that for example.
  But, to use HZIP, Silo needs to maintain a cache of recently
  used nodelists. Since Silo imposes no order on how objects are
  written or read, if the writer doen't write in the correct order,
  things will fail. During read, however, the silo library can
  ensure tha all information necessary to decompress is available
  when necessary. 

  The design here attempts to localize as much of the compression
  know how to an HDF5 'filter.' It is best if the compressor is
  properly integrated with HDF5 using HDF5's filter architecture.
  This is substantially easier for generic compressors because
  there isn't any information other than the data stream required
  to compress or decompress. That is not true for HZIP compression.
  The HDF5 filter that implements HZIP compression,
  db_hdf5_hzip_filter_op, must exploit a lot of information apart
  from the actual data stream it is compressing or decompressing.
  This extra information is managed in db_hdf5_hzip_params. In
  a handful of places in the driver, we've had to add code to
  manage HZIP compression information so that the compressor will
  have all the information it needs. These places are the
  PrepareForXXXCompression|Decompression methods.

  There are a number of issues...
  a) HZIP compresses only node-centered data, not zone-centerd.
  b) HZIP compresses only 2D quad meshes or 3D hex meshes.
  c) HZIP is designed for unstructured grid. To use it on structured
     grid, we create a fictitious nodelist at will.
  d) The silo client must call DBFreeResources (which happens
     automatically on DBClose) to free up stored nodelists
     used by HZIP. It is best if the client operate in such a
     way that reads or writes for all variables of a mesh
     occur shortly after the mesh itself and that the client
     does not iterate through a slew of meshes and then the
     variables for those meshes.
  e) To use HZIP, you need to give it a buffer to place the
     compressed result it. But, you can't allocate such a 
     buffer without knowing size. So, we take the approach
     of having a min. compression ratio and allocating a
     (possibly too large) buffer for HZIP to compress into.
     If HZIP cannot compress into this size, it fails and
     we skip the compression. If it can, HDF5 ultimately
     writes only the compressed bytes. Same is true for FPZIP.
  f) To use HDF5 filters, the datasets must be 'chunked.'
     With chunked datasets, HDF5 essentially makes a copy
     of the data on write effecting performance and memory
     usage.
  g) Silo supports data read masks. If you attempt to read
     a mesh without reading its zonelist and the zonelist
     (and mesh) are compressed, then the result will fail
     (presently in unpredictable ways) because HZIP needs
     the nodelist in order to decompress.
  h) In integrating compression, we have to take care that
     new calls introduced to support compression that ALWAYS
     happen (whether we are compressing or not) do not 
     negatively affect performance of the 'old' Silo.
     An example is in PrepareForUcdvarDecompression.
  f) Silex cannot browse HZIP compressed objects because it
     does not operate at the correct level of abstraction. 
     It uses DBGetObject calls only. 

*/

/* For Lindstrom compression libs, hzip and fpzip.
   These IDS were reserved by The HDF Group 11/22/2024.
   Previously they had been computed offsets from H5Z_FILTER_RESERVED.
   fpzip also has a reserved ID of 32014 for an implementation from NCAR,
   https://github.com/NCAR/fpzip_plugin */
#define DB_HDF5_HZIP_ID 257
#define DB_HDF5_FPZIP_ID 258
#ifdef HAVE_HZIP
#include "hzip.h"
#ifdef HAVE_LIBZ
static struct HZMCODECzlib db_hdf5_hzip_zlib_codec_params;
#endif
static struct HZMCODECbase db_hdf5_hzip_base_codec_params;
static const unsigned SILO_HZIP_PERMUTATION[4] = {0,0,((unsigned) (0x00002130)), ((unsigned) (0x65217430))};
#endif
#ifdef HAVE_FPZIP
#include "fpzip.h"
#endif
#ifdef HAVE_ZFP
#include "H5Zzfp.h"
extern void zfp_init_zfp();
#endif

/* Defining these to check overhead of PROTECT */
#if 0
#   define PROTECT      if(1)
#   define CLEANUP      else
#   define END_PROTECT  /*void*/
#   define UNWIND()     ; 
#endif

#define COMPRESSION_ERRMODE_FALLBACK 0
#define COMPRESSION_ERRMODE_FAIL     1
#define ALLOW_MESH_COMPRESSION 0x00000001

#define FALSE           0
#define TRUE            1

#define LINKGRP         "/.silo/"       /*name of link group            */
#define MAX_VARS        16              /*max vars per DB*var object    */
#define OPTDUP(S)       ((S)&&*(S)?strdup(S):NULL)
#define BASEDUP(S)       ((S)&&*(S)?db_FullName2BaseName(S):NULL)
#define ALIGN(ADDR,N)   (((ADDR)+(N)-1)&~((N)-1))

#define db_hdf5_H5LT_FILE_IMAGE_OPEN_RW      0x0001
#define db_hdf5_H5LT_FILE_IMAGE_DONT_COPY    0x0002
#define db_hdf5_H5LT_FILE_IMAGE_DONT_RELEASE 0x0004

/* useful macro for comparing HDF5 versions */
#include "hdf5_version_ge.h"

/* to encode the version of the hdf5 library in any silo executable */
char SILO_built_with_H5_lib_vers_info_g[] = "SILO built with "
#if HDF5_VERSION_GE(1,4,2)
H5_VERS_INFO;
#else
"HDF5 library: Version 1.4.1 or earlier";
#endif

/* some necessary forward declarations */
PRIVATE int db_hdf5_fullname(DBfile_hdf5 *dbfile, char *name, char *full);
PRIVATE int hdf2silo_type(hid_t type);
PRIVATE int db_hdf5_WriteCKZ(DBfile *_dbfile, char const *vname, void const *var,
              int const *dims, int ndims, int datatype, int nofilters);
PRIVATE int db_hdf5_getslink(hid_t cwg, char const *in_candidate_link, char *out_target);

/* callbacks prototypes for file image ops */
#if HDF5_VERSION_GE(1,8,9)
PRIVATE void *image_malloc(size_t size, H5FD_file_image_op_t file_image_op, void *udata);
PRIVATE void *image_memcpy(void *dest, const void *src, size_t size, H5FD_file_image_op_t file_image_op, void *udata);
PRIVATE void *image_realloc(void *ptr, size_t size, H5FD_file_image_op_t file_image_op, void *udata);
PRIVATE herr_t image_free(void *ptr, H5FD_file_image_op_t file_image_op, void *udata);
PRIVATE void *udata_copy(void *udata);
PRIVATE herr_t udata_free(void *udata);

/* Data structure to pass application data to callbacks. */
typedef struct {
    void    *app_image_ptr;   /* Pointer to application buffer */
    size_t   app_image_size;  /* Size of application buffer */
    void    *fapl_image_ptr;  /* Pointer to FAPL buffer */
    size_t   fapl_image_size; /* Size of FAPL buffer */
    int      fapl_ref_count;  /* Reference counter for FAPL buffer */
    void    *vfd_image_ptr;   /* Pointer to VFD buffer */
    size_t   vfd_image_size;  /* Size of VFD buffer */
    int      vfd_ref_count;   /* Reference counter for VFD buffer */
    unsigned flags;           /* Flags indicate how the file image will be open */
    int ref_count;            /* Reference counter on udata struct */
} db_hdf5_H5LT_file_image_ud_t;
#endif

/* Symbolic constants used in calls to db_StringListToStringArray
   to indicate behavior. A '!' in front means to not perform the
   associated action. For HDF5 driver, we handle the slash swap
   on the 'names' member of multi-block objects only and we
   never skip first semicolon. */
static int const        skipFirstSemicolon = 0;

/* Use `float' for all memory floating point values? */
static int              force_single_g;

/* used to control behavior of GetZonelist */
static char const      *calledFromGetUcdmesh = 0;

/* Struct used when building the CWD name */
typedef struct silo_hdf5_comp_t {
    char                *name;
    unsigned long       objno[2];
} silo_hdf5_comp_t;

/* Attributes for various types of objects */
typedef struct DBcurve_mt {
    int                 npts;
    int                 guihide;
    int                 coord_sys;
    char                xvarname[256];
    char                yvarname[256];
    char                label[256];
    char                xlabel[256];
    char                ylabel[256];
    char                xunits[256];
    char                yunits[256];
    char                reference[256];
    double              missing_value;
} DBcurve_mt;
static hid_t DBcurve_mt5 = -1;

typedef struct DBcsgmesh_mt {
    int            block_no;
    int            group_no;
    int            cycle;
    int            nbounds;
    int            lcoeffs;
    float          time;
    double         dtime;
    int            ndims;
    int            origin;
    int            guihide;
    double         min_extents[3];
    double         max_extents[3];
    char           units[3][256];
    char           labels[3][256];
    char           name[256];
    char           typeflags[256];
    char           bndids[256];
    char           coeffs[256];
    char           zonel_name[256];
    char           bndnames[256];
    char           mrgtree_name[256];
    int            tv_connectivity;
    int            disjoint_mode;
    char           alt_nodenum_vars[256];
} DBcsgmesh_mt;
static hid_t DBcsgmesh_mt5;

typedef struct DBcsgvar_mt {
    int            cycle;
    float          time;
    double         dtime;
    int            datatype;
    int            nels;
    int            nvals;
    int            centering;
    int            use_specmf;
    int            ascii_labels;
    int            guihide;
    char           name[256];
    char           units[256];
    char           label[256];
    char           vals[MAX_VARS][256];
    char           meshname[256];
    char           region_pnames[256];
    int            conserved;
    int            extensive;
    double         missing_value;
} DBcsgvar_mt;
static hid_t DBcsgvar_mt5;

typedef struct DBcsgzonelist_mt {
    int            nregs;
    int            origin;
    int            lxform;
    int            datatype;
    int            nzones;
    int            min_index;
    int            max_index;
    char           typeflags[256];
    char           leftids[256];
    char           rightids[256];
    char           xform[256];
    char           zonelist[256];
    char           regnames[256];
    char           zonenames[256];
    char           alt_zonenum_vars[256];
} DBcsgzonelist_mt;
static hid_t DBcsgzonelist_mt5;

typedef struct DBdefvars_mt {
    int            ndefs;
    char           names[256];
    char           types[256];
    char           defns[256];
    char           guihides[256];
} DBdefvars_mt;
static hid_t DBdefvars_mt5;

typedef struct DBquadmesh_mt {
    char                coord[3][256];
    double              min_extents[3];
    double              max_extents[3];
    int                 ndims;
    int                 coordtype;
    int                 nspace;
    int                 nnodes;
    int                 facetype;
    int                 major_order;
    int                 cycle;
    int                 coord_sys;
    int                 planar;
    int                 origin;
    int                 group_no;
    int                 dims[3];
    int                 min_index[3];
    int                 max_index[3];
    int                 baseindex[3];
    float               time;
    double              dtime;
    int                 guihide;
    char                label[3][256];
    char                units[3][256];
    char                mrgtree_name[256];
    char                ghost_node_labels[256];
    char                ghost_zone_labels[256];
    char                alt_nodenum_vars[256];
    char                alt_zonenum_vars[256];
} DBquadmesh_mt;
static hid_t DBquadmesh_mt5;

typedef struct DBquadvar_mt {
    char                value[MAX_VARS][256];
    char                mixed_value[MAX_VARS][256];
    char                meshid[256];
    int                 ndims;
    int                 nvals;
    int                 nels;
    int                 origin;
    int                 mixlen;
    int                 major_order;
    int                 datatype;
    int                 cycle;
    float               time;
    double              dtime;
    int                 use_specmf;
    int                 ascii_labels;
    int                 dims[3];
    int                 zones[3];
    int                 min_index[3];
    int                 max_index[3];
    float               align[3];
    int                 guihide;
    char                label[256];
    char                units[256];
    char                region_pnames[256];
    int                 conserved;
    int                 extensive;
    int                 centering;
    double              missing_value;
} DBquadvar_mt;
static hid_t    DBquadvar_mt5;

typedef struct DBucdmesh_mt {
    char                coord[3][256];
    int                 ndims;
    int                 nnodes;
    int                 nzones;
    int                 facetype;
    int                 cycle;
    int                 coord_sys;
    int                 topo_dim;
    int                 planar;
    int                 origin;
    int                 group_no;
    float               time;
    double              dtime;
    int                 guihide;
    char                facelist[256];
    char                zonelist[256];
    char                gnodeno[256];
    double              min_extents[3];
    double              max_extents[3];
    char                label[3][256];
    char                units[3][256];
    char                phzonelist[256];
    char                mrgtree_name[256];
    int                 tv_connectivity;
    int                 disjoint_mode;
    int                 gnznodtype;
    char                ghost_node_labels[256];
    char                alt_nodenum_vars[256];
} DBucdmesh_mt;
static hid_t    DBucdmesh_mt5;

typedef struct DBucdvar_mt {
    char                value[MAX_VARS][256];
    char                mixed_value[MAX_VARS][256];
    char                meshid[256];
    int                 ndims;
    int                 nvals;
    int                 nels;
    int                 centering;
    int                 origin;
    int                 mixlen;
    int                 datatype;
    int                 cycle;
    int                 use_specmf;
    int                 ascii_labels;
    float               time;
    double              dtime;
    int                 lo_offset;
    int                 hi_offset;
    int                 guihide;
    char                label[256];
    char                units[256];
    char                region_pnames[256];
    int                 conserved;
    int                 extensive;
    double              missing_value;
} DBucdvar_mt;
static hid_t    DBucdvar_mt5;

typedef struct DBfacelist_mt {
    int                 ndims;
    int                 nfaces;
    int                 nshapes;
    int                 ntypes;
    int                 lnodelist;
    int                 origin;
    char                nodelist[256];
    char                shapecnt[256];
    char                shapesize[256];
    char                typelist[256];
    char                types[256];
    char                zoneno[256];
} DBfacelist_mt;
static hid_t    DBfacelist_mt5;

typedef struct DBzonelist_mt {
    int                 ndims;
    int                 nzones;
    int                 nshapes;
    int                 lnodelist;
    int                 origin;
    int                 lo_offset;
    int                 hi_offset;
    char                nodelist[256];
    char                shapecnt[256];
    char                shapesize[256];
    char                shapetype[256];
    char                gzoneno[256];
    int                 gnznodtype;
    char                ghost_zone_labels[256];
    char                alt_zonenum_vars[256];
} DBzonelist_mt;
static hid_t    DBzonelist_mt5;

typedef struct DBphzonelist_mt {
    int                 nfaces;
    int                 lnodelist;
    int                 nzones;
    int                 lfacelist;
    int                 origin;
    int                 lo_offset;
    int                 hi_offset;
    char                nodecnt[256];
    char                nodelist[256];
    char                extface[256];
    char                facecnt[256];
    char                facelist[256];
    char                gzoneno[256];
    int                 gnznodtype;
    char                ghost_zone_labels[256];
    char                alt_zonenum_vars[256];
} DBphzonelist_mt;
static hid_t    DBphzonelist_mt5;

typedef struct DBmaterial_mt {
    int                 ndims;
    int                 nmat;
    int                 mixlen;
    int                 origin;
    int                 major_order;
    int                 datatype;
    int                 dims[3];
    int                 allowmat0;
    int                 guihide;
    char                meshid[256];
    char                matlist[256];
    char                matnos[256];
    char                mix_vf[256];
    char                mix_next[256];
    char                mix_mat[256];
    char                mix_zone[256];
    char                matnames[256];
    char                matcolors[256];
} DBmaterial_mt;
static hid_t    DBmaterial_mt5;

typedef struct DBmultimesh_mt {
    int                 nblocks;
    int                 cycle;
    int                 ngroups;
    int                 blockorigin;
    int                 grouporigin;
    float               time;
    double              dtime;
    int                 guihide;
    int                 extentssize;
    char                meshtypes[256];
    char                meshnames[256];
    char                extents[256];
    char                zonecounts[256];
    char                has_external_zones[256];
    int                 lgroupings;
    char                groupings[256];
    char                groupnames[256];
    char                mrgtree_name[256];
    int                 tv_connectivity;
    int                 disjoint_mode;
    int                 topo_dim;
    char                file_ns_name[256];
    char                block_ns_name[256];
    int                 block_type;
    char                empty_list[256];
    int                 empty_cnt;
    int                 repr_block_idx;
    char                alt_nodenum_vars[256];
    char                alt_zonenum_vars[256];
} DBmultimesh_mt;
static hid_t    DBmultimesh_mt5;

typedef struct DBmultimeshadj_mt {
    int                 nblocks;
    int                 blockorigin;
    int                 lneighbors;
    int                 totlnodelists;
    int                 totlzonelists;
    char                meshtypes[256];
    char                nneighbors[256];
    char                neighbors[256];
    char                back[256];
    char                lnodelists[256];
    char                nodelists[256];
    char                lzonelists[256];
    char                zonelists[256];
} DBmultimeshadj_mt;
static hid_t DBmultimeshadj_mt5;

typedef struct DBmultivar_mt {
    int                 nvars;
    int                 cycle;
    int                 ngroups;
    int                 blockorigin;
    int                 grouporigin;
    float               time;
    double              dtime;
    int                 extentssize;
    int                 guihide;
    char                vartypes[256];
    char                varnames[256];
    char                extents[256];
    char                region_pnames[256];
    char                mmesh_name[256];
    int                 tensor_rank;
    int                 conserved;
    int                 extensive;
    char                file_ns_name[256];
    char                block_ns_name[256];
    int                 block_type;
    char                empty_list[256];
    int                 empty_cnt;
    int                 repr_block_idx;
    double              missing_value;
} DBmultivar_mt;
static hid_t    DBmultivar_mt5;

typedef struct DBmultimat_mt {
    int                 nmats;
    int                 cycle;
    int                 ngroups;
    int                 blockorigin;
    int                 grouporigin;
    float               time;
    double              dtime;
    int                 allowmat0;
    int                 guihide;
    char                matnames[256];
    char                matnos[256];
    char                mixlens[256];
    char                matcounts[256];
    char                matlists[256];
    int                 nmatnos;
    char                material_names[256];
    char                mat_colors[256];
    char                mmesh_name[256];
    char                file_ns_name[256];
    char                block_ns_name[256];
    char                empty_list[256];
    int                 empty_cnt;
    int                 repr_block_idx;
} DBmultimat_mt;
static hid_t    DBmultimat_mt5;

typedef struct DBmultimatspecies_mt {
    int                 nspec;
    int                 nmat;
    int                 cycle;
    int                 ngroups;
    int                 blockorigin;
    int                 grouporigin;
    float               time;
    double              dtime;
    int                 guihide;
    char                specnames[256];
    char                nmatspec[256];
    char                matname[256];
    char                species_names[256];
    char                speccolors[256];
    char                file_ns_name[256];
    char                block_ns_name[256];
    char                empty_list[256];
    int                 empty_cnt;
    int                 repr_block_idx;
} DBmultimatspecies_mt;
static hid_t    DBmultimatspecies_mt5;

typedef struct DBmatspecies_mt {
    int                 ndims;
    int                 nmat;
    int                 nspecies_mf;
    int                 mixlen;
    int                 major_order;
    int                 datatype;
    int                 dims[3];
    int                 guihide;
    char                matname[256];
    char                speclist[256];
    char                nmatspec[256];
    char                species_mf[256];
    char                mix_speclist[256];
    char                specnames[256];
    char                speccolors[256];
} DBmatspecies_mt;
static hid_t    DBmatspecies_mt5;

typedef struct DBpointmesh_mt {
    int                 ndims;
    int                 nspace;
    int                 nels;
    int                 cycle;
    int                 group_no;
    float               time;
    double              dtime;
    int                 origin;
    int                 min_index;
    int                 max_index;
    double              min_extents[3];
    double              max_extents[3];
    int                 guihide;
    char                coord[3][256];
    char                label[3][256];
    char                units[3][256];
    char                gnodeno[256];
    char                mrgtree_name[256];
    char                ghost_node_labels[256];
    int                 gnznodtype;
    char                alt_nodenum_vars[256];
} DBpointmesh_mt;
static hid_t    DBpointmesh_mt5;

typedef struct DBpointvar_mt {
    int                 nvals;
    int                 nels;
    int                 ndims;
    int                 nspace;
    int                 origin;
    int                 min_index;
    int                 max_index;
    int                 datatype;
    int                 cycle;
    float               time;
    double              dtime;
    int                 guihide;
    int                 ascii_labels;
    char                meshid[256];
    char                label[256];
    char                units[256];
    char                data[MAX_VARS][256];
    char                region_pnames[256];
    int                 conserved;
    int                 extensive;
    double              missing_value;
} DBpointvar_mt;
static hid_t    DBpointvar_mt5;

typedef struct DBcompoundarray_mt {
    int                 nelems;
    int                 nvalues;
    int                 datatype;
    char                values[256];
    char                elemnames[256];
    char                elemlengths[256];
} DBcompoundarray_mt;
static hid_t    DBcompoundarray_mt5;

typedef struct DBmrgtree_mt {
    /* from the DBmrgtree header struct */
    int            src_mesh_type;
    int            type_info_bits;
    int            num_nodes;
    int            root;
    char           src_mesh_name[256];

    /* contents of tree from the DBmrgtnode struct */
    char           n_scalars[256]; /* scalar data for a node */
    char           n_name[256];
    char           n_names[256];
    char           n_maps_name[256];
    char           n_seg_ids[256];
    char           n_seg_lens[256];
    char           n_seg_types[256];
    char           n_children[256];

    char           mrgvar_onames[256];
    char           mrgvar_rnames[256];
} DBmrgtree_mt;
static hid_t   DBmrgtree_mt5;

typedef struct DBgroupelmap_mt {
    int            num_segments;
    int            fracs_data_type;
    char           groupel_types[256];
    char           segment_lengths[256];
    char           segment_ids[256];
    char           segment_data[256];
    char           frac_lengths[256];
    char           segment_fracs[256]; 
} DBgroupelmap_mt;
static hid_t DBgroupelmap_mt5;

typedef struct DBmrgvar_mt {
    int nregns;
    int ncomps;
    int datatype;
    char mrgt_name[256];
    char reg_pnames[256];
    char compnames[256];
    char data[MAX_VARS][256];
} DBmrgvar_mt;
static hid_t DBmrgvar_mt5;

static hid_t    T_char = -1;
static hid_t    T_short = -1;
static hid_t    T_int = -1;
static hid_t    T_long = -1;
static hid_t    T_llong = -1;
static hid_t    T_float = -1;
static hid_t    T_double = -1;
static hid_t    T_str256 = -1;
static hid_t    SCALAR = -1;
static hid_t    P_crprops = -1;
static hid_t    P_ckcrprops = -1;
static hid_t    P_rdprops = -1;
static hid_t    P_ckrdprops = -1;

#define OPT(V)          ((V)?(V):"")
#define OFFSET(P,F)     ((char*)&((P).F)-(char*)&(P))
#define ENDOF(S)        ((S)+strlen(S))

/*
 * Use these macros to define compound data types. The following form defines
 * an hdf5 data type called `DBcurve_mt5' which describes the C data type
 * `DBcurve_mt'.
 *
 *      STRUCT(DBcurve) {
 *          MEMBER(int,         npts);
 *          MEMBER(int,         datatype);
 *          MEMBER(str256,      label);
 *      } DEFINE;
 *
 * The following construct creates a temporary hdf5 data type `_mt' that
 * which is based on the C data type `DBcurve_mt' but which has minimal sizes
 * for character strings and may be missing character string fields which are
 * empty.
 *
 *      DBcurve_mt m;
 *      STRUCT(DBcurve) {
 *          MEMBER(int,         npts);
 *          m.npts = npts;
 *          MEMBER(int,         datatype);
 *          m.datatype = dtype;
 *          MEMBER(str(xname),  xvarname);
 *          strcpy(m.xvarname, OPT(xname));
 *      } OUTPUT(dbfile, DB_CURVE, "objname", &m);
 */
#define STRUCT(S) {                                                           \
    int         _i, _j=0;       /*counters*/                                  \
    size_t      _f_off=0;       /*offset into file data type*/                \
    DBfile_hdf5 *_f=NULL;       /*file for target types*/                     \
    S##_mt      _m;             /*temp to calculate offsets*/                 \
    hid_t       _tmp_m, _tmp_f; /*memory and file temporaries*/               \
    hid_t       _mt=-1, _ft=-1; /*memory and file data types*/                \
    char        _fullname[256]; /*name for repeated members*/                 \
                                                                              \
    for (_i=0; _i<3; _i++) {                                                  \
        switch (_i) {                                                         \
        case 0:                                                               \
            /*                                                                \
             * Fall through to pick up closing arguments. Touch local         \
             * variables so compiler doesn't complain about them not being    \
             * used.                                                          \
             */                                                               \
            break;                                                            \
                                                                              \
        case 2:                                                               \
            /* Define global DB*_mt5 data type for memory */                  \
            S##_mt5 = _mt;                                                    \
            break;                                                            \
                                                                              \
        case 1:                                                               \
            /* Build data types, file multiplier is arbitrary */              \
            _mt = H5Tcreate(H5T_COMPOUND, sizeof _m);                         \
            if (_f) _ft = H5Tcreate(H5T_COMPOUND, 3*sizeof _m);               \
            /* MEMBER DEFINITIONS HERE... */

#define MEMBER_DUMMY(TYPE,NAME) {                                             \
    _tmp_m = T_##TYPE; /*possible function call*/                             \
    if (_tmp_m>=0) {                                                          \
        db_hdf5_put_cmemb(_mt, #NAME, 0, 0, NULL, _tmp_m);                    \
        if (_f && (_tmp_f=_f->T_##TYPE)>=0) {                                 \
            db_hdf5_put_cmemb(_ft, #NAME, _f_off, 0, NULL, _tmp_f);           \
            _f_off += H5Tget_size(_tmp_f);                                    \
        }                                                                     \
    }                                                                         \
}

#define MEMBER_S(TYPE,NAME) {                                                 \
    _tmp_m = T_##TYPE; /*possible function call*/                             \
    if (_tmp_m>=0) {                                                          \
        db_hdf5_put_cmemb(_mt, #NAME, OFFSET(_m, NAME), 0, NULL, _tmp_m);     \
        if (_f && (_tmp_f=_f->T_##TYPE)>=0) {                                 \
            db_hdf5_put_cmemb(_ft, #NAME, _f_off, 0, NULL, _tmp_f);           \
            _f_off += H5Tget_size(_tmp_f);                                    \
        }                                                                     \
    }                                                                         \
}

#if HDF5_VERSION_GE(1,4,0)
#define MEMBER_3(TYPE,NAME) {                                                 \
    _tmp_m = T_##TYPE; /*possible function call*/                             \
    if (_tmp_m>=0) {                                                          \
        hid_t _m_ary;                                                         \
        hsize_t _size = 3;                                                    \
        _m_ary = H5Tarray_create(_tmp_m, 1, &_size);                          \
        db_hdf5_put_cmemb(_mt, #NAME, OFFSET(_m, NAME), 0, NULL, _m_ary);     \
        H5Tclose(_m_ary);                                                     \
        if (_f && (_tmp_f=_f->T_##TYPE)>=0) {                                 \
            hid_t _f_ary = H5Tarray_create(_tmp_f, 1, &_size);                \
            db_hdf5_put_cmemb(_ft, #NAME, _f_off, 0, NULL, _f_ary);           \
            _f_off += H5Tget_size(_f_ary);                                    \
            H5Tclose(_f_ary);                                                 \
        }                                                                     \
    }                                                                         \
}
#else
#define MEMBER_3(TYPE,NAME) {                                                 \
    _tmp_m = T_##TYPE; /*possible function call*/                             \
    if (_tmp_m>=0) {                                                          \
        hsize_t _size = 3;                                                    \
        db_hdf5_put_cmemb(_mt, #NAME, OFFSET(_m, NAME), 1, &_size, _tmp_m);   \
        if (_f && (_tmp_f=_f->T_##TYPE)>=0) {                                 \
            db_hdf5_put_cmemb(_ft, #NAME, _f_off, 1, &_size, _tmp_f);         \
            _f_off += 3*H5Tget_size(_tmp_f);                                  \
        }                                                                     \
    }                                                                         \
}
#endif

#define MEMBER_R(TYPE,NAME,N) {                                               \
    for (_j=0; _j<N; _j++) {                                                  \
        _tmp_m = T_##TYPE; /*possible function call*/                         \
        if (_tmp_m>=0) {                                                      \
            sprintf(_fullname, "%s%d", #NAME, _j);                            \
            db_hdf5_put_cmemb(_mt, _fullname, OFFSET(_m, NAME[_j]), 0, NULL,  \
                              _tmp_m);                                        \
            if (_f && (_tmp_f=_f->T_##TYPE)>=0) {                             \
                db_hdf5_put_cmemb(_ft, _fullname, _f_off, 0, NULL, _tmp_f);   \
                _f_off += H5Tget_size(_tmp_f);                                \
            }                                                                 \
        }                                                                     \
    }                                                                         \
}
 
#define OUTPUT(DBFILE,DBTYPE,NAME,MEM)                                        \
            break;                                                            \
        }                                                                     \
        if (0==_i) _f=(DBFILE);                                               \
        else if (1==_i) break;                                                \
    }                                                                         \
    if (!_f_off) MEMBER_DUMMY(int,dummy);                                     \
    H5Tpack(_ft);                                                             \
    db_hdf5_hdrwr(DBFILE, (char*)NAME, _mt, _ft, MEM, DBTYPE);                \
    H5Tclose(_mt);                                                            \
    H5Tclose(_ft);                                                            \
}

#define DEFINE                                                                \
            break;                                                            \
        }                                                                     \
    }                                                                         \
}

#ifdef HAVE_FPZIP /* { */

/* The following section of code are HDF5 filters to implement FPZIP
   compression algorithms

   Programmer: Mark C. Miller
   Created:    July, 2008
*/

typedef struct db_hdf5_fpzip_params_t {
    int                 loss; /* set by client call to DBSetCompression() */
    int                 isfp;
    int                 dp;
    int                 totsize1d;
    int                 ndims;
    int                 dims[10];
} db_hdf5_fpzip_params_t;
static db_hdf5_fpzip_params_t db_hdf5_fpzip_params;

static int
db_hdf5_safe_mul_size(size_t a, size_t b, size_t *out)
{
    if (a != 0 && b > SIZE_MAX / a)
        return -1;
    *out = a * b;
    return 0;
}

static htri_t 
db_hdf5_fpzip_can_apply(hid_t dcpl_id, hid_t type_id, hid_t space_id)
{
#if HDF5_VERSION_GE(1,8,8)
    if (H5Tget_class(type_id) != H5T_FLOAT)
        return 0;
    return 1;
#else
    /* Older versions of HDF5 don't like being told '0' for certain cases.
     * So, let it pass and handle it in the actual filter operation by
     * skipping non-float data there. */
    return 1;
#endif
}

static herr_t
db_hdf5_fpzip_set_local(hid_t dcpl_id, hid_t type_id, hid_t space_id)
{
    int i;
    hsize_t dims[10] = {1,1,1,1,1,1,1,1,1,1}, maxdims[10];
    db_hdf5_fpzip_params.dp = H5Tget_size(type_id) > sizeof(float) ? 1 : 0;
    db_hdf5_fpzip_params.totsize1d = (int) H5Sget_simple_extent_npoints(space_id);
    db_hdf5_fpzip_params.isfp = H5Tget_class(type_id) == H5T_FLOAT;
    db_hdf5_fpzip_params.ndims = H5Sget_simple_extent_ndims(space_id);
    H5Sget_simple_extent_dims(space_id, dims, maxdims);
    for (i = 0; i < db_hdf5_fpzip_params.ndims; i++)
        db_hdf5_fpzip_params.dims[i] = (int) dims[i];
    return 1;
}

static size_t
db_hdf5_fpzip_filter_op(unsigned int flags, size_t cd_nelmts,
    const unsigned int cd_values[], size_t nbytes,
    size_t *buf_size, void **buf)
{
    size_t early_retval = 0;

    if (flags & H5Z_FLAG_REVERSE) /* read case */
    {
        int prec, dp;
        unsigned nx, ny, nz, nf;
        size_t new_buf_size, tmp_size;
        size_t elemsz;
        void *uncbuf;

        /* first, decode fpzip's header */
        fpzip_memory_read(*buf, 0, &prec, &dp, &nx, &ny, &nz, &nf); 
        elemsz = dp ? sizeof(double) : sizeof(float);
        if (db_hdf5_safe_mul_size((size_t) nx, (size_t) ny, &tmp_size) < 0 ||
            db_hdf5_safe_mul_size(tmp_size, (size_t) nz, &tmp_size) < 0 ||
            db_hdf5_safe_mul_size(tmp_size, (size_t) nf, &tmp_size) < 0 ||
            db_hdf5_safe_mul_size(tmp_size, elemsz, &new_buf_size) < 0 ||
            new_buf_size == 0)
           return early_retval;

        /* allocate space and do the decompression */
        uncbuf = malloc(new_buf_size);
        if (!fpzip_memory_read(*buf, uncbuf, &prec, &dp, &nx, &ny, &nz, &nf))
        {
            free(uncbuf);
            return early_retval;
        }

        free(*buf);
        *buf = uncbuf;
        *buf_size = new_buf_size;
        return new_buf_size;
    }
    else /* write case */
    {
        unsigned char *cbuf;
        size_t max_outbytes, outbytes;
        int prec;

        /* We'll only compress floating point data here, not integer data */
        if (!db_hdf5_fpzip_params.isfp)
            return 0;

        /* We can't operate in place like HDF5 wants. But, thats ok.
         * Next, we can't easily predict compressed size but we need
         * to allocate a buffer to compress into. Fortunately, fpzip
         * will try to compress into any sized buffer we pass and
         * fail if it cannot. So, we decide here what is the minimum
         * compression we want, allocate a buffer of that size and
         * try to compress into it. If it fails, we return the right
         * stuff to HDF5 and do not compress */
        
        if (SILO_Globals.compressionMinratio <= 0)
            return early_retval;

        max_outbytes = nbytes / SILO_Globals.compressionMinratio;
        if (max_outbytes == 0)
            return early_retval;

        cbuf = (unsigned char *) malloc(max_outbytes);
        if (cbuf == 0)
            return early_retval;

        /* full precision */
        prec = 8 * (db_hdf5_fpzip_params.dp ? sizeof(double) : sizeof(float));
        
        /* precision with loss factored in */
        prec = (prec * (4 - db_hdf5_fpzip_params.loss)) / 4;

        if (db_hdf5_fpzip_params.ndims == 1 || db_hdf5_fpzip_params.ndims > 3)
        {
            outbytes = fpzip_memory_write(cbuf, max_outbytes, *buf,
                    &prec, db_hdf5_fpzip_params.dp,
                    db_hdf5_fpzip_params.totsize1d, 1, 1, 1);
        }
        else if (db_hdf5_fpzip_params.ndims == 2)
            outbytes = fpzip_memory_write(cbuf, max_outbytes, *buf,
                    &prec, db_hdf5_fpzip_params.dp,
                    db_hdf5_fpzip_params.dims[0],
                    db_hdf5_fpzip_params.dims[1], 1, 1);
        else
        {
            outbytes = fpzip_memory_write(cbuf, max_outbytes, *buf,
                    &prec, db_hdf5_fpzip_params.dp,
                    db_hdf5_fpzip_params.dims[0],
                    db_hdf5_fpzip_params.dims[1],
                    db_hdf5_fpzip_params.dims[2], 1);
        }

        /* If fpzip failed in any way, it returns zero */
        if (outbytes == 0)
        {
            free(cbuf);
            return early_retval;
        }

        /* We had a success. So, free old buffer and return new values */
        free(*buf);
        *buf = cbuf;
        *buf_size = max_outbytes;
        return outbytes;
    }
}
static H5Z_class_t db_hdf5_fpzip_class;
#endif /* HAVE_FPZIP } */

#ifndef HAVE_HZIP /* { */
/*ARGSUSED*/
static void
FreeNodelists(DBfile_hdf5 *dbfile, char const *meshname) {}
#else

/* The following section of code is used to mange caching of nodelists
   to support HZIP compression.

   Programmer: Mark C. Miller
   Created:    July, 2008
*/

#define MAX_NODELIST_INFOS 32 
typedef struct _zlInfo {
    DBfile_hdf5 *db5file;
    char *meshname;
    char *zlname;
    DBzonelist *zl;
} zlInfo_t;
static zlInfo_t keptNodelistInfos[MAX_NODELIST_INFOS];

/*
   We can lookup a nodelist either by its name or the name of the mesh that
   uses it. The preference is to use the name of the zonelist itself.
*/
static const DBzonelist*
LookupNodelist(DBfile_hdf5 *dbfile, char const *zlname, char const *meshname)
{
    int i;
    char fullmname[256];
    char fullzlname[256];
    if (zlname)
        db_hdf5_fullname(dbfile, (char*) zlname, fullzlname);
    if (meshname)
        db_hdf5_fullname(dbfile, (char*) meshname, fullmname);
    for (i = 0; i < MAX_NODELIST_INFOS; i++)
    {
        if (keptNodelistInfos[i].zl && (keptNodelistInfos[i].db5file == dbfile))
        {
            if (zlname && keptNodelistInfos[i].zlname &&
                (strcmp(fullzlname, keptNodelistInfos[i].zlname) == 0))
            {
                if (meshname && !keptNodelistInfos[i].meshname)
                    keptNodelistInfos[i].meshname = STRDUP(fullmname);
                return keptNodelistInfos[i].zl;
            }
            if (meshname && keptNodelistInfos[i].meshname &&
                (strcmp(fullmname, keptNodelistInfos[i].meshname) == 0))
            {
                if (zlname && !keptNodelistInfos[i].zlname)
                    keptNodelistInfos[i].zlname = STRDUP(fullzlname);
                return keptNodelistInfos[i].zl;
            }
        }
    }
    return 0;
}

/*
 Part of registering a nodelist is to first check to see if we don't already
 have one. So, we call LookupNodelist as a first step.
*/
static void 
RegisterNodelist(DBfile_hdf5 *dbfile, char const *zlname, char const *meshname,
    int ntopodims, int nzones, int origin, int const *nodelist)
{
    DBzonelist *zl;
    int i;
    int lnodelist = (1<<ntopodims) * nzones;
    int snodelist = lnodelist * sizeof(int);
    char fullname[256], fullmname[256];

    if (LookupNodelist(dbfile, zlname, meshname))
        return;

    if (zlname)
        db_hdf5_fullname(dbfile, (char*) zlname, fullname);
    if (meshname)
        db_hdf5_fullname(dbfile, (char*) meshname, fullmname);

    zl = DBAllocZonelist();
    if (!zl) return;
    zl->ndims = ntopodims;
    zl->nzones = nzones;
    zl->nshapes = 1;
    zl->origin = origin;
    zl->shapecnt = (int *)malloc(sizeof(int));
    zl->shapecnt[0] = nzones;
    zl->shapetype = (int *)malloc(sizeof(int));
    zl->shapetype[0] = ntopodims == 2 ? DB_ZONETYPE_QUAD : DB_ZONETYPE_HEX;
    zl->shapesize = (int *)malloc(sizeof(int));
    zl->shapesize[0] = (1 << ntopodims); 
    zl->lnodelist = lnodelist;
    zl->nodelist = (int *)malloc(snodelist);
    memcpy(zl->nodelist, nodelist, snodelist);

    for (i = 0; i < MAX_NODELIST_INFOS; i++)
    {
        if (keptNodelistInfos[i].zl == 0)
            break;
    }
    if (i == MAX_NODELIST_INFOS)
        return;

    keptNodelistInfos[i].db5file = dbfile;
    if (zlname)
        keptNodelistInfos[i].zlname = STRDUP(fullname);
    if (meshname)
        keptNodelistInfos[i].meshname = STRDUP(fullmname);
    keptNodelistInfos[i].zl = zl;
}

static void
AddMeshnameToNodelist(DBfile_hdf5 *dbfile, char const *zlname, char const *meshname)
{
    int i;
    char fullmname[256];
    char fullzlname[256];

    db_hdf5_fullname(dbfile, (char*) zlname, fullzlname);
    db_hdf5_fullname(dbfile, (char*) meshname, fullmname);

    for (i = 0; i < MAX_NODELIST_INFOS; i++)
    {
        if (keptNodelistInfos[i].zl && (keptNodelistInfos[i].db5file == dbfile))
        {
            if (keptNodelistInfos[i].zlname && !keptNodelistInfos[i].meshname &&
                strcmp(fullzlname, keptNodelistInfos[i].zlname) == 0)
            {
                keptNodelistInfos[i].meshname = STRDUP(fullmname);
                break;
            }
        }
    }
}

static void
FreeNodelists(DBfile_hdf5 *dbfile, char const *meshname)
{
    int i;
    if (meshname && dbfile) /* clear all for a given mesh */
    {
        char fullmname[256];
        db_hdf5_fullname(dbfile, (char*) meshname, fullmname);

        for (i = 0; i < MAX_NODELIST_INFOS; i++)
        {
            if (keptNodelistInfos[i].zl && keptNodelistInfos[i].meshname &&
                (strcmp(fullmname, keptNodelistInfos[i].meshname) == 0))
            {
                FREE(keptNodelistInfos[i].meshname);
                FREE(keptNodelistInfos[i].zlname);
                DBFreeZonelist(keptNodelistInfos[i].zl);
                memset(&keptNodelistInfos[i], 0x0, sizeof(keptNodelistInfos[i]));
            }
        }
    }
    else if (dbfile) /* clear all for a given file */
    {
        for (i = 0; i < MAX_NODELIST_INFOS; i++)
        {
            if (keptNodelistInfos[i].zl && (keptNodelistInfos[i].db5file == dbfile))
            {
                FREE(keptNodelistInfos[i].meshname);
                FREE(keptNodelistInfos[i].zlname);
                DBFreeZonelist(keptNodelistInfos[i].zl);
                memset(&keptNodelistInfos[i], 0x0, sizeof(keptNodelistInfos[i]));
            }
        }
    }
    else /* clear everything that is non-empty */
    {
        for (i = 0; i < MAX_NODELIST_INFOS; i++)
        {
            if (keptNodelistInfos[i].zl)
            {
                FREE(keptNodelistInfos[i].meshname);
                FREE(keptNodelistInfos[i].zlname);
                DBFreeZonelist(keptNodelistInfos[i].zl);
                memset(&keptNodelistInfos[i], 0x0, sizeof(keptNodelistInfos[i]));
            }
        }
    }
}

/* The following section of code are HDF5 filters to implement FPZIP
   compression algorithms

   Programmer: Mark C. Miller
   Created:    July, 2008

   Modifications:
     Mark C. Miller, Thu Feb  4 20:02:00 PST 2010
     Corrected type for hztype local variable from int to HZtype.

*/

static HZtype silo2hztype(int silo_type)
{
    switch (silo_type)
    {
        case DB_CHAR: return hzUCHAR;
        case DB_SHORT: return hzUSHORT;
        case DB_INT: return hzINT;
        case DB_FLOAT: return hzFLOAT;
        case DB_DOUBLE: return hzDOUBLE;
    }
    return hzUCHAR;
}

static int hztype2silo(HZtype hztype)
{
    switch (hztype)
    {
        case hzUCHAR: return DB_CHAR;
        case hzUSHORT: return DB_SHORT;
        case hzINT: return DB_INT;
        case hzFLOAT: return DB_FLOAT;
        case hzDOUBLE: return DB_DOUBLE;
    }
    return DB_CHAR;
}

static int sizeof_hztype(HZtype hztype)
{
    switch (hztype)
    {
        case hzUCHAR: return sizeof(unsigned char);
        case hzUSHORT: return sizeof(unsigned short);
        case hzINT: return sizeof(int);
        case hzFLOAT: return sizeof(float);
        case hzDOUBLE: return sizeof(double);
    }
    return sizeof(double);
}

typedef struct db_hdf5_hzip_params_t {
    /* these items set DBSetCompression call */
    unsigned            codec;
    void const         *params;

    /* these items set by hdf5 driver as needed. */
    int                 iszl;
    char const         *zlname;
    char const         *meshname;
    DBfile_hdf5        *dbfile;
    int                 zlorigin;
    int                 isquad;

    /* these items set by hdf5 filter's set_local method */
    int                 totsize1d;
    int                 ndims;
    int                 dims[10];
    HZtype              hztype;
} db_hdf5_hzip_params_t;
static db_hdf5_hzip_params_t db_hdf5_hzip_params;

static void
db_hdf5_hzip_clear_params()
{
    unsigned tmpcodec = db_hdf5_hzip_params.codec;
    void const *tmpparams = db_hdf5_hzip_params.params;
    memset(&db_hdf5_hzip_params, 0, sizeof(db_hdf5_hzip_params));
    db_hdf5_hzip_params.codec = tmpcodec;
    db_hdf5_hzip_params.params = tmpparams;
}

static htri_t
db_hdf5_hzip_can_apply(hid_t dcpl_id, hid_t type_id, hid_t space_id)
{
    return 1;
}

static herr_t
db_hdf5_hzip_set_local(hid_t dcpl_id, hid_t type_id, hid_t space_id)
{
    int i;
    hsize_t dims[10] = {1,1,1,1,1,1,1,1,1,1}, maxdims[10];
    db_hdf5_hzip_params.hztype = silo2hztype(hdf2silo_type(type_id));
    db_hdf5_hzip_params.totsize1d = (int) H5Sget_simple_extent_npoints(space_id);
    db_hdf5_hzip_params.ndims = H5Sget_simple_extent_ndims(space_id);
    H5Sget_simple_extent_dims(space_id, dims, maxdims);
    for (i = 0; i < db_hdf5_hzip_params.ndims; i++)
        db_hdf5_hzip_params.dims[i] = (int) dims[i];
    return 1;
}

static size_t
db_hdf5_hzip_filter_op(unsigned int flags, size_t cd_nelmts,
    const unsigned int cd_values[], size_t nbytes,
    size_t *buf_size, void **buf)
{
    int early_retval = 0;

    if (flags & H5Z_FLAG_REVERSE) /* read case */
    {
        if (db_hdf5_hzip_params.iszl) /* ucd mesh zonelist */
        {
            HZMstream *stream;
            int ndims = 0, nzones = 0, nread;
            int *nodelist = 0;
            size_t new_buf_size, tmp_size;

            /* To query stream for ndims, we need to specify a permutation
               which may be wrong. So, we open, query, close and re-open. */
            stream = hzip_mesh_open_mem(*buf, nbytes, 0);
            if (stream == 0) return early_retval;

            ndims = hzip_mesh_dimensions(stream);
            hzip_mesh_close(stream);
            if (ndims < 2 || ndims > 3) return early_retval;

            /* Ok, no re-open the stream with correct permutation */
            stream = hzip_mesh_open_mem(*buf, nbytes, SILO_HZIP_PERMUTATION[ndims]);
            if (stream == 0) return early_retval;

            ndims = hzip_mesh_dimensions(stream);
            if (ndims < 2 || ndims > 3) {hzip_mesh_close(stream); return early_retval;}

            nzones = hzip_mesh_cells(stream);
            if (nzones < 0) {hzip_mesh_close(stream); return early_retval;}

            if (db_hdf5_safe_mul_size((size_t) (1u<<ndims), (size_t) nzones, &tmp_size) < 0 ||
                db_hdf5_safe_mul_size(tmp_size, sizeof(int), &new_buf_size) < 0 ||
                new_buf_size == 0)
            {hzip_mesh_close(stream); return early_retval;}

            nodelist = (int *) malloc(new_buf_size);
            if (nodelist == 0) {hzip_mesh_close(stream); return early_retval;}

            nread = hzip_mesh_read(stream, nodelist, nzones);
            hzip_mesh_close(stream);
            if (nread != nzones)
            {
                free(nodelist);
                return early_retval;
            }

            RegisterNodelist(db_hdf5_hzip_params.dbfile, db_hdf5_hzip_params.zlname,
                db_hdf5_hzip_params.meshname, ndims, nzones,
                db_hdf5_hzip_params.zlorigin, nodelist);

            free(*buf);
            *buf = (void*) nodelist;
            *buf_size = new_buf_size; 
            return new_buf_size;
        }
        else /* ucd or quad mesh node-centered (not zone-centered) variables */
        {
            HZNstream *stream;
            int i, ndims, nread, nzones;
            unsigned nnodes;
            void *var = 0;
            size_t new_buf_size, tmp_size;
            HZtype hztype;
            int *nodelist;
            int perm, origin;

            /* To query stream for ndims, we need to specify a permutation
               which may be wrong. So, we open, query, close and re-open. */
            stream = hzip_node_open_mem(*buf, nbytes, 0, 0);
            if (stream == 0) return early_retval;

            ndims = hzip_node_dimensions(stream);
            hzip_node_close(stream);
            if (ndims < 2 || ndims > 3) return early_retval;

            if (db_hdf5_hzip_params.isquad)
            {
                size_t nzones_sz = 1;
                for (i = 0; i < db_hdf5_hzip_params.ndims; i++)
                {
                    if (db_hdf5_hzip_params.dims[i] < 1 ||
                        db_hdf5_safe_mul_size(nzones_sz, (size_t) (db_hdf5_hzip_params.dims[i]-1), &nzones_sz) < 0 ||
                        nzones_sz > INT_MAX)
                        return early_retval;
                }
                nzones = (int) nzones_sz;

                if (db_hdf5_safe_mul_size((size_t) (1u<<ndims), nzones_sz, &tmp_size) < 0 ||
                    db_hdf5_safe_mul_size(tmp_size, sizeof(int), &tmp_size) < 0 ||
                    tmp_size == 0)
                    return early_retval;

                nodelist = (int *) malloc(tmp_size);
                if (nodelist == 0) return early_retval;
                perm = hzip_mesh_construct(nodelist, (unsigned) ndims,
                    (const unsigned *) db_hdf5_hzip_params.dims, 0);

                origin = 0;
            }
            else
            {
                const DBzonelist *zl = LookupNodelist(db_hdf5_hzip_params.dbfile,
                    db_hdf5_hzip_params.zlname, db_hdf5_hzip_params.meshname);
                if (zl == 0 || zl->nodelist == 0)
                    return early_retval;
                nodelist = zl->nodelist;
                nzones = zl->nzones;
                origin = zl->origin;
                perm = SILO_HZIP_PERMUTATION[ndims];
            }

            /* Ok, now open with correct permutation */
            stream = hzip_node_open_mem(*buf, nbytes, perm, origin);
            if (stream == 0) return early_retval;

            ndims = hzip_node_dimensions(stream);
            if (ndims < 2 || ndims > 3) {hzip_node_close(stream); return early_retval;}

            nnodes = hzip_node_count(stream);
            if (nnodes > INT_MAX) {hzip_node_close(stream); return early_retval;}

            hztype = hzip_node_type(stream);
            if (db_hdf5_safe_mul_size((size_t) nnodes, sizeof_hztype(hztype), &new_buf_size) < 0 ||
                new_buf_size == 0)
            {hzip_node_close(stream); return early_retval;}

            var = malloc(new_buf_size);
            if (var == 0) {hzip_node_close(stream); return early_retval;}

            nread = hzip_node_read(stream, var, nodelist, nzones);
            hzip_node_close(stream);
            if (nread != (int) nnodes)
            {
                if (db_hdf5_hzip_params.isquad)
                    free(nodelist);
                free(var);
                return early_retval;
            }

            if (db_hdf5_hzip_params.isquad)
                free(nodelist);
            free(*buf);
            *buf = var;
            *buf_size = new_buf_size; 
            return new_buf_size;
        }
    }
    else /* write case */
    {
        if (db_hdf5_hzip_params.iszl) /* ucd mesh zonelist */
        {
            int outbytes, ntopo, nzones;
            size_t max_outbytes;
            HZMstream* stream;
            unsigned char *buffer = 0;

            const DBzonelist *zl = LookupNodelist(db_hdf5_hzip_params.dbfile,
                db_hdf5_hzip_params.zlname, db_hdf5_hzip_params.meshname);
            if (zl == 0 || zl->nodelist == 0) return early_retval;
            ntopo = zl->ndims;
            nzones = zl->nzones;

            size_t tmp_size;
            if (SILO_Globals.compressionMinratio <= 0) return early_retval;
            if (db_hdf5_safe_mul_size((size_t) (1u<<ntopo), (size_t) nzones, &tmp_size) < 0 ||
                db_hdf5_safe_mul_size(tmp_size, sizeof(int), &tmp_size) < 0)
                return early_retval;
            max_outbytes = tmp_size / SILO_Globals.compressionMinratio;
            if (max_outbytes == 0) return early_retval;
            buffer = (unsigned char *) malloc(max_outbytes);
            if (buffer == 0) return early_retval;

            stream = hzip_mesh_create_mem(buffer, max_outbytes, SILO_HZIP_PERMUTATION[ntopo],
                ntopo, nzones, db_hdf5_hzip_params.codec, db_hdf5_hzip_params.params);

            if (stream == 0)
            {
                free(buffer); 
                return early_retval;
            }

            if (hzip_mesh_write(stream, (int *)(*buf), nzones) < 0)
            {
                hzip_mesh_close(stream);
                free(buffer); 
                return early_retval;
            }

            if ((outbytes = hzip_mesh_close(stream)) < 0)
            {
                free(buffer); 
                return early_retval;
            }

            free(*buf);
            *buf = buffer;
            *buf_size = max_outbytes;
            return outbytes;
        }
        else /* ucd or quad mesh coords or nodal variables */
        {
            int i, outbytes;
            size_t max_outbytes;
            HZNstream* stream;
            unsigned char *buffer = 0;
            int *nodelist;
            int ndims, origin, nzones, perm;

            /* Find the nodelist for this write */
            if (db_hdf5_hzip_params.isquad)
            {
                ndims = db_hdf5_hzip_params.ndims;
                origin = 0;
                size_t nzones_sz = 1, nodelist_sz;
                for (i = 0; i < ndims; i++)
                {
                    if (db_hdf5_hzip_params.dims[i] < 1 ||
                        db_hdf5_safe_mul_size(nzones_sz, (size_t) (db_hdf5_hzip_params.dims[i]-1), &nzones_sz) < 0 ||
                        nzones_sz > INT_MAX)
                        return early_retval;
                }
                nzones = (int) nzones_sz;
                if (db_hdf5_safe_mul_size((size_t) (1u<<ndims), nzones_sz, &nodelist_sz) < 0 ||
                    db_hdf5_safe_mul_size(nodelist_sz, sizeof(int), &nodelist_sz) < 0 ||
                    nodelist_sz == 0)
                    return early_retval;
                nodelist = (int *) malloc(nodelist_sz);
                if (nodelist == 0) return early_retval;
                perm = hzip_mesh_construct(nodelist, (unsigned) ndims,
                    (const unsigned int *) db_hdf5_hzip_params.dims, 0);
            }
            else
            {
                const DBzonelist *zl = LookupNodelist(db_hdf5_hzip_params.dbfile,
                                          db_hdf5_hzip_params.zlname,
                                          db_hdf5_hzip_params.meshname);
                if (zl == 0 || zl->nodelist == 0) return early_retval;
                nodelist = zl->nodelist;
                ndims = zl->ndims;
                origin = zl->origin;
                nzones = zl->nzones;
                perm = SILO_HZIP_PERMUTATION[ndims];
            }

            if (SILO_Globals.compressionMinratio <= 0) return early_retval;
            if (db_hdf5_hzip_params.totsize1d < 0 ||
                db_hdf5_safe_mul_size((size_t) db_hdf5_hzip_params.totsize1d,
                                      sizeof_hztype(db_hdf5_hzip_params.hztype),
                                      &max_outbytes) < 0)
                return early_retval;
            max_outbytes /= SILO_Globals.compressionMinratio;
            if (max_outbytes == 0) return early_retval;
            buffer = (unsigned char *) malloc(max_outbytes);
            if (buffer == 0) return early_retval;

            stream = hzip_node_create_mem(buffer, max_outbytes, perm,
                origin, ndims, db_hdf5_hzip_params.totsize1d, db_hdf5_hzip_params.hztype,
                db_hdf5_hzip_params.codec, 0);

            if (stream == 0)
            {
                if (db_hdf5_hzip_params.isquad)
                    free(nodelist);
                free(buffer);
                return early_retval;
            }

            if (hzip_node_write(stream, *buf, nodelist, nzones) < 0)
            {
                hzip_node_close(stream);
                if (db_hdf5_hzip_params.isquad)
                    free(nodelist);
                free(buffer);
                return early_retval;
            }

            if ((outbytes = hzip_node_close(stream)) < 0)
            {
                if (db_hdf5_hzip_params.isquad)
                    free(nodelist);
                free(buffer);
                return early_retval;
            }

            /* We had a success. So, free old buffer and return new values */
            if (db_hdf5_hzip_params.isquad)
                free(nodelist);
            free(*buf);
            *buf = buffer;
            *buf_size = max_outbytes;
            return outbytes;
        }
    }
}
static H5Z_class_t db_hdf5_hzip_class;
#endif /* !HAVE_HZIP } */

INTERNAL char const *
friendly_name(DBfile *_dbfile, char const *base_name, char const *fmtstr, void const *val)
{
    static char retval[1024];
    static char totfmtstr[1024];
    char typechar;
    int i, flen;

    if (DBGetFriendlyHDF5NamesFile(_dbfile) == FALSE)
        return 0;

    if (fmtstr == 0)
        return base_name;

    sprintf(totfmtstr, "%s%s", base_name, fmtstr);
    if (val == 0)
        return totfmtstr;

    flen = strlen(fmtstr);
    for (i = 0; i < flen; i++)
    {
        if (fmtstr[i] == '%')
           break;
    }
    typechar = i+1 < flen ? fmtstr[i+1] : '\0';
    switch (typechar)
    {
        case 'd': sprintf(retval, totfmtstr, *((int const*) val)); break; 
        case 's': sprintf(retval, totfmtstr, *((char const*) val)); break;
        case 'f': sprintf(retval, totfmtstr, *((const float*) val)); break;
        default: return totfmtstr;
    }
    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_file
 *
 * Purpose:     Returns the hdf5 file ID associated with the DBfile. The
 *              hdf5 file is not reopened; the caller should not close the
 *              file id.
 *
 * Return:      Success:        HDF5 file id
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 23, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t
db_hdf5_file(DBfile *_dbfile)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    return dbfile->fid;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_cwg
 *
 * Purpose:     Returns the hdf5 group ID for the current working group of
 *              the DBfile. The group is not reopened; the caller should not
 *              close the group ID.
 *
 * Return:      Success:        HDF5 current working group ID
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 23, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t
db_hdf5_cwg(DBfile *_dbfile)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    return dbfile->cwg;
}

/*-------------------------------------------------------------------------
 * Function: db_hdf5_get_obj_dsnames
 *
 * Purpose: Given arbitrary Silo object name string, return the list of
 * names of datasets for the bulk storage (raw data) of the object.
 * 
 * Mark C. Miller, Wed Apr 20 17:30:36 PDT 2016
 *-------------------------------------------------------------------------
 */
#define DB_OBJ_CASE(ST, H5MT, MEMCNT, MEMNAME)                   \
    case ST:                                                     \
    {   H5MT m; int i;                                           \
        memset(&m, 0, sizeof m);                                 \
        if ((attr=H5Aopen_name(o, "silo"))<0 ||                  \
             H5Aread(attr, H5MT##5, &m)<0 ||                     \
             H5Aclose(attr)<0)                                   \
             *dscount = 0;                                       \
        else                                                     \
             *dscount = m.MEMCNT;                                \
        *dsnames = (char **) calloc(*dscount, sizeof(char**));   \
        for (i = 0; i < *dscount; i++)                           \
            (*dsnames)[i] = strdup(m.MEMNAME);                   \
        break;                                                   \
    }

PRIVATE int
db_hdf5_get_obj_dsnames(DBfile *_dbfile, char const *name, int *dscount, char ***dsnames)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_get_obj_dsnames";

    H5E_BEGIN_TRY
    {
        int _objtype;
        hid_t o, attr;

        /* If the named object is a group, there isn't anything to do */
        o = H5Gopen(dbfile->cwg, name, H5P_DEFAULT);
        if (o > 0)
        {
            H5Gclose(o);
            return 0;
        }

        /* If the named object *is* a dataset, just return that name */
        o = H5Dopen(dbfile->cwg, name, H5P_DEFAULT);
        if (o > 0)
        {
            H5Dclose(o);
            *dsnames = (char **) calloc(1, sizeof(char**));
            (*dsnames)[0] = strdup(name);
            *dscount = 1;
            return 1;
        }

        /* Its probably an aggregate Silo object */
        o = H5Topen(dbfile->cwg, name, H5P_DEFAULT);
        if (o < 0) return -1;

        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0)
        {
            H5Tclose(o);
            return -1;
        }

        switch(_objtype)
        {
            DB_OBJ_CASE(DB_QUADVAR, DBquadvar_mt, nvals, value[i])
            /*DB_OBJ_CASE(DB_QUAD_RECT, DBquadmesh_mt, nspace, coord[i]) wont work for rect case */
            DB_OBJ_CASE(DB_QUAD_CURV, DBquadmesh_mt, nspace, coord[i])
            DB_OBJ_CASE(DB_QUADMESH, DBquadmesh_mt, nspace, coord[i])
            DB_OBJ_CASE(DB_UCDVAR, DBucdvar_mt, nvals, value[i])
            DB_OBJ_CASE(DB_UCDMESH, DBucdmesh_mt, ndims, coord[i])
            DB_OBJ_CASE(DB_POINTVAR, DBpointvar_mt, nvals, data[i])
            DB_OBJ_CASE(DB_POINTMESH, DBpointmesh_mt, ndims, coord[i])
            DB_OBJ_CASE(DB_CSGVAR, DBcsgvar_mt, nvals, vals[i])
            DB_OBJ_CASE(DB_CURVE, DBcurve_mt, npts?1:1, yvarname)
        }
        H5Tclose(o);

    } H5E_END_TRY;

    return 1;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_get_cmemb
 *
 * Purpose:     Returns information about a member of a compound type. If
 *              the member is an array then it returns information about
 *              the array instead.
 *
 * Return:      Success:        HDF5 datatype of member. The number of
 *                              items is returned in SIZE.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke, 2001-01-26
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t
db_hdf5_get_cmemb(hid_t compound_type, int membno, int *ndims/*out*/,
                  hsize_t size[3]/*out*/)
{
    hid_t       type;

    if ((type=H5Tget_member_type(compound_type, membno))<0) return -1;
    
#if HDF5_VERSION_GE(1,4,0)
    if (H5T_ARRAY==H5Tget_class(type)) {
        int i;
        *ndims = H5Tget_array_ndims(type);
        assert(*ndims<=3);
        H5Tget_array_dims(type, size);
        type = H5Tget_super(type);
    } else {
        *ndims = 0;
    }
#else
    *ndims = H5Tget_member_dims(compound_type, membno, size, NULL);
#endif
    return type;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_put_cmemb
 *
 * Purpose:     
 *
 * Return:      Success:
 *                      
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke, 2001-01-26
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_put_cmemb(hid_t compound_type, char const *name, size_t offset,
                  int ndims, hsize_t const *dim, hid_t type)
{
    int         retval;
    
#if HDF5_VERSION_GE(1,4,0)
    if (ndims) {
        int i;
        type = H5Tarray_create(type, ndims, dim);
    }
    retval = H5Tinsert(compound_type, name, offset, type);
    if (ndims) H5Tclose(type);
#else
    retval = H5Tinsert_array(compound_type, name, offset, ndims, dim, NULL,
                             type);
#endif
    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    T_str
 *
 * Purpose:     Returns fixed-length hdf5 string data type which has just
 *              enough space to store the specified string.
 *
 * Return:      Success:        An hdf5 data type which will be closed on the
 *                              next call to this function.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 23, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int T_str_stype_set = 0;
PRIVATE hid_t
T_str(char *s)
{
    static hid_t        stype = -1;

    if (!s || !*s) return -1;
    if (T_str_stype_set && stype>=0) H5Tclose(stype);
    stype = H5Tcopy(H5T_C_S1);
    H5Tset_size(stype, strlen(s)+1);
    T_str_stype_set = 1;
    return stype;
}

/*-------------------------------------------------------------------------
 * Function:    silo_walk_cb 
 *
 * Purpose:     Error stack walk callback.
 *              Currently, only detects checksum errors.
 *
 * Return:      SUCCESS 
 *
 * Programmer:  Mark C. Miller 
 *              Tuesday, May 2, 2006 
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Jul 17 23:11:41 PDT 2008
 *   Added code to detect compression errors.
 *-------------------------------------------------------------------------
 */
PRIVATE herr_t
silo_walk_cb(unsigned n, const H5E_error2_t *err_desc, void *client_data) 
{
    int *silo_error_code_p = (int *) client_data;

    /* Note that error code can be overwritten by later strstr
       comparisons. Where both checksum and compression errors
       occur, we declare it a compression error. */
    if (strstr(err_desc->desc, "letcher32") != 0)
        *silo_error_code_p = E_CHECKSUM;
    if (strstr(err_desc->desc, "zip") != 0)
        *silo_error_code_p = E_COMPRESSION;
    if (strstr(err_desc->desc, "Lindstrom-") != 0)
        *silo_error_code_p = E_COMPRESSION;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    hdf5_to_silo_error 
 *
 * Purpose:     Convert HDF5 error stack to a silo error code. 
 *              Currently, only detects checksum errors.
 *
 * Return:      SUCCESS 
 *
 * Programmer:  Mark C. Miller 
 *              Tuesday, May 2, 2006 
 *
 *-------------------------------------------------------------------------
 */
PRIVATE void
hdf5_to_silo_error(char const *vname, char const *fname)
{
    int silo_error_code = E_NOERROR;

    H5Ewalk(H5E_DEFAULT, H5E_WALK_UPWARD, silo_walk_cb, &silo_error_code);

    if (silo_error_code == E_NOERROR)
        silo_error_code = E_CALLFAIL;

    db_perror((char*)vname, silo_error_code, (char*)fname);
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_init
 *
 * Purpose:     One-time initializations for this driver.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Monday, March 22, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Wed Sep  2 16:41:32 PDT 2009
 *   Made conditional compilation logic for VERSION_GE take into account
 *   H5_USE_16_API setting.
 *
 *   Mark C. Miller, Mon Sep 21 15:17:08 PDT 2009
 *   Adding support for long long type.
 *
 *   Mark C. Miller, Thu Feb 11 09:37:41 PST 2010
 *   Added logic to set HDF5's error output based on Silo's settings.
 *
 *   Mark C. Miller, Wed Jul 14 20:48:15 PDT 2010
 *   Added support for namescheme/empty_list options for multi-block objs.
 *-------------------------------------------------------------------------
 */
PRIVATE void
db_hdf5_init(void)
{

#if !(HDF5_VERSION_GE(1,8,4) && !HDF5_VERSION_GE(1,8,5))
    static int          ncalls;
    if (ncalls++) return;               /*already initialized*/
#else
    H5open();
#endif

    /* Turn off error messages from the hdf5 library */
    if (SILO_Globals._db_err_level_drvr == DB_ALL)
        H5Eset_auto(H5E_DEFAULT, (H5E_auto2_t) H5Eprint2, stderr);
    else
        H5Eset_auto(H5E_DEFAULT, NULL, NULL);

    /* Define a scalar data space */
    SCALAR = H5Screate(H5S_SCALAR);

    /* Define atomic data types */
    T_char = H5T_NATIVE_UCHAR;
    T_short = H5T_NATIVE_SHORT;
    T_int = H5T_NATIVE_INT;
    T_long = H5T_NATIVE_LONG;
    T_llong = H5T_NATIVE_LLONG;
    T_float = H5T_NATIVE_FLOAT;
    T_double = H5T_NATIVE_DOUBLE;
    
    T_str256 = H5Tcopy(H5T_C_S1);       /*this is never freed!*/
    H5Tset_size(T_str256, 256);

    P_ckcrprops = H5Pcreate(H5P_DATASET_CREATE); /* never freed */
    if (DBGetEnableChecksums())
       H5Pset_fletcher32(P_ckcrprops);

    /* for H5Dread calls, H5P_DEFAULT results in *enabled*
       checksums. So, we build the DISabled version here. */
    P_ckrdprops = H5Pcreate(H5P_DATASET_XFER);   /* never freed */
    H5Pset_edc_check(P_ckrdprops, H5Z_DISABLE_EDC);

#ifdef HAVE_FPZIP /* { */
    db_hdf5_fpzip_params.loss = 0;
#if HDF5_VERSION_GE(1,8,0) && !defined(H5_USE_16_API)
    db_hdf5_fpzip_class.version = H5Z_CLASS_T_VERS;
    db_hdf5_fpzip_class.encoder_present = 1;
    db_hdf5_fpzip_class.decoder_present = 1;
#endif
    db_hdf5_fpzip_class.id = DB_HDF5_FPZIP_ID;
    db_hdf5_fpzip_class.name = "Lindstrom-fpzip";
    db_hdf5_fpzip_class.can_apply = db_hdf5_fpzip_can_apply;
    db_hdf5_fpzip_class.set_local = db_hdf5_fpzip_set_local;
    db_hdf5_fpzip_class.filter = db_hdf5_fpzip_filter_op;
    H5Zregister(&db_hdf5_fpzip_class);
#endif /* HAVE_FPZIP } */

#ifdef HAVE_HZIP /* { */

#ifdef HAVE_LIBZ
    db_hdf5_hzip_zlib_codec_params = hzm_codec_zlib;
#else
    db_hdf5_hzip_base_codec_params = hzm_codec_base;
#endif

#ifdef HAVE_LIBZ
    db_hdf5_hzip_params.codec = HZM_CODEC_ZLIB;
    db_hdf5_hzip_params.params = &db_hdf5_hzip_zlib_codec_params;
    ((struct HZMCODECzlib *) db_hdf5_hzip_params.params)->bits = 12;
#else
    db_hdf5_hzip_params.codec = HZM_CODEC_BASE;
    db_hdf5_hzip_params.params = &db_hdf5_hzip_base_codec_params;
    ((struct HZMCODECbase *) db_hdf5_hzip_params.params)->bits = 12;
#endif

#if HDF5_VERSION_GE(1,8,0) && !defined(H5_USE_16_API)
    db_hdf5_hzip_class.version = H5Z_CLASS_T_VERS;
#ifndef _MSC_VER
#warning SHOULD WE DISABLE HZIP AND FPZIP ENCODING
#endif
    db_hdf5_hzip_class.encoder_present = 1;
    db_hdf5_hzip_class.decoder_present = 1;
#endif
    db_hdf5_hzip_class.id = DB_HDF5_HZIP_ID;
    db_hdf5_hzip_class.name = "Lindstrom-hzip";
    db_hdf5_hzip_class.can_apply = db_hdf5_hzip_can_apply;
    db_hdf5_hzip_class.set_local = db_hdf5_hzip_set_local;
    db_hdf5_hzip_class.filter = db_hdf5_hzip_filter_op;

    H5Zregister(&db_hdf5_hzip_class);
    /* Initialize support data structures for hzip */
    memset(keptNodelistInfos, 0x0, sizeof(keptNodelistInfos));

#endif /* HAVE_HZIP } */

#ifdef HAVE_ZFP /* { */
    H5Z_zfp_initialize();
    zfp_init_zfp();
#endif /* HAVE_ZFP } */

    /* Define compound data types */
    STRUCT(DBcurve) {
        MEMBER_S(int,           npts);
        MEMBER_S(int,           guihide);
        MEMBER_S(int,           coord_sys);
        MEMBER_S(str256,        xvarname);
        MEMBER_S(str256,        yvarname);
        MEMBER_S(str256,        label);
        MEMBER_S(str256,        xlabel);
        MEMBER_S(str256,        ylabel);
        MEMBER_S(str256,        xunits);
        MEMBER_S(str256,        yunits);
        MEMBER_S(str256,        reference);
        MEMBER_S(double,        missing_value);
    } DEFINE;

    STRUCT(DBcsgmesh) {
        MEMBER_S(int,           block_no);
        MEMBER_S(int,           group_no);
        MEMBER_S(int,           cycle);
        MEMBER_S(int,           nbounds);
        MEMBER_S(int,           lcoeffs);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           origin);
        MEMBER_3(double,        min_extents);
        MEMBER_3(double,        max_extents);
        MEMBER_S(int,           guihide);
        MEMBER_R(str256,        units, 3);
        MEMBER_R(str256,        labels, 3);
        MEMBER_S(str256,        name);
        MEMBER_S(str256,        typeflags);
        MEMBER_S(str256,        bndids);
        MEMBER_S(str256,        coeffs);
        MEMBER_S(str256,        zonel_name);
        MEMBER_S(str256,        bndnames);
        MEMBER_S(str256,        mrgtree_name);
        MEMBER_S(int,           tv_connectivity);
        MEMBER_S(int,           disjoint_mode);
        MEMBER_S(str256,        alt_nodenum_vars);
    } DEFINE;

    STRUCT(DBcsgvar) {
        MEMBER_S(int,           cycle);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           datatype);
        MEMBER_S(int,           nels);
        MEMBER_S(int,           nvals);
        MEMBER_S(int,           centering);
        MEMBER_S(int,           use_specmf);
        MEMBER_S(int,           ascii_labels);
        MEMBER_S(int,           guihide);
        MEMBER_S(str256,        name);
        MEMBER_S(str256,        units);
        MEMBER_S(str256,        label);
        MEMBER_R(str256,        vals,           MAX_VARS);
        MEMBER_S(str256,        meshname);
        MEMBER_S(str256,        region_pnames);
        MEMBER_S(int,           conserved);
        MEMBER_S(int,           extensive);
        MEMBER_S(double,        missing_value);
    } DEFINE;

    STRUCT(DBcsgzonelist) {
        MEMBER_S(int,           nregs);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           lxform);
        MEMBER_S(int,           datatype);
        MEMBER_S(int,           nzones);
        MEMBER_S(int,           min_index);
        MEMBER_S(int,           max_index);
        MEMBER_S(str256,        typeflags);
        MEMBER_S(str256,        leftids);
        MEMBER_S(str256,        rightids);
        MEMBER_S(str256,        xform);
        MEMBER_S(str256,        zonelist);
        MEMBER_S(str256,        regnames);
        MEMBER_S(str256,        zonenames);
        MEMBER_S(str256,        alt_zonenum_vars);
    } DEFINE;

    STRUCT(DBdefvars) {
        MEMBER_S(int,           ndefs);
        MEMBER_S(str256,        names);
        MEMBER_S(str256,        types);
        MEMBER_S(str256,        defns);
        MEMBER_S(str256,        guihides);
    } DEFINE;

    STRUCT(DBquadmesh) {
        MEMBER_R(str256,        coord,          3);
        MEMBER_3(double,        min_extents);
        MEMBER_3(double,        max_extents);
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           coordtype);
        MEMBER_S(int,           nspace);
        MEMBER_S(int,           nnodes);
        MEMBER_S(int,           facetype);
        MEMBER_S(int,           major_order);
        MEMBER_S(int,           cycle);
        MEMBER_S(int,           coord_sys);
        MEMBER_S(int,           planar);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           group_no);
        MEMBER_3(int,           dims);
        MEMBER_3(int,           min_index);
        MEMBER_3(int,           max_index);
        MEMBER_3(int,           baseindex);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           guihide);
        MEMBER_R(str256,        label,          3);
        MEMBER_R(str256,        units,          3);
        MEMBER_S(str256,        mrgtree_name);
        MEMBER_S(str256,        ghost_node_labels);
        MEMBER_S(str256,        ghost_zone_labels);
        MEMBER_S(str256,        alt_nodenum_vars);
        MEMBER_S(str256,        alt_zonenum_vars);
    } DEFINE;
    
    STRUCT(DBquadvar) {
        MEMBER_R(str256,        value,          MAX_VARS);
        MEMBER_R(str256,        mixed_value,    MAX_VARS);
        MEMBER_S(str256,        meshid);
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           nvals);
        MEMBER_S(int,           nels);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           mixlen);
        MEMBER_S(int,           major_order);
        MEMBER_S(int,           datatype);
        MEMBER_S(int,           cycle);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           use_specmf);
        MEMBER_S(int,           ascii_labels);
        MEMBER_3(int,           dims);
        MEMBER_3(int,           zones);
        MEMBER_3(int,           min_index);
        MEMBER_3(int,           max_index);
        MEMBER_3(float,         align);
        MEMBER_S(int,           guihide);
        MEMBER_S(str256,        label);
        MEMBER_S(str256,        units);
        MEMBER_S(str256,        region_pnames);
        MEMBER_S(int,           conserved);
        MEMBER_S(int,           extensive);
        MEMBER_S(int,           centering);
        MEMBER_S(double,        missing_value);
    } DEFINE;

    STRUCT(DBucdmesh) {
        MEMBER_R(str256,        coord,          3);
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           nnodes);
        MEMBER_S(int,           nzones);
        MEMBER_S(int,           facetype);
        MEMBER_S(int,           cycle);
        MEMBER_S(int,           coord_sys);
        MEMBER_S(int,           topo_dim);
        MEMBER_S(int,           planar);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           group_no);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(str256,        facelist);
        MEMBER_S(str256,        zonelist);
        MEMBER_S(str256,        phzonelist);
        MEMBER_S(str256,        gnodeno);
        MEMBER_3(double,        min_extents);
        MEMBER_3(double,        max_extents);
        MEMBER_S(int,           guihide);
        MEMBER_R(str256,        label,          3);
        MEMBER_R(str256,        units,          3);
        MEMBER_S(str256,        mrgtree_name);
        MEMBER_S(int,           tv_connectivity);
        MEMBER_S(int,           disjoint_mode);
        MEMBER_S(int,           gnznodtype);
        MEMBER_S(str256,        ghost_node_labels);
        MEMBER_S(str256,        alt_nodenum_vars);
    } DEFINE;
    
    STRUCT(DBucdvar) {
        MEMBER_R(str256,        value,          MAX_VARS);
        MEMBER_R(str256,        mixed_value,    MAX_VARS);
        MEMBER_S(str256,        meshid);
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           nvals);
        MEMBER_S(int,           nels);
        MEMBER_S(int,           centering);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           mixlen);
        MEMBER_S(int,           datatype);
        MEMBER_S(int,           cycle);
        MEMBER_S(int,           use_specmf);
        MEMBER_S(int,           ascii_labels);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           lo_offset);
        MEMBER_S(int,           hi_offset);
        MEMBER_S(int,           guihide);
        MEMBER_S(str256,        label);
        MEMBER_S(str256,        units);
        MEMBER_S(str256,        region_pnames);
        MEMBER_S(int,           conserved);
        MEMBER_S(int,           extensive);
        MEMBER_S(double,        missing_value);
    } DEFINE;

    STRUCT(DBfacelist) {
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           nfaces);
        MEMBER_S(int,           nshapes);
        MEMBER_S(int,           ntypes);
        MEMBER_S(int,           lnodelist);
        MEMBER_S(int,           origin);
        MEMBER_S(str256,        nodelist);
        MEMBER_S(str256,        shapecnt);
        MEMBER_S(str256,        shapesize);
        MEMBER_S(str256,        typelist);
        MEMBER_S(str256,        types);
        MEMBER_S(str256,        zoneno);
    } DEFINE;

    STRUCT(DBzonelist) {
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           nzones);
        MEMBER_S(int,           nshapes);
        MEMBER_S(int,           lnodelist);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           lo_offset);
        MEMBER_S(int,           hi_offset);
        MEMBER_S(str256,        nodelist);
        MEMBER_S(str256,        shapecnt);
        MEMBER_S(str256,        shapesize);
        MEMBER_S(str256,        shapetype);
        MEMBER_S(str256,        gzoneno);
        MEMBER_S(int,           gnznodtype);
        MEMBER_S(str256,        ghost_zone_labels);
        MEMBER_S(str256,        alt_zonenum_vars);
    } DEFINE;

    STRUCT(DBphzonelist) {
        MEMBER_S(int,           nfaces);
        MEMBER_S(int,           lnodelist);
        MEMBER_S(int,           nzones);
        MEMBER_S(int,           lfacelist);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           lo_offset);
        MEMBER_S(int,           hi_offset);
        MEMBER_S(str256,        nodecnt);
        MEMBER_S(str256,        nodelist);
        MEMBER_S(str256,        extface);
        MEMBER_S(str256,        facecnt);
        MEMBER_S(str256,        facelist);
        MEMBER_S(int,           gnznodtype);
        MEMBER_S(str256,        ghost_zone_labels);
        MEMBER_S(str256,        alt_zonenum_vars);
    } DEFINE;

    STRUCT(DBmaterial) {
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           nmat);
        MEMBER_S(int,           mixlen);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           major_order);
        MEMBER_S(int,           datatype);
        MEMBER_3(int,           dims);
        MEMBER_S(int,           allowmat0);
        MEMBER_S(int,           guihide);
        MEMBER_S(str256,        meshid);
        MEMBER_S(str256,        matlist);
        MEMBER_S(str256,        matnos);
        MEMBER_S(str256,        mix_vf);
        MEMBER_S(str256,        mix_next);
        MEMBER_S(str256,        mix_mat);
        MEMBER_S(str256,        mix_zone);
        MEMBER_S(str256,        matnames);
        MEMBER_S(str256,        matcolors);
    } DEFINE;

    STRUCT(DBmultimesh) {
        MEMBER_S(int,           nblocks);
        MEMBER_S(int,           cycle);
        MEMBER_S(int,           ngroups);
        MEMBER_S(int,           blockorigin);
        MEMBER_S(int,           grouporigin);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           extentssize);
        MEMBER_S(int,           guihide);
        MEMBER_S(str256,        meshtypes);
        MEMBER_S(str256,        meshnames);
        MEMBER_S(str256,        extents);
        MEMBER_S(str256,        zonecounts);
        MEMBER_S(str256,        has_external_zones);
        MEMBER_S(int,           lgroupings);
        MEMBER_S(str256,        groupings);
        MEMBER_S(str256,        groupnames);
        MEMBER_S(str256,        mrgtree_name);
        MEMBER_S(int,           tv_connectivity);
        MEMBER_S(int,           disjoint_mode);
        MEMBER_S(int,           topo_dim);
        MEMBER_S(str256,        file_ns_name);
        MEMBER_S(str256,        block_ns_name);
        MEMBER_S(int,           block_type);
        MEMBER_S(str256,        empty_list);
        MEMBER_S(int,           empty_cnt);
        MEMBER_S(int,           repr_block_idx);
        MEMBER_S(str256,        alt_nodenum_vars);
        MEMBER_S(str256,        alt_zonenum_vars);
    } DEFINE;

    STRUCT(DBmultimeshadj) {
        MEMBER_S(int,           nblocks);
        MEMBER_S(int,           blockorigin);
        MEMBER_S(int,           lneighbors);
        MEMBER_S(int,           totlnodelists);
        MEMBER_S(int,           totlzonelists);
        MEMBER_S(str256,        meshtypes);
        MEMBER_S(str256,        nneighbors);
        MEMBER_S(str256,        neighbors);
        MEMBER_S(str256,        back);
        MEMBER_S(str256,        lnodelists);
        MEMBER_S(str256,        nodelists);
        MEMBER_S(str256,        lzonelists);
        MEMBER_S(str256,        zonelists);
    } DEFINE;

    STRUCT(DBmultivar) {
        MEMBER_S(int,           nvars);
        MEMBER_S(int,           cycle);
        MEMBER_S(int,           ngroups);
        MEMBER_S(int,           blockorigin);
        MEMBER_S(int,           grouporigin);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           extentssize);
        MEMBER_S(int,           guihide);
        MEMBER_S(str256,        vartypes);
        MEMBER_S(str256,        varnames);
        MEMBER_S(str256,        extents);
        MEMBER_S(str256,        region_pnames);
        MEMBER_S(str256,        mmesh_name);
        MEMBER_S(int,           tensor_rank);
        MEMBER_S(int,           conserved);
        MEMBER_S(int,           extensive);
        MEMBER_S(str256,        file_ns_name);
        MEMBER_S(str256,        block_ns_name);
        MEMBER_S(int,           block_type);
        MEMBER_S(str256,        empty_list);
        MEMBER_S(int,           empty_cnt);
        MEMBER_S(int,           repr_block_idx);
        MEMBER_S(double,        missing_value);
    } DEFINE;

    STRUCT(DBmultimat) {
        MEMBER_S(int,           nmats);
        MEMBER_S(int,           cycle);
        MEMBER_S(int,           ngroups);
        MEMBER_S(int,           blockorigin);
        MEMBER_S(int,           grouporigin);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           allowmat0);
        MEMBER_S(int,           guihide);
        MEMBER_S(str256,        matnames);
        MEMBER_S(str256,        matnos);
        MEMBER_S(str256,        mixlens);
        MEMBER_S(str256,        matcounts);
        MEMBER_S(str256,        matlists);
        MEMBER_S(int,           nmatnos);
        MEMBER_S(str256,        material_names);
        MEMBER_S(str256,        mat_colors);
        MEMBER_S(str256,        mmesh_name);
        MEMBER_S(str256,        file_ns_name);
        MEMBER_S(str256,        block_ns_name);
        MEMBER_S(str256,        empty_list);
        MEMBER_S(int,           empty_cnt);
        MEMBER_S(int,           repr_block_idx);
    } DEFINE;

    STRUCT(DBmultimatspecies) {
        MEMBER_S(int,           nspec);
        MEMBER_S(int,           nmat);
        MEMBER_S(int,           cycle);
        MEMBER_S(int,           ngroups);
        MEMBER_S(int,           blockorigin);
        MEMBER_S(int,           grouporigin);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           guihide);
        MEMBER_S(str256,        specnames);
        MEMBER_S(str256,        nmatspec);
        MEMBER_S(str256,        matname);
        MEMBER_S(str256,        species_names);
        MEMBER_S(str256,        speccolors);
        MEMBER_S(str256,        file_ns_name);
        MEMBER_S(str256,        block_ns_name);
        MEMBER_S(str256,        empty_list);
        MEMBER_S(int,           empty_cnt);
        MEMBER_S(int,           repr_block_idx);
    } DEFINE;

    STRUCT(DBmatspecies) {
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           nmat);
        MEMBER_S(int,           nspecies_mf);
        MEMBER_S(int,           mixlen);
        MEMBER_S(int,           major_order);
        MEMBER_S(int,           datatype);
        MEMBER_3(int,           dims);
        MEMBER_S(int,           guihide);
        MEMBER_S(str256,        matname);
        MEMBER_S(str256,        speclist);
        MEMBER_S(str256,        nmatspec);
        MEMBER_S(str256,        species_mf);
        MEMBER_S(str256,        mix_speclist);
        MEMBER_S(str256,        specnames);
        MEMBER_S(str256,        speccolors);
    } DEFINE;

    STRUCT(DBpointmesh) {
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           nspace);
        MEMBER_S(int,           nels);
        MEMBER_S(int,           cycle);
        MEMBER_S(int,           group_no);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           min_index);
        MEMBER_S(int,           max_index);
        MEMBER_3(double,        min_extents);
        MEMBER_3(double,        max_extents);
        MEMBER_S(int,           guihide);
        MEMBER_R(str256,        coord,          3);
        MEMBER_R(str256,        label,          3);
        MEMBER_R(str256,        units,          3);
        MEMBER_S(str256,        gnodeno);
        MEMBER_S(str256,        mrgtree_name);
        MEMBER_S(int,           gnznodtype);
        MEMBER_S(str256,        ghost_node_labels);
        MEMBER_S(str256,        alt_nodenum_vars);
    } DEFINE;

    STRUCT(DBpointvar) {
        MEMBER_S(int,           nvals);
        MEMBER_S(int,           nels);
        MEMBER_S(int,           ndims);
        MEMBER_S(int,           nspace);
        MEMBER_S(int,           origin);
        MEMBER_S(int,           min_index);
        MEMBER_S(int,           max_index);
        MEMBER_S(int,           datatype);
        MEMBER_S(int,           cycle);
        MEMBER_S(float,         time);
        MEMBER_S(double,        dtime);
        MEMBER_S(int,           guihide);
        MEMBER_S(int,           ascii_labels);
        MEMBER_S(str256,        meshid);
        MEMBER_S(str256,        label);
        MEMBER_S(str256,        units);
        MEMBER_R(str256,        data,           MAX_VARS);
        MEMBER_S(str256,        region_pnames);
        MEMBER_S(int,           conserved);
        MEMBER_S(int,           extensive);
        MEMBER_S(double,        missing_value);
    } DEFINE;

    STRUCT(DBcompoundarray) {
        MEMBER_S(int,           nelems);
        MEMBER_S(int,           nvalues);
        MEMBER_S(int,           datatype);
        MEMBER_S(str256,        values);
        MEMBER_S(str256,        elemnames);
        MEMBER_S(str256,        elemlengths);
    } DEFINE;

    STRUCT(DBmrgtree) {
        MEMBER_S(int,           src_mesh_type);
        MEMBER_S(int,           type_info_bits);
        MEMBER_S(int,           num_nodes);
        MEMBER_S(int,           root);
        MEMBER_S(str256,        src_mesh_name);

        MEMBER_S(str256,        n_scalars);
        MEMBER_S(str256,        n_name);
        MEMBER_S(str256,        n_names);
        MEMBER_S(str256,        n_maps_name);
        MEMBER_S(str256,        n_seg_ids);
        MEMBER_S(str256,        n_seg_lens);
        MEMBER_S(str256,        n_seg_types);
        MEMBER_S(str256,        n_children);

        MEMBER_S(str256,        mrgvar_onames);
        MEMBER_S(str256,        mrgvar_rnames);
    } DEFINE;

    STRUCT(DBgroupelmap) {
        MEMBER_S(int,           num_segments);
        MEMBER_S(int,           fracs_data_type);
        MEMBER_S(str256,        groupel_types);
        MEMBER_S(str256,        segment_lengths);
        MEMBER_S(str256,        segment_ids);
        MEMBER_S(str256,        segment_data);
        MEMBER_S(str256,        frac_lengths);
        MEMBER_S(str256,        segment_fracs);
    } DEFINE;

    STRUCT(DBmrgvar) {
        MEMBER_S(int,           nregns);
        MEMBER_S(int,           ncomps);
        MEMBER_S(int,           datatype);
        MEMBER_S(str256,        mrgt_name);
        MEMBER_S(str256,        reg_pnames);
        MEMBER_S(str256,        compnames);
        MEMBER_R(str256,        data,          MAX_VARS);
    } DEFINE;

}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_InitCallbacks
 *
 * Purpose:     Initialize the callbacks in a DBfile structure.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *
 *    Jeremy Meredith, Wed Oct 25 16:16:59 PDT 2000
 *    Added DB_INTEL so we had a little-endian target.
 *
 *    Mark C. Miller, Tue Feb  3 09:47:43 PST 2009
 *    Changed dbfile arg from public DBfile* to private DBfile_hdf5*
 *
 *    Mark C. Miller, Mon Sep 21 15:17:08 PDT 2009
 *    Adding support for long long type.
 *-------------------------------------------------------------------------
 */
PRIVATE void
db_hdf5_InitCallbacks(DBfile_hdf5 *dbfile, int target)
{
    static char         *me = "db_hdf5_InitCallbacks";
    
    /* Initialize the driver global data structures */
    db_hdf5_init();

    /* Target data types */
    switch (target) {
    case DB_LOCAL:
        dbfile->T_char   = T_char;
        dbfile->T_short  = T_short;
        dbfile->T_int    = T_int;
        dbfile->T_long   = T_long;
        dbfile->T_llong  = T_llong;
        dbfile->T_float  = T_float;
        dbfile->T_double = T_double;
        dbfile->T_str    = T_str;
        break;

    case DB_SGI:
    case DB_SUN3:
    case DB_SUN4:
    case DB_RS6000:
        dbfile->T_char   = H5T_STD_I8BE;
        dbfile->T_short  = H5T_STD_I16BE;
        dbfile->T_int    = H5T_STD_I32BE;
        dbfile->T_long   = H5T_STD_I32BE;
        dbfile->T_llong  = H5T_STD_I64BE;
        dbfile->T_float  = H5T_IEEE_F32BE;
        dbfile->T_double = H5T_IEEE_F64BE;
        dbfile->T_str    = T_str;
        break;

    case DB_CRAY:
        dbfile->T_char   = H5T_STD_I8BE;
        dbfile->T_short  = H5T_STD_I64BE;
        dbfile->T_int    = H5T_STD_I64BE;
        dbfile->T_long   = H5T_STD_I64BE;
        dbfile->T_llong  = H5T_STD_I64BE;
        dbfile->T_float  = H5T_IEEE_F64BE; /*assuming new cray*/
        dbfile->T_double = H5T_IEEE_F64BE; /*assuming new cray*/
        dbfile->T_str    = T_str;
        break;

    case DB_INTEL:
        dbfile->T_char   = H5T_STD_I8LE;
        dbfile->T_short  = H5T_STD_I16LE;
        dbfile->T_int    = H5T_STD_I32LE;
        dbfile->T_long   = H5T_STD_I32LE;
        dbfile->T_llong  = H5T_STD_I64LE;
        dbfile->T_float  = H5T_IEEE_F32LE;
        dbfile->T_double = H5T_IEEE_F64LE;
        dbfile->T_str    = T_str;
        break;

    default:
        db_perror("target data type", E_BADARGS, me);
        return;
    }

    /* Properties of the driver */
    dbfile->pub.pathok = TRUE;
    dbfile->pub.Grab = FALSE;

    /* File operations */
    dbfile->pub.close = db_hdf5_Close;
    dbfile->pub.module = db_hdf5_Filters;
    dbfile->pub.flush = db_hdf5_Flush;

    /* Directory operations */
    dbfile->pub.cd = db_hdf5_SetDir;
    dbfile->pub.g_dir = db_hdf5_GetDir;
    dbfile->pub.newtoc = db_hdf5_NewToc;
    dbfile->pub.mkdir = db_hdf5_MkDir;
    dbfile->pub.cpdir = db_hdf5_CpDir;
#ifndef _MSC_VER
#warning GET RID OF CPLISTEDOBJECTS
#endif
    dbfile->pub.cpnobjs = db_hdf5_CpListedObjects;
    dbfile->pub.mksymlink = db_hdf5_MkSymlink;
    dbfile->pub.g_symlink = db_hdf5_GetSymlink;

    /* Variable inquiries */
    dbfile->pub.exist = db_hdf5_InqVarExists;
    dbfile->pub.g_varlen = db_hdf5_GetVarLength;
    dbfile->pub.g_varbl = db_hdf5_GetVarByteLength;
    dbfile->pub.g_varblf = db_hdf5_GetVarByteLengthInFile;
    dbfile->pub.g_vartype = db_hdf5_GetVarType;
    dbfile->pub.g_vardims = db_hdf5_GetVarDims;

    /* Variable I/O operations */
    dbfile->pub.g_var = db_hdf5_GetVar;
    dbfile->pub.r_var = db_hdf5_ReadVar;
    dbfile->pub.r_varslice = db_hdf5_ReadVarSlice;
    dbfile->pub.r_varvals = db_hdf5_ReadVarVals;
    dbfile->pub.write = db_hdf5_Write;
    dbfile->pub.writeslice = db_hdf5_WriteSlice;

    /* Low-level object functions */
    dbfile->pub.g_obj = db_hdf5_GetObject;
    dbfile->pub.inqvartype = db_hdf5_InqVarType;
    dbfile->pub.i_meshtype = (int (*)(struct DBfile *, char const *)) db_hdf5_InqVarType; /*yes, Vartype*/
    dbfile->pub.i_meshname = db_hdf5_InqMeshName;
    dbfile->pub.g_comp = db_hdf5_GetComponent;
    dbfile->pub.g_comptyp = db_hdf5_GetComponentType;
    dbfile->pub.g_compnames = db_hdf5_GetComponentNames;
    dbfile->pub.c_obj = db_hdf5_WriteObject; /*DBChangeObject==DBWriteObject*/
    dbfile->pub.w_obj = db_hdf5_WriteObject;
    dbfile->pub.w_comp = db_hdf5_WriteComponent;
    
    /* Curve functions */
    dbfile->pub.g_cu = db_hdf5_GetCurve;
    dbfile->pub.p_cu = db_hdf5_PutCurve;

    /* CSG mesh functions */
    dbfile->pub.p_csgm = db_hdf5_PutCsgmesh;
    dbfile->pub.g_csgm = db_hdf5_GetCsgmesh;
    dbfile->pub.p_csgzl = db_hdf5_PutCSGZonelist;
    dbfile->pub.g_csgzl = db_hdf5_GetCSGZonelist;
    dbfile->pub.p_csgv = db_hdf5_PutCsgvar;
    dbfile->pub.g_csgv = db_hdf5_GetCsgvar;

    /* Defvars functions */
    dbfile->pub.g_defv = db_hdf5_GetDefvars;
    dbfile->pub.p_defv = db_hdf5_PutDefvars;

    /* Quadmesh functions */
    dbfile->pub.g_qm = db_hdf5_GetQuadmesh;
    dbfile->pub.g_qv = db_hdf5_GetQuadvar;
    dbfile->pub.p_qm = db_hdf5_PutQuadmesh;
    dbfile->pub.p_qv = db_hdf5_PutQuadvar;

    /* Unstructured mesh functions */
    dbfile->pub.g_um = db_hdf5_GetUcdmesh;
    dbfile->pub.g_uv = db_hdf5_GetUcdvar;
    dbfile->pub.g_fl = db_hdf5_GetFacelist;
    dbfile->pub.g_zl = db_hdf5_GetZonelist;
    dbfile->pub.g_phzl = db_hdf5_GetPHZonelist;
    dbfile->pub.p_um = db_hdf5_PutUcdmesh;
    dbfile->pub.p_sm = db_hdf5_PutUcdsubmesh;
    dbfile->pub.p_uv = db_hdf5_PutUcdvar;
    dbfile->pub.p_fl = db_hdf5_PutFacelist;
    dbfile->pub.p_zl = db_hdf5_PutZonelist;
    dbfile->pub.p_zl2 = db_hdf5_PutZonelist2;
    dbfile->pub.p_phzl = db_hdf5_PutPHZonelist;

    /* Material functions */
    dbfile->pub.g_ma = db_hdf5_GetMaterial;
    dbfile->pub.g_ms = db_hdf5_GetMatspecies;
    dbfile->pub.p_ma = db_hdf5_PutMaterial;
    dbfile->pub.p_ms = db_hdf5_PutMatspecies;

    /* Pointmesh functions */
    dbfile->pub.g_pm = db_hdf5_GetPointmesh;
    dbfile->pub.g_pv = db_hdf5_GetPointvar;
    dbfile->pub.p_pm = db_hdf5_PutPointmesh;
    dbfile->pub.p_pv = db_hdf5_PutPointvar;

    /* Multiblock functions */
    dbfile->pub.g_mm = db_hdf5_GetMultimesh;
    dbfile->pub.g_mmadj = db_hdf5_GetMultimeshadj;
    dbfile->pub.g_mv = db_hdf5_GetMultivar;
    dbfile->pub.g_mt = db_hdf5_GetMultimat;
    dbfile->pub.g_mms = db_hdf5_GetMultimatspecies;
    dbfile->pub.p_mm = db_hdf5_PutMultimesh;
    dbfile->pub.p_mmadj = db_hdf5_PutMultimeshadj;
    dbfile->pub.p_mv = db_hdf5_PutMultivar;
    dbfile->pub.p_mt = db_hdf5_PutMultimat;
    dbfile->pub.p_mms = db_hdf5_PutMultimatspecies;

    /* Compound arrays */
    dbfile->pub.g_ca = db_hdf5_GetCompoundarray;
    dbfile->pub.p_ca = db_hdf5_PutCompoundarray;

    /* Mrgtree functions */
    dbfile->pub.g_mrgt = db_hdf5_GetMrgtree;
    dbfile->pub.p_mrgt = db_hdf5_PutMrgtree;

    /* Groupel map functions */
    dbfile->pub.g_grplm = db_hdf5_GetGroupelmap;
    dbfile->pub.p_grplm = db_hdf5_PutGroupelmap;

    /* mrgvar functions */
    dbfile->pub.g_mrgv = db_hdf5_GetMrgvar;
    dbfile->pub.p_mrgv = db_hdf5_PutMrgvar;

    /* Compression support functions */
    dbfile->pub.free_z = db_hdf5_FreeCompressionResources;

    dbfile->pub.sort_obo = db_hdf5_SortObjectsByOffset;
}

/*-------------------------------------------------------------------------
 * Function:    build_fspace
 *
 * Purpose:     Build a file data space selection based on the silo OFFSET,
 *              LENGTH, and STRIDE. The COUNT for hdf5 hyperslabs is the
 *              number of times to stride whereas the LENGTH argument
 *              passed in is the number of elements over which we stride.
 *              An example:
 *              
 *                0 1 2 3 4 5 6 7 8 9 10
 *               +-+-+-+-+-+-+-+-+-+-+-+  Silo: offset=2, stride=2, length=7
 *               | | |X| |X| |X| |X| | |  HDF5: offset=2, stride=2, count=4
 *               +-+-+-+-+-+-+-+-+-+-+-+
 *
 * Return:      Success:        Data space to be closed later. If SIZE is
 *                              non-null then the number of elements selected
 *                              in each dimension is written to that array.
 *
 *              Failure:        Negative, SIZE is undefined.
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *    Thomas R. Treadway, Wed Mar  7 09:59:27 PST 2007
 *    Changed hs_offset from hssize_t to hsize_t
 *
 *
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t
build_fspace(hid_t dset, int ndims, int const *offset, int const *length, int const *stride,
             hsize_t *size/*out*/)
{
    hid_t       fspace = -1;
    int         i;
    hsize_t    hs_offset[H5S_MAX_RANK];
    hsize_t     hs_count[H5S_MAX_RANK], hs_stride[H5S_MAX_RANK];
    
    if (ndims>H5S_MAX_RANK) return -1;
    for (i=0; i<ndims; i++) {
        hs_offset[i] = offset[i];
        hs_stride[i] = stride[i];
        
        if (stride[i]) {
            hs_count[i] = (length[i]+stride[i]-1)/stride[i];
        } else {
            hs_count[i] = 1;
        }
        if (size) size[i] = hs_count[i];
    }

    if ((fspace=H5Dget_space(dset))<0) return -1;
    if (H5Sselect_hyperslab(fspace, H5S_SELECT_SET, hs_offset, hs_stride,
                            hs_count, NULL)<0) {
        H5Sclose(fspace);
        return -1;
    }
    return fspace;
}

/*-------------------------------------------------------------------------
 * Function:    build_fspace_vals
 *
 * Purpose:     Build a file data space selection based on the a list of
 *              array indices (either logical tuples or linear indices).
 *
 * Return:      Success:        Data space to be closed later.
 *
 * Programmer: Mark C. Miller, Mon Apr 18 13:20:20 PDT 2016
 *
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t
build_fspace_vals(hid_t dset, int nvals, int ndims, int const *indices)
{
    int         i,j;
    hsize_t     *coords = 0;
    hid_t       fspace = -1;
    
    coords = (hsize_t *) malloc(ndims * nvals * sizeof(hsize_t));
    if (!coords) return -1;

    if ((fspace=H5Dget_space(dset))<0) return -1;

    for (i = 0; i < nvals; i++)
    {
        for (j = 0; j < ndims; j++)
        {
            coords[i*ndims+j] = (hsize_t) indices[i*ndims+j];
        }
    }

    if (H5Sselect_elements(fspace, H5S_SELECT_SET, (size_t) nvals, coords)<0) {
        H5Sclose(fspace);
        return -1;
    }

    free(coords);

    return fspace;
}

/*-------------------------------------------------------------------------
 * Function:    silom2hdfm_type
 *
 * Purpose:     Return the hdf5 data type for memory that is equivalent to
 *              the specified silo memory data type.
 *
 * Return:      Success:        An hdf5 data type. Do not close it (well, if
 *                              you must, but you'll get an error from hdf5).
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Sep 21 15:17:08 PDT 2009
 *   Adding support for long long type.
 *
 *   Mark C. Miller, Mon Dec  7 09:50:19 PST 2009
 *   Conditionally compile long long support only when its
 *   different from long.
 *
 *   Mark C. Miller, Mon Jan 11 16:02:16 PST 2010
 *   Made long long support UNconditionally compiled.
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t
silom2hdfm_type(int datatype)
{
    hid_t       mtype = -1;

    switch (datatype) {
    case DB_INT:
        mtype = H5T_NATIVE_INT;
        break;
    case DB_SHORT:
        mtype = H5T_NATIVE_SHORT;
        break;
    case DB_LONG:
        mtype = H5T_NATIVE_LONG;
        break;
    case DB_LONG_LONG:
        mtype = H5T_NATIVE_LLONG;
        break;
    case DB_FLOAT:
        mtype = H5T_NATIVE_FLOAT;
        break;
    case DB_DOUBLE:
        mtype = H5T_NATIVE_DOUBLE;
        break;
    case DB_CHAR:
        mtype = H5T_NATIVE_UCHAR;
        break;
    }
    return mtype;
}

/*-------------------------------------------------------------------------
 * Function:    silof2hdff_type
 *
 * Purpose:     Return the hdf5 data type for the file that is equivalent to
 *              the specified silo file data type.
 *
 * Return:      Success:        An hdf5 data type. Do not close this type.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, April 15, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Sep 21 15:17:08 PDT 2009
 *   Adding support for long long type.
 *
 *   Mark C. Miller, Mon Dec  7 09:50:19 PST 2009
 *   Conditionally compile long long support only when its
 *   different from long.
 *
 *   Mark C. Miller, Mon Jan 11 16:02:16 PST 2010
 *   Made long long support UNconditionally compiled.
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t
silof2hdff_type(DBfile_hdf5 *dbfile, int datatype)
{
    hid_t       ftype = -1;

    switch (datatype) {
    case DB_INT:
        ftype = dbfile->T_int;
        break;
    case DB_SHORT:
        ftype = dbfile->T_short;
        break;
    case DB_LONG:
        ftype = dbfile->T_long;
        break;
    case DB_LONG_LONG:
        ftype = dbfile->T_llong;
        break;
    case DB_FLOAT:
        ftype = dbfile->T_float;
        break;
    case DB_DOUBLE:
        ftype = dbfile->T_double;
        break;
    case DB_CHAR:
        ftype = dbfile->T_char;
        break;
    }
    return ftype;
}

/*-------------------------------------------------------------------------
 * Function:    hdf2silo_type
 *
 * Purpose:     Given an HDF5 file data type return and appropriate memory
 *              silo data type.
 *
 * Return:      Success:        Silo data type
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Monday, April 12, 1999
 *
 * Modifications:
 *
 *   Mark C. Miler, made it return DB_FLOAT or DB_DOUBLE based on
 *   data type passed in and NOT on current forceSingle mode
 *
 *   Mark C. Miller, Mon Sep 21 15:17:08 PDT 2009
 *   Adding support for long long type.
 *-------------------------------------------------------------------------
 */
PRIVATE int
hdf2silo_type(hid_t type)
{
    size_t      size = H5Tget_size(type);
    int         retval = -1;
    
    switch (H5Tget_class(type)) {
    case H5T_INTEGER:
        if (sizeof(char)>=size) {
            retval = DB_CHAR;
        } else if (sizeof(int)!=sizeof(short) && sizeof(short)>=size) {
            retval = DB_SHORT;
        } else if (sizeof(int)>=size) {
            retval = DB_INT;
        } else if (sizeof(long)>=size) {
            retval = DB_LONG;
        } else {
            retval = DB_LONG_LONG;
        }
        break;

    case H5T_FLOAT:
        if (sizeof(double)!=sizeof(float) && sizeof(float)>=size) {
            retval = DB_FLOAT;
        } else if (sizeof(double)>=size) {
            retval = DB_DOUBLE;
        }
        break;

    default:
        /* Silo doesn't handle other types */
        break;
    }
    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    silo2silo_type
 *
 * Purpose:     Translate a file silo data type to a memory silo data type.
 *              If the file data type is zero then we use either DB_FLOAT or
 *              DB_DOUBLE.
 *
 * Return:      Success:        silo data type
 *
 *              Failure:        never fails
 *
 * Programmer:  Robb Matzke
 *              Monday, April 12, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PRIVATE int
silo2silo_type(int datatype)
{
    switch (datatype) {
    case 0: return force_single_g ? DB_FLOAT : DB_DOUBLE;
    case DB_CHAR: return DB_CHAR;
    case DB_SHORT: return DB_SHORT;
    case DB_INT: return DB_INT;
    case DB_LONG: return DB_LONG;
    case DB_LONG_LONG: return DB_LONG_LONG;
    case DB_FLOAT: return DB_FLOAT;
    case DB_DOUBLE: return force_single_g ? DB_FLOAT : DB_DOUBLE;
    }
    return DB_FLOAT;
}

/*-------------------------------------------------------------------------
 * Function:    hdf2hdf_type
 *
 * Purpose:     Given a file data type choose an appropriate memory data
 *              type.
 *
 * Return:      Success:        Memory data type which should not be closed
 *                              later (well, if you must, but hdf5 will
 *                              report an error).
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *   Made it return NATIVE_FLOAT or NATIVE_DOUBLE NOT depending on
 *   current force_single_g mode
 *
 *   Mark C. Miller, Mon Sep 21 15:17:08 PDT 2009
 *   Adding support for long long type.
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t
hdf2hdf_type(hid_t ftype)
{
    hid_t       mtype=-1;
    
    switch (H5Tget_class(ftype)) {
    case H5T_INTEGER:
        if (sizeof(char)>=H5Tget_size(ftype)) {
            mtype = H5T_NATIVE_UCHAR;
        } else if (sizeof(short)>=H5Tget_size(ftype)) {
            mtype = H5T_NATIVE_SHORT;
        } else if (sizeof(int)>=H5Tget_size(ftype)) {
            mtype = H5T_NATIVE_INT;
        } else if (sizeof(long)>=H5Tget_size(ftype)) {
            mtype = H5T_NATIVE_LONG;
        } else {
            mtype = H5T_NATIVE_LLONG;
        }
        break;
    case H5T_FLOAT:
        if (sizeof(double)!=sizeof(float) && sizeof(float)>=H5Tget_size(ftype)) {
            mtype = H5T_NATIVE_FLOAT;
        } else if (sizeof(double)>=H5Tget_size(ftype)) {
            mtype = H5T_NATIVE_DOUBLE;
        } else {
            mtype = H5T_NATIVE_FLOAT;
        }
        break;
    default:
        break;
    }
    return mtype;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_set_compression
 *
 * Purpose:     Set the HDF5 compression properties.
 *
 * Bugs:        
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Thomas R. Treadway, Tue Feb 27 15:27:11 PST 2007
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Jul 17 15:00:46 PDT 2008
 *   Added globals for minimum compression ratio and error mode.
 *   Added support for HZIP and FPZIP. Added flags to control whether
 *   HZIP compression filter gets added or not.
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_set_compression(DBfile *dbfile, int flags)
{
    static char *me = "db_hdf5_set_compression";
    char *ptr;
    char chararray[32];
    char *check;
    int level, block, nfilters;
    int nbits, prec;
    int have_gzip, have_szip, have_fpzip, have_hzip, have_zfp, i;
    H5Z_filter_t filtn;
    unsigned int filter_config_flags, opt_flag;

    /* Check what filters already exist */
    have_gzip = FALSE;
    have_szip = FALSE;
    have_fpzip = FALSE;
    have_hzip = FALSE;
    have_zfp = FALSE;
    if ((nfilters = H5Pget_nfilters(P_ckcrprops))<0)
    {
       db_perror("H5Pget_nfilters", E_CALLFAIL, me);
       return (-1);
    }
    for (i=0; i<nfilters; i++) {           
#if defined H5_USE_16_API || !HDF5_VERSION_GE(1,8,0)
            filtn = H5Pget_filter(P_ckcrprops,(unsigned)i,0,0,0,0,0);
#else
            filtn = H5Pget_filter(P_ckcrprops,(unsigned)i,0,0,0,0,0,NULL);
#endif
        if (H5Z_FILTER_DEFLATE==filtn)     
            have_gzip = TRUE;
#ifdef H5_HAVE_FILTER_SZIP
        if (H5Z_FILTER_SZIP==filtn)     
            have_szip = TRUE;
#endif
        if (DB_HDF5_FPZIP_ID==filtn)
            have_fpzip = TRUE;
        if (DB_HDF5_HZIP_ID==filtn)
            have_hzip = TRUE;
#ifdef HAVE_ZFP
        if (H5Z_FILTER_ZFP==filtn)
            have_zfp = TRUE;
#endif
    }
#ifndef _MSC_VER
#warning WHAT ABOUT NULL RETURN FROM DBGETCOMPRESSION
#endif
/* Handle some global compression parameters */
    if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
       "ERRMODE=")) != (char *)NULL) 
    {
        (void)strncpy(chararray, ptr+8, 4); 
        chararray[4] = '\0';
        if (strcmp(chararray, "FALL") == 0)
            SILO_Globals.compressionErrmode = COMPRESSION_ERRMODE_FALLBACK;
        else if (strcmp(chararray, "FAIL") == 0)
            SILO_Globals.compressionErrmode = COMPRESSION_ERRMODE_FAIL;
        else
        {
            db_perror(DBGetCompressionFile(dbfile), E_COMPRESSION, me);
            return (-1);
        }
    }
    if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
       "MINRATIO=")) != (char *)NULL) 
    {
        float mcr;
        (void)strncpy(chararray, ptr+9, 5); 
        mcr = (float) strtod(chararray, &check);
        if (mcr > 1.0)
            SILO_Globals.compressionMinratio = mcr;
        else
        {
            db_perror(DBGetCompressionFile(dbfile), E_COMPRESSION, me);
            return (-1);
        }
    }
#ifndef _MSC_VER
#warning FIX MISSING .compressionMinsize member
#endif
#if 0
    if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
       "MINSIZE=")) != (char *)NULL) 
    {
        unsigned int minsize;
        strncpy(chararray, ptr+8, 8); 
        minsize = strtol(chararray, &check, 10);
        if (minsize > 0)
            SILO_Globals.compressionMinsize = minsize;
        else
        {
            db_perror(DBGetCompressionFile(dbfile), E_COMPRESSION, me);
            return -1;
        }
    }
#endif

#ifndef _MSC_VER
#warning QUERY FILE LEVEL COMPRESSION PARAMS HERE
#endif
    opt_flag = SILO_Globals.compressionErrmode == COMPRESSION_ERRMODE_FALLBACK ?
                   H5Z_FLAG_OPTIONAL : H5Z_FLAG_MANDATORY;

    /* Select the compression algorthm */
    if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
       "METHOD=GZIP")) != (char *)NULL) 
    {
       if (have_gzip == FALSE)
       {
          if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
             "LEVEL=")) != (char *)NULL)
          {
             (void)strncpy(chararray, ptr+6, 1); 
             level = (int) strtol(chararray, &check, 10);
             if ((chararray != check) && (level >= 0) && (level <=9))
             {
                if (H5Pset_shuffle(P_ckcrprops)<0 ||
                    H5Pset_deflate(P_ckcrprops, level)<0)
                {
                   db_perror("H5Pset_deflate", E_CALLFAIL, me);
                   return (-1);
                }
             }
             else
             {
                db_perror(DBGetCompressionFile(dbfile), E_COMPRESSION, me);
                return (-1);
             }
          }
          else
          {
             if (H5Pset_shuffle(P_ckcrprops)<0 ||
                 H5Pset_deflate(P_ckcrprops, 1)<0)
             {
                db_perror("H5Pset_deflate", E_CALLFAIL, me);
                return (-1);
             }
          }
       }  /* if (have_gzip == FALSE) */
    }
#ifdef H5_HAVE_FILTER_SZIP
    else if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile),
       "METHOD=SZIP"))!=(char *)NULL)
    {
       if (have_szip == FALSE)
       {
          filtn = H5Z_FILTER_SZIP;
          if (H5Zget_filter_info(filtn, &filter_config_flags)<0)
          {
             db_perror(DBGetCompressionFile(dbfile), E_COMPRESSION, me);
             return (-1);
          }
          if ((filter_config_flags &
          (H5Z_FILTER_CONFIG_ENCODE_ENABLED|H5Z_FILTER_CONFIG_DECODE_ENABLED))==
          (H5Z_FILTER_CONFIG_ENCODE_ENABLED|H5Z_FILTER_CONFIG_DECODE_ENABLED))
          {
             if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
                "BLOCK=")) != (char *)NULL)
             {
                (void)strncpy(chararray, ptr+6, 2); 
                block = (int) strtol(chararray, &check, 10);
                if ((chararray != check) && (block >= 0) && (block <=32))
                { 
                   if (strstr(DBGetCompressionFile(dbfile), 
                      "MASK=EC") != NULL)
                   {
                      if (H5Pset_shuffle(P_ckcrprops)<0 ||
                          H5Pset_szip(P_ckcrprops, H5_SZIP_EC_OPTION_MASK,block)<0)
                      {
                         db_perror("H5Pset_szip", E_CALLFAIL, me);
                         return (-1);
                      }
                   }
                   else if(strstr(DBGetCompressionFile(dbfile),
                      "MASK=NN")!=NULL)
                   {
                      if (H5Pset_shuffle(P_ckcrprops)<0 ||
                          H5Pset_szip(P_ckcrprops, H5_SZIP_NN_OPTION_MASK,block)<0)
                      {
                         db_perror("H5Pset_szip", E_CALLFAIL, me);
                         return (-1);
                      }
                   }
                   else
                   {
                      if (H5Pset_shuffle(P_ckcrprops)<0 ||
                          H5Pset_szip(P_ckcrprops, H5_SZIP_NN_OPTION_MASK, block)<0)
                      {
                         db_perror("H5Pset_szip", E_CALLFAIL, me);
                         return (-1);
                      }
                   }
                }
                else
                {
                   db_perror(DBGetCompressionFile(dbfile), E_COMPRESSION, me);
                   return (-1);
                }
             }
             else
             {
                if (H5Pset_shuffle(P_ckcrprops)<0 ||
                    H5Pset_szip(P_ckcrprops, H5_SZIP_NN_OPTION_MASK, 4)<0)
                {
                   db_perror("H5Pset_szip", E_CALLFAIL, me);
                   return (-1);
                }
             }
          }  /* if ((filter_config_flags & */
       }  /* if (have_szip == FALSE) */
    }
#endif
#ifdef HAVE_HZIP
    else if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
       "METHOD=HZIP")) != (char *)NULL) 
    {
       if (have_hzip == FALSE && (flags & ALLOW_MESH_COMPRESSION))
       {
           if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
              "CODEC=")) != (char *)NULL)
           {
              (void)strncpy(chararray, ptr+6, 4); 
              chararray[4] = '\0';
#ifdef HAVE_LIBZ
              if (strcmp(chararray, "zlib") == 0)
              {
                  db_hdf5_hzip_params.codec = HZM_CODEC_ZLIB;
                  db_hdf5_hzip_params.params = &hzm_codec_zlib;
              }
              else
#endif
              if (strcmp(chararray, "base") == 0)
              {
                  db_hdf5_hzip_params.codec = HZM_CODEC_BASE;
                  db_hdf5_hzip_params.params = &hzm_codec_base;
              }
              else
              {
                  db_perror("hzip codec not recongized", E_COMPRESSION, me);
                  return (-1);
              }
           }
           if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
              "BITS=")) != (char *)NULL)
           {
              (void)strncpy(chararray, ptr+5, 2); 
              nbits = (int) strtol(chararray, &check, 10);
              if ((chararray != check) && (nbits >= 0) && (nbits <=64))
              {
#ifdef HAVE_LIBZ
                  if (db_hdf5_hzip_params.codec == HZM_CODEC_ZLIB)
                      ((struct HZMCODECzlib *) db_hdf5_hzip_params.params)->bits = nbits;
                  else
#endif
                  if (db_hdf5_hzip_params.codec == HZM_CODEC_BASE)
                      ((struct HZMCODECbase *) db_hdf5_hzip_params.params)->bits = nbits;
              }
              else
              {
                 db_perror("invalid nbits for hzip", E_COMPRESSION, me);
                 return (-1);
              }
           }

           if (H5Pset_filter(P_ckcrprops, DB_HDF5_HZIP_ID, opt_flag, 0, 0)<0)
           {
               db_perror("hzip filter setup", E_CALLFAIL, me);
               return (-1);
           }
       }
    }
#endif
#ifdef HAVE_FPZIP
    else if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
       "METHOD=FPZIP")) != (char *)NULL) 
    {
       if (have_fpzip == FALSE)
       {
          if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
             "LOSS=")) != (char *)NULL)
          {
             (void)strncpy(chararray, ptr+5, 2); 
             prec = (int) strtol(chararray, &check, 10);
             if ((chararray != check) && (prec >= 0) && (prec <=3))
             {
                db_hdf5_fpzip_params.loss = prec;
             }
             else
             {
                db_perror(DBGetCompressionFile(dbfile), E_COMPRESSION, me);
                return (-1);
             }
          }

          if (H5Pset_filter(P_ckcrprops, DB_HDF5_FPZIP_ID, opt_flag, 0, 0)<0)
          {
              db_perror("H5Pset_filter", E_CALLFAIL, me);
              return (-1);
          }
       }
    }
#endif
#ifdef HAVE_ZFP
    else if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
       "METHOD=ZFP")) != (char *)NULL) 
    {
       if (have_zfp == FALSE)
       {
          double tmpdbl = -1;
          uint tmpuint = 0;
          unsigned int cd_values[H5Z_ZFP_CD_NELMTS_MEM];
          int cd_nelmts = H5Z_ZFP_CD_NELMTS_MEM;
          
          if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
             "RATE=")) != (char *)NULL)
          {
             strncpy(chararray, ptr+5, 8); 
             tmpdbl = strtod(chararray, &check);
             if (chararray != check && errno == 0 && tmpdbl > 0)
                 H5Pset_zfp_rate_cdata(tmpdbl, cd_nelmts, cd_values);
          }
          else if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
             "PRECISION=")) != (char *)NULL)
          {
             strncpy(chararray, ptr+10, 2); 
             tmpuint = (uint) strtoul(chararray, &check, 10);
             if (chararray != check && errno == 0 && tmpuint > 0)
                 H5Pset_zfp_precision_cdata(tmpuint, cd_nelmts, cd_values);
          }
          else if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
             "ACCURACY=")) != (char *)NULL)
          {
             strncpy(chararray, ptr+9, 8); 
             tmpdbl = strtod(chararray, &check);
             if (chararray != check && errno == 0 && tmpdbl > 0)
                 H5Pset_zfp_accuracy_cdata(tmpdbl, cd_nelmts, cd_values);
          }
          else if ((ptr=(char *)strstr(DBGetCompressionFile(dbfile), 
             "EXPERT=")) != (char *)NULL)
          {
             int nvals, minexp; unsigned int minbits, maxbits, maxprec;
             strncpy(chararray, ptr+7, 20); 
             nvals = sscanf(chararray, "%u,%u,%u,%d", &minbits, &maxbits, &maxprec, &minexp);
             if (nvals == 4 && errno == 0)
                 H5Pset_zfp_expert_cdata(minbits, maxbits, maxprec, minexp, cd_nelmts, cd_values);
          }
          else if ((ptr=(char *)strstr(SILO_Globals.compressionParams, 
             "REVERSIBLE")) != (char *)NULL)
          {
              H5Pset_zfp_reversible_cdata(cd_nelmts, cd_values);
          }
          else
          {
              db_perror(DBGetCompressionFile(dbfile), E_COMPRESSION, me);
              return -1;
          }

          if (H5Pset_filter(P_ckcrprops, H5Z_FILTER_ZFP, opt_flag, cd_nelmts, cd_values)<0)
          {
              db_perror("H5Pset_filter", E_CALLFAIL, me);
              return (-1);
          }
       }
    }
#endif
    else
    {
       db_perror(DBGetCompressionFile(dbfile), E_COMPRESSION, me);
       return (-1);
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_set_properties
 *
 * Purpose:     Set the HDF5 properties.
 *
 * Bugs:        
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Thomas R. Treadway, Tue Mar  6 14:23:50 PST 2007
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_set_properties(DBfile *dbfile, int rank, hsize_t size[])
{
    static char *me = "db_hdf5_set_properties";
    P_crprops = H5P_DEFAULT;
    if (DBGetEnableChecksumsFile(dbfile) && 
        !DBGetCompressionFile(dbfile))
    {
        H5Pset_chunk(P_ckcrprops, rank, size);
        P_crprops = P_ckcrprops;
    }
    else if (DBGetEnableChecksumsFile(dbfile) && 
        DBGetCompressionFile(dbfile))
    {
        H5Pset_chunk(P_ckcrprops, rank, size);
        if (db_hdf5_set_compression(dbfile, 0)<0) {
            db_perror("db_hdf5_set_compression", E_CALLFAIL, me);
            return(-1);
        }
        P_crprops = P_ckcrprops;
    }
    else if (DBGetCompressionFile(dbfile))
    {
        H5Pset_chunk(P_ckcrprops, rank, size);
        if (db_hdf5_set_compression(dbfile, 0)<0) {
            db_perror("db_hdf5_set_compression", E_CALLFAIL, me);
            return(-1);
        }
        P_crprops = P_ckcrprops;
    }
    return 0;
}
/*-------------------------------------------------------------------------
 * Function:    db_hdf5_get_comp_var 
 *
 * Purpose:     Given a file handle and varname containing at least one
 *              underscore, attempts to get size and, optionally, read
 *              the corresponding HDF5 datatype name and member
 *              name pair, if it exists, formed by splitting the given
 *              varname at the underscore character. If multiple splits
 *              could possibly match with an HDF5 datatype name and member
 *              name pair in the file, only the first found is returned.
 *
 *              This function is necessary to support the old pdb-ish way
 *              of permitting a Silo client to request raw data arrays
 *              for the component of an object by the naming convention
 *              "<objname>_<compname>"
 *
 * Return:      If a valid split exists:                1
 *
 *              If a valid split does not exist:        0 
 *
 * Programmer:  Mark C. Miller 
 *              October 11, 2005 
 *
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_get_comp_var(DBfile *_dbfile, char const *name, hsize_t *nelmts,
    size_t *elsize, hsize_t *fsize, hid_t *datatype, void **buf)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    hid_t fileid = dbfile->cwg;
    hid_t type_id = -1, stypeid = -1, attr = -1, comptype = -1, memtype = -1;
    int membno = -1;
    int retval = 0;
    hsize_t numvals = 0;
    size_t valsize = 0;
    hsize_t filesize = 0;

    /* loop trying different typename, member name combinations */
    char *tmpname = STRDUP(name);
    char *p = strrchr(tmpname, '_');
    char *type_name, *memname;
    while (p != 0 && *p != '\0')
    {
        char *tmpp = p;
        *p = '\0';
        type_name = tmpname;
        memname = p+1;

        stypeid = attr = type_id = -1;
        if ((type_id=H5Topen(fileid, type_name, H5P_DEFAULT))>=0 &&
            (attr=H5Aopen_name(type_id, "silo"))>=0 &&
            (stypeid=H5Aget_type(attr))>=0 &&
            (membno=H5Tget_member_index(stypeid, memname))>=0)
            retval = 1;

        if (retval == 1) break;

        if (attr != -1) H5Aclose(attr);
        if (stypeid != -1) H5Tclose(stypeid);
        if (type_id != -1) H5Tclose(type_id);

        p = strrchr(tmpname, '_');
        *tmpp = '_';
    }

    if (retval == 1)
    {
        hid_t mbtype = H5Tget_member_type(stypeid, membno);
        H5T_class_t mbclass = H5Tget_class(mbtype);

        switch (mbclass) {
            case H5T_INTEGER:
                numvals = 1;
                comptype = H5T_NATIVE_INT;
                break;
            case H5T_FLOAT:
                numvals = 1;
                comptype = H5T_NATIVE_FLOAT;
                break;
            case H5T_ARRAY:
                {
                    int i, ndims, len = 1;
                    hsize_t size[3];
                    comptype = db_hdf5_get_cmemb(stypeid, membno, &ndims, size);
                    for (i = 0; i < ndims; i++)
                        len *= (int) size[i];
                    numvals = len;
                    break;
                }
            case H5T_STRING:
                numvals = 1;
                comptype = T_str256; /* it may be an indirect dataset */
                break;
            default:
                numvals = 0;
                break;
        }

        /* note, a comptype of T_str256 means either that the component
           contains the name of a dataset (indirect) case which we will
           need to read or the component is some other 256 character
           string */

        /* read the component data only if caller requested it */
        if ((numvals && buf) || comptype == T_str256)
        {
            char tmp[256];
            int buf_was_allocated = 0;

            valsize = H5Tget_size(comptype);

            if (comptype != T_str256 && *buf == 0)
            {
                *buf = malloc(*nelmts * *elsize);
                buf_was_allocated = 1;
            }

            /* create a component type with just one member,
               the one we're interested in */
            memtype = H5Tcreate(H5T_COMPOUND, H5Tget_size(comptype));
            memname = H5Tget_member_name(stypeid, membno);
            H5Tinsert(memtype, memname, 0, comptype);
            free(memname);

            /* read attribute for the silo object data */
            H5Aread(attr, memtype, comptype==T_str256?tmp:*buf);
            H5Tclose(memtype);

            /* do the indirection if necessary */
            if (comptype == T_str256 &&
                strncmp(tmp,"/.silo/#",8) == 0) /* indirect case */
            {
                hid_t d, fspace, ftype, mtype;

                d = H5Dopen(fileid, tmp, H5P_DEFAULT);
                fspace = H5Dget_space(d);
                filesize = H5Dget_storage_size(d);               
                numvals = H5Sget_simple_extent_npoints(fspace);
                ftype = H5Dget_type(d);
                mtype = hdf2hdf_type(ftype);
                valsize = H5Tget_size(mtype);
                comptype = mtype;
                if (buf)
                {
                    if (*buf == 0)
                    {
                        *buf = malloc(numvals*H5Tget_size(mtype));
                        buf_was_allocated = 1;
                    }
                    else if (buf_was_allocated)
                    {
                        *buf = realloc(*buf, numvals*H5Tget_size(mtype));
                    }

                    P_rdprops = H5P_DEFAULT;
                    if (!DBGetEnableChecksumsFile(_dbfile))
                        P_rdprops = P_ckrdprops;

                    if (H5Dread(d, mtype, H5S_ALL, H5S_ALL, P_rdprops, *buf)<0) {
                        hdf5_to_silo_error(name, "db_hdf5_get_comp_var");
                        if (buf_was_allocated)
                        {
                            free(*buf);
                            *buf = 0;
                        }
                        retval = 0;
                    }
                }
                H5Tclose(ftype);
                H5Dclose(d);
                H5Sclose(fspace);
            }
            else if (comptype == T_str256) /* other string case */
            {
                if (buf)
                {
                    int n = strlen(tmp)+1;
                    if (*buf == 0)
                    {
                        *buf = malloc(n);
                    }
                    else if (buf_was_allocated)
                    {
                        *buf = realloc(*buf, n);
                    }
                    strncpy((char *)(*buf), tmp, n);
                    valsize = n;
                }
            }
            else
            {
                /* any other case, the component's data will have already
                   been read in to *buf in the H5Aread call, above */
            }
        }

        if (retval == 1)
        {
            if (nelmts)
                *nelmts = numvals;
            if (elsize)
                *elsize = valsize; 
            if (datatype)
                *datatype = comptype;
            if (fsize)
                *fsize = filesize;
        }
    }

    if (attr != -1)    H5Aclose(attr);
    if (stypeid != -1) H5Tclose(stypeid);
    if (type_id != -1)  H5Tclose(type_id);

    FREE(tmpname);
    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    load_toc
 *
 * Purpose:     Add an object to the table of contents
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Feb  1 13:48:33 PST 2005
 *   Made it deal with case of QUAD_RECT or QUAD_CURV
 *
 *-------------------------------------------------------------------------
 */
PRIVATE herr_t
load_toc(hid_t grp, char const *name, H5L_info_t const *dummy, void *_toc)
{
    DBtoc               *toc = (DBtoc*)_toc;
    H5G_stat_t          sb;
    H5L_info_t          lb;
    DBObjectType        objtype = DB_INVALID_OBJECT;
    int                 *nvals=NULL, _objtype, islink=0;
    char                ***names=NULL;
    hid_t               obj=-1, attr=-1;

    if (H5Gget_objinfo(grp, name, FALSE, &sb)<0) return -1;
    if (H5Lget_info(grp, name, &lb, H5P_DEFAULT)<0) return -1;
    if (sb.type == H5G_LINK)
    {
        islink = 1;
        if (H5Gget_objinfo(grp, name, TRUE, &sb)<0) return -1;
    }
    else if (lb.type == H5L_TYPE_EXTERNAL)
    {
        /* external links are presently constrained to work
           only for group tagets */
        islink = 1;
        sb.type = H5G_GROUP;
    }
    switch (sb.type) {
    case H5G_GROUP:
        /*
         * Any group which has a `..' entry is a silo directory. The `..'
         * names do not appear in the silo table of contents.
         */
        if (!strcmp(name, "..") || (obj=H5Gopen(grp, name, H5P_DEFAULT))<0) break;
        H5E_BEGIN_TRY {
            if (H5Gget_objinfo(obj, "..", FALSE, NULL)>=0) objtype = DB_DIR;
        } H5E_END_TRY;
        H5Gclose(obj);
        break;

    case H5G_TYPE:
        if ((obj=H5Topen(grp, name, H5P_DEFAULT))<0) break;
        if ((attr=H5Aopen_name(obj, "silo_type"))<0) break;
        if (H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0) break;
        objtype = (DBObjectType)_objtype;
        H5Aclose(attr);
        H5Tclose(obj);
        break;

    case H5G_DATASET:
        objtype = DB_VARIABLE;
        islink = 0; /* ignore links on raw data */
        break;

    default:
        /*ignore*/
        break;
    }

    /* What table of contents field does this object belong to? */
    switch (objtype) {
    case DB_INVALID_OBJECT:
        break;
    case DB_QUADMESH:
    case DB_QUAD_RECT:
    case DB_QUAD_CURV:
        names = &(toc->qmesh_names);
        nvals = &(toc->nqmesh);
        break;
    case DB_QUADVAR:
        names = &(toc->qvar_names);
        nvals = &(toc->nqvar);
        break;
    case DB_CSGMESH:
        names = &(toc->csgmesh_names);
        nvals = &(toc->ncsgmesh);
        break;
    case DB_CSGVAR:
        names = &(toc->csgvar_names);
        nvals = &(toc->ncsgvar);
        break;
    case DB_UCDMESH:
        names = &(toc->ucdmesh_names);
        nvals = &(toc->nucdmesh);
        break;
    case DB_UCDVAR:
        names = &(toc->ucdvar_names);
        nvals = &(toc->nucdvar);
        break;
    case DB_MULTIMESH:
        names = &(toc->multimesh_names);
        nvals = &(toc->nmultimesh);
        break;
    case DB_MULTIMESHADJ:
        names = &(toc->multimeshadj_names);
        nvals = &(toc->nmultimeshadj);
        break;
    case DB_MULTIVAR:
        names = &(toc->multivar_names);
        nvals = &(toc->nmultivar);
        break;
    case DB_MULTIMAT:
        names = &(toc->multimat_names);
        nvals = &(toc->nmultimat);
        break;
    case DB_MULTIMATSPECIES:
        names = &(toc->multimatspecies_names);
        nvals = &(toc->nmultimatspecies);
        break;
    case DB_MATERIAL:
        names = &(toc->mat_names);
        nvals = &(toc->nmat);
        break;
    case DB_MATSPECIES:
        names = &(toc->matspecies_names);
        nvals = &(toc->nmatspecies);
        break;
    case DB_CURVE:
        names = &(toc->curve_names);
        nvals = &(toc->ncurve);
        break;
    case DB_DEFVARS:
        names = &(toc->defvars_names);
        nvals = &(toc->ndefvars);
        break;
    case DB_POINTMESH:
        names = &(toc->ptmesh_names);
        nvals = &(toc->nptmesh);
        break;
    case DB_POINTVAR:
        names = &(toc->ptvar_names);
        nvals = &(toc->nptvar);
        break;
    case DB_ARRAY:
        names = &(toc->array_names);
        nvals = &(toc->narray);
        break;
    case DB_DIR:
        names = &(toc->dir_names);
        nvals = &(toc->ndir);
        break;
    case DB_VARIABLE:
        names = &(toc->var_names);
        nvals = &(toc->nvar);
        break;
    case DB_MRGTREE:
        names = &(toc->mrgtree_names);
        nvals = &(toc->nmrgtree);
        break;
    case DB_GROUPELMAP:
        names = &(toc->groupelmap_names);
        nvals = &(toc->ngroupelmap);
        break;
    case DB_MRGVAR:
        names = &(toc->mrgvar_names);
        nvals = &(toc->nmrgvar);
        break;
    case DB_USERDEF:  /*fall through*/
    case DB_FACELIST: /*fall through*/
    case DB_EDGELIST: /*fall through*/
    case DB_ZONELIST:
    case DB_PHZONELIST:
    case DB_CSGZONELIST:
        names = &(toc->obj_names);
        nvals = &(toc->nobj);
        break;
    }

    /* Append to table of contents */
#ifndef _MSC_VER
#warning REVISIT THIS CODE BLOCK
#endif
#if 0
    if (islink) {
        int n = toc->nsymlink++;
        toc->symlink_names = (char **) realloc(toc->symlink_names, (n+1)*sizeof(char*));
        toc->symlink_names[n] = STRDUP(name);
    }
    else if (names && nvals) {
        int n = (*nvals)++;
        *names = (char **)realloc(*names, *nvals*sizeof(char*));
        (*names)[n] = STRDUP(name);
    }
#endif
    if (names && nvals) {
        int n1 = (*nvals)++;
        *names = (char **)realloc(*names, *nvals*sizeof(char*));
        (*names)[n1] = STRDUP(name);
        if (islink) {
            char target[2*256];
            int n2 = toc->nsymlink++;
            toc->symlink_names = (char **) realloc(toc->symlink_names, (n2+1)*sizeof(char*));
            toc->symlink_names[n2] = (*names)[n1]; /* note: copy of the pointer */
            toc->symlink_target_names = (char **) realloc(toc->symlink_target_names, (n2+1)*sizeof(char*));
            if (db_hdf5_getslink(grp, name, target) == 0)
                toc->symlink_target_names[n2] = STRDUP(target);
            else
                toc->symlink_target_names[n2] = STRDUP("unknown");
        }
    }

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    find_objno
 *
 * Purpose:     Determines if the specified object has the same object ID as
 *              what is stored in comp->objno and if so copies NAME to
 *              comp->name.
 *
 * Return:      Success:        1 if objno's are the same
 *
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PRIVATE herr_t
find_objno(hid_t grp, char const *name, void *_comp)
{
    silo_hdf5_comp_t    *comp = (silo_hdf5_comp_t*)_comp;
    H5G_stat_t          sb;

    if (H5Gget_objinfo(grp, name, TRUE, &sb)<0) return -1;
    if (sb.objno[0]!=comp->objno[0] || sb.objno[1]!=comp->objno[1]) return 0;
    comp->name = STRDUP(name);
    return 1;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_handle_ctdt 
 *
 * Purpose:     Handle special logic for cycle, time and dtime optins. 
 *
 * Programmer:  Mark C. Miller, Thu Feb  4 08:58:10 PST 2010
 *
 * Modifications:
 * 
 *  Mark C. Miller, Sat Feb  6 09:54:37 PST 2010
 *  In hopes that H5Lexists might be faster than attempting to open a
 *  dataset, added that as initial test for existance.
 *-------------------------------------------------------------------------
 */
PRIVATE void 
db_hdf5_handle_ctdt(DBfile_hdf5 *dbfile, int ts, float t, int dts, double dt, int c)
{
    int set[3];
    void *buf[3];
    char *names[] = {"time","dtime","cycle"};
    int types[] = {DB_FLOAT, DB_DOUBLE, DB_INT};
    hid_t space = -1;
    int i;

    set[0] = ts;
    buf[0] = (void*)&t;
    set[1] = dts;
    buf[1] = (void*)&dt;
    set[2] = 1;
    buf[2] = (void*)&c;

    H5E_BEGIN_TRY
    {
        for (i = 0; i < 3; i++)
        {
            hid_t dset, mtype, ftype;
            const hsize_t one = 1;
            htri_t h5lexists = -1;

            if (!set[i]) continue;

#if HDF5_VERSION_GE(1,8,0)
            if ((h5lexists = H5Lexists(dbfile->cwg, names[i], H5P_DEFAULT)) == TRUE)
                continue;
#endif
            if (h5lexists != FALSE)
            {
                dset = H5Dopen(dbfile->cwg, names[i], H5P_DEFAULT);
                if (!(dset<0))
                {
                    H5Dclose(dset);
                    continue;
                }
            }

            mtype = silom2hdfm_type(types[i]);
            ftype = silof2hdff_type(dbfile, types[i]);
            if (space == -1)
                space = H5Screate_simple(1, &one, &one);
            dset = H5Dcreate(dbfile->cwg, names[i], ftype, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            H5Dwrite(dset, mtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf[i]);
            H5Dclose(dset);
        }

        if (space != -1)
            H5Sclose(space);

    }
    H5E_END_TRY;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_compname
 *
 * Purpose:     Returns a new name relative to the link directory. The name
 *              is generated by reading the `nlinks' attribute of the link
 *              directory, incrementing it, creating a file name, and saving
 *              it back to the file.
 *
 * Return:      Success:        0, A new link name not more than 8 characters
 *                              long counting the null terminator is returned
 *                              through the NAME argument.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 23, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_compname(DBfile_hdf5 *dbfile, char name[8]/*out*/)
{
    static char *me = "db_hdf5_compname";
    hid_t       attr=-1;
    int         nlinks;

    PROTECT {
        /* Open or create the `nlinks' attribute of the link group */
        H5E_BEGIN_TRY {
            attr = H5Aopen_name(dbfile->link, "nlinks");
        } H5E_END_TRY;
        if (attr<0 && (attr=H5Acreate(dbfile->link, "nlinks", H5T_NATIVE_INT,
                                      SCALAR, H5P_DEFAULT, H5P_DEFAULT))<0) {
            db_perror("nlinks attribute", E_CALLFAIL, me);
            UNWIND();
        }

        /* Increment the nlinks value */
        if (H5Aread(attr, H5T_NATIVE_INT, &nlinks)<0) {
            db_perror("nlinks attribute", E_CALLFAIL, me);
            UNWIND();
        }

        nlinks++;
        if (nlinks > 999999) {
            db_perror("exceeded maximum number of nlinks", E_CALLFAIL, me);
            UNWIND();
        }

        if (H5Awrite(attr, H5T_NATIVE_INT, &nlinks)<0) {
            db_perror("nlinks attribute", E_CALLFAIL, me);
            UNWIND();
        }
        H5Aclose(attr);

        /* Create a name */
        sprintf(name, "#%06d", nlinks);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
        } H5E_END_TRY;
    } END_PROTECT;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_compwrz
 *
 * Purpose:     Creates a new dataset in the link group.
 *
 * Return:      Success:        >=0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 23, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver 
 *
 *   Mark C. Miller, Thu Sep  8 12:54:39 PDT 2005
 *   Made it permit buf to be NULL in which case it will only create the
 *   dataset but not attempt to write to it. Also, made it return hid_t of
 *   created dataset.
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Added support for friendly hdf5 dataset names (as soft links)
 *
 *   Mark C. Miller, Tue Jul 15 22:37:31 PDT 2008
 *   Added 'z' to name and compressionFlags argument to provide control
 *   over mesh-level compresion algorithms. The older (non-z) method 
 *   remains as a stub that simply calls this one with zero for flags.
 *
 *   Mark C. Miller, Thu Nov  5 16:15:00 PST 2009
 *   Added support for hard links for friendly names too. 
 *
 *   Mark C. Miller, Thu Feb  4 11:24:24 PST 2010
 *   Removed logic specific to support cycle, time and time.
 *   Mark C. Miller, Wed Jul 14 20:49:21 PDT 2010
 *
 *   Added logic to support a different kind of 'friendly-names' mode
 *   where no datasets are put in the 'LINKGRP' and are instead put
 *   'next to' the objects they bind with. The intention is to eliminate
 *   the one, very, very large '/.silo' group.
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_compwrz(DBfile_hdf5 *dbfile, int dtype, int rank, int const _size[],
               void const *buf, char *name/*in,out*/, char const *fname,
               int compressionFlags)
{
    static char *me = "db_hdf5_compwr";
    hid_t       dset=-1, mtype=-1, ftype=-1, space=-1;
    int         i, nels;
    hsize_t     size[8];
    int         alloc = 0;

    if (rank < 0)
    {
        rank = -rank;
        alloc = 1;
    }

    /* Not an error if there is no data */
    for (i=0, nels=1; i<rank; i++) nels *= _size[i];
    if ((!buf || !nels) && !alloc) {
        *name = '\0';
        return 0;
    }

    PROTECT {
        /* Obtain a unique name for the dataset or use the name supplied */
        if (!*name) {
            strcpy(name, LINKGRP);
            if (db_hdf5_compname(dbfile, ENDOF(name)/*out*/)<0) {
                db_perror("compname", E_CALLFAIL, me);
                UNWIND();
            }
        }
        
        /* Obtain the memory and file types for the dataset */
        if ((mtype=silom2hdfm_type(dtype))<0 ||
            (ftype=silof2hdff_type(dbfile, dtype))<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create the dataset and write data */
        assert(rank>0 && (size_t)rank<=NELMTS(size));
        for (i=0; i<rank; i++) size[i] = _size[i];
        if ((space=H5Screate_simple(rank, size, size))<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
 
        if (db_hdf5_set_properties((DBfile*) dbfile, rank, size) < 0 ) {
            db_perror("db_hdf5_set_properties", E_CALLFAIL, me);
            UNWIND();
        }
        if (DBGetCompressionFile((DBfile*)dbfile) && compressionFlags)
        {
            if (db_hdf5_set_compression((DBfile*)dbfile, compressionFlags)<0)
            {
                db_perror("db_hdf5_set_compression", E_CALLFAIL, me);
                UNWIND();
            }
        }

        if (DBGetFriendlyHDF5NamesFile((DBfile*)dbfile) == 2)
        {
            if (fname)
            {
                if ((dset=H5Dcreate(dbfile->cwg, fname, ftype, space, H5P_DEFAULT, P_crprops, H5P_DEFAULT))<0) {
                    db_perror(name, E_CALLFAIL, me);
                    UNWIND();
                }
                strcpy(name, fname);
            }
            else
            {
                if ((dset=H5Dcreate(dbfile->link, name, ftype, space, H5P_DEFAULT, P_crprops, H5P_DEFAULT))<0) {
                    db_perror(name, E_CALLFAIL, me);
                    UNWIND();
                }
            }
        }
        else
        {
            if ((dset=H5Dcreate(dbfile->link, name, ftype, space, H5P_DEFAULT, P_crprops, H5P_DEFAULT))<0) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }
            if (fname && DBGetFriendlyHDF5NamesFile((DBfile*)dbfile) == 1)
                H5Glink(dbfile->cwg, H5G_LINK_SOFT, name, fname);
        }

        if (buf && H5Dwrite(dset, mtype, space, space, H5P_DEFAULT, buf)<0) {
            hdf5_to_silo_error(name, "db_hdf5_compwrz");
            UNWIND();
        }

        /* Release resources */
        H5Dclose(dset);
        H5Sclose(space);

        /* remove any mesh specific filters if we have 'em */
        if (DBGetCompressionFile((DBfile*)dbfile) && compressionFlags)
        {
            int i;
            for (i=0; i<H5Pget_nfilters(P_crprops); i++)
            {
#if defined H5_USE_16_API || !HDF5_VERSION_GE(1,8,0)
                if (H5Pget_filter(P_crprops,(unsigned)i,0,0,0,0,0) == DB_HDF5_HZIP_ID)
#else
                if (H5Pget_filter(P_crprops,(unsigned)i,0,0,0,0,0,NULL) == DB_HDF5_HZIP_ID)
#endif
                {
                    H5Premove_filter(P_crprops, DB_HDF5_HZIP_ID);
                    break;
                }
            }
        }

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Dclose(dset);
            H5Sclose(space);
        } H5E_END_TRY;
    } END_PROTECT;
    return dset; 
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_compwr
 *
 * Purpose:     Creates a new dataset in the link group without compression.
 *
 * Return:      Success:        >=0
 *
 *              Failure:        -1
 *
 * Created: Mark C. Miller, Thu Jul 17 15:03:55 PDT 2008
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_compwr(DBfile_hdf5 *dbfile, int dtype, int rank, int const _size[],
               void const *buf, char *name/*in,out*/, char const *fname)
{
    return db_hdf5_compwrz(dbfile, dtype, rank, _size, buf, name, fname, 0);
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_compckz
 *
 * Purpose:     Check if a dataset is compressed with HZIP 
 *
 * Return:      Success:        >=0
 *
 *              Failure:        -1
 *
 * Created: Mark C. Miller, Thu Jul 17 15:03:55 PDT 2008
 *-------------------------------------------------------------------------
 */
PRIVATE int 
db_hdf5_compckz(DBfile_hdf5 *dbfile, char *name)
{
    int retval = 0;
    int i;
    hid_t d = -1, plist = -1;
    static char *me = "db_hdf5_compzkz";

    PROTECT {
        if (name && *name) {
            if ((d=H5Dopen(dbfile->cwg, name, H5P_DEFAULT))<0) {
                db_perror(name, E_NOTFOUND, me);
                UNWIND();
            }
            if ((plist=H5Dget_create_plist(d))<0) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }

            for (i=0; i<H5Pget_nfilters(plist); i++)
            {
                char name[256];
#if defined H5_USE_16_API || !HDF5_VERSION_GE(1,8,0)
                H5Pget_filter(plist,(unsigned)i,0,0,0,sizeof(name),name);
#else
                H5Pget_filter(plist,(unsigned)i,0,0,0,sizeof(name),name,NULL);
#endif
                if (strstr(name, "Lindstrom-hzip"))
                {
                    retval = 1;
                    break;
                }
            }
            H5Dclose(d);
            H5Pclose(plist);
        }
    } CLEANUP {
        if (d != -1) H5Dclose(d);
        if (plist != -1) H5Pclose(plist);
    } END_PROTECT;

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_comprd
 *
 * Purpose:     Reads a dataset from the file into memory.
 *
 * Return:      Success:        Pointer to dataset values.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, March 31, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-10-13
 *              Uses the current working directory instead of the root
 *              directory.
 *-------------------------------------------------------------------------
 */
PRIVATE void *
db_hdf5_comprd(DBfile_hdf5 *dbfile, char *name, int ignore_force_single)
{
    static char *me = "db_hdf5_comprd";
    void        *buf = NULL;
    hid_t       d=-1, fspace=-1, ftype=-1, mtype=-1;
    int         i, nelmts;
    void       *retval = NULL;
    
    PROTECT {
        if (name && *name) {
            if ((d=H5Dopen(dbfile->cwg, name, H5P_DEFAULT))<0) {
                db_perror(name, E_NOTFOUND, me);
                UNWIND();
            }
            if ((fspace=H5Dget_space(d))<0 || (ftype=H5Dget_type(d))<0) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }
            nelmts = H5Sget_simple_extent_npoints(fspace);

            /* Choose a memory type based on the file type */
            mtype = hdf2hdf_type(ftype);

            /* if we decided on a memory type of double but
             * we are forcing single precision, then select
             * a memory type of float */
            if (mtype == H5T_NATIVE_DOUBLE &&
                force_single_g && !ignore_force_single)
                mtype = H5T_NATIVE_FLOAT;

            /* Read the data */
            if (NULL==(buf=malloc(nelmts*H5Tget_size(mtype)))) {
                db_perror(name, E_NOMEM, me);
                UNWIND();
            }

            P_rdprops = H5P_DEFAULT;
            if (!DBGetEnableChecksumsFile((DBfile*)dbfile))
                P_rdprops = P_ckrdprops;

            if (H5Dread(d, mtype, H5S_ALL, H5S_ALL, P_rdprops, buf)<0) {
                hdf5_to_silo_error(name, me);
                UNWIND();
            }

            /* Free resources */
            H5Dclose(d);
            H5Tclose(ftype);
            H5Sclose(fspace);

            /* Setup return value */
            retval = buf;
            
            /* Convert to float if necessary */
            /* With newer versions of HDF5, this could have been done
             * automatically in the H5Dread call, above by selecting
             * the appropriate memory type. However, the current version
             * of HDF5 does not support conversion from integral types
             * to float (or double) */
            if (force_single_g && !ignore_force_single &&
                mtype != H5T_NATIVE_FLOAT)
            {
                float *newbuf;

                /* allocate a new buffer */
                if (NULL==(newbuf=(float*)malloc(nelmts*sizeof(float)))) {
                    db_perror(name, E_NOMEM, me);
                    UNWIND();
                }

                /* do the conversion */
                if      (mtype == H5T_NATIVE_UCHAR)
                {
                    char *cbuf = (char *) buf;
                    for (i = 0; i < nelmts; i++)
                        newbuf[i] = (float)(cbuf[i]);
                }
                else if (mtype == H5T_NATIVE_SHORT)
                {
                    short *sbuf = (short *) buf;
                    for (i = 0; i < nelmts; i++)
                        newbuf[i] = (float)(sbuf[i]);
                }
                else if (mtype == H5T_NATIVE_INT)
                {
                    int *ibuf = (int *) buf;
                    for (i = 0; i < nelmts; i++)
                        newbuf[i] = (float)(ibuf[i]);
                }
                else if (mtype == H5T_NATIVE_LONG)
                {
                    long *lbuf = (long *) buf;
                    for (i = 0; i < nelmts; i++)
                        newbuf[i] = (float)(lbuf[i]);
                }
                else if (mtype == H5T_NATIVE_LLONG)
                {
                    long long *lbuf = (long long *) buf;
                    for (i = 0; i < nelmts; i++)
                        newbuf[i] = (float)(lbuf[i]);
                }

                /* Free old buffer and setup return value */
                free(buf);
                retval = newbuf;
            }
        }
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Dclose(d);
            H5Tclose(ftype);
            H5Sclose(fspace);
        } H5E_END_TRY;
        FREE(buf);
    } END_PROTECT;

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_fullname
 *
 * Purpose:     Given the current working directory name, the parent object
 *              name as passed by the Silo library client and the child
 *              object name as stored in the parent object, determine the
 *              absolute, full path name of the child object.
 *
 * Return:      Success:     fully resolved full path name of child object
 *
 *              Failure:     0 
 *-------------------------------------------------------------------------
 */
PRIVATE char * 
db_hdf5_resolvename(DBfile *_dbfile,
                    char const *parent_objname,
                    char const *child_objname)
{
#define SAVE_AND_RETURN(S,FREES)  \
{                                 \
    if (cbuf[n]) free(cbuf[n]);   \
    cbuf[n] = strdup(S);          \
    if (FREES) free(S);           \
    n = (n + 1) % nmax;           \
    return cbuf[n?n-1:nmax-1];    \
}
    static int n = 0;
    static int const nmax = 32;
    static char *cbuf[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                             0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#ifndef _MSC_VER
#warning HARD CODED SIZE HERE
#endif
    static char cwgname[4096];
    char *parent_objdirname = 0;
    char *parent_fullname = 0;
    char *child_fullname = 0;


    /* logic to clear circ_buf by passing all 0 args */
    if (!_dbfile && !parent_objname && !child_objname)
    {
        int i;
        for (i = 0; i < nmax; i++)
        {
            if (cbuf[i]) free(cbuf[i]);
            cbuf[i] = 0;
        }
        return 0;
    }

    if (!child_objname || !*child_objname)
        SAVE_AND_RETURN("",0);

    db_hdf5_GetDir(_dbfile, cwgname);
    parent_objdirname = db_dirname(parent_objname);
    if (parent_objdirname)
        parent_fullname = db_join_path(cwgname, parent_objdirname);
    if (parent_fullname)
        child_fullname = db_join_path(parent_fullname, child_objname);

    if (parent_objdirname) free(parent_objdirname);
    if (parent_fullname) free(parent_fullname);

    if (child_fullname)
        SAVE_AND_RETURN(child_fullname,1);

    SAVE_AND_RETURN("",0);
#undef SAVE_AND_RETURN
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_fullname
 *
 * Purpose:     Given a name return the corresponding absolute name. If NAME
 *              is the empty string then the FULL output value will also be
 *              the empty string.
 *
 * Return:      Success:        0, full name returned through FULL argument.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 23, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_fullname(DBfile_hdf5 *dbfile, char *name, char *full/*out*/)
{
    if (!name || !*name) {
        *full = '\0';
    } else if ('/'==*name) {
        strcpy(full, name);
    } else {
        db_hdf5_GetDir((DBfile*)dbfile, full);
        if (strcmp(full, "/")) strcat(full, "/");
        strcat(full, name);
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_hdrwr
 *
 * Purpose:     Writes BUF as the `silo' attribute of an object.  If the
 *              object does not exist then it is created as a named data type
 *              (because a named data type has less overhead than an empty
 *              dataset in hdf5).
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 23, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_hdrwr(DBfile_hdf5 *dbfile, char *name, hid_t mtype, hid_t ftype,
              void *buf, DBObjectType objtype)
{
    static char *me = "db_hdf5_hdrwr";
    hid_t       obj=-1, attr=-1;
    int         _objtype = (int)objtype;
    int         created = FALSE;
    
    PROTECT {
        /* Open an existing object or create a named type */
        H5E_BEGIN_TRY {
            obj = H5Topen(dbfile->cwg, name, H5P_DEFAULT);
        } H5E_END_TRY;
        if (obj<0) {
            obj = H5Tcopy(H5T_NATIVE_INT);
            if (H5Tcommit(dbfile->cwg, name, obj, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)<0) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }
            created = TRUE;
        }

        /* Open or create the `silo' attribute */
        if (created) {
            attr = -1;
        } else {
            H5E_BEGIN_TRY {
                attr = H5Aopen_name(obj, "silo");
            } H5E_END_TRY;
        }
        if (attr<0 && (attr=H5Acreate(obj, "silo", ftype, SCALAR,
                                      H5P_DEFAULT, H5P_DEFAULT))<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Write data to the attribute */
        if (H5Awrite(attr, mtype, buf)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        H5Aclose(attr);

        /* Open or create the `silo_type' attribute */
        if (created) {
            attr = -1;
        } else {
            H5E_BEGIN_TRY {
                attr = H5Aopen_name(obj, "silo_type");
            } H5E_END_TRY;
        }
        if (attr<0 && (attr=H5Acreate(obj, "silo_type", H5T_NATIVE_INT, SCALAR,
                                      H5P_DEFAULT, H5P_DEFAULT))<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (H5Awrite(attr, H5T_NATIVE_INT, &_objtype)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        H5Aclose(attr);
        H5Tclose(obj);

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(obj);
        } H5E_END_TRY;
    } END_PROTECT;
    return 0;
}


/*-------------------------------------------------------------------------
 * Function:    db_hdf5_ForceSingle
 *
 * Purpose:     If STATUS is non-zero then all floating point raw data values
 *              transferred between the application and the silo API will be
 *              of type `float'; otherwise type `double' is assumed.
 *
 * Return:      Success:        0
 *
 *              Failure:        never fails
 *
 * Programmer:  Robb Matzke
 *              Monday, March 22, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
INTERNAL int
db_hdf5_ForceSingle(int status)
{
    force_single_g = status;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_process_file_options
 *
 * Purpose:     Laboriously digest a file options set and set up correct
 *              HDF5 vfd behavior.
 *
 * Return:      The constructed file access properties list hid_t
 * 
 * Programmer:  Mark C. Miller
 *              Febuary, 2010 
 *
 * Modifications:
 *
 *  Mark C. Miller, Wed Jul 14 20:52:19 PDT 2010
 *  Moved default silo settings to macro constants in H5FDsilo.h.
 *  Added support for direct I/O to silo vfd.
 *
 *  Mark C. Miller, Wed Aug  4 16:06:41 PDT 2010
 *  Added conditional compilation logic for silo fapl for HDF5 1.8.4 or
 *  greater.
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t
db_hdf5_process_file_options(int opts_set_id, int mode, hid_t *fcpl)
{
    static char *me = "db_hdf5_process_file_options";
    hid_t retval = H5Pcreate(H5P_FILE_ACCESS);
    herr_t h5status = 0;
#if HDF5_VERSION_GE(1,8,4)
    H5AC_cache_config_t h5mdc_config_best_perf;
#endif

#if HDF5_VERSION_GE(1,10,0)
    if (mode & DB_PERF_OVER_COMPAT)
        H5Pset_libver_bounds(retval, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
    if (mode & DB_COMPAT_OVER_PERF)
        H5Pset_libver_bounds(retval, H5F_LIBVER_V18, H5F_LIBVER_V18);
#endif

#if HDF5_VERSION_GE(1,10,1)
    H5Pset_evict_on_close(retval, (hbool_t)1);
#endif

    /* First, initialize our copy of h5mdc_config */
    h5mdc_config_best_perf.version = H5AC__CURR_CACHE_CONFIG_VERSION;
    H5Pget_mdc_config(retval, &h5mdc_config_best_perf);

    /* Setup Mainzer (best performance) mdc config params */
    h5mdc_config_best_perf.set_initial_size = 1;
    h5mdc_config_best_perf.initial_size = 16384;
    h5mdc_config_best_perf.min_size = 8192;
    h5mdc_config_best_perf.epoch_length = 3000;
    h5mdc_config_best_perf.lower_hr_threshold = 1e-5;

    /* Set mdc config params */
    H5Pset_mdc_config(retval, &h5mdc_config_best_perf);

    /* This property effects how HDF5 deals with objects that are left
       open when the file containing them is closed. The SEMI setting
       means that HDF5 will flag it is an error. The STRONG setting
       means that HDF5 will attempt to close any option objects
       automatically. That can wind up hiding coding errors in this
       Silo plugin, so we do that only when user has requested that
       either top or no errors are reported. */
    if (SILO_Globals._db_err_level_drvr == DB_ALL)
        h5status |= H5Pset_fclose_degree(retval, H5F_CLOSE_SEMI);
    else
        h5status |= H5Pset_fclose_degree(retval, H5F_CLOSE_STRONG);

    /* Disable chunk caching since we single chunk */
    H5Pset_cache(retval, 0, 0, 0, 0);

    /* Handle cases where we are running on Windows. If a client
       request anything other than the default driver, we issue
       a warning message and continue only on windows (default) vfd. */
#if !defined(_MSC_VER)
#warning REMOVED WINDOWS SPECIFIC CHECK
#endif
#if 0
#if defined(_WIN32)
    if (opts_set_id != DB_FILE_OPTS_H5_DEFAULT_DEFAULT && 
        opts_set_id < NUM_DEFAULT_FILE_OPTIONS_SETS)
    {
        H5Pclose(retval);
        return db_perror("Non-default HDF5 VFD specified on Windows.", E_CALLFAIL, me);
    }
#endif
#endif

    switch (opts_set_id)
    {
        /* Winds up using whatever the default vfd defined by HDF5 library is. */
        case DB_FILE_OPTS_H5_DEFAULT_DEFAULT:
            break;

        case DB_FILE_OPTS_H5_DEFAULT_SILO:
#if HDF5_VERSION_GE(1,8,4)
            h5status |= H5Pset_fapl_silo(retval);
#else
            H5Pclose(retval);
            return db_perror("Silo block VFD >= HDF5 1.8.4", E_NOTENABLEDINBUILD, me);
#endif
            break;

        /* default HDF5 sec2 driver */
        case DB_FILE_OPTS_H5_DEFAULT_SEC2:
            h5status |= H5Pset_fapl_sec2(retval);
            break;

        /* default HDF5 stdio driver */
        case DB_FILE_OPTS_H5_DEFAULT_STDIO:
            h5status |= H5Pset_fapl_stdio(retval);
            /* I tried also having option to set buffer size. But, you need
               a file handle that has NOT had any I/O occur on it yet and
               HDF5 can't do that */
            break;

        /* default HDF5 core driver 1 Meg inc & backing store */
        case DB_FILE_OPTS_H5_DEFAULT_CORE:
            h5status |= H5Pset_fapl_core(retval, (1<<20), TRUE);
            break;

        /* default HDF5 log driver. Should NOT do any actual I/O. */
        case DB_FILE_OPTS_H5_DEFAULT_LOG:
            h5status |= H5Pset_fapl_log(retval, "silo_hdf5_log.out",
                H5FD_LOG_LOC_IO|H5FD_LOG_NUM_IO|
                H5FD_LOG_TIME_IO|H5FD_LOG_ALLOC, 0);
            break;

        /* default HDF5 split file driver. */
        case DB_FILE_OPTS_H5_DEFAULT_SPLIT:
        {
            hid_t meta_fapl = db_hdf5_process_file_options(DB_FILE_OPTS_H5_DEFAULT_CORE, mode, 0);
            hid_t raw_fapl = db_hdf5_process_file_options(DB_FILE_OPTS_H5_DEFAULT_DEFAULT, mode, 0);
            h5status |= H5Pset_fapl_split(retval, "", meta_fapl, "-raw", raw_fapl);
            H5Pclose(meta_fapl);
            H5Pclose(raw_fapl);
            break;
        }

        /* default HDF5 direct driver. */
        case DB_FILE_OPTS_H5_DEFAULT_DIRECT:
        {
#ifdef H5_HAVE_DIRECT
            int const fourkb = (1<<12);
            h5status |= H5Pset_fapl_direct(retval, fourkb, fourkb, fourkb*256);
            h5status |= H5Pset_alignment(retval, fourkb/2, fourkb); 
#else
            H5Pclose(retval);
            return db_perror("HDF5 Direct VFD", E_NOTENABLEDINBUILD, me);
#endif
            break;
        }

        /* default HDF5 family driver w/1Gig (2^30) members. */
        case DB_FILE_OPTS_H5_DEFAULT_FAMILY:
        {
            hid_t memb_fapl = db_hdf5_process_file_options(DB_FILE_OPTS_H5_DEFAULT_DEFAULT, mode, 0);
            h5status |= H5Pset_fapl_family(retval, (1<<30), memb_fapl);
            H5Pclose(memb_fapl);
            break;
        }

        /* default HDF5 mpi drivers */
        case DB_FILE_OPTS_H5_DEFAULT_MPIP:
        {
#ifndef _MSC_VER
#warning DO WE STILL NEED THIS
#endif
            H5Pclose(retval);
            return db_perror("HDF5 MPI VFD", E_NOTENABLEDINBUILD, me);
            break;
        }

        case DB_FILE_OPTS_H5_DEFAULT_MPIO:
        {
#ifdef H5_HAVE_PARALLEL
            MPI_Info info = MPI_INFO_NULL;
            h5status |= H5Pset_fapl_mpio(retval, MPI_COMM_SELF, info);
#else
            H5Pclose(retval);
            return db_perror("HDF5 MPI VFD", E_NOTENABLEDINBUILD, me);
#endif
            break;
        }

        /* More complex cases where parameters specified by user
           registered options sets. */
        default:
        {
            int _opts_set_id = opts_set_id - NUM_DEFAULT_FILE_OPTIONS_SETS;
            DBoptlist const *opts;
            void *p; int vfd = DB_H5VFD_DEFAULT;

            if (_opts_set_id >= MAX_FILE_OPTIONS_SETS ||
                (opts = SILO_Globals.fileOptionsSets[_opts_set_id]) == 0)
            {
                H5Pclose(retval);
                return db_perror("Bad file options set index", E_CALLFAIL, me);
            }

            /* Handle the case where the hid_t's for FCPL/FAPL are given
               directly in the options set. In this case, we assume *all*
               relevant parameters are handled via FCPL/FAPL specified here
               and do no further procssing of the registered options set. */
            if ((p = DBGetOption(opts, DBOPT_H5_FCPL_HID_T)) && fcpl)
                *fcpl = *((hid_t*)p);

            if ((p = DBGetOption(opts, DBOPT_H5_FAPL_HID_T)))
            {
                retval = *((hid_t*)p);
                return retval;
            }

            /* get the vfd specification */
            if ((p = DBGetOption(opts, DBOPT_H5_VFD)))
            {
                vfd = *((int*)p);
            }
            else if ((p = DBGetOption(opts, DBOPT_H5_USER_DRIVER_ID)))
            {
                int new_driver_id = *((int*)p);
                p = DBGetOption(opts, DBOPT_H5_USER_DRIVER_INFO);
                h5status |= H5Pset_driver(retval, new_driver_id, p);
            }

#if !defined(_MSC_VER)
#warning REMOVED WINDOWS SPECIFIC CHECK
#endif
#if 0
#if defined(_WIN32)
            if (vfd != DB_H5VFD_DEFAULT)
            {
                db_perror("Non-default HDF5 VFD specified on Windows.", E_CALLFAIL, me);
                vfd = DB_H5VFD_DEFAULT;
            }
#endif
#endif
            
            switch (vfd)
            {
                case DB_H5VFD_DEFAULT:
                    break;
                case DB_H5VFD_SEC2:
                    h5status |= H5Pset_fapl_sec2(retval);
                    break;
                case DB_H5VFD_STDIO:
                    h5status |= H5Pset_fapl_stdio(retval);
                    break;
                case DB_H5VFD_SILO:
                {
#if HDF5_VERSION_GE(1,8,4)
                    hsize_t block_size = H5FD_SILO_DEFAULT_BLOCK_SIZE; 
                    int block_count = H5FD_SILO_DEFAULT_BLOCK_COUNT; 
                    int log_stats = H5FD_SILO_DEFAULT_LOG_STATS;
                    int use_direct = H5FD_SILO_DEFAULT_USE_DIRECT;

                    if ((p = DBGetOption(opts, DBOPT_H5_SILO_BLOCK_SIZE)))
                        block_size = (hsize_t) (*((int*) p));
                    if ((p = DBGetOption(opts, DBOPT_H5_SILO_BLOCK_COUNT)))
                        block_count = *((int*) p);
                    if ((p = DBGetOption(opts, DBOPT_H5_SILO_LOG_STATS)))
                        log_stats = *((int*) p);
                    if ((p = DBGetOption(opts, DBOPT_H5_SILO_USE_DIRECT)))
                        use_direct = *((int*) p);

                    h5status |= H5Pset_fapl_silo(retval);
                    h5status |= H5Pset_silo_block_size_and_count(retval, block_size, block_count);
                    h5status |= H5Pset_silo_log_stats(retval, log_stats);
                    h5status |= H5Pset_silo_use_direct(retval, use_direct);
#else
                    H5Pclose(retval);
                    return db_perror("Silo block VFD >= HDF5 1.8.4", E_NOTENABLEDINBUILD, me);
#endif
                    break;
                }
                case DB_H5VFD_LOG:
                {
                    int bufsize = 0;
                    int flags = H5FD_LOG_LOC_IO|H5FD_LOG_NUM_IO|H5FD_LOG_TIME_IO|H5FD_LOG_ALLOC;
                    char *logname = "silo_hdf5_log.out";

                    if ((p = DBGetOption(opts, DBOPT_H5_LOG_NAME)))
                        logname = (char*) p;

                    if ((p = DBGetOption(opts, DBOPT_H5_LOG_BUF_SIZE)))
                    {
                        bufsize = *((int*) p);
                        flags = H5FD_LOG_ALL;
                    }

                    h5status |= H5Pset_fapl_log(retval, logname, flags, bufsize);
                    break;
                }
                case DB_H5VFD_CORE:
                case DB_H5VFD_FIC:
                {
                    int inc = 1<<20; /* default of 1 Meg */
                    hbool_t bs = TRUE; /* default back store on */

                    /* get core allocation inc */
                    if ((p = DBGetOption(opts, DBOPT_H5_CORE_ALLOC_INC)))
                        inc = *((int*)p);

                    /* get backing store flag */
                    if ((p = DBGetOption(opts, DBOPT_H5_CORE_NO_BACK_STORE)))
                        bs = *((int*)p) ? FALSE : TRUE;

                    h5status |= H5Pset_fapl_core(retval, inc, bs);

                    /* Set up the file image too */
                    if (vfd == DB_H5VFD_FIC)
                    {
#if HDF5_VERSION_GE(1,8,9)
                        H5FD_file_image_callbacks_t callbacks = {&image_malloc, &image_memcpy,
                                                                 &image_realloc, &image_free,
                                                                 &udata_copy, &udata_free,
                                                                 (void *)NULL /* overwritten below */};
                        db_hdf5_H5LT_file_image_ud_t *udata;

                        /* no possible default values can be specified for FIC */
                        unsigned h5flags = 0x0;
                        int flags = 0x0, size = -1;
                        void *buf = 0;

                        /* cannot use backing store in this case */
                        h5status |= H5Pset_fapl_core(retval, inc, FALSE);

                        /* get file image size */
                        if ((p = DBGetOption(opts, DBOPT_H5_FIC_SIZE)))
                            size = *((int*)p);
                        else
                        {
                            H5Pclose(retval);
                            return db_perror("To use DB_H5VFD_FIC, you must specify DBOPT_H5_FIC_SIZE", E_BADARGS, me);
                        }

                        /* get file image buffer pointer */
                        if ((p = DBGetOption(opts, DBOPT_H5_FIC_BUF)))
                            buf = (void*)p;
                        else
                        {
                            H5Pclose(retval);
                            return db_perror("To use DB_H5VFD_FIC, you must specify DBOPT_H5_FIC_BUF", E_BADARGS, me);
                        }

                        /* get optional flags */
                        if ((p = DBGetOption(opts, DBOPT_H5_FIC_FLAGS)))
                        {
                            flags = *((int*)p);
                            if (flags & DB_H5_FIC_DONT_COPY)
                                h5flags |= db_hdf5_H5LT_FILE_IMAGE_DONT_COPY;
                            if (flags & DB_H5_FIC_DONT_RELEASE)
                                h5flags |= db_hdf5_H5LT_FILE_IMAGE_DONT_RELEASE;
                        }
                        if (mode != DB_READ)
                            h5flags |= db_hdf5_H5LT_FILE_IMAGE_OPEN_RW;

                        /* Set callbacks for file image ops ONLY if the file image is NOT copied */
                        if (flags & db_hdf5_H5LT_FILE_IMAGE_DONT_COPY)
                        {

                            /* Allocate buffer to communicate user data to callbacks */
                            if (NULL == (udata = (db_hdf5_H5LT_file_image_ud_t *)malloc(sizeof(db_hdf5_H5LT_file_image_ud_t))))
                            {
                                H5Pclose(retval);
                                return db_perror("Unable to allocate udata for FIC VFD", E_CALLFAIL, me);
                            }

                            /* Initialize udata with info about app buffer containing file image and flags */
                            udata->app_image_ptr   = buf;
                            udata->app_image_size  = size;
                            udata->fapl_image_ptr  = NULL;
                            udata->fapl_image_size = 0;
                            udata->fapl_ref_count  = 0;
                            udata->vfd_image_ptr   = NULL;
                            udata->vfd_image_size  = 0;
                            udata->vfd_ref_count   = 0;
                            udata->flags           = flags;

                            /*
                             * Initialize the udata structure with a reference count of 1. At
                             * first, nothing holds this reference to the udata structure. The
                             * call to H5Pset_file_image_callbacks below will associate the
                             * udata structure with the FAPL, incrementing the structure's
                             * reference count and causing the FAPL to hold one of the two
                             * references to the structure in preparation for transfer of
                             * ownership to the file driver. Once the file has been opened with
                             * this FAPL and the FAPL is closed, the reference held by the FAPL
                             * is released and ownership is transferred to the file driver, which
                             * will then hold the remaining reference to the udata structure.
                             * The udata structure will then be freed when the file driver calls
                             * the image_free callback and releases its reference to the structure.
                             */
                            udata->ref_count = 1;

                            /* copy address of udata into callbacks */
                            callbacks.udata = (void *)udata;

                            /* Set file image callbacks */
                            h5status |= H5Pset_file_image_callbacks(retval, &callbacks);

                        }

                        /* Assign file image in user buffer to FAPL */
                        h5status |= H5Pset_file_image(retval, buf, (size_t)size);
#else
                        H5Pclose(retval);
                        return db_perror("DB_H5VFD_FIC >= HDF5 1.8.9", E_NOTENABLEDINBUILD, me);
#endif
                    }

                    break;
                }
                case DB_H5VFD_DIRECT:
                {
#ifdef H5_HAVE_DIRECT
                    int direct_block_size = (1<<12); /* 4 kilobytes (lustre default) */
                    int direct_alignment, direct_cbuf_size;
                    int align_min, align_val;
    
                    /* get direct block size */ 
                    if ((p = DBGetOption(opts, DBOPT_H5_DIRECT_BLOCK_SIZE)))
                        direct_block_size = *((int*)p);

                    /* get direct alignment */ 
                    if ((p = DBGetOption(opts, DBOPT_H5_DIRECT_MEM_ALIGN)))
                        direct_alignment = *((int*)p);
                    else
                        direct_alignment = direct_block_size;

                    /* get direct buffer size */ 
                    if ((p = DBGetOption(opts, DBOPT_H5_DIRECT_BUF_SIZE)))
                        direct_cbuf_size = *((int*)p);
                    else
                        direct_cbuf_size = direct_block_size * 256;

                    /* get overall alignment threshold */ 
                    if ((p = DBGetOption(opts, DBOPT_H5_ALIGN_MIN)))
                        align_min = *((int*)p);
                    else
                        align_min = direct_block_size / 2;

                    /* get overall alignment value */ 
                    if ((p = DBGetOption(opts, DBOPT_H5_ALIGN_VAL)))
                        align_val = *((int*)p);
                    else
                        align_val = direct_block_size;

                    h5status |= H5Pset_fapl_direct(retval, direct_alignment, direct_block_size, direct_cbuf_size);
                    h5status |= H5Pset_alignment(retval, align_min, align_val); 
#else
                    H5Pclose(retval);
                    return db_perror("HDF5 Direct VFD", E_NOTENABLEDINBUILD, me);
#endif
                    break;

                }
                case DB_H5VFD_SPLIT: 
                {
                    int meta_opts_set_id = DB_FILE_OPTS_H5_DEFAULT_CORE;
                    int raw_opts_set_id = DB_FILE_OPTS_H5_DEFAULT_DEFAULT;
                    char *mext = "", *rext = "-raw";
                    hid_t meta_fapl = -1, raw_fapl = -1;

                    /* get meta opts_set_id */
                    if ((p = DBGetOption(opts, DBOPT_H5_META_FILE_OPTS)))
                        meta_opts_set_id = *((int*)p);

                    /* get meta fapl from opts_set_id */
                    meta_fapl = db_hdf5_process_file_options(meta_opts_set_id, mode, 0);

                    /* get meta extension */
                    if ((p = DBGetOption(opts, DBOPT_H5_META_EXTENSION)))
                        mext = (char *) p;

                    /* get raw opts_set_id */
                    if ((p = DBGetOption(opts, DBOPT_H5_RAW_FILE_OPTS)))
                        raw_opts_set_id = *((int*)p);

                    /* get raw fapl from opts_set_id */
                    raw_fapl = db_hdf5_process_file_options(raw_opts_set_id, mode, 0);

                    /* get raw extension */
                    if ((p = DBGetOption(opts, DBOPT_H5_RAW_EXTENSION)))
                        rext = (char *) p;

                    /* make sure the exentions are not identical */
                    if (!strcmp(mext, rext))
                    {
                        H5Pclose(retval);
                        return db_perror("meta & raw extensions must be different", E_CALLFAIL, me);
                    }

                    h5status |= H5Pset_fapl_split(retval, mext, meta_fapl, rext, raw_fapl);
                    H5Pclose(meta_fapl);
                    H5Pclose(raw_fapl);
                    break;
                }
                case DB_H5VFD_MPIO:
                case DB_H5VFD_MPIP:
                {
#ifdef H5_HAVE_PARALLEL
                    MPI_Comm mpi_comm = MPI_COMM_SELF;
                    MPI_Info mpi_info = MPI_INFO_NULL; 
                    hbool_t use_gpfs_hints = TRUE;
                    int flag = 0;

                    /* get the communicator */
                    if ((p = DBGetOption(opts, DBOPT_H5_MPIO_COMM)))
                        mpi_comm = *((MPI_Comm *)p);

                    /* get the info */
                    if ((p = DBGetOption(opts, DBOPT_H5_MPIO_INFO)))
                        mpi_info = *((MPI_Info *)p);

                    /* get use_gpfs_hints flag */
                    if ((p = DBGetOption(opts, DBOPT_H5_MPIP_NO_GPFS_HINTS)))
                        use_gpfs_hints = FALSE;

                    if (vfd == DB_H5VFD_MPIO)
                    {
                        h5status |= H5Pset_fapl_mpio(retval, mpi_comm, mpi_info);
                    }
                    else
                    {
#ifndef _MSC_VER
#warning DO WE STILL NEED THIS
#endif
                        H5Pclose(retval);
                        return db_perror("HDF5 MPIPOSIX VFD not available in >HDF5-1.8.12", E_NOTENABLEDINBUILD, me);
                    }
#else 
                    H5Pclose(retval);
                    return db_perror("HDF5 MPI VFD", E_NOTENABLEDINBUILD, me);
#endif
                    break;
                }
                case DB_H5VFD_FAMILY:
                {
                    int memb_size = (1<<30); /* 1 gigabyte */
                    int memb_opts_set_id = DB_FILE_OPTS_H5_DEFAULT_DEFAULT;
                    hid_t memb_fapl;

                    /* get size of files in family */
                    if ((p = DBGetOption(opts, DBOPT_H5_FAM_SIZE)))
                        memb_size = *((int*)p);

                    /* get underlying family fapl */
                    if ((p = DBGetOption(opts, DBOPT_H5_FAM_FILE_OPTS)))
                        memb_opts_set_id = *((int*)p);

                    memb_fapl = db_hdf5_process_file_options(memb_opts_set_id, mode, 0);
                    h5status |= H5Pset_fapl_family(retval, memb_size, memb_fapl);
                    H5Pclose(memb_fapl);
                }
            }

            /* handle overall alignment requests */
            if ((p = DBGetOption(opts, DBOPT_H5_ALIGN_MIN)))
            {
                int align_min = *((int*)p);
                int align_val = align_min * 2;

                /* get overall alignment value */ 
                if ((p = DBGetOption(opts, DBOPT_H5_ALIGN_VAL)))
                    align_val = *((int*)p);

                h5status |= H5Pset_alignment(retval, align_min, align_val); 
            }

            /* handle meta block size */
            if ((p = DBGetOption(opts, DBOPT_H5_META_BLOCK_SIZE)))
            {
                int size = *((int*)p);
                h5status |= H5Pset_meta_block_size(retval, size);
            }

            /* handle raw block size */
            if ((p = DBGetOption(opts, DBOPT_H5_SMALL_RAW_SIZE)))
            {
                int size = *((int*)p);
                h5status |= H5Pset_small_data_block_size(retval, size);
            }

            /* handle sieve buffer size */
            if ((p = DBGetOption(opts, DBOPT_H5_SIEVE_BUF_SIZE)))
            {
                int size = *((int*)p);
                h5status |= H5Pset_sieve_buf_size(retval, size);
            }

            /* handle cache settings */
            if ((p = DBGetOption(opts, DBOPT_H5_CACHE_NELMTS)))
            {
                int nelmts = *((int*)p);
                int nbytes = nelmts * sizeof(double);
                double policy = 1.0;

                /* get size in bytes */
                if ((p = DBGetOption(opts, DBOPT_H5_CACHE_NBYTES)))
                    nbytes = *((int*)p);

                /* get pre-emption policy */
                if ((p = DBGetOption(opts, DBOPT_H5_CACHE_POLICY)))
                    policy = *((double*)p);

                h5status |= H5Pset_cache(retval, 0, nelmts, nbytes, policy);
            }
        }
    }

#ifndef _MSC_VER
#warning FIX THIS
#endif
#if 0
    /* Handle cases where we are running on Windows. We need to ensure
       that fapl VFD is windows. So, we re-set fapl here if running on windows.
       The intent is that all other fapl properties that are not specific to
       the VFD will pass through ok but that anything that winds up setting
       non-windows VFD on windows is reverted here. This has the effect of
       dis-allowing things like core, split or family VFDs on windows which
       might otherwise work there fine. All of this is in the hopes of
       resolving file locking issues on Windows where a code like Ale3d
       creates a Silo file there and then VisIt cannot open that file until
       Ale3d exits. */
#if defined(_WIN32)
    h5status |= H5Pset_fapl_windows(retval);
#endif
#endif

    if (h5status < 0)
    {
        H5Pclose(retval);
        return db_perror("Problem setting HDF5 VFD options", E_CALLFAIL, me);
    }

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_file_accprops 
 *
 * Purpose:     Create file access property lists 
 *
 * Programmer:  Mark C. Miller, Aug 1, 2006 
 *
 * Notes: The least significant 4 bits in the 'driver' argument passed to
 * an open or create call are stripped off and the value shifted 4 to the
 * right to produce the 'subtype' that is passed into driver's open or
 * create implementations. So, the 'subtype' arriving here is a 28 bit
 * number.
 *
 * Modifications:
 *   Mark C. Miller, Thu Feb 11 09:38:32 PST 2010
 *   Added support for split vfd. Changed how allocation inc for core
 *   and split vfd's is handled. Changed int values in case to symbolics.
 *-------------------------------------------------------------------------
 */
PRIVATE hid_t 
db_hdf5_file_accprops(int subtype, int mode, hid_t *fcpl)
{
    int opts_set_id = subtype & 0x0000003F;
    return db_hdf5_process_file_options(opts_set_id, mode, fcpl);
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_finish_open
 *
 * Purpose:     Completes opening of a Silo/HDF5 file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Friday, March 26, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Feb  3 09:48:23 PST 2009
 *   Changed dbfile arg from public DBfile* to private DBfile_hdf5*
 *   Changed return type from int to DBfile*
 *   Removed PROTECT/UNWIND/END_PROTECT and replaced UNWIND() calls with
 *   return silo_db_close(). This is because UNWIND was causing it to
 *   NOT correctly handle the case in which the given filename was NOT
 *   an HDF5 file and properly RETURNing NULL when necessary.
 *-------------------------------------------------------------------------
 */
PRIVATE DBfile* 
db_hdf5_finish_open(DBfile_hdf5 *dbfile)
{
    static char *me = "db_hdf5_finish_open";
    hid_t       cwg=-1, link=-1, attr=-1;
    int         tmp, target=DB_LOCAL;
    
    /* Open "/" as current working group */
    if ((cwg=H5Gopen(dbfile->fid, "/", H5P_DEFAULT))<0) {
        H5Fclose(dbfile->fid);
        silo_db_close((DBfile*) dbfile);
        db_perror("root group", E_CALLFAIL, me);
        return 0;
    }

    /*
     * Open the link directory. If it doesn't exist then create one (it
     * might not exist in old SAMI files).
     */
    H5E_BEGIN_TRY {
        link = H5Gopen(dbfile->fid, LINKGRP, H5P_DEFAULT);
    } H5E_END_TRY;
    if (link<0 && (link=H5Gcreate(dbfile->fid, LINKGRP, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT))<0) {
        H5Fclose(dbfile->fid);
        silo_db_close((DBfile*) dbfile);
        db_perror("link group", E_CALLFAIL, me);
        return 0;
    }

    /*
     * Read the targetting information from the `target' attribute
     * of the link group if there is one, otherwise assume DB_LOCAL
     */
    H5E_BEGIN_TRY {
        attr = H5Aopen_name(link, "target");
    } H5E_END_TRY;
    if (attr>=0 &&
        H5Aread(attr, H5T_NATIVE_INT, &tmp)>=0 &&
        H5Aclose(attr)>=0) {
        target = tmp;
    }

    /*
     * Initialize the file struct. Use the same target architecture that
     * was specified when the file was created.
     */
    dbfile->cwg = cwg;
    dbfile->link = link;
    db_hdf5_InitCallbacks(dbfile, target);
        
    return (DBfile*) dbfile;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_finish_create
 *
 * Purpose:     Finish creating a file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Friday, March 26, 1999
 *
 * Modifications:
 *   Robb Matzke, 1999-08-17
 *   The file information string is written as a variable in the
 *   file instead of as a comment on the root group.
 *
 *   Mark C. Miller, Tue Feb  3 09:48:23 PST 2009
 *   Changed dbfile arg from public DBfile* to private DBfile_hdf5*
 *   Changed return type from int to DBfile*
 *   Removed PROTECT/UNWIND/END_PROTECT and replaced UNWIND() calls with
 *   return silo_db_close(). This is because UNWIND was causing it to
 *   NOT correctly handle the case in which the given filename was NOT
 *   an HDF5 file and properly RETURNing NULL when necessary.
 *
 *   Mark C. Miller, Mon Jun 28 20:19:35 PDT 2010
 *   Added logic to handle HDF5 header/lib version numbers separately.
 *
 *   Mark C. Miller, Thu Aug 30 17:44:56 PDT 2012
 *   Added new, internal, db_hdf5_WriteCKZ method to enable caller to
 *   indicate if filters should be turned off. This is useful for disabling
 *   compression or checksuming for tiny metadata datasets such as hdf5
 *   library info.
 *-------------------------------------------------------------------------
 */
PRIVATE DBfile* 
db_hdf5_finish_create(DBfile_hdf5 *dbfile, int target, char const *finfo)
{
    static char *me = "db_hdf5_finish_create";
    hid_t       attr=-1;
    int         size;
    char        hdf5VString[64];
    unsigned    majno, minno, relno;
    const       int nofilters = 1;
    
    /* Open root group as CWG */
    if ((dbfile->cwg=H5Gopen(dbfile->fid, "/", H5P_DEFAULT))<0) {
        db_perror("root group", E_CALLFAIL, me);
        return silo_db_close((DBfile*) dbfile);
    }

    /* Create the link group */
    if ((dbfile->link=H5Gcreate(dbfile->fid, LINKGRP, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT))<0) {
        db_perror("link group", E_CALLFAIL, me);
        return silo_db_close((DBfile*) dbfile);
    }

    /* Callbacks */
    db_hdf5_InitCallbacks(dbfile, target);
        
    /*
     * Write the target architecture into the `target' attribute of the
     * link group so we can retrieve it later when the file is reopened.
     */
    if ((attr=H5Acreate(dbfile->link, "target", dbfile->T_int, SCALAR,
                        H5P_DEFAULT, H5P_DEFAULT))<0 ||
        H5Awrite(attr, H5T_NATIVE_INT, &target)<0 ||
        H5Aclose(attr)<0) {
        db_perror("targetinfo", E_CALLFAIL, me);
        return silo_db_close((DBfile*) dbfile);
    }

    if (finfo) {
        /* Write file info as a variable in the file */
        size = strlen(finfo)+1;
        if (db_hdf5_WriteCKZ((DBfile*)dbfile, "_fileinfo", (void*)finfo, &size, 1, DB_CHAR, nofilters)<0) {
            db_perror("fileinfo", E_CALLFAIL, me);
            return silo_db_close((DBfile*) dbfile);
        }
    }

    /*
     * Write HDF5 library version information to the file 
     */
    H5get_libversion(&majno, &minno, &relno);
    if (majno != H5_VERS_MAJOR || minno != H5_VERS_MINOR || relno != H5_VERS_RELEASE)
    {
        /* Since headers and libs don't match, write information about headers first */
        sprintf(hdf5VString, "hdf5-%d.%d.%d%s%s", H5_VERS_MAJOR, H5_VERS_MINOR, H5_VERS_RELEASE,
            strlen(H5_VERS_SUBRELEASE) ? "-" : "", H5_VERS_SUBRELEASE);
        size = strlen(hdf5VString)+1;
        if (db_hdf5_WriteCKZ((DBfile*)dbfile, "_hdf5incinfo", hdf5VString, &size, 1, DB_CHAR, nofilters)<0) {
            db_perror("_hdf5incinfo", E_CALLFAIL, me);
            return silo_db_close((DBfile*) dbfile);
        }
    }

    sprintf(hdf5VString, "hdf5-%d.%d.%d", majno, minno, relno);
    size = strlen(hdf5VString)+1;
    if (db_hdf5_WriteCKZ((DBfile*)dbfile, "_hdf5libinfo", hdf5VString, &size, 1, DB_CHAR, nofilters)<0) {
        db_perror("_hdf5libinfo", E_CALLFAIL, me);
        return silo_db_close((DBfile*) dbfile);
    }

    return (DBfile*) dbfile;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_initiate_close
 *
 * Purpose:     Start closing the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Friday, March 26, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Wed Jul 14 20:53:33 PDT 2010
 *   Added logic to check for open objects and try to print their names.
 *-------------------------------------------------------------------------
 */
PRIVATE int
db_hdf5_initiate_close(DBfile *_dbfile)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_initiate_close";
    int i;

    /* Close all datasets in the circular buffer */
    for (i=0; i<NDSETTAB; i++) {
        FREE(dbfile->dsettab[i]);
        dbfile->dsettab[i] = NULL;
    }
    dbfile->dsettab_ins = dbfile->dsettab_rem = 0;
    
    /* Close current working group and link group */
    if (H5Gclose(dbfile->cwg)<0 || H5Gclose(dbfile->link)<0) {
        return db_perror("closing", E_CALLFAIL, me);
    }
    dbfile->cwg = -1;
    dbfile->link = -1;

    if (dbfile->cwg_name)
        free(dbfile->cwg_name);
    dbfile->cwg_name = NULL;

    /* Check for any open objects in this file */
#if HDF5_VERSION_GE(1,6,0)
    if (SILO_Globals._db_err_level_drvr == DB_ALL)
    {
        const unsigned int obj_flags = H5F_OBJ_LOCAL | H5F_OBJ_DATASET |
            H5F_OBJ_GROUP | H5F_OBJ_DATATYPE | H5F_OBJ_ATTR;

        int noo = H5Fget_obj_count(dbfile->fid, obj_flags);
        if (noo > 0)
        {
            int n;
            char msg[4096];
            hid_t *ooids = (hid_t *) malloc(noo * sizeof(hid_t));
            sprintf(msg, "Internal Silo error: %d objects left open in file: ", noo);
#if HDF5_VERSION_GE(1,6,5)
            H5Fget_obj_ids(dbfile->fid, obj_flags, noo, ooids);
#else
            H5Fget_obj_ids(dbfile->fid, obj_flags, noo, ooids);
#endif
            n = strlen(msg);
            for (i = 0; i < noo && n < sizeof(msg); i++)
            {
                char name[256], tmp[256];
                H5Iget_name(ooids[i], name, sizeof(name));
                sprintf(tmp, "\"%.235s\" (id=%llu), ", name, (unsigned long long) ooids[i]);
                if ((strlen(msg) + strlen(tmp) + 1) >= sizeof(msg))
                    break;
                strcat(msg, tmp);
                n += strlen(tmp);
            }
            free(ooids);

            return db_perror(dbfile->pub.name, E_CALLFAIL, msg);
        }
    }
#endif

    // Tell HDF5 to garbage collect (whatever it can)
    H5garbage_collect();

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_Open
 *
 * Purpose:     Opens an hdf5 file that already exists.
 *
 * Return:      Success:        Ptr to the file struct
 *
 *              Failure:        NULL, db_errno set.
 *
 * Programmer:  Robb Matzke
 *              Tuesday, February  9, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Feb  1 18:13:28 PST 2005
 *   Added call to H5Eset_auto(). Open can be called outside of init
 *
 *   Mark C. Miller, Tue Feb  3 09:48:23 PST 2009
 *   Removed PROTECT/UNWIND/END_PROTECT and replaced UNWIND() calls with
 *   return silo_db_close(). This is because UNWIND was causing it to
 *   NOT correctly handle the case in which the given filename was NOT
 *   an HDF5 file and properly RETURNing NULL when necessary.
 *
 *   Mark C. Miller, Wed Feb 25 09:37:10 PST 2009
 *   Changed error code for failure to open to indicate better error
 *   message.
 *
 *   Mark C. Miller, Thu Feb 11 09:37:41 PST 2010
 *   Added logic to set HDF5's error output based on Silo's settings.
 *-------------------------------------------------------------------------
 */
INTERNAL DBfile *
db_hdf5_Open(char const *name, int mode, int opts_set_id)
{
    DBfile_hdf5 *dbfile=NULL;
    hid_t       fid=-1, faprops=-1;
    hid_t      *fidp = 0;
    unsigned    hmode;
    static char *me = "db_hdf5_Open";

    if (SILO_Globals._db_err_level_drvr == DB_ALL)
        H5Eset_auto(H5E_DEFAULT, (H5E_auto2_t) H5Eprint2, stderr);
    else
        H5Eset_auto(H5E_DEFAULT, NULL, NULL);

    /* File access mode */
    if (DB_READ==(mode & 0x00000003)) {
        hmode = H5F_ACC_RDONLY;
    } else if (DB_APPEND==(mode & 0x00000003)) {
        hmode = H5F_ACC_RDWR;
    } else {
        db_perror("mode", E_INTERNAL, me);
        return NULL;
    }

#if HDF5_VERSION_GE(1,10,0)
    if (DB_CONCURRENT==(mode & 0x00000004))
        hmode |= H5F_ACC_SWMR_READ;
#endif

    /* use mode to pass compatibility option into
       db_hdf5_file_accprops */
    if (((mode & 0x00000003) == DB_APPEND))
    {
        if (SILO_Globals.compatibilityMode == DB_PERF_OVER_COMPAT)
            mode |= DB_PERF_OVER_COMPAT;
        if (SILO_Globals.compatibilityMode == DB_COMPAT_OVER_PERF)
            mode |= DB_COMPAT_OVER_PERF;
    }

    faprops = db_hdf5_file_accprops(opts_set_id, mode, 0);
#ifndef _MSC_VER
#warning QUERY FILE IMAGE STUFF HERE TO GET UDATA PTR
#endif

    /* Open existing hdf5 file */
    if ((fid=H5Fopen(name, hmode, faprops))<0) {
        H5Pclose(faprops);
        db_perror(name, E_DRVRCANTOPEN, me);
        return NULL;
    }

#if HDF5_VERSION_GE(1,10,0)
    if (DB_APPEND == (mode & 0x00000003))
    {
        if (SILO_Globals.compatibilityMode == DB_PERF_OVER_COMPAT)
        {
            if (H5Fset_libver_bounds(fid, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST)<0)
            {
                H5Fclose(fid);
                db_perror(name, E_CALLFAIL, me);
                return NULL;
            }
            if (DB_CONCURRENT==(mode & 0x00000004))
                H5Fstart_swmr_write(fid);
        }
        else if (SILO_Globals.compatibilityMode == DB_COMPAT_OVER_PERF)
        {
            if (H5Fset_libver_bounds(fid, H5F_LIBVER_V18, H5F_LIBVER_V18)<0)
            {
                H5Fclose(fid);
                db_perror(name, E_CALLFAIL, me);
                return NULL;
            }
        }
    }
#endif

    H5Pclose(faprops);

    /* Create silo file struct */
    if (NULL==(dbfile=(DBfile_hdf5*)calloc(1, sizeof(DBfile_hdf5)))) {
        db_perror(name, E_NOMEM, me);
        return NULL;
    }
    dbfile->pub.name = STRDUP(name);
    dbfile->pub.type = DB_HDF5;
    dbfile->pub.Grab = FALSE;
    fidp = (hid_t *) malloc(sizeof(hid_t));
    *fidp = fid;
    dbfile->pub.GrabId = (void*) fidp;
    dbfile->fid = fid;
    return db_hdf5_finish_open(dbfile);
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_Create
 *
 * Purpose:     Creates an hdf5 file and begins the process of writing mesh
 *              and mesh-related data into that file.
 *
 *              The `target' is always assumed to be the local architecture.
 *
 * Return:      Success:        Pointer to a new file
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, February  9, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Feb  1 18:13:28 PST 2005
 *   Added call to H5Eset_auto(). Create can be called outside of init
 *
 *   Mark C. Miller, Tue Feb  3 09:48:23 PST 2009
 *   Removed PROTECT/UNWIND/END_PROTECT and replaced UNWIND() calls with
 *   return silo_db_close(). This is because UNWIND was causing it to
 *   NOT correctly handle the case in which the given filename was NOT
 *   an HDF5 file and properly RETURNing NULL when necessary.
 *
 *   Mark C. Miller, Thu Feb 11 09:37:41 PST 2010
 *   Added logic to set HDF5's error output based on Silo's settings.
 *-------------------------------------------------------------------------
 */
INTERNAL DBfile *
db_hdf5_Create(char const *name, int mode, int target, int opts_set_id, char const *finfo)
{
    DBfile_hdf5 *dbfile=NULL;
    hid_t       fid=-1, faprops=-1, fcprops=-1;
    hid_t      *fidp = 0;
    static char *me = "db_hdf5_Create";

    /* Turn off error messages from the hdf5 library */
    if (SILO_Globals._db_err_level_drvr == DB_ALL)
        H5Eset_auto(H5E_DEFAULT, (H5E_auto2_t) H5Eprint2, stderr);
    else
        H5Eset_auto(H5E_DEFAULT, NULL, NULL);

    /* use mode to pass compatibility option into
       db_hdf5_file_accprops */
    if (SILO_Globals.compatibilityMode == DB_PERF_OVER_COMPAT)
        mode |= DB_PERF_OVER_COMPAT;
    if (SILO_Globals.compatibilityMode == DB_COMPAT_OVER_PERF)
        mode |= DB_COMPAT_OVER_PERF;

    faprops = db_hdf5_file_accprops(opts_set_id, mode, &fcprops);

        /* Create or open hdf5 file */
    if (DB_CLOBBER==(mode & 0x00000003)) {
        /* If we ever use checksumming (which requires chunked datasets),
         * HDF5's BTree's will effect storage overhead. Since Silo really
         * doesn't support growing/shrinking datasets, we just use a value
         * of '1' for istore_k */
        int created_fcprops = 0;
        if (fcprops == -1)
        {
            fcprops = H5Pcreate(H5P_FILE_CREATE);
#ifndef _MSC_VER
#warning BACKWARD COMPAT ISSUE FOR HDF5
#endif
            /*H5Pset_istore_k(fcprops, 1);*/
            created_fcprops = 1;
        }
        fid = H5Fcreate(name, H5F_ACC_TRUNC, fcprops, faprops);
        if (created_fcprops)
            H5Pclose(fcprops);
        H5Glink(fid, H5G_LINK_HARD, "/", ".."); /*don't care if fails*/
    } else if (DB_NOCLOBBER==(mode & 0x00000003)) {
        fid = H5Fopen(name, H5F_ACC_RDWR, faprops);
    } else {
        H5Pclose(faprops);
        db_perror("mode", E_BADARGS, me);
        return NULL;
    }
    if (fid<0) {
        H5Pclose(faprops);
        db_perror(name, E_NOFILE, me);
        return NULL;
    }

    H5Pclose(faprops);

#if HDF5_VERSION_GE(1,10,0)
    if (SILO_Globals.compatibilityMode == DB_PERF_OVER_COMPAT)
    {
        if (H5Fset_libver_bounds(fid, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST)<0)
        {
            H5Fclose(fid);
            db_perror(name, E_CALLFAIL, me);
            return NULL;
        }
        if (DB_CONCURRENT==(mode & 0x00000004))
            H5Fstart_swmr_write(fid);
    }
    else if (SILO_Globals.compatibilityMode == DB_COMPAT_OVER_PERF)
    {
        if (H5Fset_libver_bounds(fid, H5F_LIBVER_V18, H5F_LIBVER_V18)<0)
        {
            H5Fclose(fid);
            db_perror(name, E_CALLFAIL, me);
            return NULL;
        }
    }
#endif

    /* Create silo file struct */
    if (NULL==(dbfile=(DBfile_hdf5 *)calloc(1, sizeof(DBfile_hdf5)))) {
        db_perror(name, E_NOMEM, me);
        return NULL;
    }
    dbfile->pub.name = STRDUP(name);
    dbfile->pub.type = DB_HDF5;
    dbfile->pub.Grab = FALSE;
    fidp = (hid_t *) malloc(sizeof(hid_t));
    *fidp = fid;
    dbfile->pub.GrabId = (void*) fidp;
    dbfile->fid = fid;
#ifndef _MSC_VER
#warning FIX FILE SCOPE GLOBAL INITIALIZATION
#endif
#if 0
    *(dbfile->pub.file_scope_globals) = SILO_Globals;
#endif
    return db_hdf5_finish_create(dbfile, target, finfo);
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_Close
 *
 * Purpose:     Closes an hdf5 file and frees memory associated with the
 *              file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *   Mark C. Miller, Thu Jul 17 15:05:16 PDT 2008
 *   Added call to FreeNodelists for this file.
 *
 *   Mark C. Miller, Tue Feb  3 09:52:51 PST 2009
 *   Moved code to free pub.GrabId and set Grab related entries to zero to
 *   silo_db_close() function and then added a call to that function here.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_Close(DBfile *_dbfile)
{
    int retval = 0;
    DBfile_hdf5    *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_Close";

    if (dbfile) {

        PROTECT {
            H5FD_file_image_callbacks_t callbacks = {0, 0, 0, 0, 0, 0, 0};
            hid_t faplid = H5Fget_access_plist(dbfile->fid);
 
            if (H5I_INVALID_HID != faplid &&
                H5Pget_file_image_callbacks(faplid, &callbacks)>=0)
            {
                H5Pclose(faplid);
                if (callbacks.udata)
                    free(callbacks.udata);
            }

            /* Free the private parts of the file */
            if (db_hdf5_initiate_close((DBfile*)dbfile)<0 ||
                H5Fclose(dbfile->fid)<0)
            {
                db_perror(dbfile->pub.name, E_CALLFAIL, me);
                UNWIND();
            }
            dbfile->fid = -1;

            FreeNodelists(dbfile, 0);

            /* Free the public parts of the file */
            silo_db_close(_dbfile);
        
        } CLEANUP {
            retval = -1;
            dbfile->fid = -1;
            silo_db_close(_dbfile);
        } END_PROTECT;
    }

    /* work-around leaks in HDF5, version 1.8.4 only */
#if HDF5_VERSION_GE(1,8,4) && !HDF5_VERSION_GE(1,8,5)
    if (db_num_registered_files() == 0)
    {
        T_str_stype_set = 0;
        H5close();
    }
#endif

    /* free circular buffer stuff */
    db_hdf5_resolvename(0,0,0);

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_Flush
 *
 * Purpose:     Flushes an HDF5 file
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer: Mark C. Miller, Fri Aug 14 11:49:05 PDT 2015
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_Flush(DBfile *_dbfile)
{
    int retval = -1;
    DBfile_hdf5    *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_Flush";

    if (!dbfile)
        return retval;

    PROTECT {
        if (H5Fflush(dbfile->fid, H5F_SCOPE_LOCAL)>=0)
            retval = 0;
    } CLEANUP {
    } END_PROTECT;

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_Filters
 *
 * Purpose:     Output the name of this device driver to the specified stream.
 *
 * Return:      Success:        0
 *
 *              Failure:        never fails
 *
 * Programmer:  Robb Matzke
 *              Wednesday, March 17, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
SILO_CALLBACK int
db_hdf5_Filters(DBfile *_dbfile, FILE *stream)
{
    fprintf(stream, "HDF5 Device Driver\n");
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_MkDir
 *
 * Purpose:     Create a new directory.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Wednesday, February 10, 1999
 *
 * Modifications:
 *     Robb Matzke, 1999-10-13
 *     Uses the current working group instead of the root group when
 *     adding the `..' entry.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_MkDir(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_MkDir";
    char        *dotdot = NULL,  *parent=NULL, *t=NULL;
    hid_t       grp = -1;

    PROTECT {

        /* Create the new group */
        if ((grp=H5Gcreate(dbfile->cwg, name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT))<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* What is the name of the parent directory of the new directory? */
        parent = STRDUP(name);
        t = parent+strlen(parent);
        while (t>parent && '/'==t[-1]) *(--t) = '\0';   /*trailing slashes*/
        while (t>parent && '/'!=t[-1]) *(--t) = '\0';   /*last component*/
        if (!*parent) strcpy(parent, '/'==*name?"/":".");

        /* What is the name of the `..' entry? */
        dotdot = (char *)malloc(strlen(name)+4);
        strcpy(dotdot, name);
        strcat(dotdot, "/..");

        /* Make the `..' point to the parent */
        if (H5Glink(dbfile->cwg, H5G_LINK_HARD, parent, dotdot)<0) {
            db_perror(dotdot, E_CALLFAIL, me);
            UNWIND();
        }

        /* Close everything */
        H5Gclose(grp);
        FREE(dotdot);
        FREE(parent);

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Gclose(grp);
        } H5E_END_TRY;
        FREE(dotdot);
        FREE(parent);
    } END_PROTECT;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_SetDir
 *
 * Purpose:     Set the current working directory.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Wednesday, February 10, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Nov  5 10:55:46 PST 2009
 *   Added some 'hidden' logic to support view of contents of /.silo dir
 *   from browser. Its 'hidden' because I have no intention of letting
 *   users know about it.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_SetDir(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_SetDir";
    hid_t       newdir = -1;
    
    PROTECT {
        if ((newdir=H5Gopen(dbfile->cwg, name, H5P_DEFAULT))<0 ||
            (strcmp(name,"/.silo")!=0 &&
            H5Gget_objinfo(newdir, "..", FALSE, NULL)<0)) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }

        H5Gclose(dbfile->cwg);
        dbfile->cwg = newdir;

        if (dbfile->cwg_name) {
            char *new_cwg_name = db_absoluteOf_path(dbfile->cwg_name?dbfile->cwg_name:"/", name);
            free(dbfile->cwg_name);
            dbfile->cwg_name = new_cwg_name;
        }
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Gclose(newdir);
        } H5E_END_TRY;
    } END_PROTECT;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetDir
 *
 * Purpose:     Writes the absolute name of the current working directory
 *              into the NAME argument without checking for overflow (if NAME
 *              is not large enough then a core dump is likely).
 *
 *              The name is computed by following the `..' entries out of
 *              each directory and then looking for the entry in the parent
 *              that matches the object id of the current directory.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_GetDir(DBfile *_dbfile, char *name/*out*/)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_GetDir";
    hid_t       cwg = -1, parent = -1;
    H5G_stat_t  cur_sb, par_sb;
    int         i, ncomps=0;
    silo_hdf5_comp_t    comp[100];

    /* Return quickly if name is cached. But, confirm the name first. */
    if (dbfile->cwg_name) {
        H5G_stat_t sb;
        herr_t status;
        H5E_BEGIN_TRY {
            status = H5Gget_objinfo(dbfile->cwg, dbfile->cwg_name, TRUE, &sb);
        } H5E_END_TRY;
        if (status>=0)
        {
            strcpy(name, dbfile->cwg_name);
            return 0;
        }
    }
    
    memset(comp, 0, sizeof comp);
    PROTECT {
        cwg = H5Gopen(dbfile->cwg, ".", H5P_DEFAULT);
        if (H5Gget_objinfo(cwg, ".", TRUE, &cur_sb)<0) {
            db_perror("stat(\".\")", E_CALLFAIL, me);
            UNWIND();
        }

        while ((size_t)ncomps<NELMTS(comp)) {
            /*
             * Get info about parent. If parent object ID is the same as
             * current group object ID then we must be at the root.
             */
            if (H5Gget_objinfo(cwg, "..", TRUE, &par_sb)<0) {
                db_perror("stat(\"..\")", E_CALLFAIL, me);
                UNWIND();
            }
            if (cur_sb.objno[0]==par_sb.objno[0] &&
                cur_sb.objno[1]==par_sb.objno[1]) break;

            /*
             * Iterate over entries in parent to find first name that has the
             * same object ID as the current group and use that as the
             * component of the name
             */
            if ((parent=H5Gopen(cwg, "..", H5P_DEFAULT))<0) {
                db_perror("no `..' entry", E_NOTFOUND, me);
                UNWIND();
            }
            comp[ncomps].objno[0] = cur_sb.objno[0];
            comp[ncomps].objno[1] = cur_sb.objno[1];
            if (H5Giterate(parent, ".", NULL, find_objno, comp+ncomps)<=0) {
                db_perror("inconsistent directory structure", E_CALLFAIL, me);
                UNWIND();
            }

            /* Move upward in the directory try */
            H5Gclose(cwg);
            cwg = parent;
            parent = -1;
            cur_sb = par_sb;
            ncomps++;
        }

        /* Build the name */
        if (0==ncomps) {
            strcpy(name, "/");
        } else {
            name[0] = '\0';
            for (i=ncomps-1; i>=0; --i) {
                strcat(name, "/");
                strcat(name, comp[i].name);
                free(comp[i].name);
            }
        }

        /* Close everything */
        H5Gclose(cwg);
        if (parent>=0) H5Gclose(parent);

        /* Cache the name for later */
        if (dbfile->cwg_name)
            free(dbfile->cwg_name);
        dbfile->cwg_name = strdup(name);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Gclose(cwg);
            H5Gclose(parent);
        } H5E_END_TRY;
        for (i=0; i<=ncomps && (size_t)i<NELMTS(comp); i++) {
            if (comp->name) free(comp->name);
        }
    } END_PROTECT;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    copy_obj
 *
 * Purpose:     Support, recursive function, for CpDir using HDF5's
 *              H5Giterate method.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller, Wed Aug  6 18:29:53 PDT 2008
 *
 *-------------------------------------------------------------------------
 */
typedef struct copy_obj_data_t {
    DBfile     *dstFile;
    char const *dstName;
} copy_obj_data_t;

static herr_t 
copy_obj(hid_t hobj, char const *name, void *op_data)
{
    static char         *me = "copy_obj";
#if HDF5_VERSION_GE(1,8,0)
    H5G_stat_t          sb;
    hid_t               obj;
    int                 objtype = -1;
    copy_obj_data_t *cp_data = (copy_obj_data_t *)op_data;
    DBfile_hdf5 *dstfile = (DBfile_hdf5*)cp_data->dstFile;
    char const  *dstName = cp_data->dstName ? cp_data->dstName : name;

    if (H5Gget_objinfo(hobj, name, TRUE, &sb)<0) return -1;
    switch (sb.type) {
    case H5G_GROUP:
        /*
         * Any group which has a `..' entry is a silo directory. The `..'
         * names do not appear in the silo table of contents.
         */
        if (!strcmp(name, "..") || (obj=H5Gopen(hobj, name, H5P_DEFAULT))<0) break;
        H5E_BEGIN_TRY {
            if (H5Gget_objinfo(obj, "..", FALSE, NULL)>=0) objtype = DB_DIR;
        } H5E_END_TRY;
        H5Gclose(obj);

        if (objtype == DB_DIR)
        {
            db_hdf5_MkDir(cp_data->dstFile, (char*) name);
            db_hdf5_SetDir(cp_data->dstFile, (char*) name);

            /* recurse on the members of this group */
            H5Giterate(hobj, name, 0, copy_obj, op_data);

            db_hdf5_SetDir(cp_data->dstFile, "..");
        }

        break;

    case H5G_TYPE:
    {
        hid_t       o=-1, attr=-1, atype=-1, s1024=-1;
        char        *file_value=NULL, *mem_value=NULL, *bkg=NULL;
        DBObjectType objtype;
        int         _objtype, nmembs, i;
        DBobject    *obj=NULL;
        size_t      asize, nelmts, msize;

        /* Open the object as a named data type */
        if ((o=H5Topen(hobj, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }

        /* Open the `silo_type' attribute and read it */
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        objtype = (DBObjectType)_objtype;
        
        /*
         * Open the `silo' attribute (all silo objects have one), retrieve
         * its data type (a single-level H5T_COMPOUND type), and read the
         * attribute.
         */
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            (atype=H5Aget_type(attr))<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        asize = H5Tget_size(atype);
#ifndef _MSC_VER
#warning WHY THIS SIZE
#endif
        msize = MAX(asize, 3*1024);
        if (NULL==(file_value=(char *)malloc(asize)) ||
            NULL==(mem_value=(char *)malloc(msize)) ||
            NULL==(bkg=(char *)malloc(msize))) {
            db_perror(name, E_NOMEM, me);
            UNWIND();
        }
        if (H5Aread(attr, atype, file_value)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        nmembs = H5Tget_nmembers(atype);

        s1024 = H5Tcopy(H5T_C_S1);
        H5Tset_size(s1024, 1024);
        for (i=0; i<nmembs; i++) {
            int ndims, j;
            hsize_t memb_size[4];
            hid_t member_type = db_hdf5_get_cmemb(atype, i, &ndims, memb_size);
            
            if (H5Tget_class(member_type) == H5T_STRING)
            {
                /* build up an in-memory rep that is akin to a struct
                 * with just this one member */
                char *memname = H5Tget_member_name(atype, i);
                size_t offset = H5Tget_member_offset(atype, i);
                hid_t mtype = H5Tcreate(H5T_COMPOUND, msize);
                for (nelmts=1, j=0; j<ndims; j++) nelmts *= memb_size[j];
                db_hdf5_put_cmemb(mtype, memname, 0, ndims, memb_size, s1024);

                /* use hdf5's type conversion func to extract this one member's
                   data into 'mem_value' */
                memcpy(mem_value, file_value, H5Tget_size(atype));
                H5Tconvert(atype, mtype, 1, mem_value, bkg, H5P_DEFAULT);

                /* if its one of the special datasets, copy it and update
                   the attribute to refer to the copy in the new file */
                if (strncmp(mem_value, "/.silo/#", 8) == 0)
                {
                    /* get unique name for this dataset in dst file */
                    char cname[8];
                    db_hdf5_compname(dstfile, cname);

                    /* copy this dataset to /.silo dir in dst file */
                    H5Ocopy(hobj, mem_value, dstfile->link, cname, H5P_DEFAULT, H5P_DEFAULT);

                    /* update this attribute's entry with name for this dataset */
                    sprintf(file_value+offset, "%s%s", LINKGRP, cname);
                }
                else
                {
                    hid_t tid;
                    char *srcSubObjDirName = db_dirname(name);
                    char *srcSubObjAbsName = db_join_path(srcSubObjDirName, mem_value);
                    char *dstSubObjDirName = db_dirname(dstName);
                    char *dstSubObjAbsName = db_join_path(dstSubObjDirName, mem_value);
                    if (strcmp(srcSubObjDirName, ".") && ((tid = H5Topen(hobj, srcSubObjAbsName, H5P_DEFAULT)) >= 0))
                    {
                        copy_obj_data_t cp_data2 = {cp_data->dstFile, dstSubObjAbsName};
                        H5Tclose(tid);
                        copy_obj(hobj, srcSubObjAbsName, &cp_data2);
                    }
                    free(srcSubObjDirName);
                    free(srcSubObjAbsName);
                    free(dstSubObjDirName);
                    free(dstSubObjAbsName);
                }

                free(memname);
                H5Tclose(mtype);
            }

            /* Release member resources */
            H5Tclose(member_type);
        }

        /* write the header for this silo object */
        db_hdf5_hdrwr(dstfile, (char *)dstName, atype, atype, file_value, objtype);

        /* Cleanup */
        H5Tclose(atype);
        H5Tclose(s1024);
        H5Aclose(attr);
        H5Tclose(o);
        free(file_value);
        free(mem_value);
        free(bkg);

        break;
    }

    case H5G_DATASET:
        H5Ocopy(hobj, name, dstfile->cwg, name, H5P_DEFAULT, H5P_DEFAULT);
        break;

    default:
        /*ignore*/
        break;
    }
    return 0;
#else
    return db_perror("H5O API for copy", E_NOTENABLEDINBUILD);
#endif
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_CpDir
 *
 * Purpose:     Copy a directory hierarchy from one file to another. 
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller, Wed Aug  6 18:29:53 PDT 2008
 *
 * Modifications:
 *   Mark C. Miller, Thu Feb  4 20:02:53 PST 2010
 *   Removed unused variable and function call referencing it.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_CpDir(DBfile *_dbfile, char const *srcDir,
    DBfile *dstFile, char const *dstDir)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    DBfile_hdf5 *dstfile = (DBfile_hdf5*)dstFile;
    static char *me = "db_hdf5_CpDir";
    copy_obj_data_t cp_data = {0,0};
    char dstcwg[256], srccwg[256];

    srccwg[0] = '\0';
    dstcwg[0] = '\0';

#if !HDF5_VERSION_GE(1,8,0)
    db_perror("Requires HDF5-1.8 or later", E_CALLFAIL, me);
    return -1;
#endif

    PROTECT {

        /* Save current dirs for both src and dst files */
        db_hdf5_GetDir(_dbfile, srccwg);
        db_hdf5_GetDir(dstFile, dstcwg);

        /* Next, make destination directory and cd into it */
        db_hdf5_MkDir(dstFile, (char*)dstDir);
        db_hdf5_SetDir(dstFile, (char*)dstDir);

        /* Enter the recursion to make copy of the directory */
        cp_data.dstFile = dstFile;
        H5Giterate(dbfile->cwg, srcDir, 0, copy_obj, &cp_data);

        /* Restore current dirs for src and dst files */
        db_hdf5_SetDir(_dbfile, srccwg);
        db_hdf5_SetDir(dstFile, dstcwg);

    } CLEANUP {
        H5E_BEGIN_TRY {
            if (srccwg[0] != '\0')
                db_hdf5_SetDir(_dbfile, srccwg);
            if (dstcwg[0] != '\0')
                db_hdf5_SetDir(dstFile, dstcwg);
        } H5E_END_TRY;
    } END_PROTECT;

    return 0;
}

#ifndef _MSC_VER
#warning REMOVE THIS MAYBE
#endif
SILO_CALLBACK int
db_hdf5_CpListedObjects(int nobjs,
    DBfile *_dbfile, char const * const *srcObjs,
    DBfile *dstFile, char const * const *dstObjs)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    DBfile_hdf5 *dstfile = (DBfile_hdf5*)dstFile;
    static char *me = "db_hdf5_CpListedObjects";
    char *srcAbsName = 0, *dstAbsName = 0;
    char *dirPresent = 0;
    char dstcwg[256], srccwg[256];

    srccwg[0] = '\0';
    dstcwg[0] = '\0';

    db_hdf5_GetDir(_dbfile, srccwg);
    db_hdf5_GetDir(dstFile, dstcwg);

#if !HDF5_VERSION_GE(1,8,0)
    db_perror("Requires HDF5-1.8 or later", E_CALLFAIL, me);
    return -1;
#endif

    PROTECT {
        int i, pass;
         
        /* pass 0 ==> ensure all needed dst dirs are made, pass 1 ==> do the copies */
        dirPresent = (char *) calloc(nobjs,1);
        for (pass = 0; pass < 2; pass++)
        {
            for (i = 0; i < nobjs; i++)
            {
                DBObjectType _objtype;

                srcAbsName = db_join_path(srccwg, srcObjs[i]);

                if (dstObjs==0||dstObjs[i]==0||dstObjs[i][0]=='\0')
                    dstAbsName = db_join_path(dstcwg, srcObjs[i]);
                else
                {
                    int n = (int) strlen(dstObjs[i]);
                    dstAbsName = db_join_path(dstcwg, dstObjs[i]);
                    if ('/'==dstObjs[i][n-1] ||
                        (pass==0 && DB_DIR==DBInqVarType(dstFile, dstAbsName)) ||
                        (pass==1 && 0x0!=dirPresent[i]))
                    {
                        char *srcBaseName = db_basename(srcAbsName);
                        char *dstAbsName2 = db_join_path(dstAbsName, srcBaseName);
                        FREE(dstAbsName);
                        FREE(srcBaseName);
                        dstAbsName = dstAbsName2;
                        dirPresent[i] = 0x1;
                    }
                }

                _objtype = DBInqVarType(_dbfile, srcAbsName);

                if (_objtype == DB_DIR && pass==0)
                    db_hdf5_CpDir(_dbfile, srcAbsName, dstFile, dstAbsName);
                else
                {
                    copy_obj_data_t cp_data = {dstFile, dstAbsName};
                    if (pass == 0)
                    {
                        char *dname = db_dirname(dstAbsName); 
                        DBMkDirP(dstFile, dname);
                        FREE(dname);
                    }
                    else
                        copy_obj(dbfile->cwg, srcAbsName, &cp_data);
                }

                FREE(srcAbsName);
                FREE(dstAbsName);
            }
        }

    } CLEANUP {
        FREE(srcAbsName);
        FREE(dstAbsName);
        FREE(dirPresent);
    } END_PROTECT;

    return 0;
}

SILO_CALLBACK int
db_hdf5_MkSymlink(DBfile *_dbfile, char const *target, char const *link)
{
    char const *me = "db_hdf5_MkSymlink";
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    char *p = strchr(target, ':');
    char *tmp = 0;

    PROTECT
    {   
        if (p)
        {
            int n = (int) (p - target);
            tmp = strdup(target);
            tmp[n] = '\0';
            if (H5Lcreate_external(tmp, &tmp[n+1], dbfile->cwg, link, H5P_DEFAULT, H5P_DEFAULT)<0) {
                db_perror(link, E_CALLFAIL, me);
                UNWIND();
            }
            free(tmp);
            tmp = 0;
        }
        else
        {
            if (H5Lcreate_soft(target, dbfile->cwg, link, H5P_DEFAULT, H5P_DEFAULT)<0) {
                db_perror(link, E_CALLFAIL, me);
                UNWIND();
            }
        }
    } CLEANUP {
        FREE(tmp);
    } END_PROTECT;

    return 0;
}

PRIVATE int
db_hdf5_getslink(hid_t cwg, char const *in_candidate_link, char *out_target)
{
    char const *me = "db_hdf5_GetSymlink";
    H5L_info_t h5linfo;
    char lval[2*256];
    herr_t info;

    /* protect attempt get link info to allow this method to be used to query of a
       given name is a symlink or not */
    H5E_BEGIN_TRY {
        info  = H5Lget_info(cwg, in_candidate_link, &h5linfo, H5P_DEFAULT);
    } H5E_END_TRY;
    if (info < 0 || h5linfo.type == H5L_TYPE_ERROR) return -1;
    if (!out_target) return 0;
    if (h5linfo.u.val_size > sizeof(lval)) return -1;

    PROTECT
    {   
        if (H5Lget_val(cwg, in_candidate_link, lval, h5linfo.u.val_size, H5P_DEFAULT)<0) {
            db_perror(in_candidate_link, E_CALLFAIL, me);
            UNWIND();
        }

        if (h5linfo.type == H5L_TYPE_EXTERNAL)
        {
            char const *f=0,*p=0;
            if (H5Lunpack_elink_val(lval, h5linfo.u.val_size, 0, &f, &p)<0)
            {
                db_perror(in_candidate_link, E_CALLFAIL, me);
                UNWIND();
            }
            *out_target = '\0';
            if (f)
            {
                strcat(out_target, f);
                strcat(out_target, ":");
            }
            if (p)
                strcat(out_target, p);
        }
        else
        {
            *out_target = '\0';
            strncpy(out_target, lval, h5linfo.u.val_size);
        }
    } CLEANUP {
    } END_PROTECT;

    return 0;

}

SILO_CALLBACK int
db_hdf5_GetSymlink(DBfile *_dbfile, char const *in_candidate_link, char *out_target)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    return db_hdf5_getslink(dbfile->cwg, in_candidate_link, out_target);
}


/*-------------------------------------------------------------------------
 * Function:    db_hdf5_NewToc
 *
 * Purpose:     Destroy the previous table of contents and replace it with a
 *              new table of contents for the current working directory.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_NewToc(DBfile *_dbfile)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    DBtoc       *toc=NULL;
    
    db_FreeToc(_dbfile);
    dbfile->pub.toc = toc = db_AllocToc();

    if (H5Literate(dbfile->cwg, H5_INDEX_NAME, H5_ITER_INC, NULL, load_toc, toc)<0) return -1;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetComponentType
 *
 * Purpose:     Return the data type of the component 
 *
 * Programmer:  Mark C. Miller
 *              Wednesday, April 20, 2005
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_GetComponentType(DBfile *_dbfile, char const *objname, char const *compname)
{
    int datatype = DB_NOTYPE;
    db_hdf5_GetComponentStuff(_dbfile, objname, compname, &datatype);
    return datatype;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetComponent
 *
 * Purpose:     Reads the component COMPNAME of object OBJNAME, allocates
 *              memory and copies the value into it to return.
 *
 * Warning:     The names of components in an object in an HDF5 file may not
 *              be the same as the names of components in a PDB file.
 *
 * Programmer:  Mark C. Miller
 *              Wednesday, April 20, 2005
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK void *
db_hdf5_GetComponent(DBfile *_dbfile, char const *objname, char const *compname)
{
    return db_hdf5_GetComponentStuff(_dbfile, objname, compname, 0);
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetComponentStuff
 *
 * Purpose:     Reads the component COMPNAME of object OBJNAME, allocates
 *              memory and copies the value into it to return.
 *
 * Warning:     The names of components in an object in an HDF5 file may not
 *              be the same as the names of components in a PDB file.
 *
 * Return:      Success:        Pointer to allocated memory which contains
 *                              the component value. Floating-point
 *                              components are returned in either single or
 *                              double precision according to the
 *                              DBForceSingle() setting.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, August 17, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *   This routine had assumed that any string-valued data member was
 *   specifying the name of a dataset to read. That is not always true.
 *   I modified it to attempt to read a dataset of the given name or
 *   just return the string.
 *
 *   Mark C. Miller, Wed Apr 20 15:09:41 PDT 2005
 *   Renamed from db_hdf5_GetComponent. Added just_get_dataype argument
 *   and logic to just return data type when requested and avoid 
 *   actual read of data
 *
 *   Mark C. Miller, Wed Jan 21 16:03:02 PST 2009
 *   Added logic to deal with components that are vectors of tuples 
 *   but where DBGetObject flattens these tuples into groups of related
 *   scalars with digits appended to their names. This fixes a problem
 *   in silex displaying these vector-based components.
 *
 *   Mark C. Miller, Tue Apr 28 15:57:43 PDT 2009
 *   Added missing statement to set mnofname = 0 after free'ing it.
 *
 *   Mark C. Miller, Tue Oct  6 10:24:01 PDT 2009
 *   Removed call to free(mnofname) and set it to zero. Added logic
 *   to free it just prior to STRDUP'ing it.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK void *
db_hdf5_GetComponentStuff(DBfile *_dbfile, char const *objname, char const *compname,
    int *just_get_datatype)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_GetComponent";
    hid_t       o=-1, attr=-1, atype=-1, ftype=-1, mtype=-1, dset=-1;
    int         datatype, mno, n, ndims, i, mult, complen, mnof=-1, mnofidx=-1;
    hsize_t     dim[3];
    char        *mnofname=0;
    void        *retval=NULL;
    
    PROTECT {
        /* Open the object as a named data type */
        if ((o=H5Topen(dbfile->cwg, objname, H5P_DEFAULT))<0) {
            db_perror(objname, E_NOTFOUND, me);
            UNWIND();
        }

        /*
         * Open the `silo' attribute (all silo objects have one), and
         * retrieve its data type (a single-level H5T_COMPOUND type).
         */
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            (atype=H5Aget_type(attr))<0) {
            db_perror(objname, E_CALLFAIL, me);
            UNWIND();
        }

        /* Scan through the compound type to find the requested component. */
        n = H5Tget_nmembers(atype);
        for (mno=0; mno<n; mno++) {
            char *memb_name = H5Tget_member_name(atype, mno);
            if (memb_name && !strcmp(memb_name, compname)) {
                free(memb_name);
                break;
            }

            /* If we didn't get a match but the last char is a digit, then
               it might be a funky component named by adding a digit to an
               existing component name by a preceding DBGetObject() call */ 
            complen = strlen(compname);
            while (57 >= compname[complen-1] && compname[complen-1] >= 48)
                complen--;
            if (57 >= compname[complen] && compname[complen] >= 48)
            {
                if (memb_name && !strncmp(memb_name, compname, complen))
                {
                    mnof = mno;
                    errno = 0;
                    mnofidx = strtol(&compname[complen], 0, 0) - 1;
                    if (errno != 0) mnofidx = -1;
                    if (mnofname) free(mnofname);
                    mnofname = STRDUP(memb_name);
                }
            }
            if (memb_name) free(memb_name);
        }
        if (mno>=n) {
            if (mnof == -1)
            {
                db_perror(compname, E_NOTFOUND, me);
                UNWIND();
            }
            mno = mnof;
        }

        /* Get the datatype and multiplicity for the component */
        if ((ftype=db_hdf5_get_cmemb(atype, mno, &ndims, dim))<0) {
            db_perror(compname, E_CALLFAIL, me);
            UNWIND();
        }

        if (H5T_STRING==H5Tget_class(ftype)) {

            if (just_get_datatype == 0)
            {
                /*
                 * A string is usually a pointer to some other dataset. So, try
                 * read that dataset instead.
                 */
                char dataset_name[] =
                    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

                /* Read the dataset name from the attribute */
                if ((H5Tget_size(ftype)+1<sizeof dataset_name) &&
                    (mtype=H5Tcreate(H5T_COMPOUND, H5Tget_size(ftype)))>=0 &&
                    db_hdf5_put_cmemb(mtype, compname, 0, 0, NULL, ftype)>=0 && 
                    H5Aread(attr, mtype, dataset_name)>=0) {
                    if ((dset=H5Dopen(dbfile->cwg, dataset_name, H5P_DEFAULT))>=0) {
                        retval = db_hdf5_comprd(dbfile, dataset_name, 1);
                        H5Dclose(dset);
                    }
                    else {
                        retval = STRDUP(dataset_name);
                    }
                }
            }
            else
            {
                *just_get_datatype = DB_VARIABLE;
            }
        }
        else
        {
            /* Read a single component of the compound attribute */
            if ((datatype=hdf2silo_type(ftype))<0) {
                db_perror(compname, E_CALLFAIL, me);
                UNWIND();
            }

            for (i=0, mult=1; i<ndims; i++) mult *= (int) dim[i];

            if (just_get_datatype == 0)
            {
                /* Allocate the return value */
                if (NULL==(retval=calloc(mult, db_GetMachDataSize(datatype)))) {
                    db_perror(compname, E_CALLFAIL, me);
                    UNWIND();
                }

                /* Build the hdf5 data type to read */
                mtype = H5Tcreate(H5T_COMPOUND, mult * db_GetMachDataSize(datatype));
                if (mnof != -1 && mnofidx != -1 && mnofname != 0)
                    db_hdf5_put_cmemb(mtype, mnofname, 0, ndims, dim, hdf2hdf_type(ftype));
                else
                    db_hdf5_put_cmemb(mtype, compname, 0, ndims, dim, hdf2hdf_type(ftype));

                /* Read the data into the output buffer */
                if (H5Aread(attr, mtype, retval)<0) {
                    db_perror(compname, E_CALLFAIL, me);
                    UNWIND();
                }

                /* Handle the case where we got a match on member name modulo trailing
                   digits (which really represent an index into the member) */
                if (mnof != -1 && mnofidx != -1 && mnofname != 0)
                {
                    void *newretval;
                    char *pretval = (char *) retval + mnofidx * db_GetMachDataSize(datatype);
                    if (NULL==(newretval=calloc(1, db_GetMachDataSize(datatype)))) {
                        db_perror(mnofname, E_CALLFAIL, me);
                        UNWIND();
                    }
                    memcpy(newretval, pretval, db_GetMachDataSize(datatype));
                    free(retval);
                    retval = newretval;
                }
            }
            else
            {
                *just_get_datatype = datatype;
                if (mult > 1)
                {
                    if (datatype == DB_INT)
                        *just_get_datatype = DB_INTA;
                    else if (datatype == DB_FLOAT)
                        *just_get_datatype = DB_FLOATA;
                    else if (datatype == DB_DOUBLE)
                        *just_get_datatype = DB_DOUBLEA;
                }
            }
        }
        
        /* Release objects */
        if (mnofname) free(mnofname);
        H5Tclose(o);
        H5Aclose(attr);
        H5Tclose(atype);
        H5Tclose(ftype);
        if (mtype != -1) H5Tclose(mtype);
    } CLEANUP {
        /* Release objects */
        if (retval) {
            free(retval);
            retval = NULL;
        }
        if (mnofname) free(mnofname);
        H5E_BEGIN_TRY {
            H5Tclose(o);
            H5Aclose(attr);
            H5Tclose(atype);
            H5Tclose(ftype);
            H5Tclose(mtype);
        } H5E_END_TRY;
    } END_PROTECT;

    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetComponentNames
 *
 * Purpose:     Returns the component names for the specified object. The
 *              COMP_NAMES and FILE_NAMES output arguments will point to
 *              an array of pointers to names. Each name as well as the
 *              two arrays will be allocated with `malloc'.  This is
 *              essentially the same as DBGetObject().
 *
 * Return:      Success:        Number of components found for the
 *                              specified object.
 *
 *              Failure:        zero
 *
 * Programmer:  Robb Matzke, 2001-02-06
 *
 * Modifications:
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_GetComponentNames(DBfile *_dbfile, char const *objname, char ***comp_names,
                          char ***file_names)
{
    DBobject    *obj;
    int         n;

    if (NULL==(obj=db_hdf5_GetObject(_dbfile, objname))) return 0;
    if (comp_names) {
        *comp_names = obj->comp_names;
    } else {
        free(obj->comp_names);
        obj->comp_names = 0;
    }
    if (file_names) {
        *file_names = obj->pdb_names;
    } else {
        free(obj->pdb_names);
        obj->pdb_names = 0;
    }
    n = obj->ncomponents;
    obj->ncomponents = 0;
    DBFreeObject(obj);
    return n;
}
    
/*-------------------------------------------------------------------------
 * Function:    db_hdf5_WriteObject
 *
 * Purpose:     Write/overwrite a DBobject into the given file.

 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke, 2001-02-05
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *   I made it support writing a DBZonelist object. I also fixed an
 *   off-by-one error in indexing of string valued component names
 *
 *   Mark C. Miller, Wed Jan 13 14:21:50 PST 2010
 *   Added condition of flags!=OVERWRITE before erroring on non-user
 *   defined objects. A user could be using browser to OVERWRITE a
 *   standard Silo object.
 *
 *   Mark C. Miller, Fri Apr 10 15:00:03 PDT 2026
 *   Limit loops looking for tiny array cases to DB_MAX_H5_OBJ_VALS.
 *-------------------------------------------------------------------------
 */
static int count_commas(char const *str)
{
    int n = 0;
    char const *p = strchr(str, ',');
    while (p)
    {
        n++;
        p = strchr(p, ',');
    }
    return n;
}

SILO_CALLBACK int
db_hdf5_WriteObject(DBfile *_dbfile,    /*File to write into */
                    DBobject const *obj,/*Object description to write out */
                    int flags)          /*1=>free associated memory */
{
    DBfile_hdf5         *dbfile=(DBfile_hdf5*)_dbfile;
    static char         *me="db_hdf5_WriteObject";
    size_t              msize=0, fsize=0, moffset=0, foffset=0;
    unsigned char       *object=NULL;
    hid_t               mtype=-1, ftype=-1;
    int                 i;

    PROTECT {
        /* If flags is set then delete any existing object by the same
         * name, ignoring failures. */
        if (flags) {
            H5E_BEGIN_TRY {
                H5Gunlink(dbfile->cwg, obj->name);
            } H5E_END_TRY;
        }
        
        /* How much memory do we need? Align all components */
        for (i=0, msize=fsize=0; i<obj->ncomponents; i++) {
            int nvals = count_commas(obj->pdb_names[i]) + 1;
            if (!strncmp(obj->pdb_names[i], "'<i>", 4)) {
                msize = ALIGN(msize, sizeof(int)) + nvals * sizeof(int);
                fsize += (nvals * H5Tget_size(dbfile->T_int));
            } else if (!strncmp(obj->pdb_names[i], "'<f>", 4)) {
                msize = ALIGN(msize, sizeof(float)) + nvals * sizeof(float);
                fsize += (nvals * H5Tget_size(dbfile->T_float));
            } else if (!strncmp(obj->pdb_names[i], "'<d>", 4)) {
                msize = ALIGN(msize, sizeof(double)) + nvals * sizeof(double);
                fsize += (nvals * H5Tget_size(dbfile->T_double));
            } else if (!strncmp(obj->pdb_names[i], "'<s>", 4)) {
                msize += strlen(obj->pdb_names[i]+4); /* inc. trailing ' for '\0' */
                fsize += strlen(obj->pdb_names[i]+4); /* inc. trailing ' for '\0' */
            } else if (obj->pdb_names[i][0]=='\'') {
                /* variable has invalid name or we don't handle type */
                db_perror(obj->pdb_names[i], E_INVALIDNAME, me);
                UNWIND();
            } else {
                /* Its possible this is a 'special' tiny array 'var' component.
                 * So, first, search for it in obj->h5_names and if found, handle
                 * it as such */
                int j, off=0, found=0;
                for (j = 0; obj->h5_names[j] && j < DB_MAX_H5_OBJ_VALS; j++)
                {
                    off += obj->h5_sizes[j] * db_GetMachDataSize(obj->h5_types[j]);
                    if (!strcmp(obj->comp_names[i], obj->h5_names[j]))
                    {
                        found = 1;
                        break;
                    }
                }
                if (found)
                {
                    switch (obj->h5_types[j])
                    {
                        case DB_INT:
                        {
                            int dummy[3];
                            hsize_t _size = 3;
                            hid_t _f_ary;
                            msize = ALIGN(msize, sizeof(dummy)) + sizeof(dummy);
#if HDF5_VERSION_GE(1,4,0)
                            _f_ary = H5Tarray_create(dbfile->T_int, 1, &_size);
                            fsize += H5Tget_size(_f_ary);
                            H5Tclose(_f_ary);
#else
                            fsize += 3*H5Tget_size(dbfile->T_int);
#endif
                            break;
                        }
                        case DB_FLOAT:
                        {
                            float dummy[3];
                            hsize_t _size = 3;
                            hid_t _f_ary;
                            msize = ALIGN(msize, sizeof(dummy)) + sizeof(dummy);
#if HDF5_VERSION_GE(1,4,0)
                            _f_ary = H5Tarray_create(dbfile->T_float, 1, &_size);
                            fsize += H5Tget_size(_f_ary);
                            H5Tclose(_f_ary);
#else
                            fsize += 3*H5Tget_size(dbfile->T_float);
#endif
                            break;
                        }
                        case DB_DOUBLE:
                        {
                            double dummy[3];
                            hsize_t _size = 3;
                            hid_t _f_ary;
                            msize = ALIGN(msize, sizeof(dummy)) + sizeof(dummy);
#if HDF5_VERSION_GE(1,4,0)
                            _f_ary = H5Tarray_create(dbfile->T_double, 1, &_size);
                            fsize += H5Tget_size(_f_ary);
                            H5Tclose(_f_ary);
#else
                            fsize += 3*H5Tget_size(dbfile->T_double);
#endif
                            break;
                        }
                        default:
                            break;
                    }
                }
                else
                {
                    /* variable added by DBAddVarComponent() */
                    msize += strlen(obj->pdb_names[i]) + 1;
                    fsize += strlen(obj->pdb_names[i]) + 1;
                }
            }
        }

        if (!msize) {
            db_perror("Object has zero size", E_BADARGS, me);
            UNWIND();
        }

        /* Create the object and initialize it */
        if (NULL==(object=(unsigned char *)calloc(1, msize))) {
            db_perror(NULL, E_NOMEM, me);
            UNWIND();
        }
        if ((mtype=H5Tcreate(H5T_COMPOUND, msize))<0 ||
            (ftype=H5Tcreate(H5T_COMPOUND, fsize))<0) {
            free(object);
            object = 0;
            db_perror("H5Tcreate", E_CALLFAIL, me);
            UNWIND();
        }
        for (i=0, moffset=foffset=0; i<obj->ncomponents; i++) {
            int nvals = count_commas(obj->pdb_names[i]) + 1;
            if (!strncmp(obj->pdb_names[i], "'<i>", 4)) {
                moffset = ALIGN(moffset, sizeof(int));
                if (H5Tinsert(mtype, obj->comp_names[i], moffset,
                              H5T_NATIVE_INT)<0 ||
                    H5Tinsert(ftype, obj->comp_names[i], foffset,
                              dbfile->T_int)<0) {
                    free(object);
                    object = 0;
                    db_perror("H5Tinsert", E_CALLFAIL, me);
                    UNWIND();
                }
                *(int*)(object+moffset) = strtol(obj->pdb_names[i]+4, NULL, 0);
                moffset += sizeof(int);
                foffset += H5Tget_size(dbfile->T_int);
            } else if (!strncmp(obj->pdb_names[i], "'<f>", 4)) {
                moffset = ALIGN(moffset, sizeof(float));
                if (H5Tinsert(mtype, obj->comp_names[i], moffset,
                              H5T_NATIVE_FLOAT)<0 ||
                    H5Tinsert(ftype, obj->comp_names[i], foffset,
                              dbfile->T_float)<0) {
                    free(object);
                    object = 0;
                    db_perror("H5Tinsert", E_CALLFAIL, me);
                    UNWIND();
                }
                *(float*)(object+moffset) = strtod(obj->pdb_names[i]+4, NULL);
                moffset += sizeof(float);
                foffset += H5Tget_size(dbfile->T_float);
            } else if (!strncmp(obj->pdb_names[i], "'<d>", 4)) {
                moffset = ALIGN(moffset, sizeof(double));
                if (H5Tinsert(mtype, obj->comp_names[i], moffset,
                              H5T_NATIVE_DOUBLE)<0 ||
                    H5Tinsert(ftype, obj->comp_names[i], foffset,
                              dbfile->T_double)<0) {
                    free(object);
                    object = 0;
                    db_perror("H5Tinsert", E_CALLFAIL, me);
                    UNWIND();
                }
                *(double*)(object+moffset) = strtod(obj->pdb_names[i]+4, NULL);
                moffset += sizeof(double);
                foffset += H5Tget_size(dbfile->T_double);
            } else if (!strncmp(obj->pdb_names[i], "'<s>", 4)) {
                size_t len = strlen(obj->pdb_names[i]+4); /* inc. trailing ' for '\0' */
#ifndef _WIN32
#warning COMPATABILITY ISSUE
#endif
                hid_t str_type;
                if (len > 1024 && !DBGetAllowLongStrComponentsFile(_dbfile))
                {
                    free(object);
                    object = 0;
                    db_perror("encountered Str component > 1024 chars", E_CALLFAIL, me);
                    UNWIND();
                }
                str_type = H5Tcopy(H5T_C_S1);
                H5Tset_size(str_type, len);
                if (H5Tinsert(mtype, obj->comp_names[i], moffset,
                              str_type)<0 ||
                    H5Tinsert(ftype, obj->comp_names[i], foffset,
                              str_type)<0) {
                    free(object);
                    object = 0;
                    db_perror("H5Tinsert", E_CALLFAIL, me);
                    UNWIND();
                }
                H5Tclose(str_type);
                strncpy((char*)(object+moffset), obj->pdb_names[i]+4, len);
                object[moffset+len-1] = '\0'; /*overwrite trailing ' as '\0' */
                moffset += len;
                foffset += len;
            } else {
                /* Its possible this is a 'special' tiny array 'var' component.
                 * So, first, search for it in obj->h5_names and if found, handle
                 * it as such */
                int j, found=0;
                for (j = 0; obj->h5_names[j] && j < DB_MAX_H5_OBJ_VALS; j++)
                {
                    if (!strcmp(obj->comp_names[i], obj->h5_names[j]))
                    {
                        found = 1;
                        break;
                    }
                }
                if (found)
                {
                    switch (obj->h5_types[j])
                    {
                        case DB_INT:
                        {
                            int dummy[3];
                            hid_t _m_ary, _f_ary;
                            hsize_t _size = 3;
                            moffset = ALIGN(moffset, sizeof(dummy));
#if HDF5_VERSION_GE(1,4,0)
                            _m_ary = H5Tarray_create(H5T_NATIVE_INT, 1, &_size);
                            _f_ary = H5Tarray_create(dbfile->T_int, 1, &_size);
                            if (H5Tinsert(mtype, obj->comp_names[i], moffset, _m_ary)<0 ||
                                H5Tinsert(ftype, obj->comp_names[i], foffset, _f_ary)<0) {
                                free(object);
                                object = 0;
                                db_perror("H5Tinsert", E_CALLFAIL, me);
                                UNWIND();
                            }
                            foffset += H5Tget_size(_f_ary);
                            H5Tclose(_f_ary);
                            H5Tclose(_m_ary);
#else
                            free(object);
                            object = 0;
                            db_perror("Cannot customize standard object", E_CALLFAIL, me);
                            UNWIND();
#endif
                            memcpy(object+moffset, &obj->h5_vals[obj->h5_offs[j]], sizeof(dummy));
                            moffset += sizeof(dummy);
                            break;
                        }
                        case DB_FLOAT:
                        {
                            float dummy[3];
                            hid_t _m_ary, _f_ary;
                            hsize_t _size = 3;
                            moffset = ALIGN(moffset, sizeof(dummy));
#if HDF5_VERSION_GE(1,4,0)
                            _m_ary = H5Tarray_create(H5T_NATIVE_FLOAT, 1, &_size);
                            _f_ary = H5Tarray_create(dbfile->T_float, 1, &_size);
                            if (H5Tinsert(mtype, obj->comp_names[i], moffset, _m_ary)<0 ||
                                H5Tinsert(ftype, obj->comp_names[i], foffset, _f_ary)<0) {
                                free(object);
                                object = 0;
                                db_perror("H5Tinsert", E_CALLFAIL, me);
                                UNWIND();
                            }
                            foffset += H5Tget_size(_f_ary);
                            H5Tclose(_f_ary);
                            H5Tclose(_m_ary);
#else
                            free(object);
                            object = 0;
                            db_perror("Cannot customize standard object", E_CALLFAIL, me);
                            UNWIND();
#endif
                            memcpy(object+moffset, &obj->h5_vals[obj->h5_offs[j]], sizeof(dummy));
                            moffset += sizeof(dummy);
                            break;
                        }
                        case DB_DOUBLE:
                        {
                            double dummy[3];
                            hid_t _m_ary, _f_ary;
                            hsize_t _size = 3;
                            moffset = ALIGN(moffset, sizeof(dummy));
#if HDF5_VERSION_GE(1,4,0)
                            _m_ary = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, &_size);
                            _f_ary = H5Tarray_create(dbfile->T_double, 1, &_size);
                            if (H5Tinsert(mtype, obj->comp_names[i], moffset, _m_ary)<0 ||
                                H5Tinsert(ftype, obj->comp_names[i], foffset, _f_ary)<0) {
                                free(object);
                                object = 0;
                                db_perror("H5Tinsert", E_CALLFAIL, me);
                                UNWIND();
                            }
                            foffset += H5Tget_size(_f_ary);
                            H5Tclose(_f_ary);
                            H5Tclose(_m_ary);
#else
                            free(object);
                            object = 0;
                            db_perror("Cannot customize standard object", E_CALLFAIL, me);
                            UNWIND();
#endif
                            memcpy(object+moffset, &obj->h5_vals[obj->h5_offs[j]], sizeof(dummy));
                            moffset += sizeof(dummy);
                            break;
                        }
                    }
                }
                else
                {
                    size_t len = strlen(obj->pdb_names[i])+1;
                    hid_t str_type = H5Tcopy(H5T_C_S1);
                    H5Tset_size(str_type, len);
                    if (H5Tinsert(mtype, obj->comp_names[i], moffset,
                                  str_type)<0 ||
                        H5Tinsert(ftype, obj->comp_names[i], foffset,
                              str_type)<0) {
                        free(object);
                        object = 0;
                        db_perror("H5Tinsert", E_CALLFAIL, me);
                        UNWIND();
                    }
                    H5Tclose(str_type);
                    strcpy((char*)(object+moffset), obj->pdb_names[i]);
                    moffset += len;
                    foffset += len;
                }
            }
        }

        if (db_hdf5_hdrwr(dbfile, obj->name, mtype, ftype, object,
                          (DBObjectType) DBGetObjtypeTag(obj->type))<0) {
            free(object);
            object = 0;
            UNWIND();
        }
        H5Tclose(mtype);
        H5Tclose(ftype);
        if (object) free(object);
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Tclose(mtype);
            H5Tclose(ftype);
        } H5E_END_TRY;
        if (object) free(object);
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_WriteComponent
 *
 * Purpose:     Add a variable component to the given object structure and
 *              write out the associated data.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke, 2001-02-06
 *
 * Modifications:
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Fri Apr 10 14:59:06 PDT 2026
 *   Fix bug in tiny array handling where offsets into temp. buffer were
 *   being computed incorrectly.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_WriteComponent(DBfile *_dbfile, DBobject *obj, char const *compname,
                       char const *prefix, char const *dataname, void const *data,
                       int rank, long const *_size)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    int         size[32], i, totsize;
    char        varname[256];
    int         datatype = db_GetDatatypeID(dataname);

    /*
     * We have to be careful here to deal with possible customization of
     * 'standard' objects. The HDF5 driver handles small arrays of things
     * like dimensions, extents, alignment, etc. as 'array' members of
     * the attribute 'header' struct. So, we cannot arbitrarily decide to
     * write every component we see here to the file. Sometimes, we'll have
     * to avert that operation and instead store the data to the DBobject
     * struct, temporarily, for eventual write via DBWriteObject
     */
    
    totsize = 1;
    for (i=0; i<rank; i++) {
        size[i] = _size[i];
        totsize *= size[i];
    }

    if (rank == 1 && totsize <= 3)
    {
        int add_it = 0;
        DBObjectType objtype = (DBObjectType) DBGetObjtypeTag(obj->type);

        switch (objtype)
        {
            case DB_CSGMESH:
            {
                add_it |= !strcmp(compname, "min_extents");
                add_it |= !strcmp(compname, "max_extents");
                break;
            }
            case DB_QUADMESH:
            case DB_QUADCURV:
            case DB_QUADRECT:
            {
                add_it |= !strcmp(compname, "min_extents");
                add_it |= !strcmp(compname, "max_extents");
                add_it |= !strcmp(compname, "dims");
                add_it |= !strcmp(compname, "min_index");
                add_it |= !strcmp(compname, "max_index");
                add_it |= !strcmp(compname, "baseindex");
                break;
            }
            case DB_QUADVAR:
            {
                add_it |= !strcmp(compname, "zones");
                add_it |= !strcmp(compname, "align");
                add_it |= !strcmp(compname, "dims");
                add_it |= !strcmp(compname, "min_index");
                add_it |= !strcmp(compname, "max_index");
                break;
            }
            case DB_UCDMESH:
            {
                add_it |= !strcmp(compname, "min_extents");
                add_it |= !strcmp(compname, "max_extents");
                break;
            }
            case DB_MATERIAL:
            case DB_MATSPECIES:
            {
                add_it |= !strcmp(compname, "dims");
                break;
            } 
            case DB_POINTMESH:
            {
                add_it |= !strcmp(compname, "min_extents");
                add_it |= !strcmp(compname, "max_extents");
                break;
            }
            default: break;
        }

        if (add_it)
        {
            int i;
            for (i = 0; obj->h5_names[i] && i < DB_MAX_H5_OBJ_VALS; i++); /* find first open slot */
            if (i >= DB_MAX_H5_OBJ_VALS)
                return db_perror(compname, E_OBJBUFFULL, "db_hdf5_WriteComponent");
            obj->h5_names[i] = strdup(compname);
            obj->h5_types[i] = datatype;
            obj->h5_sizes[i] = totsize;
            obj->h5_offs[i] = i==0?0:obj->h5_offs[i-1]+obj->h5_sizes[i-1]*db_GetMachDataSize(obj->h5_types[i-1]);
            if (obj->h5_offs[i] + totsize * db_GetMachDataSize(datatype) > sizeof(obj->h5_vals))
                return db_perror(compname, E_OBJBUFFULL, "db_hdf5_WriteComponent");
            memcpy(&obj->h5_vals[obj->h5_offs[i]], data, totsize * db_GetMachDataSize(datatype));
            DBAddVarComponent(obj, compname, compname);
            return 0;
        }
    }

    varname[0] = '\0';
    db_hdf5_compwr(dbfile, datatype, rank, size, data, varname,
        friendly_name(_dbfile,obj->name, compname, 0));
    DBAddVarComponent(obj, compname, varname);
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_InqVarExists
 *
 * Purpose:     Check whether the variable VARNAME exists.
 *
 * Return:      Success:        positive if exists, zero otherwise.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, February  9, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *   I made it return true or false based on existence of named entity
 *   only and not that the entity also be a dataset.
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_InqVarExists (DBfile *_dbfile, char const *varname)
{
   DBfile_hdf5  *dbfile = (DBfile_hdf5*)_dbfile;
   herr_t       status;
   H5G_stat_t   sb;

   /* Check existence */
   H5E_BEGIN_TRY {
       status = H5Gget_objinfo(dbfile->cwg, varname, TRUE, &sb);
   } H5E_END_TRY;
   if (status<0)
       return FALSE;
   else
       return TRUE;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetVarLength
 *
 * Purpose:     Returns the number of elements in the specified variable.
 *
 * Return:      Success:        Number of elements
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_GetVarLength(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_GetVarLength";
    hid_t       dset=-1, space=-1;
    hsize_t     nelmts=-1;

    PROTECT {
        if ((dset=H5Dopen(dbfile->cwg, name, H5P_DEFAULT))>=0) {
            if ((space=H5Dget_space(dset))<0) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }
            nelmts = H5Sget_simple_extent_npoints(space);
            H5Dclose(dset);
            H5Sclose(space);
        }
        else
        {
            if (!db_hdf5_get_comp_var(_dbfile, name, &nelmts,
                 NULL, NULL, NULL, NULL)) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }
        }
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Dclose(dset);
            H5Sclose(space);
        } H5E_END_TRY;
    } END_PROTECT;

    return nelmts;
}

PRIVATE int
db_hdf5_get_var_byte_length(DBfile *_dbfile, char const *name, int use_file_size)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_GetVarByteLength";
    hid_t       dset=-1, ftype=-1, mtype=-1, space=-1;
    hsize_t     nbytes_big;
    int         nbytes_small=-1;

#ifndef _MSC_VER
#warning REMOVED db_perror CALLS
#endif
    PROTECT {
        /* Open the dataset */
        if ((dset=H5Dopen(dbfile->cwg, name, H5P_DEFAULT))>=0) {
        
            /* Get data type and space */
            if ((ftype=H5Dget_type(dset))<0 ||
                (mtype=hdf2hdf_type(ftype))<0 ||
                (space=H5Dget_space(dset))<0) {
                /*db_perror(name, E_CALLFAIL, me);*/
                UNWIND();
            }

            /* Get total size in bytes and check for overflow */
            if (use_file_size)
                nbytes_big = H5Dget_storage_size(dset);
            else
                nbytes_big = H5Sget_simple_extent_npoints(space) * H5Tget_size(mtype);
            nbytes_small = (int)nbytes_big;
            if (nbytes_big!=(hsize_t)nbytes_small) {
                /*db_perror("overflow", E_INTERNAL, me);*/
                UNWIND();
            }

            /* Release resources */
            H5Tclose(ftype);
            H5Sclose(space);
            H5Dclose(dset);
        }
        else
        {
            hsize_t nelmts;
            hsize_t fsize;
            size_t elsize;
            if (!db_hdf5_get_comp_var(_dbfile, name, &nelmts,
                 &elsize, &fsize, NULL, NULL)) {
                /*db_perror(name, E_CALLFAIL, me);*/
                UNWIND();
            }
            if (use_file_size)
                nbytes_small = (int) fsize;
            else
                nbytes_small = (int)(nelmts * elsize);
        }
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Tclose(ftype);
            H5Sclose(space);
            H5Dclose(dset);
        } H5E_END_TRY;
        CANCEL_UNWIND;
    } END_PROTECT;

    return nbytes_small;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetVarByteLength
 *
 * Purpose:     Returns the number of bytes needed to store the entire
 *              variable in memory.
 *
 * Return:      Success:        Number of bytes
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Wednesday, March 17, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_GetVarByteLength(DBfile *_dbfile, char const *name)
{
    int const use_file_size = 0;
    return db_hdf5_get_var_byte_length(_dbfile, name, use_file_size);
}

SILO_CALLBACK int
db_hdf5_GetVarByteLengthInFile(DBfile *_dbfile, char const *name)
{
    int const use_file_size = 1;
    return db_hdf5_get_var_byte_length(_dbfile, name, use_file_size);
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetVarType
 *
 * Purpose:     Query the data type of the variable.
 *
 * Return:      Success:        One of the DB_* constants that describes data
 *                              type. The type is chosen to be at least as
 *                              large as the type in the file, but using
 *                              DB_LONG in cases where there is no native
 *                              type which is large enough.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Sunday, February 14, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *   I made it return -1 if name or *name is null.
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_GetVarType(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_GetVarType";
    hid_t       dset=-1, ftype=-1;
    int         silo_type=-1;

    if ((name == 0) || (*name == 0))
        return -1;

    PROTECT {
        if ((dset=H5Dopen(dbfile->cwg, name, H5P_DEFAULT))>=0) {
            if ((ftype=H5Dget_type(dset))<0) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }
            silo_type = hdf2silo_type(ftype);
            H5Dclose(dset);
            H5Tclose(ftype);
        }
        else
        {
            if (!db_hdf5_get_comp_var(_dbfile, name, NULL,
                 NULL, NULL, &ftype, NULL)) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }
            silo_type = hdf2silo_type(ftype);
        }
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Dclose(dset);
            H5Tclose(ftype);
        } H5E_END_TRY;
    } END_PROTECT;

    return silo_type;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetVarDims
 *
 * Purpose:     Obtains the size of the dimensions of some variable.
 *
 * Return:      Success:        Number of dimensions. At most MAXDIMS values
 *                              are copied into the DIMS argument.
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Monday, February 15, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_GetVarDims(DBfile *_dbfile, char const *name, int maxdims, int *dims/*out*/)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_GetVarDims";
    hid_t       dset=-1, space=-1;
    hsize_t     ds_size[H5S_MAX_RANK];
    int         i, ndims=-1;
    
    PROTECT {
        if ((dset=H5Dopen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((space=H5Dget_space(dset))<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if ((ndims=H5Sget_simple_extent_dims(space, ds_size, NULL))<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        for (i=0; i<MIN(ndims,maxdims); i++) dims[i] = ds_size[i];
        H5Sclose(space);
        H5Dclose(dset);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Sclose(space);
            H5Dclose(dset);
        } H5E_END_TRY;
    } END_PROTECT;

    return ndims;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetVar
 *
 * Purpose:     Same as db_hdf5_ReadVar except it allocates space for the
 *              result.
 *
 * Return:      Success:        Ptr to data read, allocated with malloc()
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, February 11, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Feb 15 14:53:29 PST 2005
 *   Forced it to ignore force_single setting 
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK void *
db_hdf5_GetVar(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_GetVar";
    hid_t       dset=-1, ftype=-1, mtype=-1, space=-1;
    hsize_t     np;
    void        *result=NULL;

    PROTECT {

        /* Get dataset, type, and space */
        if ((dset=H5Dopen(dbfile->cwg, name, H5P_DEFAULT))>=0) {
            if ((ftype=H5Dget_type(dset))<0 ||
                (space=H5Dget_space(dset))<0) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }

            /* Choose a memory type based on the file type */
            if ((mtype=hdf2hdf_type(ftype))<0) {
                db_perror("data type", E_BADARGS, me);
                UNWIND();
            }
        
            /* Allocate space for the result */
            np = H5Sget_simple_extent_npoints(space);

            if (np) {
                if (NULL==(result=malloc(np * H5Tget_size(mtype)))) {
                    db_perror(NULL, E_NOMEM, me);
                    UNWIND();
                }

                P_rdprops = H5P_DEFAULT;
                if (!DBGetEnableChecksumsFile(_dbfile))
                    P_rdprops = P_ckrdprops;

                /* Read entire variable */
                if (H5Dread(dset, mtype, H5S_ALL, H5S_ALL, P_rdprops, result)<0) {
                    hdf5_to_silo_error(name, me);
                    UNWIND();
                }
            }

            /* Close everything */
            H5Dclose(dset);
            H5Tclose(ftype);
            H5Sclose(space);
        }
        else
        {
            if (!db_hdf5_get_comp_var(_dbfile, name, NULL,
                 NULL, NULL, NULL, &result)) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }
        }

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Dclose(dset);
            H5Tclose(ftype);
            H5Sclose(space);
        } H5E_END_TRY;
        if (result) free(result);
    } END_PROTECT;

    return result;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_ReadVar
 *
 * Purpose:     Reads a variable into the given space.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, February  9, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Feb 15 14:53:29 PST 2005
 *   Forced it to ignore force_single setting 
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_ReadVar(DBfile *_dbfile, char const *vname, void *result)
{
   DBfile_hdf5  *dbfile = (DBfile_hdf5*)_dbfile;
   static char  *me = "db_hdf5_ReadVar";
   hid_t        dset=-1, mtype=-1, ftype=-1;

   PROTECT {

       /* Get dataset and data type */
       if ((dset=H5Dopen(dbfile->cwg, vname, H5P_DEFAULT))>=0) {
           if ((ftype=H5Dget_type(dset))<0) {
               db_perror(vname, E_CALLFAIL, me);
               UNWIND();
           }

           /* Memory data type is based on file data type */
           if ((mtype=hdf2hdf_type(ftype))<0) {
               db_perror("data type", E_BADARGS, me);
               UNWIND();
           }

           P_rdprops = H5P_DEFAULT;
           if (!DBGetEnableChecksumsFile(_dbfile))
               P_rdprops = P_ckrdprops;

           /* Read entire variable */
           if (H5Dread(dset, mtype, H5S_ALL, H5S_ALL, P_rdprops, result)<0) {
               hdf5_to_silo_error(vname, me);
               UNWIND();
           }

           /* Close everything */
           H5Dclose(dset);
           H5Tclose(ftype);
        }
        else
        {
            if (!db_hdf5_get_comp_var(_dbfile, vname, NULL,
                 NULL, NULL, NULL, &result)) {
                db_perror(vname, E_CALLFAIL, me);
                UNWIND();
            }
        }

   } CLEANUP {
       H5E_BEGIN_TRY {
           H5Dclose(dset);
           H5Tclose(ftype);
       } H5E_END_TRY;
   } END_PROTECT;
   
   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_ReadVarSlice
 *
 * Purpose:     Reads a slice of a variable into the given memory. The slice
 *              is described with a multi-dimensional offset, length, and
 *              stride and is translated into an HDF5 hyperslab selection.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, February  9, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_ReadVarSlice(DBfile *_dbfile, char const *vname, int const *offset, int const *length,
                     int const *stride, int ndims, void *result)
{
   DBfile_hdf5  *dbfile = (DBfile_hdf5*)_dbfile;
   static char  *me = "db_hdf5_ReadVarSlice";
   hid_t        dset=-1, ftype=-1, mtype=-1, mspace=-1, fspace=-1;
   hsize_t      mem_size[H5S_MAX_RANK];

   PROTECT {
       /* Get dataset and data type */
       if ((dset=H5Dopen(dbfile->cwg, vname, H5P_DEFAULT))<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }
       if ((ftype=H5Dget_type(dset))<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }

       /* Memory data type is based on file data type */
       if ((mtype=hdf2hdf_type(ftype))<0) {
           db_perror("data type", E_BADARGS, me);
           UNWIND();
       }

       /* Build file selection */
       if ((fspace=build_fspace(dset, ndims, offset, length, stride,
                                mem_size))<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }
       
       /* Build the memory space */
       if ((mspace=H5Screate_simple(ndims, mem_size, NULL))<0) {
           db_perror("memory data space", E_CALLFAIL, me);
           UNWIND();
       }

       P_rdprops = H5P_DEFAULT;
       if (!DBGetEnableChecksumsFile(_dbfile))
           P_rdprops = P_ckrdprops;

       /* Read the data */
       if (H5Dread(dset, mtype, mspace, fspace, P_rdprops, result)<0) {
           hdf5_to_silo_error(vname, me);
           UNWIND();
       }
   
       /* Close everything */
       H5Dclose(dset);
       H5Tclose(ftype);
       H5Sclose(fspace);
       H5Sclose(mspace);
       
   } CLEANUP {
       H5E_BEGIN_TRY {
           H5Dclose(dset);
           H5Tclose(ftype);
           H5Sclose(fspace);
           H5Sclose(mspace);
       } H5E_END_TRY;
   } END_PROTECT;
   
   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_ReadVarVals
 *
 * Purpose:     Reads specific values from a var into memory. The value
 *              indices are tuples, each tuple of ndims values specific
 *              either ijk or linear indices of the values to be read.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer: Mark C. Miller, Mon Apr 18 13:13:30 PDT 2016
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_ReadVarVals(DBfile *_dbfile, char const *vname, int mode,
    int nvals, int ndims, void const *indices, void **result,
    int *ncomps, int *nitems)
{
   DBfile_hdf5  *dbfile = (DBfile_hdf5*)_dbfile;
   static char  *me = "db_hdf5_ReadVarVals";
   hid_t        dset=-1, ftype=-1, mtype=-1, mspace=-1, fspace=-1;
   hsize_t      mem_size;
   int i;

   PROTECT {
       const hsize_t zero = 0;
       int dscount; char **dsnames = 0; char *p;
       hsize_t _dscount, _nvals;

       if (db_hdf5_get_obj_dsnames(_dbfile, vname, &dscount, &dsnames) < 0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }

       if (dscount <= 0) return -1;

       /* We'll get info about the first dataset and assume all datasets are same */
       if ((dset=H5Dopen(dbfile->cwg, dsnames[0], H5P_DEFAULT))<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }

       /* Get dataset type in the file */
       if ((ftype=H5Dget_type(dset))<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }

       /* Memory data type is based on file data type */
       if ((mtype=hdf2hdf_type(ftype))<0) {
           db_perror("data type", E_BADARGS, me);
           UNWIND();
       }

       /* Build file selection */
       if ((fspace=build_fspace_vals(dset, nvals, ndims, indices))<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }
       
       /* Build the memory space */
       mem_size = dscount * nvals;
       if ((mspace=H5Screate_simple(1, &mem_size, NULL))<0) {
           db_perror("memory data space", E_CALLFAIL, me);
           UNWIND();
       }
       _dscount = (hsize_t) dscount;
       _nvals = (hsize_t) nvals;
       H5Sselect_hyperslab(mspace, H5S_SELECT_SET, &zero, &_dscount, &_nvals, 0);

       P_rdprops = H5P_DEFAULT;
       if (!DBGetEnableChecksumsFile(_dbfile))
           P_rdprops = P_ckrdprops;

       /* allocate space for returned array of values */
       if (!*result)
       {
           *result = malloc(nvals*dscount*H5Tget_size(mtype));
           if (!*result)
           {
               db_perror(vname, E_NOMEM, me);
               UNWIND();
           }
       }

       /* Loop to read the equivalent value(s) from each dataset */
       p = (char *) *result;
       for (i = 0; i < dscount; i++)
       {
           /* Open the dataset */
           if ((dset=H5Dopen(dbfile->cwg, dsnames[i], H5P_DEFAULT))<0) {
               db_perror(vname, E_CALLFAIL, me);
               UNWIND();
           }

           /* Read the data */
           if (H5Dread(dset, mtype, mspace, fspace, P_rdprops, p)<0) {
               hdf5_to_silo_error(vname, me);
               UNWIND();
           }

           H5Dclose(dset);
           p += H5Tget_size(mtype);
       }
   
       if (dsnames)
       {
           for (i = 0; i < dscount; i++)
               FREE(dsnames[i]);
           FREE(dsnames);
       }

       /* Close everything */
       H5Tclose(ftype);
       H5Sclose(fspace);
       H5Sclose(mspace);
       
       if (ncomps) *ncomps = dscount;
       if (nitems) *nitems = nvals;
   
   } CLEANUP {
       H5E_BEGIN_TRY {
           H5Dclose(dset);
           H5Tclose(ftype);
           H5Sclose(fspace);
           H5Sclose(mspace);
       } H5E_END_TRY;
   } END_PROTECT;

   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_Write
 *
 * Purpose:     Writes a single variable into a file. The variable may
 *              already exist, in which case the NDIMS and DIMS must match
 *              what has already been defined.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, February  9, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_Write(DBfile *_dbfile, char const *vname, void const *var,
              int const *dims, int ndims, int datatype)
{
    const int nofilters = 0;
    return db_hdf5_WriteCKZ(_dbfile, vname, var, dims, ndims, datatype, nofilters);
}

PRIVATE int
db_hdf5_WriteCKZ(DBfile *_dbfile, char const *vname, void const *var,
              int const *dims, int ndims, int datatype, int nofilters)
{
   DBfile_hdf5  *dbfile = (DBfile_hdf5*)_dbfile;
   static char  *me = "db_hdf5_Write";
   hid_t        mtype=-1, ftype=-1, space=-1, dset=-1, dset_type=-1;
   hsize_t      ds_size[H5S_MAX_RANK], new_ds_size[H5S_MAX_RANK];
   H5T_class_t  fclass, mclass;
   int          i;

   PROTECT {
       /* Create the memory and file data type */
       if ((mtype=silom2hdfm_type(datatype))<0 ||
           (ftype=silof2hdff_type(dbfile, datatype))<0) {
           db_perror("datatype", E_BADARGS, me);
           UNWIND();
       }

       /*
        * If the dataset already exists then make sure that the supplied
        * NDIMS and DIMS match what's already defined; otherwise create a new
        * dataset.
        */
       H5E_BEGIN_TRY {
           dset = H5Dopen(dbfile->cwg, vname, H5P_DEFAULT);
       } H5E_END_TRY;
       if (dset>=0) {
           space = H5Dget_space(dset);
           if (ndims!=H5Sget_simple_extent_ndims(space)) {
               db_perror("ndims", E_BADARGS, me);
               UNWIND();
           }
           H5Sget_simple_extent_dims(space, ds_size, NULL);
           for (i=0; i<ndims; i++) {
               if (ds_size[i]<(hsize_t)dims[i]) {
                   db_perror("dims", E_BADARGS, me);
                   UNWIND();
               }
           }
           /* Now, reset the space for the actual data being written */
           for (i=0; i<ndims; i++) new_ds_size[i] = dims[i];
           H5Sset_extent_simple(space, ndims, new_ds_size, ds_size);
#ifndef _MSC_VER
#warning WHAT IF EXISTING DATASET WAS COMPRESSED
#endif
       } else {
           /* Create memory and file data space (both identical) */
           for (i=0; i<ndims; i++) ds_size[i] = dims[i];
           if ((space=H5Screate_simple(ndims, ds_size, NULL))<0) {
               db_perror("data space", E_CALLFAIL, me);
               UNWIND();
           }

           if (nofilters == 0)
           {
               if (db_hdf5_set_properties(_dbfile, ndims, ds_size) < 0 ) {
                   db_perror("db_hdf5_set_properties", E_CALLFAIL, me);
                   UNWIND();
               }

               /* Create dataset if it doesn't already exist */
               if ((dset=H5Dcreate(dbfile->cwg, vname, ftype, space, H5P_DEFAULT, P_crprops, H5P_DEFAULT))<0) {
                   db_perror(vname, E_CALLFAIL, me);
                   UNWIND();
               }
           }
           else
           {
               /* Create dataset if it doesn't already exist */
               if ((dset=H5Dcreate(dbfile->cwg, vname, ftype, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT))<0) {
                   db_perror(vname, E_CALLFAIL, me);
                   UNWIND();
               }
           }
       }
       
#if HDF5_VERSION_GE(1,8,0)
       /* Do an additional explicit check for compatibile data types.
          This was NOT necessary prior to 1.8. But, 1.8 now permits
          writes of int data to float dataset, for example. */
       if ((dset_type = H5Dget_type(dset))<0 ||
           (fclass = H5Tget_class(dset_type))<0 ||
           (mclass = H5Tget_class(mtype))<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }
#ifndef _MSC_VER
#warning COMPARE TO -1 OR TO NEGATIVE
#endif
       if (dset_type != -1)
           H5Tclose(dset_type);
       if (mclass != fclass) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }
#endif

       /* Write data */
       if (H5Dwrite(dset, mtype, space, space, H5P_DEFAULT, var)<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }

       /* Close everything */
       H5Dclose(dset);
       H5Sclose(space);
   } CLEANUP {
       H5E_BEGIN_TRY {
           H5Dclose(dset);
           H5Sclose(space);
#ifndef _MSC_VER
#warning COMPARE TO -1 OR NEGATIVE
#endif
           if (dset_type != -1)
               H5Tclose(dset_type);
       } H5E_END_TRY;
   } END_PROTECT;
   
   return 0;
}

/*------------------------------------------------------------------------- 
 * Function:    db_hdf5_WriteSlice
 *
 * Purpose:     Similar to db_hdf5_ReadVarSlice() except it writes data
 *              instead.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, February  9, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_WriteSlice(
    DBfile *_dbfile,
    char const *vname,
    void const *values,
    int dtype,
    int const *offset,
    int const *length,
    int const *stride,
    int const *dims,
    int ndims
)
{
   DBfile_hdf5  *dbfile = (DBfile_hdf5*)_dbfile ;
   static char  *me = "db_hdf5_WriteSlice" ;
   hid_t        mtype=-1, ftype=-1, fspace=-1, mspace=-1, dset=-1;
   hsize_t      ds_size[H5S_MAX_RANK];
   int          i;

   PROTECT {
       if ((mtype=silom2hdfm_type(dtype))<0 ||
           (ftype=silof2hdff_type(dbfile, dtype))<0) {
           db_perror("datatype", E_BADARGS, me);
           UNWIND();
       }
       
       /*
        * If the dataset already exists then make sure that the supplied
        * NDIMS and DIMS match what's already defined. Otherwise create the
        * dataset.
        */
       H5E_BEGIN_TRY {
           dset = H5Dopen(dbfile->cwg, vname, H5P_DEFAULT);
       } H5E_END_TRY;
       if (dset>=0) {
           fspace = H5Dget_space(dset);
           if (ndims!=H5Sget_simple_extent_ndims(fspace)) {
               db_perror("ndims", E_BADARGS, me);
               UNWIND();
           }
           H5Sget_simple_extent_dims(fspace, ds_size, NULL);
           for (i=0; i<ndims; i++) {
               if (ds_size[i]!=(hsize_t)dims[i]) {
                   db_perror("dims", E_BADARGS, me);
                   UNWIND();
               }
           }
           H5Sclose(fspace);
       } else {
           for (i=0; i<ndims; i++) ds_size[i] = dims[i];
           if ((fspace=H5Screate_simple(ndims, ds_size, NULL))<0) {
               db_perror("data space", E_CALLFAIL, me);
               UNWIND();
           }

           if (db_hdf5_set_properties(_dbfile, ndims, ds_size) < 0 ) {
               db_perror("db_hdf5_set_properties", E_CALLFAIL, me);
               UNWIND();
           }

           if ((dset=H5Dcreate(dbfile->cwg, vname, ftype, fspace, H5P_DEFAULT,
                               P_crprops, H5P_DEFAULT))<0) {
               db_perror(vname, E_CALLFAIL, me);
               UNWIND();
           }
           H5Sclose(fspace);
       }

       /*
        * Verify that offset and length are compatible with the supplied
        * dimensions.
        */
       for (i=0; i<ndims; i++) {
           if (offset[i]<0 || offset[i]>=dims[i]) {
               db_perror("offset", E_BADARGS, me);
               UNWIND();
           }
           if (length[i]<=0 || length[i]>dims[i]) {
               db_perror("length", E_BADARGS, me);
               UNWIND();
           }
           if (offset[i]+length[i]>dims[i]) {
               db_perror("offset+length", E_BADARGS, me);
               UNWIND();
           }
       }
       
       /* Build the file space selection */
       if ((fspace=build_fspace(dset, ndims, offset, length, stride,
                                ds_size/*out*/))<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }

       /* Build the memory data space */
       if ((mspace=H5Screate_simple(ndims, ds_size, NULL))<0) {
           db_perror("memory data space", E_CALLFAIL, me);
           UNWIND();
       }

       /* Write data */
       if (H5Dwrite(dset, mtype, mspace, fspace, H5P_DEFAULT, values)<0) {
           db_perror(vname, E_CALLFAIL, me);
           UNWIND();
       }

       /* Close everything */
       H5Dclose(dset);
       H5Sclose(fspace);
       H5Sclose(mspace);
   } CLEANUP {
       H5E_BEGIN_TRY {
           H5Dclose(dset);
           H5Sclose(fspace);
           H5Sclose(mspace);
       } H5E_END_TRY;
   } END_PROTECT;
   
   return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetObject
 *
 * Purpose:     Reads information about a silo object.
 *
 * Return:      Success:        Ptr to new object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 23, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *   I removed the seen_datatype functionality
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBobject *
db_hdf5_GetObject(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_GetObject";
    hid_t       o=-1, attr=-1, atype=-1, h5str=-1;
    char        *file_value=NULL, *mem_value=NULL, *bkg=NULL, bigname[1024];
    DBObjectType objtype;
    int         _objtype, nmembs, i, j;
    hsize_t     memb_size[4];
    DBobject    *obj=NULL;
    size_t      asize, nelmts, msize;

    PROTECT {
        /* Open the object as a named data type */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }

        /* Open the `silo_type' attribute and read it */
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        objtype = (DBObjectType)_objtype;
        
        /*
         * Open the `silo' attribute (all silo objects have one), retrieve
         * its data type (a single-level H5T_COMPOUND type), and read the
         * attribute.
         */
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            (atype=H5Aget_type(attr))<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        asize = H5Tget_size(atype);
        msize = MAX(asize, 3*1024);
        if (NULL==(file_value=(char *)calloc(1,asize)) ||
            NULL==(mem_value=(char *)calloc(1,msize)) ||
            NULL==(bkg=(char *)calloc(1,msize))) {
            db_perror(name, E_NOMEM, me);
            UNWIND();
        }
        if (H5Aread(attr, atype, file_value)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        nmembs = H5Tget_nmembers(atype);

        /* Create the empty DBobject */
        if (NULL==(obj=DBMakeObject(name, objtype, 3*nmembs))) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Add members to the DBobject */
#ifndef _MSC_VER
#warning THIS IFDEFD CODE IS IN TRANSITION TO BETTER GENERIC OBJECTS
#endif
        for (i=0; i<nmembs; i++) {
            int ndims = 0;
            hid_t member_type = db_hdf5_get_cmemb(atype, i, &ndims, memb_size);
            char *name = H5Tget_member_name(atype, i);
            hid_t mtype = H5Tcreate(H5T_COMPOUND, msize);
            for (nelmts=1, j=0; j<ndims; j++) nelmts *= memb_size[j];
            
            switch (H5Tget_class(member_type)) {
            case H5T_INTEGER:
                db_hdf5_put_cmemb(mtype, name, 0, ndims, memb_size,
                                  H5T_NATIVE_INT);
                memcpy(mem_value, file_value, H5Tget_size(atype));
                H5Tconvert(atype, mtype, 1, mem_value, bkg, H5P_DEFAULT);
                DBAddIntNComponent(obj, name, nelmts, (int*)mem_value);
                break;

            case H5T_FLOAT:
                db_hdf5_put_cmemb(mtype, name, 0, ndims, memb_size,
                                  H5T_NATIVE_DOUBLE);
                memcpy(mem_value, file_value, H5Tget_size(atype));
                H5Tconvert(atype, mtype, 1, mem_value, bkg, H5P_DEFAULT);
                if (4 == (int) H5Tget_size(member_type))
                {
                    DBAddFltNComponent(obj, name, nelmts, (double*)mem_value);
                }
                else
                {
                    DBAddDblNComponent(obj, name, nelmts, (double*)mem_value);
                }
                break;

            case H5T_STRING:
                h5str = H5Tcopy(H5T_C_S1);
                H5Tset_size(h5str, H5Tget_size(member_type));
                db_hdf5_put_cmemb(mtype, name, 0, ndims, memb_size, h5str);
                memcpy(mem_value, file_value, H5Tget_size(atype));
                H5Tconvert(atype, mtype, 1, mem_value, bkg, H5P_DEFAULT);
                if (1==nelmts) {
                    DBAddStrComponent(obj, name, mem_value);
                } else {
                    for (j=0; (size_t)j<nelmts; j++) {
                        sprintf(bigname, "%s%d", name, j+1);
                        DBAddStrComponent(obj, bigname,
                                          mem_value+j*H5Tget_size(member_type));
                    }
                }
                H5Tclose(h5str);
                h5str = -1;
                break;

            default:
                /* Silo doesn't handle other types */
                break;
            }

            /* Release member resources */
            free(name);
            H5Tclose(mtype);
            H5Tclose(member_type);
        }

#ifndef _MSC_VER
#warning REVISIT THIS CLEANUP CODE. IS THERE A BETTER SYMBOLIC VALUE THAN -1
#endif
        /* Cleanup */
        H5Tclose(atype);
        H5Aclose(attr);
        H5Tclose(o);
#ifndef _MSC_VER
#warning USE FREE() MACRO HERE
#endif
        free(file_value);
        free(mem_value);
        free(bkg);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Tclose(atype);
            H5Tclose(h5str);
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
#ifndef _MSC_VER
#warning USE FREE() MACRO HERE
#endif
        if (file_value) free(file_value);
        if (mem_value) free(mem_value);
        if (bkg) free(bkg);
    } END_PROTECT;

    return obj;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutCurve
 *
 * Purpose:     Put a curve object into the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1, db_errno set
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 23, 1999
 *
 * Modifications:
 *
 *  Thomas R. Treadway, Fri Jul  7 12:44:38 PDT 2006
 *  Added support for DBOPT_REFERENCE in Curves
 *
 *  Mark C. Miller, Mon Jul 31 17:57:29 PDT 2006
 *  Eliminated use of db_hdf5_fullname for xvarname and yvarname
 *
 *  Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *  Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutCurve(DBfile *_dbfile, char const *name, void const *xvals, void const *yvals,
                 int dtype, int npts, DBoptlist const *opts)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_PutCurve";
    DBcurve_mt  m;

    memset(&m, 0, sizeof m);
    PROTECT {
        /* Check datatype */
        if (DB_FLOAT!=dtype && DB_DOUBLE!=dtype) {
            db_perror("invalid floating-point datatype", E_BADARGS, me);
            UNWIND();
        }
        
        /* Reset global curve options */
        memset(&_cu, 0, sizeof _cu);
        _cu._missing_value = DB_MISSING_VALUE_NOT_SET;
        if (0!=db_ProcessOptlist(DB_CURVE, opts)) {
            db_perror("bad options", E_CALLFAIL, me);
            UNWIND();
        }

        /* Write X and Y arrays if supplied */
        if (npts && xvals) {
            if (_cu._varname[0])
                db_hdf5_fullname(dbfile, _cu._varname[0], m.xvarname/*out*/);
            db_hdf5_compwr(dbfile, dtype, 1, &npts, xvals, m.xvarname/*out*/,
                friendly_name(_dbfile,name, "_xvals", 0));
        } else if (_cu._varname[0]) {
            db_hdf5_fullname(dbfile, _cu._varname[0], m.xvarname/*out*/);
        }

        if (npts && yvals) {
            if (_cu._varname[1])
                db_hdf5_fullname(dbfile, _cu._varname[1], m.yvarname/*out*/);
            db_hdf5_compwr(dbfile, dtype, 1, &npts, yvals, m.yvarname/*out*/,
                friendly_name(_dbfile,name, "_yvals", 0));
        } else if (_cu._varname[1]) {
            db_hdf5_fullname(dbfile, _cu._varname[1], m.yvarname/*out*/);
        }

        /* Build the curve header in memory */
        m.npts = npts;
        m.guihide = _cu._guihide;
        m.coord_sys = _cu._coord_sys;
        db_SetMissingValueForPut(m.missing_value, _cu._missing_value);
        strcpy(m.label, OPT(_cu._label));
        strcpy(m.xlabel, OPT(_cu._labels[0]));
        strcpy(m.ylabel, OPT(_cu._labels[1]));
        strcpy(m.xunits, OPT(_cu._units[0]));
        strcpy(m.yunits, OPT(_cu._units[1]));
        strcpy(m.reference, OPT(_cu._reference));

        /* Write curve header to file */
        STRUCT(DBcurve) {
            if (m.npts) MEMBER_S(int, npts);
            if (m.guihide) MEMBER_S(int, guihide);
            if (m.coord_sys) MEMBER_S(int, coord_sys);
            if (m.missing_value) MEMBER_S(double, missing_value);
            MEMBER_S(str(_cu._label), label);
            MEMBER_S(str(m.xvarname), xvarname);
            MEMBER_S(str(m.yvarname), yvarname);
            MEMBER_S(str(_cu._labels[0]), xlabel);
            MEMBER_S(str(_cu._labels[1]), ylabel);
            MEMBER_S(str(_cu._units[0]), xunits);
            MEMBER_S(str(_cu._units[1]), yunits);
            MEMBER_S(str(_cu._reference), reference);
        } OUTPUT(dbfile, DB_CURVE, name, &m);

    } CLEANUP {
        /*void*/;
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetCurve
 *
 * Purpose:     Read a curve object from a PDB data file.
 *
 * Return:      Success:        Pointer to new curve object
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, March 25, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *   Made it set datatype correctly. Added support for dataReadMask
 *
 *  Thomas R. Treadway, Fri Jul  7 12:44:38 PDT 2006
 *  Added support for DBOPT_REFERENCE in Curves
 *
 *   Mark C. Miller, Thu Sep  7 10:50:55 PDT 2006
 *   Added use of db_hdf5_resolvename for retrieval of x-data object
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBcurve *
db_hdf5_GetCurve(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetCurve";
    DBcurve             *cu = NULL;
    DBcurve_mt          m;
    hid_t               o=-1, attr=-1;
    int                 _objtype;

    PROTECT {
        /* Open object and make sure it's a curve */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_CURVE!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read the curve data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBcurve_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a curve object and initialize meta data */
        if (NULL==(cu=DBAllocCurve())) return NULL;
        cu->npts = m.npts;
        cu->guihide = m.guihide;
        cu->coord_sys = m.coord_sys;
        cu->datatype = DB_FLOAT;
        if (strlen(m.xvarname)) {
            if ((cu->datatype = db_hdf5_GetVarType(_dbfile, 
                                db_hdf5_resolvename(_dbfile, name, m.xvarname))) < 0)
                cu->datatype = DB_FLOAT;
        }
        if (force_single_g) cu->datatype = DB_FLOAT;
        cu->title = OPTDUP(m.label);
        if (strncmp(m.xvarname, "/.silo/#", 8))
            cu->xvarname = OPTDUP(m.xvarname);
        if (strncmp(m.yvarname, "/.silo/#", 8))
            cu->yvarname = OPTDUP(m.yvarname);
        cu->xlabel = OPTDUP(m.xlabel);
        cu->ylabel = OPTDUP(m.ylabel);
        cu->xunits = OPTDUP(m.xunits);
        cu->yunits = OPTDUP(m.yunits);
        cu->reference = OPTDUP(m.reference);
        db_SetMissingValueForGet(cu->missing_value, m.missing_value);
        
        /* Read X and Y data */
        if (DBGetDataReadMask2File(_dbfile) & DBCurveArrays)
        {
            if (cu->reference) {
                cu->x = NULL;
                cu->y = NULL;
            } else {
                cu->x = db_hdf5_comprd(dbfile, m.xvarname, 0);
                cu->y = db_hdf5_comprd(dbfile, m.yvarname, 0);
            }
        }
        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeCurve(cu);
    } END_PROTECT;

    return cu;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutCsgmesh
 *
 * Purpose:     Writes a Csgmesh to the silo file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller 
 *              Tuesday, September 6, 2005 
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Jul 31 17:57:29 PDT 2006
 *   Eliminated use of db_hdf5_fullname for zonel_name
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
SILO_CALLBACK int
db_hdf5_PutCsgmesh(DBfile *_dbfile, char const *name, int ndims,
                   int nbounds, int const *typeflags, int const *bndids,
                   void const *coeffs, int lcoeffs, int datatype,
                   double const *extents, char const *zonel_name,
                   DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_PutCsgmesh";
    int                 i;
    DBcsgmesh_mt        m;

    memset(&m, 0, sizeof m); 
    
    PROTECT {

        /* Set global options */
        db_ResetGlobalData_Csgmesh();
        if (db_ProcessOptlist(DB_CSGMESH, optlist)<0) {
            db_perror("bad options", E_CALLFAIL, me);
            UNWIND();
        }

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _csgm._time_set, _csgm._time,
            _csgm._dtime_set, _csgm._dtime, _csgm._cycle);

        if (extents)
        {
            m.min_extents[0] = extents[0];
            m.min_extents[1] = extents[1];
            m.min_extents[2] = extents[2];
            m.max_extents[0] = extents[3];
            m.max_extents[1] = extents[4];
            m.max_extents[2] = extents[5];
        }

        if (nbounds)
        {
            db_hdf5_compwr(dbfile,DB_INT,1,&nbounds,(void*)typeflags,m.typeflags/*out*/, friendly_name(_dbfile,name, "_typeflags", 0));
            if (bndids)
                db_hdf5_compwr(dbfile,DB_INT,1,&nbounds,(void*)bndids,m.bndids/*out*/, friendly_name(_dbfile,name, "_bndids", 0));
            db_hdf5_compwr(dbfile,datatype,1,&lcoeffs,(void*)coeffs,m.coeffs/*out*/, friendly_name(_dbfile,name, "_coeffs", 0));
        }

        /* Build csgmesh header in memory */
        m.ndims = ndims;
        m.cycle = _csgm._cycle;
        m.origin = _csgm._origin;
        m.group_no = _csgm._group_no;
        m.guihide = _csgm._guihide;
        strcpy(m.name, name);
        for (i=0; i<ndims; i++) {
            strcpy(m.labels[i], OPT(_csgm._labels[i]));
            strcpy(m.units[i], OPT(_csgm._units[i]));
        }
        m.time = _csgm._time_set ? _csgm._time : 0;
        m.dtime = _csgm._dtime_set ? _csgm._dtime : 0;
        m.nbounds = nbounds;
        m.lcoeffs = lcoeffs;
        m.tv_connectivity = _csgm._tv_connectivity;
        m.disjoint_mode = _csgm._disjoint_mode;
        if (zonel_name)
            strcpy(m.zonel_name, zonel_name);
        strcpy(m.mrgtree_name, OPT(_csgm._mrgtree_name));

        if (_csgm._alt_nodenum_vars != NULL) {
            char *s; int len;
            DBStringArrayToStringList((char const * const *)_csgm._alt_nodenum_vars, -1, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.alt_nodenum_vars/*out*/,
                friendly_name(_dbfile,name, "_alt_nodenum_vars", 0));
            FREE(s);
        }

        /* Build csgmesh header in file */
        STRUCT(DBcsgmesh) {
            if (m.group_no)       MEMBER_S(int, group_no);
            if (m.cycle)          MEMBER_S(int, cycle);
            if (_csgm._time_set)  MEMBER_S(float, time);
            if (_csgm._dtime_set) MEMBER_S(double, dtime);
            if (m.origin)         MEMBER_S(int, origin);
            if (bndids)           MEMBER_S(str(m.bndids), bndids);
            if (m.guihide)        MEMBER_S(int, guihide);
            MEMBER_S(int, lcoeffs);
            MEMBER_S(int, nbounds);
            MEMBER_S(int, ndims);
            MEMBER_3(double, min_extents);
            MEMBER_3(double, max_extents);
            MEMBER_R(str(m.units[_j]), units, ndims);
            MEMBER_R(str(m.labels[_j]), labels, ndims);
            MEMBER_S(str(m.typeflags), typeflags);
            MEMBER_S(str(m.coeffs), coeffs);
            MEMBER_S(str(m.zonel_name), zonel_name);
            MEMBER_S(str(m.mrgtree_name), mrgtree_name);
            if (m.tv_connectivity) MEMBER_S(int, tv_connectivity);
            if (m.disjoint_mode)   MEMBER_S(int, disjoint_mode);
            MEMBER_S(str(m.alt_nodenum_vars), alt_nodenum_vars);
        } OUTPUT(dbfile, DB_CSGMESH, name, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetCsgmesh
 *
 * Purpose:     Reads a CSG mesh object from the file.
 *
 * Return:      Success:        Pointer to a new csgmesh.
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller 
 *              Wednesday, September 7, 2005 
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Sep  7 10:50:55 PDT 2006
 *   Added use of db_hdf5_resolvename for retrieval of subobjects
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBcsgmesh *
db_hdf5_GetCsgmesh(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetCsgmesh";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i;
    DBcsgmesh_mt        m;
    DBcsgmesh           *csgm=NULL;

    PROTECT {
        /* Open object and make sure it's a csgmesh */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror((char*)name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_CSGMESH!=(DBObjectType)_objtype) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read header into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBcsgmesh_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a ucdmesh object and initialize meta data */
        if (NULL==(csgm=DBAllocCsgmesh())) return NULL;
        csgm->name = BASEDUP(name);
        csgm->cycle = m.cycle;
        if ((csgm->datatype = db_hdf5_GetVarType(_dbfile, m.coeffs)) < 0)
            csgm->datatype = DB_FLOAT;
        if (force_single_g) csgm->datatype = DB_FLOAT;
        csgm->time = m.time;
        csgm->dtime = m.dtime;
        csgm->ndims = m.ndims;
        csgm->nbounds = m.nbounds;
        csgm->lcoeffs = m.lcoeffs;
        csgm->origin = m.origin;
        csgm->group_no = m.group_no;
        csgm->guihide = m.guihide;
        for (i=0; i<m.ndims; i++) {
            csgm->units[i] = OPTDUP(m.units[i]);
            csgm->labels[i] = OPTDUP(m.labels[i]);
            csgm->min_extents[i] = m.min_extents[i];
            csgm->max_extents[i] = m.max_extents[i];
        }
        csgm->mrgtree_name = OPTDUP(m.mrgtree_name);
        csgm->tv_connectivity = m.tv_connectivity;
        csgm->disjoint_mode = m.disjoint_mode;

        /* Read the raw data */
        if ((DBGetDataReadMask2File(_dbfile) & DBCSGMBoundaryInfo) && (m.nbounds > 0))
        {
            csgm->typeflags = (int *)db_hdf5_comprd(dbfile, m.typeflags, 1);
            csgm->bndids = (int *)db_hdf5_comprd(dbfile, m.bndids, 1);
        }

        if ((DBGetDataReadMask2File(_dbfile) & DBCSGMBoundaryNames) && (m.nbounds > 0))
        {
            char *tmpbndnames = (char *)db_hdf5_comprd(dbfile, m.bndnames, 1);
            if (tmpbndnames)
                csgm->bndnames = DBStringListToStringArray(tmpbndnames, &m.nbounds, !skipFirstSemicolon);
            FREE(tmpbndnames);
        }

        if ((DBGetDataReadMask2File(_dbfile) & DBCSGMBoundaryInfo) && (m.lcoeffs > 0))
            csgm->coeffs = db_hdf5_comprd(dbfile, m.coeffs, 0);

        if ((m.nbounds>0 && m.zonel_name[0] && (DBGetDataReadMask2File(_dbfile) & DBCSGMZonelist)))
            csgm->zones = db_hdf5_GetCSGZonelist(_dbfile, 
                              db_hdf5_resolvename(_dbfile, name, m.zonel_name));

        /* alt nodenum vars */
        {
            int nnames = -1;
            char *tmpannums = (char *)db_hdf5_comprd(dbfile, m.alt_nodenum_vars, 1);
            if (tmpannums)
                csgm->alt_nodenum_vars = DBStringListToStringArray(tmpannums, &nnames, !skipFirstSemicolon);
            FREE(tmpannums);
        }

        H5Tclose(o);
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
    } END_PROTECT;

    return csgm;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutCsgvar
 *
 * Purpose:     Writes CSG variables to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller 
 *              Wednesday, September 7, 2005 
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
SILO_CALLBACK int
db_hdf5_PutCsgvar(DBfile *_dbfile, char const *vname, char const *meshname,
                  int nvars, char const * const *varnames, void const * const *vars,
                  int nvals, int datatype, int centering, DBoptlist const *optlist)

{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_PutCsgvar";
    char                *s = 0;
    DBcsgvar_mt         m;
    int                 i; 

    memset(&m, 0, sizeof m);

    PROTECT {
        db_ResetGlobalData_Csgmesh();
        db_ProcessOptlist(DB_CSGMESH, optlist);

        /* Write variable arrays: vars[] */
        if (nvars>MAX_VARS) {
            db_perror("too many variables", E_BADARGS, me);
            UNWIND();
        }
        for (i=0; i<nvars && nvals; i++) {
            db_hdf5_compwr(dbfile, datatype, 1, &nvals, (void*)vars[i],
                m.vals[i]/*out*/, friendly_name(_dbfile,varnames[i], "_data", 0));
        }

        /* output mrgtree info if we have it */
        if (_csgm._region_pnames != NULL) {
            int len;
            DBStringArrayToStringList((char const * const *)_csgm._region_pnames, -1, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.region_pnames/*out*/,
                friendly_name(_dbfile,vname, "_region_pnames", 0));
            FREE(s);
        }

        /* Build header in memory */
        m.nvals = nvars;
        m.nels = nvals;
        m.centering = centering;
        m.cycle = _csgm._cycle;
        m.guihide = _csgm._guihide;
        if (_csgm._time_set)
            m.time = _csgm._time;
        if (_csgm._dtime_set)
            m.dtime = _um._dtime;
        m.use_specmf = _um._use_specmf;
        m.datatype = datatype;
        m.conserved = _csgm._conserved;
        m.extensive = _csgm._extensive;
        strcpy(m.meshname, OPT(meshname));
        strcpy(m.label, OPT(_csgm._label));
        strcpy(m.units, OPT(_csgm._unit));
        db_SetMissingValueForPut(m.missing_value, _csgm._missing_value);

        /* Write header to file */
        STRUCT(DBcsgvar) {
            MEMBER_R(str(m.vals[_j]), vals, nvars);
            MEMBER_S(str(m.meshname), meshname);
            MEMBER_S(int, cycle);
            MEMBER_S(str(m.label), label);
            MEMBER_S(str(m.units), units);
            if (m.nvals)        MEMBER_S(int, nvals);
            if (m.nels)         MEMBER_S(int, nels);
            if (m.centering)    MEMBER_S(int, centering);
            if (m.use_specmf)   MEMBER_S(int, use_specmf);
            if (m.datatype)     MEMBER_S(int, datatype);
            if (m.guihide)      MEMBER_S(int, guihide);
            if (_csgm._time_set)  MEMBER_S(float, time);
            if (_csgm._dtime_set) MEMBER_S(double, dtime);
            if (m.conserved)    MEMBER_S(int, conserved);
            if (m.extensive)    MEMBER_S(int, extensive);
            if (m.missing_value) MEMBER_S(double, missing_value);
            MEMBER_S(str(m.region_pnames), region_pnames);
        } OUTPUT(dbfile, DB_CSGVAR, vname, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetCsgvar
 *
 * Purpose:     Reads a CSG variable object from the file.
 *
 * Return:      Success:        Ptr to new CSG variable object
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller
 *              Wednesday, September 7, 2005 
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBcsgvar *
db_hdf5_GetCsgvar(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetCsgvar";
    char                *s = 0;
    hid_t               o=-1, attr=-1;
    int                 _objtype, i;
    DBcsgvar_mt         m;
    DBcsgvar           *csgv=NULL;

    PROTECT {
        /* Open object and make sure it's a ucdvar */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror((char*)name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_CSGVAR!=(DBObjectType)_objtype) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read ucdvar data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBcsgvar_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a ucdvar object and initialize meta data */
        if (NULL==(csgv=DBAllocCsgvar())) return NULL;
        csgv->name = BASEDUP(name);
        csgv->meshname = OPTDUP(m.meshname);
        csgv->cycle = m.cycle;
        csgv->units = OPTDUP(m.units);
        csgv->label = OPTDUP(m.label);
        csgv->time = m.time;
        csgv->dtime = m.dtime;
        if ((csgv->datatype = db_hdf5_GetVarType(_dbfile, m.vals[0])) < 0)
            csgv->datatype = silo2silo_type(m.datatype);
        if (force_single_g) csgv->datatype = DB_FLOAT;
        csgv->nels = m.nels;
        csgv->nvals = m.nvals;
        csgv->centering = m.centering;
        csgv->use_specmf = m.use_specmf;
        csgv->ascii_labels = m.ascii_labels;
        csgv->guihide = m.guihide;
        csgv->conserved = m.conserved;
        csgv->extensive = m.extensive;
        db_SetMissingValueForGet(csgv->missing_value, m.missing_value);

        /* Read the raw data */
        if (m.nvals>MAX_VARS) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        if ((DBGetDataReadMask2File(_dbfile) & DBCSGVData) && m.nvals)
        {
            csgv->vals = (void **)calloc(m.nvals, sizeof(void*));
            for (i=0; i<m.nvals; i++) {
                csgv->vals[i] = db_hdf5_comprd(dbfile, m.vals[i], 0);
            }
        }

        s = (char *)db_hdf5_comprd(dbfile, m.region_pnames, 1);
        if (s) csgv->region_pnames = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
        FREE(s);

        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeCsgvar(csgv);
    } END_PROTECT;

    return csgv;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutCSGZonelist
 *
 * Purpose:     Write a DBcsgzonelist object into the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller 
 *              Wednesday, September 7, 2005 
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutCSGZonelist(DBfile *_dbfile, char const *name, int nregs,
                 int const *typeflags,
                 int const *leftids, int const *rightids,
                 void const *xforms, int lxforms, int datatype,
                 int nzones, int const *zonelist, DBoptlist const *optlist)

{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBcsgzonelist_mt     m;
    static char         *me = "db_hdf5_PutCSGZonelist";

    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        memset(&_csgzl, 0, sizeof _csgzl);
        if (db_ProcessOptlist(DB_CSGZONELIST, optlist)<0) {
            db_perror("bad options", E_CALLFAIL, me);
            UNWIND();
        }
        
        /* Write variable arrays */
        db_hdf5_compwr(dbfile, DB_INT, 1, &nregs, (void*)typeflags,
            m.typeflags/*out*/, friendly_name(_dbfile,name, "_typeflags",0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &nregs, (void*)leftids,
            m.leftids/*out*/, friendly_name(_dbfile,name, "_leftids",0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &nregs, (void*)rightids,
            m.rightids/*out*/, friendly_name(_dbfile,name, "_rightids",0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &nzones, (void*)zonelist,
            m.zonelist/*out*/, friendly_name(_dbfile,name, "_zonelist",0));
        if (xforms && lxforms > 0) {
            db_hdf5_compwr(dbfile, datatype, 1, &lxforms, (void*)xforms,
                m.xform/*out*/, friendly_name(_dbfile,name, "_xforms",0));
        }

        if (_csgzl._regnames) {
            int len; char *tmp;
            DBStringArrayToStringList((char const * const *)_csgzl._regnames, nregs, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.regnames/*out*/, friendly_name(_dbfile,name, "_regnames",0));
            FREE(tmp);
        }

        if (_csgzl._zonenames) {
            int len; char *tmp;
            DBStringArrayToStringList((char const * const *)_csgzl._zonenames, nzones, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.zonenames/*out*/, friendly_name(_dbfile,name, "_zonenames",0));
            FREE(tmp);
        }

        if (_csgzl._alt_zonenum_vars) {
            int len, nvars=-1; char *tmp;
            DBStringArrayToStringList((char const * const *)_csgzl._alt_zonenum_vars, nvars, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.alt_zonenum_vars/*out*/, friendly_name(_dbfile,name, "_alt_zonenum_vars",0));
            FREE(tmp);
        }

        /* Build header in memory */
        m.nregs = nregs;
        m.lxform = lxforms;
        m.nzones = nzones;
        if (nregs == 0 && nzones == 0) m.origin = 1;

        /* Write header to file */
        STRUCT(DBcsgzonelist) {
            if (m.nregs)        MEMBER_S(int, nregs);
            if (m.lxform)       MEMBER_S(int, lxform);
            if (m.nzones)       MEMBER_S(int, nzones);
            if (m.origin)       MEMBER_S(int, origin);
            MEMBER_S(str(m.typeflags), typeflags);
            MEMBER_S(str(m.leftids), leftids);
            MEMBER_S(str(m.rightids), rightids);
            MEMBER_S(str(m.zonelist), zonelist);
            MEMBER_S(str(m.xform), xform);
            MEMBER_S(str(m.regnames), regnames);
            MEMBER_S(str(m.zonenames), zonenames);
            MEMBER_S(str(m.alt_zonenum_vars), alt_zonenum_vars);
        } OUTPUT(dbfile, DB_CSGZONELIST, name, &m);
        
    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetCSGZonelist
 *
 * Purpose:     Reads a CSG zonelist object from the file
 *
 * Return:      Success:        Ptr to new CSG zonelist
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller 
 *              Wednesday, September 7, 2005 
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Mar 23 15:31:50 PST 2006
 *   Fixed case where we were forcing single on an integer array for the
 *   zonelist.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBcsgzonelist *
db_hdf5_GetCSGZonelist(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetCSGZonelist";
    hid_t               o=-1, attr=-1;
    int                 _objtype;
    DBcsgzonelist_mt     m;
    DBcsgzonelist       *zl=NULL;

    PROTECT {
        /* Open object and make sure it's a zonelist */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror((char*)name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_CSGZONELIST!=(DBObjectType)_objtype) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read zonelist data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBcsgzonelist_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a zonelist object and initialize meta data */
        if (NULL==(zl=DBAllocCSGZonelist())) return NULL;
        zl->nregs = m.nregs;
        zl->nzones = m.nzones;
        zl->lxform = m.lxform;
        if ((zl->datatype = db_hdf5_GetVarType(_dbfile, m.xform)) < 0)
            zl->datatype = DB_FLOAT;
        if (force_single_g) zl->datatype = DB_FLOAT;

        /* Read the raw data */
        if (DBGetDataReadMask2File(_dbfile) & DBZonelistInfo)
        {
            zl->typeflags = (int *)db_hdf5_comprd(dbfile, m.typeflags, 1);
            zl->leftids = (int *)db_hdf5_comprd(dbfile, m.leftids, 1);
            zl->rightids = (int *)db_hdf5_comprd(dbfile, m.rightids, 1);
            zl->xform = db_hdf5_comprd(dbfile, m.xform, 0);
            zl->zonelist = (int *)db_hdf5_comprd(dbfile, m.zonelist, 1);
        }

        if (DBGetDataReadMask2File(_dbfile) & DBCSGZonelistRegNames)
        {
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, m.regnames, 1);
            if (tmpnames)
                zl->regnames = DBStringListToStringArray(tmpnames, &m.nregs, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        if (DBGetDataReadMask2File(_dbfile) & DBCSGZonelistZoneNames)
        {
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, m.zonenames, 1);
            if (tmpnames)
                zl->zonenames = DBStringListToStringArray(tmpnames, &m.nzones, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        /* alternate zone number variables */
        {
            int nvars = -1;
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, m.alt_zonenum_vars, 1);
            if (tmpnames)
                zl->alt_zonenum_vars = DBStringListToStringArray(tmpnames, &nvars, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        H5Tclose(o);
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeCSGZonelist(zl);
    } END_PROTECT;

    return zl;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutDefvars
 *
 * Purpose:     Put a defvars object into the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1, db_errno set
 *
 * Programmer:  Mark C. Miller 
 *              Tuesday, September 6, 2005 
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutDefvars(
    DBfile *_dbfile,
    char const *name,
    int ndefs,
    char const * const *names,
    int const *types,
    char const * const *defns,
    DBoptlist const * const *opts
)
{
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    static char *me = "db_hdf5_PutDefvars";
    DBdefvars_mt  m;
    int len;
    char *s;
    int *guihide = NULL;

   /*
    * Optlists are a little funky for this object because we were
    * concerned about possibly handling things like units, etc. So,
    * we really have an array of optlists that needs to get serialized.
    */
   if (opts)
   {
       int i;
       for (i = 0; i < ndefs; i++)
       {
           memset(&_dv, 0, sizeof(_dv)); /* reset global dv data */
           db_ProcessOptlist (DB_DEFVARS, opts[i]);
           if (_dv._guihide)
           {
               if (guihide == NULL)
                   guihide = (int *) calloc(ndefs, sizeof(int));
               guihide[i] = _dv._guihide;
           }
       }
   }

    memset(&m, 0, sizeof m);
    PROTECT {

        DBStringArrayToStringList((char const * const *)names, ndefs, &s, &len);
        db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.names/*out*/,
            friendly_name(_dbfile,name, "_names",0));
        FREE(s);

        db_hdf5_compwr(dbfile, DB_INT, 1, &ndefs, (void*)types, m.types/*out*/,
            friendly_name(_dbfile,name, "_types",0));

        DBStringArrayToStringList((char const * const *)defns, ndefs, &s, &len);
        db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.defns/*out*/,
            friendly_name(_dbfile,name, "_defns",0));
        FREE(s);

        if (guihide)
        {
            db_hdf5_compwr(dbfile, DB_INT, 1, &ndefs, (void*)guihide, m.guihides/*out*/,
                friendly_name(_dbfile,name, "_guihids",0));
            free(guihide);
        }

        /* Build the curve header in memory */
        m.ndefs = ndefs;

        /* Write curve header to file */
        STRUCT(DBdefvars) {
            if (m.ndefs) MEMBER_S(int, ndefs);
            MEMBER_S(str(m.names), names);
            MEMBER_S(str(m.types), types);
            MEMBER_S(str(m.defns), defns);
            MEMBER_S(str(m.guihides), guihides);
        } OUTPUT(dbfile, DB_DEFVARS, name, &m);

    } CLEANUP {
        if (guihide) free(guihide);
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetDefvars
 *
 * Purpose:     Read a defvars object from a data file.
 *
 * Return:      Success:        Pointer to new defvars object
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, March 25, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *   Made it set datatype correctly. Added support for dataReadMask
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBdefvars*
db_hdf5_GetDefvars(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetDefvars";
    DBdefvars           *defv = NULL;
    DBdefvars_mt         m;
    hid_t               o=-1, attr=-1;
    int                 _objtype;
    char               *s;

    PROTECT {
        /* Open object and make sure it's a curve */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror((char*)name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_DEFVARS!=(DBObjectType)_objtype) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read the data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBdefvars_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a defvars object and initialize meta data */
        if (NULL==(defv=DBAllocDefvars(0))) return NULL;
        defv->ndefs = m.ndefs;

        s = (char *)db_hdf5_comprd(dbfile, m.names, 1);
        if (s) defv->names = DBStringListToStringArray(s, &(defv->ndefs), !skipFirstSemicolon);
        FREE(s);

        defv->types = (int *)db_hdf5_comprd(dbfile, m.types, 1);

        s = (char *)db_hdf5_comprd(dbfile, m.defns, 1);
        if (s) defv->defns = DBStringListToStringArray(s, &(defv->ndefs), !skipFirstSemicolon);
        FREE(s);

        defv->guihides = (int *)db_hdf5_comprd(dbfile, m.guihides, 1);

        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeDefvars(defv);
    } END_PROTECT;

    return defv;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForQuadmeshCompression
 *
 * Purpose:     Does some small work to prepare for possible mesh
 *              compression 
 *
 * Return:      Success:        ALLOW_MESH_COMPRESSION 
 *
 *              Failure:        0 
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *-------------------------------------------------------------------------
 */
static int PrepareForQuadmeshCompression(DBfile *_dbfile)
{
    if (DBGetCompressionFile(_dbfile) == 0) return 0;

#ifdef HAVE_HZIP
    db_hdf5_hzip_clear_params();
    db_hdf5_hzip_params.isquad = 1;
    return ALLOW_MESH_COMPRESSION;
#endif
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutQuadmesh
 *
 * Purpose:     Writes a Quadmesh to the silo file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Monday, March 29, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `group_no' and `baseindex[]' properties to duplicate
 *              changes made to the PDB driver.
 *
 *              Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *              Made it set the correct mesh type
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Jul 17 15:07:49 PDT 2008
 *   Added call to prepare for mesh compression. Changed call for
 *   non-collinear coordinate case to use db_hdf5_compwrz to support
 *   possible compression.
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
SILO_CALLBACK int
db_hdf5_PutQuadmesh(DBfile *_dbfile, char const *name, char const * const *coordnames,
                    DBVCP2_t _coords, int const *dims, int ndims, int datatype,
                    int coordtype, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_PutQuadmesh";
    int                 i, is_empty = 1;
    DBquadmesh_mt       m;
    int                 compressionFlags;
    void const * const *coords = (void const * const *) _coords;

    memset(&_qm, 0, sizeof _qm);
    memset(&m, 0, sizeof m); 

    for (i = 0; i < ndims; i++)
    {
        if (dims[i] > 0)
        {
            is_empty = 0;
            break;
        }
    }
    
    PROTECT {
        /* Check datatype */
        if (DB_FLOAT!=datatype && DB_DOUBLE!=datatype) {
            db_perror("invalid floating-point datatype", E_BADARGS, me);
            UNWIND();
        }

        /* Set global options */
        _qm._coord_sys = DB_OTHER;
        _qm._facetype = DB_RECTILINEAR;
        _qm._ndims = ndims;
        _qm._nspace = ndims;
        _qm._planar = DB_AREA;
        _qm._use_specmf = DB_OFF;
        _qm._group_no = -1;
        if (db_ProcessOptlist(DB_QUADMESH, optlist)<0) {
            db_perror("bad options", E_CALLFAIL, me);
            UNWIND();
        }

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _qm._time_set, _qm._time,
            _qm._dtime_set, _qm._dtime, _qm._cycle);

        /*
         * Number of zones and nodes. We have to do this because
         * _DBQMCalcExtents uses this global information.
         */
        for (_qm._nzones=_qm._nnodes=(is_empty?0:1), i=0; i<ndims; i++) {
            _qm._nzones *= (dims[i]-1);
            _qm._nnodes *= dims[i];
            _qm._dims[i] = dims[i];
            _qm._zones[i] = dims[i]-1;
            _qm._minindex[i] = _qm._lo_offset[i];
            _qm._maxindex_n[i] = dims[i] - _qm._hi_offset[i] - 1;
            _qm._maxindex_z[i] = _qm._maxindex_n[i] - 1;
        }
        
        /* Calculate extents, stored as DB_DOUBLE */
        if (DB_DOUBLE==datatype) {
            int ii;
            for (ii = 0; ii < 3; ii++)
                m.min_extents[ii] = m.max_extents[ii] = 0;
            _DBQMCalcExtents(coords, datatype, _qm._minindex, _qm._maxindex_n,
                             dims, ndims, coordtype,
                             &(m.min_extents)/*out*/,
                             &(m.max_extents)/*out*/);
        } else {
            float       min_extents[3] = {0,0,0};
            float       max_extents[3] = {0,0,0};
            _DBQMCalcExtents(coords, datatype, _qm._minindex, _qm._maxindex_n,
                             dims, ndims, coordtype,
                             min_extents/*out*/, max_extents/*out*/);
            for (i=0; i<ndims; i++) {
                m.min_extents[i] = min_extents[i];
                m.max_extents[i] = max_extents[i];
            }
        }
        for (i=0; i<ndims; i++) {
            m.min_index[i] = _qm._minindex[i];
            m.max_index[i] = _qm._maxindex_n[i];
        }

        compressionFlags = PrepareForQuadmeshCompression(_dbfile);
        
        /* Write coordinate arrays */
        if (!is_empty)
        {
            if (DB_COLLINEAR==coordtype) {
                for (i=0; i<ndims; i++) {
                    db_hdf5_compwr(dbfile, datatype, 1, dims+i, coords[i],
                        m.coord[i]/*out*/, friendly_name(_dbfile,name, "_coord%d",&i));
                }
            } else {
                for (i=0; i<ndims; i++) {
                    db_hdf5_compwrz(dbfile, datatype, ndims, dims, coords[i],
                        m.coord[i]/*out*/, friendly_name(_dbfile,name, "_coord%d",&i),
                        compressionFlags);
                }
            }
        }
        
        if (!is_empty && _qm._ghost_node_labels)
        {
            db_hdf5_compwr(dbfile, DB_CHAR, ndims, dims, _qm._ghost_node_labels,
                    m.ghost_node_labels/*out*/, friendly_name(_dbfile,name, "_ghost_node_labels", 0));
        }
        if (!is_empty && _qm._ghost_zone_labels)
        {
            int tmpdims[3];
            for (i = 0; i < ndims; i++)
                tmpdims[i] = dims[i]-1;

            db_hdf5_compwr(dbfile, DB_CHAR, ndims, tmpdims, _qm._ghost_zone_labels,
                    m.ghost_zone_labels/*out*/, friendly_name(_dbfile,name, "_ghost_zone_labels", 0));
        }

        if (_qm._alt_nodenum_vars) {
            int len, nvars=-1; char *tmp;
            DBStringArrayToStringList((char const * const *)_qm._alt_nodenum_vars, nvars, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.alt_nodenum_vars/*out*/, friendly_name(_dbfile,name, "_alt_nodenum_vars",0));
            FREE(tmp);
        }

        if (_qm._alt_zonenum_vars) {
            int len, nvars=-1; char *tmp;
            DBStringArrayToStringList((char const * const *)_qm._alt_zonenum_vars, nvars, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.alt_zonenum_vars/*out*/, friendly_name(_dbfile,name, "_alt_zonenum_vars",0));
            FREE(tmp);
        }

        /* Build quadmesh header in memory */
        m.ndims = ndims;
        m.coordtype = coordtype;
        m.nspace = _qm._nspace;
        m.nnodes = _qm._nnodes;
        m.facetype = _qm._facetype;
        m.major_order = _qm._majororder;
        m.cycle = _qm._cycle;
        m.coord_sys = _qm._coord_sys;
        m.planar = _qm._planar;
        m.origin = _qm._origin;
        m.group_no = _qm._group_no;
        m.guihide = _qm._guihide;
        for (i=0; i<ndims; i++) {
            m.dims[i] = dims[i];
            m.baseindex[i] = _qm._baseindex[i];
            strcpy(m.label[i], OPT(_qm._labels[i]));
            strcpy(m.units[i], OPT(_qm._units[i]));
        }
        m.time = _qm._time_set ? _qm._time : 0;
        m.dtime = _qm._dtime_set ? _qm._dtime : 0;
        strcpy(m.mrgtree_name, OPT(_qm._mrgtree_name));

        /* Build quadmesh header in file */
        STRUCT(DBquadmesh) {
            MEMBER_R(str(m.coord[_j]), coord, ndims);
            MEMBER_3(double, min_extents);
            MEMBER_3(double, max_extents);
            MEMBER_S(int, ndims);
            MEMBER_S(int, coordtype);
            MEMBER_S(int, nspace);
            MEMBER_S(int, nnodes);
            MEMBER_S(int, facetype);
            if (m.major_order)  MEMBER_S(int, major_order);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (m.origin)       MEMBER_S(int, origin);
            if (m.group_no)     MEMBER_S(int, group_no);
            if (m.guihide)      MEMBER_S(int, guihide);
            MEMBER_S(int, coord_sys);
            MEMBER_S(int, planar);
            MEMBER_3(int, dims);
            MEMBER_3(int, min_index);
            MEMBER_3(int, max_index);
            MEMBER_3(int, baseindex);
            if (_qm._time_set) MEMBER_S(float, time);
            if (_qm._dtime_set) MEMBER_S(double, dtime);
            MEMBER_R(str(m.label[_j]), label, ndims);
            MEMBER_R(str(m.units[_j]), units, ndims);
            MEMBER_S(str(m.mrgtree_name), mrgtree_name);
            MEMBER_S(str(m.ghost_node_labels), ghost_node_labels);
            MEMBER_S(str(m.ghost_zone_labels), ghost_zone_labels);
            MEMBER_S(str(m.alt_nodenum_vars), alt_nodenum_vars);
            MEMBER_S(str(m.alt_zonenum_vars), alt_zonenum_vars);
        } OUTPUT(dbfile, coordtype == DB_COLLINEAR ? DB_QUADRECT : DB_QUADCURV, name, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForQuadmeshDeompression
 *
 * Purpose:     Does some small work to prepare for possible mesh
 *              decompression 
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *-------------------------------------------------------------------------
 */
static void
PrepareForQuadmeshDecompression(DBfile_hdf5 *dbfile, char const *meshname,
    const DBquadmesh *qm)
{
#ifdef HAVE_HZIP
    int i;
    db_hdf5_hzip_clear_params();
    db_hdf5_hzip_params.dbfile = dbfile;
    db_hdf5_hzip_params.isquad = 1;
    db_hdf5_hzip_params.meshname = meshname;
    db_hdf5_hzip_params.ndims = qm->ndims;
    for (i = 0; i < qm->ndims; i++)
        db_hdf5_hzip_params.dims[i] = qm->dims[i];
#endif
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetQuadmesh
 *
 * Purpose:     Reads a quadmesh object from the file.
 *
 * Return:      Success:        Ptr to new quadmesh
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 30, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `group_no' and `baseindex[]' properties to duplicate
 *              changes made to the PDB driver.
 *
 *              Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *              Added support for dataReadMask
 *
 *              Mark C. Miller, Thu Jul 17 15:09:51 PDT 2008
 *              Added call to prepare for mesh decompression.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBquadmesh *
db_hdf5_GetQuadmesh(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetQuadmesh";
    DBquadmesh          *qm = NULL;
    DBquadmesh_mt       m;
    hid_t               o=-1, attr=-1;
    int                 _objtype, stride, i;
    
    PROTECT {
        /* Open object and make sure it's a quadmesh */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if ((DB_QUADMESH!=(DBObjectType)_objtype) &&
            (DB_QUAD_CURV!=(DBObjectType)_objtype) &&
            (DB_QUAD_RECT!=(DBObjectType)_objtype)) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read quadmesh data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBquadmesh_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a quadmesh object and initialize meta data */
        if (NULL==(qm=DBAllocQuadmesh())) return NULL;
        qm->name = BASEDUP(name);
        qm->cycle = m.cycle;
        qm->coord_sys = m.coord_sys;
        qm->major_order = m.major_order;
        qm->coordtype = m.coordtype;
        qm->facetype = m.facetype;
        qm->planar = m.planar;
        if ((qm->datatype = db_hdf5_GetVarType(_dbfile, m.coord[0])) < 0)
            qm->datatype = DB_FLOAT;
        if (force_single_g) qm->datatype = DB_FLOAT;
        qm->time = m.time;
        qm->dtime = m.dtime;
        qm->ndims = m.ndims;
        qm->nspace = m.nspace;
        qm->nnodes = m.nnodes;
        qm->origin = m.origin;
        qm->group_no = m.group_no;
        qm->guihide = m.guihide;
        qm->mrgtree_name = OPTDUP(m.mrgtree_name);

        for (stride=1, i=0; i<qm->ndims; i++) {
            if (qm->datatype == DB_DOUBLE)
            {
                ((double*)qm->min_extents)[i] = m.min_extents[i];
                ((double*)qm->max_extents)[i] = m.max_extents[i];
            }
            else
            {
                qm->min_extents[i] = m.min_extents[i];
                qm->max_extents[i] = m.max_extents[i];
            }
            qm->labels[i] = OPTDUP(m.label[i]);
            qm->units[i] = OPTDUP(m.units[i]);
            qm->dims[i] = m.dims[i];
            qm->min_index[i] = m.min_index[i];
            qm->max_index[i] = m.max_index[i];
            qm->base_index[i] = m.baseindex[i];
            qm->stride[i] = stride;
            stride *= qm->dims[i];
        }

        PrepareForQuadmeshDecompression(dbfile, name, qm);

        /* Read coordinate arrays */
        for (i=0; i<qm->ndims; i++) {
            if (DBGetDataReadMask2File(_dbfile) & DBQMCoords)
                qm->coords[i] = db_hdf5_comprd(dbfile, m.coord[i], 0);
        }

        if (DBGetDataReadMask2File(_dbfile) & DBQMGhostNodeLabels)
            qm->ghost_node_labels = (char *)db_hdf5_comprd(dbfile, m.ghost_node_labels, 0);
        if (DBGetDataReadMask2File(_dbfile) & DBQMGhostZoneLabels)
            qm->ghost_zone_labels = (char *)db_hdf5_comprd(dbfile, m.ghost_zone_labels, 0);

        /* alternate node number variables */
        {
            int nvars = -1;
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, m.alt_nodenum_vars, 1);
            if (tmpnames)
                qm->alt_nodenum_vars = DBStringListToStringArray(tmpnames, &nvars, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        /* alternate zone number variables */
        {
            int nvars = -1;
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, m.alt_zonenum_vars, 1);
            if (tmpnames)
                qm->alt_zonenum_vars = DBStringListToStringArray(tmpnames, &nvars, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeQuadmesh(qm);
    } END_PROTECT;

    return qm;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForQuadvarCompression
 *
 * Purpose:     Does some small work to prepare for possible quadvar 
 *              compression 
 *
 * Return:      Success:        ALLOW_MESH_COMPRESSION 
 *
 *              Failure:        0 
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *-------------------------------------------------------------------------
 */
static int
PrepareForQuadvarCompression(DBfile *_dbfile, int centering, int datatype)
{
    if (DBGetCompressionFile(_dbfile) == 0) return 0;
    if (centering == DB_ZONECENT) return 0;

#ifdef HAVE_HZIP
    db_hdf5_hzip_clear_params();
    db_hdf5_hzip_params.isquad = 1;
    return ALLOW_MESH_COMPRESSION;
#endif
    return 0;

}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutQuadvar
 *
 * Purpose:     Write a quadvar object to the file. The VARS and MIXVARS
 *              arguments should actually be of type `void*[]' because they
 *              point to either `float' or `double' depending on the value of
 *              the DATATYPE argument.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Wednesday, March 31, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Jul 17 15:10:50 PDT 2008
 *   Added call to prepare for compression. Changed call to write variable
 *   data to comprdz to support possible comporession. 
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Wed Nov 11 09:18:12 PST 2009
 *   Fixed support for edge/face centered variables.
 *
 *   Mark C. Miller, Wed Jan  6 13:46:45 PST 2010
 *   Fixed setting of align member based on centering.
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
SILO_CALLBACK int
db_hdf5_PutQuadvar(DBfile *_dbfile, char const *name, char const *meshname, int nvars,
                   char const * const *varnames, DBVCP2_t _vars,
                   int const *dims, int ndims, DBVCP2_t _mixvars,
                   int mixlen, int datatype, int centering, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_PutQuadvar";
    char                *s = 0;
    DBquadvar_mt        m;
    int                 i, nels, is_empty = 1;
    int                 compressionFlags;
    void const * const    *vars = (void const * const *) _vars;
    void const * const *mixvars = (void const * const *) _mixvars;

    for (i = 0; i < ndims; i++)
    {
        if (dims[i] > 0)
        {
            is_empty = 0;
            break;
        }
    }

    memset(&_qm, 0, sizeof _qm);
    memset(&m, 0, sizeof m);

    PROTECT {
        /* Set global options */
        _qm._coord_sys = DB_OTHER;
        _qm._facetype = DB_RECTILINEAR;
        _qm._ndims = _qm._nspace = ndims;
        _qm._planar = DB_AREA;
        _qm._use_specmf = DB_OFF;
        _qm._group_no = -1;
        _qm._missing_value = DB_MISSING_VALUE_NOT_SET;
        db_ProcessOptlist(DB_QUADMESH, optlist); /*yes, QUADMESH*/
        _qm._nzones = _qm._nnodes = 1; /*initial value only*/
        for (nels=(ndims?1:0), i=0; i<ndims; i++) {
            nels *= dims[i];
            _qm._nzones *= (dims[i]-1);
            _qm._nnodes *= dims[i];
            _qm._dims[i] = dims[i];
            _qm._zones[i] = dims[i]-1;
            _qm._minindex[i] = _qm._lo_offset[i];
            _qm._maxindex_n[i] = dims[i] - _qm._hi_offset[i] - 1;
            _qm._maxindex_z[i] = _qm._maxindex_n[i] - 1;
        }

        /* adjust things for edge/face centerings */
        if ((ndims > 1 && centering == DB_EDGECENT) ||
            (ndims > 2 && centering == DB_FACECENT))
           nels *= ndims;

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _qm._time_set, _qm._time,
            _qm._dtime_set, _qm._dtime, _qm._cycle); 

        compressionFlags = PrepareForQuadvarCompression(_dbfile, centering, datatype);

        /* Write variable arrays: vars[] and mixvars[] */
        if (nvars>MAX_VARS) {
            db_perror("too many subvariables", E_BADARGS, me);
            UNWIND();
        }
        for (i=0; i<nvars && !is_empty; i++) {
            /* Handle adjustment for edge/face centerings */
            if ((ndims > 1 && centering == DB_EDGECENT) ||
                (ndims > 2 && centering == DB_FACECENT))
            {
                int j, tmpndims = ndims+1;
                int tmpdims[4];
                for (j = ndims; j > 0; j--)
                    tmpdims[j] = dims[j-1];
                tmpdims[0] = ndims;
                db_hdf5_compwrz(dbfile, datatype, tmpndims, tmpdims, vars[i],
                    m.value[i]/*out*/, friendly_name(_dbfile,varnames[i], "_data", 0),
                    compressionFlags);
            }
            else
            {
                db_hdf5_compwrz(dbfile, datatype, ndims, dims, vars[i],
                    m.value[i]/*out*/, friendly_name(_dbfile,varnames[i], "_data", 0),
                    compressionFlags);
            }
            if (mixvars && mixvars[i] && mixlen>0) {
                db_hdf5_compwr(dbfile, datatype, 1, &mixlen, mixvars[i],
                    m.mixed_value[i]/*out*/, friendly_name(_dbfile,varnames[i], "_mix", 0));
            }
        }
        
        /* Build quadvar header in memory */
        m.ndims = ndims;
        m.nvals = nvars;
        m.nels = nels;
        m.origin = _qm._origin;
        m.mixlen = mixlen;
        m.major_order = _qm._majororder;
        m.cycle = _qm._cycle;
        m.time = _qm._time;
        m.dtime = _qm._dtime;
        m.use_specmf = _qm._use_specmf;
        m.ascii_labels = _qm._ascii_labels;
        m.guihide = _qm._guihide;
        m.datatype = (DB_FLOAT==datatype || DB_DOUBLE==datatype)?0:datatype;
        m.conserved = _qm._conserved;
        m.extensive = _qm._extensive;
        m.centering = centering;
        strcpy(m.label, OPT(_qm._label));
        strcpy(m.units, OPT(_qm._unit));
        strcpy(m.meshid, OPT(meshname));
        db_SetMissingValueForPut(m.missing_value, _qm._missing_value);

        for (i=0; i<ndims; i++) {
            m.dims[i] = _qm._dims[i];
            m.zones[i] = _qm._zones[i];
            m.min_index[i] = _qm._minindex[i];
            m.max_index[i] = _qm._maxindex_n[i];
            switch (centering) {
                case DB_NODECENT:
                    m.align[i] = 0.0; break;
                case DB_EDGECENT: /* edge centering on 1D mesh is like zone centering */
                    if (ndims == 1) m.align[i] = 0.5; break;
                case DB_FACECENT: /* face centering on 2D mesh is like zone centering */
                    if (ndims == 2) m.align[i] = 0.5; break;
                case DB_ZONECENT:
                    m.align[i] = 0.5; break;
                default:
                    m.align[i] = 0.5; break;
            }
        }

        /* output mrgtree info if we have it */
        if (_qm._region_pnames != NULL) {
            int len;
            DBStringArrayToStringList((char const * const *)_qm._region_pnames, -1, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.region_pnames/*out*/,
                friendly_name(_dbfile,name, "_region_pnames", 0));
            FREE(s);
        }

        /* Write quadvar header to file */
        STRUCT(DBquadvar) {
            MEMBER_S(str(m.meshid), meshid);
            MEMBER_R(str(m.value[_j]), value, nvars);
            MEMBER_R(str(m.mixed_value[_j]), mixed_value, nvars);
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nvals)        MEMBER_S(int, nvals);
            if (m.nels)         MEMBER_S(int, nels);
            if (m.origin)       MEMBER_S(int, origin);
            if (m.mixlen)       MEMBER_S(int, mixlen);
            if (m.major_order)  MEMBER_S(int, major_order);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (_qm._time_set)  MEMBER_S(float, time);
            if (_qm._dtime_set) MEMBER_S(double, dtime);
            if (m.use_specmf)   MEMBER_S(int, use_specmf);
            if (m.ascii_labels) MEMBER_S(int, ascii_labels);
            if (m.datatype)     MEMBER_S(int, datatype);
            if (m.guihide)      MEMBER_S(int, guihide);
            if (m.conserved)    MEMBER_S(int, conserved);
            if (m.extensive)    MEMBER_S(int, extensive);
            if (m.centering)    MEMBER_S(int, centering);
            if (m.missing_value)MEMBER_S(double, missing_value);
            MEMBER_3(int, dims);
            MEMBER_3(int, zones);
            MEMBER_3(int, min_index);
            MEMBER_3(int, max_index);
            MEMBER_3(float, align);
            MEMBER_S(str(m.label), label);
            MEMBER_S(str(m.units), units);
            MEMBER_S(str(m.region_pnames), region_pnames);
        } OUTPUT(dbfile, DB_QUADVAR, name, &m);
        
    } CLEANUP {
        void;
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForQuadvarDecompression
 *
 * Purpose:     Does some small work to prepare for possible quadvar 
 *              decompression 
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *-------------------------------------------------------------------------
 */
static void
PrepareForQuadvarDecompression(DBfile_hdf5 *dbfile, char const *varname,
    const DBquadvar *qv)
{
#ifdef HAVE_HZIP
    int i;
    db_hdf5_hzip_clear_params();
    db_hdf5_hzip_params.dbfile = dbfile;
    db_hdf5_hzip_params.isquad = 1;
    db_hdf5_hzip_params.meshname = qv->meshname;
    db_hdf5_hzip_params.ndims = qv->ndims;
    for (i = 0; i < qv->ndims; i++)
        db_hdf5_hzip_params.dims[i] = qv->dims[i];
#endif
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetQuadvar
 *
 * Purpose:     Reads a quadvar object from the file
 *
 * Return:      Success:        Ptr to new quadvar object
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, March 31, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *   Made it set datatype correctly. Added support for dataReadMask
 *
 *   Mark C. Miller, Thu Jul 17 15:13:57 PDT 2008
 *   Added to call to prepare for possible decompression.
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *
 *   Mark C. Miller, Thu Feb  4 11:27:59 PST 2010
 *   Added missing setting for recently added centering member.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBquadvar *
db_hdf5_GetQuadvar(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetQuadvar";
    char                *s = 0;
    hid_t               o=-1, attr=-1;
    int                 _objtype, stride, i;
    DBquadvar_mt        m;
    DBquadvar           *qv=NULL;
    
    PROTECT {
        /* Open object and make sure it's a quadvar */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_QUADVAR!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read quadvar data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBquadvar_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a quadvar object and initialize meta data */
        if (NULL==(qv=DBAllocQuadvar())) return NULL;
        qv->name = BASEDUP(name);
        qv->meshname = OPTDUP(m.meshid);
        qv->units = OPTDUP(m.units);
        qv->label = OPTDUP(m.label);
        qv->cycle = m.cycle;
        if ((qv->datatype = db_hdf5_GetVarType(_dbfile, m.value[0])) < 0)
            qv->datatype = silo2silo_type(m.datatype);
        if (force_single_g) qv->datatype = DB_FLOAT;
        qv->nels = m.nels;
        qv->nvals = m.nvals;
        qv->ndims = m.ndims;
        qv->major_order = m.major_order;
        qv->origin = m.origin;
        qv->time = m.time;
        qv->dtime = m.dtime;
        qv->mixlen = m.mixlen;
        qv->use_specmf = m.use_specmf;
        qv->ascii_labels = m.ascii_labels;
        qv->guihide = m.guihide;
        qv->conserved = m.conserved;
        qv->extensive = m.extensive;
        qv->centering = db_fix_obsolete_centering(m.ndims, m.align, m.centering);
        db_SetMissingValueForGet(qv->missing_value, m.missing_value);
        for (stride=1, i=0; i<m.ndims; stride*=m.dims[i++]) {
            qv->dims[i] = m.dims[i];
            qv->stride[i] = stride;
            qv->min_index[i] = m.min_index[i];
            qv->max_index[i] = m.max_index[i];
            qv->align[i] = m.align[i];
        }

        PrepareForQuadvarDecompression(dbfile, name, qv);

        /* Read the raw data */
        if (m.nvals>MAX_VARS) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if ((DBGetDataReadMask2File(_dbfile) & DBQVData) && m.nvals && m.ndims>0)
        {
            qv->vals = (void **)calloc(m.nvals, sizeof(void*));
            if (m.mixlen) qv->mixvals = (void **)calloc(m.nvals, sizeof(void*));
            for (i=0; i<m.nvals; i++) {
                qv->vals[i] = db_hdf5_comprd(dbfile, m.value[i], 0);
                if (m.mixlen && m.mixed_value[i][0]) {
                    qv->mixvals[i] = db_hdf5_comprd(dbfile, m.mixed_value[i], 0);
                }
            }
        }

        s = (char *)db_hdf5_comprd(dbfile, m.region_pnames, 1);
        if (s) qv->region_pnames = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
        FREE(s);

        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeQuadvar(qv);
    } END_PROTECT;

    return qv;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForUcdmeshCompression
 *
 * Purpose:     Does some small work to prepare for possible mesh
 *              compression 
 *
 * Return:      Success:        ALLOW_MESH_COMPRESSION 
 *
 *              Failure:        0 
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *-------------------------------------------------------------------------
 */
static int PrepareForUcdmeshCompression(DBfile_hdf5 *dbfile,
    char const *meshname, char const *zlname)
{
    if (DBGetCompressionFile((DBfile*)dbfile) == 0) return 0;

#ifdef HAVE_HZIP
    if (LookupNodelist(dbfile, zlname, meshname) != 0)
    {
        db_hdf5_hzip_clear_params();
        db_hdf5_hzip_params.dbfile = dbfile;
        db_hdf5_hzip_params.zlname = zlname;
        db_hdf5_hzip_params.meshname = meshname;
        AddMeshnameToNodelist(dbfile, zlname, meshname);
        return ALLOW_MESH_COMPRESSION;
    }
#endif
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutUcdmesh
 *
 * Purpose:     Write a ucdmesh object to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  1, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `group_no' and `gnodeno' properties to duplicate
 *              changes made to the PDB driver.
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Mon Jul 31 17:43:52 PDT 2006
 *   Removed use of fullname for zonelist, facelist, phzonelist options
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Jul 17 15:14:44 PDT 2008
 *   Added call to prepare for possible compression. Changed calls that
 *   write coordinates and gnodeno to use compwrz
 *
 *   Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *   Added support for long long global node numbers.
 *   Changed how long long global node/zone numbers are supported
 *   from a int (bool), "llong_gnode|zoneno" to an int holding
 *   the actual datatype. The type is assumed int if it its
 *   value is zero or it does not exist. Otherwise, the type is
 *   is whatever is stored in gnznodtype member. 
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
SILO_CALLBACK int
db_hdf5_PutUcdmesh(DBfile *_dbfile, char const *name, int ndims, char const * const *coordnames,
                   DBVCP2_t _coords, int nnodes, int nzones, char const *zlname,
                   char const *flname, int datatype, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_PutUcdmesh";
    DBucdmesh_mt        m;
    int                 i, compressionFlags;
    void const * const *coords = (void const * const *) _coords;

    memset(&_um, 0, sizeof _um);
    memset(&m, 0, sizeof m);

    PROTECT {
        /* Check datatype */
        if (DB_FLOAT!=datatype && DB_DOUBLE!=datatype) {
            db_perror("invalid floating-point datatype", E_BADARGS, me);
            UNWIND();
        }

        /* Set global options */
        _um._coord_sys = DB_OTHER;
        _um._facetype = DB_RECTILINEAR;
        _um._ndims = ndims;
        _um._nnodes = nnodes;
        _um._nzones = nzones;
        _um._planar = DB_OTHER;
        _um._use_specmf = DB_OFF;
        _um._group_no = -1;
        db_ProcessOptlist(DB_UCDMESH, optlist);

        /* Prepare for possible compression of coords/gnodeno */
        compressionFlags = PrepareForUcdmeshCompression(dbfile,
            name, zlname); 

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _um._time_set, _um._time,
            _um._dtime_set, _um._dtime, _um._cycle);

        /* Obtain extents as doubles */
        if (DB_DOUBLE==datatype) {
            UM_CalcExtents(coords, datatype, ndims, nnodes,
                           m.min_extents/*out*/, m.max_extents/*out*/);
        } else {
            float min_extents[3], max_extents[3];
            UM_CalcExtents(coords, datatype, ndims, nnodes,
                           min_extents/*out*/, max_extents/*out*/);
            for (i=0; i<ndims; i++) {
                m.min_extents[i] = min_extents[i];
                m.max_extents[i] = max_extents[i];
            }
        }
        
        /* Write variable arrays: coords[], gnodeno[] */
        for (i=0; (i<ndims) && (nnodes>0); i++) {
            db_hdf5_compwrz(dbfile, datatype, 1, &nnodes, coords[i],
                m.coord[i]/*out*/, friendly_name(_dbfile,name, "_coord%d", &i), 
                compressionFlags);
        }
        if (_um._llong_gnodeno && (nnodes > 0))
            db_hdf5_compwr(dbfile, DB_LONG_LONG, 1, &nnodes, _um._gnodeno,
                m.gnodeno/*out*/, friendly_name(_dbfile,name, "_gnodeno",0));
        else if (nnodes > 0)
            db_hdf5_compwrz(dbfile, DB_INT, 1, &nnodes, _um._gnodeno,
                m.gnodeno/*out*/, friendly_name(_dbfile,name, "_gnodeno",0),
                compressionFlags);
        
        if (nnodes && _um._ghost_node_labels)
        {
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &nnodes, _um._ghost_node_labels,
                m.ghost_node_labels/*out*/, friendly_name(_dbfile,name, "_ghost_node_labels",0));
        }

        if (_um._alt_nodenum_vars != NULL) {
            char *s; int len;
            DBStringArrayToStringList((char const * const *)_um._alt_nodenum_vars, -1, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.alt_nodenum_vars/*out*/,
                friendly_name(_dbfile,name, "_alt_nodenum_vars", 0));
            FREE(s);
        }

        /* Build ucdmesh header in memory */
        m.ndims = ndims;
        m.nnodes = nnodes;
        m.nzones = nzones;
        m.facetype = _um._facetype;
        m.coord_sys = _um._coord_sys;
        m.topo_dim = _um._topo_dim;
        m.planar = _um._planar;
        m.origin = _um._origin;
        m.cycle = _um._cycle;
        m.time = _um._time;
        m.dtime = _um._dtime;
        m.group_no = _um._group_no;
        m.guihide = _um._guihide;
        strcpy(m.zonelist, OPT(zlname));
        strcpy(m.facelist, OPT(flname));
        strcpy(m.phzonelist, OPT(_um._phzl_name));
        for (i=0; i<ndims; i++) {
            strcpy(m.label[i], OPT(_um._labels[i]));
            strcpy(m.units[i], OPT(_um._units[i]));
        }
        strcpy(m.mrgtree_name, OPT(_um._mrgtree_name));
        m.tv_connectivity = _um._tv_connectivity;
        m.disjoint_mode = _um._disjoint_mode;
        m.gnznodtype = _um._llong_gnodeno?DB_LONG_LONG:0;

        /* Write ucdmesh header to file */
        STRUCT(DBucdmesh) {
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nnodes)       MEMBER_S(int, nnodes);
            if (m.nzones)       MEMBER_S(int, nzones);
            if (m.facetype)     MEMBER_S(int, facetype);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (m.coord_sys)    MEMBER_S(int, coord_sys);
            if (m.topo_dim)     MEMBER_S(int, topo_dim);
            if (m.planar)       MEMBER_S(int, planar);
            if (m.origin)       MEMBER_S(int, origin);
            if (m.group_no)     MEMBER_S(int, group_no);
            if (m.guihide)      MEMBER_S(int, guihide);
            if (_um._time_set)  MEMBER_S(float, time);
            if (_um._dtime_set) MEMBER_S(double, dtime);
            MEMBER_S(str(m.facelist), facelist);
            MEMBER_S(str(m.zonelist), zonelist);
            MEMBER_S(str(m.gnodeno), gnodeno);
            MEMBER_3(double, min_extents);
            MEMBER_3(double, max_extents);
            MEMBER_R(str(m.coord[_j]), coord, ndims);
            MEMBER_R(str(m.label[_j]), label, ndims);
            MEMBER_R(str(m.units[_j]), units, ndims);
            MEMBER_S(str(m.phzonelist), phzonelist);
            MEMBER_S(str(m.mrgtree_name), mrgtree_name);
            if (m.tv_connectivity) MEMBER_S(int, tv_connectivity);
            if (m.disjoint_mode)   MEMBER_S(int, disjoint_mode);
            if (m.gnznodtype)   MEMBER_S(int, gnznodtype);
            MEMBER_S(str(m.ghost_node_labels), ghost_node_labels);
            MEMBER_S(str(m.alt_nodenum_vars), alt_nodenum_vars);
        } OUTPUT(dbfile, DB_UCDMESH, name, &m);
        
    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutUcdsubmesh
 *
 * Purpose:     Write a subset of a ucdmesh object into the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Friday, April  2, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Mon Jul 31 17:57:29 PDT 2006
 *   Eliminated use of db_hdf5_fullname for zlname, flname and added
 *   possible optional ph_zlname which was mistakenly left out in the
 *   initial implementation.
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
SILO_CALLBACK int
db_hdf5_PutUcdsubmesh(DBfile *_dbfile, char const *name, char const *parentmesh,
                      int nzones, char const *zlname, char const *flname,
                      DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_pdb_PutUcdmesh";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i;
    DBucdmesh_mt        m;

    PROTECT {
        /* Get metadata from the parent UCD mesh */
        if ((o=H5Topen(dbfile->cwg, parentmesh, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_UCDMESH!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBucdmesh_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        H5Tclose(o);

        /* Set global options */
        _um._coord_sys = DB_OTHER;
        _um._facetype = DB_RECTILINEAR;
        _um._ndims = m.ndims;
        _um._nnodes = m.nnodes;
        _um._nzones = m.nzones;
        _um._planar = DB_OTHER;
        _um._use_specmf = DB_OFF;
        _um._group_no = -1;
        db_ProcessOptlist(DB_UCDMESH, optlist);

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _um._time_set, _um._time,
            _um._dtime_set, _um._dtime, _um._cycle);

        /* Build header in memory -- most fields are already initialized */
        m.ndims = _um._ndims;
        m.nnodes = _um._nnodes;
        m.nzones = _um._nzones;
        m.facetype = _um._facetype;
        m.cycle = _um._cycle;
        m.coord_sys = _um._coord_sys;
        m.topo_dim = _um._topo_dim;
        m.planar = _um._planar;
        m.origin = _um._origin;
        m.time = _um._time;
        m.dtime = _um._dtime;
        m.guihide = _um._guihide;
        strcpy(m.zonelist, zlname);
        strcpy(m.facelist, OPT(flname));
        strcpy(m.phzonelist, OPT(_um._phzl_name));
        for (i=0; i<m.ndims; i++) {
            strcpy(m.label[i], OPT(_um._labels[i]));
            strcpy(m.units[i], OPT(_um._units[i]));
        }
        strcpy(m.mrgtree_name, OPT(_um._mrgtree_name));
        m.tv_connectivity = _um._tv_connectivity;
        m.disjoint_mode = _um._disjoint_mode;
        
        /* Write header to file */
        STRUCT(DBucdmesh) {
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nnodes)       MEMBER_S(int, nnodes);
            if (m.nzones)       MEMBER_S(int, nzones);
            if (m.facetype)     MEMBER_S(int, facetype);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (m.coord_sys)    MEMBER_S(int, coord_sys);
            if (m.topo_dim)     MEMBER_S(int, topo_dim);
            if (m.planar)       MEMBER_S(int, planar);
            if (m.origin)       MEMBER_S(int, origin);
            if (m.guihide)      MEMBER_S(int, guihide);
            if (_um._time_set)  MEMBER_S(float, time);
            if (_um._dtime_set) MEMBER_S(double, dtime);
            MEMBER_S(str(m.facelist), facelist);
            MEMBER_S(str(m.zonelist), zonelist);
            MEMBER_3(double, min_extents);
            MEMBER_3(double, max_extents);
            MEMBER_R(str(m.coord[_j]), coord, m.ndims);
            MEMBER_R(str(m.label[_j]), label, m.ndims);
            MEMBER_R(str(m.units[_j]), units, m.ndims);
            MEMBER_S(str(m.mrgtree_name), mrgtree_name);
            if (m.tv_connectivity) MEMBER_S(int, tv_connectivity);
            if (m.disjoint_mode)   MEMBER_S(int, disjoint_mode);
        } OUTPUT(dbfile, DB_UCDMESH, name, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForUcdmeshDecompression
 *
 * Purpose:     Does some small work to prepare for possible mesh
 *              decompression 
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *-------------------------------------------------------------------------
 */
static void
PrepareForUcdmeshDecompression(DBfile_hdf5 *dbfile, char const *meshname,
    char const *zlname)
{
#ifdef HAVE_HZIP
    db_hdf5_hzip_clear_params();
    db_hdf5_hzip_params.dbfile = dbfile;
    db_hdf5_hzip_params.meshname = meshname;
    db_hdf5_hzip_params.zlname = zlname;
#endif
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetUcdmesh
 *
 * Purpose:     Reads a UCD mesh object from the file.
 *
 * Return:      Success:        Pointer to a new ucdmesh.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  1, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `group_no', and `gnodeno' to duplicate changes made to
 *              the PDB driver.
 *
 *              Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *              Made it set datatype correctly.
 *              Added support for dataReadMask
 *
 *              Mark C. Miller, Thu Sep  7 10:50:55 PDT 2006
 *              Added use of db_hdf5_resolvename for retrieval of
 *              sub-objects.
 *
 *              Mark C. Miller, Thu Jul 17 15:16:13 PDT 2008
 *              Added support for decompression. Needed to change order
 *              of operations to read zonelist first as that is the 
 *              bootstrap HZIP needs to decompress anything.
 *
 *              Mark C. Miller, Thu Nov  5 16:17:18 PST 2009
 *              Made it NOT db_Split() the zonelist if mask is such
 *              that zonelist info is NOT included.
 *
 *              Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *              Changed how long long global node/zone numbers are supported
 *              from a int (bool), "llong_gnode|zoneno" to an int holding
 *              the actual datatype. The type is assumed int if it its
 *              value is zero or it does not exist. Otherwise, the type is
 *              is whatever is stored in gnznodtype member. 
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBucdmesh *
db_hdf5_GetUcdmesh(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetUcdmesh";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i;
    DBucdmesh_mt        m;
    DBucdmesh           *um=NULL;

    PROTECT {
        /* Open object and make sure it's a ucdmesh */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_UCDMESH!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read header into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBucdmesh_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a ucdmesh object and initialize meta data */
        if (NULL==(um=DBAllocUcdmesh())) return NULL;
        um->name = BASEDUP(name);
        um->cycle = m.cycle;
        um->coord_sys = m.coord_sys;
        um->topo_dim = m.topo_dim;
        /* The value we store to the file for 'topo_dim' member is
           designed such that zero indicates a value that was NOT
           specified in the file. Since zero is a valid topological
           dimension, when we store topo_dim to a file, we always
           add 1. So, we have to subtract it here. However, this
           data member was not being handled correctly in files
           versions before 4.7. So, for older files, if topo_dim
           is non-zero we just pass it without alteration. */
        if (!DBFileVersionGE(_dbfile,4,5,1) || DBFileVersionGE(_dbfile, 4,7,0))
            um->topo_dim = um->topo_dim - 1;
        if ((um->datatype = db_hdf5_GetVarType(_dbfile, m.coord[0])) < 0)
            um->datatype = DB_FLOAT;
        if (force_single_g) um->datatype = DB_FLOAT;
        um->time = m.time;
        um->dtime = m.dtime;
        um->ndims = m.ndims;
        um->nnodes = m.nnodes;
        um->origin = m.origin;
        um->group_no = m.group_no;
        um->guihide = m.guihide;
        for (i=0; i<m.ndims; i++) {
            um->units[i] = OPTDUP(m.units[i]);
            um->labels[i] = OPTDUP(m.label[i]);
            if (um->datatype == DB_DOUBLE)
            {
                ((double*)um->min_extents)[i] = m.min_extents[i];
                ((double*)um->max_extents)[i] = m.max_extents[i];
            }
            else
            {
                um->min_extents[i] = m.min_extents[i];
                um->max_extents[i] = m.max_extents[i];
            }
        }
        um->mrgtree_name = OPTDUP(m.mrgtree_name);
        um->tv_connectivity = m.tv_connectivity;
        um->disjoint_mode = m.disjoint_mode;

        /* We have a problem with data read mask and compression. If 
           masks says not to read zonelist but the zonelist is actually
           compressed, we're gonna need it to complete the mesh read.
           So, we need a cheap way to ask if the zonelist is compressed
           and hope it is cheap enough that always asking that question
           won't present performance problems. For now, we are just
           going to let it fail. */

        /* For compression's sake, read the zonelist first */
        if (um->nnodes>0 && m.zonelist[0] && (DBGetDataReadMask2File(_dbfile) & DBUMZonelist)) {
            calledFromGetUcdmesh = um->name;
            um->zones = db_hdf5_GetZonelist(_dbfile, 
                            db_hdf5_resolvename(_dbfile, name, m.zonelist));
            calledFromGetUcdmesh = 0;

            /*----------------------------------------------------------*/
            /* If we have ghost zones, split any group of shapecnt so   */
            /* all the shapecnt refer to all real zones or all ghost    */
            /* zones.  This will make dealing with ghost zones easier   */
            /* for applications.                                        */
            /*----------------------------------------------------------*/
            if (um->zones)
            {
                if (((um->zones->min_index != 0) || 
                     (um->zones->max_index != um->zones->nzones - 1)) &&
                     (DBGetDataReadMask2File(_dbfile) & DBZonelistInfo))
                {
                    db_SplitShapelist (um);
                }
            }
        }

        /* Prepare for possible ucdmesh decompression */
        PrepareForUcdmeshDecompression(dbfile, name, m.zonelist);

        /* Read the raw data */
        if (DBGetDataReadMask2File(_dbfile) & DBUMCoords)
        {
            for (i=0; i<m.ndims; i++) {
                um->coords[i] = db_hdf5_comprd(dbfile, m.coord[i], 0);
            }
        }
        if (DBGetDataReadMask2File(_dbfile) & DBUMGlobNodeNo)
            um->gnodeno = db_hdf5_comprd(dbfile, m.gnodeno, 1);
        um->gnznodtype = m.gnznodtype?m.gnznodtype:DB_INT;
        if (DBGetDataReadMask2File(_dbfile) & DBUMGhostNodeLabels)
            um->ghost_node_labels = (char *)db_hdf5_comprd(dbfile, m.ghost_node_labels, 1);

        /* alt nodenum vars */
        {
            int nnames = -1;
            char *tmpannums = (char *)db_hdf5_comprd(dbfile, m.alt_nodenum_vars, 1);
            if (tmpannums)
                um->alt_nodenum_vars = DBStringListToStringArray(tmpannums, &nnames, !skipFirstSemicolon);
            FREE(tmpannums);
        }

        /* Read face, zone, and edge lists */
        if (um->nnodes>0 && m.facelist[0] && (DBGetDataReadMask2File(_dbfile) & DBUMFacelist)) {
            um->faces = db_hdf5_GetFacelist(_dbfile,
                            db_hdf5_resolvename(_dbfile, name, m.facelist));
        }
        if (um->nnodes>0 && m.phzonelist[0] && (DBGetDataReadMask2File(_dbfile) & DBUMZonelist)) {
            um->phzones = db_hdf5_GetPHZonelist(_dbfile, 
                              db_hdf5_resolvename(_dbfile, name, m.phzonelist));
        }
        um->edges = NULL;                               /*FIXME*/

        H5Tclose(o);
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
    } END_PROTECT;

#ifdef HAVE_HZIP
    db_hdf5_hzip_clear_params();
#endif

    return um;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForUcdvarCompression
 *
 * Purpose:     Does some small work to prepare for possible ucdvar 
 *              compression 
 *
 * Return:      Success:        ALLOW_MESH_COMPRESSION 
 *
 *              Failure:        0 
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *-------------------------------------------------------------------------
 */
static int
PrepareForUcdvarCompression(DBfile_hdf5 *dbfile, char const *varname,
    char const *meshname, int datatype, int centering)
{
    if (DBGetCompressionFile((DBfile*)dbfile) == 0) return 0;

#ifdef HAVE_HZIP
    if (centering == DB_NODECENT)
    {
        db_hdf5_hzip_clear_params();
        db_hdf5_hzip_params.meshname = meshname;
        db_hdf5_hzip_params.dbfile = dbfile;
        return ALLOW_MESH_COMPRESSION;
    }
#endif
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutUcdvar
 *
 * Purpose:     Writes UCD variables to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  1, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Brad Whitlock, Wed Jan 18 15:17:15 PST 2006 
 *   Added ascii_labels.
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 * 
 *   Mark C. Miller, Thu Jul 17 15:17:51 PDT 2008
 *   Added call to prepare for compression. Changed call to write data
 *   to use compwrz.
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
SILO_CALLBACK int
db_hdf5_PutUcdvar(DBfile *_dbfile, char const *name, char const *meshname, int nvars,
                  char const * const *varnames, DBVCP2_t _vars,
                  int nels, DBVCP2_t _mixvars, int mixlen,
                  int datatype, int centering, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_PutUcdvar";
    char                *s = 0;
    DBucdvar_mt         m;
    int                 i, saved_ndims, saved_nnodes, saved_nzones;
    int                 compressionFlags;
    void const * const    *vars = (void const * const *) _vars;
    void const * const *mixvars = (void const * const *) _mixvars;

    memset(&m, 0, sizeof m);

    PROTECT {
        /* Set global options  - based on previous PutUcdmesh() call */
        saved_ndims = _um._ndims;
        saved_nnodes = _um._nnodes;
        saved_nzones = _um._nzones;
        memset(&_um, 0, sizeof _um);
        _um._coord_sys = DB_OTHER;
        _um._topo_dim = saved_ndims;
        _um._facetype = DB_RECTILINEAR;
        _um._ndims = saved_ndims;
        _um._nnodes = saved_nnodes;
        _um._nzones = saved_nzones;
        _um._planar = DB_OTHER;
        _um._use_specmf = DB_OFF;
        _um._group_no = -1;
        _um._missing_value = DB_MISSING_VALUE_NOT_SET;
        db_ProcessOptlist(DB_UCDMESH, optlist); /*yes, UCDMESH*/

        /* Prepare for possible compression of ucdvars */
        compressionFlags = PrepareForUcdvarCompression(dbfile, name, meshname,
            datatype, centering);

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _um._time_set, _um._time,
            _um._dtime_set, _um._dtime, _um._cycle);

        /* Write variable arrays: vars[], mixvars[] */
        if (nvars>MAX_VARS) {
            db_perror("too many variables", E_BADARGS, me);
            UNWIND();
        }
        for (i=0; i<nvars && nels; i++) {
            db_hdf5_compwrz(dbfile, datatype, 1, &nels, vars[i],
                m.value[i]/*out*/, friendly_name(_dbfile,varnames[i], "_data", 0), compressionFlags);
//#ifndef _MSC_VER
//#warning WHY NOT COMPRESS MIX DATA TOO
//#endif
            if (mixvars && mixvars[i] && mixlen>0) {
                db_hdf5_compwr(dbfile, datatype, 1, &mixlen, mixvars[i],
                    m.mixed_value[i]/*out*/, friendly_name(_dbfile,varnames[i], "_mix", 0));
            }
        }

        /* output mrgtree info if we have it */
        if (_um._region_pnames != NULL) {
            int len;
            DBStringArrayToStringList((char const * const *)_um._region_pnames, -1, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.region_pnames/*out*/,
                friendly_name(_dbfile,name, "_region_pnames", 0));
            FREE(s);
        }

        /* Build header in memory */
        m.ndims = _um._ndims;
        m.nvals = nvars;
        m.nels = nels;
        m.centering = centering;
        m.origin = _um._origin;
        m.mixlen = mixlen;
        m.cycle = _um._cycle;
        m.time = _um._time;
        m.dtime = _um._dtime;
        m.lo_offset = _um._lo_offset;
        m.hi_offset = _um._hi_offset;
        m.use_specmf = _um._use_specmf;
        m.ascii_labels = _um._ascii_labels;
        m.guihide = _um._guihide;
        m.datatype = (DB_FLOAT==datatype || DB_DOUBLE==datatype)?0:datatype;
        m.conserved = _um._conserved;
        m.extensive = _um._extensive;
        db_SetMissingValueForPut(m.missing_value, _um._missing_value);
        strcpy(m.meshid, OPT(meshname));
        strcpy(m.label, OPT(_um._label));
        strcpy(m.units, OPT(_um._unit));

        /* Write header to file */
        STRUCT(DBucdvar) {
            MEMBER_R(str(m.value[_j]), value, nvars);
            MEMBER_R(str(m.mixed_value[_j]), mixed_value, nvars);
            MEMBER_S(str(m.meshid), meshid);
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nvals)        MEMBER_S(int, nvals);
            if (m.nels)         MEMBER_S(int, nels);
            if (m.centering)    MEMBER_S(int, centering);
            if (m.origin)       MEMBER_S(int, origin);
            if (m.mixlen)       MEMBER_S(int, mixlen);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (m.use_specmf)   MEMBER_S(int, use_specmf);
            if (m.ascii_labels) MEMBER_S(int, ascii_labels);
            if (m.guihide)      MEMBER_S(int, guihide);
            if (m.datatype)     MEMBER_S(int, datatype);
            if (_um._time_set)  MEMBER_S(float, time);
            if (_um._dtime_set) MEMBER_S(double, dtime);
            if (_um._lo_offset_set) MEMBER_S(int, lo_offset);
            if (_um._hi_offset_set) MEMBER_S(int, hi_offset);
            if (m.conserved)    MEMBER_S(int, conserved);
            if (m.extensive)    MEMBER_S(int, extensive);
            if (m.missing_value)MEMBER_S(double, missing_value);
            MEMBER_S(str(m.label), label);
            MEMBER_S(str(m.units), units);
            MEMBER_S(str(m.region_pnames), region_pnames);
        } OUTPUT(dbfile, DB_UCDVAR, name, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForUcdvarDecompression
 *
 * Purpose:     Does some small work to prepare for possible ucdvar 
 *              decompression 
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *
 *     We assume here that looking up the nodelist is cheaper than
 *     checking if the values are compressed, so we perform that
 *     operation first. Reason being is that checking if the 
 *     dataset is compressed may involve disk I/O. Looking up
 *     the nodelist is a bunch of string and pointer compars.
 *     But, that operation is always performed!!!
 *-------------------------------------------------------------------------
 */
static void
PrepareForUcdvarDecompression(DBfile *_dbfile, char const *varname,
    char const *meshname, char valnames[MAX_VARS][256], int nvals)
{

#ifdef HAVE_HZIP

    /* We have to have read the mesh zonelist first,
       if we don't already have it. */
    if (LookupNodelist((DBfile_hdf5*)_dbfile, 0, meshname) == 0)
    {
        /* See if we even have compressed data to begin with */
        int i, haveCompressed = 0;
        for (i=0; i<nvals && !haveCompressed; i++)
            haveCompressed = db_hdf5_compckz((DBfile_hdf5*)_dbfile, (char*) valnames[i]);

        if (haveCompressed)
        {
            unsigned long long currentMask =
                DBSetDataReadMask2File(_dbfile, DBUMZonelist|DBZonelistInfo);
            DBucdmesh *um;
            um = db_hdf5_GetUcdmesh(_dbfile, (char*) meshname);

            /* Note that if for some reason the zl was not compressed, then
               getting it via the above GetUcdmesh call won't register it.
               Also, registering an already existing zonelist has no effect.
               So, as a precaution, we register the zonelist here, too.
               And, since we know the meshname here also, that will get
               added to the registered zonelist if it already exists. */
            if (um && um->zones)
            {
                RegisterNodelist((DBfile_hdf5*)_dbfile, 0, meshname,
                    um->zones->ndims, um->zones->nzones, um->zones->origin,
                    um->zones->nodelist);
                DBSetDataReadMask2File(_dbfile, currentMask);
                DBFreeUcdmesh(um);
            }
        }
    }

    db_hdf5_hzip_clear_params();
    db_hdf5_hzip_params.dbfile = (DBfile_hdf5*)_dbfile;
    db_hdf5_hzip_params.meshname = meshname;

#endif
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetUcdvar
 *
 * Purpose:     Reads a UCD variable object from the file.
 *
 * Return:      Success:        Ptr to new UCD variable object
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  1, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *   Made it set the correct datatype. Added support for dataReadMask
 *
 *   Brad Whitlock, Wed Jan 18 15:17:48 PST 2006
 *   Added ascii_labels.
 *
 *   Mark C. Miller, Thu Jul 17 15:19:14 PDT 2008
 *   Added call to prepare for possible decompression.
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBucdvar *
db_hdf5_GetUcdvar(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetUcdvar";
    char                *s = 0;
    hid_t               o=-1, attr=-1;
    int                 _objtype, i;
    DBucdvar_mt         m;
    DBucdvar            *uv=NULL;

    PROTECT {
        /* Open object and make sure it's a ucdvar */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_UCDVAR!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read ucdvar data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBucdvar_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a ucdvar object and initialize meta data */
        if (NULL==(uv=DBAllocUcdvar())) return NULL;
        uv->name = BASEDUP(name);
        uv->meshname = OPTDUP(m.meshid);
        uv->cycle = m.cycle;
        uv->units = OPTDUP(m.units);
        uv->label = OPTDUP(m.label);
        uv->time = m.time;
        uv->dtime = m.dtime;
        if ((uv->datatype = db_hdf5_GetVarType(_dbfile, m.value[0])) < 0)
            uv->datatype = silo2silo_type(m.datatype);
        if (force_single_g) uv->datatype = DB_FLOAT;
        uv->nels = m.nels;
        uv->nvals = m.nvals;
        uv->ndims = m.ndims;
        uv->origin = m.origin;
        uv->centering = m.centering;
        uv->mixlen = m.mixlen;
        uv->use_specmf = m.use_specmf;
        uv->ascii_labels = m.ascii_labels;
        uv->guihide = m.guihide;
        uv->conserved = m.conserved;
        uv->extensive = m.extensive;
        db_SetMissingValueForGet(uv->missing_value, m.missing_value);

        /* If var is compressed, we need to do some work to decompress it */
        PrepareForUcdvarDecompression(_dbfile, name, uv->meshname?uv->meshname:"", m.value, m.nvals);

        /* Read the raw data */
        if (m.nvals>MAX_VARS) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if ((DBGetDataReadMask2File(_dbfile) & DBUVData) && m.nvals)
        {
            uv->vals = (void **)calloc(m.nvals, sizeof(void*));
            if (m.mixlen) uv->mixvals = (void **)calloc(m.nvals, sizeof(void*));
            for (i=0; i<m.nvals; i++) {
                uv->vals[i] = db_hdf5_comprd(dbfile, m.value[i], 0);
                if (m.mixlen && m.mixed_value[i][0]) {
                    uv->mixvals[i] = db_hdf5_comprd(dbfile, m.mixed_value[i], 0);
                }
            }
        }

        s = (char *)db_hdf5_comprd(dbfile, m.region_pnames, 1);
        if (s) uv->region_pnames = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
        FREE(s);

        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeUcdvar(uv);
    } END_PROTECT;

    return uv;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutFacelist
 *
 * Purpose:     Writes facelist information to a facelist object in the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  1, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutFacelist(
    DBfile *_dbfile,
    char const *name,
    int nfaces,
    int ndims,
    int const *nodelist,
    int lnodelist,
    int origin,
    int const *zoneno,
    int const *shapesize,
    int const *shapecnt,
    int nshapes,
    int const *types,
    int const *typelist,
    int ntypes
)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBfacelist_mt       m;

    memset(&m, 0, sizeof m);

    PROTECT {
        /* Write variable arrays */
        if (lnodelist) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &lnodelist, nodelist,
                m.nodelist/*out*/, friendly_name(_dbfile,name, "_nodelist", 0));
        }
        if (2==ndims || 3==ndims) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &nshapes, shapecnt,
                m.shapecnt/*out*/, friendly_name(_dbfile,name, "_shapecnt", 0));
            db_hdf5_compwr(dbfile, DB_INT, 1, &nshapes, shapesize,
                m.shapesize/*out*/, friendly_name(_dbfile,name, "_shapesize", 0));
        }
        if (ntypes && typelist) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &ntypes, typelist,
                m.typelist/*out*/, friendly_name(_dbfile,name, "_typelist", 0));
        }
        if (ntypes && types) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &nfaces, types,
                m.types/*out*/, friendly_name(_dbfile,name, "_types", 0));
        }
        if (zoneno) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &nfaces, zoneno,
                m.zoneno/*out*/, friendly_name(_dbfile,name, "_zoneno", 0));
        }
        
        /* Build header in memory */
        m.ndims = ndims;
        m.nfaces = nfaces;
        m.nshapes = nshapes;
        m.ntypes = ntypes;
        m.lnodelist = lnodelist;
        m.origin = origin;

        /* Write header to file */
        STRUCT(DBfacelist) {
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nfaces)       MEMBER_S(int, nfaces);
            if (m.nshapes)      MEMBER_S(int, nshapes);
            if (m.ntypes)       MEMBER_S(int, ntypes);
            if (m.lnodelist)    MEMBER_S(int, lnodelist);
            if (m.origin)       MEMBER_S(int, origin);
            MEMBER_S(str(m.nodelist), nodelist);
            MEMBER_S(str(m.shapecnt), shapecnt);
            MEMBER_S(str(m.shapesize), shapesize);
            MEMBER_S(str(m.typelist), typelist);
            MEMBER_S(str(m.types), types);
            MEMBER_S(str(m.zoneno), zoneno);
        } OUTPUT(dbfile, DB_FACELIST, name, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetFacelist
 *
 * Purpose:     Reads a facelist object from the file.
 *
 * Return:      Success:        Ptr to a new facelist object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  1, 1999
 *
 * Modifications:
 *
 *  Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *  Added support for dataReadMask
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBfacelist *
db_hdf5_GetFacelist(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetFacelist";
    hid_t               o=-1, attr=-1;
    int                 _objtype;
    DBfacelist_mt       m;
    DBfacelist          *fl=NULL;

    PROTECT {
        /* Open object and make sure it's a facelist */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_FACELIST!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read facelist data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBfacelist_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a facelist object and initialize meta data */
        if (NULL==(fl=DBAllocFacelist())) return NULL;
        fl->ndims = m.ndims;
        fl->nfaces = m.nfaces;
        fl->origin = m.origin;
        fl->lnodelist = m.lnodelist;
        fl->nshapes = m.nshapes;
        fl->ntypes = m.ntypes;

        /* Read the raw data */
        if (DBGetDataReadMask2File(_dbfile) & DBFacelistInfo)
        {
            fl->nodelist = (int *)db_hdf5_comprd(dbfile, m.nodelist, 1);
            fl->shapecnt = (int *)db_hdf5_comprd(dbfile, m.shapecnt, 1);
            fl->shapesize = (int *)db_hdf5_comprd(dbfile, m.shapesize, 1);
            fl->typelist = (int *)db_hdf5_comprd(dbfile, m.typelist, 1);
            fl->types = (int *)db_hdf5_comprd(dbfile, m.types, 1);
            fl->zoneno = (int *)db_hdf5_comprd(dbfile, m.zoneno, 1);
        }

        H5Tclose(o);
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeFacelist(fl);
    } END_PROTECT;

    return fl;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForZonelistCompression
 *
 * Purpose:     Does some small work to prepare for possible zonelist 
 *              compression 
 *
 * Return:      Success:        ALLOW_MESH_COMPRESSION 
 *
 *              Failure:        0 
 *
 * HZIP supports only 2D quad meshes and 3D hex meshes.
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *-------------------------------------------------------------------------
 */
static int
PrepareForZonelistCompression(DBfile_hdf5 *dbfile, char const *name,
    int origin, int ndims, int nshapes, int const *shapetype, int const *shapesize,
    int const *shapecnt, int const *nodelist)
{
	int i;
    int ntopo = 0, zncnt = 0;

    if (nshapes == 0) return 0;
    if (DBGetCompressionFile((DBfile*)dbfile) == 0) return 0;

    zncnt = shapecnt[0];
#ifdef HAVE_HZIP

    if (shapetype)
    {
        /* hzip supports only quad/hex meshes */
        if (shapetype[0] == DB_ZONETYPE_QUAD)
        {
            ntopo = 2;
            for (i = 1; i < nshapes; i++)
            {
                zncnt += shapecnt[i];
                if (shapetype[i] != DB_ZONETYPE_QUAD)
                    return 0;
            }
        }
        else if (shapetype[0] == DB_ZONETYPE_HEX)
        {
            ntopo = 3;
            for (i = 1; i < nshapes; i++)
            {
                zncnt += shapecnt[i];
                if (shapetype[i] != DB_ZONETYPE_HEX)
                    return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if (ndims == 2)
        {
            ntopo = 2;
            for (i = 0; i < nshapes; i++)
            {
                zncnt += shapecnt[i];
                if (shapesize[i] != 4)
                    return 0;
            }
        }
        else if (ndims == 3)
        {
            ntopo = 3;
            for (i = 0; i < nshapes; i++)
            {
                zncnt += shapecnt[i];
                if (shapesize[i] != 8)
                    return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    db_hdf5_hzip_clear_params();
    db_hdf5_hzip_params.iszl = 1;
    db_hdf5_hzip_params.zlname = name;
    db_hdf5_hzip_params.dbfile = dbfile;
    RegisterNodelist(dbfile, name, 0, ntopo, zncnt, origin, nodelist);
    return ALLOW_MESH_COMPRESSION;

#else

    return 0;

#endif
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutZonelist
 *
 * Purpose:     Writes a zonelist to a file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  1, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutZonelist(DBfile *_dbfile, char const *name, int nzones, int ndims,
                    int const *nodelist, int lnodelist, int origin, int const *shapesize,
                    int const *shapecnt, int nshapes)
{
    db_hdf5_PutZonelist2(_dbfile, name, nzones, ndims, nodelist, lnodelist,
                         origin, 0, 0, NULL, shapesize, shapecnt, nshapes,
                         NULL);
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutZonelist2
 *
 * Purpose:     Write a ucd zonelist object into the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Monday, April 12, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added an option list argument to duplicate changes to the
 *              PDB driver. Added gzoneno property.
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Jul 17 15:20:31 PDT 2008
 *   Added code to prepare for possible compression. Changed call to
 *   write nodelist to use compwrz.
 *
 *   Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *   Changed how long long global node/zone numbers are supported
 *   from a int (bool), "llong_gnode|zoneno" to an int holding
 *   the actual datatype. The type is assumed int if it its
 *   value is zero or it does not exist. Otherwise, the type is
 *   is whatever is stored in gnznodtype member. 
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutZonelist2(DBfile *_dbfile, char const *name, int nzones, int ndims,
                     int const *nodelist, int lnodelist, int origin,
                     int lo_offset, int hi_offset, int const *shapetype,
                     int const *shapesize, int const *shapecnt, int nshapes,
                     DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBzonelist_mt       m;
    int                 compressionFlags;

    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        memset(&_uzl, 0, sizeof _uzl);
        db_ProcessOptlist(DB_ZONELIST, optlist);

        /* Prepare for possible compression of zonelist */
        compressionFlags = PrepareForZonelistCompression(dbfile,
            name, origin, ndims, nshapes, shapetype, shapesize, shapecnt, nodelist);

        /* Write variable arrays (currently only support compression of nodelist) */
        db_hdf5_compwrz(dbfile, DB_INT, 1, &lnodelist, nodelist,
            m.nodelist/*out*/, friendly_name(_dbfile,name,"_nodelist", 0), compressionFlags);
        db_hdf5_compwr(dbfile, DB_INT, 1, &nshapes, shapecnt,
            m.shapecnt/*out*/, friendly_name(_dbfile,name,"_shapecnt", 0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &nshapes, shapesize,
            m.shapesize/*out*/, friendly_name(_dbfile,name,"_shapesize", 0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &nshapes, shapetype,
            m.shapetype/*out*/, friendly_name(_dbfile,name,"_shapetype", 0));

        if (_uzl._llong_gzoneno)
            db_hdf5_compwr(dbfile, DB_LONG_LONG, 1, &nzones, _uzl._gzoneno,
                m.gzoneno/*out*/, friendly_name(_dbfile,name,"_gzoneno", 0));
        else
            db_hdf5_compwr(dbfile, DB_INT, 1, &nzones, _uzl._gzoneno,
                m.gzoneno/*out*/, friendly_name(_dbfile,name,"_gzoneno", 0));

        if (_uzl._ghost_zone_labels)
        {
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &nzones, _uzl._ghost_zone_labels,
                m.ghost_zone_labels/*out*/, friendly_name(_dbfile,name,"_ghost_zone_labels", 0));
        }

        if (_uzl._alt_zonenum_vars) {
            int len, nvars=-1; char *tmp;
            DBStringArrayToStringList((char const * const *)_uzl._alt_zonenum_vars, nvars, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.alt_zonenum_vars/*out*/, friendly_name(_dbfile,name, "_alt_zonenum_vars",0));
            FREE(tmp);
        }

        /* Build header in memory */
        m.ndims = ndims;
        m.nzones = nzones;
        m.nshapes = nshapes;
        m.lnodelist = lnodelist;
        m.origin = origin;
        m.lo_offset = lo_offset;
        m.hi_offset = hi_offset;
        m.gnznodtype = _uzl._llong_gzoneno?DB_LONG_LONG:0;

        /* Write header to file */
        STRUCT(DBzonelist) {
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nzones)       MEMBER_S(int, nzones);
            if (m.nshapes)      MEMBER_S(int, nshapes);
            if (m.lnodelist)    MEMBER_S(int, lnodelist);
            if (m.origin)       MEMBER_S(int, origin);
            if (m.lo_offset)    MEMBER_S(int, lo_offset);
            if (m.hi_offset)    MEMBER_S(int, hi_offset);
            MEMBER_S(str(m.nodelist), nodelist);
            MEMBER_S(str(m.shapecnt), shapecnt);
            MEMBER_S(str(m.shapesize), shapesize);
            MEMBER_S(str(m.shapetype), shapetype);
            MEMBER_S(str(m.gzoneno), gzoneno);
            if (m.gnznodtype)   MEMBER_S(int, gnznodtype);
            MEMBER_S(str(m.ghost_zone_labels), ghost_zone_labels);
            MEMBER_S(str(m.alt_zonenum_vars), alt_zonenum_vars);
        } OUTPUT(dbfile, DB_ZONELIST, name, &m);
        
    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutPHZonelist
 *
 * Purpose:     Write a DBphzonelist object into the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Monday, April 12, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added an option list argument to duplicate changes to the
 *              PDB driver. Added gzoneno property.
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Fri Nov 13 15:26:38 PST 2009
 *   Add support for long long global node/zone numbers.
 *
 *   Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *   Changed how long long global node/zone numbers are supported
 *   from a int (bool), "llong_gnode|zoneno" to an int holding
 *   the actual datatype. The type is assumed int if it its
 *   value is zero or it does not exist. Otherwise, the type is
 *   is whatever is stored in gnznodtype member. 
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutPHZonelist(DBfile *_dbfile, char const *name, 
                      int nfaces, int const *nodecnt, int lnodelist, int const *nodelist,
                      char const *extface,
                      int nzones, int const *facecnt, int lfacelist, int const *facelist,
                      int origin, int lo_offset, int hi_offset,
                      DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBphzonelist_mt       m;

    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        memset(&_phzl, 0, sizeof _phzl);
        db_ProcessOptlist(DB_PHZONELIST, optlist);
        
        /* Write variable arrays */
        db_hdf5_compwr(dbfile, DB_INT, 1, &nfaces, nodecnt,
            m.nodecnt/*out*/, friendly_name(_dbfile,name,"_nodecnt", 0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &lnodelist, nodelist,
            m.nodelist/*out*/, friendly_name(_dbfile,name,"_nodelist", 0));
        db_hdf5_compwr(dbfile, DB_CHAR, 1, &nfaces, extface,
            m.extface/*out*/, friendly_name(_dbfile,name,"_extface", 0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &nzones, facecnt,
            m.facecnt/*out*/, friendly_name(_dbfile,name,"_facecnt", 0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &lfacelist, facelist,
            m.facelist/*out*/, friendly_name(_dbfile,name,"_facelist", 0));
        if (_phzl._llong_gzoneno)
            db_hdf5_compwr(dbfile, DB_LONG_LONG, 1, &nzones, _phzl._gzoneno,
                m.gzoneno/*out*/, friendly_name(_dbfile,name,"_gzoneno", 0));
        else
            db_hdf5_compwr(dbfile, DB_INT, 1, &nzones, _phzl._gzoneno,
                m.gzoneno/*out*/, friendly_name(_dbfile,name,"_gzoneno", 0));

        if (_phzl._ghost_zone_labels)
        {
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &nzones, _phzl._ghost_zone_labels,
                m.ghost_zone_labels/*out*/, friendly_name(_dbfile,name,"_ghost_zone_labels", 0));
        }

        if (_phzl._alt_zonenum_vars) {
            int len, nvars=-1; char *tmp;
            DBStringArrayToStringList((char const * const *)_phzl._alt_zonenum_vars, nvars, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.alt_zonenum_vars/*out*/, friendly_name(_dbfile,name, "_alt_zonenum_vars",0));
            FREE(tmp);
        }

        /* Build header in memory */
        m.nfaces = nfaces;
        m.lnodelist = lnodelist;
        m.nzones = nzones;
        m.lfacelist = lfacelist;
        m.origin = origin;
        m.lo_offset = lo_offset;
        m.hi_offset = hi_offset;
        m.gnznodtype = _phzl._llong_gzoneno?DB_LONG_LONG:0;

        /* Write header to file */
        STRUCT(DBphzonelist) {
            if (m.nfaces)       MEMBER_S(int, nfaces);
            if (m.lnodelist)    MEMBER_S(int, lnodelist);
            if (m.nzones)       MEMBER_S(int, nzones);
            if (m.lfacelist)    MEMBER_S(int, lfacelist);
            if (m.origin)       MEMBER_S(int, origin);
            if (m.lo_offset)    MEMBER_S(int, lo_offset);
            if (m.hi_offset)    MEMBER_S(int, hi_offset);
            MEMBER_S(str(m.nodecnt), nodecnt);
            MEMBER_S(str(m.nodelist), nodelist);
            MEMBER_S(str(m.extface), extface);
            MEMBER_S(str(m.facecnt), facecnt);
            MEMBER_S(str(m.facelist), facelist);
            MEMBER_S(str(m.gzoneno), gzoneno);
            if (m.gnznodtype)   MEMBER_S(int, gnznodtype);
            MEMBER_S(str(m.ghost_zone_labels), ghost_zone_labels);
        } OUTPUT(dbfile, DB_PHZONELIST, name, &m);
        
    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    PrepareForZonelistDecompression
 *
 * Purpose:     Does some small work to prepare for possible zonelist 
 *              de compression 
 *
 * Programmer:  Mark C. Miller, Thu Jul 17 15:07:21 PDT 2008
 *-------------------------------------------------------------------------
 */
static void
PrepareForZonelistDecompression(DBfile_hdf5* dbfile, char const *zlname,
    char const *meshname, int origin)
{
#ifdef HAVE_HZIP
    db_hdf5_hzip_clear_params();
    db_hdf5_hzip_params.dbfile = dbfile;
    db_hdf5_hzip_params.iszl = 1;
    db_hdf5_hzip_params.zlname = zlname;
    db_hdf5_hzip_params.meshname = meshname;
    db_hdf5_hzip_params.zlorigin = origin;
#endif
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetZonelist
 *
 * Purpose:     Reads a zonelist object from the file
 *
 * Return:      Success:        Ptr to new zonelist
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, April  2, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-14
 *              Added the `gzoneno' property to mirror changes made to the
 *              PDB driver.
 *
 *              Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *              Made it behave identically to PDB driver (for better or
 *              worse) when called from GetUcdmesh. Added support for
 *              dataReadMask
 *
 *              Mark C. Miller, Thu Jul 17 15:21:35 PDT 2008
 *              Added code to prepare for possible zonelist decompression.
 *
 *              Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *              Changed how long long global node/zone numbers are supported
 *              from a int (bool), "llong_gnode|zoneno" to an int holding
 *              the actual datatype. The type is assumed int if it its
 *              value is zero or it does not exist. Otherwise, the type is
 *              is whatever is stored in gnznodtype member. 
 *
 *              Mark C. Miller, Tue Jul 27 23:20:27 PDT 2010
 *              Removed odd-ball logic to summarily set min/max index to
 *              zero unless 'calledFromGetUcdmesh'
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBzonelist *
db_hdf5_GetZonelist(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetZonelist";
    hid_t               o=-1, attr=-1;
    int                 _objtype;
    DBzonelist_mt       m;
    DBzonelist          *zl=NULL;

    PROTECT {
        /* Open object and make sure it's a zonelist */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_ZONELIST!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read zonelist data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBzonelist_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a zonelist object and initialize meta data */
        if (NULL==(zl=DBAllocZonelist())) return NULL;
        zl->ndims = m.ndims;
        zl->nzones = m.nzones;
        zl->nshapes = m.nshapes;
        zl->lnodelist = m.lnodelist;
        zl->origin = m.origin;
        zl->min_index = m.lo_offset;
        zl->max_index = m.nzones - m.hi_offset - 1;

        /* Prepare for possible zonelist decompression */
        PrepareForZonelistDecompression(dbfile, name,
            calledFromGetUcdmesh, zl->origin);

        /* Read the raw data */
        if (DBGetDataReadMask2File(_dbfile) & DBZonelistInfo)
        {
            zl->shapecnt = (int *)db_hdf5_comprd(dbfile, db_hdf5_resolvename(_dbfile, name, m.shapecnt), 1);
            zl->shapesize = (int *)db_hdf5_comprd(dbfile, db_hdf5_resolvename(_dbfile, name, m.shapesize), 1);
            zl->shapetype = (int *)db_hdf5_comprd(dbfile, db_hdf5_resolvename(_dbfile, name, m.shapetype), 1);
            zl->nodelist = (int *)db_hdf5_comprd(dbfile, db_hdf5_resolvename(_dbfile, name, m.nodelist), 1);
        }
        if (DBGetDataReadMask2File(_dbfile) & DBZonelistGlobZoneNo)
            zl->gzoneno = db_hdf5_comprd(dbfile, db_hdf5_resolvename(_dbfile, name, m.gzoneno), 1);
        zl->gnznodtype = m.gnznodtype?m.gnznodtype:DB_INT;
        if (DBGetDataReadMask2File(_dbfile) & DBZonelistGhostZoneLabels)
            zl->ghost_zone_labels = (char *)db_hdf5_comprd(dbfile, db_hdf5_resolvename(_dbfile, name, m.ghost_zone_labels), 1);

        /* alternate zone number variables */
        {
            int nvars = -1;
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, db_hdf5_resolvename(_dbfile, name, m.alt_zonenum_vars), 1);
            if (tmpnames)
                zl->alt_zonenum_vars = DBStringListToStringArray(tmpnames, &nvars, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        H5Tclose(o);
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeZonelist(zl);
    } END_PROTECT;

    return zl;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetPHZonelist
 *
 * Purpose:     Reads a DBphzonelist object from the file
 *
 * Return:      Success:        Ptr to new DBphzonelist
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, April  2, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-14
 *              Added the `gzoneno' property to mirror changes made to the
 *              PDB driver.
 *
 *  Mark C. Miller, Wed Jan 27 10:37:25 PST 2010
 *  Added missing initialization of gnznodtype
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBphzonelist *
db_hdf5_GetPHZonelist(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetPHZonelist";
    hid_t               o=-1, attr=-1;
    int                 _objtype;
    DBphzonelist_mt       m;
    DBphzonelist          *phzl=NULL;

    PROTECT {
        /* Open object and make sure it's a phzonelist */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_PHZONELIST!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read phzonelist data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBphzonelist_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a phzonelist object and initialize meta data */
        if (NULL==(phzl=DBAllocPHZonelist())) return NULL;
        phzl->nfaces = m.nfaces;
        phzl->lnodelist = m.lnodelist;
        phzl->nzones = m.nzones;
        phzl->lfacelist = m.lfacelist;
        phzl->origin = m.origin;
        phzl->lo_offset = m.lo_offset;
        phzl->hi_offset = m.hi_offset;
        phzl->gnznodtype = m.gnznodtype?m.gnznodtype:DB_INT;

        if (DBGetDataReadMask2File(_dbfile) & DBZonelistInfo)
        {
            phzl->nodecnt = (int *)db_hdf5_comprd(dbfile, m.nodecnt, 1);
            phzl->nodelist = (int *)db_hdf5_comprd(dbfile, m.nodelist, 1);
            phzl->extface = (char *)db_hdf5_comprd(dbfile, m.extface, 1);
            phzl->facecnt = (int *)db_hdf5_comprd(dbfile, m.facecnt, 1);
            phzl->facelist = (int  *)db_hdf5_comprd(dbfile, m.facelist, 1);
        }
        if (DBGetDataReadMask2File(_dbfile) & DBZonelistGlobZoneNo)
            phzl->gzoneno = db_hdf5_comprd(dbfile, m.gzoneno, 1);
        if (DBGetDataReadMask2File(_dbfile) & DBZonelistGhostZoneLabels)
            phzl->ghost_zone_labels = (char *)db_hdf5_comprd(dbfile, m.ghost_zone_labels, 1);

        /* alternate zone number variables */
        {
            int nvars = -1;
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, m.alt_zonenum_vars, 1);
            if (tmpnames)
                phzl->alt_zonenum_vars = DBStringListToStringArray(tmpnames, &nvars, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        H5Tclose(o);
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreePHZonelist(phzl);
    } END_PROTECT;

    return phzl;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutMaterial
 *
 * Purpose:     Write a material object to the file
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Friday, April  2, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *   Added support for dataReadMask. Added warning regarding missing
 *   material name functionality
 *
 *   Mark C. Miller, August 9, 2004
 *   Added code to output optional material names
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Feb 11 09:40:10 PST 2010
 *   Set global values in _ma to zero after use.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutMaterial(
    DBfile *_dbfile,
    char const *name,
    char const *mname,
    int nmat,
    int const *matnos,
    int const *matlist,
    int const *dims,
    int ndims,
    int const *mix_next,
    int const *mix_mat,
    int const *mix_zone,
    void const *mix_vf,
    int mixlen,
    int datatype,
    DBoptlist const *optlist
)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBmaterial_mt       m;
    int                 i, nels, is_empty = 1;
    char               *s = NULL;

    for (i = 0; i < ndims; i++)
    {
        if (dims[i] > 0)
        {
            is_empty = 0;
            break;
        }
    }   

    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        db_ProcessOptlist(DB_MATERIAL, optlist);
        nels = 1;
        for (i=0; i<ndims; i++) nels *= dims[i];

        /* Write raw data arrays */
        if (!is_empty)
            db_hdf5_compwr(dbfile, DB_INT, 1, &nels, matlist, m.matlist/*out*/, friendly_name(_dbfile,name,"_matlist", 0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &nmat, matnos,
            m.matnos/*out*/, friendly_name(_dbfile,name,"_matnos", 0));
        if (!is_empty && mixlen>0) {
            db_hdf5_compwr(dbfile, datatype, 1, &mixlen, mix_vf,
                m.mix_vf/*out*/, friendly_name(_dbfile,name,"_mix_vf", 0));
            db_hdf5_compwr(dbfile, DB_INT, 1, &mixlen, mix_next,
                m.mix_next/*out*/, friendly_name(_dbfile,name,"_mix_next", 0));
            db_hdf5_compwr(dbfile, DB_INT, 1, &mixlen, mix_mat,
                m.mix_mat/*out*/, friendly_name(_dbfile,name,"_mix_mat", 0));
            db_hdf5_compwr(dbfile, DB_INT, 1, &mixlen, mix_zone,
                m.mix_zone/*out*/, friendly_name(_dbfile,name,"_mix_zone", 0));
        }

        if (_ma._matnames != NULL) {
            int len;
            DBStringArrayToStringList((char const * const *)_ma._matnames, nmat, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.matnames/*out*/,
                friendly_name(_dbfile,name, "_matnames", 0));
            FREE(s);
            _ma._matnames = NULL;
        }

        if (_ma._matcolors != NULL) {
            int len;
            DBStringArrayToStringList((char const * const *)_ma._matcolors, nmat, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.matcolors/*out*/,
                friendly_name(_dbfile,name,"_matcolors", 0));
            FREE(s);
            _ma._matcolors = NULL;
        }
        
        /* Build header in memory */
        m.ndims = ndims;
        m.nmat = nmat;
        m.mixlen = mixlen;
        m.origin = _ma._origin;
        m.major_order = _ma._majororder;
        m.allowmat0 = _ma._allowmat0;
        m.guihide = _ma._guihide;
        m.datatype = (DB_FLOAT==datatype || DB_DOUBLE==datatype)?0:datatype;
        strcpy(m.meshid, OPT(mname));
        for (nels=1, i=0; i<ndims; i++) {
            m.dims[i] = dims[i];
        }
        
        /* Write header to file */
        STRUCT(DBmaterial) {
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nmat)         MEMBER_S(int, nmat);
            if (m.mixlen)       MEMBER_S(int, mixlen);
            if (m.origin)       MEMBER_S(int, origin);
            if (m.major_order)  MEMBER_S(int, major_order);
            if (m.datatype)     MEMBER_S(int, datatype);
            if (m.allowmat0)    MEMBER_S(int, allowmat0);
            if (m.guihide)      MEMBER_S(int, guihide);
            MEMBER_3(int, dims);
            MEMBER_S(str(m.meshid), meshid);
            MEMBER_S(str(m.matlist), matlist);
            MEMBER_S(str(m.matnos), matnos);
            MEMBER_S(str(m.mix_vf), mix_vf);
            MEMBER_S(str(m.mix_next), mix_next);
            MEMBER_S(str(m.mix_mat), mix_mat);
            MEMBER_S(str(m.mix_zone), mix_zone);
            MEMBER_S(str(m.matnames), matnames);
            MEMBER_S(str(m.matcolors), matcolors);
        } OUTPUT(dbfile, DB_MATERIAL, name, &m);

        FREE(s);

    } CLEANUP {
        FREE(s);
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetMaterial
 *
 * Purpose:     Reads a material object from the file.
 *
 * Return:      Success:        Ptr to new material
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, April  2, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *   Made it set correct datatype. Added support for dataReadMask
 *
 *   Mark C. Miller, August 9, 2004
 *   Added code to read in optional material names
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmaterial *
db_hdf5_GetMaterial(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetMaterial";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i, nels;
    DBmaterial_mt       m;
    DBmaterial          *ma=NULL;
    char                *s=NULL;
    
    PROTECT {
        /* Open object and make sure it's a material */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_MATERIAL!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBmaterial_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (NULL==(ma=DBAllocMaterial())) return NULL;
        ma->name = BASEDUP(name);
        ma->meshname = OPTDUP(m.meshid);
        ma->ndims = m.ndims;
        ma->origin = m.origin;
        ma->major_order = m.major_order;
        ma->allowmat0 = m.allowmat0;
        ma->guihide = m.guihide;
        ma->nmat = m.nmat;
        ma->mixlen = m.mixlen;
        if (ma->mixlen == 0)
            ma->datatype = DB_NOTYPE;
        else if ((ma->datatype = db_hdf5_GetVarType(_dbfile, m.mix_vf)) < 0)
            ma->datatype = DB_NOTYPE;
        if (force_single_g) ma->datatype = DB_FLOAT;
        for (nels=1, i=0; i<m.ndims; i++) {
            ma->dims[i] = m.dims[i];
            ma->stride[i] = nels;
            nels *= m.dims[i];
        }

        /* Read the raw data */
        if (DBGetDataReadMask2File(_dbfile) & DBMatMatlist)
            ma->matlist = (int *)db_hdf5_comprd(dbfile, m.matlist, 1);
        if (DBGetDataReadMask2File(_dbfile) & DBMatMatnos)
            ma->matnos = (int *)db_hdf5_comprd(dbfile, m.matnos, 1);
        if (DBGetDataReadMask2File(_dbfile) & DBMatMixList)
        {
            ma->mix_vf = db_hdf5_comprd(dbfile, m.mix_vf, 0);
            ma->mix_next = (int *)db_hdf5_comprd(dbfile, m.mix_next, 1);
            ma->mix_mat = (int *)db_hdf5_comprd(dbfile, m.mix_mat, 1);
            ma->mix_zone = (int *)db_hdf5_comprd(dbfile, m.mix_zone, 1);
        }
        if (DBGetDataReadMask2File(_dbfile) & DBMatMatnames)
        {
            s = (char *)db_hdf5_comprd(dbfile, m.matnames, 1);
            if (s) ma->matnames = DBStringListToStringArray(s, &(ma->nmat), !skipFirstSemicolon);
            FREE(s);
        }
        if (DBGetDataReadMask2File(_dbfile) & DBMatMatcolors)
        {
            s = (char *)db_hdf5_comprd(dbfile, m.matcolors, 1);
            if (s) ma->matcolors = DBStringListToStringArray(s, &(ma->nmat), !skipFirstSemicolon);
            FREE(s);
        }

        H5Tclose(o);
        FREE(s);

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeMaterial(ma);
        FREE(s);
    } END_PROTECT;

    return ma;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutMatspecies
 *
 * Purpose:     Write a matspecies object to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Removed the `origin' property to duplicate changes made to
 *              the PDB driver.
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *   Added names and colors for species.
 *
 *   Mark C. Miller, Thu Feb 11 09:40:10 PST 2010
 *   Set global values in _ms to zero after use.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutMatspecies(DBfile *_dbfile, char const *name, char const *matname, int nmat,
                      int const *nmatspec, int const *speclist, int const *dims, int ndims,
                      int nspecies_mf, void const *species_mf, int const *mix_speclist,
                      int mixlen, int datatype, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBmatspecies_mt     m;
    char               *s = NULL;
    int                 i, nels, nstrs = 0, is_empty = 1;

    for (i = 0; i < ndims; i++)
    {
        if (dims[i] > 0)
        {
            is_empty = 0;
            break;
        }
    }   
    
    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        db_ProcessOptlist(DB_MATSPECIES, optlist);

        /* Write raw data arrays */
        nels = 1;
        for (i=0; i<ndims; i++) nels *= dims[i];
        if (!is_empty)
            db_hdf5_compwr(dbfile, DB_INT, 1, &nels, speclist, m.speclist/*out*/, friendly_name(_dbfile,name,"_speclist", 0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &nmat, nmatspec, m.nmatspec/*out*/,
            friendly_name(_dbfile,name,"_nmatspec", 0));
        if (!is_empty)
        {
            db_hdf5_compwr(dbfile, datatype, 1, &nspecies_mf, species_mf, m.species_mf/*out*/,
                friendly_name(_dbfile,name,"_species_mf", 0));
            db_hdf5_compwr(dbfile, DB_INT, 1, &mixlen, mix_speclist, m.mix_speclist/*out*/,
                friendly_name(_dbfile,name,"_mix_speclist", 0));
        }
        
        if (_ms._specnames != NULL) {
            int len;
            for (i = 0; i < nmat; i++)
                nstrs += nmatspec[i];
            DBStringArrayToStringList((char const * const *)_ms._specnames, nstrs, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.specnames/*out*/,
                friendly_name(_dbfile,name, "_species_names", 0));
            FREE(s);
            _ms._specnames = NULL;
        }

        if (_ms._speccolors != NULL) {
            int len;
            if (nstrs == 0)
            {
                for (i = 0; i < nmat; i++)
                    nstrs += nmatspec[i];
            }
            DBStringArrayToStringList((char const * const *)_ms._speccolors, nstrs, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.speccolors/*out*/,
                friendly_name(_dbfile,name,"_speccolors", 0));
            FREE(s);
            _ms._speccolors = NULL;
        }

        /* Build header in memory */
        m.ndims = ndims;
        m.nmat = nmat;
        m.nspecies_mf = nspecies_mf;
        m.mixlen = mixlen;
        m.major_order = _ms._majororder;
        m.guihide = _ms._guihide;
        m.datatype = (DB_FLOAT==datatype || DB_DOUBLE==datatype)?0:datatype;
        strcpy(m.matname, OPT(matname));
        for (i=0; i<ndims; i++) m.dims[i] = dims[i];
        
        /* Write header to file */
        STRUCT(DBmatspecies) {
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nmat)         MEMBER_S(int, nmat);
            if (m.nspecies_mf)  MEMBER_S(int, nspecies_mf);
            if (m.mixlen)       MEMBER_S(int, mixlen);
            if (m.major_order)  MEMBER_S(int, major_order);
            if (m.datatype)     MEMBER_S(int, datatype);
            if (m.guihide)      MEMBER_S(int, guihide);
            MEMBER_3(int, dims);
            MEMBER_S(str(m.matname), matname);
            MEMBER_S(str(m.speclist), speclist);
            MEMBER_S(str(m.nmatspec), nmatspec);
            MEMBER_S(str(m.species_mf), species_mf);
            MEMBER_S(str(m.mix_speclist), mix_speclist);
            MEMBER_S(str(m.specnames), specnames);
            MEMBER_S(str(m.speccolors), speccolors);
        } OUTPUT(dbfile, DB_MATSPECIES, name, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetMatspecies
 *
 * Purpose:     Reads a matspecies object from the file.
 *
 * Return:      Success:        Ptr to new matspecies object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Removed the `origin' property, duplicating changes made to
 *              the PDB driver.
 *
 *              Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *              Made it set correct datatype.
 *
 *              Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *              Added names and colors for species.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmatspecies *
db_hdf5_GetMatspecies(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetMatspecies";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i, nels, nstrs = 0;
    DBmatspecies_mt     m;
    DBmatspecies        *ms=NULL;
    char                *s=NULL;

    PROTECT {
        /* Open object and make sure it's a matspecies */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_MATSPECIES!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBmatspecies_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (NULL==(ms=DBAllocMatspecies())) return NULL;
        ms->name = BASEDUP(name);
        ms->matname = OPTDUP(m.matname);
        ms->nmat = m.nmat;
        ms->ndims = m.ndims;
        ms->guihide = m.guihide;
        ms->major_order = m.major_order;
        ms->nspecies_mf = m.nspecies_mf;
        ms->mixlen = m.mixlen;
        if ((ms->datatype = db_hdf5_GetVarType(_dbfile, m.species_mf)) < 0)
            ms->datatype = silo2silo_type(m.datatype);
        if (force_single_g) ms->datatype = DB_FLOAT;
        for (i=0, nels=1; i<m.ndims; i++) {
            ms->dims[i] = m.dims[i];
            ms->stride[i] = nels;
            nels *= m.dims[i];
        }

        /* Read the raw data */
        ms->nmatspec = (int *)db_hdf5_comprd(dbfile, m.nmatspec, 1);
        ms->species_mf = db_hdf5_comprd(dbfile, m.species_mf, 0);
        ms->speclist = (int *)db_hdf5_comprd(dbfile, m.speclist, 1);
        ms->mix_speclist = (int *)db_hdf5_comprd(dbfile, m.mix_speclist, 1);
        if (ms->nmatspec && (DBGetDataReadMask2File(_dbfile) & DBMatMatnames))
        {
            for (i=0; i < ms->nmat; i++)
                nstrs += ms->nmatspec[i];
            s = (char *)db_hdf5_comprd(dbfile, m.specnames, 1);
            if (s) ms->specnames = DBStringListToStringArray(s, &nstrs, !skipFirstSemicolon);
            FREE(s);
        }
        if (ms->nmatspec && (DBGetDataReadMask2File(_dbfile) & DBMatMatcolors))
        {
            if (nstrs == 0)
            {
                for (i=0; i < ms->nmat; i++)
                    nstrs += ms->nmatspec[i];
            }
            s = (char *)db_hdf5_comprd(dbfile, m.speccolors, 1);
            if (s) ms->speccolors = DBStringListToStringArray(s, &nstrs, !skipFirstSemicolon);
            FREE(s);
        }

        H5Aclose(attr);
        H5Tclose(o);

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeMatspecies(ms);
    } END_PROTECT;
    return ms;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutMultimesh
 *
 * Purpose:     Write a multi-block mesh object to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Friday, April  2, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `ngroups', `blockorigin', and `grouporigin' to
 *              duplicate changes to the PDB driver.
 *
 *              Eric Brugger, 2004-03-12
 *              Split the declaration and initialization of sizes
 *              into multiple statements so that it compiles on old
 *              sgi compilers.
 *
 *              Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *              Added call to reset global data
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *
 *   Mark C. Miller, Wed Jul 14 20:48:15 PDT 2010
 *   Added support for namescheme/empty_list options for multi-block objs.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutMultimesh(DBfile *_dbfile, char const *name, int nmesh,
                     char const *const *meshnames, int const *meshtypes,
                     DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBmultimesh_mt      m;
    int                 i, len;
    char                *s=NULL;
    char                *t=NULL;
    
    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        db_ResetGlobalData_MultiMesh();
        db_ProcessOptlist(DB_MULTIMESH, optlist);

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _mm._time_set, _mm._time,
            _mm._dtime_set, _mm._dtime, _mm._cycle);

        /*
         * Create a character string which is a semi-colon separated list of
         * mesh names.
         */
        if (meshnames)
        {
            for (i=len=0; i<nmesh; i++) len += strlen(meshnames[i])+1;
            s = (char *)malloc(len+1);
            for (i=len=0; i<nmesh; i++) {
                if (i) s[len++] = ';';
                strcpy(s+len, meshnames[i]);
                len += strlen(meshnames[i]);
            }
            len++; /*count null*/
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s,
                m.meshnames/*out*/, friendly_name(_dbfile,name,"_meshnames", 0));
        }
        
        /* Write raw data arrays */
        if (meshtypes)
            db_hdf5_compwr(dbfile, DB_INT, 1, &nmesh, meshtypes,
                m.meshtypes/*out*/, friendly_name(_dbfile,name,"_meshtypes", 0));
        if (_mm._extents && _mm._extentssize) {
            int sizes[2];
            sizes[0] = nmesh;
            sizes[1] = _mm._extentssize;
            db_hdf5_compwr(dbfile, DB_DOUBLE, 2, sizes, _mm._extents,
                m.extents/*out*/, friendly_name(_dbfile,name,"_extents",0));
        }
        if (_mm._zonecounts) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &nmesh, _mm._zonecounts,
                m.zonecounts/*out*/, friendly_name(_dbfile,name,"_zoneconts",0));
        }
        if (_mm._has_external_zones) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &nmesh, _mm._has_external_zones,
                m.has_external_zones/*out*/, friendly_name(_dbfile,name,"_has_external_zones",0));
        }
        if (_mm._lgroupings > 0 && _mm._groupings != NULL) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &_mm._lgroupings, _mm._groupings,
                m.groupings/*out*/, friendly_name(_dbfile,name,"_groupings",0));
        }
        if (_mm._lgroupings > 0 && _mm._groupnames != NULL) {
           DBStringArrayToStringList((char const * const *)_mm._groupnames, 
                           _mm._lgroupings, &t, &len);
           db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, t,
                m.groupnames/*out*/, friendly_name(_dbfile,name,"_groupnames",0));
           FREE(t);
        }
        if (_mm._file_ns)
        {
           len = strlen(_mm._file_ns)+1;
           db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, _mm._file_ns,
                m.file_ns_name/*out*/, friendly_name(_dbfile,name,"_file_ns",0));
        }
        if (_mm._block_ns)
        {
           len = strlen(_mm._block_ns)+1;
           db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, _mm._block_ns,
                m.block_ns_name/*out*/, friendly_name(_dbfile,name,"_block_ns",0));
        }
        if (_mm._empty_list && _mm._empty_cnt>0) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &_mm._empty_cnt, _mm._empty_list,
                m.empty_list/*out*/, friendly_name(_dbfile,name,"_empty_list",0));
        }

        if (_mm._alt_nodenum_vars) {
            int len, nvars=-1; char *tmp;
            DBStringArrayToStringList((char const * const *)_mm._alt_nodenum_vars, nvars, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.alt_nodenum_vars/*out*/, friendly_name(_dbfile,name, "_alt_nodenum_vars",0));
            FREE(tmp);
        }
        
        if (_mm._alt_zonenum_vars) {
            int len, nvars=-1; char *tmp;
            DBStringArrayToStringList((char const * const *)_mm._alt_zonenum_vars, nvars, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.alt_zonenum_vars/*out*/, friendly_name(_dbfile,name, "_alt_zonenum_vars",0));
            FREE(tmp);
        }
        
        /* Initialize meta data */
        m.nblocks = nmesh;
        m.cycle = _mm._cycle;
        m.time = _mm._time;
        m.dtime = _mm._dtime;
        m.ngroups = _mm._ngroups;
        m.blockorigin = _mm._blockorigin;
        m.grouporigin = _mm._grouporigin;
        m.extentssize = _mm._extentssize;
        m.guihide = _mm._guihide;
        m.lgroupings = _mm._lgroupings;
        m.tv_connectivity = _mm._tv_connectivity;
        m.disjoint_mode = _mm._disjoint_mode;
        m.topo_dim = _mm._topo_dim;
        strcpy(m.mrgtree_name, OPT(_mm._mrgtree_name));
        m.block_type = _mm._block_type;
        m.empty_cnt = _mm._empty_cnt;
        m.repr_block_idx = _mm._repr_block_idx;

        /* Write meta data to file */
        STRUCT(DBmultimesh) {
            if (m.nblocks)      MEMBER_S(int, nblocks);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (m.ngroups)      MEMBER_S(int, ngroups);
            if (m.blockorigin)  MEMBER_S(int, blockorigin);
            if (m.grouporigin)  MEMBER_S(int, grouporigin);
            if (_mm._time_set)  MEMBER_S(float, time);
            if (_mm._dtime_set) MEMBER_S(double, dtime);
            if (m.extentssize)  MEMBER_S(int, extentssize);
            if (m.guihide)      MEMBER_S(int, guihide);
            MEMBER_S(str(m.meshtypes), meshtypes);
            MEMBER_S(str(m.meshnames), meshnames);
            MEMBER_S(str(m.extents), extents);
            MEMBER_S(str(m.zonecounts), zonecounts);
            MEMBER_S(str(m.has_external_zones), has_external_zones);
            if (m.lgroupings)   MEMBER_S(int, lgroupings);
            MEMBER_S(str(m.groupings), groupings);
            MEMBER_S(str(m.groupnames), groupnames);
            MEMBER_S(str(m.mrgtree_name), mrgtree_name);
            if (m.tv_connectivity) MEMBER_S(int, tv_connectivity);
            if (m.disjoint_mode)   MEMBER_S(int, disjoint_mode);
            if (m.topo_dim)     MEMBER_S(int, topo_dim);
            MEMBER_S(str(m.file_ns_name), file_ns_name);
            MEMBER_S(str(m.block_ns_name), block_ns_name);
            if (m.block_type)   MEMBER_S(int, block_type);
            MEMBER_S(str(m.empty_list), empty_list);
            if (m.empty_cnt)   MEMBER_S(int, empty_cnt);
            if (m.repr_block_idx)   MEMBER_S(int, repr_block_idx);
            MEMBER_S(str(m.alt_nodenum_vars), alt_nodenum_vars);
            MEMBER_S(str(m.alt_zonenum_vars), alt_zonenum_vars);
        } OUTPUT(dbfile, DB_MULTIMESH, name, &m);

        /* Free resources */
        FREE(s);
        
    } CLEANUP {
        FREE(s);
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetMultimesh
 *
 * Purpose:     Reads a multimesh object from the file.
 *
 * Return:      Success:        Ptr to new multimesh object
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `ngroups', `blockorigin', and `grouporigin' to
 *              duplicate changes to the PDB driver.
 *
 *              Mark C. Miller, Wed Feb  2 07:52:22 PST 2005
 *              Added code to temporarily disable force single when
 *              reading extents
 *
 *   Mark C. Miller, Tue Feb 15 14:53:29 PST 2005
 *   Changed how force_single was handled to deal with possible throw
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *
 *   Mark C. Miller, Wed Jul 14 20:48:15 PDT 2010
 *   Added support for namescheme/empty_list options for multi-block objs.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmultimesh *
db_hdf5_GetMultimesh(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetMultimesh";
    hid_t               o=-1, attr=-1;
    int                 _objtype;
    DBmultimesh_mt      m;
    DBmultimesh         *mm=NULL;
    char                *t=NULL;
    char                *meshnames=NULL;

    PROTECT {
        /* Open object and make sure it's a multimesh */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_MULTIMESH!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBmultimesh_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (NULL==(mm=DBAllocMultimesh(0))) return NULL;
        mm->nblocks = m.nblocks;
        mm->ngroups = m.ngroups;
        mm->blockorigin = m.blockorigin;
        mm->grouporigin = m.grouporigin;
        mm->extentssize = m.extentssize;
        mm->guihide = m.guihide;
        mm->lgroupings = m.lgroupings;
        mm->mrgtree_name = OPTDUP(m.mrgtree_name);
        mm->tv_connectivity = m.tv_connectivity;
        mm->disjoint_mode = m.disjoint_mode;
        /* The value we store to the file for 'topo_dim' member is
           designed such that zero indicates a value that was NOT
           specified in the file. Since zero is a valid topological
           dimension, when we store topo_dim to a file, we always
           add 1. So, we have to subtract it here. This was implemented
           for multimeshes in 4.7 and so is handled correctly for
           them in all cases. */
        mm->topo_dim = m.topo_dim - 1;
        mm->repr_block_idx = m.repr_block_idx - 1;

        /* Read the raw data */
        if (mm->nblocks>0 && (DBGetDataReadMask2File(_dbfile) & DBMBNamesAndTypes))
        {
            mm->meshtypes = (int *)db_hdf5_comprd(dbfile, m.meshtypes, 1);
            meshnames = (char *)db_hdf5_comprd(dbfile, m.meshnames, 1);
            db_StringListToStringArrayMBOpt(meshnames, &(mm->meshnames), &(mm->meshnames_alloc), m.nblocks);
        }

        /* Read optional data */
        if (mm->nblocks>0 && (DBGetDataReadMask2File(_dbfile) & DBMBOptions))
        {
            if (mm->extentssize>0)
               mm->extents = (double*)db_hdf5_comprd(dbfile, m.extents, 1);
            mm->zonecounts =  (int *)db_hdf5_comprd(dbfile, m.zonecounts, 1);
            mm->has_external_zones =  (int *)db_hdf5_comprd(dbfile, m.has_external_zones, 1);
            mm->groupings =  (int *)db_hdf5_comprd(dbfile, m.groupings, 1);
            t = (char *)db_hdf5_comprd(dbfile, m.groupnames, 1);
            if (t) mm->groupnames = DBStringListToStringArray(t, &(mm->lgroupings), !skipFirstSemicolon);
            FREE(t);
        }

        /* Namescheme related stuff */
        mm->file_ns =  (char *)db_hdf5_comprd(dbfile, m.file_ns_name, 1);
        mm->block_ns =  (char *)db_hdf5_comprd(dbfile, m.block_ns_name, 1);
        mm->block_type = m.block_type;
        mm->empty_list =  (int* )db_hdf5_comprd(dbfile, m.empty_list, 1);
        mm->empty_cnt = m.empty_cnt;
        
        /* alternate node number variables */
        {
            int nvars = -1;
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, m.alt_nodenum_vars, 1);
            if (tmpnames)
                mm->alt_nodenum_vars = DBStringListToStringArray(tmpnames, &nvars, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        /* alternate zone number variables */
        {
            int nvars = -1;
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, m.alt_zonenum_vars, 1);
            if (tmpnames)
                mm->alt_zonenum_vars = DBStringListToStringArray(tmpnames, &nvars, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        H5Tclose(o);

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeMultimesh(mm);
        FREE(t);
    } END_PROTECT;
    return mm;
}


/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutMultimeshadj
 *
 * Purpose:     Write a multi-block mesh adjacency object to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Mark C. Miller 
 *              Thursday, September 8, 2005 
 *
 * Modifcations:
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Sat Oct 18 08:22:18 PDT 2008
 *   Added patch from Sean Ahern where the write of the zonelists was
 *   actually referring to the nodelists array.
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *
 *   Mark C. Miller, Sat Mar 27 10:51:48 PDT 2010
 *   Add missing call to H5Tclose(o). Wouldn't have detected this without
 *   changing close degree setting based on DBShowErrors setting.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutMultimeshadj(DBfile *_dbfile, char const *name, int nmesh,
                  int const *meshtypes, int const *nneighbors,
                  int const *neighbors, int const *back,
                  int const *lnodelists, int const * const *nodelists,
                  int const *lzonelists, int const * const *zonelists,
                  DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBmultimeshadj_mt   m;
    int                 i, len, lneighbors, zoff, noff;
    char                *s=NULL;
    hid_t               o=-1, attr=-1, nldset=-1, zldset=-1;
    hid_t               mtype=-1, fspace=-1, mspace=-1;
    static char         *me = "db_hdf5_PutMultimeshadj";
    int                 _objtype;
    
    memset(&m, 0, sizeof m);

    /* compute expected size of neighbors array */
    lneighbors = 0;
    for (i = 0; i < nmesh; i++)
       lneighbors += nneighbors[i];

    PROTECT {

       H5E_BEGIN_TRY {
           o = H5Topen(dbfile->cwg, name, H5P_DEFAULT);
       } H5E_END_TRY;

       if (o >= 0)
       {
            /* Object exists, do some simple sanity checking */
            if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
                H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
                H5Aclose(attr)<0) {
                db_perror((char*)name, E_CALLFAIL, me);
                UNWIND();
            }
            if (DB_MULTIMESHADJ!=(DBObjectType)_objtype) {
                db_perror("not a DBmultimeshadj object", E_BADARGS, me);
                UNWIND();
            }

            /* Read meta data into memory */
            if ((attr=H5Aopen_name(o, "silo"))<0 ||
                H5Aread(attr, DBmultimeshadj_mt5, &m)<0 ||
                H5Aclose(attr)<0) {
                db_perror((char*)name, E_CALLFAIL, me);
                UNWIND();
            }

            /* should add correct sanity checks here */

            H5Tclose(o);

       }
       else
       {

           /* Object doesn't exist, allocate space in the file for the datasets */
           db_ResetGlobalData_MultiMesh();
           db_ProcessOptlist(DB_MULTIMESH, optlist);

           /* Initialize meta data */
           m.nblocks = nmesh;
           m.blockorigin = _mm._blockorigin;
           m.lneighbors = lneighbors;

           /* compute length of neighbors, back, lnodelists, nodelists,
              lzonelists, zonelists arrays */
           lneighbors = 0;
           for (i = 0; i < nmesh; i++)
               lneighbors += nneighbors[i];

           db_hdf5_compwr(dbfile, DB_INT, 1, &nmesh, (void*)meshtypes,
               m.meshtypes/*out*/, friendly_name(_dbfile,name, "_meshtypes",0));
           db_hdf5_compwr(dbfile, DB_INT, 1, &nmesh, (void*)nneighbors,
               m.nneighbors/*out*/, friendly_name(_dbfile,name,"_nneighbors",0));
           db_hdf5_compwr(dbfile, DB_INT, 1, &lneighbors, (void*)neighbors,
               m.neighbors/*out*/, friendly_name(_dbfile,name,"_neighbors",0));
           if (back)
           {
               db_hdf5_compwr(dbfile, DB_INT, 1, &lneighbors, (void*)back,
                   m.back/*out*/, friendly_name(_dbfile,name,"_back",0));
           }
           if (lnodelists)
           {
               db_hdf5_compwr(dbfile, DB_INT, 1, &lneighbors, (void*)lnodelists,
                   m.lnodelists/*out*/, friendly_name(_dbfile,name,"_lnodelists",0));
           }
           if (lzonelists)
           {
               db_hdf5_compwr(dbfile, DB_INT, 1, &lneighbors, (void*)lzonelists,
                   m.lzonelists/*out*/, friendly_name(_dbfile,name,"_lzonelists",0));
           }

           /* All object components up to here are invariant and *should*
              be identical in repeated calls. Now, handle the parts of the
              object that can vary from call to call. Reserve space for
              the entire nodelists and/or zonelists arrays */

           if (lnodelists && nodelists) {

               /* compute total length of nodelists array */
               len = 0;
               for (i = 0; i < lneighbors; i++)
                   len += lnodelists[i];
               m.totlnodelists = len;

               /* reserve space for the nodelists array in the file */
               /* negative rank means to reserve space */
               if (db_hdf5_compwr(dbfile, DB_INT, -1, &len, NULL,
                       m.nodelists/*out*/, friendly_name(_dbfile,name,"_nodelists",0))<0) {
                  db_perror ("db_hdf5_compwr", E_CALLFAIL, me) ;
                  UNWIND();
               }
           }

           if (lzonelists && zonelists) {

               /* compute total length of nodelists array */
               len = 0;
               for (i = 0; i < lneighbors; i++)
                   len += lzonelists[i];
               m.totlzonelists = len;

               /* reserve space for the zonelists array in the file */
               /* negative rank means to reserve space */
               if (db_hdf5_compwr(dbfile, DB_INT, -1, &len, NULL,
                       m.zonelists/*out*/, friendly_name(_dbfile,name, "_zonelists",0))<0) {
                  db_perror ("db_hdf5_compwr", E_CALLFAIL, me) ;
                  UNWIND();
               }
           }

           /* hack to maintain backward compatibility with pdb driver */
           db_hdf5_handle_ctdt(dbfile, _mm._time_set, _mm._time,
               _mm._dtime_set, _mm._dtime, _mm._cycle);

           /* Write meta data to file */
           STRUCT(DBmultimeshadj) {
               MEMBER_S(int, nblocks);
               MEMBER_S(int, blockorigin);
               MEMBER_S(int, lneighbors);
               if (m.totlnodelists) MEMBER_S(int, totlnodelists);
               if (m.totlzonelists) MEMBER_S(int, totlzonelists);
               MEMBER_S(str(m.meshtypes), meshtypes);
               MEMBER_S(str(m.nneighbors), nneighbors);
               MEMBER_S(str(m.neighbors), neighbors);
               if (m.back[0]) MEMBER_S(str(m.back), back);
               if (m.lnodelists[0]) MEMBER_S(str(m.lnodelists), lnodelists);
               if (m.nodelists[0]) MEMBER_S(str(m.nodelists), nodelists);
               if (m.lzonelists[0]) MEMBER_S(str(m.lzonelists), lzonelists);
               if (m.zonelists[0]) MEMBER_S(str(m.zonelists), zonelists);
           } OUTPUT(dbfile, DB_MULTIMESHADJ, name, &m);
       }

       if (m.nodelists[0] && 
           (nldset = H5Dopen(dbfile->cwg, m.nodelists, H5P_DEFAULT)) < 0) {
           db_perror((char*)name, E_CALLFAIL, me);
           UNWIND();
       }

       if (m.zonelists[0] &&
           (zldset = H5Dopen(dbfile->cwg, m.zonelists, H5P_DEFAULT)) < 0) {
           db_perror((char*)name, E_CALLFAIL, me);
           UNWIND();
       }

       if ((mtype=silom2hdfm_type(DB_INT))<0) {
           db_perror("datatype", E_BADARGS, me);
           UNWIND();
       }

       /* Ok, now write contents of nodelists and/or zonelists */
       noff = 0;
       zoff = 0;
       for (i = 0; i < lneighbors; i++)
       {
          hsize_t ds_size[H5S_MAX_RANK];

          if (nodelists && lnodelists)
          {
             if (nodelists[i] && lnodelists[i])
             {
                int offset = noff;
                int length = lnodelists[i];
                int stride = 1;

                /* Build the file space selection */
                if ((fspace=build_fspace(nldset, 1, &offset, &length, &stride,
                                        ds_size/*out*/))<0) {
                   db_perror("file data space", E_CALLFAIL, me);
                   UNWIND();
               }

               /* Build the memory data space */
               if ((mspace=H5Screate_simple(1, ds_size, NULL))<0) {
                   db_perror("memory data space", E_CALLFAIL, me);
                   UNWIND();
               }

               /* Write data */
               if (H5Dwrite(nldset, mtype, mspace, fspace, H5P_DEFAULT, nodelists[i])<0) {
                   db_perror("partial write", E_CALLFAIL, me);
                   UNWIND();
               }

               /* Close everything */
               H5Sclose(fspace);
               H5Sclose(mspace);
             }
             noff += lnodelists[i];
          }

          if (zonelists && lzonelists)
          {
             if (zonelists[i] && lzonelists[i])
             {
                int offset = zoff;
                int length = lzonelists[i];
                int stride = 1;

               /* Build the file space selection */
               if ((fspace=build_fspace(zldset, 1, &offset, &length, &stride,
                                        ds_size/*out*/))<0) {
                   db_perror("file data space", E_CALLFAIL, me);
                   UNWIND();
               }

               /* Build the memory data space */
               if ((mspace=H5Screate_simple(1, ds_size, NULL))<0) {
                   db_perror("memory data space", E_CALLFAIL, me);
                   UNWIND();
               }

               /* Write data */
               if (H5Dwrite(zldset, mtype, mspace, fspace, H5P_DEFAULT, zonelists[i])<0) {
                   db_perror("partial write", E_CALLFAIL, me);
                   UNWIND();
               }

               /* Close everything */
               H5Sclose(fspace);
               H5Sclose(mspace);

             }
             zoff += lzonelists[i];
          }
       }

       if (nldset != -1)
           H5Dclose(nldset);
       if (zldset != -1)
           H5Dclose(zldset);

    } CLEANUP {
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetMultimeshadj
 *
 * Purpose:     Reads a multimesh adjacency object from the file.
 *
 * Return:      Success:        Ptr to new multimesh adjacency object
 *
 *              Failure:        NULL
 *
 * Programmer:  Mark C. Miller 
 *              Thursday, September 8, 2005 
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmultimeshadj *
db_hdf5_GetMultimeshadj(DBfile *_dbfile, char const *name, int nmesh,
                        int const *block_map)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetMultimesh";
    hid_t               o=-1, attr=-1, nldset = -1, zldset = -1;
    hid_t               mtype=-1, fspace=-1, mspace=-1;
    DBmultimeshadj_mt   m;
    DBmultimeshadj      *mmadj=NULL;
    char                *typestring = NULL;
    int                 i, j, tmpnmesh, _objtype;
    int                 *offsetmap=0, *offsetmapn=0, *offsetmapz=0, lneighbors, tmpoff;
    int                 blockno;

    PROTECT {
        /* Open object and make sure it's a multimesh */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror((char*)name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_MULTIMESHADJ!=(DBObjectType)_objtype) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBmultimeshadj_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (m.nblocks < 0 || m.lneighbors < 0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }
        if (NULL==(mmadj=DBAllocMultimeshadj(0))) return NULL;
        mmadj->nblocks = m.nblocks;
        mmadj->blockorigin = m.blockorigin;
        mmadj->lneighbors = m.lneighbors;

        /* Read the raw data */
        mmadj->meshtypes = (int *)db_hdf5_comprd(dbfile, m.meshtypes, 1);
        mmadj->nneighbors = (int *)db_hdf5_comprd(dbfile, m.nneighbors, 1);
        mmadj->neighbors = (int *)db_hdf5_comprd(dbfile, m.neighbors, 1);
        mmadj->back = (int *)db_hdf5_comprd(dbfile, m.back, 1);

        if ((DBGetDataReadMask2File(_dbfile) & DBMMADJNodelists) ||
            (DBGetDataReadMask2File(_dbfile) & DBMMADJZonelists))
        {
            offsetmap = ALLOC_N(int, mmadj->nblocks);
            lneighbors = 0;
            for (i = 0; (i < mmadj->nblocks) && mmadj->nneighbors; i++)
            {
                if (mmadj->nneighbors[i] < 0 ||
                    lneighbors > INT_MAX - mmadj->nneighbors[i]) {
                    FREE(offsetmap);
                    DBFreeMultimeshadj(mmadj);
                    db_perror((char*)name, E_CALLFAIL, me);
                    UNWIND();
                }
                offsetmap[i] = lneighbors;
                lneighbors += mmadj->nneighbors[i];
            }
        }
 
        if (offsetmap && mmadj->nneighbors &&
            (DBGetDataReadMask2File(_dbfile) & DBMMADJNodelists))
        {
           mmadj->lnodelists = (int *)db_hdf5_comprd(dbfile, m.lnodelists, 1);
           if (mmadj->lnodelists)
           {
               mmadj->nodelists = ALLOC_N(int *, lneighbors); 
               offsetmapn = ALLOC_N(int, mmadj->nblocks);
               tmpoff = 0;
               for (i = 0; i < mmadj->nblocks; i++)
               {
                   offsetmapn[i] = tmpoff;
                   for (j = 0; j < mmadj->nneighbors[i]; j++) {
                      int len = mmadj->lnodelists[offsetmap[i]+j];
                      if (len < 0 || tmpoff > INT_MAX - len) {
                          FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
                          DBFreeMultimeshadj(mmadj);
                          db_perror((char*)name, E_CALLFAIL, me);
                          UNWIND();
                      }
                      tmpoff += len;
                   }
               }
               mmadj->totlnodelists = m.totlnodelists;
           }
        }
 
        if (offsetmap && mmadj->nneighbors &&
            (DBGetDataReadMask2File(_dbfile) & DBMMADJZonelists))
        {
           mmadj->lzonelists = (int *)db_hdf5_comprd(dbfile, m.lzonelists, 1);
           if (mmadj->lzonelists)
           {
               mmadj->zonelists = ALLOC_N(int *, lneighbors); 
               offsetmapz = ALLOC_N(int, mmadj->nblocks);
               tmpoff = 0;
               for (i = 0; i < mmadj->nblocks; i++)
               {
                   offsetmapz[i] = tmpoff;
                   for (j = 0; j < mmadj->nneighbors[i]; j++) {
                      int len = mmadj->lzonelists[offsetmap[i]+j];
                      if (len < 0 || tmpoff > INT_MAX - len) {
                          FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
                          DBFreeMultimeshadj(mmadj);
                          db_perror((char*)name, E_CALLFAIL, me);
                          UNWIND();
                      }
                      tmpoff += len;
                   }
               }
               mmadj->totlzonelists = m.totlzonelists;
            }
        }
        
        tmpnmesh = nmesh;
        if (nmesh <= 0 || !block_map)
            tmpnmesh = mmadj->nblocks;
 
        if (m.nodelists[0] && 
            (nldset = H5Dopen(dbfile->cwg, m.nodelists, H5P_DEFAULT)) < 0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        if (m.zonelists[0] &&
            (zldset = H5Dopen(dbfile->cwg, m.zonelists, H5P_DEFAULT)) < 0) {
            db_perror((char*)name, E_CALLFAIL, me);
            UNWIND();
        }

        if ((mtype=silom2hdfm_type(DB_INT))<0) {
            FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
            DBFreeMultimeshadj(mmadj);
            db_perror("datatype", E_BADARGS, me);
            UNWIND();
        }

        /* This loop could be optimized w.r.t. number of I/O requests
           it makes. The nodelists and/or zonelists could be read in
           a single call. But then we'd have to split it into separate
           arrays duplicating memory */
        for (i = 0; (i < tmpnmesh) &&
                    (DBGetDataReadMask2File(_dbfile) & (DBMMADJNodelists|DBMMADJZonelists)); i++)
        {
           hsize_t ds_size[H5S_MAX_RANK];
           blockno = block_map ? block_map[i] : i;

           if (blockno < 0 || blockno >= mmadj->nblocks) {
               FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
               DBFreeMultimeshadj(mmadj);
               db_perror((char*)name, E_CALLFAIL, me);
               UNWIND();
           }
 
           if (offsetmapn && mmadj->lnodelists && mmadj->nodelists &&
               mmadj->nneighbors && (DBGetDataReadMask2File(_dbfile) & DBMMADJNodelists))
           {
              tmpoff = offsetmapn[blockno];
              for (j = 0; j < mmadj->nneighbors[blockno]; j++)
              {
                 int stride = 1;
                 int len = mmadj->lnodelists[offsetmap[blockno]+j];
                 int *nlist;

                 if (len < 0) {
                     FREE(offsetmap);
                     FREE(offsetmapn);
                     FREE(offsetmapz);
                     DBFreeMultimeshadj(mmadj);
                     db_perror((char*)name, E_CALLFAIL, me);
                     UNWIND();
                 }

                 if (len == 0) {
                     mmadj->nodelists[offsetmap[blockno]+j] = NULL;
                     continue;
                 }
                 nlist = ALLOC_N(int, len);

                 /* Build the file space selection */
                 if ((fspace=build_fspace(nldset, 1, &tmpoff, &len, &stride,
                                          ds_size/*out*/))<0) {
                     FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
                     DBFreeMultimeshadj(mmadj);
                     db_perror("file data space", E_CALLFAIL, me);
                     UNWIND();
                 }

                 /* Build the memory data space */
                 if ((mspace=H5Screate_simple(1, ds_size, NULL))<0) {
                     FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
                     DBFreeMultimeshadj(mmadj);
                     db_perror("memory data space", E_CALLFAIL, me);
                     UNWIND();
                 }

                 P_rdprops = H5P_DEFAULT;
                 if (!DBGetEnableChecksumsFile(_dbfile))
                     P_rdprops = P_ckrdprops;

                 /* Read data */
                 if (H5Dread(nldset, mtype, mspace, fspace, P_rdprops, nlist)<0) {
                     FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
                     DBFreeMultimeshadj(mmadj);
                     hdf5_to_silo_error(name, me);
                     UNWIND();
                 }

                 /* Close everything */
                 H5Sclose(fspace);
                 H5Sclose(mspace);
 
                 mmadj->nodelists[offsetmap[blockno]+j] = nlist;
                 tmpoff += len;
              }
           }
 
           if (offsetmap && offsetmapz && mmadj->lzonelists && mmadj->zonelists &&
               mmadj->nneighbors && (DBGetDataReadMask2File(_dbfile) & DBMMADJZonelists))
           {
              tmpoff = offsetmapz[blockno];
              for (j = 0; j < mmadj->nneighbors[blockno]; j++)
              {
                 int stride = 1;
                 int len = mmadj->lzonelists[offsetmap[blockno]+j];
                 int *zlist;

                 if (len < 0) {
                     FREE(offsetmap);
                     FREE(offsetmapn);
                     FREE(offsetmapz);
                     DBFreeMultimeshadj(mmadj);
                     db_perror((char*)name, E_CALLFAIL, me);
                     UNWIND();
                 }

                 if (len == 0) {
                     mmadj->zonelists[offsetmap[blockno]+j] = NULL;
                     continue;
                 }
                 zlist = ALLOC_N(int, len);

                 /* Build the file space selection */
                 if ((fspace=build_fspace(zldset, 1, &tmpoff, &len, &stride,
                                          ds_size/*out*/))<0) {
                     FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
                     DBFreeMultimeshadj(mmadj);
                     db_perror("file data space", E_CALLFAIL, me);
                     UNWIND();
                 }

                 /* Build the memory data space */
                 if ((mspace=H5Screate_simple(1, ds_size, NULL))<0) {
                     FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
                     DBFreeMultimeshadj(mmadj);
                     db_perror("memory data space", E_CALLFAIL, me);
                     UNWIND();
                 }

                 P_rdprops = H5P_DEFAULT;
                 if (!DBGetEnableChecksumsFile(_dbfile))
                     P_rdprops = P_ckrdprops;

                 /* Read data */
                 if (H5Dread(zldset, mtype, mspace, fspace, P_rdprops, zlist)<0) {
                     FREE(offsetmap); FREE(offsetmapn); FREE(offsetmapz);
                     DBFreeMultimeshadj(mmadj);
                     hdf5_to_silo_error(name, me);
                     UNWIND();
                 }

                 /* Close everything */
                 H5Sclose(fspace);
                 H5Sclose(mspace);
 
                 mmadj->zonelists[offsetmap[blockno]+j] = zlist;
                 tmpoff += len;
              }
           }
        }
 
        FREE(offsetmap);
        FREE(offsetmapn);
        FREE(offsetmapz);
        if (nldset != -1)
            H5Dclose(nldset);
        if (zldset != -1)
            H5Dclose(zldset);
        H5Tclose(o);

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeMultimeshadj(mmadj);
    } END_PROTECT;

    return mmadj;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutMultivar
 *
 * Purpose:     Writes a multivar object to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `ngroups', `blockorigin', and `grouporigin' to
 *              duplicate changes to the PDB driver.
 *
 *              Eric Brugger, 2004-03-12
 *              Split the declaration and initialization of sizes
 *              into multiple statements so that it compiles on old
 *              sgi compilers.
 *
 *              Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *              Added call to reset global data
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *
 *   Mark C. Miller, Wed Jul 14 20:48:15 PDT 2010
 *   Added support for namescheme/empty_list options for multi-block objs.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutMultivar(DBfile *_dbfile, char const *name, int nvars, char const * const *varnames,
                    int const *vartypes, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBmultivar_mt       m;
    int                 i, len;
    char                *s=NULL;

    memset(&m, 0, sizeof m);
    PROTECT {

        /* Set global options */
        db_ResetGlobalData_MultiMesh();
        db_ProcessOptlist(DB_MULTIMESH, optlist);

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _mm._time_set, _mm._time,
            _mm._dtime_set, _mm._dtime, _mm._cycle);

        /*
         * Create a character string which is a semi-colon separated list of
         * variable names.
         */
        if (varnames)
        {
            for (i=len=0; i<nvars; i++) len += strlen(varnames[i])+1;
            s = (char *)malloc(len+1);
            for (i=len=0; i<nvars; i++) {
                if (i) s[len++] = ';';
                strcpy(s+len, varnames[i]);
                len += strlen(varnames[i]);
            }
            len++; /*count null*/
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s,
                m.varnames/*out*/, friendly_name(_dbfile,name, "_varnames", 0));
            FREE(s);
            FREE(s);
        }
        
        /* Write raw data arrays */
        if (vartypes)
            db_hdf5_compwr(dbfile, DB_INT, 1, &nvars, vartypes,
                m.vartypes/*out*/, friendly_name(_dbfile,name, "_vartypes", 0));
        if (_mm._extents && _mm._extentssize) {
            int sizes[2];
            sizes[0] = nvars;
            sizes[1] = _mm._extentssize;
            db_hdf5_compwr(dbfile, DB_DOUBLE, 2, sizes, _mm._extents,
                m.extents/*out*/, friendly_name(_dbfile,name, "_extents", 0));
        }

        /* output mrgtree info if we have it */
        if (_mm._region_pnames != NULL) {
            int len;
            DBStringArrayToStringList((char const * const *)_mm._region_pnames, -1, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.region_pnames/*out*/,
                friendly_name(_dbfile,name, "_region_pnames", 0));
            FREE(s);
        }

        /* output nameschemes if we have 'em */
        if (_mm._file_ns)
        {
           len = strlen(_mm._file_ns)+1;
           db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, _mm._file_ns,
                m.file_ns_name/*out*/, friendly_name(_dbfile,name,"_file_ns",0));
        }
        if (_mm._block_ns)
        {
           len = strlen(_mm._block_ns)+1;
           db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, _mm._block_ns,
                m.block_ns_name/*out*/, friendly_name(_dbfile,name,"_block_ns",0));
        }
        if (_mm._empty_list && _mm._empty_cnt>0) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &_mm._empty_cnt, _mm._empty_list,
                m.empty_list/*out*/, friendly_name(_dbfile,name,"_empty_list",0));
        }

        /* Initialize meta data */
        m.nvars = nvars;
        m.cycle = _mm._cycle;
        m.time = _mm._time;
        m.dtime = _mm._dtime;
        m.ngroups = _mm._ngroups;
        m.blockorigin = _mm._blockorigin;
        m.grouporigin = _mm._grouporigin;
        m.extentssize = _mm._extentssize;
        m.guihide = _mm._guihide;
        strcpy(m.mmesh_name, OPT(_mm._mmesh_name));
        m.tensor_rank = _mm._tensor_rank;
        m.conserved = _mm._conserved;
        m.extensive = _mm._extensive;
        m.block_type = _mm._block_type;
        m.empty_cnt = _mm._empty_cnt;
        m.repr_block_idx = _mm._repr_block_idx;
        db_SetMissingValueForPut(m.missing_value, _mm._missing_value);

        /* Write meta data to file */
        STRUCT(DBmultivar) {
            if (m.nvars)        MEMBER_S(int, nvars);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (m.ngroups)      MEMBER_S(int, ngroups);
            if (m.blockorigin)  MEMBER_S(int, blockorigin);
            if (m.grouporigin)  MEMBER_S(int, grouporigin);
            if (_mm._time_set)  MEMBER_S(float, time);
            if (_mm._dtime_set) MEMBER_S(double, dtime);
            if (m.extentssize)  MEMBER_S(int, extentssize);
            if (m.guihide)      MEMBER_S(int, guihide);
            if (m.tensor_rank)  MEMBER_S(int, tensor_rank);
            if (m.conserved)    MEMBER_S(int, conserved);
            if (m.extensive)    MEMBER_S(int, extensive);
            if (m.missing_value)MEMBER_S(double, missing_value);
            MEMBER_S(str(m.vartypes), vartypes);
            MEMBER_S(str(m.varnames), varnames);
            MEMBER_S(str(m.extents), extents);
            MEMBER_S(str(m.region_pnames), region_pnames);
            MEMBER_S(str(m.mmesh_name), mmesh_name);
            MEMBER_S(str(m.file_ns_name), file_ns_name);
            MEMBER_S(str(m.block_ns_name), block_ns_name);
            if (m.block_type)   MEMBER_S(int, block_type);
            MEMBER_S(str(m.empty_list), empty_list);
            if (m.empty_cnt)   MEMBER_S(int, empty_cnt);
            if (m.repr_block_idx)   MEMBER_S(int, repr_block_idx);
        } OUTPUT(dbfile, DB_MULTIVAR, name, &m);

        /* Free resources */
        FREE(s);
        
    } CLEANUP {
        FREE(s);
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetMultivar
 *
 * Purpose:     Reads a multivar object from the file.
 *
 * Return:      Success:        Ptr to new multivar object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `ngroups', `blockorigin', and `grouporigin' to
 *              duplicate changes to the PDB driver.
 *
 *              Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *              Removed erroneous code setting vartypes
 *
 *              Mark C. Miller, Wed Feb  2 07:52:22 PST 2005
 *              Added code to temporarily disable force single when
 *              reading extents
 *
 *   Mark C. Miller, Tue Feb 15 14:53:29 PST 2005
 *   Changed how force_single was handled to deal with possible throw
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *
 *   Mark C. Miller, Wed Jul 14 20:48:15 PDT 2010
 *   Added support for namescheme/empty_list options for multi-block objs.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmultivar *
db_hdf5_GetMultivar(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetMultivar";
    hid_t               o=-1, attr=-1;
    int                 _objtype;
    DBmultivar_mt       m;
    DBmultivar          *mv=NULL;
    char                *s=NULL;
    char                *mvnames=NULL;

    PROTECT {
        /* Open object and make sure it's a multivar */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_MULTIVAR!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBmultivar_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (NULL==(mv=DBAllocMultivar(0))) return NULL;
        mv->nvars = m.nvars;
        mv->ngroups = m.ngroups;
        mv->blockorigin = m.blockorigin;
        mv->grouporigin = m.grouporigin;
        mv->extentssize = m.extentssize;
        mv->guihide = m.guihide;
        mv->tensor_rank = m.tensor_rank;
        mv->mmesh_name = OPTDUP(m.mmesh_name);
        mv->conserved = m.conserved;
        mv->extensive = m.extensive;
        db_SetMissingValueForGet(mv->missing_value, m.missing_value);

        /* Read the raw data variable names */
        if (mv->nvars>0 && (DBGetDataReadMask2File(_dbfile) & DBMBNamesAndTypes))
        {
            mv->vartypes = (int *)db_hdf5_comprd(dbfile, m.vartypes, 1);
            mvnames = (char *)db_hdf5_comprd(dbfile, m.varnames, 1);
            db_StringListToStringArrayMBOpt(mvnames, &(mv->varnames), &(mv->varnames_alloc), m.nvars);
        }

        if (mv->extentssize>0 && (DBGetDataReadMask2File(_dbfile) & DBMBOptions))
           mv->extents = (double *)db_hdf5_comprd(dbfile, m.extents, 1);

        s = (char *)db_hdf5_comprd(dbfile, m.region_pnames, 1);
        if (s) mv->region_pnames = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
        FREE(s);

        mv->file_ns =  (char *)db_hdf5_comprd(dbfile, m.file_ns_name, 1);
        mv->block_ns =  (char *)db_hdf5_comprd(dbfile, m.block_ns_name, 1);
        mv->block_type = m.block_type;
        mv->empty_list =  (int *)db_hdf5_comprd(dbfile, m.empty_list, 1);
        mv->empty_cnt = m.empty_cnt;
        mv->repr_block_idx = m.repr_block_idx - 1;
        
        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeMultivar(mv);
        FREE(s);
    } END_PROTECT;
    return mv;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutMultimat
 *
 * Purpose:     Write a multimat object into the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `ngroups', `blockorigin', and `grouporigin' to
 *              duplicate changes to the PDB driver.
 *
 *              Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *              Added call to reset global data
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Mon Aug  7 17:03:51 PDT 2006
 *   Added material names and matcolors options
 *
 *   Thoamas R. Treadway, Tue Aug 15 14:05:59 PDT 2006
 *   Added DBOPT_ALLOWMAT0
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *
 *   Mark C. Miller, Wed Jul 14 20:48:15 PDT 2010
 *   Added support for namescheme/empty_list options for multi-block objs.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutMultimat(DBfile *_dbfile, char const *name, int nmats, char const * const *matnames,
                    DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBmultimat_mt       m;
    int                 i, len;
    char                *s=NULL;

    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        db_ResetGlobalData_MultiMesh();
        db_ProcessOptlist(DB_MULTIMESH, optlist);

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _mm._time_set, _mm._time,
            _mm._dtime_set, _mm._dtime, _mm._cycle);

        /*
         * Create a character string which is a semi-colon separated list of
         * material names.
         */
        /* Write raw data arrays */
#ifndef _MSC_VER
#warning REPLACE WITH STRING UTILITIES
#endif
        if (nmats > 0 && matnames)
        {
            for (i=len=0; i<nmats; i++) len += strlen(matnames[i])+1;
            s = (char *)malloc(len+1);
            for (i=len=0; i<nmats; i++) {
                if (i) s[len++] = ';';
                strcpy(s+len, matnames[i]);
                len += strlen(matnames[i]);
            }
            len++; /*count null*/

            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.matnames/*out*/,
                friendly_name(_dbfile,name, "_matnames", 0));
        }
        if (_mm._matnos && _mm._nmatnos > 0) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &_mm._nmatnos, _mm._matnos,
                m.matnos/*out*/, friendly_name(_dbfile,name,"_matnos", 0));
        }
        if (_mm._mixlens) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &nmats, _mm._mixlens,
                m.mixlens/*out*/, friendly_name(_dbfile,name,"_mixlens", 0));
        }
        if (_mm._matcounts && _mm._matlists) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &nmats, _mm._matcounts,
                m.matcounts/*out*/, friendly_name(_dbfile,name,"_matcounts", 0));
            for (i=len=0; i<nmats; i++)
               len += _mm._matcounts[i];
            db_hdf5_compwr(dbfile, DB_INT, 1, &len, _mm._matlists,
                m.matlists/*out*/, friendly_name(_dbfile,name,"_matlists", 0));
        }
        if (_mm._matcolors && _mm._nmatnos > 0) {
            int len; char *tmp;
            DBStringArrayToStringList((char const * const *)_mm._matcolors,
                _mm._nmatnos, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.mat_colors/*out*/, friendly_name(_dbfile,name,"_matcolors", 0));
            FREE(tmp);
#ifndef _MSC_VER
#warning IS THIS TAKEN CARE OF ELSEWHERE
#endif
            _mm._matcolors = 0;
        }
        if (_mm._matnames && _mm._nmatnos > 0) {
            int len; char *tmp;
            DBStringArrayToStringList((char const * const *)_mm._matnames,
                _mm._nmatnos, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.material_names/*out*/, friendly_name(_dbfile,name,"_material_names", 0));
            FREE(tmp);
#ifndef _MSC_VER
#warning IS THIS TAKEN CARE OF ELSEWHERE
#endif
            _mm._matnames = 0;
        }
        /* output nameschemes if we have 'em */
        if (_mm._file_ns)
        {
           len = strlen(_mm._file_ns)+1;
           db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, _mm._file_ns,
                m.file_ns_name/*out*/, friendly_name(_dbfile,name,"_file_ns",0));
        }
        if (_mm._block_ns)
        {
           len = strlen(_mm._block_ns)+1;
           db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, _mm._block_ns,
                m.block_ns_name/*out*/, friendly_name(_dbfile,name,"_block_ns",0));
        }
        if (_mm._empty_list && _mm._empty_cnt>0) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &_mm._empty_cnt, _mm._empty_list,
                m.empty_list/*out*/, friendly_name(_dbfile,name,"_empty_list",0));
        }

        /* Initialize meta data */
        m.nmats = nmats;
        m.cycle = _mm._cycle;
        m.time = _mm._time;
        m.dtime = _mm._dtime;
        m.ngroups = _mm._ngroups;
        m.blockorigin = _mm._blockorigin;
        m.grouporigin = _mm._grouporigin;
        m.nmatnos = _mm._nmatnos;
        m.allowmat0 = _mm._allowmat0;
        m.guihide = _mm._guihide;
        strcpy(m.mmesh_name, OPT(_mm._mmesh_name));
        m.empty_cnt = _mm._empty_cnt;
        m.repr_block_idx = _mm._repr_block_idx;

        /* Write meta data to file */
        STRUCT(DBmultimat) {
            if (m.nmats)        MEMBER_S(int, nmats);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (m.ngroups)      MEMBER_S(int, ngroups);
            if (m.blockorigin)  MEMBER_S(int, blockorigin);
            if (m.grouporigin)  MEMBER_S(int, grouporigin);
            if (_mm._time_set)  MEMBER_S(float, time);
            if (_mm._dtime_set) MEMBER_S(double, dtime);
            MEMBER_S(str(m.matnames), matnames);
            MEMBER_S(str(m.matnos), matnos);
            MEMBER_S(str(m.mixlens), mixlens);
            MEMBER_S(str(m.matcounts), matcounts);
            MEMBER_S(str(m.matlists), matlists);
            if (m.nmatnos)      MEMBER_S(int, nmatnos);
            if (m.allowmat0)    MEMBER_S(int, allowmat0);
            if (m.guihide)      MEMBER_S(int, guihide);
            MEMBER_S(str(m.material_names), material_names);
            MEMBER_S(str(m.mat_colors), mat_colors);
            MEMBER_S(str(m.mmesh_name), mmesh_name);
            MEMBER_S(str(m.file_ns_name), file_ns_name);
            MEMBER_S(str(m.block_ns_name), block_ns_name);
            MEMBER_S(str(m.empty_list), empty_list);
            if (m.empty_cnt)   MEMBER_S(int, empty_cnt);
            if (m.repr_block_idx)   MEMBER_S(int, repr_block_idx);
        } OUTPUT(dbfile, DB_MULTIMAT, name, &m);

        /* Free resources */
        FREE(s);
        
    } CLEANUP {
        FREE(s);
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetMultimat
 *
 * Purpose:     Reads a multimat object from the file.
 *
 * Return:      Success:        Ptr to new multimat object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `ngroups', `blockorigin', and `grouporigin' to
 *              duplicate changes to the PDB driver.
 *
 *    Mark C. Miller, Mon Aug  7 17:03:51 PDT 2006
 *    Added material names and material colors options as well as nmatnos
 *    and matnos
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *
 *   Mark C. Miller, Wed Jul 14 20:48:15 PDT 2010
 *   Added support for namescheme/empty_list options for multi-block objs.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmultimat *
db_hdf5_GetMultimat(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetMultimat";
    hid_t               o=-1, attr=-1;
    int                 _objtype;
    DBmultimat_mt       m;
    DBmultimat          *mm=NULL;
    char                *matnames=NULL;

    PROTECT {
        /* Open object and make sure it's a multimat */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_MULTIMAT!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBmultimat_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (NULL==(mm=DBAllocMultimat(0))) return NULL;
        mm->nmats = m.nmats;
        mm->ngroups = m.ngroups;
        mm->blockorigin = m.blockorigin;
        mm->grouporigin = m.grouporigin;
        mm->allowmat0 = m.allowmat0;
        mm->guihide = m.guihide;
        mm->nmatnos = m.nmatnos;
        mm->mmesh_name = OPTDUP(m.mmesh_name);
        mm->matnos = (int *)db_hdf5_comprd(dbfile, m.matnos, 1);

        /* Read the raw data */
        if (mm->nmats>0 && (DBGetDataReadMask2File(_dbfile) & DBMBNamesAndTypes))
        {
            matnames = (char *)db_hdf5_comprd(dbfile, m.matnames, 1);
            db_StringListToStringArrayMBOpt(matnames, &(mm->matnames), &(mm->matnames_alloc), m.nmats);
        }

        if (mm->nmats>0 && (DBGetDataReadMask2File(_dbfile) & DBMBOptions))
        {
            mm->mixlens = (int *)db_hdf5_comprd(dbfile, m.mixlens, 1);
            mm->matcounts = (int *)db_hdf5_comprd(dbfile, m.matcounts, 1);
            mm->matlists = (int *)db_hdf5_comprd(dbfile, m.matlists, 1);
        }

        if (m.nmatnos > 0) {
            char *tmpmaterial_names = (char *)db_hdf5_comprd(dbfile, m.material_names, 1);
            char *tmpmat_colors = (char *)db_hdf5_comprd(dbfile, m.mat_colors, 1);
            if (tmpmaterial_names)
                mm->material_names = DBStringListToStringArray(tmpmaterial_names,
                    &m.nmatnos, !skipFirstSemicolon);
            if (tmpmat_colors)
                mm->matcolors = DBStringListToStringArray(tmpmat_colors,
                    &m.nmatnos, !skipFirstSemicolon);
            FREE(tmpmaterial_names);
            FREE(tmpmat_colors);
        }

        mm->file_ns =  (char *)db_hdf5_comprd(dbfile, m.file_ns_name, 1);
        mm->block_ns =  (char *)db_hdf5_comprd(dbfile, m.block_ns_name, 1);
        mm->empty_list =  (int *)db_hdf5_comprd(dbfile, m.empty_list, 1);
        mm->empty_cnt = m.empty_cnt;
        mm->repr_block_idx = m.repr_block_idx - 1;
        
        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeMultimat(mm);
    } END_PROTECT;
    return mm;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutMultimatspecies
 *
 * Purpose:     Write a multi-mat species object to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `ngroups', `blockorigin', and `grouporigin' to
 *              duplicate changes to the PDB driver.
 *
 *              Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *              Added call to reset global data
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *   Added names and colors for species.
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *
 *   Mark C. Miller, Wed Jul 14 20:48:15 PDT 2010
 *   Added support for namescheme/empty_list options for multi-block objs.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutMultimatspecies(DBfile *_dbfile, char const *name, int nspec,
                           char const * const *specnames, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBmultimatspecies_mt m;
    int                 i, len, nstrs = 0;
    char                *s=NULL;

    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        db_ResetGlobalData_MultiMesh();
        db_ProcessOptlist(DB_MULTIMESH, optlist);

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _mm._time_set, _mm._time,
            _mm._dtime_set, _mm._dtime, _mm._cycle);

        /*
         * Create a character string which is a semi-colon separated list of
         * material names.
         */
        /* Write raw data arrays */
        if (nspec > 0 && specnames)
        {
            for (i=len=0; i<nspec; i++) len += strlen(specnames[i])+1;
            s = (char *)malloc(len+1);
            for (i=len=0; i<nspec; i++) {
                if (i) s[len++] = ';';
                strcpy(s+len, specnames[i]);
                len += strlen(specnames[i]);
            }
            len++; /*count null*/

            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.specnames/*out*/,
                friendly_name(_dbfile,name, "_specnames", 0));
        }

        if (_mm._nmat>0 && _mm._nmatspec) {

            db_hdf5_compwr(dbfile, DB_INT, 1, &_mm._nmat, _mm._nmatspec,
                m.nmatspec/*out*/, friendly_name(_dbfile,name, "_nmatspec", 0));

            if (_mm._specnames) {
                int len; char *tmp;
                for (i=0; i < _mm._nmat; i++)
                    nstrs += _mm._nmatspec[i];
                DBStringArrayToStringList((char const * const *)_mm._specnames, nstrs, &tmp, &len);
                db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                    m.species_names/*out*/, friendly_name(_dbfile,name,"_species_names", 0));
                FREE(tmp);
            }

            if (_mm._speccolors) {
                int len; char *tmp;
                if (nstrs == 0)
                {
                    for (i=0; i < _mm._nmat; i++)
                        nstrs += _mm._nmatspec[i];
                }
                DBStringArrayToStringList((char const * const *)_mm._speccolors, nstrs, &tmp, &len);
                db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                    m.speccolors/*out*/, friendly_name(_dbfile,name,"_speccolors", 0));
                FREE(tmp);
            }
        }
        /* output nameschemes if we have 'em */
        if (_mm._file_ns)
        {
           len = strlen(_mm._file_ns)+1;
           db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, _mm._file_ns,
                m.file_ns_name/*out*/, friendly_name(_dbfile,name,"_file_ns",0));
        }
        if (_mm._block_ns)
        {
           len = strlen(_mm._block_ns)+1;
           db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, _mm._block_ns,
                m.block_ns_name/*out*/, friendly_name(_dbfile,name,"_block_ns",0));
        }
        if (_mm._empty_list && _mm._empty_cnt>0) {
            db_hdf5_compwr(dbfile, DB_INT, 1, &_mm._empty_cnt, _mm._empty_list,
                m.empty_list/*out*/, friendly_name(_dbfile,name,"_empty_list",0));
        }

        /* Initialize meta data */
        m.nspec = nspec;
        m.nmat = _mm._nmat;
        m.cycle = _mm._cycle;
        m.time = _mm._time;
        m.dtime = _mm._dtime;
        m.ngroups = _mm._ngroups;
        m.blockorigin = _mm._blockorigin;
        m.grouporigin = _mm._grouporigin;
        m.guihide = _mm._guihide;
        strcpy(m.matname, OPT(_mm._matname));
        m.empty_cnt = _mm._empty_cnt;
        m.repr_block_idx = _mm._repr_block_idx;

        /* Write meta data to file */
        STRUCT(DBmultimatspecies) {
            if (m.nspec)        MEMBER_S(int, nspec);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (m.ngroups)      MEMBER_S(int, ngroups);
            if (m.blockorigin)  MEMBER_S(int, blockorigin);
            if (m.grouporigin)  MEMBER_S(int, grouporigin);
            if (m.guihide)      MEMBER_S(int, guihide);
            if (_mm._time_set)  MEMBER_S(float, time);
            if (_mm._dtime_set) MEMBER_S(double, dtime);
            if (_mm._nmat>0 && _mm._nmatspec) MEMBER_S(int, nmat);
            MEMBER_S(str(m.specnames), specnames);
            MEMBER_S(str(m.nmatspec), nmatspec);
            MEMBER_S(str(m.matname), matname);
            MEMBER_S(str(m.species_names), species_names);
            MEMBER_S(str(m.speccolors), speccolors);
            MEMBER_S(str(m.file_ns_name), file_ns_name);
            MEMBER_S(str(m.block_ns_name), block_ns_name);
            MEMBER_S(str(m.empty_list), empty_list);
            if (m.empty_cnt)   MEMBER_S(int, empty_cnt);
            if (m.repr_block_idx)   MEMBER_S(int, repr_block_idx);
        } OUTPUT(dbfile, DB_MULTIMATSPECIES, name, &m);

        /* Free resources */
        FREE(s);
        
    } CLEANUP {
        FREE(s);
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetMultimatspecies
 *
 * Purpose:     Reads a multimat species object from the file.
 *
 * Return:      Success:        Ptr to new multimatspecies.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `ngroups', `blockorigin', and `grouporigin' to
 *              duplicate changes to the PDB driver.
 *
 *              Mark C. Miller, Tue Sep  8 15:40:51 PDT 2009
 *              Added names and colors for species.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *
 *   Mark C. Miller, Wed Jul 14 20:48:15 PDT 2010
 *   Added support for namescheme/empty_list options for multi-block objs.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmultimatspecies *
db_hdf5_GetMultimatspecies(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetMultimatspecies";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i, nstrs=0;
    DBmultimatspecies_mt m;
    DBmultimatspecies   *mm=NULL;
    char                *specnames=NULL;

    PROTECT {
        /* Open object and make sure it's a multimatspecies */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_MULTIMATSPECIES!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBmultimatspecies_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (NULL==(mm=DBAllocMultimatspecies(0))) return NULL;
        mm->matname = OPTDUP(m.matname);
        mm->nspec = m.nspec;
        mm->ngroups = m.ngroups;
        mm->blockorigin = m.blockorigin;
        mm->grouporigin = m.grouporigin;
        mm->guihide = m.guihide;
        mm->nmat = m.nmat;
        mm->nmatspec = (int *)db_hdf5_comprd(dbfile, m.nmatspec, 1);

        /* Read the raw data */
        if (mm->nspec>0 && (DBGetDataReadMask2File(_dbfile) & DBMBNamesAndTypes))
        {
            specnames = (char *)db_hdf5_comprd(dbfile, m.specnames, 1);
            db_StringListToStringArrayMBOpt(specnames, &(mm->specnames), &(mm->specnames_alloc), m.nspec);
        }
        
        if (mm->nmat > 0 && mm->nmatspec) {
            char *tmpspecies_names = (char *)db_hdf5_comprd(dbfile, m.species_names, 1);
            char *tmpspeccolors = (char *)db_hdf5_comprd(dbfile, m.speccolors, 1);
  
            if (tmpspecies_names)
            {
                for (i = 0; i < mm->nmat; i++)
                    nstrs += mm->nmatspec[i];
                mm->species_names = DBStringListToStringArray(tmpspecies_names, &nstrs, !skipFirstSemicolon);
            }
            if (tmpspeccolors)
            {
                if (nstrs == 0)
                {
                    for (i = 0; i < mm->nmat; i++)
                        nstrs += mm->nmatspec[i];
                }
                mm->speccolors = DBStringListToStringArray(tmpspeccolors, &nstrs, !skipFirstSemicolon);
            }
            FREE(tmpspecies_names);
            FREE(tmpspeccolors);
        }

        mm->file_ns =  (char *)db_hdf5_comprd(dbfile, m.file_ns_name, 1);
        mm->block_ns =  (char *)db_hdf5_comprd(dbfile, m.block_ns_name, 1);
        mm->empty_list =  (int *)db_hdf5_comprd(dbfile, m.empty_list, 1);
        mm->empty_cnt = m.empty_cnt;
        mm->repr_block_idx = m.repr_block_idx - 1;

        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeMultimatspecies(mm);
    } END_PROTECT;
    return mm;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutPointmesh
 *
 * Purpose:     Writes a pointmesh object to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Tuesday, April  6, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `group_no' property to duplicate changes made to the
 *              PDB driver.
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Jan 17 21:41:52 PST 2008
 *   Fixed missing origin member
 *
 *   Mark C. Miller, Sat Nov 14 20:28:34 PST 2009
 *   Changed how long long global node/zone numbers are supported
 *   from a int (bool), "llong_gnode|zoneno" to an int holding
 *   the actual datatype. The type is assumed int if it its
 *   value is zero or it does not exist. Otherwise, the type is
 *   is whatever is stored in gnznodtype member. 
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutPointmesh(DBfile *_dbfile, char const *name, int ndims, DBVCP2_t _coords,
                     int nels, int datatype, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_PutPointmesh";
    DBpointmesh_mt      m;
    int                 i;
    void const * const *coords = (void const * const *) _coords;
    
    memset(&m, 0, sizeof m);
    PROTECT {
        /* Check datatype */
        if (DB_FLOAT!=datatype && DB_DOUBLE!=datatype) {
            db_perror("invalid floating-point datatype", E_BADARGS, me);
            UNWIND();
        }

        /* Set global options */
        memset(&_pm, 0, sizeof _pm);
        _pm._ndims = _pm._nspace = ndims;
        _pm._group_no = -1;
        db_ProcessOptlist(DB_POINTMESH, optlist);
        _pm._nels = nels;
        _pm._minindex = _pm._lo_offset;
        _pm._maxindex = nels - _pm._hi_offset - 1;

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _pm._time_set, _pm._time,
            _pm._dtime_set, _pm._dtime, _pm._cycle);

        /* Write raw data arrays */
        for (i=0; i<ndims && nels>0; i++) {
            db_hdf5_compwr(dbfile, datatype, 1, &nels, coords[i],
                m.coord[i]/*out*/, friendly_name(_dbfile,name, "_coord%d", &i));
        }

        /* Find the mesh extents from the coordinate arrays */
        if (nels>0)
        {
            if (DB_DOUBLE==datatype) {
                for (i=0; i<ndims; i++) {
                    _DBdarrminmax(((double**)coords)[i], nels,
                                  m.min_extents+i, m.max_extents+i);
                }
            } else {
                for (i=0; i<ndims; i++) {
                    float min_extents, max_extents;
                    _DBarrminmax((float*)coords[i], nels, &min_extents, &max_extents);
                    m.min_extents[i] = min_extents;
                    m.max_extents[i] = max_extents;
                }
            }
        }

        /* Global node numbers */
        if (nels>0 && _pm._gnodeno)
        {
            if (_pm._llong_gnodeno)
                db_hdf5_compwr(dbfile, DB_LONG_LONG, 1, &nels, _pm._gnodeno,
                    m.gnodeno/*out*/, friendly_name(_dbfile,name, "_gnodeno", 0));
            else
                db_hdf5_compwr(dbfile, DB_INT, 1, &nels, _pm._gnodeno,
                    m.gnodeno/*out*/, friendly_name(_dbfile,name, "_gnodeno", 0));
        }

        if (nels>0 && _pm._ghost_node_labels)
        {
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &nels, _pm._ghost_node_labels,
                    m.ghost_node_labels/*out*/, friendly_name(_dbfile,name, "_ghost_node_labels", 0));
        }

        if (_pm._alt_nodenum_vars) {
            int len, nvars=-1; char *tmp;
            DBStringArrayToStringList((char const * const *)_pm._alt_nodenum_vars, nvars, &tmp, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, tmp,
                m.alt_nodenum_vars/*out*/, friendly_name(_dbfile,name, "_alt_nodenum_vars",0));
            FREE(tmp);
        }

        /* Build header in memory */
        m.ndims = ndims;
        m.nspace = _pm._nspace;
        m.nels = _pm._nels;
        m.cycle = _pm._cycle;
        m.origin = _pm._origin;
        m.min_index = _pm._minindex;
        m.max_index = _pm._maxindex;
        m.time = _pm._time;
        m.dtime = _pm._dtime;
        m.group_no = _pm._group_no;
        m.guihide = _pm._guihide;
        for (i=0; i<ndims; i++) {
            strcpy(m.label[i], OPT(_pm._labels[i]));
            strcpy(m.units[i], OPT(_pm._units[i]));
        }
        strcpy(m.mrgtree_name, OPT(_pm._mrgtree_name));
        m.gnznodtype = _pm._llong_gnodeno?DB_LONG_LONG:0;
        
        /* Write header to file */
        STRUCT(DBpointmesh) {
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nspace)       MEMBER_S(int, nspace);
            if (m.nels)         MEMBER_S(int, nels);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (_pm._time_set)  MEMBER_S(float, time);
            if (_pm._dtime_set) MEMBER_S(double, dtime);
            if (m.min_index)    MEMBER_S(int, min_index);
            if (m.max_index)    MEMBER_S(int, max_index);
            if (m.group_no)     MEMBER_S(int, group_no);
            if (m.guihide)      MEMBER_S(int, guihide);
            if (m.origin)       MEMBER_S(int, origin);
            MEMBER_3(double, min_extents);
            MEMBER_3(double, max_extents);
            MEMBER_R(str(m.coord[_j]), coord, ndims);
            MEMBER_R(str(m.label[_j]), label, ndims);
            MEMBER_R(str(m.units[_j]), units, ndims);
            MEMBER_S(str(m.gnodeno), gnodeno);
            MEMBER_S(str(m.mrgtree_name), mrgtree_name);
            if (m.gnznodtype    )MEMBER_S(int, gnznodtype);
            MEMBER_S(str(m.ghost_node_labels), ghost_node_labels);
            MEMBER_S(str(m.alt_nodenum_vars), alt_nodenum_vars);
        } OUTPUT(dbfile, DB_POINTMESH, name, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetPointmesh
 *
 * Purpose:     Reads a pointmesh object from the file.
 *
 * Return:      Success:        Ptr to new pointmesh.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April  7, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-07-13
 *              Added `group_no' to duplicate changes to the PDB driver.
 *
 *              Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *              Made it set datatype correctly. Added support for dataReadMask
 *
 *              Mark C. Miller, Thu Jan 17 21:41:52 PST 2008
 *              Fixed missing origin member
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBpointmesh *
db_hdf5_GetPointmesh(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetPointmesh";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i;
    DBpointmesh_mt      m;
    DBpointmesh         *pm=NULL;

    PROTECT {
        /* Open object and make sure it's a pointmesh */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_POINTMESH!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBpointmesh_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (NULL==(pm=DBAllocPointmesh())) return NULL;
        pm->name = BASEDUP(name);
        pm->cycle = m.cycle;
        pm->time = m.time;
        pm->dtime = m.dtime;
        if ((pm->datatype = db_hdf5_GetVarType(_dbfile, m.coord[0])) < 0)
            pm->datatype = DB_FLOAT;
        if (force_single_g) pm->datatype = DB_FLOAT;
        pm->ndims = m.ndims;
        pm->nels = m.nels;
        pm->group_no = m.group_no;
        pm->guihide = m.guihide;
        pm->origin = m.origin;
        pm->mrgtree_name = OPTDUP(m.mrgtree_name);
        for (i=0; i<m.ndims; i++) {
            pm->units[i] = OPTDUP(m.units[i]);
            pm->labels[i] = OPTDUP(m.label[i]);
            if (pm->datatype == DB_DOUBLE)
            {
                ((double*)pm->min_extents)[i] = m.min_extents[i];
                ((double*)pm->max_extents)[i] = m.max_extents[i];
            }
            else
            {
                pm->min_extents[i] = m.min_extents[i];
                pm->max_extents[i] = m.max_extents[i];
            }
        }

        /* Read raw data */
        if (DBGetDataReadMask2File(_dbfile) & DBPMCoords)
        {
            for (i=0; i<m.ndims; i++) {
                pm->coords[i] = db_hdf5_comprd(dbfile, m.coord[i], 0);
            }
        }
        if (DBGetDataReadMask2File(_dbfile) & DBPMGlobNodeNo)
            pm->gnodeno = db_hdf5_comprd(dbfile, m.gnodeno, 1);
        pm->gnznodtype = m.gnznodtype?m.gnznodtype:DB_INT;

        if (DBGetDataReadMask2File(_dbfile) & DBPMGhostNodeLabels)
        {
            pm->ghost_node_labels = (char *)db_hdf5_comprd(dbfile, m.ghost_node_labels, 0);
        }

        /* alternate node number variables */
        {
            int nvars = -1;
            char *tmpnames = (char *)db_hdf5_comprd(dbfile, m.alt_nodenum_vars, 1);
            if (tmpnames)
                pm->alt_nodenum_vars = DBStringListToStringArray(tmpnames, &nvars, !skipFirstSemicolon);
            FREE(tmpnames);
        }

        H5Tclose(o);
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreePointmesh(pm);
    } END_PROTECT;
    return pm;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutPointvar
 *
 * Purpose:     Write a pointvar object to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April  7, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Added Hack to make HDF5 driver deal with cycle/time same as PDB driver
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Thu Feb  4 11:25:00 PST 2010
 *   Refactored logic to handle time, dtime and cycle to a new method,
 *   db_hdf5_handle_ctdt().
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutPointvar(DBfile *_dbfile, char const *name, char const *meshname, int nvars,
                    DBVCP2_t _vars, int nels, int datatype, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBpointvar_mt       m;
    int                 i, saved_ndims;
    char               *s = 0;
    void const * const *vars = (void const * const *) _vars;

    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        saved_ndims = _pm._ndims;
        memset(&_pm, 0, sizeof _pm);
        _pm._ndims = saved_ndims;
        _pm._group_no = -1;
        _pm._missing_value = DB_MISSING_VALUE_NOT_SET;
        db_ProcessOptlist(DB_POINTMESH, optlist);
        _pm._nels = nels;
        _pm._minindex = _pm._lo_offset;
        _pm._maxindex = nels - _pm._hi_offset - 1;

        if (nvars>MAX_VARS) {
            db_perror("too many variables", E_BADARGS, "db_hdf5_PutPointvar");
            UNWIND();
        }

        /* hack to maintain backward compatibility with pdb driver */
        db_hdf5_handle_ctdt(dbfile, _pm._time_set, _pm._time,
            _pm._dtime_set, _pm._dtime, _pm._cycle);

        /* Write raw data arrays */
        for (i=0; i<nvars && nels; i++) {
            db_hdf5_compwr(dbfile, datatype, 1, &nels, vars[i],
                m.data[i]/*out*/, friendly_name(_dbfile,name, nvars==1?"_data":"%d_data", &i));
        }

        /* output mrgtree info if we have it */
        if (_pm._region_pnames != NULL) {
            int len;
            DBStringArrayToStringList((char const * const *)_pm._region_pnames, -1, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.region_pnames/*out*/,
                friendly_name(_dbfile,name, "_region_pnames", 0));
            FREE(s);
        }

        /* Build header in memory */
        m.nvals = nvars;
        m.nels = nels;
        m.ndims = _pm._ndims;
        m.nspace = _pm._nspace;
        m.origin = _pm._origin;
        m.min_index = _pm._minindex;
        m.max_index = _pm._maxindex;
        m.cycle = _pm._cycle;
        m.guihide = _pm._guihide;
        m.time = _pm._time;
        m.dtime = _pm._dtime;
        m.ascii_labels = _pm._ascii_labels;
        m.datatype = (DB_FLOAT==datatype || DB_DOUBLE==datatype)?0:datatype;
        m.conserved = _pm._conserved;
        m.extensive = _pm._extensive;
        db_SetMissingValueForPut(m.missing_value, _pm._missing_value);
        strcpy(m.meshid, OPT(meshname));
        strcpy(m.label, OPT(_pm._label));
        strcpy(m.units, OPT(_pm._unit));

        /* Write header to file */
        STRUCT(DBpointvar) {
            if (m.nvals)        MEMBER_S(int, nvals);
            if (m.nels)         MEMBER_S(int, nels);
            if (m.ndims)        MEMBER_S(int, ndims);
            if (m.nspace)       MEMBER_S(int, nspace);
            if (m.origin)       MEMBER_S(int, origin);
            if (m.datatype)     MEMBER_S(int, datatype);
            if (m.min_index)    MEMBER_S(int, min_index);
            if (m.max_index)    MEMBER_S(int, max_index);
            if (m.cycle)        MEMBER_S(int, cycle);
            if (m.guihide)      MEMBER_S(int, guihide);
            if (m.ascii_labels) MEMBER_S(int, ascii_labels);
            if (_pm._time_set)  MEMBER_S(float, time);
            if (_pm._dtime_set) MEMBER_S(double, dtime);
            if (m.conserved)    MEMBER_S(int, conserved);
            if (m.extensive)    MEMBER_S(int, extensive);
            if (m.missing_value) MEMBER_S(double, missing_value);
            MEMBER_S(str(m.meshid), meshid);
            MEMBER_S(str(m.label), label);
            MEMBER_S(str(m.units), units);
            MEMBER_R(str(m.data[_j]), data, m.nvals);
            MEMBER_S(str(m.region_pnames), region_pnames);
        } OUTPUT(dbfile, DB_POINTVAR, name, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetPointvar
 *
 * Purpose:     Reads a pointvar object from the file.
 *
 * Return:      Success:        Ptr to new pointvar object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  8, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Jul 29 11:26:24 PDT 2004
 *   Made it set datatype correctly. Added support for dataReadMask
 *
 *   Mark C. Miller, Thu Nov  5 16:15:49 PST 2009
 *   Added support for conserved/extensive options.
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmeshvar *
db_hdf5_GetPointvar(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetPointvar";
    char                *s = 0;
    hid_t               o=-1, attr=-1;
    int                 _objtype, i;
    DBpointvar_mt       m;
    DBmeshvar           *pv=NULL;

    PROTECT {
        /* Open object and make sure it's a pointvar */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_POINTVAR!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBpointvar_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (NULL==(pv=DBAllocMeshvar())) return NULL;
        pv->name = BASEDUP(name);
        pv->units = OPTDUP(m.units);
        pv->meshname = OPTDUP(m.meshid);
        pv->label = OPTDUP(m.label);
        pv->cycle = m.cycle;
        if ((pv->datatype = db_hdf5_GetVarType(_dbfile, m.data[0])) < 0)
            pv->datatype = silo2silo_type(m.datatype);
        if (force_single_g) pv->datatype = DB_FLOAT;
        pv->nels = m.nels;
        pv->nvals = m.nvals;
        pv->nspace = m.nspace;
        pv->ndims = m.ndims;
        pv->origin = m.origin;
        pv->time = m.time;
        pv->dtime = m.dtime;
        pv->min_index[0] = m.min_index;
        pv->max_index[0] = m.max_index;
        pv->guihide = m.guihide;
        pv->ascii_labels = m.ascii_labels;
        pv->conserved = m.conserved;
        pv->extensive = m.extensive;
        db_SetMissingValueForGet(pv->missing_value, m.missing_value);

        if (pv->nvals>MAX_VARS) {
            db_perror("too many variables", E_BADARGS, me);
            UNWIND();
        }

        /* Read raw data */
        if ((DBGetDataReadMask2File(_dbfile) & DBPVData) && m.nvals && m.nels)
        {
            pv->vals = (void **)calloc(m.nvals, sizeof(void*));
            for (i=0; i<m.nvals; i++) {
                pv->vals[i] = db_hdf5_comprd(dbfile, m.data[i], 0);
            }
        }

        s = (char *)db_hdf5_comprd(dbfile, m.region_pnames, 1);
        if (s) pv->region_pnames = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
        FREE(s);

        H5Tclose(o);
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeMeshvar(pv);
    } END_PROTECT;
    return pv;
}
        
/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutCompoundarray
 *
 * Purpose:     Writes a compound array object to the file.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  8, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Thu Apr 19 19:16:11 PDT 2007
 *   Modifed db_hdf5_compwr interface for friendly hdf5 dataset names
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
SILO_CALLBACK int
db_hdf5_PutCompoundarray(
    DBfile *_dbfile,
    char const *name,
    char const * const *elmtnames,
    int const *elmtlen,
    int nelmts,
    void const *values,
    int nvalues,
    int datatype,
    DBoptlist const *optlist
)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBcompoundarray_mt  m;
    int                 i, len;
    char                *s=NULL;

    memset(&m, 0, sizeof m);
    PROTECT {
        /*
         * Create a character string which is a semi-colon separated list of
         * component names.
         */
        for (i=len=0; i<nelmts; i++) len += strlen(elmtnames[i])+1;
        s = (char *)malloc(len+1);
        for (i=len=0; i<nelmts; i++) {
            if (i) s[len++] = ';';
            strcpy(s+len, elmtnames[i]);
            len += strlen(elmtnames[i]);
        }
        len++; /*count null*/

        /* Write raw data arrays */
        db_hdf5_compwr(dbfile, datatype, 1, &nvalues, values, m.values,
            friendly_name(_dbfile,name, "_values", 0));
        db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.elemnames,
            friendly_name(_dbfile,name, "_elemnames", 0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &nelmts, elmtlen, m.elemlengths,
            friendly_name(_dbfile,name, "_elemlengths", 0));

        /* Initialize meta data */
        m.nelems = nelmts;
        m.nvalues = nvalues;
        m.datatype = (DB_FLOAT==datatype || DB_DOUBLE==datatype)?0:datatype;

        /* Write meta data to file */
        STRUCT(DBcompoundarray) {
            if (m.nelems)       MEMBER_S(int, nelems);
            if (m.nvalues)      MEMBER_S(int, nvalues);
            if (m.datatype)     MEMBER_S(int, datatype);
            MEMBER_S(str(m.values), values);
            MEMBER_S(str(m.elemnames), elemnames);
            MEMBER_S(str(m.elemlengths), elemlengths);
        } OUTPUT(dbfile, DB_ARRAY, name, &m);

        /* Free resources */
        FREE(s);
        
    } CLEANUP {
        FREE(s);
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetCompoundarray
 *
 * Purpose:     Reads a compound array object from the file.
 *
 * Return:      Success:        Ptr to new compound array.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  8, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Mon Aug  2 15:06:57 PDT 2004
 *   Made it set datatype correctly
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBcompoundarray *
db_hdf5_GetCompoundarray(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetCompoundarray";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i;
    DBcompoundarray_mt m;
    DBcompoundarray     *ca=NULL;
    char                *s=NULL;

    PROTECT {
        /* Open object and make sure it's a compund array */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_ARRAY!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBcompoundarray_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        if (NULL==(ca=DBAllocCompoundarray())) return NULL;
        ca->name = BASEDUP(name);
        ca->nelems = m.nelems;
        ca->nvalues = m.nvalues;
        if ((ca->datatype = db_hdf5_GetVarType(_dbfile, m.values)) < 0)
            ca->datatype = silo2silo_type(m.datatype);
        if (force_single_g) ca->datatype = DB_FLOAT;
        
        /* Read the raw data */
        ca->elemlengths = (int *)db_hdf5_comprd(dbfile, m.elemlengths, 1);
        ca->values = db_hdf5_comprd(dbfile, m.values, 1);
        ca->elemnames = (char **)calloc(m.nelems, sizeof(char*));
        s = (char *)db_hdf5_comprd(dbfile, m.elemnames, 1);
        for (i=0; i<m.nelems; i++) {
            char *tok = strtok(i?NULL:s, ";");
            ca->elemnames[i] = STRDUP(tok);
        }
        
        H5Tclose(o);
        FREE(s);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeCompoundarray(ca);
        FREE(s);
    } END_PROTECT;
    return ca;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_InqVarType
 *
 * Purpose:     Returns the object type for an object.
 *
 * Return:      Success:        Object type
 *
 *              Failure:        DB_INVALID_OBJECT
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  8, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Feb  1 13:48:33 PST 2005
 *   Fixed return of variable type for case of QUAD_RECT | QUAD_CURV
 *
 *   Mark C. Miller, Mon Feb 14 20:16:50 PST 2005
 *   Undid fix above
 *
 *   Mark C. Miller, Wed Apr 20 16:05:23 PDT 2005
 *   Made it return DB_VARIABLE for any inquiry where it couldn't
 *   get actual variable type from the object
 *
 *   Mark C. Miller, Mon Jul 17 18:07:57 PDT 2006
 *   Improved fix, above, for inquries on "/.silo/#000XXXX" datasets
 *   for silex
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBObjectType
db_hdf5_InqVarType(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_InqVarType";
    hid_t               o=-1, attr=-1;
    int                 _objtype = DB_INVALID_OBJECT;

    PROTECT {

        /* Open object */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            if ((o=H5Gopen(dbfile->cwg, name, H5P_DEFAULT))<0) {
                if ((o=H5Dopen(dbfile->cwg, name, H5P_DEFAULT))<0) {
                    _objtype = DB_INVALID_OBJECT;
                }
                else
                {
                    _objtype = DB_VARIABLE;
                    H5Dclose(o);
                }
            }
            else
            {
                _objtype = DB_DIR;
                H5Gclose(o);
            }
        }
        else
        {
            /* Read the `silo_type' attribute */
            if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
                H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
                H5Aclose(attr)<0) {
                _objtype = DB_INVALID_OBJECT;
            }
            H5Tclose(o);
        }

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
    } END_PROTECT;
    return (DBObjectType)_objtype;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_InqMeshName
 *
 * Purpose:     Returns the name of the mesh with which the specified
 *              variable is associated.
 *
 * Bugs:        The user must pass a large enough buffer for the `meshname'
 *              argument. This function will never copy more than 1024 bytes
 *              into the `meshname' array, counting the null termination.
 *
 * Return:      Success:        0
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              Friday, April  9, 1999
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Oct 17 08:16:13 PDT 2006
 *   Made it try either "meshid" or "meshname" data members
 *
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_InqMeshName(DBfile *_dbfile, char const *name, char *meshname/*out*/)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_InqMeshName";
    int                  pass;

    for (pass = 0; pass < 2; pass++)
    {
        char   s[1024];
        hid_t  o=-1, attr=-1, type=-1, str_type=-1;

        s[0] = '\0';
        PROTECT {
            /* Describe memory */
            if ((str_type=H5Tcopy(H5T_C_S1))<0 ||
                H5Tset_size(str_type, sizeof s)<0 ||
                (type=H5Tcreate(H5T_COMPOUND, sizeof s))<0 ||
                db_hdf5_put_cmemb(type, pass==0?"meshid":"meshname",
                                  0, 0, NULL, str_type)<0) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }

            /* Open variable */
            if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0 ||
                (attr=H5Aopen_name(o, "silo"))<0) {
                db_perror(name, E_NOTFOUND, me);
                UNWIND();
            }

            /*
             * Read "silo" attribute. If the read fails it's probably because
             * there is no "meshid" field in the attribute, in which case we
             * haven't opened a mesh variable and we should fail.
             */
            if (H5Aread(attr, type, s)<0) {
                db_perror(name, E_CALLFAIL, me);
                UNWIND();
            }
        
            /* Copy result to output buffer and release resources */
            strcpy(meshname, s);
            H5Aclose(attr);
            H5Tclose(type);
            H5Tclose(str_type);
            H5Tclose(o);

        } CLEANUP {
            H5E_BEGIN_TRY {
                H5Aclose(attr);
                H5Tclose(type);
                H5Tclose(str_type);
                H5Tclose(o);
            } H5E_END_TRY;
        } END_PROTECT;

        if (s[0] != '\0')
            break;
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutMrgtree
 *
 * Purpose:     Write a mesh region grouping tree to a file.
 *
 * Programmer: Mark C. Miller
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutMrgtree(DBfile *_dbfile, char const *name, char const *mesh_name,
    DBmrgtree const *tree, DBoptlist const *opts)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBmrgtree_mt        m;
    int                 i, j, n, len, pass, num_nodes = tree->num_nodes;
    int                 tot_children, tot_segs;
    char               *s = 0;
    DBmrgtnode        **ltree = 0;
    char              **strArray = 0;
    int                *intArray = 0;

    memset(&m, 0, sizeof m);
    PROTECT {

        /* Set global options */
        db_ResetGlobalData_Mrgtree();
        db_ProcessOptlist(DB_MRGTREE, opts);

        /* allocate an emtpy, linearized list of tree node pointers */
        ltree = (DBmrgtnode **) malloc(num_nodes * sizeof(DBmrgtnode*));

        /* walk tree to populate the linearized list of nodes ptrs */
        DBWalkMrgtree(tree, (DBmrgwalkcb) DBLinearizeMrgtree, ltree, DB_POSTORDER);

        /* form an array of the integer, scalar data at each node */
        intArray = (int *) malloc(num_nodes * sizeof(int) * 6);
        for (i = 0; i < num_nodes; i++)
        {
            intArray[i*6+0] = ltree[i]->narray;
            intArray[i*6+1] = ltree[i]->type_info_bits;
            intArray[i*6+2] = ltree[i]->max_children;
            intArray[i*6+3] = ltree[i]->nsegs;
            intArray[i*6+4] = ltree[i]->num_children;
            intArray[i*6+5] = ltree[i] != tree->root ? ltree[i]->parent->walk_order : -1;
        }
        len = num_nodes * 6;
        db_hdf5_compwr(dbfile, DB_INT, 1, &len, intArray,
            m.n_scalars/*out*/, friendly_name(_dbfile,name,"_scalars", 0));
        FREE(intArray);

        /* form an array of strings of the name data member */
        strArray = (char **) malloc(num_nodes * sizeof(char *));
        for (i = 0; i < num_nodes; i++)
            strArray[i] = ltree[i]->name;
        
        /* output all the node names as one long dataset */
        s = 0;
        DBStringArrayToStringList((char const * const *)strArray, num_nodes, &s, &len);
        db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.n_name/*out*/,
            friendly_name(_dbfile,name, "_name", 0));
        FREE(s);
        FREE(strArray);

        /* form an array of strings of names data member */
        /* requires 2 passes; first pass only counts, 2nd pass builds the array */
        for (pass = 0; pass < 2; pass ++)
        {
            if (pass == 1)
            {
                if (n == 0)
                    break;
                strArray = (char **) malloc(n * sizeof(char *));
            }
            n = 0;
            for (i = 0; i < num_nodes; i++)
            {
                if (ltree[i]->narray > 0)
                {
                    if (strchr(ltree[i]->names[0], '%') == 0)
                    {
                        for (j = 0; j < ltree[i]->narray; j++)
                        {
                            if (pass == 1)
                                strArray[n] = ltree[i]->names[j];
                            n++;
                        }
                    }
                    else
                    {
                        if (pass == 1)
                            strArray[n] = ltree[i]->names[0];
                        n++;
                    }
                }
            }
        }
        if (n > 0)
        {
            s = 0;
            DBStringArrayToStringList((char const * const *)strArray, n, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.n_names/*out*/,
                friendly_name(_dbfile,name, "_names", 0));
            FREE(s);
            FREE(strArray);
        }

        /* linearize and output map name data */
        strArray = (char **) malloc(num_nodes * sizeof(char*));
        for (i = 0; i < num_nodes; i++)
            strArray[i] = ltree[i]->maps_name;
        s = 0;
        len = 0;
        DBStringArrayToStringList((char const * const *)strArray, num_nodes, &s, &len);
        db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.n_maps_name/*out*/,
            friendly_name(_dbfile,name, "_maps_name", 0));
        FREE(s);
        FREE(strArray);

        tot_segs = 0;
        for (i = 0; i < num_nodes; i++)
            tot_segs += ltree[i]->nsegs * (ltree[i]->narray?ltree[i]->narray:1);

        /* linearize and output map segment id data */
        if (tot_segs)
        {
            intArray = (int *) malloc(tot_segs * sizeof(int));
            n = 0;
            for (i = 0; i < num_nodes; i++)
                for (j = 0; j < ltree[i]->nsegs*(ltree[i]->narray?ltree[i]->narray:1); j++)
                    intArray[n++] = ltree[i]->seg_ids[j];
            db_hdf5_compwr(dbfile, DB_INT, 1, &tot_segs, intArray,
                m.n_seg_ids/*out*/, friendly_name(_dbfile,name,"_seg_ids", 0));
            FREE(intArray);

            /* linearize and output seg len type data */
            intArray = (int *) malloc(tot_segs * sizeof(int));
            n = 0;
            for (i = 0; i < num_nodes; i++)
                for (j = 0; j < ltree[i]->nsegs*(ltree[i]->narray?ltree[i]->narray:1); j++)
                    intArray[n++] = ltree[i]->seg_lens[j];
            db_hdf5_compwr(dbfile, DB_INT, 1, &tot_segs, intArray,
                m.n_seg_lens/*out*/, friendly_name(_dbfile,name,"_seg_lens", 0));
            FREE(intArray);

            /* linearize and output seg type data */
            intArray = (int *) malloc(tot_segs * sizeof(int));
            n = 0;
            for (i = 0; i < num_nodes; i++)
                for (j = 0; j < ltree[i]->nsegs*(ltree[i]->narray?ltree[i]->narray:1); j++)
                    intArray[n++] = ltree[i]->seg_types[j];
            db_hdf5_compwr(dbfile, DB_INT, 1, &tot_segs, intArray,
                m.n_seg_types/*out*/, friendly_name(_dbfile,name,"_seg_types", 0));
            FREE(intArray);
        }

        /* form integer array for children data */
        tot_children = 0;
        for (i = 0; i < num_nodes; i++)
            tot_children += ltree[i]->num_children;
        if (tot_children)
        {
            intArray = (int *) malloc(tot_children * sizeof(int));
            n = 0;
            for (i = 0; i < num_nodes; i++)
                for (j = 0; j < ltree[i]->num_children; j++)
                    intArray[n++] = ltree[i]->children[j]->walk_order;
            db_hdf5_compwr(dbfile, DB_INT, 1, &tot_children, intArray,
                m.n_children/*out*/, friendly_name(_dbfile,name,"_children", 0));
            FREE(intArray);
        }

        FREE(ltree);

        if (_mrgt._mrgvar_onames)
        {
            s = 0;
            len = 0;
            DBStringArrayToStringList((char const * const *)_mrgt._mrgvar_onames, -1, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.mrgvar_onames/*out*/,
                friendly_name(_dbfile,name, "_mrgvar_onames", 0));
            FREE(s);
        }

        if (_mrgt._mrgvar_rnames)
        {
            s = 0;
            len = 0;
            DBStringArrayToStringList((char const * const *)_mrgt._mrgvar_rnames, -1, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.mrgvar_rnames/*out*/,
                friendly_name(_dbfile,name, "_mrgvar_rnames", 0));
            FREE(s);
        }

        /* Build header in memory */
        m.src_mesh_type = tree->src_mesh_type;
        strcpy(m.src_mesh_name, OPT(mesh_name));
        m.type_info_bits = tree->type_info_bits;
        m.num_nodes = tree->num_nodes;
        m.root = tree->root->walk_order;
        
        /* Write header to file */
        STRUCT(DBmrgtree) {
            if (m.src_mesh_type)    MEMBER_S(int, src_mesh_type);
            MEMBER_S(str(m.src_mesh_name), src_mesh_name);
            if (m.type_info_bits)   MEMBER_S(int, type_info_bits);
            if (m.num_nodes)        MEMBER_S(int, num_nodes);
            MEMBER_S(int, root);
            MEMBER_S(str(m.n_scalars), n_scalars);
            MEMBER_S(str(m.n_name), n_name);
            MEMBER_S(str(m.n_names), n_names);
            MEMBER_S(str(m.n_maps_name), n_maps_name);
            MEMBER_S(str(m.n_seg_ids), n_seg_ids);
            MEMBER_S(str(m.n_seg_lens), n_seg_lens);
            MEMBER_S(str(m.n_seg_types), n_seg_types);
            MEMBER_S(str(m.n_children), n_children);
            MEMBER_S(str(m.mrgvar_onames), mrgvar_onames);
            MEMBER_S(str(m.mrgvar_rnames), mrgvar_rnames);
        } OUTPUT(dbfile, DB_MRGTREE, name, &m);

    } CLEANUP {
        FREE(ltree);
        FREE(intArray);
        FREE(strArray);
        FREE(s);
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetMrgtree
 *
 * Purpose:     Read a mesh region grouping tree from a file.
 *
 * Programmer: Mark C. Miller
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmrgtree *
db_hdf5_GetMrgtree(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetMrgtree";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i, j, n, num_nodes;
    DBmrgtree_mt         m;
    int                 *intArray = 0;
    char               **strArray = 0;
    DBmrgtnode         **ltree = 0;
    char                *s = 0;
    DBmrgtree           *tree = 0;
    
    PROTECT {
        /* Open object and make sure it's a material */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_MRGTREE!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBmrgtree_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        tree = (DBmrgtree*) malloc(sizeof(DBmrgtree));
        memset(tree, 0, sizeof(DBmrgtree));
        tree->name = BASEDUP(name);
        tree->src_mesh_name = OPTDUP(m.src_mesh_name);
        tree->type_info_bits = m.type_info_bits;
        tree->num_nodes = m.num_nodes;
        num_nodes = m.num_nodes;
        tree->root = 0;
        tree->cwr = 0;

        /* allocate all the nodes of the tree and a linear list of pointers
           to them */
        ltree = (DBmrgtnode **) malloc(num_nodes * sizeof(DBmrgtnode*));
        for (i = 0; i < num_nodes; i++)
            ltree[i] = (DBmrgtnode *) calloc(1,sizeof(DBmrgtnode));

        /* Read the nodal scalar data */
        intArray = (int *)db_hdf5_comprd(dbfile, m.n_scalars, 1);
        for (i = 0; (i < num_nodes) && intArray; i++)
        {
            ltree[i]->narray           = intArray[i*6+0];
            ltree[i]->type_info_bits   = intArray[i*6+1];
            ltree[i]->max_children     = intArray[i*6+2];
            ltree[i]->nsegs            = intArray[i*6+3];
            ltree[i]->num_children     = intArray[i*6+4];
            ltree[i]->parent           = intArray[i*6+5] >= 0 ? ltree[intArray[i*6+5]] : 0;
        }
        FREE(intArray);

        /* read the node 'name' member */
        s = (char *)db_hdf5_comprd(dbfile, m.n_name, 1);
        strArray = DBStringListToStringArray(s, &num_nodes, !skipFirstSemicolon);
        for (i = 0; (i < num_nodes) && strArray; i++)
            ltree[i]->name = strArray[i];
        FREE(s);
        FREE(strArray); /* free only top-level array of pointers */

        /* read the node 'names' member */
        s = (char *)db_hdf5_comprd(dbfile, m.n_names, 1);
        if (s)
        {
            strArray = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
            n = 0;
            for (i = 0; i < num_nodes; i++)
            {
                if (ltree[i]->narray == 0)
                    continue;

                if (strchr(strArray[n], '%') == 0)
                {
                    ltree[i]->names = (char**) malloc(ltree[i]->narray * sizeof(char*));
                    for (j = 0; j < ltree[i]->narray; j++, n++)
                        ltree[i]->names[j] = strArray[n];
                }
                else
                {
                    ltree[i]->names = (char**) malloc(1 * sizeof(char*));
                    ltree[i]->names[0] = strArray[n];
                    n++;
                }
            }
        }
        FREE(s);
        FREE(strArray); /* free only top-level array of pointers */

        /* read the maps_name data */
        s = (char *)db_hdf5_comprd(dbfile, m.n_maps_name, 1);
        strArray = DBStringListToStringArray(s, &num_nodes, !skipFirstSemicolon);
        for (i = 0; i < num_nodes; i++)
            ltree[i]->maps_name = strArray[i];
        FREE(s);
        FREE(strArray); /* free only top-level array of pointers */

        /* read the map segment id data */
        intArray = (int *)db_hdf5_comprd(dbfile, m.n_seg_ids, 1);
        n = 0;
        for (i = 0; (i < num_nodes) && intArray; i++)
        {
            int ns = ltree[i]->nsegs*(ltree[i]->narray?ltree[i]->narray:1);
            if (ns > 0)
            {
                ltree[i]->seg_ids = (int*) malloc(ns * sizeof(int));
                for (j = 0; j < ns; j++)
                    ltree[i]->seg_ids[j] = intArray[n++];
            }
        }
        FREE(intArray);

        /* read the map segment len data */
        intArray = (int *)db_hdf5_comprd(dbfile, m.n_seg_lens, 1);
        n = 0;
        for (i = 0; (i < num_nodes) && intArray; i++)
        {
            int ns = ltree[i]->nsegs*(ltree[i]->narray?ltree[i]->narray:1);
            if (ns > 0)
            {
                ltree[i]->seg_lens = (int*) malloc(ns * sizeof(int));
                for (j = 0; j < ns; j++)
                    ltree[i]->seg_lens[j] = intArray[n++];
            }
        }
        FREE(intArray);

        /* read the map segment type data */
        intArray = (int *)db_hdf5_comprd(dbfile, m.n_seg_types, 1);
        n = 0;
        for (i = 0; (i < num_nodes) && intArray; i++)
        {
            int ns = ltree[i]->nsegs*(ltree[i]->narray?ltree[i]->narray:1);
            if (ns > 0)
            {
                ltree[i]->seg_types = (int*) malloc(ns * sizeof(int));
                for (j = 0; j < ns; j++)
                    ltree[i]->seg_types[j] = intArray[n++];
            }
        }
        FREE(intArray);

        /* read the child ids */
        intArray = (int *)db_hdf5_comprd(dbfile, m.n_children, 1);
        n = 0;
        for (i = 0; (i < num_nodes) && intArray; i++)
        {
            int nc = ltree[i]->num_children;
            if (nc > 0)
            {
                ltree[i]->children = (DBmrgtnode**) malloc(nc * sizeof(DBmrgtnode*));
                for (j = 0; j < nc; j++)
                    ltree[i]->children[j] = ltree[intArray[n++]];
            }
        }
        FREE(intArray);

        s = (char *)db_hdf5_comprd(dbfile, m.mrgvar_onames, 1);
        if (s) tree->mrgvar_onames = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
        FREE(s);

        s = (char *)db_hdf5_comprd(dbfile, m.mrgvar_rnames, 1);
        if (s) tree->mrgvar_rnames = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
        FREE(s);

        tree->root = ltree[m.root];
        tree->cwr = tree->root;

        H5Tclose(o);
        FREE(ltree);

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        FREE(strArray);
        FREE(intArray);
        FREE(ltree);
        FREE(tree);
        FREE(s);
    } END_PROTECT;

    return tree;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutGroupelmap
 *
 * Purpose:     Write a grouping element map to a file.
 *
 * Programmer: Mark C. Miller
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutGroupelmap(DBfile *_dbfile, char const *name,
    int num_segments, int const *groupel_types, int const *segment_lengths,
    int const *segment_ids, int const * const *segment_data, void const * const *segment_fracs,
    int fracs_data_type, DBoptlist const *opts)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    DBgroupelmap_mt      m;
    int                  i, j, tot_len;
    int                *intArray;
    char               *s = NULL;

    memset(&m, 0, sizeof m);
    PROTECT {
        /* Set global options */
        db_ProcessOptlist(DB_GROUPELMAP, opts);

        /* Write raw data arrays */
        db_hdf5_compwr(dbfile, DB_INT, 1, &num_segments, groupel_types,
            m.groupel_types/*out*/, friendly_name(_dbfile,name,"_groupel_types", 0));
        db_hdf5_compwr(dbfile, DB_INT, 1, &num_segments, segment_lengths,
            m.segment_lengths/*out*/, friendly_name(_dbfile,name,"_segment_lengths", 0));
        if (segment_ids)
            db_hdf5_compwr(dbfile, DB_INT, 1, &num_segments, segment_ids,
                m.segment_ids/*out*/, friendly_name(_dbfile,name,"_segment_ids", 0));

        tot_len = 0;
        for (i = 0; i < num_segments; i++)
            tot_len += segment_lengths[i];
        if (tot_len)
        {
            intArray = (int *) malloc(tot_len * sizeof(int));
            tot_len = 0;
            for (i = 0; i < num_segments; i++)
                for (j = 0; j < segment_lengths[i]; j++)
                    intArray[tot_len++] = segment_data[i][j]; 
            db_hdf5_compwr(dbfile, DB_INT, 1, &tot_len, intArray,
                m.segment_data/*out*/, friendly_name(_dbfile,name,"_segment_data", 0));
            FREE(intArray);
        }

        /* write out fractional data if we have it */
        if (segment_fracs)
        {
            void *fracsArray;

            /* write array of frac lengths */
            tot_len = 0;
            intArray = (int *) malloc(num_segments * sizeof(int));
            for (i = 0; i < num_segments; i++)
            {
                int len = segment_fracs[i] == 0 ? 0 : segment_lengths[i];
                intArray[i] = len;
                tot_len += len; 
            }
            db_hdf5_compwr(dbfile, DB_INT, 1, &num_segments, intArray,
                m.frac_lengths/*out*/, friendly_name(_dbfile,name,"_frac_lengths", 0));
            FREE(intArray);

            /* build and write out fractional data array */
            fracsArray = (void *) malloc(tot_len * ((fracs_data_type==DB_FLOAT)?sizeof(float):sizeof(double))); 
            tot_len = 0;
            for (i = 0; i < num_segments; i++)
            {
                if (segment_fracs[i] == 0)
                    continue;

                for (j = 0; j < segment_lengths[i]; j++)
                {
                    if (fracs_data_type == DB_FLOAT)
                    {
                        float *pfa = (float *) fracsArray;
                        float *psf = (float *) segment_fracs[i];
                        pfa[tot_len++] = psf[j];
                    }
                    else
                    {
                        double *pfa = (double *) fracsArray;
                        double *psf = (double *) segment_fracs[i];
                        pfa[tot_len++] = psf[j];
                    }
                }
            }
            db_hdf5_compwr(dbfile, fracs_data_type, 1, &tot_len, fracsArray,
                m.segment_fracs/*out*/, friendly_name(_dbfile,name,"_segment_fracs", 0));
            FREE(fracsArray);
        } 
        
        /* Build header in memory */
        m.num_segments = num_segments;
        m.fracs_data_type = (DB_FLOAT==fracs_data_type || DB_DOUBLE==fracs_data_type)?0:fracs_data_type;
        
        /* Write header to file */
        STRUCT(DBgroupelmap) {
            if (m.num_segments)    MEMBER_S(int, num_segments);
            if (m.fracs_data_type) MEMBER_S(int, fracs_data_type);
            MEMBER_S(str(m.groupel_types), groupel_types);
            MEMBER_S(str(m.segment_lengths), segment_lengths);
            MEMBER_S(str(m.segment_ids), segment_ids);
            MEMBER_S(str(m.segment_data), segment_data);
            MEMBER_S(str(m.frac_lengths), frac_lengths);
            MEMBER_S(str(m.segment_fracs), segment_fracs);
        } OUTPUT(dbfile, DB_GROUPELMAP, name, &m);
    } CLEANUP {
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetGroupelmap
 *
 * Purpose:     Read a grouping element map from a file.
 *
 * Programmer: Mark C. Miller
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBgroupelmap *
db_hdf5_GetGroupelmap(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetGroupelmap";
    hid_t               o=-1, attr=-1;
    int                 _objtype, i, j, n;
    int                *intArray = 0;
    void               *fracsArray = 0;
    DBgroupelmap        *gm=NULL;
    DBgroupelmap_mt      m;
    
    PROTECT {
        /* Open object and make sure it's a material */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_GROUPELMAP!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read meta data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBgroupelmap_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create object and initialize meta data */
        gm = (DBgroupelmap *) malloc(sizeof(DBgroupelmap));
        gm->name = BASEDUP(name);
        gm->num_segments = m.num_segments;
        if ((gm->fracs_data_type = db_hdf5_GetVarType(_dbfile, m.segment_fracs)) < 0)
            gm->fracs_data_type = DB_DOUBLE;  /* PDB driver assumes double */
        if (gm->fracs_data_type == DB_DOUBLE && force_single_g)
            gm->fracs_data_type = DB_FLOAT;

        /* Read the raw data */
        gm->groupel_types = (int *)db_hdf5_comprd(dbfile, m.groupel_types, 1);
        gm->segment_lengths = (int *)db_hdf5_comprd(dbfile, m.segment_lengths, 1);
        gm->segment_ids = (int *)db_hdf5_comprd(dbfile, m.segment_ids, 1);

        /* read the map segment data */
        gm->segment_data = (int **) malloc(m.num_segments * sizeof(int*));
        intArray = (int *)db_hdf5_comprd(dbfile, m.segment_data, 1);
        n = 0;
        for (i = 0; (i < m.num_segments) && intArray && gm->segment_lengths; i++)
        {
            int sl = gm->segment_lengths[i];
            gm->segment_data[i] = 0;
            if (sl > 0)
            {
                gm->segment_data[i] = (int*) malloc(sl * sizeof(int));
                for (j = 0; j < sl; j++)
                    gm->segment_data[i][j] = intArray[n++];
            }
        }
        FREE(intArray);

        intArray = (int *)db_hdf5_comprd(dbfile, m.frac_lengths, 1);
        if (intArray)
        {
            gm->segment_fracs = (void **)malloc(m.num_segments * sizeof(void*));
            fracsArray = db_hdf5_comprd(dbfile, m.segment_fracs, 1);
            n = 0;
            for (i = 0; (i < m.num_segments) && fracsArray; i++)
            {
                int len = intArray[i];

                if (len <= 0)
                {
                    gm->segment_fracs[i] = 0;
                    continue;
                }

                gm->segment_fracs[i] = malloc(len * ((gm->fracs_data_type==DB_FLOAT)?sizeof(float):sizeof(double)));
                for (j = 0; j < len; j++)
                {
                    if (gm->fracs_data_type == DB_FLOAT)
                    {
                        float *pfa = (float *) fracsArray;
                        float *psf = (float *) gm->segment_fracs[i];
                        psf[j] = pfa[n++];
                    }
                    else
                    {
                        double *pfa = (double *) fracsArray;
                        double *psf = (double *) gm->segment_fracs[i];
                        psf[j] = pfa[n++];
                    }
                }
            }
        }
        else
        {
            gm->segment_fracs = 0;
        }
        FREE(intArray);
        FREE(fracsArray);

        H5Tclose(o);

    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        FREE(intArray);
        FREE(fracsArray);
    } END_PROTECT;

    return gm;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_PutMrgvar
 *
 * Purpose:     Write an mrgvar to a file 
 *
 * Programmer: Mark C. Miller
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK int
db_hdf5_PutMrgvar(DBfile *_dbfile, char const *name,
    char const *mrgt_name,
    int ncomps, char const * const *compnames,
    int nregns, char const * const *reg_pnames,
    int datatype, void const * const *data, DBoptlist const *optlist)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_PutMrgvar";
    char                *s = 0;
    int                 i, len, nstrs;
    DBmrgvar_mt         m;

    memset(&m, 0, sizeof m);

    PROTECT {

        db_ProcessOptlist(DB_MRGVAR, optlist);

        /* Write variable arrays */
        if (ncomps>MAX_VARS) {
            db_perror("too many variables", E_BADARGS, me);
            UNWIND();
        }
        for (i=0; i<ncomps; i++) {
            char tmpname[256];
            char const *p = tmpname;
            if (compnames)
                sprintf(tmpname, "%s_%s", name, compnames[i]);
            else
                sprintf(tmpname, "%s_comp%d", name, i);
            db_hdf5_compwr(dbfile, datatype, 1, &nregns, data[i],
                m.data[i]/*out*/, friendly_name(_dbfile,p, "_data", 0));
        }

        /* output reg_pnames */
        nstrs = nregns;
        if (strchr(reg_pnames[0], '%') != 0)
            nstrs = 1;
        DBStringArrayToStringList((char const * const *)reg_pnames, nstrs, &s, &len);
        db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.reg_pnames/*out*/,
                friendly_name(_dbfile,name, "_reg_pnames", 0));
        FREE(s);

        if (compnames)
        {
            /* output compnames */
            DBStringArrayToStringList((char const * const *)compnames, ncomps, &s, &len);
            db_hdf5_compwr(dbfile, DB_CHAR, 1, &len, s, m.compnames/*out*/,
                    friendly_name(_dbfile,name, "_compnames", 0));
            FREE(s);
        }

        /* Build header in memory */
        m.ncomps = ncomps;
        m.nregns = nregns;
        m.datatype = datatype;
        strcpy(m.mrgt_name, OPT(mrgt_name));

        /* Write header to file */
        STRUCT(DBmrgvar) {
            MEMBER_R(str(m.data[_j]), data, ncomps);
            MEMBER_S(str(m.mrgt_name), mrgt_name);
            if (m.ncomps)       MEMBER_S(int, ncomps);
            if (m.nregns)       MEMBER_S(int, nregns);
            if (m.datatype)     MEMBER_S(int, datatype);
            MEMBER_S(str(m.compnames), compnames);
            MEMBER_S(str(m.reg_pnames), reg_pnames);
        } OUTPUT(dbfile, DB_MRGVAR, name, &m);

    } CLEANUP {
        /*void*/
    } END_PROTECT;
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_GetMrgvar
 *
 * Purpose:     Read an mrgvar from a file.
 *
 * Programmer: Mark C. Miller
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Nov 10 09:14:01 PST 2009
 *   Added logic to control behavior of slash character swapping for
 *   windows/linux and skipping of first semicolon in calls to
 *   db_StringListToStringArray.
 *-------------------------------------------------------------------------
 */
SILO_CALLBACK DBmrgvar *
db_hdf5_GetMrgvar(DBfile *_dbfile, char const *name)
{
    DBfile_hdf5         *dbfile = (DBfile_hdf5*)_dbfile;
    static char         *me = "db_hdf5_GetMrgvar";
    char                *s = 0;
    hid_t               o=-1, attr=-1;
    int                 _objtype, i;
    DBmrgvar_mt         m;
    DBmrgvar           *mrgv=NULL;

    PROTECT {
        /* Open object and make sure it's a ucdvar */
        if ((o=H5Topen(dbfile->cwg, name, H5P_DEFAULT))<0) {
            db_perror(name, E_NOTFOUND, me);
            UNWIND();
        }
        if ((attr=H5Aopen_name(o, "silo_type"))<0 ||
            H5Aread(attr, H5T_NATIVE_INT, &_objtype)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        if (DB_MRGVAR!=(DBObjectType)_objtype) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Read ucdvar data into memory */
        memset(&m, 0, sizeof m);
        if ((attr=H5Aopen_name(o, "silo"))<0 ||
            H5Aread(attr, DBmrgvar_mt5, &m)<0 ||
            H5Aclose(attr)<0) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }

        /* Create a mrgvar object and initialize meta data */
        mrgv = (DBmrgvar *) calloc(1,sizeof(DBmrgvar));
        mrgv->name = BASEDUP(name);
        mrgv->mrgt_name = OPTDUP(m.mrgt_name);
        mrgv->nregns = m.nregns;
        mrgv->ncomps = m.ncomps;
        if ((mrgv->datatype = db_hdf5_GetVarType(_dbfile, m.data[0])) < 0)
            mrgv->datatype = silo2silo_type(m.datatype);
        if (force_single_g) mrgv->datatype = DB_FLOAT;

        /* Read the raw data */
        if (m.ncomps>MAX_VARS) {
            db_perror(name, E_CALLFAIL, me);
            UNWIND();
        }
        mrgv->data = (void **)calloc(m.ncomps, sizeof(void*));
        for (i=0; i<m.ncomps; i++) {
            mrgv->data[i] = db_hdf5_comprd(dbfile, m.data[i], 0);
        }

        s = (char *)db_hdf5_comprd(dbfile, m.compnames, 1);
        if (s) mrgv->compnames = DBStringListToStringArray(s, &m.ncomps, !skipFirstSemicolon);
        FREE(s);

        s = (char *)db_hdf5_comprd(dbfile, m.reg_pnames, 1);
        if (s) mrgv->reg_pnames = DBStringListToStringArray(s, 0, !skipFirstSemicolon);
        FREE(s);

        H5Tclose(o);
        
    } CLEANUP {
        H5E_BEGIN_TRY {
            H5Aclose(attr);
            H5Tclose(o);
        } H5E_END_TRY;
        DBFreeMrgvar(mrgv);
    } END_PROTECT;

    return mrgv;
}

SILO_CALLBACK int 
db_hdf5_FreeCompressionResources(DBfile *_dbfile, char const *meshname)
{
    FreeNodelists((DBfile_hdf5*)_dbfile, meshname);
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    db_hdf5_SortObjectsByOffset
 *
 * Purpose:     Returned array of indexes into an array of object names
 *              sorted by offset within the file. This can be used by
 *              readers to 'schedule' reading of objects to minimize
 *              jumping around the file.
 *
 * Programmer: Mark C. Miller, Thu Jul 15 07:42:46 PDT 2010
 *
 *-------------------------------------------------------------------------
 */

/* Support type for db_hdf5_SortObjectsByOffset */
typedef struct _index_offset_pair_t {
    int index;
    haddr_t offset;
} index_offset_pair_t;

/* Support function for db_hdf5_SortObjectsByOffset */
static int compare_index_offset_pair(void const *a1, void const *a2)
{
    index_offset_pair_t *p1 = (index_offset_pair_t*) a1;
    index_offset_pair_t *p2 = (index_offset_pair_t*) a2;
    if (p1->index == -1) return 1;
    if (p2->index == -1) return -1;
    if (p1->offset < p2->offset) return -1;
    else if (p1->offset > p2->offset) return 1;
    else return 0;
}

SILO_CALLBACK int
db_hdf5_SortObjectsByOffset(DBfile *_dbfile, int nobjs,
    char const *const *const names, int *ordering)
{

#if HDF5_VERSION_GE(1,12,0) /* [ */

    static char *me = "db_hdf5_SortObjectsByOffset";
    DBfile_hdf5 *dbfile = (DBfile_hdf5*)_dbfile;
    index_offset_pair_t *iop = (index_offset_pair_t*)
        malloc(nobjs * sizeof(index_offset_pair_t));
    int i;

    /* Gather up object addresses. If object names contain colon, ':'
       this indicates the object is in a different file than current.
       These will always be pushed to the 'back' of the list */
    for (i = 0; i < nobjs; i++)
    {
        iop[i].index = i;
        if (strchr(names[i], ':'))
            iop[i].offset = HADDR_MAX;
        else
        {
            hid_t objid;
            H5O_info2_t oinfo;
            char *objtokstr = 0;
            unsigned long long offset;

            if (H5Oget_info_by_name3(dbfile->cwg, names[i], &oinfo, H5O_INFO_BASIC, H5P_DEFAULT) < 0)
            {
                iop[i].offset = HADDR_MAX;
                continue;
            }
            objid = H5Oopen(dbfile->cwg, names[i], H5P_DEFAULT);
            H5Otoken_to_str(objid, &oinfo.token, &objtokstr);
            if (objtokstr)
            {
                errno = 0;
                offset = strtoull(objtokstr, 0, 0);
                iop[i].offset = errno ? HADDR_MAX : (haddr_t) offset;
                free(objtokstr);
            }
            H5Oclose(objid);
        }
    }

    /* Ok, sort the index/offset pairs */
    qsort(iop, nobjs, sizeof(index_offset_pair_t), compare_index_offset_pair);

    /* Populate ordering array */
    for (i = 0; i < nobjs; i++)
        ordering[i] = iop[i].index;

    free(iop);

    return 0;

#else /* ] HDF5_VERSION_GE(1,12,0) [ */

    return -1;

#endif /* ] HDF5_VERSION_GE(1,12,0) */
}

#if HDF5_VERSION_GE(1,8,9) /* [ */
/* Definition of callbacks for file image operations. */
/* Verbatim copy from 1.14.5 H5LT.c in HDF5 sources */

/*-------------------------------------------------------------------------
 * Function: image_malloc
 *
 * Purpose: Simulates malloc() function to avoid copying file images.
 *          The application buffer is set to the buffer on only one FAPL.
 *          Then the FAPL buffer can be copied to other FAPL buffers or
 *          to only one VFD buffer.
 *
 * Return: Address of "allocated" buffer, if successful. Otherwise, it returns
 *         NULL.
 *
 *-------------------------------------------------------------------------
 */
static void *
image_malloc(size_t size, H5FD_file_image_op_t file_image_op, void *_udata)
{
    db_hdf5_H5LT_file_image_ud_t *udata        = (db_hdf5_H5LT_file_image_ud_t *)_udata;
    void                 *return_value = NULL;

    /* callback is only used if the application buffer is not actually copied */
    if (!(udata->flags & db_hdf5_H5LT_FILE_IMAGE_DONT_COPY))
        goto out;

    switch (file_image_op) {
        /* the app buffer is "copied" to only one FAPL. Afterwards, FAPLs can be "copied" */
        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_SET:
            if (udata->app_image_ptr == NULL)
                goto out;
            if (udata->app_image_size != size)
                goto out;
            if (udata->fapl_image_ptr != NULL)
                goto out;
            if (udata->fapl_image_size != 0)
                goto out;
            if (udata->fapl_ref_count != 0)
                goto out;

            udata->fapl_image_ptr  = udata->app_image_ptr;
            udata->fapl_image_size = udata->app_image_size;
            return_value           = udata->fapl_image_ptr;
            udata->fapl_ref_count++;
            break;

        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_COPY:
            if (udata->fapl_image_ptr == NULL)
                goto out;
            if (udata->fapl_image_size != size)
                goto out;
            if (udata->fapl_ref_count == 0)
                goto out;

            return_value = udata->fapl_image_ptr;
            udata->fapl_ref_count++;
            break;

        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_GET:
            goto out;

        case H5FD_FILE_IMAGE_OP_FILE_OPEN:
            /* FAPL buffer is "copied" to only one VFD buffer */
            if (udata->vfd_image_ptr != NULL)
                goto out;
            if (udata->vfd_image_size != 0)
                goto out;
            if (udata->vfd_ref_count != 0)
                goto out;
            if (udata->fapl_image_ptr == NULL)
                goto out;
            if (udata->fapl_image_size != size)
                goto out;
            if (udata->fapl_ref_count == 0)
                goto out;

            udata->vfd_image_ptr  = udata->fapl_image_ptr;
            udata->vfd_image_size = size;
            udata->vfd_ref_count++;
            return_value = udata->vfd_image_ptr;
            break;

        /* added unused labels to shut the compiler up */
        case H5FD_FILE_IMAGE_OP_NO_OP:
        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE:
        case H5FD_FILE_IMAGE_OP_FILE_RESIZE:
        case H5FD_FILE_IMAGE_OP_FILE_CLOSE:
        default:
            goto out;
    } /* end switch */

    return (return_value);

out:
    return NULL;
} /* end image_malloc() */

/*-------------------------------------------------------------------------
 * Function: image_memcpy
 *
 * Purpose:  Simulates memcpy() function to avoid copying file images.
 *           The image buffer can be set to only one FAPL buffer, and
 *           "copied" to only one VFD buffer. The FAPL buffer can be
 *           "copied" to other FAPLs buffers.
 *
 * Return: The address of the destination buffer, if successful. Otherwise, it
 *         returns NULL.
 *
 *-------------------------------------------------------------------------
 */
static void *
image_memcpy(void *dest, const void *src, size_t size, H5FD_file_image_op_t file_image_op, void *_udata)
{
    db_hdf5_H5LT_file_image_ud_t *udata = (db_hdf5_H5LT_file_image_ud_t *)_udata;

    /* callback is only used if the application buffer is not actually copied */
    if (!(udata->flags & db_hdf5_H5LT_FILE_IMAGE_DONT_COPY))
        goto out;

    switch (file_image_op) {
        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_SET:
            if (dest != udata->fapl_image_ptr)
                goto out;
            if (src != udata->app_image_ptr)
                goto out;
            if (size != udata->fapl_image_size)
                goto out;
            if (size != udata->app_image_size)
                goto out;
            if (udata->fapl_ref_count == 0)
                goto out;
            break;

        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_COPY:
            if (dest != udata->fapl_image_ptr)
                goto out;
            if (src != udata->fapl_image_ptr)
                goto out;
            if (size != udata->fapl_image_size)
                goto out;
            if (udata->fapl_ref_count < 2)
                goto out;
            break;

        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_GET:
            goto out;

        case H5FD_FILE_IMAGE_OP_FILE_OPEN:
            if (dest != udata->vfd_image_ptr)
                goto out;
            if (src != udata->fapl_image_ptr)
                goto out;
            if (size != udata->vfd_image_size)
                goto out;
            if (size != udata->fapl_image_size)
                goto out;
            if (udata->fapl_ref_count == 0)
                goto out;
            if (udata->vfd_ref_count != 1)
                goto out;
            break;

        /* added unused labels to shut the compiler up */
        case H5FD_FILE_IMAGE_OP_NO_OP:
        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE:
        case H5FD_FILE_IMAGE_OP_FILE_RESIZE:
        case H5FD_FILE_IMAGE_OP_FILE_CLOSE:
        default:
            goto out;
    } /* end switch */

    return (dest);

out:
    return NULL;
} /* end image_memcpy() */

/*-------------------------------------------------------------------------
 * Function: image_realloc
 *
 * Purpose: Reallocates the shared application image buffer and updates data
 *          structures that manage buffer "copying".
 *
 * Return: Address of reallocated buffer, if successful. Otherwise, it returns
 *         NULL.
 *
 *-------------------------------------------------------------------------
 */
static void *
image_realloc(void *ptr, size_t size, H5FD_file_image_op_t file_image_op, void *_udata)
{
    db_hdf5_H5LT_file_image_ud_t *udata        = (db_hdf5_H5LT_file_image_ud_t *)_udata;
    void                 *return_value = NULL;

    /* callback is only used if the application buffer is not actually copied */
    if (!(udata->flags & db_hdf5_H5LT_FILE_IMAGE_DONT_COPY))
        goto out;

    /* realloc() is not allowed when the HDF5 library won't release the image
       buffer because reallocation may change the address of the buffer. The
       new address cannot be communicated to the application to release it. */
    if (udata->flags & db_hdf5_H5LT_FILE_IMAGE_DONT_RELEASE)
        goto out;

    /* realloc() is not allowed if the image is open in read-only mode */
    if (!(udata->flags & db_hdf5_H5LT_FILE_IMAGE_OPEN_RW))
        goto out;

    if (file_image_op == H5FD_FILE_IMAGE_OP_FILE_RESIZE) {
        void *tmp_realloc;

        if (udata->vfd_image_ptr != ptr)
            goto out;

        if (udata->vfd_ref_count != 1)
            goto out;

        /* Make sure all the udata structure image pointers
         * match each other before we update them
         */
        assert(udata->vfd_image_ptr == udata->app_image_ptr);
        assert(udata->vfd_image_ptr == udata->fapl_image_ptr);

        tmp_realloc = realloc(ptr, size);
        if (tmp_realloc) {
            udata->vfd_image_ptr  = tmp_realloc;
            udata->app_image_ptr  = udata->vfd_image_ptr;
            udata->fapl_image_ptr = udata->vfd_image_ptr;
        }
        else {
            free(ptr);
            udata->vfd_image_ptr  = NULL;
            udata->app_image_ptr  = NULL;
            udata->fapl_image_ptr = NULL;
            goto out;
        }

        udata->vfd_image_size = size;
        return_value          = udata->vfd_image_ptr;
    } /* end if */
    else
        goto out;

    return (return_value);

out:
    return NULL;
} /* end image_realloc() */

/*-------------------------------------------------------------------------
 * Function: image_free
 *
 * Purpose: Simulates deallocation of FAPL and VFD buffers by decreasing
 *          reference counters. Shared application buffer is actually
 *          deallocated if there are no outstanding references.
 *
 * Return: SUCCEED or FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
image_free(void *ptr, H5FD_file_image_op_t file_image_op, void *_udata)
{
    db_hdf5_H5LT_file_image_ud_t *udata = (db_hdf5_H5LT_file_image_ud_t *)_udata;

    /* callback is only used if the application buffer is not actually copied */
    if (!(udata->flags & db_hdf5_H5LT_FILE_IMAGE_DONT_COPY))
        goto out;

    switch (file_image_op) {
        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE:
            if (udata->fapl_image_ptr != ptr)
                goto out;
            if (udata->fapl_ref_count == 0)
                goto out;

            udata->fapl_ref_count--;

            /* release the shared buffer only if indicated by the respective flag and there are no outstanding
             * references */
            if (udata->fapl_ref_count == 0 && udata->vfd_ref_count == 0 &&
                !(udata->flags & db_hdf5_H5LT_FILE_IMAGE_DONT_RELEASE)) {
                free(udata->fapl_image_ptr);
                udata->app_image_ptr  = NULL;
                udata->fapl_image_ptr = NULL;
                udata->vfd_image_ptr  = NULL;
            } /* end if */
            break;

        case H5FD_FILE_IMAGE_OP_FILE_CLOSE:
            if (udata->vfd_image_ptr != ptr)
                goto out;
            if (udata->vfd_ref_count != 1)
                goto out;

            udata->vfd_ref_count--;

            /* release the shared buffer only if indicated by the respective flag and there are no outstanding
             * references */
            if (udata->fapl_ref_count == 0 && udata->vfd_ref_count == 0 &&
                !(udata->flags & db_hdf5_H5LT_FILE_IMAGE_DONT_RELEASE)) {
                /* Make sure we aren't going to leak memory elsewhere */
                assert(udata->app_image_ptr == udata->vfd_image_ptr || udata->app_image_ptr == NULL);
                assert(udata->fapl_image_ptr == udata->vfd_image_ptr || udata->fapl_image_ptr == NULL);

                free(udata->vfd_image_ptr);
                udata->app_image_ptr  = NULL;
                udata->fapl_image_ptr = NULL;
                udata->vfd_image_ptr  = NULL;
            }

            /* release reference to udata structure */
            if (udata_free(udata) < 0)
                goto out;

            break;

        /* added unused labels to keep the compiler quite */
        case H5FD_FILE_IMAGE_OP_NO_OP:
        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_SET:
        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_COPY:
        case H5FD_FILE_IMAGE_OP_PROPERTY_LIST_GET:
        case H5FD_FILE_IMAGE_OP_FILE_OPEN:
        case H5FD_FILE_IMAGE_OP_FILE_RESIZE:
        default:
            goto out;
    } /* end switch */

    return (0);

out:
    return (-1);
} /* end image_free() */

/*-------------------------------------------------------------------------
 * Function: udata_copy
 *
 * Purpose: Simulates the copying of the user data structure utilized in the
 *          management of the "copying" of file images.
 *
 * Return: Address of "newly allocated" structure, if successful. Otherwise, it
 *         returns NULL.
 *
 *-------------------------------------------------------------------------
 */
static void *
udata_copy(void *_udata)
{
    db_hdf5_H5LT_file_image_ud_t *udata = (db_hdf5_H5LT_file_image_ud_t *)_udata;

    /* callback is only used if the application buffer is not actually copied */
    if (!(udata->flags & db_hdf5_H5LT_FILE_IMAGE_DONT_COPY))
        goto out;
    if (udata->ref_count == 0)
        goto out;

    udata->ref_count++;

    return (udata);

out:
    return NULL;
} /* end udata_copy */

/*-------------------------------------------------------------------------
 * Function: udata_free
 *
 * Purpose: Simulates deallocation of the user data structure utilized in the
 *          management of the "copying" of file images. The data structure is
 *          actually deallocated when there are no outstanding references.
 *
 * Return: SUCCEED or FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
udata_free(void *_udata)
{
    db_hdf5_H5LT_file_image_ud_t *udata = (db_hdf5_H5LT_file_image_ud_t *)_udata;

    /* callback is only used if the application buffer is not actually copied */
    if (!(udata->flags & db_hdf5_H5LT_FILE_IMAGE_DONT_COPY))
        goto out;
    if (udata->ref_count == 0)
        goto out;

    udata->ref_count--;

    if (udata->ref_count == 0) {
        /* There should not be any outstanding references
         * to the udata structure at this point.
         */
        assert(udata->fapl_ref_count == 0);
        assert(udata->vfd_ref_count == 0);

        free(udata);
    }

    return (0);

out:
    return (-1);
} /* end udata_free */

/* End of callbacks definitions for file image operations */
#endif /* ] #if HDF5_VERSION_GE(1,8,9) */

#else /* ] defined(HAVE_HDF5_H) && defined(HAVE_LIBHDF5) [ */

/* Stub for when we don't have hdf5 */
INTERNAL DBfile *
db_hdf5_Open(char const *name, int mode, int subtype)
{
    db_perror(name, E_NOTIMP, "db_hdf5_Open");
    return NULL;
}

/* Stub for when we don't have hdf5 */
INTERNAL DBfile *
db_hdf5_Create(char const *name, int mode, int target, int subtype, char const *finfo)
{
    db_perror(name, E_NOTIMP, "db_hdf5_Create");
    return NULL;
}

/* Stub for when we don't have hdf5 */
INTERNAL int
db_hdf5_ForceSingle(int satus)
{
    return 0; /*no-op, don't fail*/
}

#endif /* ] defined(HAVE_HDF5_H) && defined(HAVE_LIBHDF5) */
