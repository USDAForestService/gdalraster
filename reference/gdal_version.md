# Get GDAL version

`gdal_version()` returns a character vector of GDAL runtime version
information. `gdal_version_num()` returns only the full version number
(`gdal_version()[2]`) as an integer value.

## Usage

``` r
gdal_version()

gdal_version_num()
```

## Value

`gdal_version()` returns a character vector of length four containing:

- "-version" - one line version message, e.g., "GDAL 3.6.3, released
  2023/03/12"

- "GDAL_VERSION_NUM" - formatted as a string, e.g., "3060300" for GDAL
  3.6.3.0

- "GDAL_RELEASE_DATE" - formatted as a string, e.g., "20230312"

- "GDAL_RELEASE_NAME" - e.g., "3.6.3"

`gdal_version_num()` returns `as.integer(gdal_version()[2])`

## Examples

``` r
gdal_version()
#> [1] "GDAL 3.8.4, released 2024/02/08" "3080400"                        
#> [3] "20240208"                        "3.8.4"                          

gdal_version_num()
#> [1] 3080400
```
