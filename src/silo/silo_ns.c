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
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <silo_private.h>

typedef struct _DBexprnode {
    char type;
    long long val;
    char sval[128];
    struct _DBexprnode *left;
    struct _DBexprnode *right;
} DBexprnode;

static void
FreeTree(DBexprnode *tree)
{
    if (!tree)
        return;
    FreeTree(tree->left);
    FreeTree(tree->right);
    free(tree);
}

static DBexprnode *
UpdateTree(DBexprnode *tree, const char t, long long v, char *s)
{
    DBexprnode *retval = 0;
    DBexprnode *newnode = (DBexprnode *) calloc(1,sizeof(DBexprnode));
    newnode->type = t;
    if (t == 'c')
        newnode->val = v;
    else 
    {
        if (s)
            strncpy(newnode->sval, s, sizeof(newnode->sval)-1);
        else
            newnode->sval[0] = '\0';
    }

    newnode->left = 0;
    newnode->right = 0;

    if (tree == 0)
    {
        retval = newnode;
    }
    else if (tree->left == 0 && tree->right == 0)
    {
        /* t better be an operator */
        newnode->left = tree;
        retval = newnode;
    }
    else if (tree->left != 0 && tree->right == 0)
    {
        /* t better be a constant */
        tree->right = newnode;
        retval = tree;
    }
    else if (tree->left == 0 && tree->right != 0)
    {
        /* should never happen */
        ;
    }
    else
    {
        /* t better be an operator */
        newnode->left = tree;
        retval = newnode;
    }

    return retval;
}

static DBexprnode *
BuildExprTree(const char **porig)
{
    DBexprnode *tree = 0;
    const char *p = *porig;

    while (*p != '\0')
    {
        switch (*p)
        {
            case ' ':
            {
                break; /* ignore spaces */
            }

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            {
                char tokbuf[256];
                char *tp = tokbuf;
                long long val;
                while ('0' <= *p && *p <= '9')
                    *tp++ = *p++;
                p--;
                *tp = '\0';
                errno = 0;
                val = strtol(tokbuf, 0, 0);
                if (errno == 0 && val != LONG_MIN && val != LONG_MAX)
                    tree = UpdateTree(tree, 'c', val, 0);
                break;
            }

            case '\'': /* beginning of string */
            {
                char tokbuf[129];
                char *tp = tokbuf;
                p++;
                while (*p != '\'')
                    *tp++ = *p++;
                *tp = '\0';
                errno = 0;
                tree = UpdateTree(tree, 's', 0, tokbuf);
                break;
            }

            case '$': /* array ref */
            case '#':
            {
                char typec = *p;
                char tokbuf[129];
                char *tp = tokbuf;
                p++;
                while (*p != '[')
                    *tp++ = *p++;
                p--;
                *tp = '\0';
                errno = 0;
                tree = UpdateTree(tree, typec, 0, tokbuf);
                break;
            }

            case 'n':
            case '+': case '-': case '*': case '/': case '%':
            case '|': case '&': case '^':
            {
                tree = UpdateTree(tree, *p, 0, 0);
                break;
            }

            case '(': case '[':
            {
                DBexprnode *subtree;
                p++;
                subtree = BuildExprTree(&p);
                if (tree == 0)
                    tree = subtree;
                else if (tree->left == 0)
                    tree->left = subtree;
                else if (tree->right == 0)
                    tree->right = subtree;
                break;
            }

            case ']': /* terminating array ref */
            case ')': /* terminating subtree */
            {
                *porig = p;
                return tree;
            }

            case '?':
            {
                DBexprnode *newtreeq = 0;
                DBexprnode *newnodec = 0;
                DBexprnode *subtreel, *subtreer;
                newtreeq = UpdateTree(newtreeq, *p, 0, 0);
                newtreeq->left = tree;
                p++;
                subtreel = BuildExprTree(&p);
                p++;
                subtreer = BuildExprTree(&p);
                newnodec = UpdateTree(newnodec, ':', 0, 0);
                newnodec->left = subtreel;
                newnodec->right = subtreer;
                newtreeq->right = newnodec;
                tree = newtreeq;
                break;
            }

            case ':': /* terminating if-then-else (?::) */
            {
                *porig = p;
                return tree;
            }


        }
        p++;
    }
    *porig = p;
    return tree;
}

/* very simple circular cache for a handful of embedded strings */
static int SaveInternalString(DBnamescheme const *ns, const char *sval)
{
    /* The modn/embedns portion of a namescheme is 'internal' state
       allowed to disobey const rules */
    DBnamescheme *non_const_ns = (DBnamescheme*) ns;
    int modn = non_const_ns->nembed++ % DB_MAX_EXPNS;
    if (non_const_ns->embedns[modn])
        DBFreeNamescheme(non_const_ns->embedns[modn]);
    non_const_ns->embedns[modn] = DBMakeNamescheme(sval);
    return modn;
}

