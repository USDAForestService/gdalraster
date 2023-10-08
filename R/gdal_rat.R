# Functions for working with GDAL Raster Attribute Tables
# (currently only buildRAT())
# Chris Toney <chris.toney at usda.gov>


#' Build a basic GDAL Raster Attribute Table as data frame
#'
#' @description
#' `buildRAT()` reads all pixels of the input raster to obtain the set of
#' unique values and their counts. The result is returned as a data frame
#' suitable for use with the class method `GDALRaster$setDefaultRAT()`. The
#' returned data frame might be further modified before setting as a default
#' RAT, for example, by adding columns containing class names, color values,
#' or other information (see Details).
#'
#' @details
#' A GDAL Raster Attribute Table (or RAT) provides attribute information about
#' pixel values. Raster attribute tables can be used to represent histograms,
#' color tables, and classification information. Each row in the table applies
#' to either a single pixel value or a range of values, and might have
#' attributes such as the histogram count for that range, the color that pixels
#' of that range should be displayed, names of classes, or any other generic
#' information.
#'
#' Each column in a raster attribute table has a name, a type (integer, 
#' floating point or string), and a `GDALRATFieldUsage`. The usage
#' distinguishes columns with particular understood purposes (such as color,
#' histogram count, name), and columns that have other purposes not understood
#' by the library (long label, e.g., wildfire_risk_category, etc).
#'
#' In the general case, each row has a column indicating the minimum pixel
#' value falling into that category, and a column indicating the maximum pixel
#' value. In the GDAL API, these are indicated with usage values of `GFU_Min`
#' and `GFU_Max`. In other cases where each row is a discrete pixel value, one
#' column of usage `GFU_MinMax` would be used. In R, the table is represented
#' as a data frame with column attribute `"GFU"` containing the field usage as
#' a string, `"Max"`, `"Min"` or `"MinMax"` (case-sensitive).
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
#' In other cases all the categories are of equal size and regularly spaced,
#' and the categorization can be determined by knowing the value at which
#' the categories start and the size of a category. This is called
#' "Linear Binning" and the information is kept specially on the raster
#' attribute table as a whole. In R, an attribute table that uses linear
#' binning would have the following attributes set on the data frame:
#' attr `"Row0Min"` = numeric lower bound (pixel value) of the first category,
#' and attr `"BinSize"` = numeric width of each category (in pixel value
#' units).
#'
#' A raster attribute table is thematic or athematic (continuous). In R, the
#' data frame has an attribute named `"GDALRATTableType"` with string value of
#' either `"thematic"` or `"athematic"`.
#'
#' @note
#' The full raster will be scanned.
#'
#' If `na_value` is not specified, then the `NA` pixel value (if present)
#' will not be recoded in the output data frame. This may have implications
#' if joining to other data (`NA` will not match), or when using the returned
#' data frame to set a default RAT on a dataset (`NA` will be interpreted
#' as the internal value that R uses to represent it for the type, e.g.,
#' -2147483648 for integer).
#'
#' @param raster Either a `GDALRaster` object, or a character string containing
#' the file name of a raster dataset to open.
#' @param band Integer scalar. Band number to read (default `1`).
#' @param col_names Character vector of length two containing names to use for
#' column 1 (pixel values) and column 2 (pixel counts) in the output data
#' frame (default `c("VALUE", "COUNT")`).
#' @param table_type A character string describing the type of the attribute
#' table. One of either `"thematic"`, or `"athematic"` for continuous data
#' (the default).
#' @param na_value Numeric scalar. An `NA` pixel value (if present) will be
#' recoded to `na_value` in the returned data frame. If `NULL` (the default),
#' `NA` will not be recoded.
#' @returns A data frame with two columns containing the set of unique pixel
#' values and their counts. The columns have attribute `"GFU"` set to `"MinMax"`
#' for the values and `"PixelCount"` for the counts. The data frame has
#' attribute `"GDALRATTableType"` set to `table_type` (see Details).
#'
#' @seealso
#' [`GDALRaster$getDefaultRAT()`][GDALRaster],
#' [`GDALRaster$setDefaultRAT()`][GDALRaster]
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
#' df <- buildRAT(ds, table_type = "thematic", na_value = -9999)
#' attributes(df)
#' attributes(df$VALUE)
#' attributes(df$COUNT)
#' 
#' # get the full attribute table for LANDFIRE EVT from the CSV file
#' evt_csv <- system.file("extdata/LF20_EVT_220.csv", package="gdalraster")
#' evt_tbl <- read.csv(evt_csv)
#' head(evt_tbl)
#' evt_tbl <- evt_tbl[,1:7]
#' 
#' df <- merge(df, evt_tbl)
#' head(df)
#' 
#' # set column attributes for GDAL field usage
#' attr(df$EVT_NAME, "GFU") <- "Name"
#' attr(df$EVT_LF, "GFU") <- "Generic"
#' attr(df$EVT_PHYS, "GFU") <- "Generic"
#' attr(df$R, "GFU") <- "Red"
#' attr(df$G, "GFU") <- "Green"
#' attr(df$B, "GFU") <- "Blue"
#' 
#' ds$setDefaultRAT(band=1, df)
#' ds$flushCache()
#' 
#' df2 <- ds$getDefaultRAT(band=1)
#' head(df2)
#' 
#' ds$close()
#' @export
buildRAT <- function(raster,
				band = 1,
				col_names = c("VALUE", "COUNT"),
				table_type = "athematic",
				na_value = NULL) {

	if (length(raster) != 1)
		stop ("raster argument must have length 1.", call. = FALSE)
	if (is(raster, "Rcpp_GDALRaster"))
		f <- raster$getFilename()
	else if (is(raster, "character"))
		f <- raster
	else
		stop ("raster argument must be a GDALRaster object or filename.",
				call. = FALSE)
	
	if (length(band) != 1)
		stop ("band argument must be an integer scalar.", call. = FALSE)
	else
		band <- as.integer(band)
	
	if (length(col_names) != 2)
		stop ("col_names must have length 2.", call. = FALSE)

	if (length(table_type) != 1 || !is(table_type, "character"))
		stop ("table_type must be a character string.", call. = FALSE)

	df <- combine(f, bands=band)
	df <- df[,c(3,2)]
	names(df) <- col_names
	if (!is.null(na_value))
		df[is.na(df[,1]), 1] <- na_value
	df <- df[order(df[,1]),]
	row.names(df) <- NULL
	attr(df[,1], "GFU") <- "MinMax"
	attr(df[,2], "GFU") <- "PixelCount"
	attr(df, "GDALRATTableType") <- table_type
	return(df)
}

