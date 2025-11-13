# Unary operations on WKB or WKT geometries

These functions implement algorithms that operate on one input geometry
for which a new output geometry is generated. The input geometries may
be given as a single raw vector of WKB, a list of WKB raw vectors, or a
character vector containing one or more WKT strings.

## Usage

``` r
g_buffer(
  geom,
  dist,
  quad_segs = 30L,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_boundary(
  geom,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_convex_hull(
  geom,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_concave_hull(
  geom,
  ratio,
  allow_holes,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_delaunay_triangulation(
  geom,
  constrained = FALSE,
  tolerance = 0,
  only_edges = FALSE,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_simplify(
  geom,
  tolerance,
  preserve_topology = TRUE,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_unary_union(
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

- dist:

  Numeric buffer distance in units of the input `geom`.

- quad_segs:

  Integer number of segments used to define a 90 degree curve (quadrant
  of a circle). Large values result in large numbers of vertices in the
  resulting buffer geometry while small numbers reduce the accuracy of
  the result.

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

- ratio:

  Numeric value in interval `[0, 1]`. The target criterion parameter for
  `g_concave_hull()`, expressed as a ratio between the lengths of the
  longest and shortest edges. `1` produces the convex hull; `0` produces
  a hull with maximum concaveness (see Note).

- allow_holes:

  Logical value, whether holes are allowed.

- constrained:

  Logical value, `TRUE` to return a constrained Delaunay triangulation
  of the vertices of the given polygon(s). Defaults to `FALSE`.

- tolerance:

  Numeric value. For `g_simplify()`, the simplification tolerance as
  distance in units of the input `geom`. Simplification removes vertices
  which are within the tolerance distance of the simplified linework (as
  long as topology is preserved when `preserve_topology = TRUE`). For
  `g_delaunay_triangulation()`, an optional snapping tolerance to use
  for improved robustness (ignored if `constrained = TRUE`).

- only_edges:

  Logical value. If `TRUE`, `g_delaunay_triangulation()` will return a
  MULTILINESTRING, otherwise it will return a GEOMETRYCOLLECTION
  containing triangular POLYGONs (the default). Ignored if
  `constrained = TRUE`

- preserve_topology:

  Logical value, `TRUE` to simplify geometries while preserving topology
  (the default). Setting to `FALSE` simplifies geometries using the
  standard Douglas-Peucker algorithm which is significantly faster (see
  Note).

## Value

A geometry as WKB raw vector or WKT string, or a list/character vector
of geometries as WKB/WKT with length equal to the number of input
geometries. `NULL` (`as_wkb = TRUE`) / `NA` (`as_wkb = FALSE`) is
returned with a warning if WKB input cannot be converted into an OGR
geometry object, or if an error occurs in the call to the underlying OGR
API.

## Details

These functions use the GEOS library via GDAL headers.

`g_boundary()` computes the boundary of a geometry. Wrapper of
`OGR_G_Boundary()` in the GDAL Geometry API.

`g_buffer()` builds a new geometry containing the buffer region around
the geometry on which it is invoked. The buffer is a polygon containing
the region within the buffer distance of the original geometry. Wrapper
of `OGR_G_Buffer()` in the GDAL API.

`g_convex_hull()` computes a convex hull, the smallest convex geometry
that contains all the points in the input geometry. Wrapper of
`OGR_G_ConvexHull()` in the GDAL API.

`g_concave_hull()` returns a "concave hull" of a geometry. A concave
hull is a polygon which contains all the points of the input, but is a
better approximation than the convex hull to the area occupied by the
input. Frequently used to convert a multi-point into a polygonal area
that contains all the points in the input geometry. Requires GDAL \>=
3.6 and GEOS \>= 3.11.

`g_delaunay_triangulation()`

- `constrained = FALSE`: returns a Delaunay triangulation of the
  vertices of the input geometry. Wrapper of
  `OGR_G_DelaunayTriangulation()` in the GDAL API. Requires GEOS \>=
  3.4.

- `constrained = TRUE`: returns a constrained Delaunay triangulation of
  the vertices of the given polygon(s). For non-polygonal inputs,
  silently returns an empty geometry collection. Requires GDAL \>= 3.12
  and GEOS \>= 3.10.

`g_simplify()` computes a simplified geometry. By default, it simplifies
the input geometries while preserving topology (see Note). Wrapper of
`OGR_G_Simplify()` / `OGR_G_SimplifyPreserveTopology()` in the GDAL API.

`g_unary_union()` returns the union of all components of a single
geometry. Usually used to convert a collection into the smallest set of
polygons that cover the same area. See
<https://postgis.net/docs/ST_UnaryUnion.html> for more details. Requires
GDAL \>= 3.7.

## Note

Definitions of these operations are given in the GEOS documentation
(<https://libgeos.org/doxygen/>, GEOS 3.15.0dev), some of which is
copied here.

`g_boundary()` computes the "boundary" as defined by the DE9IM
(<https://en.wikipedia.org/wiki/DE-9IM>):

- the boundary of a `Polygon` is the set of linear rings dividing the
  exterior from the interior

- the boundary of a `LineString` is the two end points

- the boundary of a `Point`/`MultiPoint` is defined as empty

`g_buffer()` always returns a polygonal result. The negative or
zero-distance buffer of lines and points is always an empty `Polygon`.

`g_convex_hull()` uses the Graham Scan algorithm.

`g_concave_hull()`: A set of points has a sequence of hulls of
increasing concaveness, determined by a numeric target parameter. The
concave hull is constructed by removing the longest outer edges of the
Delaunay Triangulation of the space between the polygons, until the
target criterion parameter is reached. This can be expressed as a ratio
between the lengths of the longes and shortest edges. `1` produces the
convex hull; `0` produces a hull with maximum concaveness.

`g_simplify()`:

- With `preserve_topology = TRUE` (the default):  
  Simplifies a geometry, ensuring that the result is a valid geometry
  having the same dimension and number of components as the input. The
  simplification uses a maximum distance difference algorithm similar to
  the one used in the Douglas-Peucker algorithm. In particular, if the
  input is an areal geometry (`Polygon` or `MultiPolygon`), the result
  has the same number of shells and holes (rings) as the input, in the
  same order. The result rings touch at no more than the number of
  touching point in the input (although they may touch at fewer points).

- With `preserve_topology = FALSE`:  
  Simplifies a geometry using the standard Douglas-Peucker algorithm.
  Ensures that any polygonal geometries returned are valid. Simple lines
  are not guaranteed to remain simple after simplification. Note that in
  general D-P does not preserve topology - e.g. polygons can be split,
  collapse to lines or disappear, holes can be created or disappear, and
  lines can cross. To simplify geometry while preserving topology use
  `TopologyPreservingSimplifier`. (However, using D-P is significantly
  faster.)

`preserve_topology = TRUE` does not preserve boundaries shared between
polygons.

## Examples

``` r
g <- "POLYGON((0 0,1 1,1 0,0 0))"
g_boundary(g, as_wkb = FALSE)
#> [1] "LINESTRING (0 0,1 1,1 0,0 0)"

