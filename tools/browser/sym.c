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
 * Created:             sym.c
 *                      Dec  4 1996
 *                      Robb Matzke <matzke@viper.llnl.gov>
 *
 * Purpose:             Symbol table functions.
 *
 * Modifications:       
 *
 *   Hank Childs, Thu Nov  1 08:49:23 PST 2001
 *   Include float.h for linux systems that don't have DBL_DIG in math.h
 *
 *-------------------------------------------------------------------------
 */
#include <assert.h>
#include <browser.h>
#include <float.h>
#include <limits.h>
#include <math.h>

#define NSYMS           1024
#define MYCLASS(X)      ((obj_sym_t*)(X))


typedef struct sym_t {
    char        *name;                  /*symbol name                   */
    obj_t       doc;                    /*documentation string          */
    obj_t       var;                    /*value as a variable           */
    obj_t       func;                   /*functional value              */
} sym_t;                                /*symbol table entry            */

typedef struct obj_sym_t {
   obj_pub_t    pub;
   sym_t        *sym;
} obj_sym_t;


class_t         C_SYM;
static int      NSymbols;               /*symbol slots allocated        */
static sym_t    *Symbol;                /*symbol table                  */

static obj_t    sym_new (va_list);
static void     sym_print (obj_t, out_t*);
static obj_t    sym_eval (obj_t);
static obj_t    sym_feval (obj_t);
static char *   sym_name (obj_t);


/*-------------------------------------------------------------------------
 * Function:    sym_class
 *
 * Purpose:     Initializes the SYM class.
 *
 * Return:      Success:        Ptr to the SYM class.
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
sym_class (void) {

   class_t      cls = (class_t)calloc (1, sizeof(*cls));

   cls->name = safe_strdup ("SYM");
   cls->newobj = sym_new;
   cls->dest = NULL;
   cls->copy = NULL;
   cls->print = sym_print;
   cls->eval = sym_eval;
   cls->feval = sym_feval;
   cls->objname = sym_name;
   return cls;
}


/*-------------------------------------------------------------------------
 * Function:    sym_new
 *
 * Purpose:     Creates a new symbol object with the specified name.
 *
 * Return:      Success:        Ptr to the new object.
 *
 *              Failure:        NIL
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
static obj_t
sym_new (va_list ap) {

   obj_sym_t    *self;
   int          i, unused;
   char         *name;
   sym_t        *sym=NULL;

   name = va_arg (ap, char*);
   assert (name);

   for (i=0,unused=(-1); i<NSymbols; i++) {
      if (NULL==Symbol[i].name) {
         if (unused<0) unused = i;
      } else if (!strcmp(Symbol[i].name, name)) {
         sym = Symbol+i;
         break;
      }
   }

   if (!sym && unused<0) {
      if (!Symbol) {
         NSymbols = 1024;
         Symbol = (sym_t*)calloc (NSymbols, sizeof(sym_t));
         unused = 0;
      } else {
         Symbol = (sym_t*)realloc (Symbol, (NSymbols+1024) * sizeof(sym_t));
         memset (Symbol+NSymbols, 0, 1024*sizeof(sym_t));
         unused = NSymbols;
         NSymbols += 1024;
      }
   }

   if (!sym) {
      sym = Symbol + unused;
      sym->name = safe_strdup (name);
   }

   self = (obj_sym_t*)calloc (1, sizeof(obj_sym_t));
   self->sym = sym;
   return (obj_t)self;
}


/*-------------------------------------------------------------------------
 * Function:    sym_print
 *
 * Purpose:     Prints a symbol to the specified file.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
sym_print (obj_t _self, out_t *f) {

   out_puts (f, MYCLASS(_self)->sym->name);
}


/*-------------------------------------------------------------------------
 * Function:    sym_eval
 *
 * Purpose:     Returns the variable value of a symbol if it has one.
 *
 * Return:      Success:        Ptr to a copy of the variable value.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *      Robb Matzke, 4 Feb 1997
 *      Fixed the arguments for the obj_deref() call.
 *
 *      Robb Matzke, 2000-07-03
 *      The symbol `$*' evaluates to a list of all $N files for
 *      consecutive N beginning at 1.
 *-------------------------------------------------------------------------
 */
