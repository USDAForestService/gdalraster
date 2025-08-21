# Documentation and helper functions for OGR feature class definition
# Chris Toney <chris.toney at usda.gov>

#' OGR feature class definition for vector data
#'
#' This topic contains documentation and helper functions for defining an
#' OGR feature class.
#' A named list containing zero or more attribute field definitions, along with
#' one or more geometry field definitions, comprise an OGR feature class
#' definition (a.k.a. layer definition). `ogr_def_layer()` initializes such a
#' list with the geometry type and (optionally) a spatial reference system.
#' Attribute fields may then be added to the layer definition.
#' `ogr_def_field()` creates an attribute field definition, a list
#' containing the field's data type and potentially other optional field
#' properties.
#' `ogr_def_geom_field()` similarly creates a geometry field definition. This
#' might be used with certain vector formats that support multiple geometry
#' columns (e.g., PostGIS).
#' `ogr_def_field_domain()` creates a field domain definition. A field domain
#' is a set of constraints that apply to one or several fields. This is a
#' concept found, e.g., in ESRI File Geodatabase and in GeoPackage via the
#' Schema extension (see \url{https://github.com/OSGeo/gdal/pull/3638}).
#' GDAL >= 3.3 supports reading and writing field domains with certain drivers
#' (e.g., GPKG and OpenFileGDB).
#'
#' @name ogr_define
#' @details
#' All features in an OGR Layer share a common schema (feature class), modeled
#' in GDAL by its `OGRFeatureDefn` class. A feature class definition includes
#' the set of attribute fields and their data types and the geometry field(s).
#' In \R, a feature class definition is represented as a named list, with names
#' being the attribute/geometry field names, and each list element holding an
#' attribute or geometry field definition.
#'
#' The definition for an attribute field is a named list with elements:
#' ```
#' $type       : OGR Field Type ("OFTReal", "OFTString" etc.)
#' $subtype    : optional ("OFSTBoolean", ...)
#' $width      : optional max number of characters
#' $precision  : optional number of digits after the decimal point
#' $is_nullable: optional NOT NULL constraint (logical value)
#' $is_unique  : optional UNIQUE constraint (logical value)
#' $default    : optional default value as character string
#' $domain     : optional field domain name
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
#' By default, fields are nullable and have no unique constraint. Not-null and
#' unique constraints are not supported by all format drivers.
#'
#' A default field value is taken into account by format drivers (generally
#' those with a SQL interface) that support it at field creation time.
#' If given in the field definition, `default` must be a character string.
#' The accepted values are `"NULL"`, a numeric value (e.g., `"0"`), a literal
#' value enclosed between single quote characters (e.g., `"'a default value'"`,
#' with any inner single quote characters escaped by repetition of the single
#' quote character), `"CURRENT_TIMESTAMP"`, `"CURRENT_TIME"`, `"CURRENT_DATE"`
#' or a driver-specific expression (that might be ignored by other drivers).
#' For a datetime literal value, format should be
#' `"'YYYY/MM/DD HH:MM:SS[.sss]'"` (considered as UTC time).
#'
#' The definition for a geometry field is a named list with elements:
#' ```
#' $type       : geom type ("Point", "Polygon", etc.)
#' $srs        : optional spatial reference as WKT string
#' $is_nullable: optional NOT NULL constraint (logical value)
#' $is_geom    : TRUE (required) for geometry fields
#' ```
#'
#' Typically, there is one geometry field on a layer, but some formats support
#' more than one geometry column per table (e.g., "PostgreSQL / PostGIS" and
#' "SQLite / Spatialite RDBMS").
#'
#' Geometry types are specified as a character string containing OGC WKT.
#' Common types include: `Point`, `LineString`, `Polygon`, `MultiPoint`,
#' `MultiLineString`, `MultiPolygon`. See the GDAL documentation for a list
#' of all supported geometry types:\cr
#' \url{https://gdal.org/en/stable/api/vector_c_api.html#_CPPv418OGRwkbGeometryType}
#'
#' Format drivers may or may not support not-null constraints on attribute and
#' geometry fields. If they support creating fields with not-null constraints,
#' this is generally before creating any features to the layer. In some cases,
#' a not-null constraint may be available as a layer creation option. For
#' example, GeoPackage format has a layer creation option
#' `GEOMETRY_NULLABLE=[YES/NO]`.
#'
#' The definition for a field domain is a named list with elements:
#' ```
#' $type             : domain type ("Coded", "Range", "RangeDateTime", "GLOB")
#' $domain_name      : name of the field domain (character string)
#' $description      : optional domain description (character string)
#' $field_type       : OGR Field Type (see attribute field definitions above)
#' $field_subtype    : optional OGR Field Subtype ("OFSTBoolean", ...)
#' $split_policy     : split policy of the field domain (see below)
#' $merge_policy     : merge policy of the field domain (see below)
#' $coded_values     : vector of allowed codes, or data frame of (codes, values)
#' $min_value        : minimum value (data type compatible with $field_type)
#' $min_is_inclusive : whether the minimum value is included in the range
#' $max_value        : maximum value (data type compatible with $field_type)
#' $max_is_inclusive : whether the maximum value is included in the range
#' $glob             : GLOB expression (character string)
#' ```
#'
#' A field domain can be one of three types:
#' * `Coded`: an enumerated list of allowed codes with their descriptive values
#' * `Range`: a range constraint (min, max)
#' * `GLOB`: a GLOB expression (matching expression like `"*[a-z][0-1]?"`)
#'
#' `type` can also be specified as `"RangeDateTime"`, a range constraint for
#' `OFTDateTime` fields with `min_value` and `max_value` given as
#' `"POSIXct"` DateTimes.
#'
#' Split and merge policies are supported by ESRI File Geodatabase format via
#' OpenFileGDB driver (or FileGDB driver dependent on FileGDB API library).
#' When a feature is split in two, `split_policy` defines how the value of
#' attributes following the domain are computed. Possible values are
#' `"DEFAULT_VALUE"` (default value), `"DUPLICATE"` (duplicate), and
#' `"GEOMETRY_RATIO"` (new values are computed by the ratio of their
#' area/length compared to the area/length of the original feature).
#' When a feature is built by merging two features, `merge_policy` defines
#' how the value of attributes following the domain are computed. Possible
#' values are `"DEFAULT_VALUE"` (default value), `"SUM"` (sum), and
#' `"GEOMETRY_WEIGHTED"` (new values are computed as the weighted average of
#' the source values).
#'
#' @param geom_type Character string specifying a geometry type (see Details).
#' @param geom_fld_name Character string specifying a geometry field name
#' Defaults to `"geom"`.
#' @param srs Character string containing a spatial reference system definition
#' as OGC WKT or other well-known format (e.g., the input formats usable with
#' [srs_to_wkt()]).
#' @param fld_type Character string containing the name of a field data type
#' (e.g., `"OFTInteger"`, `"OFTInteger64"`, `"OFTReal"`, `"OFTString"`, ...).
#' @param fld_subtype Character string containing the name of a field subtype.
#' One of `"OFSTNone"` (the default), `"OFSTBoolean"`, `"OFSTInt16"`,
#' `"OFSTFloat32"`, `"OFSTJSON"`, `"OFSTUUID"`.
#' @param fld_width Optional integer value specifying max number of characters.
#' @param fld_precision Optional integer value specifying number of digits
#' after the decimal point.
#' @param is_nullable Optional NOT NULL field constraint (logical value).
#' Defaults to `TRUE`.
#' @param is_unique Optional UNIQUE constraint on the field (logical value).
#' Defaults to `FALSE`.
#' @param default_value Optional default value for the field given as a
#' character string.
#' @param domain_type Character string specifying a field domain type (see
#' Details). Must be one of `"Coded"`, `"Range"`, `"RangeDateTime"`, `"GLOB"`
#' (case-insensitive).
#' @param domain_name Character string specifying the name of the field domain.
#' Optional for `ogr_def_field()`, required for `ogr_def_field_domain()`.
#' @param description Optional character string giving a description of the
#' field domain.
#' @param split_policy Character string specifying the split policy of the
#' field domain. One of `"DEFAULT_VALUE"`, `"DUPLICATE"`, `"GEOMETRY_RATIO"`
#' (supported by ESRI File Geodatabase format via OpenFileGDB driver).
#' @param merge_policy Character string specifying the merge policy of the
#' field domain. One of `"DEFAULT_VALUE"`, `"SUM"`, `"GEOMETRY_WEIGHTED"`
#' (supported by ESRI File Geodatabase format via OpenFileGDB driver).
#' @param coded_values Either a vector of the allowed codes, or character
#' vector of `"CODE=VALUE"` pairs (the expanded "value" associated with a code
#' is optional), or a two-column data frame with (codes, values).
#' If data frame, the second column of values must be character type (i.e.,
#' the descriptive text for each code). This argument is required if
#' `domain_type = "Coded"`. Each code should appear only once, but it is the
#' responsibility of the user to check it.
#' @param range_min Minimum value in a Range or RangeDateTime field domain (can
#' be NULL). The data type must be consistent with with the field type given in
#' the `fld_type` argument.
#' @param min_is_inclusive = Logical value, whether the minimum value is
#' included in the range. Defaults to `TRUE`. Required argument if
#' `domain_type` is `"Range"` or `"RangeDateTime"`.
#' @param range_max Maximum value in a Range or RangeDateTime field domain (can
#' be NULL). The data type must be consistent with with the field type given in
#' the `fld_type` argument.
#' @param max_is_inclusive = Logical value, whether the maximum value is
#' included in the range. Defaults to `TRUE`. Required argument if
#' `domain_type` is `"Range"` or `"RangeDateTime"`.
#' @param glob Character string containing the GLOB expression. Required if
#' `domain_type` is `"GLOB"`.
#'
#' @note
#' The feature id (FID) is a special property of a feature and not treated as
#' an attribute of the feature. Additional information is given in the GDAL
#' documentation for the
#' [OGR SQL](https://gdal.org/en/stable/user/ogr_sql_dialect.html#feature-id-fid) and
#' [SQLite](https://gdal.org/en/stable/user/sql_sqlite_dialect.html#feature-id-fid)
#' SQL dialects. Implications for SQL statements and result sets may depend
#' on the dialect used.
#'
#' Some vector formats do not support schema definition prior to creating
#' features. For example, with GeoJSON only the *Feature* object has a member
#' with name *properties*. The specification does not require all *Feature*
#' objects in a collection to have the same schema of properties, nor does
#' it require all *Feature* objects in a collection to have geometry of the
#' same type (\url{https://geojson.org/}).
#'
#' @seealso
#' [ogr_ds_create()], [ogr_layer_create()], [ogr_field_create()]
#'
#' WKT representation of geometry:\cr
#' \url{https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry}
#'
#' Field domains:\cr
#' \url{https://desktop.arcgis.com/en/arcmap/latest/manage-data/geodatabases/an-overview-of-attribute-domains.htm}\cr
#' \url{https://www.geopackage.org/spec/#extension_schema}\cr
#' \url{https://gdal.org/en/stable/doxygen/classOGRFieldDomain.html#details}
#'
#' @examples
#' # create a SQLite data source, with SpatiaLite extensions if available
#' dsn <- file.path(tempdir(), "test.sqlite")
#' opt <- NULL
#' if (has_spatialite()) {
#'   opt <- "SPATIALITE=YES"
#' }
#'
#' # This creates an empty data source. Note that we could also create a layer
#' # at the same time in this function call, but for this example we do that
#' # separately, to show creation of a layer on an existing data source.
#' ogr_ds_create("SQLite", dsn, dsco = opt)
#'
#' # define a layer
#' defn <- ogr_def_layer("Point", srs = "EPSG:4326")
#' defn$my_id <- ogr_def_field("OFTInteger64")
#' defn$my_description <- ogr_def_field("OFTString")
#'
#' # create a layer in the existing data source
#' ogr_ds_test_cap(dsn)$CreateLayer  # TRUE
#'
#' ogr_layer_create(dsn, "layer1", layer_defn = defn)
#'
#' ogr_ds_layer_names(dsn)
#'
#' ogr_layer_field_names(dsn, "layer1")
#'
#' deleteDataset(dsn)
#' @export
ogr_def_layer <- function(geom_type, geom_fld_name = "geom", srs = NULL) {

    defn <- list()

    if (missing(srs) || is.null(srs))
        srs <- ""

    if (missing(geom_fld_name) || is.null(geom_fld_name))
        geom_fld_name <- "geom"

    if (!(is.character(geom_fld_name) && length(geom_fld_name) == 1))
        stop("'geom_fld_name' must be a length-1 character vector",
             call. = FALSE)
    else
        defn[[geom_fld_name]] <- ogr_def_geom_field(geom_type, srs)

    return(defn)
}

