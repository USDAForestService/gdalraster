# Display raster data using base graphics

#' @noRd
.normalize <- function(x, minmax=NULL) {

# Normalize to a range of [0,1].
# Normalize to the full range of the input data by default.
# Optionally normalize to a user-defined range in terms of the input.

	if (is.null(minmax)) {
		xn <- (x - min(x,na.rm=TRUE)) / (max(x,na.rm=TRUE) - min(x,na.rm=TRUE))
	}
	else {
		xn <- (x - minmax[1]) / (minmax[2] - minmax[1])
		xn[xn < 0] <- 0
		xn[xn > 1] <- 1
	}
	
	return(xn)
}

#' @noRd
.as_raster <- function(a, col_tbl=NULL, normalize=TRUE,
				minmax_def=NULL, minmax_pct_cut=NULL,
				col_map_fn=NULL, na_col=rgb(0,0,0,0), ...) {
				
# Create an object of class "raster", a matrix of color values representing
# a bitmap image for input to graphics::rasterImage(). Input is an array of
# pixel values with dimensions xsize, ysize, nbands.

	nbands <- dim(a)[3]
	
	if ( !(nbands %in% c(1,3)) )
		stop("Number of bands must be 1 or 3")
	
	has_na <- logical(0)
	nas <- array()
	r <- array()
	
	if (!is.null(col_tbl)) {
		# map to a color table
		
		if(nbands != 1)
			stop("A color table can only be used with single-band data.")
			
		if(!is.data.frame(ct <- as.data.frame(col_tbl)))
			stop("Color table must be a matrix or data frame.")
		
		if(ncol(ct) != 4)
			stop("Color table must have four columns.")

		nas <- is.na(a)
		has_na <- any(nas)
		if (has_na) {
			dim(nas) <- dim(nas)[1:2]
			nas <- t(nas)
		}
		
		ct[,5] <- rgb(ct[,2], ct[,3], ct[,4])
		f <- function(x) { ct[ct[,1]==x, 5][1] }
		r <- vapply(a, FUN=f, FUN.VALUE="#000000", USE.NAMES=FALSE)
		dim(r) <- dim(a)[2:1]
		class(r) <- "raster"
	}
	else {
		# gray/rgb color scaling
		
		if (normalize) {
			if (!is.null(minmax_def)) {
				for (b in 1:nbands) {
					a[,,b] <- .normalize(a[,,b], minmax_def[c(b, b+nbands)])
				}
			}
			else if (!is.null(minmax_pct_cut)) {
				for (b in 1:nbands) {
					q <- stats::quantile(a[,,b],
								probs=c(minmax_pct_cut[1] / 100,
										minmax_pct_cut[2] / 100),
								na.rm = TRUE, names=FALSE)
					a[,,b] <- .normalize(a[,,b], q)
				}
			}
			else {
				for (b in 1:nbands) {
					a[,,b] <- .normalize(a[,,b])
				}
			}
		}

		if (is.null(col_map_fn))
			col_map_fn <- ifelse(nbands==1, grDevices::gray, grDevices::rgb)
		
		nas <- is.na(a)
		has_na <- any(nas)
		if (has_na) {
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
		}
		else {
			# rgb
			r <- col_map_fn(a[,,1], a[,,2], a[,,3])
			dim(r) <- dim(a)[2:1]
			class(r) <- "raster"
		}
	}
	
	if (has_na)
        r[nas] <- na_col
	
	return(r)
}

