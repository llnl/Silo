/*
 * all_silo_objects.c
 *
 * Create one small instance of each major DBPut... object type in Silo.
 *
 * Purpose:
 *   Security/robustness test input.  The resulting file is intended to be
 *   copied and deliberately corrupted so DBGet... readers can be tested
 *   against malformed objects.
 *
 * Integer payloads deliberately use recognizable octal literal families
 * (010xx, 020xx, ...).  With a PDB file, commands such as
 *
 *     od -A x -t o4 all_objects.pdb | less
 *
 * can make selected integer payloads relatively easy to spot.
 *
 * This is NOT intended to describe a mutually consistent simulation.
 * Individual objects are merely kept internally valid enough for DBPut...
 * to accept them.
 *
 * Build example (adjust include/library paths as needed):
 *
 *     cc -I/path/to/silo/include all_silo_objects.c \
 *        -L/path/to/silo/lib -lsilo -lm -o all_silo_objects
 *
 * Usage:
 *
 *     ./all_silo_objects              # PDB, all_objects.pdb
 *     ./all_silo_objects pdb          # PDB
 *     ./all_silo_objects hdf5         # HDF5, all_objects.h5
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "silo.h"

#define NELMTS(a) ((int)(sizeof(a)/sizeof((a)[0])))

static int failures = 0;

#define PUT(call)                                                        \
    do {                                                                 \
        int _r = (call);                                                 \
        printf("%-32s : %s (%d)\n", #call, _r < 0 ? "FAILED" : "ok", _r);\
        if (_r < 0) failures++;                                          \
    } while (0)

#define SET_DIR(DNAME)                                                   \
    do {                                                                 \
        DBSetDir(db, "/");                                               \
        DBMkDir(db, #DNAME);                                             \
        DBSetDir(db, #DNAME);                                            \
    } while (0)

/* ---------------------------------------------------------------------- */
/* Small 2-D quad mesh: 3x3 nodes => 2x2 = 4 zones.                       */
/* ---------------------------------------------------------------------- */
static void
put_quad_objects(DBfile *db)
{
    int dims_nodes[2] = {3, 3};
    int dims_zones[2] = {2, 2};
    float x[3] = {0.f, 1.f, 2.f};
    float y[3] = {0.f, 1.f, 2.f};
    void *coords[2] = {x, y};
    char *coordnames[2] = {"qx", "qy"};

    float qv0[4] = {1.f, 2.f, 3.f, 4.f};
    float qv1[4] = {5.f, 6.f, 7.f, 8.f};
    void *qvals[2] = {qv0, qv1};
    char *qnames[2] = {"qcomp0", "qcomp1"};

    float qone[4] = {11.f, 12.f, 13.f, 14.f};

    PUT(DBPutQuadmesh(db, "quadmesh",
                      (DBCAS_t)coordnames, coords, dims_nodes, 2,
                      DB_FLOAT, DB_COLLINEAR, NULL));

    PUT(DBPutQuadvar(db, "quadvar", "quadmesh", 2,
                     (DBCAS_t)qnames, qvals, dims_zones, 2,
                     NULL, 0, DB_FLOAT, DB_ZONECENT, NULL));

    PUT(DBPutQuadvar1(db, "quadvar_one", "quadmesh",
                      qone, dims_zones, 2, NULL, 0,
                      DB_FLOAT, DB_ZONECENT, NULL));
}

