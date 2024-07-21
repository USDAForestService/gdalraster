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
#' See \url{https://gdal.org/api/index.html} for details of the GDAL
#' Vector API.
#'
#' @param dsn Character string containing the data source name (DSN, usually a
#' filename or database connection string). See the GDAL vector format
#' descriptions at \url{https://gdal.org/drivers/vector/index.html}.
#' @param layer Character string containing either the name of a layer of
#' features within the data source, or an SQL SELECT statement to be executed
#' against the data source that defines a layer via its result set.
#' @param read_only Logical. `TRUE` to open the layer read-only (the default),
#' or `FALSE` to open with write access.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#' specifying dataset open options.
#' @param spatial_filter Optional character string containing a geometry in
#' Well Known Text (WKT) format which represents a spatial filter.
#' @param dialect Optional character string to control the statement dialect
#' when SQL is used to define the layer. By default, the OGR SQL engine will
#' be used, except for RDBMS drivers that will use their dedicated SQL engine,
#' unless `"OGRSQL"` is explicitly passed as the dialect. The `SQLITE`
#' dialect can also be used
#' (see \url{https://gdal.org/user/ogr_sql_sqlite_dialect.html}).
#' @returns An object of class `GDALVector` which contains pointers to the
#' opened layer and the dataset that contains it, and methods that operate on
#' the layer as described in Details. `GDALVector` is a C++ class exposed
#' directly to R (via `RCPP_EXPOSED_CLASS`). Fields and methods of the class
#' are accessed using the `$` operator. The read/write fields are used for
#' per-object settings.
#'
#' @section Usage:
#' \preformatted{
#' ## Constructors
#' # read-only by default:
#' ds <- new(GDALVector, dsn)
#' ds <- new(GDALVector, dsn, layer)
#' # for update access:
#' ds <- new(GDALVector, dsn, layer, read_only = FALSE)
#' # to use dataset open options
#' ds <- new(GDALVector, dsn, layer, read_only = TRUE|FALSE, open_options)
#' # to specify a spatial filter and/or dialect
#' new(GDALVector, dsn, layer, read_only, open_options, spatial_filter, dialect)
#'
#' ## Read/write fields (see Details)
#' lyr$defaultGeomFldName
#' lyr$returnGeomAs
#' lyr$wkbByteOrder
#'
#' ## Methods (see Details)
#' lyr$open(read_only)
#' lyr$isOpen()
#' lyr$getDsn()
#' lyr$getFileList()
#' lyr$getDriverShortName()
#' lyr$getDriverLongName()
#'
#' lyr$getName()
#' lyr$testCapability()
#' lyr$getFIDColumn()
#' lyr$getGeomType()
#' lyr$getGeometryColumn()
#' lyr$getSpatialRef()
#' lyr$bbox()
#' lyr$getLayerDefn()
#'
#' lyr$setAttributeFilter(query)
#' lyr$setSpatialFilterRect(bbox)
#' lyr$clearSpatialFilter()
#'
#' lyr$getFeatureCount()
#' lyr$getNextFeature()
#' lyr$getFeature(fid)
#' lyr$resetReading()
#'
#' lyr$fetch(n);
#'
#' lyr$close()
#' }
#' @section Details:
#'
#' \code{new(GDALVector, dsn)}
#' Constructor. If `layer` is omitted, it defaults to the first layer in the
#' data source by index, so this form of the constructor might be used for
#' single-layer formats like shapefile. `read_only` defaults to `TRUE`.
#'
#' \code{new(GDALVector, dsn, layer)}
#' Constructor specifying the name of a layer to open. `layer` may also be given
#' as an SQL SELECT statement to define a layer as the result set (read only).
#'
#' \code{new(GDALVector, dsn, layer, read_only = TRUE|FALSE)}
#' Constructor specifying read/write access. The `layer` argument is required in
#' this form of the constructor, but may be given as empty string (`""`), in
#' which case the first layer in the data source by index will be opened.
#'
#' \code{new(GDALVector, dsn, layer, read_only = TRUE|FALSE, open_options)}
#' Constructor specifying dataset open options as a character vector of
#' `NAME=VALUE` pairs.
#'
#' \code{new(GDALVector, dsn, layer, read_only, open_options, spatial_filter, dialect))}
#' Constructor specifying a spatial filter and/or SQL dialect. All arguments
#' are required in this form of the constructor, but `open_options` may be
#' `NULL`, and `spatial_filter` or `dialect` may be empty string (`""`).
#'
#' \code{$defaultGeomFldName}
#' Read/write field specifying a return column name when the geometry column
#' name in the source layer is empty, like with shapefiles etc.
#' Character string, defaults to `geometry`.
#'
#' \code{$returnGeomAs}
#' Read/write field specifying the return format for feature geometries.
#' Character string, one of `WKT`, `WKT_ISO`, `WKB`, `WKB_ISO`, `TYPE_NAME` or
#' `NONE` (the default).
#'
#' \code{$wkbByteOrder}
#' Read/write field specifying the byte order for WKB geometries.
#' Character string, one `LSB` (Least Significant Byte First, the default) or
#' `MSB` (Most Significant Byte First).
#'
#' \code{$open(read_only)}
#' (Re-)opens the vector layer on the existing DSN. Use this method to
#' open a layer that has been closed using \code{$close()}. May be used to
#' re-open a layer with a different read/write access (`read_only` set to
#' `TRUE` or `FALSE`). The method will first close an open dataset, so it is
#' not required to call \code{$close()} explicitly in this case.
#' No return value, called for side effects.
#'
#' \code{$isOpen()}
#' Returns a `logical` scalar indicating whether the vector dataset is open.
#'
#' \code{$getDsn()}
#' Returns a character string containing the `dsn` associated with this
#' `GDALVector` object (`dsn` originally used to open the layer).
#'
#' \code{$getFileList()}
#' Returns a character vector of files believed to be part of the data source.
#' If it returns an empty string (`""`) it means there is believed to be no
#' local file system files associated with the dataset (e.g., a virtual file
#' system). The returned filenames will normally be relative or absolute
#' paths depending on the path used to originally open the dataset.
#'
#' \code{$getDriverShortName()}
#' Returns the short name of the vector format driver.
#'
#' \code{$getDriverLongName()}
#' Returns the long name of the vector format driver.
#'
#' \code{$getName()}
#' Returns the layer name.
#'
#' \code{$testCapability()}
#' Tests whether the layer supports named capabilities based on the current
#' read/write access. Returns a list of capabilities with values `TRUE` or
#' `FALSE`. See `ogr_layer_test_cap()` for a list of the capabilities tested.
#'
#' \code{$getFIDColumn()}
#' Returns the name of the underlying database column being used as the FID
#' column, or empty string (`""`) if not supported.
#'
#' \code{$getGeomType()}
#' Returns the well known name of the layer geometry type as character string.
#' For layers with multiple geometry fields, this method only returns the
#' geometry type of the first geometry column. For other columns, use
#' `$getLayerDefn()`. For layers without any geometry field, this method
#' returns `NONE`.
#'
#' \code{$getGeometryColumn()}
#' Returns he name of the underlying database column being used as the geometry
#' column, or an empty string (`""`) if not supported.
#' For layers with multiple geometry fields, this method only returns the
#' name of the first geometry column. For other columns, use `$getLayerDefn()`.
#'
#' \code{$getSpatialRef()}
#' Returns a WKT string containing the spatial reference system for this layer.
#'
#' \code{$bbox()}
#' Returns a numeric vector of length four containing the bounding box
#' (xmin, ymin, xmax, ymax) for this layer. Note that `bForce = true` is set in
#' the underlying API call to `OGR_L_GetExtent()`, so the entire layer may be
#' scanned to compute minimum bounding rectangle (see `FastGetExtent` in the
#' list returned by `$testCapability()`). Depending on the driver, a spatial
#' filter may/may not be taken into account, so it is safer to call `$bbox()`
#' without setting a spatial filter.
#'
#' \code{$getLayerDefn()}
#' Returns a list containing the OGR feature class definition for this layer
#' (a.k.a. layer definition). The list contains zero or more attribute field
#' definitions, along with one or more geometry field definitions.
#' See [ogr_define] for details of the field and feature class definitions.
#'
#' \code{$setAttributeFilter(query)}
#' Sets an attribute query string to be used when fetching features via the
#' `$getNextFeature()` or `$fetch()` methods.
#' Only features for which `query` evaluates as true will be returned.
#' The query string should be in the format of an SQL WHERE clause, e.g.,
#' `"population > 1000000 and population < 5000000"` where population is an
#' attribute in the layer. The query format is normally a SQL WHERE clause as
#' described in the ["WHERE"](https://gdal.org/user/ogr_sql_dialect.html#where)
#' section of the OGR SQL dialect documentation.
#' In some cases (RDBMS backed drivers, SQLite, GeoPackage) the native
#' capabilities of the database may be used to to interpret the WHERE clause,
#' in which case the capabilities will be broader than those of OGR SQL.
#' Note that installing a query string will generally result in resetting the
#' current reading position (as with `$resetReading()` below).
#' The `query` parameter may be set to `""` (empty string) to clear the current
#' attribute filter.
#'
#' \code{$setSpatialFilterRect(bbox)}
#' Sets a new rectangular spatial filter. This method sets a rectangle to be
#' used as a spatial filter when fetching features via the `$getNextFeature()`
#' or `$fetch()` methods. Only features that geometrically intersect the given
#' rectangle will be returned.
#' The x/y values in `bbox` (a `numeric` vector of length four: xmin, ymin,
#' xmax, ymax) should be in the same coordinate system as the layer as a whole
#' (as returned by `$getSpatialRef()`).
#'
#' \code{$clearSpatialFilter()}
#' Clears a spatial filter that was set with `$setSpatialFilterRect()`.
#' No return value, called for that side effect.
#'
#' \code{$getFeatureCount()}
#' Returns the number of features in the layer. For dynamic databases the count
#' may not be exact. This method forces a count in the underlying API call
#' (i.e., `bForce = TRUE` in the call to `OGR_L_GetFeatureCount()`). Note that
#' some vector drivers will actually scan the entire layer once to count
#' features. The list element `FastFeatureCount` returned by
#' `$testCapability()` can be checked if this might be a concern.
#' The returned count takes the spatial and/or attribute filters into account.
#' Note that some driver implementations of this method may alter the read
#' cursor of the layer.
#'
#' \code{$getNextFeature()}
#' Fetch the next available feature from this layer. Only features matching the
#' current spatial and/or attribute filter (if defined) will be returned.
#' This method implements sequential access to the features of a layer.
#' The `$resetReading()` method can be used to start at the beginning again.
#' Returns a list with the unique feature identifier (FID), the attribute and
#' geometry field names, and their values. `NULL` is retunred if no more
#' features are available.
#'
#' \code{$getFeature(fid)}
#' Returns a feature by its identifier. The value of `fid` must be a numeric
#' scalar, optionally carrying the `bit64::integer64` class attribute.
#' Success or failure of this operation is unaffected by any spatial or
#' attribute filters that may be in effect.
#' The list element `RandomRead` returned by `$testCapability()` can be checked
#' to establish if this layer supports efficient random access reading;
#' however, the call should always work if the feature exists since a fallback
#' implementation just scans all the features in the layer looking for the
#' desired feature. Returns a list with the unique feature identifier (FID),
#' the attribute and geometry field names, and their values, or `NULL` on
#' failure. Note that sequential reads (with `$getNextFeature()`) are generally
#' considered interrupted by a `$getFeature()` call.
#'
#' \code{$resetReading()}
#' Reset feature reading to start on the first feature. No return value, called
#' for that side effect.
#'
#' \code{$fetch(n)}
#' Fetches the next `n` features from the layer and returns them as a data
#' frame. This allows retrieving the entire feature set, one page of features
#' at a time, or the remaining features (potentially with an attribute and/or
#' spatial filter applied). This function is an analog of `DBI::dbFetch()`,
#' where the `GDALVector` object itself is analogous to a DBI result set.
#' The `n` parameter is the maximum number of features to retrieve per fetch
#' given as `numeric` scalar (assumed to be a whole number, will be truncated).
#' Use `n = -1` or `n = Inf` to retrieve all pending features (resets reading
#' to the first feature).
#' Otherwise, `fetch(n)` can be called multiple times to perform forward paging
#' from the current cursor position. Passing `n = NA` is supported and returns
#' the remaining features (from the current cursor position).
#' Fetching zero features is also possible to retrieve the structure of the
#' result set as a data frame.
#' OGR field types are returned as the following R types (`NA` for OGR NULL
#' values):
#' * `OFTInteger`: `integer` (or `logical` for subtype `OFSTBoolean`)
#' * `OFTIntegerList`: vector of `integer` (data frame list column)
#' * `OFTInteger64`: `bit64::integer64` (or `logical` for subtype `OFSTBoolean`)
#' * `OFTInteger64List`: vector of `bit64::integer64` (data frame list column)
#' * `OFTReal`: `numeric`
#' * `OFTRealList`: vector of `numeric` (data frame list column)
#' * `OFTString`: `character` string
#' * `OFTStringList`: vector of `character` strings (data frame list column)
#' * `OFTDate`: `Date`
#' * `OFTDateTime`: `POSIXct` (millisecond accuracy, adjustment for time zone
#' flag if present)
#' * `OFTBinary`: `raw` vector (data frame list column)
#'
#' Geomtries are not returned if the field `returnGeomAs` is set to `NONE`
#' (currently the default).
#' Geometries are returned as `raw` vectors in a data frame list column if the
#' field `returnGeomAs` is set to `WKB` or `WKB_ISO`.
#' Otherwise, geometries are returned as `character` (`returnGeomAs` set to one
#' of `WKT`, `WKT_ISO` or `TYPE_NAME`).
#'
#' \code{$close()}
#' Closes the vector dataset (no return value, called for side effects).
#' Calling \code{$close()} results in proper cleanup, and flushing of any
#' pending writes.
#' The `GDALVector` object is still available after calling \code{$close()}.
#' The layer can be re-opened on the existing \code{dsn} with
#' \code{$open(read_only=TRUE)} or \code{$open(read_only=FALSE)}.
#'
#' @note
#'
#' @seealso
#' [ogr_define], [ogr_manage], [ogr2ogr()], [ogrinfo()]
#'
#' @examples
#'
NULL

Rcpp::loadModule("mod_GDALVector", TRUE)
