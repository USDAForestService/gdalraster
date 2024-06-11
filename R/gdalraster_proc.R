# R functions for various processing with gdalraster
# Chris Toney <chris.toney at usda.gov>


#' List of default nodata values by raster data type
#'
#' These values are currently used in `gdalraster` when a nodata value is
#' needed but has not been specified:
#' \preformatted{
#'     list("Byte" = 255, "Int8" = -128,
#'          "UInt16" = 65535, "Int16" = -32767,
#'          "UInt32" = 4294967293, "Int32" = -2147483647,
#'          "Float32" = -99999.0, "Float64" = -99999.0)
#' }
#' @export
DEFAULT_NODATA <- list(
    "Byte" = 255, "Int8" = -128,
    "UInt16" = 65535, "Int16" = -32767,
    "UInt32" = 4294967293, "Int32" = -2147483647,
    "Float32" = -99999.0, "Float64" = -99999.0
)


#' List of default DEM processing options
#'
#' These values are used in `dem_proc()` as the default processing options:
#' \preformatted{
#'     list(
#'          hillshade =    c("-z", "1", "-s", "1", "-az", "315",
#'                           "-alt", "45", "-alg", "Horn",
#'                           "-combined", "-compute_edges"),
#'          slope =        c("-s", "1", "-alg", "Horn", "-compute_edges"),
#'          aspect =       c("-alg", "Horn", "-compute_edges"),
#'          color_relief = character(),
#'          TRI =          c("-alg", "Riley", "-compute_edges"),
#'          TPI =          c("-compute_edges"),
#'          roughness =    c("-compute_edges")
#'          )
#' }
#' @seealso
#' [dem_proc()]
#'
#' \url{https://gdal.org/programs/gdaldem.html} for a description of all
#' available command-line options for each processing mode
#' @export
DEFAULT_DEM_PROC <- list(
    hillshade = c("-z", "1", "-s", "1", "-az", "315",
                  "-alt", "45", "-alg", "Horn",
                  "-combined", "-compute_edges"),
    slope = c("-s", "1", "-alg", "Horn", "-compute_edges"),
    aspect = c("-alg", "Horn", "-compute_edges"),
    color_relief = character(),
    TRI = c("-alg", "Riley", "-compute_edges"),
    TPI = c("-compute_edges"),
    roughness = c("-compute_edges")
)


.VRT_KERNEL_TEMPLATE <-
"<KernelFilteredSource>
  <SourceFilename relativeToVRT=\"%d\">%s</SourceFilename><SourceBand>%d</SourceBand>
  <SrcRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" ySize=\"%d\"/>
  <DstRect xOff=\"0\" yOff=\"0\" xSize=\"%d\" ySize=\"%d\"/>
  <Kernel normalized=\"%d\">
    <Size>%d</Size>
    <Coefs>%s</Coefs>
  </Kernel>
</KernelFilteredSource>"


#' @noRd
.getGDALformat <- function(file) {
    # Only for guessing common output formats
    file <- as.character(file)
    if (endsWith(tolower(file), ".img")) {
        return("HFA")
    }
    if (endsWith(tolower(file), ".tif")) {
        return("GTiff")
    }
    if (endsWith(tolower(file), ".vrt")) {
        return("VRT")
    }
    return(NULL)
}

#' @noRd
.getOGRformat <- function(file) {
    # Only for guessing common output formats
    file <- as.character(file)
    if (startsWith(file, "PG:")) {
        return("PostgreSQL")
    }
    if (endsWith(tolower(file), ".gpkg")) {
        return("GPKG")
    }
    if (endsWith(tolower(file), ".shp")) {
        return("ESRI Shapefile")
    }
    if (endsWith(tolower(file), ".sqlite")) {
        return("SQLite")
    }
    if (endsWith(tolower(file), ".fgb")) {
        return("FlatGeobuf")
    }
    return(NULL)
}


#' Get a pixel or line offset for a north-up raster
#' @param coord A georeferenced x or y
#' @param origin Raster xmin or ymax
#' @param gt_pixel_size Geotransform pixel x or y size (negative for y)
#' @returns pixel (column) or line (row) offset for coord x or y
#' @noRd
.getOffset <- function(coord, origin, gt_pixel_size) {
    (coord-origin)/gt_pixel_size
}


#' Convenience wrapper for `GDALRaster$read()`
#'
#' @description
#' `read_ds()` will read from a raster dataset that is already open in a
#' `GDALRaster` object. By default, it attempts to read the full raster
#' extent from all bands at full resolution. `read_ds()` is sometimes more
#' convenient than `GDALRaster$read()`, e.g., to read specific multiple bands
#' for display with [plot_raster()], or simply for the argument defaults to
#' read an entire raster into memory (see Note).
#'
#' @details
#' `NA` will be returned in place of the nodata value if the raster dataset has
#' a nodata value defined for the band. Data are read as R `integer` type when
#' possible for the raster data type (Byte, Int8, Int16, UInt16, Int32),
#' otherwise as type `double` (UInt32, Float32, Float64).
#'
#' The output object has attribute `gis`, a list containing:
#' \preformatted{
#'   $type = "raster"
#'   $bbox = c(xmin, ymin, xmax, ymax)
#'   $dim = c(xsize, ysize, nbands)
#'   $srs = <projection as WKT2 string>
#' }
#' The WKT version used for the projection string can be overridden by setting
#' the `OSR_WKT_FORMAT` configuration option. See [srs_to_wkt()] for a list of
#' supported values.
#'
#' @param ds An object of class `GDALRaster` in open state.
#' @param bands Integer vector of band numbers to read. By default all bands
#' will be read.
#' @param xoff Integer. The pixel (column) offset to the top left corner of the
#' raster region to be read (zero to start from the left side).
#' @param yoff Integer. The line (row) offset to the top left corner of the
#' raster region to be read (zero to start from the top).
#' @param xsize Integer. The width in pixels of the region to be read.
#' @param ysize Integer. The height in pixels of the region to be read.
#' @param out_xsize Integer. The width in pixels of the output buffer into
#' which the desired region will be read (e.g., to read a reduced resolution
#' overview).
#' @param out_ysize Integer. The height in pixels of the output buffer into
#' which the desired region will be read (e.g., to read a reduced resolution
#' overview).
#' @param as_list Logical. If `TRUE`, return output as a list of band vectors.
#' If `FALSE` (the default), output is a vector of pixel data interleaved by
#' band.
#' @param as_raw Logical. If `TRUE` and the underlying data type is Byte,
#' return output as R's raw vector type. This maps to the setting
#' `$readByteAsRaw` on the `GDALRaster` object, which is used to temporarily
#' update that field in this function. To control this behaviour in a
#' persistent way on a data set see \code{$readByteAsRaw} in
#' [`GDALRaster-class`][GDALRaster].
#' @returns If `as_list = FALSE` (the default), a `numeric` or `complex` vector
#' containing the values that were read. It is organized in left to right, top
#' to bottom pixel order, interleaved by band.
#' If `as_list = TRUE`, a list with number of elements equal to the number of
#' bands read. Each element contains a `numeric` or `complex` vector
#' containing the pixel data read for the band.
#'
#' @note
#' There is small overhead in calling `read_ds()` compared with
#' calling `GDALRaster$read()` directly. This would only matter if calling
#' the function repeatedly to read a raster in chunks. For the case of reading
#' a large raster in many chunks, it will be optimal performance-wise to call
#' `GDALRaster$read()` directly.
#'
#' By default, this function will attempt to read the full raster into memory.
#' It generally should not be called on large raster datasets using the default
#' argument values. The memory size in bytes of the returned vector will be
#' approximately (xsize * ysize * number of bands * 4) for data read as
#' `integer`, and (xsize * ysize * number of bands * 8) for data read as
#' `double` (plus small object overhead for the vector).
#'
#' @seealso
#' [`GDALRaster$read()`][GDALRaster]
#'
#' @examples
#' # read three bands from a multi-band dataset
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds <- new(GDALRaster, lcp_file)
#'
#' # as a vector of pixel data interleaved by band
#' r <- read_ds(ds, bands=c(6,5,4))
#' typeof(r)
#' length(r)
#' object.size(r)
#'
#' # as a list of band vectors
#' r <- read_ds(ds, bands=c(6,5,4), as_list=TRUE)
#' typeof(r)
#' length(r)
#' object.size(r)
#'
#' # gis attribute list
#' attr(r, "gis")
#'
#' ds$close()
#' @export
read_ds <- function(ds, bands=NULL, xoff=0, yoff=0,
                    xsize=ds$getRasterXSize(), ysize=ds$getRasterYSize(),
                    out_xsize=xsize, out_ysize=ysize,
                    as_list=FALSE, as_raw = FALSE) {

    if (is.null(bands))
        bands <- seq_len(ds$getRasterCount())

    if (as_list)
        r <- list()
    else
        r <- NULL

    i <- 1
    readByteAsRaw <- ds$readByteAsRaw
    if (as_raw) {
      ds$readByteAsRaw <- TRUE
      dtype <- ds$getDataTypeName(bands[1L])
      if (!dtype == "Byte") {
        warning(sprintf("'as_raw' set to 'TRUE' only affects read for band type 'Byte',  current data type: '%s'", dtype))
      }
    }
    for (b in bands) {
        if (as_list) {
            r[[i]] <- ds$read(b, xoff, yoff, xsize, ysize,
                              out_xsize, out_ysize)
            i <- i + 1
        } else {
            r <- c(r, ds$read(b, xoff, yoff, xsize, ysize,
                              out_xsize, out_ysize))
        }
    }

    ## restore the field, note that it may have had no impact
    ds$readByteAsRaw <- readByteAsRaw
    gt <- ds$getGeoTransform()
    ul_xy <- .apply_geotransform(gt, xoff, yoff)
    lr_xy <- .apply_geotransform(gt, (xoff + xsize), (yoff + ysize))
    bb <- c(ul_xy[1], lr_xy[2], lr_xy[1], ul_xy[2])

    # gis: a list with the raster bbox, dimensions, projection
    wkt_fmt_config <- get_config_option("OSR_WKT_FORMAT")
    if (wkt_fmt_config == "")
        set_config_option("OSR_WKT_FORMAT", "WKT2")
    attr(r, "gis") <- list(type = "raster",
                           bbox = bb,
                           dim = c(out_xsize, out_ysize, length(bands)),
                           srs = ds$getProjectionRef())
    set_config_option("OSR_WKT_FORMAT", wkt_fmt_config)

    return(r)
}


