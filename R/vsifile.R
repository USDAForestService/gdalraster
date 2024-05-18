#' @name VSIFile-class
#'
#' @aliases
#' Rcpp_VSIFile Rcpp_VSIFile-class VSIFile
#'
#' @title Class wrapping the GDAL VSIVirtualHandle API for low-level file I/O
#' @description
#' `VSIFile` provides bindings to the GDAL VSIVirtualHandle API. Encapsulates a
#' `VSIVirtualHandle`
#' (\url{https://gdal.org/api/cpl_cpp.html#_CPPv416VSIVirtualHandle}).
#' This API abstracts low-level file I/O for standard filesystems, URLs,
#' cloud storage services, Zip/GZip/7z/RAR, and in-memory files.
#'
#' @param filename Character string containing the filename to open. It may be
#' a file in a "standard" local filesystem, or a filename with a GDAL
#' /vsiPREFIX/ (see \url{https://gdal.org/user/virtual_file_systems.html}).
#' @param access Character string containing the access requested (i.e., `"r"`,
#' `"r+"`, `"w"`). Defaults to `"r"`.
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
#' vf$truncate(offset)
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
#' `options` as a character vector of NAME=VALUE pairs (both arguments
#' required).
#' Returns an object of class `VSIFile` with an open file handle, or an error
#' is raised if a file handle cannot be obtained.
#'
#' \code{$seek(offset, origin)}
#'






#'
#' \code{$get_filename()}
#' Returns a character string containing the `filename` associated with this
#' `VSIFile` object (`filename` originally used to open the dataset).
#'

NULL

Rcpp::loadModule("mod_VSIFile", TRUE)
