# Wrappers for GDALCreate() and GDALCreateCopy()

#' Create a new uninitialized raster
#'
#' `create()` makes an empty raster in the specified format.
#'
#' @param format Raster format short name (e.g., "GTiff").
#' @param dst_filename Filename to create.
#' @param xsize Integer width of raster in pixels.
#' @param ysize Integer height of raster in pixels.
#' @param nbands Integer number of bands.
#' @param dataType Character data type name.
#' (e.g., common data types include Byte, Int16, UInt16, Int32, Float32).
#' @param options Optional list of format-specific creation options in a
#' vector of `"NAME=VALUE"` pairs
#' (e.g., \code{options = c("COMPRESS=LZW")} to set LZW
#' compression during creation of a GTiff file).
#' The APPEND_SUBDATASET=YES option can be
#' specified to avoid prior destruction of existing dataset.
#' @param return_obj Logical scalar. If `TRUE`, an object of class
#' [`GDALRaster`][GDALRaster] opened on the newly created dataset will be
#' returned, otherwise returns a logical value. Defaults to `FALSE`.
#' @returns By default, returns a logical value indicating success (invisible
#' \code{TRUE}, output written to `dst_filename`). An error is raised if the
#' operation fails. An object of class [`GDALRaster`][GDALRaster] opened on the
#' output dataset will be returned if `return_obj = TRUE`.
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [createCopy()], [rasterFromRaster()],
#' [getCreationOptions()]
#' @examples
#' new_file <- file.path(tempdir(), "newdata.tif")
#' create(format="GTiff", dst_filename=new_file, xsize=143, ysize=107,
#'        nbands=1, dataType="Int16")
#' ds <- new(GDALRaster, new_file, read_only=FALSE)
#' ## EPSG:26912 - NAD83 / UTM zone 12N
#' ds$setProjection(epsg_to_wkt(26912))
#' gt <- c(323476.1, 30, 0, 5105082.0, 0, -30)
#' ds$setGeoTransform(gt)
#' ds$setNoDataValue(band = 1, -9999)
#' ds$fillRaster(band = 1, -9999, 0)
#' ## ...
#' ## close the dataset when done
#' ds$close()
#'
#' deleteDataset(new_file)
#' @export
create <- function(format, dst_filename, xsize, ysize, nbands, dataType,
                   options = NULL, return_obj = FALSE) {

    ds <- .create(format, dst_filename, xsize, ysize, nbands, dataType, options)

    if (return_obj) {
        return(ds)
    } else {
        ds$close()
        return(TRUE)
    }
}
