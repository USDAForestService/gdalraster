# This file contains the R interface for src/ogr_util.h.
# Chris Toney <chris.toney at usda.gov>

#' Utility functions for managing vector data sources
#'
#' Bindings to OGR wrap portions of GDAL's Vector API (ogr_core.h and
#' ogr_api.h).
#' This set of OGR management functions can be used to create new vector
#' datasets, test existence of dataset/layer/field, test read/write access,
#' create new layers in an existing dataset, delete layers, and create new
#' attribute and geometry fields in an existing layer. They are intended to be
#' complementary to `ogrinfo()` and `ogr2ogr()`, and to support implementation
#' of OGR bindings for vector I/O in a future release of {gdalraster}.
#'
#' @name ogr_manage
#' @details
#' `ogr_ds_exists()` tests whether a vector dataset can be opened with the
#' given DSN, potentially testing for update access.
#'
#' `ogr_ds_create()` creates a new vector data store, optionally creating
#' also a layer, and optionally creating one or more fields on the layer.
#' The attribute fields and geometry field(s) to create can be specified as a
#' feature class definition (`layer_defn` as list, see below), or
#' alternatively, by giving the `geom_type` and `srs`, optionally along with
#' one `fld_name` and `fld_type` to be created in the layer.
#'
#' `ogr_ds_layer_count()` returns the number of layers in a GDAL vector dataset.
#'
#' `ogr_layer_exists()` tests whether a layer can be accessed by name for a
#' given DSN.
#'
#' `ogr_layer_create()` creates a new layer in a vector dataset, with a
#' specified geometry type and spatial reference definition. This function also
#' accepts a feature class definition given as a list of field names and their
#' definitions (see below).
#'
#' `ogr_layer_delete()` deletes a layer in a vector dataset.
#'
#' `ogr_field_index()` tests for existence of an attribute field by name.
#' Returns the field index on the layer, or `-1` if the field does not exist.
#'
#' `ogr_field_create()` creates a new attribute field of specified data type in
#' a given DSN/layer. Several optional field properties can be specified in
#' addition to the type.
#'
#' `ogr_geom_field_create()` creates a new geometry field of specified type in
#' a given DSN/layer.
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
#' An OGR field type is specified as a case-sensitive character string with
#' possible values:
#' `OFTInteger`, `OFTIntegerList`, `OFTReal`, `OFTRealList`, `OFTString`,
#' `OFTStringList`, `OFTBinary`,  `OFTDate`, `OFTTime`, `OFTDateTime`,
#' `OFTInteger64`, `OFTInteger64List`.
#'
#' An optional field subtype is specified as a case-sensitive character string
#' with possible values:
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
#' Geometry types are specified as a character string containing OGC WKT
#' (case-insensitive).
#' Common types include: `Point`, `LineString`, `Polygon`, `MultiPoint`,
#' `MultiLineString`, `MultiPolygon`. See the GDAL documentation for a list
#' of all supported geometry types
#' ([https://gdal.org/api/vector_c_api.html#_CPPv418OGRwkbGeometryType]).
#'
#' @param dsn Character string. The vector data source name, e.g., a filename
#' or database connection string.
#' @param with_update Logical scalar. `TRUE` to test for update access on the
#' dataset, or `FALSE` (the default) to test for existence regardless of
#' read/write access.
#' @param format GDAL short name of the vector format as character string.
#' Examples of some common output formats include: `"GPKG"`, `"FlatGeobuf"`,
#' `"PostgreSQL"`, `"ESRI Shapefile"`, `"SQLite"`.
#' @param layer Character string for a layer name in a vector dataset.
#' @param layer_defn A feature class definition for `layer` as a list of
#' attribute field definition(s), and at least one geometry field definition.
#' Each field definition is a list containing the field name, type and other
#' properties (see Details). If a `layer_defn` is given, it will be used and
#' any additional values passed for defining a single field will be ignored.
#' The first geometry field definition in `layer_defn` defines the latyer
#' `geom_type` and `srs`.
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
#' @return Functions for create/delete return a logical scalar indicating
#' success or failure of the operation. Functions that test existence also
#' return a logical scalar. Otherwise, an integer scalar (see Details).
#'
#' @seealso
#' [https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry]
#'
#' @note
#'
#' @export
ogr_ds_exists <- function(dsn, with_update = FALSE) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    return(.ogr_ds_exists(dsn, with_update))
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
                stop("field definition must contain '$is_geom' as logical",
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
    if (fld_idx == -1)
        stop("field ", fld_name, " already exists", call. = FALSE)

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
