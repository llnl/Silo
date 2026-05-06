# Files and File Structure

If you are looking for information regarding how to use Silo from a parallel application, please See the section on [Multi-Block Objects Parallel I/O](#multi-block-objects-and-parallel-i-o).

The Silo API is implemented on a number of different low-level drivers.
These drivers control the low-level file format Silo generates.
For example, Silo can generate PDB (Portable DataBase) and HDF5 formatted files.
The specific choice of low-level file format is made at file creation time.

In addition, Silo files can themselves have directories (folders).
That is, within a single Silo file, one can create directory hierarchies for storage of various objects.
These directory hierarchies are analogous to the Unix file system.
Directories serve to divide the name space of a Silo file so the user can organize content within a Silo file in a way that is natural to the application.

Note that the organization of objects into directories within a Silo file may have direct implications for how these collections of objects are presented to users by post-processing tools.
For example, except for directories used to store multi-block objects (See [Multi-Block Objects Parallel I/O](#multi-block-objects-and-parallel-i-o)), VisIt will use directories in a Silo file to create sub-menus within its Graphical User Interface (GUI).
If VisIt opens a Silo file with two directories, "foo" and "bar", and there are various meshes and variables in each of these directories, then many of VisIt's GUI menus will contain sub-menus named "foo" and "bar" where the objects found in those directories will be placed in the GUI.

Silo also supports the concept of *grabbing* the low-level driver.
For example, if Silo is using the HDF5 driver, an application can obtain the actual HDF5 file id and then use the native HDF5 API with that file id.

{{ EndFunc }}

## `DBRegisterFileOptionsSet()`

* **Summary:** Register a set of options for advanced control of the low-level I/O driver

* **C Signature:**

  ```
  int DBRegisterFileOptionsSet(const DBoptlist *opts)
  ```

* **Fortran Signature:**

  ```
  int dbregfopts(int optlist_id)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `opts` | an options list object obtained from a `DBMakeOptlist()` call

* **Returned value:**

  -1 on failure.
  Otherwise, the integer index of a registered file options set is returned.

* **Description:**

  File options sets are used in concert with the `DB_HDF5_OPTS()` macro in `DBCreate` or `DBOpen` calls to provide advanced and fine-tuned control over the behavior of the underlying driver library and may be needed to affect memory usage and I/O performance as well as vary the behavior of the underlying I/O driver away from its default mode of operation.

  A file options set is nothing more than an optlist object (see [Optlists](./optlists.md)), populated with file driver related options.
  A *registered* file options set is such an optlist that has been registered with the Silo library via a call to this method, `DBRegisterFileOptionsSet`.
  A maximum of 32 registered file options sets are currently permitted.
  Use `DBUnregisterFileOptionsSet` to free up a slot in the list of registered file options sets.

  Before a specific file options set may be used as part of a `DBCreate` or `DBOpen` call, the file options set must be registered with the Silo library.
  In addition, the associated optlist object should not be freed until after the last call to `DBCreate` or `DBOpen` in which it is needed.

  Presently, the options sets are defined for the HDF5 driver *only*.
  The table below defines and describes the various options.
  A key option is the selection of the HDF5 [Virtual File Driver](https://docs.hdfgroup.org/hdf5/develop/_h5_f__u_g.html#subsec_file_alternate_drivers) or VFD.
  See [`DBCreate`](#dbcreate) for a description of the available VFDs.

  In the table of options below, some options are relevant to only a specific HDF5 VFD.
  Other options effect the behavior of the HDF5 library as a whole, regardless of which underlying VFD is used.
  This difference is notated in the scope column.

  All of the options described here relate to options documented in the HDF5 library's [file access property lists](https://docs.hdfgroup.org/hdf5/develop/group___f_a_p_l.html#title0).

  Note that all option names listed in left-most column of the table below have had their prefix "`DBOPT_H5_`" removed to save space in the table.
  So, for example, the real name of the `CORE_ALLOC_INC` option is `DBOPT_H5_CORE_ALLOC_INC`.

  `DBOPT_H5_`...|Type|Meaning|Default
  :---|:---|:---|:---
  `VFD`|`int`|Specifies which Virtual File Driver (VFD) the HDF5 library should use. Set the integer value for this option to one of the following values. `DB_H5VFD_DEFAULT`, (use HDF5 default driver) `DB_H5VFD_SEC2` (use HDF5 sec2 driver) `DB_H5VFD_STDIO`, (use HDF5 stdio driver) `DB_H5VFD_CORE`, (use HDF5 core driver) `DB_H5VFD_LOG`, (use HDF5 log river) `DB_H5VFD_SPLIT`, (use HDF5 split driver) `DB_H5VFD_DIRECT`, (use HDF5 direct i/o driver) `DB_H5VFD_FAMILY`, (use HDF5 family driver) `DB_H5VFD_MPIO`, (use HDF5 mpi-io driver) `DB_H5VFD_MPIP`, (use HDF5 mpi posix driver) `DB_H5VFD_SILO`, (use `SILO` BG/Q driver) `DB_H5VFD_FIC` (use `SILO` file in core driver). Many of the remaining options described in this table apply to only certain of the above VFDs.|
  `RAW_FILE_OPTS`|`int`|Applies only for the split VFD. Specifies a file options set to use for the raw data file. May be any value returned from a call to DBRegisterFileOptionsSet() or can be any one of the following pre-defined file options sets... `DB_FILE_OPTS_H5_DEFAULT_`... DEFAULT, SEC2, STDIO, CORE, LOG, SPLIT, DIRECT, FAMILY, MPIO, MPIP, `SILO`. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_split|
  `RAW_EXTENSION`|`char*`|Applies only for the split VFD. Specifies the file extension/naming convention for raw data file. If the string contains a '%s' printf-like conversion specifier, that will be replaced with the name of the file passed in the DBCreate/DBOpen call. If the string does **not** contain a '%s' printf-like conversion specifier, it is treated as an 'extension' which is appended to the name of the file passed in DBCreate/DBopen call. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_split|"-raw"
  `META_FILE_OPTS`|`int`|Same as `DBOPT_H5_RAW_FILE_OPTS`, above, except for meta data file. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_split.|
  `META_EXTENSION`||Same as `DBOPT_H5_RAW_EXTENSION` above, except for meta data file. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_split.|""
  `CORE_ALLOC_INC`|`int`|Applies only for core VFD. Specifies allocation increment. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_core.|(1<<20)
  `CORE_NO_BACK_STORE`|`int`|Applies only for core VFD. Specifies whether or not to store the file on close. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_core.|FALSE
  `LOG_NAME`|`char*`|Applies only for the log VFD. This is primarily a debugging feature. Specifies name of the file to which logging data shall be stored. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_log.|"log.out"
  `LOG_BUF_SIZE`|`int`|Applies only for the log VFD. This is primarily a debugging feature. Specifies size of the buffer to which byte-for-byte HDF5 data type information is written. See HDF5 reference manual for H5Pset_fapl_log.|0
  `META_BLOCK_SIZE`|`int`|Applies the the HDF5 library as a whole (e.g. globally). Specifies the size of memory allocations the library should use when allocating meta data. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_meta_block_size.|0
  `SMALL_RAW_SIZE`|`int`|Applies to the HDF5 library as a whole (e.g. globally). Specifies a threshold below which allocations for raw data are aggregated into larger blocks within HDF5. This can improve I/O performance by reducing number of small I/O requests. Note, however, that with a block-oriented VFD such as the Silo specific VFD, this parameter must be set to be consistent with block size of the VFD. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_small_data_block_size.|0
  `ALIGN_MIN`|`int`|Applies to the HDF5 library as a whole. Specified a size threshold above which all datasets are aligned in the file using the value specified in `ALIGN_VAL`. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_alignment.|0
  `ALIGN_VAL`|`int`|The alignment to be applied to datasets of size greater than `ALIGN_MIN`. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_alignment.|0
  `DIRECT_MEM_ALIGN`|`int`|Applies only to the direct VFD. Specifies the alignment option. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_direct.|0
  `DIRECT_BLOCK_SIZE`|`int`|Applies only to the direct VFD. Specifies the block size the underlying file system is using. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_direct.|
  `DIRECT_BUF_SIZE`||Applies only to the direct VFD. Specifies a copy buffer size. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_direct.|
  `MPIO_COMM`||||
  `MPIO_INFO`||||
  `MPIP_NO_GPFS_HINTS`||||
  `SIEVE_BUF_SIZE`|`int`|HDF5 sieve buf size. Only relevant if using either compression and/or checksumming. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_sieve_buf_size.|
  `CACHE_NELMTS`|`int`|HDF5 raw data chunk cache parameters. Only relevant if using either compression and/or checksumming. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_cache.|
  `CACHE_NBYTES`||HDF5 raw data chunk cache parameters. Only relevant if using either compression and/or checksumming. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_cache.|
  `CACHE_POLICY`||HDF5 raw data chunk cache parameters. Only relevant if using either compression and/or checksumming. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_cache.|
  `FAM_SIZE`|`int`|Size option for family VFD. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_family. The family `VFD` is useful for handling files that would otherwise be larger than 2Gigabytes on file systems that support a maximum file size of 2Gigabytes.|
  `FAM_FILE_OPTS`|`int`|VFD options for each file in family VFD. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_fapl_family. The family VFD is useful for handling files that would otherwise be larger than 2Gigabytes on file systems that support a maximum file size of 2Gigabytes.|
  `USER_DRIVER_ID`|`int`|Specify some user-defined VFD. Permits application to specify any user-defined VFD. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_driver.|
  `USER_DRIVER_INFO`||Specify user-defined VFD information struct. Permits application to specify any user-defined VFD. See [HDF5 reference manual](https://docs.hdfgroup.org/hdf5/develop/) for H5Pset_driver.|
  `SILO_BLOCK_SIZE`|`int`|Block size option for Silo VFD. All I/O requests to/from disk will occur in blocks of this size.|(1<<16)
  `SILO_BLOCK_COUNT`|`int`|Block count option for Silo VFD. This is the maximum number of blocks the Silo VFD will maintain in memory at any one time.|32
  `SILO_LOG_STATS`|`int`|Flag to indicate if Silo VFD should gather I/O performance statistics. This is primarily for debugging and performance tuning of the Silo VFD.|0
  `SILO_USE_DIRECT`|`int`|Flag to indicate if Silo VFD should attempt to use direct I/O. Tells the Silo VFD to use direct I/O where it can. Note, if it cannot, this option will be silently ignored.|0
  `FIC_BUF`|`void*`|The buffer of bytes to be used as the "file in core" to be opened in a `DBOpen()` call.|none
  `FIC_SIZE`|`int`|Size of the buffer of bytes to be used as the "file in core" to be opened in a `DBOpen()` call.|none

{{ EndFunc }}

## `DBUnregisterFileOptionsSet()`

* **Summary:** Unregister a registered file options set

* **C Signature:**

  ```
  int DBUnregisterFileOptionsSet(int opts_set_id)
  ```

* **Fortran Signature:**

  ```
  int dbunregfopts(int optlist_id)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `opts_set_id` | The identifier (obtained from a previous call to DBRegisterFileOptionsSet()) of a file options set to unregister.

* **Returned value:**

  Zero on success.
  -1 on failure.

* **Description:**

  Unregister a specific file options set identified by `opts_set_id`.

{{ EndFunc }}

## `DBUnregisterAllFileOptionsSets()`

* **Summary:** Unregister all file options sets

* **C Signature:**

  ```
  int DBUnregisterAllFileOptionsSets()
  ```

* **Fortran Signature:**

  ```
  int dbunregafopts()
  ```

* **Arguments:**

  `None`
* **Returned value:**

  Zero on success, -1 on failure.

* **Description:**

  Unregister all file options sets.

{{ EndFunc }}

## `DBSetUnknownDriverPriorities()`

* **Summary:** Set driver priorities for opening files with the `DB_UNKNOWN` driver.

* **C Signature:**

  ```
  static const int *DBSetUnknownDriverPriorities(int *driver_ids)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `driver_ids` | A `-1` terminated list of driver ids such as `DB_HDF5`, `DB_PDB`, `DB_HDF5_CORE`, or any driver id constructed with the `DB_HDF5_OPTS()` macro.

* **Returned value:**

  The previous unknown driver prioritization as a `-1` terminated array of integer values.
  The caller should **NOT** free the returned value.

* **Description:**

  When opening files with `DB_UNKNOWN` driver, Silo iterates over drivers, trying each until it successfully opens a file.

  This call can be used to affect the order in which driver ids are attempted and can improve behavior and performance for opening files using `DB_UNKNOWN` driver.

  If any of the driver ids specified in `driver_ids` is constructed using the `DB_HDF5_OPTS()` macro, then the associated file options set must be registered with the Silo library.

{{ EndFunc }}

## `DBGetUnknownDriverPriorities()`

* **Summary:** Return the currently defined ordering of drivers the `DB_UNKNOWN` driver will attempt.

* **C Signature:**

  ```
  static const int *DBGetUnknownDriverPriorities(void)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Description:**

  Returns a `-1` terminated list of integer driver ids indicating the prioritization of drivers used by the `DB_UNKNOWN` driver.
  The caller should **NOT** free the returned value.

{{ EndFunc }}

## `DBCreate()`

* **Summary:** Create a Silo output file.

* **C Signature:**

  ```
  DBfile *DBCreate (char *pathname, int mode, int target,
      char *fileinfo, int filetype)
  ```

* **Fortran Signature:**

  ```
  integer function dbcreate(pathname, lpathname, mode, target,
     	fileinfo, lfileinfo, filetype, dbid)
  ```

  returns the newly created database file handle in `dbid`.

* **Arguments:**

  Arg name | Description
  :---|:---
  `pathname` | Path name of file to create. This can be either an absolute or relative path.
  `mode` | Creation `mode`. Pass `DB_CLOBBER` or `DB_NOCLOBBER`, optionally OR'd with `DB_PERF_OVER_COMPAT` or `DB_COMPAT_OVER_PERF` (see [`DBSetCompatibilityMode`](globals.md#dbsetcompatibilitymode), optionally OR'd with `DB_CONCURRENT` if another process intends to read the file's contents concurrently with the writer (concurrent acccess is available only on the HDF5 driver).
  `target` | Destination file format. In the distant past, this option was used to target binary numeric formats in the file to a specific host CPU architecture (such as Sun or Sgi or Cray). More recently, this argument has become less relevant and should most likely always be set to `DB_LOCAL`.
  `fileinfo` | Character string containing descriptive information about the file's contents. This information is usually printed by applications when this file is opened. If no such information is needed, pass `NULL` for this argument.
  `filetype` | Destination file type. Applications typically use one of either `DB_PDB`, which will create PDB files, or `DB_HDF5`, which will create HDF5 files. Other options include `DB_PDBP`, `DB_HDF5_SEC2`, `DB_HDF5_STDIO`, `DB_HDF5_CORE`, `DB_HDF5_SPLIT` or `DB_FILE_OPTS(optlist_id)` where `optlist_id` is a registered file options set. For a description of the meaning of these options as well as many other advanced features and control of underlying I/O behavior, see [DBRegisterFileOptionsSet](#dbregisterfileoptionsset).

* **Returned value:**

  DBCreate returns a `DBfile` pointer on success and `NULL` on failure.
  If the `pathname` argument contains a *path* components (e.g. `foo/bar/baz/file.silo`), note that `DBCreate` creates only the file part of the path.
  The containing directories (folders) must already exist and be writable.

* **Description:**

  The `DBCreate` function creates a Silo file and initializes it for writing data.

  Silo supports two underlying drivers for storing named arrays and objects of machine independent data.
  One is called the Portable DataBase Library ([PDBLib](https://pubs.aip.org/aip/cip/article/7/3/304/281673/Software-for-Portable-Scientific-Data-Management) or just PDB), and the other is Hierarchical Data Format, Version 5 ([HDF5](https://www.hdfgroup.org/solutions/hdf5/))

  When Silo is CMake'd with the `-DSILO_PACT_DIR=<path-to-PACT-PDB>` CMake variable, the Silo library supports both the PDB library that is built-in to Silo (which is actually an ancient version of PACT's PDB referred to internally as *PDB Lite*) identified with a `filetype` of `DB_PDB` and a second variant of the PDB driver using a *current* PACT installation with a `filetype` of `DB_PDBP` (Note the trailing `P` for *PDB Proper*).
  PDB Proper is known to give far superior performance than PDB Lite on BG/P and BG/L class systems and so is recommended when using PDB driver on such systems.

  For the HDF5 library, there are many more available options for fine tuned control of the underlying I/O through the use of HDF5's Virtual File Drivers (VFDs).
  For example, HDF5's `sec2` VFD uses Unix Manual Section 2 I/O routines (e.g. create/open/read/write/close) while the `stdio` VFD uses Standard I/O routines (e.g. fcreate/fopen/fread/fwrite/fclose).

  Depending on the circumstances, the choice of VFD can have a profound impact on actual I/O performance.
  For example, on BlueGene systems the customized Silo block-based VFD (introduced to the Silo library in Version 4.8) has demonstrated excellent performance compared to the default HDF5 VFD; sec2.
  The remaining paragraphs describe each of the available Virtual File Drivers as well as parameters that govern their behavior.

  `DB_HDF5`
  : From among the several VFDs that come pre-packaged with the HDF5 library, this driver type uses whatever the HDF5 library defines as the default VFD.
    On non-Windows platforms, this is the Section 2 (see below) VFD.
    On Windows platforms, it is a Windows specific VFD.

  `DB_HDF5_SEC2`
  : Uses the I/O system interface defined in section 2 of the Unix manual.
    That is `create`, `open`, `read`, `write`, `close`.
    This is a VFD that comes pre-packaged with the HDF5 library.
    It does little to no optimization of I/O requests.
    For example, two I/O requests that abut in file address space wind up being issued through the section 2 I/O routines as independent requests.
    This can be disastrous for high latency file systems such as might be available on BlueGene class systems.

  `DB_HDF5_STDIO`
  : Uses the Standard I/O system interface defined in Section 3 of the Unix manual.
    That is `fcreate`, `fopen`, `fread`, `fwrite`, `fclose`.
    This is a VFD that comes pre-packaged with the HDF5 library.
    It does little to no optimization of I/O requests.
    However, since it uses the stdio routines, it does benefit from whatever default buffering the implementation of the stdio interface on the given platform provides.
    Because section 2 routines are unbuffered, the sec2 VFD typically performs better when there are fewer, larger I/O requests while the stdio VFD performs better when there are more, smaller requests.
    Unfortunately, the metric for what constitutes a "small" or "large" request is system dependent.
    So, it helps to experiment with the different VFDs for the HDF5 driver by running some typically sized use cases.
    Some results on the Luster file system for tiny I/O requests (100's of bytes) showed that the stdio VFD can perform 100x or more better than the section 2.
    So, it pays to spend some time experimenting with this [Note: In future, it should be possible to manipulate the buffer used for a given Silo file opened via the stdio VFD as one would ordinarily do via such stdio calls as setvbuf().
    However, due to limitations in the current implementation, that is not yet possible.
    When and if that becomes possible, to use something other than non-default stdio buffering, the Silo client will have to create and register an appropriate file options set.

  `DB_HDF5_CORE`
  : Uses a memory buffer for the file with the option of either writing the resultant buffer to disk or not.
    Conceptually, this VFD behaves more or less like a ramdisk.
    This is a VFD that comes pre-packaged with the HDF5 library.
    I/O performance is optimal in the sense that only a single I/O request for the entire file is issued to the underlying file system.
    However, this optimization comes at the expense of memory.
    The entire file must be capable of residing in memory.
    In addition, releases of HDF5 library prior to 1.8.2 support the core VFD only when creating a new file and not when open an existing file.
    Two parameters govern behavior of the core VFD.
    The *allocation increment* specifies the amount of memory the core VFD allocates, each time it needs to increase the buffer size to accommodate the (possibly growing) file.
    The *backing store* indicates whether the buffer should be saved to disk (if it has been changed) upon close.
    By default, using `DB_HDF5_CORE` as the driver type results in an allocation increment of 1 Megabyte and a backing store option of TRUE, meaning it will store the file to disk upon close.
    To specify parameters other than these default values, the Silo client will have to create and register an appropriate file options set, see [DBRegisterFileOptionsSet](#dbregisterfileoptionsset).

  `DB_HDF5_SPLIT`
  : Splits HDF5 I/O operations across two VFDs.
    One VFD is used for all raw data while the other VFD is used for everything else (e.g. meta data).
    For example, in Silo's `DBPutPointvar()` call, the data the caller passes in the vars argument is raw data.
    Everything else including the object's name, number of points, datatype, optlist options, etc. including all underlying HDF5 metadata gets treated as meta data.
    This is a VFD that comes pre-packaged with the HDF5 library.
    It results in two files being produced; one for the raw data and one for the meta data.
    The reason this can be a benefit is that tiny bits of metadata intermingling with large raw data operations can degrade performance overall.
    Separating the data streams can have a profound impact on performance at the expense of two files being produced.
    Four parameters govern the behavior of the split VFD.
    These are the VFD and filename extension for the raw and meta data, respectively.
    By default, using `DB_HDF5_SPLIT` as the driver type results in Silo using sec2 and `"-raw"` as the VFD and filename extension for raw data and core (default params) and `""` (empty string) as the VFD and extension for meta data.
    To specify parameters other than these default values, the Silo client will have to create and register an appropriate file options set, see [DBRegisterFileOptionsSet](#dbregisterfileoptionsset). 

  `DB_HDF5_FAMILY`
  : Allows for the management of files larger than 2{sup}`32` bytes on 32-bit systems.
    The virtual file is decomposed into real files of size small enough to be managed on 32-bit systems.
    This is a VFD that comes pre-packaged with the HDF5 library.
    Two parameters govern the behavior of the family VFD.
    The size of each file in a family of files and the VFD used for the individual files.
    By default, using `DB_HDF5_FAMILY` as the driver type results in Silo using a size of 1 Gigabyte (2{sup}`32`) and the default VFD for the individual files.
    To specify parameters other than these default values, the Silo client will have to create and register an appropriate file options set, see [DBRegisterFileOptionsSet](#dbregisterfileoptionsset).

  `DB_HDF5_LOG`
  : While doing the I/O for HDF5 data, also collects detailed information regarding VFD calls issued by the HDF5 library.
    The logging VFD writes detailed information regarding VFD operations to a log file.
    This is a VFD that comes pre-packaged with the HDF5 library.
    However, the logging VFD is a different code base than any other VFD that comes pre-packaged with HDF5.
    So, while the logging information it produces is representative of the VFD calls made by HDF5 library to the VFD interface, it is **not** representative of the actual I/O requests made by the sec2 or stdio or other VFDs.
    Behavior of the logging VFD is governed by 3 parameters; the name of the file to which log information is written, a set of flags which are or'd together to specify the types of operations and information logged and, optionally, a buffer (which must be at least as large as the actual file being written) which serves to map the kind of HDF5 data (there are about 8 different kinds) stores at each byte in the file.
    By default, using `DB_HDF5_LOG` as the driver type results in Silo using a log file name of "silo_hdf5_log.out", flags of `H5FD_LOG_LOC_IO|H5FD_LOG_NUM_IO|H5FD_LOG_TIME_IO|H5FD_LOG_ALLOC` and a `NULL` buffer for the mapping information.
    To specify parameters other than these default values, the Silo client will have to create and register an appropriate file options set, see [DBRegisterFileOptionsSet](#dbregisterfileoptionsset).

    Users interested in this VFD should consult HDF5's reference manual for the meaning of the flags as well as how to interpret logging VFD output.

  `DB_HDF5_DIRECT`
  : On systems that support the `O_DIRECT` flag in section 2 `create`/`open` calls, this VFD will use direct I/O.
    This VFD comes pre-packaged with the HDF5 library.
    Most systems (both the system interfaces implementations for section 2 I/O as well as underlying file systems) do a lot of work to buffer and cache data to improve I/O performance.
    In some cases, however, this extra work can actually get in the way of good performance, particularly when the I/O operations are streaming like and large.
    Three parameters govern the behavior of the direct VFD.
    The alignment specifies memory alignment requirement of raw data buffers.
    That generally means that posix_memalign should be used to allocate any buffers you use to hold raw data passed in calls to the Silo library.
    The block size indicates the underlying file system block size and the copy buffer size gives the HDF5 library some additional flexibility in dealing with unaligned requests.
    Few systems support the `O_DIRECT` flag and so this VFD is not often available in practice.
    However, when it is, using `DB_HDF5_DIRECT` as the driver type results in Silo using an alignment of 4 kilobytes (2{sup}`12`), an alignment equal to the block size and a copy buffer size equal to `256` times the block size.

  `DB_HDF5_SILO`
  : This is a custom VFD designed specifically to address some of the performance shortcomings of VFDs that come pre-packaged with the HDF5 library.
    The silo VFD is a very, very simple, block-based VFD.
    It decomposes the file into blocks, keeps some number of blocks in memory at any one time and issues I/O requests *only* in whole blocks using section 2 I/O routines.
    In addition, it sets up some parameters that control HDF5 library's allocation of meta data and raw data such that each block winds up consisting primarily of either raw or meta data but not both.
    It also disables meta data caching in HDF5 to reduce memory consumption of the HDF5 library to the bare minimum as there is no need for HDF5 to maintain cached metadata if it resides in blocks kept in memory in the VFD.
    This is a suitable VFD for most scientific computing applications that are dumping either post-processing or restart files as applications that do that tend to open the file, write a bunch of stuff from start to finish and close it or read a bunch of stuff from start to finish and close it.
    Two parameters govern the behavior of the silo VFD; the block size and the block count.
    The block size determines the size of individual blocks.
    All I/O requests will be issued in whole blocks.
    The block count determines the number of blocks the silo VFD is permitted to keep in memory at any one time.
    On BG/P class systems, good values are 1 Megabyte (2{sup}`20`) block size and block count of 16 or 32.
    By default, the silo VFD uses a block size of 16 Kilobytes (2{sup}`14`) and a block count also of 16.
    To specify parameters other than these default values, the Silo client will have to create and register an appropriate file options set, see [DBRegisterFileOptionsSet](#dbregisterfileoptionsset).

  `DB_HDF5_MPIO` and `DB_HDF5_MPIOP`
  : These have been removed from Silo as of version 4.10.3.

  The HDF5 driver also supports one or more *concurrent* readers reading a Silo file produced by a *single* writer.
  This feature uses HDF5's *SWMR* (pronounced `swimmer` meaning single writer, multiple reader) capabilities.
  In this case, both the data producer (calling `DBCreate()`) and each consumer (calling `DBOpen()`) must also pass the `DB_CONCURRENT` flag OR'd into the `mode` argument in their respective calls.

  In Fortran, an integer represent the file's id is returned.
  That integer is then used as the database file id in all functions to read and write data from the file.

  Note that regardless of what type of file is created, it can still be read on any machine.

  See notes in the documentation on `DBOpen` regarding use of the `DB_UNKNOWN` driver type.

{{ EndFunc }}

## `DBOpen()`

* **Summary:** Open an existing Silo file.

* **C Signature:**

  ```
  DBfile *DBOpen (char *name, int type, int mode)
  ```

* **Fortran Signature:**

  ```
  integer function dbopen(name, lname, type, mode,
     dbid)
  ```

  returns database file handle in dbid.

* **Arguments:**

  Arg name | Description
  :---|:---
  `name` | Name of the file to open. Can be either an absolute or relative path.
  `type` | The `type` of file to open. One of the predefined types, typically `DB_UNKNOWN`, `DB_PDB`, or `DB_HDF5`. However, there are other options as well as subtle but important issues in using them. So, read description, below for more details.
  `mode` | The `mode` of the file to open. Pass `DB_READ` or `DB_APPEND`. Optionally, `DB_APPEND` can be OR'd with `DB_PERF_OVER_COMPAT` or `DB_COMPAT_OVER_PERF` (see [`DBSetCompatibilityMode`](globals.md#dbsetcompatibilitymode). OR'ing of `DB_READ` with compatibility mode flags is not allowed. Optionally, either `DB_READ` or `DB_APPEND` can be OR'd with `DB_CONCURRENT` when another process intends to read the file's contents concurrently with a writer and vise versa.

* **Returned value:**

  DBOpen returns a `DBfile` pointer on success and a `NULL` on failure.

* **Description:**

  The `DBOpen` function opens an existing Silo file.
  If the file `type` passed here is `DB_UNKNOWN`, Silo will attempt to guess at the file `type` by iterating through the known types attempting to open the file with each driver until it succeeds.
  This iteration may incur a small performance penalty.
  In addition, use of `DB_UNKNOWN` can have other undesirable behavior described below.
  So, if at all possible, it is best to open using a specific `type`.
  See [`DBGetDriverTypeFromPath`](#dbgetdrivertypefrompath)() for a function that uses cheap heuristics to determine the driver `type` given a candidate filename.

  When writing general purpose code to read Silo files and you cannot know for certain ahead of time what the correct driver to use is, there are a few options.

  First, you can iterate over the available driver ids, calling `DBOpen()` using each one until one of them succeeds.
  But, that is exactly what the `DB_UNKNOWN` driver does so there is no need for a Silo client to have to write that code.
  In addition, if you have a specific preference of order of drivers, you can use `DBSetUnknownDriverPriorities()` to specify that ordering.

  Undesirable behavior with `DB_UNKNOWN` can occur when the specified file can be successfully opened using multiple available drivers and/or file options sets and it succeeds with one or one using options the caller neither expected or intended and/or which offers poorer performance.
  See [`DBSetUnknownDriverPriorities`](#dbsetunknowndriverpriorities) for a way to specify the order of drivers tried by the `DB_UNKNOWN` driver.

  Indeed, in order to use a specific VFD (see [`DBCreate`](#dbcreate)) in HDF5, it is necessary to pass the specific `DB_HDF5_XXX` argument in this call or to set the unknown driver priorities such that whatever specific HDF5 VFD(s) are desired are tried first before falling back to other, perhaps less desirable ones.

  The `mode` parameter allows a user to append to an existing Silo file.
  If a file is opened with a `mode` of `DB_APPEND`, the file will support write operations as well as read operations.

{{ EndFunc }}

## `DBFlush()`

* **Summary:** 

* **C Signature:**

  ```
  int DBFlush(DBfile *dbile)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | the file to be flushed
  
* **Returned value:**

  Zero on success; -1 on failure.

* **Description:**

Flush any changes to a file to disk without having to actually close the file.

{{ EndFunc }}

## `DBClose()`

* **Summary:** Close a Silo database.

* **C Signature:**

  ```
  int DBClose (DBfile *dbfile)
  ```

* **Fortran Signature:**

  ```
  integer function dbclose(dbid)
  ```


* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Database file pointer.

* **Returned value:**

  DBClose returns zero on success and -1 on failure.

* **Description:**

  The `DBClose` function closes a Silo database.

  **Note:** For files produced by the HDF5 driver, the occasional bug in the Silo library can lead to situations where `DBClose()` might fail to actually close the file.
  There is logic in Silo to try to detect this if it is happening and then issue a warning or error message.
  However, a caller may not be paying attention to Silo function return values or error messages.

{{ EndFunc }}

## `DBGetToc()`

* **Summary:** Get the table of contents of a Silo database.

* **C Signature:**

  ```
  DBtoc *DBGetToc (DBfile *dbfile)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Database file pointer.


* **Returned value:**

  `DBGetToc` returns a pointer to a [`DBtoc`](header.md#dbtoc) structure on success and `NULL` on error.

* **Description:**

  The `DBGetToc` function returns a pointer to a [`DBtoc`](header.md#dbtoc) structure, which contains the names of the various Silo object contained in the Silo database.
  The returned pointer points into Silo private space and must not be modified or freed.
  Also, calls to `DBSetDir` will free the [`DBtoc`](header.md#dbtoc) structure, invalidating the pointer returned previously by `DBGetToc`.

{{ EndFunc }}

## File-level properties

There are a number of methods that control overall *behavior* either globally to the whole library or for a specific file.
These are all documented in the [Global Library Behavior](globals.md) section.

* [`DBSetAllowOverwritesFile()`](globals.md#dbsetallowoverwritesfile)
* [`DBGetAllowOverwritesFile()`](globals.md#dbgetallowoverwritesfile)
* [`DBSetAllowEmptyObjectsFile()`](globals.md#dbsetallowemptyobjectsfile)
* [`DBGetAllowEmptyObjectsFile()`](globals.md#dbgetallowemptyobjectsfile)
* [`DBSetDataReadMask2File()`](globals.md#dbsetdatareadmask2file)
* [`DBGetDataReadMask2File()`](globals.md#dbgetdatareadmask2file)
* [`DBSetEnableChecksumsFile()`](globals.md#dbsetenablechecksumsfile)
* [`DBGetEnableChecksumsFile()`](globals.md#dbgetenablechecksumsfile)
* [`DBSetCompressionFile()`](globals.md#dbsetcompressionfile)
* [`DBGetCompressionFile()`](globals.md#dbgetcompressionfile)
* [`DBSetFriendlyHDF5NamesFile()`](globals.md#dbsetfriendlyhdf5namesfile)
* [`DBGetFriendlyHDF5NamesFile()`](globals.md#dbgetfriendlyhdf5namesfile)
* [`DBSetDeprecateWarningsFile()`](globals.md#dbsetdeprecatewarningsfile)
* [`DBGetDeprecateWarningsFile()`](globals.md#dbgetdeprecatewarningsfile)

## `DBFileName()`

* **Summary:** 

* **C Signature:**

  ```
  char const *DBFileName(DBfile *dbfile)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | the Silo database file for which the name is to be queried.
  
* **Returned value:**

  Always succeeds. May return string containing "unknown".

{{ EndFunc }}

## `DBFileVersion()`

* **Summary:** Version of the Silo library used to create the specified file

* **C Signature:**

  ```
  char const *DBFileVersion(DBfile *dbfile)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Database file handle


* **Returned value:**

  A character string representation of the version number of the Silo library that was used to create the Silo file.
  The caller should **not** free the returned string.

* **Description:**

  Note, that this is distinct from (e.g. can be the same or different from) the version of the Silo library returned by the `DBVersion()` function.

  `DBFileVersion()`, here, returns the version of the Silo library that was used when `DBCreate()` was called on the specified file.
  `DBVersion()` returns the version of the Silo library the executable is currently linked with.

  Most often, these two will be the same.
  But, not always.
  Also note that although is possible that a single Silo file may have contents created within it from multiple versions of the Silo library, a call to this function will return *only* the version that was in use when `DBCreate()` was called; that is when the file was first created.

{{ EndFunc }}

## `DBFileVersionDigits()`

* **Summary:** Return integer digits of file version number

* **C Signature:**

  ```
  int DBFileVersionDigits(const DBfile *dbfile,
      int *maj, int *min, int *pat, int *pre)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Silo database file handle
  `maj` | Pointer to returned major version digit
  `min` | Pointer to returned minor version digit
  `pat` | Pointer to returned patch version digit
  `pre` | Pointer to returned pre-release version digit (if any)

* **Returned value:**

  Zero on success.
  Negative value on failure.

{{ EndFunc }}

## `DBFileVersionGE()`

* **Summary:** Greater than or equal comparison for version of the Silo library a given file was created with

* **C Signature:**

  ```
  int DBFileVersionGE(DBfile *dbfile, int Maj, int Min, int Pat)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Database file handle
  `Maj` | Integer major version number
  `Min` | Integer minor version number
  `Pat` | Integer patch version number


* **Returned value:**

  One (1) if the version number of the library used to create the specified file is greater than or equal to the version number specified by Maj, Min, Pat arguments, zero (0) otherwise.
  A negative value is returned if a failure occurs.

{{ EndFunc }}

## `DBVersionGEFileVersion()`

* **Summary:** Compare library version with file version

* **C Signature:**

  ```
  int DBVersionGEFileVersion(const DBfile *dbfile)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Silo database file handle obtained with a call to DBOpen


* **Returned value:**

  Non-zero if the library version is greater than or equal to the file version.
  Zero otherwise.

{{ EndFunc }}

## `DBSortObjectsByOffset()`

* **Summary:** Sort list of object names by order of offset in the file

* **C Signature:**

  ```
  int DBSortObjectsByOffset(DBfile *, int nobjs,
      const char *const *const obj_names, int *ordering)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `DBfile` | Database file pointer.
  `nobjs` | Number of object names in `obj_names`.
  `obj_names` | The list of object names to order.
  `ordering` | Returned integer array of relative order of seek offset in the file of each object. For example, if `ordering[i]==k`, that means the object whose name is `obj_names[i]` occurs kth when the objects are ordered according to offset at which they exist in the file.

* **Returned value:**

  0 on success; -1 on failure.
  The only possible reason for failure is if the HDF5 driver is being used to read the file and Silo is not compiled with HDF5 version 1.8 or later.

* **Description:**

  The intention of this function is to permit applications reading Silo files to order their reads in such a way that objects are read in the order in which they occur in the file.
  This can have a positive impact on I/O performance, particularly using a block-oriented VFD such as the Silo VFD as it can reduce and/or eliminate unnecessary block preemption.
  That said, the degree to which ordering reads in this way effects performance is not known.

{{ EndFunc }}

## `DBMkDir()`

* **Summary:** Create a new directory in a Silo file.

* **C Signature:**

  ```
  int DBMkDir (DBfile *dbfile, char const *dirname)
  ```

* **Fortran Signature:**

  ```
  integer function dbmkdir(dbid, dirname, ldirname, status)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Database file pointer.
  `dirname` | Name of the directory to create.

* **Returned value:**

  DBMkDir returns zero on success and -1 on failure.

* **Description:**

  The `DBMkDir` function creates a new directory in the Silo file as a child of the current directory (see [`DBSetDir`](#dbsetdir)).
  The directory name may be an absolute path name similar to `"/dir/subdir"`, or may be a relative path name similar to `"../../dir/subdir"`.

{{ EndFunc }}

## `DBMkDirP()`

* **Summary:** Create a new directory, as well as any necessary intervening directories, in a Silo file.

* **C Signature:**

  ```
  int DBMkDirP(DBfile *dbfile, char const *dirname)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Database file pointer.
  `dirname` | Name of the directory to create.

* **Returned value:**

  Returns zero on success and -1 on failure.

* **Description:**

  The `DBMkDirP` function creates a new directory in the Silo file including making any necessary intervening directories.
  This is the Silo equivalent of Unix' `mkdir -p`.
  The directory name may be an absolute path name similar to `"/dir/subdir"`, or may be a relative path name similar to `"../../dir/subdir"`.

{{ EndFunc }}

## `DBSetDir()`

* **Summary:** Set the current directory within the Silo database.

* **C Signature:**

  ```
  int DBSetDir (DBfile *dbfile, char const *pathname)
  ```

* **Fortran Signature:**

  ```
  integer function dbsetdir(dbid, pathname, lpathname)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Database file pointer.
  `pathname` | Path name of the directory. This can be either an absolute or relative path name.

* **Returned value:**

  DBSetDir returns zero on success and -1 on failure.

* **Description:**

  The `DBSetDir` function sets the current directory within the given Silo database.
  Also, calls to `DBSetDir` will free the [`DBtoc`](header.md#dbtoc) structure, invalidating the pointer returned previously by `DBGetToc`.
  `DBGetToc` must be called again in order to obtain a pointer to the new directory's `DBtoc` structure.

{{ EndFunc }}

## `DBGetDir()`

* **Summary:** Get the name of the current directory.

* **C Signature:**

  ```
  int DBGetDir (DBfile *dbfile, char *dirname)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | Database file pointer.
  `dirname` | Returned current directory name. The caller must allocate space for the returned name. The maximum space used is `256` characters, including the `NULL` terminator.

* **Returned value:**

  DBGetDir returns zero on success and -1 on failure.

* **Description:**

  The `DBGetDir` function returns the name of the current directory.

{{ EndFunc }}

## `DBCpDir()`

* **Summary:** Copy a directory hierarchy from one Silo file to another.

* **C Signature:**

  ```
  int DBCpDir(DBfile *srcFile, const char *srcDir,
      DBfile *dstFile, const char *dstDir)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `srcFile` | Source database file pointer.
  `srcDir` | Name of the directory within the source database file to copy.
  `dstFile` | Destination database file pointer.
  `dstDir` | Name of the top-level directory in the destination file. If an absolute path is given, then all components of the path except the last must already exist. Otherwise, the new directory is created relative to the current working directory in the file.


* **Returned value:**

  DBCpDir returns 0 on success, -1 on failure

* **Description:**

  `DBCpDir` copies an entire directory hierarchy from one Silo file to another.

  Note that this function is available only on the HDF5 driver and only if the Silo library has been compiled with HDF5 version 1.8 or later.
  This is because the implementation exploits functionality available only in versions of HDF5 1.8 and later.

{{ EndFunc }}

## `DBCpListedObjects()`

* **Summary:** Copy lists of objects from one Silo database to another

* **C Signature:**

  ```
  int DBCpListedObjects(int nobjs,
      DBfile *srcDb, char const * const *srcObjList,
      DBfile *dstDb, char const * const *dstObjList)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `nobjs` | The number of objects to be copied (e.g. number of strings in srcObjList)
  `srcDb` | The Silo database to be used as the source of the copies
  `srcObjList` | An array of `nobjs` strings of the (path) names of objects to be copied. See description for interpretation of relative path names.
  `dstDB` | The Silo database to be used as the destination of the copies.
  `dstObjList` | [Optional] An optional array of `nobjs` strings of the (path) names where the objects are to be copied in `dstDb`. If any entry in `dstObjList` is `NULL` or is a string of zero length, this indicates that object in the `dstDb` will have the same (path) name as the corresponding object (path) name given in `srcObjList`. If the entire `dstObjList` is `NULL`, then this is true for all objects. See description for interpretation of relative (path) names.

* **Returned value:**

  Returns 0 on success, -1 on failure

* **Description:**

  Note that this function is available only if both Silo databases are from the HDF5 driver and only if the Silo library has been compiled with HDF5 version 1.8 or later.
  This is because the implementation exploits functionality available only in versions of HDF5 1.8 and later.

  Directories required in the destination database to satisfy the path names given in the `dstObjList` are created as necessary.
  There is no need for the caller to pre-create any directories in the destination database.

  Relative path names in the `srcObjList` are interpreted relative to the source database's current working directory.
  Likewise, relative path names in the dstObjectList are interpreted relative to the destination databases's current working directory.
  Of course, if objects are specified using absolute path names, then copies will occur regardless of source or destination databases's current working directory.

  If an object specified in the `srcObjList` is itself a directory, then the entire directory tree rooted at that name will be copied to the destination database.

  If `dstObjList` is `NULL`, then it is assumed that all the objects to be copied will be copied to paths in the destination database that are intended to be the same as those in `srcObjList`.
  If `dstObjList` is non-`NULL`, but any given entry is either `NULL` or a string of zero length, than that object's destination name will be assumed the same as its equivalent `srcObjList` entry.
  Note, when using `NULL`s relative paths in `srcObjList` will appear in destination databases relative to the destination database's current working directory.

  If dstObjList[i] ends in a '/' character or identifies a directory that existed in the destination database either before `DBCpListedObjects` was called or that was created on behalf of a preceding object copy within the execution of DBCpListedObjects, then the source object will be copied to that directory with its original (source) name.
  This is equivalent to the behavior of the file system command `cp foo /gorfo/bar/` or `cp foo /gorfo/bar` when `bar` exists as a directory.

  Finally, users should be aware that if there are numeric architecture differences between the host where the source object data was produced and the host where this copy operation is being performed, then in all likelihood the destination copies of any floating point data may not match bit-for-bit with the source data.
  This is because data conversion may have been involved in the process of reading the data into memory and writing the copy back out.

  For example, suppose we have two databases...

  1. `DBfile *dbfile` ("dir.silo") containing...

     * `/ucd_dir/ucdmesh` (ucd mesh object)
     * `/tri_dir/trimesh` (ucd mesh object) <-- current working directory
     * `/quad_dir/quadmesh` (quad mesh object)

  2. `DBfile *dbfile2` ("dir2.silo") containing...

     * `/tmp` <-- current working directory

  And the following source and destination lists...

  * `char *srcObjs[] = {"trimesh", "../ucd_dir/ucdmesh", "/quad_dir/quadmesh", "trimesh"};`
  * `char *dstObjs[] = {"/tmp/foo/bar/gorfo", "../foogar", 0, "foo"};`

  Then, `DBCpListedObjects(4, dbfile, srcObjs, dbfile2, dstObjs)` does the following...

  1. Copies `trimesh` in `cwg` of `dbfile` to `/tmp/foo/bar/gorfo` in `dbfile2`
  2. Copies `../ucd_dir/ucdmesh` of `dbfile` to `/foogar` in `dbfile2`
  3. Copies `/quad_dir/quadmesh` to `cwg` (e.g. `/tmp`) `/tmp/quadmesh` in `dbfile2`
  4. Copies `trimesh` in `cwg` of `dbfile` to `cwg/foo` (`/tmp/foo/trimesh` in dbfile2`

{{ EndFunc }}

## `DBCp()`

* **Summary:** Generalized copy function emulating unix `cp`

* **C Signature:**

  ```
  int DBCp(char const *opts, DBfile *srcFile, DBfile *dstFile, ...)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `opts` | A space-separated options string.
  `srcFile` | The file holding the source objects to be copied.
  `dstFile` | The file holding the destination object paths into which source objects will be copied.
  `...` | A `varargs` list of remaining arguments terminated in `DB_EOA` (end of arguments).

* **Return value:**

  0 on success. -1 on failure.

* **Description:**

  This is the Silo equivalent of a unix `cp` command obeying all unix semantics and applicable flags and works on either *above* any driver.
  This means the method can be used for PDB files, HDF5 files or even a mixture of drivers.
  In particular, it can be used to copy a whole file from one low-level driver to another.

  * Copy a single source object to a single destination object
    * ```
      DBCp("", srcFile, dstFile, srcPATH, dstPATH, DB_EOA);`
      ```
  * Copy multiple source objects to single destination (dir) object
    * ```
      DBCp("", srcFile, dstFile, srcPATH1, srcPATH2, ..., dstDIR, DB_EOA);`
      ```
  * Copy multiple source objects to multiple destination objects
    * ```
      DBCp("-2", DBfile *srcFile, DBfile *dstFile,
              char const *srcPATH1, char const *dstPATH1,
              char const *srcPATH2, char const *dstPATH2,
              char const *srcPATH3, char const *dstPATH3, ..., DB_EOA);
      ```
  
  `srcFile` and `dstFile` may be the same Silo file.
  `srcFile` cannot be null.
  `dstFile` may be null in which case it is assumed same as `srcFile`.
  The argument list *must* be terminated by the `DB_EOA` sentinel.
  Just as for unix `cp`, the options and their meanings are...
 
    * `-R/-r`: recurse on any directory objects.
    * `-L/-P`: dereference links / never dereference links.
    * `-d`: preserve links.
    * `-s/-l`: don't actually copy, just sym/hard link (only possible when srcFile==dstFile).

  There are some additional options specific to Silo's `DBCp`...

    * `-2`: treat varargs list of args as `src/dst` path pairs and where any `NULL` `dst` is inferred to have same path as associated `src` except that relative paths are interpreted relative to `dst` file's cwg.
    * `-1`: like `-2` except caller passes only `src` paths.
      All `dst` paths are inferred to be same as associated `src` path.
      The `dst` file's cwg will then determine how any relative `src` paths are interpreted.
    * `-3` only 3 args follow the `dstFile` arg...
      * `int N`: number of objects in the following lists.
      * `DBCAS_t srcPathNames`: list of `N` source path names.
      * `DBCAS_t dstPathNames`: list of `N` destination path names.
      * In this case, a terminating `DB_EOA` is not necessary.
    * `-4`: Like `-3`, except 3rd arg is treated as a single `dst` dir name.
      * `int N`: number of paths in `srcPathNames`.
      * `DBCAS_t srcPathNames`: list of `N` source path names.
      * `char const *dstDIR`: pre-existing destination dir path.
      * In this case, a terminating `DB_EOA` is not necessary.
    * `-5`: Internal use only...like `-4` except used only internally when `DBCp` recursively calls itself.
 
  **Other rules:**

    * If any `src` is a dir, then the operation is an error without `-R/-r` option.
    * If corresponding `dst` exists and is a dir, `src` is copied *into* (e.g. becomes a child of) `dst`.
    * If corresponding `dst` exists and is **not** a dir (e.g. is just a normal Silo object), then it is an error if src is not also the same kind of Silo object.
     The copy overwrites (destructively) `dst`.
     However, if the file space `dst` object occupies is smaller than that needed to copy `src`, behavior is indeterminate but will almost certainly result in the `dst` file (not just the `dst` object) being corrupted.

  If none of the preceding numeric arguments are specified, then the varags list of args is treated as (default) where the last is a pre-existing destination directory and all the others are the paths of source objects to be copied into that directory.

 Relative path names are interpreted relative to the current working directory of the associated (`src` or `dst`) file when DBCp is invoked.

 In all the different ways this function can be invoked, there are really just two fundamentally different interpretations of the list(s) of names.
 Either each source path is paired also with a destination path or all source paths go into a single destination path which, just as for linux `cp`, must then also be a directory already present in the destination.

{{ EndFunc }}

## `DBGrabDriver()`

* **Summary:** Obtain the low-level driver file handle

* **C Signature:**

  ```
  void *DBGrabDriver(DBfile *file)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `file` | The Silo database `file` handle.

* **Returned value:**

  A void pointer to the low-level driver's file handle on success.
  `NULL`(0) on failure.

* **Description:**

  This method is used to obtain the low-level driver's file handle.
  For example, one can use it to obtain the HDF5 `hid_t` returned from the `H5Fopen`/`H5Fcreate` call.
  The caller is responsible for casting the returned pointer to a pointer to the correct type.
  Use `DBGetDriverType()` to obtain information on the type of driver currently in use.

  When the low-level driver's file handle is grabbed, all Silo-level operations on `file` are prevented until `file` is UNgrabbed.
  For example, after a call to DBGrabDriver, calls to functions like `DBPutQuadmesh` or `DBGetCurve` will fail until the driver is UNgrabbed using `DBUngrabDriver()`.

  Notes:

  As far as the integrity of a Silo `file` goes, grabbing is inherently dangerous.
  If the client is not careful, one can easily wind up corrupting the `file` for the Silo library (though all may be 'normal' for the underlying driver library).
  Therefore, to minimize the likelihood of corrupting the Silo `file` while it is grabbed, it is recommended that all operations with the low-level driver grabbed be confined to a separate sub-directory in the silo `file`.
  That is, one should not mix writing of Silo objects and low-level driver objects in the same directory.
  To achieve this, before grabbing, create the desired directory and descend into it using Silo's `DBMkDir()` and `DBSetDir()` functions.
  Then, grab the driver and do all the work with the low-level driver that is necessary.
  Finally, ungrab the driver and immediately ascend out of the directory using Silo's DBSetDir("..").

  For reasons described above, if problems occur on files that have been grabbed, users will likely be asked to re-produce the problem on a similar `file` that has **not** been grabbed to rule out possible corruption from grabbing.

{{ EndFunc }}

## `DBUngrabDriver()`

* **Summary:** Ungrab the low-level file driver

* **C Signature:**

  ```
  int DBUngrabDriver(DBfile *file, const void *drvr_hndl)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `file` | The Silo database `file` handle.
  `drvr_hndl` | The low-level driver handle.

* **Returned value:**

  The driver type on success, `DB_UNKNOWN` on failure.

* **Description:**

  This function returns the Silo `file` to an ungrabbed state, permitting Silo calls to again proceed as normal.

{{ EndFunc }}

## `DBGetDriverType()`

* **Summary:** Get the type of driver for the specified file

* **C Signature:**

  ```
  int DBGetDriverType(const DBfile *file)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `file` | A Silo database `file` handle.


* **Returned value:**

  `DB_UNKNOWN` for failure.
  Otherwise, the specified driver type is returned

* **Description:**

  This function returns the type of driver used for the specified `file`.
  If you want to ask this question without actually opening the file, use `DBGetDriverTypeFromPath`.

{{ EndFunc }}

## `DBGetDriverTypeFromPath()`

* **Summary:** Guess the driver type used by a file with the given path name

* **C Signature:**

  ```
  int DBGetDriverTypeFromPath(char const *path)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `path` | Path to a file on the file system

* **Returned value:**

  `DB_UNKNOWN` on failure to determine type.
  Otherwise, the driver type such as `DB_PDB` or `DB_HDF5`.

* **Description:**

  This function examines the first few bytes of the file for tell-tale signs of whether it is a PDB file or an HDF5 file.

  If it is a PDB file, it cannot distinguish between a file generated by `DB_PDB` driver and `DB_PDBP` (PDB Proper) driver.
  It will always return `DB_PDB` for a PDB file.

  If the file is an HDF5, the function is currently not implemented to distinguish between various HDF5 VFDs the file may have been generated with.
  It will always return `DB_HDF5` for an HDF5 file.

  Note, this function will determine only whether the underlying file is a PDB or HDF5 file.
  It will not however, indicate whether the file is a PDB or HDF5 file that was indeed generated by Silo and is readable by Silo.
  See [`DBInqFile`](#dbinqfile) for a function that will indicate whether the file is indeed a Silo file.
  Note, however, that `DBInqFile` is a more expensive operation.

{{ EndFunc }}

## `DBInqFile()`

* **Summary:** Inquire if filename is a Silo file.

* **C Signature:**

  ```
  int DBInqFile (char const *filename)
  ```

* **Fortran Signature:**

  ```
  integer function dbinqfile(filename, lfilename, is_file)
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `filename` | Name of file.

* **Returned value:**

  `DBInqFile` returns 0 if filename is not a Silo file, a positive number if filename is a Silo file, and a negative number if an error occurred.

* **Description:**

  The `DBInqFile` function is mainly used for its return value, as seen above.

  Prior to version 4.7.1 of the Silo library, this function could return false positives when the `filename` referred to a PDB file that was **not** created by Silo.
  The reason for this is that all this function really did was check whether or not `DBOpen` would succeed on the file.

  Starting in version 4.7.1 of the Silo library, this function will attempt to count the number of Silo objects (not including directories) in the first non-empty directory it finds.
  If it cannot find any Silo objects in the file, it will return zero (0) indicating the file is **not** a Silo file.

  Because very early versions of the Silo library did not store anything to a Silo file to distinguish it from a PDB file, it is conceivable that this function will return false negatives for very old, empty Silo files.
  But, that case should be rare.

  Similar problems do not exist for HDF5 files because Silo's HDF5 driver has always stored information in the HDF5 file which helps to distinguish it as a Silo file.

{{ EndFunc }}

## `DBInqFileHasObjects()`

* **Summary:** Determine if an open file has any Silo objects

* **C Signature:**

  ```
  int DBInqFileHasObjects(DBfile *dbfile)
  ```

* **Fortran Signature:**

  ```
  None
  ```

* **Arguments:**

  Arg name | Description
  :---|:---
  `dbfile` | The Silo database file handle


* **Description:**

  Examine an open file for existence of any Silo objects.

{{ EndFunc }}

## `_silolibinfo`

* **Summary:** A character array written by Silo to root directory indicating the Silo library version number used to generate the file

* **C Signature:**

  ```C
      int n;
      char vers[1024];
      sprintf(vers, "silo-4.6");
      n = strlen(vers);
      DBWrite(dbfile, "_silolibinfo", vers, &n, 1, DB_CHAR);
  ```

* **Description:**

  This is a simple array variable written at the root directory in a Silo file that contains the Silo library version string.
  It cannot be disabled.

{{ EndFunc }}

## `_hdf5libinfo`

* **Summary:** character array written by Silo to root directory indicating the HDF5 library version number used to generate the file

* **C Signature:**

  ```C
      int n;
      char vers[1024];
      sprintf(vers, "hdf5-1.6.6");
      n = strlen(vers);
      DBWrite(dbfile, "_hdf5libinfo", vers, &n, 1, DB_CHAR);
  ```

* **Description:**

  This is a simple array variable written at the root directory in a Silo file that contains the HDF5 library version string.
  It cannot be disabled.
  Of course, it exists, only in files created with the HDF5 driver.

{{ EndFunc }}

## `_was_grabbed`

* **Summary:** single integer written by Silo to root directory whenever a Silo file has been grabbed.

* **C Signature:**

  ```C
      int n=1;
      DBWrite(dbfile, "_was_grabbed", &n, &n, 1, DB_INT);
  ```

* **Description:**

  This is a simple array variable written at the root directory in a Silo whenever a Silo file has been grabbed by the `DBGrabDriver()` function.
  It cannot be disabled.

{{ EndFunc }}

