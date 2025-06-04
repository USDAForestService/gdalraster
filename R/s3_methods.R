#' Print an `OGRFeature` object
#'
#' @param x An `OGRFeature` object.
#' @param ... Optional arguments passed to `base::print()`.
#' @return The input, invisibly.
#' @export
#' @method print OGRFeature
print.OGRFeature <- function(x, ...) {
    geom_column <- attr(x, "gis")$geom_column
    geom_format <- toupper(attr(x, "gis")$geom_format)

    if (geom_format == "NONE" || length(geom_column) == 0)
        cat("OGR feature (attributes)\n")
    else
        cat("OGR feature\n")

    if (geom_format == "WKB" || geom_format == "WKB_ISO") {
        y <- unclass(x)
        for (i in seq_along(geom_column)) {
            if (.is_raw_or_null(x[[geom_column[i]]])) {
                wkb <- x[[geom_column[i]]]
                geom_name <- g_name(wkb)
                if (is.na(geom_name))
                    geom_name <- "NULL geometry"
            } else if (.is_raw_or_null(x[[geom_column[i]]][[1]])) {
                # in case of nested list, i.e., from a data frame list column
                wkb <- x[[geom_column[i]]][[1]]
                geom_name <- g_name(wkb)
                if (is.na(geom_name))
                    geom_name <- "NULL geometry"
            } else {
                wkb <- "error"
                geom_name <- "unknown"
            }
            wkb_starts_with <- paste(wkb[1:4], collapse = " ")
            y[[geom_column[i]]] <- paste0(geom_format, " ",
                                          geom_name, ": raw ",
                                          wkb_starts_with, " ...")
        }
        attr(y, "gis") <- NULL
        print(y, quote = FALSE, ...)

    } else if (geom_format == "WKT" || geom_format == "WKT_ISO") {
        y <- unclass(x)
        for (i in seq_along(geom_column)) {
            wkt <- x[[geom_column[i]]]
            if (is.na(wkt))
                geom_format <- "WKT NULL geometry"
            wkt_starts_with <- substring(wkt, 1, 28)
            y[[geom_column[i]]] <- paste0(geom_format, ": chr \"",
                                          wkt_starts_with, " ...\"")
        }
        attr(y, "gis") <- NULL
        print(y, quote = FALSE, ...)

    } else {
        print(x[seq_along(x)], quote = FALSE, ...)
    }

    invisible(x)
}

#' Print an `OGRFeatureSet`
#'
#' @param x An `OGRFeatureSet`.
#' @param ... Optional arguments passed to `base::print.data.frame()`.
#' @return The input, invisibly.
#' @export
#' @method print OGRFeatureSet
print.OGRFeatureSet <- function(x, ...) {
    geom_column <- attr(x, "gis")$geom_column
    geom_format <- toupper(attr(x, "gis")$geom_format)

    if (geom_format == "NONE" || length(geom_column) == 0)
        cat("OGR feature set (attribute table)\n")
    else
        cat("OGR feature set\n")

    if (nrow(x) == 0) {
        print.data.frame(x[seq_along(x)], right = FALSE, ...)

    } else if (geom_format == "WKB" || geom_format == "WKB_ISO") {
        y <- x
        for (i in seq_along(geom_column)) {
            geom_name <- g_name(x[, geom_column[i]])
            geom_name[is.na(geom_name)] <- "NULL geometry"
            wkb_starts_with <- sapply(x[, geom_column[i]],
                                      function(g) paste(g[1:4], collapse = " "))

            geom_col_print <- paste0(geom_format, " ",
                                     geom_name, ": ", "raw ",
                                     wkb_starts_with, " ...")
            y[geom_column[i]] <- geom_col_print
        }
        attr(y, "gis") <- NULL
        print.data.frame(y, ...)

    } else if (geom_format == "WKT" || geom_format == "WKT_ISO") {
        y <- x
        for (i in seq_along(geom_column)) {
            wkt <- x[[geom_column[i]]]
            wkt_starts_with <- substring(wkt, 1, 28)
            y[geom_column[i]] <- paste0(geom_format, ": chr \"",
                                        wkt_starts_with, " ...\"")
            y[is.na(wkt), geom_column[i]] <- paste0("WKT NULL geometry",
                                                    ": chr \"",
                                                    wkt_starts_with,
                                                    " ...\"")
        }
        attr(y, "gis") <- NULL
        print.data.frame(y, ...)

    } else {
        print.data.frame(x[seq_along(x)], right = FALSE, ...)
    }

    invisible(x)
}

#' Plot the geometry of an `OGRFeature` object
#'
#' @param x An `OGRFeature` object.
#' @param xlab Title for the x axis.
#' @param ylab Title for the y axis.
#' @param main The main title (on top).
#' @param ... Optional arguments passed to `wk::wk_plot()`.
#' @return The input, invisibly.
#' @export
#' @method plot OGRFeature
plot.OGRFeature <- function(x, xlab = "x", ylab = "y",  main = "", ...) {
    geom_column <- NULL
    if (length(attr(x, "gis")$geom_column) == 0)
        stop("no geometry column")
    else
        geom_column <- attr(x, "gis")$geom_column[1]

    geom_format <- ""
    if (length(attr(x, "gis")$geom_format) == 0)
        stop("missing geometry format")
    else
        geom_format <- toupper(attr(x, "gis")$geom_format)

    if (!geom_format %in% c("WKB", "WKB_ISO", "WKT", "WKT_ISO", "BBOX"))
        stop("no supported geometry format for plot")

    srs <- attr(x, "gis")$geom_col_srs[1]

    if (geom_format == "BBOX") {
        bb <- x[[geom_column]]
        wk_obj <- wk::rct(bb[1], bb[2], bb[3], bb[4], crs = srs)
    } else if (startsWith(geom_format, "WKB")) {
        wk_obj <- wk::wkb(list(x[[geom_column]]), crs = srs)
    } else {
        wk_obj <- wk::wkt(x[[geom_column]], crs = srs)
    }

    wk::wk_plot(wk_obj, ..., xlab = xlab, ylab = ylab)
    graphics::title(main = main)

    invisible(x)
}

#' Plot the geometry column of an `OGRFeatureSet`
#'
#' @param x An `OGRFeatureSet`.
#' @param xlab Title for the x axis.
#' @param ylab Title for the y axis.
#' @param main The main title (on top).
#' @param ... Optional arguments passed to `wk::wk_plot()`.
#' @return The input, invisibly.
#' @export
#' @method plot OGRFeatureSet
plot.OGRFeatureSet <- function(x, xlab = "x", ylab = "y", main = "", ...) {
    if (length(attr(x, "gis")$geom_column) == 0)
        stop("no geometry column")
    else
        geom_column <- attr(x, "gis")$geom_column[1]

    geom_format <- toupper(attr(x, "gis")$geom_format)
    if (!geom_format %in% c("WKB", "WKB_ISO", "WKT", "WKT_ISO", "BBOX"))
        stop("no supported geometry format for plot")

    srs <- attr(x, "gis")$geom_col_srs[1]

    if (geom_format == "BBOX") {
        wk_obj <- matrix(unlist(x[[geom_column]]), ncol = 4, byrow = TRUE) |>
            wk::as_rct()
    } else if (startsWith(geom_format, "WKB")) {
        wk_obj <- wk::wkb(x[[geom_column]], crs = srs)
    } else {
        wk_obj <- wk::wkt(x[[geom_column]], crs = srs)
    }

    wk::wk_plot(wk_obj, ..., xlab = xlab, ylab = ylab)
    graphics::title(main = main)

    invisible(x)
}