/* ---------------------------------------------------------------------- */
/* 2-D UCD mesh: 9 nodes and four quadrilateral zones.                    */
/* ---------------------------------------------------------------------- */
static void
put_ucd_objects(DBfile *db)
{
    float x[9] = {0,1,2, 0,1,2, 0,1,2};
    float y[9] = {0,0,0, 1,1,1, 2,2,2};
    void *coords[2] = {x,y};
    char *coordnames[2] = {"ux","uy"};

    int nodelist[16] = {
        0,1,4,3,
        1,2,5,4,
        3,4,7,6,
        4,5,8,7
    };

    int shapesize[1] = {4};
    int shapecnt[1] = {4};
    int shapetype[1] = {DB_ZONETYPE_QUAD};

    float u0[4] = {21.f,22.f,23.f,24.f};
    float u1[4] = {25.f,26.f,27.f,28.f};
    void *uvals[2] = {u0,u1};
    char *unames[2] = {"ucomp0","ucomp1"};
    float uone[4] = {31.f,32.f,33.f,34.f};

    PUT(DBPutZonelist(db, "zl_old",
                      4, 2, nodelist, 16, 0,
                      shapesize, shapecnt, 1));

    PUT(DBPutZonelist2(db, "zl2",
                       4, 2, nodelist, 16, 0,
                       0, 0, shapetype, shapesize, shapecnt, 1, NULL));

    PUT(DBPutUcdmesh(db, "ucdmesh", 2, (DBCAS_t)coordnames, coords,
                     9, 4, "zl2", NULL, DB_FLOAT, NULL));

    PUT(DBPutUcdsubmesh(db, "ucdsubmesh", "ucdmesh",
                        4, "zl2", NULL, NULL));

    PUT(DBPutUcdvar(db, "ucdvar", "ucdmesh", 2,
                    (DBCAS_t)unames, uvals, 4,
                    NULL, 0, DB_FLOAT, DB_ZONECENT, NULL));

    PUT(DBPutUcdvar1(db, "ucdvar_one", "ucdmesh",
                     uone, 4, NULL, 0, DB_FLOAT, DB_ZONECENT, NULL));
}

/* ---------------------------------------------------------------------- */
/* Point mesh and point variables.                                       */
/* ---------------------------------------------------------------------- */
static void
put_point_objects(DBfile *db)
{
    float x[5] = {0,1,2,3,4};
    float y[5] = {4,3,2,1,0};
    void *coords[2] = {x,y};

    float p0[5] = {41,42,43,44,45};
    float p1[5] = {46,47,48,49,50};
    void *pvals[2] = {p0,p1};
    char *pnames[2] = {"pcomp0","pcomp1"};
    float pone[5] = {51,52,53,54,55};

    PUT(DBPutPointmesh(db, "pointmesh", 2, coords, 5, DB_FLOAT, NULL));

    PUT(DBPutPointvar(db, "pointvar", "pointmesh", 2,
                      pvals, 5, DB_FLOAT, NULL));

    PUT(DBPutPointvar1(db, "pointvar_one", "pointmesh",
                       pone, 5, DB_FLOAT, NULL));
}

/* ---------------------------------------------------------------------- */
/* Material and material-species objects.                                */
/* ---------------------------------------------------------------------- */
static void
put_material_objects(DBfile *db)
{
    int dims[2] = {2,2};

    /* recognizable values in octal when dumped */
    int matnos[2] = {01001, 01002};
    int matlist[4] = {01001,01002,01001,01002};
    int matlist2[4] = {01001,-1,01001,-3};
    int mixlen = 4;
    float mix_vf[4] = {0.5, 0.5, 0.5, 0.5};
    int mix_mat[4] = {01001, 01002, 01001, 01002};
    int mix_next[4] = {1, 0, 3, 0};
    int mix_zone[4] = {1,1,3,3};
    char *matnames[2] = {"copper", "steel"};
    char *matcolors[2] = {"red", "blue"};
    DBoptlist *ol;
    int seven = 7;
    int int_data[7] = {1,1,1,1,1,1,1};

    /* basic material */
    PUT(DBPutMaterial(db, "material", "quadmesh",
                      2, matnos, matlist, dims, 2,
                      NULL,NULL,NULL,NULL,0,DB_FLOAT,NULL));

    /* material with mixing */
    PUT(DBPutMaterial(db, "material_mix", "quadmesh",
                      2, matnos, matlist, dims, 2,
                      mix_next,mix_mat,mix_zone,mix_vf,mixlen,DB_FLOAT,NULL));

    /* material with optional material names and colors */
    ol = DBMakeOptlist(10);
    DBAddOption(ol, DBOPT_MATNAMES, matnames);
    DBAddOption(ol, DBOPT_MATCOLORS, matcolors);
    PUT(DBPutMaterial(db, "material_names_colors", "quadmesh",
                      2, matnos, matlist, dims, 2,
                      NULL,NULL,NULL,NULL,0,DB_FLOAT,ol));
    DBFreeOptlist(ol);

    /* some extra data to use to spoil above objects from browser */
    DBWrite(db, "ed", int_data, &seven, 1, DB_INT);

    /*
     * Two materials; first has 1 species, second has 2.
     * speclist entries are valid positive 1-origin indices into species_mf.
     */
    {
        int nmatspec[2] = {1,2};
        int speclist[4] = {1,2,1,2};
        float species_mf[3] = {1.0f, 0.25f, 0.75f};

        PUT(DBPutMatspecies(db, "matspecies", "material",
                            2, nmatspec, speclist, dims, 2,
                            3, species_mf, NULL, 0, DB_FLOAT, NULL));
    }
}

