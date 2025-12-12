# Binary operations on WKB or WKT geometries

These functions implement operations on pairs of geometries in OGC WKB
or WKT format.

## Usage

``` r
g_intersection(
  this_geom,
  other_geom,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_union(
  this_geom,
  other_geom,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_difference(
  this_geom,
  other_geom,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_sym_difference(
  this_geom,
  other_geom,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)
```

## Arguments

- this_geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings.

- other_geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings. Must contain the same
  number of geometries as `this_geom`.

- as_wkb:

  Logical value, `TRUE` to return the output geometry in WKB format (the
  default), or `FALSE` to return as WKT.

- as_iso:

  Logical value, `TRUE` to export as ISO WKB/WKT (ISO 13249 SQL/MM Part
  3), or `FALSE` (the default) to export as "Extended WKB/WKT".

- byte_order:

  Character string specifying the byte order when output is WKB. One of
  `"LSB"` (the default) or `"MSB"` (uncommon).

- quiet:

  Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.

## Value

A geometry as WKB raw vector or WKT string, or a list/character vector
of geometries as WKB/WKT with length equal to the number of input
geometry pairs. `NULL` (`as_wkb = TRUE`) / `NA` (`as_wkb = FALSE`) is
returned with a warning if WKB input cannot be converted into an OGR
geometry object, or if an error occurs in the call to the underlying OGR
API function.

## Details

These functions use the GEOS library via GDAL headers.

`g_intersection()` returns a new geometry which is the region of
intersection of the two geometries operated on.
[`g_intersects()`](https://firelab.github.io/gdalraster/reference/g_binary_pred.md)
can be used to test if two geometries intersect.

`g_union()` returns a new geometry which is the region of union of the
two geometries operated on.

`g_difference()` returns a new geometry which is the region of this
geometry with the region of the other geometry removed.

`g_sym_difference()` returns a new geometry which is the symmetric
difference of this geometry and the other geometry (union minus
intersection).

## Note

`this_geom` and `other_geom` are assumed to be in the same coordinate
reference system.

Geometry validity is not checked. In case you are unsure of the validity
of the input geometries, call
[`g_is_valid()`](https://firelab.github.io/gdalraster/reference/g_query.md)
before, otherwise the result might be wrong.

## Examples

``` r
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, elev_file)
g1 <- ds$bbox() |> bbox_to_wkt()
ds$close()

g2 <- "POLYGON ((327381.9 5104541.2, 326824.0 5104092.5, 326708.8 5103182.9,
  327885.2 5102612.9, 329334.5 5103322.4, 329304.2 5104474.5,328212.7
  5104656.4, 328212.7 5104656.4, 327381.9 5104541.2))"

# see spatial predicate defintions at https://en.wikipedia.org/wiki/DE-9IM
g_intersects(g1, g2)  # TRUE
#> [1] TRUE
g_overlaps(g1, g2)  # TRUE
#> [1] TRUE
# therefore,
g_contains(g1, g2)  # FALSE
#> [1] FALSE

g_sym_difference(g1, g2) |> g_area()
#> [1] 14834452

g3 <- g_intersection(g1, g2)
g4 <- g_union(g1, g2)
g_difference(g4, g3) |> g_area()
#> [1] 14834452
```