#' Create a raster from an existing raster as template
#'
#' @description
#' `rasterFromRaster()` creates a new raster with spatial reference,
#' extent and resolution taken from a template raster, without copying data.
#' Optionally changes the format, number of bands, data type and nodata value,
#' sets driver-specific dataset creation options, and initializes to a value.
#'
#' @param srcfile Source raster filename.
#' @param dstfile Output raster filename.
#' @param fmt Output raster format name (e.g., "GTiff" or "HFA"). Will attempt
#' to guess from the output filename if \code{fmt} is not specified.
#' @param nbands Number of output bands.
#' @param dtName Output raster data type name. Commonly used types include
#' `"Byte"`, `"Int16"`, `"UInt16"`, `"Int32"` and `"Float32"`.
#' @param options Optional list of format-specific creation options in a
#' vector of "NAME=VALUE" pairs
#' (e.g., \code{options = c("COMPRESS=LZW")} to set LZW compression
#' during creation of a GTiff file).
#' @param init Numeric value to initialize all pixels in the output raster.
#' @param dstnodata Numeric nodata value for the output raster.
#' @returns Returns the destination filename invisibly.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()],
#' [bandCopyWholeRaster()], [translate()]
#'
#' @examples
#' # band 2 in a FARSITE landscape file has slope degrees
#' # convert slope degrees to slope percent in a new raster
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds_lcp <- new(GDALRaster, lcp_file)
#' ds_lcp$getMetadata(band=2, domain="")
#'
#' slpp_file <- file.path(tempdir(), "storml_slpp.tif")
#' opt = c("COMPRESS=LZW")
#' rasterFromRaster(srcfile = lcp_file,
#'                  dstfile = slpp_file,
#'                  nbands = 1,
#'                  dtName = "Int16",
#'                  options = opt,
#'                  init = -32767)
#' ds_slp <- new(GDALRaster, slpp_file, read_only=FALSE)
#'
#' # slpp_file is initialized to -32767 and nodata value set
#' ds_slp$getNoDataValue(band=1)
#'
#' # extent and cell size are the same as lcp_file
#' ds_lcp$bbox()
#' ds_lcp$res()
#' ds_slp$bbox()
#' ds_slp$res()
#'
#' # convert slope degrees in lcp_file band 2 to slope percent in slpp_file
#' # bring through LCP nodata -9999 to the output nodata value
#' ncols <- ds_slp$getRasterXSize()
#' nrows <- ds_slp$getRasterYSize()
#' for (row in 0:(nrows-1)) {
#'     rowdata <- ds_lcp$read(band=2,
#'                            xoff=0, yoff=row,
#'                            xsize=ncols, ysize=1,
#'                            out_xsize=ncols, out_ysize=1)
#'     rowslpp <- tan(rowdata*pi/180) * 100
#'     rowslpp[rowdata==-9999] <- -32767
#'     dim(rowslpp) <- c(1, ncols)
#'     ds_slp$write(band=1, xoff=0, yoff=row, xsize=ncols, ysize=1, rowslpp)
#' }
#'
#' # min, max, mean, sd
#' ds_slp$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#'
#' ds_slp$close()
#' ds_lcp$close()
#' deleteDataset(slpp_file)
#' @export
rasterFromRaster <- function(srcfile, dstfile, fmt=NULL, nbands=NULL,
                             dtName=NULL, options=NULL, init=NULL,
                             dstnodata=init) {

    if (is.null(fmt)) {
        fmt <- .getGDALformat(dstfile)
        if (is.null(fmt)) {
            stop("use 'fmt' to specify a GDAL raster format name",
                 call. = FALSE)
        }
    }

    src_ds <- new(GDALRaster, srcfile, read.only=TRUE)
    if (is.null(nbands)) {
        nbands <- src_ds$getRasterCount()
    }
    nrows <- src_ds$getRasterYSize()
    ncols <- src_ds$getRasterXSize()
    gt <- src_ds$getGeoTransform()
    if (is.null(dtName))
        dtName <- src_ds$getDataTypeName(1)
    srs <- src_ds$getProjectionRef()
    src_ds$close()

    create(fmt, dstfile, ncols, nrows, nbands, dtName, options)
    dst_ds <- new(GDALRaster, dstfile, FALSE)
    dst_ds$setGeoTransform(gt)
    dst_ds$setProjection(srs)
    if (!is.null(dstnodata)) {
        for (b in 1:nbands) {
            dst_ds$setNoDataValue(b, dstnodata)
        }
    }

    if (!is.null(init)) {
        message("initializing destination raster...")
        for (b in 1:nbands) {
            dst_ds$fillRaster(b, init, 0)
        }
        message("done")
    }

    dst_ds$close()
    return(invisible(dstfile))
}


