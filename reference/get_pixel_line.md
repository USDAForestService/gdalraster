# Raster pixel/line from geospatial x,y coordinates

`get_pixel_line()` converts geospatial coordinates to pixel/line (raster
column, row numbers). The upper left corner pixel is the raster origin
(0,0) with column, row increasing left to right, top to bottom.

## Usage

``` r
get_pixel_line(xy, gt)
```

## Arguments

- xy:

  Numeric matrix of geospatial x, y coordinates in the same spatial
  reference system as `gt` (or two-column data frame that will be
  coerced to numeric matrix, or a vector x, y for one coordinate).

- gt:

  Either a numeric vector of length six containing the affine
  geotransform for the raster, or an object of class `GDALRaster` from
  which the geotransform will be obtained (see Note).

## Value

Integer matrix of raster pixel/line.

## Note

This function applies the inverse geotransform to the input points. If
`gt` is given as the numeric vector, no bounds checking is done (i.e.,
min pixel/line could be less than zero and max pixel/line could be
greater than the raster x/y size). If `gt` is obtained from an object of
class `GDALRaster`, then `NA` is returned for points that fall outside
the raster extent and a warning emitted giving the number points that
were outside. This latter case is equivalent to calling the
`$get_pixel_line()` class method on the `GDALRaster` object (see
Examples).

## See also

[`GDALRaster$getGeoTransform()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`inv_geotransform()`](https://firelab.github.io/gdalraster/reference/inv_geotransform.md)

## Examples

``` r
pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
# id, x, y in NAD83 / UTM zone 12N
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

raster_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
ds <- new(GDALRaster, raster_file)
gt <- ds$getGeoTransform()
get_pixel_line(pts[, -1], gt)
#>       [,1] [,2]
#>  [1,]   39   57
#>  [2,]   23   68
#>  [3,]    1   58
#>  [4,]   58   52
#>  [5,]   74   90
#>  [6,]   94   38
#>  [7,]   68   31
#>  [8,]   92   85
#>  [9,]  141   20
#> [10,]   23   39

# or, using the class method
ds$get_pixel_line(pts[, -1])
#>       [,1] [,2]
#>  [1,]   39   57
#>  [2,]   23   68
#>  [3,]    1   58
#>  [4,]   58   52
#>  [5,]   74   90
#>  [6,]   94   38
#>  [7,]   68   31
#>  [8,]   92   85
#>  [9,]  141   20
#> [10,]   23   39

# add a point outside the raster extent
pts[11, ] <- c(11, 323318, 5105104)
get_pixel_line(pts[, -1], gt)
#>       [,1] [,2]
#>  [1,]   39   57
#>  [2,]   23   68
#>  [3,]    1   58
#>  [4,]   58   52
#>  [5,]   74   90
#>  [6,]   94   38
#>  [7,]   68   31
#>  [8,]   92   85
#>  [9,]  141   20
#> [10,]   23   39
#> [11,]   -6   -1

# with bounds checking on the raster extent
ds$get_pixel_line(pts[, -1])
#> Warning: 1 point(s) were outside the raster extent, NA returned
#>       [,1] [,2]
#>  [1,]   39   57
#>  [2,]   23   68
#>  [3,]    1   58
#>  [4,]   58   52
#>  [5,]   74   90
#>  [6,]   94   38
#>  [7,]   68   31
#>  [8,]   92   85
#>  [9,]  141   20
#> [10,]   23   39
#> [11,]   NA   NA

ds$close()
```
