# Copy the files of a dataset

`copyDatasetFiles()` copies all the files associated with a dataset.
Wrapper for `GDALCopyDatasetFiles()` in the GDAL API.

## Usage

``` r
copyDatasetFiles(new_filename, old_filename, format = "")
```

## Arguments

- new_filename:

  New name for the dataset (copied to).

- old_filename:

  Old name for the dataset (copied from).

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
[`renameDataset()`](https://usdaforestservice.github.io/gdalraster/reference/renameDataset.md),
[`vsi_copy_file()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_copy_file.md)

## Examples

``` r
lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
ds <- new(GDALRaster, lcp_file)
ds$getFileList()
#> [1] "/home/runner/work/_temp/Library/gdalraster/extdata/storm_lake.lcp"
#> [2] "/home/runner/work/_temp/Library/gdalraster/extdata/storm_lake.prj"
ds$close()

lcp_tmp <- file.path(tempdir(), "storm_lake_copy.lcp")
copyDatasetFiles(lcp_tmp, lcp_file)
#> [1] TRUE
ds_copy <- new(GDALRaster, lcp_tmp)
ds_copy$getFileList()
#> [1] "/tmp/RtmpqKhUt9/storm_lake_copy.lcp" "/tmp/RtmpqKhUt9/storm_lake_copy.prj"
ds_copy$close()

deleteDataset(lcp_tmp)
#> [1] TRUE
```
