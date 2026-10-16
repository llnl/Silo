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
/*-------------------------------------------------------------------------
 *
 * Created:             num.c
 *                      Dec  4 1996
 *                      Robb Matzke <matzke@viper.llnl.gov>
 *
 * Purpose:             Number functions.
 *
 * Modifications:       
 *
 *-------------------------------------------------------------------------
 */
#include <assert.h>
#include <browser.h>
#include <ctype.h>
#include <math.h>

#define MYCLASS(X)      ((obj_num_t*)(X))

typedef struct obj_num_t {
   obj_pub_t    pub;
   int          fp;
   union {
      int       i;
      double    d;
   } u;
} obj_num_t;

class_t         C_NUM;
static obj_t    num_new (va_list);
static void     num_print (obj_t, out_t*);
static int      num_diff (obj_t, obj_t);


/*-------------------------------------------------------------------------
 * Function:    num_class
 *
 * Purpose:     Initializes the number class.
 *
 * Return:      Success:        Ptr to the num class.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *    Lisa J. Roberts, Mon Nov 22 17:27:53 PST 1999
 *    I changed strdup to safe_strdup.
 *
 *-------------------------------------------------------------------------
 */
class_t
num_class (void) {

   class_t      cls = (class_t)calloc (1, sizeof(*cls));

   cls->name = safe_strdup ("NUM");
   cls->newobj = num_new;
   cls->dest = NULL;
   cls->copy = NULL;
   cls->print = num_print;
   cls->diff = num_diff;
   return cls;
}


/*-------------------------------------------------------------------------
 * Function:    num_new
 *
 * Purpose:     Creates a new number object from a string.  The string can
 *              have leading and trailing space.  Strings that start with
 *              `0x' are hexadecimal and strings starting with `0' are
 *              octal.
 *
 * Return:      Success:        Ptr to new number object.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static obj_t
num_new (va_list ap) {

   obj_num_t    *self = (obj_num_t *)calloc (1, sizeof(obj_num_t));
   char         *lexeme;

   lexeme = va_arg (ap, char*);
   if (strchr(lexeme, '.') ||
       strchr(lexeme, 'e') ||
       strchr(lexeme, 'E')) {
      self->fp = true;
      self->u.d = strtod (lexeme, NULL);
   } else {
      self->fp = false;
      self->u.i = strtol (lexeme, NULL, 0);
   }
   return (obj_t)self;
}


/*-------------------------------------------------------------------------
 * Function:    num_print
 *
 * Purpose:     Prints a number.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *              Robb Matzke, 2000-06-28
 *              Uses the $fmt_int or $fmt_double format.
 *
 *              Robb Matzke, 2000-10-23
 *              Looks at $obase.
 *-------------------------------------------------------------------------
 */
static void
num_print(obj_t _self, out_t *f)
{
    obj_num_t           *self=MYCLASS(_self);
    int                 obase=sym_bi_true("obase");
    unsigned char       *mem=NULL;
    int                 i, j;
    char                buf[512];
    unsigned            u, mask, nbits;

    if (self->fp) {
        mem = (unsigned char*)&(self->u.d);
        if (16==obase) {
            for (i=0; i<sizeof(double); i++) {
                sprintf(buf+2*i, "%02x", *(mem+i));
            }
            out_puts(f, buf);
        } else if (8==obase) {
            prim_octal(buf, mem, sizeof(double));
            out_puts(f, buf);
        } else if (2==obase) {
            for (i=0; i<sizeof(double); i++) {
                u = *((unsigned char*)mem+i);
                for (j=0, mask=0x80; j<8; j++, mask>>=1) {
                    sprintf(buf+i*8+j, "%c", u&mask?'1':'0');
                }
            }
            out_puts(f, buf);
        } else {
            char *fmt = sym_bi_gets("fmt_double");
            if (fmt && *fmt)
                out_printf(f, fmt, self->u.d);
            else
                out_printf(f, "%g", self->u.d);
            free(fmt);
        }
    } else {

        mem = (unsigned char*)&(self->u.i);
        u = *((unsigned*)mem);
        nbits = 8*sizeof(self->u.i);

        if (16==obase) {
            out_printf(f, "%0*x", 2*sizeof(int), (unsigned)(self->u.i));
        } else if (8==obase) {
            out_printf(f, "%0*o", (nbits+2)/3, (unsigned)(self->u.i));
        } else if (2==obase) {
            for (i=0; i<sizeof(self->u.i); i++) {
                u = *((unsigned char*)mem+i);
                for (j=0, mask=0x80; j<8; j++, mask>>=1) {
                    sprintf(buf+i*8+j, "%c", u&mask?'1':'0');
                }
            }
            out_puts(f, buf);
        } else {
            char *fmt = sym_bi_gets("fmt_int");
            if (fmt && *fmt)
                out_printf(f, fmt, self->u.i);
            else
                out_printf(f, "%d", self->u.i);
            free(fmt);
        }
    }
}


