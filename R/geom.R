# R interface for the GEOS functions defined in src/geom_api.h
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
            this_bbox <- g_intersection(this_bbox, bbox_to_wkt(ds$bbox()))
            ds$close()
        } else {
            this_bbox <- g_intersection(this_bbox, bbox_to_wkt(x[[i]]))
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
            this_bbox <- g_union(this_bbox, bbox_to_wkt(ds$bbox()))
            ds$close()
        } else {
            this_bbox <- g_union(this_bbox, bbox_to_wkt(x[[i]]))
        }
        i <- i + 1
    }

    if (as_wkt)
        return(this_bbox)
    else
        return(bbox_from_wkt(this_bbox))
}

#' Transform a bounding box to a different projection
#'
#' `bbox_transform()` is a convenience function for:
#' ```
#' bbox_to_wkt(bbox) |>
#'   g_transform(srs_from, srs_to) |>
#'   bbox_from_wkt()
#' ```
#'
#' @param bbox Numeric vector of length four containing a bounding box
#' (xmin, ymin, xmax, ymax) to transform.
#' @param srs_from Character string in OGC WKT format specifying the
#' spatial reference system for `bbox`.
#' @param srs_to Character string in OGC WKT format specifying the target
#' spatial reference system.
#' @return Numeric vector of length four containing a transformed bounding box
#' (xmin, ymin, xmax, ymax).
#'
#' @seealso
#' [g_transform()], [bbox_from_wkt()], [bbox_to_wkt()]
#'
#' @examples
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file)
#' ds$bbox()
#' bbox_transform(ds$bbox(), ds$getProjection(), epsg_to_wkt(4326))
#' ds$close()
#' @export
bbox_transform <- function(bbox, srs_from, srs_to) {
    if (!(is.numeric(bbox) && length(bbox) == 4))
        stop("'bbox' must be a length-4 numeric vector", call. = FALSE)

    return(bbox_to_wkt(bbox) |>
               g_transform(srs_from, srs_to) |>
               bbox_from_wkt())
}

#' Geometry WKB/WKT conversion
#'
#' `g_wk2wk()` converts geometries between Well Known Binary (WKB) and
#' Well Known Text (WKT) formats. A geometry given as a raw vector of WKB will
#' be converted to a WKT string, while a geometry given as a WKT string will be
#' converted to a WKB raw vector. Input may also be a list of WKB raw vectors
#' or a character vector of WKT strings.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors to convert
#' to WKT, or a character vector containing one or more WKT strings to
#' convert to WKB.
#' @param as_iso Logical scalar. `TRUE` to export as ISO WKB/WKT (ISO 13249
#' SQL/MM Part 3), or `FALSE` (the default) to export as "Extended WKB/WKT"
#' (see Note).
#' @param byte_order Character string specifying the byte order when converting
#' to WKB. One of `"LSB"` (the default) or `"MSB"` (uncommon).
#' @return
#' For input of a WKB raw vector or list of raw vectors, returns a character
#' vector of WKT strings, with length of the returned vector equal to the
#' number of input raw vectors. For input of a single WKT string, returns a raw
#' vector of WKB. For input of a character vector containing more than one WKT
#' string, returns a list of WKB raw vectors, with length of the returned list
#' equal to the number of input strings.
#'
#' @note
#' With `as_iso = FALSE` (the default), geometries are exported as extended
#' dimension (Z) WKB/WKT for types Point, LineString, Polygon, MultiPoint,
#' MultiLineString, MultiPolygon and GeometryCollection. For other geometry
#' types, it is equivalent to ISO.
#'
#' When the return value is a list of WKB raw vectors, an element in the
#' returned list will contain `NA` if the corresponding input string was `NA`
#' or empty (`""`).
#'
#' When input is a list of WKB raw vectors, a corresponding element in the
#' returned character vector will be an empty string (`""`) if the input was
#' a raw vector of length `0` (`raw(0)`). If an input list element is not a raw
#' vector, then the corresponding element in the returned character vector will
#' be `NA`.
#'
#' @seealso
#' GEOS reference for geometry formats:\cr
#' \url{https://libgeos.org/specifications/}
#'
#' @examples
#' wkt <- "POINT (-114 47)"
#' wkb <- g_wk2wk(wkt)
#' print(wkb)
#' g_wk2wk(wkb)
#' @export
g_wk2wk <- function(geom, as_iso = FALSE, byte_order = "LSB") {
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a logical scalar", call. = FALSE)
    # byte_order
    if (is.null(byte_order))
        byte_order <- "LSB"
    if (!is.character(byte_order) || length(byte_order) > 1)
        stop("'byte_order' must be a character string", call. = FALSE)
    byte_order <- toupper(byte_order)
    if (byte_order != "LSB" && byte_order != "MSB")
        stop("invalid 'byte_order'", call. = FALSE)

    if (is.character(geom)) {
        if (length(geom) == 1) {
            return(.g_wkt2wkb(geom, as_iso, byte_order))
        } else {
            return(.g_wkt_vector2wkb(geom, as_iso, byte_order))
        }
    } else if (is.raw(geom)) {
        return(.g_wkb2wkt(geom, as_iso))
    } else if (is.list(geom)) {
        return(.g_wkb_list2wkt(geom, as_iso))
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }
}

