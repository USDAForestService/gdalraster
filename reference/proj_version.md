# Get PROJ version

`proj_version()` returns version information for the PROJ library in use
by GDAL.

## Usage

``` r
proj_version()
```

## Value

A list of length four containing:

- `name` - a string formatted as "major.minor.patch"

- `major` - major version as integer

- `minor` - minor version as integer

- `patch` - patch version as integer

## See also

[`gdal_version()`](https://firelab.github.io/gdalraster/reference/gdal_version.md),
[`geos_version()`](https://firelab.github.io/gdalraster/reference/geos_version.md),
[`proj_search_paths()`](https://firelab.github.io/gdalraster/reference/proj_search_paths.md),
[`proj_networking()`](https://firelab.github.io/gdalraster/reference/proj_networking.md)

## Examples

``` r
proj_version()
#> $name
#> [1] "9.4.0"
#> 
#> $major
#> [1] 9
#> 
#> $minor
#> [1] 4
#> 
#> $patch
#> [1] 0
#> 
```
