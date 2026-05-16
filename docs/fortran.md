(fortran-interface)=
# Fortran Interface

The functions described in this section are either unique to the Fortran interface or facilitate the mixing of C/C++ and Fortran within a single application interacting with a Silo file.
Note that when Silo was originally written, the vision was that only visualization/post-processing tools would ever attempt to read the contents of Silo files.
Therefore, the Fortran interface has never included all the companion functions to read objects.
That said, it is possible to write simple Fortran callable wrappers to the C functions much like the write interface already implemented.
Have a look in the source file silo_f.c for examples.

{{ EndFunc }}

## `dbmkptr()`

* **Summary:** create a pointer-id from a pointer

* **C Signature:**

  ```
  integer function dbmkptr(void p)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `p` | pointer for which a pointer-id is needed

* **Returned value:**

  the integer pointer id to associate with the pointer

* **Description:**

  In cases where the C interface returns to the application a pointer to an abstract Silo object, in the Fortran interface an integer pointer-id is created and returned instead.
  In addition, in cases where the C interface would accept an array of pointers, such as in [`DBPutCsgvar()`](objects.md#dbputcsgvar), the Fortran interface accepts an array of pointer-ids.
  This function is used to create a pointer-id from a pointer.

  A table of pointers is maintained internally in the Fortran wrapper library.
  The pointer-id is simply the index into this table where the associated object's pointer actually is.
  The caller can free up space in this table using [`dbrmptr()`](#dbrmptr)

{{ EndFunc }}

## `dbrmptr()`

* **Summary:** remove an old and no longer needed pointer-id

* **Fortran Signature:**

  ```Fortran
     integer function dbrmptr(ptr_id)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `ptr_id` | the pointer-id to remove

* **Returned value:**

  always 0

{{ EndFunc }}

## `dbset2dstrlen()`

* **Summary:** Set the size of a 'row' for pointers to 'arrays' of strings

* **Fortran Signature:**

  ```Fortran
     integer function dbset2dstrlen(int len)
     integer len
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `len` | The length to set

* **Returned value:**

  Returns the previously set value.

* **Description:**

  A number of functions in the Fortran interface take a `char*` argument that is really treated internally in the Fortran interface as a 2D array of characters.
  Calling this function allows the caller to specify the length of the rows in this 2D array of characters.
  If necessary, this setting can be varied from call to call.

  The default value is 32 characters.

{{ EndFunc }}

## `dbget2dstrlen()`

* **Summary:** Get the size of a 'row' for pointers to 'arrays' of character strings

* **Fortran Signature:**

  ```Fortran
     integer function dbget2dstrlen()
  ```

* **Arguments:**

  `None`

* **Returned value:**

  The current setting for the 2D string length.

{{ EndFunc }}

## `DBFortranAllocPointer()`

* **Summary:** Facilitates accessing C objects through Fortran

* **Fortran Signature:**

  ```
  int DBFortranAllocPointer (void *pointer)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `pointer` | A `pointer` to a Silo object for which a Fortran identifier is needed

* **Returned value:**

  DBFortranAllocPointer returns an integer that Fortran code can use to reference the given Silo object.

* **Description:**

  The `DBFortranAllocPointer` function allows programs written in both C and Fortran to access the same data structures.
  Many of the routines in the Fortran interface to Silo use an "object id", an integer which refers to a Silo object.
  DBFortanAllocPointer converts a `pointer` to a Silo object into an integer that Fortran code can use.
  In some ways, this function is the inverse of `DBFortranAccessPointer`.

  The integer that `DBFortranAllocPointer` returns is used to index a table of Silo object pointers.
  When done with the integer, the entry in the table may be freed for use later through the use of `DBFortranRemovePointer`.

  See [`DBFortranAccessPointer`](#dbfortranaccesspointer) and [`DBFortranRemovePointer`](#dbfortranremovepointer) for more information about how to use Silo objects in code that uses C and Fortran together.

  For example, if you have a `DBfile*` pointer for a Silo database file and wish to pass this object to some Fortran function(s), the coding pattern would look like the following...

  ```
  DBfile *db = DBOpen("foo.silo", DB_UNKNOWN, DB_APPEND);
  /*
  .
  . C/C++ code operates on db `pointer`
  .
  */

  /* create entry in Fortran wrappers for this Silo Object */
  int dbid = DBFortranAllocPointer(db);

  /* pass dbid to any Fortran code to interact with file */

  /* free up Fortran wrapper resources for this Silo object */

  DBFortranRemovePointer(dbid);
  ```

{{ EndFunc }}

## `DBFortranAccessPointer()`

* **Summary:** Access Silo objects created through the Fortran Silo interface.

* **C Signature:**

  ```
  void *DBFortranAccessPointer (int value)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `value` | The `value` returned by a Silo Fortran function, referencing a Silo object.

* **Returned value:**

  DBFortranAccessPointer returns a pointer to a Silo object (which must be cast to the appropriate type) on success, and `NULL` on failure.

* **Description:**

  The `DBFortranAccessPointer` function allows programs written in both C and Fortran to access the same data structures.
  Many of the routines in the Fortran interface to Silo return an "object id", an integer which refers to a Silo object.
  DBFortranAccessPointer converts this integer into a C pointer so that the sections of code written in C can access the Silo object directly.

  See [`DBFortranAllocPointer`](#dbfortranallocpointer) and [`DBFortranRemovePointer`](#dbfortranremovepointer) for more information about how to use Silo objects in code that uses C and Fortran together.

{{ EndFunc }}

## `DBFortranRemovePointer()`

* **Summary:** Removes a pointer from the Fortran-C index table

* **C Signature:**

  ```
  void DBFortranRemovePointer (int value)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `value` | An integer returned by DBFortranAllocPointer


* **Returned value:**

  Nothing

* **Description:**

  The `DBFortranRemovePointer` function frees up the storage associated with Silo object pointers as allocated by `DBFortranAllocPointer`.

  Code that uses both C and Fortran may make use of `DBFortranAllocPointer` to allocate space in a translation table so that the same Silo object may be referenced by both languages.
  `DBFortranAccessPointer` provides access to this Silo object from the C side.
  Once the Fortran side of the code is done referencing the object, the space in the translation table may be freed by calling `DBFortranRemovePointer`.
  See [`DBFortranAccessPointer`](#dbfortranaccesspointer) and [`DBFortranAllocPointer`](#dbfortranallocpointer) for more information about how to use Silo objects in code that uses C and Fortran together.

{{ EndFunc }}

## `dbwrtfl()`

* **Summary:** Write a facelist object referenced by its object_id to a silo file

* **C Signature:**

  ```
  dbwrtfl(dbid, name, lname, object_id, status)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbid` | The identifier for the Silo database to write the object to.
  `name` | The `name` to be assigned to the object in the file.
  `lname` | The length of the `name` argument.
  `object_id` | The identifier for the facelist object, obtained via `dbcalcfl`.
  `status` | Return value indicating success or failure of the operation; 0 on success, -1 on failure.

* **Returned value:**

  Nothing

* **Description:**

  This function is designed to go hand-in-hand with `dbcalcfl`, the function used to calculate an external facelist.
  When `dbcalcfl` is called, an object identifier is returned in `object_id` for the newly created facelist.
  This call can then be used to write that facelist object to a Silo database.

{{ EndFunc }}