/*-------------------------------------------------------------------------
 * Function:    num_diff
 *
 * Purpose:     Determines if two numbers are the same or different.
 *
 * Return:      Success:
 *                 0:   Numbers are the same (or close enough)
 *                 1:   Numbers are different and the difference was printed.
 *                 2:   Numbers are sufficiently different.
 *
 *              Failure:
 *                 -1:  Some type of failure.
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 21 1997
 *
 * Modifications:
 *      Robb Matzke, 7 Feb 1997
 *      If both numbers are integers then we use the integer differencing
 *      tolerances.
 *
 *      Robb Matzke, 2000-06-28
 *      Honors the DiffOpt settings. Supports two-column output.
 *
 *  Mark C. Miller, Wed Nov 11 22:18:17 PST 2009
 *  Added suppot for alternate relative diff option using epsilon param.
 *-------------------------------------------------------------------------
 */
static int
num_diff (obj_t _a, obj_t _b) {

   obj_num_t    *a = MYCLASS(_a);
   obj_num_t    *b = MYCLASS(_b);
   double       ad, bd, abs, rel, eps;
   int          status;
   out_t        *f = OUT_STDOUT;

   if (a->fp || b->fp) {
      ad = a->fp ? a->u.d : (double)(a->u.i);
      bd = b->fp ? b->u.d : (double)(b->u.i);
      abs = DiffOpt.d_abs;
      rel = DiffOpt.d_rel;
      eps = DiffOpt.d_eps;
   } else {
      ad = a->u.i;
      bd = b->u.i;
      abs = DiffOpt.i_abs;
      rel = DiffOpt.i_rel;
      eps = DiffOpt.i_eps;
   }

   status = different (ad, bd, abs, rel, eps) ? 2 : 0;
   if (status>0 && DIFF_REP_ALL==DiffOpt.report && DiffOpt.two_column) {
       obj_print(_a, f);
       out_column(f, OUT_COL2, DIFF_SEPARATOR);
       obj_print(_b, f);
       out_nl(f);
       status = 1;
   }
   return status;
}


/*-------------------------------------------------------------------------
 * Function:    num_int
 *
 * Purpose:     Returns the integer value of SELF
 *
 * Return:      Success:        An integer value.
 *
 *              Failure:        Floating point values are truncated.
 *                              If SELF is not a number then -1 is returned.
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      It is safe to call this function for any type of object.
 *
 *-------------------------------------------------------------------------
 */
int
num_int (obj_t _self) {

   int          retval;
   obj_num_t    *self = MYCLASS(_self);

   if (!self || C_NUM!=self->pub.cls) return -1;
   if (self->fp) retval = (int)(self->u.d);
   else retval = self->u.i;
   return retval;
}


/*-------------------------------------------------------------------------
 * Function:    num_isint
 *
 * Purpose:     Returns true iff the object is an integer.
 *
 * Return:      Success:        true or false.
 *
 *              Failure:        never fails
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan  3 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
num_isint (obj_t _self) {

   obj_num_t    *self = MYCLASS(_self);

   return (self && C_NUM==self->pub.cls && !self->fp);
}


/*-------------------------------------------------------------------------
 * Function:    num_fp
 *
 * Purpose:     Returns the floating point value of SELF
 *
 * Return:      Success:        A double value.
 *
 *              Failure:        Never fails
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
double
num_fp (obj_t _self) {

   obj_num_t    *self = MYCLASS(_self);
   
   return self->fp ? self->u.d : (double)(self->u.i);
}


/*-------------------------------------------------------------------------
 * Function:    num_isfp
 *
 * Purpose:     Returns true iff the object is an integer or floating
 *              point value.
 *
 * Return:      Success:        true or false.
 *
 *              Failure:        never fails
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan  3 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
num_isfp (obj_t _self) {

   obj_num_t    *self = MYCLASS(_self);

   return (self && C_NUM==self->pub.cls);
}
