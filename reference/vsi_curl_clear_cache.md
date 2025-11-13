# Clean cache associated with /vsicurl/ and related file systems

`vsi_curl_clear_cache()` cleans the local cache associated with
/vsicurl/ (and related file systems). This function is a wrapper for
`VSICurlClearCache()` and `VSICurlPartialClearCache()` in the GDAL
Common Portability Library. See Details for the GDAL documentation.

## Usage

``` r
vsi_curl_clear_cache(partial = FALSE, file_prefix = "", quiet = TRUE)
```

## Arguments

- partial:

  Logical scalar. Whether to clear the cache only for a given filename
  (see Details).

- file_prefix:

  Character string. Filename prefix to use if `partial = TRUE`.

- quiet:

  Logical scalar. `TRUE` (the default) to wrap the API call in a quiet
  error handler, or `FALSE` to print any potential error messages to the
  console.

## Value

No return value, called for side effects.

## Details

/vsicurl/ (and related file systems like /vsis3/, /vsigs/, /vsiaz/,
/vsioss/, /vsiswift/) cache a number of metadata and data for faster
execution in read-only scenarios. But when the content on the
server-side may change during the same process, those mechanisms can
prevent opening new files, or give an outdated version of them. If
`partial = TRUE`, cleans the local cache associated for a given filename
(and its subfiles and subdirectories if it is a directory).

## Examples

``` r
vsi_curl_clear_cache()
```