#' Compute buffer of a WKT geometry
#'
#' `g_buffer()` builds a new geometry containing the buffer region around
#' the geometry on which it is invoked. The buffer is a polygon containing
#' the region within the buffer distance of the original geometry.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param dist Numeric buffer distance in units of the `wkt` geometry.
#' @param quad_segs Integer number of segments used to define a 90 degree
#' curve (quadrant of a circle). Large values result in large numbers of
#' vertices in the resulting buffer geometry while small numbers reduce the
#' accuracy of the result.
#' @param as_wkb Logical, `TRUE` to return the output geometry in WKB
#' format (the default), or `FALSE` to return as WKT.
#' @param as_iso Logical, `TRUE` to export as ISO WKB/WKT (ISO 13249
#' SQL/MM Part 3), or `FALSE` (the default) to export as "Extended WKB/WKT".
#' @param byte_order Character string specifying the byte order when output is
#' WKB. One of `"LSB"` (the default) or `"MSB"` (uncommon).
#' @param quiet Logical, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return
#' A polygon as WKB raw vector or WKT string, or a list/character vector of
#' polygons as WKB/WKT with length equal to `length(geom)`. `NA` is returned
#' with a warning if an input geometry cannot be converted into an OGR
#' geometry object, or if an error occurs in the call to Buffer() in the
#' underlying OGR API.
#'
#' @examples
#' g_buffer("POINT (0 0)", dist = 10, as_wkb = FALSE)
#' @export
g_buffer <- function(geom, dist, quad_segs = 30L, as_wkb = TRUE,
                     as_iso = FALSE, byte_order = "LSB", quiet = FALSE) {

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a logical scalar", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a logical scalar", call. = FALSE)
    # byte_order
    if (is.null(byte_order))
        byte_order <- "LSB"
    if (!is.character(byte_order) || length(byte_order) > 1)
        stop("'byte_order' must be a character string", call. = FALSE)
    byte_order <- toupper(byte_order)
    if (byte_order != "LSB" && byte_order != "MSB")
        stop("invalid 'byte_order'", call. = FALSE)
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a logical scalar", call. = FALSE)

    wkb <- NULL
    if (is.raw(geom)) {
        wkb <- .g_buffer(geom, dist, quad_segs, as_iso, byte_order, quiet)
    } else if (is.list(geom) && is.raw(geom[[1]])) {
        wkb <- lapply(geom, .g_buffer, dist, quad_segs, as_iso,
                      byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_buffer(g_wk2wk(geom), dist, quad_segs, as_iso,
                             byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_buffer, dist, quad_segs,
                          as_iso, byte_order, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))
}