static obj_t
sym_eval (obj_t _self)
{
    obj_t       name_1=NIL, file_1=NIL, retval=NIL;
    obj_sym_t   *self = MYCLASS(_self);

    /* If the symbol has a variable value then return it. */    
    if (MYCLASS(_self)->sym->var) {
        return obj_copy (MYCLASS(_self)->sym->var, SHALLOW);
    }

    /* The symbol `$*' evaluates to a list of the first consecutive files
     * bound to the $N symbols. */
    if (!strcmp(self->sym->name, "$*")) {
        obj_t   opands[1024], retval;
        int     nopands, i;
        
        for (nopands=0; nopands<NELMTS(opands); nopands++) {
            obj_t symbol;
            char tmp[32];
            sprintf(tmp, "$%d", nopands+1);
            symbol = obj_new(C_SYM, tmp);
            opands[nopands] = sym_vboundp(symbol);
            obj_dest(symbol);
            if (!opands[nopands] || C_FILE!=opands[nopands]->pub.cls) {
                /* We reached the last file or something isn't a file */
                obj_dest(opands[nopands]);
                break;
            }
        }
        retval = V_make_list(nopands, opands);
        for (i=0; i<nopands; i++) {
            obj_dest(opands[i]);
        }
        return retval;
    }

    /* If the symbol exists in the first data file, then return
     * that SDO. */
    name_1 = obj_new (C_SYM, "$1");
    file_1 = MYCLASS(name_1)->sym->var;
    name_1 = obj_dest (name_1);

    if (file_1 && C_FILE==file_1->pub.cls) {
        retval = obj_deref (file_1, 1, &_self);
        return retval;
    }

    /* Symbol has no value. */    
    out_errorn ("eval: variable `%s' has no value", obj_name(_self));
    return NIL;
}
   

/*-------------------------------------------------------------------------
 * Function:    sym_feval
 *
 * Purpose:     Evaluates a symbol to obtain a function of some sort.
 *
 * Return:      Success:        Ptr to a copy of the function associated
 *                              with the specified symbol.
 *
 *              Failure:        
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
sym_feval (obj_t _self) {

   return obj_copy (MYCLASS(_self)->sym->func, SHALLOW);
}


/*-------------------------------------------------------------------------
 * Function:    sym_name
 *
 * Purpose:     Returns a pointer to the symbol name.
 *
 * Return:      Success:        Ptr to sym name.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static char *
sym_name (obj_t _self) {

   obj_sym_t    *self = MYCLASS(_self);

   if (!self || C_SYM!=self->pub.cls) return NULL;
   return self->sym->name;
}


/*-------------------------------------------------------------------------
 * Function:    sym_fbind
 *
 * Purpose:     Binds a function to a symbol.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
sym_fbind (obj_t _self, obj_t func) {

   obj_sym_t    *self = MYCLASS(_self);

   if (self->sym->func) obj_dest (self->sym->func);
   self->sym->func = func;
}


/*-------------------------------------------------------------------------
 * Function:    sym_fboundp
 *
 * Purpose:     Determines if the specified object is a symbol with a
 *              functional value.
 *
 * Return:      Success:        Ptr to a copy of the functional value.
 *
 *              Failure:        NIL if the SELF is not a symbol or is a
 *                              symbol with no functional value.
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
obj_t
sym_fboundp (obj_t _self) {

   obj_sym_t    *self = MYCLASS(_self);

   if (self && C_SYM==self->pub.cls && self->sym->func) {
      return obj_copy (self->sym->func, SHALLOW);
   }
   return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    sym_vbind
 *
 * Purpose:     Binds a value to a symbol w/o copying the value.  Any
 *              previous value is destroyed.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
sym_vbind (obj_t _self, obj_t value) {

   obj_sym_t    *self = MYCLASS(_self);

   if (self->sym->var) obj_dest (self->sym->var);
   self->sym->var = value;
}


/*-------------------------------------------------------------------------
 * Function:    sym_vboundp
 *
 * Purpose:     Determines if the specified object is a symbol with a
 *              variable value.
 *
 * Return:      Success:        A copy of the symbol's value as a variable.
 *
 *              Failure:        NIL if the SELF is not a symbol or is a
 *                              symbol without a value as a variable.
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  5 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
obj_t
sym_vboundp(obj_t _self)
{
    obj_sym_t   *self = MYCLASS(_self);

    if (!self || C_SYM!=self->pub.cls) return NIL;
    return obj_copy (self->sym->var, SHALLOW);
}

/*---------------------------------------------------------------------------
 * Purpose:     Binds a documentation value to a symbol w/o copying the
 *              value. Any previous documentation is destroyed.
 *
 * Programmer:  Robb Matzke
 *              Friday, June  2, 2000
 *
 * Modifications:
 *---------------------------------------------------------------------------
 */
