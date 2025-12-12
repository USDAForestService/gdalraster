# Validate the list of creation options that are handled by a driver

`validateCreationOptions()` is a helper function primarily used by
GDAL's Create() and CreateCopy() to validate that the passed-in list of
creation options is compatible with the GDAL_DMD_CREATIONOPTIONLIST
metadata item defined by some drivers. If the
GDAL_DMD_CREATIONOPTIONLIST metadata item is not defined, this function
will return `TRUE`. Otherwise it will check that the keys and values in
the list of creation options are compatible with the capabilities
declared by the GDAL_DMD_CREATIONOPTIONLIST metadata item. In case of
incompatibility a message will be emitted and `FALSE` will be returned.
Wrapper of `GDALValidateCreationOptions()` in the GDAL API.

## Usage

``` r
validateCreationOptions(format, options)
```

## Arguments

- format:

  Character string giving a format driver short name (e.g., `"GTiff"`).

- options:

  A character vector of format-specific creation options as
  `"NAME=VALUE"` pairs.

## Value

A logical value, `TRUE` if the given creation options are compatible
with the capabilities declared by the GDAL_DMD_CREATIONOPTIONLIST
metadata item for the specified format driver (or if the
GDAL_DMD_CREATIONOPTIONLIST metadata item is not defined for this
driver), otherwise `FALSE`.

## See also

[`getCreationOptions()`](https://firelab.github.io/gdalraster/reference/getCreationOptions.md),
[`create()`](https://firelab.github.io/gdalraster/reference/create.md),
[`createCopy()`](https://firelab.github.io/gdalraster/reference/createCopy.md)

## Examples

``` r
validateCreationOptions("GTiff", c("COMPRESS=LZW", "TILED=YES"))
#> [1] TRUE
```