#' Create a GDAL virtual raster derived from one source dataset
#'
#' @description
#' `rasterToVRT()` creates a virtual raster dataset (VRT format) derived from
#' one source dataset with options for virtual subsetting, virtually resampling
#' the source data at a different pixel resolution, or applying a virtual
#' kernel filter. (See [buildVRT()] for virtual mosaicing.)
#'
#' @details
#' `rasterToVRT()` can be used to virtually clip and pixel-align
#' various raster layers with each other or in relation to vector
#' polygon boundaries. It also supports VRT kernel filtering.
#'
#' A VRT dataset is saved as a plain-text file with extension .vrt. This file
#' contains a description of the dataset in an XML format. The description
#' includes the source raster filename which can be a full path
#' (`relativeToVRT = FALSE`) or relative path (`relativeToVRT = TRUE`).
#' For relative path, `rasterToVRT()` assumes that the .vrt file will be in
#' the same directory as the source file and uses `basename(srcfile)`. The
#' elements of the XML schema describe how the source data will be read, along
#' with algorithms potentially applied and so forth. Documentation of the XML
#' format for .vrt is at:
#' \url{https://gdal.org/drivers/raster/vrt.html}.
#'
#' Since .vrt is a small plain-text file it is fast to write and requires
#' little storage space. Read performance is not degraded for certain simple
#' operations (e.g., virtual clip without resampling). Reading will be
#' slower for virtual resampling to a different pixel resolution or virtual
#' kernel filtering since the operations are performed on-the-fly (but .vrt
#' does not require the up front writing of a resampled or kernel-filtered
#' raster to a regular format). VRT is sometimes useful as an intermediate
#' raster in a series of processing steps, e.g., as a `tempfile` (the
#' default).
#'
#' GDAL VRT format has several capabilities and uses beyond those
#' covered by `rasterToVRT()`. See the URL above for a full discussion.
#'
#' @note
#' Pixel alignment is specified in terms of the source raster pixels (i.e.,
#' `srcfile` of the virtual raster). The use case in mind is virtually
#' clipping a raster to the bounding box of a vector polygon and keeping
#' pixels aligned with `srcfile` (`src_align = TRUE`). `src_align` would be
#' set to `FALSE` if the intent is "target alignment". For example, if
#' `subwindow` is the bounding box of another raster with a different layout,
#' then also setting `resolution` to the pixel resolution of the target raster
#' and `src_align = FALSE` will result in a virtual raster pixel-aligned with
#' the target (i.e., pixels in the virtual raster are no longer aligned with
#' its `srcfile`). Resampling defaults to `nearest` if not specified.
#' Examples for both cases of `src_align` are given below.
#'
#' `rasterToVRT()` assumes `srcfile` is a north-up raster.
#'
#' @param srcfile Source raster filename.
#' @param relativeToVRT Logical. Indicates whether the source filename should
#' be interpreted as relative to the .vrt file (`TRUE`) or not relative
#' to the .vrt file (`FALSE`, the default). If `TRUE`, the .vrt
#' file is assumed to be in the same directory as `srcfile` and
#' `basename(srcfile)` is used in the .vrt file. Use `TRUE` if the .vrt file
#' will always be stored in the same directory with `srcfile`.
#' @param vrtfile Output VRT filename.
#' @param resolution A numeric vector of length two (xres, yres). The pixel
#' size must be expressed in georeferenced units. Both must be positive values.
#' The source pixel size is used if `resolution` is not specified.
#' @param subwindow A numeric vector of length four (xmin, ymin, xmax, ymax).
#' Selects `subwindow` of the source raster with corners given in
#' georeferenced coordinates (in the source CRS).
#' If not given, the upper left corner of the VRT will be the
#' same as source, and the VRT extent will be the same or larger than source
#' depending on `resolution`.
#' @param src_align Logical.
#'   * `TRUE`: the upper left corner of the VRT extent will be set to the
#'   upper left corner of the source pixel that contains `subwindow` xmin, ymax.
#'   The VRT will be pixel-aligned with source if the VRT `resolution` is the
#'   same as the source pixel size, otherwise VRT extent will be the minimum
#'   rectangle that contains `subwindow` for the given pixel size.
#'   Often, `src_align=TRUE` when selecting a raster minimum bounding box
#'   for a vector polygon.
#'   * `FALSE`: the VRT upper left corner will be exactly `subwindow`
#'   xmin, ymax, and the VRT extent will be the minimum rectangle that contains
#'   `subwindow` for the given pixel size. If `subwindow` is not given, the
#'   source raster extent is used in which case `src_align=FALSE` has no effect.
#'   Use `src_align=FALSE` to pixel-align two rasters of different sizes, i.e.,
#'   when the intent is target alignment.
#' @param resampling The resampling method to use if xsize, ysize of the VRT is
#' different than the size of the underlying source rectangle (in number of
#' pixels). The values allowed are nearest, bilinear, cubic, cubicspline,
#' lanczos, average and mode (as character).
#' @param krnl A filtering kernel specified as pixel coefficients.
#' `krnl` is a array with dimensions (size, size), where
#' size must be an odd number. `krnl` can also be given as a vector with
#' length size x size. For example, a 3x3 average filter is given by:
#' \preformatted{
#' krnl <- c(
#' 0.11111, 0.11111, 0.11111,
#' 0.11111, 0.11111, 0.11111,
#' 0.11111, 0.11111, 0.11111)
#' }
#' A kernel cannot be applied to sub-sampled or over-sampled data.
#' @param normalized Logical. Indicates whether the kernel is normalized.
#' Defaults to `TRUE`.
#' @returns Returns the VRT filename invisibly.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [bbox_from_wkt()], [buildVRT()]
#'
#' [warp()] can write VRT for virtual reprojection
#'
#' @examples
#' ## resample
#'
#' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
#' ds <- new(GDALRaster, evt_file)
#' ds$res()
#' ds$bbox()
#' ds$close()
#'
#' # table of the unique pixel values and their counts
#' tbl <- buildRAT(evt_file)
#' print(tbl)
#' sum(tbl$COUNT)
#'
#' # resample at 90-m resolution
#' # EVT is thematic vegetation type so use a majority value
#' vrt_file <- rasterToVRT(evt_file,
#'                         resolution=c(90,90),
#'                         resampling="mode")
#'
#' # .vrt is a small xml file pointing to the source raster
#' file.size(vrt_file)
#'
#' tbl90m <- buildRAT(vrt_file)
#' print(tbl90m)
#' sum(tbl90m$COUNT)
#'
#' ds <- new(GDALRaster, vrt_file)
#' ds$res()
#' ds$bbox()
#' ds$close()
#' vsi_unlink(vrt_file)
#'
#'
#' ## clip
#'
#' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
#' ds_evt <- new(GDALRaster, evt_file)
#' ds_evt$bbox()
#'
#' # WKT string for a boundary within the EVT extent
#' bnd = "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
#' 5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
#' 325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
#'
#' # src_align = TRUE
#' vrt_file <- rasterToVRT(evt_file,
#'                         subwindow = bbox_from_wkt(bnd),
#'                         src_align=TRUE)
#' ds_vrt <- new(GDALRaster, vrt_file)
#'
#' # VRT is a virtual clip, pixel-aligned with the EVT raster
#' bbox_from_wkt(bnd)
#' ds_vrt$bbox()
#' ds_vrt$res()
#'
#' ds_vrt$close()
#' vsi_unlink(vrt_file)
#'
#' # src_align = FALSE
#' vrt_file <- rasterToVRT(evt_file,
#'                         subwindow = bbox_from_wkt(bnd),
#'                         src_align=FALSE)
#' ds_vrt_noalign <- new(GDALRaster, vrt_file)
#'
#' # VRT upper left corner (xmin, ymax) is exactly bnd xmin, ymax
#' ds_vrt_noalign$bbox()
#' ds_vrt_noalign$res()
#'
#' ds_vrt_noalign$close()
#' vsi_unlink(vrt_file)
#' ds_evt$close()
#'
#'
#' ## subset and pixel align two rasters
#'
#' # FARSITE landscape file for the Storm Lake area
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds_lcp <- new(GDALRaster, lcp_file)
#'
#' # Landsat band 5 file covering the Storm Lake area
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#' ds_b5 <- new(GDALRaster, b5_file)
#'
#' ds_lcp$bbox()  # 323476.1 5101872.0  327766.1 5105082.0
#' ds_lcp$res()   # 30 30
#'
#' ds_b5$bbox()   # 323400.9 5101815.8  327870.9 5105175.8
#' ds_b5$res()    # 30 30
#'
#' # src_align = FALSE because we need target alignment in this case:
#' vrt_file <- rasterToVRT(b5_file,
#'                         resolution = ds_lcp$res(),
#'                         subwindow = ds_lcp$bbox(),
#'                         src_align = FALSE)
#' ds_b5vrt <- new(GDALRaster, vrt_file)
#'
#' ds_b5vrt$bbox() # 323476.1 5101872.0  327766.1 5105082.0
#' ds_b5vrt$res()  # 30 30
#'
#' # read the the Landsat file pixel-aligned with the LCP file
#' # summarize band 5 reflectance where FBFM = 165
#' # LCP band 4 contains FBFM (a classification of fuel beds):
#' ds_lcp$getMetadata(band=4, domain="")
#'
#' # verify Landsat nodata (0):
#' ds_b5vrt$getNoDataValue(band=1)
#' # will be read as NA and omitted from stats
#' rs <- new(RunningStats, na_rm=TRUE)
#'
#' ncols <- ds_lcp$getRasterXSize()
#' nrows <- ds_lcp$getRasterYSize()
#' for (row in 0:(nrows-1)) {
#'     row_fbfm <- ds_lcp$read(band=4, xoff=0, yoff=row,
#'                             xsize=ncols, ysize=1,
#'                             out_xsize=ncols, out_ysize=1)
#'     row_b5 <- ds_b5vrt$read(band=1, xoff=0, yoff=row,
#'                             xsize=ncols, ysize=1,
#'                             out_xsize=ncols, out_ysize=1)
#' 	   rs$update(row_b5[row_fbfm == 165])
#' }
#' rs$get_count()
#' rs$get_mean()
#' rs$get_min()
#' rs$get_max()
#' rs$get_sum()
#' rs$get_var()
#' rs$get_sd()
#'
#' ds_b5vrt$close()
#' vsi_unlink(vrt_file)
#' ds_lcp$close()
#' ds_b5$close()
#' @export
rasterToVRT <- function(srcfile,
                        relativeToVRT = FALSE,
                        vrtfile = tempfile("tmprast", fileext=".vrt"),
                        resolution = NULL,
                        subwindow = NULL,
                        src_align = TRUE,
                        resampling = "nearest",
                        krnl = NULL,
                        normalized = TRUE) {

    if (relativeToVRT) relativeToVRT <- 1 else relativeToVRT <- 0
    if (normalized) normalized <- 1 else normalized <- 0

    src_ds <- new(GDALRaster, srcfile, read.only=TRUE)
    src_gt <- src_ds$getGeoTransform()
    src_bbox <- src_ds$bbox()
    src_xmin <- src_bbox[1]
    src_ymin <- src_bbox[2]
    src_xmax <- src_bbox[3]
    src_ymax <- src_bbox[4]
    src_xres <- src_gt[2]
    src_yres <- src_gt[6]
    src_bands <- src_ds$getRasterCount()
    src_ds$close()

    if (!is.null(resolution) && !is.null(krnl)) {
        if (src_xres != resolution[1] || src_yres != resolution[2])
            stop("cannot apply a kernel to sub-sampled or over-sampled data",
                 call. = FALSE)
    }

    if (!is.null(krnl)) {
        if ((length(krnl) %% 2) == 0)
            stop("kernel size must be an odd number", call. = FALSE)
    }

    tmp_vrtfile <- tempfile("src", fileext=".vrt")
    createCopy("VRT", tmp_vrtfile, srcfile)

    if (is.null(resolution)) {
        vrt_xres <- src_xres
        vrt_yres <- src_yres
    } else {
        vrt_xres <- resolution[1]
        vrt_yres <- -resolution[2]
    }

    # src offsets
    if (is.null(subwindow)) {
        subwindow <- c(src_xmin, src_ymin, src_xmax, src_ymax)
    } else {
        if (subwindow[1] < src_xmin || subwindow[3] > src_xmax ||
                subwindow[2] < src_ymin || subwindow[4] > src_ymax) {

            stop("'subwindow' is not within source raster extent",
                 call. = FALSE)
        }
    }
    src_xoff <- floor(.getOffset(subwindow[1], src_xmin, src_xres))
    src_yoff <- floor(.getOffset(subwindow[4], src_ymax, src_yres))

    #get vrt geotransform and size (assuming a north-up raster)
    if (src_align) {
        #lay out the vrt raster so it is aligned with ul corner
        #of ul src pixel
        vrt_xmin <- src_xmin + src_xoff * src_xres
        vrt_ymax <- src_ymax + src_yoff * src_yres
    } else {
        #lay out the vrt raster with origin at the subwindow origin
        vrt_xmin <- subwindow[1]
        vrt_ymax <- subwindow[4]
    }
    vrt_ncols <- ceiling(.getOffset(subwindow[3], vrt_xmin, vrt_xres))
    vrt_xmax <- (vrt_xmin + vrt_ncols * vrt_xres)
    vrt_nrows <- ceiling(.getOffset(subwindow[2], vrt_ymax, vrt_yres))
    vrt_ymin <- (vrt_ymax + vrt_nrows * vrt_yres)
    vrt_gt <- c(vrt_xmin, vrt_xres, 0, vrt_ymax, 0, vrt_yres)
    srcwin_xsize <- ceiling(.getOffset(vrt_xmax, vrt_xmin, src_xres))
    srcwin_ysize <- ceiling(.getOffset(vrt_ymin, vrt_ymax, src_yres))

    x <- xml2::read_xml(tmp_vrtfile)

    xds <- xml2::xml_find_first(x, "/VRTDataset")
    xml2::xml_attrs(xds) <- c("rasterXSize" = as.character(vrt_ncols),
                              "rasterYSize" = as.character(vrt_nrows))

    xgt <- xml2::xml_find_first(x, "/VRTDataset/GeoTransform")
    xml2::xml_text(xgt) <- paste(vrt_gt, collapse=", ")

    xpath <- "/VRTDataset/VRTRasterBand/SimpleSource/SrcRect"
    xsrcrect <- xml2::xml_find_all(x, xpath)
    xml2::xml_attrs(xsrcrect) <- c("xOff" = as.character(src_xoff),
                                   "yOff" = as.character(src_yoff),
                                   "xSize" = as.character(srcwin_xsize),
                                   "ySize" = as.character(srcwin_ysize))

    xpath <- "/VRTDataset/VRTRasterBand/SimpleSource/DstRect"
    xdstrect <- xml2::xml_find_all(x, xpath)
    xml2::xml_attrs(xdstrect) <- c("xOff" = "0",
                                   "yOff" = "0",
                                   "xSize" = as.character(vrt_ncols),
                                   "ySize" = as.character(vrt_nrows))

    if (vrt_ncols != srcwin_xsize || vrt_nrows != srcwin_ysize) {
        xpath <- "/VRTDataset/VRTRasterBand/SimpleSource"
        xssrc <- xml2::xml_find_all(x, xpath)
        xml2::xml_attr(xssrc, "resampling") <- resampling
    }

    xpath <- "/VRTDataset/VRTRasterBand/SimpleSource/SourceFilename"
    xfn <- xml2::xml_find_all(x, xpath)
    if (relativeToVRT) {
        #this assumes the VRT file will be in the same directory as source file
        xml2::xml_attr(xfn, "relativeToVRT") <- "1"
        xml2::xml_text(xfn) <- basename(srcfile)
    } else {
        xml2::xml_attr(xfn, "relativeToVRT") <- "0"
        xml2::xml_text(xfn) <- srcfile
    }

    # clear statistics metadata if present
    xsrcstats <- xml2::xml_find_all(x,
                                    "//MDI[starts-with(@key, 'STATISTICS_')]")
    if (length(xsrcstats) != 0) {
        xml2::xml_text(xsrcstats) <- rep("", length(xsrcstats))
    }

    xml2::write_xml(x, vrtfile, options=c("format", "no_declaration"))

    if (!is.null(krnl)) {
        vrt_ds <- new(GDALRaster, vrtfile, read_only=FALSE)
        if (relativeToVRT) srcfile <- basename(srcfile)
        for (band in 1:src_bands) {
            krnl_xml <- enc2utf8(sprintf(.VRT_KERNEL_TEMPLATE,
                                         relativeToVRT,
                                         srcfile,
                                         band,
                                         src_xoff,
                                         src_yoff,
                                         srcwin_xsize,
                                         srcwin_ysize,
                                         vrt_ncols,
                                         vrt_nrows,
                                         normalized,
                                         sqrt(length(krnl)),
                                         paste(krnl, collapse=" ")))

            vrt_ds$setMetadataItem(band, "source_0", krnl_xml, "vrt_sources")
        }
        vrt_ds$close()
    }

    return(invisible(vrtfile))
}


