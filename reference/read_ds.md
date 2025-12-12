# Convenience wrapper for `GDALRaster$read()`

`read_ds()` will read from a raster dataset that is already open in a
`GDALRaster` object. By default, it attempts to read the full raster
extent from all bands at full resolution. `read_ds()` is sometimes more
convenient than `GDALRaster$read()`, e.g., to read specific multiple
bands for display with
[`plot_raster()`](https://firelab.github.io/gdalraster/reference/plot_raster.md),
or simply for the default arguments that read an entire raster into
memory (see Note).

## Usage

``` r
read_ds(
  ds,
  bands = NULL,
  xoff = 0,
  yoff = 0,
  xsize = ds$getRasterXSize(),
  ysize = ds$getRasterYSize(),
  out_xsize = xsize,
  out_ysize = ysize,
  as_list = FALSE,
  as_raw = FALSE
)
```

## Arguments

- ds:

  An object of class `GDALRaster` in open state.

- bands:

  Integer vector of band numbers to read. By default all bands will be
  read.

- xoff:

  Integer. The pixel (column) offset to the top left corner of the
  raster region to be read (zero to start from the left side).

- yoff:

  Integer. The line (row) offset to the top left corner of the raster
  region to be read (zero to start from the top).

- xsize:

  Integer. The width in pixels of the region to be read.

- ysize:

  Integer. The height in pixels of the region to be read.

- out_xsize:

  Integer. The width in pixels of the output buffer into which the
  desired region will be read (e.g., to read a reduced resolution
  overview).

- out_ysize:

  Integer. The height in pixels of the output buffer into which the
  desired region will be read (e.g., to read a reduced resolution
  overview).

- as_list:

  Logical. If `TRUE`, return output as a list of band vectors. If
  `FALSE` (the default), output is a vector of pixel data interleaved by
  band.

- as_raw:

  Logical. If `TRUE` and the underlying data type is Byte, return output
  as R `raw` vector type. This maps to the setting `$readByteAsRaw` on
  the `GDALRaster` object, which will be temporarily updated in this
  function. To control this behavior in a persistent way on a dataset
  see `$readByteAsRaw` in
  [`GDALRaster-class`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md).

## Value

If `as_list = FALSE` (the default), a vector of `raw`, `integer`,
`double` or `complex` containing the values that were read. It is
organized in left to right, top to bottom pixel order, interleaved by
band. If `as_list = TRUE`, a list with number of elements equal to the
number of bands read. Each element contains a vector of `raw`,
`integer`, `double` or `complex` containing the pixel values that were
read for the band.

## Details

`NA` will be returned in place of the nodata value if the raster dataset
has a nodata value defined for the band. Data are read as R `integer`
type when possible for the raster data type (Byte, Int8, Int16, UInt16,
Int32), otherwise as type `double` (UInt32, Float32, Float64).

The output object has attribute `gis`, a list containing:

      $type = "raster"
      $bbox = c(xmin, ymin, xmax, ymax)
      $dim = c(xsize, ysize, nbands)
      $srs = <projection as WKT2 string>
      $datatype = <character vector of data type name by band>

The WKT version used for the projection string can be overridden by
setting the `OSR_WKT_FORMAT` configuration option. See
[`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md)
for a list of supported values.

## Note

There is small overhead in calling `read_ds()` compared with calling
`GDALRaster$read()` directly. This would only matter if calling the
function repeatedly to read a raster in chunks. For the case of reading
a large raster in many chunks, it will be optimal performance-wise to
call `GDALRaster$read()` directly.

By default, this function will attempt to read the full raster into
memory. It generally should not be called on large raster datasets using
the default argument values. The memory size in bytes of the returned
vector will be, e.g., (xsize \* ysize \* number of bands \* 4) for data
read as `integer`, or (xsize \* ysize \* number of bands \* 8) for data
read as `double` (plus small object overhead for the vector).

## See also

[`GDALRaster$read()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md)

## Examples

``` r
# read three bands from a multi-band dataset
lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
ds <- new(GDALRaster, lcp_file)

# as a vector of pixel data interleaved by band
r <- read_ds(ds, bands = c(6,5,4))
typeof(r)
#> [1] "integer"
length(r)
#> [1] 45903
object.size(r)
#> 185776 bytes

# as a list of band vectors
r <- read_ds(ds, bands = c(6,5,4), as_list = TRUE)
typeof(r)
#> [1] "list"
length(r)
#> [1] 3
object.size(r)
#> 185960 bytes

# gis attributes
attr(r, "gis")
#> $type
#> [1] "raster"
#> 
#> $bbox
#> [1]  323476.1 5101872.0  327766.1 5105082.0
#> 
#> $dim
#> [1] 143 107   3
#> 
#> $srs
#> [1] "PROJCRS[\"NAD83 / UTM zone 12N\",BASEGEOGCRS[\"NAD83\",DATUM[\"North American Datum 1983\",ELLIPSOID[\"GRS 1980\",6378137,298.257222101,LENGTHUNIT[\"metre\",1]],ID[\"EPSG\",6269]],PRIMEM[\"Greenwich\",0,ANGLEUNIT[\"Degree\",0.0174532925199433]]],CONVERSION[\"UTM zone 12N\",METHOD[\"Transverse Mercator\",ID[\"EPSG\",9807]],PARAMETER[\"Latitude of natural origin\",0,ANGLEUNIT[\"Degree\",0.0174532925199433],ID[\"EPSG\",8801]],PARAMETER[\"Longitude of natural origin\",-111,ANGLEUNIT[\"Degree\",0.0174532925199433],ID[\"EPSG\",8802]],PARAMETER[\"Scale factor at natural origin\",0.9996,SCALEUNIT[\"unity\",1],ID[\"EPSG\",8805]],PARAMETER[\"False easting\",500000,LENGTHUNIT[\"metre\",1],ID[\"EPSG\",8806]],PARAMETER[\"False northing\",0,LENGTHUNIT[\"metre\",1],ID[\"EPSG\",8807]],ID[\"EPSG\",16012]],CS[Cartesian,2],AXIS[\"(E)\",east,ORDER[1],LENGTHUNIT[\"metre\",1,ID[\"EPSG\",9001]]],AXIS[\"(N)\",north,ORDER[2],LENGTHUNIT[\"metre\",1,ID[\"EPSG\",9001]]]]"
#> 
#> $datatype
#> [1] "Int16" "Int16" "Int16"
#> 

ds$close()
```
