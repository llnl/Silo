# Python Interface

:::{tip}
Silo's python interface is designed to compile and work with either Python 2 or Python 3
:::

Silo's CMake logic looks for `python3` first.
If it cannot find `python3`, then it looks for `python`.

:::{warning}
CMake'ing with `-DSILO_ENABLE_SHARED:BOOL=OFF` will disable the python module.
:::

It is probably easiest to understand the Python interface to Silo by examining some examples and tests.
In the source code distribution, you can find some examples in tools/python and tests in tests directories.
Here, we briefly describe Silo's Python interface.

The Python interface will be in the `lib` dir of the Silo installation, named `Silo.so`.
To use it, Python needs to be told where to find it.
You can do this a couple of ways; through the `PYTHONPATH` environment variable or by explicitly adding the Silo installation `lib` dir to Python's path using `sys.path.append()`.

For example, if Silo is installed to /foo/bar, this works...

```
% env PYTHONPATH=/foo/bar/lib python
Python 2.7.10 (default, Oct 23 2015, 19:19:21)
[GCC 4.2.1 Compatible Apple `LLVM` 7.0.0] on darwin
Type "help", "copyright", "credits" or "license" for more info.
>>> import Silo
```

Or, if you prefer to use `sys.path.append`...

```
% python
Python 2.7.10 (default, Oct 23 2015, 19:19:21)
[GCC 4.2.1 Compatible Apple `LLVM` 7.0.0] on darwin
Type "help", "copyright", "credits" or "license" for more info.
>>> import sys
>>> sys.path.append("/foo/bar/lib")
>>> import Silo
```

{{ EndFunc }}

## `Silo.Open()`

