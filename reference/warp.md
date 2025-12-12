# Raster reprojection and mosaicing

`warp()` is a wrapper of the `gdalwarp` command-line utility for raster
reprojection and warping (see
<https://gdal.org/en/stable/programs/gdalwarp.html>). The function can
reproject to any supported spatial reference system (SRS). It can also
be used to crop, mosaic, resample, and optionally write output to a
different raster format. See Details for a list of commonly used
processing options that can be passed as arguments to `warp()`.

## Usage

``` r
warp(src_files, dst_filename, t_srs, cl_arg = NULL, quiet = FALSE)
```

## Arguments

- src_files:

  Either a character vector of source filenames(s) to be reprojected, or
  a `GDALRaster` object or list of `GDALRaster` objects for the source
  data.

- dst_filename:

  Either a character string giving the filename of the output dataset,
  or an object of class `GDALRaster` for the output.

- t_srs:

  Character string. Target spatial reference system. Usually an EPSG
  code ("EPSG:#####") or a well known text (WKT) SRS definition. Can be
  set to empty string `""` and the spatial reference of `src_files[1]`
  will be used unless the destination raster already exists (see Note).

- cl_arg:

  Optional character vector of command-line arguments to `gdalwarp` in
  addition to `-t_srs` (see Details).

- quiet:

  Logical scalar. If `TRUE`, a progress bar will not be displayed.
  Defaults to `FALSE`.

## Value

Logical indicating success (invisible `TRUE`). An error is raised if the
operation fails.

## Details

Several processing options can be performed in one call to `warp()` by
passing the necessary command-line arguments. The following list
describes several commonly used arguments. Note that `gdalwarp` supports
a large number of arguments that enable a variety of different
processing options. Users are encouraged to review the original source
documentation provided by the GDAL project at the URL above for the full
list.

- `-te <xmin> <ymin> <xmax> <ymax>`  
  Georeferenced extents of output file to be created (in target SRS by
  default).

- `-te_srs <srs_def>`  
  SRS in which to interpret the coordinates given with `-te` (if
  different than `t_srs`).

- `-tr <xres> <yres>`  
  Output pixel resolution (in target georeferenced units).

- `-tap`  
  (target aligned pixels) align the coordinates of the extent of the
  output file to the values of the `-tr`, such that the aligned extent
  includes the minimum extent. Alignment means that xmin / resx, ymin /
  resy, xmax / resx and ymax / resy are integer values.

- `-ovr <level>|AUTO|AUTO-<n>|NONE`  
  Specify which overview level of source files must be used. The default
  choice, `AUTO`, will select the overview level whose resolution is the
  closest to the target resolution. Specify an integer value (0-based,
  i.e., 0=1st overview level) to select a particular level. Specify
  `AUTO-n` where `n` is an integer greater or equal to `1`, to select an
  overview level below the `AUTO` one. Or specify `NONE` to force the
  base resolution to be used (can be useful if overviews have been
  generated with a low quality resampling method, and the warping is
  done using a higher quality resampling method).

- `-wo <NAME>=<VALUE>`  
  Set a warp option as described in the GDAL documentation for
  [`GDALWarpOptions`](https://gdal.org/en/stable/api/gdalwarp_cpp.html#_CPPv415GDALWarpOptions)
  Multiple `-wo` may be given. See also `-multi` below.

- `-ot <type>`  
  Force the output raster bands to have a specific data type supported
  by the format, which may be one of the following: `Byte`, `Int8`,
  `UInt16`, `Int16`, `UInt32`, `Int32`, `UInt64`, `Int64`, `Float32`,
  `Float64`, `CInt16`, `CInt32`, `CFloat32` or `CFloat64`.

- `-r <resampling_method>`  
  Resampling method to use. Available methods are: `near` (nearest
  neighbour, the default), `bilinear`, `cubic`, `cubicspline`,
  `lanczos`, `average`, `rms` (root mean square, GDAL \>= 3.3), `mode`,
  `max`, `min`, `med`, `q1` (first quartile), `q3` (third quartile),
  `sum` (GDAL \>= 3.1).

- `-srcnodata "<value>[ <value>]..."`  
  Set nodata masking values for input bands (different values can be
  supplied for each band). If more than one value is supplied all values
  should be quoted to keep them together as a single operating system
  argument. Masked values will not be used in interpolation. Use a value
  of `None` to ignore intrinsic nodata settings on the source dataset.
  If `-srcnodata` is not explicitly set, but the source dataset has
  nodata values, they will be taken into account by default.

- `-dstnodata "<value>[ <value>]..."`  
  Set nodata values for output bands (different values can be supplied
  for each band). If more than one value is supplied all values should
  be quoted to keep them together as a single operating system argument.
  New files will be initialized to this value and if possible the nodata
  value will be recorded in the output file. Use a value of `"None"` to
  ensure that nodata is not defined. If this argument is not used then
  nodata values will be copied from the source dataset.

- `-srcband <n>`  
  (GDAL \>= 3.7) Specify an input band number to warp (between 1 and the
  number of bands of the source dataset). This option is used to warp a
  subset of the input bands. All input bands are used when it is not
  specified. This option may be repeated multiple times to select
  several input bands. The order in which bands are specified will be
  the order in which they appear in the output dataset (unless
  `-dstband` is specified). The alpha band should not be specified in
  the list, as it will be automatically retrieved (unless `-nosrcalpha`
  is specified).

- `-dstband <n>`  
  (GDAL \>= 3.7) Specify the output band number in which to warp. In
  practice, this option is only useful when updating an existing
  dataset, e.g to warp one band at at time. If `-srcband` is specified,
  there must be as many occurrences of `-dstband` as there are of
  `-srcband`.  
  If `-dstband` is not specified, then:  
  `c("-dstband", "1", "-dstband", "2", ... "-dstband", "N")`  
  is assumed where N is the number of input bands (implicitly, or
  specified explicitly with `-srcband`). The alpha band should not be
  specified in the list, as it will be automatically retrieved (unless
  `-nosrcalpha` is specified).

- `-wm <memory_in_mb>`  
  Set the amount of memory that the warp API is allowed to use for
  caching. The value is interpreted as being in megabytes if the value
  is \<10000. For values \>=10000, this is interpreted as bytes. The
  warper will total up the memory required to hold the input and output
  image arrays and any auxiliary masking arrays and if they are larger
  than the "warp memory" allowed it will subdivide the chunk into
  smaller chunks and try again. If the `-wm` value is very small there
  is some extra overhead in doing many small chunks so setting it larger
  is better but it is a matter of diminishing returns.

- `-multi`  
  Use multithreaded warping implementation. Two threads will be used to
  process chunks of image and perform input/output operation
  simultaneously. Note that computation is not multithreaded itself. To
  do that, you can use the `-wo NUM_THREADS=val/ALL_CPUS` option, which
  can be combined with `-multi`.

- `-of <format>` Set the output raster format. Will be guessed from the
  extension if not specified. Use the short format name (e.g.,
  `"GTiff"`).

- `-co <NAME>=<VALUE>`  
  Set one or more format specific creation options for the output
  dataset. For example, the GeoTIFF driver supports creation options to
  control compression, and whether the file should be tiled.
  [`getCreationOptions()`](https://firelab.github.io/gdalraster/reference/getCreationOptions.md)
  can be used to look up available creation options, but the GDAL
  [Raster drivers](https://gdal.org/en/stable/drivers/raster/index.html)
  documentation is the definitive reference for format specific options.
  Multiple `-co` may be given, e.g.,

       c("-co", "COMPRESS=LZW", "-co", "BIGTIFF=YES") 

- `-overwrite`  
  Overwrite the target dataset if it already exists. Overwriting means
  deleting and recreating the file from scratch. Note that if this
  option is not specified and the output file already exists, it will be
  updated in place.

The documentation for
[`gdalwarp`](https://gdal.org/en/stable/programs/gdalwarp.html)
describes additional command-line options related to spatial reference
systems, alpha bands, masking with polygon cutlines including blending,
and more.

Mosaicing into an existing output file is supported if the output file
already exists. The spatial extent of the existing file will not be
modified to accommodate new data, so you may have to remove it in that
case, or use the `-overwrite` option.

Command-line options are passed to `warp()` as a character vector. The
elements of the vector are the individual options followed by their
individual values, e.g.,

    cl_arg = c("-tr", "30", "30", "-r", "bilinear"))

to set the target pixel resolution to 30 x 30 in target georeferenced
units and use bilinear resampling.

## Note

`warp()` can be used to reproject and also perform other processing such
as crop, resample, and mosaic. This processing is generally done with a
single function call by passing arguments for the output ("target")
pixel resolution, extent, resampling method, nodata value, format, and
so forth.

If `warp()` is called with `t_srs = ""` and the output raster does not
already exist, the target spatial reference will be set to that of
`src_files[1]`. In that case, the processing options given in `cl_arg`
will be performed without reprojecting (if there is one source raster or
multiple sources that all use the same spatial reference system,
otherwise would reproject inputs to the SRS of `src_files[1]` where they
are different). If `t_srs = ""` and the destination raster already
exists, the output SRS will be the projection of the destination
dataset.

## See also

[`GDALRaster-class`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md),
[`translate()`](https://firelab.github.io/gdalraster/reference/translate.md)

## Examples

``` r
# reproject the elevation raster to NAD83 / CONUS Albers (EPSG:5070)
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")

# command-line arguments for gdalwarp
# resample to 90-m resolution and keep pixels aligned:
args <- c("-tr", "90", "90", "-r", "cubic", "-tap")
# write to Erdas Imagine format (HFA) with compression:
args <- c(args, "-of", "HFA", "-co", "COMPRESSED=YES")

alb83_file <- file.path(tempdir(), "storml_elev_alb83.img")
warp(elev_file, alb83_file, t_srs = "EPSG:5070", cl_arg = args)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.

ds <- new(GDALRaster, alb83_file)
ds$getDriverLongName()
#> [1] "Erdas Imagine Images (.img)"
ds$getProjectionRef()
#> [1] "PROJCS[\"NAD83 / Conus Albers\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.0174532925199433]],PROJECTION[\"Albers_Conic_Equal_Area\"],PARAMETER[\"latitude_of_center\",23],PARAMETER[\"longitude_of_center\",-96],PARAMETER[\"standard_parallel_1\",29.5],PARAMETER[\"standard_parallel_2\",45.5],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH]]"
ds$res()
#> [1] 90 90
ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#> [1] 2438.0000 3043.0000 2673.9259  133.3519
ds$close()
```