#' @name ogr_define
#' @export
ogr_def_field <- function(fld_type, fld_subtype = NULL, fld_width = NULL,
                          fld_precision = NULL, is_nullable = NULL,
                          is_unique = NULL, default_value = NULL,
                          domain_name = NULL) {

    defn <- list()

    if (!(is.character(fld_type) && length(fld_type) == 1))
        stop("'fld_type' must be a length-1 character vector", call. = FALSE)
    else
        defn$type <- fld_type

    if (!is.null(fld_subtype)) {
        if (!(is.character(fld_subtype) && length(fld_subtype) == 1))
            stop("'fld_subtype' must be a length-1 character vector",
                 call. = FALSE)
        else
            defn$subtype <- fld_subtype
    }

    if (!is.null(fld_width)) {
        if (!(is.numeric(fld_width) && length(fld_width) == 1))
            stop("'fld_width' must be an integer scalar", call. = FALSE)
        else
            defn$width <- fld_width
    }

    if (!is.null(fld_precision)) {
        if (!(is.numeric(fld_precision) && length(fld_precision) == 1))
            stop("'fld_precision' must be an integer scalar", call. = FALSE)
        else
            defn$precision <- fld_precision
    }

    if (!is.null(is_nullable)) {
        if (!(is.logical(is_nullable) && length(is_nullable) == 1))
            stop("'is_nullable' must be a single logical value", call. = FALSE)
        else
            defn$is_nullable <- is_nullable
    }

    if (!is.null(is_unique)) {
        if (!(is.logical(is_unique) && length(is_unique) == 1))
            stop("'is_unique' must be a single logical value", call. = FALSE)
        else
            defn$is_unique <- is_unique
    }

    if (!is.null(default_value)) {
        default_value <- as.character(default_value)
        if (!(length(default_value) == 1))
            stop("'default_value' must be a length-1 character vector",
                 call. = FALSE)
        else
            defn$default <- default_value
    }

    if (!is.null(domain_name)) {
        if (!(is.character(domain_name) && length(domain_name) == 1))
            stop("'domain_name' must be a length-1 character vector",
                 call. = FALSE)
        else
            defn$domain <- domain_name
    }

    defn$is_geom <- FALSE

    return(defn)
}

