# Raster overlay for unique combinations

`combine()` overlays multiple rasters so that a unique ID is assigned to
each unique combination of input values. The input raster layers
typically have integer data types (floating point will be coerced to
integer by truncation), and must have the same projection, extent and
cell size. Pixel counts for each unique combination are obtained, and
combination IDs are optionally written to an output raster.

## Usage

``` r
combine(
  rasterfiles,
  var.names = NULL,
  bands = NULL,
  dstfile = NULL,
  fmt = NULL,
  dtName = "UInt32",
  options = NULL,
  quiet = FALSE
)
```

## Arguments

- rasterfiles:

  Character vector of raster filenames to combine.

- var.names:

  Character vector of `length(rasterfiles)` containing variable names
  for each raster layer. Defaults will be assigned if `var.names` are
  omitted.

- bands:

  Numeric vector of `length(rasterfiles)` containing the band number to
  use for each raster in `rasterfiles`. Band 1 will be used for each
  input raster if `bands` are not specified.

- dstfile:

  Character. Optional output raster filename for writing the per-pixel
  combination IDs. The output raster will be created (and overwritten if
  it already exists).

- fmt:

  Character. Output raster format name (e.g., "GTiff" or "HFA").

- dtName:

  Character. Output raster data type name. Combination IDs are
  sequential integers starting at 1. The data type for the output raster
  should be large enough to accommodate the potential number of unique
  combinations of the input values (e.g., "UInt16" or the default
  "UInt32").

- options:

  Optional list of format-specific creation options in a vector of
  "NAME=VALUE" pairs (e.g., `options = c("COMPRESS=LZW")` to set LZW
  compression during creation of a GTiff file).

- quiet:

  Logical scalar. If `TRUE`, progress bar and messages will be
  suppressed. Defaults to `FALSE`.

## Value

A data frame with column `cmbid` containing the combination IDs, column
`count` containing the pixel counts for each combination, and
`length(rasterfiles)` columns named `var.names` containing the integer
values comprising each unique combination.

## Details

To specify input raster layers that are bands of a multi-band raster
file, repeat the filename in `rasterfiles` and provide the corresponding
band numbers in `bands`. For example:

    rasterfiles <- c("multi-band.tif", "multi-band.tif", "other.tif")
    bands <- c(4, 5, 1)
    var.names <- c("multi_b4", "multi_b5", "other")

