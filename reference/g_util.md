# Geometry utility functions operating on WKB or WKT

These functions operate on input geometries in OGC WKB or WKT format to
perform various manipulations for utility purposes.

## Usage

``` r
g_make_valid(
  geom,
  method = "LINEWORK",
  keep_collapsed = FALSE,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_normalize(
  geom,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_set_3D(
  geom,
  is_3d,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_set_measured(
  geom,
  is_measured,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_swap_xy(
  geom,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)
```

## Arguments

- geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings.

- method:

  Character string. One of `"LINEWORK"` (the default) or `"STRUCTURE"`
  (requires GEOS \>= 3.10 and GDAL \>= 3.4). See Details.

- keep_collapsed:

  Logical value, applies only to the STRUCTURE method. Defaults to
  `FALSE`. See Details.

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

- is_3d:

  Logical value, `TRUE` if the input geometries should have a Z
  dimension, or `FALSE` to remove the Z dimension.

- is_measured:

  Logical value, `TRUE` if the input geometries should have a M
  dimension, or `FALSE` to remove the M dimension.

## Value

A geometry as WKB raw vector or WKT string, or a list/character vector
of geometries as WKB/WKT with length equal to `length(geom)`. `NULL` is
returned with a warning if WKB input cannot be converted into an OGR
geometry object, or if an error occurs in the call to the underlying OGR
API.

## Details

These functions use the GEOS library via GDAL headers.

`g_make_valid()` attempts to make an invalid geometry valid without
losing vertices. Already-valid geometries are cloned without further
intervention. Wrapper of `OGR_G_MakeValid()`/`OGR_G_MakeValidEx()` in
the GDAL API. Requires the GEOS \>= 3.8 library, check it for the
definition of the geometry operation. If GDAL is built without GEOS \>=
3.8, this function will return a clone of the input geometry if it is
valid, or `NULL` (`as_wkb = TRUE`) / `NA` (`as_wkb = FALSE`) if it is
invalid.

- `"LINEWORK"` is the default method, which combines all rings into a
  set of noded lines and then extracts valid polygons from that linework
  (requires GEOS \>= 3.10 and GDAL \>= 3.4). The `"STRUCTURE"` method
  first makes all rings valid, then merges shells and subtracts holes
  from shells to generate a valid result. Assumes that holes and shells
  are correctly categorized.

- `keep_collapsed` only applies to the `"STRUCTURE"` method:

  - `FALSE` (the default): collapses are converted to empty geometries

  - `TRUE`: collapses are converted to a valid geometry of lower
    dimension

`g_normalize()` organizes the elements, rings, and coordinate order of
geometries in a consistent way, so that geometries that represent the
same object can be easily compared. Wrapper of `OGR_G_Normalize()` in
the GDAL API. Requires GDAL \>= 3.3. Normalization ensures the
following:

- Lines are oriented to have smallest coordinate first (apart from
  duplicate endpoints)

- Rings start with their smallest coordinate (using XY ordering)

- Polygon shell rings are oriented clockwise, and holes
  counter-clockwise

- Collection elements are sorted by their first coordinate

`g_set_3D()` adds or removes the explicit Z coordinate dimension.
Removing the Z coordinate dimension of a geometry will remove any
existing Z values. Adding the Z dimension to a geometry collection, a
compound curve, a polygon, etc. will affect the children geometries.
Wrapper of `OGR_G_Set3D()` in the GDAL API.

`g_set_measured()` adds or removes the explicit M coordinate dimension.
Removing the M coordinate dimension of a geometry will remove any
existing M values. Adding the M dimension to a geometry collection, a
compound curve, a polygon, etc. will affect the children geometries.
Wrapper of `OGR_G_SetMeasured()` in the GDAL API.

`g_swap_xy()` swaps x and y coordinates of the input geometry. Wrapper
of `OGR_G_SwapXY()` in the GDAL API.

## See also

[`g_is_valid()`](https://firelab.github.io/gdalraster/reference/g_query.md),
[`g_is_3D()`](https://firelab.github.io/gdalraster/reference/g_query.md),
[`g_is_measured()`](https://firelab.github.io/gdalraster/reference/g_query.md)

## Examples

``` r
## g_make_valid() requires GEOS >= 3.8, otherwise is only a validity test
geos_version()
#> $name
#> [1] "3.12.1"
#> 
#> $major
#> [1] 3
#> 
#> $minor
#> [1] 12
#> 
#> $patch
#> [1] 1
#> 

# valid
wkt <- "POINT (0 0)"
g_make_valid(wkt, as_wkb = FALSE)
#> [1] "POINT (0 0)"

# invalid to valid
wkt <- "POLYGON ((0 0,10 10,0 10,10 0,0 0))"
g_make_valid(wkt, as_wkb = FALSE)
#> [1] "MULTIPOLYGON (((10 0,0 0,5 5,10 0)),((10 10,5 5,0 10,10 10)))"

# invalid - error
wkt <- "LINESTRING (0 0)"
g_make_valid(wkt)  # NULL
#> GDAL FAILURE 1: IllegalArgumentException: point array must contain 0 or >1 elements
#> 
#> Warning: OGR MakeValid() gave NULL geometry
#> NULL

## g_normalize() requires GDAL >= 3.3
if (gdal_version_num() >= gdal_compute_version(3, 3, 0)) {
  g <- "POLYGON ((0 1,1 1,1 0,0 0,0 1))"
  g_normalize(g) |> g_wk2wk()
}
#> [1] "POLYGON ((0 0,0 1,1 1,1 0,0 0))"

## set 3D / set measured
pt_xyzm <- g_create("POINT", c(1, 9, 100, 2000))

g_wk2wk(pt_xyzm, as_iso = TRUE)
#> [1] "POINT ZM (1 9 100 2000)"

g_set_3D(pt_xyzm, is_3d = FALSE) |> g_wk2wk(as_iso = TRUE)
#> [1] "POINT M (1 9 2000)"

g_set_measured(pt_xyzm, is_measured = FALSE) |> g_wk2wk(as_iso = TRUE)
#> [1] "POINT Z (1 9 100)"

## swap XY
g <- "GEOMETRYCOLLECTION(POINT(1 2),
                         LINESTRING(1 2,2 3),
                         POLYGON((0 0,0 1,1 1,0 0)))"

g_swap_xy(g, as_wkb = FALSE)
#> [1] "GEOMETRYCOLLECTION (POINT (2 1),LINESTRING (2 1,3 2),POLYGON ((0 0,1 0,1 1,0 0)))"
```
