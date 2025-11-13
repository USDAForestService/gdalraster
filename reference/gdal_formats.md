# Retrieve information on GDAL format drivers for raster and vector

`gdal_formats()` returns a table of the supported raster and vector
formats, with information about the capabilities of each format driver.

## Usage

``` r
gdal_formats(format = "")
```

## Arguments

- format:

  A character string containing a driver short name. By default,
  information for all configured raster and vector format drivers will
  be returned.

## Value

A data frame containing the format short name, long name, raster
(logical), vector (logical), read/write flag (`ro` is read-only, `w`
supports CreateCopy, `w+` supports Create), virtual I/O supported
(logical), and subdatasets (logical).

## Note

Virtual I/O refers to operations on GDAL Virtual File Systems. See
<https://gdal.org/en/stable/user/virtual_file_systems.html#virtual-file-systems>.

## Examples

``` r
nrow(gdal_formats())
#> [1] 214
head(gdal_formats())
#>   short_name raster multidim_raster vector rw_flag virtual_io subdatasets
#> 1        VRT   TRUE            TRUE  FALSE     rw+       TRUE       FALSE
#> 2    DERIVED   TRUE           FALSE  FALSE      ro      FALSE       FALSE
#> 3      GTiff   TRUE           FALSE  FALSE     rw+       TRUE        TRUE
#> 4        COG   TRUE           FALSE  FALSE       w       TRUE       FALSE
#> 5       NITF   TRUE           FALSE  FALSE     rw+       TRUE        TRUE
#> 6     RPFTOC   TRUE           FALSE  FALSE      ro       TRUE        TRUE
#>                                    long_name
#> 1                             Virtual Raster
#> 2 Derived datasets using VRT pixel functions
#> 3                                    GeoTIFF
#> 4          Cloud optimized GeoTIFF generator
#> 5       National Imagery Transmission Format
#> 6           Raster Product Format TOC format

gdal_formats("GPKG")
#>   short_name raster multidim_raster vector rw_flag virtual_io subdatasets
#> 1       GPKG   TRUE           FALSE   TRUE     rw+       TRUE        TRUE
#>    long_name
#> 1 GeoPackage
```
