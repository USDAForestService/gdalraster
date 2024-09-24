#' @export
print.OGRFeature <- function(x, ...) {
    geom_col_name <- attr(x, "gis")$geom_col_name
    geom_format <- toupper(attr(x, "gis")$geom_format)

    cat("OGR feature\n")

    if (geom_format == "WKB" || geom_format == "WKB_ISO") {
        y <- unclass(x)
        for (i in seq_along(geom_col_name)) {
            if (is.raw(x[[geom_col_name[i]]])) {
                wkb <- x[[geom_col_name[i]]]
                geom_name <- g_name(wkb)
            } else if (is.raw(x[[geom_col_name[i]]][[1]])) {
                wkb <- x[[geom_col_name[i]]][[1]]
                geom_name <- g_name(wkb)
            } else {
                wkb <- "error"
                geom_name <- "unknown"
            }
            wkb_starts_with <- paste(wkb[1:4], collapse = " ")
            y[[geom_col_name[i]]] <- paste0(geom_format,
                                            " ",
                                            geom_name,
                                            ": ",
                                            "raw ",
                                            wkb_starts_with,
                                            " ...")
        }
        attr(y, "gis") <- NULL
        print(y, quote = FALSE, ...)

    } else if (geom_format == "WKT" || geom_format == "WKT_ISO") {
        y <- unclass(x)
        for (i in seq_along(geom_col_name)) {
            wkt <- x[[geom_col_name[i]]]
            wkt_starts_with <- substring(wkt, 1, 28)
            y[[geom_col_name[i]]] <- paste0(geom_format,
                                            ": chr \"",
                                            wkt_starts_with,
                                            " ...\"")
        }
        attr(y, "gis") <- NULL
        print(y, quote = FALSE, ...)

    } else {
        print(x[seq_along(x)], quote = FALSE, ...)
    }

    return(invisible(x))
}

#' @export
print.OGRFeature.set <- function(x, ...) {
    geom_col_name <- attr(x, "gis")$geom_col_name
    geom_format <- toupper(attr(x, "gis")$geom_format)

    cat("OGR feature set\n")

    if (nrow(x) == 0) {
        print.data.frame(x[seq_along(x)], right = FALSE, ...)

    } else if (geom_format == "WKB" || geom_format == "WKB_ISO") {
        y <- x
        for (i in seq_along(geom_col_name)) {
            geom_name <- g_name(x[, geom_col_name[i]])
            wkb_starts_with <- sapply(x[, geom_col_name[i]],
                                      function(g) paste(g[1:4], collapse = " "))

            geom_col_print <- paste0(geom_format, " ",
                                     geom_name, ": ", "raw ",
                                     wkb_starts_with, " ...")
            y[geom_col_name[i]] <- geom_col_print
        }
        attr(y, "gis") <- NULL
        print.data.frame(y, ...)

    } else if (geom_format == "WKT" || geom_format == "WKT_ISO") {
        y <- x
        for (i in seq_along(geom_col_name)) {
            wkt <- x[[geom_col_name[i]]]
            wkt_starts_with <- substring(wkt, 1, 28)
            y[geom_col_name[i]] <- paste0(geom_format, ": chr \"",
                                          wkt_starts_with, " ...\"")
        }
        attr(y, "gis") <- NULL
        print.data.frame(y, ...)

    } else {
        print.data.frame(x[seq_along(x)], right = FALSE, ...)
    }

    return(invisible(x))
}
