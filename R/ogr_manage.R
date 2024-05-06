# This file contains the R interface for src/ogr_util.h.
# Chris Toney <chris.toney at usda.gov>

#' Utility functions for managing vector data sources
#'
#' This set of management functions can be used to create new vector datasets,
#' test existence of dataset/layer/field, test dataset capabilities,
#' create new layers in an existing dataset, delete layers, create new
#' attribute and geometry fields on an existing layer, and delete fields.
#'
#' @name ogr_manage
#' @details
#' ### Utilities for vector data sources
#'
#' These functions should be complementary to `ogrinfo()` and `ogr2ogr()` for
#' vector data management. Bindings to OGR wrap portions of the GDAL Vector
#' API (ogr_core.h and ogr_api.h, [https://gdal.org/api/vector_c_api.html]).
#'
#' `ogr_ds_exists()` tests whether a vector dataset can be opened from the
#' given data source name (DSN), potentially testing for update access.
#' Returns a logical scalar.
#'
#' `ogr_ds_test_cap()` tests the capabilities of a vector dataset, attempting
#' to open it with update access by default. Returns a list of capabilities
#' with values `TRUE` or `FALSE`, or `NULL` is returned if `dsn` cannot be
#' opened with the requested access. The returned list elements include the
#' following:
#' * `CreateLayer`: `TRUE` if this datasource can create new layers
#' * `DeleteLayer`: `TRUE` if this datasource can delete existing layers
#' * `CreateGeomFieldAfterCreateLayer`: `TRUE` if the layers of this
#' datasource support geometry field creation just after layer creation
#' * `CurveGeometries`: `TRUE` if this datasource supports curve geometries
#' * `Transactions`: `TRUE` if this datasource supports (efficient)
#' transactions
#' * `EmulatedTransactions`: `TRUE` if this datasource supports transactions
#' through emulation
#' * `RandomLayerRead`: `TRUE` if this datasource has a dedicated
#' `GetNextFeature()` implementation, potentially returning features from
#' layers in a non-sequential way
#' * `RandomLayerWrite`: `TRUE` if this datasource supports calling
#' `CreateFeature()` on layers in a non-sequential way
#'
#' `ogr_ds_create()` creates a new vector datasource, optionally also creating
#' a layer, and optionally creating one or more fields on the layer.
#' The attribute fields and geometry field(s) to create can be specified as a
#' feature class definition (`layer_defn` as list, see below), or
#' alternatively, by giving the `geom_type` and `srs`, optionally along with
#' one `fld_name` and `fld_type` to be created in the layer. Returns a logical
#' scalar, `TRUE` indicating success.
#'
#' `ogr_ds_layer_count()` returns the number of layers in a vector dataset.
#'
#' `ogr_ds_layer_names()` returns a character vector of layer names in a
#' vector dataset, or `NULL` if no layers are found.
#'
#' `ogr_layer_exists()` tests whether a layer can be accessed by name in a
#' given vector dataset. Returns a logical scalar.
#'
#' `ogr_layer_create()` creates a new layer in a vector dataset, with a
#' specified geometry type and spatial reference definition. This function also
#' accepts a feature class definition given as a list of field names and their
#' definitions (see below). Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_layer_fld_names()` returns a character vector of field names on a
#' layer, or `NULL` if no fields are found.
#'
#' `ogr_layer_delete()` deletes an existing layer in a vector dataset.
#' Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_field_index()` tests for existence of an attribute field by name.
#' Returns the field index on the layer (0-based), or `-1` if the field does
#' not exist.
#'
#' `ogr_field_create()` creates a new attribute field of specified data type in
#' a given DSN/layer. Several optional field properties can be specified in
#' addition to the type. Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_geom_field_create()` creates a new geometry field of specified type in
#' a given DSN/layer. Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_field_delete()` deletes an existing field on a vector layer.
#' Not all format drivers support this function. Some drivers may only support
#' deleting a field while there are still no features in the layer.
#' Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_execute_sql()` executes an SQL statement against the data store.
#' This function can be used to modify the schema or edit data using SQL
#' (e.g., `ALTER TABLE`, `DROP TABLE`, `CREATE INDEX`, `DROP INDEX`, `INSERT`,
#' `UPDATE`, `DELETE`). Currently, this function does not return a result set
#' for a `SELECT` statement. Returns `NULL` invisibly.
#'
#' ### Feature class definition
#'
#' All features in an OGR Layer share a common schema (feature class), modeled
#' in GDAL as OGR Feature Definition. The feature class definition includes the
#' set of attribute fields and their data types and the geometry field(s).
#' In R, a feature class definition is represented as a list, having as names
#' the attribute/geometry field names, with each list element holding a field
#' definition.
#'
#' An attribute field definition is a list with named elements:
#' ```
#' $type       : OGR Field Type ("OFTReal", "OFTString" etc.)
#' $subtype    : optional ("OFSTBoolean", ...)
#' $width      : optional max number of characters
#' $precision  : optional number of digits after the decimal point
#' $is_nullable: optional NOT NULL constraint (logical scalar)
#' $is_unique  : optional UNIQUE constraint (logical scalar)
#' $default    : optional default value as character string
#' $is_ignored : optionally ignored when retrieving features (logical scalar)
#' $is_geom    : FALSE (the default) for attribute fields
#' ```
#'
#' An OGR field type is specified as a character string with possible values:
#' `OFTInteger`, `OFTIntegerList`, `OFTReal`, `OFTRealList`, `OFTString`,
#' `OFTStringList`, `OFTBinary`,  `OFTDate`, `OFTTime`, `OFTDateTime`,
#' `OFTInteger64`, `OFTInteger64List`.
#'
#' An optional field subtype is specified as a character string with possible
#' values:
#' `OFSTNone`, `OFSTBoolean`, `OFSTInt16`, `OFSTFloat32`, `OFSTJSON`,
#' `OFSTUUID`.
#'
#' A geometry field definition is a list with named elements:
#' ```
#' $type       : geom type ("Point", "Polygon", etc.)
#' $srs        : optional spatial reference as WKT string
#' $is_nullable: optional NOT NULL constraint (logical scalar)
#' $is_ignored : optionally ignored when retrieving features (logical scalar)
#' $is_geom    : TRUE (required) for geometry fields
#' ```
#'
#' Typically, there is one geometry field on a layer, but some formats support
#' more than one geometry column per table (e.g., PostGIS).
#'
#' Geometry types are specified as a character string containing OGC WKT.
#' Common types include: `Point`, `LineString`, `Polygon`, `MultiPoint`,
#' `MultiLineString`, `MultiPolygon`. See the GDAL documentation for a list
#' of all supported geometry types:\cr
#' [https://gdal.org/api/vector_c_api.html#_CPPv418OGRwkbGeometryType]
#'
#' Format drivers may or may not support not-null constraints on attribute and
#' geometry fields. If they support creating fields with not-null constraints,
#' this is generally before creating any features to the layer. In some cases,
#' a not-null constraint may be available as a layer creation option. For
#' example, GeoPackage format has a layer creation option
#' `GEOMETRY_NULLABLE=[YES/NO]`.
#'
#' @param dsn Character string. The vector data source name, e.g., a filename
#' or database connection string.
#' @param with_update Logical scalar. `TRUE` to request update access when
#' opening the dataset, or `FALSE` to open read-only.
#' @param format GDAL short name of the vector format as character string.
#' Examples of some common output formats include: `"GPKG"`, `"FlatGeobuf"`,
#' `"ESRI Shapefile"`, `"SQLite"`.
#' @param layer Character string for a layer name in a vector dataset.
#' @param layer_defn A feature class definition for `layer` as a list of
#' zero or more attribute field definitions, and at least one geometry field
#' definition (see Details).
#' Each field definition is a list with named elements containing values for
#' the field type and other properties.
#' If `layer_defn` is given, it will be used and any additional parameters
#' passed that relate to the feature class definition will be ignored (i.e.,
#' `geom_type` and `srs`, as well as `fld_name` and `fld_type` in
#' `ogr_ds_create()`).
#' The first geometry field definition in `layer_defn` defines the
#' `geom_type` and `srs` for the layer.
#' @param geom_type Character string specifying a geometry type (see Details).
#' @param srs Character string containing a spatial reference system definition
#' as OGC WKT or other well-known format (e.g., the input formats usable with
#' [srs_to_wkt()]).
#' @param fld_name Character string containing the name of an attribute field
#' in `layer`.
#' @param fld_defn A field definition as list (see Details). Additional
#' arguments in `ogr_field_create()` will be ignored if a `fld_defn` is given.
#' @param fld_type Character string containing the name of a field data type
#' (e.g., `OFTInteger`, `OFTReal`, `OFTString`, see Details.)
#' @param fld_subtype Character string containing the name of a field subtype.
#' One of  `OFSTNone` (the default), `OFSTBoolean`, `OFSTInt16`, `OFSTFloat32`,
#' `OFSTJSON`, `OFSTUUID`.
#' @param fld_width Optional integer scalar specifying max number of characters.
#' @param fld_precision Optional integer scalar specifying number of digits
#' after the decimal point.
#' @param is_nullable Optional NOT NULL field constraint (logical scalar).
#' Defaults to `TRUE`.
#' @param is_ignored Whether field is ignored when retrieving features (logical
#' scalar). Defaults to `FALSE`.
#' @param is_unique Optional UNIQUE constraint on the field (logical scalar).
#' Defaults to `FALSE`.
#' @param default_value Optional default value for the field as a character
#' string.
#' @param geom_fld_defn A geometry field definition as list (see Details).
#' Additional arguments in `ogr_geom_field_create()` will be ignored if a
#' `geom_fld_defn` is given.
#' @param dsco Optional character vector of format-specific creation options
#' for `dsn` (`"NAME=VALUE"` pairs).
#' @param lco Optional character vector of format-specific creation options
#' for `layer` (`"NAME=VALUE"` pairs).
#' @param sql Character string containing an SQL statement (see Note).
#' @param spatial_filter Either a numeric vector of length four containing a
#' bounding box (xmin, ymin, xmax, ymax), or a character string containing a
#' geometry as OGC WKT, representing a spatial filter.
#' @param dialect Character string specifying the SQL dialect to use.
#' The OGR SQL engine (`"OGRSQL"`) will be used by default if a value is not
#' given. The `"SQLite"` dialect can also be used (see Note).
#'
#' @note
#' The OGR SQL document linked under **See Also** contains information on the
#' SQL dialect supported internally by OGR. Some format drivers (e.g., PostGIS)
#' pass the SQL directly through to the underlying RDBMS (unless `OGRSQL` is
#' explicitly passed as the dialect). The SQLite dialect can also be requested
#' with the `SQLite` string passed as the `dialect` argument of
#' `ogr_execute_sql()`. This assumes that GDAL/OGR is built with support for
#' SQLite, and preferably also with Spatialite support to benefit from spatial
#' functions. The GDAL document for the SQLite dialect has detailed information.
#'
#' Other SQL dialects may also be present for some vector formats.
#' For example, the `"INDIRECT_SQLITE"` dialect could potentially be used with
#' GeoPackage format ([https://gdal.org/drivers/vector/gpkg.html#sql]).
#'
#' `ogrinfo()` can also be used to edit data with SQL statements (GDAL >= 3.7).
#'
#' @seealso
#' [ogrinfo()], [ogr2ogr()]
#'
#' WKT representation of geometry:\cr
#' [https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry]
#'
#' OGR SQL dialect:\cr
#' [https://gdal.org/user/ogr_sql_dialect.html]
#'
#' SQLite dialect:\cr
#' [https://gdal.org/user/sql_sqlite_dialect.html#sql-sqlite-dialect]
#'
#' @export
ogr_ds_exists <- function(dsn, with_update = FALSE) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    return(.ogr_ds_exists(dsn, with_update))
}