/* ---------------------------------------------------------------------- */
/* Curve, compound array and defvars.                                    */
/* ---------------------------------------------------------------------- */
static void
put_simple_objects(DBfile *db)
{
    {
        float x[5] = {0,1,2,3,4};
        float y[5] = {0,1,4,9,16};
        PUT(DBPutCurve(db, "curve", x, y, DB_FLOAT, 5, NULL));
    }

    {
        char *names[3] = {"partA","partB","partC"};
        int lens[3] = {2,3,1};
        int vals[6] = {02001,02002, 02011,02012,02013, 02021};
        PUT(DBPutCompoundarray(db, "compound",
                               (DBCAS_t)names, lens, 3,
                               vals, 6, DB_INT, NULL));
    }

    {
        char *names[3] = {"dv0","dv1","dv2"};
        int types[3] = {DB_VARTYPE_SCALAR, DB_VARTYPE_SCALAR, DB_VARTYPE_SCALAR};
        char *defns[3] = {"quadvar1", "quadvar1+1", "quadvar1*2"};
        DBoptlist *opts[3] = {NULL,NULL,NULL};
        PUT(DBPutDefvars(db, "defvars", 3,
                         (DBCAS_t)names, types, (DBCAS_t)defns,
                         (DBoptlist const * const *)opts));
    }
}

/* ---------------------------------------------------------------------- */
/* Facelist: four 2-D edges.                                             */
/* ---------------------------------------------------------------------- */
static void
put_facelist(DBfile *db)
{
    int nodelist[8] = {0,1, 1,2, 2,3, 3,0};
    int shapesize[1] = {2};
    int shapecnt[1] = {4};
    int zoneno[4] = {03001,03002,03003,03004};

    PUT(DBPutFacelist(db, "facelist",
                      4, 2, nodelist, 8, 0, zoneno,
                      shapesize, shapecnt, 1,
                      NULL, NULL, 0));
}

/* ---------------------------------------------------------------------- */
/* One polyhedral hex zone.                                              */
/* ---------------------------------------------------------------------- */
static void
put_phzonelist(DBfile *db)
{
    /* Six quad faces. */
    int nodecnt[6] = {4,4,4,4,4,4};
    int nodelist[24] = {
        0,1,2,3,  /* bottom */
        4,7,6,5,  /* top */
        0,4,5,1,
        1,5,6,2,
        2,6,7,3,
        3,7,4,0
    };
    int facecnt[1] = {6};
    int facelist[6] = {0,1,2,3,4,5};

    PUT(DBPutPHZonelist(db, "phzl",
                        6, nodecnt, 24, nodelist, NULL,
                        1, facecnt, 6, facelist,
                        0, 0, 0, NULL));
}