#' Raster calculation
#'
#' @description
#' `calc()` evaluates an R expression for each pixel in a raster layer or
#' stack of layers. Each layer is defined by a raster filename, band number,
#' and a variable name to use in the R expression. If not specified, band
#' defaults to 1 for each input raster.
#' Variable names default to `LETTERS` if not specified
#' (`A` (layer 1), `B` (layer 2), ...).
#' All of the input layers must have the same extent and cell size.
#' The projection will be read from the first raster in the list
#' of inputs.
#' Individual pixel coordinates are also available as variables in the
#' R expression, as either x/y in the raster projected coordinate system or
#' inverse projected longitude/latitude.
#' Multiband output is supported as of gdalraster 1.11.0.
#'
#' @details
#' The variables in `expr` are vectors of length raster xsize
#' (row vectors of the input raster layer(s)).
#' The expression should return a vector also of length raster xsize
#' (an output row).
#' Four special variable names are available in `expr`:
#' `pixelX` and `pixelY` provide pixel center coordinates in projection units.
#' `pixelLon` and `pixelLat` can also be used, in which case the pixel x/y
#' coordinates will be inverse projected to longitude/latitude
#' (in the same geographic coordinate system used by the input projection,
#' which is read from the first input raster). Note that inverse projection
#' adds computation time.
#'
#' To refer to specific bands in a multi-band input file, repeat the filename in
#' `rasterfiles` and specify corresponding band numbers in `bands`, along with
#' optional variable names in `var.names`, for example,
#' \preformatted{
#' rasterfiles = c("multiband.tif", "multiband.tif")
#' bands = c(4, 5)
#' var.names = c("B4", "B5")
#' }
#'
#' Output will be written to `dstfile`. To update a file that already
#' exists, set `write_mode = "update"` and set `out_band` to an existing
#' band number(s) in `dstfile` (new bands cannot be created in `dstfile`).
#'
#' To write multiband output, `expr` must return a vector of values
#' interleaved by band. This is equivalent to, and can also be returned as,
#' a matrix `m` with `nrow(m)` equal to `length()` of an input vector, and
#' `ncol(m)` equal to the number of output bands. In matrix form, each column
#' contains a vector of output values for a band.
#' `length(m)` must be equal to the `length()` of an input vector multiplied by
#' `length(out_band)`. The dimensions described above are assumed and not
#' read from the return value of `expr`.
#'
#' @param expr An R expression as a character string (e.g., `"A + B"`).
#' @param rasterfiles Character vector of source raster filenames.
#' @param bands Integer vector of band numbers to use for each raster layer.
#' @param var.names Character vector of variable names to use for each raster
#' layer.
#' @param dstfile Character filename of output raster.
#' @param fmt Output raster format name (e.g., "GTiff" or "HFA"). Will attempt
#' to guess from the output filename if not specified.
#' @param dtName Character name of output data type (e.g., Byte, Int16,
#' UInt16, Int32, UInt32, Float32).
#' @param out_band Integer band number(s) in `dstfile` for writing output.
#' Defaults to `1`. Multiband output is supported as of gdalraster 1.11.0,
#' in which case `out_band` would be a vector of band numbers.
#' @param options Optional list of format-specific creation options in a
#' vector of "NAME=VALUE" pairs
#' (e.g., \code{options = c("COMPRESS=LZW")} to set LZW compression
#' during creation of a GTiff file).
#' @param nodata_value Numeric value to assign if `expr` returns NA.
#' @param setRasterNodataValue Logical. `TRUE` will attempt to set the raster
#' format nodata value to `nodata_value`, or `FALSE` not to set a raster
#' nodata value.
#' @param usePixelLonLat This argument is deprecated and will be removed in a
#' future version. Variable names `pixelLon` and `pixelLat` can be used in
#' `expr`, and the pixel x/y coordinates will be inverse projected to
#' longitude/latitude (adds computation time).
#' @param write_mode Character. Name of the file write mode for output.
#' One of:
#'   * `safe` - execution stops if `dstfile` already exists (no output written)
#'   * `overwrite` - if `dstfile` exists if will be overwritten with a new file
#'   * `update` - if `dstfile` exists, will attempt to open in update mode
#'   and write output to `out_band`
#' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
#' displayed. Defaults to `FALSE`.
#' @returns Returns the output filename invisibly.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [combine()], [rasterToVRT()]
#'
#' @examples
#' ## Using pixel longitude/latitude
#'
#' # Hopkins bioclimatic index (HI) as described in:
#' # Bechtold, 2004, West. J. Appl. For. 19(4):245-251.
#' # Integrates elevation, latitude and longitude into an index of the
#' # phenological occurrence of springtime. Here it is relativized to
#' # mean values for an eight-state region in the western US.
#' # Positive HI means spring is delayed by that number of days relative
#' # to the reference position, while negative values indicate spring is
#' # advanced. The original equation had elevation units as feet, so
#' # converting m to ft in `expr`.
#'
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#'
#' # expression to calculate HI
#' expr <- "round( ((ELEV_M * 3.281 - 5449) / 100) +
#'                 ((pixelLat - 42.16) * 4) +
#'                 ((-116.39 - pixelLon) * 1.25) )"
#'
#' # calc() writes to a tempfile by default
#' hi_file <- calc(expr = expr,
#'                 rasterfiles = elev_file,
#'                 var.names = "ELEV_M",
#'                 dtName = "Int16",
#'                 nodata_value = -32767,
#'                 setRasterNodataValue = TRUE)
#'
#' ds <- new(GDALRaster, hi_file)
#' # min, max, mean, sd
#' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#' ds$close()
#' deleteDataset(hi_file)
#'
#'
#' ## Calculate normalized difference vegetation index (NDVI)
#'
#' # Landast band 4 (red) and band 5 (near infrared):
#' b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#'
#' expr <- "((B5 * 0.0000275 - 0.2) - (B4 * 0.0000275 - 0.2)) /
#'          ((B5 * 0.0000275 - 0.2) + (B4 * 0.0000275 - 0.2))"
#' ndvi_file <- calc(expr = expr,
#'                   rasterfiles = c(b4_file, b5_file),
#'                   var.names = c("B4", "B5"),
#'                   dtName = "Float32",
#'                   nodata_value = -32767,
#'                   setRasterNodataValue = TRUE)
#'
#' ds <- new(GDALRaster, ndvi_file)
#' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#' ds$close()
#' deleteDataset(ndvi_file)
#'
#'
#' ## Reclassify a variable by rule set
#'
#' # Combine two raster layers and look for specific combinations. Then
#' # recode to a new value by rule set.
#' #
#' # Based on example in:
#' #   Stratton, R.D. 2009. Guidebook on LANDFIRE fuels data acquisition,
#' #   critique, modification, maintenance, and model calibration.
#' #   Gen. Tech. Rep. RMRS-GTR-220. U.S. Department of Agriculture,
#' #   Forest Service, Rocky Mountain Research Station. 54 p.
#' # Context: Refine national-scale fuels data to improve fire simulation
#' #   results in localized applications.
#' # Issue: Areas with steep slopes (40+ degrees) were mapped as
#' #   GR1 (101; short, sparse dry climate grass) and
#' #   GR2 (102; low load, dry climate grass) but were not carrying fire.
#' # Resolution: After viewing these areas in Google Earth,
#' #   NB9 (99; bare ground) was selected as the replacement fuel model.
#'
#' # look for combinations of slope >= 40 and FBFM 101 or 102
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' rasterfiles <- c(lcp_file, lcp_file)
#' var.names <- c("SLP", "FBFM")
#' bands <- c(2, 4)
#' tbl <- combine(rasterfiles, var.names, bands)
#' nrow(tbl)
#' tbl_subset <- subset(tbl, SLP >= 40 & FBFM %in% c(101,102))
#' print(tbl_subset)       # twelve combinations meet the criteria
#' sum(tbl_subset$count)   # 85 total pixels
#'
#' # recode these pixels to 99 (bare ground)
#' # the LCP driver does not support in-place write so make a copy as GTiff
#' tif_file <- file.path(tempdir(), "storml_lndscp.tif")
#' createCopy("GTiff", tif_file, lcp_file)
#'
#' expr <- "ifelse( SLP >= 40 & FBFM %in% c(101,102), 99, FBFM)"
#' calc(expr = expr,
#'      rasterfiles = c(lcp_file, lcp_file),
#'      bands = c(2, 4),
#'      var.names = c("SLP", "FBFM"),
#'      dstfile = tif_file,
#'      out_band = 4,
#'      write_mode = "update")
#'
#' # verify the ouput
#' rasterfiles <- c(tif_file, tif_file)
#' tbl <- combine(rasterfiles, var.names, bands)
#' tbl_subset <- subset(tbl, SLP >= 40 & FBFM %in% c(101,102))
#' print(tbl_subset)
#' sum(tbl_subset$count)
#'
#' # if LCP file format is needed:
#' # createCopy("LCP", "storml_edited.lcp", tif_file)
#'
#' deleteDataset(tif_file)
#' @export
calc <- function(expr,
                 rasterfiles,
                 bands = NULL,
                 var.names = NULL,
                 dstfile = tempfile("rastcalc", fileext=".tif"),
                 fmt = NULL,
                 dtName = "Int16",
                 out_band = NULL,
                 options = NULL,
                 nodata_value = NULL,
                 setRasterNodataValue = FALSE,
                 usePixelLonLat = NULL,
                 write_mode = "safe",
                 quiet = FALSE) {

    calc_expr <- parse(text=expr)

    if (write_mode == "safe" && file.exists(dstfile))
        stop("'dstfile' already exists and 'write_mode' is \"safe\"",
             call. = FALSE)

    if (write_mode == "update" && is.null(out_band))
        stop("'out_band' must be specified for \"update\" mode",
             call. = FALSE)

    if (write_mode == "update") {
        update_mode <- TRUE
    } else if (write_mode == "safe" || write_mode == "overwrite") {
        update_mode <- FALSE
        if (is.null(out_band))
            out_band <- 1
    } else {
        stop("unknown 'write_mode'", call. = FALSE)
    }

    nrasters <- length(rasterfiles)

    if (!is.null(bands)) {
        if (length(bands) != nrasters) {
            stop("'bands' must have same length as 'rasterfiles'",
                 call. = FALSE)
        }
    } else {
        bands <- rep(1, nrasters)
    }

    if (!is.null(var.names)) {
        if (length(var.names) != nrasters) {
            stop("'var.names' must have same length as 'rasterfiles'",
                 call. = FALSE)
        }
    } else {
        var.names <- LETTERS[1:nrasters]
    }

    for (nm in var.names) {
        if (!length(grep(nm, expr, fixed = TRUE)))
            stop("variable name '", nm, "' not in 'expr'", call. = FALSE)
    }

    if (is.null(fmt)) {
        fmt <- .getGDALformat(dstfile)
        if (is.null(fmt)) {
            stop("use 'fmt' to specify a GDAL raster format code",
                 call. = FALSE)
        }
    }

    if (is.null(nodata_value)) {
        nodata_value <- DEFAULT_NODATA[[dtName]]
        if (is.null(nodata_value)) {
            stop("a default nodata value is not available for 'dtName'",
                 call. = FALSE)
        }
    }

    # use first raster as reference
    ref <- new(GDALRaster, rasterfiles[1], TRUE)
    nrows <- ref$getRasterYSize()
    ncols <- ref$getRasterXSize()
    cellsizeX <- ref$res()[1]
    cellsizeY <- ref$res()[2]
    xmin <- ref$bbox()[1]
    ymax <- ref$bbox()[4]
    srs <- ref$getProjectionRef()
    ref$close()

    if (nrasters > 1) {
        for (r in rasterfiles) {
            ds <- new(GDALRaster, r, read_only=TRUE)
            dm <- ds$dim()
            ds$close()
            if (dm[2] != nrows || dm[1] != ncols) {
                message(r)
                stop("all input rasters must have the same extent",
                     call. = FALSE)
            }
        }
    }

    if (update_mode) {
        # write to an existing raster
        dst_ds <- new(GDALRaster, dstfile, read_only=FALSE)
    } else {
        # create the output raster
        dstnodata <- NULL
        if (setRasterNodataValue)
            dstnodata <- nodata_value

        rasterFromRaster(rasterfiles[1],
                         dstfile,
                         fmt,
                         nbands = length(out_band),
                         dtName = dtName,
                         options = options,
                         dstnodata = dstnodata)
        dst_ds <- new(GDALRaster, dstfile, read_only=FALSE)
    }

    # list of GDALRaster objects for each raster layer
    ds_list <- list()
    for (i in seq_len(nrasters)) {
        ds_list[[i]] <- new(GDALRaster, rasterfiles[i], read_only=TRUE)
    }

    # are pixel coordinates being used
    usePixelX <- FALSE
    usePixelY <- FALSE
    usePixelLonLat <- FALSE
    if (length(grep("pixelX", expr, fixed = TRUE)))
        usePixelX <- TRUE

    if (length(grep("pixelY", expr, fixed = TRUE)))
        usePixelY <- TRUE

    if (length(grep("pixelLon", expr, fixed = TRUE)) ||
            length(grep("pixelLat", expr, fixed = TRUE)))
        usePixelLonLat <- TRUE

    if (usePixelLonLat) {
        usePixelX <- TRUE
        usePixelY <- TRUE
    }

    if (usePixelX) {
        x <- seq(from = xmin + (cellsizeX/2),
                 by = cellsizeX,
                 length.out = ncols)
        assign("pixelX", x)
    }

    # expected size of vector returned by expr
    num_out_bands <- length(out_band)
    expect_outrow_len <- ncols * num_out_bands

    process_row <- function(row) {
        if (usePixelY) {
            y <- rep((ymax - (cellsizeY / 2) - (cellsizeY * row)), ncols)
            assign("pixelY", y)
        }

        if (usePixelLonLat) {
            lonlat <- inv_project(cbind(x, y), srs)
            assign("pixelLon", lonlat[, 1])
            assign("pixelLat", lonlat[, 2])
        }

        for (i in seq_len(nrasters)) {
            inrow <- ds_list[[i]]$read(band = bands[i],
                                       xoff = 0,
                                       yoff = row,
                                       xsize = ncols,
                                       ysize = 1,
                                       out_xsize = ncols,
                                       out_ysize = 1)
            assign(var.names[i], inrow)
        }

        outrow <- eval(calc_expr)
        if (length(outrow) != expect_outrow_len) {
            dst_ds$close()
            for (i in seq_len(nrasters)) {
                ds_list[[i]]$close()
            }
            stop("result vector is the wrong size", call. = FALSE)
        }
        outrow <- ifelse(is.na(outrow), nodata_value, outrow)
        dim(outrow) <- c(ncols, num_out_bands)
        i <- 1
        for (b in out_band) {
            dst_ds$write(band = b,
                         offx = 0,
                         offy = row,
                         xsize = ncols,
                         ysize = 1,
                         rasterData = outrow[, i])
            i <- i + 1
        }
        if (!quiet)
            setTxtProgressBar(pb, row+1)

        return()
    }

    if (!quiet) {
        message("calculating from ", nrasters, " input layer(s)...")
        pb <- txtProgressBar(min=0, max=nrows)
    }
    lapply(0:(nrows-1), process_row)
    close(pb)

    message("output written to: ", dstfile)
    dst_ds$close()
    for (i in seq_len(nrasters)) {
        ds_list[[i]]$close()
    }

    return(invisible(dstfile))
}


