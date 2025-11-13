# Returns if the file/filesystem is "local".

`vsi_is_local()` returns whether the file/filesystem is "local". Wrapper
for `VSIIsLocal()` in the GDAL API. Requires GDAL \>= 3.6.

## Usage

``` r
vsi_is_local(filename)
```

## Arguments

- filename:

  Character string. The path of the filesystem object to be tested.

## Value

Logical scalar. `TRUE` if if the input file path is local.

## Note

The concept of local is mostly by opposition with a network / remote
file system whose access time can be long.

/vsimem/ is considered to be a local file system, although a
non-persistent one.

## Examples

``` r
# Requires GDAL >= 3.6
if (gdal_version_num() >= gdal_compute_version(3, 6, 0))
  print(vsi_is_local("/vsimem/test-mem-file.tif"))
#> [1] TRUE
```