#' @name ogr_define
#' @export
ogr_def_geom_field <- function(geom_type, srs = NULL, is_nullable = NULL) {

    defn <- list()

    if (!(is.character(geom_type) && length(geom_type) == 1))
        stop("'geom_type' must be a length-1 character vector", call. = FALSE)
    else
        defn$type <- geom_type

    if (missing(srs) || is.null(srs))
        srs <- ""

    if (!(is.character(srs) && length(srs) == 1))
        stop("'srs' must be a length-1 character vector", call. = FALSE)
    else
        defn$srs <- srs


    if (!is.null(is_nullable)) {
        if (!(is.logical(is_nullable) && length(is_nullable) == 1))
            stop("'is_nullable' must be a logical value", call. = FALSE)
        else
            defn$is_nullable <- is_nullable
    }

    defn$is_geom <- TRUE

    return(defn)
}

#' @name ogr_define
#' @export
ogr_def_field_domain <- function(domain_type, domain_name, description = NULL,
                                 fld_type = NULL, fld_subtype = "OFSTNone",
                                 split_policy = "DEFAULT_VALUE",
                                 merge_policy = "DEFAULT_VALUE",
                                 coded_values = NULL,
                                 range_min = NULL, min_is_inclusive = TRUE,
                                 range_max = NULL, max_is_inclusive = TRUE,
                                 glob = "") {

    if (gdal_version_num() <= gdal_compute_version(3, 3, 0)) {
        warning("GDAL >= 3.3 is required for writing field domains",
                call. = FALSE)
    }

    defn <- list()

    if (!(is.character(domain_type) && length(domain_type) == 1)) {
        stop("'domain_type' must be a length-1 character vector", call. = FALSE)
    }
    if (is.na(domain_type) || !nzchar(domain_type)) {
        stop("'domain_type' is required", call. = FALSE)
    }
    if (!(tolower(domain_type) %in%
            c("coded", "range", "rangedatetime", "glob"))) {

        cat("valid domain types are:\n")
        cat("  \"Coded\", \"Range\", \"RangeDateTime\", \"GLOB\"\n")
        stop("invalid 'domain_type'", call. = FALSE)
    }
    defn$type <- domain_type

    if (!(is.character(domain_name) && length(domain_name) == 1)) {
        stop("'domain_name' must be a length-1 character vector", call. = FALSE)
    }
    if (is.na(domain_name) || !nzchar(domain_name)) {
        stop("'domain_name' is required", call. = FALSE)
    }
    defn$domain_name <- domain_name

    if (missing(description) || is.null(description) || is.na(description)) {
        description <- ""
    } else if (!(is.character(description) && length(description) == 1)) {
        stop("'description' must be a length-1 character vector", call. = FALSE)
    }
    defn$description <- description

    if (missing(fld_type) || is.null(fld_type) || is.na(fld_type)) {
        stop("'fld_type' is required", call. = FALSE)
    }
    if (!(is.character(fld_type) && length(fld_type) == 1)) {
        stop("'fld_type' must be a length-1 character vector", call. = FALSE)
    }
    defn$field_type <- fld_type

    if (missing(fld_subtype) || is.null(fld_subtype) || is.na(fld_subtype)) {
        fld_subtype <- "OFSTNone"
    } else if (!(is.character(fld_subtype) && length(fld_subtype) == 1)) {
        stop("'fld_subtype' must be a length-1 character vector",
             call. = FALSE)
    }
    defn$field_subtype <- fld_subtype

    if (!(is.character(split_policy) && length(split_policy) == 1)) {
        stop("'split_policy' must be a length-1 character vector",
             call. = FALSE)
    }
    if (is.na(split_policy) || !nzchar(split_policy)) {
        split_policy <- "DEFAULT_VALUE"
    }
    if (!(toupper(split_policy) %in%
            c("DEFAULT_VALUE", "DUPLICATE", "GEOMETRY_RATIO"))) {

        cat("valid split policies are:\n")
        cat("  \"DEFAULT_VALUE\", \"DUPLICATE\", \"GEOMETRY_RATIO\"\n")
        stop("invalid 'split_policy'", call. = FALSE)
    }
    defn$split_policy <- toupper(split_policy)

    if (!(is.character(merge_policy) && length(merge_policy) == 1)) {
        stop("'merge_policy' must be a length-1 character vector",
             call. = FALSE)
    }
    if (is.na(merge_policy) || !nzchar(merge_policy)) {
        merge_policy <- "DEFAULT_VALUE"
    }
    if (!(toupper(merge_policy) %in%
            c("DEFAULT_VALUE", "SUM", "GEOMETRY_WEIGHTED"))) {

        cat("valid merge policies are:\n")
        cat("   \"DEFAULT_VALUE\", \"SUM\", \"GEOMETRY_WEIGHTED\"\n")
        stop("invalid 'merge_policy'", call. = FALSE)
    }
    defn$merge_policy <- toupper(merge_policy)

    defn["coded_values"] <- list(NULL)
    defn["min_value"] <- list(NULL)
    defn$min_is_inclusive <- TRUE
    defn["max_value"] <- list(NULL)
    defn$max_is_inclusive <- TRUE
    defn$glob <- ""

    if (tolower(domain_type) == "coded") {
        if (missing(coded_values) || is.null(coded_values)) {
            stop("'coded_values' is required for Coded domain type",
                 call. = FALSE)
        }
        if (is.data.frame(coded_values)) {
            # as two-column data frame of codes, values
            if (ncol(coded_values) != 2) {
                stop("'coded_values' data frame must have two columns",
                     call. = FALSE)
            }
            if (nrow(coded_values) == 0) {
                stop("'coded_values' is empty",
                     call. = FALSE)
            }
            if (any(is.na(coded_values[, 1]))) {
                stop("'coded_values' cannot contain NA codes", call. = FALSE)
            }
            coded_values[, 1] <- as.character(coded_values[, 1])
            coded_values[, 2] <- as.character(coded_values[, 2])
        } else {
            # as vector of codes, or "CODE=VALUE" pairs
            coded_values <- as.character(coded_values)
            if (length(coded_values) == 0) {
                stop("'coded_values' is empty", call. = FALSE)
            }
            if (any(is.na(coded_values))) {
                stop("'coded_values' cannot contain NA codes", call. = FALSE)
            }
        }
        defn$coded_values <- coded_values

    } else if (tolower(domain_type) == "range") {
        if (!is.null(range_min) && !all(is.na(range_min))) {
            if (!(is.numeric(range_min) && length(range_min) == 1)) {
                stop("'range_min' must be a single numeric value or NULL",
                     call. = FALSE)
            } else {
                defn$min_value <- range_min
            }
        }
        if (!(is.logical(min_is_inclusive) && length(min_is_inclusive) == 1)) {
            stop("'min_is_inclusive' must be a single logical value",
                 call. = FALSE)
        } else {
            defn$min_is_inclusive <- min_is_inclusive
        }
        if (!is.null(range_max) && !all(is.na(range_max))) {
            if (!(is.numeric(range_max) && length(range_max) == 1)) {
                stop("'range_max' must be a single numeric value or NULL",
                     call. = FALSE)
            } else {
                defn$max_value <- range_max
            }
        }
        if (!(is.logical(max_is_inclusive) && length(max_is_inclusive) == 1)) {
            stop("'max_is_inclusive' must be a single logical value",
                 call. = FALSE)
        } else {
            defn$max_is_inclusive <- max_is_inclusive
        }

    } else if (tolower(domain_type) == "rangedatetime") {
        if (!is.null(range_min) && !all(is.na(range_min))) {
            if (!(is(range_min, "POSIXct") && length(range_min) == 1)) {
                stop("'range_min' must be a single POSIXct value or NULL",
                     call. = FALSE)
            } else {
                defn$min_value <- range_min
            }
        }
        if (!(is.logical(min_is_inclusive) && length(min_is_inclusive) == 1)) {
            stop("'min_is_inclusive' must be a single logical value",
                 call. = FALSE)
        } else {
            defn$min_is_inclusive <- min_is_inclusive
        }
        if (!is.null(range_max) && !all(is.na(range_max))) {
            if (!(is(range_max, "POSIXct") && length(range_max) == 1)) {
                stop("'range_max' must be a single POSIXct value or NULL",
                     call. = FALSE)
            } else {
                defn$max_value <- range_max
            }
        }
        if (!(is.logical(max_is_inclusive) && length(max_is_inclusive) == 1)) {
            stop("'max_is_inclusive' must be a single logical value",
                 call. = FALSE)
        } else {
            defn$max_is_inclusive <- max_is_inclusive
        }

    } else if (tolower(domain_type) == "glob") {
        if (missing(glob) || is.null(glob)) {
            stop("'glob' is required", call. = FALSE)
        }
        if (!(is.character(glob) && length(glob) == 1)) {
            stop("'glob' must be a length-1 character vector", call. = FALSE)
        }
        if (is.na(glob) || !nzchar(glob)) {
            stop("'glob' is required", call. = FALSE)
        }
        defn$glob <- glob

    } else {
        stop("'domain_type' not recognized", call. = FALSE)
    }

    return(defn)
}
