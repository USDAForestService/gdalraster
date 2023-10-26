# Miscellaneous helper functions for working with the GDAL API
# (currently only getCreationOptions())
# Chris Toney <chris.toney at usda.gov>

#' Return the list of creation options of a GDAL driver
#'
#' `getCreationOptions()` returns the list of creation options supported by a
#' GDAL format driver as an XML string (invisibly).
#' Wrapper for `GDALGetDriverCreationOptionList()` in the GDAL API.
#' Information about the available creation options is also printed to the
#' console by default.
#'
#' @param format Raster format short name (e.g., "GTiff").
#' @param filter Optional character vector of creation option names. Controls
#' only the amount of information printed to the console.
#' By default, information for all creation options is printed. Can be set to
#' empty string `""` to disable printing information to the console.
#' @returns Invisibly, an XML string that describes the full list of creation
#' options or empty string `""` (full output of 
#' `GDALGetDriverCreationOptionList()` in the GDAL API).
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()]
#'
#' @examples
#' getCreationOptions("GTiff", filter="COMPRESS")
#' @export
getCreationOptions <- function(format, filter=NULL) {

	if (!is.character(format))
		stop("format argument must be a string.", call.=FALSE)
		
	if (is.null(filter))
		filter = "_all_"
		
	if (filter[1] == "")
		return(invisible(.getCreationOptions(format)))
		
	if (.getCreationOptions(format) == "") {
		message(paste0("No creation options found for ", format, "."))
		return(invisible(.getCreationOptions(format)))
	}
	
	x = xml2::read_xml(.getCreationOptions(format))
	opt_list <- xml2::xml_find_all(x, xpath = "//Option")
	for (n in 1:length(opt_list)) {
		if (filter[1] == "_all_"
			|| xml2::xml_attr(opt_list[[n]], "name") %in% filter)
				
			print(opt_list[[n]])
	}
	return(invisible(.getCreationOptions(format)))
}

