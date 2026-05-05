# Silo Library Header File

We include the contents of the Silo header file here including a description of all `DBxxx` object structs that are returned in `DBGetXXX()` calls as well as all other constant and symbols defined by the library.

---

## DBtoc

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBtoc_ {"
   :end-at: "} DBtoc;"
   ```

## DBcurve

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBcurve_ {"
   :end-at: "} DBcurve;"
   ```

## DBdefvars

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBdefvars_ {"
   :end-at: "} DBdefvars;"
   ```

## DBpointmesh

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBpointmesh_ {"
   :end-at: "} DBpointmesh;"
   ```

## DBmultimesh

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBmultimesh_ {"
   :end-at: "} DBmultimesh;"
   ```

## DBmultimeshadj

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBmultimeshadj_ {"
   :end-at: "} DBmultimeshadj;"
   ```

## DBmultivar

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBmultivar_ {"
   :end-at: "} DBmultivar;"
   ```

## DBmultimat

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBmultimat_ {"
   :end-at: "} DBmultimat;"
   ```

## DBmultimatspecies

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBmultimatspecies_ {"
   :end-at: "} DBmultimatspecies;"
   ```

## DBzonelist

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBzonelist_ {"
   :end-at: "} DBzonelist;"
   ```

## DBphzonelist

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBphzonelist_ {"
   :end-at: "} DBphzonelist;"
   ```

## DBfacelist

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBfacelist_ {"
   :end-at: "} DBfacelist;"
   ```

## DBedgelist

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBedgelist_ {"
   :end-at: "} DBedgelist;"
   ```

## DBquadmesh

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBquadmesh_ {"
   :end-at: "} DBquadmesh;"
   ```

## DBucdmesh

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBucdmesh_ {"
   :end-at: "} DBucdmesh;"
   ```

## DBquadvar

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBquadvar_ {"
   :end-at: "} DBquadvar;"
   ```

## DBucdvar

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBucdvar_ {"
   :end-at: "} DBucdvar;"
   ```

## DBmeshvar

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBmeshvar_ {"
   :end-at: "} DBmeshvar;"
   ```

## DBmaterial

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBmaterial_ {"
   :end-at: "} DBmaterial;"
   ```

## DBmatspecies

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBmatspecies_ {"
   :end-at: "} DBmatspecies;"
   ```

## DBcsgzonelist

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBcsgzonelist_ {"
   :end-at: "} DBcsgzonelist;"
   ```

## DBcsgmesh

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBcsgmesh_ {"
   :end-at: "} DBcsgmesh;"
   ```

## DBcsgvar

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBcsgvar_ {"
   :end-at: "} DBcsgvar;"
   ```

## DBmrgtree

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct _DBmrgtree {"
   :end-at: "} DBmrgtree"
   ```

## DBmrgvar

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct _DBmrgvar {"
   :end-at: "} DBmrgvar"
   ```

## DBgroupelmap

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct _DBgroupelmap {"
   :end-at: "} DBgroupelmap"
   ```

## DBcompoundarray

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBcompoundarray_ {"
   :end-at: "} DBcompoundarray;"
   ```

## DBoptlist

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBoptlist_ {"
   :end-at: "} DBoptlist;"
   ```

## DBobject

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "typedef struct DBobject_ {"
   :end-at: "} DBobject;"
   ```

## DBdatatype

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Data types */"
   :end-at: "} DBdatatype;"
   ```

## DBObjectType

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Objects that can be stored in a data file */"
   :end-at: "} DBObjectType;"
   ```

## Open/Create flags

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Flags for DBCreate (used only in DBCreate) */"
   :end-before: "/* Options */"
   ```

## Optlist options

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Options */"
   :end-at: "#define DBOPT_LAST"
   ```

## VFD options

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Options relating to virtual file drivers */"
   :end-at: "#define DBOPT_H5_LAST"
   ```

## Error handling options

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Error trapping method */"
   :end-at: "#define         DB_ALL_AND_DRVR"
   ```

## Error return codes

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Errors */"
   :end-at: "#define     E_NERRORS"
   ```
## Major order options

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Definitions for MAJOR_ORDER */"
   :end-at: "#define  DB_COLMAJOR"
   ```

## Variable centering options

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Definitions for CENTERING */"
   :end-at: "#define  DB_BLOCKCENT"
   ```
## Coordinate system options

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Definitions for COORD_SYSTEM */"
   :end-at: "#define  DB_OTHER"
   ```

## Derived variable types 

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Definitions for derived variable types */"
   :end-at: "#define DB_VARTYPE_LABEL"
   ```

## CSG options

   ```{literalinclude} ../src/silo/silo.h.in
   :start-at: "/* Definitions for CSG boundary types"
   :end-at: "#define DBCSG_SWEEP"
   ```

