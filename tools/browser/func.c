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
 * Created:             func.c
 *                      Dec  4 1996
 *                      Robb Matzke <matzke@viper.llnl.gov>
 *
 * Purpose:             Builtin functions.
 *
 * Modifications:       
 *
 *      Thomas Treadway, Thu Jun  8 16:56:35 PDT 2006
 *      Modified readline definitions to support new configure macro.
 *
 *-------------------------------------------------------------------------
 */
#include <config.h>     /*MeshTV configuration record*/

#include <assert.h>
#include <browser.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_FNMATCH_H
#  include <fnmatch.h>
#  ifndef FNM_FILE_NAME
#     define FNM_FILE_NAME 0
#  endif
#endif
#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  else /* !defined(HAVE_HISTORY_H) */
extern void add_history ();
extern int write_history ();
extern int read_history ();
#  endif /* defined(HAVE_READLINE_HISTORY_H) */
  /* no history */
#endif /* HAVE_READLINE_HISTORY */
#include <stdlib.h>
#ifndef _WIN32
  #include <sys/wait.h>
#endif
#if HAVE_UNISTD_H
  #include <unistd.h>
#endif


/* Non-posix functions */
#ifndef _WIN32
extern FILE *popen (const char *, const char *);
extern int pclose (FILE*);
#endif

/* Global variables. */
diffopt_t       DiffOpt;
helptoc_t       HelpFuncToc[25];
int             NHelpFuncToc;
helptoc_t       HelpVarToc[50];
int             NHelpVarToc;
helptoc_t       HelpOpToc[25];
int             NHelpOpToc;


/*-------------------------------------------------------------------------
 * Function:    V_array
 *
 * Purpose:     Creates a new array type.
 *
 * Return:      Success:        Ptr to new array type.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  6 1996
 *
 * Modifications:
 *
 *      Robb Matzke, 30 Jul 1997
 *      Fixed a bug with the comma disappearing between string arguments.
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_array (int argc, obj_t argv[]) {

   char         buf[1024];
   int          i, at=0;
   char         *s;

   if (argc<2) {
      out_errorn ("Array: wrong number of arguments");
      return NIL;
   }

   buf[0] = '\0';
   for (i=0; i<argc-1; i++) {
      if (argv[i] && C_NUM==argv[i]->pub.cls && num_int(argv[i])>0) {
         sprintf (buf+at, "%s%d", at?", ":"", num_int(argv[i]));
         at += strlen (buf+at);

      } else if (argv[i] && (s=obj_name(argv[i]))) {
         if (at) {
            buf[at++] = ',';
            buf[at++] = ' ';
         }
         strcpy (buf+at, s);
         at += strlen (buf+at);

      } else {
         out_error ("Array: inappropriate dimension: ", argv[i]);
         return NIL;
      }
   }

   return obj_new (C_ARY, buf, obj_copy (argv[argc-1], SHALLOW));
}


/*-------------------------------------------------------------------------
 * Function:    V_assign
 *
 * Purpose:     Assigns an RVALUE to an LVALUE.  If LVALUE is a symbol
 *              that has a variable value, then we assign RVALUE to that
 *              symbol.  Otherwise, if LVALUE evaluates to an silo data
 *              object, we assign RVALUE to that SDO.  Otherwise if LVALUE
 *              (unevaluated) is a symbol we assign RVALUE to that new
 *              symbol.
 *
 * Return:      Success:        The RVALUE
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Feb  7 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 19 Feb 1997
 *      Supports assignments to silo data objects.
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_assign (int argc, obj_t argv[]) {

   int          isa_symbol;
   obj_t        val=NIL, retval=NIL;

   if (2!=argc) {
      out_errorn ("Assign: wrong number of arguments");
      return NIL;
   }

   if (!argv[0]) return NIL;    /*error detected below*/
   isa_symbol = (C_SYM == argv[0]->pub.cls);

   /*
    * The LVALUE is a symbol with a variable value. Make the RVALUE the
    * new variable value for that symbol.
    */
   if (isa_symbol && (val=sym_vboundp(argv[0]))) {
      val = obj_dest (val);
      sym_vbind (argv[0], obj_copy (argv[1], SHALLOW));
      return obj_copy (argv[1], SHALLOW);
   }

   /*
    * Eval the LVALUE to see if it's a silo data object.
    */
   out_error_disable();
   val = obj_eval (argv[0]);
   out_error_restore();
   if (val && C_SDO==val->pub.cls) {
      retval = sdo_assign (val, argv[1]);
      val = obj_dest (val);
      return retval;
   }
   val = obj_dest (val);

   /*
    * The LVALUE is a symbol that doesn't evaluate to a silo data object.
    * Assign the RVALUE as the variable value of the symbol.
    */
   if (isa_symbol) {
      sym_vbind (argv[0], obj_copy (argv[1], SHALLOW));
      return obj_copy (argv[1], SHALLOW);
   }

   /*
    * The LVALUE is not a symbol and doesn't evaluate to a silo
    * data object.
    */
   out_errorn ("Assign: left operand has no L-value");
   return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    V_close
 *
 * Purpose:     Closes the files associated with the specified symbols.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 20 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      Cleaned up error messages.
 *-------------------------------------------------------------------------
 */
