# gdalraster 1.4.1 (dev)

* `plot_raster()`: a legend can now be used with a color table for continuous data. Default value of the `legend` argument has been changed to `FALSE` (2023-09-17)

* add `CmbTable$asMatrix()`: returns the table of combinations as a numeric matrix with named columns (alternative to `$asDataFrame()`) (2023-09-15)

* add `CmbTable$updateFromMatrixByRow()`: class method to update the hash table from a matrix with combinations arranged in rows (2023-09-15)

* add `bandCopyWholeRaster()`: efficiently copy a whole raster band, wrapper for `GDALRasterBandCopyWholeRaster()` in the GDAL API (2023-09-14)

* add `GDALRaster$flushCache()`: flush all write cached data to disk (2023-09-13)

* add `GDALRaster$infoAsJSON()`: returns output of the `gdalinfo` command-line utility as a JSON-formatted string (2023-09-13)

* add `createColorRamp()`: wrapper for `GDALCreateColorRamp()` in the GDAL Raster API. Automatically creates a ramp from one color to another (2023-09-13)

* add `GDALRaster$getRasterColorInterp()` and `GDALRaster$setRasterColorInterp()`: get/set color interpretation for raster bands (2023-09-11)

* add support for color tables in class `GDALRaster`: `GDALRaster$getColorTable()`, `GDALRaster$getPaletteInterp()`, `GDALRaster$setColorTable()` (2023-09-10)

* `plot_raster()`: `col_tbl` can have an optional alpha channel; add argument `maxColorValue` (2023-09-09)

* `bbox_from_wkt()`, `bbox_to_wkt()`: add arguments `extend_x`, `extend_y` (2023-09-07)

* add `GDALRaster$getDescription()` and `GDALRaster$setDescription()`: get/set description for raster band objects (2023-09-06)

* add `g_buffer()`: compute buffer of a WKT geometry (2023-09-05)

* add `sieveFilter()`: wrapper for `GDALSieveFilter()` in the GDAL Algorithms API (2023-09-04)

* add PROJ utility functions: `proj_version()`, `proj_search_paths()`, `proj_networking()` (2023-09-03)

* add `GDALRaster$getFileList()`: returns a list of files forming the dataset (2023-09-01)

* on Windows, reset environment variables on package unload (2023-08-31)

# gdalraster 1.4.0

* add `dem_proc()`: wrapper for the `gdaldem` command-line utility to generate DEM derivatives

* add the following set methods in class `GDALRaster`: `setMetadataItem()`, `setUnitType()`, `setScale()`, `setOffset()`

* add `GDALRaster$buildOverviews()`: build raster overviews

* add `GDALRaster$dim()`: returns a vector of xsize, ysize, nbands

* `transform_xy()` and `inv_project()`: `pts` can be a data frame or matrix

* `plot_raster()` now accepts a `GDALRaster` object for the `data` argument

* `plot_raster()`: make the legend narrower and add argument `digits` to format legend labels when raster data are floating point

* add test suite and code coverage report

# gdalraster 1.3.0

* `GDALRaster::read()`: data are now read as R `integer` type when possible for the raster data type (#23)

* add `fillNodata()`: wrapper for `GDALFillNodata()` in the GDAL Algorithms API

* add `read_ds()`: convenience wrapper for `GDALRaster$read()`

* add `plot_raster()`: display raster data using base R graphics

* add `get_cache_used()`: wrapper for `GDALGetCacheUsed64()` with return value in MB

* add `GDALRaster$getOverviewCount()`: return the number of overview layers available

* `GDALRaster$info()`: drop `-nomd` argument from the internal call

* `bbox_from_wkt()`: return `NA` if creation of the geometry object fails (#27)

* fix `GDALRaster$getMetadata()`: requesting band-level metadata for a domain other than the default metadata domain was returning dataset-level metadata instead

* add vignette containing an [R port of the GDAL Raster API tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html)

* add description of the `GDAL_RASTERIO_RESAMPLING` configuration option in the documentation for `GDALRaster$read()`

* add web article on the [GDAL block cache](https://usdaforestservice.github.io/gdalraster/articles/gdal-block-cache.html) and configuration of `GDAL_CACHEMAX`

# gdalraster 1.2.0

* starting at v. 1.2.0, `{gdalraster}` will require R >= 4.2.0

* fix: check for GEOS availability in bbox geometry functions

* fix: wrong array dimensions in `read()` (#5). Starting at v. 1.2.0, `read()` will return vector instead of matrix which better matches the concept of a native GDAL-like interface (thanks to Michael Sumner).

* add: `has_geos()` exported to R

* add: `srs_is_same()` - wrapper for OSRIsSame() in the GDAL Spatial Reference System C API

* documentation - minor edits throughout to improve clarity

# gdalraster 1.1.1

* Initial public release.