void
sym_dbind(obj_t _self, obj_t value)
{
    obj_sym_t   *self = MYCLASS(_self);
    assert(C_SYM==self->pub.cls);
    if (self->sym->doc) obj_dest(self->sym->doc);
    self->sym->doc = value;
}

/*---------------------------------------------------------------------------
 * Purpose:     Determines if the specified symbol has a documentation value.
 *
 * Return:      A copy of the symbol's documentation value, or NIL if the
 *              symbol is not documented.
 *
 * Programmer:  Robb Matzke
 *              Friday, June  2, 2000
 *
 * Modifications:
 *---------------------------------------------------------------------------
 */
obj_t
sym_dboundp(obj_t _self)
{
    obj_sym_t   *self = MYCLASS(_self);
    if (self && C_SYM==self->pub.cls) return obj_copy(self->sym->doc, SHALLOW);
    return NIL;
}

/*---------------------------------------------------------------------------
 * Purpose:     Convenience function for setting the documentation string
 *              of some symbol.
 *
 * Programmer:  Robb Matzke
 *              Tuesday, June  6, 2000
 *
 * Modifications:
 *---------------------------------------------------------------------------
 */
void
sym_doc(const char *symname, const char *docstr)
{
    obj_t       sym = obj_new(C_SYM, symname);

    if (docstr) {
        obj_t doc = obj_new(C_STR, docstr);
        sym_dbind(sym, doc);
    } else {
        sym_dbind(sym, NIL);
    }
    obj_dest(sym);
}


/*-------------------------------------------------------------------------
 * Function:    sym_self_set
 *
 * Purpose:     Gives symbol `self' a new variable value and returns the
 *              previous value.  The new value is not copied.
 *
 * Return:      Success:        Previous value of variable `self'.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 13 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
obj_t
sym_self_set (obj_t newval) {

   obj_t        oldval=NIL, selfvar=NIL;

   selfvar = obj_new (C_SYM, "self");
   oldval = sym_vboundp (selfvar);
   sym_vbind (selfvar, newval);
   selfvar = obj_dest(selfvar);
   return oldval;
}

/*---------------------------------------------------------------------------
 * Purpose:     Set a builtin symbol to the specified value. If VALUE
 *              looks like a number then it is treated as such, otherwise
 *              the symbol is assigned a string value.
 *
 *              If NAME does not begin with the standard prefix used for
 *              builtin variables then it will be automatically added.
 *
 * Programmer:  Robb Matzke
 *              Thursday, June  1, 2000
 *
 * Modifications:
 *---------------------------------------------------------------------------
 */
void
sym_bi_set(const char *name, const char *value, const char *desc,
           const char *doc)
{
    char        fullname[1024], *rest;
    obj_t       symbol;

    /* Add built-in prefix */
    if (*name!='$') {
        fullname[0] = '$';
        strcpy(fullname+1, name);
        name = fullname;
    }
    symbol = obj_new(C_SYM, name);

    /* Does value look like a number or a string? */
    if (!value || !*value) {
        sym_vbind(symbol, NIL);
    } else {
        strtod(value, &rest);
        if (rest && *rest) {
            sym_vbind(symbol, obj_new(C_STR, value));
        } else {
            sym_vbind(symbol, obj_new(C_NUM, value));
        }
    }

    /* Description for var table of contents */
    if (desc) {
        HelpVarToc[NHelpVarToc].name = safe_strdup(name);
        HelpVarToc[NHelpVarToc].desc = safe_strdup(desc);
        NHelpVarToc++;
    }
    
    /* The documentation string */
    if (doc) sym_dbind(symbol, obj_new(C_STR, doc));
    obj_dest(symbol);
}

