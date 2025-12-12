# Create a new uninitialized raster

`create()` makes an empty raster in the specified format.

## Usage

``` r
create(
  format,
  dst_filename,
  xsize,
  ysize,
  nbands,
  dataType,
  options = NULL,
  return_obj = FALSE
)
```

## Arguments

- format:

  Character string giving a raster format short name (e.g., `"GTiff"`).

- dst_filename:

  Character string giving the filename to create.

- xsize:

  Integer width of raster in pixels.

- ysize:

  Integer height of raster in pixels.

- nbands:

  Integer number of bands.

- dataType:

  Character string containing the data type name. (e.g., common data
  types include Byte, Int16, UInt16, Int32, Float32).

- options:

  Optional list of format-specific creation options in a character
  vector of `"NAME=VALUE"` pairs (e.g., `options = c("COMPRESS=LZW")` to
  set LZW compression during creation of a GTiff file). The
  APPEND_SUBDATASET=YES option can be specified to avoid prior
  destruction of existing dataset.

- return_obj:

  Logical scalar. If `TRUE`, an object of class
  [`GDALRaster`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md)
  opened on the newly created dataset will be returned, otherwise
  returns a logical value. Defaults to `FALSE`.

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
[`createCopy()`](https://firelab.github.io/gdalraster/reference/createCopy.md),
[`getCreationOptions()`](https://firelab.github.io/gdalraster/reference/getCreationOptions.md),
[`rasterFromRaster()`](https://firelab.github.io/gdalraster/reference/rasterFromRaster.md)

## Examples

``` r
new_file <- file.path(tempdir(), "newdata.tif")
ds <- create(format = "GTiff",
             dst_filename = new_file,
             xsize = 143,
             ysize = 107,
             nbands = 1,
             dataType = "Int16",
             return_obj = TRUE)

# EPSG:26912 - NAD83 / UTM zone 12N
ds$setProjection(epsg_to_wkt(26912))
#> [1] TRUE

gt <- c(323476, 30, 0, 5105082, 0, -30)
ds$setGeoTransform(gt)
#> [1] TRUE

ds$setNoDataValue(band = 1, -9999)
#> [1] TRUE
ds$fillRaster(band = 1, -9999, 0)

# ...

# close the dataset when done
ds$close()
```
