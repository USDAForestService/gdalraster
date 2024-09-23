
#' @export
print.OGRFeature <- function(x, ...) {
    geom_col_name <- attr(x, "gis")$geom_col_name
    geom_format <- toupper(attr(x, "gis")$geom_format)

    cat("OGR Feature\n")

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
            y[[geom_col_name[i]]] <- paste0(geom_format, " ", geom_name, ": ",
                                            "raw ", wkb_starts_with, " ...")
        }
        attr(y, "gis") <- NULL
        print(y, quote = FALSE, ...)

    } else if (geom_format == "WKT" || geom_format == "WKT_ISO") {
        y <- unclass(x)
        for (i in seq_along(geom_col_name)) {
            if (is.character(x[[geom_col_name[i]]])) {
                wkt <- x[[geom_col_name[i]]]
            } else {
                wkt <- "error: not character"
            }
            wkt_starts_with <- substring(wkt, 1, 28)
            y[[geom_col_name[i]]] <- paste0(geom_format, ": chr \"",
                                            wkt_starts_with, " ...\"")
        }
        attr(y, "gis") <- NULL
        print(y, quote = FALSE, ...)

    } else {
        print(x[seq_along(x)], quote = FALSE, ...)
    }

    return(invisible(x))
}

