# Transform boundary

`transform_bounds()` transforms a bounding box, densifying the edges to
account for nonlinear transformations along these edges and extracting
the outermost bounds. Multiple bounding boxes may be given as rows of a
numeric matrix or data frame. Wrapper of `OCTTransformBounds()` in the
GDAL Spatial Reference System API. Requires GDAL \>= 3.4.

## Usage

``` r
transform_bounds(
  bbox,
  srs_from,
  srs_to,
  densify_pts = 21L,
  traditional_gis_order = TRUE
)
```

## Arguments

- bbox:

  Either a numeric vector of length four containing the input bounding
  box (xmin, ymin, xmax, ymax), or a four-column numeric matrix of
  bounding boxes (or data frame that can be coerced to a four-column
  numeric matrix).

- srs_from:

  Character string specifying the spatial reference system for `pts`.
  May be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

- srs_to:

  Character string specifying the output spatial reference system. May
  be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

- densify_pts:

  Integer value giving the number of points to use to densify the
  bounding polygon in the transformation. Recommended to use `21` (the
  default).

- traditional_gis_order:

  Logical value, `TRUE` to use traditional GIS order of axis mapping
  (the default) or `FALSE` to use authority compliant axis order (see
  Note).

## Value

For a single input bounding box, a numeric vector of length four
containing the transformed bounding box in the output spatial reference
system (xmin, ymin, xmax, ymax). For input of multiple bounding boxes, a
four-column numeric matrix with each row containing the corresponding
transformed bounding box (xmin, ymin, xmax, ymax).

## Details

The following refer to the *output* values `xmin`, `ymin`, `xmax`,
`ymax`:

If the destination CRS is geographic, the first axis is longitude, and
`xmax < xmin` then the bounds crossed the antimeridian. In this scenario
there are two polygons, one on each side of the antimeridian. The first
polygon should be constructed with `(xmin, ymin, 180, ymax)` and the
second with `(-180, ymin, xmax, ymax)`.

If the destination CRS is geographic, the first axis is latitude, and
`ymax < ymin` then the bounds crossed the antimeridian. In this scenario
there are two polygons, one on each side of the antimeridian. The first
polygon should be constructed with `(ymin, xmin, ymax, 180)` and the
second with `(ymin, -180, ymax, xmax)`.

## Note

`traditional_gis_order = TRUE` (the default) means that for geographic
CRS with lat/long order, the data will still be long/lat ordered.
Similarly for a projected CRS with northing/easting order, the data will
still be easting/northing ordered (GDAL's OAMS_TRADITIONAL_GIS_ORDER).

`traditional_gis_order = FALSE` means that the data axis will be
identical to the CRS axis (GDAL's OAMS_AUTHORITY_COMPLIANT).

See
<https://gdal.org/en/stable/tutorials/osr_api_tut.html#crs-and-axis-order>.

## Examples

``` r
bb <- c(-1405880.71737, -1371213.76254, 5405880.71737, 5371213.76254)

# traditional GIS axis ordering by  default (lon, lat)
transform_bounds(bb, "EPSG:32761", "EPSG:4326")
#> [1] -180.00000  -90.00000  180.00000  -48.65641

# authority compliant axis ordering
transform_bounds(bb, "EPSG:32761", "EPSG:4326",
                 traditional_gis_order = FALSE)
#> [1]  -90.00000 -180.00000  -48.65641  180.00000
```