/*---------------------------------------------------------------------------
 * Purpose:     Return the string value of a builtin symbol.
 *
 * Return:      Copy of the string value or NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, June  2, 2000
 *
 * Modifications:
 *---------------------------------------------------------------------------
 */
char *
sym_bi_gets(const char *name)
{
    char        fullname[1024], *retval;
    obj_t       var, val;

    /* Add built-in prefix */
    if (*name!='$') {
        fullname[0] = '$';
        strcpy(fullname+1, name);
        name = fullname;
    }

    var = obj_new(C_SYM, name);
    val = sym_vboundp(var);
    var = obj_dest(var);

    retval = safe_strdup(obj_name(val));
    obj_dest(val);
    return retval;
}

/*---------------------------------------------------------------------------
 * Purpose:     Returns the integer value of a built-in symbol. If the
 *              symbol is set to something not an integer then return 1.
 *
 * Programmer:  Robb Matzke
 *              Thursday, June  1, 2000
 *
 * Modifications:
 *---------------------------------------------------------------------------
 */
int
sym_bi_true(const char *name)
{
    char        fullname[1024];
    int         retval;
    obj_t       var_name, val;

    /* Add built-in prefix */
    if (*name!='$') {
        fullname[0] = '$';
        strcpy(fullname+1, name);
        name = fullname;
    }

    /* Get value */
    var_name = obj_new(C_SYM, name);
    val = sym_vboundp(var_name);
    var_name = obj_dest(var_name);
    if (num_isint(val)) retval = num_int(val);
    else retval = val ? 1 : 0;
    val = obj_dest (val);
    return retval;
}


/*-------------------------------------------------------------------------
 * Function:    sym_truth
 *
 * Purpose:     Returns true if the symbol has a variable value which
 *              is true.
 *
 * Return:      Success:        true
 *
 *              Failure:        false
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Feb  7 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
sym_truth (char *name) {

   obj_sym_t    *self=NULL;
   int          retval;

   if (!name || !*name) return false;
   self = (obj_sym_t *)obj_new (C_SYM, name);
   retval = obj_truth (self->sym->var);
   obj_dest ((obj_t)self);
   return retval;
}


/*-------------------------------------------------------------------------
 * Function:    sym_init
 *
 * Purpose:     Initializes browser variables.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 20 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      Removed the C_print_DBobject symbol.
 *
 *      Robb Matzke, 2 Apr 1997
 *      Added the `$rdonly' variable.
 *
 *      Robb Matzke, 11 Jun 1997
 *      Added the `doc_url' variable.
 *
 *      Robb Matzke, 29 Jul 1997
 *      Added the `html_browsers' and `$trapfpe' symbols.
 *
 *      Robb Matzke, 2 Sep 1997
 *      Added symbols for the new `int8' datatype.
 *
 *      Robb Matzke, 2000-06-01
 *      Calls sym_bi_set() for numbers and strings. Added documentation.
 *
 *      Robb Matzke, 2000-06-02
 *      Removed initialization of $truncate; added $height and $width.
 *
 *      Robb Matzke, 2000-06-27
 *      The $fmt_float and $fmt_double formats are based on FLT_DIG and
 *      DBL_DIG, which according to POSIX are `the number of decimal
 *      digits in the fraction'.
 *
 *      Robb Matzke, 2000-06-27
 *      Added the `$exclude' variable.
 *
 *      Robb Matzke, 2000-10-19
 *      Added the `$obase' variable.
 *
 *      Mark C. Miller, Wed Sep 23 11:53:59 PDT 2009
 *      Added $fmt_llong for long long data.
 *
 *      Mark C. Miller, Wed Nov 11 22:18:17 PST 2009
 *      Added suppot for alternate relative diff option using epsilon.
 *
 *      Mark C. Miller, Fri Nov 13 15:38:07 PST 2009
 *      Changed name of "long long" type to "longlong" as PDB is sensitive
 *
 *      Mark C. Miller, Tue Nov 17 22:30:30 PST 2009
 *      Changed name of long long datatype to match PDB proper.
 *
 *      Mark C. Miller, Tue Dec 15 10:14:32 PST 2009
 *      Fixed problem with default format for long type being '%d'. It
 *      should really be '%ld'
 *
 *      Mark C. Miller, Mon Jan 11 16:14:51 PST 2010
 *      Fixed default formats for int8, short and long long. Added
 *      initialization of diffing parameters for long long.
 *
 *      Mark C. Miller, Fri Feb 12 08:41:39 PST 2010
 *      Added $splitvfdexts variable.
 *
 *      Mark C. Miller, Fri Mar 12 01:23:15 PST 2010
 *      Replaced splitvfdexts with $hdf5_vfd_opts
 *
 *      Kathleen Bonnell, Thu Dec 9 09:40:03 PST 2010
 *      Surround use of PUBLIC_INIT_FILE with ifdef for its existence.
 *-------------------------------------------------------------------------
 */
