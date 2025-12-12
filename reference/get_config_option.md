# Get GDAL configuration option

`get_config_option()` gets the value of GDAL runtime configuration
option. Configuration options are essentially global variables the user
can set. They are used to alter the default behavior of certain raster
format drivers, and in some cases the GDAL core. For a full description
and listing of available options see
<https://gdal.org/en/stable/user/configoptions.html>.

## Usage

``` r
get_config_option(key)
```

## Arguments

- key:

  Character name of a configuration option.

## Value

Character. The value of a (key, value) option previously set with
[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md).
An empty string (`""`) is returned if `key` is not found.

## See also

[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md)

[`vignette("gdal-config-quick-ref")`](https://firelab.github.io/gdalraster/articles/gdal-config-quick-ref.md)

## Examples

``` r
## this option is set during initialization of the gdalraster package
get_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER")
#> [1] "YES"
```
