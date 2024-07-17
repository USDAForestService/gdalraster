#' @name GDALVector-class
#'
#' @aliases
#' Rcpp_GDALVector Rcpp_GDALVector-class GDALVector
#'
#' @title Class encapsulating a vector layer in a GDAL dataset
#'
#' @description
#' `GDALVector` provides an interface for accessing a vector layer in a GDAL
#' dataset and calling methods on the underlying `OGRLayer` and `OGRFeature`
#' objects. See \url{https://gdal.org/api/index.html} for details of the GDAL
#' Vector API.
#'
#' @param dsn Character string containing the data source name (usually a
#' filename or database connection string, see GDAL vector format
#' descriptions: \url{https://gdal.org/drivers/vector/index.html}).
#' @param layer Character string containing either the name of a layer of
#' features within the data source, or an SQL SELECT statement to be executed
#' against the data source that defines a layer via its result set.
#' @param read_only Logical. `TRUE` to open the layer read-only (the default),
#' or `FALSE` to open with write access.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#' specifying layer open options.
#' @param spatial_filter Optional character string containing a geometry in
#' Well Known Text (WKT) format which represents a spatial filter.
#' @param dialect Optional character string to control the statement dialect
#' when SQL is used to define the layer. By default, the OGR SQL engine will
#' be used, except for RDBMS drivers that will use their dedicated SQL engine,
#' unless `"OGRSQL"` is explicitly passed as the dialect. The `"SQLITE"`
#' dialect can also be used
#' (see \url{https://gdal.org/user/ogr_sql_sqlite_dialect.html}).
#' @returns An object of class `GDALVector` which contains pointers to the
#' opened layer and the dataset that contains it, and methods that operate on
#' the layer as described in Details. `GDALVector` is a C++ class exposed
#' directly to R (via `RCPP_EXPOSED_CLASS`). Fields and methods of the class
#' are accessed using the `$` operator. The read/write fields are used for
#' per-object settings.
#'
#' @section Usage:
#' \preformatted{
#' ## Constructors
#' # read-only by default:
#' ds <- new(GDALVector, dsn)
#' # for update access:
#' ds <- new(GDALVector, dsn, read_only = FALSE)
#' # to use dataset open options:
#' ds <- new(GDALVector, dsn, read_only = TRUE|FALSE, open_options)
#' # to open without shared mode:
#' new(GDALVector, dsn, read_only, open_options, shared = FALSE)
#'
#' ## Read/write fields (see Details)
#'
#' ## Methods (see Details)
#' ds$open(read_only)
#' ds$isOpen()
#' ds$getDsn()
#' ds$getFileList()
#'
#' ds$getDriverShortName()
#' ds$getDriverLongName()
#'
#' ds$close()
#' }
#' @section Details:
#'
#' \code{new(GDALVector, dsn)}
#' Constructor. Returns an object of class `GDALVector`.
#' `read_only` defaults to `TRUE` if not specified.
#'
#' \code{$quiet}
#' Read/write field.
#' A logical value, `FALSE` by default. This field can be set to `TRUE` which
#' will suppress various messages as well as progress reporting for potentially
#' long-running processes such as building overviews and computation of
#' statistics and histograms.
#'
#' \code{$open(read_only)}
#' (Re-)opens the raster dataset on the existing dsn. Use this method to
#' open a dataset that has been closed using \code{$close()}. May be used to
#' re-open a dataset with a different read/write access (`read_only` set to
#' `TRUE` or `FALSE`). The method will first close an open dataset, so it is
#' not required to call \code{$close()} explicitly in this case.
#' No return value, called for side effects.
#'
#' \code{$isOpen()}
#' Returns logical indicating whether the associated vector dataset is open.
#'
#' \code{$getDsn()}
#' Returns a character string containing the `dsn` associated with this
#' `GDALVector` object (`dsn` originally used to open the layer).
#'
#' \code{$getFileList()}
#' Returns a character vector of files believed to be part of this dataset.
#' If it returns an empty string (`""`) it means there is believed to be no
#' local file system files associated with the dataset (e.g., a virtual file
#' system). The returned filenames will normally be relative or absolute
#' paths depending on the path used to originally open the dataset.
#'
#' \code{$close()}
#' Closes the GDAL dataset (no return value, called for side effects).
#' Calling \code{$close()} results in proper cleanup, and flushing of any
#' pending writes.
#' The `GDALVector` object is still available after calling \code{$close()}.
#' The dataset can be re-opened on the existing \code{dsn} with
#' \code{$open(read_only=TRUE)} or \code{$open(read_only=FALSE)}.
#'
#' @note
#'
#' @seealso
#'
#' @examples
#'
NULL

Rcpp::loadModule("mod_GDALVector", TRUE)
