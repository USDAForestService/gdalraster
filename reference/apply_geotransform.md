# Apply geotransform (raster column/row to geospatial x/y)

`apply_geotransform()` applies geotransform coefficients to raster
coordinates in pixel/line space (column/row), converting into
georeferenced (x/y) coordinates. Wrapper of `GDALApplyGeoTransform()` in
the GDAL API, operating on matrix input.

## Usage

``` r
apply_geotransform(col_row, gt)
```

## Arguments

- col_row:

  Numeric matrix of raster column, row (pixel/line) coordinates (or
  two-column data frame that will be coerced to numeric matrix, or a
  vector of column, row for one coordinate).

- gt:

  Either a numeric vector of length six containing the affine
  geotransform for the raster, or an object of class `GDALRaster` from
  which the geotransform will be obtained.

## Value

Numeric matrix of geospatial x/y coordinates.

## Note

Bounds checking on the input coordinates is done if `gt` is obtained
from an object of class `GDALRaster`. See Note for
[`get_pixel_line()`](https://firelab.github.io/gdalraster/reference/get_pixel_line.md).

## See also

[`GDALRaster$getGeoTransform()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`get_pixel_line()`](https://firelab.github.io/gdalraster/reference/get_pixel_line.md)

## Examples

``` r
raster_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
ds <- new(GDALRaster, raster_file)

# compute some raster coordinates in column/row space
set.seed(42)
col_coords <- runif(10, min = 0, max = ds$getRasterXSize() - 0.00001)
row_coords <- runif(10, min = 0, max = ds$getRasterYSize() - 0.00001)
col_row <- cbind(col_coords, row_coords)

# convert to geospatial x/y coordinates
gt <- ds$getGeoTransform()
apply_geotransform(col_row, gt)
#>           [,1]    [,2]
#>  [1,] 327400.6 5103613
#>  [2,] 327496.1 5102774
#>  [3,] 324703.6 5102082
#>  [4,] 327038.7 5104262
#>  [5,] 326229.2 5103598
#>  [6,] 325703.0 5102065
#>  [7,] 326636.0 5101942
#>  [8,] 324053.8 5104705
#>  [9,] 326294.6 5103557
#> [10,] 326500.8 5103283

# or, using the class method
ds$apply_geotransform(col_row)
#>           [,1]    [,2]
#>  [1,] 327400.6 5103613
#>  [2,] 327496.1 5102774
#>  [3,] 324703.6 5102082
#>  [4,] 327038.7 5104262
#>  [5,] 326229.2 5103598
#>  [6,] 325703.0 5102065
#>  [7,] 326636.0 5101942
#>  [8,] 324053.8 5104705
#>  [9,] 326294.6 5103557
#> [10,] 326500.8 5103283

# bounds checking
col_row <- rbind(col_row, c(ds$getRasterXSize(), ds$getRasterYSize()))
ds$apply_geotransform(col_row)
#>           [,1]    [,2]
#>  [1,] 327400.6 5103613
#>  [2,] 327496.1 5102774
#>  [3,] 324703.6 5102082
#>  [4,] 327038.7 5104262
#>  [5,] 326229.2 5103598
#>  [6,] 325703.0 5102065
#>  [7,] 326636.0 5101942
#>  [8,] 324053.8 5104705
#>  [9,] 326294.6 5103557
#> [10,] 326500.8 5103283
#> [11,] 327766.1 5101872

ds$close()
```