[`rasterToVRT()`](https://usdaforestservice.github.io/gdalraster/reference/rasterToVRT.md)
provides options for virtual clipping, resampling and pixel alignment,
which may be helpful here if the input rasters are not already aligned
on a common extent and cell size.

If an output raster of combination IDs is written, the user should
verify that the number of combinations obtained did not exceed the range
of the output data type. Combination IDs are sequential integers
starting at 1. Typical output data types are the unsigned types: Byte (0
to 255), UInt16 (0 to 65,535) and UInt32 (the default, 0 to
4,294,967,295).

## See also

[`CmbTable-class`](https://usdaforestservice.github.io/gdalraster/reference/CmbTable-class.md),
[`GDALRaster-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.md),
[`calc()`](https://usdaforestservice.github.io/gdalraster/reference/calc.md),
[`rasterToVRT()`](https://usdaforestservice.github.io/gdalraster/reference/rasterToVRT.md)

[`buildRAT()`](https://usdaforestservice.github.io/gdalraster/reference/buildRAT.md)
to compute a table of the unique pixel values and their counts for a
single raster layer

## Examples

``` r
evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")
evh_file <- system.file("extdata/storml_evh.tif", package="gdalraster")
rasterfiles <- c(evt_file, evc_file, evh_file)
var.names <- c("veg_type", "veg_cov", "veg_ht")
tbl <- combine(rasterfiles, var.names)
#> combining 3 rasters...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
nrow(tbl)
#> [1] 2083
tbl <- tbl[order(-tbl$count),]
head(tbl, n = 20)
#>      cmbid count veg_type veg_cov veg_ht
#> 1345   115  2277     9016     100    100
#> 1131    19  1213     9018     100    100
#> 1410     1   876       NA      NA     NA
#> 873    429   397     7292      11     11
#> 1745   116   209     9016      31     31
#> 1181     7   142     7046     159    115
#> 1108     8   112     7046     159    116
#> 1538    26    99     7046     159    114
#> 1724    75    78     7126     243    210
#> 1132   844    67     9018      31     31
#> 779    186    54     7046     110    108
#> 823    344    53     7126     243    209
#> 1213   146    47     7046     159    117
#> 808     42    44     7050     162    116
#> 645     15    39     7046     149    114
#> 923    821    37     7046     110    107
#> 1594   603    37     7126     243    211
#> 1273     2    36     7046     110    106
#> 1305   108    36     7126     243    208
#> 1447    68    36     7046     159    113

# combine two bands from a multi-band file and write the combination IDs
# to an output raster
lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
rasterfiles <- c(lcp_file, lcp_file)
bands <- c(4, 5)
var.names <- c("fbfm", "tree_cov")
cmb_file <- file.path(tempdir(), "fbfm_cov_cmbid.tif")
opt <- c("COMPRESS=LZW")
tbl <- combine(rasterfiles, var.names, bands, cmb_file, options = opt)
#> combining 2 rasters...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
head(tbl)
#>   cmbid count fbfm tree_cov
#> 1    26    98  122       25
#> 2    24   397   98        0
#> 3    23    59  162       55
#> 4    25    14  183        0
#> 5    22    65  183       35
#> 6    19    34  162       75
ds <- new(GDALRaster, cmb_file)
ds$info()
#> Driver: GTiff/GeoTIFF
#> Files: /tmp/RtmpzbvXWj/fbfm_cov_cmbid.tif
#> Size is 143, 107
#> Coordinate System is:
#> PROJCRS["NAD83 / UTM zone 12N",
#>     BASEGEOGCRS["NAD83",
#>         DATUM["North American Datum 1983",
#>             ELLIPSOID["GRS 1980",6378137,298.257222101,
#>                 LENGTHUNIT["metre",1]]],
#>         PRIMEM["Greenwich",0,
#>             ANGLEUNIT["degree",0.0174532925199433]],
#>         ID["EPSG",4269]],
#>     CONVERSION["UTM zone 12N",
#>         METHOD["Transverse Mercator",
#>             ID["EPSG",9807]],
#>         PARAMETER["Latitude of natural origin",0,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8801]],
#>         PARAMETER["Longitude of natural origin",-111,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8802]],
#>         PARAMETER["Scale factor at natural origin",0.9996,
#>             SCALEUNIT["unity",1],
#>             ID["EPSG",8805]],
#>         PARAMETER["False easting",500000,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8806]],
#>         PARAMETER["False northing",0,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8807]]],
#>     CS[Cartesian,2],
#>         AXIS["(E)",east,
#>             ORDER[1],
#>             LENGTHUNIT["metre",1]],
#>         AXIS["(N)",north,
#>             ORDER[2],
#>             LENGTHUNIT["metre",1]],
#>     USAGE[
#>         SCOPE["Engineering survey, topographic mapping."],
#>         AREA["North America - between 114°W and 108°W - onshore and offshore. Canada - Alberta; Northwest Territories; Nunavut; Saskatchewan. United States (USA) - Arizona; Colorado; Idaho; Montana; New Mexico; Utah; Wyoming."],
#>         BBOX[31.33,-114,84,-108]],
#>     ID["EPSG",26912]]
#> Data axis to CRS axis mapping: 1,2
#> Origin = (323476.071970863151364,5105081.983031376264989)
#> Pixel Size = (30.000000000000000,-30.000000000000000)
#> Metadata:
#>   AREA_OR_POINT=Area
#> Image Structure Metadata:
#>   COMPRESSION=LZW
#>   INTERLEAVE=BAND
#> Corner Coordinates:
#> Upper Left  (  323476.072, 5105081.983) (113d16'58.40"W, 46d 4'35.44"N)
#> Lower Left  (  323476.072, 5101871.983) (113d16'54.12"W, 46d 2'51.51"N)
#> Upper Right (  327766.072, 5105081.983) (113d13'38.83"W, 46d 4'39.38"N)
#> Lower Right (  327766.072, 5101871.983) (113d13'34.65"W, 46d 2'55.45"N)
#> Center      (  325621.072, 5103476.983) (113d15'16.50"W, 46d 3'45.46"N)
#> Band 1 Block=143x14 Type=UInt32, ColorInterp=Gray
ds$close()
```
