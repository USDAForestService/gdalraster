# Clear path specific configuration options

`vsi_clear_path_options()` clears path specific options previously set
with
[`vsi_set_path_option()`](https://firelab.github.io/gdalraster/reference/vsi_set_path_option.md).
Wrapper for `VSIClearPathSpecificOptions()` in the GDAL Common
Portability Library. Requires GDAL \>= 3.6.

## Usage

``` r
vsi_clear_path_options(path_prefix)
```

## Arguments

- path_prefix:

  Character string. If set to `""` (empty string), all path specific
  options are cleared. If set to a path prefix, only those options set
  with `vsi_set_path_option(path_prefix, ...)` will be cleared.

## Value

No return value, called for side effect.

## Note

No particular care is taken to remove options from RAM in a secure way.

## See also

[`vsi_set_path_option()`](https://firelab.github.io/gdalraster/reference/vsi_set_path_option.md)
