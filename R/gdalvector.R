#' @name GDALVector-class
#'
#' @aliases
#' Rcpp_GDALVector Rcpp_GDALVector-class GDALVector
#'
#' @title Class encapsulating a vector layer in a GDAL dataset
#'
#' @description
#' `GDALVector` provides an interface for accessing a vector layer in a GDAL
#' dataset and calling methods on the underlying `OGRLayer` object.
#' An object of class `GDALVector` persists an open connection to the dataset,
#' and exposes methods for retrieving layer information, setting attribute and
#' spatial filters, and reading/writing feature data.
#' See \url{https://gdal.org/en/stable/api/index.html} for details of the GDAL
#' Vector API.
#'
#' **Class `GDALVector` is currently under development**. An initial
#' implementation supporting read access was added in gdalraster 1.11.1.9100.
#' A working document with draft specifications is available at:\cr
#' \url{https://usdaforestservice.github.io/gdalraster/articles/gdalvector-draft.html}\cr
#' and discussion thread/status updates at:\cr
#' \url{https://github.com/USDAForestService/gdalraster/issues/241}.
#'
#' @param dsn Character string containing the data source name (DSN), usually a
#' filename or database connection string.
#' @param layer Character string containing the name of a layer within the
#' data source. May also be given as an SQL SELECT statement to be executed
#' against the data source, defining a layer as the result set.
#' @param read_only Logical scalar. `TRUE` to open the layer read-only (the
#' default), or `FALSE` to open with write access.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#' specifying dataset open options.
#' @param spatial_filter Optional character string containing a geometry in
#' Well Known Text (WKT) format which represents a spatial filter.
#' @param dialect Optional character string to control the statement dialect
#' when SQL is used to define the layer. By default, the OGR SQL engine will
#' be used, except for RDBMS drivers that will use their dedicated SQL engine,
#' unless `"OGRSQL"` is explicitly passed as the dialect. The `"SQLITE"`
#' dialect can also be used.
#' @returns An object of class `GDALVector` which contains pointers to the
#' opened layer and the dataset that owns it, and methods that operate on
#' the layer as described in Details. `GDALVector` is a C++ class exposed
#' directly to R (via `RCPP_EXPOSED_CLASS`). Fields and methods of the class
#' are accessed using the `$` operator. Note that all arguments to exposed
#' class methods are required (but do not have to be named). The read/write
#' fields are per-object settings which can be changed as needed during the
#' lifetime of the object.
#'
#' @section Usage (see Details):
#' \preformatted{
#' ## Constructors
#' # for single-layer file formats such as shapefile
#' lyr <- new(GDALVector, dsn)
#' # specifying the layer name, or SQL statement defining the layer
#' lyr <- new(GDALVector, dsn, layer)
#' # for update access
#' lyr <- new(GDALVector, dsn, layer, read_only = FALSE)
#' # using dataset open options
#' lyr <- new(GDALVector, dsn, layer, read_only, open_options)
#' # setting a spatial filter and/or specifying the SQL dialect
#' lyr <- new(GDALVector, dsn, layer, read_only, open_options, spatial_filter, dialect)
#'
#' ## Read/write fields (per-object settings)
#' lyr$defaultGeomColName
#' lyr$promoteToMulti
#' lyr$quiet
#' lyr$returnGeomAs
#' lyr$wkbByteOrder
#'
#' ## Methods
#' lyr$open(read_only)
#' lyr$isOpen()
#' lyr$getDsn()
#' lyr$getFileList()
#' lyr$getDriverShortName()
#' lyr$getDriverLongName()
#'
#' lyr$getName()
#' lyr$getFieldNames()
#' lyr$testCapability()
#' lyr$getFIDColumn()
#' lyr$getGeomType()
#' lyr$getGeometryColumn()
#' lyr$getSpatialRef()
#' lyr$bbox()
#' lyr$getLayerDefn()
#' lyr$getFieldDomain(domain_name)
#'
#' lyr$setAttributeFilter(query)
#' lyr$getAttributeFilter()
#' lyr$setIgnoredFields(fields)
#' lyr$setSelectedFields(fields)
#'
#' lyr$setSpatialFilter(wkt)
#' lyr$setSpatialFilterRect(bbox)
#' lyr$getSpatialFilter()
#' lyr$clearSpatialFilter()
#'
#' lyr$getFeatureCount()
#' lyr$getNextFeature()
#' lyr$setNextByIndex(i)
#' lyr$getFeature(fid)
#' lyr$resetReading()
#' lyr$fetch(n)
#'
#' lyr$setFeature(feature)
#' lyr$createFeature(feature)
#' lyr$upsertFeature(feature)
#' lyr$getLastWriteFID()
#' lyr$deleteFeature(fid)
#' lyr$syncToDisk()
#'
#' lyr$startTransaction(force)
#' lyr$commitTransaction()
#' lyr$rollbackTransaction()
#'
#' lyr$getMetadata()
#' lyr$setMetadata(metadata)
#' lyr$getMetadataItem(mdi_name)
#'
#' lyr$close()
#' }
#' @section Details:
#' ## Constructors
#'
#' \code{new(GDALVector, dsn)}\cr
#' The first layer by index is assumed if the `layer` argument is omitted, so
#' this form of the constructor might be used for single-layer formats like
#' shapefile.
#'
#' \code{new(GDALVector, dsn, layer)}\cr
#' Constructor specifying the name of a layer to open. The `layer` argument
#' may also be given as an SQL SELECT statement to define a layer as the result
#' set.
#'
#' \code{new(GDALVector, dsn, layer, read_only)}\cr
#' Constructor specifying read/write access (`read_only = {TRUE|FALSE})`.
#' The `layer` argument is required in this form of the constructor, but may be
#' given as empty string (`""`), in which case the first layer by index will be
#' assumed.
#'
#' \code{new(GDALVector, dsn, layer, read_only, open_options)}\cr
#' Constructor specifying dataset open options as a character vector of
#' `NAME=VALUE` pairs.
#'
#' \code{new(GDALVector, dsn, layer, read_only, open_options, spatial_filter, dialect))}\cr
#' Constructor to specify a spatial filter and/or SQL dialect. All arguments
#' are required in this form of the constructor, but `open_options` may be
#' `NULL`, and `spatial_filter` or `dialect` may be an empty string (`""`).
#'
#' ## Read/write fields
#'
#' \code{$defaultGeomColName}\cr
#' Character string specifying a name to use for returned columns when the
#' geometry column name in the source layer is empty, like with shapefiles etc.
#' Defaults to `"geometry"`.
#'
#' \code{$promoteToMulti}\cr
#' A logical value specifying whether to automatically promote geometries from
#' Polygon to MultiPolygon, Point to MultiPoint, or LineString to
#' MultiLineString during read operations (i.e., with methods `$getFeature()`,
#' `$getNextFeature()`, `$fetch()`). Defaults to `FALSE`. Setting to `TRUE` may
#' be useful when reading from layers such as shapefiles that mix, e.g.,
#' Polygons and MultiPolygons.
#'
#' \code{$quiet}\cr
#' A logical value, `FALSE` by default. Set to `TRUE` to suppress various
#' messages and warnings.
#'
#' \code{$returnGeomAs}\cr
#' Character string specifying the return format of feature geometries.
#' Must be one of `WKB` (the default), `WKB_ISO`, `WKT`, `WKT_ISO`, `BBOX`, or
#' `NONE`.
#' Using `WKB`/`WKT` exports as 99-402 extended dimension (Z) types for Point,
#' LineString, Polygon, MultiPoint, MultiLineString, MultiPolygon and
#' GeometryCollection. For other geometry types, it is equivalent to using
#' `WKB_ISO`/`WKT_ISO` (see \url{https://libgeos.org/specifications/wkb/}).
#' Using `BBOX` exports as a list of numeric vectors, each of length 4 with values
#' `xmin, ymin, xmax, ymax`. If an empty geometry is encountered these values will
#' be `NA_real_` in the corresponding location.
#' Using `NONE` will result in no geometry value being present in the feature
#' returned.
#'
#' \code{$wkbByteOrder}\cr
#' Character string specifying the byte order for WKB geometries.
#' Must be either `LSB` (Least Significant Byte first, the default) or
#' `MSB` (Most Significant Byte first).
#'
#' ## Methods
#'
#' \code{$open(read_only)}\cr
#' (Re-)opens the vector layer on the existing DSN. Use this method to
#' open a layer that has been closed using \code{$close()}. May be used to
#' re-open a layer with a different read/write access (`read_only` set to
#' `TRUE` or `FALSE`). The method will first close an open dataset, so it is
#' not required to call \code{$close()} explicitly in this case.
#' No return value, called for side effects.
#'
#' \code{$isOpen()}\cr
#' Returns a `logical` scalar indicating whether the vector dataset is open.
#'
#' \code{$getDsn()}\cr
#' Returns a character string containing the `dsn` associated with this
#' `GDALVector` object (`dsn` originally used to open the layer).
#'
#' \code{$getFileList()}\cr
#' Returns a character vector of files believed to be part of the data source.
#' If it returns an empty string (`""`) it means there is believed to be no
#' local file system files associated with the dataset (e.g., a virtual file
#' system). The returned filenames will normally be relative or absolute
#' paths depending on the path used to originally open the dataset.
#'
#' \code{$getDriverShortName()}\cr
#' Returns the short name of the vector format driver.
#'
#' \code{$getDriverLongName()}\cr
#' Returns the long name of the vector format driver.
#'
#' \code{$getName()}\cr
#' Returns the layer name.
#'
#' \code{$getFieldNames()}\cr
#' Returns a character vector of the layer's field names.
#'
#' \code{$testCapability()}\cr
#' Tests whether the layer supports named capabilities based on the current
#' read/write access. Returns a list of capabilities with values `TRUE` or
#' `FALSE`. The returned list contains the following named elements:
#' `RandomRead`, `SequentialWrite`, `RandomWrite`, `UpsertFeature`,
#' `FastSpatialFilter`, `FastFeatureCount`, `FastGetExtent`,
#' `FastSetNextByIndex`, `CreateField`, `CreateGeomField`, `DeleteField`,
#' `ReorderFields`, `AlterFieldDefn`, `AlterGeomFieldDefn`, `DeleteFeature`,
#' `StringsAsUTF8`, `Transactions`, `CurveGeometries`.
#' (See the GDAL documentation for
#' [`OGR_L_TestCapability()`](https://gdal.org/en/stable/api/vector_c_api.html#_CPPv420OGR_L_TestCapability9OGRLayerHPKc).)
#'
#' \code{$getFIDColumn()}\cr
#' Returns the name of the underlying database column being used as the FID
#' column, or empty string (`""`) if not supported.
#'
#' \code{$getGeomType()}\cr
#' Returns the well known name of the layer geometry type as character string.
#' For layers with multiple geometry fields, this method only returns the
#' geometry type of the first geometry column. For other columns, use
#' `$getLayerDefn()`. For layers without any geometry field, this method
#' returns `"NONE"`.
#'
#' \code{$getGeometryColumn()}\cr
#' Returns he name of the underlying database column being used as the geometry
#' column, or an empty string (`""`) if not supported.
#' For layers with multiple geometry fields, this method only returns the
#' name of the first geometry column. For other columns, use `$getLayerDefn()`.
#'
#' \code{$getSpatialRef()}\cr
#' Returns a WKT string containing the spatial reference system for this layer,
#' or empty string (`""`) if no spatial reference exists.
#'
#' \code{$bbox()}\cr
#' Returns a numeric vector of length four containing the bounding box
#' for this layer (xmin, ymin, xmax, ymax). Note that `bForce = true` is set in
#' the underlying API call to `OGR_L_GetExtent()`, so the entire layer may be
#' scanned to compute a minimum bounding rectangle (see `FastGetExtent` in the
#' list returned by `$testCapability()`). Depending on the format driver, a
#' spatial filter may or may not be taken into account, so it is safer to call
#' `$bbox()` without setting a spatial filter.
#'
#' \code{$getLayerDefn()}\cr
#' Returns a list containing the OGR feature class definition for this layer
#' (a.k.a. layer definition). The list contains zero or more attribute field
#' definitions, along with one or more geometry field definitions.
#' See [ogr_define] for details of the field and feature class definitions.
#'
#' \code{$getFieldDomain(domain_name)}\cr
#' Returns a list containing specifications of the OGR field domain with the
#' passed `domain_name`, or `NULL` if `domain_name` is not found.
#' Some formats support the use of field domains that describe the valid values
#' that can be stored in a given attribute field, e.g., coded values that are
#' present in a specified enumeration, values constrained to a specified
#' range, or values that must match a specified pattern.
#' See
#' \url{https://gdal.org/en/stable/user/vector_data_model.html#field-domains}.
#' Requires GDAL >= 3.3.
#'
#' \code{$setAttributeFilter(query)}\cr
#' Sets an attribute query string to be used when fetching features via the
#' `$getNextFeature()` or `$fetch()` methods.
#' Only features for which `query` evaluates as true will be returned.
#' The query string should be in the format of an SQL WHERE clause, described
#' in the ["WHERE"](https://gdal.org/en/stable/user/ogr_sql_dialect.html#where)
#' section of the OGR SQL dialect documentation (e.g.,
#' `"population > 1000000 and population < 5000000"`, where `population` is an
#' attribute in the layer).
#' In some cases (RDBMS backed drivers, SQLite, GeoPackage) the native
#' capabilities of the database may be used to to interpret the WHERE clause,
#' in which case the capabilities will be broader than those of OGR SQL.
#' Note that installing a query string will generally result in resetting the
#' current reading position (as with `$resetReading()` described below).
#' The `query` parameter may be set to empty string (`""`) to clear the current
#' attribute filter.
#'
#' \code{$getAttributeFilter()}\cr
#' Returns the attribute query string currently in use, or empty string (`""`)
#' if an attribute filter is not set.
#'
#' \code{$setIgnoredFields(fields)}\cr
#' Set which fields can be omitted when retrieving features from the layer.
#' The `fields` argument is a character vector of field names. Passing an
#' empty string (`""`) for `fields` will reset to no ignored fields.
#' If the format driver supports this functionality (testable using
#' `$testCapability()$IgnoreFields`), it will not fetch the specified fields
#' in subsequent calls to `$getFeature()` / `$getNextFeature()` / `$fetch()`,
#' and thus save some processing time and/or bandwidth. Besides field names of
#' the layer, the following special fields can be passed: `"OGR_GEOMETRY"` to
#' ignore geometry and `"OGR_STYLE"` to ignore layer style. By default, no
#' fields are ignored. Note that fields that are used in an attribute filter
#' should generally not be set as ignored fields, as most drivers (such as
#' those relying on the OGR SQL engine) will be unable to correctly evaluate
#' the attribute filter. No return value, called for side effects.
#'
#' \code{$setSelectedFields(fields)}\cr
#' Set which fields will be included when retrieving features from the layer.
#' The `fields` argument is a character vector of field names. Passing an
#' empty string (`""`) for `fields` will reset to no ignored fields.
#' See the `$setIgnoredFields()` method above for more information. The data
#' source must provide IgnoreFields capability in order to set selected
#' fields. Note that geometry fields, if desired, must be specified when setting
#' selected fields, either by including named geometry field(s) or the special
#' field `"OGR_GEOMETRY"` in the `fields` argument.
#' No return value, called for side effects.
#'
#' \code{$setSpatialFilter(wkt)}\cr
#' Sets a new spatial filter from a geometry in WKT format. This method sets
#' the geometry to be used as a spatial filter when fetching features via the
#' `$getNextFeature()` or `$fetch()` methods. Only features that geometrically
#' intersect the filter geometry will be returned. Currently this test may be
#' inaccurately implemented (depending on the vector format driver), but it is
#' guaranteed that all features whose envelope overlaps the envelope of the
#' spatial filter will be returned. This can result in more shapes being
#' returned that should strictly be the case.
#' `wkt` is a character string containing a WKT geometry in the same coordinate
#' system as the layer. An empty string (`""`) may be passed indicating that
#' the current spatial filter should be cleared, but no new one instituted.
#'
#' \code{$setSpatialFilterRect(bbox)}\cr
#' Sets a new rectangular spatial filter. This method sets a rectangle to be
#' used as a spatial filter when fetching features via the `$getNextFeature()`
#' or `$fetch()` methods. Only features that geometrically intersect the given
#' rectangle will be returned.
#' `bbox` is a numeric vector of length four containing xmin, ymin, xmax, ymax
#' in the same coordinate system as the layer as a whole (as returned by
#' `$getSpatialRef()`).
#'
#' \code{$getSpatialFilter()}\cr
#' Returns the current spatial filter geometry as a WKT string, or empty string
#' (`""`) if a spatial filter is not set.
#'
#' \code{$clearSpatialFilter()}\cr
#' Clears a spatial filter that was set with `$setSpatialFilterRect()`.
#' No return value, called for that side effect.
#'
#' \code{$getFeatureCount()}\cr
#' Returns the number of features in the layer. For dynamic databases the count
#' may not be exact. This method forces a count in the underlying API call
#' (i.e., `bForce = TRUE` in the call to `OGR_L_GetFeatureCount()`). Note that
#' some vector drivers will actually scan the entire layer once to count
#' features. The `FastFeatureCount` element in the list returned by
#' the `$testCapability()` method can be checked if this might be a concern.
#' The number of features returned takes into account the spatial and/or
#' attribute filters. Some driver implementations of this method may alter the
#' read cursor of the layer.
#'
#' \code{$getNextFeature()}\cr
#' Fetch the next available feature from this layer. Only features matching the
#' current spatial and/or attribute filter (if defined) will be returned.
#' This method implements sequential access to the features of a layer.
#' The `$resetReading()` method can be used to start at the beginning again.
#' Returns a list with the unique feature identifier (FID), the attribute and
#' geometry field names, and their values. The returned list carries the
#' `OGRFeature` class attribute with S3 methods for for `print()` and `plot()`.
#' `NULL` is returned if no more features are available.
#'
#' \code{$setNextByIndex(i)}\cr
#' Moves the read cursor to feature `i` in the current result set
#' (with 0-based indexing).
#' This method allows positioning of a layer such that a call to
#' `$getNextFeature()` or `$fetch()` will read the requested feature(s), where
#' `i` is an absolute index into the current result set. So, setting `i = 3`
#' would mean the next feature read with `$getNextFeature()` would have been
#' the fourth feature read if sequential reading took place from the beginning
#' of the layer, including accounting for spatial and attribute filters.
#' This method is not implemented efficiently by all vector format drivers. The
#' default implementation simply resets reading to the beginning and then calls
#' `GetNextFeature()` `i` times.
#' To determine if fast seeking is available on the current layer, check
#' the `FastSetNextByIndex` element in the list returned by the
#' `$testCapability()` method. No return value, called for side effect.
#'
#' \code{$getFeature(fid)}\cr
#' Returns a feature by its identifier. The value of `fid` must be a numeric
#' scalar, optionally carrying the `bit64::integer64` class attribute.
#' Success or failure of this operation is unaffected by any spatial or
#' attribute filters that may be in effect.
#' The `RandomRead` element in the list returned by `$testCapability()` can
#' be checked to establish if this layer supports efficient random access
#' reading; however, the call should always work if the feature exists since a
#' fallback implementation just scans all the features in the layer looking for
#' the desired feature. Returns a list with the unique feature identifier (FID),
#' the attribute and geometry field names, and their values, or `NULL` on
#' failure. Note that sequential reads (with `$getNextFeature()`) are generally
#' considered interrupted by a call to `$getFeature()`.
#'
#' \code{$resetReading()}\cr
#' Reset feature reading to start on the first feature. No return value, called
#' for that side effect.
#'
#' \code{$fetch(n)}\cr
#' Fetches the next `n` features from the layer and returns them as a data
#' frame. This allows retrieving the entire set of features, one page of
#' features at a time, or the remaining features (from the current cursor
#' position). Returns a data frame with as many rows as features were fetched,
#' and as many columns as attribute plus geometry fields in the result set,
#' even if the result is a single value or has one or zero rows.
#' The returned data frame carries the `OGRFeatureSet` class attribute with S3
#' methods for for `print()` and `plot()`.
#'
#' This method is an analog of
#' [`DBI::dbFetch()`](https://dbi.r-dbi.org/reference/dbFetch.html).
#'
#' The `n` argument is the maximum number of features to retrieve per fetch
#' given as `integer` or `numeric` but assumed to be a whole number (will
#' be truncated). Use `n = -1` or `n = Inf` to retrieve all pending features
#' (resets reading to the first feature).
#' Otherwise, `$fetch()` can be called multiple times to perform forward paging
#' from the current cursor position. Passing `n = NA` is also supported and
#' returns the remaining features.
#' Fetching zero features is possible to retrieve the structure of the feature
#' set as a data frame (columns fully typed).
#'
#' OGR field types are returned as the following R types (`NA` for OGR NULL
#' values):
#' * `OFTInteger`: `integer`
#' * `OFTInteger` subtype `OFSTBoolean`: `logical`
#' * `OFTIntegerList`: vector of `integer` (list column)
#' * `OFTInteger64`: `numeric` carrying `"integer64"` class attribute \{bit64\}
#' * `OFTInteger64` subtype `OFSTBoolean`: `logical`
#' * `OFTInteger64List`: vector of `bit64::integer64` (list column)
#' * `OFTReal`: `numeric`
#' * `OFTRealList`: vector of `numeric` (list column)
#' * `OFTString`: `character` string
#' * `OFTStringList`: vector of `character` strings (list column)
#' * `OFTDate`: class `"Date"` (`numeric`)
#' * `OFTDateTime`: class `"POSIXct"` (`numeric`, millisecond accuracy)
#' * `OFTTime`: `character` string (`"HH:MM:SS"`)
#' * `OFTBinary`: `raw` vector (list column, `NULL` entries for OGR NULL values)
#'
#' Geometries are not returned if the field `returnGeomAs` is set to `NONE`.
#' Omitting the geometries may be beneficial for performance and memory usage
#' when access only to feature attributes is needed. Geometries are returned
#' as `raw` vectors in a data frame list column when `returnGeomAs` is set to
#' `WKB` (the default) or `WKB_ISO`, or as `character` strings when
#' `returnGeomAs` is set to one of `WKT` or `WKT_ISO`.
#'
#' Note that `$getFeatureCount()` is called internally when fetching the full
#' feature set or all remaining features (but not for a page of features).
#'
#' \code{$setFeature(feature)}\cr
#' Rewrites/replaces an existing feature. This method writes a feature based on
#' the feature id within the input feature. The `feature` argument is a named
#' list of fields and their values, and must include a `$FID` element
#' referencing the existing feature to rewrite. The `RandomWrite` element in
#' the list returned by `$testCapability()` can be checked to establish if this
#' layer supports random access writing via `$setFeature()`.
#' The way omitted fields in the passed `feature` are processed is driver
#' dependent:
#' * SQL-based drivers which implement set feature through SQL UPDATE will skip
#'   unset fields, and thus the content of the existing feature will be
#'   preserved.
#' * The shapefile driver will write a NULL value in the DBF file.
#' * The GeoJSON driver will take into account unset fields to remove the
#'   corresponding JSON member.
#'
#' Returns logical `TRUE` upon successful completion, or `FALSE` if setting the
#' feature did not succeed. The FID of the last feature written to the layer
#' may be obtained with the method `$getLastWriteFID()` (see below). To set a
#' feature, but create it if it doesn't exist see the `$upsertFeature()` method.
#'
#' \code{$createFeature(feature)}\cr
#' Creates and writes a new feature within the layer. The `feature` argument is
#' a named list of fields and their values.
#' The passed feature is written to the layer as a new feature, rather than
#' overwriting an existing one. If the feature has a `$FID` element other than
#' `NA`, then the vector format driver may use that as the feature id of the
#' new feature, but not necessarily. The FID of the last feature written
#' to the layer may be obtained with the method `$getLastWriteFID()` (see
#' below).
#' Returns logical `TRUE` upon successful completion, or `FALSE` if creating
#' the feature did not succeed. To create a feature, but set it if it already
#' exists see the `$upsertFeature()` method.
#'
#' \code{$upsertFeature(feature)}\cr
#' Rewrites/replaces an existing feature or creates a new feature within the
#' layer. This method will write a feature to the layer, based on the feature
#' id within the input feature. The `feature` argument is a named list of
#' fields and their values, potentially including a `$FID` element referencing
#' an existing feature to rewrite. If the feature id doesn't exist a new
#' feature will be written. Otherwise, the existing feature will be rewritten.
#' The `UpsertFeature` element in the list returned by `$testCapability()` can
#' be checked to determine if this layer supports upsert writing. See
#' `$setFeature()` above for a description of how omitted fields in the passed
#' `feature` are processed.
#' Returns logical `TRUE` upon successful completion, or `FALSE` if upserting
#' the feature did not succeed. Requires GDAL >= 3.6.
#'
#' \code{$getLastWriteFID()}\cr
#' Returns the FID of the last feature written (either newly created or updated
#' existing). `NULL` is returned if no features have been written in the layer.
#' Note that OGRNullFID (`-1`) may be returned after writing a feature in some
#' formats. This is the case if a FID has not been assigned yet, and generally
#' does not indicate an error (e.g., formats that do not store a persistent FID
#' and assign FIDs upon a sequential read operation). The returned FID is a
#' numeric scalar carrying the `bit64::integer64` class attribute.
#'
#' \code{$deleteFeature(fid)}\cr
#' Deletes a feature from the layer. The feature with the indicated feature ID
#' is deleted from the layer if supported by the format driver. The value of
#' `fid` must be a numeric scalar, optionally carrying the `bit64::integer64`
#' class attribute (should be a whole number, will be truncated).
#' The `DeleteFeature` element in the list returned by `$testCapability()` can
#' be checked to establish if this layer has delete feature capability. Returns
#' logical `TRUE` if the operation succeeds, or `FALSE` on failure.
#'
#' \code{$syncToDisk()}\cr
#' Flushes pending changes to disk. This call is intended to force the layer to
#' flush any pending writes to disk, and leave the disk file in a consistent
#' state. It would not normally have any effect on read-only datasources. Some
#' formats do not implement this method, and will still return no error. An
#' error is only returned if an error occurs while attempting to flush to disk.
#' In any event, you should always close any opened datasource with `$close()`
#' which will ensure all data is correctly flushed. Returns logical `TRUE` if
#' no error occurs (even if nothing is done) or `FALSE` on error.
#'
#' \code{$startTransaction(force)}\cr
#' Creates a transaction if supported by the vector data source. The `force`
#' argument is a logical value. If `force = FALSE`, only "efficient"
#' transactions will be attempted. Some drivers may offer an emulation of
#' transactions, but sometimes with significant overhead, in which case the
#' user must explicitly allow for such an emulation by setting `force =TRUE`.
#' The function `ogr_ds_test_cap()` can be used to determine whether a vector
#' data source supports efficient or emulated transactions.
#'
#' All changes done after the start of the transaction are definitely applied
#' in the data source if `$commitTransaction()` is called. They can be canceled
#' by calling `rollbackTransaction()` instead.
#' Nested transactions are not supported. Transactions are implemented at the
#' dataset level, so multiple `GDALVector` objects using the same data source
#' should not have transactions active at the same time.
#'
#' In case `$startTransaction()` fails, neither `$commitTransaction()` nor
#' `$rollbackTransaction()` should be called.
#' If an error occurs after a successful `$startTransaction()`, the whole
#' transaction may or may not be implicitly canceled, depending on the format
#' driver (e.g., the PostGIS driver will cancel it, SQLite/GPKG will not). In
#' any case, in the event of an error, an explicit call to
#' `rollbackTransaction()` should be done to keep things balanced.
#'
#' Returns logical `TRUE` if the transaction is created, or `FALSE` on failure.
#'
#' \code{$commitTransaction()}\cr
#' Commits a transaction if supported by the vector data source.
#' Returns a logical value, `TRUE` if the transaction is successfully committed.
#' Returns `FALSE` if no transaction is active, or the rollback fails, or if the
#' data source does not support transactions.
#' Depending on the format driver, this may or may not abort layer sequential
#' reading that may be active.
#'
#' \code{$rollbackTransaction()}\cr
#' Rolls back a data source to its state before the start of the current
#' transaction, if transactions are supported by the data source.
#' Returns a logical value, `TRUE` if the transaction is successfully rolled
#' back. Returns `FALSE` if no transaction is active, or the rollback fails,
#' or if the data source does not support transactions.
#'
#' \code{$getMetadata()}\cr
#' Returns a character vector of all metadata `NAME=VALUE` pairs for the
#' layer or empty string (`""`) if there are no metadata items.
#'
#' \code{$setMetadata(metadata)}\cr
#' Sets metadata on the layer if the format supports it. The \code{metadata}
#' argument is given as a character vector of `NAME=VALUE` pairs.
#' Returns logical \code{TRUE} on success or \code{FALSE} if metadata could
#' not be set.
#'
#' \code{$getMetadataItem(mdi_name)}\cr
#' Returns the value of a specific metadata item named \code{mdi_name}, or empty
#' string (`""`) if no matching item is found.
#'
#' \code{$close()}\cr
#' Closes the vector dataset (no return value, called for side effects).
#' Calling \code{$close()} results in proper cleanup, and flushing of any
#' pending writes.
#' The `GDALVector` object is still available after calling \code{$close()}.
#' The layer can be re-opened on the existing \code{dsn} with
#' \code{$open(read_only = {TRUE|FALSE})}.
#'
#' @seealso
#' [ogr_define], [ogr_manage], [ogr2ogr()], [ogrinfo()]
#'
#' GDAL vector format descriptions:\cr
#' \url{https://gdal.org/en/stable/drivers/vector/index.html}
#'
#' GDAL-supported SQL dialects:\cr
#' \url{https://gdal.org/en/stable/user/ogr_sql_sqlite_dialect.html})
#'
#' @examples
#' ## MTBS fire perimeters in Yellowstone National Park 1984-2022
#' f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
#'
#' ## copy to a temporary file that is writeable
#' dsn <- file.path(tempdir(), basename(f))
#' file.copy(f, dsn)
#'
#' lyr <- new(GDALVector, dsn, "mtbs_perims")
#'
#' ## object of class GDALVector
#' lyr
#' str(lyr)
#'
#' ## dataset info
#' lyr$getDriverShortName()
#' lyr$getDriverLongName()
#' lyr$getFileList()
#'
#' ## layer info
#' lyr$getName()
#' lyr$getGeomType()
#' lyr$getGeometryColumn()
#' lyr$getFIDColumn()
#' lyr$getSpatialRef()
#' lyr$bbox()
#'
#' ## layer capabilities
#' lyr$testCapability()
#'
#' ## re-open with write access
#' lyr$open(read_only = FALSE)
#' lyr$testCapability()$SequentialWrite
#' lyr$testCapability()$RandomWrite
#'
#' ## feature class definition - a list of field names and their definitions
#' defn <- lyr$getLayerDefn()
#' names(defn)
#' str(defn)
#'
#' ## default value of the read/write field 'returnGeomAs'
#' lyr$returnGeomAs
#'
#' lyr$getFeatureCount()
#'
#' ## sequential read cursor
#' feat <- lyr$getNextFeature()
#' # a list of field names and their values, with class attribute `OGRFeature`
#' feat
#'
#' ## set an attribute filter
#' lyr$setAttributeFilter("ig_year = 2020")
#' lyr$getFeatureCount()
#'
#' feat <- lyr$getNextFeature()
#' plot(feat)
#'
#' ## NULL when no more features are available
#' lyr$getNextFeature()
#'
#' ## reset reading to the start
#' lyr$resetReading()
#' lyr$getNextFeature()
#'
#' ## clear the attribute filter
#' lyr$setAttributeFilter("")
#' lyr$getFeatureCount()
#'
#' ## set a spatial filter
#' ## get the bounding box of the largest 1988 fire and use as spatial filter
#' ## first set a temporary attribute filter to do the lookup
#' lyr$setAttributeFilter("ig_year = 1988 ORDER BY burn_bnd_ac DESC")
#' feat <- lyr$getNextFeature()
#' feat
#'
#' bbox <- g_wk2wk(feat$geom) |> bbox_from_wkt()
#'
#' ## set spatial filter on the full layer
#' lyr$setAttributeFilter("")  # clears
#' lyr$setSpatialFilterRect(bbox)
#' lyr$getFeatureCount()
#'
#' ## fetch in chunks and return as data frame (class `OGRFeatureSet`)
#' feat_set <- lyr$fetch(20)
#' head(feat_set)
#' plot(feat_set)
#'
#' ## the next chunk
#' feat_set <- lyr$fetch(20)
#' nrow(feat_set)
#'
#' ## no features remaining
#' feat_set <- lyr$fetch(20)
#' nrow(feat_set)
#' str(feat_set)  # 0-row data frame with columns typed
#'
#' ## fetch all pending features
#' feat_set <- lyr$fetch(-1)  # resets reading to the first feature
#' nrow(feat_set)
#' plot(feat_set)
#'
#' lyr$clearSpatialFilter()
#' lyr$getFeatureCount()
#'
#' lyr$close()
#' \dontshow{unlink(dsn)}
#'
#' ## simple example for feature write methods showing use of various data types
#' ## create and write to a new layer in a GeoPackage data source
#' dsn2 <- tempfile(fileext = ".gpkg")
#'
#' ## define a feature class
#' defn <- ogr_def_layer("POINT", srs = epsg_to_wkt(4326))
#'
#' ## add field definitions
#' defn$unique_int <- ogr_def_field("OFTInteger", is_nullable = FALSE,
#'                                  is_unique = TRUE)
#' defn$bool_data <- ogr_def_field("OFTInteger", fld_subtype = "OFSTBoolean")
#' defn$large_ints <- ogr_def_field("OFTInteger64")
#' defn$doubles <- ogr_def_field("OFTReal")
#' defn$strings <- ogr_def_field("OFTString", fld_width = 50)
#' defn$dates <- ogr_def_field("OFTDate")
#' defn$dt_modified <- ogr_def_field("OFTDateTime",
#'                                   default_value = "CURRENT_TIMESTAMP")
#' defn$blobs <- ogr_def_field("OFTBinary")
#'
#' ogr_ds_create("GPKG", dsn2, "test_layer", layer_defn = defn)
#'
#' lyr <- new(GDALVector, dsn2, "test_layer", read_only = FALSE)
#' # lyr$getLayerDefn() |> str()
#'
#' ## define a feature to write
#' feat1 <- list()
#' ## $FID is omitted since it is assigned when written (could also be NA)
#' ## $dt_modified is omitted since the datasource sets a default timestamp
#' feat1$unique_int <- 1001
#' feat1$bool_data <- TRUE
#' ## passing a string to as.integer64()
#' ## this value is too large to be represented exactly as R numeric (double)
#' feat1$large_ints <- bit64::as.integer64("90071992547409910")
#' feat1$doubles <- 1.234
#' feat1$strings <- "A test string"
#' feat1$dates <- as.Date("2024-01-01")
#' feat1$blobs <- charToRaw("A binary object")
#' feat1$geom <- "POINT (1 1)"  # can be a WKT string or raw vector of WKB
#'
#' ## create as a new feature in the layer
#' lyr$createFeature(feat1)
#'
#' ## the assigned FID
#' lyr$getLastWriteFID()
#'
#' ## this fails due to the unique constraint
#' lyr$createFeature(feat1)
#'
#' feat2 <- list()
#' feat2$unique_int <- 1002
#' feat2$bool_data <- FALSE
#' feat2$large_ints <- bit64::as.integer64("90071992547409920")
#' feat2$doubles <- 2.345
#' feat2$strings <- "A test string 2"
#' feat2$dates <- as.Date("2024-01-02")
#' feat2$blobs <- charToRaw("A binary object 2")
#' feat2$geom <- "POINT (2 2)"
#'
#' lyr$createFeature(feat2)
#' lyr$getLastWriteFID()
#'
#' ## close and re-open as a read-only layer
#' lyr$open(read_only = TRUE)
#'
#' lyr$getFeatureCount()
#' feat_set <- lyr$fetch(-1)  # -1 for all features reading from start
#' str(feat_set)
#'
#' ## edit an existing feature, e.g., feat <- lyr$getFeature(2)
#' ## here we copy a row of the data frame returned by lyr$fetch() above
#' feat <- feat_set[2,]
#' str(feat)
#'
#' Sys.sleep(1)  # only to ensure a timestamp difference
#'
#' feat$bool_data <- TRUE
#' feat$strings <- paste(feat$strings, "- edited")
#' feat$dt_modified <- Sys.time()
#' feat$geom <- "POINT (2.001 2.001)"
#'
#' lyr$open(read_only = FALSE)
#'
#' ## lyr$setFeature() re-writes the feature identified by the $FID element
#' ## N.B., all fields are re-written:
#' ##   any fields omitted from the input feature, or set to NA, will be
#' ##   re-written as OGR NULL
#' lyr$setFeature(feat)
#'
#' lyr$open(read_only = TRUE)
#' lyr$getFeatureCount()
#'
#' lyr$returnGeomAs <- "WKT"
#' feat_set <- lyr$fetch(-1)
#' str(feat_set)
#'
#' lyr$close()
#' \dontshow{unlink(dsn2)}
NULL

Rcpp::loadModule("mod_GDALVector", TRUE)