#' Apply a coordinate transformation to a WKT geometry
#'
#' `g_transform()` will transform the coordinates of a geometry from their
#' current spatial reference system to a new target spatial reference system.
#' Normally this means reprojecting the vectors, but it could include datum
#' shifts, and changes of units.
#'
#' @param wkt Character. OGC WKT string for a simple feature geometry.
#' @param srs_from Character string in OGC WKT format specifying the
#' spatial reference system for the geometry given by `wkt`.
#' @param srs_to Character string in OGC WKT format specifying the target
#' spatial reference system.
#' @param wrap_date_line Logical scalar. `TRUE` to correct geometries that
#' incorrectly go from a longitude on a side of the antimeridian to the other
#' side. Defaults to `FALSE`.
#' @param date_line_offset Integer scalar. Longitude gap in degree. Defaults
#' to `10`.
#' @return Character string for a transformed OGC WKT geometry.
#'
#' @note
#' This function uses the `OGR_GeomTransformer_Create()` and
#' `OGR_GeomTransformer_Transform()` functions in the GDAL API: "This is an
#' enhanced version of `OGR_G_Transform()`. When reprojecting geometries from
#' a Polar Stereographic projection or a projection naturally crossing the
#' antimeridian (like UTM Zone 60) to a geographic CRS, it will cut geometries
#' along the antimeridian. So a `LineString` might be returned as a
#' `MultiLineString`."
#'
#' The `wrap_date_line = TRUE` option might be specified for circumstances to
#' correct geometries that incorrectly go from a longitude on a side of the
#' antimeridian to the other side, e.g., `LINESTRING (-179 0,179 0)` will be
#' transformed to `MULTILINESTRING ((-179 0,-180 0),(180 0,179 0))`. For that
#' use case, `srs_to` might be the same as `srs_from`.
#'
#' @seealso
#' [bbox_transform()]
#'
#' @examples
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file)
#'
#' # the convenience function bbox_transform() does this:
#' bbox_to_wkt(ds$bbox()) |>
#'   g_transform(ds$getProjection(), epsg_to_wkt(4326)) |>
#'   bbox_from_wkt()
#'
#' ds$close()
#'
#' # correct geometries that incorrectly go from a longitude on a side of the
#' # antimeridian to the other side
#' geom <- "LINESTRING (-179 0,179 0)"
#' srs <- epsg_to_wkt(4326)
#' g_transform(geom, srs, srs, wrap_date_line = TRUE)
#' @export
g_transform <- function(wkt, srs_from, srs_to, wrap_date_line = FALSE,
                        date_line_offset = 10L) {
    if (!(is.character(wkt) && length(wkt) == 1))
        stop("'wkt' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(srs_from) && length(srs_from) == 1))
        stop("'srs_from' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(srs_to) && length(srs_to) == 1))
        stop("'srs_to' must be a length-1 character vector", call. = FALSE)

    return(.g_transform(wkt, srs_from, srs_to, wrap_date_line,
                        date_line_offset))
}

#' Extract geometry type names from WKB/WKT geometries
#'
#' `g_name()` returns geometry type names in well known text format.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param quiet Logical, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return character vector of the same length as the number of input
#' geometries in `geom`, containing the WKT names for the corresponding
#' geometries.
#'
#' @examples
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file)
#' bbox_to_wkt(ds$bbox()) |> g_name()
#' ds$close()
#' @export
g_name <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a logical scalar", call. = FALSE)

    ret <- NULL
    if (is.raw(geom)) {
        ret <- .g_name(geom, quiet)
    } else if (is.list(geom) && is.raw(geom[[1]])) {
        ret <- sapply(geom, .g_name, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_name(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_name, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' Obtain text summaries of WKB/WKT geometries
#'
#' `g_summary()` returns text summaries of WKB/WKT geometries.
#' Requires GDAL >= 3.7.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param quiet Logical, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return character vector of the same length as the number of input
#' geometries in `geom`, containing summaries for the corresponding geometries.
#'
#' @examples
#' # Requires GDAL >= 3.7
#' if (as.integer(gdal_version()[2]) >= 3070000) {
#'   f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
#'   lyr <- new(GDALVector, f, "mtbs_perims")
#'
#'   feat <- lyr$getNextFeature()
#'   g_summary(feat$geom)
#'
#'   feat_set <- lyr$fetch(5)
#'   g_summary(feat_set$geom)
#'
#'   lyr$close()
#' }
#' @export
g_summary <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a logical scalar", call. = FALSE)

    ret <- NULL
    if (is.raw(geom)) {
        ret <- .g_summary(geom, quiet)
    } else if (is.list(geom) && is.raw(geom[[1]])) {
        ret <- sapply(geom, .g_summary, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_summary(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_summary, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' Test if a geometry is empty
#'
#' `g_is_empty()` tests whether a geometry has no points.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param quiet Logical, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return logical vector of the same length as the number of input
#' geometries in `geom`, containing `TRUE` for the corresponding geometries
#' that are empty or `FALSE` for non-empty geometries.
#'
#' @examples
#' g1 <- "POLYGON ((0 0, 10 10, 10 0, 0 0))"
#' g2 <- "POLYGON ((5 1, 9 5, 9 1, 5 1))"
#' g_difference(g2, g1) |> g_is_empty()
#' @export
g_is_empty <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a logical scalar", call. = FALSE)

    ret <- NULL
    if (is.raw(geom)) {
        ret <- .g_is_empty(geom, quiet)
    } else if (is.list(geom) && is.raw(geom[[1]])) {
        ret <- sapply(geom, .g_is_empty, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_is_empty(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_is_empty, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' Test if a geometry is valid
#'
#' `g_is_valid()` tests whether a geometry is valid.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param quiet Logical, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return logical vector of the same length as the number of input
#' geometries in `geom`, containing `TRUE` for the corresponding geometries
#' that are valid or `FALSE` for invalid geometries.
#'
#' @examples
#' g1 <- "POLYGON ((0 0, 10 10, 10 0, 0 0))"
#' g2 <- "POLYGON ((0 0, 10 10, 10 0))"
#' g3 <- "POLYGON ((0 0, 10 10, 10 0, 0 1))"
#' g_is_valid(c(g1, g2, g3))
#' @export
g_is_valid <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a logical scalar", call. = FALSE)

    ret <- NULL
    if (is.raw(geom)) {
        ret <- .g_is_valid(geom, quiet)
    } else if (is.list(geom) && is.raw(geom[[1]])) {
        ret <- sapply(geom, .g_is_valid, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_is_valid(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_is_valid, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' Attempt to make invalid geometries valid
#'
#' `g_make_valid()` attempts to make an invalid geometry valid without losing
#' vertices. Already-valid geometries are cloned without further intervention.
#' Wrapper of `OGR_G_MakeValid()`/`OGR_G_MakeValidEx()` in the GDAL Vector C
#' API.
#'
#' @details
#' LINEWORK is the default method, which combines all rings into a set of noded
#' lines and then extracts valid polygons from that linework. The STRUCTURE
#' method (requires GEOS >= 3.10 and GDAL >= 3.4) first makes all rings valid,
#' then merges shells and subtracts holes from shells to generate a valid
#' result. Assumes that holes and shells are correctly categorized.
#'
#' KEEP_COLLAPSED only applies to the STRUCTURE method:
#' * `FALSE` (the default): collapses are converted to empty geometries
#' * `TRUE`: collapses are converted to a valid geometry of lower dimension
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param method Character string. One of `"LINEWORK"` (the default) or
#' `"STRUCTURE"` (requires GEOS >= 3.10 and GDAL >= 3.4). See Details.
#' @param keep_collapsed Logical, applies only to the STRUCTURE method.
#' Defaults to `FALSE`. See Details.
#' @param as_wkb Logical, `TRUE` to return the output geometry in WKB
#' format (the default), or `FALSE` to return as WKT.
#' @param as_iso Logical, `TRUE` to export as ISO WKB/WKT (ISO 13249
#' SQL/MM Part 3), or `FALSE` (the default) to export as "Extended WKB/WKT".
#' @param byte_order Character string specifying the byte order when output is
#' WKB. One of `"LSB"` (the default) or `"MSB"` (uncommon).
#' @param quiet Logical, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return
#' A geometry as WKB raw vector or WKT string, or a list/character vector of
#' geometries as WKB/WKT with length equal to `length(geom)`. `NA` is returned
#' with a warning if an input geometry cannot be converted into an OGR
#' geometry object, or if an error occurs in the call to MakeValid() in the
#' underlying OGR API.
#'
#' @note
#' This function is built on the GEOS >= 3.8 library, check it for the
#' definition of the geometry operation. If OGR is built without GEOS >= 3.8,
#' this function will return a clone of the input geometry if it is valid, or
#' `NA` if it is invalid.
#'
#' @examples
#' # requires GEOS >= 3.8, otherwise is only a validity test (see Note)
#' geos_version()
#'
#' # valid
#' wkt <- "POINT (0 0)"
#' g_make_valid(wkt, as_wkb = FALSE)
#'
#' # invalid to valid
#' wkt <- "POLYGON ((0 0,10 10,0 10,10 0,0 0))"
#' g_make_valid(wkt, as_wkb = FALSE)
#'
#' # invalid - error
#' wkt <- "LINESTRING (0 0)"
#' g_make_valid(wkt)  # NA
#' @export
g_make_valid <- function(geom, method = "LINEWORK", keep_collapsed = FALSE,
                         as_wkb = TRUE, as_iso = FALSE, byte_order = "LSB",
                         quiet = FALSE) {

    # method
    if (is.null(method))
        method <- "LINEWORK"
    if (!is.character(method) || length(method) > 1)
        stop("'method' must be a character string", call. = FALSE)
    # keep_collapsed
    if (is.null(keep_collapsed))
        keep_collapsed <- FALSE
    if (!is.logical(keep_collapsed) || length(keep_collapsed) > 1)
        stop("'keep_collapsed' must be a logical scalar", call. = FALSE)
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a logical scalar", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a logical scalar", call. = FALSE)
    # byte_order
    if (is.null(byte_order))
        byte_order <- "LSB"
    if (!is.character(byte_order) || length(byte_order) > 1)
        stop("'byte_order' must be a character string", call. = FALSE)
    byte_order <- toupper(byte_order)
    if (byte_order != "LSB" && byte_order != "MSB")
        stop("invalid 'byte_order'", call. = FALSE)
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a logical scalar", call. = FALSE)

    wkb <- NULL
    if (is.raw(geom)) {
        wkb <- .g_make_valid(geom, method, keep_collapsed, as_iso,
                             byte_order, quiet)
    } else if (is.list(geom) && is.raw(geom[[1]])) {
        wkb <- lapply(geom, .g_make_valid, method, keep_collapsed, as_iso,
                      byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_make_valid(g_wk2wk(geom), method, keep_collapsed, as_iso,
                                 byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_make_valid, method, keep_collapsed,
                          as_iso, byte_order, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))
}

#' Geometry binary predicates operating on WKT
#'
#' These functions implement tests for pairs of geometries in OGC WKT format.
#' @name g_binary_pred
#' @details
#' These functions use the GEOS library via GDAL headers.
#'
#' `g_intersects()` tests whether two geometries intersect.
#'
#' `g_disjoint()` tests if this geometry and the other geometry are disjoint.
#'
#' `g_touches()` tests if this geometry and the other geometry are touching.
#'
#' `g_contains()` tests if this geometry contains the other geometry.
#'
#' `g_within()` tests if this geometry is within the other geometry.
#'
#' `g_crosses()` tests if this geometry and the other geometry are crossing.
#'
#' `g_overlaps()` tests if this geometry and the other geometry overlap, that
#' is, their intersection has a non-zero area (they have some but not all
#' points in common).
#'
#' `g_equals()` tests whether two geometries are equivalent.
#' The GDAL documentation says: "This operation implements the SQL/MM
#' `ST_OrderingEquals()` operation. The comparison is done in a structural way,
#' that is to say that the geometry types must be identical, as well as the
#' number and ordering of sub-geometries and vertices. Or equivalently, two
#' geometries are considered equal by this method if their WKT/WKB
#' representation is equal. Note: this must be distinguished from equality in
#' a spatial way."
#'
#' @param this_geom Character. OGC WKT string for a simple feature geometry.
#' @param other_geom Character. OGC WKT string for a simple feature geometry.
#' @return Logical scalar
#'
#' @seealso
#' \url{https://en.wikipedia.org/wiki/DE-9IM}
#'
#'
#' @note
#' Geometry validity is not checked. In case you are unsure of the validity
#' of the input geometries, call `g_is_valid()` before, otherwise the result
#' might be wrong.
#' @export
g_intersects <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_intersects(this_geom, other_geom))
}

#' @name g_binary_pred
#' @export
g_disjoint <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_disjoint(this_geom, other_geom))
}

#' @name g_binary_pred
#' @export
g_touches <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_touches(this_geom, other_geom))
}

#' @name g_binary_pred
#' @export
g_contains <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_contains(this_geom, other_geom))
}

#' @name g_binary_pred
#' @export
g_within <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_within(this_geom, other_geom))
}

#' @name g_binary_pred
#' @export
g_crosses <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_crosses(this_geom, other_geom))
}

#' @name g_binary_pred
#' @export
g_overlaps <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_overlaps(this_geom, other_geom))
}

