# Returns a signed URL for a supplied VSI filename

`vsi_get_signed_url()` Returns a signed URL of a supplied filename.
Currently only returns a non-NULL value for /vsis3/, /vsigs/, /vsiaz/
and /vsioss/ For example "/vsis3/bucket/filename" will be expanded as
"https://bucket.s3.amazon.com/filename?X-Amz-Algorithm=AWS4-HMAC-SHA256..."
Configuration options that apply for file opening (typically to provide
credentials), and are returned by
[`vsi_get_fs_options()`](https://firelab.github.io/gdalraster/reference/vsi_get_fs_options.md),
are also valid in that context. Wrapper for `VSIGetSignedURL()` in the
GDAL API.

## Usage

``` r
vsi_get_signed_url(filename, options = NULL)
```

## Arguments

- filename:

  Character string containing a /vsiPREFIX/ filename.

- options:

  Character vector of `NAME=VALUE` pairs (see Details).

## Value

Character string containing the signed URL, or `NULL` if `filename` is
not a network-based virtual file system.

## Details

The `options` argument accepts a character vector of name=value pairs.
For /vsis3/, /vsigs/, /vsiaz/ and /vsioss/, the following options are
supported:

- `START_DATE=YYMMDDTHHMMSSZ`: date and time in UTC following ISO 8601
  standard, corresponding to the start of validity of the URL. If not
  specified, current date time.

- `EXPIRATION_DELAY=number_of_seconds`: number between 1 and 604800
  (seven days) for the validity of the signed URL. Defaults to 3600 (one
  hour).

- `VERB=GET/HEAD/DELETE/PUT/POST`: HTTP VERB for which the request will
  be used. Defaults to `GET`.

/vsiaz/ supports additional options:

- `SIGNEDIDENTIFIER=value`: to relate the given shared access signature
  to a corresponding stored access policy.

- `SIGNEDPERMISSIONS=r|w`: permissions associated with the shared access
  signature. Normally deduced from `VERB`.

## See also

[`vsi_get_actual_url()`](https://firelab.github.io/gdalraster/reference/vsi_get_actual_url.md)

## Examples

``` r
if (FALSE) { # \dontrun{
f <- "/vsiaz/items/io-lulc-9-class.parquet"
set_config_option("AZURE_STORAGE_ACCOUNT", "pcstacitems")
# token obtained from:
# https://planetarycomputer.microsoft.com/api/sas/v1/token/pcstacitems/items
set_config_option("AZURE_STORAGE_SAS_TOKEN", "<token>")
vsi_get_actual_url(f)
#> [1] "https://pcstacitems.blob.core.windows.net/items/io-lulc-9-class.parquet"
vsi_get_signed_url(f)
#> [1] "https://pcstacitems.blob.core.windows.net/items/io-lulc-9-class.parquet?<token>"
} # }
```
