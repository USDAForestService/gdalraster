# Display raster data using base graphics

#' @noRd
.normalize <- function(x) {
	return((x - min(x,na.rm=TRUE)) / (max(x,na.rm=TRUE) - min(x,na.rm=TRUE)))
}

#' @noRd
.as_raster <- function(a, normalize=TRUE, col_scale_fn=NULL, 
				col_na=rgb(0,0,0,0), ...) {
				
	# Create an object of class "raster", a matrix of color values representing
	# a bitmap image for input to graphics::rasterImage().

	if ( !(dim(a)[3] %in% c(1,3)) )
		stop("Number of bands must be 1 or 3")
	
    if (normalize)
    	a <- .normalize(a)
    	
    if (is.null(col_scale_fn))
        col_scale_fn <- ifelse (dim(a)[3]==1, grDevices::gray, grDevices::rgb)
    
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
		r <- col_scale_fn(a)
		dim(r) <- dim(a)[2:1]
		class(r) <- "raster"
    }
    else {
    	# rgb
		r <- col_scale_fn(a[,,1], a[,,2], a[,,3])
		dim(r) <- dim(a)[2:1]
		class(r) <- "raster"
	}
	
	if (has_na)
        r[nas] <- col_na
	
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
#' @param normalize Logical. `TRUE` to rescale pixel values so that their
#' range is `[0,1`].
#' @param col_scale_fn An optional color scale function (default is `gray` for
#' single-band data or `rgb` for 3-band).
#' @param xlim Numeric vector of length two giving the x coordinate range in
#' pixel/line space. The default is `c(0, xsize)`.
#' @param ylim Numeric vector of length two giving the y coordinate range in
#' pixel/line space. The default is `c(ysize, 0)`.
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
#' @param col_na Color to use for `NA` as a 7 or 9-character hexadecimal code.
#' The default is transparent (`"#00000000"`, the return value of
#' `rgb(0,0,0,0)`).
#' @param main The main title (on top).
#' @param ... Other parameters to be passed to `plot.default`.
#' @export
plot_raster_data <- function(data, xsize, ysize, nbands=1,
						normalize=TRUE, col_scale_fn=NULL,
						xlim=c(0, xsize), ylim=c(ysize, 0),
                 		xlab="x", ylab="y",
                 		interpolate=TRUE, asp=1, axes=TRUE, main="",
                 		xaxs="i", yaxs="i", col_na=rgb(0,0,0,0), ...) {

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
					normalize=normalize,
					col_scale_fn=col_scale_fn,
					col_na=col_na)

	plot.new()
	plot.window(xlim=xlim, ylim=ylim, asp=asp, xaxs=xaxs, yaxs=yaxs, ...)
	rasterImage(r, 1, nrow(r), ncol(r), 1, interpolate=interpolate)
	title(main)
	if (axes) {
		axis(1)
		axis(2)
	}

	invisible(TRUE)
}