g <- "POINT (0 0)"
g_buffer(g, dist = 10, as_wkb = FALSE)
#> [1] "POLYGON ((10 0,9.98629534754574 -0.523359562429438,9.94521895368273 -1.04528463267653,9.87688340595138 -1.56434465040231,9.78147600733806 -2.07911690817759,9.65925826289068 -2.58819045102521,9.51056516295153 -3.09016994374947,9.33580426497202 -3.583679495453,9.13545457642601 -4.067366430758,8.91006524188368 -4.53990499739547,8.66025403784439 -5,8.38670567945424 -5.44639035015027,8.09016994374947 -5.87785252292473,7.77145961456971 -6.29320391049837,7.43144825477394 -6.69130606358858,7.07106781186548 -7.07106781186547,6.69130606358858 -7.43144825477394,6.29320391049838 -7.77145961456971,5.87785252292473 -8.09016994374947,5.44639035015027 -8.38670567945424,5.0 -8.66025403784439,4.53990499739547 -8.91006524188368,4.067366430758 -9.13545457642601,3.583679495453 -9.33580426497202,3.09016994374947 -9.51056516295153,2.58819045102521 -9.65925826289068,2.07911690817759 -9.78147600733806,1.56434465040231 -9.87688340595138,1.04528463267654 -9.94521895368273,0.52335956242944 -9.98629534754574,0.0 -10,-0.523359562429436 -9.98629534754574,-1.04528463267653 -9.94521895368273,-1.56434465040231 -9.87688340595138,-2.07911690817759 -9.78147600733806,-2.58819045102521 -9.65925826289068,-3.09016994374947 -9.51056516295154,-3.583679495453 -9.33580426497202,-4.067366430758 -9.13545457642601,-4.53990499739547 -8.91006524188368,-5 -8.66025403784439,-5.44639035015027 -8.38670567945424,-5.87785252292473 -8.09016994374947,-6.29320391049837 -7.77145961456971,-6.69130606358858 -7.43144825477394,-7.07106781186547 -7.07106781186548,-7.43144825477394 -6.69130606358858,-7.77145961456971 -6.29320391049838,-8.09016994374947 -5.87785252292473,-8.38670567945424 -5.44639035015027,-8.66025403784439 -5,-8.91006524188368 -4.53990499739547,-9.13545457642601 -4.067366430758,-9.33580426497202 -3.58367949545301,-9.51056516295153 -3.09016994374947,-9.65925826289068 -2.58819045102521,-9.78147600733806 -2.0791169081776,-9.87688340595138 -1.56434465040231,-9.94521895368273 -1.04528463267654,-9.98629534754574 -0.523359562429442,-10 -0.0,-9.98629534754574 0.523359562429436,-9.94521895368273 1.04528463267653,-9.87688340595138 1.56434465040231,-9.78147600733806 2.07911690817759,-9.65925826289068 2.5881904510252,-9.51056516295154 3.09016994374947,-9.33580426497202 3.583679495453,-9.13545457642601 4.067366430758,-8.91006524188368 4.53990499739547,-8.66025403784439 5.0,-8.38670567945424 5.44639035015027,-8.09016994374948 5.87785252292473,-7.77145961456971 6.29320391049837,-7.43144825477395 6.69130606358858,-7.07106781186548 7.07106781186547,-6.69130606358858 7.43144825477394,-6.29320391049838 7.77145961456971,-5.87785252292473 8.09016994374947,-5.44639035015028 8.38670567945424,-5 8.66025403784438,-4.53990499739547 8.91006524188368,-4.06736643075801 9.135454576426,-3.58367949545301 9.33580426497202,-3.09016994374948 9.51056516295153,-2.58819045102522 9.65925826289068,-2.0791169081776 9.78147600733806,-1.56434465040231 9.87688340595138,-1.04528463267654 9.94521895368273,-0.523359562429443 9.98629534754574,-0.0 10.0,0.523359562429431 9.98629534754574,1.04528463267653 9.94521895368273,1.56434465040231 9.87688340595138,2.07911690817759 9.78147600733806,2.5881904510252 9.65925826289068,3.09016994374947 9.51056516295154,3.583679495453 9.33580426497202,4.067366430758 9.13545457642601,4.53990499739547 8.91006524188368,4.99999999999999 8.66025403784439,5.44639035015027 8.38670567945424,5.87785252292473 8.09016994374948,6.29320391049837 7.77145961456971,6.69130606358858 7.43144825477395,7.07106781186547 7.07106781186548,7.43144825477394 6.69130606358859,7.77145961456971 6.29320391049838,8.09016994374947 5.87785252292473,8.38670567945424 5.44639035015028,8.66025403784438 5.0,8.91006524188368 4.53990499739547,9.135454576426 4.06736643075801,9.33580426497202 3.58367949545301,9.51056516295153 3.09016994374948,9.65925826289068 2.58819045102522,9.78147600733806 2.0791169081776,9.87688340595138 1.56434465040231,9.94521895368273 1.04528463267654,9.98629534754574 0.523359562429444,10 0))"

