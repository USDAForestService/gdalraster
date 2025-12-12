# Copy a source file to a target filename

`vsi_copy_file()` is a wrapper for `VSICopyFile()` in the GDAL Common
Portability Library. The GDAL VSI functions allow virtualization of disk
I/O so that non file data sources can be made to appear as files. See
<https://gdal.org/en/stable/user/virtual_file_systems.html>. Requires
GDAL \>= 3.7.

## Usage

``` r
vsi_copy_file(src_file, target_file, show_progress = FALSE)
```

## Arguments

- src_file:

  Character string. Filename of the source file.

- target_file:

  Character string. Filename of the target file.

- show_progress:

  Logical scalar. If `TRUE`, a progress bar will be displayed (the size
  of `src_file` will be retrieved in GDAL with `VSIStatL()`). Default is
  `FALSE`.

## Value

`0` on success or `-1` on an error.

## Details

The following copies are made fully on the target server, without local
download from source and upload to target:

- /vsis3/ -\> /vsis3/

- /vsigs/ -\> /vsigs/

- /vsiaz/ -\> /vsiaz/

- /vsiadls/ -\> /vsiadls/

- any of the above or /vsicurl/ -\> /vsiaz/ (starting with GDAL 3.8)

## Note

If `target_file` has the form /vsizip/foo.zip/bar, the default options
described for the function
[`addFilesInZip()`](https://firelab.github.io/gdalraster/reference/addFilesInZip.md)
will be in effect.

## See also

[`copyDatasetFiles()`](https://firelab.github.io/gdalraster/reference/copyDatasetFiles.md),
[`vsi_stat()`](https://firelab.github.io/gdalraster/reference/vsi_stat.md),
[`vsi_sync()`](https://firelab.github.io/gdalraster/reference/vsi_sync.md)

## Examples

``` r
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
tmp_file <- "/vsimem/elev_temp.tif"

# Requires GDAL >= 3.7
if (gdal_version_num() >= gdal_compute_version(3, 7, 0)) {
  result <- vsi_copy_file(elev_file, tmp_file)
  (result == 0)
  print(vsi_stat(tmp_file, "size"))

  vsi_unlink(tmp_file)
}
#> integer64
#> [1] 20043
#> [1] 0
```
