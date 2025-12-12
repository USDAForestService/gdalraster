# Create a copy of a raster

`createCopy()` copies a raster dataset, optionally changing the format.
The extent, cell size, number of bands, data type, projection, and
geotransform are all copied from the source raster.

## Usage

``` r
createCopy(
  format,
  dst_filename,
  src_filename,
  strict = FALSE,
  options = NULL,
  quiet = FALSE,
  return_obj = FALSE
)
```

## Arguments

- format:

  Character string giving the format short name for the output raster
  (e.g., `"GTiff"`).

- dst_filename:

  Character string giving the filename to create.

- src_filename:

  Either a character string giving the filename of the source raster, or
  object of class `GDALRaster` for the source.

- strict:

  Logical. `TRUE` if the copy must be strictly equivalent, or more
  normally `FALSE` (the default) indicating that the copy may adapt as
  needed for the output format.

- options:

  Optional list of format-specific creation options in a vector of
  `"NAME=VALUE"` pairs (e.g., `options = c("COMPRESS=LZW")` to set `LZW`
  compression during creation of a GTiff file). The
  APPEND_SUBDATASET=YES option can be specified to avoid prior
  destruction of existing dataset.

- quiet:

  Logical scalar. If `TRUE`, a progress bar will be not be displayed.
  Defaults to `FALSE`.

- return_obj:

  Logical scalar. If `TRUE`, an object of class
  [`GDALRaster`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md)
  opened on the newly created dataset will be returned. Defaults to
  `FALSE`.

## Value

By default, returns a logical value indicating success (invisible
`TRUE`, output written to `dst_filename`). An error is raised if the
operation fails. An object of class
[`GDALRaster`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md)
open on the output dataset will be returned if `return_obj = TRUE`.

## Note

`dst_filename` may be an empty string (`""`) with `format = "MEM"` and
`return_obj = TRUE` to create an In-memory Raster
(<https://gdal.org/en/stable/drivers/raster/mem.html>).

## See also

[`GDALRaster-class`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`create()`](https://firelab.github.io/gdalraster/reference/create.md),
[`getCreationOptions()`](https://firelab.github.io/gdalraster/reference/getCreationOptions.md),
[`rasterFromRaster()`](https://firelab.github.io/gdalraster/reference/rasterFromRaster.md),
[`translate()`](https://firelab.github.io/gdalraster/reference/translate.md)

## Examples

``` r
lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
tif_file <- file.path(tempdir(), "storml_lndscp.tif")
ds <- createCopy(format = "GTiff",
                 dst_filename = tif_file,
                 src_filename = lcp_file,
                 options = "COMPRESS=LZW",
                 return_obj = TRUE)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.

ds$getMetadata(band = 0, domain = "IMAGE_STRUCTURE")
#> [1] "COMPRESSION=LZW"  "INTERLEAVE=PIXEL"

for (band in 1:ds$getRasterCount())
    ds$setNoDataValue(band, -9999)
ds$getStatistics(band = 1, approx_ok = FALSE, force = TRUE)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#> [1] 2438.0000 3046.0000 2675.9713  133.0185

ds$close()
```
