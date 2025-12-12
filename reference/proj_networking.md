# Check, enable or disable PROJ networking capabilities

`proj_networking()` returns the status of PROJ networking capabilities,
optionally enabling or disabling first. Requires GDAL 3.4 or later and
PROJ 7 or later.

## Usage

``` r
proj_networking(enabled = NULL)
```

## Arguments

- enabled:

  Optional logical scalar. Set to `TRUE` to enable networking
  capabilities or `FALSE` to disable.

## Value

Logical `TRUE` if PROJ networking capabilities are enabled (as indicated
by the return value of `OSRGetPROJEnableNetwork()` in the GDAL Spatial
Reference System C API). Logical `NA` is returned if GDAL \< 3.4.

## See also

[`proj_version()`](https://firelab.github.io/gdalraster/reference/proj_version.md),
[`proj_search_paths()`](https://firelab.github.io/gdalraster/reference/proj_search_paths.md)

[PROJ-data on GitHub](https://github.com/OSGeo/PROJ-data), [PROJ Content
Delivery Network](https://cdn.proj.org/)

## Examples

``` r
proj_networking()
#> [1] FALSE
```
