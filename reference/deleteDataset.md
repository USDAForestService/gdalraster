# Delete named dataset

`deleteDataset()` will attempt to delete the named dataset in a format
specific fashion. Full featured drivers will delete all associated
files, database objects, or whatever is appropriate. The default
behavior when no format specific behavior is provided is to attempt to
delete all the files that would be returned by
`GDALRaster$getFileList()` on the dataset. The named dataset should not
be open in any existing `GDALRaster` objects when `deleteDataset()` is
called. Wrapper for `GDALDeleteDataset()` in the GDAL API.

## Usage

``` r
deleteDataset(filename, format = "")
```

## Arguments

- filename:

  Filename to delete (should not be open in a `GDALRaster` object).

- format:

  Raster format short name (e.g., "GTiff"). If set to empty string `""`
  (the default), will attempt to guess the raster format from
  `filename`.

## Value

Logical `TRUE` if no error or `FALSE` on failure.

## Note

If `format` is set to an empty string `""` (the default) then the
function will try to identify the driver from `filename`. This is done
internally in GDAL by invoking the `Identify` method of each registered
`GDALDriver` in turn. The first driver that successful identifies the
file name will be returned. An error is raised if a format cannot be
determined from the passed file name.

## See also

[`GDALRaster-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.md),
[`create()`](https://usdaforestservice.github.io/gdalraster/reference/create.md),
[`createCopy()`](https://usdaforestservice.github.io/gdalraster/reference/createCopy.md),
[`copyDatasetFiles()`](https://usdaforestservice.github.io/gdalraster/reference/copyDatasetFiles.md),
[`renameDataset()`](https://usdaforestservice.github.io/gdalraster/reference/renameDataset.md)

## Examples

``` r
b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
b5_tmp <- file.path(tempdir(), "b5_tmp.tif")
file.copy(b5_file,  b5_tmp)
#> [1] TRUE

ds <- new(GDALRaster, b5_tmp)
ds$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
files <- ds$getFileList()
print(files)
#> [1] "/tmp/RtmpD8M3Si/b5_tmp.tif"     "/tmp/RtmpD8M3Si/b5_tmp.tif.ovr"
ds$close()
file.exists(files)
#> [1] TRUE TRUE
deleteDataset(b5_tmp)
#> [1] TRUE
file.exists(files)
#> [1] FALSE FALSE
```