obj_t
V_close (int argc, obj_t argv[]) {

   int          i;
   char         ebuf[1024];
   obj_t        file=NIL;

   for (i=0; i<argc; i++) {
      if (!argv[i] || C_SYM!=argv[i]->pub.cls) {
         sprintf (ebuf, "close: inappropriate file symbol as arg-%d: ", i+1);
         out_error (ebuf, argv[i]);
      } else if (NIL==(file=sym_vboundp(argv[i])) || C_FILE!=file->pub.cls) {
         out_errorn ("close: no file associated with %s", obj_name(argv[i]));
         file = obj_dest (file);
      } else {
         file = obj_dest (file);
         sym_vbind (argv[i], NIL);
      }
   }
   return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    F_cons
 *
 * Purpose:     Creates a new cons cell with a HEAD and TAIL.
 *
 * Return:      Success:        Ptr to new cons cell.
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
obj_t
F_cons (obj_t head, obj_t tail) {

   return obj_new (C_CONS, obj_copy(head, SHALLOW), obj_copy(tail, SHALLOW));
}


/*-------------------------------------------------------------------------
 * Function:    diff_lookup
 *
 * Purpose:     Looks up a diff constant in the symbol table and returns
 *              its value.  If the symbol has a value which is not a
 *              number or a value which is a negative number then the value
 *              is removed with a warning.
 *
 * Return:      Success:        Double value of the variable.
 *
 *              Failure:        -1.0
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Feb  6 1997
 *
 * Modifications:
 *
 *   Mark C. Miller, Tue Nov 17 22:34:51 PST 2009
 *   Added logic to exclude epsilon diff params from requirement to not be
 *   less than zero.
 *-------------------------------------------------------------------------
 */
static double
diff_lookup (char *ascii_name) {

   obj_t        name=NIL, val=NIL;
   char         buf[1024];
   double       retval = -1.0;


   name = obj_new (C_SYM, ascii_name);
   if ((val=sym_vboundp(name))) {
      if (!num_isfp(val)) {
         sprintf (buf, "diff: value of `%s' is inappropriate: ", ascii_name);
         out_error (buf, val);
         sym_vbind (name, NIL);
      } else if ((retval=num_fp(val))<0.0 && !strstr(ascii_name, "_eps")) {
         out_errorn ("diff: value of `%s' is inappropriate: %d",
                     ascii_name, retval);
         sym_vbind (name, NIL);
         retval = -1.0;
      }
      val = obj_dest (val);
   }
   name = obj_dest (name);

   if (Verbosity>=2) {
      if (retval<=0.0) {
         out_info ("diff: %-15s is disabled", ascii_name);
      } else {
         out_info ("diff: %-15s = %e", ascii_name, retval);
      }
   }

   return retval;
}


/*-------------------------------------------------------------------------
 * Function:    V_diff
 *
 * Purpose:     Determines whether two things differ.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 21 1997
 *
 * Modifications:
 *              Robb Matzke, 2000-06-27
 *              Added the `two_column' diff option.
 *
 *              Robb Matzke, 2000-06-28
 *              If more than two arguments are given then the argument
 *              list is split in half and operands from the first half are
 *              differenced against corresponding operands from the second
 *              half. If called with no arguments then the command-line
 *              database files are used as arguments.
 *
 *              Robb Matzke, 2000-06-29
 *              The contents of the $exclude variable is parsed and cached
 *              in the DiffOpts.
 *
 *              Robb Matzke, 2000-07-05
 *              If invoked with one argument and that argument is a silo
 *              database object from file $1 then it will be differenced
 *              against an object of the same name from file $2.
 *
 *              Robb Matzke, 2000-07-10
 *              Fixed a memory corruption bug when called with no
 *              arguments and no files opened on the command-line
 *
 *              Mark C. Miller, Wed Nov 11 22:18:17 PST 2009
 *              Added suppot for alternate relative diff option using
 *              epsilon.
 *
 *              Mark C. Miller, Mon Jan 11 16:21:21 PST 2010
 *              Added support for long long diffing params.
 *-------------------------------------------------------------------------
 */
obj_t
V_diff (int argc, obj_t argv[])
{
    int         status;
    int         old_rtmargin = OUT_STDOUT->rtmargin;
    obj_t       opands[1024], head=NIL, value=NIL, symbol=NIL, word=NIL;
    int         nopands=0, i, j;

    memset(&DiffOpt, 0, sizeof DiffOpt);
    
    if (0==argc) {
        /* When invoked with no arguments use the list of command-line
         * files as arguments. */  
        for (nopands=0; nopands<NELMTS(opands); nopands++) {
            char tmp[32];
            
            sprintf(tmp, "$%d", nopands+1);
            symbol = obj_new(C_SYM, tmp);
            opands[nopands] = sym_vboundp(symbol);
            symbol = obj_dest(symbol);
            if (!opands[nopands] || C_FILE!=opands[nopands]->pub.cls) {
                /*we reached the last file or something isn't a file*/
                opands[nopands] = obj_dest(opands[nopands]);
                break;
            }
        }
    } else if (1==argc) {
        /* When invoked with one argument which is a silo object from the
         * file represented by `$1', the second argument is the silo
         * object of the same name from file `$2'. */
        obj_t my_file=NIL, file_1=NIL, file_2=NIL;
        
        if (!argv[0] || C_SDO!=argv[0]->pub.cls) {
            out_errorn("diff: single-argument must be a silo object");
            goto done;
        }
        my_file = sdo_file(argv[0]);
        
        symbol = obj_new(C_SYM, "$1");
        file_1 = sym_vboundp(symbol);
        symbol = obj_dest(symbol);
        if (!file_1 || C_FILE!=file_1->pub.cls) {
            out_errorn("diff: single-argument must be from file $1");
            my_file = obj_dest(my_file);
            file_1 = obj_dest(file_1);
            goto done;
        }
        if (strcmp(obj_name(my_file), obj_name(file_1))) {
            out_errorn("diff: single-argument must be from file $1");
            my_file = obj_dest(my_file);
            file_1 = obj_dest(file_1);
            goto done;
        }
        my_file = obj_dest(my_file);
        file_1 = obj_dest(file_1);
        

        symbol = obj_new(C_SYM, "$2");
        file_2 = sym_vboundp(symbol);
        symbol = obj_dest(symbol);
        if (!file_2 || C_FILE!=file_2->pub.cls) {
            out_errorn("diff: file $2 is not defined");
            file_2 = obj_dest(file_2);
            goto done;
        }

        symbol = obj_new(C_SYM, obj_name(argv[0]));
        opands[nopands++] = obj_copy(argv[0], SHALLOW);
        opands[nopands++] = obj_deref(file_2, 1, &symbol);
        symbol = obj_dest(symbol);
        
    } else {
        for (nopands=0; nopands<argc && nopands<NELMTS(opands); nopands++) {
            opands[nopands] = obj_copy(argv[nopands], SHALLOW);
        }
    }

    /* The number of operands had better be even */
    if (nopands % 2) {
        out_errorn("diff requires an even number of arguments or "
                   "command-line database files");
        goto done;
    }
    if (0==nopands) {
        out_errorn("nothing to difference");
        goto done;
    }
    
    /* Parse and cache $diff value */
    symbol = obj_new(C_SYM, "$diff");
    head = sym_vboundp(symbol);
    symbol = obj_dest(symbol);
    if (head && C_CONS!=head->pub.cls) {
        head = obj_new(C_CONS, obj_copy(head, SHALLOW), NIL);
    }
    for (value=head; value; value=cons_tail(value)) {
        if (C_CONS!=value->pub.cls) {
            out_errorn("diff: invalid value for $diff");
            goto done;
        }
        word = cons_head(value);
        if (C_SYM==word->pub.cls) {
            if (!strcmp(obj_name(word), "all")) {
                DiffOpt.report = DIFF_REP_ALL;
            } else if (!strcmp(obj_name(word), "detail")) {
                DiffOpt.report = DIFF_REP_ALL;
            } else if (!strcmp(obj_name(word), "detailed")) {
                DiffOpt.report = DIFF_REP_ALL;
            } else if (!strcmp(obj_name(word), "brief")) {
                DiffOpt.report = DIFF_REP_BRIEF;
            } else if (!strcmp(obj_name(word), "sum")) {
                DiffOpt.report = DIFF_REP_SUMMARY;
            } else if (!strcmp(obj_name(word), "summary")) {
                DiffOpt.report = DIFF_REP_SUMMARY;
            } else if (!strcmp(obj_name(word), "summarize")) {
                DiffOpt.report = DIFF_REP_SUMMARY;
            } else if (!strcmp(obj_name(word), "ignore_additions")) {
                DiffOpt.ignore_adds = true;
            } else if (!strcmp(obj_name(word), "ignore_deletions")) {
                DiffOpt.ignore_dels = true;
            } else if (!strcmp(obj_name(word), "two_column")) {
                DiffOpt.two_column = true;
            } else {
                out_errorn("word `%s' of $diff isn't recognized (ignored)",
                           obj_name(word));
            }
        } else {
            out_errorn("diff: invalid value for $diff");
            goto done;
        }
    }
    head = obj_dest(head);

    /* Parse and cache $exclude values */
    symbol = obj_new(C_SYM, "$exclude");
    head = sym_vboundp(symbol);
    symbol = obj_dest(symbol);
    if (head && C_CONS!=head->pub.cls) {
        head = obj_new(C_CONS, obj_copy(head, SHALLOW), NIL);
    }
    for (value=head; value; value=cons_tail(value)) {
        if (C_CONS!=value->pub.cls) {
            out_errorn("diff: invalid value for $exclude");
            goto done;
        }
        if (DiffOpt.exclude.nused>=NELMTS(DiffOpt.exclude.value)) {
            out_errorn("diff: too many exclusions (limit %lu)",
                       (unsigned long)NELMTS(DiffOpt.exclude.value));
            goto done;
        }
        word = cons_head(value);
        if (C_STR==word->pub.cls) {
            i = DiffOpt.exclude.nused++;
            DiffOpt.exclude.value[i] = safe_strdup(obj_name(word));
#ifndef HAVE_FNMATCH
            if (strpbrk(DiffOpt.exclude.value[i], "*?[]")) {
                out_errorn("diff: $exclude = \"%s\" contains wildcards but "
                           "your C library doesn't have the `fnmatch' "
                           "function. Names will be matched literally.",
                           DiffOpt.exclude.value[i]);
            }
#endif
        } else {
            out_errorn("diff: $exclude values should be strings");
            goto done;
        }
    }
    head = obj_dest(head);

    /* Cache tolerances */
    DiffOpt.c_abs = diff_lookup("$diff_int8_abs");
    DiffOpt.c_rel = diff_lookup("$diff_int8_rel");
    DiffOpt.c_eps = diff_lookup("$diff_int8_eps");
    DiffOpt.s_abs = diff_lookup("$diff_short_abs");
    DiffOpt.s_rel = diff_lookup("$diff_short_rel");
    DiffOpt.s_eps = diff_lookup("$diff_short_eps");
    DiffOpt.i_abs = diff_lookup("$diff_int_abs");
    DiffOpt.i_rel = diff_lookup("$diff_int_rel");
    DiffOpt.i_eps = diff_lookup("$diff_int_eps");
    DiffOpt.l_abs = diff_lookup("$diff_long_abs");
    DiffOpt.l_rel = diff_lookup("$diff_long_rel");
    DiffOpt.l_eps = diff_lookup("$diff_long_eps");
    DiffOpt.f_abs = diff_lookup("$diff_float_abs");
    DiffOpt.f_rel = diff_lookup("$diff_float_rel");
    DiffOpt.f_eps = diff_lookup("$diff_float_eps");
    DiffOpt.d_abs = diff_lookup("$diff_double_abs");
    DiffOpt.d_rel = diff_lookup("$diff_double_rel");
    DiffOpt.d_eps = diff_lookup("$diff_double_eps");
    DiffOpt.ll_abs = diff_lookup("$diff_llong_abs");
    DiffOpt.ll_rel = diff_lookup("$diff_llong_rel");
    DiffOpt.ll_eps = diff_lookup("$diff_llong_eps");

            
    for (i=0; i<nopands/2; i++) {
        char header[8192], a_buf[32], b_buf[32];
        const char *a_name, *b_name;
        
        /* Print a table header for each pair of arguments */
        if (NULL==(a_name=obj_name(opands[i]))) {
            sprintf(a_buf, "Argument %d", i+1);
            a_name = a_buf;
        }
        if (NULL==(b_name=obj_name(opands[nopands/2+i]))) {
            sprintf(b_buf, "Argument %d", nopands/2+i+1);
            b_name = b_buf;
        }

        /* Skip a line between each pair of arguments */
        strcpy(header, i?"\n":"");

        /* Choose a header line appropriate for the output style */
        if (DIFF_REP_ALL==DiffOpt.report && DiffOpt.two_column) {
            sprintf(header+strlen(header), "%-*s%-*s%*s%s\n",
                    OUT_LTMAR, "Object", OUT_COL2-OUT_LTMAR, a_name,
                    (int)strlen(DIFF_SEPARATOR), "", b_name);
            OUT_STDOUT->rtmargin = 0; /*don't split long lines*/
        } else {
            sprintf(header+strlen(header), "Differences between %s and %s\n",
                    a_name, b_name);
        }

        /* Put a line below the header */
        for (j=0; j<OUT_NCOLS-2; j++) strcat(header, "-");
        out_header(OUT_STDOUT, header);
    
        /* The difference... */
        status = obj_diff(opands[i], opands[nopands/2+i]);
        if (!out_brokenpipe(OUT_STDOUT)) {
            switch (DiffOpt.report) {
            case DIFF_REP_ALL:
                if (2==status) {
                    out_line(OUT_STDOUT, "***************");
                    obj_print(opands[i], OUT_STDOUT);
                    out_line(OUT_STDOUT, "---------------");
                    obj_print(opands[nopands/2+i], OUT_STDOUT);
                    out_line(OUT_STDOUT, "***************");
                }
                break;
            case DIFF_REP_BRIEF:
                if (2==status) {
                    out_puts(OUT_STDOUT, "different value(s)");
                    out_nl(OUT_STDOUT);
                }
                break;
            case DIFF_REP_SUMMARY:
                if (status>0) {
                    out_line(OUT_STDOUT, "objects differ");
                }
                break;
            }
        }
    }
    
 done:
    /* Restore output margins and cancel table headers*/
    OUT_STDOUT->rtmargin = old_rtmargin;
    out_header(OUT_STDOUT, NULL);

    /* Free temp expressions */
    obj_dest(symbol);
    obj_dest(head);

    /* Free operands */
    for (i=0; i<nopands; i++) obj_dest(opands[i]);

    /* Free DiffOpt */
    for (i=0; i<DiffOpt.exclude.nused; i++) {
        if (DiffOpt.exclude.value[i]) {
            free(DiffOpt.exclude.value[i]);
            DiffOpt.exclude.value[i] = NULL;
        }
    }
    
    return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    V_dot
 *
 * Purpose:     A binary operator.  The left operand should be a file
 *              and the right operand should be an object name within
 *              that file.
 *
 *              Or the left operand should be memory with a structure
 *              type and the right operand should be a field name within
 *              that structure.
 *
 *              Or the left operand should be memory with an array type
 *              and the right operand(s) should be indices or ranges
 *              thereof.
 *
 * Return:      Success:        Ptr to a SILO database object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  5 1996
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Feb 1997
 *      More than one argument is allowed.
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_dot (int argc, obj_t argv[]) {

   obj_t        retval=NIL;

   if (argv[0]) {
      retval = obj_deref(argv[0], argc-1, argv+1);
   } else {
      out_error ("Dot: inappropriate left operand: ", argv[0]);
   }

   return retval;
}


/*-------------------------------------------------------------------------
 * Function:    V_exit
 *
 * Purpose:     Exit the program.  If a numeric argument is specified then
 *              we exit with that value.
 *
 * Return:      Success:        Does not return
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      If an argument is supplied then it must be an integer.
 *
 *      Robb Matzke, 10 Feb 1997
 *      History is saved in a history file.
 *
 *      Sean Ahern, Fri Feb 28 14:12:58 PST 1997
 *      Added a check for the readline library.
 *
 *      Thomas R. Treadway, Tue Jun 27 13:59:21 PDT 2006
 *      Added HAVE_STRERROR wrapper
 *
 *      Thomas R. Treadway, Thu Mar  1 09:37:31 PST 2007
 *      Corrected write history logic
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_exit (int argc, obj_t argv[]) {

#if defined(HAVE_READLINE_HISTORY_H) && defined(HISTORY_FILE) && defined(HAVE_READLINE_HISTORY)
   if (!(HistoryFile[0] && !write_history (HistoryFile))) 
   {
#ifdef HAVE_STRERROR
      out_errorn ("command history not saved in %s (%s)",
                  HistoryFile, strerror(errno));
#else
      out_errorn ("command history not saved in %s (errno=%d)",
                  HistoryFile, errno);
#endif
   }
#endif

   if (0==argc) {
      if (sym_bi_true("properec"))
         exit(browserErrno);
      exit (0);
   }
   if (1==argc) {
      if (!num_int(argv[0])) {
         out_errorn ("exit: arg-1 is not an integer");
         return NIL;
      }
      if (sym_bi_true("properec") && num_int(argv[0])==0)
         exit(browserErrno);
      exit (num_int(argv[0]));
   }

   out_errorn ("exit: wrong number of arguments");
   return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    F_fbind
 *
 * Purpose:     Bind a function to a name.
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
F_fbind (obj_t self, obj_t func) {

   assert (self && C_SYM==self->pub.cls);
   sym_fbind (self, obj_copy(func, SHALLOW));
}


/*-------------------------------------------------------------------------
 * Function:    V_file
 *
 * Purpose:     Opens a SILO file but does not associate that file with
 *              a symbol.  Thus, as soon as all references to this file
 *              object dissappear, the file is closed.
 *
 * Return:      Success:        Ptr to a silo file object.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *      Robb Matzke, 7 Feb 1997
 *      Changed the name of this function from F_open to V_file.
 *
 *      Robb Matzke, 2 Apr 1997
 *      If `$rdonly' is true then the file is open for reading only.
 *
 *      Mark C. Miller, Wed Dec  4 11:22:26 PST 2013
 *      Replaced `$rdonly' behavior with `$writable' thereby swaping the
 *      default open behavior to be to open for write only when it is
 *      acutally requested.
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_file (int argc, obj_t argv[]) {

   obj_t        retval=NIL, filename=NIL;
   char         *fname;
   int          writeable = sym_bi_true("writeable");

   if (1!=argc) {
      out_errorn ("file: wrong number of arguments");
      return NIL;
   }
   filename = argv[0];
   
   if (!filename) {
      out_errorn ("file: no file name given");
      
   } else if (C_FILE==filename->pub.cls) {
      retval = obj_copy (filename, SHALLOW);    /*already opened*/
      
   } else if (NULL==(fname=obj_name(filename))) {
      out_errorn ("file: arg-1 is inappropriate");

   } else if (NIL==(retval=obj_new (C_FILE, fname, writeable))) {
#if 0 /*error message already printed*/
      out_errorn ("file: could not open `%s'", fname);
#endif
   }
   return retval;
}