* **Summary:** Open a Silo file (See [`DBOpen`](./files.md#dbopen))

* **C Signature:**

  ```
  DBfile Silo.Open(filename, flags);
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `filename` | Name of the Silo file to open
  `flags` | Pass either `Silo.DB_READ` if you will only read objects from the file or `Silo.DB_APPEND` if you need to also write data to the file.

* **Description:**

  Returns a `DBfile` object as a Python object

{{ EndFunc }}

## `Silo.Create()`

* **Summary:** Create a new silo file (See [`DBCreate`](./files.md#dbcreate))

* **C Signature:**

  ```
  DBfile Silo.Create(filename, info, driver, clobber)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `filename` | [required string] name of the file to create
  `info` | [required string] comment to be stored in the file
  `driver` | [optional int] which `driver` to use. Pass either `Silo.DB_PDB` or `Silo.DB_HDF5`. Note that advanced `driver` features are not available through the Python interface. Default is `Silo.DB_PDB`.
  `clobber` | [optional int] indicate whether any existing file should be clobbered. Pass either `Silo.DB_CLOBBER` or `Silo.DB_NOCLOBBER`. Default is `Silo.DB_CLOBBER`.

* **Description:**

  Returns a `DBfile` object as a Python object

{{ EndFunc }}

## `<DBfile>.GetToc()`

* **Summary:** Get the table of contents

* **C Signature:**

  ```
  DBtoc <DBfile>.GetToc()
  ```

* **Description:**

  Returns a `DBToc` object as a Python object.
  This probably should really be a Python dictionary object but it is not presently.
  There are no methods defined for a `DBToc` object but if you print it, you can get the list of objects in the current working directory in the file.

{{ EndFunc }}

(dbfile-getvarinfo)=
## `<DBfile>.GetVarInfo()`

* **Summary:** Get metadata and bulk data of any object (See [`DBGetObject`](./generic.md#dbgetobject))

* **C Signature:**

  ```
  dict <DBfile>.GetVarInfo(name, flag)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `name` | [required string] `name` of object to read
  `flag` | [optional int] `flag` to indicate if object bulk/raw data should be included. Pass 0 to **not** also read object bulk/raw data. Pass non-zero to also read object bulk/raw data. Default is 0.

* **Description:**

  Returns a Python dictionary object for any Silo object.
  If object bulk data is not also read, then the dictionary members for those sub-objects will contain a string holding the name/path of either a sub-object or a primitive array.
  Note that on the HDF5 driver, if friendly HDF5 names were not used to create the file, then the string paths for these sub-objects are often cryptic references to primitive arrays in the hidden `/.silo` directory of the Silo file such as `/.silo/#000042`.

  When this method is applied to raw (or primitive) variable object, it will return a python dictionary object indicating the data type and dimensions of the primitive variable.

  This method is poorly named.
  A better name is probably `GetObject`.

{{ EndFunc }}

## `<DBfile>.GetVar()`

* **Summary:** Get a primitive array (See [`DBReadVar`](./generic.md#dbreadvar))

* **C Signature:**

  ```
  tuple <DBfile>.GetVar(name)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `name` | [required string] `name` of primitive array to read

* **Description:**

  This method returns the bulk data of a raw Silo variable as a Python tuple.
  However, because the method provides no means for returning the variable's type, it will return python integer data for anything that has Silo type `DB_SHORT`, `DB_INT`, `DB_LONG` or `DB_LONG_LONG` and python floating point data for `DB_FLOAT` or `DB_DOUBLE`.
  This is potentially problematic when a writer intends to *overwrite* the data in the file with new raw variable data.
  Overwriting Silo data requires that the new data take no more space in the file than the original data.
  Otherwise, bad things will happen.
  To get better information regarding a raw Silo variable's true data type in the file, use [`DBGetVarInfo()`](#dbfile-getvarinfo).

{{ EndFunc }}

## `<DBfile>.SetDir()`

* **Summary:** Set current working directory of the Silo file (See [`DBSetDir`](./files.md#dbsetdir))

* **C Signature:**

  ```
  NoneType <DBfile>.SetDir(name)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `name` | [required string] `name` of directory to set

* **Description:**

  Sets the current working directory of the Silo file

{{ EndFunc }}

## `<DBfile>.Close()`

* **Summary:** Close the Silo file

* **C Signature:**

  ```
  NoneType <DBfile>.Close()
  ```

* **Description:**

  Close the Silo file

{{ EndFunc }}

## `<DBfile>.WriteObject()`

* **Summary:** Write a Python dictionary as a Silo object (See [`DBWriteObject`](./generic.md#dbwriteobject))

* **C Signature:**

  ```
  NoneType <DBfile>.WriteObject(name, obj_dict)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `name` | [required string] `name` of the new object to write
  `obj_dict` | [required dict] Python dictionary containing object data

* **Description:**

  This method will write any Python dictionary object to a Silo file as a Silo object.
  Here's the rub.
  Readers employing Silo's high level C interface (e.g. `DBGetUcdmesh`, `DBGetQuadvar`, etc.) will be able recognize an object so written if and only if the dict object's structure *matches* a known high-level Silo object.

  So, you can use this method to write objects that can be read later via Silo's C language, high-level object methods such as `DBGetUcdmesh` and `DBGetMaterial`, etc. as long as the Python dictionary's members match what Silo expects.

  Often, the easiest way to interrogate how a given Python dict object should be structured to match a Silo high-level object is to find an example object in some Silo file and read it into Python with [`GetVarInfo()`](#dbfile-getvarinfo).
  Be aware, however, that many Silo objects have a lot of optional members which are not *required* to ensure the given object matches a high-level Silo object.

  We demonstrate this approach by first reading a multi-mesh object from an existing Silo file.
  Then, using it as a template, we write a multi-mesh object to a new Silo file and finally confirm we have created a valid, high-level object by test reading it into Silo's browser.

  **Python code**:

  ```
  import Silo

  #
  # create destination database
  #
  db2 = Silo.Create("foobar.silo", "test python", Silo.DB_HDF5, Silo.DB_CLOBBER)

  #
  # Find an existing object type to read
  # Note: example file can be found in https://github.com/visit-dav/visit/blob/develop/data/silo_hdf5_test_data.tar.xz
  db = Silo.Open("multi_ucd3d.silo")
  mm = db.GetVarInfo("mesh1",0)
  print(mm)

  #
  # Remove any optional (optlist) items you don't plan to use
  #
  try:
      mm['extentssize']=0
  except:
      pass
  try:
      del mm['extents']
  except:
      pass
  try:
      del mm['zonecounts']
  except:
      pass
  try:
      del mm['has_external_zones']
  except:
      pass

  #
  # overwrite relevant entries with YOUR data
  #
  mm['nblocks']=3
  mm['meshtypes']=(510,510,510) # 510 is DB_UCDMESH
  mm['meshnames']=("snap","crackle","pop")

  #
  # Write the object
  #
  db2.WriteObject("foo",mm)
  db2.Close()
  db.Close()
  ```

  Ok, save the above script as `gorfo.py` and run it 

  ```
  python3 ./gorfo.py
  ```

  It generates the following output...

  ```
  {'name': 'mesh1', 'type': 'multiblockmesh', 'nblocks': 36, 'cycle': 48, 'blockorigin': 1, 'grouporigin': 1, 'time': 4.800000190734863, 'dtime': 4.8, 'extentssize': 6, 'meshtypes': '/.silo/#001419', 'meshnames': '/.silo/#001420', 'extents': '/.silo/#001421', 'zonecounts': '/.silo/#001422', 'has_external_zones': '/.silo/#001423'}
  ```

  It also creates the new multimesh object in `foobar.silo`.
  We can confirm that the object is correct by reading it with browser...

  ```
  browser -e foo foobar.silo
  INFO: opening `foobar.silo' as $1
                                 = DBmultimesh: struct
  id                             =    0
  nblocks                        =    3
  ngroups                        =    0
  guihide                        =    false
  blockorigin                    =    1
  grouporigin                    =    1
  extentssize                    =    0
  mrgtree_name                   =    (null)
  tv_connectivity                =    0
  disjoint_mode                  =    0
  topo_dim                       =    not specified
  file_ns                        =    (null)
  block_ns                       =    (null)
  block_type                     =    0
  repr_block_idx                 =    not specified
  alt_nodenum_vars               =    NULL
  alt_zonenum_vars               =    NULL
  empty_cnt                      =    0
  empty_list                     =    NULL
  meshids                        =    NULL
  meshnames[0]                   =    {"snap", "crackle", "pop"}
  meshtypes[0]                   =    {DB_UCDMESH, DB_UCDMESH, DB_UCDMESH}
  dirids                         =    NULL
  extents                        =    NULL
  zonecounts                     =    NULL
  has_external_zones             =    NULL
  lgroupings                     =    0
  groupings                      =    NULL
  groupnames  
  ```

  It is fine to create a dict object with additional members too.
  For example, if you create a dict object that is intended to be a Silo material object, you can add additional members to it and readers will still be able to read it via the high-level C interface method `DBGetMaterial`.
  Of course, such readers will not be aware of any additional members so handled.

  It is also fine to create wholly new kinds of Silo objects for which there are no corresponding high-level interface methods in the C language interface.
  Such an object can be read only as a generic object using the `DBGetObject()` C language interface method.

{{ EndFunc }}

## `<DBfile>.Write()`

* **Summary:** Write primitive array data to a Silo file (see DBWrite)

* **C Signature:**

  ```
  NoneType <DBfile>.Write(name, data, [dims, datatype])
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `name` | [required string] `name` of the primitive array
  `data` | [required tuple] the `data` to write
  `dims` | [optional tuple] the dimensions of `data`. If not present, defaults to size of `data`.
  `datatype` | [optional int] the Silo data type (e.g. `DB_INT`, ...). If not present, guessed by type of first value in `data`.

* **Description:**

  This method will write a raw variable to a Silo file.
  If you need to *overwrite* Silo data, this is the method to use because it allows the writer to *specify* the Silo datatype.

{{ EndFunc }}

## `<DBfile>.MkDir()`

* **Summary:** Make a directory in a Silo file

* **C Signature:**

  ```
  NoneType <DBfile>.MkDir(name)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `name` | [required string] `name` of the directory to create

* **Description:**

  Creates a new directory in a Silo file

{{ EndFunc }}
