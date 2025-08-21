# R interface to GDAL utils via the wrapper functions in src/gdal_exp.cpp


#' Convert raster data between different formats
#'
#' `translate()` is a wrapper of the \command{gdal_translate} command-line
#' utility (see \url{https://gdal.org/en/stable/programs/gdal_translate.html}).
#' The function can be used to convert raster data between different
#' formats, potentially performing some operations like subsetting,
#' resampling, and rescaling pixels in the process. Refer to the GDAL
#' documentation at the URL above for a list of command-line arguments that
#' can be passed in `cl_arg`.
#'
#' @param src_filename Either a character string giving the filename of the
#' source raster, or an object of class `GDALRaster` for the source.
#' @param dst_filename Character string. Filename of the output raster.
#' @param cl_arg Optional character vector of command-line arguments for
#' \code{gdal_translate} (see URL above).
#' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
#' displayed. Defaults to `FALSE`.
#' @returns Logical indicating success (invisible \code{TRUE}).
#' An error is raised if the operation fails.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [rasterFromRaster()], [warp()]
#'
#' [ogr2ogr()] for vector data
#'
#' @examples
#' # convert the elevation raster to Erdas Imagine format and resample to 90m
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' img_file <- file.path(tempdir(), "storml_elev_90m.img")
#'
#' # command-line arguments for gdal_translate
#' args <- c("-tr", "90", "90", "-r", "average")
#' args <- c(args, "-of", "HFA", "-co", "COMPRESSED=YES")
#'
#' translate(elev_file, img_file, args)
#'
#' ds <- new(GDALRaster, img_file)
#' ds$info()
#'
#' ds$close()
#' \dontshow{deleteDataset(img_file)}
#' @export
translate <- function(src_filename, dst_filename, cl_arg = NULL,
                      quiet = FALSE) {

    src_ds <- NULL
    close_src_ds <- FALSE
    if (is(src_filename, "Rcpp_GDALRaster")) {
        src_ds <- src_filename
        if (!src_ds$isOpen()) {
            stop("source dataset is not open", call. = FALSE)
        }
    } else if (is.character(src_filename) && length(src_filename) == 1) {
        src_ds <- new(GDALRaster, src_filename)
        close_src_ds <- TRUE
    } else {
        stop("'src_filename' must be a character string or 'GDALRaster' object",
             call. = FALSE)
    }

    if (!(is.character(dst_filename) && length(dst_filename) == 1))
        stop("'dst_filename' must be a character string", call. = FALSE)

    if (!is.null(cl_arg) && !is.character(cl_arg))
        stop("'cl_arg' must be a character vector", call. = FALSE)

    if (!is.null(quiet)) {
        if (!(is.logical(quiet) && length(quiet) == 1))
            stop("'quiet' must be a logical scalar", call. = FALSE)
    }

    ret <- .translate(src_ds, dst_filename, cl_arg, quiet)

    if (close_src_ds)
        src_ds$close()

    if (!ret) {
        stop("translate raster failed", call. = FALSE)
    } else {
        return(invisible(TRUE))
    }
}