#' @name ogr_manage
#' @export
ogr_ds_test_cap <- function(dsn, with_update = TRUE) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    return(.ogr_ds_test_cap(dsn, with_update))
}

#' @name ogr_manage
#' @export
ogr_ds_create <- function(format, dsn, layer = NULL, layer_defn = NULL,
                          geom_type = NULL, srs = NULL, fld_name = NULL,
                          fld_type = NULL, dsco = NULL, lco = NULL) {

    if (!(is.character(format) && length(format) == 1))
        stop("'format' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (is.null(layer))
        layer <- ""
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (is.null(geom_type))
        geom_type <- ""
    if (!(is.character(geom_type) && length(geom_type) == 1))
        stop("'geom_type' must be a length-1 character vector", call. = FALSE)
    if (is.null(srs))
        srs <- ""
    if (is.null(fld_name))
        fld_name <- ""
    if (is.null(fld_type))
        fld_type <- ""

    if (is.null(layer_defn)) {
        return(.create_ogr(format, dsn, 0, 0, 0, "Unknown",
                           layer, geom_type, srs, fld_name, fld_type,
                           dsco, lco))
    } else {
        ds_ok <- .create_ogr(format, dsn, 0, 0, 0, "Unknown",
                             layer = "", geom_type = "", srs = "",
                             fld_name = "", fld_type = "",
                             dsco = dsco, lco = lco)
        if (!ds_ok) {
            return(FALSE)

        } else {
            return(ogr_layer_create(dsn, layer, layer_defn, lco = lco))

        }
    }
}

#' @name ogr_manage
#' @export
ogr_ds_layer_count <- function(dsn) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    return(.ogr_ds_layer_count(dsn))
}

