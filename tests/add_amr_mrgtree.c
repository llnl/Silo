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
   WARNING, YOU ALSO NEED THE EXPAT XML PARSER LIBRARY TO COMPILE THIS CODE
   You can find expat here... http://expat.sourceforge.net/
*/

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
extern int errno;

#include <assert.h>
#if !defined(NDEBUG) && !defined(DEBUG)
#undef assert
#define assert(a)
#endif

#include <expat.h>

#include <silo.h>


typedef enum _xmlParseState_t
{
    None = 0,
    AMRDecomp,
    Level,
    Patch
} xmlParseState_t;

typedef struct patchInfo_t
{
    int   idx;
    int   vid;
    double rank;
    int level;
    int logSize[3];
    int logExtents[6];
    double spatExtents[6];
    int numChildren;
    int *children;
} patchInfo_t;

typedef struct amrConfig_t
{
    char *meshName;
    int numDims;
    int numLevels;
    int numPatches;
    double dx, dy, dz; /* all -1 ==> curvilinear */

    xmlParseState_t currentState;
    int currentLevel;
    int currentPatchOnLevel;
    int currentChildForPatch;

    int *numPatchesOnLevel;
    int **levels;
    int *ratios;

    patchInfo_t *patches;
    int currentPatch;
} amrConfig_t;

static void freeAmrConf(amrConfig_t *amrconf)
{
    int i;
    free(amrconf->meshName);
    for (i = 0; i < amrconf->numLevels; i++)
        free(amrconf->levels[i]);
    free(amrconf->levels);
    for (i = 0; i < amrconf->numPatches; i++)
        free(amrconf->patches[i].children);
    free(amrconf->numPatchesOnLevel);
    free(amrconf->ratios);
    free(amrconf->patches);
}

#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#define XML_FMT_INT_MOD "I64"
#else
#define XML_FMT_INT_MOD "ll"
#endif
#else
#define XML_FMT_INT_MOD "l"
#endif

#define BUFFSIZE        8192
char Buff[BUFFSIZE];

#define INT     ((int)'i')
#define DBL     ((int)'d')
#define STR     ((int)'s'), 1
#define EOA     "EOA"

static int
getAttrVals(const char **attrs, ...)
{
    int i, j, nvals;
    const char *attrName, *p;
    char *pend;
    int type;
    va_list ap;

    va_start(ap, attrs);
    while (1)
    {
        attrName = va_arg(ap, const char *);
        if (strncmp(attrName, EOA, 3) == 0)
            break;

        /* find this attr's name in the list of xml attrs */
        for (i = 0; attrs[i]; i += 2)
        {
            if (strcmp(attrs[i], attrName) == 0)
                break;
        }
        if (!attrs[i])
            break;

        type = va_arg(ap, int);
        nvals = va_arg(ap, int);
        p = attrs[i+1];
        switch (type)
        {
            case 'i':
            {
                int *ptr = va_arg(ap, int *);
                for (j = 0; j < nvals; j++)
                {
                    ptr[j] = strtol(p,&pend,10);
                    p = pend;
                }
                break;
            }
            case 'd':
            {
                double *ptr = va_arg(ap, double *);
                for (j = 0; j < nvals; j++)
                {
                    ptr[j] = strtod(p,&pend);
                    p = pend;
                }
                break;
            }
            case 's':
            {
                char **ptr = va_arg(ap, char **);
                *ptr = strdup(attrs[i+1]); 
                break;
            }
            default: return -1;
        }
    }

    va_end(ap);
    return 0;
}

