# Get metadata on files

`vsi_get_file_metadata()` returns metadata for file system objects.
Implemented for network-like filesystems. Starting with GDAL 3.7,
implemented for /vsizip/ with SOZip metadata. Wrapper of
`VSIGetFileMetadata()` in the GDAL Common Portability Library.

## Usage

``` r
vsi_get_file_metadata(filename, domain)
```

## Arguments

- filename:

  Character string. The path of the file system object to be queried.

- domain:

  Character string. Metadata domain to query. Depends on the file
  system, see Details.

## Value

A named list of values, or `NULL` in case of error or empty list.

## Details

The metadata available depends on the file system. The following are
supported as of GDAL 3.9:

- HEADERS: to get HTTP headers for network-like filesystems (/vsicurl/,
  /vsis3/, /vsgis/, etc).

- TAGS: for /vsis3/, to get S3 Object tagging information. For /vsiaz/,
  to get blob tags.

- STATUS: specific to /vsiadls/: returns all system-defined properties
  for a path (seems in practice to be a subset of HEADERS).

- ACL: specific to /vsiadls/ and /vsigs/: returns the access control
  list for a path. For /vsigs/, a single `XML=xml_content` string is
  returned.

- METADATA: specific to /vsiaz/: blob metadata (this will be a subset of
  what `domain=HEADERS` returns).

- ZIP: specific to /vsizip/: to obtain ZIP specific metadata, in
  particular if a file is SOZIP-enabled (`SOZIP_VALID=YES`).

## See also

[`vsi_stat()`](https://firelab.github.io/gdalraster/reference/vsi_stat.md),
[`addFilesInZip()`](https://firelab.github.io/gdalraster/reference/addFilesInZip.md)

## Examples

``` r
# validate an SOZip-enabled file
# Requires GDAL >= 3.7
f <- system.file("extdata/ynp_features.zip", package = "gdalraster")

zf <- file.path("/vsizip", f)
# files in zip archive
vsi_read_dir(zf)
#> [1] "readme_lf_forest_canopy_cover.txt" "readme_lf_operational_roads.txt"  
#> [3] "readme_ynp_vector_features.txt"    "ynp_bnd_metadata.html"            
#> [5] "ynp_features.gpkg"                

# SOZip metadata for ynp_features.gpkg
zf_gpkg <- file.path(zf, "ynp_features.gpkg")
vsi_get_file_metadata(zf_gpkg, domain = "ZIP")
#> $START_DATA_OFFSET
#> [1] "5188"
#> 
#> $COMPRESSION_METHOD
#> [1] "8 (DEFLATE)"
#> 
#> $COMPRESSED_SIZE
#> [1] "532555"
#> 
#> $UNCOMPRESSED_SIZE
#> [1] "1220608"
#> 
#> $SOZIP_FOUND
#> [1] "YES"
#> 
#> $SOZIP_VERSION
#> [1] "1"
#> 
#> $SOZIP_OFFSET_SIZE
#> [1] "8"
#> 
#> $SOZIP_CHUNK_SIZE
#> [1] "32768"
#> 
#> $SOZIP_START_DATA_OFFSET
#> [1] "537801"
#> 
#> $SOZIP_VALID
#> [1] "YES"
#> 
```