#' @name ogr_manage
#' @export
ogr_ds_layer_names <- function(dsn) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    return(.ogr_ds_layer_names(dsn))
}

#' @name ogr_manage
#' @export
ogr_layer_exists <- function(dsn, layer) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)

    return(.ogr_layer_exists(dsn, layer))
}

#' @name ogr_manage
#' @export
ogr_layer_create <- function(dsn, layer, layer_defn = NULL, geom_type = NULL,
                             srs = NULL, lco = NULL) {

    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!ogr_ds_exists(dsn, with_update = TRUE))
        stop("'dsn' does not exist, or no update access", call. = FALSE)
    if (ogr_layer_exists(dsn, layer))
        stop("'layer' already exists", call. = FALSE)

    if (!is.null(layer_defn)) {
        if (!is.list(layer_defn))
            stop("'layer_defn' must be a list", call. = FALSE)

        # using layer_defn so get geom_type and srs from the geom field defn
        has_geom_fld_defn <- FALSE
        for (nm in names(layer_defn)) {
            if (is.null(layer_defn[[nm]]$is_geom) ||
                    !is.logical(layer_defn[[nm]]$is_geom)) {
                stop("field definitions must contain `$is_geom=TRUE|FALSE`",
                     call. = FALSE)
            }
            if (layer_defn[[nm]]$is_geom) {
                geom_type <- layer_defn[[nm]]$type
                srs <- layer_defn[[nm]]$srs
                has_geom_fld_defn <- TRUE
                break
            }
        }
        if (!has_geom_fld_defn) {
            stop("'layer_defn' is missing a geometry field definition",
                 call. = FALSE)
        }
    }

    if (is.null(geom_type))
        stop("'geom_type' is required", call. = FALSE)
    else
        geom_type <- geom_type

    if (is.null(srs))
        srs <- ""

    ret <- .ogr_layer_create(dsn, layer, geom_type, srs, lco)
    if (!ret)
        return(FALSE)

    # create fields if layer_defn given
    if (!is.null(layer_defn)) {
        geom_fld_count <- 0
        for (nm in names(layer_defn)) {
            if (!layer_defn[[nm]]$is_geom) {
                ret <- ogr_field_create(dsn, layer, nm, layer_defn[[nm]])
                if (!ret)
                    warning("failed to create field ", nm, call. = FALSE)
            } else {
                if (geom_fld_count == 0) {
                    geom_fld_count <- 1
                    next
                } else {
                    geom_fld_count <- geom_fld_count + 1
                    ret <- ogr_geom_field_create(dsn, layer, nm,
                                                 layer_defn[[nm]])
                    if (!ret)
                        warning("failed to create geom field ", nm,
                                call. = FALSE)
                }
            }
        }
    }

    return(TRUE)
}

