# gdalraster 1.7.0

## GDAL API stand-alone functions

* add `polygonize()`: create a polygon feature layer from raster data, wrapper for `GDALPolygonize` in the GDAL Algorithms API.
* add `rasterize()`: burn vector geometries (points, lines, or polygons) into a raster, wrapper for the `gdal_rasterize` command-line utility
* add `buildVRT()`: build a GDAL virtual raster mosaic from a list of datasets, wrapper for the `gdalbuildvrt` command-line utility
* add `translate()`: convert raster data between different formats, wrapper for the `gdal_translate` command-line utility
* make `t_srs` optional in `warp()`
* the GDAL configuration option `TRADITIONAL_GIS_ORDER=OSR_DEFAULT_AXIS_MAPPING_STRATEGY` is now set on package load

## Documentation

* add [GDAL Config Quick Reference](https://usdaforestservice.github.io/gdalraster/articles/gdal-config-quick-ref.html) to vignettes
* `warp()` has additional documentation covering several processing options
* add the `COMPRESS_OVERVIEW` configuration option to the documentation for `GDALRaster::buildOverviews()`
* add `str()` of the `GDALRaster` object in the [Raster API Tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html)
* add display of the EVT raster itself along with its attribute table in [Raster Attribute Tables](https://usdaforestservice.github.io/gdalraster/articles/raster-attribute-tables.html)

## Internal

* add src/ogr_util.cpp and src/ogr_util.h: OGR utility functions for vector data sources
* src/gdal_exp.cpp: additional error handling in several wrapper functions, increased test coverage

# gdalraster 1.6.0

## Dependencies

* GDAL >= 2.4.0 (previously >= 2.3.0)
* package [**xml2**](https://CRAN.R-project.org/package=xml2) is now required (previously in Suggests)
* package [**gt**](https://CRAN.R-project.org/package=gt) has been added to Suggests (required for `displayRAT()`)

## GDALRaster-class

* add methods for Raster Attribute Tables: `getDefaultRAT()`, `setDefaultRAT()`
* add `getDefaultHistogram()`: fetch default raster histogram for a band
* add `getHistogram()`: compute raster histogram for a band
* add `getMinMax()`: compute min/max for a raster band
* add `getMetadataDomainList()`: get a list of metadata domains for a dataset or raster band
* fix `getMetadataItem()` for a specific domain at dataset level (#109)

## GDAL API stand-alone functions

* add `buildRAT()`: compute for a raster band the set of unique pixel values and their counts, and build a GDAL Raster Attribute Table as data frame
* add `displayRAT()`: generate a presentation Raster Attribute Table, showing colors if the table contains RGB columns
* add `gdal_formats()`: report the supported raster formats
* add `getCreationOptions()`: get the list of creation options of a raster format
* add `copyDatasetFiles()`: copy all the files associated with a dataset
* add `deleteDataset()`: delete a dataset in a format-specific way
* add `renameDataset()`: rename a dataset in a format-specific way
* add some missing error checks in src/gdal_exp.cpp (#104)

## CmbTable-class

* argument `incr` for the count increment in `CmbTable::update()` can be zero

## RunningStats-class

* use `uint64_t` for the count accumulator (previously `long long`) and make explicit the return cast in `get_count()` (no user-visible changes)
* slightly faster update

## Other miscellaneous

* `plot_raster()`: normalize legend correctly for `minmax_def` and `minmax_pct_cut` (#131)

## Documentation

* add vignette [Raster Attribute Tables](https://usdaforestservice.github.io/gdalraster/articles/raster-attribute-tables.html)
* add [notes](https://usdaforestservice.github.io/gdalraster/reference/RunningStats-class.html#note) for `RunningStats-class`
* update vignette [Raster API Tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html) with `gdal_formats()` and `getCreationOptions()`
* update installation instructions in [README](https://github.com/USDAForestService/gdalraster/blob/main/README.md)


# gdalraster 1.5.0

## GDALRaster-class

* add methods for color tables: `getColorTable()`, `getPaletteInterp()`, `setColorTable()`
* add `getRasterColorInterp()`, `setRasterColorInterp()`: get/set color interpretation for raster bands
* add `getDescription()`, `setDescription()`: get/set description for raster band objects
* add `flushCache()`: flush all write cached data to disk
* add `getFileList()`: returns a list of files forming the dataset
* add `infoAsJSON()`: returns output of the `gdalinfo` command-line utility as a JSON-formatted string
* `new()`: add a warning in the class constructor if the raster has an int64 data type (would be handled as double for now)

## GDAL API stand-alone functions

* add `bandCopyWholeRaster()`: wrapper for `GDALRasterBandCopyWholeRaster()`, efficiently copy a whole raster band
* add `createColorRamp()`: wrapper for `GDALCreateColorRamp()`, automatically create a ramp from one color to another
* add `sieveFilter()`: wrapper for `GDALSieveFilter()` in the Algorithms API, remove small raster polygons
* add PROJ utility functions: `proj_version()`, `proj_search_paths()`, `proj_networking()` (via GDAL headers)
* add `g_buffer()`: compute buffer of a WKT geometry (GEOS convenience function via GDAL headers)

## CmbTable-class

* add `updateFromMatrixByRow()`: update the hash table from a matrix having integer combinations arranged in rows
* add `asMatrix()`: return the combinations table as a numeric matrix (alternative to `asDataFrame()`)

## Bug fix

* `warp()` caused segfault if proj.db could not be found (#96)

## Other miscellaneous

* `plot_raster()`: default value of the `legend` argument has been changed to `FALSE`; legend can now use a color table for continuous data; add argument `maxColorValue` (e.g., to use RGB 0:255 instead of 0:1 in `col_tbl`)
* `bbox_from_wkt()`, `bbox_to_wkt()`: add arguments `extend_x`, `extend_y`
* on Windows, reset GDAL environment variables on package unload if they were previously set on load
* add inst/extdata/storml_tcc.tif: example dataset of NLCD Tree Canopy Cover
* update the package vignette for color tables and raster display


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
