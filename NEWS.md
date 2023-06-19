# gdalraster 1.2.1 (dev)

* add `fillNodata()`: wrapper for `GDALFillNodata()` in the GDAL Algorithms API (2023-06-18)

* `GDALRaster$info()`: drop `-nomd` argument from the internal call (2023-06-17)

* add `get_cache_used()`: wrapper for `GDALGetCacheUsed64()` with return value in MB (2023-06-16)

* `bbox_from_wkt()`: return `NA` if creation of the geometry object fails (#27) (2023-06-15)

* add `GDALRaster$getOverviewCount()` (2023-06-15)

* documentation: add vignette containing an R port of the GDAL Raster API tutorial (2023-06-15)

* `GDALRaster::read()`: data are now read as R `integer` type when possible for the raster data type (#23) (2023-06-10)

* documentation: add web article on the GDAL block cache and configuration of `GDAL_CACHEMAX` (2023-06-09)

* fix `GDALRaster$getMetadata()`: requesting band-level metadata for a domain other than the default metadata domain was returning dataset-level metadata instead (2023-05-29)

* documentation: add description of the `GDAL_RASTERIO_RESAMPLING` configuration option for `GDALRaster$read()` (2023-05-29)

# gdalraster 1.2.0

* starting at v. 1.2.0, `{gdalraster}` will require R >= 4.2.0

* fix: check for GEOS availability in bbox geometry functions

* fix: wrong array dimensions in `read()` (#5). Starting at v. 1.2.0, `read()` will return vector instead of matrix which better matches the concept of a native GDAL-like interface (thanks to Michael Sumner).

* add: `has_geos()` exported to R

* add: `srs_is_same()` - wrapper for OSRIsSame() in the GDAL Spatial Reference System C API

* documentation - minor edits throughout to improve clarity

# gdalraster 1.1.1

* Initial public release.
