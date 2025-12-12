# Compute measurements for WKB/WKT geometries

These functions compute measurements for geometries. The input
geometries may be given as a single raw vector of WKB, a list of WKB raw
vectors, or a character vector containing one or more WKT strings.

## Usage

``` r
g_area(geom, quiet = FALSE)

g_centroid(geom, quiet = FALSE)

g_distance(geom, other_geom, quiet = FALSE)

g_length(geom, quiet = FALSE)

g_geodesic_area(geom, srs, traditional_gis_order = TRUE, quiet = FALSE)

g_geodesic_length(geom, srs, traditional_gis_order = TRUE, quiet = FALSE)
```

## Arguments

- geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings.

- quiet:

  Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.

- other_geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings. Must contain the same
  number of geometries as `geom`, unless `geom` contains a single
  geometry in which case pairwise distances will be computed for
  one-to-many if `other_geom` contains multiple geometries (i.e.,
  "this-to-others").

- srs:

  Character string specifying the spatial reference system for `geom`.
  May be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

- traditional_gis_order:

  Logical value, `TRUE` to use traditional GIS order of axis mapping
  (the default) or `FALSE` to use authority compliant axis order. By
  default, input `geom` vertices are assumed to be in longitude/latitude
  order if `srs` is a geographic coordinate system. This can be
  overridden by setting `traditional_gis_order = FALSE`.

## Details

These functions use the GEOS library via GDAL headers.

`g_area()` computes the area for a `Polygon` or `MultiPolygon`.
Undefined for all other geometry types (returns zero). Returns a numeric
vector, having length equal to the number of input geometries,
containing computed area or '0' if undefined.

`g_centroid()` returns a numeric vector of length 2 containing the
centroid (X, Y), or a two-column numeric matrix (X, Y) with number of
rows equal to the number of input geometries. The GDAL documentation
states "This method relates to the SFCOM `ISurface::get_Centroid()`
method however the current implementation based on GEOS can operate on
other geometry types such as multipoint, linestring, geometrycollection
such as multipolygons. OGC SF SQL 1.1 defines the operation for surfaces
(polygons). SQL/MM-Part 3 defines the operation for surfaces and
multisurfaces (multipolygons)."

`g_distance()` returns the distance between two geometries or `-1` if an
error occurs. Returns the shortest distance between the two geometries.
The distance is expressed into the same unit as the coordinates of the
geometries. Returns a numeric vector, having length equal to the number
of input geometry pairs, containing computed distance or '-1' if an
error occurs.

`g_length()` computes the length for `LineString` or `MultiCurve`
objects. Undefined for all other geometry types (returns zero). Returns
a numeric vector, having length equal to the number of input geometries,
containing computed length or '0' if undefined.

`g_geodesic_area()` computes geometry area, considered as a surface on
the underlying ellipsoid of the SRS attached to the geometry. The
returned area will always be in square meters, and assumes that polygon
edges describe geodesic lines on the ellipsoid. If the geometry SRS is
not a geographic one, geometries are reprojected to the underlying
geographic SRS. By default, input geometry vertices are assumed to be in
longitude/latitude order if using a geographic coordinate system. This
can be overridden with the `traditional_gis_order` argument. Returns the
area in square meters, or `NA` in case of error (unsupported geometry
type, no SRS attached, etc.) Requires GDAL \>= 3.9.

`g_geodesic_length()` computes the length of the curve, considered as a
geodesic line on the underlying ellipsoid of the SRS attached to the
geometry. The returned length will always be in meters. If the geometry
SRS is not a geographic one, geometries are reprojected to the
underlying geographic SRS. By default, input geometry vertices are
assumed to be in longitude/latitude order if using a geographic
coordinate system. This can be overridden with the
`traditional_gis_order` argument. Returns the length in meters, or `NA`
in case of error (unsupported geometry type, no SRS attached, etc.)
Requires GDAL \>= 3.10.

## Note

For `g_distance()`, `geom` and `other_geom` must be in the same
coordinate reference system. If `geom` is a single geometry and
`other_geom` is a list or vector of multiple geometries, then distances
will be calculated between `geom` and each geometry in `other_geom`.
Otherwise, no recycling is done and `length(geom)` must equal
`length(other_geom)` to calculate distance between each corresponding
pair of input geometries.

Geometry validity is not checked. In case you are unsure of the validity
of the input geometries, call
[`g_is_valid()`](https://firelab.github.io/gdalraster/reference/g_query.md)
before, otherwise the result might be wrong.

## Examples

``` r
g_area("POLYGON ((0 0, 10 10, 10 0, 0 0))")
#> [1] 50

g_centroid("POLYGON ((0 0, 10 10, 10 0, 0 0))")
#>        x        y 
#> 6.666667 3.333333 

g_distance("POINT (0 0)", "POINT (5 12)")
#> [1] 13

g_length("LINESTRING (0 0, 3 4)")
#> [1] 5

f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
lyr <- new(GDALVector, f, "mtbs_perims")

# read all features into a data frame
feat_set <- lyr$fetch(-1)
head(feat_set)
#> OGR feature set
#>   FID              event_id  incid_name        incid_type   map_id burn_bnd_ac
#> 1   1 WY4413411069519870807     POLECAT          Wildfire 10015934        1093
#> 2   2 WY4509511033019880815 HELLROARING          Wildfire 10014146       77598
#> 3   3 MT4507511010219880619 STORM CREEK          Wildfire 10014147      116027
#> 4   4 WY4491210990219880711      CLOVER Wildland Fire Use 10014202       34035
#> 5   5 WY4463411003119880709        MIST          Wildfire 10014203        4376
#> 6   6 WY4499211096519880625         FAN Wildland Fire Use 10014215       20422
#>   burn_bnd_lat burn_bnd_lon    ig_date ig_year
#> 1       44.132     -110.696 1987-08-07    1987
#> 2       45.089     -110.328 1988-08-15    1988
#> 3       45.146     -110.013 1988-06-19    1988
#> 4       44.717     -110.034 1988-07-11    1988
#> 5       44.633     -110.028 1988-07-09    1988
#> 6       44.994     -110.976 1988-06-25    1988
#>                                    geom
#> 1 WKB MULTIPOLYGON: raw 01 06 00 00 ...
#> 2 WKB MULTIPOLYGON: raw 01 06 00 00 ...
#> 3 WKB MULTIPOLYGON: raw 01 06 00 00 ...
#> 4 WKB MULTIPOLYGON: raw 01 06 00 00 ...
#> 5 WKB MULTIPOLYGON: raw 01 06 00 00 ...
#> 6 WKB MULTIPOLYGON: raw 01 06 00 00 ...

g_area(feat_set$geom) |> head()
#> [1]   3626011 219210971 261779718 137157336  17666635  82653959

g_centroid(feat_set$geom) |> head()
#>             x         y
#> [1,] 504185.6 -11945.72
#> [2,] 534834.7  86727.91
#> [3,] 553856.8  89236.52
#> [4,] 556496.3  51930.36
#> [5,] 557768.3  42471.93
#> [6,] 483922.9  82676.71

lyr$close()
```
