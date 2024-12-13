% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RcppExports.R
\name{srs_find_epsg}
\alias{srs_find_epsg}
\title{Try to identify a matching EPSG code for a given SRS definition}
\usage{
srs_find_epsg(srs, all_matches = FALSE)
}
\arguments{
\item{srs}{Character string containing an SRS definition in various
formats (e.g., WKT, PROJ.4 string, well known name such as NAD27, NAD83,
WGS84, etc).}

\item{all_matches}{Logical scalar. \code{TRUE} to return all identified matches
in a data frame, including a confidence value (0-100) for each match. The
default is \code{FALSE} which returns a character string in the form
\code{"EPSG:<code>"} for the first match (highest confidence).}
}
\value{
Character string or data frame, or \code{NULL} if matching failed.
}
\description{
\code{srs_find_epsg()} accepts a spatial reference system definition
in various text formats and tries to find a matching EPSG code.
See \code{\link[=srs_to_wkt]{srs_to_wkt()}} for a description of the possible input formats.
This function is an interface to \code{OSRFindMatches()} in the GDAL Spatial
Reference System API.
}
\details{
Matching may be partial, or may fail. Returned entries will be sorted by
decreasing match confidence (first entry has the highest match confidence).
}
\examples{
srs_find_epsg("WGS84")

srs_find_epsg("WGS84", all_matches = TRUE)
}
\seealso{
\code{\link[=epsg_to_wkt]{epsg_to_wkt()}}, \code{\link[=srs_to_wkt]{srs_to_wkt()}}
}
