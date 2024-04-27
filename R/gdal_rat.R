# Functions for working with GDAL Raster Attribute Tables
# Chris Toney <chris.toney at usda.gov>


#' Build a GDAL Raster Attribute Table with VALUE, COUNT
#'
#' @description
#' `buildRAT()` reads all pixels of an input raster to obtain the set of
#' unique values and their counts. The result is returned as a data frame
#' suitable for use with the class method `GDALRaster$setDefaultRAT()`. The
#' returned data frame might be further modified before setting as a Raster
#' Attribute Table in a dataset, for example, by adding columns containing
#' class names, color values, or other information (see Details).
#' An optional input data frame containing such attributes may be given, in
#' which case `buildRAT()` will attempt to join the additional columns and
#' automatically assign the appropriate metadata on the output data frame
#' (i.e., assign R attributes on the data frame and its columns that define
#' usage in a GDAL Raster Attribute Table).
#'
#' @details
#' A GDAL Raster Attribute Table (or RAT) provides attribute information about
#' pixel values. Raster attribute tables can be used to represent histograms,
#' color tables, and classification information. Each row in the table applies
#' to either a single pixel value or a range of values, and might have
#' attributes such as the histogram count for that value (or range), the color
#' that pixels of that value (or range) should be displayed, names of classes,
#' or various other information.
#'
#' Each column in a raster attribute table has a name, a type (integer,
#' double, or string), and a `GDALRATFieldUsage`. The usage
#' distinguishes columns with particular understood purposes (such as color,
#' histogram count, class name), and columns that have other purposes not
#' understood by the library (long labels, ancillary attributes, etc).
#'
#' In the general case, each row has a field indicating the minimum pixel
#' value falling into that category, and a field indicating the maximum pixel
#' value. In the GDAL API, these are indicated with usage values of `GFU_Min`
#' and `GFU_Max`. In the common case where each row is a discrete pixel value,
#' a single column with usage `GFU_MinMax` would be used instead.
#' In R, the table is represented as a data frame with column attribute `"GFU"`
#' containing the field usage as a string, e.g., `"Max"`, `"Min"` or `"MinMax"`
#' (case-sensitive).
#' The full set of possible field usage descriptors is:
#' \tabular{lll}{
#'  GFU attr       \tab GDAL enum        \tab Description\cr
#'	`"Generic"`    \tab `GFU_Generic`    \tab General purpose field\cr
#'	`"PixelCount"` \tab `GFU_PixelCount` \tab Histogram pixel count\cr
#'	`"Name"`       \tab `GFU_Name`       \tab Class name\cr
#'	`"Min"`        \tab `GFU_Min`        \tab Class range minimum\cr
#'	`"Max"`        \tab `GFU_Max`        \tab Class range maximum\cr
#'	`"MinMax"`     \tab `GFU_MinMax`     \tab Class value (min=max)\cr
#'	`"Red"`        \tab `GFU_Red`        \tab Red class color (0-255)\cr
#'	`"Green"`      \tab `GFU_Green`      \tab Green class color (0-255)\cr
#'	`"Blue"`       \tab `GFU_Blue`       \tab Blue class color (0-255)\cr
#'	`"Alpha"`      \tab `GFU_Alpha`      \tab Alpha transparency (0-255)\cr
#'	`"RedMin"`     \tab `GFU_RedMin`     \tab Color range red minimum\cr
#'	`"GreenMin"`   \tab `GFU_GreenMin`   \tab Color range green minimum\cr
#'	`"BlueMin"`    \tab `GFU_BlueMin`    \tab Color range blue minimum\cr
#'	`"AlphaMin"`   \tab `GFU_AlphaMin`   \tab Color range alpha minimum\cr
#'	`"RedMax"`     \tab `GFU_RedMax`     \tab Color range red maximum\cr
#'	`"GreenMax"`   \tab `GFU_GreenMax`   \tab Color range green maximum\cr
#'	`"BlueMax"`    \tab `GFU_BlueMax`    \tab Color range blue maximum\cr
#'	`"AlphaMax"`   \tab `GFU_AlphaMax`   \tab Color range alpha maximum
#' }
#'
#' `buildRAT()` assigns GFU `"MinMax"` on the column of pixel values (named
#' `"VALUE"` by default) and GFU `"PixelCount"` on the column of counts (named
#' `"COUNT"` by default).
#' If `join_df` is given, the additional columns that result from joining will
#' have GFU assigned automatically based on the column names (_ignoring case_).
#' First, the additional column names are checked for containing
#' the string `"name"` (e.g., `"classname"`, `"TypeName"`, `"EVT_NAME"`, etc).
#' The first matching column (if any) will be assigned a GFU of `"Name"`
#' (=`GFU_Name`, the field usage descriptor for class names). Next, columns
#' named `"R"` or `"Red"` will be assigned GFU `"Red"`, columns named `"G"` or
#' `"Green"` will be assigned GFU `"Green"`, columns named `"B"` or `"Blue"`
#' will be assigned GFU `"Blue"`, and columns named `"A"` or `"Alpha"` will be
#' assigned GFU `"Alpha"`. Finally, any remaining columns that have not been
#' assigned a GFU will be assigned `"Generic"`.
#'
#' In a variation of RAT, all the categories are of equal size and regularly
#' spaced, and the categorization can be determined by knowing the value at
#' which the categories start and the size of a category. This is called
#' "Linear Binning" and the information is kept specially on the raster
#' attribute table as a whole. In R, a RAT that uses linear binning would
#' have the following attributes set on the data frame:
#' attribute `"Row0Min"` = the numeric lower bound (pixel value) of the first
#' category, and attribute `"BinSize"` = the numeric width of each category (in
#' pixel value units). `buildRAT()` does not create tables with linear binning,
#' but one could be created manually based on the specifications above, and
#' applied to a raster with the class method `GDALRaster$setDefaultRAT()`.
#'
#' A raster attribute table is thematic or athematic (continuous). In R, this
#' is defined by an attribute on the data frame named `"GDALRATTableType"` with
#' value of either `"thematic"` or `"athematic"`.
#'
#' @note
#' The full raster will be scanned.
#'
#' If `na_value` is not specified, then an `NA` pixel value (if present)
#' will not be recoded in the output data frame. This may have implications
#' if joining to other data (`NA` will not match), or when using the returned
#' data frame to set a default RAT on a dataset (`NA` will be interpreted
#' as the value that R uses internally to represent it for the type, e.g.,
#' -2147483648 for `NA_integer_`). In some cases, removing the row in the output
#' data frame with value `NA`, rather than recoding, may be desirable (i.e., by
#' removing manually or by side effect of joining via `merge()`, for example).
#' Users should consider what is appropriate for a particular case.
#'
#' @param raster Either a `GDALRaster` object, or a character string containing
#' the file name of a raster dataset to open.
#' @param band Integer scalar, band number to read (default `1L`).
#' @param col_names Character vector of length two containing names to use for
#' column 1 (pixel values) and column 2 (pixel counts) in the output data
#' frame (defaults are `c("VALUE", "COUNT")`).
#' @param table_type Character string describing the type of the attribute
#' table. One of either `"thematic"`, or `"athematic"` for continuous data
#' (the default).
#' @param na_value Numeric scalar. If the set of unique pixel values has an
#' `NA`, it will be recoded to `na_value` in the returned data frame.
#' If `NULL` (the default), `NA` will not be recoded.
#' @param join_df Optional data frame for joining additional attributes. Must
#' have a column of unique values with the same name as `col_names[1]`
#' (`"VALUE"` by default).
#' @param quiet Logical scalar. If `TRUE``, a progress bar will not be
#' displayed. Defaults to `FALSE``.
#' @returns A data frame with at least two columns containing the set of unique
#' pixel values and their counts. These columns have attribute `"GFU"` set to
#' `"MinMax"` for the values, and `"PixelCount"` for the counts. If `join_df` is
#' given, the returned data frame will have additional columns that result from
#' `merge()`. The `"GFU"` attribute of the additional columns will be assigned
#' automatically based on the column names (_case-insensitive_ matching, see
#' Details). The returned data frame has attribute `"GDALRATTableType"` set to
#' `table_type`.
#'
#' @seealso
#' [`GDALRaster$getDefaultRAT()`][GDALRaster],
#' [`GDALRaster$setDefaultRAT()`][GDALRaster],
#' [displayRAT()]
#'
#' `vignette("raster-attribute-tables")`
#'
#' @examples
#' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
#' # make a copy to modify
#' f <- paste0(tempdir(), "/", "storml_evt_tmp.tif")
#' file.copy(evt_file,  f)
#'
#' ds <- new(GDALRaster, f, read_only=FALSE)
#' ds$getDefaultRAT(band=1) # NULL
#'
#' # get the full attribute table for LANDFIRE EVT from the CSV file
#' evt_csv <- system.file("extdata/LF20_EVT_220.csv", package="gdalraster")
#' evt_df <- read.csv(evt_csv)
#' nrow(evt_df)
#' head(evt_df)
#' evt_df <- evt_df[,1:7]
#'
#' tbl <- buildRAT(ds,
#'                 table_type = "thematic",
#'                 na_value = -9999,
#'                 join_df = evt_df)
#'
#' nrow(tbl)
#' head(tbl)
#'
#' # attributes on the data frame and its columns define usage in a GDAL RAT
#' attributes(tbl)
#' attributes(tbl$VALUE)
#' attributes(tbl$COUNT)
#' attributes(tbl$EVT_NAME)
#' attributes(tbl$EVT_LF)
#' attributes(tbl$EVT_PHYS)
#' attributes(tbl$R)
#' attributes(tbl$G)
#' attributes(tbl$B)
#'
#' ds$setDefaultRAT(band=1, tbl)
#' ds$flushCache()
#'
#' tbl2 <- ds$getDefaultRAT(band=1)
#' nrow(tbl2)
#' head(tbl2)
#'
#' ds$close()
#' deleteDataset(f)
#'
#' # Display
#' evt_gt <- displayRAT(tbl2, title = "Storm Lake EVT Raster Attribute Table")
#' class(evt_gt)  # an object of class "gt_tbl" from package gt
#' # To show the table:
#' # evt_gt
#' # or simply call `displayRAT()` as above but without assignment
#' # `vignette("raster-attribute-tables")` has example output
#' @export
buildRAT <- function(raster,
                     band = 1L,
                     col_names = c("VALUE", "COUNT"),
                     table_type = "athematic",
                     na_value = NULL,
                     join_df = NULL,
                     quiet = FALSE) {

    if (length(raster) != 1)
        stop("'raster' argument must have length 1", call. = FALSE)

    ds <- NULL
    close_ds <- FALSE
    if (is(raster, "Rcpp_GDALRaster")) {
        ds <- raster
    } else if (is(raster, "character")) {
        ds <- new(GDALRaster, raster)
        close_ds <- TRUE
    } else {
        stop("'raster' must be a 'GDALRaster' object or a filename",
             call. = FALSE)
    }

    if (length(band) != 1)
        stop("'band' must be an integer scalar", call. = FALSE)
    else
        band <- as.integer(band)

    if (length(col_names) != 2)
        stop("'col_names' must have length 2", call. = FALSE)

    if (length(table_type) != 1 || !is(table_type, "character"))
        stop("'table_type' must be a character string", call. = FALSE)

    join <- FALSE
    if (!is.null(join_df)) {
        join <- TRUE
        if (!is.data.frame(join_df))
            stop("'join_df' must be a data frame", call. = FALSE)
        if (!(col_names[1] %in% names(join_df)))
            stop("'names(join_df)' must contain 'col_names[1]'",
                 call. = FALSE)
        if (col_names[2] %in% names(join_df))
            stop("'names(join_df)' cannot contain 'col_names[2]'",
                 call. = FALSE)
    }

    d <- .value_count(ds, band, quiet)
    if (close_ds)
        ds$close()
    names(d) <- col_names
    if (!is.null(na_value))
        d[is.na(d[, 1]), 1] <- na_value
    if (join) {
        if (anyNA(d[, 1]))
            message("row with 'NA' value will be dropped in join")
        n_before <- nrow(d)
        d <- merge(d, join_df)
        if (n_before != nrow(d))
            message("rows before join: ", n_before, ", rows after: ", nrow(d))
    }
    d <- d[order(d[, col_names[1]]), ]
    row.names(d) <- NULL
    attr(d[, col_names[1]], "GFU") <- "MinMax"
    attr(d[, col_names[2]], "GFU") <- "PixelCount"
    if (join) {
        for (nm in setdiff(names(join_df), col_names)) {
            if (regexpr("name", tolower(nm), fixed=TRUE) > 0) {
                attr(d[, nm], "GFU") <- "Name"
                break
            }
        }
        hasRed <- FALSE
        hasGreen <- FALSE
        hasBlue <- FALSE
        hasAlpha <- FALSE
        for (nm in setdiff(names(join_df), col_names)) {
            if (tolower(nm) == "r" || tolower(nm) == "red") {
                if (!hasRed) {
                    attr(d[, nm], "GFU") <- "Red"
                    hasRed <- TRUE
                }
            } else if (tolower(nm) == "g" || tolower(nm) == "green") {
                if (!hasGreen) {
                    attr(d[, nm], "GFU") <- "Green"
                    hasGreen <- TRUE
                }
            } else if (tolower(nm) == "b" || tolower(nm) == "blue") {
                if (!hasBlue) {
                    attr(d[, nm], "GFU") <- "Blue"
                    hasBlue <- TRUE
                }
            } else if (tolower(nm) == "a" || tolower(nm) == "alpha") {
                if (!hasAlpha) {
                    attr(d[, nm], "GFU") <- "Alpha"
                    hasAlpha <- TRUE
                }
            }
            if (is.null(attr(d[, nm], "GFU"))) {
                attr(d[, nm], "GFU") <- "Generic"
            }
        }
    }
    attr(d, "GDALRATTableType") <- table_type
    return(d)
}


