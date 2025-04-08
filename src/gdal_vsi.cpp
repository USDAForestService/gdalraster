/* GDAL VSI wrappers for virtual file system operations
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include <algorithm>

#include "gdal.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

#include "gdalraster.h"
#include "gdal_vsi.h"


//' Copy a source file to a target filename
//'
//' `vsi_copy_file()` is a wrapper for `VSICopyFile()` in the GDAL Common
//' Portability Library. The GDAL VSI functions allow virtualization of disk
//' I/O so that non file data sources can be made to appear as files.
//' See \url{https://gdal.org/en/stable/user/virtual_file_systems.html}.
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
//' @param show_progress Logical scalar. If `TRUE`, a progress bar will be
//' displayed (the size of `src_file` will be retrieved in GDAL with
//' `VSIStatL()`). Default is `FALSE`.
//' @returns `0` on success or `-1` on an error.
//'
//' @note
//' If `target_file` has the form /vsizip/foo.zip/bar, the default options
//' described for the function `addFilesInZip()` will be in effect.
//'
//' @seealso
//' [copyDatasetFiles()], [vsi_stat()], [vsi_sync()]
//'
//' @examples
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' tmp_file <- "/vsimem/elev_temp.tif"
//'
//' # Requires GDAL >= 3.7
//' if (gdal_version_num() >= gdal_compute_version(3, 7, 0)) {
//'   result <- vsi_copy_file(elev_file, tmp_file)
//'   (result == 0)
//'   print(vsi_stat(tmp_file, "size"))
//'
//'   vsi_unlink(tmp_file)
//' }
// [[Rcpp::export()]]
int vsi_copy_file(const Rcpp::CharacterVector &src_file,
                  const Rcpp::CharacterVector &target_file,
                  bool show_progress = false) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 7, 0)
    Rcpp::stop("vsi_copy_file() requires GDAL >= 3.7");

#else
    std::string src_file_in = Rcpp::as<std::string>(
            check_gdal_filename(src_file));

    std::string target_file_in = Rcpp::as<std::string>(
            check_gdal_filename(target_file));

    GDALProgressFunc pfnProgress = nullptr;
    if (show_progress)
        pfnProgress = GDALTermProgressR;

    int result = VSICopyFile(src_file_in.c_str(), target_file_in.c_str(),
                             nullptr, -1, nullptr, pfnProgress, nullptr);

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
//' /vsicurl/ (and related file systems like /vsis3/, /vsigs/, /vsiaz/,
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
//' @param quiet Logical scalar. `TRUE` (the default) to wrap the API call in
//' a quiet error handler, or `FALSE` to print any potential error messages to
//' the console.
//' @returns No return value, called for side effects.
//'
//' @examples
//' vsi_curl_clear_cache()
// [[Rcpp::export()]]
void vsi_curl_clear_cache(bool partial = false,
                          const Rcpp::CharacterVector &file_prefix = "",
                          bool quiet = true) {

    if (quiet)
        push_error_handler("quiet");

    if (!partial) {
        VSICurlClearCache();
    }
    else {
        std::string f_prefix_in = Rcpp::as<std::string>(
                check_gdal_filename(file_prefix));

        VSICurlPartialClearCache(f_prefix_in.c_str());
    }

    if (quiet)
        pop_error_handler();
}


//' Read names in a directory
//'
//' `vsi_read_dir()` abstracts access to directory contents. It returns a
//' character vector containing the names of files and directories in this
//' directory. With `recursive = TRUE`, reads the list of entries in the
//' directory and subdirectories.
//' This function is a wrapper for `VSIReadDirEx()` and `VSIReadDirRecursive()`
//' in the GDAL Common Portability Library.
//'
//' @param path Character string. The relative or absolute path of a
//' directory to read.
//' @param max_files Integer scalar. The maximum number of files after which to
//' stop, or 0 for no limit (see Note). Ignored if `recursive = TRUE`.
//' @param recursive Logical scalar. `TRUE` to read the directory and its
//' subdirectories. Defaults to `FALSE`.
//' @param all_files Logical scalar. If `FALSE` (the default), only the names
//' of visible files are returned (following Unix-style visibility, that is
//' files whose name does not start with a dot). If `TRUE`, all file names
//' will be returned.
//' @returns A character vector containing the names of files and directories
//' in the directory given by `path`. The listing is in alphabetical order, and
//' does not include the special entries '.' and '..' even if they are present
//' in the directory. An empty string (`""`) is returned if `path` does not
//' exist.
//'
//' @note
//' If `max_files` is set to a positive number, directory listing will stop
//' after that limit has been reached. Note that to indicate truncation, at
//' least one element more than the `max_files` limit will be returned. If the
//' length of the returned character vector is lesser or equal to `max_files`,
//' then no truncation occurred. The `max_files` parameter is ignored when
//' `recursive = TRUE`.
//'
//' @seealso
//' [vsi_mkdir()], [vsi_rmdir()], [vsi_stat()], [vsi_sync()]
//'
//' @examples
//' # regular file system for illustration
//' data_dir <- system.file("extdata", package="gdalraster")
//' vsi_read_dir(data_dir)
// [[Rcpp::export()]]
Rcpp::CharacterVector vsi_read_dir(const Rcpp::CharacterVector &path,
                                   int max_files = 0,
                                   bool recursive = false,
                                   bool all_files = false) {

    std::string path_in = Rcpp::as<std::string>(check_gdal_filename(path));

    char **papszFiles = nullptr;
    if (recursive)
        papszFiles = VSIReadDirRecursive(path_in.c_str());
    else
        papszFiles = VSIReadDirEx(path_in.c_str(), max_files);

    int nItems = CSLCount(papszFiles);
    if (nItems > 0) {
        std::vector<std::string> files{};
        for (int i=0; i < nItems; ++i) {
            if (!all_files && STARTS_WITH(papszFiles[i], "."))
                continue;
            if (!EQUAL(papszFiles[i], ".") && !EQUAL(papszFiles[i], "..")) {
                files.push_back(papszFiles[i]);
            }
            std::sort(files.begin(), files.end());
        }
        CSLDestroy(papszFiles);
        return Rcpp::wrap(files);
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
//' * `NUM_THREADS=integer`. Number of threads to use for parallel file
//' copying. Only use for when /vsis3/, /vsigs/, /vsiaz/ or /vsiadls/ is
//' in source or target. The default is 10 since GDAL 3.3.\cr
//' * `CHUNK_SIZE=integer`. Maximum size of chunk (in bytes) to use to split
//' large objects when downloading them from /vsis3/, /vsigs/, /vsiaz/
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
//' @param show_progress Logical scalar. If `TRUE`, a progress bar will be
//' displayed. Defaults to `FALSE`.
//' @param options Character vector of `NAME=VALUE` pairs (see Details).
//' @returns Logical scalar, `TRUE` on success or `FALSE` on an error.
//'
//' @seealso
//' [copyDatasetFiles()], [vsi_copy_file()]
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
//' result <- vsi_sync(src, dst, show_progress = TRUE)
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
// [[Rcpp::export()]]
bool vsi_sync(const Rcpp::CharacterVector &src,
              const Rcpp::CharacterVector &target,
              bool show_progress = false,
              const Rcpp::Nullable<Rcpp::CharacterVector>
                    &options = R_NilValue) {

    std::string src_file_in = Rcpp::as<std::string>(check_gdal_filename(src));

    std::string target_file_in = Rcpp::as<std::string>(
            check_gdal_filename(target));

    GDALProgressFunc pfnProgress = nullptr;
    if (show_progress)
        pfnProgress = GDALTermProgressR;

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) options_in[i];
        }
        opt_list[options_in.size()] = nullptr;
    }

    int result = VSISync(src_file_in.c_str(), target_file_in.c_str(),
                         opt_list.data(), pfnProgress, nullptr, nullptr);

    return result;
}


//' Create a directory
//'
//' `vsi_mkdir()` creates a new directory with the indicated mode.
//' For POSIX-style systems, the mode is modified by the file creation mask
//' (umask). However, some file systems and platforms may not use umask, or
//' they may ignore the mode completely. So a reasonable cross-platform
//' default mode value is `0755`.
//' With `recursive = TRUE`, creates a directory and all its ancestors.
//' This function is a wrapper for `VSIMkdir()` and `VSIMkdirRecursive()` in
//' the GDAL Common Portability Library.
//'
//' @param path Character string. The path to the directory to create.
//' @param mode Character string. The permissions mode in octal with prefix
//' `0`, e.g., `"0755"` (the default).
//' @param recursive Logical scalar. `TRUE` to create the directory and its
//' ancestors. Defaults to `FALSE`.
//' @returns `0` on success or `-1` on an error.
//'
//' @seealso
//' [vsi_read_dir()], [vsi_rmdir()]
//'
//' @examples
//' new_dir <- file.path(tempdir(), "newdir")
//' vsi_mkdir(new_dir)
//' vsi_stat(new_dir, "type")
//' vsi_rmdir(new_dir)
// [[Rcpp::export()]]
int vsi_mkdir(const Rcpp::CharacterVector &path,
              const std::string &mode = "0755",
              bool recursive = false) {

    std::string path_in = Rcpp::as<std::string>(check_gdal_filename(path));

    long mode_in = std::stol(mode, nullptr, 8);

    if (recursive)
        return VSIMkdirRecursive(path_in.c_str(), mode_in);
    else
        return VSIMkdir(path_in.c_str(), mode_in);
}


//' Delete a directory
//'
//' `vsi_rmdir()` deletes a directory object from the file system. On some
//' systems the directory must be empty before it can be deleted.
//' With `recursive = TRUE`, deletes a directory object and its content from
//' the file system.
//' This function goes through the GDAL `VSIFileHandler` virtualization and may
//' work on unusual filesystems such as in memory.
//' It is a wrapper for `VSIRmdir()` and `VSIRmdirRecursive()` in the GDAL
//' Common Portability Library.
//'
//' @param path Character string. The path to the directory to be deleted.
//' @param recursive Logical scalar. `TRUE` to delete the directory and its
//' content. Defaults to `FALSE`.
//' @returns `0` on success or `-1` on an error.
//'
//' @note
//' /vsis3/ has an efficient implementation for deleting recursively. Starting
//' with GDAL 3.4, /vsigs/ has an efficient implementation for deleting
//' recursively, provided that OAuth2 authentication is used.
//'
//' @seealso
//' [deleteDataset()], [vsi_mkdir()], [vsi_read_dir()], [vsi_unlink()]
//'
//' @examples
//' new_dir <- file.path(tempdir(), "newdir")
//' vsi_mkdir(new_dir)
//' vsi_rmdir(new_dir)
// [[Rcpp::export()]]
int vsi_rmdir(const Rcpp::CharacterVector &path, bool recursive = false) {

    std::string path_in = Rcpp::as<std::string>(check_gdal_filename(path));

    if (recursive)
        return VSIRmdirRecursive(path_in.c_str());
    else
        return VSIRmdir(path_in.c_str());
}


//' Delete a file
//'
//' `vsi_unlink()` deletes a file object from the file system.
//' This function goes through the GDAL `VSIFileHandler` virtualization and may
//' work on unusual filesystems such as in memory.
//' It is a wrapper for `VSIUnlink()` in the GDAL Common Portability Library.
//' Analog of the POSIX `unlink()` function.
//'
//' @param filename Character string. The path of the file to be deleted.
//' @returns `0` on success or `-1` on an error.
//'
//' @seealso
//' [deleteDataset()], [vsi_rmdir()], [vsi_unlink_batch()]
//'
//' @examples
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' mem_file <- file.path("/vsimem", "tmp.tif")
//' copyDatasetFiles(mem_file, elev_file)
//' vsi_read_dir("/vsimem")
//' vsi_unlink(mem_file)
//' vsi_read_dir("/vsimem")
// [[Rcpp::export()]]
int vsi_unlink(const Rcpp::CharacterVector &filename) {

    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(filename));

    return VSIUnlink(filename_in.c_str());
}


//' Delete several files in a batch
//'
//' `vsi_unlink_batch()` deletes a list of files passed in a character vector.
//' All files should belong to the same file system handler.
//' This is implemented efficiently for /vsis3/ and /vsigs/ (provided for
//' /vsigs/ that OAuth2 authentication is used).
//' This function is a wrapper for `VSIUnlinkBatch()` in the GDAL Common
//' Portability Library.
//'
//' @param filenames Character vector. The list of files to delete.
//' @returns Logical vector of `length(filenames)` with values depending
//' on the success of deletion of the corresponding file.
//' `NULL` might be returned in case of a more general error (for example,
//' files belonging to different file system handlers).
//'
//' @seealso
//' [deleteDataset()], [vsi_rmdir()], [vsi_unlink()]
//'
//' @examples
//' # regular file system for illustration
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' tcc_file <- system.file("extdata/storml_tcc.tif", package="gdalraster")
//'
//' tmp_elev <- file.path(tempdir(), "tmp_elev.tif")
//' file.copy(elev_file, tmp_elev)
//' tmp_tcc <- file.path(tempdir(), "tmp_tcc.tif")
//' file.copy(tcc_file, tmp_tcc)
//' vsi_unlink_batch(c(tmp_elev, tmp_tcc))
// [[Rcpp::export()]]
SEXP vsi_unlink_batch(const Rcpp::CharacterVector &filenames) {

    if (filenames.size() == 0)
        return R_NilValue;

    R_xlen_t nFiles = filenames.size();
    std::vector<std::string> filenames_in(nFiles);
    std::vector<const char *> filenames_cstr(nFiles + 1);
    for (R_xlen_t i = 0; i < nFiles; ++i) {
        filenames_in[i] = Rcpp::as<std::string>(check_gdal_filename(
                Rcpp::as<Rcpp::CharacterVector>(filenames[i])));

        filenames_cstr[i] = filenames_in[i].c_str();
    }
    filenames_cstr[nFiles] = nullptr;

    int *result = VSIUnlinkBatch(filenames_cstr.data());

    if (result == nullptr) {
        return R_NilValue;
    }
    else {
        Rcpp::LogicalVector ret(nFiles);
        for (R_xlen_t i = 0; i < nFiles; ++i)
            ret[i] = result[i];
        VSIFree(result);
        return ret;
    }
}


//' Get filesystem object info
//'
//' `vsi_stat()` fetches status information about a filesystem object (file,
//' directory, etc).
//' This function goes through the GDAL `VSIFileHandler` virtualization and may
//' work on unusual filesystems such as in memory.
//' It is a wrapper for `VSIStatExL()` in the GDAL Common Portability Library.
//' Analog of the POSIX `stat()` function.
//'
//' @param filename Character string. The path of the filesystem object to be
//' queried.
//' @param info Character string. The type of information to fetch, one of
//' `"exists"` (the default), `"type"` or `"size"`.
//' @returns If `info = "exists"`, returns logical `TRUE` if the file system
//' object exists, otherwise `FALSE`. If `info = "type"`, returns a character
//' string with one of `"file"` (regular file), `"dir"` (directory),
//' `"symlink"` (symbolic link), or empty string (`""`). If `info = "size"`,
//' returns the file size in bytes (as `bit64::integer64` type), or `-1` if an
//' error occurs.
//'
//' @note
//' For portability, `vsi_stat()` supports a subset of `stat()`-type
//' information for filesystem objects. This function is primarily intended
//' for use with GDAL virtual file systems (e.g., URLs, cloud storage systems,
//' ZIP/GZip/7z/RAR archives, in-memory files).
//' The base R function `utils::file_test()` could be used instead for file
//' tests on regular local filesystems.
//'
//' @seealso
//' GDAL Virtual File Systems:\cr
//' \url{https://gdal.org/en/stable/user/virtual_file_systems.html}
//'
//' @examples
//' data_dir <- system.file("extdata", package="gdalraster")
//' vsi_stat(data_dir)
//' vsi_stat(data_dir, "type")
//' # stat() on a directory doesn't return the sum of the file sizes in it,
//' # but rather how much space is used by the directory entry
//' vsi_stat(data_dir, "size")
//'
//' elev_file <- file.path(data_dir, "storml_elev.tif")
//' vsi_stat(elev_file)
//' vsi_stat(elev_file, "type")
//' vsi_stat(elev_file, "size")
//'
//' nonexistent <- file.path(data_dir, "nonexistent.tif")
//' vsi_stat(nonexistent)
//' vsi_stat(nonexistent, "type")
//' vsi_stat(nonexistent, "size")
//'
//' # /vsicurl/ file system handler
//' base_url <- "https://raw.githubusercontent.com/usdaforestservice/"
//' f <- "gdalraster/main/sample-data/landsat_c2ard_sr_mt_hood_jul2022_utm.tif"
//' url_file <- paste0("/vsicurl/", base_url, f)
//'
//' # try to be CRAN-compliant for the example:
//' set_config_option("GDAL_HTTP_CONNECTTIMEOUT", "10")
//' set_config_option("GDAL_HTTP_TIMEOUT", "10")
//'
//' vsi_stat(url_file)
//' vsi_stat(url_file, "type")
//' vsi_stat(url_file, "size")
// [[Rcpp::export()]]
SEXP vsi_stat(const Rcpp::CharacterVector &filename,
              const std::string &info = "exists") {

    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(filename));

    const char *fn = filename_in.c_str();
    VSIStatBufL sStat;

    if (EQUALN(info.c_str(), "exists", 6)) {
        bool ret = false;
        if (VSIStatExL(fn, &sStat, VSI_STAT_EXISTS_FLAG) == 0)
            ret = true;

        return Rcpp::LogicalVector(Rcpp::wrap(ret));
    }
    else if (EQUALN(info.c_str(), "type", 4)) {
        std::string ret = "";
        if (VSIStatExL(fn, &sStat, VSI_STAT_NATURE_FLAG) == 0) {
            if (VSI_ISDIR(sStat.st_mode))
                ret = "dir";
            else if (VSI_ISLNK(sStat.st_mode))
                ret = "symlink";
            else if (VSI_ISREG(sStat.st_mode))
                ret = "file";
        }

        return Rcpp::CharacterVector(Rcpp::wrap(ret));
    }
    else if (EQUALN(info.c_str(), "size", 4)) {
        std::vector<int64_t> ret(1);
        if (VSIStatExL(fn, &sStat, VSI_STAT_SIZE_FLAG) == 0)
            ret[0] = static_cast<int64_t>(sStat.st_size);
        else
            ret[0] = -1;

        return Rcpp::NumericVector(Rcpp::wrap(ret));
    }
    else {
        Rcpp::stop("invalid value for 'info'");
    }
}


//' Rename a file
//'
//' `vsi_rename()` renames a file object in the file system. The GDAL
//' documentation states it should be possible to rename a file onto a new
//' filesystem, but it is safest if this function is only used to rename files
//' that remain in the same directory.
//' This function goes through the GDAL `VSIFileHandler` virtualization and may
//' work on unusual filesystems such as in memory.
//' It is a wrapper for `VSIRename()` in the GDAL Common Portability Library.
//' Analog of the POSIX `rename()` function.
//'
//' @param oldpath Character string. The name of the file to be renamed.
//' @param newpath Character string. The name the file should be given.
//' @returns `0` on success or `-1` on an error.
//'
//' @seealso
//' [renameDataset()], [vsi_copy_file()]
//'
//' @examples
//' # regular file system for illustration
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' tmp_file <- tempfile(fileext = ".tif")
//' file.copy(elev_file, tmp_file)
//' new_file <- file.path(dirname(tmp_file), "storml_elev_copy.tif")
//' vsi_rename(tmp_file, new_file)
//' vsi_stat(new_file)
//' vsi_unlink(new_file)
// [[Rcpp::export()]]
int vsi_rename(const Rcpp::CharacterVector &oldpath,
               const Rcpp::CharacterVector &newpath) {

    std::string oldpath_in = Rcpp::as<std::string>(
            check_gdal_filename(oldpath));

    std::string newpath_in = Rcpp::as<std::string>(
            check_gdal_filename(newpath));

    return VSIRename(oldpath_in.c_str(), newpath_in.c_str());
}


//' Return the list of virtual file system handlers currently registered
//'
//' `vsi_get_fs_prefixes()` returns the list of prefixes for virtual file
//' system handlers currently registered (e.g., `"/vsimem/"`, `"/vsicurl/"`,
//' etc). Wrapper for `VSIGetFileSystemsPrefixes()` in the GDAL API.
//'
//' @returns Character vector containing prefixes of the virtual file system
//' handlers.
//'
//' @seealso
//' [vsi_get_fs_options()]
//'
//' \url{https://gdal.org/en/stable/user/virtual_file_systems.html}
//'
//' @examples
//' vsi_get_fs_prefixes()
// [[Rcpp::export()]]
Rcpp::CharacterVector vsi_get_fs_prefixes() {
    char **papszPrefixes = VSIGetFileSystemsPrefixes();
    int nItems = CSLCount(papszPrefixes);
    if (nItems > 0) {
        Rcpp::CharacterVector prefixes(nItems);
        for (int i=0; i < nItems; ++i) {
            prefixes(i) = papszPrefixes[i];
        }
        CSLDestroy(papszPrefixes);
        return prefixes;
    }
    else {
        CSLDestroy(papszPrefixes);
        return "";
    }
}


//' Return the list of options associated with a virtual file system handler
//' as a serialized XML string.
//'
//' Called from and documented in R/gdal_helpers.R
//' @noRd
// [[Rcpp::export(name = ".vsi_get_fs_options")]]
std::string vsi_get_fs_options_(const Rcpp::CharacterVector &filename) {

    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(filename));

    if (VSIGetFileSystemOptions(filename_in.c_str()) != nullptr)
        return VSIGetFileSystemOptions(filename_in.c_str());
    else
        return "";
}


//' Return whether the filesystem supports sequential write
//'
//' `vsi_supports_seq_write()` returns whether the filesystem supports
//' sequential write.
//' Wrapper for `VSISupportsSequentialWrite()` in the GDAL API.
//'
//' @param filename Character string. The path of the filesystem object to be
//' tested.
//' @param allow_local_tmpfile Logical scalar. `TRUE` if the filesystem is
//' allowed to use a local temporary file before uploading to the target
//' location.
//' @returns Logical scalar. `TRUE` if sequential write is supported.
//'
//' @note
//' The location GDAL uses for temporary files can be forced via the
//' `CPL_TMPDIR` configuration option.
//'
//' @seealso
//' [vsi_supports_rnd_write()]
//'
//' @examples
//' # Requires GDAL >= 3.6
//' if (gdal_version_num() >= gdal_compute_version(3, 6, 0))
//'   vsi_supports_seq_write("/vsimem/test-mem-file.gpkg", TRUE)
// [[Rcpp::export()]]
bool vsi_supports_seq_write(const Rcpp::CharacterVector &filename,
                            bool allow_local_tmpfile) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 6, 0)
    Rcpp::stop("vsi_supports_seq_write() requires GDAL >= 3.6");

#else
    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(filename));

    if (VSISupportsSequentialWrite(filename_in.c_str(), allow_local_tmpfile))
        return true;
    else
        return false;

#endif
}


//' Return whether the filesystem supports random write
//'
//' `vsi_supports_rnd_write()` returns whether the filesystem supports
//' random write.
//' Wrapper for `VSISupportsRandomWrite()` in the GDAL API.
//'
//' @param filename Character string. The path of the filesystem object to be
//' tested.
//' @param allow_local_tmpfile Logical scalar. `TRUE` if the filesystem is
//' allowed to use a local temporary file before uploading to the target
//' location.
//' @returns Logical scalar. `TRUE` if random write is supported.
//'
//' @note
//' The location GDAL uses for temporary files can be forced via the
//' `CPL_TMPDIR` configuration option.
//'
//' @seealso
//' [vsi_supports_seq_write()]
//'
//' @examples
//' # Requires GDAL >= 3.6
//' if (gdal_version_num() >= gdal_compute_version(3, 6, 0))
//'   vsi_supports_rnd_write("/vsimem/test-mem-file.gpkg", TRUE)
// [[Rcpp::export()]]
bool vsi_supports_rnd_write(const Rcpp::CharacterVector &filename,
                            bool allow_local_tmpfile) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 6, 0)
    Rcpp::stop("vsi_supports_rnd_write() requires GDAL >= 3.6");

#else
    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(filename));

    if (VSISupportsRandomWrite(filename_in.c_str(), allow_local_tmpfile))
        return true;
    else
        return false;

#endif
}

//' Return free disk space available on the filesystem
//'
//' `vsi_get_disk_free_space()` returns the free disk space available on the
//' filesystem. Wrapper for `VSIGetDiskFreeSpace()` in the GDAL Common
//' Portability Library.
//'
//' @param path Character string. A directory of the filesystem to query.
//' @returns Numeric scalar. The free space in bytes (as `bit64::integer64`
//' type), or `-1` in case of error.
//'
//' @examples
//' tmp_dir <- file.path("/vsimem", "tmpdir")
//' vsi_mkdir(tmp_dir)
//' vsi_get_disk_free_space(tmp_dir)
//' vsi_rmdir(tmp_dir)
// [[Rcpp::export()]]
Rcpp::NumericVector vsi_get_disk_free_space(const Rcpp::CharacterVector &path) {

    std::string path_in = Rcpp::as<std::string>(check_gdal_filename(path));
    std::vector<int64_t> ret(1);
    ret[0] = VSIGetDiskFreeSpace(path_in.c_str());
    return Rcpp::wrap(ret);
}

//' Set a path specific option for a given path prefix
//'
//' `vsi_set_path_option()` sets a path specific option for a given path
//' prefix. Such an option is typically, but not limited to, setting
//' credentials for a virtual file system.
//' Wrapper for `VSISetPathSpecificOption()` in the GDAL Common Portability
//' Library. Requires GDAL >= 3.6.
//'
//' @details
//' Options may also be set with `set_config_option()`, but
//' `vsi_set_path_option()` allows specifying them with a granularity at the
//' level of a file path. This makes it easier if using the same virtual file
//' system but with different credentials (e.g., different credentials for
//' buckets "/vsis3/foo" and "/vsis3/bar"). This is supported for the following
//' virtual file systems: /vsis3/, /vsigs/, /vsiaz/, /vsioss/, /vsiwebhdfs,
//' /vsiswift.
//'
//' @param path_prefix Character string. A path prefix of a virtual file system
//' handler. Typically of the form `/vsiXXX/bucket`.
//' @param key Character string. Option key.
//' @param value Character string. Option value. Passing `value = ""` (empty
//' string) will unset a value previously set by `vsi_set_path_option()`.
//' @returns No return value, called for side effect.
//'
//' @note
//' Setting options for a path starting with /vsiXXX/ will also apply for
//' /vsiXXX_streaming/ requests.
//'
//' No particular care is taken to store options in RAM in a secure way.
//' So they might accidentally hit persistent storage if swapping occurs,
//' or someone with access to the memory allocated by the process may be
//' able to read them.
//'
//' @seealso
//' [set_config_option()], [vsi_clear_path_options()]
// [[Rcpp::export()]]
void vsi_set_path_option(const Rcpp::CharacterVector &path_prefix,
                         const std::string &key,
                         const std::string &value) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 6, 0)
    Rcpp::stop("vsi_set_path_option() requires GDAL >= 3.6");

#else
    std::string path_prefix_in = Rcpp::as<std::string>(
            check_gdal_filename(path_prefix));

    const char* value_in = nullptr;
    if (value != "")
        value_in = value.c_str();

    VSISetPathSpecificOption(path_prefix_in.c_str(), key.c_str(), value_in);

#endif
}

//' Clear path specific configuration options
//'
//' `vsi_clear_path_options()` clears path specific options previously set
//' with `vsi_set_path_option()`.
//' Wrapper for `VSIClearPathSpecificOptions()` in the GDAL Common Portability
//' Library. Requires GDAL >= 3.6.
//'
//' @param path_prefix Character string. If set to `""` (empty string), all
//' path specific options are cleared. If set to a path prefix, only those
//' options set with `vsi_set_path_option(path_prefix, ...)` will be cleared.
//' @returns No return value, called for side effect.
//'
//' @note
//' No particular care is taken to remove options from RAM in a secure way.
//'
//' @seealso
//' [vsi_set_path_option()]
// [[Rcpp::export()]]
void vsi_clear_path_options(const Rcpp::CharacterVector &path_prefix) {
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 6, 0)
    Rcpp::stop("vsi_clear_path_options() requires GDAL >= 3.6");

#else
    std::string path_prefix_in = Rcpp::as<std::string>(
            check_gdal_filename(path_prefix));

    const char* path_cstr = nullptr;
    if (path_prefix_in != "")
        path_cstr = path_prefix_in.c_str();

    VSIClearPathSpecificOptions(path_cstr);

#endif
}

//' Get metadata on files
//'
//' `vsi_get_file_metadata()` returns metadata for file system objects.
//' Implemented for network-like filesystems. Starting with GDAL 3.7,
//' implemented for /vsizip/ with SOZip metadata.
//' Wrapper of `VSIGetFileMetadata()` in the GDAL Common Portability Library.
//'
//' @details
//' The metadata available depends on the file system. The following are
//' supported as of GDAL 3.9:
//'   * HEADERS: to get HTTP headers for network-like filesystems (/vsicurl/,
//'     /vsis3/, /vsgis/, etc).
//'   * TAGS: for /vsis3/, to get S3 Object tagging information. For /vsiaz/,
//'     to get blob tags.
//'   * STATUS: specific to /vsiadls/: returns all system-defined properties
//'     for a path (seems in practice to be a subset of HEADERS).
//'   * ACL: specific to /vsiadls/ and /vsigs/: returns the access control list
//'     for a path. For /vsigs/, a single `XML=xml_content` string is returned.
//'   * METADATA: specific to /vsiaz/: blob metadata (this will be a subset of
//'     what `domain=HEADERS` returns).
//'   * ZIP: specific to /vsizip/: to obtain ZIP specific metadata, in
//'     particular if a file is SOZIP-enabled (`SOZIP_VALID=YES`).
//'
//' @param filename Character string. The path of the file system object to be
//' queried.
//' @param domain Character string. Metadata domain to query. Depends on the
//' file system, see Details.
//' @returns A named list of values, or `NULL` in case of error or empty list.
//'
//' @seealso
//' [vsi_stat()], [addFilesInZip()]
//'
//' @examplesIf gdal_version_num() >= gdal_compute_version(3, 7, 0)
//' # validate an SOZip-enabled file
//' # Requires GDAL >= 3.7
//' f <- system.file("extdata/ynp_features.zip", package = "gdalraster")
//'
//' zf <- file.path("/vsizip", f)
//' # files in zip archive
//' vsi_read_dir(zf)
//'
//' # SOZip metadata for ynp_features.gpkg
//' zf_gpkg <- file.path(zf, "ynp_features.gpkg")
//' vsi_get_file_metadata(zf_gpkg, domain = "ZIP")
// [[Rcpp::export()]]
SEXP vsi_get_file_metadata(const Rcpp::CharacterVector &filename,
                           const std::string &domain) {

    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(filename));

    char **papszStringList = nullptr;
    papszStringList = VSIGetFileMetadata(filename_in.c_str(), domain.c_str(),
                                         nullptr);

    if (papszStringList == nullptr) {
        return R_NilValue;
    }
    else {
        int nItems = CSLCount(papszStringList);
        Rcpp::List md = Rcpp::List::create();
        for (int i = 0; i < nItems; ++i) {
            char *pszName = nullptr;
            Rcpp::CharacterVector value(1);
            const char *pszValue = CPLParseNameValue(papszStringList[i],
                                                     &pszName);
            if (pszName && pszValue) {
                value[0] = pszValue;
                md.push_back(value, pszName);
            }
            CPLFree(pszName);
        }
        CSLDestroy(papszStringList);
        return md;
    }
}

//' Returns the actual URL of a supplied VSI filename
//'
//' `vsi_get_actual_url()` returns the actual URL of a supplied filename.
//' Currently only returns a non-NULL value for network-based virtual file
//' systems. For example "/vsis3/bucket/filename" will be expanded as
//' "https://bucket.s3.amazon.com/filename".
//' Wrapper for `VSIGetActualURL()` in the GDAL API.
//'
//' @param filename Character string containing a /vsiPREFIX/ filename.
//' @returns Character string containing the actual URL, or `NULL` if
//' `filename` is not a network-based virtual file system.
//'
//' @seealso
//' [vsi_get_signed_url()]
//'
//' @examples
//' \dontrun{
//' f <- "/vsiaz/items/io-lulc-9-class.parquet"
//' set_config_option("AZURE_STORAGE_ACCOUNT", "pcstacitems")
//' # token obtained from:
//' # https://planetarycomputer.microsoft.com/api/sas/v1/token/pcstacitems/items
//' set_config_option("AZURE_STORAGE_SAS_TOKEN","<token>")
//' vsi_get_actual_url(f)
//' #> [1] "https://pcstacitems.blob.core.windows.net/items/io-lulc-9-class.parquet"
//' vsi_get_signed_url(f)
//' #> [1] "https://pcstacitems.blob.core.windows.net/items/io-lulc-9-class.parquet?<token>"
//' }
// [[Rcpp::export()]]
SEXP vsi_get_actual_url(const Rcpp::CharacterVector &filename) {

    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(filename));

    if (VSIGetActualURL(filename_in.c_str()) != nullptr)
        return Rcpp::wrap(VSIGetActualURL(filename_in.c_str()));
    else
        return R_NilValue;
}

//' Returns a signed URL for a supplied VSI filename
//'
//' `vsi_get_signed_url()` Returns a signed URL of a supplied filename.
//' Currently only returns a non-NULL value for /vsis3/, /vsigs/, /vsiaz/ and
//' /vsioss/ For example "/vsis3/bucket/filename" will be expanded as
//' "https://bucket.s3.amazon.com/filename?X-Amz-Algorithm=AWS4-HMAC-SHA256..."
//' Configuration options that apply for file opening (typically to provide
//' credentials), and are returned by `vsi_get_fs_options()`, are also valid
//' in that context.
//' Wrapper for `VSIGetSignedURL()` in the GDAL API.
//'
//' @details
//' The `options` argument accepts a character vector of name=value pairs.
//' For /vsis3/, /vsigs/, /vsiaz/ and /vsioss/, the following options are
//' supported:
//' * `START_DATE=YYMMDDTHHMMSSZ`: date and time in UTC following ISO 8601
//'   standard, corresponding to the start of validity of the URL. If not
//'   specified, current date time.
//' * `EXPIRATION_DELAY=number_of_seconds`: number between 1 and 604800 (seven
//'   days) for the validity of the signed URL. Defaults to 3600 (one hour).
//' * `VERB=GET/HEAD/DELETE/PUT/POST`: HTTP VERB for which the request will be
//'   used. Defaults to `GET`.
//'
//' /vsiaz/ supports additional options:
//' * `SIGNEDIDENTIFIER=value`: to relate the given shared access signature to
//'   a corresponding stored access policy.
//' * `SIGNEDPERMISSIONS=r|w`: permissions associated with the shared access
//'   signature. Normally deduced from `VERB`.
//'
//' @param filename Character string containing a /vsiPREFIX/ filename.
//' @param options Character vector of `NAME=VALUE` pairs (see Details).
//' @returns Character string containing the signed URL, or `NULL` if
//' `filename` is not a network-based virtual file system.
//'
//' @seealso
//' [vsi_get_actual_url()]
//'
//' @examples
//' \dontrun{
//' f <- "/vsiaz/items/io-lulc-9-class.parquet"
//' set_config_option("AZURE_STORAGE_ACCOUNT", "pcstacitems")
//' # token obtained from:
//' # https://planetarycomputer.microsoft.com/api/sas/v1/token/pcstacitems/items
//' set_config_option("AZURE_STORAGE_SAS_TOKEN", "<token>")
//' vsi_get_actual_url(f)
//' #> [1] "https://pcstacitems.blob.core.windows.net/items/io-lulc-9-class.parquet"
//' vsi_get_signed_url(f)
//' #> [1] "https://pcstacitems.blob.core.windows.net/items/io-lulc-9-class.parquet?<token>"
//' }
// [[Rcpp::export()]]
SEXP vsi_get_signed_url(const Rcpp::CharacterVector &filename,
                        const Rcpp::Nullable<Rcpp::CharacterVector>
                                &options = R_NilValue) {

    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(filename));

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) options_in[i];
        }
        opt_list[options_in.size()] = nullptr;
    }

    char *pszSignedURL = VSIGetSignedURL(filename_in.c_str(), opt_list.data());
    std::string url = "";
    if (pszSignedURL != nullptr) {
        url = pszSignedURL;
        VSIFree(pszSignedURL);
        return Rcpp::wrap(url);
    }
    else {
        return R_NilValue;
    }
}

//' Returns if the file/filesystem is "local".
//'
//' `vsi_is_local()` returns whether the file/filesystem is "local".
//' Wrapper for `VSIIsLocal()` in the GDAL API. Requires GDAL >= 3.6.
//'
//' @param filename Character string. The path of the filesystem object to be
//' tested.
//' @returns Logical scalar. `TRUE` if if the input file path is local.
//'
//' @note
//' The concept of local is mostly by opposition with a network / remote file
//' system whose access time can be long.
//'
//' /vsimem/ is considered to be a local file system, although a
//' non-persistent one.
//'
//' @examples
//' # Requires GDAL >= 3.6
//' if (gdal_version_num() >= gdal_compute_version(3, 6, 0))
//'   print(vsi_is_local("/vsimem/test-mem-file.tif"))
// [[Rcpp::export()]]
bool vsi_is_local(const Rcpp::CharacterVector &filename) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 6, 0)
    Rcpp::stop("vsi_is_local() requires GDAL >= 3.6");

#else
    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(filename));

    return VSIIsLocal(filename_in.c_str());
#endif
}
