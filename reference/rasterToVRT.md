# Create a GDAL virtual raster derived from one source dataset

`rasterToVRT()` creates a virtual raster dataset (VRT format) derived
from one source dataset with options for virtual subsetting, virtually
resampling the source data at a different pixel resolution, or applying
a virtual kernel filter. (See
[`buildVRT()`](https://usdaforestservice.github.io/gdalraster/reference/buildVRT.md)
for virtual mosaicing.)

## Usage

``` r
rasterToVRT(
  srcfile,
  relativeToVRT = FALSE,
  vrtfile = tempfile("tmprast", fileext = ".vrt"),
  resolution = NULL,
  subwindow = NULL,
  src_align = TRUE,
  resampling = "nearest",
  krnl = NULL,
  normalized = TRUE,
  krnl_fn = NULL
)
```

## Arguments

- srcfile:

  Source raster filename.

- relativeToVRT:

  Logical. Indicates whether the source filename should be interpreted
  as relative to the .vrt file (`TRUE`) or not relative to the .vrt file
  (`FALSE`, the default). If `TRUE`, the .vrt file is assumed to be in
  the same directory as `srcfile` and `basename(srcfile)` is used in the
  .vrt file. Use `TRUE` if the .vrt file will always be stored in the
  same directory with `srcfile`.

- vrtfile:

  Output VRT filename.

- resolution:

  A numeric vector of length two (xres, yres). The pixel size must be
  expressed in georeferenced units. Both must be positive values. The
  source pixel size is used if `resolution` is not specified.

- subwindow:

  A numeric vector of length four (xmin, ymin, xmax, ymax). Selects
  `subwindow` of the source raster with corners given in georeferenced
  coordinates (in the source CRS). If not given, the upper left corner
  of the VRT will be the same as source, and the VRT extent will be the
  same or larger than source depending on `resolution`.

- src_align:

  Logical.

  - `TRUE`: the upper left corner of the VRT extent will be set to the
    upper left corner of the source pixel that contains `subwindow`
    xmin, ymax. The VRT will be pixel-aligned with source if the VRT
    `resolution` is the same as the source pixel size, otherwise VRT
    extent will be the minimum rectangle that contains `subwindow` for
    the given pixel size. Often, `src_align=TRUE` when selecting a
    raster minimum bounding box for a vector polygon.

  - `FALSE`: the VRT upper left corner will be exactly `subwindow` xmin,
    ymax, and the VRT extent will be the minimum rectangle that contains
    `subwindow` for the given pixel size. If `subwindow` is not given,
    the source raster extent is used in which case `src_align=FALSE` has
    no effect. Use `src_align=FALSE` to pixel-align two rasters of
    different sizes, i.e., when the intent is target alignment.

- resampling:

  The resampling method to use if xsize, ysize of the VRT is different
  than the size of the underlying source rectangle (in number of
  pixels). The values allowed are nearest, bilinear, cubic, cubicspline,
  lanczos, average and mode (as character).

- krnl:

  A filtering kernel specified as pixel coefficients. `krnl` is a array
  with dimensions (size, size), where size must be an odd number. `krnl`
  can also be given as a vector with length size x size. For example, a
  3x3 average filter is given by:

      krnl <- c(0.11111, 0.11111, 0.11111,
                0.11111, 0.11111, 0.11111,
                0.11111, 0.11111, 0.11111)

  A kernel cannot be applied to sub-sampled or over-sampled data.

- normalized:

  Logical. Indicates whether the kernel is normalized. Defaults to
  `TRUE`.

- krnl_fn:

  Character string specifying a function to compute on the given `krnl`.
  Must be one of `"min"`, `"max"`, `"stddev"`, `"median"` or `"mode"`.
  *Requires GDAL \>= 3.12*. E.g., to compute the median value in a 3x3
  neighborhood around each pixel:

      krnl <- c(1, 1, 1,
                1, 1, 1,
                1, 1, 1)

      krnl_fn <- "median"

## Value

Returns the VRT filename invisibly.

## Details

`rasterToVRT()` can be used to virtually clip and pixel-align various
raster layers with each other or in relation to vector polygon
boundaries. It also supports VRT kernel filtering.

A VRT dataset is saved as a plain-text file with extension .vrt. This
file contains a description of the dataset in an XML format. The
description includes the source raster filename which can be a full path
(`relativeToVRT = FALSE`) or relative path (`relativeToVRT = TRUE`). For
relative path, `rasterToVRT()` assumes that the .vrt file will be in the
same directory as the source file and uses `basename(srcfile)`. The
elements of the XML schema describe how the source data will be read,
along with algorithms potentially applied and so forth. Documentation of
the XML format for .vrt is at:
<https://gdal.org/en/stable/drivers/raster/vrt.html>.

Since .vrt is a small plain-text file it is fast to write and requires
little storage space. Read performance is not degraded for certain
simple operations (e.g., virtual clip without resampling). Reading will
be slower for virtual resampling to a different pixel resolution or
virtual kernel filtering since the operations are performed on-the-fly
(but .vrt does not require the up front writing of a resampled or
kernel-filtered raster to a regular format). VRT is sometimes useful as
an intermediate raster in a series of processing steps, e.g., as a
`tempfile` (the default).

GDAL VRT format has several capabilities and uses beyond those covered
by `rasterToVRT()`. See the URL above for a full discussion.

## Note

Pixel alignment is specified in terms of the source raster pixels (i.e.,
`srcfile` of the virtual raster). The use case in mind is virtually
clipping a raster to the bounding box of a vector polygon and keeping
pixels aligned with `srcfile` (`src_align = TRUE`). `src_align` would be
set to `FALSE` if the intent is "target alignment". For example, if
`subwindow` is the bounding box of another raster with a different
layout, then also setting `resolution` to the pixel resolution of the
target raster and `src_align = FALSE` will result in a virtual raster
pixel-aligned with the target (i.e., pixels in the virtual raster are no
longer aligned with its `srcfile`). Resampling defaults to `nearest` if
not specified. Examples for both cases of `src_align` are given below.

`rasterToVRT()` assumes `srcfile` is a north-up raster.

## See also

[`GDALRaster-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.md),
[`bbox_from_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_from_wkt.md),
[`buildVRT()`](https://usdaforestservice.github.io/gdalraster/reference/buildVRT.md)

[`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md)
can write VRT for virtual reprojection

## Examples

``` r
## resample

evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
ds <- new(GDALRaster, evt_file)
ds$res()
#> [1] 30 30
ds$bbox()
#> [1]  323476.1 5101872.0  327766.1 5105082.0
ds$close()

# table of the unique pixel values and their counts
tbl <- buildRAT(evt_file)
#> scanning raster...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
print(tbl)
#>    VALUE COUNT
#> 1   7011    28
#> 2   7046  4564
#> 3   7050   570
#> 4   7055   889
#> 5   7056   304
#> 6   7057    11
#> 7   7070   267
#> 8   7106     3
#> 9   7125     1
#> 10  7126  1082
#> 11  7140   679
#> 12  7143   199
#> 13  7144   765
#> 14  7145   681
#> 15  7166    32
#> 16  7169    60
#> 17  7292   397
#> 18  7901     2
#> 19  9016  2486
#> 20  9017    13
#> 21  9018  1280
#> 22  9021    14
#> 23  9022    98
#> 24    NA   876
sum(tbl$COUNT)
#> [1] 15301

# resample at 90-m resolution
# EVT is thematic vegetation type so use a majority value
vrt_file <- rasterToVRT(evt_file,
                        resolution = c(90, 90),
                        resampling = "mode")

# .vrt is a small xml file pointing to the source raster
file.size(vrt_file)
#> [1] 1652

tbl90m <- buildRAT(vrt_file)
#> scanning raster...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
print(tbl90m)
#>    VALUE COUNT
#> 1   7011     1
#> 2   7046   614
#> 3   7050    59
#> 4   7055    75
#> 5   7056    27
#> 6   7057     2
#> 7   7070    27
#> 8   7126   119
#> 9   7140    59
#> 10  7143    13
#> 11  7144    75
#> 12  7145    64
#> 13  7166     2
#> 14  7169     4
#> 15  7292    48
#> 16  9016   322
#> 17  9017     1
#> 18  9018   152
#> 19  9021     1
#> 20  9022     9
#> 21    NA    54
sum(tbl90m$COUNT)
#> [1] 1728

ds <- new(GDALRaster, vrt_file)
ds$res()
#> [1] 90 90
ds$bbox()
#> [1]  323476.1 5101842.0  327796.1 5105082.0
ds$close()

## clip

evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
ds_evt <- new(GDALRaster, evt_file)
ds_evt$bbox()
#> [1]  323476.1 5101872.0  327766.1 5105082.0

# WKT string for a boundary within the EVT extent
bnd = "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"

# src_align = TRUE
vrt_file <- rasterToVRT(evt_file,
                        subwindow = bbox_from_wkt(bnd),
                        src_align = TRUE)
ds_vrt <- new(GDALRaster, vrt_file)

# VRT is a virtual clip, pixel-aligned with the EVT raster
bbox_from_wkt(bnd)
#> [1]  323794.2 5102885.8  326420.0 5104929.4
ds_vrt$bbox()
#> [1]  323776.1 5102862.0  326446.1 5104932.0
ds_vrt$res()
#> [1] 30 30
ds_vrt$close()

# src_align = FALSE
vrt_file <- rasterToVRT(evt_file,
                        subwindow = bbox_from_wkt(bnd),
                        src_align = FALSE)
ds_vrt_noalign <- new(GDALRaster, vrt_file)

# VRT upper left corner (xmin, ymax) is exactly bnd xmin, ymax
ds_vrt_noalign$bbox()
#> [1]  323794.2 5102859.4  326434.2 5104929.4
ds_vrt_noalign$res()
#> [1] 30 30

ds_vrt_noalign$close()
ds_evt$close()


## subset and pixel align two rasters

# FARSITE landscape file for the Storm Lake area
lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
ds_lcp <- new(GDALRaster, lcp_file)

# Landsat band 5 file covering the Storm Lake area
b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
ds_b5 <- new(GDALRaster, b5_file)

ds_lcp$bbox()  # 323476.1 5101872.0  327766.1 5105082.0
#> [1]  323476.1 5101872.0  327766.1 5105082.0
ds_lcp$res()   # 30 30
#> [1] 30 30

ds_b5$bbox()   # 323400.9 5101815.8  327870.9 5105175.8
#> [1]  323400.9 5101815.8  327870.9 5105175.8
ds_b5$res()    # 30 30
#> [1] 30 30

# src_align = FALSE because we need target alignment in this case:
vrt_file <- rasterToVRT(b5_file,
                        resolution = ds_lcp$res(),
                        subwindow = ds_lcp$bbox(),
                        src_align = FALSE)
ds_b5vrt <- new(GDALRaster, vrt_file)

ds_b5vrt$bbox() # 323476.1 5101872.0  327766.1 5105082.0
#> [1]  323476.1 5101872.0  327766.1 5105082.0
ds_b5vrt$res()  # 30 30
#> [1] 30 30

# read the the Landsat file pixel-aligned with the LCP file
# summarize band 5 reflectance where FBFM = 165
# LCP band 4 contains FBFM (a classification of fuel beds):
ds_lcp$getMetadata(band = 4, domain = "")
#> [1] "FUEL_MODEL_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif"
#> [2] "FUEL_MODEL_MAX=183"                                                                                                                        
#> [3] "FUEL_MODEL_MIN=-9999"                                                                                                                      
#> [4] "FUEL_MODEL_NUM_CLASSES=12"                                                                                                                 
#> [5] "FUEL_MODEL_OPTION=0"                                                                                                                       
#> [6] "FUEL_MODEL_OPTION_DESC=no custom models AND no conversion file needed"                                                                     
#> [7] "FUEL_MODEL_VALUES=0,98,99,101,102,121,122,123,142,162,165,181,183"                                                                         

# verify Landsat nodata (0):
ds_b5vrt$getNoDataValue(band=1)
#> [1] 0
# will be read as NA and omitted from stats
rs <- new(RunningStats, na_rm = TRUE)

ncols <- ds_lcp$getRasterXSize()
nrows <- ds_lcp$getRasterYSize()
for (row in 0:(nrows-1)) {
    row_fbfm <- ds_lcp$read(band = 4, xoff = 0, yoff = row,
                            xsize = ncols, ysize = 1,
                            out_xsize = ncols, out_ysize = 1)
    row_b5 <- ds_b5vrt$read(band = 1, xoff = 0, yoff = row,
                            xsize = ncols, ysize = 1,
                            out_xsize = ncols, out_ysize = 1)
     rs$update(row_b5[row_fbfm == 165])
}
rs$get_count()
#> [1] 2498
rs$get_mean()
#> [1] 12992.35
rs$get_min()
#> [1] 7635
rs$get_max()
#> [1] 17866
rs$get_sum()
#> [1] 32454886
rs$get_var()
#> [1] 1214912
rs$get_sd()
#> [1] 1102.23

ds_b5vrt$close()
ds_lcp$close()
ds_b5$close()
```
