# Check if GDAL CPLHTTP services can be useful (libcurl)

`http_enabled()` returns `TRUE` if `libcurl` support is enabled. Wrapper
of `CPLHTTPEnabled()` in the GDAL Common Portability Library.

## Usage

``` r
http_enabled()
```

## Value

Logical scalar, `TRUE` if GDAL was built with `libcurl` support.

## Examples

``` r
http_enabled()
#> [1] TRUE
```
