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
/*
 * PDLOW.C - low level routines for PDBlib
 *
 * Source Version: 9.0
 * Software Release #92-0043
 *
 * Modifications:
 *
 *      Robb Matzke, 17 Apr 1997
 *      Includes config.h for the HAVE_F*_POINTER feature test results.
 */
#include "config.h"
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if !defined(_WIN32)
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif
#include "pdb.h"

/* These prototypes are only included if we can't get them any other place.
 * We need them for their pointers, below.
 */
#ifndef HAVE_FCLOSE_POINTER
extern int fclose(FILE*);
#endif
#ifndef HAVE_FSEEK_POINTER
extern int fseek(FILE*, long, int);
#endif
#ifndef HAVE_FPRINTF_POINTER
extern int fprintf(FILE*, const char*, ...);
#endif

#define BUFINCR         4096
#define N_CASTS_INCR    30


#define PD_REVERSE_LIST(type, var, member)                                  \
   {type *ths, *nxt, *prv;                                                  \
    for (ths = NULL, nxt = var; nxt != NULL; )                              \
        {prv = ths;                                                         \
         ths = nxt;                                                         \
         nxt = ths->member;                                                 \
         ths->member = prv;};                                               \
    var = ths;}

#define PD_COMPARE_PTR_STD(eq, a, b, c, d)                                   \
    eq = (a->ptr_bytes != b->ptr_bytes) ||                                   \
         (c->ptr_alignment != d->ptr_alignment)

#define PD_COMPARE_CHAR_STD(eq, a, b, c, d)                                  \
    eq = (c->char_alignment != d->char_alignment)

#define PD_COMPARE_SHORT_STD(eq, a, b, c, d)                                 \
   PD_COMPARE_FIX_STD(eq,                                                    \
                      a->short_bytes,     b->short_bytes,                    \
                      a->short_order,     b->short_order,                    \
                      c->short_alignment, d->short_alignment)

#define PD_COMPARE_INT_STD(eq, a, b, c, d)                                   \
   PD_COMPARE_FIX_STD(eq,                                                    \
                      a->int_bytes,     b->int_bytes,                        \
                      a->int_order,     b->int_order,                        \
                      c->int_alignment, d->int_alignment)

#define PD_COMPARE_LONG_STD(eq, a, b, c, d)                                  \
   PD_COMPARE_FIX_STD(eq,                                                    \
                      a->long_bytes,     b->long_bytes,                      \
                      a->long_order,     b->long_order,                      \
                      c->long_alignment, d->long_alignment)

#define PD_COMPARE_LONGLONG_STD(eq, a, b, c, d)                              \
   PD_COMPARE_FIX_STD(eq,                                                    \
                      a->longlong_bytes,     b->longlong_bytes,              \
                      a->longlong_order,     b->longlong_order,              \
                      c->longlong_alignment, d->longlong_alignment)

#define PD_COMPARE_FIX_STD(eq, na, nb, oa, ob, la, lb)                       \
    eq = (na != nb) || (oa != ob) || (la != lb)

#define PD_COMPARE_FLT_STD(eq, a, b, c, d)                                   \
   PD_COMPARE_FP_STD(eq,                                                     \
                     a->float_bytes,     b->float_bytes,                     \
                     a->float_order,     b->float_order,                     \
                     a->float_format,    b->float_format,                    \
                     c->float_alignment, d->float_alignment)

#define PD_COMPARE_DBL_STD(eq, a, b, c, d)                                   \
   PD_COMPARE_FP_STD(eq,                                                     \
                     a->double_bytes,     b->double_bytes,                   \
                     a->double_order,     b->double_order,                   \
                     a->double_format,    b->double_format,                  \
                     c->double_alignment, d->double_alignment)

#define PD_COMPARE_FP_STD(eq, na, nb, oa, ob, fa, fb, la, lb)                \
   {int j, *poa, *pob;                                                       \
    long *pfa, *pfb;                                                         \
    poa = oa;                                                                \
    pob = ob;                                                                \
    pfa = fa;                                                                \
    pfb = fb;                                                                \
    eq  = (na != nb) || (la != lb);                                          \
    if (!eq)                                                                 \
       {for (j = 0; j < na; j++, eq |= (*(poa++) != *(pob++)));              \
        for (j = 0; j < lite_FORMAT_FIELDS; j++, eq |= (*(pfa++) != *(pfb++)));};}


char            *_lite_PD_tbuffer = NULL;
char            *lite_PD_SYMENT_S = NULL;
char            *lite_PD_DEFSTR_S = NULL;

static long     _PD_n_casts = 0L;
static int      _PD_has_dirs = FALSE;
static char     local[LRG_TXT_BUFFER];
static char     **_PD_cast_lst ;

static defstr * _lite_PD_defstr (HASHTAB*,char*,int,long,int,int,int*,long*);
static char *   _PD_get_tok (char*,int,FILE*,int);
static char *   _PD_get_token (char*,char*,int,int);
static int      _PD_consistent_dims (PDBfile*,syment*,dimdes*);

#ifdef PDB_WRITE
static int      _PD_put_string (int,char*,...);
#endif /* PDB_WRITE */
   
/*-------------------------------------------------------------------------
 * Function:    _lite_PD_rd_format
 *
 * Purpose:     Read the primitive data format information from the
 *              file header block.
 *
 *              Floating Point Format Descriptor                        
 *                format[0] = # of bits per number                      
 *                format[1] = # of bits in exponent                     
 *                format[2] = # of bits in mantissa                     
 *                format[3] = start bit of sign                         
 *                format[4] = start bit of exponent                     
 *                format[5] = start bit of mantissa                     
 *                format[6] = high order mantissa bit (CRAY needs this) 
 *                format[7] = bias of exponent
 *
 * Return:      Success:        TRUE
 *
 *              Failure:        FALSE
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996 11:55 AM EST
 *
 * Modifications:
 *
 *   Mark C. Miller, Fri Nov 13 15:33:42 PST 2009
 *   Added support for long long datatype.
 *
 *   Mark C. Miller, Tue Nov 17 22:25:58 PST 2009
 *   Moved support for long long to 'extras' to match more closely
 *   what PDB proper would do.
 *-------------------------------------------------------------------------
 */
