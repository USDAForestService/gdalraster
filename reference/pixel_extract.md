# Extract pixel values at geospatial point locations

`pixel_extract()` returns raster pixel values for a set of geospatial
point locations. The coordinates are given as a two-column matrix of x/y
values in the same spatial reference system as the input raster (unless
`xy_srs` is specified). Coordinates can also be given in a data frame
with an optional column of point IDs. Values are extracted from all
bands of the raster by default, or specific band numbers may be given.
An optional interpolation method may be specified for bilinear (2 x 2
kernel), cubic convolution (4 x 4 kernel, GDAL \>= 3.10), or cubic
spline (4 x 4 kernel, GDAL \>= 3.10). Alternatively, an optional kernel
dimension `N` may be given to extract values of the individual pixels
within an `N x N` kernel centered on the pixel containing the point
location. If `xy_srs` is given, the function will attempt to transform
the input points to the projection of the raster with a call to
[`transform_xy()`](https://usdaforestservice.github.io/gdalraster/reference/transform_xy.md).

## Usage

``` r
pixel_extract(
  raster,
  xy,
  bands = NULL,
  interp = NULL,
  krnl_dim = NULL,
  xy_srs = NULL,
  max_ram = 300,
  as_data_frame = NULL
)
```

## Arguments

- raster:

  Either a character string giving the filename of a raster, or an
  object of class `GDALRaster` for the source dataset.

- xy:

  A two-column numeric matrix or two-column data frame of geospatial
  coordinates (x, y), or a vector for a single point (x, y), in the same
  spatial reference system as `raster`. Can also be given as a data
  frame with three or more columns, in which the first column must be a
  vector of point IDs (`character` or `numeric`), and the second and
  third columns must contain the geospatial coordinates (x, y).

- bands:

  Optional numeric vector of band numbers. All bands in `raster` will be
  processed by default if not specified, or if `0` is given.

- interp:

  Optional character string specifying an interpolation method. Must be
  one of `"bilinear"`, `"cubic"`, `"cubicspline"`, or `"nearest"` (the
  default if not specified, i.e., no interpolation). GDAL \>= 3.10 is
  required for `"cubic"` and `"cubicspline"`.

- krnl_dim:

  Optional numeric value specifying the dimension `N` pixels of an
  `N x N` kernel for which all individual pixel values will be returned.
  Should be a positive whole number (will be coerced to integer by
  truncation). Currently only supported when extracting from a single
  raster band. Ignored if `interp` is specified as other than
  `"nearest"` (i.e., the kernel implied by the interpolation method will
  always be used).

- xy_srs:

  Optional character string specifying the spatial reference system for
  `xy`. May be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md).

- max_ram:

  Numeric value giving the maximum amount of RAM (in MB) to use for
  potentially copying a remote raster into memory for processing (see
  Note). Defaults to 300 MB. Set to zero to disable potential copy of
  remote files into memory.

- as_data_frame:

  Logical value, `TRUE` to return output as a data frame. The default is
  to return a numeric matrix unless point IDs are present in the first
  column of `xy` given as a data frame. In that latter case, the output
  will always be a data frame with the point IDs in the first column.

## Value

A numeric matrix or data frame of pixel values with number of rows equal
to the number of rows in `xy`. The number of columns is equal to the
number of `bands` (plus optional point ID column), or if `krnl_dim = N`
is used, number of columns is equal to `N * N` (plus optional point ID
column). Output is always a data frame if `xy` is given as a data frame
with a column of point IDs (`as_data_frame` will be ignored in that
case). Named columns indicate the band, e.g., `"b1"`. If `krnl_dim` is
used, named columns indicate band and pixel, e.g., `"b1_p1"`, `"b1_p2"`,
..., `"b1_p9"` if `krnl_dim = 3`. Pixels are in left-to-right,
top-to-bottom order in the kernel.

## Note

Depending on the number of input points, extracting from a raster on a
remote filesystem may require a large number of HTTP range requests
which may be slow (i.e., URLs/remote VSI filesystems). In that case, it
may be faster to copy the raster into memory first (either as MEM format
or to a /vsimem/ filesystem). `pixel_extract()` will attempt to automate
that process if the total size of file(s) that would be copied does not
exceed the threshold given by `max_ram`, and `nrow(xy) > 10` (requires
GDAL \>= 3.6).