static void XMLCALL
start(void *data, const char *el, const char **attr)
{
    int i;
    amrConfig_t *ud = (amrConfig_t *) data;
    
    if      (strcmp(el, "AMRDecomp") == 0)
    {
        getAttrVals(attr, "meshName", STR, &ud->meshName,
                          "numDims", INT, 1, &ud->numDims,
                          "numLevels", INT, 1, &ud->numLevels,
                          "numPatches", INT, 1, &ud->numPatches,
                    EOA);
        ud->currentLevel = -1;
        ud->levels = (int **) malloc(ud->numLevels * sizeof(int*));
        ud->ratios = (int *) malloc(ud->numLevels * ud->numDims * sizeof(int));
        ud->numPatchesOnLevel = (int *) malloc(ud->numLevels * sizeof(int));
        ud->patches = (patchInfo_t *) malloc(ud->numPatches * sizeof(patchInfo_t));
        ud->currentState = AMRDecomp;
    }
    else if (strcmp(el, "Level") == 0)
    {
        int cl;
        ud->currentLevel++;
        cl = ud->currentLevel;
        getAttrVals(attr, "numPatches", INT, 1, &(ud->numPatchesOnLevel[cl]),
                          "ratios", INT, ud->numDims, &(ud->ratios[ud->numDims*cl]),
                    EOA);
        ud->levels[cl] = (int *) malloc(ud->numPatchesOnLevel[cl] * sizeof(int));
        ud->currentPatchOnLevel = 0;
        ud->currentPatch = -1;
        ud->currentState = Level;
    }
    else if (strcmp(el, "Patch") == 0)
    {
        ud->currentPatch++;
        patchInfo_t *p = &(ud->patches[ud->currentPatch]);
        getAttrVals(attr, "iDx", INT, 1, &(p->idx),
                          "vId", INT, 1, &(p->vid),
                          "level", INT, 1, &(p->level),
                          "rank", DBL, 1, &(p->rank),
                          "numChildren", INT, 1, &(p->numChildren),
                          "logSize", INT, ud->numDims, &(p->logSize),
                          "logExtents", INT, 2*(ud->numDims), &(p->logExtents),
                          "spatExtents", DBL, 2*(ud->numDims), &(p->spatExtents),
                    EOA);
        if (p->numChildren == 0)
            p->children = 0;
        else
            p->children = (int *) malloc(p->numChildren * sizeof(int));
        ud->currentChildForPatch = 0;
        ud->currentState = Patch;
    } 
}

static void XMLCALL
end(void *data, const char *el)
{
}

static void XMLCALL
cdstart(void *userData)
{
}

static void XMLCALL
cdend(void *userData)
{
}

static int
readVals(const char *s, int type, void *pvals)
{
    const char *p1;
    char *p2;
    int n, done;

    p1 = s;
    n = 0;
    done = 0;
    errno = 0;
    while (!done)
    {
        switch (type)
        {
            case 'i':
            {
                int *iptr = (int *) pvals;
                int ival = strtol(p1, &p2, 10);
                if (ival == 0 && p1 == p2 || errno != 0)
                {
                    done = 1;
                    break;
                }
                if (*p2 == '\0')
                    done = 1;
                if (iptr) iptr[n] = ival;
                break;
            }
            case 'd':
            {
                double *dptr = (double *) pvals;
                double dval = strtod(p1, &p2);
                if (dval == 0 && p1 == p2 || errno != 0)
                {
                    done = 1;
                    break;
                }
                if (*p2 == '\0')
                    done = 1;
                if (dptr) dptr[n] = dval;
                break;
            }
        }
        p1 = p2;
        if (!done) n++;
    }
    return n;
}

static void XMLCALL
chardata(void *userData, const XML_Char *s, int len)
{
    amrConfig_t *ud = (amrConfig_t *) userData;
    int i, val, done;
    char *s1, *p1, *p2;

    if (len == 0)
        return;

    s1 = (char*) malloc(len+1);
    strncpy(s1, s, len);
    s1[len] = '\0';

    /* if its all whitespace, ignore it */
    if (strspn(s1, " \\\f\\\n\\\r\\\t\\\v") == len)
    {
        free(s1);
        return;
    }

    switch (ud->currentState)
    {
        case Level:
        {
            int cl = ud->currentLevel;
            int cp = ud->currentPatchOnLevel;
            int *levels = &(ud->levels[cl][cp]);
            int npatches = readVals(s1, INT, levels);
            int i;
            for (i = 0; i < npatches; i++)
                printf("ud->levels[%d][%d]=%d\n", cl, cp+i, ud->levels[cl][cp+i]);
            ud->currentPatchOnLevel += npatches;
            break;
        }
        case Patch:
        {
            int cp = ud->currentPatch;
            int cc = ud->currentChildForPatch;
            int *children = &(ud->patches[cp].children[cc]);
            int nchildren = readVals(s1, INT, children);
            int i;
            for (i = 0; i < nchildren; i++)
                printf("ud->patches[%d].children[%d]=%d\n", cp, cc+i, ud->patches[cp].children[cc+i]);
            ud->currentChildForPatch += nchildren;
            break;
        }
    }

    free(s1);
}