/*-------------------------------------------------------------------------
 * Function:    F_flatten
 *
 * Purpose:     Flattens a list so (a (b (c)) d) becomes (a b c d).
 *
 * Return:      Success:        A new list with shallow copies of the atoms.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Apr  3 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
obj_t
F_flatten (obj_t lst) {

   obj_t        opstack=NIL;
   obj_t        ptr=NIL;
   obj_t        retval=NIL;

   if (!lst || C_CONS!=lst->pub.cls) {
      return obj_copy (lst, SHALLOW); /*nothing to flatten*/
   }

   /*
    * Create a stack of all the atoms.
    */
   for (/*void*/; lst; lst=cons_tail(lst)) {
      obj_t hd = cons_head (lst);
      if (!hd) {
         /*
          * Head is NIL
          */
         opstack = obj_new (C_CONS, NIL, opstack);
         
      } else if (C_CONS==hd->pub.cls) {
         /*
          * Head is a list.  Flatten it and then add those elements
          * into the opstack.
          */
         obj_t flattened = F_flatten (hd);
         for (ptr=flattened; ptr; ptr=cons_tail(ptr)) {
            opstack = obj_new (C_CONS, F_head(ptr), opstack);
         }
         flattened = obj_dest (flattened);
         
      } else {
         /*
          * Add a copy of the head to the opstack.
          */
         opstack = obj_new (C_CONS, obj_copy(hd, SHALLOW), opstack);
      }
   }

   /*
    * Return the reversal of the stack.
    */
   retval = F_reverse (opstack);
   opstack = obj_dest (opstack);
   return retval;
}
         

