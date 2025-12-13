# Set the maximum memory size for the GDAL block cache

`set_cache_max()` sets the maximum amount of memory that GDAL is
permitted to use for GDALRasterBlock caching. *The unit of the value to
set is bytes.* Wrapper of `GDALSetCacheMax64()`.

## Usage

``` r
set_cache_max(nbytes)
```

## Arguments

- nbytes:

  A numeric value optionally carrying the `integer64` class attribute
  (assumed to be a whole number, will be coerced to integer by
  truncation). Specifies the new cache size in bytes (maximum number of
  bytes for caching).

## Value

No return value, called for side effects.

## Note

**This function will not make any attempt to check the consistency of
the passed value with the effective capabilities of the OS.**

It is recommended to consult the documentation for
[`get_cache_max()`](https://firelab.github.io/gdalraster/reference/get_cache_max.md)
and
[`get_cache_used()`](https://firelab.github.io/gdalraster/reference/get_cache_used.md)
before using this function.

[`get_cache_max()`](https://firelab.github.io/gdalraster/reference/get_cache_max.md),
[`get_cache_used()`](https://firelab.github.io/gdalraster/reference/get_cache_used.md)

## Examples

``` r
(cachemax <- get_cache_max("bytes"))
#> integer64
#> [1] 838629171

set_cache_max(1e8)
get_cache_max()  # returns in MB by default
#> integer64
#> [1] 100

# reset to original
set_cache_max(cachemax)
get_cache_max()
#> integer64
#> [1] 838
```
