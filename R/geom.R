# R interface for the GEOS functions defined in src/geom_api.h
# Chris Toney <chris.toney at usda.gov>


#' @noRd
#' @export
.is_raw_or_null <- function(x) {
    if (is.raw(x) || is.null(x))
        return(TRUE)
    else
        return(FALSE)
}

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
            this_bbox <- g_intersection(this_bbox, bbox_to_wkt(ds$bbox()),
                                        as_wkb = FALSE)
            ds$close()
        } else {
            this_bbox <- g_intersection(this_bbox, bbox_to_wkt(x[[i]]),
                                        as_wkb = FALSE)
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
            this_bbox <- g_union(this_bbox, bbox_to_wkt(ds$bbox()),
                                 as_wkb = FALSE)
            ds$close()
        } else {
            this_bbox <- g_union(this_bbox, bbox_to_wkt(x[[i]]), as_wkb = FALSE)
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
#' `bbox_transform()` is a convenience function to transform the coordinates
#' of a boundary from their current spatial reference system to a new target
#' spatial reference system.
#'
#' @details
#' With `use_transform_bounds = TRUE` (the default) this function returns:
#' ```
#' # requires GDAL >= 3.4
#' transform_bounds(bbox, srs_from, srs_to)
#' ```
#'
#' See Details for [transform_bounds()] for cases where the bounds crossed the
#' antimeridian.
#'
#' With `use_transform_bounds = FALSE`, this function returns:
#' ```
#' bbox_to_wkt(bbox) |>
#'   g_transform(srs_from, srs_to) |>
#'   bbox_from_wkt()
#' ```
#'
#' See the Note for [g_transform()] for cases where the bounds crossed the
#' antimeridian.
#'
#' @param bbox Numeric vector of length four containing a bounding box
#' (xmin, ymin, xmax, ymax) to transform.
#' @param srs_from Character string specifying the spatial reference system
#' for `pts`. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param srs_to Character string specifying the output spatial reference
#' system. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param use_transform_bounds Logical value, `TRUE` to use
#' `transform_bounds()` (the default, requires GDAL >= 3.4). If `FALSE`,
#' transformation is done with `g_transform()`.
#' @return Numeric vector of length four containing a transformed bounding box
#' (xmin, ymin, xmax, ymax).
#'
#' @seealso
#' [bbox_from_wkt()], [g_transform()], [transform_bounds()]
#'
#' @examples
#' bb <- c(-1405880.72, -1371213.76, 5405880.72, 5371213.76)
#'
#' # the default assumes GDAL >= 3.4
#' if (gdal_version_num() >= gdal_compute_version(3, 4, 0)) {
#'   bb_wgs84 <- bbox_transform(bb, "EPSG:32661", "EPSG:4326")
#' } else {
#'   bb_wgs84 <- bbox_transform(bb, "EPSG:32661", "EPSG:4326",
#'                              use_transform_bounds = FALSE)
#' }
#'
#' print(bb_wgs84)
#' @export
bbox_transform <- function(bbox, srs_from, srs_to,
                           use_transform_bounds = TRUE) {

    if (!(is.numeric(bbox) && length(bbox) == 4))
        stop("'bbox' must be a length-4 numeric vector", call. = FALSE)
    if (!(is.character(srs_from) && length(srs_from) == 1))
        stop("'srs_from' must be a character string", call. = FALSE)
    if (!(is.character(srs_to) && length(srs_to) == 1))
        stop("'srs_to' must be a character string", call. = FALSE)

    if (use_transform_bounds) {
        return(transform_bounds(bbox, srs_from, srs_to))
    } else {
        return(
            bbox_to_wkt(bbox) |>
                g_transform(srs_from, srs_to, as_wkb = FALSE) |>
                bbox_from_wkt()
        )
    }
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
#' @param as_iso Logical value, `TRUE` to export as ISO WKB/WKT (ISO 13249
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
#' dimension (Z) WKB/WKT for types `Point`, `LineString`, `Polygon`,
#' `MultiPoint`, `MultiLineString`, `MultiPolygon` and `GeometryCollection`.
#' For other geometry types, it is equivalent to ISO.
#'
#' When the return value is a list of WKB raw vectors, an element in the
#' returned list will contain `NULL` (and a warning emitted) if the
#' corresponding input string was `NA` or empty (`""`).
#'
#' When input is a list of WKB raw vectors, a corresponding element in the
#' returned character vector will be `NA` if the input was a raw vector of
#' length `0` (i.e., `raw(0)`). If an input list element is not a raw vector,
#' then the corresponding element in the returned character vector will also
#' be `NA`. A warning is emitted in each case.
#'
#' @seealso
#' GEOS reference for geometry formats:\cr
#' \url{https://libgeos.org/specifications/}
#'
#' @examples
#' wkt <- "POINT (-114 47)"
#' wkb <- g_wk2wk(wkt)
#' str(wkb)
#' g_wk2wk(wkb)
#' @export
g_wk2wk <- function(geom, as_iso = FALSE, byte_order = "LSB") {
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
            if (is.na(geom)) {
                return(NULL)
            } else {
                return(.g_wkt2wkb(geom, as_iso, byte_order))
            }
        } else {
            return(.g_wkt_vector2wkb(geom, as_iso, byte_order))
        }
    } else if (is.raw(geom)) {
        return(.g_wkb2wkt(geom, as_iso))
    } else if (is.list(geom)) {
        return(.g_wkb_list2wkt(geom, as_iso))
    } else if (is.null(geom)) {
        return(NA_character_)
    } else if (is.na(geom)) {
        return(NULL)
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }
}

#' Create WKB/WKT geometries from vertices, and add/get sub-geometries
#'
#' These functions create WKB/WKT geometries from input vertices, and build
#' container geometry types from sub-geometries.
#' @name g_factory
#' @details
#' These functions use the GEOS library via GDAL headers.
#'
#' `g_create()` creates a geometry object from the given point(s) and returns
#' a raw vector of WKB (the default) or a character string of WKT. Currently
#' supports creating `Point`, `MultiPoint`, `LineString`, `Polygon`, and
#' `GeometryCollection.`
#' If multiple input points are given for creating `Point` type, then multiple
#' geometries will be returned as a list of WKB raw vectors, or character
#' vector of WKT strings (if `as_wkb = FALSE`). Otherwise, a single geometry
#' is created from the input points. Only an empty `GeometryCollection` can be
#' created with this function, for subsequent use with `g_add_geom()`.
#'
#' `g_add_geom()` adds a geometry to a geometry container, e.g.,
#' `Polygon` to `Polygon` (to add an interior ring), `Point` to `MultiPoint`,
#' `LineString` to `MultiLineString`, `Polygon` to `MultiPolygon`, or mixed
#' geometry types to a `GeometryCollection`. Returns a new geometry, i.e,
#' the container geometry is not modified.
#'
#' `g_get_geom()` fetches a geometry from a geometry container (1-based
#' indexing). For a polygon, requesting the first sub-geometry returns the
#' exterior ring (`sub_geom_idx = 1`), and the interior rings are returned for
#' `sub_geom_idx > 1`.
#'
#' @param geom_type Character string (case-insensitive), one of `"POINT"`,
#' `"MULTIPOINT"`, `"LINESTRING"`, `"POLYGON"` (see Note) or
#' `"GEOMETRYCOLLECTION"`.
#' @param pts Numeric matrix of points (x, y, z, m), or `NULL` to create an
#' empty geometry. The points can be given as (x, y), (x, y, z) or
#' (x, y, z, m), so the input must have two, three or four columns.
#' Data frame input will be coerced to numeric matrix. Rings for polygon
#' geometries should be closed.
#' @param as_wkb Logical value, `TRUE` to return the output geometry in WKB
#' format (the default), or `FALSE` to return a WKT string.
#' @param as_iso Logical value, `TRUE` to export as ISO WKB/WKT (ISO 13249
#' SQL/MM Part 3), or `FALSE` (the default) to export as "Extended WKB/WKT".
#' @param byte_order Character string specifying the byte order when output is
#' WKB. One of `"LSB"` (the default) or `"MSB"` (uncommon).
#' @param sub_geom Either a raw vector of WKB or a character string of WKT.
#' @param container Either a raw vector of WKB or a character string of WKT for
#' a container geometry type.
#' @param sub_geom_idx An integer value giving the 1-based index of a
#' sub-geometry (numeric values will be coerced to integer by truncation).
#' @return
#' A geometry as WKB raw vector by default, or a WKT string if
#' `as_wkb = FALSE`. In the case of multiple input points for creating Point
#' geometry type, a list of WKB raw vectors or character vector of WKT strings
#' will be returned.
#'
#' @note
#' A `POLYGON` can be created for a single ring which will be the
#' exterior ring. Additional `POLYGON`s can be created and added to an
#' existing `POLYGON` with `g_add_geom()`. These will become interior rings.
#' Alternatively, an empty polygon can be created with `g_create("POLYGON")`,
#' followed by creation and addition of `POLYGON`s as sub-geometries. In that
#' case, the first added `POLYGON` will be the exterior ring. The next ones will
#' be the interior rings.
#'
#' Only an empty `GeometryCollection` can be created with `g_create()`, which
#' can then be used as a container with `g_add_geom()`. If given, input points
#' will be ignored by `g_create()` if `geom_type = "GEOMETRYCOLLECTION"`.
#'
#' @examples
#' # raw vector of WKB by default
#' g_create("POINT", c(1, 2))
#'
#' # as WKT
#' g_create("POINT", c(1, 2), as_wkb = FALSE)
#'
#' # or convert in either direction
#' g_create("POINT", c(1, 2)) |> g_wk2wk()
#' g_create("POINT", c(1, 2), as_wkb = FALSE) |> g_wk2wk()
#'
#' # create MultiPoint from a matrix of xyz points
#' x <- c(9, 1)
#' y <- c(1, 9)
#' z <- c(0, 10)
#' pts <- cbind(x, y, z)
#' mp <- g_create("MULTIPOINT", pts)
#' g_wk2wk(mp)
#' g_wk2wk(mp, as_iso = TRUE)
#'
#' # create an empty container and add sub-geometries
#' mp2 <- g_create("MULTIPOINT")
#' mp2 <- g_create("POINT", c(11, 2)) |> g_add_geom(mp2)
#' mp2 <- g_create("POINT", c(12, 3)) |> g_add_geom(mp2)
#' g_wk2wk(mp2)
#'
#' # get sub-geometry from container
#' g_get_geom(mp2, 2, as_wkb = FALSE)
#'
#' # plot WKT strings or a list of WKB raw vectors
#' pts <- c(0, 0,
#'          3, 0,
#'          3, 4,
#'          0, 0)
#' m <- matrix(pts, ncol = 2, byrow = TRUE)
#' (g <- g_create("POLYGON", m, as_wkb = FALSE))
#' plot_geom(g)
#' @export
g_create <- function(geom_type, pts = NULL, as_wkb = TRUE, as_iso = FALSE,
                     byte_order = "LSB") {

    # geom_type
    if (!(is.character(geom_type) && length(geom_type) == 1))
        stop("'geom_type' must be a character string", call. = FALSE)
    # pts
    if (!(is.numeric(pts) || is.data.frame(pts) || is.null(pts)))
        stop("'pts' must be a numeric matrix of xy[zm]", call. = FALSE)
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
    # byte_order
    if (is.null(byte_order))
        byte_order <- "LSB"
    if (!is.character(byte_order) || length(byte_order) > 1)
        stop("'byte_order' must be a character string", call. = FALSE)
    byte_order <- toupper(byte_order)
    if (byte_order != "LSB" && byte_order != "MSB")
        stop("invalid 'byte_order'", call. = FALSE)

    wkb <- NULL
    if (toupper(geom_type) == "POINT" && (is.matrix(pts) ||
        is.data.frame(pts))) {

        wkb <- lapply(seq_len(nrow(pts)),
                      function(i) {
                          .g_create("POINT", pts[i, ], as_iso, byte_order)
                      })
    } else {
        wkb <- .g_create(geom_type, pts, as_iso, byte_order)
    }

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))
}

