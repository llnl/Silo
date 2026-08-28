C***********************************************************************
C Copyright (C) 1994-2026 Lawrence Livermore National Security, LLC.
C LLNL-CODE-425250.
C All rights reserved.
C
C This file is part of Silo. For details, see silo.llnl.gov.
C***********************************************************************
C
C Purpose
C
C     Exercise the DB_F77NULL convention in Silo's Fortran interface.
C
C     In particular, verify
C
C       1. DB_F77NULL may be supplied for an absent mixed-data array.
C       2. DB_F77NULL may be supplied for optional facelist arrays.
C       3. A legitimate INTEGER mixed-data array whose first value is
C          -99 is not mistaken for DB_F77NULL.
C
C     Case 3 intentionally demonstrates why a wrapper must not blindly
C     apply FPTR() to a nullable data array when another argument (such
C     as mixlen) can unambiguously say whether the array is present.
C
C***********************************************************************

      program f77null

      implicit none
      include "silo.inc"

      integer dbid, err, driver, nargs, errmode
      integer status, nerrors
      integer dims(1), vdims(1)
      integer ivar(1), mixvar(1)
      integer rvar(1), rmixvar(1), rdims(1)
      integer rndims, rmixlen, rdatatype, rcentering
      integer nodelist(2), shapesize(1), shapecnt(1)
      character*256 cloption
      real x(2)

      errmode = DB_NONE
      driver = DB_PDB
      nargs = iargc()
      if (nargs .gt. 0) then
          call getarg(1, cloption)
          if (cloption .eq. "DB_HDF5") driver = DB_HDF5
          if (cloption .eq. "show-all-errors") then
              errmode = DB_ALL_AND_DRVR
          endif
      endif

      status = dbshowerrors(errmode)

      nerrors = 0

C...Create a tiny 1-D quad mesh.

      x(1) = 0.0
      x(2) = 1.0
      dims(1) = 2

      err = dbcreate("f77null.silo", 12, DB_CLOBBER, DB_LOCAL,
     .               "DB_F77NULL regression test", 26,
     .               driver, dbid)
      if (err .ne. 0) then
          print *, "dbcreate failed"
          stop 1
      endif

      status = 0
      err = dbputqm(dbid, "mesh", 4,
     .              "X", 1, DB_F77NULLSTRING, 0,
     .              DB_F77NULLSTRING, 0,
     .              x, DB_F77NULL, DB_F77NULL,
     .              dims, 1, DB_FLOAT, DB_COLLINEAR,
     .              DB_F77NULL, status)
      if (err .ne. 0 .or. status .lt. 0) then
          print *, "dbputqm failed"
          nerrors = nerrors + 1
      endif

C...A scalar zone-centered variable with no mixed data.

      vdims(1) = 1
      ivar(1) = 17

      status = 0
      err = dbputqv1(dbid, "nomix", 5, "mesh", 4,
     .               ivar, vdims, 1, DB_F77NULL, 0,
     .               DB_INT, DB_ZONECENT, DB_F77NULL, status)
      if (err .ne. 0 .or. status .lt. 0) then
          print *, "dbputqv1 with DB_F77NULL mixvar failed"
          nerrors = nerrors + 1
      endif

C...Now provide real mixed data whose first value happens to equal the
C...DB_F77NULL magic integer value.  This is legitimate data and must
C...NOT be converted to a C NULL pointer.
C...
C...A wrapper of the form FPTR(mixvar) gets this wrong because FPTR
C...examines the first INTEGER value.  For DBPutQuadvar1, mixlen already
C...tells the wrapper whether mixvar is present, so the safer conversion
C...is conceptually: *mixlen == 0 ? NULL : mixvar

      mixvar(1) = -99

      status = 0
      err = dbputqv1(dbid, "minus99", 7, "mesh", 4,
     .               ivar, vdims, 1, mixvar, 1,
     .               DB_INT, DB_ZONECENT, DB_F77NULL, status)
      if (err .ne. 0 .or. status .lt. 0) then
          print *, "dbputqv1 rejected legitimate mixvar(1)=-99"
          nerrors = nerrors + 1
      endif

C...Exercise DB_F77NULL for optional facelist arrays.

      nodelist(1) = 0
      nodelist(2) = 1
      shapesize(1) = 2
      shapecnt(1) = 1

      status = 0
      err = dbputfl(dbid, "fl", 2, 1, 2,
     .              nodelist, 2, 0, DB_F77NULL,
     .              shapesize, shapecnt, 1,
     .              DB_F77NULL, DB_F77NULL, 0, status)
      if (err .ne. 0 .or. status .lt. 0) then
          print *, "dbputfl with DB_F77NULL optional arrays failed"
          nerrors = nerrors + 1
      endif

      err = dbclose(dbid)
      if (err .ne. 0) then
          print *, "dbclose failed"
          nerrors = nerrors + 1
      endif

C...Read the -99 mixed data back.

      err = dbopen("f77null.silo", 12, driver, DB_READ, dbid)
      if (err .ne. 0) then
          print *, "dbopen failed"
          stop 1
      endif

      rvar(1) = 0
      rmixvar(1) = 0
      rdims(1) = 0
      rndims = 0
      rmixlen = 0
      rdatatype = 0
      rcentering = 0

      err = dbgetqv1(dbid, "minus99", 7, rvar, rdims, rndims,
     .               rmixvar, rmixlen, rdatatype, rcentering)
      if (err .ne. 0) then
          print *, "dbgetqv1(minus99) failed"
          nerrors = nerrors + 1
      else
          if (rmixlen .ne. 1) then
              print *, "wrong mixlen; expected 1, got ", rmixlen
              nerrors = nerrors + 1
          endif
          if (rmixvar(1) .ne. -99) then
              print *, "wrong mixed value; expected -99, got ",
     .                 rmixvar(1)
              nerrors = nerrors + 1
          endif
      endif

C...Also exercise DB_F77NULL on the output side of dbgetqv1.

      rvar(1) = 0
      rdims(1) = 0
      rndims = 0
      rmixlen = 0
      rdatatype = 0
      rcentering = 0

      err = dbgetqv1(dbid, "nomix", 5, rvar, rdims, rndims,
     .               DB_F77NULL, rmixlen, rdatatype, rcentering)
      if (err .ne. 0) then
          print *, "dbgetqv1 with DB_F77NULL mixvar failed"
          nerrors = nerrors + 1
      endif

      err = dbclose(dbid)
      if (err .ne. 0) then
          print *, "final dbclose failed"
          nerrors = nerrors + 1
      endif

      if (nerrors .ne. 0) then
          print *, "DB_F77NULL regression test failed:", nerrors
          stop 1
      endif

      print *, "DB_F77NULL regression test passed"
      stop
      end