/*-------------------------------------------------------------------------
 * Function:    F_head
 *
 * Purpose:     Returns the head of a list.
 *
 * Return:      Success:        Ptr to the head.
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
obj_t
F_head (obj_t lst) {

   if (!lst) return NIL;
   if (C_CONS!=lst->pub.cls) return NIL;

   return obj_copy (cons_head(lst), SHALLOW);
}

/*---------------------------------------------------------------------------
 * Purpose:     Callback for help apropos function.
 *
 * Programmer:  Robb Matzke
 *              Wednesday, June  7, 2000
 *
 * Modifications:
 *---------------------------------------------------------------------------
 */
static int
help_apropos(obj_t sym, void *cdata)
{
    const char  *s = (const char*)cdata;
    obj_t       doc = sym_dboundp(sym);
    int         found = false;

    if (doc && C_STR==doc->pub.cls) {
        const char *docstr = obj_name(doc);
        if (strstr(obj_name(sym), s) || strstr(docstr, s)) {
            char buf[256];
            sprintf(buf, "help %s", obj_name(sym));
            out_line(OUT_STDOUT, buf);
            found = true;
        }
    }
    obj_dest(doc);
    return found;
}


/*-------------------------------------------------------------------------
 * Function:    V_help
 *
 * Purpose:     Offers help.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Feb  3 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 2000-06-02
 *      Real users want a quick-and-dirty text-based help system a la
 *      meshtvx.  This command takes zero or one argument. When invoked
 *      with zero arguments is prints a table of contents (TOC). When
 *      invoked with a symbol name it prints the documentation string for
 *      that symbol. When invoked with a string it searches all symbols
 *      for the specified word and prints those that match. When invoked
 *      with a symbol and a string it assigns the string as the
 *      documentation for the symbol and returns null.
 *-------------------------------------------------------------------------
 */
obj_t
V_help (int argc, obj_t argv[])
{
    int                 i, is_run;
    obj_t               doc=NIL, sym=NIL;
    static int          ncalls=0;
    static helptoc_t    toc[] = {
        {"help",        "Help on the `help' function"},
        {"delta",       "Changes since previous version"},
        {"faq",         "Frequently asked questions"},
        {"run",         "Browser execution and switches"},
        {"syntax",      "Browser syntax"},
        {"functions",   "Built-in functions"},
        {"operators",   "Operators and precedence"},
        {"variables",   "Predefined variables"},
        {"formats",     "Data output formats"},
        {"paging",      "Paging long output"},
        {"redirection", "Piping and output redirection"},
        {"interrupts",  "Interrupting long-running commands"},
        {"traps",       "Traps for the unwary"},
        {"$FOO",        "Help for variable $FOO"},
        {"--FOO",       "Help for command-line switch --FOO"},
        {"FOO",         "Help for built-in function FOO"},
        {"\"opFOO\"",   "Help for operator FOO"},
        {"\"FOO\"",     "Help containing string \"FOO\""}, 
    };

    if (0==ncalls++) {
        /* Table of Contents */
        obj_t type = obj_new(C_STC, NULL, NULL);
        for (i=0; i<NELMTS(toc); i++) {
            char buf[32];
            sprintf(buf, "help %s", toc[i].name);
            stc_add(type, obj_new(C_PRIM, "string"),
                    i*sizeof(*toc)+sizeof(char*), buf);
        }
        doc = obj_new(C_SDO, NIL, NULL, toc, type, toc, type,
                      NULL, NULL, NULL);
        sym = obj_new(C_SYM, "$toc");
        sym_dbind(sym, doc);
        obj_dest(sym);
        obj_dest(type);

        /* Function list */
        type = obj_new(C_STC, NULL, NULL);
        for (i=0; i<NHelpFuncToc; i++) {
            char buf[32];
            sprintf(buf, "help %s", HelpFuncToc[i].name);
            stc_add(type, obj_new(C_PRIM, "string"),
                    i*sizeof(helptoc_t)+sizeof(char*), buf);
        }
        doc = obj_new(C_SDO, NIL, NULL, HelpFuncToc, type, HelpFuncToc, type,
                      NULL, NULL, NULL);
        sym = obj_new(C_SYM, "functions");
        sym_dbind(sym, doc);
        obj_dest(sym);
        obj_dest(type);

        /* Operator list */
        type = obj_new(C_STC, NULL, NULL);
        for (i=0; i<NHelpOpToc; i++) {
            char buf[32];
            sprintf(buf, "help %s", HelpOpToc[i].name);
            stc_add(type, obj_new(C_PRIM, "string"),
                    i*sizeof(helptoc_t)+sizeof(char*), buf);
        }
        doc = obj_new(C_SDO, NIL, NULL, HelpOpToc, type, HelpOpToc, type,
                      NULL, NULL, NULL);
        sym = obj_new(C_SYM, "operators");
        sym_dbind(sym, doc);
        obj_dest(sym);
        obj_dest(type);

        /* Variable list */
        type = obj_new(C_STC, NULL, NULL);
        for (i=0; i<NHelpVarToc; i++) {
            char buf[32];
            sprintf(buf, "help %s", HelpVarToc[i].name);
            stc_add(type, obj_new(C_PRIM, "string"),
                    i*sizeof(helptoc_t)+sizeof(char*), buf);
        }
        doc = obj_new(C_SDO, NIL, NULL, HelpVarToc, type, HelpVarToc, type,
                      NULL, NULL, NULL);
        sym = obj_new(C_SYM, "variables");
        sym_dbind(sym, doc);
        obj_dest(sym);
        obj_dest(type);
    }

    /* Obtain the symbol */
    if (0==argc) {
        sym = obj_new(C_SYM, "$toc");
    } else if (!argv[0]) {
        out_errorn("help: first argument cannot be NIL");
        return NIL;
    } else if (C_STR==argv[0]->pub.cls) {
        const char *s = obj_name(argv[0]);
        if (!strncmp("op", s, 2) && s[2]) {
            sym = obj_new(C_SYM, s);
        } else {
            if (!sym_map(help_apropos, (void*)s)) {
                out_errorn("help: nothing appropriate");
            }
            return NIL;
        }
    } else if (C_SYM==argv[0]->pub.cls) {
        sym = argv[0];
    } else {
        out_errorn("help: wrong type for first argument");
        return NIL;
    }
    
    /* Set documentation string? */
    if (2==argc) {
        sym_dbind(argv[0], obj_copy(argv[1], SHALLOW));
        return NIL;
    } else if (argc>2) {
        out_errorn("help: wrong number of arguments");
        return NIL;
    }
    
    /* Obtain documentation string */
    assert(sym);
    is_run = !strcmp("run", obj_name(sym));
    doc = sym_dboundp(sym);
    if (sym!=argv[0]) obj_dest(sym);
    sym = NIL;

    /* `help run' is a special case */
    if (!doc && is_run) {
        usage();
        return NIL;
    }
    
    /* Print documentation */
    if (doc) {
        /* Turn off string formating -- use out_putw() instead */
        obj_t fmt_string = obj_new(C_SYM, "$fmt_string");
        obj_t old_fmt = sym_vboundp(fmt_string);
        sym_bi_set("$fmt_string", NULL, NULL, NULL);

        /* Print documentation */
        obj_print(doc, OUT_STDOUT);
        out_nl(OUT_STDOUT);

        /* Restore previous string format */
        sym_vbind(fmt_string, old_fmt);
        obj_dest(fmt_string);
        doc = obj_dest(doc);
    } else {
        out_errorn("help: no documentation found.");
    }
    return doc;
}

