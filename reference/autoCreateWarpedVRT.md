# Create a virtual warped dataset automatically

`autoCreateWarpedVRT()` creates a warped virtual dataset representing
the input raster warped into a target coordinate system. The output
virtual dataset will be "north-up" in the target coordinate system. GDAL
automatically determines the bounds and resolution of the output virtual
raster which should be large enough to include all the input raster.
Wrapper of `GDALAutoCreateWarpedVRT()` in the GDAL Warper API.

## Usage

``` r
autoCreateWarpedVRT(
  src_ds,
  dst_wkt,
  resample_alg,
  src_wkt = "",
  max_err = 0,
  alpha_band = FALSE
)
```

## Arguments

- src_ds:

  An object of class `GDALRaster` for the source dataset.

- dst_wkt:

  WKT string specifying the coordinate system to convert to. If empty
  string (`""`) no change of coordinate system will take place.

- resample_alg:

  Character string specifying the sampling method to use. One of
  NearestNeighbour, Bilinear, Cubic, CubicSpline, Lanczos, Average, RMS
  or Mode.

- src_wkt:

  WKT string specifying the coordinate system of the source raster. If
  empty string it will be read from the source raster (the default).

- max_err:

  Numeric scalar specifying the maximum error measured in input pixels
  that is allowed in approximating the transformation (`0.0` for exact
  calculations, the default).

- alpha_band:

  Logical scalar, `TRUE` to create an alpha band if the source dataset
  has none. Defaults to `FALSE`.

## Value

An object of class `GDALRaster` for the new virtual dataset. An error is
raised if the operation fails.

## Note

The returned dataset will have no associated filename for itself. If you
want to write the virtual dataset to a VRT file, use the
`$setFilename()` method on the returned `GDALRaster` object to assign a
filename before it is closed.

## Examples

``` r
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, elev_file)

ds2 <- autoCreateWarpedVRT(ds, epsg_to_wkt(5070), "Bilinear")
ds2$info()
#> Driver: VRT/Virtual Raster
#> Files: /home/runner/work/_temp/Library/gdalraster/extdata/storml_elev.tif
#> Size is 158, 127
#> Coordinate System is:
#> PROJCRS["NAD83 / Conus Albers",
#>     BASEGEOGCRS["NAD83",
#>         DATUM["North American Datum 1983",
#>             ELLIPSOID["GRS 1980",6378137,298.257222101,
#>                 LENGTHUNIT["metre",1]]],
#>         PRIMEM["Greenwich",0,
#>             ANGLEUNIT["degree",0.0174532925199433]],
#>         ID["EPSG",4269]],
#>     CONVERSION["unnamed",
#>         METHOD["Albers Equal Area",
#>             ID["EPSG",9822]],
#>         PARAMETER["Latitude of false origin",23,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8821]],
#>         PARAMETER["Longitude of false origin",-96,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8822]],
#>         PARAMETER["Latitude of 1st standard parallel",29.5,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8823]],
#>         PARAMETER["Latitude of 2nd standard parallel",45.5,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8824]],
#>         PARAMETER["Easting at false origin",0,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8826]],
#>         PARAMETER["Northing at false origin",0,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8827]]],
#>     CS[Cartesian,2],
#>         AXIS["easting",east,
#>             ORDER[1],
#>             LENGTHUNIT["metre",1]],
#>         AXIS["northing",north,
#>             ORDER[2],
#>             LENGTHUNIT["metre",1]],
#>     ID["EPSG",5070]]
#> Data axis to CRS axis mapping: 1,2
#> Origin = (-1332272.514894899679348,2686786.510187031701207)
#> Pixel Size = (30.017093608756117,-30.017093608756117)
#> Corner Coordinates:
#> Upper Left  (-1332272.515, 2686786.510) (113d17'20.73"W, 46d 4'32.57"N)
#> Lower Left  (-1332272.515, 2682974.339) (113d16'48.70"W, 46d 2'30.94"N)
#> Upper Right (-1327529.814, 2686786.510) (113d13'44.00"W, 46d 5' 0.36"N)
#> Lower Right (-1327529.814, 2682974.339) (113d13'12.07"W, 46d 2'58.72"N)
#> Center      (-1329901.164, 2684880.425) (113d15'16.38"W, 46d 3'45.66"N)
#> Band 1 Block=158x127 Type=Int32, ColorInterp=Gray
#>   NoData Value=32767
#>   Metadata:
#>     RepresentationType=ATHEMATIC

## set filename before close if a VRT file is needed for the virtual dataset
# ds2$setFilename("/path/to/file.vrt")

ds2$close()
ds$close()
```
