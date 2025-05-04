# R interface to OGR vector geoprocessing
# Chris Toney <chris.toney at usda.gov>

#' GDAL OGR facilities for vector geoprocessing
#'
#' @description
#' `ogr_proc()` performs GIS overlay operations on vector layers
#' (\url{https://en.wikipedia.org/wiki/Vector_overlay}). It provides
#' an interface to the GDAL API methods for these operations
#' (`OGRLayer::Intersection()`, `OGRLayer::Union()`, etc).
#' Inputs are given as objects of class [`GDALVector`][GDALVector], which
#' may have spatial and/or attribute filters applied.
#' The output layer will be created if it does not exist, but output can also
#' be appended to an existing layer, or written to an existing empty layer that
#' has a custom schema defined.
#' `ogr_proc()` is basically a port of the
#' [`ogr_layer_algebra`](https://gdal.org/en/stable/programs/ogr_layer_algebra.html#ogr-layer-algebra)
#' utility in the GDAL Python bindings.
#'
#' @details
#' Seven processing modes are available:
#' * `Intersection`: The output layer contains features whose geometries
#' represent areas that are common between features in the input layer and in
#' the method layer. The features in the output layer have attributes from
#' both input and method layers.
#' * `Union`: The output layer contains features whose geometries represent
#' areas that are either in the input layer, in the method layer, or in
#' both. The features in the output layer have attributes from both input and
#' method layers. For features which represent areas that are only in the
#' input layer or only in the method layer the respective attributes have
#' undefined values.
#' * `SymDifference`: The output layer contains features whose geometries
#' represent areas that are in either in the input layer or in the method layer
#' but not in both. The features in the output layer have attributes from both
#' input and method layers. For features which represent areas that are only in
#' the input or only in the method layer the respective attributes have
#' undefined values.
#' * `Identity`: Identifies the features of the input layer with the ones from
#' the method layer. The output layer contains features whose geometries
#' represent areas that are in the input layer. The features in the output layer
#' have attributes from both the input and method layers.
#' * `Update`: The update method creates a layer which adds features into the
#' input layer from the method layer, possibly cutting features in the input
#' layer. The features in the output layer have areas of the features of the
#' method layer or those areas of the features of the input layer that are not
#' covered by the method layer. The features of the output layer get their
#' attributes from the input layer.
#' * `Clip`: The clip method creates a layer which has features from the input
#' layer clipped to the areas of the features in the method layer. By default
#' the output layer has attributes of the input layer.
#' * `Erase`: The erase method creates a layer which has features from the input
#' layer whose areas are erased by the features in the method layer. By default,
#' the output layer has attributes of the input layer.
#'
#' By default, `ogr_proc()` will create the output layer with an empty schema.
#' It will be initialized by GDAL to contain all fields in the input layer, or
#' depending on the operation, all fields in both the input and method layers.
#' In the latter case, the prefixes `"input_"` and `"method_"` will be added to
#' the output field names by default. The default prefixes can be overridden in
#' the `mode_opt` argument as described below.
#'
#' Alternatively, the functions in the [`ogr_manage`][ogr_manage] interface
#' could be used to create an empty layer with user-defined schema (e.g.,
#' [ogr_ds_create()], [ogr_layer_create()] and [ogr_field_create()]). If the
#' schema of the output layer is set by the user and contains fields that have
#' the same name as a field in both the input and method layers, then the
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
#' pretest intersection of features of method layer with features of input
#' layer. Applies to `Intersection`, `Union`, `Identity`.
#' * PRETEST_CONTAINMENT=YES/NO. Set to YES to pretest the containment of
#' features of method layer within the features of input layer. This will speed
#' up the operation significantly in some cases. Requires that the prepared
#' geometries are in effect. Applies to `Intersection`.
#' * KEEP_LOWER_DIMENSION_GEOMETRIES=YES/NO. Set to NO to skip result features
#' with lower dimension geometry that would otherwise be added to the output
#' layer. The default is YES, to add features with lower dimension geometry, but
#' only if the result output has an UNKNOWN geometry type. Applies to
#' `Intersection`, `Union`, `Identity`.
#'
#' The input and method layers should have the same spatial reference system. No
#' on-the-fly reprojection is done. When an output layer is created it will have
#' the SRS of `input_lyr`.
#'
#' @param mode Character string specifying the operation to perform. One of
#' `Intersection`, `Union`, `SymDifference`, `Identity`, `Update`, `Clip` or
#' `Erase` (see Details).
#' @param input_lyr An object of class [`GDALVector`][GDALVector] to use as the
#' input layer. For overlay operations, this is the first layer in the
#' operation.
#' @param method_lyr An object of class [`GDALVector`][GDALVector] to use as the
#' method layer. This is the conditional layer supplied to an operation (e.g.,
#' Clip, Erase, Update), or the second layer in overlay operations (e.g., Union,
#' Intersection, SymDifference).
#' @param out_dsn The destination vector filename or database connection string
#' to which the output layer will be written.
#' @param out_lyr_name Layer name where the output vector will be written. May
#' be `NULL` (e.g., shapefile), but typically must be specified.
#' @param out_geom_type Character string specifying the geometry type of the
#' output layer. One of NONE, GEOMETRY, POINT, LINESTRING, POLYGON,
#' GEOMETRYCOLLECTION, MULTIPOINT, MULTIPOLYGON, GEOMETRY25D, POINT25D,
#' LINESTRING25D, POLYGON25D, GEOMETRYCOLLECTION25D, MULTIPOINT25D,
#' MULTIPOLYGON25D. Defaults to UNKNOWN if not specified.
#' @param out_fmt GDAL short name of the output vector format. If unspecified,
#' the function will attempt to guess the format from the value of `out_dsn`.
#' @param dsco Optional character vector of format-specific creation options
#' for `out_dsn` (`"NAME=VALUE"` pairs).
#' @param lco Optional character vector of format-specific creation options
#' for `out_layer` (`"NAME=VALUE"` pairs).
#' @param mode_opt Optional character vector of `"NAME=VALUE"` pairs that
#' specify processing options. Available options depend on the value of `mode`
#' (see Details).
#' @param overwrite Logical value. `TRUE` to overwrite the output layer if it
#' already exists. Defaults to `FALSE`.
#' @param quiet Logical value. If `TRUE`, a progress bar will not be
#' displayed. Defaults to `FALSE`.
#' @param return_obj Logical value. If `TRUE` (the default), an object of
#' class [`GDALVector`][GDALVector] opened on the output layer will be returned,
#' otherwise the function returns a logical value.
#'
#' @returns Upon successful completion, an object of class
#' [`GDALVector`][GDALVector] is returned by default (`return_obj = TRUE`), or
#' logical `TRUE` is returned if `return_obj = FALSE`.
#' Logical `FALSE` is returned if an error occurs during processing.
#'
#' @note
#' The first geometry field on a layer is always used.
#'
#' For best performance use the minimum amount of features in the method layer
#' and copy into a memory layer.
#'
#' @seealso
#' [`GDALVector-class`][GDALVector], [`ogr_define`][ogr_define],
#' [`ogr_manage`][ogr_manage]
#'
#' Vector overlay operators:\cr
#' \url{https://en.wikipedia.org/wiki/Vector_overlay}
#'
#' @examples
#' # MTBS fires in Yellowstone National Park 1984-2022
#' dsn <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
#'
#' # layer filtered to fires after 1988
#' lyr1 <- new(GDALVector, dsn, "mtbs_perims")
#' lyr1$setAttributeFilter("ig_year > 1988")
#' lyr1$getFeatureCount()
#'
#' # second layer for the 1988 North Fork fire perimeter
#' sql <- "SELECT incid_name, ig_year, geom FROM mtbs_perims
#'         WHERE incid_name = 'NORTH FORK'"
#' lyr2 <- new(GDALVector, dsn, sql)
#' lyr2$getFeatureCount()
#'
#' # intersect to obtain areas in the North Fork perimeter that have re-burned
#' tmp_dsn <- tempfile(fileext = ".gpkg")
#' opt <- c("INPUT_PREFIX=layer1_",
#'          "METHOD_PREFIX=layer2_",
#'          "PROMOTE_TO_MULTI=YES")
#'
#' lyr_out <- ogr_proc(mode = "Intersection",
#'                     input_lyr = lyr1,
#'                     method_lyr = lyr2,
#'                     out_dsn = tmp_dsn,
#'                     out_lyr_name = "north_fork_reburned",
#'                     out_geom_type = "MULTIPOLYGON",
#'                     mode_opt = opt)
#'
#' # the output layer has attributes of both the input and method layers
#' (d <- lyr_out$fetch(-1))
#'
#' # clean up
#' lyr1$close()
#' lyr2$close()
#' lyr_out$close()
#' \dontshow{deleteDataset(tmp_dsn)}
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
                     return_obj = TRUE) {

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
            stop("failed to create the output layer", call. = FALSE)
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

    if (ret && return_obj) {
        out_lyr$open(read_only = TRUE)
        return(out_lyr)
    } else {
        out_lyr$close()
        return(ret)
    }
}
