# Convenience function for using the reprojection capability of ogr2ogr().
# Supports basic reprojection capabilities applied to one input layer.
# `add_cl_arg` allows also passing additional command-line arguments for
# access to more advanced geometry and SRS related options of ogr2ogr.
# Chris Toney <chris.toney at usda.gov>

#' Reproject a vector layer
#'
#' @description
#' `ogr_reproject()` reprojects the features of a vector layer to a different
#' spatial reference system and writes the new layer to a specified output
#' dataset. The output may be in a different vector file format than the input
#' dataset. A source SRS definition must be available in the source layer
#' for reprojection to occur.
#'
#' @details
#' `ogr_reproject()` is a convenience wrapper to perform vector reprojection
#' via `ogr2ogr()`, which in turn is an API binding to GDAL's `ogr2ogr`
#' command-line utility.
#'
#' @param src_dsn Character string. The filename or database connection string
#' specifying the vector data source containing the input layer.
#' @param src_layer Character string. The name of the input layer in `src_dsn`
#' to reproject. Optionally can be given as an SQL SELECT statement to be
#' executed against `src_dsn`, defining the source layer as the result set.
#' May also be given as empty string (`""`), in which case the first layer by
#' index will be used (mainly useful for single-layer file formats such as
#' ESRI Shapefile).
#' @param out_dsn Character string. The filename or database connection string
#' specifying the vector data source to which the output layer will be written.
#' @param out_srs Character string specifying the output spatial reference
#' system. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param out_fmt Optional character string giving the GDAL short name
#' of the output dataset format. Only used if `out_dsn` needs to be created.
#' Generally can be `NULL` in which case the format will be guessed from the
#' file extension.
#' @param overwrite Logical value. `TRUE` to overwrite the output layer if it
#' already exists. Defaults to `FALSE`.
#' @param append Logical value. `TRUE` to append to the output layer if it
#' already exists. Defaults to `FALSE`.
#' @param nln Optional character string giving an alternate name to assign the
#' new layer. By default, `src_layer` is used, but `nln` is required if
#' `src_layer`is a SQL SELECT statement.
#' @param nlt Optional character string to define the geometry type for the
#' output layer. Mainly useful when `nlt = PROMOTE_TO_MULTI` might be given to
#' automatically promote layers that mix polygon / multipolygons to
#' multipolygons, and layers that mix linestrings / multilinestrings to
#' multilinestrings. Can be useful when converting shapefiles to PostGIS and
#' other output formats that implement strict checks for geometry types.
#' @param dsco Optional character vector of format-specific creation options
#' for `out_dsn` (`"NAME=VALUE"` pairs). Should only be used if `out_dsn` does
#' not already exist.
#' @param lco Optional character vector of format-specific creation options
#' for the output layer (`"NAME=VALUE"` pairs). Should not be used if appending
#' to an existing layer.
#' @param dialect Optional character string specifying the SQL dialect to use.
#' The OGR SQL engine (`"OGRSQL"`) will be used by default if a value is not
#' given. The `"SQLite"` dialect can also be use. Only relevant if `src_layer`
#' is given as a SQL SELECT statement.
#' @param spat_bbox Optional numeric vector of length four specifying a spatial
#' bounding box (xmin, ymin, xmax, ymax), *in the SRS of the source layer*.
#' Only features whose geometry intersects `spat_bbox` will be selected for
#' reprojection.
#' @param src_open_options Optional character vector of dataset open options
#' for `src_dsn` (format-specific `"NAME=VALUE"` pairs).
#' @param progress Logical value, `TRUE` to display progress on the terminal.
#' Defaults to `FALSE`. Only works if the input layer has "fast feature count"
#' capability.
#' @param add_cl_arg Optional character vector of additional command-line
#' arguments to be passed to `ogr2ogr()` (see Note).
#' @param return_obj Logical value, `TRUE` to return an object of class
#' [`GDALVector`][GDALVector] open on the output layer (the default).
#' @return
#' Upon successful completion, an object of class [`GDALVector`][GDALVector] is
#' returned by default (if `return_obj = TRUE`), or logical `TRUE` is returned
#' (invisibly) if `return_obj = FALSE`. An error is raised if reprojection
#' fails.
#'
#' @note
#' For advanced use, additional command-line arguments may be passed to
#' `ogr2ogr()` in `add_cl_arg` (e.g., advanced geometry and SRS related
#' options). Users should be aware of possible implications and
#' compatibility with the arguments already implied by the parameterization
#' of `ogr_reproject()`.
#'
#' The function will attempt to create the output datasource if it does not
#' already exist. Some formats (e.g., PostgreSQL) do not support creation of
#' new datasets (i.e., a database within PostgreSQL), but output layers can be
#' written to an existing database.
#'
#' @seealso
#' [ogr2ogr()], [warp()] for raster reprojection
#'
#' GDAL documentation for `ogr2ogr`:\cr
#' \url{https://gdal.org/en/stable/programs/ogr2ogr.html}
#'
#' @examples
#' # MTBS fire perimeters
#' f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
#' (mtbs <- new(GDALVector, f, "mtbs_perims"))
#'
#' mtbs$getSpatialRef() |> srs_is_projected()  # TRUE
#'
#' # YNP boundary
#' f <- system.file("extdata/ynp_features.zip", package = "gdalraster")
#' ynp_dsn <- file.path("/vsizip", f, "ynp_features.gpkg")
#' (bnd <- new(GDALVector, ynp_dsn, "ynp_bnd"))
#'
#' bnd$getSpatialRef() |> srs_is_projected()  # FALSE
#'
#' # project the boundary to match the MTBS layer
#' out_dsn <- tempfile(fileext = ".gpkg")
#' (bnd_mtsp <- ogr_reproject(ynp_dsn, "ynp_bnd", out_dsn, mtbs$getSpatialRef()))
#'
#' bnd_mtsp$getFeatureCount()
#'
#' plot(bnd_mtsp$getNextFeature(), col = "wheat")
#'
#' mtbs$setAttributeFilter("incid_name = 'MAPLE'")
#' mtbs$getFeatureCount()  # 1
#'
#' (feat <- mtbs$getNextFeature())
#'
#' plot(feat, col = "red", border = NA, add = TRUE)
#'
#' mtbs$close()
#' bnd$close()
#' bnd_mtsp$close()
#' \dontshow{unlink(out_dsn)}
#' @export
ogr_reproject <- function(src_dsn, src_layer, out_dsn, out_srs,
                          out_fmt = NULL, overwrite = FALSE,
                          append = FALSE, nln = NULL, nlt = NULL,
                          dsco = NULL, lco = NULL,
                          dialect = NULL, spat_bbox = NULL,
                          src_open_options = NULL, progress = FALSE,
                          add_cl_arg = NULL, return_obj = TRUE) {

    if (!(is.character(src_dsn) && length(src_dsn) == 1))
        stop("'src_dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(src_layer) && length(src_layer) == 1))
        stop("'src_layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(out_dsn) && length(out_dsn) == 1))
        stop("'out_dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(out_srs) && length(out_srs) == 1))
        stop("'out_srs' must be a length-1 character vector", call. = FALSE)
    if (!is.null(out_fmt)) {
        if (!(is.character(out_fmt) && length(out_fmt) == 1)) {
            stop("'out_fmt' must be a length-1 character vector",
                 call. = FALSE)
        }
    }
    if (!(is.logical(overwrite) && length(overwrite) == 1))
        stop("'overwrite' must be a single logical value", call. = FALSE)
    if (!(is.logical(append) && length(append) == 1))
        stop("'append' must be a single logical value", call. = FALSE)
    if (!is.null(nln)) {
        if (!(is.character(nln) && length(nln) == 1))
            stop("'nln' must be a length-1 character vector", call. = FALSE)
    }
    if (!is.null(nlt)) {
        if (!(is.character(nlt) && length(nlt) == 1))
            stop("'nlt' must be a length-1 character vector", call. = FALSE)
    }
    if (!is.null(dsco)) {
        if (!is.character(dsco))
            stop("'dsco' must be a character vector", call. = FALSE)
    }
    if (!is.null(lco)) {
        if (!is.character(lco))
            stop("'lco' must be a character vector", call. = FALSE)
    }
    if (!is.null(dialect)) {
        if (!(is.character(dialect) && length(dialect) == 1)) {
            stop("'dialect' must be a length-1 character vector",
                 call. = FALSE)
        }
    }
    if (!is.null(spat_bbox)) {
        if (!(is.numeric(spat_bbox) && length(spat_bbox) == 4)) {
            stop("'spat_bbox' must be a numeric vector of length 4",
                 call. = FALSE)
        }
    }
    if (!is.null(src_open_options)) {
        if (!is.character(src_open_options)) {
            stop("'src_open_options' must be a character vector",
                 call. = FALSE)
        }
    }
    if (!(is.logical(progress) && length(progress) == 1))
        stop("'progress' must be a single logical value", call. = FALSE)
    if (!is.null(add_cl_arg)) {
        if (!is.character(add_cl_arg)) {
            stop("'add_cl_arg' must be a character vector",
                 call. = FALSE)
        }
    }

    out_layer_name <- src_layer

    # special case for shapefile
    chk_fmt <- .getOGRformat(out_dsn)
    if (!is.null(chk_fmt)) {
        if (chk_fmt == "ESRI Shapefile")
            out_layer_name <- tools::file_path_sans_ext(basename(out_dsn))
    } else {
        chk_fmt <- "_unknown_"
    }

    args <- c("-t_srs", out_srs)

    if (!is.null(out_fmt))
        args <- c(args, c("-of", out_fmt))

    is_sql_layer <- FALSE
    if (toupper(substr(src_layer, 1, 7)) == "SELECT ") {
        args <- c(args, "-sql", src_layer)
        is_sql_layer <- TRUE
        if (!is.null(dialect))
            args <- c(args, c("-dialect", dialect))
    }

    if (!is.null(nln)) {
        args <- c(args, c("-nln", nln))
        out_layer_name <- nln
    } else if (chk_fmt == "ESRI Shapefile") {
        args <- c(args, c("-nln", out_layer_name))
    } else if (is_sql_layer) {
        stop("SQL layer requires the 'nln' argument for an output layer name",
             call. = FALSE)
    }

    if (!is.null(nlt))
        args <- c(args, c("-nlt", nlt))

    if (!ogr_ds_exists(out_dsn, with_update = TRUE)) {
        if (ogr_ds_exists(out_dsn) && !overwrite) {
            msg <- "'out_dsn' exists but cannot be updated.\n"
            msg <- paste0(msg, "You may need to remove it first, ")
            msg <- paste0(msg, "or use 'overwrite = TRUE'.")
            stop(msg, call. = FALSE)
        }
    }

    if (ogr_ds_exists(out_dsn, with_update = TRUE) && !overwrite) {
        args <- c(args, "-update")
        if (ogr_layer_exists(out_dsn, out_layer_name) && append)
            args <- c(args, "-append")
    } else if (ogr_ds_exists(out_dsn, with_update = TRUE) && overwrite) {
        args <- c(args, "-update", "-overwrite")
    }

    if (!is.null(dsco)) {
        for (opt in dsco) {
            args <- c(args, c("-dsco", opt))
        }
    }

    if (!is.null(lco)) {
        for (opt in lco) {
            args <- c(args, c("-lco", opt))
        }
    }

    if (!is.null(spat_bbox))
        args <- c(args, c("-spat", spat_bbox))

    if (progress)
        args <- c(args, "-progress")

    if (!is.null(add_cl_arg))
        args <- c(args, add_cl_arg)

    src_layers <- NULL
    if (src_layer == "") {
        ds_layers <- ogr_ds_layer_names(src_dsn)
        if (is.null(ds_layers)) {
            stop("no layers found in 'src_dsn'", call. = FALSE)
        } else {
            src_layer <- ds_layers[1]
            if (out_layer_name == "")
                out_layer_name <- src_layer
        }
    }
    if (!is_sql_layer)
        src_layers <- src_layer

    ret <- ogr2ogr(src_dsn, out_dsn, src_layers, cl_arg = args,
                   open_options = src_open_options)

    if (ret && return_obj) {
        return(new(GDALVector, out_dsn, out_layer_name))
    } else {
        return(invisible(ret))
    }
}
