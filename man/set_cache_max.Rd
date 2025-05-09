% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RcppExports.R
\name{set_cache_max}
\alias{set_cache_max}
\title{Set the maximum memory size for the GDAL block cache}
\usage{
set_cache_max(nbytes)
}
\arguments{
\item{nbytes}{A numeric value optionally carrying the \code{integer64} class
attribute (assumed to be a whole number, will be coerced to integer by
truncation). Specifies the new cache size in bytes (maximum number of bytes
for caching).}
}
\value{
No return value, called for side effects.
}
\description{
\code{set_cache_max()} sets the maximum amount of memory that GDAL is permitted
to use for GDALRasterBlock caching.
\emph{The unit of the value to set is bytes.} Wrapper of \code{GDALSetCacheMax64()}.
}
\note{
\strong{This function will not make any attempt to check the consistency of the
passed value with the effective capabilities of the OS.}

It is recommended to consult the documentation for \code{get_cache_max()} and
\code{get_cache_used()} before using this function.

\code{\link[=get_cache_max]{get_cache_max()}}, \code{\link[=get_cache_used]{get_cache_used()}}
}
\examples{
(cachemax <- get_cache_max("bytes"))

set_cache_max(1e8)
get_cache_max()  # returns in MB by default

# reset to original
set_cache_max(cachemax)
get_cache_max()
}