#' @name g_binary_pred
#' @export
g_equals <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_equals(this_geom, other_geom))
}

#' Binary operations on WKT geometries
#'
#' These functions implement operations on pairs of geometries in OGC WKT
#' format.
#' @name g_binary_op
#' @details
#' These functions use the GEOS library via GDAL headers.
#'
#' `g_intersection()` returns a new geometry which is the region of
#' intersection of the two geometries operated on. `g_intersects()` can be used
#' to test if two geometries intersect.
#'
#' `g_union()` returns a new geometry which is the region of
#' union of the two geometries operated on.
#'
#' `g_difference()` returns a new geometry which is the region of this geometry
#' with the region of the other geometry removed.
#'
#' `g_sym_difference()` returns a new geometry which is the symmetric
#' difference of this geometry and the other geometry (union minus
#' intersection).
#'
#' @param this_geom Character. OGC WKT string for a simple feature geometry.
#' @param other_geom Character. OGC WKT string for a simple feature geometry.
#' @return Character string. The resulting geometry as OGC WKT.
#'
#' @note
#' Geometry validity is not checked. In case you are unsure of the validity
#' of the input geometries, call `g_is_valid()` before, otherwise the result
#' might be wrong.
#'
#' @examples
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file)
#' g1 <- ds$bbox() |> bbox_to_wkt()
#' ds$close()
#'
#' g2 <- "POLYGON ((327381.9 5104541.2, 326824.0 5104092.5, 326708.8 5103182.9,
#'   327885.2 5102612.9, 329334.5 5103322.4, 329304.2 5104474.5,328212.7
#'   5104656.4, 328212.7 5104656.4, 327381.9 5104541.2))"
#'
#' # see spatial predicate defintions at https://en.wikipedia.org/wiki/DE-9IM
#' g_intersects(g1, g2)  # TRUE
#' g_overlaps(g1, g2)  # TRUE
#' # therefore,
#' g_contains(g1, g2)  # FALSE
#'
#' g_sym_difference(g1, g2) |> g_area()
#'
#' g3 <- g_intersection(g1, g2)
#' g4 <- g_union(g1, g2)
#' g_difference(g4, g3) |> g_area()
#' @export
g_intersection <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_intersection(this_geom, other_geom))
}

