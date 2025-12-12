# Transform a bounding box to a different projection

`bbox_transform()` is a convenience function to transform the
coordinates of a boundary from their current spatial reference system to
a new target spatial reference system.

## Usage

``` r
bbox_transform(bbox, srs_from, srs_to, use_transform_bounds = TRUE)
```

## Arguments

- bbox:

  Numeric vector of length four containing a bounding box (xmin, ymin,
  xmax, ymax) to transform.

- srs_from:

  Character string specifying the spatial reference system for `pts`.
  May be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

- srs_to:

  Character string specifying the output spatial reference system. May
  be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

- use_transform_bounds:

  Logical value, `TRUE` to use
  [`transform_bounds()`](https://firelab.github.io/gdalraster/reference/transform_bounds.md)
  (the default, requires GDAL \>= 3.4). If `FALSE`, transformation is
  done with
  [`g_transform()`](https://firelab.github.io/gdalraster/reference/g_transform.md).

## Value

Numeric vector of length four containing a transformed bounding box
(xmin, ymin, xmax, ymax).

## Details

With `use_transform_bounds = TRUE` (the default) this function returns:

    # requires GDAL >= 3.4
    transform_bounds(bbox, srs_from, srs_to)

See Details for
[`transform_bounds()`](https://firelab.github.io/gdalraster/reference/transform_bounds.md)
for cases where the bounds crossed the antimeridian.

With `use_transform_bounds = FALSE`, this function returns:

    bbox_to_wkt(bbox) |>
      g_transform(srs_from, srs_to) |>
      bbox_from_wkt()

See the Note for
[`g_transform()`](https://firelab.github.io/gdalraster/reference/g_transform.md)
for cases where the bounds crossed the antimeridian.

## See also

[`bbox_from_wkt()`](https://firelab.github.io/gdalraster/reference/bbox_from_wkt.md),
[`g_transform()`](https://firelab.github.io/gdalraster/reference/g_transform.md),
[`transform_bounds()`](https://firelab.github.io/gdalraster/reference/transform_bounds.md)

## Examples

``` r
bb <- c(-1405880.72, -1371213.76, 5405880.72, 5371213.76)

# the default assumes GDAL >= 3.4
if (gdal_version_num() >= gdal_compute_version(3, 4, 0)) {
  bb_wgs84 <- bbox_transform(bb, "EPSG:32661", "EPSG:4326")
} else {
  bb_wgs84 <- bbox_transform(bb, "EPSG:32661", "EPSG:4326",
                             use_transform_bounds = FALSE)
}

print(bb_wgs84)
#> [1] -180.00000   48.65641  180.00000   90.00000
```
