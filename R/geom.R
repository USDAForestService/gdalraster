# Exported functions that use the GEOS convenience library defined
# in src/geos_wkt.h.
# Chris Toney <chris.toney at usda.gov>


#' Get GEOS version
#'
#' @description
#' `geos_version()` returns version information for the GEOS library in use by
#' GDAL. Requires GDAL >= 3.4.
#'
#' @returns A list of length four containing:
#'   * `name` - a string formatted as "major.minor.patch"
#'   * `major` - major version as integer
#'   * `minor` - minor version as integer
#'   * `patch` - patch version as integer
#'
#' List elements will be `NA` if GDAL < 3.4.
#'
#' @seealso
#' [gdal_version()], [proj_version()]
#'
#' @examples
#' geos_version()
#' @export
geos_version <- function() {
    ver <- .getGEOSVersion()
    gv <- list()
    if (anyNA(ver))
        gv$name <- NA_character_
    else
        gv$name <- paste(ver, collapse=".")
    gv$major <- ver[1]
    gv$minor <- ver[2]
    gv$patch <- ver[3]
    return(gv)
}


#' Bounding box intersection / union
#'
#' @description
#' `bbox_intersect()` returns the bounding box intersection, and
#' `bbox_union()` returns the bounding box union, for input of
#' either raster file names or list of bounding boxes. All of the inputs
#' must be in the same projected coordinate system.
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
#'
#' @seealso
#' [bbox_from_wkt()], [bbox_to_wkt()]
#'
#' @examples
#' bbox_list <-list()
#'
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file)
#' bbox_list[[1]] <- ds$bbox()
#' ds$close()
#'
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#' ds <- new(GDALRaster, b5_file)
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
    n <- length(x)
    this_bbox <- ""

    if (is.character(x)) {
        ds <- new(GDALRaster, x[1], read_only=TRUE)
        this_bbox <- bbox_to_wkt(ds$bbox())
        ds$close()
    } else if (is.list(x)) {
        this_bbox <- bbox_to_wkt(x[[1]])
    } else {
        stop("input must be a list or character vector", call. = FALSE)
    }

    i <- 2
    while (i <= n) {
        if (is.character(x)) {
            ds <- new(GDALRaster, x[i], read_only=TRUE)
            this_bbox <- .g_intersection(this_bbox, bbox_to_wkt(ds$bbox()))
            ds$close()
        } else {
            this_bbox <- .g_intersection(this_bbox, bbox_to_wkt(x[[i]]))
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
    n <- length(x)
    this_bbox <- ""

    if (is.character(x)) {
        ds <- new(GDALRaster, x[1], read_only=TRUE)
        this_bbox <- bbox_to_wkt(ds$bbox())
        ds$close()
    } else if (is.list(x)) {
        this_bbox <- bbox_to_wkt(x[[1]])
    } else {
        stop("input must be a list or character vector", call. = FALSE)
    }

    i <- 2
    while (i <= n) {
        if (is.character(x)) {
            ds <- new(GDALRaster, x[i], read_only=TRUE)
            this_bbox <- .g_union(this_bbox, bbox_to_wkt(ds$bbox()))
            ds$close()
        } else {
            this_bbox <- .g_union(this_bbox, bbox_to_wkt(x[[i]]))
        }
        i <- i + 1
    }

    if (as_wkt)
        return(this_bbox)
    else
        return(bbox_from_wkt(this_bbox))
}


#' Compute buffer of a WKT geometry
#'
#' `g_buffer()` builds a new geometry containing the buffer region around
#' the geometry on which it is invoked. The buffer is a polygon containing
#' the region within the buffer distance of the original geometry.
#'
#' @param wkt Character. OGC WKT string for a simple feature 2D geometry.
#' @param dist Numeric buffer distance in units of the `wkt` geometry.
#' @param quad_segs Integer number of segments used to define a 90 degree
#' curve (quadrant of a circle). Large values result in large numbers of
#' vertices in the resulting buffer geometry while small numbers reduce the
#' accuracy of the result.
#' @return Character string for an OGC WKT polygon.
#'
#' @seealso
#' [bbox_from_wkt()], [bbox_to_wkt()]
#'
#' @examples
#' g_buffer(wkt = "POINT (0 0)", dist = 10)
g_buffer <- function(wkt, dist, quad_segs = 30) {
    if (!(is.character(wkt) && length(wkt) == 1))
        stop("'wkt' must be a length-1 character vector", call. = FALSE)

    return(.g_buffer(wkt, dist, quad_segs))
}

#' Apply a coordinate transformation to a WKT geometry
#'
#' `g_transform()` will transform the coordinates of a geometry from their
#' current spatial reference system to a new target spatial reference system.
#' Normally this means reprojecting the vectors, but it could include datum
#' shifts, and changes of units.
#'
#' @param wkt Character. OGC WKT string for a simple feature 2D geometry.
#' @param srs_from Character string in OGC WKT format specifying the
#' spatial reference system for the geometry given by `wkt`.
#' @param srs_to Character string in OGC WKT format specifying the target
#' spatial reference system.
#' @return Character string for a transformed OGC WKT geometry.
#'
#' @note
#' This function only does reprojection on a point-by-point basis. It does not
#' include advanced logic to deal with discontinuities at poles or antimeridian.
#'
#' @seealso
#' [bbox_from_wkt()], [bbox_to_wkt()]
#'
#' @examples
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file)
#' bbox_to_wkt(ds$bbox()) |>
#'   g_transform(ds$getProjectionRef(), epsg_to_wkt(4326)) |>
#'   bbox_from_wkt()
#' ds$close()
g_transform <- function(wkt, srs_from, srs_to) {
    if (!(is.character(wkt) && length(wkt) == 1))
        stop("'wkt' must be a length-1 character vector", call. = FALSE)

    return(.g_transform(wkt, srs_from, srs_to))
}
