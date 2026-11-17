# Object Allocation, Free and IsEmpty Tests

This section describes methods to allocate and initialize many of Silo's objects.

{{ EndFunc }}

## `DBAllocXxx()`

* **Summary:** Allocate and initialize a Silo structure.

* **C Signature:**

  ```
  DBcompoundarray  *DBAllocCompoundarray (void)
  DBcsgmesh        *DBAllocCsgmesh (void)
  DBcsgvar         *DBAllocCsgvar (void)
  DBcurve          *DBAllocCurve (void)
  DBcsgzonelist    *DBAllocCSGZonelist (void)
  DBdefvars        *DBAllocDefvars (void)
  DBedgelist       *DBAllocEdgelist (void)
  DBfacelist       *DBAllocFacelist (void)
  DBmaterial       *DBAllocMaterial (void)
  DBmatspecies     *DBAllocMatspecies (void)
  DBmeshvar        *DBAllocMeshvar (void)
  DBmultimat       *DBAllocMultimat (void)
  DBmultimatspecies *DBAllocMultimatspecies (void)
  DBmultimesh      *DBAllocMultimesh (void)
  DBmultimeshadj   *DBAllocMultimeshadj (void)
  DBmultivar       *DBAllocMultivar (void)
  DBpointmesh      *DBAllocPointmesh (void)
  DBquadmesh       *DBAllocQuadmesh (void)
  DBquadvar        *DBAllocQuadvar (void)
  DBucdmesh        *DBAllocUcdmesh (void)
  DBucdvar         *DBAllocUcdvar (void)
  DBzonelist       *DBAllocZonelist (void)
  DBphzonelist     *DBAllocPHZonelist (void)
  DBnamescheme     *DBAllocNamescheme(void);
  DBgroupelmap     *DBAllocGroupelmap(int, DBdatatype)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Returned value:**

  These allocation functions return a pointer to a newly allocated and initialized structure on success and `NULL` on failure.

* **Description:**

  The allocation functions allocate a new structure of the requested type, and initialize all values to `NULL` or zero.
  There are counterpart functions for freeing structures of a given type (see DBFree….

{{ EndFunc }}

## `DBFreeXxx()`

* **Summary:** Release memory associated with a Silo structure.

* **C Signature:**

  ```
  void DBFreeCompoundarray (DBcompoundarray *x)
  void DBFreeCsgmesh (DBcsgmesh *x)
  void DBFreeCsgvar (DBcsgvar *x)
  void DBFreeCSGZonelist (DBcsgzonelist *x)
  void DBFreeCurve(DBcurve *);
  void DBFreeDefvars (DBdefvars *x)
  void DBFreeEdgelist (DBedgelist *x)
  void DBFreeFacelist (DBfacelist *x)
  void DBFreeMaterial (DBmaterial *x)
  void DBFreeMatspecies (DBmatspecies *x)
  void DBFreeMeshvar (DBmeshvar *x)
  void DBFreeMultimesh (DBmultimesh *x)
  void DBFreeMultimeshadj (DBmultimeshadj *x)
  void DBFreeMultivar (DBmultivar *x)
  void DBFreeMultimat(DBmultimat *)
  void DBFreeMultimatspecies(DBmultimatspecies *)
  void DBFreePointmesh (DBpointmesh *x)
  void DBFreeQuadmesh (DBquadmesh *x)
  void DBFreeQuadvar (DBquadvar *x)
  void DBFreeUcdmesh (DBucdmesh *x)
  void DBFreeUcdvar (DBucdvar *x)
  void DBFreeZonelist (DBzonelist *x)
  void DBFreePHZonelist (DBphzonelist *x)
  void DBFreeNamescheme(DBnamescheme *)
  void DBFreeMrgvar(DBmrgvar *mrgv)
  void DBFreeMrgtree(DBmrgtree *tree)
  void DBFreeGroupelmap(DBgroupelmap *map)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `x` | A pointer to a structure which is to be freed. Its type must correspond to the type in the function name.
  `Fortran Equivalent:` | None

* **Returned value:**

  These free functions return zero on success and -1 on failure.

* **Description:**

  The free functions release the given structure as well as all memory pointed to by these structures.
  This is the preferred method for releasing these structures.
  There are counterpart functions for allocating structures of a given type (see DBAlloc…).
  The functions will not fail if a `NULL` pointer is passed to them.

{{ EndFunc }}

## `DBIsEmpty()`

* **Summary:** Query a object returned from Silo for "emptiness"

* **C Signature:**

  ```
  int     DBIsEmptyCurve(DBcurve const *curve)
  int     DBIsEmptyPointmesh(DBpointmesh const *msh)
  int     DBIsEmptyPointvar(DBpointvar const *var)
  int     DBIsEmptyMeshvar(DBmeshvar const *var)
  int     DBIsEmptyQuadmesh(DBquadmesh const *msh)
  int     DBIsEmptyQuadvar(DBquadvar const *var)
  int     DBIsEmptyUcdmesh(DBucdmesh const *msh)
  int     DBIsEmptyFacelist(DBfacelist const *fl)
  int     DBIsEmptyZonelist(DBzonelist const *zl)
  int     DBIsEmptyPHZonelist(DBphzonelist const *zl)
  int     DBIsEmptyUcdvar(DBucdvar const *var)
  int     DBIsEmptyCsgmesh(DBcsgmesh const *msh)
  int     DBIsEmptyCSGZonelist(DBcsgzonelist const *zl)
  int     DBIsEmptyCsgvar(DBcsgvar const *var)
  int     DBIsEmptyMaterial(DBmaterial const *mat)
  int     DBIsEmptyMatspecies(DBmatspecies const *spec)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `x` | Pointer to a silo object structure to be queried

* **Description:**

  These functions return non-zero if the object is indeed empty and zero otherwise.
  When `DBSetAllowEmptyObjects()` is enabled by a writer, it can produce objects in the file which contain useful metadata but no "problems-sized" data.
  These methods can be used by a reader to determine if an object read from a file is empty.

{{ EndFunc }}

## `DBFreeNamescheme()`

* **Summary:** Free all memory associated with a namescheme object 

* **C Signature:**

  ```
  DBFreeNamescheme(DBnamescheme *ns)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `ns` | Pointer to a namescheme object to be freed

* **Description:**

  `DBFreeNamescheme()` will free all memory associated with a `DBnamescheme` object.
  What happens with externally-referenced arrays used in the namescheme?
  The answer is that it depends on whether `DBMakeNamescheme()` allocated them or the caller did.
  In the 3 forms of use of `DBMakeNamescheme()`, the two forms involving external array references differ in whether the caller allocated the memory for the arrays or `DBMakeNamescheme()` did.
  In the second form, where the caller allocated the externally referenced arrays, `DBFreeNamescheme()` will **NOT** free them.
  In the third form, where `DBMakeNamescheme()` finds in the provided file, allocates and reads the externally referenced arrays, `DBFreeNamescheme()` will also free them.

{{ EndFunc }}

## Freeing library internal caches

* **Summary:** Freeing memory associated with various internal caches used in the library

* **C Signature:**

  ```
  DBSPrintf(0); /* free circular cache entries used by this method */
  DBGetName(0,-1); /* free circular cache entries used by this method */
  ```

* **Arguments:**

  Varies by function.

* **Description:**

  In various places where Silo returns a small string, the library uses a small, internal, circular cache which is never normally freed.
  This can lead to tiny leaks reported by various memory checker tools.
  These can be addressed by using calls into the associated routines with arguments specifically designed, see above, to trigger freeing of those caches.

{{ EndFunc }}
