# Display raster data using base graphics

#' @noRd
.normalize <- function(x, minmax=NULL) {
    # Normalize to a range of [0,1].
    # Normalize to the full range of the input data by default.
    # Optionally normalize to a user-defined range in terms of the input.

    if (is.null(minmax)) {
        xn <- (x - min(x,na.rm=TRUE)) / (max(x,na.rm=TRUE) - min(x,na.rm=TRUE))
    } else {
        xn <- (x - minmax[1]) / (minmax[2] - minmax[1])
        xn[xn < 0] <- 0
        xn[xn > 1] <- 1
    }

    return(xn)
}

#' @noRd
.as_raster <- function(a,
                       col_tbl=NULL,
                       maxColorValue=1,
                       normalize=TRUE,
                       minmax_def=NULL,
                       minmax_pct_cut=NULL,
                       col_map_fn=NULL,
                       na_col=rgb(0,0,0,0),
                       ...) {

    # Create an object of class "raster", a matrix of color values representing
    # a bitmap image for input to graphics::rasterImage(). Input is an array of
    # pixel values with dimensions xsize, ysize, nbands.

    nbands <- dim(a)[3]

    if (!(nbands %in% c(1, 3)))
        stop("number of bands must be 1 or 3", call.=FALSE)

    r <- array()

    if (!is.null(col_tbl)) {
        # map to a color table
        if (nbands != 1)
            stop("color table can only be used with single-band data",
                 call.=FALSE)

        ct <- as.data.frame(col_tbl)
        if (ncol(ct) < 4 || ncol(ct) > 5)
            stop("color table must have four or five columns", call.=FALSE)

        if (ncol(ct) == 4) # add alpha channel
            ct[, 5] <- rep(maxColorValue, nrow(ct))
        ct[, 6] <- rgb(ct[, 2], ct[, 3], ct[, 4], ct[, 5],
                       maxColorValue=maxColorValue)
        names(ct) <- c("value", "r", "g", "b", "a", "rgb")
        r <- ct$rgb[match(as.vector(a), ct$value)]

        if (anyNA(r))
            r[is.na(r)] <- na_col
        dim(r) <- dim(a)[2:1]
        class(r) <- "raster"

    } else {
        # gray/rgb color scaling
        if (normalize) {
            if (!is.null(minmax_def)) {
                for (b in 1:nbands) {
                    a[,,b] <- .normalize(a[,,b], minmax_def[c(b, b+nbands)])
                }
            } else if (!is.null(minmax_pct_cut)) {
                for (b in 1:nbands) {
                    q <- stats::quantile(a[,,b],
                                         probs=c(minmax_pct_cut[1] / 100,
                                                 minmax_pct_cut[2] / 100),
                                         na.rm = TRUE,
                                         names = FALSE)
                    a[,,b] <- .normalize(a[,,b], q)
                }
            } else {
                for (b in 1:nbands) {
                    a[,,b] <- .normalize(a[,,b])
                }
            }
        }

        if (is.null(col_map_fn))
            col_map_fn <- ifelse(nbands==1, grDevices::gray, grDevices::rgb)

        has_na <- FALSE
        nas <- array()
        if (anyNA(a)) {
            has_na <- TRUE
            nas <- is.na(a)
            a[nas] <- 0
            if (dim(nas)[3] == 3)
                nas <- nas[,,1] | nas[,,2] | nas[,,3]
            dim(nas) <- dim(nas)[1:2]
            nas <- t(nas)
        }

        if (nbands == 1) {
            # grayscale
            dim(a) <- dim(a)[1:2]
            r <- col_map_fn(a)
            dim(r) <- dim(a)[2:1]
            class(r) <- "raster"
        } else {
            # rgb
            r <- col_map_fn(a[,,1], a[,,2], a[,,3])
            dim(r) <- dim(a)[2:1]
            class(r) <- "raster"
        }

        if (has_na)
            r[nas] <- na_col
    }

    return(r)
}

