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
#' This API abstracts binary file I/O for standard filesystems, URLs,
#' cloud storage services, Zip/GZip/7z/RAR, and in-memory files.
#'
#' @param filename Character string containing the filename to open. It may be
#' a file in a "standard" local filesystem, or a filename with a GDAL
#' /vsiPREFIX/ (see \url{https://gdal.org/user/virtual_file_systems.html}).
#' @param access Character string containing the access requested (i.e., `"r"`,
#' `"r+"`, `"w"`). Defaults to `"r"`. Binary access is always implied and the
#' "b" does not need to be included in the `access`.
#' @param options Optional character vector of `NAME=VALUE` pairs specifying
#' filesystem-dependent options (GDAL >= 3.3, see Details).
#' @returns An object of class `VSIFile` which contains a pointer to a
#' `VSIVirtualHandle`, and methods that operate on the file pointer as
#' described in Details. `VSIFile` is a C++ class exposed directly to R (via
#' `RCPP_EXPOSED_CLASS`). Methods of the class are accessed using the
#' `$` operator.
#'
#' @section Usage:
#' \preformatted{
#' ## Constructors
#' vf <- new(VSIFile, filename)
#' # specifying access:
#' vf <- new(VSIFile, filename, access)
#' # specifying access and options (both required)
#' vf <- new(VSIFile, filename, access, options)
#'
#' ## Methods (see Details)
#' vf$seek(offset, origin)
#' vf$tell()
#' vf$rewind()
#' vf$read(nbytes)
#' vf$write(object, size)
#' vf$eof()
#' vf$truncate(new_size)
#' vf$flush()
#' vf$ingest(max_size)
#'
#' vf$close()
#' vf$open()
#' vf$get_filename()
#' }
#' @section Details:
#'
#' \code{new(VSIFile, filename)}
#' Constructor. Returns an object of class `VSIFile`, or an error is raised
#' if a file handle cannot be obtained.
#'
#' \code{new(VSIFile, filename, access)}
#' Alternate constructor for passing `access` as a character string.
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
#' as GDAL 3.9 include:
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
#' `offset` is given as a postive numeric scalar, optionally as
#' `bit64::integer64` type.
#' `origin` is given as a character string, one of `SEEK_SET`, `SEEK_CUR` or
#' `SEEK_END`. Note that `offset` is an unsigned type, so `SEEK_CUR` can only
#' be used for positive seek. If negative seek is needed, use for example:
#' ```
#' vf$seek(vf$tell() + negative_offset, "SEEK_SET")
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
#' `vf$seek(0, "SEEK_SET")`. No return value, called for that side effect.
#'
#' \code{$read(nbytes)}
#' Reads `nbytes` bytes from the file at the current offset. Returns a vector
#' of R `raw` type, or `NULL` if the operation fails.
#'
#' \code{$write(object, size)}
#' Writes objects of `size` bytes to the file at the current offset. `object`
#' is a non-character atomic vector (i.e., `raw`, `numeric`, `integer`,
#' `logical`, `complex`). `size` is the number of bytes per element in
#' `object`. The element `size` may be give as a negative number (e.g., `-1`)
#' to use the natural size for the data type, i.e., `raw` size = `1`,
#' `numeric` size = `sizeof(double)`, `integer` size = `sizeof(int)`,
#' `logical` size = `sizeof(bool)`,
#' `complex` size = `sizeof(std::complex<double>)`. Some of the information
#' given in base R `?writeBin` is relevant here, but realize that the
#' implementation here is different with minimal automatic handling.
#' Returns the number of objects successfully written, as numeric scalar
#' carrying the `integer64` class attribute.
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
#' filesystem types where it is applicable, all pending output on the file is
#' flushed to the physical disk.
#' On Windows regular files, this method does nothing, unless the
#' `VSI_FLUSH=YES` configuration option is set (and only when the file has not
#' been opened with the `WRITE_THROUGH` option).
#' Returns `0` on success or `-1` on error.
#'
#' \code{$ingest(max_size)}
#' Ingest a file into memory. Read the whole content of the file into a `raw`
#' vector.
#' `max_size` is the maximum size of file allowed, given as a positive
#' numeric scalar, optionally as `bit64::integer64` type. If no limit, set to
#' a negative value. Returns a `raw` vector, or `NULL` if the operation fails.
#'
#' \code{$close()}
#' Closes the file. The file should always be closed when I/O has been
#' completed. Returns `0` on success or `-1` on error.
#'
#' \code{$open()}
#' The method can be used to re-open the file after it has been closed, using
#' the same `filename`, `access`, and `options` (if any). No return value.
#' An error is raised if a file handle cannot be obtained.
#'
#' \code{$get_filename()}
#' Returns a character string containing the `filename` associated with this
#' `VSIFile` object (`filename` originally used to create the object).
#'
#' @note
#'
#' @seealso
#' GDAL Virtual File Systems (compressed, network hosted, etc...): /vsimem,
#' /vsizip, /vsitar, /vsicurl, ...\cr
#' \url{https://gdal.org/user/virtual_file_systems.html}
#'
#' [vsi_copy_file()], [vsi_stat()], [vsi_unlink()], [vsi_get_fs_prefixes()],
#' [vsi_get_fs_options()]
#'
#' @examples
#'
NULL

Rcpp::loadModule("mod_VSIFile", TRUE)
