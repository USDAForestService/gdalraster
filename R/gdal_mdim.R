# GDAL Multidimensional Raster functions
# public interface to src/gdal_mdim.cpp
# Chris Toney <jctoney at gmail.com>

#' Return a view of an MDArray as a "classic" GDALDataset (i.e., 2D)
#'
#' `mdim_as_classic()` returns a 2D raster view on an MDArray in a GDAL
#' Multidimensional Raster dataset, as an object of class `GDALRaster`. Only 2D
#' or more arrays are supported. In the case of > 2D arrays, additional
#' dimensions will be represented as raster bands. Requires GDAL >= 3.2.
#'
#' @param dsn Character string giving the data source name of the
#' multidimensional raster (e.g., file, VSI path).
#' @param array_name Character string giving the name of the MDarray in
#' `dsn`.
#' @param idx_xdim Integer value giving the index of the dimension that will be
#' used as the X/width axis (0-based).
#' @param idx_ydim Integer value giving the index of the dimension that will be
#' used as the Y/height axis (0-based).
#' @param read_only Logical value, `TRUE` to open the dataset in read-only mode
#' (the default).
#' @param group_name Optional character string giving the fully qualified name
#' of a group containing `array_name`.
#' @param view_expr Optional character string giving an expression for basic
#' array slicing and indexing, or field access (see section `View Expressions`).
#' @param allowed_drivers Optional character vector of driver short names that
#' must be considered. By default, all known multidimensional raster drivers are
#' considered.
#' @param open_options Optional character vector of format-specific dataset open
#' options as `"NAME=VALUE"` pairs.
#' @returns An object of class `GDALRaster`.
#'
#' @section View Expressions:
#' A character string can be passed in argument `view_expr` to specify array
#' slicing or field access. The slice expression uses the same syntax as NumPy
#' basic slicing and indexing (0-based), or it can use field access by name.
#' See \url{https://numpy.org/doc/stable/user/basics.indexing.html}.
#'
#' GDAL support for view expression on an MDArray is documented for
#' `GDALMDArray::GetView()` (see
#' \url{https://gdal.org/en/stable/api/gdalmdarray_cpp.html}) and copied here:
#'
#' Multiple [] bracket elements can be concatenated, with a slice expression or
#' field name inside each.
#'
#' For basic slicing and indexing, inside each [] bracket element, a list of
#' indexes that apply to successive source dimensions, can be specified, using
#' integer indexing (e.g. 1), range indexing (start:stop:step), ellipsis (...)
#' or newaxis, using a comma separator.
#'
#' Example expressions with a 2-dimensional array whose content is
#' `[[0,1,2,3],[4,5,6,7]]`.
#' * `"[1][2]"`: returns a 0-dimensional/scalar array with the value at
#' index 1 in the first dimension, and index 2 in the second dimension from the
#' source array. That is, `5`.
#' * `"[1,2]"`: same as above, but a bit more performant.
#' * `"[1]"`: returns a 1-dimensional array, sliced at index `1` in the
#' first dimension. That is `[4,5,6,7]`.
#' * `"[:,2]"`: returns a 1-dimensional array, sliced at index `2` in the
#' second dimension. That is `[2,6]`.
#' * `"[:,2:3:]"`: returns a 2-dimensional array, sliced at index `2` in
#' the second dimension. That is `[[2],[6]]`.
#' * `"[::,2]"`: Same as above.
#' * `"[...,2]"`: same as above, in that case, since the ellipsis only
#' expands to one dimension here.
#' * `"[:,::2]"`: returns a 2-dimensional array, with even-indexed
#' elements of the second dimension. That is `[[0,2],[4,6]]`.
#' * `"[:,1::2]"`: returns a 2-dimensional array, with odd-indexed
#' elements of the second dimension. That is `[[1,3],[5,7]]`.
#' * `"[:,1:3:]"`: returns a 2-dimensional array, with elements of the
#' second dimension with index in the range `[1,3]`. That is `[[1,2],[5,6]]`.
#' * `"[::-1,:]"`: returns a 2-dimensional array, with the values in
#' first dimension reversed. That is `[[4,5,6,7],[0,1,2,3]]`.
#' * `"[newaxis,...]"`: returns a 3-dimensional array, with an additional
#' dimension of size `1` put at the beginning. That is
#' `[[[0,1,2,3],[4,5,6,7]]]`.
#'
#' One difference with NumPy behavior is that ranges that would result in zero
#' elements are not allowed (dimensions of size 0 not being allowed in the GDAL
#' multidimensional model).
#'
#' For field access, the syntax to use is `"['field_name']"`. Multiple field
#' specification is not supported currently. Both type of access can be
#' combined, e.g. `"[1]['field_name']"`.
#'
#' @note
#' The indexing of array dimensions is 0-based consistent with the
#' `<ARRAY-SPEC>` notation that may be used with GDAL CLI commands, e.g.,
#' `gdal_usage("mdim convert")` (CLI bindings require GDAL > 3.11.3).
#' See \url{https://gdal.org/en/stable/programs/gdal_mdim_convert.html}.
#'
#' Once the returned `GDALRaster` object has been closed, it cannot be re-opened
#' with its `$open()` method.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [mdim_info()], [mdim_translate()]
#'
#' @examplesIf gdal_version_num() >= gdal_compute_version(3, 2, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
#' f <- system.file("extdata/byte.nc", package="gdalraster")
#' # mdim_info(f)
#'
#' (ds <- mdim_as_classic(f, "Band1", 1, 0))
#'
#' plot_raster(ds, interpolate = FALSE, legend = TRUE, main = "Band1")
#'
#' ds$close()
#' @export
mdim_as_classic <- function(dsn, array_name, idx_xdim, idx_ydim,
                            read_only = TRUE, group_name = NULL,
                            view_expr = NULL, allowed_drivers = NULL,
                            open_options = NULL) {

    if (missing(dsn) || is.null(dsn) || all(is.na(dsn)))
        stop("'dsn' is required", call. = FALSE)
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a character string", call. = FALSE)

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

    if (missing(view_expr) || is.null(view_expr) || all(is.na(view_expr)))
        view_expr <- ""
    if (!(is.character(view_expr) && length(view_expr) == 1))
        stop("'view_expr' must be a character string", call. = FALSE)

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
    ds <- new(GDALRaster, dsn, array_name, idx_xdim, idx_ydim, read_only,
              group_name, view_expr, allowed_drivers, open_options)

    return(ds);
}