#' Display raster data
#'
#' `plot_raster()` displays raster data using base `graphics`.
#'
#' @details
#' By default, contrast enhancement by stretch to min/max is applied when
#' the input data are single-band grayscale with any raster data type, or
#' three-band RGB with raster data type larger than Byte. The minimum/maximum
#' of the input data are used by default (i.e., no outlier removal). No stretch
#' is applied by default when the input is an RGB byte raster. These defaults
#' can be overridden by specifying either the `minmax_def` argument
#' (user-defined min/max per band), or the `minmax_pct_cut` argument (ignore
#' outlier pixels based on a percentile range per band). These settings (and
#' the `normalize` argument) are ignored if a color table is used.
#'
#' @param data Either a `GDALRaster` object from which data will be read, or
#' a numeric vector of pixel values arranged in left to right, top to
#' bottom order, or a list of band vectors. If input is vector or list,
#' the information in attribute `gis` will be used if present (see [read_ds()]),
#' potentially ignoring values below for `xsize`, `ysize`, `nbands`.
#' @param xsize The number of pixels along the x dimension in `data`. If `data`
#' is a `GDALRaster` object, specifies the size at which the raster will be
#' read (used for argument `out_xsize` in `GDALRaster$read()`). By default,
#' the entire raster will be read at full resolution.
#' @param ysize The number of pixels along the y dimension in `data`. If `data`
#' is a `GDALRaster` object, specifies the size at which the raster will be
#' read (used for argument `out_ysize` in `GDALRaster$read()`). By default,
#' the entire raster will be read at full resolution.
#' @param nbands The number of bands in `data`. Must be either 1 (grayscale) or
#' 3 (RGB). For RGB, `data` are interleaved by band. If `nbands` is `NULL` (the
#' default), then `nbands = 3` is assumed if the input data contain 3 bands,
#' otherwise band 1 is used.
#' @param max_pixels The maximum number of pixels that the function will
#' attempt to display (per band). An error is raised if `(xsize * ysize)`
#' exceeds this value. Setting to `NULL` turns off this check.
#' @param col_tbl A color table as a matrix or data frame with four or five
#' columns. Column 1 contains the numeric pixel values. Columns 2:4 contain
#' the intensities of the red, green and blue primaries (`0:1` by default,
#' or use integer `0:255` by setting `maxColorValue = 255`).
#' An optional column 5 may contain alpha transparency values, `0` for fully
#' transparent to `1` (or `maxColorValue`) for opaque (the default if column 5
#' is missing). If `data` is a `GDALRaster` object, a built-in color table will
#' be used automatically if one exists in the dataset.
#' @param maxColorValue A number giving the maximum of the color values range
#' in `col_tbl` (see above). The default is `1`.
#' @param normalize Logical. `TRUE` to rescale pixel values so that their
#' range is `[0,1]`, normalized to the full range of the pixel data by default
#' (`min(data)`, `max(data)`, per band). Ignored if `col_tbl` is used.
#' Set `normalize` to `FALSE` if a color map function is used that
#' operates on raw pixel values (see `col_map_fn` below).
#' @param minmax_def Normalize to user-defined min/max values (in terms of
#' the pixel data, per band). For single-band grayscale, a numeric vector of
#' length two containing min, max. For 3-band RGB, a numeric vector of length
#' six containing b1_min, b2_min, b3_min, b1_max, b2_max, b3_max.
#' @param minmax_pct_cut Normalize to a truncated range of the pixel data using
#' percentile cutoffs (removes outliers). A numeric vector of length two giving
#' the percentiles to use (e.g., `c(2, 98)`). Applied per band. Ignored if
#' `minmax_def` is used.
#' @param col_map_fn An optional color map function (default is
#' `grDevices::gray` for single-band data or `grDevices::rgb` for 3-band).
#' Ignored if `col_tbl` is used. Set `normalize` to `FALSE` if using a color
#' map function that operates on raw pixel values.
#' @param pixel_fn An optional function that will be applied to the input
#' pixel data. Must accept vector input and return a numeric vector of the same
#' length as its input.
#' @param xlim Numeric vector of length two giving the x coordinate range.
#' If `data` is a `GDALRaster` object, the default is the raster xmin, xmax in
#' georeferenced coordinates, otherwise the default uses pixel/line
#' coordinates (`c(0, xsize)`).
#' @param ylim Numeric vector of length two giving the y coordinate range.
#' If `data` is a `GDALRaster` object, the default is the raster ymin, ymax in
#' georeferenced coordinates, otherwise the default uses pixel/line
#' coordinates (`c(ysize, 0)`).
#' @param xlab Title for the x axis (see `?title`).
#' @param ylab Title for the y axis (see `?title`).
#' @param interpolate Logical indicating whether to apply linear interpolation
#' to the image when drawing (default `TRUE`).
#' @param asp Numeric. The aspect ratio y/x (see `?plot.window`).
#' @param xaxs The style of axis interval calculation to be used for the x axis
#' (see `?par`).
#' @param yaxs The style of axis interval calculation to be used for the y axis
#' (see `?par`).
#' @param axes Logical. `TRUE` to draw axes (the default).
#' @param main The main title (on top).
#' @param legend Logical indicating whether to include a legend on the plot.
#' Currently, legends are only supported for continuous data. A color table
#' will be used if one is specified or the raster has a built-in color table,
#' otherwise the value for `col_map_fn` will be used.
#' @param digits The number of digits to display after the decimal point in
#' the legend labels when raster data are floating point.
#' @param na_col Color to use for `NA` as a 7- or 9-character hexadecimal code.
#' The default is transparent (`"#00000000"`, the return value of
#' `rgb(0,0,0,0)`).
#' @param ... Other parameters to be passed to `plot.default()`.
#'
#' @note
#' `plot_raster()` uses the function `graphics::rasterImage()` for plotting
#' which is not supported on some devices (see `?rasterImage`).
#'
#' If `data` is an object of class `GDALRaster`, then `plot_raster()` will
#' attempt to read the entire raster into memory by default (unless the number
#' of pixels per band would exceed `max_pixels`).
#' A reduced resolution overview can be read by setting `xsize`, `ysize`
#' smaller than the raster size on disk.
#' (If `data` is instead specified as a vector of pixel values, a reduced
#' resolution overview would be read by setting `out_xsize` and `out_ysize`
#' smaller than the raster region defined by `xsize`, `ysize` in a call to
#' `GDALRaster$read()`).
#' The GDAL_RASTERIO_RESAMPLING configuration option can be
#' defined to override the default resampling (NEAREST) to one of BILINEAR,
#' CUBIC, CUBICSPLINE, LANCZOS, AVERAGE or MODE, for example:
#' \preformatted{
#' set_config_option("GDAL_RASTERIO_RESAMPLING", "BILINEAR")
#' }
#'
#' @seealso
#' [`GDALRaster$read()`][GDALRaster], [read_ds()], [set_config_option()]
#'
#' @examples
#' ## Elevation
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file)
#'
#' # all other arguments are optional when passing a GDALRaster object
#' # grayscale
#' plot_raster(ds, legend = TRUE, main = "Storm Lake elevation (m)")
#'
#' # color ramp from user-defined palette
#' elev_pal <- c("#00A60E","#63C600","#E6E600","#E9BD3B",
#'               "#ECB176","#EFC2B3","#F2F2F2")
#' ramp <- scales::colour_ramp(elev_pal, alpha = FALSE)
#' plot_raster(ds, col_map_fn = ramp, legend = TRUE,
#'             main = "Storm Lake elevation (m)")
#'
#' ds$close()
#'
#' ## Landsat band combination
#' b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#' b6_file <- system.file("extdata/sr_b6_20200829.tif", package="gdalraster")
#' band_files <- c(b6_file, b5_file, b4_file)
#'
#' vrt_file <- file.path(tempdir(), "storml_b6_b5_b4.vrt")
#' buildVRT(vrt_file, band_files, cl_arg = "-separate")
#'
#' ds <- new(GDALRaster, vrt_file)
#'
#' plot_raster(ds, main = "Landsat 6-5-4 (vegetative analysis)")
#'
#' ds$close()
#' \dontshow{vsi_unlink(vrt_file)}
#'
#' ## LANDFIRE Existing Vegetation Cover (EVC) with color map
#' evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")
#'
#' # colors from the CSV attribute table distributed by LANDFIRE
#' evc_csv <- system.file("extdata/LF20_EVC_220.csv", package="gdalraster")
#' vat <- read.csv(evc_csv)
#' head(vat)
#' vat <- vat[, c(1, 6:8)]
#'
#' ds <- new(GDALRaster, evc_file)
#' plot_raster(ds, col_tbl = vat, interpolate = FALSE,
#'             main = "Storm Lake LANDFIRE EVC")
#'
#' ds$close()
#'
#' ## Apply a pixel function
#' f <- system.file("extdata/complex.tif", package="gdalraster")
#' ds <- new(GDALRaster, f)
#' ds$getDataTypeName(band = 1)  # complex floating point
#'
#' ramp <- scales::colour_ramp(scales::pal_viridis(option = "plasma")(6),
#'                             alpha = FALSE)
#'
#' plot_raster(ds, pixel_fn = Arg, col_map_fn = ramp, interpolate = FALSE,
#'             legend = TRUE, main = "Arg(complex.tif)")
#'
#' ds$close()
#' @export
plot_raster <- function(data, xsize=NULL, ysize=NULL, nbands=NULL,
                        max_pixels=2.5e7, col_tbl=NULL, maxColorValue=1,
                        normalize=TRUE, minmax_def=NULL, minmax_pct_cut=NULL,
                        col_map_fn=NULL, pixel_fn=NULL, xlim=NULL, ylim=NULL,
                        interpolate=TRUE, asp=1, axes=TRUE, main="",
                        xlab="x", ylab="y", xaxs="i", yaxs="i",
                        legend=FALSE, digits=2, na_col=rgb(0,0,0,0), ...) {

    if (isTRUE((grDevices::dev.capabilities()$rasterImage == "no"))) {
        message("device does not support 'rasterImage()'")
        return()
    }

    if (!is.null(nbands)) {
        if (!(nbands %in% c(1, 3)))
            stop("number of bands must be 1 or 3", call.=FALSE)
    }

    if (is.null(max_pixels))
        max_pixels <- Inf

    if (!is.null(col_map_fn)) {
        if (!is.function(col_map_fn))
            stop("'col_map_fn' must be a function", call. = FALSE)
    }

    if (!is.null(pixel_fn)) {
        if (!is.function(pixel_fn))
            stop("'pixel_fn' must be a function", call. = FALSE)
    }

    data_in <- NULL
    is_byte_raster <- TRUE
    south_up <- FALSE

    if (is(data, "Rcpp_GDALRaster")) {
        dm <- as.numeric(data$dim()) ## prevent integer overflow

        if (is.null(xsize))
            xsize <- out_xsize <- dm[1]
        else
            out_xsize <- trunc(xsize)

        if (is.null(ysize))
            ysize <- out_ysize <- dm[2]
        else
            out_ysize <- trunc(ysize)

        if ((out_xsize*out_ysize) > max_pixels)
            stop("'xsize * ysize' exceeds 'max_pixels'", call.=FALSE)

        if (is.null(nbands)) {
            if (data$getRasterCount() == 3)
                nbands <- 3
            else
                nbands <- 1
        }

        for (b in 1:nbands) {
            if (data$getDataTypeName(b) != "Byte")
                is_byte_raster <- FALSE
        }

        data_in <- read_ds(data, bands=1:nbands, xoff=0, yoff=0,
                           xsize=dm[1], ysize=dm[2],
                           out_xsize=out_xsize, out_ysize=out_ysize)

        if (nbands==1 && is.null(col_tbl) && is.null(col_map_fn)) {
            # check for a built-in color table
            if (!is.null(data$getColorTable(band=1)) &&
                data$getPaletteInterp(band=1)=="RGB") {

                col_tbl <- data$getColorTable(band=1)
                maxColorValue <- 255
            }
        }

        gt <- data$getGeoTransform()
        if (is.null(xlim))
            xlim <- c(gt[1], gt[1] + gt[2] * dm[1])
        if (is.null(ylim)) {
            ylim <- c(gt[4] + gt[6] * dm[2], gt[4])
            if (gt[6] > 0) {
                south_up <- TRUE
                ylim <- rev(ylim)
            }
        }

    } else if (!is.null(attr(data, "gis"))) {
        gis <- attr(data, "gis")

        if (is.list(data))
            data_in <- unlist(data, use.names=FALSE)
        else
            data_in <- data

        xlim <- c(gis$bbox[1], gis$bbox[3])
        ylim <- c(gis$bbox[2], gis$bbox[4])
        xsize <- gis$dim[1]
        ysize <- gis$dim[2]

        if ((xsize*ysize) > max_pixels)
            stop("'xsize * ysize' exceeds 'max_pixels'", call.=FALSE)

        if (is.null(nbands)) {
            if (gis$dim[3] == 1)
                nbands <- 1
            else if (gis$dim[3] == 3)
                nbands <- 3
            else
                stop("'nbands' must be 1 or 3", call.=FALSE)
        } else {
            if (nbands != gis$dim[3]) {
                stop("'nbands' does not match the input data", call. = FALSE)
            }
        }

        if (is.list(data)) {
            if (nbands != length(data))
                stop("'nbands' is not equal to 'length(data)'", call.=FALSE)
        }

        for (b in 1:nbands) {
            if (gis$datatype[b] != "Byte")
                is_byte_raster <- FALSE
        }

    } else {
        if (is.null(nbands))
            stop("'nbands' cannot be determined from 'data'", call.=FALSE)

        if (is.null(xsize) || is.null(ysize))
            stop("'xsize' and 'ysize' of data must be specified", call.=FALSE)

        if ((xsize*ysize) > max_pixels)
            stop("'xsize * ysize' exceeds 'max_pixels'", call.=FALSE)

        if (is.list(data))
            data_in <- unlist(data, use.names=FALSE)
        else
            data_in <- data

        if (is.null(xlim))
            xlim <- c(0, xsize)
        if (is.null(ylim))
            ylim <- c(ysize, 0)

        is_byte_raster <- FALSE  # unknown
    }

    if (nbands == 3 && is_byte_raster && normalize &&
            is.null(minmax_def) && is.null(minmax_pct_cut)) {

        # default to no stretch for RGB Byte raster
        minmax_def <- c(0, 0, 0, 255, 255, 255)
    }

    if (typeof(data_in) == "raw") {
        data_in <- as.integer(data_in)
    }

    if (!is.null(pixel_fn)) {
        data_in <- pixel_fn(data_in)
    }

    if (typeof(data_in) == "complex") {
        stop("specify 'pixel_fn' when plotting complex data types",
             call. = FALSE)
    }

    if (south_up)
        data_in <- .flip_vertical(data_in, xsize, ysize, nbands)

    a <- array(data_in, dim = c(xsize, ysize, nbands))
    r <- .as_raster(a,
                    col_tbl=col_tbl,
                    maxColorValue=maxColorValue,
                    normalize=normalize,
                    minmax_def=minmax_def,
                    minmax_pct_cut=minmax_pct_cut,
                    col_map_fn=col_map_fn,
                    na_col=na_col)

    if (legend && nbands != 1) {
        message("legend is not supported for RGB plot")
        legend <- FALSE
    }

    op <- NULL
    if (legend) {
        if (is.null(col_map_fn))
            col_map_fn <- grDevices::gray
        op <- graphics::par(no.readonly = TRUE)  # save original for reset
        graphics::layout(matrix(1:2, ncol=2), width=c(6,1), height=c(1,1))
        graphics::par(mar=c(5, 4, 4, 0.5) + 0.1)
    }

    graphics::plot.new()
    graphics::plot.window(xlim=xlim, ylim=ylim, asp=asp,
                          xaxs=xaxs, yaxs=yaxs, ...)
    graphics::rasterImage(r, xlim[1], ylim[1], xlim[2], ylim[2],
                          interpolate=interpolate)

    if (axes) {
        graphics::title(main=main, xlab=xlab, ylab=ylab)
        graphics::axis(1)
        graphics::axis(2)
    } else {
        graphics::title(main=main)
    }

    if (legend) {
        mm <- NULL # define legend min/max
        if (!is.null(minmax_def))
            mm <- minmax_def
        else if (!is.null(minmax_pct_cut))
            mm <- stats::quantile(data_in,
                                  probs=c(minmax_pct_cut[1] / 100,
                                          minmax_pct_cut[2] / 100),
                                  na.rm = TRUE,
                                  names=FALSE)
        else
            mm <- c(min(data_in, na.rm=TRUE), max(data_in, na.rm=TRUE))

        if (is.null(col_tbl)) {
            # continuous data with col_map_fn (the default)
            leg_data <- seq(mm[1], mm[2], length.out=256)
            if (normalize)
                leg_data <- .normalize(leg_data, mm)
            leg_data <- sort(leg_data, decreasing=TRUE)
            leg_data <- col_map_fn(leg_data)
            leg_img <- grDevices::as.raster(matrix(leg_data, ncol=1))

        } else {
            # continuous data with col_tbl
            leg_data <- sort(seq(mm[1], mm[2], by=1), decreasing=TRUE)
            leg_data <- array(leg_data, dim=c(1, length(leg_data), 1))
            leg_img <- .as_raster(leg_data,
                                  col_tbl=col_tbl,
                                  maxColorValue=maxColorValue,
                                  na_col=na_col)
        }

        graphics::par(mar=c(6, 0.5, 6, 2) + 0.1)
        graphics::plot(c(0,2), c(0,1), type="n", axes=FALSE,
                       xlab="", ylab="", main="")

        if (is(data_in, "integer"))
            leg_lab <- formatC(seq(mm[1], mm[2], length.out=5), format="d")
        else
            leg_lab <- formatC(seq(mm[1], mm[2], length.out=5), format="f",
                               digits=digits)

        graphics::text(x=1.5,
                       y=seq(0, 1, length.out=5),
                       labels=leg_lab,
                       adj=c(0, NA),
                       xpd=TRUE)
        graphics::rasterImage(leg_img, 0, 0, 1, 1)
        graphics::par(op)  # reset to original
    }

    invisible()
}
