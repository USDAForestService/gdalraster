# Obtain information about a GDAL raster or vector dataset

`inspectDataset()` returns information about the format and content of a
dataset. The function first calls
[`identifyDriver()`](https://firelab.github.io/gdalraster/reference/identifyDriver.md),
and then opens the dataset as raster and/or vector to obtain information
about its content. The return value is a list with named elements.

## Usage

``` r
inspectDataset(filename, ...)
```

## Arguments

- filename:

  Character string containing the name of the file to access. This may
  not refer to a physical file, but instead contain information for the
  driver on how to access a dataset (e.g., connection string, URL, etc.)

- ...:

  Additional arguments passed to
  [`identifyDriver()`](https://firelab.github.io/gdalraster/reference/identifyDriver.md).

## Value

A list with the following named elements:

- `format`: character string, the format short name

- `supports_raster`: logical, `TRUE` if the format supports raster data

- `contains_raster`: logical, `TRUE` if this is a raster dataset or the
  source contains raster subdatasets

- `supports_subdatasets`: logical, `TRUE` if the format supports raster
  subdatasets

- `contains_subdatasets`: logical, `TRUE` if the source contains
  subdatasets

- `subdataset_names`: character vector containing the subdataset names,
  or empty vector if subdatasets are not supported or not present

- `supports_vector`: logical, `TRUE` if the format supports vector data

- `contains_vector`: logical, `TRUE` if the source contains one or more
  vector layers

- `layer_names`: character vector containing the vector layer names, or
  empty vector if the format does not support vector or the source does
  not contain any vector layers

## Note

Subdataset names are the character strings that can be used to
instantiate `GDALRaster` objects. See
https://gdal.org/en/stable/en/latest/user/raster_data_model.html#subdatasets-domain.

PostgreSQL / PostGISRaster are handled as a special case. If additional
arguments `raster` or `vector` are not given for
[`identifyDriver()`](https://firelab.github.io/gdalraster/reference/identifyDriver.md),
then `raster = FALSE` is assumed.

## See also

[`gdal_formats()`](https://firelab.github.io/gdalraster/reference/gdal_formats.md),
[`identifyDriver()`](https://firelab.github.io/gdalraster/reference/identifyDriver.md)

## Examples

``` r
f <- system.file("extdata/ynp_features.zip", package = "gdalraster")
ynp_dsn <- file.path("/vsizip", f, "ynp_features.gpkg")

inspectDataset(ynp_dsn)
#> $format
#> [1] "GPKG"
#> 
#> $supports_raster
#> [1] TRUE
#> 
#> $contains_raster
#> [1] TRUE
#> 
#> $supports_subdatasets
#> [1] TRUE
#> 
#> $contains_subdatasets
#> [1] TRUE
#> 
#> $subdataset_names
#> [1] "GPKG:/vsizip//home/runner/work/_temp/Library/gdalraster/extdata/ynp_features.zip/ynp_features.gpkg:operational_roads"  
#> [2] "GPKG:/vsizip//home/runner/work/_temp/Library/gdalraster/extdata/ynp_features.zip/ynp_features.gpkg:forest_canopy_cover"
#> 
#> $supports_vector
#> [1] TRUE
#> 
#> $contains_vector
#> [1] TRUE
#> 
#> $layer_names
#> [1] "ynp_bnd"            "roads"              "points_of_interest"
#> 
```