#' Raster overlay for unique combinations
#'
#' @description
#' `combine()` overlays multiple rasters so that a unique ID is assigned to
#' each unique combination of input values. The input raster layers
#' typically have integer data types (floating point will be coerced to
#' integer by truncation), and must have the same projection, extent and cell
#' size. Pixel counts for each unique combination are obtained, and
#' combination IDs are optionally written to an output raster.
#'
#' @details
#' To specify input raster layers that are bands of a multi-band
#' raster file, repeat the filename in `rasterfiles` and provide the
#' corresponding band numbers in `bands`. For example:
#' \preformatted{
#' rasterfiles <- c("multi-band.tif", "multi-band.tif", "other.tif")
#' bands <- c(4, 5, 1)
#' var.names <- c("multi_b4", "multi_b5", "other")
#' }
#'
#' [rasterToVRT()] provides options for virtual clipping, resampling and pixel
#' alignment, which may be helpful here if the input rasters are not already
#' aligned on a common extent and cell size.
#'
#' If an output raster of combination IDs is written, the user should verify
#' that the number of combinations obtained did not exceed the range of the
#' output data type. Combination IDs are sequential integers starting at 1.
#' Typical output data types are the unsigned types:
#' Byte (0 to 255), UInt16 (0 to 65,535) and UInt32 (the default, 0 to
#' 4,294,967,295).
#'
#' @param rasterfiles Character vector of raster filenames to combine.
#' @param var.names Character vector of `length(rasterfiles)` containing
#' variable names for each raster layer. Defaults will be assigned if
#' `var.names` are omitted.
#' @param bands Numeric vector of `length(rasterfiles)` containing the band
#' number to use for each raster in `rasterfiles`. Band 1 will be used for
#' each input raster if `bands` are not specified.
#' @param dstfile Character. Optional output raster filename for writing the
#' per-pixel combination IDs. The output raster will be created (and
#' overwritten if it already exists).
#' @param fmt Character. Output raster format name (e.g., "GTiff" or "HFA").
#' @param dtName Character. Output raster data type name. Combination IDs are
#' sequential integers starting at 1. The data type for the output raster
#' should be large enough to accommodate the potential number of unique
#' combinations of the input values
#' (e.g., "UInt16" or the default "UInt32").
#' @param options Optional list of format-specific creation options in a
#' vector of "NAME=VALUE" pairs
#' (e.g., \code{options = c("COMPRESS=LZW")} to set LZW compression
#' during creation of a GTiff file).
#' @param quiet Logical scalar. If `TRUE`, progress bar and messages will be
#' suppressed. Defaults to `FALSE`.
#' @returns A data frame with column `cmbid` containing the combination IDs,
#' column `count` containing the pixel counts for each combination,
#' and `length(rasterfiles)` columns named `var.names` containing the integer
#' values comprising each unique combination.
#'
#' @seealso
#' [`CmbTable-class`][CmbTable], [`GDALRaster-class`][GDALRaster], [calc()],
#' [rasterToVRT()]
#'
#' [buildRAT()] to compute a table of the unique pixel values and their counts
#' for a single raster layer
#'
#' @examples
#' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
#' evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")
#' evh_file <- system.file("extdata/storml_evh.tif", package="gdalraster")
#' rasterfiles <- c(evt_file, evc_file, evh_file)
#' var.names <- c("veg_type", "veg_cov", "veg_ht")
#' tbl <- combine(rasterfiles, var.names)
#' nrow(tbl)
#' tbl <- tbl[order(-tbl$count),]
#' head(tbl, n = 20)
#'
#' # combine two bands from a multi-band file and write the combination IDs
#' # to an output raster
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' rasterfiles <- c(lcp_file, lcp_file)
#' bands <- c(4, 5)
#' var.names <- c("fbfm", "tree_cov")
#' cmb_file <- file.path(tempdir(), "fbfm_cov_cmbid.tif")
#' opt <- c("COMPRESS=LZW")
#' tbl <- combine(rasterfiles, var.names, bands, cmb_file, options = opt)
#' head(tbl)
#' ds <- new(GDALRaster, cmb_file)
#' ds$info()
#' ds$close()
#' deleteDataset(cmb_file)
#' @export
combine <- function(rasterfiles, var.names=NULL, bands=NULL,
                    dstfile=NULL, fmt=NULL, dtName="UInt32",
                    options=NULL, quiet=FALSE) {

    if ((!is.null(dstfile)) && (is.null(fmt))) {
        fmt <- .getGDALformat(dstfile)
        if (is.null(fmt)) {
            stop("use 'fmt' to specify a GDAL raster format name",
                 call. = FALSE)
        }
    }

    if (!is.null(options)) {
        if (!is.character(options))
            stop("'options' must be a character vector", call. = FALSE)
    }

    if (is.null(dstfile))
        dstfile <- ""

    if (is.null(fmt))
        fmt <- ""

    nrasters <- length(rasterfiles)
    if (is.null(var.names)) {
        for (n in 1:nrasters) {
            var.names[n] <- tools::file_path_sans_ext(basename(rasterfiles[n]))
        }
    }

    if (is.null(bands)) {
        bands <- rep(1, nrasters)
    }

    ref_ds <- new(GDALRaster, rasterfiles[1], read.only=TRUE)
    ref_nrows <- ref_ds$getRasterYSize()
    ref_ncols <- ref_ds$getRasterXSize()
    ref_res <- ref_ds$res()
    ref_ds$close()

    if (nrasters > 1) {
        for (i in 2:length(rasterfiles)) {
            ds <- new(GDALRaster, rasterfiles[i], read.only=TRUE)
            if ((ds$getRasterYSize() != ref_nrows) ||
                    (ds$getRasterXSize() != ref_ncols))
                stop("all input rasters must have the same extent/cell size",
                     call. = FALSE)
            if (!all(ds$res() == ref_res))
                stop("all input rasters must have the same extent/cell size",
                     call. = FALSE)
            ds$close()
        }
    }

    d <- .combine(rasterfiles, var.names, bands, dstfile, fmt, dtName,
                  options, quiet)

    return(d)
}