/*---------------------------------------------------------------------------
 * Purpose:     Cause subsequent input to come from the file named by the
 *              argument. When that input source is exhausted then input
 *              will begin to come from the original source again.
 *
 * Return:      NIL
 *
 * Programmer:  Robb Matzke
 *              Monday, July 10, 2000
 *
 * Modifications:
 *---------------------------------------------------------------------------
 */
obj_t
V_include(int argc, obj_t argv[])
{
    char        *name;
    lex_t       *f;
    
    if (1!=argc) {
        out_errorn("include: wrong number of arguments");
        return NIL;
    }
    if (NULL==(name=obj_name(argv[0]))) {
        out_errorn("include: no file name given");
        return NIL;
    }
    if (!LEX_STDIN) {
        out_errorn("include: internal error -- no input source");
        return NIL;
    }
    if (NULL==(f=lex_open(name))) return NULL;
    lex_push(LEX_STDIN, f);
    return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    F_length
 *
 * Purpose:     Returns the number of elements in a list.
 *
 * Return:      Success:        Length of LST
 *
 *              Failure:        -1 if not a list, 0 if LST is NIL.
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  5 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
F_length (obj_t lst) {

   int          i;

   for (i=0; lst; lst=cons_tail(lst),i++) {
      if (C_CONS!=lst->pub.cls) return -1;
   }
   return i;
}


/*-------------------------------------------------------------------------
 * Function:    V_list
 *
 * Purpose:     Lists the current working directory in the specified
 *              file.  If no file is specified then `$1' is assumed.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      Cleaned up error messages.
 *
 *      Robb Matzke, 6 Feb 1997
 *      We list the objects ourselves instead of calling DBListDir because
 *      it allows us to redirect and/or page the output.  It also allows
 *      us to make the output look more like the rest of the browser output.
 *
 *      Robb Matzke, 7 Feb 1997
 *      Now takes any number of arguments.  If the first argument is
 *      a symbol with a file value, then use that file for the listing.
 *      If the first argument is not a symbol, then evaluate it to get the
 *      file to use for listing.  Otherwise treat the first argument as
 *      an item to list.  All other arguments are items to list.
 *
 *      Robb Matzke, 25 Jul 1997
 *      This function was indented to list the table of contents for the
 *      current directory of a subset thereof.  If object names and/or
 *      wild cards are given, they apply to the names of the objects in the
 *      current working directory.  However, many people want to be able to
 *      list the contents of a subdirectory by saying `ls dir1' where `dir1'
 *      is a member of the current working directory.  Therefore, after
 *      wild-card expansion occurs, if the display list contains a single
 *      object and that object is a directory, then we load the table of
 *      contents from that directory and display it rather than the
 *      directory name.
 *
 *      Robb Matzke, 26 Aug 1997
 *      Fixed a memory bug when the only argument is a directory name.
 *
 *      Lisa J. Roberts, Mon Nov 22 17:27:53 PST 1999
 *      I changed strdup to safe_strdup.
 *
 *      Robb Matzke, 2000-05-17
 *      If the argument is a directory name and that directory is empty
 *      then the error message will be `ls: no table of contents' instead
 *      of `ls: no matches'.
 *
 *      Robb Matzke, 2000-07-03
 *      If the first argument is a list of files then perform the
 *      operation once for each file of that list.
 *-------------------------------------------------------------------------
 */
obj_t
V_list (int argc, obj_t argv[])
{
    obj_t       fileobjs=NIL, ptr=NIL;
    int         first_arg = 0;
    int         minus_l = 0;

    if (argc>=1 && C_SYM==argv[0]->pub.cls && !strcmp("-l",obj_name(argv[0]))) {
        /* Is the first symbol a -l modifier? If so, gobble it up here. */
        int i;
        minus_l = 1;
        for (i = 0; i < argc-1; i++)
            argv[i] = argv[i+1];
        argc--;
    }

    if (argc>=1 && C_SYM==argv[0]->pub.cls) {
        /* Is the first symbol bound to a file or is it a special symbol
         * which evaluates to a file or list of files? */
        if ((fileobjs=sym_vboundp(argv[0]))) {
            if (C_FILE==fileobjs->pub.cls) {
                first_arg = 1;
                fileobjs = obj_new(C_CONS, fileobjs, NIL);
            } else {
                fileobjs = obj_dest(fileobjs);
            }
        } else if ((fileobjs=obj_eval(argv[0]))) {
            if (C_CONS==fileobjs->pub.cls) {
                first_arg = 1;
            } else {
                fileobjs = obj_dest(fileobjs);
            }
        }

    } else if (argc>=1 && C_STR!=argv[0]->pub.cls) {
        /* The file is the result of evaluating the first expression. */
        fileobjs = obj_eval(argv[0]);
        if (!fileobjs) return NIL; /*error in eval*/
        if (C_FILE==fileobjs->pub.cls) {
            first_arg = 1;
            fileobjs = obj_new(C_CONS, fileobjs, NIL);
        } else {
            out_errorn("ls: arg-1 does not evaluate to a file");
            goto error;
        }
    }

    /* Use the default file. */    
    if (!fileobjs) {
        obj_t b1 = obj_new(C_SYM, "$1");
        fileobjs = sym_vboundp(b1);
        b1 = obj_dest(b1);

        if (!fileobjs) {
            out_errorn("ls: no default open file (`$1' has no value)");
            return NIL;
        }
        fileobjs = obj_new(C_CONS, fileobjs, NIL);
    }

    for (ptr=fileobjs; ptr; ptr=cons_tail(ptr)) {
        DBfile  *file;
        DBtoc   *dbtoc;
        toc_t   *toc;
        int     i, nentries, width, old_type=(-1);
        int     argno, nprint;
        int     *selected=NULL, last_selected=-1;
        char    buf[256], *needle, nselected=0;
        char    cwd[1024], *subdir;

        /* Do we have a file? */    
        obj_t fileobj = cons_head(ptr);
        if (!fileobj || C_FILE!=fileobj->pub.cls ||
            NULL==(file=file_file(fileobj))) {
            out_error("ls: inappropriate file: ", fileobj);
            goto error;
        }

        /* Get the table of contents sorted first by object type and
         * then by object name. */
        toc = browser_DBGetToc(file, &nentries, sort_toc_by_type);
        if (!toc || 0==nentries) {
            out_errorn("ls: no table of contents");
            goto error;
        }
        dbtoc = DBGetToc(file);

        /* Prune the table of contents based on the arguments supplied. */    
        selected = (int *)calloc(nentries, sizeof(int));
        if (first_arg==argc) {
            for (i=0; i<nentries; i++) {
                selected[i] = true;
            }
        } else {
            for (argno=first_arg; argno<argc; argno++) {
                if (NULL==(needle=obj_name(argv[argno]))) {
                    out_errorn("ls: arg-%d is not an object name", argno+1);
                } else {
#ifndef HAVE_FNMATCH
                    if (strpbrk(needle, "*?[]")) {
                        out_errorn("ls: arg-%d contains wildcards but your C "
                                   "library doesn't have the `fnmatch' "
                                   "function", argno+1);
                    }
#endif
                    for (i=0; i<nentries; i++) {
#ifdef HAVE_FNMATCH
                        if (0==fnmatch(needle, toc[i].name,
                                       FNM_FILE_NAME|FNM_PERIOD)) {
                            selected[i] = true;
                            nselected++;
                            last_selected = i;
                        }
#else
                        if (!strcmp(toc[i].name, needle)) {
                            selected[i] = true;
                            nselected++;
                            last_selected = i;
                        }
#endif
                    }
                }
            }
        }

        /* If the result is a single directory, then list the contents of
         * the directory instead of the directory name. */
        if (1==nselected && BROWSER_DB_DIR==toc[last_selected].type) {
            subdir = safe_strdup(toc[last_selected].name);
            for (i=0; i<nentries; i++) free(toc[i].name);
            free(toc);
            if (DBGetDir(file, cwd)<0) return NIL;
            if (DBSetDir(file, subdir)<0) return NIL;
            toc = browser_DBGetToc(file, &nentries, sort_toc_by_type);
            if (DBSetDir(file, cwd)<0) return NIL;
            if (!toc || 0==nentries) {
                out_errorn("ls: no table of contents");
                free(selected);
                goto error;
            }
            out_info("Listing file: %s, directory: %s:",
                     obj_name(fileobj), subdir);
            out_nl(OUT_STDOUT);
            free(subdir);
            subdir = NULL;

            /* select all entries of that directory for display */
            free(selected);
            selected = (int *)calloc(nentries, sizeof(int));
            for (i=0; i<nentries; i++) selected[i] = true;
        } else {
            out_info("Listing from file %s", obj_name(fileobj));
        }
        
        /* Find the widest entry and if any entries were even selected. */    
        width = 0;
        for (i=0; i<nentries; i++) {
            if (selected[i]) width = MAX(width, (int)strlen(toc[i].name));
        }
        if (0==width) {
            out_errorn("ls: no matches");
        }

        /* Print the objects grouped by object type.  Each group of objects
         * has a prefix only on the first line. */
        if (width>0) {
            for (i=nprint=0; i<nentries && !out_brokenpipe(OUT_STDOUT); i++) {
                char const *target = DBIsSymlink(dbtoc, toc[i].name);
                if (!selected[i]) continue;
                if (toc[i].type!=old_type) {
                    if (nprint) {
                        out_nl(OUT_STDOUT);
                        out_nl(OUT_STDOUT);
                    }
                    sprintf(buf, "%s(s)", ObjTypeName[toc[i].type]);
                    out_push(OUT_STDOUT, buf);
                }
                if (target && !minus_l)
                {
                    char tmp[256];
                    snprintf(tmp, sizeof(tmp)-2, "%s@", toc[i].name);
                    out_printf(OUT_STDOUT, " %-*s", width, tmp);
                }
                else
                    out_printf(OUT_STDOUT, " %-*s", width, toc[i].name);
                if (minus_l) {
                    if (target)
                        out_printf(OUT_STDOUT, " -> %-*s", width, target?target:"***Error***");
                }
                if (toc[i].type!=old_type) {
                    out_pop(OUT_STDOUT);
                    old_type = toc[i].type;
                }
                nprint++;
            }
            out_nl(OUT_STDOUT);
        }

        /* Free data */    
        for (i=0; i<nentries; i++) free(toc[i].name);
        free(toc);
        free(selected);
        /* no free for dbtoc */
    }
    return NIL;

 error:
    obj_dest(fileobjs);
    return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    V_make_list
 *
 * Purpose:     Returns a list of the arguments.  The list is not the
 *              same list as the original arguments since the arguments
 *              have been evaluated.
 *
 * Return:      Success:        Ptr to a new list with shallow copies of
 *                              the caller-evaluated arguments
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Apr  2 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_make_list (int argc, obj_t argv[]) {

   obj_t        opstack=NIL, retval=NIL;
   int          i;

   for (i=0; i<argc; i++) {
      opstack = obj_new (C_CONS,
                         obj_copy (argv[i], SHALLOW),
                         opstack);
   }
   retval = F_reverse (opstack);
   opstack = obj_dest (opstack);
   return retval;
}
   

/*-------------------------------------------------------------------------
 * Function:    V_noprint
 *
 * Purpose:     Returns NIL.  Used to suppress the output of a non-nil
 *              expression.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 23 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/*ARGSUSED*/
obj_t
V_noprint (int argc, obj_t argv[]) {

   return NIL;
}
         

/*-------------------------------------------------------------------------
 * Function:    V_open
 *
 * Purpose:     Sets the current file to be the named SILO file.  The
 *              current file is called `$1'.  If an even number of
 *              arguments are present then the first argument of each pair
 *              is the name of a SILO file and the second argument is the
 *              name of the variable that will hold that file.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      Changed the name displayed in error messages to `open' since
 *      this function is usually invoked with the `open' command.
 *
 *      Robb Matzke, 6 Feb 1997
 *      The previous file is closed even if the new file can't be opened.
 *
 *      Robb Matzke, 7 Feb 1997
 *      Takes just one or two arguments.  Changed the name of this function
 *      from V_with to V_open.
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_open (int argc, obj_t argv[]) {

   obj_t        file=NIL, filename=NIL, var=NIL;

   /*
    * Get the variable name
    */
   if (2==argc) {
      filename = obj_copy (argv[0], SHALLOW);
      var = obj_copy (argv[1], SHALLOW);
   } else if (1==argc) {
      filename = obj_copy (argv[0], SHALLOW);
      var = obj_new (C_SYM, "$1");
   } else {
      out_errorn ("open: wrong number of arguments");
      goto error;
   }

   if (C_SYM!=var->pub.cls) {
      out_errorn ("open: arg-2 should be a symbol");
      goto error;
   }
   
   /*
    * Open the file.
    */
   if (Verbosity>=1) {
      char *ascii_name = obj_name (filename);
      out_info ("opening `%s' as %s",
                ascii_name?ascii_name:"***NO NAME***", obj_name (var));
   }
   file = V_file (1, &filename);
   if (!file) {
      sym_vbind (var, NIL);
      goto error;
   }

   /*
    * Assign the file to the variable.
    */
   sym_vbind (var, file);
   var = obj_dest (var);
   filename = obj_dest(filename);
   file = NIL ; /*do not destroy file*/
   return NIL;

error:
   if (file    ) file     = obj_dest (file    );
   if (var     ) var      = obj_dest (var     );
   if (filename) filename = obj_dest (filename);
   return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    V_pipe
 *
 * Purpose:     Evaluates and prints the first argument with standard output
 *              redirected to the shell command specified by the second
 *              argument.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 13 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      Cleaned up error messages.
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_pipe (int argc, obj_t argv[]) {

   out_t        saved;
   FILE         *f;
   char         *command, *fmode;
   obj_t        out=NIL;
   int          status;


   if (3!=argc) {
      out_errorn ("Pipe: wrong number of arguments");
      return NIL;
   }
   if (NULL==(command=obj_name(argv[1]))) {
      out_error ("Pipe: arg-2 (command) is inappropriate: ", argv[1]);
      return NIL;
   }
   if (NULL==(fmode=obj_name(argv[2]))) {
      out_error ("Pipe: arg-3 (mode) is inappropriate: ", argv[2]);
      return NIL;
   }
   if (NULL==(f=popen(command, fmode))) {
      out_errorn ("Pipe: could not run: %s", command);
      return NIL;
   }

   /*
    * Point OUT_STDOUT at the pipe.
    */
   fflush (stderr);
   fflush (stdout);
   saved = *OUT_STDOUT;
   out_reset (OUT_STDOUT);
   OUT_STDOUT->f = f;
   OUT_STDOUT->paged = false;

   /*
    * Evaluate the first argument.
    */
   out = obj_eval (argv[0]);
   if (out || Verbosity>=2) {
      obj_print (out, OUT_STDOUT);
      out_nl (OUT_STDOUT);
   }
   out = obj_dest (out);

   /*
    * Point OUT_STDOUT at the original stream.
    */
   *OUT_STDOUT = saved;

   /*
    * Close the command
    */
   status = pclose (f);
   if (WIFEXITED(status)) {
      if (WEXITSTATUS(status)) {
         out_errorn ("Pipe: command failed with exit status: %d",
                     WEXITSTATUS(status));
      }
   } else if (WIFSIGNALED(status)) {
      out_errorn ("Pipe: command received signal %d", WTERMSIG(status));
   }
   
   return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    V_pointer
 *
 * Purpose:     Creates a pointer to the first argument.
 *
 * Return:      Success:        Pointer type object.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.mdn.com
 *              Dec  9, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_pointer (int argc, obj_t argv[]) {

   if (1!=argc) {
      out_errorn ("pointer: wrong number of arguments");
      return NIL;
   }
   return obj_new (C_PTR, obj_copy (argv[0], SHALLOW));
}


/*-------------------------------------------------------------------------
 * Function:    V_primitive
 *
 * Purpose:     Given the name of a primitive type, return a new
 *              primitive type object.
 *
 * Return:      Success:        Primitive type object.
 *
 *              Failure:        NIL
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
V_primitive (int argc, obj_t argv[]) {

   char         *s, buf[32];

   if (1!=argc) {
      out_errorn ("primitive: wrong number of arguments");
      return NIL;
   }

   if (num_isint(argv[0])) {
      sprintf (buf, "%d", num_int(argv[0]));
      s = buf;
   } else if (NULL==(s=obj_name(argv[0]))) {
      out_error ("primitive: type name is inappropriate: ", argv[0]);
      return NIL;
   }

   return obj_new (C_PRIM, s);
}


/*-------------------------------------------------------------------------
 * Function:    V_print
 *
 * Purpose:     Prints each argument to standard output.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      NIL arguments are ignored.  We do this because a command like `XXX'
 *      is parsed as `print XXX' and if `XXX' is not an object we get an
 *      error message and the `XXX' turns into a NIL pointer.  Printing
 *      `nil' would be redundant.  However, this means that the command
 *      `print nil' won't do anything!
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_print (int argc, obj_t argv[]) {

   int          i;

   for (i=0; i<argc && !out_brokenpipe(OUT_STDOUT); i++) {
      if (argv[i]) {
         obj_print (argv[i], OUT_STDOUT);
         out_nl (OUT_STDOUT);
      }
   }
   return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    V_pwd
 *
 * Purpose:     Prints the current working directory of the specified file,
 *              or `$1' if no file is specified.  Actually, this function
 *              doesn't really do anything but return the file, since the
 *              print form of a file includes the current working
 *              directory name.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 20 1997
 *
 * Modifications:
 *              Robb Matzke, 3 Feb 1997
 *              Cleaned up error messages.
 *
 *              Robb Matzke, 2000-07-03
 *              If invoked with a single argument which is a list of files
 *              just return that list of files.  This allows us to invoke
 *              this command as `pwd $*' to show the current working
 *              directories of all the command-line files.
 *-------------------------------------------------------------------------
 */
obj_t
V_pwd (int argc, obj_t argv[])
{
    obj_t       retval=NIL;

    if (0==argc) {
        obj_t name = obj_new(C_SYM, "$1");
        retval = sym_vboundp(name);
        name = obj_dest(name);
        if (!retval) {
            out_errorn("pwd: no default open file (`$1' has no value)");
            goto error;
        }
    } else if (1==argc) {
        retval = obj_copy(argv[0], SHALLOW);
    } else {
        out_errorn("pwd: wrong number of arguments");
        goto error;
    }

    if (!retval) {
        out_errorn("pwd: no file specified");
        goto error;
    } else if (C_CONS==retval->pub.cls) {
        obj_t ptr, f;
        for (ptr=retval; ptr; ptr=cons_tail(ptr)) {
            f = cons_head(ptr);
            if (!f || C_FILE!=f->pub.cls) {
                out_errorn("pwd: arg is not a list of files");
                goto error;
            }
        }
    } else if (C_FILE!=retval->pub.cls) {
        out_errorn("pwd: argument is not a file");
        goto error;
    }
    
    return retval;

 error:
    obj_dest(retval);
    return NIL;
}


/*-------------------------------------------------------------------------
 * Function:    V_quote
 *
 * Purpose:     Just returns a copy of the first argument.  If there's more
 *              than one argument then return a list of the arguments.
 *
 * Return:      Success:        A copy of the first argument.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Apr  2 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_quote (int argc, obj_t argv[]) {

   if (argc<1) return NIL;
   if (1==argc) return obj_copy (argv[0], SHALLOW);
   return V_make_list (argc, argv);
}


/*-------------------------------------------------------------------------
 * Function:    V_redirect
 *
 * Purpose:     Redirects standard output to the specified file then evaluates
 *              and prints the first argument.  The file name is the
 *              second argument.
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  4 1996
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      Cleaned up error messages.
 *
 *      Thomas R. Treadway, Tue Jun 27 13:59:21 PDT 2006
 *      Added HAVE_STRERROR wrapper
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_redirect (int argc, obj_t argv[]) {

   out_t        saved;
   FILE         *f;
   obj_t        out=NIL;
   char         *fname=NULL, *fmode=NULL;

   if (3!=argc) {
      out_errorn ("Redirect: wrong number of arguments");
      return NIL;
   }
   if (!argv[1] || NULL==(fname=obj_name(argv[1]))) {
      out_error ("Redirect: arg-2 (file name) is inappropriate", argv[1]);
      return NIL;
   }
   if (!argv[2] || NULL==(fmode=obj_name(argv[2]))) {
      out_error ("Redirect: arg-3 (mode) is inappropriate", argv[2]);
      return NIL;
   }
   if (NULL==(f=fopen(fname, fmode))) {
#ifdef HAVE_STRERROR
      out_errorn ("Redirect: cannot open `%s' (%s)",
                  fname, strerror(errno));
#else
      out_errorn ("Redirect: cannot open `%s' (errno=%d)",
                  fname, errno);
#endif
      return NIL;
   }


   fflush (stderr);
   fflush (stdout);
   saved = *OUT_STDOUT;
   out_reset (OUT_STDOUT);
   OUT_STDOUT->f = f;
   OUT_STDOUT->paged = false;

   out = obj_eval (argv[0]);
   if (out || Verbosity>=2) {
      obj_print (out, OUT_STDOUT);
      out_nl (OUT_STDOUT);
   }
   out = obj_dest (out);

   *OUT_STDOUT = saved;
   fclose (f);

   return NIL;
}
   

/*-------------------------------------------------------------------------
 * Function:    F_reverse
 *
 * Purpose:     Reverses list LST.
 *
 * Return:      Success:        Ptr to a new list.
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
obj_t
F_reverse (obj_t lst) {

   obj_t        ret=NIL, b1, b2;

   if (!lst) return NIL;
   if (C_CONS!=lst->pub.cls) return obj_copy(lst, SHALLOW);

   for (/*void*/; lst; lst=cons_tail(lst)) {

      b1 = F_head (lst);
      b2 = F_cons (b1, ret);
      obj_dest (b1);
      obj_dest (ret);
      ret = b2;
   }
   return ret;
}


/*-------------------------------------------------------------------------
 * Function:    V_setcwd
 *
 * Purpose:     Sets the current working directory for a file (or for `$1'
 *              if no file is specified).
 *
 * Return:      Success:        The file.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 20 1997
 *
 * Modifications:
 *      Robb Matzke, 3 Feb 1997
 *      Cleaned up error messages.
 *
 *      Robb Matzke, 5 Feb 1997
 *      The first argument is always the directory name.  The second
 *      argument is the optional file.
 *
 *      Robb Matzke, 2000-07-03
 *      If the second argument is a list of files (instead of just a file)
 *      then the current working directory is changed for all listed files
 *      and the file list is returned.  The return value is always the list
 *      of files, even if something goes wrong (this allows the user to see
 *      what the CWD is for each file).
 *-------------------------------------------------------------------------
 */
obj_t
V_setcwd (int argc, obj_t argv[])
{
    obj_t       files=NIL, cwd=NIL, ptr=NIL;
    char        *dirname=NULL;
    DBfile      *dbfile;

    /* Get arguments */
    if (1==argc) {
        obj_t name = obj_new (C_SYM, "$1");
        files = sym_vboundp (name);
        name = obj_dest (name);
        cwd  = argv[0];

        if (!files) {
            out_errorn ("cd: no default open file (`$1' has no value)");
            return NIL;
        }
    } else if (2==argc) {
        files = obj_copy (argv[1], SHALLOW);
        cwd  = argv[0];
    } else {
        out_errorn ("cd: wrong number of arguments");
        return NIL;
    }

    /* Make sure `files' is a list of files and the directory name is some
    * name string. */
    if (C_CONS!=files->pub.cls) {
        files = obj_new(C_CONS, files, NIL);
    }
    if (!cwd || NULL==(dirname=obj_name(cwd))) {
        out_error ("cd: inappropriate directory name: ", cwd);
        goto error;
    }

    /* Change directories for each file */
    for (ptr=files; ptr; ptr=cons_tail(ptr)) {
        obj_t f = cons_head(ptr);
        if (!f || C_FILE!=f->pub.cls || NULL==(dbfile=file_file(f))) {
            out_error("cd: inappropriate file: ", f);
        } else if (DBSetDir (dbfile, dirname)<0) {
            out_errorn ("cd: cannot set CWD to \"%s\" for file %s",
                        dirname, obj_name(f));
        }
    }
   
    return files;

 error:
    obj_dest(files);
    return NIL;
}
      

/*-------------------------------------------------------------------------
 * Function:    V_setf
 *
 * Purpose:     Sets the symbols functional value (and removes an
 *              existing value).
 *
 * Return:      Success:        NIL
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan 21 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      Cleaned up error messages.
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_setf (int argc, obj_t argv[]) {

   if (2!=argc) {
      out_errorn ("fsetf: wrong number of arguments");
      return NIL;
   }
   if (!argv[0] || C_SYM!=argv[0]->pub.cls) {
      out_error ("fsetf: arg-1 (symbol) is inappropriate: ", argv[0]);
      return NIL;
   }

   sym_fbind (argv[0], obj_copy(argv[1], SHALLOW));
   return NIL;
}
      

/*-------------------------------------------------------------------------
 * Function:    V_struct
 *
 * Purpose:     Creates a structure. The first argument is the name
 *              and the remaining arguments are offset and subtype pairs.
 *
 * Return:      Success:        Ptr to a struct type object.
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              matzke@viper.llnl.gov
 *              Dec  6 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_struct (int argc, obj_t argv[]) {

   obj_t        sub[32];
   int          offset[32], i, argno;
   char         *structname, *name[32];

   if (argc<4 || argc>NELMTS(sub)*3+1) {
      out_errorn ("struct: wrong number of arguments");
      return NIL;
   }

   memset (sub, 0, sizeof(sub));
   memset (offset, 0, sizeof(offset));
   memset (name, 0, sizeof(name));
   
   structname = obj_name (argv[0]);

   for (i=0,argno=1; i<NELMTS(sub) && argno+1<argc; i++,argno+=3) {
      /*
       * The offset.
       */
      if (!argv[argno] || C_NUM!=argv[argno]->pub.cls) {
         out_errorn ("struct: offset for component %d is not numeric", i+1);
         return NIL;
      }
      if ((offset[i]=num_int(argv[argno]))<0) {
         out_errorn ("struct: offset for component %d is out of range", i+1);
         return NIL;
      }

      /*
       * The name.
       */
      if (NULL==(name[i]=obj_name(argv[argno+1]))) {
         out_errorn ("struct: component %d has no name", i+1);
         return NIL;
      }
      
      /*
       * The component type.
       */
      sub[i] = argv[argno+2];
      if (!sub[i]) {
         out_errorn ("struct: component type %d is missing", i+1);
         return NIL;
      }
   }

   return obj_new (C_STC, structname,
                   obj_copy(sub[ 0], SHALLOW), offset[ 0], name[ 0],
                   obj_copy(sub[ 1], SHALLOW), offset[ 1], name[ 1],
                   obj_copy(sub[ 2], SHALLOW), offset[ 2], name[ 2],
                   obj_copy(sub[ 3], SHALLOW), offset[ 3], name[ 3],
                   obj_copy(sub[ 4], SHALLOW), offset[ 4], name[ 4],
                   obj_copy(sub[ 5], SHALLOW), offset[ 5], name[ 5],
                   obj_copy(sub[ 6], SHALLOW), offset[ 6], name[ 6],
                   obj_copy(sub[ 7], SHALLOW), offset[ 7], name[ 7],
                   obj_copy(sub[ 8], SHALLOW), offset[ 8], name[ 8],
                   obj_copy(sub[ 9], SHALLOW), offset[ 9], name[ 9],
                   obj_copy(sub[10], SHALLOW), offset[10], name[10],
                   obj_copy(sub[11], SHALLOW), offset[11], name[11],
                   obj_copy(sub[12], SHALLOW), offset[12], name[12],
                   obj_copy(sub[13], SHALLOW), offset[13], name[13],
                   obj_copy(sub[14], SHALLOW), offset[14], name[14],
                   obj_copy(sub[15], SHALLOW), offset[15], name[15],
                   obj_copy(sub[16], SHALLOW), offset[16], name[16],
                   obj_copy(sub[17], SHALLOW), offset[17], name[17],
                   obj_copy(sub[18], SHALLOW), offset[18], name[18],
                   obj_copy(sub[19], SHALLOW), offset[19], name[19],
                   obj_copy(sub[20], SHALLOW), offset[20], name[20],
                   obj_copy(sub[21], SHALLOW), offset[21], name[21],
                   obj_copy(sub[22], SHALLOW), offset[22], name[22],
                   obj_copy(sub[23], SHALLOW), offset[23], name[23],
                   obj_copy(sub[24], SHALLOW), offset[24], name[24],
                   obj_copy(sub[25], SHALLOW), offset[25], name[25],
                   obj_copy(sub[26], SHALLOW), offset[26], name[26],
                   obj_copy(sub[27], SHALLOW), offset[27], name[27],
                   obj_copy(sub[28], SHALLOW), offset[28], name[28],
                   obj_copy(sub[29], SHALLOW), offset[29], name[29],
                   obj_copy(sub[30], SHALLOW), offset[30], name[30],
                   obj_copy(sub[31], SHALLOW), offset[31], name[31], NULL);
}


