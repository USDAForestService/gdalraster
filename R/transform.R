# Public R wrappers for the internal functions defined in src/transform.h
# Chris Toney <chris.toney at usda.gov>

#' Transform geospatial x/y coordinates
#'
#' `transform_xy()` transforms geospatial x, y coordinates to a new
#' projection. The input points may optionally have z vertices (x, y, z) or
#' time values (x, y, z, t).
#' Wrapper for `OGRCoordinateTransformation::Transform()` in the GDAL Spatial
#' Reference System C++ API.
#'
#' @param pts A data frame or numeric matrix containing geospatial point
#' coordinates, or point geometries as a list of WKB raw vectors or character
#' vector of WKT strings. If data frame or matrix, the number of columns must
#' be either two (x, y), three (x, y, z) or four (x, y, z, t).
#' May be also be given as a numeric vector for one point (xy, xyz, or xyzt).
#' @param srs_from Character string specifying the spatial reference system
#' for `pts`. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param srs_to Character string specifying the output spatial reference
#' system. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @returns Numeric matrix of geospatial (x, y) coordinates in the projection
#' specified by `srs_to` (potentially also with z, or z and t columns).
#'
#' @note
#' `transform_xy()` uses traditional GIS order for the input and output xy
#' (i.e., longitude/latitude ordered for geographic coordinates).
#'
#' Input points that contain missing values (`NA`) will be assigned `NA` in
#' the output and a warning emitted. Input points that fail to transform
#' with the GDAL API call will also be assigned `NA` in the output with a
#' specific warning indicating that case.
#'
#' @seealso
#' [srs_to_wkt()], [inv_project()]
#' @examples
#' pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
#' pts <- read.csv(pt_file)
#' print(pts)
#' # id, x, y in NAD83 / UTM zone 12N
#' # transform to NAD83 / CONUS Albers
#' transform_xy(pts = pts[, -1], srs_from = "EPSG:26912", srs_to = "EPSG:5070")
#' @export
transform_xy <- function(pts, srs_from, srs_to) {
    if (missing(srs_from) || is.null(srs_from))
        stop("'srs_from' is required", call. = FALSE)
    if (!is.character(srs_from) || length(srs_from) > 1)
        stop("'srs_from' must be a character string", call. = FALSE)
    if (missing(srs_to) || is.null(srs_to))
        stop("'srs_to' is required", call. = FALSE)
    if (!is.character(srs_to) || length(srs_to) > 1)
        stop("'srs_to' must be a character string", call. = FALSE)

    if (missing(pts) || is.null(pts))
        stop("'pts' is required", call. = FALSE)

    pts_in <- NULL
    if (is.raw(pts) || (is.list(pts) && is.raw(pts[[1]]) ||
        is.character(pts))) {

        if (is.raw(pts)) {
            geom_type <- g_name(pts)
        } else {
            geom_type <- g_name(pts[[1]])
        }
        if (toupper(geom_type) != "POINT")
            stop("'pts' does not contain POINT geometry type", call. = FALSE)

        coords <- g_coords(pts)
        if (!is.null(coords$m)) {
            pts_in <- coords[, c("x", "y", "z", "m")]
        } else if (!is.null(coords$z)) {
            pts_in <- coords[, c("x", "y", "z")]
        } else {
            pts_in <- coords[, c("x", "y")]
        }
    } else if (is.data.frame(pts) || is.matrix(pts) || is.vector(pts)) {
        pts_in <- pts
    } else {
        stop("'pts' is not a valid input type", call. = FALSE)
    }

    return(.transform_xy(pts_in, srs_from, srs_to))
}

#' Inverse project geospatial x/y coordinates to longitude/latitude
#'
#' `inv_project()` transforms geospatial x/y coordinates to
#' longitude/latitude in the same geographic coordinate system used by the
#' given projected spatial reference system. The output long/lat can
#' optionally be set to a specific geographic coordinate system by specifying
#' a well known name (see Details).
#'
#' @details
#' By default, the geographic coordinate system of the projection specified
#' by `srs` will be used. If a specific geographic coordinate system is
#' desired, then `well_known_gcs` can be set to one of the values below:
#' \tabular{rl}{
#'  EPSG:n \tab where n is the code of a geographic coordinate system\cr
#'  WGS84  \tab same as EPSG:4326\cr
#'  WGS72  \tab same as EPSG:4322\cr
#'  NAD83  \tab same as EPSG:4269\cr
#'  NAD27  \tab same as EPSG:4267\cr
#'  CRS84  \tab same as WGS84\cr
#'  CRS72  \tab same as WGS72\cr
#'  CRS27  \tab same as NAD27
#' }
#' The coordinates returned by `inv_project()`will always be in longitude,
#' latitude order (traditional GIS order) regardless of the axis order defined
#' for the GCS names above.
#'
#' @param pts A data frame or numeric matrix containing geospatial point
#' coordinates, or point geometries as a list of WKB raw vectors or character
#' vector of WKT strings. If data frame or matrix, the number of columns must
#' be either two (x, y), three (x, y, z) or four (x, y, z, t).
#' May be also be given as a numeric vector for one point (xy, xyz, or xyzt).
#' @param srs Character string specifying the projected spatial reference
#' system for `pts`. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param well_known_gcs Optional character string containing a supported
#' well known name of a geographic coordinate system (see Details for
#' supported values).
#' @returns Numeric matrix of longitude, latitude (potentially also with z,
#' or z and t columns).
#'
#' @note
#' Input points that contain missing values (`NA`) will be assigned `NA` in
#' the output and a warning emitted. Input points that fail to transform
#' with the GDAL API call will also be assigned `NA` in the output with a
#' specific warning indicating that case.
#'
#' @seealso
#' [transform_xy()]
#' @examples
#' pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
#' # id, x, y in NAD83 / UTM zone 12N
#' pts <- read.csv(pt_file)
#' print(pts)
#' inv_project(pts[,-1], "EPSG:26912")
#' @export
inv_project <- function(pts, srs, well_known_gcs = NULL) {
    if (missing(srs) || is.null(srs))
        stop("'srs' is required", call. = FALSE)
    if (!is.character(srs) || length(srs) > 1)
        stop("'srs' must be a character string", call. = FALSE)

    if (missing(well_known_gcs) || is.null(well_known_gcs))
        well_known_gcs <- ""
    if (!is.character(well_known_gcs) || length(well_known_gcs) > 1)
        stop("'well_known_gcs' must be a character string", call. = FALSE)

    pts_in <- NULL
    if (is.raw(pts) || (is.list(pts) && is.raw(pts[[1]]) ||
        is.character(pts))) {

        if (is.raw(pts)) {
            geom_type <- g_name(pts)
        } else {
            geom_type <- g_name(pts[[1]])
        }
        if (toupper(geom_type) != "POINT")
            stop("'pts' does not contain POINT geometry type", call. = FALSE)

        coords <- g_coords(pts)
        if (!is.null(coords$m)) {
            pts_in <- coords[, c("x", "y", "z", "m")]
        } else if (!is.null(coords$z)) {
            pts_in <- coords[, c("x", "y", "z")]
        } else {
            pts_in <- coords[, c("x", "y")]
        }
    } else if (is.data.frame(pts) || is.matrix(pts) || is.vector(pts)) {
        pts_in <- pts
    } else {
        stop("'pts' is not a valid input type", call. = FALSE)
    }

    return(.inv_project(pts_in, srs, well_known_gcs))
}