#' @name g_binary_op
#' @export
g_union <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_union(this_geom, other_geom))
}

#' @name g_binary_op
#' @export
g_difference <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_difference(this_geom, other_geom))
}

#' @name g_binary_op
#' @export
g_sym_difference <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_sym_difference(this_geom, other_geom))
}

#' Compute the distance between two geometries
#'
#' `g_distance()` returns the distance between two geometries or `-1` if an
#' error occurs. Returns the shortest distance between the two geometries.
#' The distance is expressed into the same unit as the coordinates of the
#' geometries.
#'
#' @param this_geom Character. OGC WKT string for a simple feature geometry.
#' @param other_geom Character. OGC WKT string for a simple feature geometry.
#' @return Numeric. Distance or '-1' if an error occurs.
#'
#' @note
#' Geometry validity is not checked. In case you are unsure of the validity
#' of the input geometries, call `g_is_valid()` before, otherwise the result
#' might be wrong.
#'
#' @examples
#' g_distance("POINT (0 0)", "POINT (5 12)")
#' @export
g_distance <- function(this_geom, other_geom) {
    if (!(is.character(this_geom) && length(this_geom) == 1))
        stop("'this_geom' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(other_geom) && length(other_geom) == 1))
        stop("'other_geom' must be a length-1 character vector", call. = FALSE)

    return(.g_distance(this_geom, other_geom))
}

#' Compute the length of a geometry
#'
#' `g_length()` computes the length for `LineString` or `MultiCurve` objects.
#' Undefined for all other geometry types (returns zero).
#'
#' @param wkt Character. OGC WKT string for a simple feature geometry.
#' @return Numeric scalar. Length of the geometry or `0`.
#'
#' @examples
#' g_length("LINESTRING (0 0, 3 4)")
#' @export
g_length <- function(wkt) {
    if (!(is.character(wkt) && length(wkt) == 1))
        stop("'wkt' must be a length-1 character vector", call. = FALSE)

    return(.g_length(wkt))
}

#' Compute the area of a geometry
#'
#' `g_area()` computes the area for a `LinearRing`, `Polygon` or
#' `MultiPolygon`. Undefined for all other geometry types (returns zero).
#'
#' @param wkt Character. OGC WKT string for a simple feature geometry.
#' @return Numeric scalar. Area of the geometry or `0`.
#'
#' @note
#' `LinearRing` is a non-standard geometry type, used in GDAL just for geometry
#' creation.
#' @export
g_area <- function(wkt) {
    if (!(is.character(wkt) && length(wkt) == 1))
        stop("'wkt' must be a length-1 character vector", call. = FALSE)

    return(.g_area(wkt))
}

#' Compute the centroid of a geometry
#'
#' `g_centroid()` returns a vector of point X, point Y.
#'
#' @details
#' The GDAL documentation states "This method relates to the SFCOM
#' `ISurface::get_Centroid()` method however the current implementation based
#' on GEOS can operate on other geometry types such as multipoint, linestring,
#' geometrycollection such as multipolygons. OGC SF SQL 1.1 defines the
#' operation for surfaces (polygons). SQL/MM-Part 3 defines the operation for
#' surfaces and multisurfaces (multipolygons)."
#'
#' @param wkt Character. OGC WKT string for a simple feature geometry.
#' @return Numeric vector of length 2 containing the centroid (X, Y).
#' @export
g_centroid <- function(wkt) {
    if (!(is.character(wkt) && length(wkt) == 1))
        stop("'wkt' must be a length-1 character vector", call. = FALSE)

    return(.g_centroid(wkt))
}
