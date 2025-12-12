# Invert geotransform

`inv_geotransform()` inverts a vector of geotransform coefficients. This
converts the equation from being:  
raster pixel/line (column/row) -\> geospatial x/y coordinate  
to:  
geospatial x/y coordinate -\> raster pixel/line (column/row)

## Usage

``` r
inv_geotransform(gt)
```

## Arguments

- gt:

  Numeric vector of length six containing the geotransform to invert.

## Value

Numeric vector of length six containing the inverted geotransform. The
output vector will contain NAs if the input geotransform is
uninvertable.

## See also

[`GDALRaster$getGeoTransform()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`get_pixel_line()`](https://firelab.github.io/gdalraster/reference/get_pixel_line.md)

## Examples

``` r
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, elev_file)
invgt <- ds$getGeoTransform() |> inv_geotransform()
ds$close()

ptX = 324181.7
ptY = 5103901.4

## for a point x, y in the spatial reference system of elev_file
## raster pixel (column number):
pixel <- floor(invgt[1] +
               invgt[2] * ptX +
               invgt[3] * ptY)

## raster line (row number):
line <- floor(invgt[4] +
              invgt[5] * ptX +
              invgt[6] * ptY)

## get_pixel_line() applies this conversion
```
