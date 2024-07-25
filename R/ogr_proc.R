# R interface to GDAL/OGR facilities for vector geoprocessing
# Chris Toney <chris.toney at usda.gov>

#' @export
ogr_proc <- function(mode,
                     input_lyr,
                     method_lyr,
                     out_dsn,
                     out_lyr_name = NULL,
                     overwrite = FALSE,
                     out_fmt = NULL,
                     out_geom_type = NULL,
                     dsco = NULL,
                     lco = NULL,
                     options = NULL,
                     return_lyr_obj = TRUE,
                     quiet = FALSE) {

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
                                           options = options)

    } else if (mode == "union") {
        ret <- input_lyr$layerUnion(method_lyr, out_lyr, quiet = quiet,
                                    options = options)

    } else if (mode == "symdifference") {
        ret <- input_lyr$layerSymDifference(method_lyr, out_lyr, quiet = quiet,
                                            options = options)

    } else if (mode == "identity") {
        ret <- input_lyr$layerIdentity(method_lyr, out_lyr, quiet = quiet,
                                       options = options)

    } else if (mode == "update") {
        ret <- input_lyr$layerUpdate(method_lyr, out_lyr, quiet = quiet,
                                     options = options)

    } else if (mode == "clip") {
        ret <- input_lyr$layerClip(method_lyr, out_lyr, quiet = quiet,
                                   options = options)

    } else if (mode == "erase") {
        ret <- input_lyr$layerErase(method_lyr, out_lyr, quiet = quiet,
                                    options = options)

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