int ProcessXMLAMRConfigFile(const char *xmlFileName, amrConfig_t *amrconfig)
{
    FILE *acf = fopen(xmlFileName, "r");
    XML_Parser p = XML_ParserCreate(NULL);
    if (!p)
        return -1;

    XML_SetUserData(p, amrconfig);
    XML_SetElementHandler(p, start, end);
    XML_SetCdataSectionHandler(p, cdstart, cdend);
    XML_SetCharacterDataHandler(p, chardata);

    for (;;) {
        int done;
        int len;

      len = (int)fread(Buff, 1, BUFFSIZE, acf);
      if (ferror(acf))
          return -1;
      done = feof(acf);

      if (XML_Parse(p, Buff, len, done) == XML_STATUS_ERROR) {
        fprintf(stderr, "Parse error at line %" XML_FMT_INT_MOD "u:\n%s\n",
              XML_GetCurrentLineNumber(p),
              XML_ErrorString(XML_GetErrorCode(p)));
          return -1;
      }

      if (done)
          break;
    }
    XML_ParserFree(p);
    fclose(acf);
    return 0;
}

/*-------------------------------------------------------------------------
 * Function: main
 *
 * Purpose: Add an mrgtree object to an existing silo file containing a
 * multi-mesh object whose individual pieces form the patches of some AMR
 * mesh.
 *
 * Return:      0
 *
 * Programmer:  Mark C. Miller, Wed Aug 27 09:17:38 PDT 2008
 *
 *-------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
    int i;
    int copy = 1;
    DBfile *dbfile = 0;
    DBmrgtree *mrgTree;
    DBmultimesh *mm;
    DBoptlist *optList;
    char *siloFileName = 0;
    char *amrconfigFileName = "amr_config.xml";
    amrConfig_t amrconf;
    char tmpName[256];
    char lvlMapsName[256];
    char chldMapsName[256];
    int show_all_errors = FALSE;

    assert(copy==0);

    /* Parse command-line */
    for (i=1; i<argc; i++)
    {
        if (!strcmp(argv[i], "-nocp"))
        {
            copy = 0;
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
	}
        else if (siloFileName == 0)
        {
            siloFileName = strdup(argv[i]);
        }
        else
        {
            amrconfigFileName = strdup(argv[i]);
        }
    }

    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ALL, NULL);


    /* by default, we make a copy of the specified file */
    if (copy)
    {
        char syscmd[256];

        /* copy the file in the filesystem */
        snprintf(syscmd, sizeof(syscmd), "cp -f %s %s_wmrgtree", siloFileName, siloFileName);
        system(syscmd);

        /* use syscmd as buffer for constructing new filename */
        snprintf(syscmd, sizeof(syscmd), "%s_wmrgtree", siloFileName);
        free(siloFileName);
        siloFileName = strdup(syscmd);
    }

    /* open and process the amr configuration file */
    ProcessXMLAMRConfigFile(amrconfigFileName, &amrconf);

    /* open the silo file */
    dbfile = DBOpen(siloFileName, DB_UNKNOWN, DB_APPEND);

    /* get the specific multi-mesh object we want to add an mrg tree too */
    mm = DBGetMultimesh(dbfile, amrconf.meshName);
    sprintf(tmpName, "%s_wmrgtree", amrconf.meshName);
    optList = DBMakeOptlist(10);
    char *foo = "mrgTree";
    DBAddOption(optList, DBOPT_MRGTREE_NAME, foo);
    DBPutMultimesh(dbfile, tmpName, mm->nblocks, mm->meshnames, mm->meshtypes, optList);
    DBClearOptlist(optList);
    DBFreeMultimesh(mm);

