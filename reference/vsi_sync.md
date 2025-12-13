# Synchronize a source file/directory with a target file/directory

`vsi_sync()` is a wrapper for `VSISync()` in the GDAL Common Portability
Library. The GDAL documentation is given in Details.

## Usage

``` r
vsi_sync(src, target, show_progress = FALSE, options = NULL)
```

## Arguments

- src:

  Character string. Source file or directory.

- target:

  Character string. Target file or directory.

- show_progress:

  Logical scalar. If `TRUE`, a progress bar will be displayed. Defaults
  to `FALSE`.

- options:

  Character vector of `NAME=VALUE` pairs (see Details).

## Value

Logical scalar, `TRUE` on success or `FALSE` on an error.

## Details

`VSISync()` is an analog of the Linux `rsync` utility. In the current
implementation, `rsync` would be more efficient for local file copying,
but `VSISync()` main interest is when the source or target is a remote
file system like /vsis3/ or /vsigs/, in which case it can take into
account the timestamps of the files (or optionally the ETag/MD5Sum) to
avoid unneeded copy operations. This is only implemented efficiently
for:

- local filesystem \<–\> remote filesystem

- remote filesystem \<–\> remote filesystem (starting with GDAL 3.1)  
  Where the source and target remote filesystems are the same and one of
  /vsis3/, /vsigs/ or /vsiaz/. Or when the target is /vsiaz/ and the
  source is /vsis3/, /vsigs/, /vsiadls/ or /vsicurl/ (starting with GDAL
  3.8)

Similarly to `rsync` behavior, if the source filename ends with a slash,
it means that the content of the directory must be copied, but not the
directory name. For example, assuming "/home/even/foo" contains a file
"bar", `VSISync("/home/even/foo/", "/mnt/media", ...)` will create a
"/mnt/media/bar" file. Whereas
`VSISync("/home/even/foo", "/mnt/media", ...)` will create a
"/mnt/media/foo" directory which contains a bar file.

The `options` argument accepts a character vector of name=value pairs.
Currently accepted options are:  

- `RECURSIVE=NO` (the default is `YES`)

- `SYNC_STRATEGY=TIMESTAMP/ETAG/OVERWRITE`. Determines which criterion
  is used to determine if a target file must be replaced when it already
  exists and has the same file size as the source. Only applies for a
  source or target being a network filesystem. The default is
  `TIMESTAMP` (similarly to how 'aws s3 sync' works), that is to say
  that for an upload operation, a remote file is replaced if it has a
  different size or if it is older than the source. For a download
  operation, a local file is replaced if it has a different size or if
  it is newer than the remote file. The `ETAG` strategy assumes that the
  ETag metadata of the remote file is the MD5Sum of the file content,
  which is only true in the case of /vsis3/ for files not using KMS
  server side encryption and uploaded in a single PUT operation (so
  smaller than 50 MB given the default used by GDAL). Only to be used
  for /vsis3/, /vsigs/ or other filesystems using a MD5Sum as ETAG. The
  `OVERWRITE` strategy (GDAL \>= 3.2) will always overwrite the target
  file with the source one.  

- `NUM_THREADS=integer`. Number of threads to use for parallel file
  copying. Only use for when /vsis3/, /vsigs/, /vsiaz/ or /vsiadls/ is
  in source or target. The default is 10 since GDAL 3.3.  

- `CHUNK_SIZE=integer`. Maximum size of chunk (in bytes) to use to split
  large objects when downloading them from /vsis3/, /vsigs/, /vsiaz/ or
  /vsiadls/ to local file system, or for upload to /vsis3/, /vsiaz/ or
  /vsiadls/ from local file system. Only used if `NUM_THREADS > 1`. For
  upload to /vsis3/, this chunk size must be set at least to 5 MB. The
  default is 8 MB since GDAL 3.3.  

- `x-amz-KEY=value`. (GDAL \>= 3.5) MIME header to pass during creation
  of a /vsis3/ object.  

- `x-goog-KEY=value`. (GDAL \>= 3.5) MIME header to pass during creation
  of a /vsigs/ object.  

- `x-ms-KEY=value`. (GDAL \>= 3.5) MIME header to pass during creation
  of a /vsiaz/ or /vsiadls/ object.

## See also

[`copyDatasetFiles()`](https://firelab.github.io/gdalraster/reference/copyDatasetFiles.md),
[`vsi_copy_file()`](https://firelab.github.io/gdalraster/reference/vsi_copy_file.md)

## Examples

``` r
if (FALSE) { # \dontrun{
# sample-data is a directory in the git repository for gdalraster that is
# not included in the R package:
# https://github.com/firelab/gdalraster/tree/main/sample-data
# A copy of sample-data in an AWS S3 bucket, and a partial copy in an
# Azure Blob container, were used to generate the example below.

src <- "/vsis3/gdalraster-sample-data/"
# s3://gdalraster-sample-data is not public, set credentials
set_config_option("AWS_ACCESS_KEY_ID", "xxxxxxxxxxxxxx")
set_config_option("AWS_SECRET_ACCESS_KEY", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
vsi_read_dir(src)
#> [1] "README.md"
#> [2] "bl_mrbl_ng_jul2004_rgb_720x360.tif"
#> [3] "blue_marble_ng_neo_metadata.xml"
#> [4] "landsat_c2ard_sr_mt_hood_jul2022_utm.json"
#> [5] "landsat_c2ard_sr_mt_hood_jul2022_utm.tif"
#> [6] "lf_elev_220_metadata.html"
#> [7] "lf_elev_220_mt_hood_utm.tif"
#> [8] "lf_fbfm40_220_metadata.html"
#> [9] "lf_fbfm40_220_mt_hood_utm.tif"

dst <- "/vsiaz/sampledata"
set_config_option("AZURE_STORAGE_CONNECTION_STRING",
                  "<connection_string_for_gdalraster_account>")
vsi_read_dir(dst)
#> [1] "lf_elev_220_metadata.html"   "lf_elev_220_mt_hood_utm.tif"

# GDAL VSISync() supports direct copy for /vsis3/ -> /vsiaz/ (GDAL >= 3.8)
result <- vsi_sync(src, dst, show_progress = TRUE)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
print(result)
#> [1] TRUE
vsi_read_dir(dst)
#> [1] "README.md"
#> [2] "bl_mrbl_ng_jul2004_rgb_720x360.tif"
#> [3] "blue_marble_ng_neo_metadata.xml"
#> [4] "landsat_c2ard_sr_mt_hood_jul2022_utm.json"
#> [5] "landsat_c2ard_sr_mt_hood_jul2022_utm.tif"
#> [6] "lf_elev_220_metadata.html"
#> [7] "lf_elev_220_mt_hood_utm.tif"
#> [8] "lf_fbfm40_220_metadata.html"
#> [9] "lf_fbfm40_220_mt_hood_utm.tif"
} # }
```