For alternative workflows that involve copying to local storage, the
data management functions (e.g.,
[`copyDatasetFiles()`](https://usdaforestservice.github.io/gdalraster/reference/copyDatasetFiles.md))
and the VSI filesystem functions (e.g.,
[`vsi_is_local()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_is_local.md),
[`vsi_stat()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md),
[`vsi_copy_file()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_copy_file.md))
may be of interest.

## Examples

``` r
pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
# id, x, y in NAD83 / UTM zone 12N, same as the raster
pts <- read.csv(pt_file)
print(pts)
#>    id   xcoord  ycoord
#> 1   1 324650.9 5103344
#> 2   2 324171.0 5103034
#> 3   3 323533.4 5103329
#> 4   4 325220.0 5103508
#> 5   5 325703.1 5102377
#> 6   6 326297.8 5103924
#> 7   7 325520.4 5104146
#> 8   8 326247.7 5102506
#> 9   9 327711.7 5104476
#> 10 10 324181.7 5103901

raster_file <- system.file("extdata/storml_elev.tif", package="gdalraster")

pixel_extract(raster_file, pts)
#> extracting from band 1...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#>    id storml_elev
#> 1   1        2648
#> 2   2        2876
#> 3   3        2724
#> 4   4        2561
#> 5   5        2913
#> 6   6        2633
#> 7   7        2548
#> 8   8        2801
#> 9   9        2473
#> 10 10        2812

# or as GDALRaster object
ds <- new(GDALRaster, raster_file)
pixel_extract(ds, pts)
#> extracting from band 1...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#>    id storml_elev
#> 1   1        2648
#> 2   2        2876
#> 3   3        2724
#> 4   4        2561
#> 5   5        2913
#> 6   6        2633
#> 7   7        2548
#> 8   8        2801
#> 9   9        2473
#> 10 10        2812

# interpolated values
pixel_extract(raster_file, pts, interp = "bilinear")
#> extracting from band 1...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#>    id storml_elev
#> 1   1    2648.589
#> 2   2    2884.232
#> 3   3    2718.675
#> 4   4    2559.443
#> 5   5    2917.230
#> 6   6    2629.937
#> 7   7    2548.441
#> 8   8    2810.815
#> 9   9    2476.014
#> 10 10    2812.528

# individual pixel values within a kernel
pixel_extract(raster_file, pts, krnl_dim = 3)
#> extracting from band 1...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#>    id b1_p1 b1_p2 b1_p3 b1_p4 b1_p5 b1_p6 b1_p7 b1_p8 b1_p9
#> 1   1  2661  2654  2649  2654  2648  2646  2650  2645  2644
#> 2   2  2909  2907  2908  2878  2876  2876  2850  2847  2846
#> 3   3  2729  2713  2701  2742  2724  2713  2754  2735  2725
#> 4   4  2555  2559  2564  2557  2561  2565  2559  2562  2566
#> 5   5  2928  2916  2902  2925  2913  2900  2923  2910  2897
#> 6   6  2634  2642  2650  2628  2633  2639  2620  2624  2628
#> 7   7  2548  2549  2551  2549  2548  2551  2550  2548  2550
#> 8   8  2782  2777  2779  2804  2801  2805  2829  2827  2829
#> 9   9  2485  2478  2470  2477  2473  2468  2474  2470  2467
#> 10 10  2842  2819  2795  2834  2812  2788  2829  2804  2780

# lont/lat xy
pts_wgs84 <- transform_xy(pts[-1], srs_from = ds$getProjection(),
                          srs_to = "WGS84")

# transform the input xy
pixel_extract(ds, xy = pts_wgs84, xy_srs = "WGS84")
#> extracting from band 1...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#>       storml_elev
#>  [1,]        2648
#>  [2,]        2876
#>  [3,]        2724
#>  [4,]        2561
#>  [5,]        2913
#>  [6,]        2633
#>  [7,]        2548
#>  [8,]        2801
#>  [9,]        2473
#> [10,]        2812

ds$close()
```
