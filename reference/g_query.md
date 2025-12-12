# Obtain information about WKB/WKT geometries

These functions return information about WKB/WKT geometries. The input
geometries may be given as a single raw vector of WKB, a list of WKB raw
vectors, or a character vector containing one or more WKT strings.

## Usage

``` r
g_is_empty(geom, quiet = FALSE)

g_is_valid(geom, quiet = FALSE)

g_is_3D(geom, quiet = FALSE)

g_is_measured(geom, quiet = FALSE)

g_is_ring(geom, quiet = FALSE)

g_name(geom, quiet = FALSE)

g_summary(geom, quiet = FALSE)

g_geom_count(geom, quiet = FALSE)
```

## Arguments

- geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings.

- quiet:

  Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.

## Details

`g_is_empty()` tests whether a geometry has no points. Returns a logical
vector of the same length as the number of input geometries containing
`TRUE` for the corresponding geometries that are empty or `FALSE` for
non-empty geometries.

`g_is_valid()` tests whether a geometry is valid. Returns a logical
vector analogous to the above for `g_is_empty()`.

`g_is_3D()` checks whether a geometry has Z coordinates. Returns a
logical vector analogous to the above for `g_is_empty()`.

`g_is_measured()` checks whether a geometry is measured (has M values).
Returns a logical vector analogous to the above for `g_is_empty()`.

`g_is_ring()` tests whether a geometry is a ring, `TRUE` if the
coordinates of the geometry form a ring by checking length and closure
(self-intersection is not checked), otherwise `FALSE`. Returns a logical
vector analogous to the above for `g_is_empty()`.

`g_name()` returns the WKT type names of the input geometries in a
character vector of the same length as the number of input geometries.

`g_summary()` returns text summaries of WKB/WKT geometries in a
character vector of the same length as the number of input geometries.
Requires GDAL \>= 3.7.

`g_geom_count()` returns the number of elements in a geometry or number
of geometries in container. Only geometries of type `Polygon[25D]`,
`MultiPoint[25D]`, `MultiLineString[25D]`, `MultiPolygon[25D]` or
`GeometryCollection[25D]` may return a valid value. Other geometry types
will silently return `0`.

## See also

[`g_make_valid()`](https://firelab.github.io/gdalraster/reference/g_util.md),
[`g_set_3D()`](https://firelab.github.io/gdalraster/reference/g_util.md),
[`g_set_measured()`](https://firelab.github.io/gdalraster/reference/g_util.md)

## Examples

``` r
g1 <- "POLYGON ((0 0, 10 10, 10 0, 0 0))"
g2 <- "POLYGON ((5 1, 9 5, 9 1, 5 1))"
g_difference(g2, g1) |> g_is_empty()
#> [1] TRUE

g1 <- "POLYGON ((0 0, 10 10, 10 0, 0 0))"
g2 <- "POLYGON ((0 0, 10 10, 10 0))"
g3 <- "POLYGON ((0 0, 10 10, 10 0, 0 1))"
g_is_valid(c(g1, g2, g3))
#> GDAL WARNING 1: IllegalArgumentException: Points of LinearRing do not form a closed linestring
#> GDAL WARNING 1: IllegalArgumentException: Points of LinearRing do not form a closed linestring
#> [1]  TRUE FALSE FALSE

g_is_3D(g1)
#> [1] FALSE
g_is_measured(g1)
#> [1] FALSE

pt_xyz <- g_create("POINT", c(1, 9, 100))
g_is_3D(pt_xyz)
#> [1] TRUE
g_is_measured(pt_xyz)
#> [1] FALSE

pt_xyzm <- g_create("POINT", c(1, 9, 100, 2000))
g_is_3D(pt_xyzm)
#> [1] TRUE
g_is_measured(pt_xyzm)
#> [1] TRUE

f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
lyr <- new(GDALVector, f, "mtbs_perims")

feat <- lyr$getNextFeature()
g_name(feat$geom)
#> [1] "MULTIPOLYGON"

# g_summary() requires GDAL >= 3.7
if (gdal_version_num() >= gdal_compute_version(3, 7, 0)) {
  feat <- lyr$getNextFeature()
  g_summary(feat$geom) |> print()

  feat_set <- lyr$fetch(5)
  g_summary(feat_set$geom) |> print()
}
#> [1] "MULTIPOLYGON : 3 geometries: POLYGON : 410 points POLYGON : 4 points POLYGON : 8 points"
#> [1] "MULTIPOLYGON : 1 geometries: POLYGON : 302 points"                                                           
#> [2] "MULTIPOLYGON : 1 geometries: POLYGON : 277 points"                                                           
#> [3] "MULTIPOLYGON : 2 geometries: POLYGON : 54 points POLYGON : 7 points"                                         
#> [4] "MULTIPOLYGON : 1 geometries: POLYGON : 159 points"                                                           
#> [5] "MULTIPOLYGON : 4 geometries: POLYGON : 1376 points POLYGON : 17 points POLYGON : 5 points POLYGON : 9 points"

g_geom_count(feat$geom)
#> [1] 3

lyr$close()
```
