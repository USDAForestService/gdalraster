# Returns the actual URL of a supplied VSI filename

`vsi_get_actual_url()` returns the actual URL of a supplied filename.
Currently only returns a non-NULL value for network-based virtual file
systems. For example "/vsis3/bucket/filename" will be expanded as
"https://bucket.s3.amazon.com/filename". Wrapper for `VSIGetActualURL()`
in the GDAL API.

## Usage

``` r
vsi_get_actual_url(filename)
```

## Arguments

- filename:

  Character string containing a /vsiPREFIX/ filename.

## Value

Character string containing the actual URL, or `NULL` if `filename` is
not a network-based virtual file system.

## See also

[`vsi_get_signed_url()`](https://firelab.github.io/gdalraster/reference/vsi_get_signed_url.md)

## Examples

``` r
if (FALSE) { # \dontrun{
f <- "/vsiaz/items/io-lulc-9-class.parquet"
set_config_option("AZURE_STORAGE_ACCOUNT", "pcstacitems")
# token obtained from:
# https://planetarycomputer.microsoft.com/api/sas/v1/token/pcstacitems/items
set_config_option("AZURE_STORAGE_SAS_TOKEN","<token>")
vsi_get_actual_url(f)
#> [1] "https://pcstacitems.blob.core.windows.net/items/io-lulc-9-class.parquet"
vsi_get_signed_url(f)
#> [1] "https://pcstacitems.blob.core.windows.net/items/io-lulc-9-class.parquet?<token>"
} # }
```
