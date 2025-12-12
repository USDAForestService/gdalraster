# Copy a whole raster band efficiently

`bandCopyWholeRaster()` copies the complete raster contents of one band
to another similarly configured band. The source and destination bands
must have the same xsize and ysize. The bands do not have to have the
same data type. It implements efficient copying, in particular
"chunking" the copy in substantial blocks. This is a wrapper for
`GDALRasterBandCopyWholeRaster()` in the GDAL API.

## Usage

``` r
bandCopyWholeRaster(
  src_filename,
  src_band,
  dst_filename,
  dst_band,
  options = NULL,
  quiet = FALSE
)
```

## Arguments

- src_filename:

  Filename of the source raster.

- src_band:

  Band number in the source raster to be copied.

- dst_filename:

  Filename of the destination raster.

- dst_band:

  Band number in the destination raster to copy into.

- options:

  Optional list of transfer hints in a vector of `"NAME=VALUE"` pairs.
  The currently supported `options` are:

  - `"COMPRESSED=YES"` to force alignment on target dataset block sizes
    to achieve best compression.

  - `"SKIP_HOLES=YES"` to skip chunks that contain only empty blocks.
    Empty blocks are blocks that are generally not physically present in
    the file, and when read through GDAL, contain only pixels whose
    value is the nodata value when it is set, or whose value is 0 when
    the nodata value is not set. The query is done in an efficient way
    without reading the actual pixel values (if implemented by the
    raster format driver, otherwise will not be skipped).

- quiet:

  Logical scalar. If `TRUE`, a progress bar will not be displayed.
  Defaults to `FALSE`.

## Value

Logical indicating success (invisible `TRUE`). An error is raised if the
operation fails.

## See also

[`GDALRaster-class`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`create()`](https://firelab.github.io/gdalraster/reference/create.md),
[`createCopy()`](https://firelab.github.io/gdalraster/reference/createCopy.md),
[`rasterFromRaster()`](https://firelab.github.io/gdalraster/reference/rasterFromRaster.md)

## Examples

``` r
## copy Landsat data from a single-band file to a new multi-band image
b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
dst_file <- file.path(tempdir(), "sr_multi.tif")
rasterFromRaster(b5_file, dst_file, nbands = 7, init = 0)
#> initializing destination raster...
#> done
opt <- c("COMPRESSED=YES", "SKIP_HOLES=YES")
bandCopyWholeRaster(b5_file, 1, dst_file, 5, options = opt)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
ds <- new(GDALRaster, dst_file)
ds$getStatistics(band = 5, approx_ok = FALSE, force = TRUE)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#> [1]  7357.000 22458.000 13527.736  2238.489
ds$close()
```
