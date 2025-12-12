# Geometry binary predicates operating on WKB or WKT

These functions implement tests for pairs of geometries in OGC WKB or
WKT format.

## Usage

``` r
g_intersects(this_geom, other_geom, quiet = FALSE)

g_disjoint(this_geom, other_geom, quiet = FALSE)

g_touches(this_geom, other_geom, quiet = FALSE)

g_contains(this_geom, other_geom, quiet = FALSE)

g_within(this_geom, other_geom, quiet = FALSE)

g_crosses(this_geom, other_geom, quiet = FALSE)

g_overlaps(this_geom, other_geom, quiet = FALSE)

g_equals(this_geom, other_geom, quiet = FALSE)
```

## Arguments

- this_geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings.

- other_geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings. Must contain the same
  number of geometries as `this_geom`, unless `this_geom` contains a
  single geometry in which case pairwise tests will be performed for
  one-to-many if `other_geom` contains multiple geometries (i.e.,
  "this-to-others").

- quiet:

  Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.

## Value

Logical vector with length equal to the number of input geometry pairs.

## Details

These functions use the GEOS library via GDAL headers.

`g_intersects()` tests whether two geometries intersect.

`g_disjoint()` tests if this geometry and the other geometry are
disjoint.

`g_touches()` tests if this geometry and the other geometry are
touching.

`g_contains()` tests if this geometry contains the other geometry.

`g_within()` tests if this geometry is within the other geometry.

`g_crosses()` tests if this geometry and the other geometry are
crossing.

`g_overlaps()` tests if this geometry and the other geometry overlap,
that is, their intersection has a non-zero area (they have some but not
all points in common).

`g_equals()` tests whether two geometries are equivalent. The GDAL
documentation says: "This operation implements the SQL/MM
`ST_OrderingEquals()` operation. The comparison is done in a structural
way, that is to say that the geometry types must be identical, as well
as the number and ordering of sub-geometries and vertices. Or
equivalently, two geometries are considered equal by this method if
their WKT/WKB representation is equal. Note: this must be distinguished
from equality in a spatial way."

## Note

`this_geom` and `other_geom` are assumed to be in the same coordinate
reference system.

If `this_geom`is a single geometry and `other_geom` is a list or vector
of multiple geometries, then `this_geom` will be tested against each
geometry in `other_geom` (otherwise no recycling is done).

Geometry validity is not checked. In case you are unsure of the validity
of the input geometries, call
[`g_is_valid()`](https://firelab.github.io/gdalraster/reference/g_query.md)
before, otherwise the result might be wrong.

## See also

<https://en.wikipedia.org/wiki/DE-9IM>
