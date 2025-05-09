% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/geom.R
\name{g_buffer}
\alias{g_buffer}
\title{Compute buffer of a WKB/WKT geometry}
\usage{
g_buffer(
  geom,
  dist,
  quad_segs = 30L,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)
}
\arguments{
\item{geom}{Either a raw vector of WKB or list of raw vectors, or a
character vector containing one or more WKT strings.}

\item{dist}{Numeric buffer distance in units of the input \code{geom}.}

\item{quad_segs}{Integer number of segments used to define a 90 degree
curve (quadrant of a circle). Large values result in large numbers of
vertices in the resulting buffer geometry while small numbers reduce the
accuracy of the result.}

\item{as_wkb}{Logical value, \code{TRUE} to return the output geometry in WKB
format (the default), or \code{FALSE} to return as WKT.}

\item{as_iso}{Logical value, \code{TRUE} to export as ISO WKB/WKT (ISO 13249
SQL/MM Part 3), or \code{FALSE} (the default) to export as "Extended WKB/WKT".}

\item{byte_order}{Character string specifying the byte order when output is
WKB. One of \code{"LSB"} (the default) or \code{"MSB"} (uncommon).}

\item{quiet}{Logical value, \code{TRUE} to suppress warnings. Defaults to \code{FALSE}.}
}
\value{
A polygon as WKB raw vector or WKT string, or a list/character vector of
polygons as WKB/WKT with length equal to the number of input geometries.
\code{NA} is returned with a warning if WKB input cannot be converted into an
OGR geometry object, or if an error occurs in the call to the underlying
OGR API.
}
\description{
\code{g_buffer()} builds a new geometry containing the buffer region around
the geometry on which it is invoked. The buffer is a polygon containing
the region within the buffer distance of the original geometry.
Wrapper of \code{OGR_G_Buffer()}in the GDAL API (GEOS via GDAL headers).
}
\examples{
g_buffer("POINT (0 0)", dist = 10, as_wkb = FALSE)
}