#' GDAL DEM processing
#'
#' @description
#' `dem_proc()` generates DEM derivatives from an input elevation raster. This
#' function is a wrapper for the \command{gdaldem} command-line utility.
#' See \url{https://gdal.org/programs/gdaldem.html} for details.
#'
#' @param mode Character. Name of the DEM processing mode. One of hillshade,
#' slope, aspect, color-relief, TRI, TPI or roughness.
#' @param srcfile Filename of the source elevation raster.
#' @param dstfile Filename of the output raster.
#' @param mode_options An optional character vector of command-line options
#' (see [DEFAULT_DEM_PROC] for default values).
#' @param color_file Filename of a text file containing lines formatted as:
#' "elevation_value red green blue". Only used when `mode = "color-relief"`.
#' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
#' displayed. Defaults to `FALSE`.
#' @returns Logical indicating success (invisible `TRUE`).
#' An error is raised if the operation fails.
#'
#' @note
#' Band 1 of the source elevation raster is read by default, but this can be
#' changed by including a `-b` command-line argument in `mode_options`.
#' See the \href{https://gdal.org/programs/gdaldem.html}{documentation for
#' `gdaldem`} for a description of all available options for each processing
#' mode.
#'
#' @examples
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' slp_file <- file.path(tempdir(), "storml_slp.tif")
#' dem_proc("slope", elev_file, slp_file)
#'
#' deleteDataset(slp_file)
#' @export
dem_proc <- function(mode,
                     srcfile,
                     dstfile,
                     mode_options = DEFAULT_DEM_PROC[[mode]],
                     color_file = NULL,
                     quiet = FALSE) {

    if (is.null(DEFAULT_DEM_PROC[[mode]]))
        stop("DEM processing 'mode' not recognized", call.=FALSE)

    return(invisible(.dem_proc(mode,
                               srcfile,
                               dstfile,
                               mode_options,
                               color_file,
                               quiet)))
}