int
_lite_PD_rd_format (PDBfile *file) {

   int j, n, *order;
   long *format;
   char infor[MAXLINE], *p;
   data_standard *std;

   /*
    * Read the number of bytes worth of format data.
    */
   if (io_read(infor, (size_t) 1, (size_t) 1, file->stream) != 1)
      lite_PD_error("FAILED TO READ FORMAT HEADER - _PD_RD_FORMAT", PD_OPEN);
    
   n = infor[0] - 1;

   /*
    * Read the format data
    */
   if (io_read(infor+1, (size_t) 1, (size_t) n, file->stream) != n)
      lite_PD_error("FAILED TO READ FORMAT DATA - _PD_RD_FORMAT", PD_OPEN);
    
   /*
    * Decipher the format data.
    */
   p             = infor + 1;
   std           = _lite_PD_mk_standard();

   /*
    * Get the byte lengths in.
    */
   std->ptr_bytes    = *(p++);
   std->short_bytes  = *(p++);
   std->int_bytes    = *(p++);
   std->long_bytes   = *(p++);
   std->float_bytes  = *(p++);
   std->double_bytes = *(p++);

   /*
    * Get the integral types byte order in.
    */
   std->short_order = (char) *(p++);
   std->int_order   = (char) *(p++);
   std->long_order  = (char) *(p++);

   /*
    * Get the float byte order in.
    */
   n     = std->float_bytes;
   order = std->float_order = FMAKE_N(int, n, "_PD_RD_FORMAT:float_order");
   for (j = 0; j < n; j++, *(order++) = *(p++)) /*void*/ ;

   /*
    * Get the double byte order in.
    */
   n     = std->double_bytes;
   order = std->double_order = FMAKE_N(int, n, "_PD_RD_FORMAT:double_order");
   for (j = 0; j < n; j++, *(order++) = *(p++)) /*void*/ ;

   /*
    * Get the float format data in.
    */
   n = lite_FORMAT_FIELDS;
   format = std->float_format = FMAKE_N(long, n, "_PD_RD_FORMAT:float_format");
   n--;
   for (j = 0; j < n; j++, *(format++) = *(p++)) /*void*/ ;

   /*
    * Get the double format data in.
    */
   n = lite_FORMAT_FIELDS;
   format = std->double_format = FMAKE_N(long, n,
                                         "_PD_RD_FORMAT:double_format");
   n--;
   for (j = 0; j < n; j++, *(format++) = *(p++)) /*void*/ ;

   /*
    * Read the biases.
    */
   if (_lite_PD_rfgets(infor, MAXLINE, file->stream) == NULL)
      lite_PD_error("CAN'T READ THE BIASES - _PD_RD_FORMAT", PD_OPEN);

   format    = std->float_format;
   format[7] = lite_SC_stol(strtok(infor, "\001"));
   format    = std->double_format;
   format[7] = lite_SC_stol(strtok(NULL, "\001"));

   file->std = std;

   return(TRUE);
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_compare_std
 *
 * Purpose:     Compare two data_standards and the associated alignments.
 *
 * Return:      Success:        TRUE if all elements are equal
 *                              FALSE otherwise
 *
 *              Failure:        Never fails.
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  4, 1996 12:50 PM EST
 *
 * Modifications:
 *
 *   Mark C. Miller, Fri Nov 13 15:33:42 PST 2009
 *   Added support for long long datatype.
 *
 *   Mark C. Miller, Tue Nov 17 22:26:49 PST 2009
 *   Fixed missing comparison for longlong_order.
 *-------------------------------------------------------------------------
 */
int
_lite_PD_compare_std (data_standard *a, data_standard *b,
                      data_alignment *c, data_alignment *d) {

   int          j, n, *oa, *ob, eq;
   long         *fa, *fb;

   eq = ((a->ptr_bytes    == b->ptr_bytes) &&
         (a->short_bytes  == b->short_bytes) &&
         (a->int_bytes    == b->int_bytes) &&
         (a->long_bytes   == b->long_bytes) &&
         (a->longlong_bytes == b->longlong_bytes) &&
         (a->float_bytes  == b->float_bytes) &&
         (a->double_bytes == b->double_bytes) &&
         (a->short_order  == b->short_order) &&
         (a->int_order    == b->int_order) &&
         (a->long_order   == b->long_order) &&
         (a->longlong_order == b->longlong_order));
   
   if (!eq) return(FALSE);

   /*
    * Check the float byte order.
    */
   n  = a->float_bytes;
   oa = a->float_order;
   ob = b->float_order;
   for (j = 0; j < n; j++, eq &= (*(oa++) == *(ob++))) /*void*/ ;

   /*
    * Check the double byte order.
    */
   n  = a->double_bytes;
   oa = a->double_order;
   ob = b->double_order;
   for (j = 0; j < n; j++, eq &= (*(oa++) == *(ob++))) /*void*/ ;

   /*
    * Check the float format data.
    */
   n  = lite_FORMAT_FIELDS;
   fa = a->float_format;
   fb = b->float_format;
   for (j = 0; j < n; j++, eq &= (*(fa++) == *(fb++))) /*void*/ ;

   /*
    * Check the double format data.
    */
   n  = lite_FORMAT_FIELDS;
   fa = a->double_format;
   fb = b->double_format;
   for (j = 0; j < n; j++, eq &= (*(fa++) == *(fb++))) /*void*/ ;

   /*
    * Check alignments.
    */
   eq &= ((c->char_alignment   == d->char_alignment) &&
          (c->ptr_alignment    == d->ptr_alignment) &&
          (c->short_alignment  == d->short_alignment) &&
          (c->int_alignment    == d->int_alignment) &&
          (c->long_alignment   == d->long_alignment) &&
          (c->longlong_alignment == d->longlong_alignment) &&
          (c->float_alignment  == d->float_alignment) &&
          (c->double_alignment == d->double_alignment));

   return(eq);
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_rd_symt
 *
 * Purpose:     Read the symbol table from the PDB file into a hash table
 *              of the PDB for future lookup.
 *
 * Return:      Success:        TRUE
 *
 *              Failure:        FALSE
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996 11:59 AM EST
 *
 * Modifications:
 *    Eric Brugger, Tue Dec  8 15:53:07 PST 1998
 *    Remove unnecessary calls to lite_SC_mark, since reference count now
 *    set when allocated.
 *
 *    Mark C. Miller, Fri Apr 13 22:40:59 PDT 2012
 *    Ignore symbols of the form "/&ptrs/ia_######" when controlling
 *    flag is set.
 *-------------------------------------------------------------------------
 */
int
_lite_PD_rd_symt (PDBfile *file) {

   char *name, *type, *tmp, *pbf;
   long numb, addr, mini, leng, symt_sz;
   FILE *fp;
   syment *ep;
   HASHTAB *tab;
   dimdes *dims, *next, *prev;

   fp = file->stream;

   /*
    * Find the overall file length.
    */
   addr = io_tell(fp);
   io_seek(fp, 0, SEEK_END);
   numb = io_tell(fp);
   io_seek(fp, addr, SEEK_SET);

   /*
    * Read in the symbol table and extras table as a single block.
    */
   symt_sz     = numb - file->symtaddr + 1;
   _lite_PD_tbuffer = MAKE_N(char, symt_sz);
   numb        = io_read(_lite_PD_tbuffer, 1, symt_sz, fp) + 1;
   if (numb != symt_sz) return(FALSE);
   _lite_PD_tbuffer[symt_sz-1] = (char) EOF;

   pbf  = _lite_PD_tbuffer;
   prev = NULL;
   tab  = file->symtab;
   while (_PD_get_token(pbf, local, LRG_TXT_BUFFER, '\n')) {
      pbf  = NULL;
      name = strtok(local, "\001");
      if (name == NULL) break;
      type = strtok(NULL, "\001");
      numb = lite_SC_stol(strtok(NULL, "\001"));
      addr = lite_SC_stol(strtok(NULL, "\001"));
      dims = NULL;
      while ((tmp = strtok(NULL, "\001\n")) != NULL) {
         mini = lite_SC_stol(tmp);
         leng = lite_SC_stol(strtok(NULL, "\001\n"));
         next = _lite_PD_mk_dimensions(mini, leng);
         if (dims == NULL) {
            dims = next;
         } else {
            prev->next = next;
         }

         prev = next;
      }
      if (file->ignore_apersand_ptr_ia_syms &&
          strstr(name, "/&ptrs/ia_")) continue;
      ep = _lite_PD_mk_syment(type, numb, addr, NULL, dims);
      _lite_PD_e_install(name, ep, tab);
   }
   return(TRUE);
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_rd_chrt
 *
 * Purpose:     Read the structure chart from the PDB file into the
 *              internal structure chart.
 *
 * Return:      Success:        TRUE
 *
 *              Failure:        FALSE
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996 11:47 AM EST
 *
 * Modifications:
 *      Sean Ahern, Wed Apr 12 10:49:15 PDT 2000
 *      Removed some compiler warnings.
 *
 *      Mark C. Miller, Tue Sep 15 18:35:42 PDT 2020
 *      Create temp. copy of _PD_cast_lst to avoid memory corruption
 *      when its used in both file and host charts.
 *-------------------------------------------------------------------------*/
int
_lite_PD_rd_chrt (PDBfile *file) {

   char *nxt, type[MAXLINE], *pbf;
   FILE *fp;
   memdes *desc, *lst, *prev;
   long i, chrt_sz;
   char **_PD_cast_lst_cpy;

   fp = file->stream;

   /*
    * Read the entire chart into memory to speed processing.
    */
   chrt_sz     = file->symtaddr - file->chrtaddr + 1;
   _lite_PD_tbuffer = MAKE_N(char, chrt_sz);
   if (chrt_sz <= 0) return(FALSE);
   if (io_read(_lite_PD_tbuffer, 1, chrt_sz, fp) != chrt_sz) return(FALSE);
   _lite_PD_tbuffer[chrt_sz-1] = (char) EOF;

   prev = NULL;
   pbf  = _lite_PD_tbuffer;
   while (_PD_get_token(pbf, type, MAXLINE, '\001')) {
      pbf = NULL;
      if (type[0] == '\002') break;
      _PD_get_token(pbf, local, MAXLINE, '\001');
      lst  = NULL;
      while ((nxt = _PD_get_token(pbf, local, MAXLINE, '\001')) != NULL) {
         if (*nxt == '\0') break;
         desc = _lite_PD_mk_descriptor(nxt, file->default_offset);
         if (lst == NULL) lst = desc;
         else prev->next = desc;
         prev = desc;
      }

      /*
       * Install the type in both charts.
       */
      _lite_PD_defstr_inst(type, lst, -1, NULL, NULL,
                           file->chart, file->host_chart,
                           file->align, file->host_align,
                           FALSE);};

   /*
    * Complete the setting of the directory indicator.
    */
   if (_PD_has_dirs) lite_PD_defncv(file, "Directory", 1, 0);
   _PD_has_dirs = FALSE;

   /*
    * Copy the cast list.
    */
   _PD_cast_lst_cpy = FMAKE_N(char *, _PD_n_casts, "_PD_RD_CHART:cast-list-copy");
   for (i = 0L; i < _PD_n_casts; i += 3) {
      _PD_cast_lst_cpy[i  ] = lite_SC_strsavef(_PD_cast_lst[i  ], "char*:_PD_RD_CHART:cast-list-copy-local1");
      _PD_cast_lst_cpy[i+1] = lite_SC_strsavef(_PD_cast_lst[i+1], "char*:_PD_RD_CHART:cast-list-copy-local2");
      _PD_cast_lst_cpy[i+2] = lite_SC_strsavef(_PD_cast_lst[i+2], "char*:_PD_RD_CHART:cast-list-copy-local3");
   }

   /*
    * Check the casts for the file->chart.
    */
   _lite_PD_check_casts(file->chart, _PD_cast_lst, _PD_n_casts);

   /*
    * Check the casts for the file->host_chart using the copied cast list.
    */
   _lite_PD_check_casts(file->host_chart, _PD_cast_lst_cpy, _PD_n_casts);

   /*
    * Clean up the mess.
    */
   for (i = 0L; i < _PD_n_casts; i += 3) {
      SFREE(_PD_cast_lst[i]);
      SFREE(_PD_cast_lst[i+1]);
      SFREE(_PD_cast_lst_cpy[i]);
      SFREE(_PD_cast_lst_cpy[i+1]);
   }
   SFREE(_PD_cast_lst);
   SFREE(_PD_cast_lst_cpy);
   _PD_n_casts = 0L;

   SFREE(_lite_PD_tbuffer);

   return(TRUE);
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_check_casts
 *
 * Purpose:     Complete the set up of the casts in the given chart.
 *
 * Return:      void
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996  1:32 PM EST
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
_lite_PD_check_casts (HASHTAB *chrt, char **lst, long n) {

   memdes *memb, *desc;
   long i;
   hashel *hp;
   defstr *dp;

   for (hp = *(chrt->table); hp != NULL; hp = hp->next) {
      dp = (defstr *) hp->def;
      for (desc = dp->members; desc != NULL; desc = desc->next) {
         for (i = 0L; i < n; i += 3) {
            if ((strcmp(dp->type, lst[i]) == 0) &&
                (strcmp(desc->member, lst[i+1]) == 0)) {
               desc->cast_memb = lst[i+2];
               desc->cast_offs = _lite_PD_member_location(desc->cast_memb,
                                                          chrt, dp, &memb);
            }
         }
      }
   }
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_rd_extras
 *
 * Purpose:     Read any extraneous data from the file. This is essentially
 *              a place for expansion of the file.
 *
 * Return:      Success:        TRUE
 *
 *              Failure:        FALSE
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996 11:49 AM EST
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Nov 17 22:27:19 PST 2009
 *   Added support for long long datatype in a manner that matches how
 *   PDB proper does it.
 *-------------------------------------------------------------------------
 */
int
_lite_PD_rd_extras (PDBfile *file) {

   char *token;
   data_alignment *pa = 0;

   _PD_n_casts = 0L;

   file->default_offset = 0;
   file->system_version = 0;
   file->date           = NULL;

   /*
    * Read the default offset.
    */
   while (_PD_get_token(NULL, local, LRG_TXT_BUFFER, '\n')) {
      token = lite_SC_firsttok(local, ":\n");
      if (token == NULL) break;

      /*
       * Get the default offset.
       */
      if (strcmp(token, "Offset") == 0) {
         token = lite_SC_firsttok(local, "\n");
         if (token != NULL) file->default_offset = atoi(token);
         
      } else if (strcmp(token, "Alignment") == 0) {
         token = lite_SC_firsttok(local, "\n");
         if (token != NULL) pa = _lite_PD_mk_alignment(token);
         else return(FALSE);

      } else if (strcmp(token, "Struct-Alignment") == 0) {
         token = lite_SC_firsttok(local, "\n");
         if (token != NULL) pa->struct_alignment = atoi(token);

      } else if (strcmp(token, "Longlong-Format-Alignment") == 0) {
         token = lite_SC_firsttok(local, "\n");
         if (token != NULL) {
             data_standard *ps = file->std;
             ps->longlong_bytes     = token[0];
             ps->longlong_order     = token[1];
             if (pa) pa->longlong_alignment = token[2];
             else return(FALSE);
         }

      } else if (strcmp(token, "Casts") == 0) {
         long n_casts, i;
         char **pl;

         n_casts = N_CASTS_INCR;
         pl      = FMAKE_N(char *, N_CASTS_INCR, "_PD_RD_EXTRAS:cast-list");
         i       = 0L;
         while (_PD_get_token(NULL, local, LRG_TXT_BUFFER, '\n')) {
            if (*local == '\002') break;
            pl[i++] = lite_SC_strsavef(strtok(local, "\001\n"),
                                       "char*:_PD_RD_EXTRAS:local1");
            pl[i++] = lite_SC_strsavef(strtok(NULL, "\001\n"),
                                       "char*:_PD_RD_EXTRAS:local2");
            pl[i++] = lite_SC_strsavef(strtok(NULL, "\001\n"),
                                       "char*:_PD_RD_EXTRAS:local3");
            if (i >= n_casts) {
               n_casts += N_CASTS_INCR;
               REMAKE_N(pl, char *, n_casts);
            }
         }
         _PD_cast_lst = pl;
         _PD_n_casts  = i;

      } else if (strcmp(token, "Blocks") == 0) {
         long j, n, nt, addr, numb, stride;
         char *name;
         symblock *sp;
         syment *ep;
         dimdes *dim;

         while (_PD_get_token(NULL, local, LRG_TXT_BUFFER, '\n')) {
            if (*local == '\002') break;

            name = strtok(local, "\001\n");
            n    = lite_SC_stoi(strtok(NULL, " \n"));
            ep   = lite_PD_inquire_entry(file, name, FALSE, NULL);
            sp   = REMAKE_N(PD_entry_blocks(ep), symblock, n);
            nt   = 0L;
            for (j = 0L; j < n; j++) {
               addr = lite_SC_stoi(strtok(NULL, " \n"));
               numb = lite_SC_stoi(strtok(NULL, " \n"));
               if ((addr == 0L) || (numb == 0L)) {
                  _PD_get_token(NULL, local, LRG_TXT_BUFFER, '\n');
                  addr = lite_SC_stoi(strtok(local, " \n"));
                  numb = lite_SC_stoi(strtok(NULL, " \n"));
               }
                         
               sp[j].diskaddr = addr;
               sp[j].number   = numb;

               nt += numb;
            }

            /*
             * Adjust the slowest varying dimension to reflect the
             * entire entry.
             */
            dim = PD_entry_dimensions(ep);
            if (PD_get_major_order(file) == COLUMN_MAJOR_ORDER) {
               for (/*void*/; dim->next != NULL; dim = dim->next) /*void*/;
            }

            stride = PD_entry_number(ep)/dim->number;
            stride = nt/stride;
            dim->number    = stride;
            dim->index_max = dim->index_min + stride - 1L;

            /*
             * Adjust the number to reflect the entire entry.
             */
            PD_entry_number(ep) = nt;

            PD_entry_blocks(ep) = sp;
         }

      } else if (strcmp(token, "Primitive-Types") == 0) {
         /*
          * Read in the primitives.
          */
         _lite_PD_rd_prim_extras(file, '\001', '\002', NULL);

      } else if (strcmp(token, "Major-Order") == 0) {
         token = lite_SC_firsttok(local, "\n");
         if (token != NULL) file->major_order = atoi(token);

      } else if (strcmp(token, "Has-Directories") == 0) {
         if (lite_SC_stoi(lite_SC_firsttok(local, "\n"))) _PD_has_dirs = TRUE;

      } else if (strcmp(token, "Previous-File") == 0) {
         token = lite_SC_firsttok(local, "\n");
         if (token != NULL)
            file->previous_file = lite_SC_strsavef(token,
                                                   "char*:_PD_RD_EXTRAS:prev");

      } else if (strcmp(token, "Version") == 0) {
         token = lite_SC_firsttok(local, "|");
         if (token != NULL) file->system_version = atoi(token);

         token = lite_SC_firsttok(local, "\n");
         if (token != NULL)
            file->date = lite_SC_strsavef(token,
                                          "char*:_PD_RD_EXTRAS:date");
      }
   }

   /*
    * Set the file->align (if pre-PDB_SYSTEM_VERSION 3 use the default
    * alignment.
    */
   if (pa != NULL) file->align = pa;
   else file->align = _lite_PD_copy_alignment(&lite_DEF_ALIGNMENT);

   /*
    * Release the buffer which held both the symbol table and the extras.
    */
   SFREE(_lite_PD_tbuffer);

   return(TRUE);
}


/*--------------------------------------------------------------------------*/
/*                     PRIMITIVE TYPE I/O ROUTINES                          */
/*--------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_rd_prim_extras
 *
 * Purpose:     Read the primitive types from the extras table.
 *
 * Return:      void
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996  2:49 PM EST
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
_lite_PD_rd_prim_extras (PDBfile *file, int dc, int rec, char *bf) {

   char *token, *type, delim[10];
   int align, flg;
   long i, size;
   int *ordr;
   long *formt;

   sprintf(delim, "%c\n", dc);

   if (bf != NULL) _PD_get_token(bf, local, LRG_TXT_BUFFER, '\n');

   while (_PD_get_token(NULL, local, LRG_TXT_BUFFER, '\n')) {
      if (*local == rec) break;
      type  = lite_SC_strsavef(strtok(local, delim),
                               "char*:_PD_RD_PRIM_EXTRAS:type");
      size  = lite_SC_stol(strtok(NULL, delim));
      align = lite_SC_stol(strtok(NULL, delim));
      flg   = lite_SC_stol(strtok(NULL, delim));
      ordr  = NULL;
      formt = NULL;

      token = strtok(NULL, delim);
      if (strcmp(token, "ORDER") == 0) {
         ordr = FMAKE_N(int, size, "_PD_RD_PRIM_EXTRAS:order");
         for (i=0L; i<size; i++) ordr[i] = lite_SC_stol(strtok(NULL, delim));
      }
                    
      token = strtok(NULL, delim);
      if (strcmp(token, "FLOAT") == 0) {
         formt = FMAKE_N(long, 8, "_PD_RD_PRIM_EXTRAS:format");
         for (i = 0L; i < 8; i++) formt[i] = lite_SC_stol(strtok(NULL, delim));
      } else if (strcmp(token, "NO-CONV") == 0) {
         _lite_PD_defstr(file->host_chart, type, align, size, flg, FALSE,
                         ordr, formt);
      }

      _lite_PD_defstr(file->chart, type, align, size, flg, TRUE, ordr, formt);

      SFREE(type);
   }
}

/*--------------------------------------------------------------------------*/
/*                            SUPPORT ROUTINES                              */
/*--------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_e_install
 *
 * Purpose:     Install a syment in the given hash table.
 *
 * Return:      void
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996  1:50 PM EST
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
_lite_PD_e_install (char *name, syment *entr, HASHTAB *tab) {

   syment *ep;

   /*
    * We can leak a lot of memory if we don't check this!!
    */
   ep = (syment *) lite_SC_def_lookup(name, tab);
   if (ep != NULL) {
      lite_SC_hash_rem(name, tab);
      _lite_PD_rl_syment_d(ep);
   }

   lite_SC_install(name, entr, lite_PD_SYMENT_S, tab);
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_d_install
 *
 * Purpose:     Install a defstr in the given hash table.
 *
 * Return:      void
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996  4:04 PM EST
 *
 * Modifications:
 *    Eric Brugger, Fri Dec  4 17:39:22 PST 1998
 *    I added calls to lite_SC_mark to bump memory reference counts as
 *    appropriate.
 *
 *-------------------------------------------------------------------------
 */
void
_lite_PD_d_install (char *name, defstr *def, HASHTAB *tab) {

   defstr *dp;

   /*
    * We can leak a lot of memory if we don't check this!!
    */
   dp = PD_inquire_table_type(tab, name);
   if (dp != NULL) {
      if (strcmp(name, dp->type) == 0) {
         /*
          * Increase the reference count on dp, since lite_SC_hash_rem
          * will try to delete it.
          */
         lite_SC_mark(dp, 1);
         lite_SC_hash_rem(name, tab);
         _lite_PD_rl_defstr(dp);
      }
   }

   lite_SC_install(name, def, lite_PD_DEFSTR_S, tab);
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_defstr
 *
 * Purpose:     Primitive defstruct used to install the primitive types
 *              in the specified chart.
 *
 * Return:      Success:        
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996  4:05 PM EST
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static defstr *
_lite_PD_defstr (HASHTAB *chart, char *name, int align, long sz, int flg,
                 int conv, int *ordr, long *formt) {

   defstr *dp;

   dp = _lite_PD_mk_defstr(name, NULL, sz, align, flg, conv, ordr, formt);
   _lite_PD_d_install(name, dp, chart);

   return(dp);
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_defstr_inst
 *
 * Purpose:     Install the defstr in both chargs.
 *
 * Return:      Success:        The file defstr if FLAG is TRUE
 *                              The file host_chart defstr if FLAG is FALSE
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996  1:39 PM EST
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
defstr *
_lite_PD_defstr_inst (char *name, memdes *desc, int flg, int *ordr,
                      long *formt, HASHTAB *chrt, HASHTAB *host_chart,
                      data_alignment *align, data_alignment *host_align,
                      int flag) {

   int algn, conv;
   long sz;
   defstr *dp, *Sdp;
   memdes *pd, *memb;

   if (desc == NULL) return(NULL);

   dp = flag ? PD_inquire_table_type(chrt, name) :
      PD_inquire_table_type(host_chart, name);
   if (dp != NULL) return(dp);

   /*
    * Install the type in the file->chart.
    */
   sz   = _lite_PD_str_size(desc, chrt);
   conv = FALSE;
   algn = align->struct_alignment;
   for (pd = desc; pd != NULL; pd = pd->next) {
      dp = PD_inquire_table_type(chrt, pd->base_type);
      if (_lite_PD_indirection(pd->type) || (dp == NULL)) {
         algn = MAX(algn, align->ptr_alignment);
         conv = TRUE;
      } else {
         algn  = MAX(algn, dp->alignment);
         conv |= (dp->convert > 0);
      }

      /*
       * In case we are installing this defstr having read it from
       * another file (as in a copy type operation) redo the cast offsets
       */
      if (pd->cast_memb != NULL)
         pd->cast_offs = _lite_PD_member_location(pd->cast_memb, chrt, dp,
                                                  &memb);
   }

   dp = _lite_PD_mk_defstr(name, desc, sz, algn, flg, conv, ordr, formt);
   _lite_PD_d_install(name, dp, chrt);

   /*
    * Install the type in the host_chart.
    */
   desc = lite_PD_copy_members(desc);
   sz   = _lite_PD_str_size(desc, host_chart);

   algn = host_align->struct_alignment;
   for (pd = desc; pd != NULL; pd = pd->next) {
      dp = PD_inquire_table_type(host_chart, pd->base_type);
      if (_lite_PD_indirection(pd->type) || (dp == NULL)) {
         algn = MAX(algn, host_align->ptr_alignment);
      } else {
         algn = MAX(algn, dp->alignment);
      }

      /*
       * In case we are installing this defstr having read it from
       * another file (as in a copy type operation) redo the cast offsets
       */
      if (pd->cast_memb != NULL) {
         pd->cast_offs =  _lite_PD_member_location(pd->cast_memb,
                                                   host_chart, dp, &memb);
      }
   }

   /*
    * NOTE: ordr, formt, and conv apply only to the file chart
    *       never to the host chart!!!
    *       these are for non-default primitive types which
    *       have no host representation
    */
   Sdp = _lite_PD_mk_defstr(name, desc, sz, algn, -1, FALSE, NULL, NULL);
   _lite_PD_d_install(name, Sdp, host_chart);

   return(flag ? dp : Sdp);
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_rfgets
 *
 * Purpose:     Our fgets looks for specified line separator in addition
 *              to the given system version.  It is also guaranteed to not
 *              split tokens in the input stream.
 *
 * Return:      same as _PD_get_tok
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996 12:02 PM EST
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
char *
_lite_PD_rfgets(char *s, int n, FILE *fp) {

   return _PD_get_tok(s, n, fp, '\n') ;
}

/*-------------------------------------------------------------------------
 * Function:    _PD_get_tok
 *
 * Purpose:     An fgets clone looks for specified char in addition to
 *              the newline.  It is also guaranteed to not split tokens
 *              in the input stream.
 *
 * Return:      Success:        
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996  1:55 PM EST
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static char *
_PD_get_tok (char *s, int n, FILE *fp, int ch) {

   int i, c, LineSep;
   long loc, nb;
   char *ps;

   /*
    * This is for old NLTSS generated files - sigh!
    */
   LineSep = 0x1f;
    
   /*
    * Find the current location and remember it.
    */
   loc = io_tell(fp);
   nb  = io_read(s, (size_t) 1, (size_t) n, fp);
   ps  = s;

   /*
    * Check for EOF and reset the file pointer if at EOF.
    */
   if (((c = *(ps++)) == EOF) || (nb == 0)) {
      io_seek(fp, loc, SEEK_SET);
      *s = '\0';
      return(NULL);
   }
   ps--;

   /*
    * Search for \n, EOF, LineSep, or given delimiter.
    */
   n = nb - 1;
   for (i = 0; i < n; i++) {
      c = *(ps++);
      if ((c == '\n') || (c == LineSep) || (c == ch)) {
         ps--;
         *ps++ = '\0';
         loc += (long) (ps - s);
         break;
      } else if (c == EOF) {
         ps--;
         *ps++ = '\0';
         loc += (long) (ps - s + 1);
         break;
      }
   }

   /*
    * If we got a full buffer backup to the last space so as to not split
    * a token.
    */
   if ((i >= n) && (c != '\n') && (c != LineSep) && (c == ch)) {
      ps--;
      n >>= 1;
      for (; i > n; i--) {
         c = *(--ps);
         loc--;
         if ((c == '\t') || (c == ' ')) {
            *ps = '\0';
            break;
         }
      }
   }

   /*
    * Reset the file pointer to the end of the string.
    */
   io_seek(fp, loc, SEEK_SET);

   return(s);
}

/*-------------------------------------------------------------------------
 * Function:    _PD_get_token
 *
 * Purpose:     Get a token from a buffer which is handled like strtok().
 *              Each time BF is non-NULL it is remembered for the next call.
 *              The return buffer is handled like fgets with the spce
 *              and maximum size being passed in.  This function is
 *              guaranteed to not split tokens in the input buffer.
 *
 * Return:      Success:        
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996  1:57 PM EST
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static char *
_PD_get_token (char *bf, char *s, int n, int ch) {

   int i, c;
   char *ps;
   static char *t = NULL;

   /*
    * This is for old NLTSS generated files - sigh!
    */
   static int LineSep = 0x1f;

   if (bf != NULL) t = bf;

   ps = s;

   /*
    * Check for EOF and reset the file pointer if at EOF.
    */
   c = *ps++ = *t++;
   if ((c == EOF) || (n == 0)) {
      t--;
      *s = '\0';
      return(NULL);
   }
   ps--;
   t--;

   /*
    * Search for \n, EOF, LineSep, or given delimiter.
    */
   n--;
   for (i = 0; i < n; i++) {
      c = *ps++ = *t++;
      if ((c == '\n') || (c == LineSep) || (c == ch)) {
         ps--;
         *ps++ = '\0';
         break;
      } else if (c == EOF) {
         ps--;
         t--;
         *ps++ = '\0';
         break;
      }
   }

   /*
    * If we got a full buffer backup to the last space so as to not split
    * a token
    */
   if ((i >= n) && (c != '\n') && (c != LineSep) && (c == ch)) {
      ps--;
      t--;
      n >>= 1;
      for (; i > n; i--) {
         c = *(--ps) = *(--t);
         if ((c == '\t') || (c == ' ')) {
            *ps = '\0';
            break;
         }
      }
   }

   return(s);
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_init_chrt
 *
 * Purpose:     Initialize the charts with the primitives.
 *
 * Note:        Define both int and integer!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * Return:      void
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996 11:41 AM EST
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
_lite_PD_init_chrt (PDBfile *file) {

   HASHTAB        *fchrt, *hchrt;
   defstr         *ret;
   data_standard  *fstd, *hstd;
   data_alignment *falign, *halign;

   /*
    * If the first time initialize some PDBLib stuff.
    */
   if (lite_PD_DEFSTR_S == NULL) {
      lite_LAST        = FMAKE(int, "_PD_INIT_CHART:LAST");
      *lite_LAST       = 0;

      if (lite_io_close_hook == (PFfclose) fclose)
         lite_io_close_hook = (PFfclose) _lite_PD_pio_close;

      if (lite_io_seek_hook == (PFfseek) fseek)
         lite_io_seek_hook = (PFfseek) _lite_PD_pio_seek;

      if (lite_io_printf_hook == (PFfprintf) fprintf)
         lite_io_printf_hook = (PFfprintf) _lite_PD_pio_printf;

      lite_PD_DEFSTR_S = lite_SC_strsavef("defstr *",
                                          "char*:_PD_INT_CHRT:defstr");
      lite_PD_SYMENT_S = lite_SC_strsavef("syment *",
                                          "char*:_PD_INIT_CHRT:syment");
   }

   fchrt  = file->chart;
   fstd   = file->std;
   falign = file->align;

   hchrt  = file->host_chart;
   hstd   = file->host_std;
   halign = file->host_align;

   _lite_PD_setup_chart(fchrt, fstd, hstd, falign, halign, TRUE);
   _lite_PD_setup_chart(hchrt, hstd, NULL, halign, NULL, FALSE);

   if (sizeof(double) == sizeof(double)) {
      lite_PD_typedef(file, "double", "REAL");
   } else {
      lite_PD_typedef(file, "float", "REAL");
   }

   ret  = PD_inquire_host_type(file, "*");
   lite_PD_defncv(file, "function", ret->size, ret->alignment);

   return;
}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_setup_chart
 *
 * Purpose:     Setup a structure and chart with the conversions selected
 *              and the types installed.
 *
 * Return:      void
 *
 * Programmer:  Adapted from PACT PDB
 *              Mar  5, 1996  3:21 PM EST
 *
 * Modifications:
 *    Eric Brugger, Tue Dec  8 15:53:07 PST 1998
 *    I added calls to lite_SC_mark to bump memory reference counts as
 *    appropriate.
 *
 *   Mark C. Miller, Fri Nov 13 15:33:42 PST 2009
 *   Added support for long long datatype.
 *
 *   Mark C. Miller, Tue Nov 17 22:23:42 PST 2009
 *   Changed support for long long to match more closely what PDB
 *   proper does.
 *
 *   Mark C. Miller, Fri May 11 15:59:35 PDT 2018
 *   Added "char *" entry per Dale Slone.
 *-------------------------------------------------------------------------
 */
void
_lite_PD_setup_chart (HASHTAB *chart, data_standard *fstd, data_standard *hstd,
                      data_alignment *falign, data_alignment *halign,
                      int flag) {

   int conv;

   if (flag) {
      PD_COMPARE_PTR_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "*", falign->ptr_alignment,
                   (long) fstd->ptr_bytes, -1, conv, NULL, NULL);

   if (flag) {
      PD_COMPARE_CHAR_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "char", falign->char_alignment,
                   1L, -1, conv, NULL, NULL);

   if (flag) {
      PD_COMPARE_PTR_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "char *", falign->ptr_alignment,
                   (long) fstd->ptr_bytes, -1, conv, NULL, NULL);

   if (flag) {
      PD_COMPARE_SHORT_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "short", falign->short_alignment,
                   (long) fstd->short_bytes, fstd->short_order,
                   conv, NULL, NULL);

   if (flag) {
      PD_COMPARE_INT_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "int", falign->int_alignment,
                   (long) fstd->int_bytes, fstd->int_order,
                   conv, NULL, NULL);

   if (flag) {
      PD_COMPARE_INT_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "integer", falign->int_alignment,
                   (long) fstd->int_bytes, fstd->int_order,
                   conv, NULL, NULL);

   if (flag) {
      PD_COMPARE_LONG_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "long", falign->long_alignment,
                   (long) fstd->long_bytes, fstd->long_order,
                   conv, NULL, NULL);

   if (flag) {
      PD_COMPARE_LONGLONG_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "long_long", falign->longlong_alignment,
                   (long) fstd->longlong_bytes, fstd->longlong_order,
                   conv, NULL, NULL);
   _lite_PD_defstr(chart, "u_long_long", falign->longlong_alignment,
                   (long) fstd->longlong_bytes, fstd->longlong_order,
                   conv, NULL, NULL);

   if (flag) {
      PD_COMPARE_FLT_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "float", falign->float_alignment,
                   (long) fstd->float_bytes, -1, conv, fstd->float_order,
                   fstd->float_format);
   lite_SC_mark(fstd->float_order, 1);
   lite_SC_mark(fstd->float_format, 1);

   if (flag) {
      PD_COMPARE_DBL_STD(conv, fstd, hstd, falign, halign);
   } else {
      conv = FALSE;
   }
   _lite_PD_defstr(chart, "double", falign->double_alignment,
                   (long) fstd->double_bytes, -1, conv, fstd->double_order,
                   fstd->double_format);
   lite_SC_mark(fstd->double_order, 1);
   lite_SC_mark(fstd->double_format, 1);

}

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_add_block
 *
 * Purpose:     Add a new block to an entry.  This does nothing to the file,
 *              only the syment.  The file extension operations are left to
 *              _PD_write or to _PD_defent.
 *
 * Return:      Success:        
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT
 *              Apr 17, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
int
_lite_PD_add_block (PDBfile *file, syment *ep, dimdes *dims) {

   symblock *sp;
   int n;
   long addr, bytespitem;

   if (!_PD_consistent_dims(file, ep, dims)) {
      lite_PD_error("INCONSISTENT DIMENSION CHANGE - _PD_ADD_BLOCK", PD_WRITE);
   }

   addr = file->chrtaddr;

   sp = PD_entry_blocks(ep);
   n  = PD_n_blocks(ep);
   REMAKE_N(sp, symblock, n+1);

   sp[n].diskaddr = addr;
   sp[n].number   = _lite_PD_comp_num(dims);

   PD_entry_blocks(ep) = sp;
   bytespitem = _lite_PD_lookup_size(PD_entry_type(ep), file->chart);

   /*
    * We are through with the dimensions
    * their information has been moved into the entry dimensions
    */
/*#warning REMOVED AS PER pdbtst.c FAILING*/
#if 0
   _lite_PD_rl_dimensions(dims);
#endif

   return(_lite_PD_extend_file(file, sp[n].number*bytespitem));
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_wr_format
 *
 * Purpose:     Write the primitive data format information to the file
 *              header block
 *
 *              - Floating Point Format Descriptor                       
 *              -   format[0] = # of bits per number                     
 *              -   format[1] = # of bits in exponent                    
 *              -   format[2] = # of bits in mantissa                    
 *              -   format[3] = start bit of sign                        
 *              -   format[4] = start bit of exponent                    
 *              -   format[5] = start bit of mantissa                    
 *              -   format[6] = high order mantissa bit (CRAY needs this)
 *              -   format[7] = bias of exponent                         
 *
 * Return:      Success:        TRUE
 *
 *              Failure:        FALSE
 *
 * Programmer:  Adapted from PACT
 *              Apr 17, 1996
 *
 * Modifications:
 *
 *   Mark C. Miller, Fri Nov 13 15:33:42 PST 2009
 *   Added support for long long datatype.
 *
 *   Mark C. Miller, Tue Nov 17 22:28:13 PST 2009
 *   Moved support for long long from here to 'extras'.
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
int
_lite_PD_wr_format (PDBfile *file) {

   int j, n, *order;
   long *format, float_bias, double_bias;
   char outfor[MAXLINE], *p;
   data_standard *std;

   p   = outfor + 1;
   std = file->std;

   /*
    * Get the byte lengths in.
    */
   *(p++) = std->ptr_bytes;
   *(p++) = std->short_bytes;
   *(p++) = std->int_bytes;
   *(p++) = std->long_bytes;
   *(p++) = std->float_bytes;
   *(p++) = std->double_bytes;

   /*
    * Get the integral types byte order in.
    */
   *(p++) = std->short_order;
   *(p++) = std->int_order;
   *(p++) = std->long_order;

   /*
    * Get the float byte order in.
    */
   order = std->float_order;
   n     = std->float_bytes;
   for (j = 0; j < n; j++, *(p++) = *(order++)) /*void*/ ;

   /*
    * Get the double byte order in.
    */
   order = std->double_order;
   n     = std->double_bytes;
   for (j = 0; j < n; j++, *(p++) = *(order++)) /*void*/ ;

   /*
    * Get the float format data in.
    */
   format = std->float_format;
   n = lite_FORMAT_FIELDS - 1;
   for (j = 0; j < n; j++, *(p++) = *(format++)) /*void*/ ;

   /*
    * Get the float bias in.
    */
   float_bias = *format;

   /*
    * Get the double format data in.
    */
   format = std->double_format;
   n      = lite_FORMAT_FIELDS - 1;
   for (j = 0; j < n; j++, *(p++) = *(format++)) /*void*/ ;

   /*
    * Get the double bias in.
    */
   double_bias = *format;

   n         = (int) (p - outfor);
   outfor[0] = n;

   if (io_write(outfor, (size_t) 1, (size_t) n, file->stream) != n) {
      lite_PD_error("FAILED TO WRITE FORMAT DATA - _PD_WR_FORMAT", PD_CREATE);
   }
    
   /*
    * Write out the biases.
    */
   sprintf(outfor, "%ld\001%ld\001\n", float_bias, double_bias);
   n = strlen(outfor);
   if (io_write(outfor, (size_t) 1, (size_t) n, file->stream) != n) {
      lite_PD_error("FAILED TO WRITE BIASES - _PD_WR_FORMAT", PD_CREATE);
   }
    
   return(TRUE);
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _PD_consistent_dims
 *
 * Purpose:     Check two sets of dimensions for consistency of the non-
 *              changing sector and proper updating of the chaning dimension.
 *
 * Return:      Success:        
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT
 *              Apr 17, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
static int
_PD_consistent_dims (PDBfile *file, syment *ep, dimdes *ndims) {

   dimdes *odims, *od, *nd;

   odims = PD_entry_dimensions(ep);

   /*
    * Check the dimensions for consistency.
    */
   if (file->major_order == COLUMN_MAJOR_ORDER) {
      for (od = odims, nd = ndims;
           (od != NULL) && (nd != NULL) && (nd->next != NULL);
           od = od->next, nd = nd->next) {
         if ((od->index_min != nd->index_min) ||
             (od->index_max != nd->index_max) ||
             (od->number != nd->number)) {
            return(FALSE);
         }
      }
   } else if (file->major_order == ROW_MAJOR_ORDER) {
      for (od = odims->next, nd = ndims->next;
           (od != NULL) && (nd != NULL);
           od = od->next, nd = nd->next) {
         if ((od->index_min != nd->index_min) ||
             (od->index_max != nd->index_max) ||
             (od->number != nd->number)) {
            return(FALSE);
         }
      }

      nd = ndims;
      od = odims;
   }
   else {
      return(FALSE);
   }

   if (nd->index_min == file->default_offset) {
      od->index_max += nd->index_max - nd->index_min + 1;
   } else if (nd->index_min == od->index_max + 1L) {
      od->index_max = nd->index_max;
   } else {
      return(FALSE);
   }

   od->number = od->index_max - od->index_min + 1L;

   PD_entry_number(ep) = _lite_PD_comp_num(odims);

   return(TRUE);
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_extend_file
 *
 * Purpose:     Extend the file by the given numer of bytes.
 *
 * Return:      Success:        
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT
 *              Apr 17, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
int
_lite_PD_extend_file (PDBfile *file, long nb) {

   FILE *fp;
   long addr;
   char bf[1];

   fp   = file->stream;
   addr = file->chrtaddr + nb;

   /*
    * Expand the file or the space will not be there.
    */
   if (io_seek(fp, addr, SEEK_SET)) {
      sprintf(lite_PD_err, "ERROR: FSEEK FAILED - _PD_EXTEND_FILE");
      return(FALSE);
   }

   bf[0] = ' ';
   nb    = io_write(bf, (size_t) 1, (size_t) 1, fp);
   if (nb != 1L) {
      sprintf(lite_PD_err, "ERROR: CAN'T SET FILE SIZE - _PD_EXTEND_FILE");
      return(FALSE);
   }

   file->chrtaddr = addr;

   return(TRUE);
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_eod
 *
 * Purpose:     Mark the end of data in the file that is set the
 *              chart address to the current file position.
 *
 * Return:      Success:        
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT
 *              Apr 17, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
void
_lite_PD_eod (PDBfile *file) {

   FILE *fp;
   long old_one, new_one;

   fp  = file->stream;
   old_one = file->chrtaddr;
   new_one = io_tell(fp);

   file->chrtaddr = MAX(old_one, new_one);
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_wr_chrt
 *
 * Purpose:     Write out the structure chart into the PDB file.
 *
 * Return:      Success:        Disk address
 *
 *              Failure:        -1
 *
 * Programmer:  Adapted from PACT
 *              Apr 18, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
long
_lite_PD_wr_chrt (PDBfile *file) {

   int n;
   long addr;
   FILE *fp;
   hashel *hp;
   defstr *dp;
   memdes *desc;

   fp   = file->stream;
   addr = io_tell(fp);
   if (addr == -1L) return(-1L);

   if (_lite_PD_tbuffer != NULL) SFREE(_lite_PD_tbuffer);

   _lite_PD_rev_chrt(file);

   /*
    * The hash array for the structure chart is one element long.
    */
   n = 0;
   for (hp = file->chart->table[0]; hp != NULL; hp = hp->next) {
      dp = (defstr *) (hp->def);

      /*
       * Use hp->name instead of dp->type or PD_typedef's will not work.
       */
      _PD_put_string(n++, "%s\001%ld\001", hp->name, dp->size);

      for (desc = dp->members; desc != NULL; desc = desc->next) {
         _PD_put_string(n++, "%s\001", desc->member);
      }

      _PD_put_string(n++, "\n");
   }

   _PD_put_string(n++, "\002\n");

   /*
    * Restore the chart because this may be a PD_flush and more types
    * may be added later
    */
   _lite_PD_rev_chrt(file);

   /*
    * Write the entire chart to the file now.
    */
   io_write(_lite_PD_tbuffer, 1, strlen(_lite_PD_tbuffer), fp);
   io_flush(fp);
   SFREE(_lite_PD_tbuffer);

   return(addr);
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_wr_symt
 *
 * Purpose:     Write out the symbol table (a hash table) into the pdb
 *              file.
 *
 * Return:      Success:        disk address
 *
 *              Failure:        -1
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.matzke.cioe.com
 *              Apr 18, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
long
_lite_PD_wr_symt (PDBfile *file) {

   long addr, nt, ne, nb, stride;
   int i, n, size, flag;
   FILE *fp;
   hashel **s_tab, *hp;
   syment *ep;
   dimdes *lst;

   fp   = file->stream;
   addr = io_tell(fp);
   if (addr == -1L) return(-1L);

   if (_lite_PD_tbuffer != NULL) SFREE(_lite_PD_tbuffer);

   n     = 0;
   s_tab = file->symtab->table;
   size  = file->symtab->size;
   for (i = 0; i < size; i++) {
      for (hp = s_tab[i]; hp != NULL; hp = hp->next) {
         ep = (syment *) (hp->def);
         nt = PD_entry_number(ep);
         nb = PD_block_number(ep, 0);
         if (nb == 0) {
            if (PD_n_blocks(ep) == 1) {
               nb = nt;
            } else {
               sprintf(lite_PD_err, "ERROR: BAD BLOCK LIST - _PD_WR_SYMT\n");
               return(-1L);
            }
         }

         _PD_put_string(n++, "%s\001%s\001%ld\001%ld\001",
                        hp->name, PD_entry_type(ep), nb,
                        PD_entry_address(ep));

         /*
          * Adjust the slowest varying dimension to reflect only
          * the first block.
          */
         flag = PD_get_major_order(file);
         for (lst = PD_entry_dimensions(ep); lst != NULL; lst = lst->next) {
            if ((flag == ROW_MAJOR_ORDER) ||
                ((flag == COLUMN_MAJOR_ORDER) && (lst->next == NULL))) {
               stride = lst->number?nt/(lst->number):0;
               ne     = stride?nb/stride:0;
               flag   = FALSE;
            } else {
               ne = lst->number;
            }

            _PD_put_string(n++, "%ld\001%ld\001", lst->index_min, ne);
         }

         _PD_put_string(n++, "\n");
      }
   }

   /*
    * Pad an extra newline to mark the end of the
    * symbol table for _PD_rd_symt.
    */
   _PD_put_string(n++, "\n");

   return(addr);
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_wr_extras
 *
 * Purpose:     Write any extraneous data to the file.  This is essentially
 *              a place for expansion of the file. To complete the definition
 *              of a pdb file the following rule applies to extensions in the
 *              extras table.
 *
 *                  An extension shall have one of two formats:         
 *                                                                      
 *                     1) <name>:<text>\n                               
 *                                                                      
 *                     2) <name>:\n                                     
 *                        <text1>\n                                     
 *                        <text2>\n                                     
 *                           .                                          
 *                           .                                          
 *                           .                                          
 *                        <textn>\n                                     
 *                        [^B]\n                                        
 *                                                                      
 *              anything else is strictly illegal!!!!!!             
 *              NOTE: the optional ^B is for backward compatibility 
 *              and is not recommmended.
 *                  
 * Return:      Success:        
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT
 *              Apr 18, 1996
 *
 * Modifications:
 *
 *   Mark C. Miller, Fri Nov 13 15:33:42 PST 2009
 *   Added support for long long datatype.
 *
 *   Mark C. Miller, Tue Nov 17 22:23:42 PST 2009
 *   Changed support for long long to match more closely what PDB
 *   proper does.
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
int
_lite_PD_wr_extras (PDBfile *file) {

   FILE *fp;
   data_alignment *pa;
   data_standard *ps;
   char al[MAXLINE];
   int has_dirs;

   fp = file->stream;

   /*
    * Write the default offset.
    */
   _PD_put_string(1, "Offset:%d\n", file->default_offset);

   /*
    * Write the alignment data.
    */
   pa    = file->align;
   al[0] = pa->char_alignment;
   al[1] = pa->ptr_alignment;
   al[2] = pa->short_alignment;
   al[3] = pa->int_alignment;
   al[4] = pa->long_alignment;
   al[5] = pa->float_alignment;
   al[6] = pa->double_alignment;
   al[7] = '\0';

   if (al[0]*al[1]*al[3]*al[4]*al[5]*al[6] == 0)
      return(FALSE);

   _PD_put_string(1, "Alignment:%s\n", al);
   _PD_put_string(1, "Struct-Alignment:%d\n",
                  file->align->struct_alignment);

    /*
     * write out the long long standard and alignment
     */
   ps = file->std;
   al[0] = ps->longlong_bytes;
   al[1] = ps->longlong_order;
   al[2] = pa->longlong_alignment;
   al[3] = '\0';

   _PD_put_string(1, "Longlong-Format-Alignment:%s\n", al);

   /*
    * Write out the date and version data.
    */
   _PD_put_string(1, "Version:%d|%s\n",
                  file->system_version, file->date);

   /*
    * Write out the casts.
    */
   {
      hashel *hp;
      defstr *dp;
      memdes *desc;

      _PD_put_string(1, "Casts:\n");
      for (hp = *(file->host_chart->table); hp != NULL; hp = hp->next) {
         dp = (defstr *) hp->def;
         for (desc = dp->members; desc != NULL; desc = desc->next) {
            if (desc->cast_memb != NULL) {
               _PD_put_string(1, "%s\001%s\001%s\001\n",
                              dp->type, desc->member, desc->cast_memb);
            }
         }
      }
      _PD_put_string(1, "\002\n");
   }

   /*
    * Write out the major order.
    */
   _PD_put_string(1, "Major-Order:%d\n", file->major_order);

   /*
    * Write out the previous file name (family).
    */
   if (file->previous_file != NULL) {
      _PD_put_string(1, "Previous-File:%s\n", file->previous_file);
   }

   /*
    * Write out the directory indicator flag.
    */
   has_dirs = PD_has_directories(file);
   _PD_put_string(1, "Has-Directories:%d\n", has_dirs);

   /*
    * Write out the primitives.
    */
   _lite_PD_rev_chrt(file);

   _lite_PD_wr_prim_extras(fp, file->chart, '\001', '\002');

   _lite_PD_rev_chrt(file);

   /*
    * Write out the blocks - this MUST follow at least the Major-Order extra
    * or else the read may improperly reconstruct the dimensions.
    */
   {
      long i, j, n, sz;
      syment *ep;
      hashel *hp, **tb;
      HASHTAB *tab;

      _PD_put_string(1, "Blocks:\n");
      tab = file->symtab;
      sz  = tab->size;
      tb  = tab->table;
      for (i = 0L; i < sz; i++) {
         for (hp = tb[i]; hp != NULL; hp = hp->next) {
            ep = (syment *) hp->def;
            n  = PD_n_blocks(ep);
            if (n > 1) {
               symblock *sp;

               sp = PD_entry_blocks(ep);
               _PD_put_string(1, "%s\001%ld", hp->name, n);
               for (j = 0L; j < n; j++) {
                  if ((j > 0) && ((j % 50) == 0)) {
                     _PD_put_string(1, "\n");
                  }
                  _PD_put_string(1, " %ld %ld", sp[j].diskaddr, sp[j].number);
               }
               _PD_put_string(1, "\n");
            }
         }
      }
      _PD_put_string(1, "\002\n");
   }

   /*
    * Pad the end of the file with some newlines to smooth over the
    * end of binary file problems on different (ie CRAY) systems.
    */
   _PD_put_string(1, "\n\n");

   /*
    * Write the symbol table and the extras table to the file now.
    */
   io_write(_lite_PD_tbuffer, 1, strlen(_lite_PD_tbuffer), fp);
   io_flush(fp);
   SFREE(_lite_PD_tbuffer);

   return(TRUE);
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_rev_chrt
 *
 * Purpose:     Reverse the list of structures in the single element hash
 *              table that makes up the chart.
 *
 * Return:      Success:        TRUE
 *
 *              Failure:        never fails
 *
 * Programmer:  Adapted from PACT
 *              Apr 18, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
int
_lite_PD_rev_chrt (PDBfile *file) {

   PD_REVERSE_LIST(hashel, *(file->chart->table), next);
   return(TRUE);
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _PD_put_string
 *
 * Purpose:     Build up the contents of the current output buffer
 *              in an fprintf style.
 *
 * Return:      Success:        TRUE
 *
 *              Failure:        
 *
 * Programmer:  Adapted from PACT
 *              Apr 18, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
static int
_PD_put_string (int reset, char *fmt, ...) {

   long ns;
   char s[MAXLINE];
   static char *pl = NULL;
   static long ncx = 0L, nc = 0L;
   va_list ap ;

   va_start(ap,fmt);
   vsprintf(s, fmt, ap);
   va_end(ap);

   ns = strlen(s);

   if (_lite_PD_tbuffer == NULL) {
      ncx = BUFINCR;
      _lite_PD_tbuffer = MAKE_N(char, ncx);
      pl = _lite_PD_tbuffer;
      nc = 0;
   } else if (!reset) {
      pl = _lite_PD_tbuffer;
      nc = 0;
      memset(_lite_PD_tbuffer, 0, ncx);
   }
    
   if (nc + ns >= ncx) {
      ncx += BUFINCR;
      REMAKE_N(_lite_PD_tbuffer, char, ncx);
      pl = _lite_PD_tbuffer + strlen(_lite_PD_tbuffer);
   }

   strcpy(pl, s);
   pl += ns;
   nc += ns;

   return(TRUE);
}
#endif /* PDB_WRITE */

/*-------------------------------------------------------------------------
 * Function:    _lite_PD_wr_prim_extras
 *
 * Purpose:     Write the primitive data types from the given structure
 *              chart in a form suitable to the extras table.
 *
 * Return:      void
 *
 * Programmer:  Adapted from PACT
 *              Apr 18, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef PDB_WRITE
/* ARGSUSED */
void
_lite_PD_wr_prim_extras (FILE *fp, HASHTAB *tab, int dc, int rec) {

   long i, n;
   hashel *hp, **tb;
   defstr *dp;
   long *formt;
   int *ordr;

   _PD_put_string(1, "Primitive-Types:\n");

   tb = tab->table;
   for (hp = *tb; hp != NULL; hp = hp->next) {
      dp = (defstr *) hp->def;
      if (dp->members != NULL) continue;

      /*
       * Use hp->name instead of dp->type or PD_typedef's won't work!!!
       */
      _PD_put_string(1, "%s%c%ld%c%d%c%d%c",
                     hp->name, dc,
                     dp->size, dc,
                     dp->alignment, dc,
                     dp->order_flag, dc);

      /*
       * Write the byte order.
       */
      ordr = dp->order;
      if (ordr !=  NULL) {
         _PD_put_string(1, "ORDER%c", dc);
         n = dp->size;
         for (i = 0L; i < n; i++) {
            _PD_put_string(1, "%d%c", ordr[i], dc);
         }
      } else {
         _PD_put_string(1, "DEFORDER%c", dc);
      }

      /*
       * Write the floating point format.
       */
      formt = dp->format;
      if (formt != NULL) {
         _PD_put_string(1, "FLOAT%c", dc);
         for (i = 0L; i < 8; i++) {
            _PD_put_string(1, "%ld%c", formt[i], dc);
         }
      } else if (dp->order_flag == -1) {
         _PD_put_string(1, "NO-CONV%c", dc);
      } else {
         _PD_put_string(1, "FIX%c", dc);
      }
      _PD_put_string(1, "\n");
   }

   _PD_put_string(1, "%c\n", rec);
}
#endif /* PDB_WRITE */
