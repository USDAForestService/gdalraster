# Apply a coordinate transformation to a WKB/WKT geometry

`g_transform()` will transform the coordinates of a geometry from their
current spatial reference system to a new target spatial reference
system. Normally this means reprojecting the vectors, but it could
include datum shifts, and changes of units.

## Usage

``` r
g_transform(
  geom,
  srs_from,
  srs_to,
  wrap_date_line = FALSE,
  date_line_offset = 10L,
  traditional_gis_order = TRUE,
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

- srs_from:

  Character string specifying the spatial reference system for `geom`.
  May be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

- srs_to:

  Character string specifying the output spatial reference system. May
  be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

- wrap_date_line:

  Logical value, `TRUE` to correct geometries that incorrectly go from a
  longitude on a side of the antimeridian to the other side. Defaults to
  `FALSE`.

- date_line_offset:

  Integer longitude gap in degree. Defaults to `10L`.

- traditional_gis_order:

  Logical value, `TRUE` to use traditional GIS order of axis mapping
  (the default) or `FALSE` to use authority compliant axis order. By
  default, input `geom` vertices are assumed to be in longitude/latitude
  order if `srs_from` is a geographic coordinate system. This can be
  overridden by setting `traditional_gis_order = FALSE`.

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
geometries. `NULL` (`as_wkb = TRUE`) / `NA` (`as_wkb = FALSE`) is
returned with a warning if WKB input cannot be converted into an OGR
geometry object, or if an error occurs in the call to the underlying OGR
API.

## Note

This function uses the `OGR_GeomTransformer_Create()` and
`OGR_GeomTransformer_Transform()` functions in the GDAL API: "This is an
enhanced version of `OGR_G_Transform()`. When reprojecting geometries
from a Polar Stereographic projection or a projection naturally crossing
the antimeridian (like UTM Zone 60) to a geographic CRS, it will cut
geometries along the antimeridian. So a `LineString` might be returned
as a `MultiLineString`."

The `wrap_date_line = TRUE` option might be specified for circumstances
to correct geometries that incorrectly go from a longitude on a side of
the antimeridian to the other side, e.g., `LINESTRING (-179 0,179 0)`
will be transformed to
`MULTILINESTRING ((-179 0,-180 0),(180 0,179 0))`. For that use case,
`srs_to` might be the same as `srs_from`.

## See also

[`bbox_transform()`](https://firelab.github.io/gdalraster/reference/bbox_transform.md),
[`transform_bounds()`](https://firelab.github.io/gdalraster/reference/transform_bounds.md)

## Examples

``` r
pt <- "POINT (-114.0 47.0)"
g_transform(pt, "WGS84", "EPSG:5070", as_wkb = FALSE)
#> [1] "POINT (-1367125.77866242 2797551.11860518)"

# correct geometries that incorrectly go from a longitude on a side of the
# antimeridian to the other side
geom <- "LINESTRING (-179 0,179 0)"
g_transform(geom, "WGS84", "WGS84", wrap_date_line = TRUE, as_wkb = FALSE)
#> [1] "MULTILINESTRING ((-179 0,-180 0),(180 0,179 0))"
```
