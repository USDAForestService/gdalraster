# Rename a dataset

`renameDataset()` renames a dataset in a format-specific way (e.g.,
rename associated files as appropriate). This could include moving the
dataset to a new directory or even a new filesystem. The dataset should
not be open in any existing `GDALRaster` objects when `renameDataset()`
is called. Wrapper for `GDALRenameDataset()` in the GDAL API.

## Usage

``` r
renameDataset(new_filename, old_filename, format = "")
```

## Arguments

- new_filename:

  New name for the dataset.

- old_filename:

  Old name for the dataset (should not be open in a `GDALRaster`
  object).

- format:

  Raster format short name (e.g., "GTiff"). If set to empty string `""`
  (the default), will attempt to guess the raster format from
  `old_filename`.

## Value

Logical `TRUE` if no error or `FALSE` on failure.

## Note

If `format` is set to an empty string `""` (the default) then the
function will try to identify the driver from `old_filename`. This is
done internally in GDAL by invoking the `Identify` method of each
registered `GDALDriver` in turn. The first driver that successful
identifies the file name will be returned. An error is raised if a
format cannot be determined from the passed file name.

## See also

[`GDALRaster-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.md),
[`create()`](https://usdaforestservice.github.io/gdalraster/reference/create.md),
[`createCopy()`](https://usdaforestservice.github.io/gdalraster/reference/createCopy.md),
[`deleteDataset()`](https://usdaforestservice.github.io/gdalraster/reference/deleteDataset.md),
[`copyDatasetFiles()`](https://usdaforestservice.github.io/gdalraster/reference/copyDatasetFiles.md)

## Examples

``` r
b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
b5_tmp <- file.path(tempdir(), "b5_tmp.tif")
file.copy(b5_file,  b5_tmp)
#> [1] TRUE

ds <- new(GDALRaster, b5_tmp)
ds$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
ds$getFileList()
#> [1] "/tmp/RtmpXPamOH/b5_tmp.tif"     "/tmp/RtmpXPamOH/b5_tmp.tif.ovr"
ds$close()
b5_tmp2 <- file.path(tempdir(), "b5_tmp_renamed.tif")
renameDataset(b5_tmp2, b5_tmp)
#> [1] TRUE
ds <- new(GDALRaster, b5_tmp2)
ds$getFileList()
#> [1] "/tmp/RtmpXPamOH/b5_tmp_renamed.tif"    
#> [2] "/tmp/RtmpXPamOH/b5_tmp_renamed.tif.ovr"
ds$close()

deleteDataset(b5_tmp2)
#> [1] TRUE
```