#' @name g_factory
#' @export
g_add_geom <- function(sub_geom, container, as_wkb = TRUE, as_iso = FALSE,
                       byte_order = "LSB") {

    if ((is.character(sub_geom) || is.list(sub_geom)) && length(sub_geom) > 1)
        stop("'sub_geom' must be a single geometry", call. = FALSE)
    if (is.character(sub_geom))
        sub_geom <- g_wk2wk(sub_geom)
    if (!is.raw(sub_geom)) {
        stop("'sub_geom' must be a raw vector or character string",
             call. = FALSE)
    }

    if ((is.character(container) || is.list(container))
        && length(container) > 1) {

        stop("'sub_geom' must be a single geometry", call. = FALSE)
    }
    if (is.character(container))
        container <- g_wk2wk(container)
    if (!is.raw(container)) {
        stop("'container' must be a raw vector or character string",
             call. = FALSE)
    }

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
    # byte_order
    if (is.null(byte_order))
        byte_order <- "LSB"
    if (!is.character(byte_order) || length(byte_order) > 1)
        stop("'byte_order' must be a character string", call. = FALSE)
    byte_order <- toupper(byte_order)
    if (byte_order != "LSB" && byte_order != "MSB")
        stop("invalid 'byte_order'", call. = FALSE)

    wkb <- .g_add_geom(sub_geom, container, as_iso, byte_order)

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))

}

#' @name g_factory
#' @export
g_get_geom <- function(container, sub_geom_idx, as_wkb = TRUE, as_iso = FALSE,
                       byte_order = "LSB") {

    if ((is.character(container) || is.list(container))
        && length(container) > 1) {

        stop("'sub_geom' must be a single geometry", call. = FALSE)
    }
    if (is.character(container))
        container <- g_wk2wk(container)
    if (!is.raw(container)) {
        stop("'container' must be a raw vector or character string",
             call. = FALSE)
    }

    if (!(is.numeric(sub_geom_idx) && length(sub_geom_idx) == 1))
        stop("'sub_geom_idx' must be a single numeric value", call. = FALSE)

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
    # byte_order
    if (is.null(byte_order))
        byte_order <- "LSB"
    if (!is.character(byte_order) || length(byte_order) > 1)
        stop("'byte_order' must be a character string", call. = FALSE)
    byte_order <- toupper(byte_order)
    if (byte_order != "LSB" && byte_order != "MSB")
        stop("invalid 'byte_order'", call. = FALSE)

    wkb <- .g_get_geom(container, sub_geom_idx - 1, as_iso, byte_order)

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))
}

