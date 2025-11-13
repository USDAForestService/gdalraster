# Create/append to a potentially Seek-Optimized ZIP file (SOZip)

`addFilesInZip()` will create new or open existing ZIP file, and add one
or more compressed files potentially using the seek optimization
extension. This function is basically a wrapper for `CPLAddFileInZip()`
in the GDAL Common Portability Library, but optionally creates a new ZIP
file first (with `CPLCreateZip()`). It provides a subset of
functionality in the GDAL `sozip` command-line utility
(<https://gdal.org/en/stable/programs/sozip.html>). Requires GDAL \>=
3.7.

## Usage

``` r
addFilesInZip(
  zip_file,
  add_files,
  overwrite = FALSE,
  full_paths = TRUE,
  sozip_enabled = NULL,
  sozip_chunk_size = NULL,
  sozip_min_file_size = NULL,
  num_threads = NULL,
  content_type = NULL,
  quiet = FALSE
)
```

## Arguments

- zip_file:

  Filename of the ZIP file. Will be created if it does not exist or if
  `overwrite = TRUE`. Otherwise will append to an existing file.

- add_files:

  Character vector of one or more input filenames to add.

- overwrite:

  Logical scalar. Overwrite the target zip file if it already exists.

- full_paths:

  Logical scalar. By default, the full path will be stored (relative to
  the current directory). `FALSE` to store just the name of a saved file
  (drop the path).

- sozip_enabled:

  String. Whether to generate a SOZip index for the file. One of
  `"AUTO"` (the default), `"YES"` or `"NO"` (see Details).

- sozip_chunk_size:

  The chunk size for a seek-optimized file. Defaults to 32768 bytes. The
  value is specified in bytes, or K and M suffix can be used
  respectively to specify a value in kilo-bytes or mega-bytes. Will be
  coerced to string.

- sozip_min_file_size:

  The minimum file size to decide if a file should be seek-optimized, in
  `sozip_enabled="AUTO"` mode. Defaults to 1 MB byte. The value is
  specified in bytes, or K, M or G suffix can be used respectively to
  specify a value in kilo-bytes, mega-bytes or giga-bytes. Will be
  coerced to string.

- num_threads:

  Number of threads used for SOZip generation. Defaults to `"ALL_CPUS"`
  or specify an integer value (coerced to string).

- content_type:

  String Content-Type value for the file. This is stored as a key-value
  pair in the extra field extension 'KV' (0x564b) dedicated to storing
  key-value pair metadata.

- quiet:

  Logical scalar. `TRUE` for quiet mode, no progress messages emitted.
  Defaults to `FALSE`.

## Value

Logical indicating success (invisible `TRUE`). An error is raised if the
operation fails.

## Details

A Seek-Optimized ZIP file (SOZip) contains one or more compressed files
organized and annotated such that a SOZip-aware reader can perform very
fast random access within the .zip file (see
<https://github.com/sozip/sozip-spec>). Large compressed files can be
accessed directly from SOZip without prior decompression. The .zip file
is otherwise fully backward compatible.

If `sozip_enabled="AUTO"` (the default), a file is seek-optimized only
if its size is above the values of `sozip_min_file_size` (default 1 MB)
and `sozip_chunk_size` (default `32768`). In `"YES"` mode, all input
files will be seek-optimized. In `"NO"` mode, no input files will be
seek-optimized. The default can be changed with the `CPL_SOZIP_ENABLED`
configuration option.

## Note

The `GDAL_NUM_THREADS` configuration option can be set to `ALL_CPUS` or
an integer value to specify the number of threads to use for
SOZip-compressed files (see
[`set_config_option()`](https://usdaforestservice.github.io/gdalraster/reference/set_config_option.md)).

SOZip can be validated with:

    vsi_get_file_metadata(zip_file, domain="ZIP")

where `zip_file` uses the /vsizip/ prefix.

## See also

[`vsi_get_file_metadata()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_file_metadata.md)

## Examples

``` r
lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
zip_file <- file.path(tempdir(), "storml_lcp.zip")

# Requires GDAL >= 3.7
if (gdal_version_num() >= gdal_compute_version(3, 7, 0)) {
  addFilesInZip(zip_file, lcp_file, full_paths = FALSE,
                sozip_enabled = "YES", num_threads = 1)

  print("Files in zip archive:")
  print(unzip(zip_file, list = TRUE))

  # Open with GDAL using Virtual File System handler '/vsizip/'
  # see: https://gdal.org/en/stable/user/virtual_file_systems.html#vsizip-zip-archives
  lcp_in_zip <- file.path("/vsizip", zip_file, "storm_lake.lcp")
  print("SOZip metadata:")
  print(vsi_get_file_metadata(lcp_in_zip, domain = "ZIP"))

  ds <- new(GDALRaster, lcp_in_zip)
  ds$info()
  ds$close()
  DONTSHOW({vsi_unlink(zip_file)})
}
#> adding /home/runner/work/_temp/Library/gdalraster/extdata/storm_lake.lcp ...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#> [1] "Files in zip archive:"
#>             Name Length                Date
#> 1 storm_lake.lcp 252132 2025-11-13 05:12:00
#> [1] "SOZip metadata:"
#> $START_DATA_OFFSET
#> [1] "44"
#> 
#> $COMPRESSION_METHOD
#> [1] "8 (DEFLATE)"
#> 
#> $COMPRESSED_SIZE
#> [1] "78479"
#> 
#> $UNCOMPRESSED_SIZE
#> [1] "252132"
#> 
#> $SOZIP_FOUND
#> [1] "YES"
#> 
#> $SOZIP_VERSION
#> [1] "1"
#> 
#> $SOZIP_OFFSET_SIZE
#> [1] "8"
#> 
#> $SOZIP_CHUNK_SIZE
#> [1] "32768"
#> 
#> $SOZIP_START_DATA_OFFSET
#> [1] "78578"
#> 
#> $SOZIP_VALID
#> [1] "YES"
#> 
#> Driver: LCP/FARSITE v.4 Landscape File (.lcp)
#> Files: /vsizip//tmp/Rtmp6xo7UF/storml_lcp.zip/storm_lake.lcp
#> Size is 143, 107
#> Coordinate System is:
#> 
#> Data axis to CRS axis mapping: 1,2,3
#> Origin = (323476.071970863151364,5105081.983031376264989)
#> Pixel Size = (30.000000000000000,-30.000000000000000)
#> Metadata:
#>   DESCRIPTION=LCP file created by GDAL.
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
```
