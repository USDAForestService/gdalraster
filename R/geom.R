# Exported functions that use the GEOS convenience library defined in
# src/geos_wkt.h.
# Chris Toney <chris.toney at usda.gov>

#' Bounding box intersection / union
#'
#' @description
#' `bbox_intersect()` returns the bounding box intersection, and 
#' `bbox_union()` returns the bounding box union, for input of 
#' either raster file names or list of bounding boxes. All of the inputs 
#' must be in the same projected coordinate system.
#' These functions require GDAL built with the GEOS library.
#' 
#' @param x Either a character vector of raster file names, or a list with 
#' each element a bounding box numeric vector (xmin, ymin, xmax, ymax).
#' @param as_wkt Logical. `TRUE` to return the bounding box as a polygon  
#' in OGC WKT format, or `FALSE` to return as a numeric vector.
#' @return The intersection (`bbox_intersect()`) or union (`bbox_union()`)  
#' of inputs. 
#' If `as_wkt = FALSE`, a numeric vector of length four containing 
#' xmin, ymin, xmax, ymax. If `as_wkt = TRUE`, a character string 
#' containing OGC WKT for the bbox as POLYGON.
#' `NA` is returned if GDAL was built without the GEOS library.
#' 
#' @seealso
#' [bbox_from_wkt()], [bbox_to_wkt()]
#' 
#' @examples
#' bbox_list <-list()
#'
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file, read_only=TRUE)
#' bbox_list[[1]] <- ds$bbox()
#' ds$close()
#'
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#' ds <- new(GDALRaster, b5_file, read_only=TRUE)
#' bbox_list[[2]] <- ds$bbox()
#' ds$close()
#'
#' bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2 
#' 5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5, 
#' 325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
#' bbox_list[[3]] <- bbox_from_wkt(bnd)
#'
#' print(bbox_list)
#' bbox_intersect(bbox_list)
#' bbox_union(bbox_list)
#' @export
bbox_intersect <- function(x, as_wkt = FALSE) {

	if (!has_geos()) {
		if (as_wkt)
			return(NA_character_)
		else
			return(rep(NA_real_, 4))
	}

	n <- length(x)
	this_bbox <- ""
	
	if (is.character(x)) {
		ds <- new(GDALRaster, x[1], read_only=TRUE)
		this_bbox <- bbox_to_wkt(ds$bbox())
		ds$close()
	}
	else if (is.list(x)) {
		this_bbox <- bbox_to_wkt(x[[1]])
	}
	else {
		stop("Input object not recognized.", call. = FALSE)
	}
	
	i <- 2
	while (i <= n) {
		if (is.character(x)) {
			ds <- new(GDALRaster, x[i], read_only=TRUE)
			this_bbox <- .g_intersection( this_bbox, bbox_to_wkt(ds$bbox()) )
			ds$close()
		}
		else {
			this_bbox <- .g_intersection( this_bbox, bbox_to_wkt(x[[i]]) )
		}
		i <- i + 1
	}
	
	if (as_wkt)
		return(this_bbox)
	else
		return(bbox_from_wkt(this_bbox))
}


#' @rdname bbox_intersect
#' @export
bbox_union <- function(x, as_wkt = FALSE) {

	if (!has_geos()) {
		if (as_wkt)
			return(NA_character_)
		else
			return(rep(NA_real_, 4))
	}

	n <- length(x)
	this_bbox <- ""
	
	if (is.character(x)) {
		ds <- new(GDALRaster, x[1], read_only=TRUE)
		this_bbox <- bbox_to_wkt(ds$bbox())
		ds$close()
	}
	else if (is.list(x)) {
		this_bbox <- bbox_to_wkt(x[[1]])
	}
	else {
		stop("Input object not recognized.", call. = FALSE)
	}
	
	i <- 2
	while (i <= n) {
		if (is.character(x)) {
			ds <- new(GDALRaster, x[i], read_only=TRUE)
			this_bbox <- .g_union( this_bbox, bbox_to_wkt(ds$bbox()) )
			ds$close()
		}
		else {
			this_bbox <- .g_union( this_bbox, bbox_to_wkt(x[[i]]) )
		}
		i <- i + 1
	}
	
	if (as_wkt)
		return(this_bbox)
	else
		return(bbox_from_wkt(this_bbox))
}

