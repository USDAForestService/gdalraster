# Get the maximum memory size available for the GDAL block cache

`get_cache_max()` returns the maximum amount of memory available to the
GDALRasterBlock caching system for caching raster read/write data.
Wrapper of `GDALGetCacheMax64()` with return value in MB by default.

## Usage

``` r
get_cache_max(units = "MB")
```

## Arguments

- units:

  Character string specifying units for the return value. One of `"MB"`
  (the default), `"GB"`, `"KB"` or `"bytes"` (values of `"byte"`, `"B"`
  and empty string `""` are also recognized to mean bytes).

## Value

A numeric value carrying the `integer64` class attribute. Maximum cache
memory available in the requested units.

## Details

The first time this function is called, it will read the `GDAL_CACHEMAX`
configuration option to initialize the maximum cache memory. The value
of the configuration option can be expressed as x% of the usable
physical RAM (which may potentially be used by other processes).
Otherwise it is expected to be a value in MB. As of GDAL 3.10, the
default value, if `GDAL_CACHEMAX` has not been set explicitly, is 5% of
usable physical RAM.

## Note

The value of the `GDAL_CACHEMAX` configuration option is only consulted
the first time the cache size is requested (i.e., it must be set as a
configuration option prior to any raster I/O during the current
session). To change this value programmatically during operation of the
program it is better to use
[`set_cache_max()`](https://firelab.github.io/gdalraster/reference/set_cache_max.md)
(in which case, always given in bytes).

## See also

[GDAL_CACHEMAX configuration
option](https://gdal.org/en/stable/user/configoptions.html#performance-and-caching)

[`get_config_option()`](https://firelab.github.io/gdalraster/reference/get_config_option.md),
[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md),
[`get_usable_physical_ram()`](https://firelab.github.io/gdalraster/reference/get_usable_physical_ram.md),
[`get_cache_used()`](https://firelab.github.io/gdalraster/reference/get_cache_used.md),
[`set_cache_max()`](https://firelab.github.io/gdalraster/reference/set_cache_max.md)

## Examples

``` r
get_cache_max()
#> integer64
#> [1] 838
```
