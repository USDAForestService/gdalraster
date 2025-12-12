# Get GEOS version

`geos_version()` returns version information for the GEOS library in use
by GDAL. Requires GDAL \>= 3.4.

## Usage

``` r
geos_version()
```

## Value

A list of length four containing:

- `name` - a string formatted as "major.minor.patch"

- `major` - major version as integer

- `minor` - minor version as integer

- `patch` - patch version as integer

List elements will be `NA` if GDAL \< 3.4.

## See also

[`gdal_version()`](https://firelab.github.io/gdalraster/reference/gdal_version.md),
[`proj_version()`](https://firelab.github.io/gdalraster/reference/proj_version.md)

## Examples

``` r
geos_version()
#> $name
#> [1] "3.12.1"
#> 
#> $major
#> [1] 3
#> 
#> $minor
#> [1] 12
#> 
#> $patch
#> [1] 1
#> 
```