#' Raster reprojection and mosaicing
#'
#' `warp()` is a wrapper of the \command{gdalwarp} command-line utility for
#' raster reprojection and warping
#' (see \url{https://gdal.org/en/stable/programs/gdalwarp.html}).
#' The function can reproject to any supported spatial reference system (SRS).
#' It can also be used to crop, mosaic, resample, and optionally write output
#' to a different raster format. See Details for a list of commonly used
#' processing options that can be passed as arguments to `warp()`.
#'
#' @details
#' Several processing options can be performed in one call to `warp()` by
#' passing the necessary command-line arguments. The following list describes
#' several commonly used arguments. Note that `gdalwarp` supports a large
#' number of arguments that enable a variety of different processing options.
#' Users are encouraged to review the original source documentation provided
#' by the GDAL project at the URL above for the full list.
#'
#'   * `-te <xmin> <ymin> <xmax> <ymax>`\cr
#'   Georeferenced extents of output file to be created (in target SRS by
#'   default).
#'   * `-te_srs <srs_def>`\cr
#'   SRS in which to interpret the coordinates given with `-te`
#'   (if different than `t_srs`).
#'   * `-tr <xres> <yres>`\cr
#'   Output pixel resolution (in target georeferenced units).
#'   * `-tap`\cr
#'   (target aligned pixels) align the coordinates of the extent of the output
#'   file to the values of the `-tr`, such that the aligned extent includes
#'   the minimum extent. Alignment means that xmin / resx, ymin / resy,
#'   xmax / resx and ymax / resy are integer values.
#'   * `-ovr <level>|AUTO|AUTO-<n>|NONE`\cr
#'   Specify which overview level of source files must be used. The default
#'   choice, `AUTO`, will select the overview level whose resolution is the
#'   closest to the target resolution. Specify an integer value (0-based,
#'   i.e., 0=1st overview level) to select a particular level. Specify
#'   `AUTO-n` where `n` is an integer greater or equal to `1`, to select an
#'   overview level below the `AUTO` one. Or specify `NONE` to force the base
#'   resolution to be used (can be useful if overviews have been generated
#'   with a low quality resampling method, and the warping is done using a
#'   higher quality resampling method).
#'   * `-wo <NAME>=<VALUE>`\cr
#'   Set a warp option as described in the GDAL documentation for
#'   [`GDALWarpOptions`](https://gdal.org/en/stable/api/gdalwarp_cpp.html#_CPPv415GDALWarpOptions)
#'   Multiple `-wo` may be given. See also `-multi` below.
#'   * `-ot <type>`\cr
#'   Force the output raster bands to have a specific data type supported by
#'   the format, which may be one of the following: `Byte`, `Int8`, `UInt16`,
#'   `Int16`, `UInt32`, `Int32`, `UInt64`, `Int64`, `Float32`, `Float64`,
#'   `CInt16`, `CInt32`, `CFloat32` or `CFloat64`.
#'   * `-r <resampling_method>`\cr
#'   Resampling method to use. Available methods are: `near` (nearest
#'   neighbour, the default), `bilinear`, `cubic`, `cubicspline`, `lanczos`,
#'   `average`, `rms` (root mean square, GDAL >= 3.3), `mode`, `max`, `min`,
#'   `med`, `q1` (first quartile), `q3` (third quartile), `sum` (GDAL >= 3.1).
#'   * `-srcnodata "<value>[ <value>]..."`\cr
#'   Set nodata masking values for input bands (different values can be
#'   supplied for each band). If more than one value is supplied all values
#'   should be quoted to keep them together as a single operating system
#'   argument. Masked values will not be used in interpolation. Use a value of
#'   `None` to ignore intrinsic nodata settings on the source dataset.
#'   If `-srcnodata` is not explicitly set, but the source dataset has nodata
#'   values, they will be taken into account by default.
#'   * `-dstnodata "<value>[ <value>]..."`\cr
#'   Set nodata values for output bands (different values can be supplied for
#'   each band). If more than one value is supplied all values should be
#'   quoted to keep them together as a single operating system argument. New
#'   files will be initialized to this value and if possible the nodata value
#'   will be recorded in the output file. Use a value of `"None"` to ensure
#'   that nodata is not defined. If this argument is not used then nodata
#'   values will be copied from the source dataset.
#'   * `-srcband <n>`\cr
#'   (GDAL >= 3.7) Specify an input band number to warp (between 1 and the
#'   number of bands of the source dataset). This option is used to warp a
#'   subset of the input bands. All input bands are used when it is not
#'   specified. This option may be repeated multiple times to select several
#'   input bands. The order in which bands are specified will be the order in
#'   which they appear in the output dataset (unless `-dstband` is specified).
#'   The alpha band should not be specified in the list, as it will be
#'   automatically retrieved (unless `-nosrcalpha` is specified).
#'   * `-dstband <n>`\cr
#'   (GDAL >= 3.7) Specify the output band number in which to warp. In
#'   practice, this option is only useful when updating an existing dataset,
#'   e.g to warp one band at at time. If `-srcband` is specified, there must be
#'   as many occurrences of `-dstband` as there are of `-srcband`.\cr
#'   If `-dstband` is not specified, then:\cr
#'   `c("-dstband", "1", "-dstband", "2", ... "-dstband", "N")`\cr
#'   is assumed where N is the number of input bands (implicitly, or specified
#'   explicitly with `-srcband`). The alpha band should not be specified in the
#'   list, as it will be automatically retrieved (unless `-nosrcalpha` is
#'   specified).
#'   * `-wm <memory_in_mb>`\cr
#'   Set the amount of memory that the warp API is allowed to use for caching.
#'   The value is interpreted as being in megabytes if the value is <10000.
#'   For values >=10000, this is interpreted as bytes. The warper will
#'   total up the memory required to hold the input and output image arrays
#'   and any auxiliary masking arrays and if they are larger than the
#'   "warp memory" allowed it will subdivide the chunk into smaller chunks and
#'   try again. If the `-wm` value is very small there is some extra overhead
#'   in doing many small chunks so setting it larger is better but it is a
#'   matter of diminishing returns.
#'   * `-multi`\cr
#'   Use multithreaded warping implementation. Two threads will be used to
#'   process chunks of image and perform input/output operation
#'   simultaneously. Note that computation is not multithreaded itself. To do
#'   that, you can use the `-wo NUM_THREADS=val/ALL_CPUS` option, which can be
#'   combined with `-multi`.
#'   * `-of <format>`
#'   Set the output raster format. Will be guessed from the extension if not
#'   specified. Use the short format name (e.g., `"GTiff"`).
#'   * `-co <NAME>=<VALUE>`\cr
#'   Set one or more format specific creation options for the output dataset.
#'   For example, the GeoTIFF driver supports creation options to control
#'   compression, and whether the file should be tiled.
#'   [getCreationOptions()] can be used to look up available creation options,
#'   but the GDAL [Raster drivers](https://gdal.org/en/stable/drivers/raster/index.html)
#'   documentation is the definitive reference for format specific options.
#'   Multiple `-co` may be given, e.g.,
#'   \preformatted{ c("-co", "COMPRESS=LZW", "-co", "BIGTIFF=YES") }
#'   * `-overwrite`\cr
#'   Overwrite the target dataset if it already exists. Overwriting means
#'   deleting and recreating the file from scratch. Note that if this option
#'   is not specified and the output file already exists, it will be updated
#'   in place.
#'
#' The documentation for [`gdalwarp`](https://gdal.org/en/stable/programs/gdalwarp.html)
#' describes additional command-line options related to spatial reference
#' systems, alpha bands, masking with polygon cutlines including blending,
#' and more.
#'
#' Mosaicing into an existing output file is supported if the output file
#' already exists. The spatial extent of the existing file will not be
#' modified to accommodate new data, so you may have to remove it in that
#' case, or use the `-overwrite` option.
#'
#' Command-line options are passed to `warp()` as a character vector. The
#' elements of the vector are the individual options followed by their
#' individual values, e.g.,
#' \preformatted{
#' cl_arg = c("-tr", "30", "30", "-r", "bilinear"))
#' }
#' to set the target pixel resolution to 30 x 30 in target georeferenced
#' units and use bilinear resampling.
#'
#' @param src_files Either a character vector of source filenames(s) to be
#' reprojected, or a `GDALRaster` object or list of `GDALRaster` objects for
#' the source data.
#' @param dst_filename Either a character string giving the filename of the
#' output dataset, or an object of class `GDALRaster` for the output.
#' @param t_srs Character string. Target spatial reference system. Usually an
#' EPSG code ("EPSG:#####") or a well known text (WKT) SRS definition.
#' Can be set to empty string `""` and the spatial reference of `src_files[1]`
#' will be used unless the destination raster already exists (see Note).
#' @param cl_arg Optional character vector of command-line arguments to
#' \code{gdalwarp} in addition to `-t_srs` (see Details).
#' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
#' displayed. Defaults to `FALSE`.
#' @returns Logical indicating success (invisible \code{TRUE}).
#' An error is raised if the operation fails.
#'
#' @note
#' `warp()` can be used to reproject and also perform other processing such
#' as crop, resample, and mosaic.
#' This processing is generally done with a single function call by passing
#' arguments for the output ("target") pixel resolution, extent, resampling
#' method, nodata value, format, and so forth.
#'
#' If `warp()` is called with `t_srs = ""` and the output raster does not
#' already exist, the target spatial reference will be set to that of
#' `src_files[1]`. In that case, the processing options given in `cl_arg` will
#' be performed without reprojecting (if there is one source raster or multiple
#' sources that all use the same spatial reference system, otherwise would
#' reproject inputs to the SRS of `src_files[1]` where they are different).
#' If `t_srs = ""` and the destination raster already exists, the output SRS
#' will be the projection of the destination dataset.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [srs_to_wkt()], [translate()]
#'
#' @examples
#' # reproject the elevation raster to NAD83 / CONUS Albers (EPSG:5070)
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#'
#' # command-line arguments for gdalwarp
#' # resample to 90-m resolution and keep pixels aligned:
#' args <- c("-tr", "90", "90", "-r", "cubic", "-tap")
#' # write to Erdas Imagine format (HFA) with compression:
#' args <- c(args, "-of", "HFA", "-co", "COMPRESSED=YES")
#'
#' alb83_file <- file.path(tempdir(), "storml_elev_alb83.img")
#' warp(elev_file, alb83_file, t_srs = "EPSG:5070", cl_arg = args)
#'
#' ds <- new(GDALRaster, alb83_file)
#' ds$getDriverLongName()
#' ds$getProjectionRef()
#' ds$res()
#' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#' ds$close()
#' \dontshow{deleteDataset(alb83_file)}
#' @export
warp <- function(src_files,
                 dst_filename,
                 t_srs,
                 cl_arg = NULL,
                 quiet = FALSE) {

    if (!(is.character(t_srs) && length(t_srs) == 1))
        stop("'t_srs' must be a character string", call. = FALSE)

    if (!is.null(cl_arg) && !is.character(cl_arg))
        stop("'cl_arg' must be a character vector", call. = FALSE)

    if (!is.null(quiet)) {
        if (!(is.logical(quiet) && length(quiet) == 1))
            stop("'quiet' must be a logical scalar", call. = FALSE)
    }

    dst_ds <- NULL
    if (is(dst_filename, "Rcpp_GDALRaster")) {
        dst_ds <- dst_filename
        if (!dst_ds$isOpen()) {
            stop("destination dataset is not open", call. = FALSE)
        }
        if (t_srs == "") {
            t_srs <- dst_ds$getProjection()
        }
    } else if (!(is.character(dst_filename) && length(dst_filename) == 1)) {
        stop("'dst_filename' must be a character string or GDALRaster object",
             call. = FALSE)
    } else {
        push_error_handler("quiet")
        ds <- try(new(GDALRaster, dst_filename), silent = TRUE)
        pop_error_handler()
        if (is(ds, "Rcpp_GDALRaster") && t_srs == "") {
            t_srs <- ds$getProjection()
            ds$close()
        }
    }

    src_datasets <- list()
    have_error <- FALSE
    close_datasets <- FALSE

    if (is(src_files, "Rcpp_GDALRaster"))
        src_files <- list(src_files)

    if (is.list(src_files)) {
        for (i in seq_along(src_files)) {
            if (!is(src_files[[i]], "Rcpp_GDALRaster")) {
                stop("'src_files' as list must contain 'GDALRaster' objects",
                     call. = FALSE)
            } else if (!src_files[[i]]$isOpen()) {
                stop("source dataset ", i, " is not open", call. = FALSE)
            }
        }
        src_datasets <- src_files
    } else if (is.character(src_files)) {
        for (i in seq_along(src_files)) {
            src_datasets[[i]] <- try(new(GDALRaster, src_files[i]))
            if (!is(src_datasets[[i]], "Rcpp_GDALRaster")) {
                have_error <- TRUE
            }
        }
        close_datasets <- TRUE
    } else {
        stop("'src_files' must be a character vector or list",
             call. = FALSE)
    }

    if (have_error) {
        for (i in seq_along(src_datasets)) {
            if (is(src_datasets[[i]], "Rcpp_GDALRaster")) {
                src_datasets[[i]]$close()
            }
        }
        stop("could not open one or more source datasets", call. = FALSE)
    }

    if (!is.null(dst_ds)) {
        ret <- .warp(src_datasets, "", list(dst_ds), t_srs, cl_arg, quiet)
    } else {
        ret <- .warp(src_datasets, dst_filename, list(), t_srs, cl_arg, quiet)
    }

    if (close_datasets) {
        for (i in seq_along(src_datasets)) {
            src_datasets[[i]]$close()
        }
    }

    if (!ret) {
        stop("warp raster failed", call. = FALSE)
    } else {
        return(invisible(TRUE))
    }
}