void
sym_init (void)
{
   obj_t        name=NIL;
   char         tmp[64];
   obj_t        list[2], symbol, value;
   
   const char *diff_abs = "This variable controls how the `diff' function "
                          "determines whether two numeric values are the same "
                          "or different. The `diff' function considers two "
                          "values, A and B, to be different if |A-B|>N where "
                          "N is the value of this variable.  If this variable "
                          "does not have a positive value then this test is "
                          "not performed (if the relative difference test is "
                          "also not performed then the browser will use an "
                          "exact match instead). This variable, which "
                          "defaults to zero, is set by the -A and "
                          "--absolute command-line switches.";
   const char *diff_rel = "This variable controls how the `diff' function "
                          "determines whether two numeric values are the same "
                          "or different. The `diff' function considers two "
                          "values, A and B, to be different if "
                          "|A-B|/|A+B|>N/2 where N is the value of this "
                          "variable. If this variable does not have a "
                          "positive value then this test is not performed (if "
                          "the absolute difference test is also not performed "
                          "then the browser will use an exact match instead.) "
                          "This variable, which defaults to zero, is set by "
                          "the -R and --relative command-line switches.";
   const char *diff_eps = "When non-negative, this variable triggers an "
                          "alternative relative `diff' algorithm where two "
                          "values, A and B, are different if "
                          "|A-B|/(|A|+|B|+EPS)>N where EPS is the value of "
                          "this variable and N is the value of the associated "
                          "relative difference tolerance variable. This "
                          "variable, which defaults to -1, is set by the "
                          "-x and --epsilon command-line switches.";

   /* Command-line options */
   sym_bi_set("lowlevel",       "0",
              "Act more like pdbdiff.",
              "If this variable has any true value (nil, zero, and the empty "
              "string are considered false) then the browser reads objects as "
              "type DBobject even if that object has some other datatype such "
              "as DBquadvar. This variable is set by the --lowlevel and -l "
              "command-line switches.\n"
              "\n"
              "If the value is two or higher then the SILO definition of "
              "`DBobject is used and the values of the `comp_names' and "
              "`pdb_names' arrays become part of the object.\n"
              "\n"
              "If the value is one or two then the browser translates the "
              "SILO DBobject structure into a structure which is more "
              "user friendly by adding additional members to the object "
              "datatype. Each new member has a name from the `comp_names' "
              "array and a value based on the corresponding member of the "
              "`pdb_names' array. Changes should not be made to the "
              "`comp_names' or `pdb_names' arrays since the SILO DBobject "
              "is regenerated from the user-friendly fields before being "
              "saved back to the file.");
   sym_bi_set("writeable",         "0",
              "Open files for write access.",
              "If this variable has any true value (nil, zero, and the empty "
              "string are considered false) then the browser opens files in "
              "write mode if the file permissions allow it. Editing "
              "objects in a writeable file is allowed.");
   sym_bi_set("diff",           NULL,
              "Influence behavior of `diff' function.",
              "This variable controls the details of the `diff' function. It "
              "should be a list of words from the set: detail, brief, "
              "summary, ignore_additions, ignore_deletions, and two_column.  "
              "The word `detail' indicates that all details of the "
              "differences are to be shown (the default), while `brief' "
              "means one line of output per difference and `summary' means "
              "one line of output total. No output is generated if the "
              "objects do not differ. The words `ignore_additions' and "
              "`ignore_deletions' mean to consider things which appear in "
              "object B but not A (or vice versa) as being not-different. "
              "For detailed output, the word `two_column' causes the diff "
              "function to display the differences side by side (like "
              "pdbdiff) instead of one above the other (like Unix diff).");
   sym_bi_set("exclude",        NULL,
              "Exclude certain objects from recursive diff.",
              "The value of this variable should be a list of object names "
              "which will be excluded from a recursive diff operation. Each "
              "name should be a string which may contain file name wildcards "
              "similar to the Bourne Shell. If an exclude name begins with a "
              "slash then the name is matched against the full name of the "
              "object, otherwise the matching function only looks at the "
              "basename of the object.  If the name is of the form `type:X' "
              "where `X' is one of the headings printed by the `ls' function "
              "(such as `dir' or `ucdmesh') then all objects of the specified "
              "type will be excluded. When $verbosity>=2 the excluded "
              "object names are displayed.");
   sym_bi_set("checksums",       "0",
              "Do checksum checks on read when database has checksums.",
              "If this variable has any true value (nil, zero, and the empty "
              "string are considered false) then the browser will enable "
              "checksum checks during subsequent read operations. "
              "This variable is set by the --checksums and -c "
              "command-line switches.\n");
   sym_bi_set("evalns",         "0",
              "Evaluate nameschemes on multi-block objects.",
              "If this variable has any true value (nil, zero, and the empty "
              "string are considered false) then the browser calls DBSetEvalNameschemes(1) "
              "forcing Silo library to evaluate nameschemes on multi-block objects. "
              "Note that this feature is NOT compatible with writable.");
   sym_bi_set("properec",         "0",
              "Exit with proper exit codes.",
              "If this variable has any true value (nil, zero, and the empty "
              "string are considered false) then the browser will terminate with a "
              "call to exit(1) iff an error is ever encountered during a run.");
   sym_bi_set("h5vfdopts",       NULL,
              "Specify hdf5 (vfd) options sets when attempting to open files.",
              "The value of this variable should be a list of OPTION=VALUE "
              "strings. The keyword '_NEWSET_' can be used to separate one "
              "group of OPTION=VALUE strings from another, each group forming "
              "one set of options to be used to open files. Browser will try "
              "Them in order when attempting to open a file.\n"); 

   /* Name of public init file */
#ifdef PUBLIC_INIT_FILE
   sym_bi_set("pubinit", PUBLIC_INIT_FILE,
              "Name of public initialization file.",
              "The name of the public initialization file is stored in this "
              "variable regardless of whether that file has actually been "
              "read. Its primary purpose is to be used as the argument to "
              "the `include' function in a user-local startup file.");
#endif
   
   /* Set $diff to something reasonable */
   list[0] = obj_new(C_SYM, "detail");
   list[1] = obj_new(C_SYM, "two_column");
   value = V_make_list(2, list);
   obj_dest(list[0]);
   obj_dest(list[1]);
   symbol = obj_new(C_SYM, "$diff");
   sym_vbind(symbol, value);
   value = NIL;
   symbol = obj_dest(symbol);

   sprintf(tmp, "%d", OUT_NROWS);
   sym_bi_set("height",         tmp,
              "Lines per page of output.",
              "The height of the output terminal in lines. If set to a "
              "positive value then the browser will pause after each "
              "screenful of interactive output (redirected output is "
              "unaffected by this setting). This variable is set by the "
              "--height command-line switch and is reset whenever the "
              "browser receives a window size change signal (SIGWINCH).");
   sprintf(tmp, "%d", OUT_NCOLS);
   sym_bi_set("width",          tmp,
              "Characters per line of output.",
              "The width of the output terminal in characters. The browser "
              "will try not to print data which would wrap from one line to "
              "the next. This variable is set by the --width command-line "
              "switch and is reset whenever the browser receives a window "
              "size change signal (SIGWINCH).");
   sym_bi_set("verbosity",      "1",
              "How much chatter is produced by the browser.",
              "This variable is set by the --quiet and --verbose switches. "
              "The --quiet sets it to zero while --verbose sets it to two "
              "(the default is one). Changing its value at runtime changes "
              "the amount of chatter produced by the browser.");

   /* Documentation category defaults */
   sym_doc("delta",
           "Documentation is initialized in the browser system startup file.");
   sym_doc("faq",
           "Documentation is initialized in the browser system startup file.");
   sym_doc("syntax",
           "Documentation is initialized in the browser system startup file.");
   sym_doc("formats",
           "Documentation is initialized in the browser system startup file.");
   sym_doc("paging",
           "Documentation is initialized in the browser system startup file.");
   sym_doc("redirection",
           "Documentation is initialized in the browser system startup file.");
   sym_doc("traps",
           "Documentation is initialized in the browser system startup file.");
   sym_doc("run", NULL); /*do not document this -- it's special in V_help()*/

   /* Variables for formatting output */
   sym_bi_set("truncate",       NULL,           "Max elmts to print",
              "If this is set to a positive integer N then at most N "
              "elements of each array are displayed. Depending on the "
              "value of $trailing, some of those elements will come from "
              "the beginning of the array and others will come from the "
              "end. The browser prints `...(2000 values omitted)...' at "
              "the point where the values are omitted. Truncation is "
              "disabled (all values are printed) by setting this variable "
              "to nil (its default).");
   sym_bi_set("trailing",       "0",            "Trailing elmts to print",
              "If array output truncation is turned on (see $truncate) and "
              "this variable has a positive integer value N, then up to N of "
              "the displayed values will be taken from the end of the array "
              "instead of the beginning. If $trailing is smaller than "
              "$truncate then the difference is the number of elements "
              "displayed at the beginning of the array; otherwise all "
              "elements displayed are from the end of the array.");
   sym_bi_set("fmt_string",     "\"%s\"",       "String format",
              "This is a C printf() format string used to render string "
              "values in the browser output.");
   sym_bi_set("fmt_null",       "(null)",       "Null format",
              "This is a C printf() format string used to render null "
              "pointers in the browser output.");
   sym_bi_set("fmt_int8",       "%hhd",   "Byte format",
              "This is a C printf() format string used to render byte "
              "values in the browser output.");
   sym_bi_set("fmt_short",      "%hd",   "Short format",
              "This is a C printf() format string used to render short "
              "integer values in the browser output.");
   sym_bi_set("fmt_int",        "%d",           "Integer format",
              "This is a C printf() format string used to render integer "
              "values in the browser output.");
   sym_bi_set("fmt_long",       "%ld",    "Long format",
              "This is a C printf() format string used to render long "
              "integer values in the browser output.");
   sym_bi_set("fmt_long_long",   "%lld",         "Long long format",
              "This is a C printf() format string used to render long "
              "long integer values in the browser output.");

   sprintf(tmp, "%%1.%dg", FLT_DIG);
   sym_bi_set("fmt_float",      tmp,            "Float format",
              "This is a C printf() format string used to render `float' "
              "values in the browser output.");
   
   sprintf(tmp, "%%1.%dg", DBL_DIG);
   sym_bi_set("fmt_double",     tmp,            "Double format",
              "This is a C printf() format string used to render `double' "
              "values in the browser output.");

   sym_bi_set("obase", NULL,                    "Output style",
              "Output of primitive data (integer, character, string, and "
              "floating-point) uses the $fmt_* variables by default. "
              "However, it is also possible to display data in hexadecimal, "
              "octal, or binary format as well by setting this variable "
              "to 16, 8, or 2 (default is anything else).");

   /* Difference tolerances are all set to zero, eps to -1. */
   sym_bi_set("diff_int8_abs",  "0", "Absolute diff tolerance for byte", diff_abs);
   sym_bi_set("diff_int8_rel",  "0", "Relative diff tolerance for byte", diff_rel);
   sym_bi_set("diff_int8_eps",  "-1", "Epsilon for alternate relative diff for byte", diff_eps);
   sym_bi_set("diff_short_abs", "0", "Absolute diff tolerance for short", diff_abs);
   sym_bi_set("diff_short_rel", "0", "Relative diff tolerance for short", diff_rel);
   sym_bi_set("diff_short_eps", "-1", "Epsilon for alternate relative diff for short", diff_eps);
   sym_bi_set("diff_int_abs",   "0", "Absolute diff tolerance for int", diff_abs);
   sym_bi_set("diff_int_rel",   "0", "Relative diff tolerance for int", diff_rel);
   sym_bi_set("diff_int_eps",   "-1", "Epsilon for alternate relative diff for int", diff_eps);
   sym_bi_set("diff_long_abs",  "0", "Absolute diff tolerance for long", diff_abs);
   sym_bi_set("diff_long_rel",  "0", "Relative diff tolerance for long", diff_rel);
   sym_bi_set("diff_long_eps",  "-1", "Epsilon for alternate relative diff for long", diff_eps);
   sym_bi_set("diff_float_abs", "0", "Absolute diff tolerance for float", diff_abs);
   sym_bi_set("diff_float_rel", "0", "Relative diff tolerance for float", diff_rel);
   sym_bi_set("diff_float_eps", "-1", "Epsilon for alternate relative diff for float", diff_eps);
   sym_bi_set("diff_double_abs","0", "Absolute diff tolerance for double", diff_abs);
   sym_bi_set("diff_double_rel","0", "Relative diff tolerance for double", diff_rel);
   sym_bi_set("diff_double_eps","-1", "Epsilon for alternate relative diff for double", diff_eps);
   sym_bi_set("diff_llong_abs",  "0", "Absolute diff tolerance for long long", diff_abs);
   sym_bi_set("diff_llong_rel",  "0", "Relative diff tolerance for long long", diff_rel);
   sym_bi_set("diff_llong_eps",  "-1", "Epsilon for alternate relative diff for long long", diff_eps);
   
   /*
    * Primitive types.
    */
   name = obj_new (C_SYM, "string");
   sym_vbind (name, obj_new (C_PRIM, "string"));
   name = obj_dest (name);

   name = obj_new (C_SYM, "int8");
   sym_vbind (name, obj_new (C_PRIM, "int8"));
   name = obj_dest (name);
   
   name = obj_new (C_SYM, "short");
   sym_vbind (name, obj_new (C_PRIM, "short"));
   name = obj_dest (name);

   name = obj_new (C_SYM, "int");
   sym_vbind (name, obj_new (C_PRIM, "int"));
   name = obj_dest (name);

   name = obj_new (C_SYM, "long");
   sym_vbind (name, obj_new (C_PRIM, "long"));
   name = obj_dest (name);

   name = obj_new (C_SYM, "float");
   sym_vbind (name, obj_new (C_PRIM, "float"));
   name = obj_dest (name);

   name = obj_new (C_SYM, "double");
   sym_vbind (name, obj_new (C_PRIM, "double"));
   name = obj_dest (name);

   /* File symbols */
   sym_doc("_1", "Symbols of the form _N are deprecated. Use $N instead.");
   sym_doc("$1", "Browser variables of the form $N where N is an integer "
           "are reserved for SILO files that are opened by the browser. "
           "The files listed on the browser command-line are opened and "
           "assigned to browser variables $1, $2, etc. Most browser commands "
           "that operate on files use the file bound to $1 by default. "
           "You can obtain the name and current working directory of the "
           "file bound to a symbol by printing the symbol.");
   sym_doc("$*", "This special variable always evaluates to a list of files "
           "opened on the browser command-line. Or more specifically, to the "
           "list of files represented by the first consecutive $N symbols.");
}

/*---------------------------------------------------------------------------
 * Purpose:     Invokes function FUNC on each defined symbol.
 *
 * Programmer:  Robb Matzke
 *              Wednesday, June  7, 2000
 *
 * Modifications:
 *---------------------------------------------------------------------------
 */
int
sym_map(int(*func)(obj_t, void*), void *cdata)
{
    int         i, retval=0;

    for (i=0; i<NSymbols; i++) {
        if (Symbol[i].name) {
            obj_t sym = obj_new(C_SYM, Symbol[i].name);
            retval += (func)(sym, cdata);
            obj_dest(sym);
        }
    }
    return retval;
}