#' Obtain information about WKB/WKT geometries
#'
#' These functions return information about WKB/WKT geometries. The input
#' geometries may be given as a single raw vector of WKB, a list of WKB raw
#' vectors, or a character vector containing one or more WKT strings.
#' @name g_query
#' @details
#' `g_is_empty()` tests whether a geometry has no points. Returns a logical
#' vector of the same length as the number of input geometries containing
#' `TRUE` for the corresponding geometries that are empty or `FALSE` for
#' non-empty geometries.
#'
#' `g_is_valid()` tests whether a geometry is valid. Returns a logical vector
#' analogous to the above for `g_is_empty()`.
#'
#' `g_is_3D()` checks whether a geometry has Z coordinates. Returns a logical
#' vector analogous to the above for `g_is_empty()`.
#'
#' `g_is_measured()` checks whether a geometry is measured (has M values).
#' Returns a logical vector analogous to the above for `g_is_empty()`.
#'
#' `g_is_ring()` tests whether a geometry is a ring, `TRUE` if the
#' coordinates of the geometry form a ring by checking length and closure
#' (self-intersection is not checked), otherwise `FALSE`.
#' Returns a logical vector analogous to the above for `g_is_empty()`.
#'
#' `g_name()` returns the WKT type names of the input geometries in a character
#' vector of the same length as the number of input geometries.
#'
#' `g_summary()` returns text summaries of WKB/WKT geometries in a
#' character vector of the same length as the number of input
#' geometries. Requires GDAL >= 3.7.
#'
#' `g_geom_count()` returns the number of elements in a geometry or number of
#' geometries in container. Only geometries of type `Polygon[25D]`,
#' `MultiPoint[25D]`, `MultiLineString[25D]`, `MultiPolygon[25D]` or
#' `GeometryCollection[25D]` may return a valid value. Other geometry types will
#' silently return `0`.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param quiet Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.
#'
#' @seealso
#' [g_make_valid()], [g_set_3D()], [g_set_measured()]
#'
#' @examples
#' g1 <- "POLYGON ((0 0, 10 10, 10 0, 0 0))"
#' g2 <- "POLYGON ((5 1, 9 5, 9 1, 5 1))"
#' g_difference(g2, g1) |> g_is_empty()
#'
#' g1 <- "POLYGON ((0 0, 10 10, 10 0, 0 0))"
#' g2 <- "POLYGON ((0 0, 10 10, 10 0))"
#' g3 <- "POLYGON ((0 0, 10 10, 10 0, 0 1))"
#' g_is_valid(c(g1, g2, g3))
#'
#' g_is_3D(g1)
#' g_is_measured(g1)
#'
#' pt_xyz <- g_create("POINT", c(1, 9, 100))
#' g_is_3D(pt_xyz)
#' g_is_measured(pt_xyz)
#'
#' pt_xyzm <- g_create("POINT", c(1, 9, 100, 2000))
#' g_is_3D(pt_xyzm)
#' g_is_measured(pt_xyzm)
#'
#' f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
#' lyr <- new(GDALVector, f, "mtbs_perims")
#'
#' feat <- lyr$getNextFeature()
#' g_name(feat$geom)
#'
#' # g_summary() requires GDAL >= 3.7
#' if (gdal_version_num() >= gdal_compute_version(3, 7, 0)) {
#'   feat <- lyr$getNextFeature()
#'   g_summary(feat$geom) |> print()
#'
#'   feat_set <- lyr$fetch(5)
#'   g_summary(feat_set$geom) |> print()
#' }
#'
#' g_geom_count(feat$geom)
#'
#' lyr$close()
#' @export
g_is_empty <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- NULL
    if (.is_raw_or_null(geom)) {
        ret <- .g_is_empty(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
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

#' @name g_query
#' @export
g_is_valid <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- NULL
    if (.is_raw_or_null(geom)) {
        ret <- .g_is_valid(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
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

#' @name g_query
#' @export
g_is_3D <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- NULL
    if (.is_raw_or_null(geom)) {
        ret <- .g_is_3D(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- sapply(geom, .g_is_3D, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_is_3D(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_is_3D, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_query
#' @export
g_is_measured <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- NULL
    if (.is_raw_or_null(geom)) {
        ret <- .g_is_measured(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- sapply(geom, .g_is_measured, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_is_measured(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_is_measured, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_query
#' @export
g_is_ring <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- NULL
    if (.is_raw_or_null(geom)) {
        ret <- .g_is_ring(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- sapply(geom, .g_is_ring, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_is_ring(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_is_ring, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_query
#' @export
g_name <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- NULL
    if (.is_raw_or_null(geom)) {
        ret <- .g_name(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
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

#' @name g_query
#' @export
g_summary <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- NULL
    if (.is_raw_or_null(geom)) {
        ret <- .g_summary(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
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

#' @name g_query
#' @export
g_geom_count <- function(geom, quiet = FALSE) {
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- NULL
    if (.is_raw_or_null(geom)) {
        ret <- .g_geom_count(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- sapply(geom, .g_geom_count, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_geom_count(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_geom_count, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' Geometry utility functions operating on WKB or WKT
#'
#' These functions operate on input geometries in OGC WKB or WKT format to
#' perform various manipulations for utility purposes.
#' @name g_util
#' @details
#' These functions use the GEOS library via GDAL headers.
#'
#' `g_make_valid()` attempts to make an invalid geometry valid without losing
#' vertices. Already-valid geometries are cloned without further intervention.
#' Wrapper of `OGR_G_MakeValid()`/`OGR_G_MakeValidEx()` in the GDAL API.
#' Requires the GEOS >= 3.8 library, check it for the definition of the
#' geometry operation. If GDAL is built without GEOS >= 3.8, this function
#' will return a clone of the input geometry if it is valid, or `NULL`
#' (`as_wkb = TRUE`) / `NA` (`as_wkb = FALSE`) if it is invalid.
#'
#' * `"LINEWORK"` is the default method, which combines all rings into a set
#' of noded lines and then extracts valid polygons from that linework
#' (requires GEOS >= 3.10 and GDAL >= 3.4). The `"STRUCTURE"` method first
#' makes all rings valid, then merges shells and subtracts holes from shells to
#' generate a valid result. Assumes that holes and shells are correctly
#' categorized.
#'
#' * `keep_collapsed` only applies to the `"STRUCTURE"` method:
#'   * `FALSE` (the default): collapses are converted to empty geometries
#'   * `TRUE`: collapses are converted to a valid geometry of lower dimension
#'
#' `g_normalize()` organizes the elements, rings, and coordinate order of
#' geometries in a consistent way, so that geometries that represent the same
#' object can be easily compared. Wrapper of `OGR_G_Normalize()` in the GDAL
#' API. Requires GDAL >= 3.3. Normalization ensures the following:
#'
#' * Lines are oriented to have smallest coordinate first (apart from duplicate
#' endpoints)
#' * Rings start with their smallest coordinate (using XY ordering)
#' * Polygon shell rings are oriented clockwise, and holes counter-clockwise
#' * Collection elements are sorted by their first coordinate
#'
#' `g_set_3D()` adds or removes the explicit Z coordinate dimension. Removing
#' the Z coordinate dimension of a geometry will remove any existing Z values.
#' Adding the Z dimension to a geometry collection, a compound curve, a
#' polygon, etc. will affect the children geometries.
#' Wrapper of `OGR_G_Set3D()` in the GDAL API.
#'
#' `g_set_measured()` adds or removes the explicit M coordinate dimension.
#' Removing the M coordinate dimension of a geometry will remove any existing M
#' values. Adding the M dimension to a geometry collection, a compound curve, a
#' polygon, etc. will affect the children geometries.
#' Wrapper of `OGR_G_SetMeasured()` in the GDAL API.
#'
#' `g_swap_xy()` swaps x and y coordinates of the input geometry.
#' Wrapper of `OGR_G_SwapXY()` in the GDAL API.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param method Character string. One of `"LINEWORK"` (the default) or
#' `"STRUCTURE"` (requires GEOS >= 3.10 and GDAL >= 3.4). See Details.
#' @param keep_collapsed Logical value, applies only to the STRUCTURE method.
#' Defaults to `FALSE`. See Details.
#' @param is_3d Logical value, `TRUE` if the input geometries should have a Z
#' dimension, or `FALSE` to remove the Z dimension.
#' @param is_measured Logical value, `TRUE` if the input geometries should have
#' a M dimension, or `FALSE` to remove the M dimension.
#' @param as_wkb Logical value, `TRUE` to return the output geometry in WKB
#' format (the default), or `FALSE` to return as WKT.
#' @param as_iso Logical value, `TRUE` to export as ISO WKB/WKT (ISO 13249
#' SQL/MM Part 3), or `FALSE` (the default) to export as "Extended WKB/WKT".
#' @param byte_order Character string specifying the byte order when output is
#' WKB. One of `"LSB"` (the default) or `"MSB"` (uncommon).
#' @param quiet Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return
#' A geometry as WKB raw vector or WKT string, or a list/character vector of
#' geometries as WKB/WKT with length equal to `length(geom)`. `NULL` is returned
#' with a warning if WKB input cannot be converted into an OGR geometry object,
#' or if an error occurs in the call to the underlying OGR API.
#'
#' @seealso
#' [g_is_valid()], [g_is_3D()], [g_is_measured()]
#'
#' @examples
#' ## g_make_valid() requires GEOS >= 3.8, otherwise is only a validity test
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
#' g_make_valid(wkt)  # NULL
#'
#' ## g_normalize() requires GDAL >= 3.3
#' if (gdal_version_num() >= gdal_compute_version(3, 3, 0)) {
#'   g <- "POLYGON ((0 1,1 1,1 0,0 0,0 1))"
#'   g_normalize(g) |> g_wk2wk()
#' }
#'
#' ## set 3D / set measured
#' pt_xyzm <- g_create("POINT", c(1, 9, 100, 2000))
#'
#' g_wk2wk(pt_xyzm, as_iso = TRUE)
#'
#' g_set_3D(pt_xyzm, is_3d = FALSE) |> g_wk2wk(as_iso = TRUE)
#'
#' g_set_measured(pt_xyzm, is_measured = FALSE) |> g_wk2wk(as_iso = TRUE)
#'
#' ## swap XY
#' g <- "GEOMETRYCOLLECTION(POINT(1 2),
#'                          LINESTRING(1 2,2 3),
#'                          POLYGON((0 0,0 1,1 1,0 0)))"
#'
#' g_swap_xy(g, as_wkb = FALSE)
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
        stop("'keep_collapsed' must be a single logical value", call. = FALSE)
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_make_valid(geom, method, keep_collapsed, as_iso,
                             byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
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

#' @name g_util
#' @export
g_normalize <- function(geom, as_wkb = TRUE, as_iso = FALSE, byte_order = "LSB",
                        quiet = FALSE) {

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_normalize(geom, as_iso, byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_normalize, as_iso, byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_normalize(g_wk2wk(geom), as_iso, byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_normalize, as_iso, byte_order, quiet)
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

#' @name g_util
#' @export
g_set_3D <- function(geom, is_3d, as_wkb = TRUE, as_iso = FALSE,
                     byte_order = "LSB", quiet = FALSE) {

    # is_3d
    if (missing(is_3d) || is.null(is_3d) || is.na(is_3d))
        stop("'is_3d' is required", call. = FALSE)
    if (!is.logical(is_3d) || length(is_3d) > 1)
        stop("'is_3d' must be a single logical value", call. = FALSE)
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_set_3D(geom, is_3d, as_iso, byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_set_3D, is_3d, as_iso, byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_set_3D(g_wk2wk(geom), is_3d, as_iso, byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_set_3D, is_3d, as_iso, byte_order,
                          quiet)
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

#' @name g_util
#' @export
g_set_measured <- function(geom, is_measured, as_wkb = TRUE, as_iso = FALSE,
                           byte_order = "LSB", quiet = FALSE) {

    # is_measured
    if (missing(is_measured) || is.null(is_measured) || is.na(is_measured))
        stop("'is_measured' is required", call. = FALSE)
    if (!is.logical(is_measured) || length(is_measured) > 1)
        stop("'is_measured' must be a single logical value", call. = FALSE)
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_set_measured(geom, is_measured, as_iso, byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_set_measured, is_measured, as_iso, byte_order,
                      quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_set_measured(g_wk2wk(geom), is_measured, as_iso,
                                   byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_set_measured, is_measured, as_iso,
                          byte_order, quiet)
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

#' @name g_util
#' @export
g_swap_xy <- function(geom, as_wkb = TRUE, as_iso = FALSE, byte_order = "LSB",
                      quiet = FALSE) {

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_swap_xy(geom, as_iso, byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_swap_xy, as_iso, byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_swap_xy(g_wk2wk(geom), as_iso, byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_swap_xy, as_iso, byte_order, quiet)
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

#' Obtain the 2D or 3D bounding envelope for input geometries
#'
#' `g_envelope()` computes and returns the bounding envelope(s) for the input
#' geometries. Wrapper of `OGR_G_GetEnvelope()` / `OGR_G_GetEnvelope3D()` in
#' the GDAL Geometry API.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param as_3d Logical value. `TRUE` to return the 3D bounding envelope.
#' The 2D envelope is returned by default (`as_3d = FALSE`).
#' @param quiet Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return Either a numeric vector of length `4` containing the 2D envelope
#' `(xmin, xmax, ymin, ymax)` or of length `6` containing the 3D envelope
#' `(xmin, xmax, ymin, ymax, zmin, zmax)`, or a four-column or six-column
#' numeric matrix with number of rows equal to the number of input geometries
#' and column names `("xmin", "xmax", "ymin", "ymax")`, or
#' `("xmin", "xmax", "ymin", "ymax", "zmin", "zmax")` for the 3D case.
#' @export
g_envelope <- function(geom, as_3d = FALSE, quiet = FALSE) {
    # as_3d
    if (is.null(as_3d))
        as_3d <- FALSE
    if (!is.logical(as_3d) || length(as_3d) > 1)
        stop("'as_3d' must be a single logical value", call. = FALSE)
    # quiet
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- 0
    if (.is_raw_or_null(geom)) {
        ret <- .g_envelope(geom, as_3d, quiet)
        if (as_3d)
            names(ret) <- c("xmin", "xmax", "ymin", "ymax", "zmin", "zmax")
        else
            names(ret) <- c("xmin", "xmax", "ymin", "ymax")
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- t(sapply(geom, .g_envelope, as_3d, quiet))
        if (as_3d)
            colnames(ret) <- c("xmin", "xmax", "ymin", "ymax", "zmin", "zmax")
        else
            colnames(ret) <- c("xmin", "xmax", "ymin", "ymax")
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_envelope(g_wk2wk(geom), as_3d, quiet)
        if (as_3d)
            names(ret) <- c("xmin", "xmax", "ymin", "ymax", "zmin", "zmax")
        else
            names(ret) <- c("xmin", "xmax", "ymin", "ymax")
        } else {
            ret <- t(sapply(g_wk2wk(geom), .g_envelope, as_3d, quiet))
        if (as_3d)
            colnames(ret) <- c("xmin", "xmax", "ymin", "ymax", "zmin", "zmax")
        else
            colnames(ret) <- c("xmin", "xmax", "ymin", "ymax")
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' Geometry binary predicates operating on WKB or WKT
#'
#' These functions implement tests for pairs of geometries in OGC WKB or
#' WKT format.
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
#' @param this_geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param other_geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings. Must contain the same
#' number of geometries as `this_geom`, unless `this_geom` contains a single
#' geometry in which case pairwise tests will be performed for one-to-many if
#' `other_geom` contains multiple geometries (i.e., "this-to-others").
#' @param quiet Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return Logical vector with length equal to the number of input geometry
#' pairs.
#'
#' @seealso
#' \url{https://en.wikipedia.org/wiki/DE-9IM}
#'
#' @note
#' `this_geom` and `other_geom` are assumed to be in the same coordinate
#' reference system.
#'
#' If `this_geom`is a single geometry and `other_geom` is a list or vector of
#' multiple geometries, then `this_geom` will be tested against each geometry
#' in `other_geom` (otherwise no recycling is done).
#'
#' Geometry validity is not checked. In case you are unsure of the validity
#' of the input geometries, call `g_is_valid()` before, otherwise the result
#' might be wrong.
#' @export
g_intersects <- function(this_geom, other_geom, quiet = FALSE) {
    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    if (is.list(this_geom) && length(this_geom) == 1)
        this_geom <- this_geom[[1]]
    if (is.list(other_geom) && length(other_geom) == 1)
        other_geom <- other_geom[[1]]

    ret <- NULL
    one_to_many <- FALSE
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        ret <- .g_intersects(this_geom, other_geom, quiet)
    } else if ((.is_raw_or_null(this_geom) || is.list(this_geom)) &&
               is.list(other_geom)) {

        if (.is_raw_or_null(this_geom) && is.list(other_geom)) {
            one_to_many <- TRUE

        } else if (is.list(this_geom) &&
                   length(this_geom) != length(other_geom)) {

            stop("many-to-many input must contain equal numbers of geometries",
                 call. = FALSE)
        }

        ret <- rep(NA, length(other_geom))
        for (i in seq_along(other_geom)) {
            if (one_to_many)
                ret[i] <- .g_intersects(this_geom, other_geom[[i]], quiet)
            else
                ret[i] <- .g_intersects(this_geom[[i]], other_geom[[i]], quiet)
        }

    } else {
        stop("inputs must contain equal number of geometries, or one-to-many",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_binary_pred
#' @export
g_disjoint <- function(this_geom, other_geom, quiet = FALSE) {
    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    if (is.list(this_geom) && length(this_geom) == 1)
        this_geom <- this_geom[[1]]
    if (is.list(other_geom) && length(other_geom) == 1)
        other_geom <- other_geom[[1]]

    ret <- NULL
    one_to_many <- FALSE
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        ret <- .g_disjoint(this_geom, other_geom, quiet)
    } else if ((.is_raw_or_null(this_geom) || is.list(this_geom)) &&
               is.list(other_geom)) {

        if (.is_raw_or_null(this_geom) && is.list(other_geom)) {
            one_to_many <- TRUE

        } else if (is.list(this_geom) &&
                   length(this_geom) != length(other_geom)) {

            stop("many-to-many input must contain equal numbers of geometries",
                 call. = FALSE)
        }

        ret <- rep(NA, length(other_geom))
        for (i in seq_along(other_geom)) {
            if (one_to_many)
                ret[i] <- .g_disjoint(this_geom, other_geom[[i]], quiet)
            else
                ret[i] <- .g_disjoint(this_geom[[i]], other_geom[[i]], quiet)
        }

    } else {
        stop("inputs must contain equal number of geometries, or one-to-many",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_binary_pred
#' @export
g_touches <- function(this_geom, other_geom, quiet = FALSE) {
    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    if (is.list(this_geom) && length(this_geom) == 1)
        this_geom <- this_geom[[1]]
    if (is.list(other_geom) && length(other_geom) == 1)
        other_geom <- other_geom[[1]]

    ret <- NULL
    one_to_many <- FALSE
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        ret <- .g_touches(this_geom, other_geom, quiet)
    } else if ((.is_raw_or_null(this_geom) || is.list(this_geom)) &&
               is.list(other_geom)) {

        if (.is_raw_or_null(this_geom) && is.list(other_geom)) {
            one_to_many <- TRUE

        } else if (is.list(this_geom) &&
                   length(this_geom) != length(other_geom)) {

            stop("many-to-many input must contain equal numbers of geometries",
                 call. = FALSE)
        }

        ret <- rep(NA, length(other_geom))
        for (i in seq_along(other_geom)) {
            if (one_to_many)
                ret[i] <- .g_touches(this_geom, other_geom[[i]], quiet)
            else
                ret[i] <- .g_touches(this_geom[[i]], other_geom[[i]], quiet)
        }

    } else {
        stop("inputs must contain equal number of geometries, or one-to-many",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_binary_pred
#' @export
g_contains <- function(this_geom, other_geom, quiet = FALSE) {
    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    if (is.list(this_geom) && length(this_geom) == 1)
        this_geom <- this_geom[[1]]
    if (is.list(other_geom) && length(other_geom) == 1)
        other_geom <- other_geom[[1]]

    ret <- NULL
    one_to_many <- FALSE
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        ret <- .g_contains(this_geom, other_geom, quiet)
    } else if ((.is_raw_or_null(this_geom) || is.list(this_geom)) &&
               is.list(other_geom)) {

        if (.is_raw_or_null(this_geom) && is.list(other_geom)) {
            one_to_many <- TRUE

        } else if (is.list(this_geom) &&
                   length(this_geom) != length(other_geom)) {

            stop("many-to-many input must contain equal numbers of geometries",
                 call. = FALSE)
        }

        ret <- rep(NA, length(other_geom))
        for (i in seq_along(other_geom)) {
            if (one_to_many)
                ret[i] <- .g_contains(this_geom, other_geom[[i]], quiet)
            else
                ret[i] <- .g_contains(this_geom[[i]], other_geom[[i]], quiet)
        }

    } else {
        stop("inputs must contain equal number of geometries, or one-to-many",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_binary_pred
#' @export
g_within <- function(this_geom, other_geom, quiet = FALSE) {
    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    if (is.list(this_geom) && length(this_geom) == 1)
        this_geom <- this_geom[[1]]
    if (is.list(other_geom) && length(other_geom) == 1)
        other_geom <- other_geom[[1]]

    ret <- NULL
    one_to_many <- FALSE
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        ret <- .g_within(this_geom, other_geom, quiet)
    } else if ((.is_raw_or_null(this_geom) || is.list(this_geom)) &&
               is.list(other_geom)) {

        if (.is_raw_or_null(this_geom) && is.list(other_geom)) {
            one_to_many <- TRUE

        } else if (is.list(this_geom) &&
                   length(this_geom) != length(other_geom)) {

            stop("many-to-many input must contain equal numbers of geometries",
                 call. = FALSE)
        }

        ret <- rep(NA, length(other_geom))
        for (i in seq_along(other_geom)) {
            if (one_to_many)
                ret[i] <- .g_within(this_geom, other_geom[[i]], quiet)
            else
                ret[i] <- .g_within(this_geom[[i]], other_geom[[i]], quiet)
        }

    } else {
        stop("inputs must contain equal number of geometries, or one-to-many",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_binary_pred
#' @export
g_crosses <- function(this_geom, other_geom, quiet = FALSE) {
    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    if (is.list(this_geom) && length(this_geom) == 1)
        this_geom <- this_geom[[1]]
    if (is.list(other_geom) && length(other_geom) == 1)
        other_geom <- other_geom[[1]]

    ret <- NULL
    one_to_many <- FALSE
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        ret <- .g_crosses(this_geom, other_geom, quiet)
    } else if ((.is_raw_or_null(this_geom) || is.list(this_geom)) &&
               is.list(other_geom)) {

        if (.is_raw_or_null(this_geom) && is.list(other_geom)) {
            one_to_many <- TRUE

        } else if (is.list(this_geom) &&
                   length(this_geom) != length(other_geom)) {

            stop("many-to-many input must contain equal numbers of geometries",
                 call. = FALSE)
        }

        ret <- rep(NA, length(other_geom))
        for (i in seq_along(other_geom)) {
            if (one_to_many)
                ret[i] <- .g_crosses(this_geom, other_geom[[i]], quiet)
            else
                ret[i] <- .g_crosses(this_geom[[i]], other_geom[[i]], quiet)
        }

    } else {
        stop("inputs must contain equal number of geometries, or one-to-many",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_binary_pred
#' @export
g_overlaps <- function(this_geom, other_geom, quiet = FALSE) {
    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    if (is.list(this_geom) && length(this_geom) == 1)
        this_geom <- this_geom[[1]]
    if (is.list(other_geom) && length(other_geom) == 1)
        other_geom <- other_geom[[1]]

    ret <- NULL
    one_to_many <- FALSE
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        ret <- .g_overlaps(this_geom, other_geom, quiet)
    } else if ((.is_raw_or_null(this_geom) || is.list(this_geom)) &&
               is.list(other_geom)) {

        if (.is_raw_or_null(this_geom) && is.list(other_geom)) {
            one_to_many <- TRUE

        } else if (is.list(this_geom) &&
                   length(this_geom) != length(other_geom)) {

            stop("many-to-many input must contain equal numbers of geometries",
                 call. = FALSE)
        }

        ret <- rep(NA, length(other_geom))
        for (i in seq_along(other_geom)) {
            if (one_to_many)
                ret[i] <- .g_overlaps(this_geom, other_geom[[i]], quiet)
            else
                ret[i] <- .g_overlaps(this_geom[[i]], other_geom[[i]], quiet)
        }

    } else {
        stop("inputs must contain equal number of geometries, or one-to-many",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_binary_pred
#' @export
g_equals <- function(this_geom, other_geom, quiet = FALSE) {
    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    if (is.list(this_geom) && length(this_geom) == 1)
        this_geom <- this_geom[[1]]
    if (is.list(other_geom) && length(other_geom) == 1)
        other_geom <- other_geom[[1]]

    ret <- NULL
    one_to_many <- FALSE
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        ret <- .g_equals(this_geom, other_geom, quiet)
    } else if ((.is_raw_or_null(this_geom) || is.list(this_geom)) &&
               is.list(other_geom)) {

        if (.is_raw_or_null(this_geom) && is.list(other_geom)) {
            one_to_many <- TRUE

        } else if (is.list(this_geom) &&
                   length(this_geom) != length(other_geom)) {

            stop("many-to-many input must contain equal numbers of geometries",
                 call. = FALSE)
        }

        ret <- rep(NA, length(other_geom))
        for (i in seq_along(other_geom)) {
            if (one_to_many)
                ret[i] <- .g_equals(this_geom, other_geom[[i]], quiet)
            else
                ret[i] <- .g_equals(this_geom[[i]], other_geom[[i]], quiet)
        }

    } else {
        stop("inputs must contain an equal number of geometries, or one-to-many",
             call. = FALSE)
    }

    return(ret)
}

#' Binary operations on WKB or WKT geometries
#'
#' These functions implement operations on pairs of geometries in OGC WKB
#' or WKT format.
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
#' @param this_geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param other_geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings. Must contain the same
#' number of geometries as `this_geom`.
#' @param as_wkb Logical value, `TRUE` to return the output geometry in WKB
#' format (the default), or `FALSE` to return as WKT.
#' @param as_iso Logical value, `TRUE` to export as ISO WKB/WKT (ISO 13249
#' SQL/MM Part 3), or `FALSE` (the default) to export as "Extended WKB/WKT".
#' @param byte_order Character string specifying the byte order when output is
#' WKB. One of `"LSB"` (the default) or `"MSB"` (uncommon).
#' @param quiet Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return
#' A geometry as WKB raw vector or WKT string, or a list/character vector of
#' geometries as WKB/WKT with length equal to the number of input geometry
#' pairs.
#' `NULL` (`as_wkb = TRUE`) / `NA` (`as_wkb = FALSE`) is returned with a
#' warning if WKB input cannot be converted into an OGR geometry object, or if
#' an error occurs in the call to the underlying OGR API function.
#'
#' @note
#' `this_geom` and `other_geom` are assumed to be in the same coordinate
#' reference system.
#'
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
g_intersection <- function(this_geom, other_geom, as_wkb = TRUE,
                           as_iso = FALSE, byte_order = "LSB",
                           quiet = FALSE) {

    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        wkb <- .g_intersection(this_geom, other_geom, as_iso,
                               byte_order, quiet)
    } else if (is.list(this_geom) && is.list(other_geom)) {
        if (length(this_geom) != length(other_geom)) {
            stop("inputs must contain an equal number of geometries",
                 call. = FALSE)
        }

        wkb <- list()
        for (i in seq_along(this_geom)) {
            wkb[[i]] <- .g_intersection(this_geom[[i]], other_geom[[i]], as_iso,
                                        byte_order, quiet)
        }

    } else {
        stop("inputs must contain an equal number of geometries",
             call. = FALSE)
    }

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))
}

#' @name g_binary_op
#' @export
g_union <- function(this_geom, other_geom, as_wkb = TRUE,
                    as_iso = FALSE, byte_order = "LSB",
                    quiet = FALSE) {

    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        wkb <- .g_union(this_geom, other_geom, as_iso,
                        byte_order, quiet)
    } else if (is.list(this_geom) && is.list(other_geom)) {
        if (length(this_geom) != length(other_geom)) {
            stop("inputs must contain an equal number of geometries",
                 call. = FALSE)
        }

        wkb <- list()
        for (i in seq_along(this_geom)) {
            wkb[[i]] <- .g_union(this_geom[[i]], other_geom[[i]], as_iso,
                                 byte_order, quiet)
        }

    } else {
        stop("inputs must contain an equal number of geometries",
             call. = FALSE)
    }

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))
}

#' @name g_binary_op
#' @export
g_difference <- function(this_geom, other_geom, as_wkb = TRUE,
                         as_iso = FALSE, byte_order = "LSB",
                         quiet = FALSE) {

    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        wkb <- .g_difference(this_geom, other_geom, as_iso,
                             byte_order, quiet)
    } else if (is.list(this_geom) && is.list(other_geom)) {
        if (length(this_geom) != length(other_geom)) {
            stop("inputs must contain an equal number of geometries",
                 call. = FALSE)
        }

        wkb <- list()
        for (i in seq_along(this_geom)) {
            wkb[[i]] <- .g_difference(this_geom[[i]], other_geom[[i]], as_iso,
                                      byte_order, quiet)
        }

    } else {
        stop("inputs must contain an equal number of geometries",
             call. = FALSE)
    }

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))
}

#' @name g_binary_op
#' @export
g_sym_difference <- function(this_geom, other_geom, as_wkb = TRUE,
                         as_iso = FALSE, byte_order = "LSB",
                         quiet = FALSE) {

    if (is.character(this_geom))
        this_geom <- g_wk2wk(this_geom)
    if (!(.is_raw_or_null(this_geom) || (is.list(this_geom) &&
                                         .is_raw_or_null(this_geom[[1]])))) {

        stop("'this_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(this_geom) && .is_raw_or_null(other_geom)) {
        wkb <- .g_sym_difference(this_geom, other_geom, as_iso,
                                 byte_order, quiet)
    } else if (is.list(this_geom) && is.list(other_geom)) {
        if (length(this_geom) != length(other_geom)) {
            stop("inputs must contain an equal number of geometries",
                 call. = FALSE)
        }

        wkb <- list()
        for (i in seq_along(this_geom)) {
            wkb[[i]] <- .g_sym_difference(this_geom[[i]], other_geom[[i]],
                                          as_iso, byte_order, quiet)
        }

    } else {
        stop("inputs must contain an equal number of geometries",
             call. = FALSE)
    }

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))
}

#' Compute measurements for WKB/WKT geometries
#'
#' These functions compute measurements for geometries. The input
#' geometries may be given as a single raw vector of WKB, a list of WKB raw
#' vectors, or a character vector containing one or more WKT strings.
#' @name g_measures
#' @details
#' These functions use the GEOS library via GDAL headers.
#'
#' `g_area()` computes the area for a `Polygon` or `MultiPolygon`. Undefined
#' for all other geometry types (returns zero). Returns a numeric vector,
#' having length equal to the number of input geometries, containing
#' computed area or '0' if undefined.
#'
#' `g_centroid()` returns a numeric vector of length 2 containing the centroid
#' (X, Y), or a two-column numeric matrix (X, Y) with number of rows equal to
#' the number of input geometries.
#' The GDAL documentation states "This method relates to the SFCOM
#' `ISurface::get_Centroid()` method however the current implementation based
#' on GEOS can operate on other geometry types such as multipoint, linestring,
#' geometrycollection such as multipolygons. OGC SF SQL 1.1 defines the
#' operation for surfaces (polygons). SQL/MM-Part 3 defines the operation for
#' surfaces and multisurfaces (multipolygons)."
#'
#' `g_distance()` returns the distance between two geometries or `-1` if an
#' error occurs. Returns the shortest distance between the two geometries.
#' The distance is expressed into the same unit as the coordinates of the
#' geometries. Returns a numeric vector, having length equal to the number of
#' input geometry pairs, containing computed distance or '-1' if an error
#' occurs.
#'
#' `g_length()` computes the length for `LineString` or `MultiCurve` objects.
#' Undefined for all other geometry types (returns zero). Returns a numeric
#' vector, having length equal to the number of input geometries, containing
#' computed length or '0' if undefined.
#'
#' `g_geodesic_area()` computes geometry area, considered as a surface on the
#' underlying ellipsoid of the SRS attached to the geometry. The returned area
#' will always be in square meters, and assumes that polygon edges describe
#' geodesic lines on the ellipsoid. If the geometry SRS is not a geographic
#' one, geometries are reprojected to the underlying geographic SRS.
#' By default, input geometry vertices are assumed to be in longitude/latitude
#' order if using a geographic coordinate system. This can be overridden with
#' the `traditional_gis_order` argument.
#' Returns the area in square meters, or `NA` in case of error (unsupported
#' geometry type, no SRS attached, etc.)
#' Requires GDAL >= 3.9.
#'
#' `g_geodesic_length()` computes the length of the curve, considered as a
#' geodesic line on the underlying ellipsoid of the SRS attached to the
#' geometry. The returned length will always be in meters. If the geometry SRS
#' is not a geographic one, geometries are reprojected to the underlying
#' geographic SRS.
#' By default, input geometry vertices are assumed to be in longitude/latitude
#' order if using a geographic coordinate system. This can be overridden with
#' the `traditional_gis_order` argument.
#' Returns the length in meters, or `NA` in case of error (unsupported geometry
#' type, no SRS attached, etc.)
#' Requires GDAL >= 3.10.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param other_geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings. Must contain the same
#' number of geometries as `geom`, unless `geom` contains a single
#' geometry in which case pairwise distances will be computed for one-to-many
#' if `other_geom` contains multiple geometries (i.e., "this-to-others").
#' @param srs Character string specifying the spatial reference system
#' for `geom`. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param traditional_gis_order Logical value, `TRUE` to use traditional GIS
#' order of axis mapping (the default) or `FALSE` to use authority compliant
#' axis order. By default, input `geom` vertices are assumed to
#' be in longitude/latitude order if `srs` is a geographic coordinate system.
#' This can be overridden by setting `traditional_gis_order = FALSE`.
#' @param quiet Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.
#'
#' @note
#' For `g_distance()`, `geom` and `other_geom` must be in the same coordinate
#' reference system. If `geom` is a single geometry and `other_geom` is a list
#' or vector of multiple geometries, then distances will be calculated between
#' `geom` and each geometry in `other_geom`. Otherwise, no recycling is done
#' and `length(geom)` must equal `length(other_geom)` to calculate distance
#' between each corresponding pair of input geometries.
#'
#' Geometry validity is not checked. In case you are unsure of the validity
#' of the input geometries, call `g_is_valid()` before, otherwise the result
#' might be wrong.
#'
#' @examples
#' g_area("POLYGON ((0 0, 10 10, 10 0, 0 0))")
#'
#' g_centroid("POLYGON ((0 0, 10 10, 10 0, 0 0))")
#'
#' g_distance("POINT (0 0)", "POINT (5 12)")
#'
#' g_length("LINESTRING (0 0, 3 4)")
#'
#' f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
#' lyr <- new(GDALVector, f, "mtbs_perims")
#'
#' # read all features into a data frame
#' feat_set <- lyr$fetch(-1)
#' head(feat_set)
#'
#' g_area(feat_set$geom) |> head()
#'
#' g_centroid(feat_set$geom) |> head()
#'
#' lyr$close()
#' @export
g_area <- function(geom, quiet = FALSE) {
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- 0
    if (.is_raw_or_null(geom)) {
        ret <- .g_area(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- sapply(geom, .g_area, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_area(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_area, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_measures
#' @export
g_centroid <- function(geom, quiet = FALSE) {
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- 0
    if (.is_raw_or_null(geom)) {
        ret <- .g_centroid(geom, quiet)
        names(ret) <- c("x", "y")
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- t(sapply(geom, .g_centroid, quiet))
        colnames(ret) <- c("x", "y")
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_centroid(g_wk2wk(geom), quiet)
            names(ret) <- c("x", "y")
        } else {
            ret <- t(sapply(g_wk2wk(geom), .g_centroid, quiet))
            colnames(ret) <- c("x", "y")
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_measures
#' @export
g_distance <- function(geom, other_geom, quiet = FALSE) {
    if (is.character(geom))
        geom <- g_wk2wk(geom)
    if (!(.is_raw_or_null(geom) || (is.list(geom) &&
                                    .is_raw_or_null(geom[[1]])))) {

        stop("'geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.character(other_geom))
        other_geom <- g_wk2wk(other_geom)
    if (!(.is_raw_or_null(other_geom) || (is.list(other_geom) &&
                                          .is_raw_or_null(other_geom[[1]])))) {

        stop("'other_geom' must be raw vector or character",
             call. = FALSE)
    }

    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    if (is.list(geom) && length(geom) == 1)
        geom <- geom[[1]]
    if (is.list(other_geom) && length(other_geom) == 1)
        other_geom <- other_geom[[1]]

    ret <- -1
    one_to_many <- FALSE
    if (.is_raw_or_null(geom) && .is_raw_or_null(other_geom)) {
        ret <- .g_distance(geom, other_geom, quiet)
    } else if ((.is_raw_or_null(geom) || is.list(geom)) &&
               is.list(other_geom)) {

        if (.is_raw_or_null(geom) && is.list(other_geom)) {
            one_to_many <- TRUE

        } else if (is.list(geom) &&
                   length(geom) != length(other_geom)) {

            stop("many-to-many input must contain equal numbers of geometries",
                 call. = FALSE)
        }

        ret <- rep(-1, length(other_geom))
        for (i in seq_along(other_geom)) {
            if (one_to_many)
                ret[i] <- .g_distance(geom, other_geom[[i]], quiet)
            else
                ret[i] <- .g_distance(geom[[i]], other_geom[[i]], quiet)
        }

    } else {
        stop("inputs must contain equal number of geometries, or one-to-many",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_measures
#' @export
g_length <- function(geom, quiet = FALSE) {
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- 0
    if (.is_raw_or_null(geom)) {
        ret <- .g_length(geom, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- sapply(geom, .g_length, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_length(g_wk2wk(geom), quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_length, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_measures
#' @export
g_geodesic_area <- function(geom, srs, traditional_gis_order = TRUE,
                            quiet = FALSE) {

    if (!(is.character(srs) && length(srs) == 1))
        stop("'srs' must be a character string", call. = FALSE)
    if (is.null(traditional_gis_order))
        traditional_gis_order <- TRUE
    if (!(is.logical(traditional_gis_order) &&
          length(traditional_gis_order) == 1)) {

        stop("'traditional_gis_order' must be a single logical value",
             call. = FALSE)
    }
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- -1.0
    if (.is_raw_or_null(geom)) {
        ret <- .g_geodesic_area(geom, srs, traditional_gis_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- sapply(geom, .g_geodesic_area, srs, traditional_gis_order,
                      quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_geodesic_area(g_wk2wk(geom), srs, traditional_gis_order,
                                    quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_geodesic_area, srs,
                          traditional_gis_order, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' @name g_measures
#' @export
g_geodesic_length <- function(geom, srs, traditional_gis_order = TRUE,
                              quiet = FALSE) {

    if (!(is.character(srs) && length(srs) == 1))
        stop("'srs' must be a character string", call. = FALSE)
    if (is.null(traditional_gis_order))
        traditional_gis_order <- TRUE
    if (!(is.logical(traditional_gis_order) &&
          length(traditional_gis_order) == 1)) {

        stop("'traditional_gis_order' must be a single logical value",
             call. = FALSE)
    }
    if (is.null(quiet))
        quiet <- FALSE
    if (!is.logical(quiet) || length(quiet) > 1)
        stop("'quiet' must be a single logical value", call. = FALSE)

    ret <- -1.0
    if (.is_raw_or_null(geom)) {
        ret <- .g_geodesic_length(geom, srs, traditional_gis_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        ret <- sapply(geom, .g_geodesic_length, srs, traditional_gis_order,
                      quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            ret <- .g_geodesic_length(g_wk2wk(geom), srs, traditional_gis_order,
                                      quiet)
        } else {
            ret <- sapply(g_wk2wk(geom), .g_geodesic_length, srs,
                          traditional_gis_order, quiet)
        }
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(ret)
}

#' Unary operations on WKB or WKT geometries
#'
#' These functions implement algorithms that operate on one input geometry
#' for which a new output geometry is generated.
#' The input geometries may be given as a single raw vector of WKB, a list of
#' WKB raw vectors, or a character vector containing one or more WKT strings.
#' @name g_unary_op
#' @details
#' These functions use the GEOS library via GDAL headers.
#'
#' `g_boundary()` computes the boundary of a geometry. Wrapper of
#' `OGR_G_Boundary()` in the GDAL Geometry API.
#'
#' `g_buffer()` builds a new geometry containing the buffer region around
#' the geometry on which it is invoked. The buffer is a polygon containing
#' the region within the buffer distance of the original geometry.
#' Wrapper of `OGR_G_Buffer()` in the GDAL API.
#'
#' `g_convex_hull()` computes a convex hull, the smallest convex geometry that
#' contains all the points in the input geometry. Wrapper of
#' `OGR_G_ConvexHull()` in the GDAL API.
#'
#' `g_concave_hull()` returns a "concave hull" of a geometry. A concave hull is
#' a polygon which contains all the points of the input, but is a better
#' approximation than the convex hull to the area occupied by the input.
#' Frequently used to convert a multi-point into a polygonal area that contains
#' all the points in the input geometry. Requires GDAL >= 3.6 and GEOS >= 3.11.
#'
#' `g_delaunay_triangulation()`
#' * `constrained = FALSE`: returns a Delaunay triangulation of the vertices of
#' the input geometry. Wrapper of `OGR_G_DelaunayTriangulation()` in the GDAL
#' API. Requires GEOS >= 3.4.
#' * `constrained = TRUE`: returns a constrained Delaunay triangulation of the
#' vertices of the given polygon(s). For non-polygonal inputs, silently returns
#' an empty geometry collection. Requires GDAL >= 3.12 and GEOS >= 3.10.
#'
#' `g_simplify()` computes a simplified geometry. By default, it simplifies
#' the input geometries while preserving topology (see Note). Wrapper of
#' `OGR_G_Simplify()` / `OGR_G_SimplifyPreserveTopology()` in the GDAL API.
#'
#' `g_unary_union()` returns the union of all components of a single geometry.
#' Usually used to convert a collection into the smallest set of polygons that
#' cover the same area. See \url{https://postgis.net/docs/ST_UnaryUnion.html}
#' for more details. Requires GDAL >= 3.7.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param dist Numeric buffer distance in units of the input `geom`.
#' @param quad_segs Integer number of segments used to define a 90 degree
#' curve (quadrant of a circle). Large values result in large numbers of
#' vertices in the resulting buffer geometry while small numbers reduce the
#' accuracy of the result.
#' @param ratio Numeric value in interval `[0, 1]`. The target criterion
#' parameter for `g_concave_hull()`, expressed as a ratio between the lengths
#' of the longest and shortest edges. `1` produces the convex hull; `0` produces
#' a hull with maximum concaveness (see Note).
#' @param allow_holes Logical value, whether holes are allowed.
#' @param constrained Logical value, `TRUE` to return a constrained Delaunay
#' triangulation of the vertices of the given polygon(s). Defaults to `FALSE`.
#' @param tolerance Numeric value. For `g_simplify()`, the simplification
#' tolerance as distance in units of the input `geom`. Simplification removes
#' vertices which are within the tolerance distance of the simplified linework
#' (as long as topology is preserved when `preserve_topology = TRUE`).
#' For `g_delaunay_triangulation()`, an optional snapping tolerance to use for
#' improved robustness (ignored if `constrained = TRUE`).
#' @param only_edges Logical value. If `TRUE`, `g_delaunay_triangulation()`
#' will return a MULTILINESTRING, otherwise it will return a GEOMETRYCOLLECTION
#' containing triangular POLYGONs (the default). Ignored if `constrained = TRUE`
#' @param preserve_topology Logical value, `TRUE` to simplify geometries while
#' preserving topology (the default). Setting to `FALSE` simplifies geometries
#' using the standard Douglas-Peucker algorithm which is significantly faster
#' (see Note).
#' @param as_wkb Logical value, `TRUE` to return the output geometry in WKB
#' format (the default), or `FALSE` to return as WKT.
#' @param as_iso Logical value, `TRUE` to export as ISO WKB/WKT (ISO 13249
#' SQL/MM Part 3), or `FALSE` (the default) to export as "Extended WKB/WKT".
#' @param byte_order Character string specifying the byte order when output is
#' WKB. One of `"LSB"` (the default) or `"MSB"` (uncommon).
#' @param quiet Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return
#' A geometry as WKB raw vector or WKT string, or a list/character vector of
#' geometries as WKB/WKT with length equal to the number of input geometries.
#'  `NULL` (`as_wkb = TRUE`) / `NA` (`as_wkb = FALSE`) is returned with a
#' warning if WKB input cannot be converted into an OGR geometry object, or if
#' an error occurs in the call to the underlying OGR API.
#'
#' @note
#' Definitions of these operations are given in the GEOS documentation
#' (\url{https://libgeos.org/doxygen/}, GEOS 3.15.0dev), some of which is
#' copied here.
#'
#' `g_boundary()` computes the "boundary" as defined by the DE9IM
#' (\url{https://en.wikipedia.org/wiki/DE-9IM}):
#' * the boundary of a `Polygon` is the set of linear rings dividing the
#' exterior from the interior
#' * the boundary of a `LineString` is the two end points
#' * the boundary of a `Point`/`MultiPoint` is defined as empty
#'
#' `g_buffer()` always returns a polygonal result. The negative or
#' zero-distance buffer of lines and points is always an empty `Polygon`.
#'
#' `g_convex_hull()` uses the Graham Scan algorithm.
#'
#' `g_concave_hull()`: A set of points has a sequence of hulls of increasing
#' concaveness, determined by a numeric target parameter. The concave hull is
#' constructed by removing the longest outer edges of the Delaunay Triangulation
#' of the space between the polygons, until the target criterion parameter is
#' reached. This can be expressed as a ratio between the lengths of the longes
#' and shortest edges. `1` produces the convex hull; `0` produces a hull with
#' maximum concaveness.
#'
#' `g_simplify()`:
#' * With `preserve_topology = TRUE` (the default):\cr
#' Simplifies a geometry, ensuring that the result is a valid geometry having
#' the same dimension and number of components as the input. The simplification
#' uses a maximum distance difference algorithm similar to the one used in the
#' Douglas-Peucker algorithm. In particular, if the input is an areal geometry
#' (`Polygon` or `MultiPolygon`), the result has the same number of shells and
#' holes (rings) as the input, in the same order. The result rings touch at no
#' more than the number of touching point in the input (although they may touch
#' at fewer points).
#' * With `preserve_topology = FALSE`:\cr
#' Simplifies a geometry using the standard Douglas-Peucker algorithm. Ensures
#' that any polygonal geometries returned are valid. Simple lines are not
#' guaranteed to remain simple after simplification. Note that in general D-P
#' does not preserve topology - e.g. polygons can be split, collapse to lines
#' or disappear, holes can be created or disappear, and lines can cross. To
#' simplify geometry while preserving topology use
#' `TopologyPreservingSimplifier`. (However, using D-P is significantly faster.)
#'
#' `preserve_topology = TRUE` does not preserve boundaries shared between
#' polygons.
#'
#' @examples
#' g <- "POLYGON((0 0,1 1,1 0,0 0))"
#' g_boundary(g, as_wkb = FALSE)
#'
#' g <- "POINT (0 0)"
#' g_buffer(g, dist = 10, as_wkb = FALSE)
#'
#' g <- "GEOMETRYCOLLECTION(POINT(0 1), POINT(0 0), POINT(1 0), POINT(1 1))"
#' g_convex_hull(g, as_wkb = FALSE)
#'
#' # g_concave_hull() requires GDAL >= 3.6 and GEOS >= 3.11
#' if (gdal_version_num() >= gdal_compute_version(3, 6, 0) &&
#'     (geos_version()$major > 3 || geos_version()$minor >= 11)) {
#'   g <- "MULTIPOINT(0 0,0.4 0.5,0 1,1 1,0.6 0.5,1 0)"
#'   g_concave_hull(g, ratio = 0.5, allow_holes = FALSE, as_wkb = FALSE)
#' }
#'
#' # g_delaunay_triangulation() requires GEOS >= 3.4
#' if (geos_version()$major > 3 || geos_version()$minor >= 4) {
#'   g <- "MULTIPOINT(0 0,0 1,1 1,1 0)"
#'   g_delaunay_triangulation(g, as_wkb = FALSE)
#' }
#'
#' g <- "LINESTRING(0 0,1 1,10 0)"
#' g_simplify(g, tolerance = 5, as_wkb = FALSE)
#'
#' # g_unary_union() requires GDAL >= 3.7
#' if (gdal_version_num() >= gdal_compute_version(3, 7, 0)) {
#'   g <- "GEOMETRYCOLLECTION(POINT(0.5 0.5), POLYGON((0 0,0 1,1 1,1 0,0 0)),
#'         POLYGON((1 0,1 1,2 1,2 0,1 0)))"
#'   g_unary_union(g, as_wkb = FALSE)
#' }
#' @export
g_buffer <- function(geom, dist, quad_segs = 30L, as_wkb = TRUE,
                     as_iso = FALSE, byte_order = "LSB", quiet = FALSE) {

    # dist
    if (missing(dist) || is.null(dist))
        stop("a value for 'dist' is required", call. = FALSE)
    if (!is.numeric(dist) || length(dist) > 1)
        stop("'dist' must be a single numeric value", call. = FALSE)
    # quad_segs
    if (missing(quad_segs) || is.null(quad_segs))
        quad_segs <- 30L
    if (!is.numeric(quad_segs) || length(quad_segs) > 1) {
        stop("'quad_segs' must be a single numeric value (integer)",
             call. = FALSE)
    }
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_buffer(geom, dist, quad_segs, as_iso, byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
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

#' @name g_unary_op
#' @export
g_boundary <- function(geom, as_wkb = TRUE, as_iso = FALSE,
                       byte_order = "LSB", quiet = FALSE) {
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_boundary(geom, as_iso, byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_boundary, as_iso, byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_boundary(g_wk2wk(geom), as_iso, byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_boundary, as_iso, byte_order,
                          quiet)
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

#' @name g_unary_op
#' @export
g_convex_hull <- function(geom, as_wkb = TRUE, as_iso = FALSE,
                          byte_order = "LSB", quiet = FALSE) {
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_convex_hull(geom, as_iso, byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_convex_hull, as_iso, byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_convex_hull(g_wk2wk(geom), as_iso, byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_convex_hull, as_iso, byte_order,
                          quiet)
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

#' @name g_unary_op
#' @export
g_concave_hull <- function(geom, ratio, allow_holes, as_wkb = TRUE,
                           as_iso = FALSE, byte_order = "LSB",
                           quiet = FALSE) {
    # ratio
    if (missing(ratio) || is.null(ratio) || all(is.na(ratio)))
        stop("'ratio' is required", call. = FALSE)
    if (!(is.numeric(ratio) && length(ratio) == 1))
        stop("'ratio' must be a single numeric value [0, 1]", call. = FALSE)
    # allow_holes
    if (missing(allow_holes) || is.null(allow_holes) || all(is.na(allow_holes)))
        stop("'allow_holes' is required", call. = FALSE)
    if (!(is.logical(allow_holes) && length(allow_holes) == 1)) {
        stop("'allow_holes' must be a single logical value", call. = FALSE)
    }
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_concave_hull(geom, ratio, allow_holes, as_iso, byte_order,
                               quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_concave_hull, ratio, allow_holes, as_iso,
                      byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_concave_hull(g_wk2wk(geom), ratio, allow_holes, as_iso,
                                   byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_concave_hull, ratio, allow_holes,
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

#' @name g_unary_op
#' @export
g_delaunay_triangulation <- function(geom, constrained = FALSE, tolerance = 0.0,
                                     only_edges = FALSE, as_wkb = TRUE,
                                     as_iso = FALSE, byte_order = "LSB",
                                     quiet = FALSE) {
    # constrained
    if (is.null(constrained))
        constrained <- FALSE
    if (!(is.logical(constrained) && length(constrained) == 1)) {
        stop("'constrained' must be a single logical value", call. = FALSE)
    }
    # tolerance
    if (is.null(tolerance))
        tolerance <- 0.0
    if (!(is.numeric(tolerance) && length(tolerance) == 1))
        stop("'tolerance' must be a single numeric value", call. = FALSE)
    # only_edges
    if (is.null(only_edges))
        only_edges <- FALSE
    if (!(is.logical(only_edges) && length(only_edges) == 1)) {
        stop("'only_edges' must be a single logical value", call. = FALSE)
    }
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_delaunay_triangulation(geom, constrained, tolerance,
                                         only_edges, as_iso, byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_delaunay_triangulation, constrained, tolerance,
                      only_edges, as_iso, byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_delaunay_triangulation(g_wk2wk(geom), constrained,
                                             tolerance, only_edges, as_iso,
                                             byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_delaunay_triangulation, constrained,
                          tolerance, only_edges, as_iso, byte_order, quiet)
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

#' @name g_unary_op
#' @export
g_simplify <- function(geom, tolerance, preserve_topology = TRUE,
                       as_wkb = TRUE, as_iso = FALSE, byte_order = "LSB",
                       quiet = FALSE) {
    # tolerance
    if (!(is.numeric(tolerance) && length(tolerance) == 1))
        stop("'tolerance' must be a single numeric value", call. = FALSE)
    # preserve_topology
    if (is.null(preserve_topology))
        preserve_topology <- TRUE
    if (!(is.logical(preserve_topology) &&
          length(preserve_topology) == 1)) {

        stop("'preserve_topology' must be a single logical value",
             call. = FALSE)
    }
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_simplify(geom, tolerance, preserve_topology, as_iso,
                           byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_simplify, tolerance, preserve_topology, as_iso,
                      byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_simplify(g_wk2wk(geom), tolerance, preserve_topology,
                               as_iso, byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_simplify, tolerance,
                          preserve_topology, as_iso, byte_order, quiet)
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

#' @name g_unary_op
#' @export
g_unary_union <- function(geom, as_wkb = TRUE, as_iso = FALSE,
                          byte_order = "LSB", quiet = FALSE) {

    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    if (.is_raw_or_null(geom)) {
        wkb <- .g_unary_union(geom, as_iso, byte_order, quiet)
    } else if (is.list(geom) && .is_raw_or_null(geom[[1]])) {
        wkb <- lapply(geom, .g_unary_union, as_iso, byte_order, quiet)
    } else if (is.character(geom)) {
        if (length(geom) == 1) {
            wkb <- .g_unary_union(g_wk2wk(geom), as_iso, byte_order, quiet)
        } else {
            wkb <- lapply(g_wk2wk(geom), .g_unary_union, as_iso, byte_order,
                          quiet)
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

#' Apply a coordinate transformation to a WKB/WKT geometry
#'
#' `g_transform()` will transform the coordinates of a geometry from their
#' current spatial reference system to a new target spatial reference system.
#' Normally this means reprojecting the vectors, but it could include datum
#' shifts, and changes of units.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @param srs_from Character string specifying the spatial reference system
#' for `geom`. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param srs_to Character string specifying the output spatial reference
#' system. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param wrap_date_line Logical value, `TRUE` to correct geometries that
#' incorrectly go from a longitude on a side of the antimeridian to the other
#' side. Defaults to `FALSE`.
#' @param date_line_offset Integer longitude gap in degree. Defaults to `10L`.
#' @param traditional_gis_order Logical value, `TRUE` to use traditional GIS
#' order of axis mapping (the default) or `FALSE` to use authority compliant
#' axis order. By default, input `geom` vertices are assumed to
#' be in longitude/latitude order if `srs_from` is a geographic coordinate
#' system. This can be overridden by setting `traditional_gis_order = FALSE`.
#' @param as_wkb Logical value, `TRUE` to return the output geometry in WKB
#' format (the default), or `FALSE` to return as WKT.
#' @param as_iso Logical value, `TRUE` to export as ISO WKB/WKT (ISO 13249
#' SQL/MM Part 3), or `FALSE` (the default) to export as "Extended WKB/WKT".
#' @param byte_order Character string specifying the byte order when output is
#' WKB. One of `"LSB"` (the default) or `"MSB"` (uncommon).
#' @param quiet Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.
#' @return
#' A geometry as WKB raw vector or WKT string, or a list/character vector of
#' geometries as WKB/WKT with length equal to the number of input geometries.
#'  `NULL` (`as_wkb = TRUE`) / `NA` (`as_wkb = FALSE`) is returned with a
#' warning if WKB input cannot be converted into an OGR geometry object, or if
#' an error occurs in the call to the underlying OGR API.
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
#' [bbox_transform()], [transform_bounds()]
#'
#' @examples
#' pt <- "POINT (-114.0 47.0)"
#' g_transform(pt, "WGS84", "EPSG:5070", as_wkb = FALSE)
#'
#' # correct geometries that incorrectly go from a longitude on a side of the
#' # antimeridian to the other side
#' geom <- "LINESTRING (-179 0,179 0)"
#' g_transform(geom, "WGS84", "WGS84", wrap_date_line = TRUE, as_wkb = FALSE)
#' @export
g_transform <- function(geom, srs_from, srs_to, wrap_date_line = FALSE,
                        date_line_offset = 10L, traditional_gis_order = TRUE,
                        as_wkb = TRUE, as_iso = FALSE, byte_order = "LSB",
                        quiet = FALSE) {
    # srs_from
    if (!(is.character(srs_from) && length(srs_from) == 1))
        stop("'srs_from' must be a character string", call. = FALSE)
    # srs_to
    if (!(is.character(srs_to) && length(srs_to) == 1))
        stop("'srs_to' must be a character string", call. = FALSE)
    # wrap_date_line
    if (is.null(wrap_date_line))
        wrap_date_line <- FALSE
    if (!(is.logical(wrap_date_line) &&
          length(wrap_date_line) == 1)) {

        stop("'wrap_date_line' must be a single logical value",
             call. = FALSE)
    }
    # date_line_offset
    if (is.null(date_line_offset))
        date_line_offset <- 10L
    if (!(is.numeric(date_line_offset) && length(date_line_offset) == 1))
        stop("'date_line_offset' must be an integer value", call. = FALSE)
    # traditional_gis_order
    if (is.null(traditional_gis_order))
        traditional_gis_order <- TRUE
    if (!(is.logical(traditional_gis_order) &&
          length(traditional_gis_order) == 1)) {

        stop("'traditional_gis_order' must be a single logical value",
             call. = FALSE)
    }
    # as_wkb
    if (is.null(as_wkb))
        as_wkb <- TRUE
    if (!is.logical(as_wkb) || length(as_wkb) > 1)
        stop("'as_wkb' must be a single logical value", call. = FALSE)
    # as_iso
    if (is.null(as_iso))
        as_iso <- FALSE
    if (!is.logical(as_iso) || length(as_iso) > 1)
        stop("'as_iso' must be a single logical value", call. = FALSE)
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
        stop("'quiet' must be a single logical value", call. = FALSE)

    wkb <- NULL
    # .g_transform() handles input as either one raw vector or list
    if (.is_raw_or_null(geom) ||
        (is.list(geom) && .is_raw_or_null(geom[[1]]))) {

        wkb <- .g_transform(geom, srs_from, srs_to, wrap_date_line,
                            date_line_offset, traditional_gis_order, as_iso,
                            byte_order, quiet)
    } else if (is.character(geom)) {
        wkb <- .g_transform(g_wk2wk(geom), srs_from, srs_to, wrap_date_line,
                            date_line_offset, traditional_gis_order, as_iso,
                            byte_order, quiet)
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    if (as_wkb)
        return(wkb)
    else
        return(g_wk2wk(wkb, as_iso))
}

#' Extract coordinate values from geometries
#'
#' `g_coords()` extracts coordinate values (vertices) from the input geometries
#' and returns a data frame with coordinates as columns.
#'
#' @param geom Either a raw vector of WKB or list of raw vectors, or a
#' character vector containing one or more WKT strings.
#' @return A data frame as returned by `wk::wk_coords()`: columns `feature_id`
#' (the index of the feature from the input), `part_id` (an arbitrary integer
#' identifying the point, line, or polygon from whence it came), `ring_id` (an
#' arbitrary integer identifying individual rings within polygons), and one
#' column per coordinate (`x`, `y`, and/or `z` and/or `m`).
#' @examples
#' dsn <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
#' lyr <- new(GDALVector, dsn)
#' d <- lyr$fetch(10)
#'
#' vertices <- g_coords(d$geom)
#' head(vertices)
#'
#' lyr$close()
#' @export
g_coords <- function(geom) {
    geom_in <- NULL
    if (is.raw(geom)) {
        geom_in <- wk::wkb(list(geom))
    } else if (is.list(geom) && is.raw(geom[[1]])) {
        geom_in <- wk::wkb(geom)
    } else if (is.character(geom)) {
        geom_in <- wk::wkt(geom)
    } else {
        stop("'geom' must be a character vector, raw vector, or list",
             call. = FALSE)
    }

    return(wk::wk_coords(geom_in))
}
