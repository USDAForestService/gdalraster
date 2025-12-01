# Convert raster data between different formats

`translate()` is a wrapper of the `gdal_translate` command-line utility
(see <https://gdal.org/en/stable/programs/gdal_translate.html>). The
function can be used to convert raster data between different formats,
potentially performing some operations like subsetting, resampling, and
rescaling pixels in the process. Refer to the GDAL documentation at the
URL above for a list of command-line arguments that can be passed in
`cl_arg`.

## Usage

``` r
translate(src_filename, dst_filename, cl_arg = NULL, quiet = FALSE)
```

## Arguments

- src_filename:

  Either a character string giving the filename of the source raster, or
  an object of class `GDALRaster` for the source.

- dst_filename:

  Character string. Filename of the output raster.

- cl_arg:

  Optional character vector of command-line arguments for
  `gdal_translate` (see URL above).

- quiet:

  Logical scalar. If `TRUE`, a progress bar will not be displayed.
  Defaults to `FALSE`.

## Value

Logical indicating success (invisible `TRUE`). An error is raised if the
operation fails.

## See also

[`GDALRaster-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.md),
[`rasterFromRaster()`](https://usdaforestservice.github.io/gdalraster/reference/rasterFromRaster.md),
[`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md)

[`ogr2ogr()`](https://usdaforestservice.github.io/gdalraster/reference/ogr2ogr.md)
for vector data

## Examples

``` r
# convert the elevation raster to Erdas Imagine format and resample to 90m
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
img_file <- file.path(tempdir(), "storml_elev_90m.img")

# command-line arguments for gdal_translate
args <- c("-tr", "90", "90", "-r", "average")
args <- c(args, "-of", "HFA", "-co", "COMPRESSED=YES")

translate(elev_file, img_file, args)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.

ds <- new(GDALRaster, img_file)
ds$info()
#> Driver: HFA/Erdas Imagine Images (.img)
#> Files: /tmp/Rtmp5DKaZ2/storml_elev_90m.img
#>        /tmp/Rtmp5DKaZ2/storml_elev_90m.img.aux.xml
#> Size is 48, 36
#> Coordinate System is:
#> PROJCRS["NAD83 / UTM zone 12N",
#>     BASEGEOGCRS["NAD83",
#>         DATUM["North American Datum 1983",
#>             ELLIPSOID["GRS 1980",6378137,298.257222101,
#>                 LENGTHUNIT["metre",1]]],
#>         PRIMEM["Greenwich",0,
#>             ANGLEUNIT["degree",0.0174532925199433]],
#>         ID["EPSG",4269]],
#>     CONVERSION["UTM zone 12N",
#>         METHOD["Transverse Mercator",
#>             ID["EPSG",9807]],
#>         PARAMETER["Latitude of natural origin",0,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8801]],
#>         PARAMETER["Longitude of natural origin",-111,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8802]],
#>         PARAMETER["Scale factor at natural origin",0.9996,
#>             SCALEUNIT["unity",1],
#>             ID["EPSG",8805]],
#>         PARAMETER["False easting",500000,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8806]],
#>         PARAMETER["False northing",0,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8807]]],
#>     CS[Cartesian,2],
#>         AXIS["(E)",east,
#>             ORDER[1],
#>             LENGTHUNIT["meters",1]],
#>         AXIS["(N)",north,
#>             ORDER[2],
#>             LENGTHUNIT["meters",1]],
#>     ID["EPSG",26912]]
#> Data axis to CRS axis mapping: 1,2
#> Origin = (323476.071970862976741,5105081.983031379990280)
#> Pixel Size = (90.000000000000000,-90.000000000000000)
#> Metadata:
#>   AREA_OR_POINT=Area
#> Corner Coordinates:
#> Upper Left  (  323476.072, 5105081.983) (113d16'58.40"W, 46d 4'35.44"N)
#> Lower Left  (  323476.072, 5101841.983) (113d16'54.08"W, 46d 2'50.54"N)
#> Upper Right (  327796.072, 5105081.983) (113d13'37.43"W, 46d 4'39.40"N)
#> Lower Right (  327796.072, 5101841.983) (113d13'33.21"W, 46d 2'54.50"N)
#> Center      (  325636.072, 5103461.983) (113d15'15.78"W, 46d 3'44.98"N)
#> Band 1 Block=64x64 Type=Int16, ColorInterp=Undefined
#>   Description = Layer_1
#>   NoData Value=32767
#>   Metadata:
#>     LAYER_TYPE=athematic
#>     RepresentationType=ATHEMATIC
#>   Image Structure Metadata:
#>     COMPRESSION=RLE
#> 

ds$close()
```
