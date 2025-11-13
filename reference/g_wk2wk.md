# Geometry WKB/WKT conversion

`g_wk2wk()` converts geometries between Well Known Binary (WKB) and Well
Known Text (WKT) formats. A geometry given as a raw vector of WKB will
be converted to a WKT string, while a geometry given as a WKT string
will be converted to a WKB raw vector. Input may also be a list of WKB
raw vectors or a character vector of WKT strings.

## Usage

``` r
g_wk2wk(geom, as_iso = FALSE, byte_order = "LSB")
```

## Arguments

- geom:

  Either a raw vector of WKB or list of raw vectors to convert to WKT,
  or a character vector containing one or more WKT strings to convert to
  WKB.

- as_iso:

  Logical value, `TRUE` to export as ISO WKB/WKT (ISO 13249 SQL/MM Part
  3), or `FALSE` (the default) to export as "Extended WKB/WKT" (see
  Note).

- byte_order:

  Character string specifying the byte order when converting to WKB. One
  of `"LSB"` (the default) or `"MSB"` (uncommon).

## Value

For input of a WKB raw vector or list of raw vectors, returns a
character vector of WKT strings, with length of the returned vector
equal to the number of input raw vectors. For input of a single WKT
string, returns a raw vector of WKB. For input of a character vector
containing more than one WKT string, returns a list of WKB raw vectors,
with length of the returned list equal to the number of input strings.

## Note

With `as_iso = FALSE` (the default), geometries are exported as extended
dimension (Z) WKB/WKT for types `Point`, `LineString`, `Polygon`,
`MultiPoint`, `MultiLineString`, `MultiPolygon` and
`GeometryCollection`. For other geometry types, it is equivalent to ISO.

When the return value is a list of WKB raw vectors, an element in the
returned list will contain `NULL` (and a warning emitted) if the
corresponding input string was `NA` or empty (`""`).

When input is a list of WKB raw vectors, a corresponding element in the
returned character vector will be `NA` if the input was a raw vector of
length `0` (i.e., `raw(0)`). If an input list element is not a raw
vector, then the corresponding element in the returned character vector
will also be `NA`. A warning is emitted in each case.

## See also

GEOS reference for geometry formats:  
<https://libgeos.org/specifications/>

## Examples

``` r
wkt <- "POINT (-114 47)"
wkb <- g_wk2wk(wkt)
str(wkb)
#>  raw [1:21] 01 01 00 00 ...
g_wk2wk(wkb)
#> [1] "POINT (-114 47)"
```