#ifndef _WIN32
#warning HACK FOR SINGLE VARIABLE
#endif
    {
        DBmultivar *mv = DBGetMultivar(dbfile, "Density");
        DBoptlist *optList2 = DBMakeOptlist(10);
        DBAddOption(optList2, DBOPT_MMESH_NAME, tmpName);
        DBPutMultivar(dbfile, "foo_wmrgtree", mv->nvars, mv->varnames, mv->vartypes, optList2);
        DBFreeOptlist(optList2);
        DBFreeMultivar(mv);
    }
    
    /* write this multi-mesh object back to the file, with a different name
       and attache the mrg tree name to it */

    /* Write the groupel maps that specify which blocks of the multi-block mesh
       belong to which levels */
    {
        int *segTypes = (int *) malloc(amrconf.numLevels * sizeof(int));
        for (i = 0; i < amrconf.numLevels; i++)
            segTypes[i] = DB_BLOCKCENT;
        sprintf(lvlMapsName, "%s_wmrgtree_lvlMaps", amrconf.meshName);
        DBPutGroupelmap(dbfile, lvlMapsName, amrconf.numLevels, segTypes, amrconf.numPatchesOnLevel,
            0, amrconf.levels, 0, 0, 0); 
        free(segTypes);
    }

    /* Write the groupel maps that specify which blocks are children (refinements)
       of other blocks */
    {
        int *segTypes = (int *) malloc(amrconf.numPatches * sizeof(int));
        int *segLens = (int *) malloc(amrconf.numPatches * sizeof(int));
        int **segData = (int **) malloc(amrconf.numPatches * sizeof(int*));
        for (i = 0; i < amrconf.numPatches; i++)
        {
            segTypes[i] = DB_BLOCKCENT;
            segLens[i] = amrconf.patches[i].numChildren;
            segData[i] = amrconf.patches[i].children;
        }
        sprintf(chldMapsName, "%s_wmrgtree_chldMaps", amrconf.meshName);
        DBPutGroupelmap(dbfile, chldMapsName, amrconf.numPatches, segTypes, segLens,
            0, segData, 0, 0, 0); 
        free(segTypes);
        free(segLens);
        free(segData);
    }

    /* Create an mrg tree for inclusion in the file */
    mrgTree = DBMakeMrgtree(DB_MULTIMESH, 0, 2, 0);

    /* Add a region for AMR configuration */
    DBAddRegion(mrgTree, "amr_decomp", 0, 2, 0, 0, 0, 0, 0, 0); 
    DBSetCwr(mrgTree, "amr_decomp");
    DBAddRegion(mrgTree, "levels", 0, amrconf.numLevels, 0, 0, 0, 0, 0, 0); 
    DBSetCwr(mrgTree, "levels");

    /* Handle the regions representing each level in the AMR mesh */
    {
        char *levelRegnNames[1];
        int *segTypes = (int *) malloc(amrconf.numLevels * sizeof(int));
        int *segIds = (int *) malloc(amrconf.numLevels * sizeof(int));
        for (i = 0; i < amrconf.numLevels; i++)
        {
            segIds[i] = i;
            segTypes[i] = DB_BLOCKCENT;
        }
        levelRegnNames[0] = "@level%d@n";
        DBAddRegionArray(mrgTree, amrconf.numLevels, levelRegnNames, 0, lvlMapsName, 1,
            segIds, amrconf.numPatchesOnLevel, segTypes, 0);
        free(segTypes);
        free(segIds);
    }
    DBSetCwr(mrgTree, "..");

    DBAddRegion(mrgTree, "patches", 0, amrconf.numPatches, 0, 0, 0, 0, 0, 0); 
    DBSetCwr(mrgTree, "patches");

    /* Handle the regions representing each patch */
    {
        char *patchRegnNames[1];
        int *segTypes = (int *) malloc(amrconf.numPatches * sizeof(int));
        int *segIds = (int *) malloc(amrconf.numPatches * sizeof(int));
        int *segLens = (int *) malloc(amrconf.numPatches * sizeof(int));
        for (i = 0; i < amrconf.numPatches; i++)
        {
            segIds[i] = i;
            segTypes[i] = DB_BLOCKCENT;
            segLens[i] = amrconf.patches[i].numChildren;
        }
        patchRegnNames[0] = "@patch%d@n";
        DBAddRegionArray(mrgTree, amrconf.numPatches, patchRegnNames, 0, chldMapsName, 1,
            segIds, segLens, segTypes, 0);
        free(segTypes);
        free(segIds);
        free(segLens);
    }

    {
        char *mrgv_onames[5];
        sprintf(tmpName, "%s_wmrgtree_lvlRatios", amrconf.meshName);
        mrgv_onames[0] = strdup(tmpName);
        sprintf(tmpName, "%s_wmrgtree_ijkExts", amrconf.meshName);
        mrgv_onames[1] = strdup(tmpName);
        sprintf(tmpName, "%s_wmrgtree_xyzExts", amrconf.meshName);
        mrgv_onames[2] = strdup(tmpName);
        mrgv_onames[3] = strdup("rank");
        mrgv_onames[4] = 0;

        DBAddOption(optList, DBOPT_MRGV_ONAMES, mrgv_onames);
        DBPutMrgtree(dbfile, "mrgTree", "amr_mesh", mrgTree, optList);
        DBFreeMrgtree(mrgTree);
        free(mrgv_onames[0]);
        free(mrgv_onames[1]);
        free(mrgv_onames[2]);
        free(mrgv_onames[3]);
    }

    /* Output level refinement ratios as an mrg variable on the array of regions
       representing the levels */
    {
        char *compnames[3] = {"iRatio","jRatio","kRatio"};
        char *levelRegnNames[1];
        int *data[3];
        for (i = 0; i < amrconf.numDims; i++)
            data[i] = (int *) malloc(amrconf.numLevels * sizeof(int));
        for (i = 0; i < amrconf.numLevels; i++)
        {
            data[0][i] = amrconf.ratios[i*amrconf.numDims+0];
            data[1][i] = amrconf.ratios[i*amrconf.numDims+1];
            if (amrconf.numDims == 3)
                data[2][i] = amrconf.ratios[i*amrconf.numDims+2];
        }
        levelRegnNames[0] = "@level%d@n";
        sprintf(tmpName, "%s_wmrgtree_lvlRatios", amrconf.meshName);
        DBPutMrgvar(dbfile, tmpName, "mrgTree", amrconf.numDims,
            compnames, amrconf.numLevels, levelRegnNames, DB_INT, (void**)data, 0);
        for (i = 0; i < amrconf.numDims; i++)
            free(data[i]);
    }

    /* Output logical extents of the patches as an mrg variable on the
       array of regions representing the patches */
    {
        char *compnames[6] = {"iMin","iMax","jMin","jMax","kMin","kMax"};
        char *scompnames[6] = {"xMin","xMax","yMin","yMax","zMin","zMax"};
        char *patchRegnNames[1];
        int *data[6];
        float *rdata[1];
        float *sdata[6];
        patchRegnNames[0] = "@patch%d@n";
        for (i = 0; i < 2*amrconf.numDims; i++)
        {
            data[i] = (int *) malloc(amrconf.numPatches * sizeof(int));
            sdata[i] = (float *) malloc(amrconf.numPatches * sizeof(float));
        }
        rdata[0] = (float *) malloc(amrconf.numPatches * sizeof(float));
        for (i = 0; i < amrconf.numPatches; i++)
        {
            data[0][i] = amrconf.patches[i].logExtents[0];
            data[1][i] = amrconf.patches[i].logExtents[1];
            data[2][i] = amrconf.patches[i].logExtents[2];
            data[3][i] = amrconf.patches[i].logExtents[3];
            sdata[0][i] = amrconf.patches[i].spatExtents[0];
            sdata[1][i] = amrconf.patches[i].spatExtents[1];
            sdata[2][i] = amrconf.patches[i].spatExtents[2];
            sdata[3][i] = amrconf.patches[i].spatExtents[3];

            if (amrconf.numDims == 3)
            {
                data[4][i] = amrconf.patches[i].logExtents[4];
                data[5][i] = amrconf.patches[i].logExtents[5];
                sdata[4][i] = amrconf.patches[i].spatExtents[4];
                sdata[5][i] = amrconf.patches[i].spatExtents[5];
            }
            rdata[0][i] = amrconf.patches[i].rank;
        }
        sprintf(tmpName, "%s_wmrgtree_ijkExts", amrconf.meshName);
        DBPutMrgvar(dbfile, tmpName, "mrgTree", 2*amrconf.numDims,
            compnames, amrconf.numPatches, patchRegnNames, DB_INT, (void**)data, 0);
        sprintf(tmpName, "%s_wmrgtree_xyzExts", amrconf.meshName);
        DBPutMrgvar(dbfile, tmpName, "mrgTree", 2*amrconf.numDims,
            scompnames, amrconf.numPatches, patchRegnNames, DB_FLOAT, (void**)sdata, 0);
        for (i = 0; i < 2*amrconf.numDims; i++)
        {
            free(data[i]);
            free(sdata[i]);
        }
        DBPutMrgvar(dbfile, "rank", "mrgTree", 1, 0,
            amrconf.numPatches, patchRegnNames, DB_FLOAT, (void**)rdata, 0);
        free(rdata[0]);
    }

    DBClose(dbfile);
    DBFreeOptlist(optList);
    freeAmrConf(&amrconf);
    free(amrconfigFileName);
    free(siloFileName);

    return (0);
}
