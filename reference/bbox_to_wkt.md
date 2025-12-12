# Convert a bounding box to POLYGON in OGC WKT format

`bbox_to_wkt()` returns a WKT POLYGON string for the given bounding box.

## Usage

``` r
bbox_to_wkt(bbox, extend_x = 0, extend_y = 0)
```

## Arguments

- bbox:

  Numeric vector of length four containing xmin, ymin, xmax, ymax.

- extend_x:

  Numeric scalar. Distance in units of `bbox` to extend the rectangle in
  both directions along the x-axis (results in
  `xmin = bbox[1] - extend_x`, `xmax = bbox[3] + extend_x`).

- extend_y:

  Numeric scalar. Distance in units of `bbox` to extend the rectangle in
  both directions along the y-axis (results in
  `ymin = bbox[2] - extend_y`, `ymax = bbox[4] + extend_y`).

## Value

Character string for an OGC WKT polygon.

## See also

[`bbox_from_wkt()`](https://firelab.github.io/gdalraster/reference/bbox_from_wkt.md),
[`g_buffer()`](https://firelab.github.io/gdalraster/reference/g_unary_op.md)

## Examples

``` r
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, elev_file, read_only=TRUE)
bbox_to_wkt(ds$bbox())
#> [1] "POLYGON ((323476.071970863 5101871.98303138,327766.071970863 5101871.98303138,327766.071970863 5105081.98303138,323476.071970863 5105081.98303138,323476.071970863 5101871.98303138))"
ds$close()
```
