# Get the bounding box of a geometry specified in OGC WKT format

`bbox_from_wkt()` returns the bounding box of a WKT 2D geometry (e.g.,
LINE, POLYGON, MULTIPOLYGON).

## Usage

``` r
bbox_from_wkt(wkt, extend_x = 0, extend_y = 0)
```

## Arguments

- wkt:

  Character. OGC WKT string for a simple feature 2D geometry.

- extend_x:

  Numeric scalar. Distance to extend the output bounding box in both
  directions along the x-axis (results in `xmin = bbox[1] - extend_x`,
  `xmax = bbox[3] + extend_x`).

- extend_y:

  Numeric scalar. Distance to extend the output bounding box in both
  directions along the y-axis (results in `ymin = bbox[2] - extend_y`,
  `ymax = bbox[4] + extend_y`).

## Value

Numeric vector of length four containing the xmin, ymin, xmax, ymax of
the geometry specified by `wkt` (possibly extended by values in
`extend_x`, `extend_y`).

## See also

[`bbox_to_wkt()`](https://firelab.github.io/gdalraster/reference/bbox_to_wkt.md)

## Examples

``` r
bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
bbox_from_wkt(bnd, 100, 100)
#> [1]  323694.2 5102785.8  326520.0 5105029.4
```
