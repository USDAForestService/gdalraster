# Return whether the filesystem supports random write

`vsi_supports_rnd_write()` returns whether the filesystem supports
random write. Wrapper for `VSISupportsRandomWrite()` in the GDAL API.

## Usage

``` r
vsi_supports_rnd_write(filename, allow_local_tmpfile)
```

## Arguments

- filename:

  Character string. The path of the filesystem object to be tested.

- allow_local_tmpfile:

  Logical scalar. `TRUE` if the filesystem is allowed to use a local
  temporary file before uploading to the target location.

## Value

Logical scalar. `TRUE` if random write is supported.

## Note

The location GDAL uses for temporary files can be forced via the
`CPL_TMPDIR` configuration option.

## See also

[`vsi_supports_seq_write()`](https://firelab.github.io/gdalraster/reference/vsi_supports_seq_write.md)

## Examples

``` r
# Requires GDAL >= 3.6
if (gdal_version_num() >= gdal_compute_version(3, 6, 0))
  vsi_supports_rnd_write("/vsimem/test-mem-file.gpkg", TRUE)
#> [1] TRUE
```