/* very simple circular cache for strings returned from DBGetName */
#define DB_MAX_RETSTRS 32
static char * retstrbuf[DB_MAX_RETSTRS];
static char * SaveReturnedString(char const * retstr)
{
    static size_t n = 0;
    size_t modn;

    /* Hack to cleanup when really needed */
    if (retstr == 0)
    {
        for (n = 0; n < DB_MAX_RETSTRS; n++)
            FREE(retstrbuf[n]);
        n = 0;
        return 0;
    }

    modn = n % DB_MAX_RETSTRS;
    n++;
    FREE(retstrbuf[modn]);
    retstrbuf[modn] = STRDUP(retstr);
    return retstrbuf[modn];
}

static long long
EvalExprTree(DBnamescheme const *ns, DBexprnode *tree, long long n)
{
    if (tree == 0)
        return 0;
    else if ((tree->type == '$' || tree->type == '#') && tree->left != 0)
    {
        long long i, q = EvalExprTree(ns, tree->left, n);
        for (i = 0; i < ns->narrefs; i++)
        {
            if (strcmp(tree->sval, ns->arrnames[i]) == 0)
            {
                if (tree->type == '$')
                    return SaveInternalString(ns,  ((char**)ns->arrvals[i])[q]);
                else
                    return ((int*)ns->arrvals[i])[q];
            }
        }
    }
    else if (tree->left == 0 && tree->right == 0)
    {
        if (tree->type == 'c')
            return tree->val;
        else if (tree->type == 'n')
            return n;
        else if (tree->type == 's')
            return SaveInternalString(ns, tree->sval);
    }
    else if (tree->left != 0 && tree->right != 0)
    {
        long long vc = 0, vl = 0, vr = 0;
        if (tree->type == '?')
        {
            vc = EvalExprTree(ns, tree->left, n);
            tree = tree->right;
            if (vc) 
                vl = EvalExprTree(ns, tree->left, n);
            else
                vr = EvalExprTree(ns, tree->right, n);
        }
        else
        {
            vl = EvalExprTree(ns, tree->left, n);
            vr = EvalExprTree(ns, tree->right, n);
        }
        switch (tree->type)
        {
            case '+': return vl + vr;
            case '-': return vl - vr;
            case '*': return vl * vr;
            case '/': return (vr != 0 ? vl / vr : 1);
            case '%': return (vr != 0 ? vl % vr : 1);
            case '|': return vl | vr;
            case '&': return vl & vr;
            case '^': return vl ^ vr;
            case ':': return vc ? vl : vr; 
        }
    }
    return 0;
}

