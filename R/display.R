# Display raster data using base graphics

#' @noRd
.normalize <- function(x) {
	return((x - min(x,na.rm=TRUE)) / (max(x,na.rm=TRUE) - min(x,na.rm=TRUE)))
}

#' @noRd
.as_raster <- function(a, col_tbl=NULL, normalize=TRUE, col_map_fn=NULL, 
				na_col=rgb(0,0,0,0), ...) {
				
# Create an object of class "raster", a matrix of color values representing
# a bitmap image for input to graphics::rasterImage().

	if ( !(dim(a)[3] %in% c(1,3)) )
		stop("Number of bands must be 1 or 3")
	
	has_na <- logical(0)
	nas <- array()
	r <- array()
	
	if (!is.null(col_tbl)) {
		# color table mapping
		if(!dim(a)[3] == 1)
			stop("A color table can only be used with single-band data.")
			
		if(!is.data.frame(ct <- as.data.frame(col_tbl)))
			stop("Color table must be a data frame.")
		
		if(ncol(ct) != 4)
			stop("Color table must have four columns.")

		nas <- is.na(a)
		has_na <- any(nas)
		if (has_na) {
			dim(nas) <- dim(nas)[1:2]
			nas <- t(nas)
		}
		
		ct[,5] <- rgb(ct[,2], ct[,3], ct[,4])
		f <- function(x) { ifelse(is.na(x), NA_character_, ct[ct[,1]==x, 5]) }
		r <- vapply(a, f, "#FFFFFF")
		dim(r) <- dim(a)[2:1]
		class(r) <- "raster"
	}
	else {
		# grayscale/rgb color scaling
		if (normalize)
			a <- .normalize(a)
			
		if (is.null(col_map_fn))
		    col_map_fn <- ifelse(dim(a)[3]==1, grDevices::gray, grDevices::rgb)
		
		nas <- is.na(a)
		has_na <- any(nas)
		if (has_na) {
			a[nas] <- 0
			if (dim(nas)[3] == 3)
				nas <- nas[,,1] || nas[,,2] || nas[,,3]
			dim(nas) <- dim(nas)[1:2]
			nas <- t(nas)
		}
		
		if (dim(a)[3] == 1) {
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

#' Display raster data that has been read into a vector
#'
#' `plot_raster_data()` displays raster data using base graphics.
#'
#' @param data A numeric vector of pixel data to display, arranged in left to
#' right, top to bottom pixel order.
#' @param xsize The number of pixels along the x dimension in `data`.
#' @param ysize The number of pixels along the y dimension in `data`.
#' @param nbands The number of bands in `data`. Must be either 1 (grayscale) or
#' 3 (RGB). For RGB, `data` are interleaved by band.
#' @param col_tbl A color table as a data frame with four columns. Column 1 
#' contains the numeric raster values, columns 2:4 contain the intensities
#' (between 0 and 1) of the red, green and blue primaries.
#' @param normalize Logical. `TRUE` to rescale pixel values so that their
#' range is `[0,1]`. Ignored if `col_tbl` is used.
#' @param col_map_fn An optional color map function (default is 
#' `grDevices::gray` for single-band data or `grDevices::rgb` for 3-band).
#' Ignored if `col_tbl` is used.
#' @param xlim Numeric vector of length two giving the x coordinate range. The
#' default uses pixel/line space (`c(0, xsize)`).
#' @param ylim Numeric vector of length two giving the y coordinate range. The
#' default uses pixel/line space (`c(ysize, 0)`).
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
#' @param na_col Color to use for `NA` as a 7- or 9-character hexadecimal code.
#' The default is transparent (`"#00000000"`, the return value of
#' `rgb(0,0,0,0)`).
#' @param main The main title (on top).
#' @param ... Other parameters to be passed to `plot.default()`.
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
#' plot_raster_data(r, xsize=ncols, ysize=nrows, main="Storm Lake: elevation")
#'
#' # color ramp using 'terrain.colors' in base R
#' col_ramp <- scales::gradient_n_pal(terrain.colors(10))
#' plot_raster_data(r, xsize=ncols, ysize=nrows, col_map_fn=col_ramp,
#'                  main="Storm Lake: elevation")
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
#' plot_raster_data(r, xsize=ncols, ysize=nrows, nbands=3,
#'                  main="Storm Lake: Landsat 6-5-4 (vegetative analysis)")
#'
#' ## LANDFIRE vegetation with colors from the CSV attribute table
#' evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")
#' evc_vat <- system.file("extdata/LF20_EVC_220.csv", package="gdalraster")
#' vat <- read.csv(evc_vat)
#' head(vat)
#' 
#' ds <- new(GDALRaster, evc_file, read_only=TRUE)
#' ncols <- ds$getRasterXSize()
#' nrows <- ds$getRasterYSize()
#' 
#' r <- read_ds(ds)
#' ds$close()
#' plot_raster_data(r, xsize=ncols, ysize=nrows, col_tbl=vat[,c(1,6:8)],
#'                  main="Storm Lake: LANDFIRE canopy cover")
#' @export
plot_raster_data <- function(data, xsize, ysize, nbands=1,
						col_tbl=NULL, normalize=TRUE, col_map_fn=NULL,
						xlim=c(0, xsize), ylim=c(ysize, 0),
                 		xlab="x", ylab="y",
                 		interpolate=TRUE, asp=1, axes=TRUE, main="",
                 		xaxs="i", yaxs="i", na_col=rgb(0,0,0,0), ...) {

	if ( !(nbands %in% c(1,3)) )
		stop("Number of bands must be 1 or 3")
		
	if (grDevices::dev.capabilities()$rasterImage == "no") {
		message("Device does not support rasterImage().")
		invisible(FALSE)
	}
	
	if (length(data) == 0)
		stop("length(data) is 0.")
		
	if (xsize < 1 || ysize < 1)
		stop("Invalid x/y dimensions.")
		
	a <- array(data, dim = c(xsize, ysize, nbands))
	r <- .as_raster(a,
					col_tbl=col_tbl,
					normalize=normalize,
					col_map_fn=col_map_fn,
					na_col=na_col)

	graphics::plot.new()
	graphics::plot.window(xlim=xlim, ylim=ylim, asp=asp,
							xaxs=xaxs, yaxs=yaxs, ...)
	graphics::rasterImage(r, xlim[1], ylim[1], xlim[2], ylim[2],
							interpolate=interpolate)
	graphics::title(main)
	if (axes) {
		graphics::axis(1)
		graphics::axis(2)
	}

	invisible(TRUE)
}

