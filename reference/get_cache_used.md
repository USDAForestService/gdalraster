# Get the size of memory in use by the GDAL block cache

`get_cache_used()` returns the amount of memory currently in use for
GDAL block caching. Wrapper of `GDALGetCacheUsed64()` with return value
in MB by default.

## Usage

``` r
get_cache_used(units = "MB")
```

## Arguments

- units:

  Character string specifying units for the return value. One of `"MB"`
  (the default), `"GB"`, `"KB"` or `"bytes"` (values of `"byte"`, `"B"`
  and empty string `""` are also recognized to mean bytes).

## Value

A numeric value carrying the `integer64` class attribute. Amount of the
available cache memory currently in use in the requested units.

## See also

[GDAL Block
Cache](https://firelab.github.io/gdalraster/articles/gdal-block-cache.html)

[`get_cache_max()`](https://firelab.github.io/gdalraster/reference/get_cache_max.md),
[`set_cache_max()`](https://firelab.github.io/gdalraster/reference/set_cache_max.md)

## Examples

``` r
get_cache_used()
#> integer64
#> [1] 0
```
