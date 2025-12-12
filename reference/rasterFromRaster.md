# Create a raster from an existing raster as template

`rasterFromRaster()` creates a new raster with spatial reference, extent
and resolution taken from a template raster, without copying data.
Optionally changes the format, number of bands, data type and nodata
value, sets driver-specific dataset creation options, and initializes to
a value.

## Usage

``` r
rasterFromRaster(
  srcfile,
  dstfile,
  fmt = NULL,
  nbands = NULL,
  dtName = NULL,
  options = NULL,
  init = NULL,
  dstnodata = init
)
```

## Arguments

- srcfile:

  Source raster filename.

- dstfile:

  Output raster filename.

- fmt:

  Output raster format name (e.g., "GTiff" or "HFA"). Will attempt to
  guess from the output filename if `fmt` is not specified.

- nbands:

  Number of output bands.

- dtName:

  Output raster data type name. Commonly used types include `"Byte"`,
  `"Int16"`, `"UInt16"`, `"Int32"` and `"Float32"`.

- options:

  Optional list of format-specific creation options in a vector of
  "NAME=VALUE" pairs (e.g., `options = c("COMPRESS=LZW")` to set LZW
  compression during creation of a GTiff file).

- init:

  Numeric value to initialize all pixels in the output raster.

- dstnodata:

  Numeric nodata value for the output raster.

## Value

Returns the destination filename invisibly.

## See also

[`GDALRaster-class`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`create()`](https://firelab.github.io/gdalraster/reference/create.md),
[`createCopy()`](https://firelab.github.io/gdalraster/reference/createCopy.md),
[`bandCopyWholeRaster()`](https://firelab.github.io/gdalraster/reference/bandCopyWholeRaster.md),
[`translate()`](https://firelab.github.io/gdalraster/reference/translate.md)

## Examples

``` r
# band 2 in a FARSITE landscape file has slope degrees
# convert slope degrees to slope percent in a new raster
lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
ds_lcp <- new(GDALRaster, lcp_file)
ds_lcp$getMetadata(band = 2, domain = "")
#> [1] "SLOPE_FILE=/netapp/sharedwebfs1/shared/landfire/public/temp_q8dTbIJ4w6Qi36Omkzk0/LCP_LF2022_FBFM40_220_CONUS/temp/merged_modified.tif"
#> [2] "SLOPE_MAX=54"                                                                                                                         
#> [3] "SLOPE_MIN=-9999"                                                                                                                      
#> [4] "SLOPE_NUM_CLASSES=53"                                                                                                                 
#> [5] "SLOPE_UNIT=0"                                                                                                                         
#> [6] "SLOPE_UNIT_NAME=Degrees"                                                                                                              

slpp_file <- file.path(tempdir(), "storml_slpp.tif")
opt = c("COMPRESS=LZW")
rasterFromRaster(srcfile = lcp_file,
                 dstfile = slpp_file,
                 nbands = 1,
                 dtName = "Int16",
                 options = opt,
                 init = -32767)
#> initializing destination raster...
#> done
ds_slp <- new(GDALRaster, slpp_file, read_only = FALSE)

# slpp_file is initialized to -32767 and nodata value set
ds_slp$getNoDataValue(band=1)
#> [1] -32767

# extent and cell size are the same as lcp_file
ds_lcp$bbox()
#> [1]  323476.1 5101872.0  327766.1 5105082.0
ds_lcp$res()
#> [1] 30 30
ds_slp$bbox()
#> [1]  323476.1 5101872.0  327766.1 5105082.0
ds_slp$res()
#> [1] 30 30

# convert slope degrees in lcp_file band 2 to slope percent in slpp_file
# bring through LCP nodata -9999 to the output nodata value
ncols <- ds_slp$getRasterXSize()
nrows <- ds_slp$getRasterYSize()
for (row in 0:(nrows-1)) {
    rowdata <- ds_lcp$read(band = 2,
                           xoff = 0, yoff = row,
                           xsize = ncols, ysize = 1,
                           out_xsize = ncols, out_ysize = 1)
    rowslpp <- tan(rowdata*pi/180) * 100
    rowslpp[rowdata==-9999] <- -32767
    dim(rowslpp) <- c(1, ncols)
    ds_slp$write(band = 1, xoff = 0, yoff = row,
                 xsize = ncols, ysize = 1,
                 rasterData = rowslpp)
}

# min, max, mean, sd
ds_slp$getStatistics(band = 1, approx_ok = FALSE, force = TRUE)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
#> [1]   0.00000 138.00000  44.76464  26.79985

ds_slp$close()
ds_lcp$close()
```
