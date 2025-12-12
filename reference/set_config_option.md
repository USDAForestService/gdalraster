# Set GDAL configuration option

`set_config_option()` sets a GDAL runtime configuration option.
Configuration options are essentially global variables the user can set.
They are used to alter the default behavior of certain raster format
drivers, and in some cases the GDAL core. For a full description and
listing of available options see
<https://gdal.org/en/stable/user/configoptions.html>.

## Usage

``` r
set_config_option(key, value)
```

## Arguments

- key:

  Character name of a configuration option.

- value:

  Character value to set for the option. `value = ""` (empty string)
  will unset a value previously set by `set_config_option()`.

## Value

No return value, called for side effects.

## See also

[`get_config_option()`](https://firelab.github.io/gdalraster/reference/get_config_option.md)

[`vignette("gdal-config-quick-ref")`](https://firelab.github.io/gdalraster/articles/gdal-config-quick-ref.md)

## Examples

``` r
set_config_option("GDAL_CACHEMAX", "10%")
get_config_option("GDAL_CACHEMAX")
#> [1] "10%"
## unset:
set_config_option("GDAL_CACHEMAX", "")
```
