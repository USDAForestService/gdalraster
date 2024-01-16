# Miscellaneous helper functions for working with the GDAL API
# currently: addFileInZip(), getCreationOptions()
# Chris Toney <chris.toney at usda.gov>

#' Create/append to a (potentially seek-optimized) ZIP file
#'
#' `addFileInZip()` creates or opens existing ZIP file, and adds one or
#' more compressed files potentially using the seek optimization extension.
#' This function is basically a wrapper for `CPLAddFileInZip()` in the GDAL
#' Common Portability Library, but optionally creates a new ZIP file first
#' (with `CPLCreateZip()`). Specification for SOZip (Seek-Optimized ZIP)
#' is at \url{https://sozip.org/}. Requires GDAL >= 3.7.
#'
#' @param zip_file Filename of the ZIP file. Will be created if it does not
#' exist or if `overwrite = TRUE`, otherwise will append to an existing file.
#' @param in_files Character vector of input filenames to add.
#' 
#' @returns Logical indicating success (invisible \code{TRUE}).
#' An error is raised if the operation fails.
#'
#' @seealso
#' 
#'
#' @examples
#' 
#' @export
addFileInZip <- function(zip_file,
						in_files,
						overwrite = FALSE,
						full_paths = TRUE,
						sozip_enabled = "AUTO",
						sozip_chunk_size = NULL,
						sozip_min_file_size = NULL,
						num_threads = NULL,
						timestamp = NULL,
						content_type = NULL,
						quiet = FALSE) {

	if (as.integer(gdal_version()[2]) < 3070000)
		stop("addFileInZip() requires GDAL >= 3.7.", call. = FALSE)
	
	
	
}

#SOZIP_ENABLED=AUTO/YES/NO: whether to generate a SOZip index for the file. The default can be changed with the CPL_SOZIP_ENABLED configuration option.

#SOZIP_CHUNK_SIZE: chunk size to use for SOZip generation. Defaults to 32768.

#SOZIP_MIN_FILE_SIZE: minimum file size to consider to enable SOZip index generation in SOZIP_ENABLED=AUTO mode. Defaults to 1 MB.

#NUM_THREADS: number of threads used for SOZip generation. Defaults to ALL_CPUS.

#TIMESTAMP=AUTO/NOW/timestamp_as_epoch_since_jan_1_1970: in AUTO mode, the timestamp of pszInputFilename will be used (if available), otherwise it will fallback to NOW.

#CONTENT_TYPE=string: Content-Type value for the file. This is stored as a key-value pair in the extra field extension 'KV' (0x564b) dedicated to storing key-value pair metadata.


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

