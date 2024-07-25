# R interface to OGR vector geoprocessing
# Chris Toney <chris.toney at usda.gov>

#' GDAL OGR facilities for vector geoprocessing
#'
#' @description
#' `ogr_proc()` performs various geoprocessing operations on vector layers
#' (intersection, union, clip, etc). It provides an inteface to the `OGRLayer`
#' methods for these operations in the GDAL API (`OGRLayer::Intersection()`,
#' etc). Inputs are given as objects of class `GDALVector`, which might have
#' spatial and/or attribute filters applied.
#' The output layer will be created, but output can optionally be appended to an
#' existing layer, or written to an existing empty layer with custom schema
#' defined.
#' This function is basically an equivalent interface as the
#' [`ogr_layer_algebra`](https://gdal.org/programs/ogr_layer_algebra.html#ogr-layer-algebra)
#' utility in the GDAL Python bindings.
#'
#' @details
#' Seven processing modes are available:
#' * `Intersection`: Intersection of two layers. The output layer contains
#' features whose geometries represent areas that are common between features
#' in the input layer and in the method layer. The features in the output layer
#' have attributes from both input and method layers.
#' * `Union`: Union of two layers. The output layer contains features whose
#' geometries represent areas that are in either in the input layer, in the
#' method layer, or in both. The features in the output layer have attributes
#' from both input and method layers. For features which represent areas that
#' are only in the input or in the method layer the respective attributes have
#' undefined values.
#' * `SymDifference`: Symmetrical difference of two layers. The output layer
#' contains features whose geometries represent areas that are in either in the
#' input layer or in the method layer but not in both. The features in the
#' output layer have attributes from both input and method layers. For features
#' which represent areas that are only in the input or in the method layer the
#' respective attributes have undefined values.
#' * `Identity`: Identify the features of the input layer with the ones from the
#' method layer. The output layer contains features whose geometries represent
#' areas that are in the input layer. The features in the output layer have
#' attributes from both input and method layers.
#' * `Update`: The update method creates a layer which adds features into the
#' input layer from the method layer, possibly cutting features in the input
#' layer. By default the output layer has attributes only from the input layer.
#' * `Clip`: The clip method creates a layer which has features from the input
#' layer clipped to the areas of the features in the method layer. By default
#' the output layer has attributes of the input layer.
#' * `Erase`: The erase method creates a layer which has features from the input
#' layer whose areas are erased by the features in the method layer. By default,
#' the output layer has attributes of the input layer.
#'
#' The schema of the output layer can be set by the user or, if it is empty, is
#' initialized to contain all fields in the input layer. The functions in the
#' [ogr_manage] interface can be used to create an empty layer with user-defined
#' schema (e.g., [ogr_ds_create()], [ogr_layer_create()], [ogr_field_create()]).
#' If the schema of the output layer is set by the user and contains fields that
#' have the same name as a field in the input and in the method layer, then the
#' attribute for an output feature will get the value from the feature of the
#' method layer.
#'
#' Options that affect processing can be set as NAME=VALUE pairs passed in the
#' `mode_opt` argument. Some options are specific to certain processing modes
#' as noted below:
#' * SKIP_FAILURES=YES/NO. Set it to YES to go on, even when a feature could
#' not be inserted or a GEOS call failed.
#' * PROMOTE_TO_MULTI=YES/NO. Set to YES to convert Polygons into MultiPolygons,
#' LineStrings to MultiLineStrings or Points to MultiPoints (only since GDAL
#' 3.9.2 for the latter).
#' * INPUT_PREFIX=string. Set a prefix for the field names that will be created
#' from the fields of the input layer.
#' * METHOD_PREFIX=string. Set a prefix for the field names that will be created
#' from the fields of the method layer.
#' * USE_PREPARED_GEOMETRIES=YES/NO. Set to NO to not use prepared geometries to
#' pretest intersection of features of method layer with features of this layer.
#' Applies to `Intersection`, `Union`, `Identity`.
#' * PRETEST_CONTAINMENT=YES/NO. Set to YES to pretest the containment of
#' features of method layer within the features of this layer. This will speed
#' up the method significantly in some cases. Requires that the prepared
#' geometries are in effect. Applies to `Intersection`.
#' * KEEP_LOWER_DIMENSION_GEOMETRIES=YES/NO. Set to NO to skip result features
#' with lower dimension geometry that would otherwise be added to the output
#' layer. The default is YES, to add features with lower dimension geometry, but
#' only if the result output has an UNKNOWN geometry type. Applies to
#' `Intersection`, `Union`, `Identity`.
#'
#' @param mode Character string specifying the operation to perform. One of
#' `Union`, `Intersection`, `SymDifference`, `Identity`, `Update`, `Clip` or
#' `Erase` (see Details).
#' @param input_lyr An object of class `GDALVector` to use as the input layer.
#' For overlay operations, this is the first layer in the operation.
#' @param method_lyr An object of class `GDALVector` to use as the method
#' layer. This is the conditional layer supplied to an operation (e.g., Clip,
#' Erase, Update), or the second layer in overlay operations (e.g., Union,
#' Intersection, SymDifference).
#' @param out_dsn The destination vector filename or database connection string
#' to which the output layer will be written.
#' @param out_lyr_name Layer name where the output vector will be written. May
#' be `NULL` (e.g., shapefile), but generally must be specified.
#' @param out_geom_type Character string specifying the geometry type of the
#' output layer. One of NONE, GEOMETRY, POINT, LINESTRING, POLYGON,
#' GEOMETRYCOLLECTION, MULTIPOINT, MULTIPOLYGON, GEOMETRY25D, POINT25D,
#' LINESTRING25D, POLYGON25D, GEOMETRYCOLLECTION25D, MULTIPOINT25D,
#' MULTIPOLYGON25D.
#' @param out_fmt GDAL short name of the output vector format. If unspecified,
#' the function will attempt to guess the format from the filename/connection
#' string.
#' @param dsco Optional character vector of format-specific creation options
#' for `out_dsn` (`"NAME=VALUE"` pairs).
#' @param lco Optional character vector of format-specific creation options
#' for `out_layer` (`"NAME=VALUE"` pairs).
#' @param mode_opt Character vector of NAME=VALUE pairs that specify processing
#' options. Available options depend on the value of `mode` (see Details).
#' @param overwrite Logical scalar. `TRUE` to overwrite the output layer if it
#' already exists. Defaults to `FALSE`.
#' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
#' displayed. Defaults to `FALSE`.
#' @param return_lyr_obj Logical scalar. If `TRUE` (the default), an object of
#' class `GDALVector` opened on the ouput layer will be returned, otherwise
#' returns a logical value.
#'
#' @returns Upon successful completion, an object of class `GDALVector` is
#' returned by default (if `return_lyr_obj = TRUE`), or logical `TRUE` is
#' returned (invisibly) if `return_lyr_obj = FALSE`.
#' Logical `FALSE` is returned (invisibly) if an error occurs during processing.
#'
#' @note
#' The `Union` operation is a "GIS union" (not geometric union). It is
#' equivalent, for example, to the Union Vector Overlay tool in
#' [QGIS](https://www.qgis.org/).
#'
#' @seealso
#' [GDALVector], [ogr_define], [ogr_manage]
#'
#' @examples
#'
#' @export
ogr_proc <- function(mode,
                     input_lyr,
                     method_lyr,
                     out_dsn,
                     out_lyr_name = NULL,
                     out_geom_type = NULL,
                     out_fmt = NULL,
                     dsco = NULL,
                     lco = NULL,
                     mode_opt = NULL,
                     overwrite = FALSE,
                     quiet = FALSE,
                     return_lyr_obj = TRUE) {

    if (is(input_lyr, "Rcpp_GDALVector")) {
        if (!input_lyr$isOpen()) {
            stop("'input_lyr' is not open", call. = FALSE)
        }
    } else {
        stop("'input_lyr' must be an object of class 'GDALVector'",
             call. = FALSE)
    }

    if (is(method_lyr, "Rcpp_GDALVector")) {
        if (!method_lyr$isOpen()) {
            stop("'method_lyr' is not open", call. = FALSE)
        }
    } else {
        stop("'method_lyr' must be an object of class 'GDALVector'",
             call. = FALSE)
    }

    input_srs <- input_lyr$getSpatialRef()
    method_srs <- method_lyr$getSpatialRef()
    if (input_srs == "" && method_srs != "") {
        warning("input layer has no SRS defined, but method layer has one")
    }
    if (input_srs != "" && method_srs == "") {
        warning("input layer has an SRS defined, but method layer does not")
    }
    if (input_srs != "" && method_srs != "" &&
            !srs_is_same(input_srs, method_srs)) {

            warning("input and method layers do not have identical SRS, and no on-the-fly reprojection will be done",
                    call. = FALSE)
    }

    if (!(is.logical(overwrite) && length(overwrite) == 1))
        stop("'overwrite' must be logical type with length 1", call. = FALSE)

    if (!(is.logical(quiet) && length(quiet) == 1))
        stop("'quiet' must be logical type with length 1", call. = FALSE)

    if (!ogr_ds_exists(out_dsn, with_update = TRUE)) {
        if (ogr_ds_exists(out_dsn) && !overwrite) {
            msg <- "'out_dsn' exists but cannot be updated.\n"
            msg <- paste0(msg, "You may need to remove it first, ")
            msg <- paste0(msg, "or use 'overwrite = TRUE'.")
            stop(msg, call. = FALSE)
        }
    }

    if (is.null(out_fmt))
        out_fmt <- .getOGRformat(out_dsn)
    if (is.null(out_fmt)) {
        message("format driver cannot be determined for: ", out_dsn)
        stop("specify 'out_fmt' to create a new dataset", call. = FALSE)
    }

    if (is.null(out_lyr_name)) {
        if (out_fmt == "ESRI Shapefile")
            out_lyr_name <- tools::file_path_sans_ext(basename(out_dsn))
        if (is.null(out_lyr_name))
            stop("'out_lyr_name' must be specified", call. = FALSE)
    }

    if (is.null(out_geom_type))
        out_geom_type <- "UNKNOWN"

    if (ogr_ds_exists(out_dsn, with_update = TRUE) && overwrite) {
        deleted <- FALSE
        if (ogr_layer_exists(out_dsn, out_lyr_name)) {
            deleted <- ogr_layer_delete(out_dsn, out_lyr_name)
        }
        if (!deleted) {
            if (ogr_ds_layer_count(out_dsn) == 1) {
                if (vsi_stat(out_dsn, "type") == "file")
                    deleted <- deleteDataset(out_dsn)
            }
        }
        if (!deleted) {
            stop("failed to overwrite the output layer", call. = FALSE)
        }
    }

    if (!ogr_ds_exists(out_dsn, with_update = TRUE)) {
        if (!ogr_ds_create(format = out_fmt,
                           dsn = out_dsn,
                           layer = out_lyr_name,
                           geom_type = out_geom_type,
                           srs = input_srs,
                           dsco = dsco,
                           lco = lco)) {

            stop("failed to create 'out_dsn'", call. = FALSE)
        }
    }

    if (!ogr_layer_exists(out_dsn, out_lyr_name)) {
        res <- ogr_layer_create(out_dsn, out_lyr_name, layer_defn = NULL,
                                geom_type = out_geom_type, srs = input_srs,
                                lco = lco)
        if (!res)
            stop("failed to create the ouput layer", call. = FALSE)
    }

    out_lyr <- new(GDALVector, out_dsn, out_lyr_name, read_only = FALSE)
    mode <- tolower(mode)
    ret <- FALSE

    if (mode == "intersection") {
        ret <- input_lyr$layerIntersection(method_lyr, out_lyr, quiet = quiet,
                                           options = mode_opt)

    } else if (mode == "union") {
        ret <- input_lyr$layerUnion(method_lyr, out_lyr, quiet = quiet,
                                    options = mode_opt)

    } else if (mode == "symdifference") {
        ret <- input_lyr$layerSymDifference(method_lyr, out_lyr, quiet = quiet,
                                            options = mode_opt)

    } else if (mode == "identity") {
        ret <- input_lyr$layerIdentity(method_lyr, out_lyr, quiet = quiet,
                                       options = mode_opt)

    } else if (mode == "update") {
        ret <- input_lyr$layerUpdate(method_lyr, out_lyr, quiet = quiet,
                                     options = mode_opt)

    } else if (mode == "clip") {
        ret <- input_lyr$layerClip(method_lyr, out_lyr, quiet = quiet,
                                   options = mode_opt)

    } else if (mode == "erase") {
        ret <- input_lyr$layerErase(method_lyr, out_lyr, quiet = quiet,
                                    options = mode_opt)

    } else {
        stop("invalid 'mode'", call. = FALSE)
    }

    if (ret && return_lyr_obj) {
        return(out_lyr)
    } else {
        out_lyr$close()
        return(ret)
    }
}
