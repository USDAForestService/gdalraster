# gdalraster 1.2.0

* starting at v. 1.2.0, gdalraster will require R >= 4.2.0

* fix: check for GEOS availability in bbox geom functions

* fix: wrong array dimensions in read() (#5). Starting at v. 1.2.0, read() will return vector instead of matrix which better matches the concept of a native GDAL-like interface (thanks to Michael Sumner).

* add: has_geos() exported to R

* add: srs_is_same() - wrapper for OSRIsSame() in the GDAL Spatial Reference System C API

* documentation - minor edits throughout to improve clarity

# gdalraster 1.1.1

* Initial public release.