#' Create a polygon feature layer from raster data
#'
#' @description
#' `polygonize()` creates vector polygons for all connected regions of pixels
#' in a source raster sharing a common pixel value. Each polygon is created
#' with an attribute indicating the pixel value of that polygon. A raster mask
#' may also be provided to determine which pixels are eligible for processing.
#' The function will create the output vector layer if it does not already
#' exist, otherwise it will try to append to an existing one.
#' This function is a wrapper of `GDALPolygonize` in the GDAL Algorithms API.
#' It provides essentially the same functionality as the `gdal_polygonize.py`
#' command-line program (\url{https://gdal.org/programs/gdal_polygonize.html}).
#'
#' @details
#' Polygon features will be created on the output layer, with polygon
#' geometries representing the polygons. The polygon geometries will be in the
#' georeferenced coordinate system of the raster (based on the geotransform of
#' the source dataset). It is acceptable for the output layer to already have
#' features. If the output layer does not already exist, it will be created
#' with coordinate system matching the source raster.
#'
#' The algorithm attempts to minimize memory use so that very large rasters can
#' be processed. However, if the raster has many polygons or very large/complex
#' polygons, the memory use for holding polygon enumerations and active polygon
#' geometries may grow to be quite large.
#'
#' The algorithm will generally produce very dense polygon geometries, with
#' edges that follow exactly on pixel boundaries for all non-interior pixels.
#' For non-thematic raster data (such as satellite images) the result will
#' essentially be one small polygon per pixel, and memory and output layer
#' sizes will be substantial. The algorithm is primarily intended for
#' relatively simple thematic rasters, masks, and classification results.
#'
#' @param raster_file Filename of the source raster.
#' @param out_dsn The destination vector filename to which the polygons will be
#' written (or database connection string).
#' @param out_layer Name of the layer for writing the polygon features. For
#' single-layer file formats such as `"ESRI Shapefile"`, the layer name is the
#' same as the filename without the path or extension (e.g., `out_dsn =
#' "path_to_file/polygon_output.shp"`, the layer name is `"polygon_output"`).
#' @param fld_name Name of an integer attribute field in `out_layer` to which
#' the pixel values will be written. Will be created if necessary when using an
#' existing layer.
#' @param out_fmt GDAL short name of the output vector format. If unspecified,
#' the function will attempt to guess the format from the filename/connection
#' string.
#' @param connectedness Integer scalar. Must be either `4` or `8`. For the
#' default 4-connectedness, pixels with the same value are considered connected
#' only if they touch along one of the four sides, while 8-connectedness
#' also includes pixels that touch at one of the corners.
#' @param src_band The band on `raster_file` to build the polygons from
#' (default is `1`).
#' @param mask_file Use the first band of the specified raster as a
#' validity mask (zero is invalid, non-zero is valid). If not specified, the
#' default validity mask for the input band (such as nodata, or alpha masks)
#' will be used (unless `nomask` is set to `TRUE`).
#' @param nomask Logical scalar. If `TRUE`, do not use the default validity
#' mask for the input band (such as nodata, or alpha masks).
#' Default is `FALSE`.
#' @param overwrite Logical scalar. If `TRUE`, overwrite `out_layer` if it
#' already exists. Default is `FALSE`.
#' @param dsco Optional character vector of format-specific creation options
#' for `out_dsn` (`"NAME=VALUE"` pairs).
#' @param lco Optional character vector of format-specific creation options
#' for `out_layer` (`"NAME=VALUE"` pairs).
#' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
#' displayed. Defaults to `FALSE`.
#'
#' @note
#' The source pixel band values are read into a signed 64-bit integer buffer
#' (`Int64`) by `GDALPolygonize`, so floating point or complex bands will be
#' implicitly truncated before processing.
#'
#' When 8-connectedness is used, many of the resulting polygons will likely be
#' invalid due to ring self-intersection (in the strict OGC definition of
#' polygon validity). They may be suitable as-is for certain purposes such as
#' calculating geometry attributes (area, perimeter). Package **sf** has
#' `st_make_valid()`, PostGIS has `ST_MakeValid()`, and QGIS has vector
#' processing utility "Fix geometries" (single polygons can become MultiPolygon
#' in the case of self-intersections).
#'
#' If writing to a SQLite database format as either `GPKG` (GeoPackage
#' vector) or `SQLite` (Spatialite vector), setting the
#' `SQLITE_USE_OGR_VFS` and `OGR_SQLITE_JOURNAL` configuration options may
#' increase performance substantially. If writing to `PostgreSQL`
#' (PostGIS vector), setting `PG_USE_COPY=YES` is faster:
#' ```
#' # SQLite: GPKG (.gpkg) and Spatialite (.sqlite)
#' # enable extra buffering/caching by the GDAL/OGR I/O layer
#' set_config_option("SQLITE_USE_OGR_VFS", "YES")
#' # set the journal mode for the SQLite database to MEMORY
#' set_config_option("OGR_SQLITE_JOURNAL", "MEMORY")
#'
#' # PostgreSQL / PostGIS
#' # use COPY for inserting data rather than INSERT
#' set_config_option("PG_USE_COPY", "YES")
#' ```
#'
#' @seealso
#' [rasterize()]
#'
#' `vignette("gdal-config-quick-ref")`
#'
#' @examples
#' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
#' dsn <- file.path(tempdir(), "storm_lake.gpkg")
#' layer <- "lf_evt"
#' fld <- "evt_value"
#' set_config_option("SQLITE_USE_OGR_VFS", "YES")
#' set_config_option("OGR_SQLITE_JOURNAL", "MEMORY")
#' polygonize(evt_file, dsn, layer, fld)
#' set_config_option("SQLITE_USE_OGR_VFS", "")
#' set_config_option("OGR_SQLITE_JOURNAL", "")
#' deleteDataset(dsn)
#' @export
polygonize <- function(raster_file,
                       out_dsn,
                       out_layer,
                       fld_name = "DN",
                       out_fmt = NULL,
                       connectedness = 4,
                       src_band = 1,
                       mask_file = NULL,
                       nomask = FALSE,
                       overwrite = FALSE,
                       dsco = NULL,
                       lco = NULL,
                       quiet = FALSE) {

    if (connectedness !=4 && connectedness != 8)
        stop("'connectedness' must be either 4 or 8", call. = FALSE)

    ds <- new(GDALRaster, raster_file, TRUE)
    srs <- ds$getProjectionRef()
    ds$close()

    if (!is.null(mask_file)) {
        ds <- new(GDALRaster, mask_file, TRUE)
        ds$close()
    } else {
        mask_file <- ""
    }

    if (!.ogr_ds_exists(out_dsn, with_update=TRUE)) {
        if (.ogr_ds_exists(out_dsn) && !overwrite) {
            msg <- "'out_dsn' exists but cannot be updated.\n"
            msg <- paste0(msg, "You may need to remove it first, ")
            msg <- paste0(msg, "or use 'overwrite = TRUE'.")
            stop(msg, call. = FALSE)
        }
    }

    if (.ogr_ds_exists(out_dsn, with_update=TRUE) && overwrite) {
        deleted <- FALSE
        if (.ogr_layer_exists(out_dsn, out_layer)) {
            deleted <- .ogr_layer_delete(out_dsn, out_layer)
        }
        if (!deleted) {
            if (.ogr_ds_layer_count(out_dsn) == 1) {
                if (utils::file_test("-f", out_dsn))
                    deleted <- deleteDataset(out_dsn)
            }
        }
        if (!deleted) {
            stop("cannot overwrite 'out_layer'", call. = FALSE)
        }
    }

    if (.ogr_layer_exists(out_dsn, out_layer)) {
        if (!is.null(lco)) {
            warning("'lco' ignored since the layer already exists",
                    call. = FALSE)
        }
    }

    if (!.ogr_ds_exists(out_dsn, with_update=TRUE)) {
        if (is.null(out_fmt))
            out_fmt <- .getOGRformat(out_dsn)
        if (is.null(out_fmt)) {
            message("format driver cannot be determined for: ", out_dsn)
            stop("specify 'out_fmt' to create a new dataset", call. = FALSE)
        }
        if (!.create_ogr(out_fmt, out_dsn, 0, 0, 0, "Unknown",
                         out_layer, "POLYGON", srs, fld_name, "OFTInteger",
                         dsco, lco)) {
            stop("failed to create 'out_dsn'", call. = FALSE)
        }
    }

    if (!.ogr_layer_exists(out_dsn, out_layer)) {
        res <- .ogr_layer_create(out_dsn, out_layer, NULL, "POLYGON", srs, lco)
        if (!res)
            stop("failed to create 'out_layer'", call. = FALSE)
        if (fld_name != "") {
            res <- .ogr_field_create(out_dsn, out_layer, fld_name, "OFTInteger")
            if (!res)
                stop("failed to create the output field", call. = FALSE)
        }
    }

    return(invisible(.polygonize(raster_file, src_band, out_dsn, out_layer,
                                 fld_name, mask_file, nomask, connectedness,
                                 quiet)))
}


