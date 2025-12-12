# Get or set search path(s) for PROJ resource files

`proj_search_paths()` returns the search path(s) for PROJ resource
files, optionally setting them first.

## Usage

``` r
proj_search_paths(paths = NULL)
```

## Arguments

- paths:

  Optional character vector containing one or more directory paths to
  set.

## Value

A character vector containing the currently used search path(s) for PROJ
resource files. An empty string (`""`) is returned if no search paths
are returned by the function `OSRGetPROJSearchPaths()` in the GDAL
Spatial Reference System C API.

## See also

[`proj_version()`](https://firelab.github.io/gdalraster/reference/proj_version.md),
[`proj_networking()`](https://firelab.github.io/gdalraster/reference/proj_networking.md)

## Examples

``` r
proj_search_paths()
#> [1] "/home/runner/.local/share/proj" "/usr/share/proj"               
```
