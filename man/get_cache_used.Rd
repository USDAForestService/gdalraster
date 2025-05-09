% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RcppExports.R
\name{get_cache_used}
\alias{get_cache_used}
\title{Get the size of memory in use by the GDAL block cache}
\usage{
get_cache_used(units = "MB")
}
\arguments{
\item{units}{Character string specifying units for the return value. One of
\code{"MB"} (the default), \code{"GB"}, \code{"KB"} or \code{"bytes"} (values of \code{"byte"},
\code{"B"} and empty string \code{""} are also recognized to mean bytes).}
}
\value{
A numeric value carrying the \code{integer64} class attribute. Amount
of the available cache memory currently in use in the requested units.
}
\description{
\code{get_cache_used()} returns the amount of memory currently in use for
GDAL block caching. Wrapper of \code{GDALGetCacheUsed64()} with return
value in MB by default.
}
\examples{
get_cache_used()
}
\seealso{
\href{https://usdaforestservice.github.io/gdalraster/articles/gdal-block-cache.html}{GDAL Block Cache}

\code{\link[=get_cache_max]{get_cache_max()}}, \code{\link[=set_cache_max]{set_cache_max()}}
}
