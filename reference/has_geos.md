# Is GEOS available?

`has_geos()` returns a logical value indicating whether GDAL was built
against the GEOS library. GDAL built with GEOS is a system requirement
as of `gdalraster` 1.10.0, so this function will always return `TRUE`
(may be removed in a future version).

## Usage

``` r
has_geos()
```

## Value

Logical. `TRUE` if GEOS is available, otherwise `FALSE`.

## Examples

``` r
has_geos()
#> [1] TRUE
```
