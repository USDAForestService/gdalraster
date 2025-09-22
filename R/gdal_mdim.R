# GDAL Multidimensional Raster functions
# public interface to src/gdal_mdim.cpp
# Chris Toney <jctoney at gmail.com>

#' Return a view of an MDArray as a "classic" GDALDataset (i.e., 2D)
#'
#' `mdim_as_classic()` returns a 2D raster view on an MDArray in a GDAL
#' Multidimensional Raster dataset, as an object of class `GDALRaster`. Only 2D
#' or more arrays are supported. In the case of > 2D arrays,additional
#' dimensions will be represented as raster bands. Requires GDAL >= 3.2.
#'
#' @param filename Character string giving the data source name of the
#' multidimensional raster (e.g., file, VSI path).
#' @param array_name Character string giving the name of the MDarray in
#' `filename`.
#' @param idx_xdim Integer value giving the index of the dimension that will be
#' used as the X/width axis (0-based).
#' @param idx_ydim Integer value giving the index of the dimension that will be
#' used as the Y/height axis (0-based).
#' @param read_only Logical value, `TRUE` to open the dataset in read-only mode
#' (the default).
#' @param group_name Optional character string giving the fully qualified name
#' of a group containing `array_name`.
#' @param allowed_drivers Optional character vector of driver short names that
#' must be considered. By default, all known multidimensional raster drivers are
#' considered.
#' @param open_options Optional character vector of format-specific dataset open
#' options as `"NAME=VALUE"` pairs.
#' @returns An object of class `GDALRaster`.
#'
#' @note
#' The indexing of array dimensions is 0-based to be consistent with the
#' `<ARRAY-SPEC>` notation that may be used with GDAL CLI commands, e.g.,
#' `gdal_usage("mdim convert")` (requires GDAL > 3.11.3).
#' See \url{https://gdal.org/en/stable/programs/gdal_mdim_convert.html}.
#'
#' Once the returned `GDALRaster` object has been closed, it cannot be re-opened
#' with its `$open()` method.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [mdim_info()]
#'
#' @examples
#' f <- system.file("extdata/byte.nc", package="gdalraster")
#' mdim_info(f) |> writeLines()
#'
#' (ds <- mdim_as_classic(f, "Band1", 1, 0))
#'
#' plot_raster(ds, interpolate = FALSE, legend = TRUE, main = "byte.nc")
#'
#' ds$close()
#' @export
mdim_as_classic <- function(filename, array_name, idx_xdim, idx_ydim,
                            read_only = TRUE, group_name = NULL,
                            allowed_drivers = NULL, open_options = NULL) {

    if (missing(filename) || is.null(filename) || all(is.na(filename)))
        stop("'filename' is required", call. = FALSE)
    if (!(is.character(filename) && length(filename) == 1))
        stop("'filename' must be a character string", call. = FALSE)

    if (missing(array_name) || is.null(array_name) || all(is.na(array_name)))
        stop("'array_name' is required", call. = FALSE)
    if (!(is.character(array_name) && length(array_name) == 1))
        stop("'array_name' must be a character string", call. = FALSE)

    if (missing(idx_xdim) || is.null(idx_xdim) || all(is.na(idx_xdim)))
        stop("'idx_xdim' is required", call. = FALSE)
    if (!(is.numeric(idx_xdim) && length(idx_xdim) == 1))
        stop("'idx_xdim' must be a numeric value (integer)", call. = FALSE)

    if (missing(idx_ydim) || is.null(idx_ydim) || all(is.na(idx_ydim)))
        stop("'idx_ydim' is required", call. = FALSE)
    if (!(is.numeric(idx_ydim) && length(idx_ydim) == 1))
        stop("'idx_ydim' must be a numeric value (integer)", call. = FALSE)

    if (missing(read_only) || is.null(read_only) || all(is.na(read_only)))
        read_only <- TRUE
    if (!(is.logical(read_only) && length(read_only) == 1))
        stop("'read_only' must be a logical value", call. = FALSE)

    if (missing(group_name) || is.null(group_name) || all(is.na(group_name)))
        group_name <- ""
    if (!(is.character(group_name) && length(group_name) == 1))
        stop("'group_name' must be a character string", call. = FALSE)

    if (missing(allowed_drivers) || all(is.na(allowed_drivers)))
        allowed_drivers <- NULL
    if (!is.null(allowed_drivers)) {
        if (!is.character(allowed_drivers))
            stop("'allowed_drivers' must be a character vector", call. = FALSE)
    }

    if (missing(open_options) || all(is.na(open_options)))
        open_options <- NULL
    if (!is.null(open_options)) {
        if (!is.character(open_options))
            stop("'open_options' must be a character vector", call. = FALSE)
    }

    # signature for mdim_as_classic() object factory
    ds <- new(GDALRaster, filename, array_name, idx_xdim, idx_ydim, read_only,
              group_name, allowed_drivers, open_options, TRUE)

    return(ds);
}
