/* GDAL VSI wrapper functions supporting virtual file systems
   Chris Toney <chris.toney at usda.gov> */

#include "gdal.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

#include "gdalraster.h"

//' Copy a source file to a target filename
//'
//' `vsi_copy_file()` is a wrapper for `VSICopyFile()` in the GDAL Common
//' Portability Library. The GDAL VSI functions allow virtualization of disk
//' I/O so that non file data sources can be made to appear as files.
//' See \url{https://gdal.org/user/virtual_file_systems.html}.
//' Requires GDAL >= 3.7.
//' 
//' The following copies are made fully on the target server, without local
//' download from source and upload to target:
//' \preformatted{
//' /vsis3/ -> /vsis3/
//' /vsigs/ -> /vsigs/
//' /vsiaz/ -> /vsiaz/
//' /vsiadls/ -> /vsiadls/
//' any of the above or /vsicurl/ -> /vsiaz/ (starting with GDAL 3.8)
//' }
//'
//' @param src_file Character string. Filename of the source file.
//' @param target_file Character string. Filename of the target file.
//' @param show_progess Logical scalar. If `TRUE`, a progress bar will be
//' displayed (the size of `src_file` will be retrieved in GDAL with
//' `VSIStatL()`). Default is `FALSE`.
//' @returns Invisibly, 0 on success or -1 on an error.
//'
//' @note
//' If `target_file` has the form /vsizip/foo.zip/bar, the default options
//' described for the function `addFilesInZip()` will be in effect.
//' 
//' @seealso
//' [copyDatasetFiles()]
//'
//' @examples
//' # for illustration only
//' # this would normally be used with GDAL virtual file systems
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' tmp_file <- tempfile(fileext = ".tif")
//'
//' # Requires GDAL >= 3.7
//' if (as.integer(gdal_version()[2]) >= 3070000) {
//'   result <- vsi_copy_file(elev_file, tmp_file)
//'   print(result)
//' }
// [[Rcpp::export(invisible = true)]]
int vsi_copy_file(Rcpp::CharacterVector src_file,
		Rcpp::CharacterVector target_file,
		bool show_progess = false) {

#if GDAL_VERSION_NUM < 3070000
	Rcpp::stop("vsi_copy_file() requires GDAL >= 3.7.");

#else
	GDALProgressFunc pfnProgress = NULL;
	std::string src_file_in;
	src_file_in = Rcpp::as<std::string>(_check_gdal_filename(src_file));
	std::string target_file_in;
	target_file_in = Rcpp::as<std::string>(_check_gdal_filename(target_file));
	
	if (show_progess)
		pfnProgress = GDALTermProgressR;
	
	int result = VSICopyFile(src_file_in.c_str(), target_file_in.c_str(),
			NULL, -1, NULL, pfnProgress, NULL);

	if (result == 0)
		return 0;
	else
		return -1;
	
#endif
}


//' Clean cache associated with /vsicurl/ and related file systems
//'
//' `vsi_curl_clear_cache()` cleans the local cache associated with /vsicurl/
//' (and related file systems). This function is a wrapper for
//' `VSICurlClearCache()` and `VSICurlPartialClearCache()` in the GDAL Common
//' Portability Library. See Details for the GDAL documentation.
//'
//' @details
//' /vsicurl (and related file systems like /vsis3/, /vsigs/, /vsiaz/,
//' /vsioss/, /vsiswift/) cache a number of metadata and data for faster
//' execution in read-only scenarios. But when the content on the server-side
//' may change during the same process, those mechanisms can prevent opening
//' new files, or give an outdated version of them.
//' If `partial = TRUE`, cleans the local cache associated for a given filename
//' (and its subfiles and subdirectories if it is a directory)
//'
//' @param partial Logical scalar. Whether to clear the cache only for a given
//' filename (see Details).
//' @param file_prefix Character string. Filename prefix to use if
//' `partial = TRUE`.
//' @returns No return value, called for side effects.
//'
//' @examples
//' vsi_curl_clear_cache()
// [[Rcpp::export()]]
void vsi_curl_clear_cache(bool partial = false,
		Rcpp::CharacterVector file_prefix = "") {

	if (!partial) {
		VSICurlClearCache();
	}
	else {
		std::string f_prefix_in;
		f_prefix_in = Rcpp::as<std::string>(_check_gdal_filename(file_prefix));
		VSICurlPartialClearCache(f_prefix_in.c_str());
	}
}


//' Read names in a directory
//'
//' `vsi_read_dir()` abstracts access to directory contents. It returns a
//' character vector containing the names of files and directories in this
//' directory. This function is a wrapper for `VSIReadDirEx()` in the GDAL
//' Common Portability Library.
//'
//' @param path Character string. The relative or absolute path of a
//' directory to read.
//' @param max_files Integer scalar. The maximum number of files after which to
//' stop, or 0 for no limit (see Note).
//' @returns A character vector containing the names of files and directories
//' in the directory given by `path`. An empty string (`""`) is returned if
//' `path` does not exist.
//'
//' @note
//' If `max_files` is set to a positive number, directory listing will stop
//' after that limit has been reached. Note that to indicate truncation, at
//' least one element more than the `max_files` limit will be returned. If the
//' length of the returned character vector is lesser or equal to `max_files`,
//' then no truncation occurred.
//'
//' @examples
//' # for illustration only
//' # this would normally be used with GDAL virtual file systems
//' data_dir <- system.file("extdata", package="gdalraster")
//' vsi_read_dir(data_dir)
// [[Rcpp::export()]]
Rcpp::CharacterVector vsi_read_dir(Rcpp::CharacterVector path,
		int max_files = 0) {
	
	std::string path_in;
	path_in = Rcpp::as<std::string>(_check_gdal_filename(path));
	
	char **papszFiles;
	papszFiles = VSIReadDirEx(path_in.c_str(), max_files);
	
	int items = CSLCount(papszFiles);
	if (items > 0) {
		Rcpp::CharacterVector files(items);
		for (int i=0; i < items; ++i) {
			files(i) = papszFiles[i];
		}
		CSLDestroy(papszFiles);
		return files;
	}
	else {
		CSLDestroy(papszFiles);
		return "";	
	}
}