PUBLIC DBnamescheme *
DBMakeNamescheme(char const *fmt, ...)
{
    va_list ap;
    int i, j, k, n, pass, ncspecs, done, saved_narrefs;
    DBnamescheme *rv = 0;
    DBfile *dbfile = 0;
    char const *relpath = 0;

    /* We have nothing to do for a null or empty format string */
    if (fmt == 0 || *fmt == '\0')
        return 0;

    /* Start by allocating an empty name scheme */
    rv = DBAllocNamescheme();

    // set the delimeter character
    n = 0;
    while (fmt[n] != '\0')
    {
        if (fmt[n] == '%' && fmt[n+1] != '%')
            break;
        n++;
    }
    if (fmt[n] == '%') // have at least one conversion spec
        rv->delim = fmt[0];
    else
        rv->delim = '\0';
    
    /* compute length up to max of 4096 of initial segment of fmt representing
       the printf-style format string. */
    n = 1;
    while (n < 4096 && fmt[n] != '\0' && fmt[n] != rv->delim)
        n++;
    if (n == 4096) /* we pick arb. upper bound in length of 4096 */
    {
        DBFreeNamescheme(rv);
        return 0;
    }

    /* grab just the part of fmt that is the printf-style format string */
    rv->fmt = STRNDUP(&fmt[1],n-1);
    rv->fmtlen = n-1;

    /* In 2 passes, count conversion specs. and then setup pointers to each */ 
    for (pass = 0; pass < 2; pass++)
    {
        if (pass == 1)
        {
            rv->fmtptrs = (const char **) calloc(rv->ncspecs+1, sizeof(char*));
            rv->ncspecs = 0;
        }
        for (i = 0; i < rv->fmtlen-1; i++)
        {
            if (rv->fmt[i] == '%' && 
                rv->fmt[i+1] != '%')
            {
                if (pass == 1)
                    rv->fmtptrs[rv->ncspecs] = &(rv->fmt[i]);
                rv->ncspecs++;
            }
        }
    }
    rv->fmtptrs[rv->ncspecs] = &(rv->fmt[n+1]);

    /* If there are no conversion specs., we have nothing to do.
       However, we now have a potential problem too. We do not know
       for sure if the first (and last) character is a (unnecessary)
       delimiter character or part of the actual namescheme. So, we
       check if the string is 3 or more characters in length and if
       the first and last characters are the same and if they are not
       any of the characters part of a "valid" Silo name. If all of
       that is true, we treat the first and last characters as
       (unnecessary) delimiter characters and remove them. Otherwise,
       we assume no delimter characters are present and keep everything.
    */
    if (rv->ncspecs == 0)
    {
        free(rv->fmt);

        /* If whole string is valid, take all of it. */
        if (db_VariableNameValid(fmt))
        {
            rv->fmt = STRNDUP(&fmt[0],n);
            rv->fmtlen = n;
            return rv;
        }

        /* If whole string but first char is valid, take all but first char */
        if (db_VariableNameValid(&fmt[1]))
        {
            rv->fmt = STRNDUP(&fmt[1],n-1);
            rv->fmtlen = n-1;
            return rv;
        }

        if (fmt[0] == fmt[n-1])
        {
            rv->fmt = STRNDUP(&fmt[1],n-2);
            rv->fmtlen = n-2;
        }
        else
        {
            rv->fmt = STRNDUP(&fmt[0],n-1);
            rv->fmtlen = n-1;
        }

        return rv;
    }

    /* Make a pass through rest of fmt string to count array refs in the
       expression substrings. */
    i = n+1;
    while (i < 4096 && fmt[i] != '\0')
    {
        if (fmt[i] == '$' || fmt[i] == '#')
            rv->narrefs++;
        i++;
    }
    if (i == 4096)
    {
        DBFreeNamescheme(rv);
        return 0;
    }

    /* allocate various arrays needed by the naming scheme */
    rv->exprstrs = (char **) calloc(rv->ncspecs, sizeof(char*));
    if (rv->narrefs > 0)
    {
        void *dummy;

        rv->arrnames = (char **) calloc(rv->narrefs, sizeof(char*));
        rv->arrvals  = (void **) calloc(rv->narrefs, sizeof(void*));
        rv->arrsizes =   (int *) calloc(rv->narrefs, sizeof(int));

        /* If we have non-zero ext. array references, then we may have the case of
           '0, DBfile*'. So, check for that now */
        va_start(ap, fmt);
        dummy = va_arg(ap, void *);
        if (dummy == 0)
        {
            dbfile = va_arg(ap, DBfile *);
            relpath = va_arg(ap, char const *);
            rv->arralloc = 1;
        }
        va_end(ap);
    }

    /* Ok, now go through rest of fmt string a second time and grab each
       expression that goes with each conversion spec. Also, handle array refs */
    i = n+1;
    saved_narrefs = rv->narrefs;
    rv->narrefs = 0;
    ncspecs = 0;
    va_start(ap, fmt);
    done = 0;
    while (!done)
    {
        if (fmt[i] == '$' || fmt[i] == '#')
        {
            for (j = 1; fmt[i+j] != '[' && 
                        fmt[i+j] != '#' &&
                        fmt[i+j] != '$' &&
                        fmt[i+j] != '%' &&
                        fmt[i+j] != '*' &&
                        fmt[i+j] != '+' &&
                        fmt[i+j] != '\0' ; j++)
                ;
            if (fmt[i+j] != '[')
            {
                DBFreeNamescheme(rv);
                rv = 0;
                done = 1;
                continue;
            }
            for (k = 0; k < rv->narrefs; k++)
            {
                if (strncmp(&fmt[i+1],rv->arrnames[k],j-1) == 0)
                    break;
            }
            if (k < rv->narrefs) /* this ext. array name has already been */
            {                    /* seen and is being used multiple times. */
                saved_narrefs--;
            }
            else
            {
                rv->arrnames[k] = STRNDUP(&fmt[i+1], j-1);
                if (!dbfile)
                {
                    rv->arrvals[k] = va_arg(ap, void *);
                }
                else
                {
                    char *arrnm = relpath?db_absoluteOf_path(relpath, rv->arrnames[k]):rv->arrnames[k];
                    if (DBInqVarExists(dbfile, arrnm))
                        rv->arrvals[k] = DBGetVar(dbfile, arrnm);
                    if (rv->arrvals[k] != 0)
                    {
                        /* Handle ext. array refs to arrays of strings */
                        if (DBGetVarType(dbfile, arrnm) == DB_CHAR)
                        {
                            char **tmp = NULL;
                            rv->arrsizes[k] = -1; /* initialize to 'unknown size' */
                            tmp = DBStringListToStringArray((char*)rv->arrvals[k], &(rv->arrsizes[k]), 0);
                            FREE(rv->arrvals[k]);
                            rv->arrvals[k] = tmp;
                        }
                    }
                    else
                    {
                        DBFreeNamescheme(rv);
                        rv = 0;
                        done = 1;
                        continue;
                    }
                    if (relpath) free(arrnm);
                }
                if (rv && !done) rv->narrefs++; /* rv could have been set to null, above */
            }
        }
        else if (fmt[i] == rv->delim || fmt[i] == '\0')
        {
            rv->exprstrs[ncspecs] = STRNDUP(&fmt[n+1],i-(n+1));
            ncspecs++;
            if ((fmt[i] == '\0') ||
                (fmt[i] == rv->delim && fmt[i+1] == '\0'))
                done = 1;
            n = i;
        }
        i++;
    }
    va_end(ap);

    if (rv && rv->narrefs != saved_narrefs)
    {
        DBFreeNamescheme(rv);
        rv = 0;
    }

    for (i = 0; rv && i < rv->ncspecs; i++)
    {
        if (!rv->exprstrs[i])
        {
            DBFreeNamescheme(rv);
            rv = 0;
            break;
        }
    }

    return rv;
}

