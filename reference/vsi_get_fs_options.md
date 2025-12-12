# Return the list of options associated with a virtual file system handler

`vsi_get_fs_options()` returns the list of options associated with a
virtual file system handler. Those options may be set as configuration
options with
[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md).
Wrapper for `VSIGetFileSystemOptions()` in the GDAL API.

## Usage

``` r
vsi_get_fs_options(filename, as_list = TRUE)
```

## Arguments

- filename:

  Filename, or prefix of a virtual file system handler.

- as_list:

  Logical scalar. If `TRUE` (the default), the XML string returned by
  GDAL will be coerced to list. `FALSE` to return the configuration
  options as a serialized XML string.

## Value

An XML string, or empty string (`""`) if no options are declared. If
`as_list = TRUE` (the default), the XML string will be coerced to list
with `xml2::as_list()`.

## See also

[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md),
[`vsi_get_fs_prefixes()`](https://firelab.github.io/gdalraster/reference/vsi_get_fs_prefixes.md)

<https://gdal.org/en/stable/user/virtual_file_systems.html>

## Examples

``` r
vsi_get_fs_options("/vsimem/")
#> [1] ""

vsi_get_fs_options("/vsizip/")
#> $Options
#> $Options$Option
#> list()
#> attr(,"name")
#> [1] "GDAL_NUM_THREADS"
#> attr(,"type")
#> [1] "string"
#> attr(,"description")
#> [1] "Number of threads for compression. Either a integer or ALL_CPUS"
#> 
#> $Options$Option
#> list()
#> attr(,"name")
#> [1] "CPL_VSIL_DEFLATE_CHUNK_SIZE"
#> attr(,"type")
#> [1] "string"
#> attr(,"description")
#> [1] "Chunk of uncompressed data for parallelization. Use K(ilobytes) or M(egabytes) suffix"
#> attr(,"default")
#> [1] "1M"
#> 
#> 

vsi_get_fs_options("/vsizip/", as_list = FALSE)
#> [1] "<Options>  <Option name='GDAL_NUM_THREADS' type='string' description='Number of threads for compression. Either a integer or ALL_CPUS'/>  <Option name='CPL_VSIL_DEFLATE_CHUNK_SIZE' type='string' description='Chunk of uncompressed data for parallelization. Use K(ilobytes) or M(egabytes) suffix' default='1M'/></Options>"
```
