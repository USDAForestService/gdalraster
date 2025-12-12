# Delete a file

`vsi_unlink()` deletes a file object from the file system. This function
goes through the GDAL `VSIFileHandler` virtualization and may work on
unusual filesystems such as in memory. It is a wrapper for `VSIUnlink()`
in the GDAL Common Portability Library. Analog of the POSIX
[`unlink()`](https://rdrr.io/r/base/unlink.html) function.

## Usage

``` r
vsi_unlink(filename)
```

## Arguments

- filename:

  Character string. The path of the file to be deleted.

## Value

`0` on success or `-1` on an error.

## See also

[`deleteDataset()`](https://firelab.github.io/gdalraster/reference/deleteDataset.md),
[`vsi_rmdir()`](https://firelab.github.io/gdalraster/reference/vsi_rmdir.md),
[`vsi_unlink_batch()`](https://firelab.github.io/gdalraster/reference/vsi_unlink_batch.md)

## Examples

``` r
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
mem_file <- file.path("/vsimem", "tmp.tif")
copyDatasetFiles(mem_file, elev_file)
#> [1] TRUE
vsi_read_dir("/vsimem")
#> [1] "ogr2sqlite"               "tmp.tif"                 
#> [3] "ynp_fires_1984_2022.gpkg"
vsi_unlink(mem_file)
#> [1] 0
vsi_read_dir("/vsimem")
#> [1] "ogr2sqlite"               "ynp_fires_1984_2022.gpkg"
```
