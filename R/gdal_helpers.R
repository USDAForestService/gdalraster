# Miscellaneous helper functions for working with the GDAL API
# currently: addFilesInZip(), getCreationOptions()
# Chris Toney <chris.toney at usda.gov>

#' Create/append to a potentially Seek-Optimized ZIP file (SOZip)
#'
#' `addFilesInZip()` will create new or open existing ZIP file, and
#' add one or more compressed files potentially using the seek optimization
#' extension. This function is basically a wrapper for `CPLAddFileInZip()`
#' in the GDAL Common Portability Library, but optionally creates a new ZIP
#' file first (with `CPLCreateZip()`). It provides a subset of functionality
#' in the GDAL `sozip` command-line utility
#' (\url{https://gdal.org/programs/sozip.html}). Requires GDAL >= 3.7.
#'
#' @details
#' A Seek-Optimized ZIP file (SOZip) contains one or more compressed files
#' organized and annotated such that a SOZip-aware reader can perform very
#' fast random access within the .zip file
#' (see \url{https://github.com/sozip/sozip-spec}).
#' Large compressed files can be accessed directly from SOZip without prior
#' decompression. The .zip file is otherwise fully backward compatible.
#'
#' If `sozip_enabled="AUTO"` (the default), a file is seek-optimized only if
#' its size is above the values of `sozip_min_file_size` (default 1 MB) and
#' `sozip_chunk_size` (default `32768`).
#' In `"YES"` mode, all input files will be seek-optimized. In `"NO"` mode, no
#' input files will be seek-optimized. The default can be changed with the
#' `CPL_SOZIP_ENABLED` configuration option.
#'
#' @param zip_file Filename of the ZIP file. Will be created if it does not
#' exist or if `overwrite = TRUE`. Otherwise will append to an existing file.
#' @param add_files Character vector of one or more input filenames to add.
#' @param overwrite Logical scalar. Overwrite the target zip file if it already
#' exists.
#' @param full_paths Logical scalar. By default, the full path will be stored
#' (relative to the current directory). `FALSE` to store just the name of a
#' saved file (drop the path).
#' @param sozip_enabled String. Whether to generate a SOZip index for the file.
#' One of `"AUTO"` (the default), `"YES"` or `"NO"` (see Details).
#' @param sozip_chunk_size The chunk size for a seek-optimized file.
#' Defaults to 32768 bytes. The value is specified in bytes, or K and M
#' suffix can be used respectively to specify a value in kilo-bytes or
#' mega-bytes. Will be coerced to string.
#' @param sozip_min_file_size The minimum file size to decide if a file
#' should be seek-optimized, in `sozip_enabled="AUTO"` mode. Defaults to
#' 1 MB byte. The value is specified in bytes, or K, M or G suffix can be used
#' respectively to specify a value in kilo-bytes, mega-bytes or giga-bytes.
#' Will be coerced to string.
#' @param num_threads Number of threads used for SOZip generation. Defaults to
#' `"ALL_CPUS"` or specify an integer value (coerced to string).
#' @param content_type String Content-Type value for the file. This is stored
#' as a key-value pair in the extra field extension 'KV' (0x564b) dedicated to
#' storing key-value pair metadata.
#' @param quiet Logical scalar. `TRUE` for quiet mode, no progress messages
#' emitted. Defaults to `FALSE`.
#' @returns Logical indicating success (invisible \code{TRUE}).
#' An error is raised if the operation fails.
#'
#' @note
#' The `GDAL_NUM_THREADS` configuration option can be set to `ALL_CPUS` or an
#' integer value to specify the number of threads to use for SOZip-compressed
#' files (see [set_config_option()]).
#'
#' @examples
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' zip_file <- paste0(tempdir(), "/", "storml_lcp.zip")
#'
#' # Requires GDAL >= 3.7
#' if (as.integer(gdal_version()[2]) >= 3070000) {
#'   # Note that the example file is too small to be seek-optimized by default
#'   # So this creates a regular zip file
#'   addFilesInZip(zip_file, lcp_file, full_paths=FALSE, num_threads=1)
#'   unzip(zip_file, list=TRUE)
#'
#'   # Open with GDAL using Virtual File System handler '/vsizip/'
#'   # see: https://gdal.org/user/virtual_file_systems.html#vsizip-zip-archives
#'   lcp_in_zip <- paste0("/vsizip/", file.path(zip_file, "storm_lake.lcp"))
#'   ds <- new(GDALRaster, lcp_in_zip)
#'   ds$info()
#'   ds$close()
#' }
#' @export
addFilesInZip <- function(
		zip_file,
		add_files,
		overwrite = FALSE,
		full_paths = TRUE,
		sozip_enabled = NULL,
		sozip_chunk_size = NULL,
		sozip_min_file_size = NULL,
		num_threads = NULL,
		content_type = NULL,
		quiet = FALSE) {

	if (as.integer(gdal_version()[2]) < 3070000)
		stop("`addFilesInZip()` requires GDAL >= 3.7.", call. = FALSE)

	if (!is.character(zip_file) || length(zip_file) > 1)
		stop("`zip_file` must be a string.", call. = FALSE)
	
	if (!is.character(add_files))
		stop("`add_files` must be a character vector of filenames.",
			call. = FALSE)

	if (!is.null(overwrite)) {
		if (!is.logical(overwrite) || length(overwrite) > 1)
			stop("`overwrite` must be a logical scalar.", call. = FALSE)
	}
	else {
		overwrite <- FALSE
	}

	if (!is.null(full_paths)) {
		if (!is.logical(full_paths) || length(full_paths) > 1)
			stop("`full_paths` must be a logical scalar.", call. = FALSE)
	}
	else {
		full_paths <- FALSE
	}

	if (!is.null(quiet)) {
		if (!is.logical(quiet) || length(quiet) > 1)
			stop("`quiet` must be a logical scalar.", call. = FALSE)
	}
	else {
		quiet <- FALSE
	}

	opt <- NULL
	if (!is.null(sozip_enabled)) {
		if (!is.character(sozip_enabled) || length(sozip_enabled) > 1)
			stop("`sozip_enabled` must be a string.", call. = FALSE)
		sozip_enabled <- toupper(sozip_enabled)
		if ( !(sozip_enabled %in% c("AUTO", "YES", "NO")) )
			stop("`sozip_enabled` must be one of AUTO, YES or NO.", call. = FALSE)
		else
			opt <- c(opt, paste0("SOZIP_ENABLED=", sozip_enabled))
	}
	if (!is.null(sozip_chunk_size)) {
		if (length(sozip_chunk_size) > 1)
			stop("`sozip_chunk_size` must be length-1.", call. = FALSE)
		opt <- c(opt, paste0("SOZIP_CHUNK_SIZE=", sozip_chunk_size))
	}
	if (!is.null(sozip_min_file_size)) {
		if (length(sozip_min_file_size) > 1)
			stop("`sozip_min_file_size` must be length-1.", call. = FALSE)
		opt <- c(opt, paste0("SOZIP_MIN_FILE_SIZE=", sozip_min_file_size))
	}
	if (!is.null(num_threads)) {
		if (length(num_threads) > 1)
			stop("`num_threads` must be length-1.", call. = FALSE)
		opt <- c(opt, paste0("NUM_THREADS=", num_threads))
	}
	if (!is.null(content_type)) {
		if (!is.character(content_type) || length(content_type) > 1)
			stop("`content_type` must be a string.", call. = FALSE)
		opt <- c(opt, paste0("CONTENT_TYPE=", content_type))
	}
	
	if (overwrite)
		unlink(zip_file)
	
	ret <- FALSE
	for (f in add_files) {
		if (!(utils::file_test("-f", f)))
			stop(paste0("File not found: ", f), call. = FALSE)
		
		archive_fname <- f
		if (!full_paths) {
			archive_fname <- basename(f)
		}
		else if (substr(f, 1, 1) == "/") {
			archive_fname <- substring(f, 2)
		}
		else if (nchar(f) > 3 && substr(f, 2, 2) == ":" &&
					(substr(f, 3, 3) == "/" || substr(f, 3, 3) == '\\')) {
			archive_fname <- substring(f, 4)
		}
		archive_fname <- enc2utf8(archive_fname)

		if (!.addFileInZip(zip_file,
							overwrite = FALSE,
							archive_fname,
							f,
							opt,
							quiet)) {
			ret <- FALSE
			break
		}
		else {
			ret <- TRUE
		}
	}
	
	if (!ret)
		stop("Failed to add file, error from CPLAddFileInZip().",
				call. = FALSE)
	
	return(invisible(ret))
}


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

	if (!is.character(format) || length(format) > 1)
		stop("`format` must be a string.", call.=FALSE)
		
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