#' @name ogr_manage
#' @export
ogr_layer_fld_names <- function(dsn, layer) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)

    return(.ogr_layer_fld_names(dsn, layer))
}

#' @name ogr_manage
#' @export
ogr_layer_delete <- function(dsn, layer) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)

    return(.ogr_layer_delete(dsn, layer))
}

#' @name ogr_manage
#' @export
ogr_field_index <- function(dsn, layer, fld_name) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)

    return(.ogr_field_index(dsn, layer, fld_name))
}

#' @name ogr_manage
#' @export
ogr_field_create <- function(dsn, layer, fld_name,
                             fld_defn = NULL,
                             fld_type = "OFTInteger",
                             fld_subtype = "OFSTNone",
                             fld_width = 0,
                             fld_precision = 0,
                             is_nullable = TRUE,
                             is_ignored = FALSE,
                             is_unique = FALSE,
                             default_value = "") {

    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)

    if (!ogr_ds_exists(dsn, with_update = TRUE))
        stop("'dsn' does not exist, or no update access", call. = FALSE)
    if (!ogr_layer_exists(dsn, layer))
        stop("'layer' does not exist", call. = FALSE)

    fld_idx <- ogr_field_index(dsn, layer, fld_name)
    if (fld_idx >= 0)
        stop("field '", fld_name, "' already exists", call. = FALSE)

    # use field definition if given
    if (!is.null(fld_defn)) {
        if (!is.list(fld_defn))
            stop("'fld_defn' must be a list", call. = FALSE)

        if (is.null(fld_defn$type))
            stop("'fld_defn$type' is required", call. = FALSE)
        else
            fld_type <- fld_defn$type

        if (!is.null(fld_defn$subtype))
            fld_subtype <- fld_defn$subtype
        else
            fld_subtype <- "OFSTNone"

        if (!is.null(fld_defn$width))
            fld_width <- fld_defn$width
        else
            fld_width <- 0

        if (!is.null(fld_defn$precision))
            fld_precision <- fld_defn$precision
        else
            fld_precision <- 0

        if (!is.null(fld_defn$is_nullable))
            is_nullable <- fld_defn$is_nullable
        else
            is_nullable <- TRUE

        if (!is.null(fld_defn$is_ignored))
            is_ignored <- fld_defn$is_ignored
        else
            is_ignored <- FALSE

        if (!is.null(fld_defn$is_unique))
            is_unique <- fld_defn$is_unique
        else
            is_unique <- FALSE

        if (!is.null(fld_defn$default))
            default_value <- fld_defn$default
        else
            default_value <- ""
    }

    return(.ogr_field_create(dsn, layer, fld_name, fld_type, fld_subtype,
                             fld_width, fld_precision, is_nullable,
                             is_ignored, is_unique, default_value))
}

