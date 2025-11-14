# Return free disk space available on the filesystem

`vsi_get_disk_free_space()` returns the free disk space available on the
filesystem. Wrapper for `VSIGetDiskFreeSpace()` in the GDAL Common
Portability Library.

## Usage

``` r
vsi_get_disk_free_space(path)
```

## Arguments

- path:

  Character string. A directory of the filesystem to query.

## Value

Numeric scalar. The free space in bytes (as
[`bit64::integer64`](https://rdrr.io/pkg/bit64/man/bit64-package.html)
type), or `-1` in case of error.

## Examples

``` r
tmp_dir <- file.path("/vsimem", "tmpdir")
vsi_mkdir(tmp_dir)
#> [1] 0
vsi_get_disk_free_space(tmp_dir)
#> integer64
#> [1] 16772579328
vsi_rmdir(tmp_dir)
#> [1] 0
```