#' Display a GDAL Raster Attribute Table
#'
#' @description
#' `displayRAT()` generates a presentation table. Colors are shown if the
#' Raster Attribute Table contains RGB columns.
#' This function requires package `gt`.
#'
#' @param tbl A data frame formatted as a GDAL RAT (e.g., as returned by
#' `buildRAT()` or `GDALRaster$getDefaultRAT()`).
#' @param title Character string to be used in the table title.
#' @returns An object of class `"gt_tbl"` (i.e., a table created with
#' `gt::gt()`).
#'
#' @seealso
#' [buildRAT()], [`GDALRaster$getDefaultRAT()`][GDALRaster]
#'
#' `vignette("raster-attribute-tables")`
#'
#' @examples
#' # see examples for `buildRAT()`
#' @export
displayRAT <- function(tbl, title = "Raster Attribute Table") {

    if (!requireNamespace("gt", quietly = TRUE))
        stop("displayRAT() requires package 'gt'", call.=FALSE)

    if (!is.data.frame(tbl))
        stop("'tbl' must be a data frame", call.=FALSE)

    if (is.null(attr(tbl, "GDALRATTableType"))) {
        message("input data frame is missing attribute 'GDALRATTableType'")
        stop("'tbl' must be formatted as a GDAL RAT",
             call.=FALSE)
    }

    if (length(title) != 1 || !is(title, "character"))
        stop("'title' must be a character string", call. = FALSE)

    col_red <- NULL
    col_green <- NULL
    col_blue <- NULL
    col_alpha <- NULL
    hasRGB <- FALSE
    for (nm in names(tbl)) {
        if (attr(tbl[, nm], "GFU") == "Red") {
            col_red <- nm
        } else if (attr(tbl[, nm], "GFU") == "Green") {
            col_green <- nm
        } else if (attr(tbl[, nm], "GFU") == "Blue") {
            col_blue <- nm
        } else if (attr(tbl[, nm], "GFU") == "Alpha") {
            col_alpha <- nm
        }
    }
    if (!is.null(col_red) && !is.null(col_green) && !is.null(col_blue)) {
        hasRGB <- TRUE
        if (!is.null(col_alpha)) {
            tbl$Color <- rgb(tbl[, col_red],
                             tbl[, col_green],
                             tbl[, col_blue],
                             tbl[, col_alpha],
                             maxColorValue=255)
        } else {
            tbl$Color <- rgb(tbl[, col_red],
                             tbl[, col_green],
                             tbl[, col_blue],
                             maxColorValue=255)
        }
    }

    gt_tbl <- gt::gt(tbl)
    if (hasRGB) {
        f <- function(x) { tbl$Color[match(x, tbl$Color)] }
        gt_tbl <- gt::data_color(gt_tbl, columns="Color", fn=f)
        gt_tbl <- gt::cols_move_to_start(gt_tbl, columns="Color")
    }
    gt_tbl <- gt::tab_header(gt_tbl, title=title)

    return(gt_tbl)
}