#' Create a virtual warped dataset automatically
#'
#' `autoCreateWarpedVRT()` creates a warped virtual dataset representing the
#' input raster warped into a target coordinate system. The output virtual
#' dataset will be "north-up" in the target coordinate system. GDAL
#' automatically determines the bounds and resolution of the output virtual
#' raster which should be large enough to include all the input raster.
#' Wrapper of `GDALAutoCreateWarpedVRT()` in the GDAL Warper API.
#'
#' @param src_ds An object of class `GDALRaster` for the source dataset.
#' @param dst_wkt WKT string specifying the coordinate system to convert to.
#' If empty string (`""`) no change of coordinate system will take place.
#' @param resample_alg Character string specifying the sampling method to use.
#' One of NearestNeighbour, Bilinear, Cubic, CubicSpline, Lanczos, Average,
#' RMS or Mode.
#' @param src_wkt WKT string specifying the coordinate system of the source
#' raster. If empty string it will be read from the source raster (the
#' default).
#' @param max_err Numeric scalar specifying the maximum error measured in
#' input pixels that is allowed in approximating the transformation (`0.0` for
#' exact calculations, the default).
#' @param alpha_band Logical scalar, `TRUE` to create an alpha band if the
#' source dataset has none. Defaults to `FALSE`.
#'
#' @returns An object of class `GDALRaster` for the new virtual dataset. An
#' error is raised if the operation fails.
#'
#' @note
#' The returned dataset will have no associated filename for itself. If you
#' want to write the virtual dataset to a VRT file, use the
#' \code{$setFilename()} method on the returned `GDALRaster` object to assign a
#' filename before it is closed.
#'
#' @examples
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file)
#'
#' ds2 <- autoCreateWarpedVRT(ds, epsg_to_wkt(5070), "Bilinear")
#' ds2$info()
#'
#' ## set filename before close if a VRT file is needed for the virtual dataset
#' # ds2$setFilename("/path/to/file.vrt")
#'
#' ds2$close()
#' ds$close()
#' @export
autoCreateWarpedVRT <- function(src_ds, dst_wkt, resample_alg, src_wkt = "",
                                max_err = 0.0, alpha_band = FALSE) {

    if (is(src_ds, "Rcpp_GDALRaster")) {
        if (!src_ds$isOpen()) {
            stop("source dataset is not open", call. = FALSE)
        }
    } else {
        stop("'src_ds' must be a 'GDALRaster' object",
             call. = FALSE)
    }

    if (is.null(dst_wkt))
        stop("'dst_wkt' cannot be NULL", call. = FALSE)
    if (!(is.character(dst_wkt) && length(dst_wkt) == 1))
        stop("'dst_wkt' must be a character string", call. = FALSE)

    if (is.null(resample_alg))
        stop("'resample_alg' cannot be NULL", call. = FALSE)
    if (!(is.character(resample_alg) && length(resample_alg) == 1))
        stop("'resample_alg' must be a character string", call. = FALSE)

    if (is.null(src_wkt))
        stop("'src_wkt' cannot be NULL", call. = FALSE)
    if (!(is.character(src_wkt) && length(src_wkt) == 1))
        stop("'src_wkt' must be a character string", call. = FALSE)

    if (is.null(max_err))
        stop("'max_err' cannot be NULL", call. = FALSE)
    if (!(is.numeric(max_err) && length(max_err) == 1))
        stop("'max_err' must be a numeric value", call. = FALSE)

    if (is.null(alpha_band))
        stop("'alpha_band' cannot be NULL", call. = FALSE)
    if (!(is.logical(alpha_band) && length(alpha_band) == 1))
        stop("'alpha_band' must be a logical value", call. = FALSE)

    # signature for autoCreateWarpedVRT() object factory
    ds <- new(GDALRaster, src_ds, dst_wkt, resample_alg, src_wkt, max_err,
              alpha_band, TRUE, TRUE)

    return(ds)
}
