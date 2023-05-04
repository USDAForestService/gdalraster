
<!-- README.md is generated from README.Rmd. Please edit that file -->

# gdalraster

<!-- badges: start -->

<!-- badges: end -->

## Overview

gdalraster is an R interface to the Raster API of the Geospatial Data
Abstraction Library ([GDAL](https://gdal.org/)). Calling signatures
resemble those of the native C, C++ and Python APIs provided by the GDAL
project.

Bindings to GDAL are implemented in class `GDALRaster` along with
several related stand-alone functions. These support:

  - manual creation of uninitialized raster datasets
  - creation from existing raster as template
  - read/set raster dataset parameters
  - low-level I/O
  - virtual raster (VRT) for virtual subsetting, resampling and kernel
    filtering
  - access to `gdalwarp` utility for reprojection

Other functionality in gdalraster supports scalable processing:

  - class `RunningStats` calculates mean and variance in one pass, and
    tracks the min, max, sum, and count (i.e., summary statistics on a
    data stream). The memory usage of a RunningStats object is
    negligible, and input can be intermittent. Scales to large datasets
    for applications such as raster zonal statistics.
  - class `CmbTable` identifies and counts unique combinations of
    integer values using a hash table.
  - `combine()` overlays multiple rasters so that a unique ID is
    assigned to each unique combination of input values. Pixel counts
    for each unique combination are obtained, and combination IDs are
    optionally written to an output raster.

gdalraster is a fast, lightweight and modern R interface to GDAL. It may
be suitable for applications that primarily need low-level raster I/O or
explicit manipulation of VRT format, or prefer native GDAL-like calling.

## Installation

Install from CRAN:

``` r
install.packages("gdalraster")
```

Install the development version from GitHub:

``` r
devtools::install_github("https://github.com/USDAForestService/gdalraster")
```

## Example

``` r
## basic usage of class GDALRaster

library(gdalraster)
#> GDAL version: GDAL 3.3.2, released 2021/09/01

## a FARSITE landscape file from https://www.landfire.gov/viewer/
lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")

ds <- new(GDALRaster, lcp_file, read_only=TRUE)

## print information about the dataset to the console
ds$info()
#> Driver: LCP/FARSITE v.4 Landscape File (.lcp)
#> Files: /tmp/RtmpH2mKEn/temp_libpath76356f6af7e3/gdalraster/extdata/storm_lake.lcp
#>        /tmp/RtmpH2mKEn/temp_libpath76356f6af7e3/gdalraster/extdata/storm_lake.prj
#> Size is 143, 107
#> Coordinate System is:
#> PROJCRS["NAD83 / UTM zone 12N",
#>     BASEGEOGCRS["NAD83",
#>         DATUM["North American Datum 1983",
#>             ELLIPSOID["GRS 1980",6378137,298.257222101,
#>                 LENGTHUNIT["metre",1]],
#>             ID["EPSG",6269]],
#>         PRIMEM["Greenwich",0,
#>             ANGLEUNIT["Degree",0.0174532925199433]]],
#>     CONVERSION["UTM zone 12N",
#>         METHOD["Transverse Mercator",
#>             ID["EPSG",9807]],
#>         PARAMETER["Latitude of natural origin",0,
#>             ANGLEUNIT["Degree",0.0174532925199433],
#>             ID["EPSG",8801]],
#>         PARAMETER["Longitude of natural origin",-111,
#>             ANGLEUNIT["Degree",0.0174532925199433],
#>             ID["EPSG",8802]],
#>         PARAMETER["Scale factor at natural origin",0.9996,
#>             SCALEUNIT["unity",1],
#>             ID["EPSG",8805]],
#>         PARAMETER["False easting",500000,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8806]],
#>         PARAMETER["False northing",0,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8807]],
#>         ID["EPSG",16012]],
#>     CS[Cartesian,2],
#>         AXIS["easting",east,
#>             ORDER[1],
#>             LENGTHUNIT["metre",1,
#>                 ID["EPSG",9001]]],
#>         AXIS["northing",north,
#>             ORDER[2],
#>             LENGTHUNIT["metre",1,
#>                 ID["EPSG",9001]]]]
#> Data axis to CRS axis mapping: 1,2
#> Origin = (323476.071970863151364,5105081.983031376264989)
#> Pixel Size = (30.000000000000000,-30.000000000000000)
#> Corner Coordinates:
#> Upper Left  (  323476.072, 5105081.983) (113d16'58.40"W, 46d 4'35.44"N)
#> Lower Left  (  323476.072, 5101871.983) (113d16'54.12"W, 46d 2'51.51"N)
#> Upper Right (  327766.072, 5105081.983) (113d13'38.83"W, 46d 4'39.38"N)
#> Lower Right (  327766.072, 5101871.983) (113d13'34.65"W, 46d 2'55.45"N)
#> Center      (  325621.072, 5103476.983) (113d15'16.50"W, 46d 3'45.46"N)
#> Band 1 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Elevation
#> Band 2 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Slope
#> Band 3 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Aspect
#> Band 4 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Fuel models
#> Band 5 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Canopy cover
#> Band 6 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Canopy height
#> Band 7 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Canopy base height
#> Band 8 Block=143x1 Type=Int16, ColorInterp=Undefined
#>   Description = Canopy bulk density

## retrieve the raster format name
ds$getDriverShortName()
#> [1] "LCP"
ds$getDriverLongName()
#> [1] "FARSITE v.4 Landscape File (.lcp)"

## retrieve dataset parameters
ds$getRasterXSize()
#> [1] 143
ds$getRasterYSize()
#> [1] 107
ds$getGeoTransform()
#> [1]  323476.1      30.0       0.0 5105082.0       0.0     -30.0
ds$getProjectionRef()
#> [1] "PROJCS[\"NAD83 / UTM zone 12N\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.0174532925199433]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",-111],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH]]"
ds$bbox()
#> [1]  323476.1 5101872.0  327766.1 5105082.0
ds$res()
#> [1] 30 30

## retrieve the number of bands and some band-level parameters
ds$getRasterCount()
#> [1] 8
ds$getBlockSize(band=1)
#> [1] 143   1
ds$getDataTypeName(band=1)
#> [1] "Int16"
ds$getNoDataValue(band=1)
#> [1] NA

## LCP driver reports several dataset- and band-level metadata
## see the format description at https://gdal.org/drivers/raster/lcp.html
## set band=0 to retrieve dataset-level metadata
## set domain="" (empty string) for the default metadata domain
ds$getMetadata(band=0, domain="")
#> [1] "DESCRIPTION=LCP file created by GDAL."
#> [2] "LATITUDE=46"                          
#> [3] "LINEAR_UNIT=Meters"

## retrieve metadata for a band as a vector of name=value pairs
ds$getMetadata(band=4, domain="")
#> [1] "FUEL_MODEL_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM13_220_CONUS/temp/merged_modified.tif"
#> [2] "FUEL_MODEL_MAX=99"                                                                                                                         
#> [3] "FUEL_MODEL_MIN=-9999"                                                                                                                      
#> [4] "FUEL_MODEL_NUM_CLASSES=8"                                                                                                                  
#> [5] "FUEL_MODEL_OPTION=0"                                                                                                                       
#> [6] "FUEL_MODEL_OPTION_DESC=no custom models AND no conversion file needed"                                                                     
#> [7] "FUEL_MODEL_VALUES=0,1,2,5,8,9,10,98,99"

## retrieve the value of a specific metadata item
ds$getMetadataItem(band=2, mdi_name="SLOPE_UNIT_NAME", domain="")
#> [1] "Degrees"

## read one row of pixel values from band 1 (elevation)
## raster row/column index are 0-based
## the upper left corner is the origin
## read the tenth row:
ncols <- ds$getRasterXSize()
rowdata <- ds$read(band=1, xoff=0, yoff=9, 
                    xsize=ncols, ysize=1, 
                    out_xsize=ncols, out_ysize=1)
dim(rowdata)
#> [1]   1 143
head(as.vector(rowdata))
#> [1] -9999 -9999 -9999  2456  2466  2479

ds$close()

## create a new raster using lcp_file as a template
new_file <- paste0(tempdir(), "/", "storml_newdata.tif")
rasterFromRaster(srcfile = lcp_file,
                 dstfile = new_file,
                 nbands = 1,
                 dtName = "Byte",
                 init = -9999)
#> Initializing destination raster...
#> Done.
ds_new <- new(GDALRaster, new_file, read_only=FALSE)

## write random values to all pixels
set.seed(42)
ncols <- ds_new$getRasterXSize()
nrows <- ds_new$getRasterYSize()
for (row in 0:(nrows-1)) {
    rowdata <- round(runif(ncols, 0, 100))
    dim(rowdata) <- c(1, ncols)
    ds_new$write(band=1, xoff=0, yoff=row, xsize=ncols, ysize=1, rowdata)
}

## re-open in read-only mode when done writing
## this will ensure flushing of any pending writes (implicit close())
ds_new$open(read_only=TRUE)

## getStatistics returns min, max, mean, sd, and sets stats in the metadata
ds_new$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#> [1]   0.00000 100.00000  49.90667  29.05491
ds_new$getMetadataItem(band=1, "STATISTICS_MEAN", "")
#> [1] "49.906672766486"

## close the dataset for proper cleanup
ds_new$close()

## using a GDAL Virtual File System handler '/vsicurl/'
## see: https://gdal.org/user/virtual_file_systems.html
web_file <- "/vsicurl/https://raw.githubusercontent.com/django/django/main/tests/gis_tests/data/rasters/raster.tif"

ds_url <- new(GDALRaster, web_file, read_only=TRUE)
ds_url$info()
#> Driver: GTiff/GeoTIFF
#> Files: /vsicurl/https://raw.githubusercontent.com/django/django/main/tests/gis_tests/data/rasters/raster.tif
#> Size is 163, 174
#> Coordinate System is:
#> PROJCRS["NAD83 / Florida GDL Albers",
#>     BASEGEOGCRS["NAD83",
#>         DATUM["North American Datum 1983",
#>             ELLIPSOID["GRS 1980",6378137,298.257222101,
#>                 LENGTHUNIT["metre",1]]],
#>         PRIMEM["Greenwich",0,
#>             ANGLEUNIT["degree",0.0174532925199433]],
#>         ID["EPSG",4269]],
#>     CONVERSION["Florida GDL Albers (meters)",
#>         METHOD["Albers Equal Area",
#>             ID["EPSG",9822]],
#>         PARAMETER["Latitude of false origin",24,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8821]],
#>         PARAMETER["Longitude of false origin",-84,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8822]],
#>         PARAMETER["Latitude of 1st standard parallel",24,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8823]],
#>         PARAMETER["Latitude of 2nd standard parallel",31.5,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8824]],
#>         PARAMETER["Easting at false origin",400000,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8826]],
#>         PARAMETER["Northing at false origin",0,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8827]]],
#>     CS[Cartesian,2],
#>         AXIS["easting (X)",east,
#>             ORDER[1],
#>             LENGTHUNIT["metre",1]],
#>         AXIS["northing (Y)",north,
#>             ORDER[2],
#>             LENGTHUNIT["metre",1]],
#>     USAGE[
#>         SCOPE["State-wide spatial data management."],
#>         AREA["United States (USA) - Florida."],
#>         BBOX[24.41,-87.63,31.01,-79.97]],
#>     ID["EPSG",3086]]
#> Data axis to CRS axis mapping: 1,2
#> Origin = (511700.468070655711927,435103.377123198588379)
#> Pixel Size = (100.000000000000000,-100.000000000000000)
#> Corner Coordinates:
#> Upper Left  (  511700.468,  435103.377) ( 82d51'46.16"W, 27d55' 1.53"N)
#> Lower Left  (  511700.468,  417703.377) ( 82d51'52.04"W, 27d45'37.50"N)
#> Upper Right (  528000.468,  435103.377) ( 82d41'48.81"W, 27d54'56.30"N)
#> Lower Right (  528000.468,  417703.377) ( 82d41'55.54"W, 27d45'32.28"N)
#> Center      (  519850.468,  426403.377) ( 82d46'50.64"W, 27d50'16.99"N)
#> Band 1 Block=163x50 Type=Byte, ColorInterp=Gray
#>   NoData Value=15
```