#' Display raster data that have been read into a vector
#'
#' `plot_raster()` displays raster data using base graphics.
#'
#' @param data A numeric vector of pixel data to display, arranged in left to
#' right, top to bottom pixel order.
#' @param xsize The number of pixels along the x dimension in `data`.
#' @param ysize The number of pixels along the y dimension in `data`.
#' @param nbands The number of bands in `data`. Must be either 1 (grayscale) or
#' 3 (RGB). For RGB, `data` are interleaved by band.
#' @param col_tbl A color table as a matrix or data frame with four columns.
#' Column 1 contains the numeric raster values, columns 2:4 contain the
#' intensities (between 0 and 1) of the red, green and blue primaries.
#' @param normalize Logical. `TRUE` to rescale pixel values so that their
#' range is `[0,1]`, normalized to the full range of the pixel data by default
#' (`min(data)`, `max(data)`, per band). Ignored if `col_tbl` is used.
#' `normalize=FALSE` if a custom color mapping function is used that
#' operates on raw pixel values (see `col_map_fn` below).
#' @param minmax_def Normalize to user-defined min/max values (in terms of
#' the pixel data, per band). For single-band grayscale, a numeric vector of
#' length two containing min, max. For 3-band RGB, a numeric vector of length
#' six containing b1_min, b2_min, b3_min, b1_max, b2_max, b3_max.
#' @param minmax_pct_cut Normalize to a truncated range of the pixel data using
#' percentile cutoffs (removes outliers). A numeric vector of length two giving
#' the percentiles to use (e.g., `c(2, 98)`). Applied per band. Ignored if
#' `minmax_def` is used. Set `normalize=FALSE` if using a color mapping
#' function that operates on raw pixel values.
#' @param col_map_fn An optional color map function (default is 
#' `grDevices::gray` for single-band data or `grDevices::rgb` for 3-band).
#' Ignored if `col_tbl` is used.
#' @param xlim Numeric vector of length two giving the x coordinate range. The
#' default uses pixel/line coordinates (`c(0, xsize)`).
#' @param ylim Numeric vector of length two giving the y coordinate range. The
#' default uses pixel/line coordinates (`c(ysize, 0)`).
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
#' By deafult, a legend will be included if plotting single-band data without
#' `col_tbl` specified. Legend is not currently supported for color tables or
#' with 3-band RGB data.
#' @param na_col Color to use for `NA` as a 7- or 9-character hexadecimal code.
#' The default is transparent (`"#00000000"`, the return value of
#' `rgb(0,0,0,0)`).
#' @param ... Other parameters to be passed to `plot.default()`.
#'
#' @note
#' `plot_raster()` uses the function `graphics::rasterImage()` for plotting
#' which is not supported on some devices (see `?rasterImage`).
#'
#' A reduced resolution overview for display can be read by setting `out_xsize`
#' and `out_ysize` smaller than the raster region specified by `xsize`, `ysize`
#' in calls to `GDALRaster$read()` or `read_ds()`.
#' The GDAL_RASTERIO_RESAMPLING configuration option can be defined to 
#' override the default resampling to one of BILINEAR, CUBIC, CUBICSPLINE, 
#' LANCZOS, AVERAGE or MODE.
#'
#' @seealso
#' [`GDALRaster$read()`][GDALRaster], [read_ds()], [set_config_option()]
#'
#' @examples
#' ## Elevation
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file, read_only=TRUE)
#' ncols <- ds$getRasterXSize()
#' nrows <- ds$getRasterYSize()
#' r <- read_ds(ds)
#' ds$close()
#'
#' # grayscale
#' plot_raster(r, xsize=ncols, ysize=nrows, main="Storm Lake elevation (m)")
#'
#' # color ramp
#' elev_pal <- c("#008435","#B4E34F","#F5D157","#CF983B","#B08153","#FFFFFF")
#' ramp <- scales::colour_ramp(elev_pal, alpha=FALSE)
#' plot_raster(r, xsize=ncols, ysize=nrows, col_map_fn=ramp,
#'             main="Storm Lake elevation (m)")
#'
#' ## Landsat band combination
#' b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#' b6_file <- system.file("extdata/sr_b6_20200829.tif", package="gdalraster")
#' band_files <- c(b6_file, b5_file, b4_file)
#' 
#' ds <- new(GDALRaster, b5_file, read_only=TRUE)
#' ncols <- ds$getRasterXSize()
#' nrows <- ds$getRasterYSize()
#' ds$close()
#'
#' r <- vector("integer")
#' for (f in band_files) {
#'   ds <- new(GDALRaster, f, read_only=TRUE)
#'   r <- c(r, read_ds(ds))
#'   ds$close()
#' }
#'
#' plot_raster(r, xsize=ncols, ysize=nrows, nbands=3,
#'             main="Landsat 6-5-4 (vegetative analysis)")
#'
#' ## LANDFIRE existing veg cover with colors from the CSV attribute table
#' evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")
#' evc_vat <- system.file("extdata/LF20_EVC_220.csv", package="gdalraster")
#' vat <- read.csv(evc_vat)
#' head(vat)
#' vat <- vat[,c(1,6:8)]
#' 
#' ds <- new(GDALRaster, evc_file, read_only=TRUE)
#' ncols <- ds$getRasterXSize()
#' nrows <- ds$getRasterYSize()
#' 
#' r <- read_ds(ds)
#' ds$close()
#' plot_raster(r, xsize=ncols, ysize=nrows, col_tbl=vat, interpolate=FALSE,
#'             main="Storm Lake Existing Vegetation Cover")
#' @export
plot_raster <- function(data, xsize, ysize, nbands=1,
						col_tbl=NULL, normalize=TRUE, minmax_def=NULL,
						minmax_pct_cut=NULL, col_map_fn=NULL,
						xlim=c(0, xsize), ylim=c(ysize, 0),
						interpolate=TRUE, asp=1, axes=TRUE, main="",
						xlab="x", ylab="y", xaxs="i", yaxs="i", 
						legend=NULL, na_col=rgb(0,0,0,0), ...) {

	if ( !(nbands %in% c(1,3)) )
		stop("Number of bands must be 1 or 3")
		
	if (isTRUE((grDevices::dev.capabilities()$rasterImage == "no"))) {
		message("Device does not support rasterImage().")
		return()
	}
	
	if (length(data) == 0)
		stop("length(data) is 0.")
		
	if (xsize < 1 || ysize < 1)
		stop("Invalid x/y dimensions.")
		
	if (is.null(legend)) {
		if (nbands==1 && is.null(col_tbl))
			legend <- TRUE
		else
			legend <- FALSE
	}

	a <- array(data, dim = c(xsize, ysize, nbands))
	r <- .as_raster(a,
					col_tbl=col_tbl,
					normalize=normalize,
					minmax_def=minmax_def,
					minmax_pct_cut=minmax_pct_cut,
					col_map_fn=col_map_fn,
					na_col=na_col)

	if (legend) {
		if (nbands != 1) {
			message("Legend is not supported for RGB plot.")
			legend <- FALSE
		}
		else if (!is.null(col_tbl)) {
			message("Legend is currently not supported for color tables.")
			legend <- FALSE
		}
	}
	
	op <- NULL
	if (legend) {
		if (is.null(col_map_fn))
			col_map_fn <- grDevices::gray
		op <- graphics::par(no.readonly = TRUE)  # save original for reset
		graphics::layout(matrix(1:2, ncol=2), width=c(5,1), height=c(1,1))
		graphics::par(mar=c(5, 4, 4, 0.5) + 0.1)
	}
	graphics::plot.new()
	graphics::plot.window(xlim=xlim, ylim=ylim, asp=asp,
							xaxs=xaxs, yaxs=yaxs, ...)
	graphics::rasterImage(r, xlim[1], ylim[1], xlim[2], ylim[2],
							interpolate=interpolate)
	graphics::title(main=main, xlab=xlab, ylab=ylab)
	if (axes) {
		graphics::axis(1)
		graphics::axis(2)
	}
	if (legend) {
		mm <- NULL
		if (!is.null(minmax_def))
			mm <- minmax_def
		else if (!is.null(minmax_pct_cut))
			mm <- stats::quantile(data,
									probs=c(minmax_pct_cut[1] / 100,
											minmax_pct_cut[2] / 100),
									na.rm = TRUE, names=FALSE)
		else
			mm <- c(min(data, na.rm=TRUE), max(data, na.rm=TRUE))

		leg_data <- .normalize(seq(mm[1], mm[2], length.out=100))
		leg_img <- grDevices::as.raster(matrix(rev(col_map_fn(leg_data)),
										ncol=1))
		graphics::par(mar=c(6, 0.5, 6, 2) + 0.1)
		graphics::plot(c(0,2), c(0,1), type="n", axes=FALSE,
						xlab="", ylab="", main="")
		graphics::text(x=1.5, y=seq(0, 1, length.out=5),
						labels=seq(mm[1], mm[2], length.out=5),
						adj=c(0, NA), xpd=TRUE)
		graphics::rasterImage(leg_img, 0, 0, 1, 1)
		graphics::par(op)  # reset to original
	}

	invisible()
}

