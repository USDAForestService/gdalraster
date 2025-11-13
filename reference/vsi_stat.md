# Get filesystem object info

`vsi_stat()` fetches status information about a filesystem object (file,
directory, etc). This function goes through the GDAL `VSIFileHandler`
virtualization and may work on unusual filesystems such as in memory. It
is a wrapper for `VSIStatExL()` in the GDAL Common Portability Library.
Analog of the POSIX `stat()` function.

## Usage

``` r
vsi_stat(filename, info = "exists")
```

## Arguments

- filename:

  Character string. The path of the filesystem object to be queried.

- info:

  Character string. The type of information to fetch, one of `"exists"`
  (the default), `"type"` or `"size"`.

## Value

If `info = "exists"`, returns logical `TRUE` if the file system object
exists, otherwise `FALSE`. If `info = "type"`, returns a character
string with one of `"file"` (regular file), `"dir"` (directory),
`"symlink"` (symbolic link), or empty string (`""`). If `info = "size"`,
returns the file size in bytes (as
[`bit64::integer64`](https://rdrr.io/pkg/bit64/man/bit64-package.html)
type), or `-1` if an error occurs.

## Note

For portability, `vsi_stat()` supports a subset of `stat()`-type
information for filesystem objects. This function is primarily intended
for use with GDAL virtual file systems (e.g., URLs, cloud storage
systems, ZIP/GZip/7z/RAR archives, in-memory files). The base R function
[`utils::file_test()`](https://rdrr.io/r/utils/filetest.html) could be
used instead for file tests on regular local filesystems.

## See also

GDAL Virtual File Systems:  
<https://gdal.org/en/stable/user/virtual_file_systems.html>

## Examples

``` r
data_dir <- system.file("extdata", package="gdalraster")
vsi_stat(data_dir)
#> [1] TRUE
vsi_stat(data_dir, "type")
#> [1] "dir"
# stat() on a directory doesn't return the sum of the file sizes in it,
# but rather how much space is used by the directory entry
vsi_stat(data_dir, "size")
#> integer64
#> [1] 4096

elev_file <- file.path(data_dir, "storml_elev.tif")
vsi_stat(elev_file)
#> [1] TRUE
vsi_stat(elev_file, "type")
#> [1] "file"
vsi_stat(elev_file, "size")
#> integer64
#> [1] 20043

nonexistent <- file.path(data_dir, "nonexistent.tif")
vsi_stat(nonexistent)
#> [1] FALSE
vsi_stat(nonexistent, "type")
#> [1] "unknown"
vsi_stat(nonexistent, "size")
#> integer64
#> [1] -1

# /vsicurl/ file system handler
base_url <- "https://raw.githubusercontent.com/usdaforestservice/"
f <- "gdalraster/main/sample-data/landsat_c2ard_sr_mt_hood_jul2022_utm.tif"
url_file <- paste0("/vsicurl/", base_url, f)

# try to be CRAN-compliant for the example:
set_config_option("GDAL_HTTP_CONNECTTIMEOUT", "10")
set_config_option("GDAL_HTTP_TIMEOUT", "10")

vsi_stat(url_file)
#> [1] TRUE
vsi_stat(url_file, "type")
#> [1] "file"
vsi_stat(url_file, "size")
#> integer64
#> [1] 13067777
```