/* ---------------------------------------------------------------------- */
/* Tiny CSG objects: one sphere, one region, one zone.                    */
/* ---------------------------------------------------------------------- */
static void
put_csg_objects(DBfile *db)
{
    int btypes[1] = {DBCSG_SPHERE_PR};
    float coeffs[4] = {0.f,0.f,0.f,1.f};
    double extents[6] = {-1,-1,-1,1,1,1};

    int rtypes[1] = {DBCSG_INNER};
    int left[1] = {0};
    int right[1] = {-1};
    int zones[1] = {0};

    double cval[1] = {61.0};
    void *vals[1] = {cval};
    char *names[1] = {"ccomp0"};

    PUT(DBPutCsgmesh(db, "csgmesh", 3, 1,
                     btypes, NULL, coeffs, 4, DB_FLOAT,
                     extents, "csgzl", NULL));

    PUT(DBPutCSGZonelist(db, "csgzl", 1,
                         rtypes, left, right,
                         NULL, 0, DB_INT,
                         1, zones, NULL));

    PUT(DBPutCsgvar(db, "csgvar", "csgmesh", 1,
                    (DBCAS_t)names, vals, 1,
                    DB_DOUBLE, DB_ZONECENT, NULL));
}

/* ---------------------------------------------------------------------- */
/* Groupel map, MRG tree and MRG variable.                               */
/* ---------------------------------------------------------------------- */
static void
put_mrg_objects(DBfile *db)
{
    /*
     * Three map segments with short data lists.  Payload families 040xx.
     */
    int groupel_types[3] = {DB_ZONECENT, DB_ZONECENT, DB_ZONECENT};
    int seglens[3] = {2,1,2};
    int segids[3] = {04001,04002,04003};

    int sd0[2] = {04011,04012};
    int sd1[1] = {04021};
    int sd2[2] = {04031,04032};
    int *segdata[3] = {sd0,sd1,sd2};

    float sf0[2] = {.25f,.75f};
    float sf1[1] = {1.f};
    float sf2[2] = {.5f,.5f};
    void *segfracs[3] = {sf0,sf1,sf2};

    PUT(DBPutGroupelmap(db, "groupelmap", 3,
                        groupel_types, seglens, segids,
                        (int const * const *)segdata,
                        segfracs, DB_FLOAT, NULL));

    /*
     * Minimal MRG tree.  The public API constructs the tree in memory;
     * DBPutMrgtree serializes it.
     */
    {
        DBmrgtree *tree = DBMakeMrgtree(DB_QUADMESH, 0, 4, NULL);
        if (!tree)
        {
            fprintf(stderr, "DBMakeMrgtree failed\n");
            failures++;
        }
        else
        {
            int seg_ids[2]   = {04001,04002};
            int seg_sizes[2] = {2,1};
            int seg_types[2] = {DB_ZONECENT,DB_ZONECENT};

            if (DBAddRegion(tree, "region0", 0, 0, "groupelmap",
                            2, seg_ids, seg_sizes, seg_types, NULL) < 0)
            {
                fprintf(stderr, "DBAddRegion failed\n");
                failures++;
            }

            PUT(DBPutMrgtree(db, "mrgtree", "quadmesh", tree, NULL));
            DBFreeMrgtree(tree);
        }
    }

    {
        char *compnames[2] = {"mrgcomp0","mrgcomp1"};
        char *regnames[3] = {"region0","region1","region2"};
        int d0[3] = {05001,05002,05003};
        int d1[3] = {05011,05012,05013};
        void *data[2] = {d0,d1};

        PUT(DBPutMrgvar(db, "mrgvar", "mrgtree",
                        2, (DBCAS_t)compnames,
                        3, (DBCAS_t)regnames,
                        DB_INT, data, NULL));
    }
}

