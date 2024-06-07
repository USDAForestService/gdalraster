#' Constants for VSIFile$seek()
#'
#' These are package global constants for convenience in calling
#' `VSIFile$seek()`.
#'
#' @name vsi_constants
#' @export
SEEK_SET <- "SEEK_SET"
#' @name vsi_constants
#' @export
SEEK_CUR <- "SEEK_CUR"
#' @name vsi_constants
#' @export
SEEK_END <- "SEEK_END"

#' @name VSIFile-class
#'
#' @aliases
#' Rcpp_VSIFile Rcpp_VSIFile-class VSIFile
#'
#' @title Class wrapping the GDAL VSIVirtualHandle API for binary file I/O
#' @description
#' `VSIFile` provides bindings to the GDAL VSIVirtualHandle API. Encapsulates a
#' `VSIVirtualHandle`
#' (\url{https://gdal.org/api/cpl_cpp.html#_CPPv416VSIVirtualHandle}).
#' This API abstracts binary file I/O across "regular" file systems, URLs,
#' cloud storage services, Zip/GZip/7z/RAR, and in-memory files.
#' It provides analogs of several Standard C file I/O functions, allowing
#' virtualization of disk I/O so that non-file data sources can be made to
#' appear as files.
#'
#' @param filename Character string containing the filename to open. It may be
#' a file in a regular local filesystem, or a filename with a GDAL /vsiPREFIX/
#' (see \url{https://gdal.org/user/virtual_file_systems.html}).
#' @param access Character string containing the access requested (i.e., `"r"`,
#' `"r+"`, `"w"`, `"w+`). Defaults to `"r"`. Binary access is always implied
#' and the "b" does not need to be included in `access`.
#' \tabular{lll}{
#'  **Access** \tab **Explanation**             \tab **If file exists**\cr
#'  `"r"`      \tab open file for reading       \tab read from start\cr
#'  `"r+"`     \tab open file for read/write    \tab read from start\cr
#'  `"w"`      \tab create file for writing     \tab destroy contents\cr
#'  `"w+"`     \tab create file for read/write  \tab destroy contents
#' }
#' @param options Optional character vector of `NAME=VALUE` pairs specifying
#' filesystem-dependent options (GDAL >= 3.3, see Details).
#' @returns An object of class `VSIFile` which contains a pointer to a
#' `VSIVirtualHandle`, and methods that operate on the file as described in
#' Details. `VSIFile` is a C++ class exposed directly to R (via
#' `RCPP_EXPOSED_CLASS`). Methods of the class are accessed using the
#' `$` operator.
#'
#' @section Usage:
#' \preformatted{
#' ## Constructors
#' vf <- new(VSIFile, filename)
#' # specifying access:
#' vf <- new(VSIFile, filename, access)
#' # specifying access and options (both required):
#' vf <- new(VSIFile, filename, access, options)
#'
#' ## Methods (see Details)
#' vf$seek(offset, origin)
#' vf$tell()
#' vf$rewind()
#' vf$read(nbytes)
#' vf$write(object)
#' vf$eof()
#' vf$truncate(new_size)
#' vf$flush()
#' vf$ingest(max_size)
#'
#' vf$close()
#' vf$open()
#' vf$get_filename()
#' vf$get_access()
#' vf$set_access(access)
#' }
#' @section Details:
#' \code{new(VSIFile, filename)}
#' Constructor. Returns an object of class `VSIFile`, or an error is raised
#' if a file handle cannot be obtained.
#'
#' \code{new(VSIFile, filename, access)}
#' Alternate constructor for passing `access` as a character string
#' (e.g., `"r"`, `"r+"`, `"w"`, `"w+"`).
#' Returns an object of class `VSIFile` with an open file handle, or an error
#' is raised if a file handle cannot be obtained.
#'
#' \code{new(VSIFile, filename, access, options)}
#' Alternate constructor for passing `access` as a character string, and
#' `options` as a character vector of "NAME=VALUE" pairs (both arguments
#' required, GDAL >= 3.3 required for `options` support).
#' Returns an object of class `VSIFile` with an open file handle, or an error
#' is raised if a file handle cannot be obtained.
#'
#' The `options` argument is highly file system dependent. Supported options
#' as of GDAL 3.9 include:
#' * MIME headers such as Content-Type and Content-Encoding are supported for
#' the /vsis3/, /vsigs/, /vsiaz/, /vsiadls/ file systems.
#' * DISABLE_READDIR_ON_OPEN=YES/NO (GDAL >= 3.6) for /vsicurl/ and other
#' network-based file systems. By default, directory file listing is done,
#' unless YES is specified.
#' * WRITE_THROUGH=YES (GDAL >= 3.8) for Windows regular files to set the
#' FILE_FLAG_WRITE_THROUGH flag to the `CreateFile()` function. In that mode,
#' the data are written to the system cache but are flushed to disk without
#' delay.
#'
#' \code{$seek(offset, origin)}
#' Seek to a requested `offset` in the file.
#' `offset` is given as a positive numeric scalar, optionally as
#' `bit64::integer64` type.
#' `origin` is given as a character string, one of `SEEK_SET`, `SEEK_CUR` or
#' `SEEK_END`. Package global constants are defined for convenience, so these
#' can be passed unquoted. Note that `offset` is an unsigned type, so `SEEK_CUR`
#' can only be used for positive seek. If negative seek is needed, use:
#' ```
#' vf$seek(vf$tell() + negative_offset, SEEK_SET)
#' ```
#' Returns `0` on success or `-1` on failure.
#'
#' \code{$tell()}
#' Returns the current file read/write offset in bytes from the beginning of
#' the file. The return value is a numeric scalar carrying the `integer64`
#' class attribute.
#'
#' \code{$rewind()}
#' Rewind the file pointer to the beginning of the file. This is equivalent to
#' `vf$seek(0, SEEK_SET)`. No return value, called for that side effect.
#'
#' \code{$read(nbytes)}
#' Read `nbytes` bytes from the file at the current offset. Returns a vector
#' of R `raw` type, or `NULL` if the operation fails.
#'
#' \code{$write(object)}
#' Write bytes to the file at the current offset. `object` is a `raw` vector.
#' Returns the number of bytes successfully written, as numeric scalar
#' carrying the `integer64` class attribute.
#' See also base R `charToRaw()` / `rawToChar()`, convert to or from raw
#' vectors, and `readBin()` / `writeBin()` which read binary data from or write
#' binary data to a raw vector.
#'
#' \code{$eof()}
#' Test for end of file. Returns `TRUE` if an end-of-file condition occurred
#' during the previous read operation. The end-of-file flag is cleared by a
#' successful call to `$seek()`.
#'
#' \code{$truncate(new_size)}
#' Truncate/expand the file to the specified `new_size`, given as a positive
#' numeric scalar, optionally as `bit64::integer64` type.
#' Returns `0` on success.
#'
#' \code{$flush()}
#' Flush pending writes to disk. For files in write or update mode and on
#' file system types where it is applicable, all pending output on the file is
#' flushed to the physical disk.
#' On Windows regular files, this method does nothing, unless the
#' `VSI_FLUSH=YES` configuration option is set (and only when the file has not
#' been opened with the `WRITE_THROUGH` option).
#' Returns `0` on success or `-1` on error.
#'
#' \code{$ingest(max_size)}
#' Ingest a file into memory. Read the whole content of the file into a `raw`
#' vector.
#' `max_size` is the maximum size of file allowed, given as a numeric scalar,
#' optionally as `bit64::integer64` type. If no limit, set to a negative value.
#' Returns a `raw` vector, or `NULL` if the operation fails.
#'
#' \code{$close()}
#' Closes the file. The file should always be closed when I/O has been
#' completed. Returns `0` on success or `-1` on error.
#'
#' \code{$open()}
#' This method can be used to re-open the file after it has been closed, using
#' the same `filename`, and same `options` if any are set. The file will be
#' opened using `access` as currently set. The `$set_access()` method can be
#' called to change the requested access while the file is closed.
#' No return value. An error is raised if a file handle cannot be obtained.
#'
#' \code{$get_filename()}
#' Returns a character string containing the `filename` associated with this
#' `VSIFile` object (the `filename` originally used to create the object).
#'
#' \code{$get_access()}
#' Returns a character string containing the `access` as currently set on this
#' `VSIFile` object.
#'
#' \code{$set_access(access)}
#' Sets the requested read/write access on this `VSIFile` object, given as a
#' character string (i.e., `"r"`, `"r+"`, `"w"`, `"w+"`). The access can be
#' changed only while the `VSIFile` object is closed, and will apply when it is
#' re-opened with a call to `$open()`.
#' Returns `0` on success or `-1` on error.
#'
#' @note
#' File offsets are given as R `numeric` (i.e., `double` type), optionally
#' carrying the `bit64::integer64` class attribute. They are returned as
#' `numeric` with the `integer64` class attribute attached. The `integer64`
#' type is signed, so the maximum file offset supported by this interface
#' is `9223372036854775807` (the value of `bit64::lim.integer64()[2]`).
#'
#' Some virtual file systems allow only sequential write, so no seeks or read
#' operations are then allowed (e.g., AWS S3 files with /vsis3/).
#' Starting with GDAL 3.2, a configuration option can be set with:
#' ```
#' set_config_option("CPL_VSIL_USE_TEMP_FILE_FOR_RANDOM_WRITE", "YES")
#' ````
#' in which case random-write access is possible (involves the creation of a
#' temporary local file, whose location is controlled by the `CPL_TMPDIR`
#' configuration option). In this case, setting `access` to `"w+"` may be
#' needed for writing with seek and read operations (if creating a new file,
#' otherwise, `"r+"` to open an existing file), while `"w"` access would
#' allow sequential write only.
#'
#' @seealso
#' GDAL Virtual File Systems (compressed, network hosted, etc...):\cr
#' /vsimem, /vsizip, /vsitar, /vsicurl, ...\cr
#' \url{https://gdal.org/user/virtual_file_systems.html}
#'
#' [vsi_copy_file()], [vsi_read_dir()], [vsi_stat()], [vsi_unlink()]
#'
#' @examples
#' # The examples make use of the FARSITE LCP format specification at:
#' # https://gdal.org/drivers/raster/lcp.html
#' # An LCP file is a raw format with a 7,316-byte header. The format
#' # specification gives byte offets and data types for fields in the header.
#'
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#'
#' # identify a FARSITE v.4 LCP file
#' # function to check if the first three fields have valid data
#' # input is the first twelve raw bytes in the file
#' is_lcp <- function(bytes) {
#'   values <- readBin(bytes, "integer", n = 3)
#'   if ((values[1] == 20 || values[1] == 21) &&
#'       (values[2] == 20 || values[2] == 21) &&
#'       (values[3] >= -90 && values[3] <= 90)) {
#'
#'     return(TRUE)
#'   } else {
#'     return(FALSE)
#'   }
#' }
#'
#' vf <- new(VSIFile, lcp_file)
#' vf$read(12) |> is_lcp()
#'
#' vf$tell()
#'
#' # read the whole file into memory
#' bytes <- vf$ingest(-1)
#' vf$close()
#'
#' # write to a VSI in-memory file
#' mem_file <- "/vsimem/storml_copy.lcp"
#' vf <- new(VSIFile, mem_file, "w")
#' vf$write(bytes)
#'
#' vf$tell()
#' vf$rewind()
#' vf$tell()
#'
#' vf$seek(0, SEEK_END)
#' (vf$tell() == vsi_stat(lcp_file, "size"))  # TRUE
#'
#' vf$rewind()
#' vf$read(12) |> is_lcp()
#'
#' # read/write an integer field
#' # write invalid data for the Latitude field and then set back
#' # save the original first
#' vf$seek(8, SEEK_SET)
#' lat_orig <- vf$read(4)
#' readBin(lat_orig, "integer")  # 46
#' # latitude -99 out of range
#' vf$seek(8, SEEK_SET)
#' writeBin(-99L, raw()) |> vf$write()
#' vf$rewind()
#' vf$read(12) |> is_lcp()  # FALSE
#' vf$seek(8, SEEK_SET)
#' vf$read(4) |> readBin("integer")  # -99
#' # set back to original
#' vf$seek(8, SEEK_SET)
#' vf$write(lat_orig)
#' vf$rewind()
#' vf$read(12) |> is_lcp()  # TRUE
#'
#' # read a vector of doubles - xmax, xmin, ymax, ymin
#' # 327766.1, 323476.1, 5105082.0, 5101872.0
#' vf$seek(4172, SEEK_SET)
#' vf$read(32) |> readBin("double", n = 4)
#'
#' # read a short int, the canopy cover units
#' vf$seek(4232, SEEK_SET)
#' vf$read(2) |> readBin("integer", size = 2)  # 1 = "percent"
#'
#' # read the Description field
#' vf$seek(6804, SEEK_SET)
#' bytes <- vf$read(512)
#' rawToChar(bytes)
#'
#' # edit the Description
#' desc <- paste(rawToChar(bytes),
#'               "Storm Lake AOI,",
#'               "Beaverhead-Deerlodge National Forest, Montana.")
#'
#' vf$seek(6804, SEEK_SET)
#' charToRaw(desc) |> vf$write()
#' vf$close()
#'
#' # verify the file as a raster dataset
#' ds <- new(GDALRaster, mem_file)
#' ds$info()
#'
#' # retrieve Description from the metadata
#' # band = 0 for dataset-level metadata, domain = "" for default domain
#' ds$getMetadata(band = 0, domain = "")
#' ds$getMetadataItem(band = 0, mdi_name = "DESCRIPTION", domain = "")
#'
#' ds$close()
#' vsi_unlink(mem_file)
NULL

Rcpp::loadModule("mod_VSIFile", TRUE)
