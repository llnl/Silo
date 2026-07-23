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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#define _db_open  _open
#define _db_close _close
#define _db_write _write
#define _db_read _read
#define _db_RDONLY _O_RDONLY
#define _db_WRONLY _O_WRONLY
#define _db_BINARY _O_BINARY
#define _db_TRUNC  _O_TRUNC
#define _db_CREAT  _O_CREAT
#define _db_MODE  (_S_IREAD | _S_IWRITE)
#else
#include <unistd.h>
#define _db_open  open
#define _db_close close
#define _db_write write
#define _db_read read
#define _db_RDONLY O_RDONLY
#define _db_WRONLY O_WRONLY
#define _db_TRUNC  O_TRUNC
#define _db_CREAT  O_CREAT
#define _db_BINARY 0
#define _db_MODE   0666
#endif

#include <silo.h>
#include <std.c>

#include <config.h>

#ifdef HAVE_HDF5_H
#include <hdf5.h>
#endif

#define ASSERT(A,M) \
{   db_errno = 0; \
    if (!(A) || db_errno!=0) \
    { char const *errmsg = db_errno ? DBErrString() : ""; \
        fprintf(stderr, "%s:%d:%s: assert=\"%s\", msg=\"%s\", sys=\"%s\".\n", \
            __FILE__, __LINE__, __func__, #A, #M, errmsg); \
        abort(); \
    } \
}

/*
typedef struct _DBmemfile_bufinfo
{
    void *buf;
    size_t size;
    size_t used;
} DBmemfile_bufinfo;
*/

static DBmemfile_bufinfo DBAssignbufinfo(void *buf, size_t size, size_t used)
{
    DBmemfile_bufinfo bi = {buf, size, used};
    return bi;
}

static int
ReadWholeFileToMem(char const *filename, size_t extrasize, DBmemfile_bufinfo *bi)
{
    struct stat statbuf;
    void *buf;
    int fd;

    /* get size of required buffer */
    ASSERT(!stat(filename, &statbuf),"");

    /* allocate a buffer to read the whole file into */
    ASSERT(buf = malloc(statbuf.st_size+extrasize),"");

    /* open and read the whole file into the buffer */
    ASSERT((fd = _db_open(filename, _db_RDONLY | _db_BINARY)) >= 0,"");

    /* read the whole file contents into memory */
    ASSERT(_db_read(fd, buf, statbuf.st_size) == statbuf.st_size,"");

    _db_close(fd);

    *bi = DBAssignbufinfo(buf, statbuf.st_size + extrasize, statbuf.st_size);

    return 0;
}

static int
WriteWholeFileFromMem(char const *filename, DBmemfile_bufinfo const *bi)
{
    int fd;
    struct stat statbuf;

    ASSERT((fd = _db_open(filename, _db_CREAT|_db_TRUNC|_db_WRONLY, _db_MODE)) >= 0,"");

    ASSERT(_db_write(fd, bi->buf, bi->used) == bi->used,"");

    _db_close(fd);

    ASSERT(!stat(filename, &statbuf),"");

    ASSERT(statbuf.st_size == bi->used,"");

    return 0;
}

int
main(int argc, char *argv[])
{
    int            i, nwrites;
    char          *basename="memfile";
    int            driver=DB_HDF5;
    char          *file_ext = "silo";
    int            show_all_errors = 0;
    char const    *filename = "rect2d.h5";
    DBmemfile_bufinfo srcbuf;
    DBmemfile_bufinfo dstbuf;
    DBfile        *srcdb, *dstdb, *dbfile;
    DBquadmesh    *qm;
    DBquadvar     *qv;
    DBoptlist *src_file_optlist;
    int fic_vfd = DB_H5VFD_FIC;
    int fic_flags = DB_H5_FIC_DONT_COPY | DB_H5_FIC_DONT_RELEASE;
    int core_vfd = DB_H5VFD_CORE;
    int src_core_optset; 
    int src_fic_optset;
    DBoptlist *dst_file_optlist;
    int dst_fic_optset;
    int no_backing_store = 1;

    /*
     * Parse the command-line.
     */
    for (i = 1; i < argc; i++)
    {
        if (!strncmp(argv[i], "DB_HDF5", 7))
            driver = StringToDriver(argv[i]);
        else if (!strcmp(argv[i], "show-all-errors"))
            show_all_errors = 1;
	else if (argv[i][0] != '\0')
            fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
    }

    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ABORT, NULL);

    /* Read whole file into a buf + 64 kilobytes (2^6*2^10) of extra space */
    if (ReadWholeFileToMem(filename, (1<<16), &srcbuf) != 0)
        return 1; 

    /* create a buffer for a new memory file we intend to copy into */
    dstbuf = DBAssignbufinfo(calloc(srcbuf.size,1),srcbuf.size,0);
    ASSERT(dstbuf.buf,"");

    /* Open the "file" we read into a memory buffer above, and then copy
       its contents into the newly created memory file using cpbuf...
       This works because I've been sure to allocate the destination 
       buffer large enough that no reallocs will be needed. */
    src_file_optlist = DBMakeOptlist(10);
    DBAddOption(src_file_optlist, DBOPT_H5_VFD, &fic_vfd);
    DBAddOption(src_file_optlist, DBOPT_H5_FIC_SIZE, &(srcbuf.size));
    DBAddOption(src_file_optlist, DBOPT_H5_FIC_BUF, srcbuf.buf);
    DBAddOption(src_file_optlist, DBOPT_H5_FIC_FLAGS, &(fic_flags));
    src_fic_optset = DBRegisterFileOptionsSet(src_file_optlist);

    /* Ok, now have Silo open this buffer */
    ASSERT(srcdb = DBOpen("dummy.silo", DB_HDF5_OPTS(src_fic_optset), DB_READ),"");

    /* Open the same buffer a second time in read-only mode. In theory, this should
     * work similarly to having the same actual file open multiple times in read-only */
    ASSERT(dstdb = DBOpen("dummy1.silo", DB_HDF5_OPTS(src_fic_optset), DB_READ),"");
    DBClose(dstdb);

    /* Confirm we can "read" this Silo file by accessing an object that should
       be present and interrogating its value(s). */
    ASSERT(qv=DBGetQuadvar(srcdb, "v"),"");
    ASSERT(!strcmp(qv->meshname,"quadmesh2d"),"");
    ASSERT(qv->nels==1271,"");
    ASSERT(qv->dims[0]==31&&qv->dims[1]==41,"");
    DBFreeQuadvar(qv);

    dst_file_optlist = DBMakeOptlist(10);
    DBAddOption(dst_file_optlist, DBOPT_H5_VFD, &core_vfd);
    DBAddOption(dst_file_optlist, DBOPT_H5_CORE_NO_BACK_STORE, &no_backing_store);
    dst_fic_optset = DBRegisterFileOptionsSet(dst_file_optlist);

    /* Lets *create* a file in memory using CORE vfd, copy something from the
       original file image we have already opened and then get the resulting buffer
       core file image and open that buffer as an HDF5 file. */
    ASSERT(dstdb = DBCreate("foo.silo", DB_CLOBBER, DB_LOCAL,
        "Copy", DB_HDF5_OPTS(dst_fic_optset)),"");
    {
        int i = 5;
        int nvals = 256;
        int vals[256]; /* 1 kilobyte worth of data */
        DBWrite(dstdb, DBSPrintf("foo_%02d",i), vals, &nvals, 1, DB_INT);
    }
    ASSERT(DBCp("-r", srcdb, dstdb, "mat1", "foo", DB_EOA)>=0, "");

#ifdef HAVE_HDF5_H
    /* Ok, we've created a "core" file. Lets flush it and get the
     * buffer of it back */
    {
        DBFlush(dstdb);
        hid_t hdf5_file_id = *((hid_t*) DBGrabDriver(dstdb));
        dstbuf.used = H5Fget_file_image(hdf5_file_id, NULL, 0);
        H5Fget_file_image(hdf5_file_id, dstbuf.buf, dstbuf.size);
        DBUngrabDriver(dstdb,0);
    }
#endif

    /* Now, close the files we were using */
    DBClose(srcdb);
    DBClose(dstdb);

    /* open origional file again, just to make sure the buffer is still ok */
    ASSERT(srcdb = DBOpen("dummy.silo", DB_HDF5_OPTS(src_fic_optset), DB_READ),"2nd open w/DB_READ");

    ASSERT(qm = DBGetQuadmesh(srcdb, "quadmesh2d"),"");
    DBClose(srcdb);

    /* free up the file options set */
    DBUnregisterFileOptionsSet(src_fic_optset);
    DBFreeOptlist(src_file_optlist);
    DBUnregisterFileOptionsSet(dst_fic_optset);
    DBFreeOptlist(dst_file_optlist);

    dst_file_optlist = DBMakeOptlist(10);
    DBAddOption(dst_file_optlist, DBOPT_H5_VFD, &fic_vfd);
    DBAddOption(dst_file_optlist, DBOPT_H5_FIC_SIZE, &(dstbuf.size));
    DBAddOption(dst_file_optlist, DBOPT_H5_FIC_BUF, dstbuf.buf);
    DBAddOption(dst_file_optlist, DBOPT_H5_FIC_FLAGS, &(fic_flags));
    dst_fic_optset = DBRegisterFileOptionsSet(dst_file_optlist);

    /* Now, open for append and write something to it */
    ASSERT(dbfile = DBOpen("dummy.silo", DB_HDF5_OPTS(dst_fic_optset), DB_APPEND), "Open core image");

    /* Try to write something to this file, but not so big it would cause a realloc */
    ASSERT(DBPutQuadmesh(dbfile, "quadmesh2d_dup", 0, qm->coords, qm->dims, qm->ndims,
        qm->datatype, qm->coordtype, 0)==0,"");
    DBFreeQuadmesh(qm);

    /* Lets close that file now too */
    DBClose(dbfile);

    /* The memfile buffer should be larger */
    ASSERT(dstbuf.size == srcbuf.size && dstbuf.used < srcbuf.used,"");

    /* Ok, now open again for append and write so much we cause a realloc */
    ASSERT(dbfile = DBOpen("dummy.silo", DB_HDF5_OPTS(dst_fic_optset), DB_APPEND), "2nd core image open");

    /* compute how many 1K writes we can do before we overrun the destination buffer */
    nwrites = (dstbuf.size - dstbuf.used) / (1<<10) / 2;
    for (i = 0; i < nwrites+1; i++)
    {
        int nvals = 256;
        int vals[256]; /* 1 kilobyte worth of data */
        DBWrite(dbfile, DBSPrintf("foo_%02d",i), vals, &nvals, 1, DB_INT);
    }

    DBClose(dbfile);
    DBSPrintf(0,0);
    DBFreeOptlist(dst_file_optlist);
    free(srcbuf.buf);
    free(dstbuf.buf);

    return 0;
}
