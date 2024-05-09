# Documentation and helper fucntion for OGR feature class definition
# Chris Toney <chris.toney at usda.gov>

#' OGR feature class definition for vector data
#'
#' This topic contains documentation and helper functions for defining an
#' OGR feature class.
#' `ogr_def_field()` creates an attribute field definition, a list
#' containing the field data type and potentially other optional field
#' properties.
#' `ogr_def_geom_field()` similarly creates a geometry field definition.
#' A list containing zero or more attribute field definitions, along with one
#' or more geomtery field definitions, comprise an OGR feature class definition
#' (a.k.a. layer definition).
#'
#' @name ogr_define
#' @details
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
#' By default, fields are nullable, have no unique constraint, and are not
#' ignored (i.e., not omitted when fetching features). Not-null and unique
#' constraints are not supported by all format drivers.
#'
#' A default field value is taken into account by format drivers (generally
#' those with a SQL interface) that support it at field creation time.
#' If given in the field definition, `$default` must be a character string.
#' The accepted values are `"NULL"`, a numeric value (e.g., `"0"`), a literal
#' value enclosed between single quote characters (e.g., `"'a default value'"`,
#' with any inner single quote characters escaped by repetition of the single
#' quote character), `"CURRENT_TIMESTAMP"`, `"CURRENT_TIME"`, `"CURRENT_DATE"`
#' or a driver-specific expression (that might be ignored by other drivers).
#' For a datetime literal value, format should be
#' `"'YYYY/MM/DD HH:MM:SS[.sss]'"` (considered as UTC time).
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
#' \url{https://gdal.org/api/vector_c_api.html#_CPPv418OGRwkbGeometryType}
#'
#' Format drivers may or may not support not-null constraints on attribute and
#' geometry fields. If they support creating fields with not-null constraints,
#' this is generally before creating any features to the layer. In some cases,
#' a not-null constraint may be available as a layer creation option. For
#' example, GeoPackage format has a layer creation option
#' `GEOMETRY_NULLABLE=[YES/NO]`.
#'
#' @seealso
#' the [ogr_manage] functions, [ogrinfo()]
#'
#' WKT representation of geometry:\cr
#' \url{https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry}
#'
#' @export
ogr_def_field <- function(fld_type, subtype = NULL, width = NULL,
                          precision = NULL, is_nullable = NULL,
                          is_unique = NULL, is_ignored = NULL,
                          default = NULL) {

    defn <- list()

    if (!(is.character(fld_type) && length(fld_type) == 1))
        stop("'fld_type' must be a length-1 character vector", call. = FALSE)
    else
        defn$type <- fld_type

    if (!is.null(subtype)) {
        if (!(is.character(subtype) && length(subtype) == 1))
            stop("'subtype' must be a length-1 character vector", call. = FALSE)
        else
            defn$subtype <- subtype
    }

    if (!is.null(width)) {
        if (!(is.integer(width) && length(width) == 1))
            stop("'width' must be an integer scalar", call. = FALSE)
        else
            defn$width <- width
    }

    if (!is.null(precision)) {
        if (!(is.integer(precision) && length(precision) == 1))
            stop("'precision' must be an integer scalar", call. = FALSE)
        else
            defn$precision <- precision
    }

    if (!is.null(is_nullable)) {
        if (!(is.logical(is_nullable) && length(is_nullable) == 1))
            stop("'is_nullable' must be a logical scalar", call. = FALSE)
        else
            defn$is_nullable <- is_nullable
    }

    if (!is.null(is_unique)) {
        if (!(is.logical(is_unique) && length(is_unique) == 1))
            stop("'is_unique' must be a logical scalar", call. = FALSE)
        else
            defn$is_unique <- is_unique
    }

    if (!is.null(is_ignored)) {
        if (!(is.logical(is_ignored) && length(is_ignored) == 1))
            stop("'is_ignored' must be a logical scalar", call. = FALSE)
        else
            defn$is_ignored <- is_ignored
    }

    if (!is.null(default)) {
        if (!(is.character(default) && length(default) == 1))
            stop("'default' must be a length-1 character vector", call. = FALSE)
        else
            defn$default <- default
    }

    defn$is_geom <- FALSE

    return(defn)
}

#' @name ogr_define
#' @export
ogr_def_geom_field <- function(geom_type, srs = NULL, is_nullable = NULL,
                               is_ignored = NULL) {

    defn <- list()

    if (!(is.character(geom_type) && length(geom_type) == 1))
        stop("'geom_type' must be a length-1 character vector", call. = FALSE)
    else
        defn$type <- geom_type

    if (!is.null(srs)) {
        if (!(is.character(srs) && length(srs) == 1))
            stop("'srs' must be a length-1 character vector", call. = FALSE)
        else
            defn$srs <- srs
    }

    if (!is.null(is_nullable)) {
        if (!(is.logical(is_nullable) && length(is_nullable) == 1))
            stop("'is_nullable' must be a logical scalar", call. = FALSE)
        else
            defn$is_nullable <- is_nullable
    }

    if (!is.null(is_ignored)) {
        if (!(is.logical(is_ignored) && length(is_ignored) == 1))
            stop("'is_ignored' must be a logical scalar", call. = FALSE)
        else
            defn$is_ignored <- is_ignored
    }

    defn$is_geom <- TRUE

    return(defn)
}