PUBLIC const char *
DBGetName(DBnamescheme const *ns, long long natnum)
{
    char *currentExpr, *tmpExpr;
    char retval[1024];
    int i;

    /* a hackish way to cleanup the saved returned string buffer */
    if (ns == 0 && natnum == -1) return SaveReturnedString(0);
    if (ns == 0) return SaveReturnedString("");

    if (!ns->fmt) return "";

    retval[0] = '\0';
    strncat(retval, ns->fmt, ns->fmtptrs[0] - ns->fmt);
    for (i = 0; i < ns->ncspecs; i++)
    {
        char tmp[256];
        char tmpfmt[256] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        DBexprnode *exprtree;
        long long theVal;

        currentExpr = STRDUP(ns->exprstrs[i]);
        tmpExpr = currentExpr;
        exprtree = BuildExprTree((const char **)&currentExpr);
        theVal = EvalExprTree(ns, exprtree, natnum);
        FreeTree(exprtree);
        strncpy(tmpfmt, ns->fmtptrs[i], ns->fmtptrs[i+1] - ns->fmtptrs[i]);
        if (strncmp(tmpfmt, "%s", 2) == 0 && 0 <= theVal && theVal < DB_MAX_EXPNS)
            sprintf(tmp, tmpfmt, DBGetName(ns->embedns[theVal],natnum));
        else
            sprintf(tmp, tmpfmt, theVal);
        strcat(retval, tmp);
        FREE(tmpExpr);
    }
    return SaveReturnedString(retval);
}

PUBLIC long long
DBGetIndex(char const *dbns_name_str, int fieldSelector, int minFieldWidth, int base)
{
    int currentField = 0;
    char const *currentPosition = dbns_name_str;
    char const *lastPosition = currentPosition + strlen(dbns_name_str);
    char *endPtr;

    /* Note: leading zeros, which are probably quite common in fields in
       nameschemes will get treated automatically as octal (base 8) if base
       is not explicitly specified by caller. */

    while (currentPosition < lastPosition)
    {
        long long mult = 1;
        long long value;

        /* skip forward over chars that cannot be part of a number */
        if (!strchr("+-0123456789ABCDEFabcdefxX", *currentPosition))
        {
            currentPosition++;
            continue;
        }

        /* walk over any sign if present */
        if (*currentPosition == '-')
        {
            mult = -1;
            currentPosition++;
        }
        else if (*currentPosition == '+')
            currentPosition++;

        /* Try to detect unconventional '0b' base-2 designator */
        if (base == 0 && (!strncmp(currentPosition,"0b0",3) || !strncmp(currentPosition,"0b1",3)))
        {
            currentPosition += 2;
            base = 2; /* explicitly set base to 2 */
        }
        
        /* Attempt a conversion. A successful conversion implies its a field. */
        errno = 0;
        value = strtoll(currentPosition, &endPtr, base);
        if (errno == 0 && endPtr >= currentPosition + minFieldWidth)
        {
            if (currentField == fieldSelector)
                return mult * value;
            currentPosition = endPtr;
            currentField++;
        }
        else
            currentPosition++;
    }

    return LLONG_MAX;
}

PUBLIC char const *
DBSPrintf(char const *fmt, ...)
{
    static char strbuf[2048];
    static size_t const nmax = sizeof(strbuf);
    va_list ap;
    int n, en;

    if (!fmt) return SaveReturnedString(0);

    va_start(ap, fmt);
    n = vsnprintf(strbuf, nmax, fmt, ap);
    en = errno;
    va_end(ap);

    if (n < 0)
        snprintf(strbuf, nmax, "DBsprintf_failed_with_error_%s", strerror(en));

    return SaveReturnedString(strbuf);
}
