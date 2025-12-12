# Bounding box intersection / union

`bbox_intersect()` returns the bounding box intersection, and
`bbox_union()` returns the bounding box union, for input of either
raster file names or list of bounding boxes. All of the inputs must be
in the same projected coordinate system.

## Usage

``` r
bbox_intersect(x, as_wkt = FALSE)

bbox_union(x, as_wkt = FALSE)
```

## Arguments

- x:

  Either a character vector of raster file names, or a list with each
  element a bounding box numeric vector (xmin, ymin, xmax, ymax).

- as_wkt:

  Logical. `TRUE` to return the bounding box as a polygon in OGC WKT
  format, or `FALSE` to return as a numeric vector.

## Value

The intersection (`bbox_intersect()`) or union (`bbox_union()`) of
inputs. If `as_wkt = FALSE`, a numeric vector of length four containing
xmin, ymin, xmax, ymax. If `as_wkt = TRUE`, a character string
containing OGC WKT for the bbox as POLYGON.

## See also

[`bbox_from_wkt()`](https://firelab.github.io/gdalraster/reference/bbox_from_wkt.md),
[`bbox_to_wkt()`](https://firelab.github.io/gdalraster/reference/bbox_to_wkt.md)

## Examples

``` r
bbox_list <-list()

elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, elev_file)
bbox_list[[1]] <- ds$bbox()
ds$close()

b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
ds <- new(GDALRaster, b5_file)
bbox_list[[2]] <- ds$bbox()
ds$close()

bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
bbox_list[[3]] <- bbox_from_wkt(bnd)

print(bbox_list)
#> [[1]]
#> [1]  323476.1 5101872.0  327766.1 5105082.0
#> 
#> [[2]]
#> [1]  323400.9 5101815.8  327870.9 5105175.8
#> 
#> [[3]]
#> [1]  323794.2 5102885.8  326420.0 5104929.4
#> 
bbox_intersect(bbox_list)
#> [1]  323794.2 5102885.8  326420.0 5104929.4
bbox_union(bbox_list)
#> [1]  323400.9 5101815.8  327870.9 5105175.8
```