g <- "GEOMETRYCOLLECTION(POINT(0 1), POINT(0 0), POINT(1 0), POINT(1 1))"
g_convex_hull(g, as_wkb = FALSE)
#> [1] "POLYGON ((0 0,0 1,1 1,1 0,0 0))"

# g_concave_hull() requires GDAL >= 3.6 and GEOS >= 3.11
if (gdal_version_num() >= gdal_compute_version(3, 6, 0) &&
    (geos_version()$major > 3 || geos_version()$minor >= 11)) {
  g <- "MULTIPOINT(0 0,0.4 0.5,0 1,1 1,0.6 0.5,1 0)"
  g_concave_hull(g, ratio = 0.5, allow_holes = FALSE, as_wkb = FALSE)
}
#> [1] "POLYGON ((0.4 0.5,0 1,0.6 0.5,1 1,1 0,0 0,0.4 0.5))"

# g_delaunay_triangulation() requires GEOS >= 3.4
if (geos_version()$major > 3 || geos_version()$minor >= 4) {
  g <- "MULTIPOINT(0 0,0 1,1 1,1 0)"
  g_delaunay_triangulation(g, as_wkb = FALSE)
}
#> [1] "GEOMETRYCOLLECTION (POLYGON ((0 1,0 0,1 0,0 1)),POLYGON ((0 1,1 0,1 1,0 1)))"

g <- "LINESTRING(0 0,1 1,10 0)"
g_simplify(g, tolerance = 5, as_wkb = FALSE)
#> [1] "LINESTRING (0 0,10 0)"

# g_unary_union() requires GDAL >= 3.7
if (gdal_version_num() >= gdal_compute_version(3, 7, 0)) {
  g <- "GEOMETRYCOLLECTION(POINT(0.5 0.5), POLYGON((0 0,0 1,1 1,1 0,0 0)),
        POLYGON((1 0,1 1,2 1,2 0,1 0)))"
  g_unary_union(g, as_wkb = FALSE)
}
#> [1] "POLYGON ((0 1,1 1,2 1,2 0,1 0,0 0,0 1))"
```