#' @name ogr_manage
#' @export
ogr_geom_field_create <- function(dsn, layer, fld_name,
                                  geom_fld_defn = NULL,
                                  geom_type = NULL,
                                  srs = NULL,
                                  is_nullable = TRUE,
                                  is_ignored = FALSE) {

    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)

    if (!ogr_ds_exists(dsn, with_update = TRUE))
        stop("'dsn' does not exist, or no update access", call. = FALSE)
    if (!ogr_layer_exists(dsn, layer))
        stop("'layer' does not exist", call. = FALSE)

    fld_idx <- ogr_field_index(dsn, layer, fld_name)
    if (fld_idx == -1)
        stop("field ", fld_name, " already exists", call. = FALSE)

    if (is.null(geom_type))
        geom_type <- "UNKNOWN"
    else if (!(is.character(geom_type) && length(geom_type) == 1))
        stop("'geom_type' must be a length-1 character vector", call. = FALSE)

    if (is.null(srs))
        srs <- ""
    else if (!(is.character(srs) && length(srs) == 1))
        stop("'srs' must be a length-1 character vector", call. = FALSE)

    # use field definition if given
    if (!is.null(geom_fld_defn)) {
        if (!is.list(geom_fld_defn))
            stop("'geom_fld_defn' must be a list", call. = FALSE)

        if (is.null(geom_fld_defn$type))
            stop("'geom_fld_defn$type' is required", call. = FALSE)
        else
            geom_type <- geom_fld_defn$type

        if (is.null(geom_fld_defn$srs))
            srs <- ""
        else
            srs <- geom_fld_defn$srs

        if (!is.null(geom_fld_defn$is_nullable))
            is_nullable <- geom_fld_defn$is_nullable
        else
            is_nullable <- TRUE

        if (!is.null(geom_fld_defn$is_ignored))
            is_ignored <- geom_fld_defn$is_ignored
        else
            is_ignored <- FALSE
    }

    return(.ogr_geom_field_create(dsn, layer, fld_name, geom_type,
                                  is_nullable, is_ignored))
}

#' @name ogr_manage
#' @export
ogr_field_delete <- function(dsn, layer, fld_name) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)

    return(.ogr_field_delete(dsn, layer, fld_name))
}

#' @name ogr_manage
#' @export
ogr_execute_sql <- function(dsn, sql, spatial_filter = NULL, dialect = NULL) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(sql) && length(sql) == 1))
        stop("'sql' must be a length-1 character vector", call. = FALSE)

    if (!is.null(spatial_filter)) {
        if (is.numeric(spatial_filter) && length(spatial_filter) == 4) {
            spatial_filter <- bbox_to_wkt(spatial_filter)
        }
        if (!(is.character(spatial_filter) && length(spatial_filter) == 1)) {
            stop("spatial_filter must be length-4 numeric or character string",
                 call. = FALSE)
        }
    }

    if (!is.null(dialect)) {
        if (!(is.character(dialect) && length(dialect) == 1))
            stop("'dialect' must be a length-1 character vector", call. = FALSE)
    }

    return(.ogr_execute_sql(dsn, sql, spatial_filter, dialect))
}
