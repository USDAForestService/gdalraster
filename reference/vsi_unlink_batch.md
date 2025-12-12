# Delete several files in a batch

`vsi_unlink_batch()` deletes a list of files passed in a character
vector. All files should belong to the same file system handler. This is
implemented efficiently for /vsis3/ and /vsigs/ (provided for /vsigs/
that OAuth2 authentication is used). This function is a wrapper for
`VSIUnlinkBatch()` in the GDAL Common Portability Library.

## Usage

``` r
vsi_unlink_batch(filenames)
```

## Arguments

- filenames:

  Character vector. The list of files to delete.

## Value

Logical vector of `length(filenames)` with values depending on the
success of deletion of the corresponding file. `NULL` might be returned
in case of a more general error (for example, files belonging to
different file system handlers).

## See also

[`deleteDataset()`](https://firelab.github.io/gdalraster/reference/deleteDataset.md),
[`vsi_rmdir()`](https://firelab.github.io/gdalraster/reference/vsi_rmdir.md),
[`vsi_unlink()`](https://firelab.github.io/gdalraster/reference/vsi_unlink.md)

## Examples

``` r
# regular file system for illustration
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
tcc_file <- system.file("extdata/storml_tcc.tif", package="gdalraster")

tmp_elev <- file.path(tempdir(), "tmp_elev.tif")
file.copy(elev_file, tmp_elev)
#> [1] TRUE
tmp_tcc <- file.path(tempdir(), "tmp_tcc.tif")
file.copy(tcc_file, tmp_tcc)
#> [1] TRUE
vsi_unlink_batch(c(tmp_elev, tmp_tcc))
#> [1] TRUE TRUE
```
