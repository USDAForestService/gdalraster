/* GDAL VSI wrapper functions supporting virtual file systems
   Chris Toney <chris.toney at usda.gov> */

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
//' @details
//' The following copies are made fully on the target server, without local
//' download from source and upload to target:
//' * /vsis3/ -> /vsis3/
//' * /vsigs/ -> /vsigs/
//' * /vsiaz/ -> /vsiaz/
//' * /vsiadls/ -> /vsiadls/
//' * any of the above or /vsicurl/ -> /vsiaz/ (starting with GDAL 3.8)
//'
//' @param src_file Character string. Filename of the source file.
//' @param target_file Character string. Filename of the target file.
//' @param show_progess Logical scalar. If `TRUE`, a progress bar will be
//' displayed (the size of `src_file` will be retrieved in GDAL with
//' `VSIStatL()`). Default is `FALSE`.
//' @returns Invisibly, `0` on success or `-1` on an error.
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

	if (_gdal_version_num() < 3070000)
		Rcpp::stop("vsi_copy_file() requires GDAL >= 3.7.");

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
//' (and its subfiles and subdirectories if it is a directory).
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


//' Synchronize a source file/directory with a target file/directory
//'
//' `vsi_sync()` is a wrapper for `VSISync()` in the GDAL Common Portability
//' Library. The GDAL documentation is given in Details.
//'
//' @details
//' `VSISync()` is an analog of the Linux `rsync` utility. In the current
//' implementation, `rsync` would be more efficient for local file copying,
//' but `VSISync()` main interest is when the source or target is a remote
//' file system like /vsis3/ or /vsigs/, in which case it can take into account
//' the timestamps of the files (or optionally the ETag/MD5Sum) to avoid
//' unneeded copy operations.
//' This is only implemented efficiently for:
//' * local filesystem <--> remote filesystem
//' * remote filesystem <--> remote filesystem (starting with GDAL 3.1)\cr
//' Where the source and target remote filesystems are the same and one of
//' /vsis3/, /vsigs/ or /vsiaz/. Or when the target is /vsiaz/ and the source
//' is /vsis3/, /vsigs/, /vsiadls/ or /vsicurl/ (starting with GDAL 3.8)
//'
//' Similarly to `rsync` behavior, if the source filename ends with a slash, it
//' means that the content of the directory must be copied, but not the
//' directory name. For example, assuming "/home/even/foo" contains a file
//' "bar", `VSISync("/home/even/foo/", "/mnt/media", ...)` will create a
//' "/mnt/media/bar" file.
//' Whereas `VSISync("/home/even/foo", "/mnt/media", ...)` will create a
//' "/mnt/media/foo" directory which contains a bar file.
//'
//' The `options` argument accepts a character vector of name=value pairs.
//' Currently accepted options are:\cr
//' * `RECURSIVE=NO` (the default is `YES`)
//' * `SYNC_STRATEGY=TIMESTAMP/ETAG/OVERWRITE`. Determines which criterion is
//' used to determine if a target file must be replaced when it already exists
//' and has the same file size as the source. Only applies for a source or
//' target being a network filesystem.
//' The default is `TIMESTAMP` (similarly to how 'aws s3 sync' works), that is
//' to say that for an upload operation, a remote file is replaced if it has a
//' different size or if it is older than the source. For a download operation,
//' a local file is replaced if it has a different size or if it is newer than
//' the remote file.
//' The `ETAG` strategy assumes that the ETag metadata of the remote file is
//' the MD5Sum of the file content, which is only true in the case of /vsis3/
//' for files not using KMS server side encryption and uploaded in a single PUT
//' operation (so smaller than 50 MB given the default used by GDAL). Only to
//' be used for /vsis3/, /vsigs/ or other filesystems using a MD5Sum as ETAG.
//' The `OVERWRITE` strategy (GDAL >= 3.2) will always overwrite the target
//' file with the source one.\cr
//' * `NUM_THREADS=integer`. (GDAL >= 3.1) Number of threads to use for parallel
//' file copying. Only use for when /vsis3/, /vsigs/, /vsiaz/ or /vsiadls/ is
//' in source or target. The default is 10 since GDAL 3.3.\cr
//' * `CHUNK_SIZE=integer`. (GDAL >= 3.1) Maximum size of chunk (in bytes) to use
//' to split large objects when downloading them from /vsis3/, /vsigs/, /vsiaz/
//' or /vsiadls/ to local file system, or for upload to /vsis3/, /vsiaz/ or
//' /vsiadls/ from local file system. Only used if `NUM_THREADS > 1`.
//' For upload to /vsis3/, this chunk size must be set at least to 5 MB. The
//' default is 8 MB since GDAL 3.3.\cr
//' * `x-amz-KEY=value`. (GDAL >= 3.5) MIME header to pass during creation of a
//' /vsis3/ object.\cr
//' * `x-goog-KEY=value`. (GDAL >= 3.5) MIME header to pass during creation of a
//' /vsigs/ object.\cr
//' * `x-ms-KEY=value`. (GDAL >= 3.5) MIME header to pass during creation of a
//' /vsiaz/ or /vsiadls/ object.
//'
//' @param src Character string. Source file or directory.
//' @param target Character string. Target file or directory.
//' @param show_progess Logical scalar. If `TRUE`, a progress bar will be
//' displayed. Defaults to `FALSE`.
//' @param options Character vector of `NAME=VALUE` pairs (see Details).
//' @returns Invisibly, `TRUE` on success or `FALSE` on an error.
//'
//' @examples
//' \dontrun{
//' # sample-data is a directory in the git repository for gdalraster that is
//' # not included in the R package:
//' # https://github.com/USDAForestService/gdalraster/tree/main/sample-data
//' # A copy of sample-data in an AWS S3 bucket, and a partial copy in an
//' # Azure Blob container, were used to generate the example below.
//' 
//' src <- "/vsis3/gdalraster-sample-data/"
//' # s3://gdalraster-sample-data is not public, set credentials
//' set_config_option("AWS_ACCESS_KEY_ID", "xxxxxxxxxxxxxx")
//' set_config_option("AWS_SECRET_ACCESS_KEY", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
//' vsi_read_dir(src)
//' #> [1] "README.md"
//' #> [2] "bl_mrbl_ng_jul2004_rgb_720x360.tif"
//' #> [3] "blue_marble_ng_neo_metadata.xml"
//' #> [4] "landsat_c2ard_sr_mt_hood_jul2022_utm.json"
//' #> [5] "landsat_c2ard_sr_mt_hood_jul2022_utm.tif"
//' #> [6] "lf_elev_220_metadata.html"
//' #> [7] "lf_elev_220_mt_hood_utm.tif"
//' #> [8] "lf_fbfm40_220_metadata.html"
//' #> [9] "lf_fbfm40_220_mt_hood_utm.tif"
//' 
//' dst <- "/vsiaz/sampledata"
//' set_config_option("AZURE_STORAGE_CONNECTION_STRING",
//'                   "<connection_string_for_gdalraster_account>")
//' vsi_read_dir(dst)
//' #> [1] "lf_elev_220_metadata.html"   "lf_elev_220_mt_hood_utm.tif"
//' 
//' # GDAL VSISync() supports direct copy for /vsis3/ -> /vsiaz/ (GDAL >= 3.8)
//' result <- vsi_sync(src, dst, show_progess = TRUE)
//' #> 0...10...20...30...40...50...60...70...80...90...100 - done.
//' print(result)
//' #> [1] TRUE
//' vsi_read_dir(dst)
//' #> [1] "README.md"
//' #> [2] "bl_mrbl_ng_jul2004_rgb_720x360.tif"
//' #> [3] "blue_marble_ng_neo_metadata.xml"
//' #> [4] "landsat_c2ard_sr_mt_hood_jul2022_utm.json"
//' #> [5] "landsat_c2ard_sr_mt_hood_jul2022_utm.tif"
//' #> [6] "lf_elev_220_metadata.html"
//' #> [7] "lf_elev_220_mt_hood_utm.tif"
//' #> [8] "lf_fbfm40_220_metadata.html"
//' #> [9] "lf_fbfm40_220_mt_hood_utm.tif"
//' }
// [[Rcpp::export(invisible = true)]]
bool vsi_sync(Rcpp::CharacterVector src,
		Rcpp::CharacterVector target,
		bool show_progess = false,
		Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

	std::string src_file_in;
	src_file_in = Rcpp::as<std::string>(_check_gdal_filename(src));
	std::string target_file_in;
	target_file_in = Rcpp::as<std::string>(_check_gdal_filename(target));
	
	GDALProgressFunc pfnProgress = NULL;
	if (show_progess)
		pfnProgress = GDALTermProgressR;
	
	std::vector<char *> opt_list = {NULL};
	if (options.isNotNull()) {
		Rcpp::CharacterVector options_in(options);
		opt_list.resize(options_in.size() + 1);
		for (R_xlen_t i = 0; i < options_in.size(); ++i) {
			opt_list[i] = (char *) (options_in[i]);
		}
		opt_list[options_in.size()] = NULL;
	}

	int result = VSISync(src_file_in.c_str(), target_file_in.c_str(),
			opt_list.data(), pfnProgress, NULL, NULL);

	return result;
}