#' Burn vector geometries into a raster
#'
#' @description
#' `rasterize()` burns vector geometries (points, lines, or polygons) into
#' the band(s) of a raster dataset. Vectors are read from any GDAL
#' OGR-supported vector format.
#' This function is a wrapper for the \command{gdal_rasterize} command-line
#' utility (\url{https://gdal.org/programs/gdal_rasterize.html}).
#'
#' @param src_dsn Data source name for the input vector layer (filename or
#' connection string).
#' @param dstfile Filename of the output raster. Must support update mode
#' access. This file will be created (or overwritten if it already exists -
#' see Note).
#' @param band Numeric vector. The band(s) to burn values into (for existing
#' `dstfile`). The default is to burn into band 1. Not used when creating a
#' new raster.
#' @param layer Character vector of layer names(s) from `src_dsn`  that will be
#' used for input features. At least one layer name or a `sql` option must be
#' specified.
#' @param where An optional SQL WHERE style query string to select features to
#' burn in from the input `layer`(s).
#' @param sql An SQL statement to be evaluated against `src_dsn` to produce a
#' virtual layer of features to be burned in (alternative to `layer`).
#' @param burn_value A fixed numeric value to burn into a band for all
#' features. A numeric vector can be supplied, one burn value per band being
#' written to.
#' @param burn_attr Character string. Name of an attribute field on the
#' features to be used for a burn-in value. The value will be burned into all
#' output bands.
#' @param invert Logical scalar. `TRUE` to invert rasterization. Burn the fixed
#' burn value, or the burn value associated with the first feature, into all
#' parts of the raster not inside the provided polygon.
#' @param te Numeric vector of length four. Sets the output raster extent. The
#' values must be expressed in georeferenced units. If not specified, the
#' extent of the output raster will be the extent of the vector layer.
#' @param tr Numeric vector of length two. Sets the target pixel resolution.
#' The values must be expressed in georeferenced units. Both must be positive.
#' @param tap Logical scalar. (target aligned pixels) Align the coordinates of
#' the extent of the output raster to the values of `tr`, such that the
#' aligned extent includes the minimum extent. Alignment means that
#' xmin / resx, ymin / resy, xmax / resx and ymax / resy are integer values.
#' @param ts Numeric vector of length two. Sets the output raster size in
#' pixels (xsize, ysize). Note that `ts` cannot be used with `tr`.
#' @param dtName Character name of output raster data type, e.g., `Byte`,
#' `Int16`, `UInt16`, `Int32`, `UInt32`, `Float32`, `Float64`.
#' Defaults to `Float64`.
#' @param dstnodata	Numeric scalar. Assign a nodata value to output bands.
#' @param init Numeric vector. Pre-initialize the output raster band(s) with
#' these value(s). However, it is not marked as the nodata value in the output
#' file. If only one value is given, the same value is used in all the bands.
#' @param fmt Output raster format short name (e.g., `"GTiff"`). Will attempt
#' to guess from the output filename if `fmt` is not specified.
#' @param co Optional list of format-specific creation options for the output
#' raster in a vector of "NAME=VALUE" pairs
#' (e.g., \code{options = c("TILED=YES","COMPRESS=LZW")} to set LZW compression
#' during creation of a tiled GTiff file).
#' @param add_options An optional character vector of additional command-line
#' options to `gdal_rasterize` (see the `gdal_rasterize` documentation at the
#' URL above for all available options).
#' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
#' displayed. Defaults to `FALSE`.
#' @returns Logical indicating success (invisible `TRUE`).
#' An error is raised if the operation fails.
#'
#' @note
#' The function creates a new target raster when any of the `fmt`, `dstnodata`,
#' `init`, `co`, `te`, `tr`, `tap`, `ts`, or `dtName` arguments are used. The
#' resolution or size must be specified using the `tr` or `ts` argument for all
#' new rasters. The target raster will be overwritten if it already exists and
#' any of these creation-related options are used.
#'
#' @seealso
#' [polygonize()]
#'
#' @examples
#' # MTBS fire perimeters for Yellowstone National Park 1984-2022
#' dsn <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
#' sql <- "SELECT * FROM mtbs_perims ORDER BY mtbs_perims.ig_year"
#' out_file <- file.path(tempdir(), "ynp_fires_1984_2022.tif")
#'
#' rasterize(src_dsn = dsn,
#'           dstfile = out_file,
#'           sql = sql,
#'           burn_attr = "ig_year",
#'           tr = c(90,90),
#'           tap = TRUE,
#'           dtName = "Int16",
#'           dstnodata = -9999,
#'           init = -9999,
#'           co = c("TILED=YES","COMPRESS=LZW"))
#'
#' ds <- new(GDALRaster, out_file)
#' pal <- scales::viridis_pal(end = 0.8, direction = -1)(6)
#' ramp <- scales::colour_ramp(pal)
#' plot_raster(ds, legend = TRUE, col_map_fn = ramp, na_col = "#d9d9d9",
#'             main="YNP Fires 1984-2022 - Most Recent Burn Year")
#'
#' ds$close()
#' deleteDataset(out_file)
#' @export
rasterize <- function(src_dsn,
                      dstfile,
                      band = NULL,
                      layer = NULL,
                      where = NULL,
                      sql = NULL,
                      burn_value = NULL,
                      burn_attr = NULL,
                      invert = NULL,
                      te = NULL,
                      tr = NULL,
                      tap = NULL,
                      ts = NULL,
                      dtName = NULL,
                      dstnodata = NULL,
                      init = NULL,
                      fmt = NULL,
                      co = NULL,
                      add_options = NULL,
                      quiet = FALSE) {

    src_dsn <- .check_gdal_filename(src_dsn)
    dstfile <- .check_gdal_filename(dstfile)

    argv <- character(0)

    if (!is.null(band)) {
        if (!is.numeric(band))
            stop("'band' must be numeric", call. = FALSE)
        for (b in band) {
            argv <- c(argv, "-b", b)
        }
    }

    if (!is.null(layer)) {
        if (!is.character(layer) || length(layer) > 1)
            stop("'layer' must be a length-1 character vector", call. = FALSE)
        for (l in layer) {
            argv <- c(argv, "-l", l)
        }
    }

    if (!is.null(where)) {
        if (is.character(where) && length(where) == 1)
            argv <- c(argv, "-where", where)
        else
            stop("'where' must be a length-1 character vector", call. = FALSE)
    }

    if (!is.null(sql)) {
        if (is.character(sql) && length(sql) == 1)
            argv <- c(argv, "-sql", sql)
        else
            stop("'sql' must be a length-1 character vector", call. = FALSE)
    }

    if (!is.null(burn_value)) {
        if (!is.numeric(burn_value))
            stop("'burn_value' must be numeric", call. = FALSE)
        for (value in burn_value) {
            argv <- c(argv, "-burn", value)
        }
    }

    if (!is.null(burn_attr)) {
        if (is.character(burn_attr) && length(burn_attr) == 1)
            argv <- c(argv, "-a", burn_attr)
        else
            stop("'burn_attr' must be a length-1 character vector",
                 call. = FALSE)
    }

    if (!is.null(invert)) {
        if (is.logical(invert) && length(invert) == 1) {
            if (invert)
                argv <- c(argv, "-i")
        } else {
            stop("'invert' must be a logical scalar", call. = FALSE)
        }
    }

    if (!is.null(te)) {
        if (is.numeric(te) && length(te) == 4)
            argv <- c(argv, "-te", te)
        else
            stop("'te' must be a numeric vector of length 4", call. = FALSE)
    }

    if (!is.null(tr)) {
        if (is.numeric(tr) && length(tr) == 2)
            argv <- c(argv, "-tr", tr)
        else
            stop("'tr' must be a numeric vector of length 2", call. = FALSE)
    }

    if (!is.null(tap)) {
        if (is.logical(tap) && length(tap) == 1) {
            if (tap)
                argv <- c(argv, "-tap")
        } else {
            stop("'tap' must be a logical scalar", call. = FALSE)
        }
    }

    if (!is.null(ts)) {
        if (!is.null(tr))
            stop("'ts' cannot be used with 'tr'", call. = FALSE)
        if (is.numeric(ts) && length(ts) == 2)
            argv <- c(argv, "-ts", ts)
        else
            stop("'ts' must be a numeric vector of length 2", call. = FALSE)
    }

    if (!is.null(dtName)) {
        if (is.character(dtName) && length(dtName) == 1)
            argv <- c(argv, "-ot", dtName)
        else
            stop("'dtName' must be a length-1 character vector",
                 call. = FALSE)
    }

    if (!is.null(dstnodata)) {
        if (is.numeric(dstnodata) && length(dstnodata) == 1)
            argv <- c(argv, "-a_nodata", dstnodata)
        else
            stop("'dstnodata' must be a numeric scalar", call. = FALSE)
    }

    if (!is.null(init)) {
        if (!is.numeric(init))
            stop("'init' must be numeric", call. = FALSE)
        for (value in init) {
            argv <- c(argv, "-init", value)
        }
    }

    if (!is.null(fmt)) {
        if (is.character(fmt) && length(fmt) == 1)
            argv <- c(argv, "-of", fmt)
        else
            stop("'fmt' must be a length-1 character vector",
                 call. = FALSE)
    }

    if (!is.null(co)) {
        if (!is.character(co))
            stop("'co' must be a character vector", call. = FALSE)
        for (name_value in co) {
            argv <- c(argv, "-co", name_value)
        }
    }

    if (!is.null(add_options)) {
        if (!is.character(add_options))
            stop("'add_options' must be a character vector", call. = FALSE)
        else
            argv <- c(argv, add_options)
    }

    return(invisible(.rasterize(src_dsn, dstfile, argv, quiet)))
}
