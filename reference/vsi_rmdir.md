# Delete a directory

`vsi_rmdir()` deletes a directory object from the file system. On some
systems the directory must be empty before it can be deleted. With
`recursive = TRUE`, deletes a directory object and its content from the
file system. This function goes through the GDAL `VSIFileHandler`
virtualization and may work on unusual filesystems such as in memory. It
is a wrapper for `VSIRmdir()` and `VSIRmdirRecursive()` in the GDAL
Common Portability Library.

## Usage

``` r
vsi_rmdir(path, recursive = FALSE)
```

## Arguments

- path:

  Character string. The path to the directory to be deleted.

- recursive:

  Logical scalar. `TRUE` to delete the directory and its content.
  Defaults to `FALSE`.

## Value

`0` on success or `-1` on an error.

## Note

/vsis3/ has an efficient implementation for deleting recursively.
Starting with GDAL 3.4, /vsigs/ has an efficient implementation for
deleting recursively, provided that OAuth2 authentication is used.

## See also

[`deleteDataset()`](https://firelab.github.io/gdalraster/reference/deleteDataset.md),
[`vsi_mkdir()`](https://firelab.github.io/gdalraster/reference/vsi_mkdir.md),
[`vsi_read_dir()`](https://firelab.github.io/gdalraster/reference/vsi_read_dir.md),
[`vsi_unlink()`](https://firelab.github.io/gdalraster/reference/vsi_unlink.md)

## Examples

``` r
new_dir <- file.path(tempdir(), "newdir")
vsi_mkdir(new_dir)
#> [1] 0
vsi_rmdir(new_dir)
#> [1] 0
```
