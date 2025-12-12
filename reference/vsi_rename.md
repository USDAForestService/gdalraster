# Rename a file

`vsi_rename()` renames a file object in the file system. The GDAL
documentation states it should be possible to rename a file onto a new
filesystem, but it is safest if this function is only used to rename
files that remain in the same directory. This function goes through the
GDAL `VSIFileHandler` virtualization and may work on unusual filesystems
such as in memory. It is a wrapper for `VSIRename()` in the GDAL Common
Portability Library. Analog of the POSIX `rename()` function.

## Usage

``` r
vsi_rename(oldpath, newpath)
```

## Arguments

- oldpath:

  Character string. The name of the file to be renamed.

- newpath:

  Character string. The name the file should be given.

## Value

`0` on success or `-1` on an error.

## See also

[`renameDataset()`](https://firelab.github.io/gdalraster/reference/renameDataset.md),
[`vsi_copy_file()`](https://firelab.github.io/gdalraster/reference/vsi_copy_file.md)

## Examples

``` r
# regular file system for illustration
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
tmp_file <- tempfile(fileext = ".tif")
file.copy(elev_file, tmp_file)
#> [1] TRUE
new_file <- file.path(dirname(tmp_file), "storml_elev_copy.tif")
vsi_rename(tmp_file, new_file)
#> [1] 0
vsi_stat(new_file)
#> [1] TRUE
vsi_unlink(new_file)
#> [1] 0
```