/*-------------------------------------------------------------------------
 * Function:    F_tail
 *
 * Purpose:     Returns the tail of a list.
 *
 * Return:      Success:        Ptr to the tail
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
obj_t
F_tail (obj_t lst) {

   if (!lst) return NIL;
   if (C_CONS!=lst->pub.cls) return NIL;

   return obj_copy (cons_tail(lst), SHALLOW);
}


/*-------------------------------------------------------------------------
 * Function:    V_typeof
 *
 * Purpose:     Prints the type of some object.
 *
 * Return:      Success:        The type
 *
 *              Failure:        NIL
 *
 * Programmer:  Robb Matzke
 *              robb@maya.nuance.mdn.com
 *              Jan  6 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 3 Feb 1997
 *      Works for all types of objects.
 *
 *-------------------------------------------------------------------------
 */
obj_t
V_typeof (int argc, obj_t argv[]) {

   obj_t        retval=NIL;
   char         buf[256];

   if (1!=argc) {
      out_errorn ("typeof: wrong number of arguments");
      return NIL;
   }
   
   if (!argv[0]) {
      retval = NIL;

   } else if (C_SDO==argv[0]->pub.cls) {
      retval = obj_copy (sdo_typeof(argv[0]), SHALLOW);

   } else if (num_isint(argv[0])) {
      sprintf (buf, "%s_int", argv[0]->pub.cls->name);
      retval = obj_new (C_SYM, buf);

   } else if (num_isfp(argv[0])) {
      sprintf (buf, "%s_fp", argv[0]->pub.cls->name);
      retval = obj_new (C_SYM, buf);
      
   } else {
      retval = obj_new (C_SYM, argv[0]->pub.cls->name);

   }
   return retval;
}

