# Compute a GDAL integer version number from major, minor, revision

`gdal_compute_version()` computes a full integer version number
(GDAL_VERSION_NUM) from individual components (major, minor, revision).
Convenience function for checking a GDAL version requirement using
[`gdal_version_num()`](https://firelab.github.io/gdalraster/reference/gdal_version.md).

## Usage

``` r
gdal_compute_version(maj, min, rev)
```

## Arguments

- maj:

  Numeric value, major version component (coerced to integer by
  truncation).

- min:

  Numeric value, min version component (coerced to integer by
  truncation).

- rev:

  Numeric value, revision version component (coerced to integer by
  truncation).

## Value

Integer version number compatible with
[`gdal_version_num()`](https://firelab.github.io/gdalraster/reference/gdal_version.md).

## See also

[`gdal_version_num()`](https://firelab.github.io/gdalraster/reference/gdal_version.md)

## Examples

``` r
(gdal_version_num() >= gdal_compute_version(3, 7, 0))
#> [1] TRUE
```
