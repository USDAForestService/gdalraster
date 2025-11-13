# Class wrapping the GDAL VSIVirtualHandle API for binary file I/O

`VSIFile` provides bindings to the GDAL VSIVirtualHandle API.
Encapsulates a `VSIVirtualHandle`
(<https://gdal.org/en/stable/api/cpl_cpp.html#_CPPv416VSIVirtualHandle>).
This API abstracts binary file I/O across "regular" file systems, URLs,
cloud storage services, Zip/GZip/7z/RAR, and in-memory files. It
provides analogs of several Standard C file I/O functions, allowing
virtualization of disk I/O so that non-file data sources can be made to
appear as files.

`VSIFile` is a C++ class exposed directly to R (via
`RCPP_EXPOSED_CLASS`). Methods of the class are accessed using the `$`
operator.

## Arguments

- filename:

  Character string containing the filename to open. It may be a file in
  a regular local filesystem, or a filename with a GDAL /vsiPREFIX/ (see
  <https://gdal.org/en/stable/user/virtual_file_systems.html>).

- access:

  Character string containing the access requested (i.e., `"r"`, `"r+"`,
  `"w"`, `"w+`). Defaults to `"r"`. Binary access is always implied and
  the "b" does not need to be included in `access`.

  |            |                            |                    |
  |------------|----------------------------|--------------------|
  | **Access** | **Explanation**            | **If file exists** |
  | `"r"`      | open file for reading      | read from start    |
  | `"r+"`     | open file for read/write   | read from start    |
  | `"w"`      | create file for writing    | destroy contents   |
  | `"w+"`     | create file for read/write | destroy contents   |

- options:

  Optional character vector of `NAME=VALUE` pairs specifying
  filesystem-dependent options (GDAL \>= 3.3, see Details).

## Value

An object of class `VSIFile` which contains a pointer to a
`VSIVirtualHandle`, and methods that operate on the file as described in
Details.

## Note

File offsets are given as R `numeric` (i.e., `double` type), optionally
carrying the
[`bit64::integer64`](https://rdrr.io/pkg/bit64/man/bit64-package.html)
class attribute. They are returned as `numeric` with the `integer64`
class attribute attached. The `integer64` type is signed, so the maximum
file offset supported by this interface is `9223372036854775807` (the
value of `bit64::lim.integer64()[2]`).

Some virtual file systems allow only sequential write, so no seeks or
read operations are then allowed (e.g., AWS S3 files with /vsis3/).
Starting with GDAL 3.2, a configuration option can be set with:

    set_config_option("CPL_VSIL_USE_TEMP_FILE_FOR_RANDOM_WRITE", "YES")

in which case random-write access is possible (involves the creation of
a temporary local file, whose location is controlled by the `CPL_TMPDIR`
configuration option). In this case, setting `access` to `"w+"` may be
needed for writing with seek and read operations (if creating a new
file, otherwise, `"r+"` to open an existing file), while `"w"` access
would allow sequential write only.

## Usage (see Details)

    ## Constructors
    vf <- new(VSIFile, filename)
    # specifying access:
    vf <- new(VSIFile, filename, access)
    # specifying access and options (both required):
    vf <- new(VSIFile, filename, access, options)

    ## Methods
    vf$seek(offset, origin)
    vf$tell()
    vf$rewind()
    vf$read(nbytes)
    vf$write(object)
    vf$eof()
    vf$truncate(new_size)
    vf$flush()
    vf$ingest(max_size)

    vf$close()
    vf$open()
    vf$get_filename()
    vf$get_access()
    vf$set_access(access)

## Details

### Constructors

`new(VSIFile, filename)`  
Returns an object of class `VSIFile`, or an error is raised if a file
handle cannot be obtained.

`new(VSIFile, filename, access)`  
Alternate constructor for passing `access` as a character string (e.g.,
`"r"`, `"r+"`, `"w"`, `"w+"`). Returns an object of class `VSIFile` with
an open file handle, or an error is raised if a file handle cannot be
obtained.

`new(VSIFile, filename, access, options)`  
Alternate constructor for passing `access` as a character string, and
`options` as a character vector of "NAME=VALUE" pairs (all arguments
required, GDAL \>= 3.3 required for `options` support).

The `options` argument is highly file system dependent. Supported
options as of GDAL 3.9 include:

- MIME headers such as Content-Type and Content-Encoding are supported
  for the /vsis3/, /vsigs/, /vsiaz/, /vsiadls/ file systems.

- DISABLE_READDIR_ON_OPEN=YES/NO (GDAL \>= 3.6) for /vsicurl/ and other
  network-based file systems. By default, directory file listing is
  done, unless YES is specified.

- WRITE_THROUGH=YES (GDAL \>= 3.8) for Windows regular files to set the
  FILE_FLAG_WRITE_THROUGH flag to the `CreateFile()` function. In that
  mode, the data are written to the system cache but are flushed to disk
  without delay.

### Methods

`$seek(offset, origin)`  
Seek to a requested `offset` in the file. `offset` is given as a
positive numeric scalar, optionally as
[`bit64::integer64`](https://rdrr.io/pkg/bit64/man/bit64-package.html)
type. `origin` is given as a character string, one of `SEEK_SET`,
`SEEK_CUR` or `SEEK_END`. Package global constants are defined for
convenience, so these can be passed unquoted. Note that `offset` is an
unsigned type, so `SEEK_CUR` can only be used for positive seek. If
negative seek is needed, use:

    vf$seek(vf$tell() + negative_offset, SEEK_SET)

Returns `0` on success or `-1` on failure.

`$tell()`  
Returns the current file read/write offset in bytes from the beginning
of the file. The return value is a numeric scalar carrying the
`integer64` class attribute.

`$rewind()`  
Rewind the file pointer to the beginning of the file. This is equivalent
to `vf$seek(0, SEEK_SET)`. No return value, called for that side effect.

`$read(nbytes)`  
Read `nbytes` bytes from the file at the current offset. Returns an R
`raw` vector, or `NULL` if the operation fails.

`$write(object)`  
Write bytes to the file at the current offset. `object` is a `raw`
vector. Returns the number of bytes successfully written, as numeric
scalar carrying the `integer64` class attribute. See also base R
[`charToRaw()`](https://rdrr.io/r/base/rawConversion.html) /
[`rawToChar()`](https://rdrr.io/r/base/rawConversion.html), convert to
or from raw vectors, and
[`readBin()`](https://rdrr.io/r/base/readBin.html) /
[`writeBin()`](https://rdrr.io/r/base/readBin.html) which read binary
data from or write binary data to a raw vector.

`$eof()`  
Test for end of file. Returns `TRUE` if an end-of-file condition
occurred during the previous read operation. The end-of-file flag is
cleared by a successful call to `$seek()`.

`$truncate(new_size)`  
Truncate/expand the file to the specified `new_size`, given as a
positive numeric scalar, optionally as
[`bit64::integer64`](https://rdrr.io/pkg/bit64/man/bit64-package.html)
type. Returns `0` on success.

`$flush()`  
Flush pending writes to disk. For files in write or update mode and on
file system types where it is applicable, all pending output on the file
is flushed to the physical disk. On Windows regular files, this method
does nothing, unless the `VSI_FLUSH=YES` configuration option is set
(and only when the file has not been opened with the `WRITE_THROUGH`
option). Returns `0` on success or `-1` on error.

`$ingest(max_size)`  
Ingest a file into memory. Read the whole content of the file into a
`raw` vector. `max_size` is the maximum size of file allowed, given as a
numeric scalar, optionally as
[`bit64::integer64`](https://rdrr.io/pkg/bit64/man/bit64-package.html)
type. If no limit, set to a negative value. Returns a `raw` vector, or
`NULL` if the operation fails.

`$close()`  
Closes the file. The file should always be closed when I/O has been
completed. Returns `0` on success or `-1` on error.

`$open()`  
This method can be used to re-open the file after it has been closed,
using the same `filename`, and same `options` if any are set. The file
will be opened using `access` as currently set. The `$set_access()`
method can be called to change the requested access while the file is
closed. No return value. An error is raised if a file handle cannot be
obtained.

`$get_filename()`  
Returns a character string containing the `filename` associated with
this `VSIFile` object (the `filename` originally used to create the
object).

`$get_access()`  
Returns a character string containing the `access` as currently set on
this `VSIFile` object.

`$set_access(access)`  
Sets the requested read/write access on this `VSIFile` object, given as
a character string (i.e., `"r"`, `"r+"`, `"w"`, `"w+"`). The access can
be changed only while the `VSIFile` object is closed, and will apply
when it is re-opened with a call to `$open()`. Returns `0` on success or
`-1` on error.

## See also

GDAL Virtual File Systems (compressed, network hosted, etc...):  
/vsimem, /vsizip, /vsitar, /vsicurl, ...  
<https://gdal.org/en/stable/user/virtual_file_systems.html>

[`vsi_copy_file()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_copy_file.md),
[`vsi_read_dir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_read_dir.md),
[`vsi_stat()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md),
[`vsi_unlink()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_unlink.md)

## Examples

``` r
# The examples make use of the FARSITE LCP format specification at:
# https://gdal.org/en/stable/drivers/raster/lcp.html
# An LCP file is a raw format with a 7,316-byte header. The format
# specification gives byte offets and data types for fields in the header.

lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")

# identify a FARSITE v.4 LCP file
# function to check if the first three fields have valid data
# input is the first twelve raw bytes in the file
is_lcp <- function(bytes) {
  values <- readBin(bytes, "integer", n = 3)
  if ((values[1] == 20 || values[1] == 21) &&
      (values[2] == 20 || values[2] == 21) &&
      (values[3] >= -90 && values[3] <= 90)) {

    return(TRUE)
  } else {
    return(FALSE)
  }
}

vf <- new(VSIFile, lcp_file)
vf
#> C++ object of class VSIFile
#>  Filename : /home/runner/work/_temp/Library/gdalraster/extdata/storm_lake.lcp
#>  Access   : r

vf$read(12) |> is_lcp()
#> [1] TRUE

vf$tell()
#> integer64
#> [1] 12

# read the whole file into memory
bytes <- vf$ingest(-1)
vf$close()
#> [1] 0

# write to a VSI in-memory file
mem_file <- "/vsimem/storml_copy.lcp"
vf <- new(VSIFile, mem_file, "w+")
vf$write(bytes)
#> integer64
#> [1] 252132

vf$tell()
#> integer64
#> [1] 252132
vf$rewind()
vf$tell()
#> integer64
#> [1] 0

vf$seek(0, SEEK_END)
#> [1] 0
(vf$tell() == vsi_stat(lcp_file, "size"))  # TRUE
#> [1] TRUE

vf$rewind()
vf$read(12) |> is_lcp()
#> [1] TRUE

# read/write an integer field
# write invalid data for the Latitude field and then set back
# save the original first
vf$seek(8, SEEK_SET)
#> [1] 0
lat_orig <- vf$read(4)
readBin(lat_orig, "integer")  # 46
#> [1] 46
# latitude -99 out of range
vf$seek(8, SEEK_SET)
#> [1] 0
writeBin(-99L, raw()) |> vf$write()
#> integer64
#> [1] 4
vf$rewind()
vf$read(12) |> is_lcp()  # FALSE
#> [1] FALSE
vf$seek(8, SEEK_SET)
#> [1] 0
vf$read(4) |> readBin("integer")  # -99
#> [1] -99
# set back to original
vf$seek(8, SEEK_SET)
#> [1] 0
vf$write(lat_orig)
#> integer64
#> [1] 4
vf$rewind()
vf$read(12) |> is_lcp()  # TRUE
#> [1] TRUE

# read a vector of doubles - xmax, xmin, ymax, ymin
# 327766.1, 323476.1, 5105082.0, 5101872.0
vf$seek(4172, SEEK_SET)
#> [1] 0
vf$read(32) |> readBin("double", n = 4)
#> [1]  327766.1  323476.1 5105082.0 5101872.0

# read a short int, the canopy cover units
vf$seek(4232, SEEK_SET)
#> [1] 0
vf$read(2) |> readBin("integer", size = 2)  # 1 = "percent"
#> [1] 1

# read the Description field
vf$seek(6804, SEEK_SET)
#> [1] 0
bytes <- vf$read(512)
rawToChar(bytes)
#> [1] "LCP file created by GDAL."

# edit the Description
desc <- paste(rawToChar(bytes),
              "Storm Lake AOI,",
              "Beaverhead-Deerlodge National Forest, Montana.")

vf$seek(6804, SEEK_SET)
#> [1] 0
charToRaw(desc) |> vf$write()
#> integer64
#> [1] 88
vf$close()
#> [1] 0

# verify the file as a raster dataset
ds <- new(GDALRaster, mem_file)
ds$info()
#> Driver: LCP/FARSITE v.4 Landscape File (.lcp)
#> Files: /vsimem/storml_copy.lcp
#> Size is 143, 107
#> Coordinate System is:
#> 
#> Data axis to CRS axis mapping: 1,2,3
#> Origin = (323476.071970863151364,5105081.983031376264989)
#> Pixel Size = (30.000000000000000,-30.000000000000000)
#> Metadata:
#>   DESCRIPTION=LCP file created by GDAL. Storm Lake AOI, Beaverhead-Deerlodge National Forest, Montana.
#>   LATITUDE=46
#>   LINEAR_UNIT=Meters
#> Corner Coordinates:
#> Upper Left  (  323476.072, 5105081.983) 
#> Lower Left  (  323476.072, 5101871.983) 
#> Upper Right (  327766.072, 5105081.983) 
#> Lower Right (  327766.072, 5101871.983) 
#> Center      (  325621.072, 5103476.983) 
#> Band 1 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Elevation
#>   Metadata:
#>     ELEVATION_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif
#>     ELEVATION_MAX=3046
#>     ELEVATION_MIN=-9999
#>     ELEVATION_NUM_CLASSES=-1
#>     ELEVATION_UNIT=0
#>     ELEVATION_UNIT_NAME=Meters
#> Band 2 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Slope
#>   Metadata:
#>     SLOPE_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif
#>     SLOPE_MAX=54
#>     SLOPE_MIN=-9999
#>     SLOPE_NUM_CLASSES=53
#>     SLOPE_UNIT=0
#>     SLOPE_UNIT_NAME=Degrees
#> Band 3 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Aspect
#>   Metadata:
#>     ASPECT_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif
#>     ASPECT_MAX=359
#>     ASPECT_MIN=-9999
#>     ASPECT_NUM_CLASSES=-1
#>     ASPECT_UNIT=2
#>     ASPECT_UNIT_NAME=Azimuth degrees
#> Band 4 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Fuel models
#>   Metadata:
#>     FUEL_MODEL_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif
#>     FUEL_MODEL_MAX=183
#>     FUEL_MODEL_MIN=-9999
#>     FUEL_MODEL_NUM_CLASSES=12
#>     FUEL_MODEL_OPTION=0
#>     FUEL_MODEL_OPTION_DESC=no custom models AND no conversion file needed
#>     FUEL_MODEL_VALUES=0,98,99,101,102,121,122,123,142,162,165,181,183
#> Band 5 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Canopy cover
#>   Metadata:
#>     CANOPY_COV_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif
#>     CANOPY_COV_MAX=75
#>     CANOPY_COV_MIN=-9999
#>     CANOPY_COV_NUM_CLASSES=8
#>     CANOPY_COV_UNIT=1
#>     CANOPY_COV_UNIT_NAME=Percent
#> Band 6 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Canopy height
#>   Metadata:
#>     CANOPY_HT_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif
#>     CANOPY_HT_MAX=390
#>     CANOPY_HT_MIN=-9999
#>     CANOPY_HT_NUM_CLASSES=8
#>     CANOPY_HT_UNIT=3
#>     CANOPY_HT_UNIT_NAME=Meters x 10
#> Band 7 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Canopy base height
#>   Metadata:
#>     CBH_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif
#>     CBH_MAX=100
#>     CBH_MIN=-9999
#>     CBH_NUM_CLASSES=22
#>     CBH_UNIT=3
#>     CBH_UNIT_NAME=Meters x 10
#> Band 8 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Canopy bulk density
#>   Metadata:
#>     CBD_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif
#>     CBD_MAX=34
#>     CBD_MIN=-9999
#>     CBD_NUM_CLASSES=15
#>     CBD_UNIT=3
#>     CBD_UNIT_NAME=kg/m^3 x 100

# retrieve Description from the metadata
# band = 0 for dataset-level metadata, domain = "" for default domain
ds$getMetadata(band = 0, domain = "")
#> [1] "DESCRIPTION=LCP file created by GDAL. Storm Lake AOI, Beaverhead-Deerlodge National Forest, Montana."
#> [2] "LATITUDE=46"                                                                                         
#> [3] "LINEAR_UNIT=Meters"                                                                                  
ds$getMetadataItem(band = 0, mdi_name = "DESCRIPTION", domain = "")
#> [1] "LCP file created by GDAL. Storm Lake AOI, Beaverhead-Deerlodge National Forest, Montana."

ds$close()
```
