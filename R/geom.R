# Exported functions using the GEOS convenience library defined in
# src/geos_conv.h.
# Chris Toney <chris.toney at usda.gov>


#' Get the bounding box intersection for an input of either raster file 
#' names or list of bounding boxes.
#' 
#' @param x Either a character vector of raster file names, or a list of
#' bbox numeric vectors (xmin, ymin, xmax, ymax).
#' @param as_wkt Logical. `TRUE` to return the bounding box as a polygon  
#' in OGC WKT format (character string). `FALSE` to return as numeric vector 
#' of xmin, ymin, xmax, ymax.
#' @return Intersection of inputs as either a numeric vector of length four 
#' containing xmin, ymin, xmax, ymax (`as_wkt = FALSE`, the default), or 
#' as a character string of OGC WKT for the bbox as POLYGON.
#' @seealso
#' [bbox_to_wkt()], [bbox_union()], [rasterToVRT()]
#' 
#' @examples
#' bbox_list <-list()
#'
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds_elev <- new(GDALRaster, elev_file, read_only=TRUE)
#' bbox_list[[1]] <- ds_elev$bbox()
#'
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#' ds_b5 <- new(GDALRaster, b5_file, read_only=TRUE)
#' bbox_list[[2]] <- ds_b5$bbox()
#'
#' bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2 
#' 5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5, 
#' 325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
#' bbox_list[[3]] <- bbox_from_wkt(bnd)
#'
#' print(bbox_list)
#' bbox_intersect(bbox_list)
bbox_intersect <- function(x, as_wkt = FALSE) {

	n <- length(x)
	if (is.character(x)) {
		ds <- new(GDALRaster, rasterfiles[1], read_only=TRUE)
		this_bbox <- bbox_to_wkt(ds$bbox())
		ds$close()
	}
	else if (is.list(x)) {
		this_bbox <- bbox_to_wkt(x[[1]])
	}
	else {
		stop("Input object not supported.", call. = FALSE)
	}
	
	i <- 2
	while (i <= n) {
		if (is.character(x)) {
			ds <- new(GDALRaster, rasterfiles[i], read_only=TRUE)
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


#' Get the bounding box union for an input of either raster file 
#' names or list of bounding boxes.
#' 
#' @param x Either a character vector of raster file names, or a list of
#' bbox numeric vectors (xmin, ymin, xmax, ymax).
#' @param as_wkt Logical. `TRUE` to return the bounding box as a polygon  
#' in OGC WKT format (character string). `FALSE` to return as numeric vector 
#' of xmin, ymin, xmax, ymax.
#' @return Union of inputs as either a numeric vector of length four 
#' containing xmin, ymin, xmax, ymax (`as_wkt = FALSE`, the default), or 
#' as a character string of OGC WKT for the bbox as POLYGON.
#' @seealso
#' [bbox_to_wkt()], [bbox_intersect()], [rasterToVRT()]
#' 
#' @examples
#' bbox_list <-list()
#'
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds_elev <- new(GDALRaster, elev_file, read_only=TRUE)
#' bbox_list[[1]] <- ds_elev$bbox()
#'
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#' ds_b5 <- new(GDALRaster, b5_file, read_only=TRUE)
#' bbox_list[[2]] <- ds_b5$bbox()
#'
#' bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2 
#' 5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5, 
#' 325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
#' bbox_list[[3]] <- bbox_from_wkt(bnd)
#'
#' print(bbox_list)
#' bbox_union(bbox_list)
bbox_union <- function(x, as_wkt = FALSE) {

	n <- length(x)
	if (is.character(x)) {
		ds <- new(GDALRaster, rasterfiles[1], read_only=TRUE)
		this_bbox <- bbox_to_wkt(ds$bbox())
		ds$close()
	}
	else if (is.list(x)) {
		this_bbox <- bbox_to_wkt(x[[1]])
	}
	else {
		stop("Input object not supported.", call. = FALSE)
	}
	
	i <- 2
	while (i <= n) {
		if (is.character(x)) {
			ds <- new(GDALRaster, rasterfiles[i], read_only=TRUE)
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