/* ---------------------------------------------------------------------- */
/* Multiblock objects.  Constituent names need not describe a coherent    */
/* simulation; they merely reference objects written elsewhere here.      */
/* ---------------------------------------------------------------------- */
static void
put_multi_objects(DBfile *db)
{
    {
        char *names[3] = {"quadmesh","ucdmesh","pointmesh"};
        int types[3] = {DB_QUADMESH,DB_UCDMESH,DB_POINTMESH};
        PUT(DBPutMultimesh(db, "multimesh", 3,
                           (DBCAS_t)names, types, NULL));
    }

    {
        char *names[3] = {"quadvar1","ucdvar1","pointvar1"};
        int types[3] = {DB_QUADVAR,DB_UCDVAR,DB_POINTVAR};
        PUT(DBPutMultivar(db, "multivar", 3,
                          (DBCAS_t)names, types, NULL));
    }

    {
        char *names[2] = {"material","material"};
        PUT(DBPutMultimat(db, "multimat", 2, (DBCAS_t)names, NULL));
    }

    {
        char *names[2] = {"matspecies","matspecies"};
        PUT(DBPutMultimatspecies(db, "multimatspecies",
                                 2, (DBCAS_t)names, NULL));
    }

    /*
     * Two blocks, each adjacent to the other.  One shared-node entry
     * in each direction.  060xx makes the flattened node-list easy to find.
     */
    {
        int meshtypes[2] = {DB_UCDMESH,DB_UCDMESH};
        int nneighbors[2] = {1,1};
        int neighbors[2] = {1,0};
        int back[2] = {0,0};
        int lnodelists[2] = {2,2};
        int nl0[2] = {06001,06002};
        int nl1[2] = {06011,06012};
        int *nodelists[2] = {nl0,nl1};

        int lzonelists[2] = {1,1};
        int zl0[1] = {06021};
        int zl1[1] = {06031};
        int *zonelists[2] = {zl0,zl1};

        PUT(DBPutMultimeshadj(db, "multimeshadj",
                              2, meshtypes, nneighbors, neighbors, back,
                              lnodelists, (int const * const *)nodelists,
                              lzonelists, (int const * const *)zonelists,
                              NULL));
    }
}

int
main(int argc, char **argv)
{
    DBfile *db = NULL;
    int driver = DB_PDB;
    const char *filename = "all_objects.pdb";

    if (argc > 1)
    {
        if (!strcmp(argv[1], "hdf5") || !strcmp(argv[1], "DB_HDF5"))
        {
            driver = DB_HDF5;
            filename = "all_objects.h5";
        }
        else if (!strcmp(argv[1], "pdb") || !strcmp(argv[1], "DB_PDB"))
        {
            driver = DB_PDB;
            filename = "all_objects.pdb";
        }
        else
        {
            fprintf(stderr, "usage: %s [pdb|hdf5]\n", argv[0]);
            return 2;
        }
    }

    DBShowErrors(DB_ALL_AND_DRVR, NULL);
    DBSetFriendlyHDF5Names(1);

    db = DBCreate(filename, DB_CLOBBER, DB_LOCAL,
                  "small examples of all Silo DBPut object types", driver);
    if (!db)
    {
        fprintf(stderr, "Could not create %s\n", filename);
        return 1;
    }

    SET_DIR(quad_objects);
    put_quad_objects(db);
    SET_DIR(ucd_objects);
    put_ucd_objects(db);
    SET_DIR(point_objects);
    put_point_objects(db);
    SET_DIR(material_objects);
    put_material_objects(db);
    SET_DIR(simple_objects);
    put_simple_objects(db);
    SET_DIR(flphzl_objects);
    put_facelist(db);
    put_phzonelist(db);
    SET_DIR(csg_objects);
    put_csg_objects(db);
    SET_DIR(mrg_objects);
    put_mrg_objects(db);
    SET_DIR(multi_objects);
    put_multi_objects(db);

    if (DBClose(db) < 0)
    {
        fprintf(stderr, "DBClose failed\n");
        failures++;
    }

    printf("\nCreated %s with %d reported DBPut/constructor failure(s).\n",
           filename, failures);

    return failures ? 1 : 0;
}
