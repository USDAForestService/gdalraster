# R interface to GDALCreate() and GDALCreateCopy(), via the wrapper functions
# in src/gdal_exp.cpp.

#' Create a new uninitialized raster
#'
#' `create()` makes an empty raster in the specified format.
#'
#' @param format Character string giving a raster format short name
#' (e.g., `"GTiff"`).
#' @param dst_filename Character string giving the filename to create.
#' @param xsize Integer width of raster in pixels.
#' @param ysize Integer height of raster in pixels.
#' @param nbands Integer number of bands.
#' @param dataType Character string containing the data type name.
#' (e.g., common data types include Byte, Int16, UInt16, Int32, Float32).
#' @param options Optional list of format-specific creation options in a
#' character vector of `"NAME=VALUE"` pairs
#' (e.g., \code{options = c("COMPRESS=LZW")} to set LZW
#' compression during creation of a GTiff file).
#' The APPEND_SUBDATASET=YES option can be
#' specified to avoid prior destruction of existing dataset.
#' @param return_obj Logical scalar. If `TRUE`, an object of class
#' [`GDALRaster`][GDALRaster] opened on the newly created dataset will be
#' returned, otherwise returns a logical value. Defaults to `FALSE`.
#' @returns By default, returns a logical value indicating success (invisible
#' \code{TRUE}, output written to `dst_filename`). An error is raised if the
#' operation fails. An object of class [`GDALRaster`][GDALRaster] open on the
#' output dataset will be returned if `return_obj = TRUE`.
#' @note
#' `dst_filename` may be an empty string (`""`) with `format = "MEM"` and
#' `return_obj = TRUE` to create an In-memory Raster
#' (\url{https://gdal.org/en/stable/drivers/raster/mem.html}).
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [createCopy()], [getCreationOptions()],
#' [rasterFromRaster()]
#' @examples
#' new_file <- file.path(tempdir(), "newdata.tif")
#' ds <- create(format="GTiff",
#'              dst_filename = new_file,
#'              xsize = 143,
#'              ysize = 107,
#'              nbands = 1,
#'              dataType = "Int16",
#'              return_obj=TRUE)
#'
#' # EPSG:26912 - NAD83 / UTM zone 12N
#' ds$setProjection(epsg_to_wkt(26912))
#'
#' gt <- c(323476, 30, 0, 5105082, 0, -30)
#' ds$setGeoTransform(gt)
#'
#' ds$setNoDataValue(band = 1, -9999)
#' ds$fillRaster(band = 1, -9999, 0)
#'
#' # ...
#'
#' # close the dataset when done
#' ds$close()
#' \dontshow{deleteDataset(new_file)}
#' @export
create <- function(format, dst_filename, xsize, ysize, nbands, dataType,
                   options = NULL, return_obj = FALSE) {

    if (is.null(return_obj))
        stop("'return_obj' must be a logical value", call. = FALSE)

    if (!is.logical(return_obj) || length(return_obj) > 1)
        stop("'return_obj' must be a logical scalar", call. = FALSE)

    # signature for create() object factory
    ds <- new(GDALRaster, format, dst_filename, xsize, ysize, nbands, dataType,
              options)

    if (return_obj) {
        return(ds)
    } else {
        ds$close()
        return(TRUE)
    }
}


#' Create a copy of a raster
#'
#' `createCopy()` copies a raster dataset, optionally changing the format.
#' The extent, cell size, number of bands, data type, projection, and
#' geotransform are all copied from the source raster.
#'
#' @param format Character string giving the format short name for the
#' output raster (e.g., `"GTiff"`).
#' @param dst_filename Character string giving the filename to create.
#' @param src_filename Either a character string giving the filename of the
#' source raster, or object of class `GDALRaster` for the source.
#' @param strict Logical. `TRUE` if the copy must be strictly equivalent,
#' or more normally `FALSE` (the default) indicating that the copy may adapt
#' as needed for the output format.
#' @param options Optional list of format-specific creation options in a
#' vector of `"NAME=VALUE"` pairs
#' (e.g., \code{options = c("COMPRESS=LZW")} to set \code{LZW}
#' compression during creation of a GTiff file).
#' The APPEND_SUBDATASET=YES option can be
#' specified to avoid prior destruction of existing dataset.
#' @param quiet Logical scalar. If `TRUE`, a progress bar will be not be
#' displayed. Defaults to `FALSE`.
#' @param return_obj Logical scalar. If `TRUE`, an object of class
#' [`GDALRaster`][GDALRaster] opened on the newly created dataset will be
#' returned. Defaults to `FALSE`.
#' @returns By default, returns a logical value indicating success (invisible
#' \code{TRUE}, output written to `dst_filename`). An error is raised if the
#' operation fails. An object of class [`GDALRaster`][GDALRaster] open on the
#' output dataset will be returned if `return_obj = TRUE`.
#' @note
#' `dst_filename` may be an empty string (`""`) with `format = "MEM"` and
#' `return_obj = TRUE` to create an In-memory Raster
#' (\url{https://gdal.org/en/stable/drivers/raster/mem.html}).
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [create()], [getCreationOptions()],
#' [rasterFromRaster()], [translate()]
#' @examples
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' tif_file <- file.path(tempdir(), "storml_lndscp.tif")
#' ds <- createCopy(format = "GTiff",
#'                  dst_filename = tif_file,
#'                  src_filename = lcp_file,
#'                  options = "COMPRESS=LZW",
#'                  return_obj = TRUE)
#'
#' ds$getMetadata(band = 0, domain = "IMAGE_STRUCTURE")
#'
#' for (band in 1:ds$getRasterCount())
#'     ds$setNoDataValue(band, -9999)
#' ds$getStatistics(band = 1, approx_ok = FALSE, force = TRUE)
#'
#' ds$close()
#' \dontshow{deleteDataset(tif_file)}
#' @export
createCopy <- function(format, dst_filename, src_filename, strict = FALSE,
                       options = NULL, quiet = FALSE, return_obj = FALSE) {

    src_ds <- NULL
    if (is(src_filename, "Rcpp_GDALRaster")) {
        src_ds <- src_filename
        if (!src_ds$isOpen())
            stop("source dataset is not open", call. = FALSE)
    } else if (is.character(src_filename) && length(src_filename) == 1) {
        src_ds <- new(GDALRaster, src_filename)
    } else {
        stop("'src_filename' must be a character string or 'GDALRaster' object",
             call. = FALSE)
    }

    if (is.null(return_obj))
        stop("'return_obj' must be a logical value", call. = FALSE)

    if (!is.logical(return_obj) || length(return_obj) > 1)
        stop("'return_obj' must be a logical scalar", call. = FALSE)

    # signature for createCopy() object factory
    ds <- new(GDALRaster, format, dst_filename, src_ds, strict, options, quiet)

    if (return_obj) {
        return(ds)
    } else {
        ds$close()
        return(TRUE)
    }
}
