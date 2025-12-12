# Create a directory

`vsi_mkdir()` creates a new directory with the indicated mode. For
POSIX-style systems, the mode is modified by the file creation mask
(umask). However, some file systems and platforms may not use umask, or
they may ignore the mode completely. So a reasonable cross-platform
default mode value is `0755`. With `recursive = TRUE`, creates a
directory and all its ancestors. This function is a wrapper for
`VSIMkdir()` and `VSIMkdirRecursive()` in the GDAL Common Portability
Library.

## Usage

``` r
vsi_mkdir(path, mode = "0755", recursive = FALSE)
```

## Arguments

- path:

  Character string. The path to the directory to create.

- mode:

  Character string. The permissions mode in octal with prefix `0`, e.g.,
  `"0755"` (the default).

- recursive:

  Logical scalar. `TRUE` to create the directory and its ancestors.
  Defaults to `FALSE`.

## Value

`0` on success or `-1` on an error.

## See also

[`vsi_read_dir()`](https://firelab.github.io/gdalraster/reference/vsi_read_dir.md),
[`vsi_rmdir()`](https://firelab.github.io/gdalraster/reference/vsi_rmdir.md)

## Examples

``` r
new_dir <- file.path(tempdir(), "newdir")
vsi_mkdir(new_dir)
#> [1] 0
vsi_stat(new_dir, "type")
#> [1] "dir"
vsi_rmdir(new_dir)
#> [1] 0
```
