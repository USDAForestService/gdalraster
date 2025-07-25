% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RcppExports.R
\name{srs_convert}
\alias{srs_convert}
\alias{epsg_to_wkt}
\alias{srs_to_wkt}
\alias{srs_to_projjson}
\title{Convert spatial reference definitions to OGC WKT or PROJJSON}
\usage{
epsg_to_wkt(epsg, pretty = FALSE)

srs_to_wkt(srs, pretty = FALSE, gcs_only = FALSE)

srs_to_projjson(
  srs,
  multiline = TRUE,
  indent_width = 2L,
  schema = NA_character_
)
}
\arguments{
\item{epsg}{Integer EPSG code.}

\item{pretty}{Logical value. \code{TRUE} to return a nicely formatted WKT string
for display to a person. \code{FALSE} for a regular WKT string (the default).}

\item{srs}{Character string containing an SRS definition in various
formats (see Details).}

\item{gcs_only}{Logical value. \code{TRUE} to return only the definition of the
GEOGCS node of the input \code{srs}. Defaults to \code{FALSE} (see Note).}

\item{multiline}{Logical value. \code{TRUE} for PROJJSON multiline output (the
default).}

\item{indent_width}{Integer value. Defaults to \code{2}. Only used if
\code{multiline = TRUE} for PROJJSON output.}

\item{schema}{Character string containing URL to PROJJSON schema. Can be
set to empty string to disable it.}
}
\value{
Character string containing OGC WKT.
}
\description{
These functions convert various spatial reference formats to Well Known
Text (WKT) or PROJJSON.
}
\details{
\code{epsg_to_wkt()} exports the spatial reference for an EPSG code to
WKT format.
Wrapper for \code{OSRImportFromEPSG()} in the GDAL Spatial Reference System API
with output to WKT.

\code{srs_to_wkt()} converts a spatial reference system (SRS) definition
in various text formats to WKT. The function will examine the input SRS,
try to deduce the format, and then export it to WKT.
Wrapper for \code{OSRSetFromUserInput()} in the GDAL Spatial Reference System
API with output to WKT.

\code{srs_to_projjson()} accepts a spatial reference system (SRS) definition in
any of the formats supported by \code{srs_to_wkt()}, and converts into PROJJSON
format. Wrapper for \code{OSRExportToPROJJSON()} in the GDAL Spatial Reference
System API.

The input SRS may take the following forms:
\itemize{
\item WKT - to convert WKT versions (see below)
\item EPSG:n - EPSG code n
\item AUTO:proj_id,unit_id,lon0,lat0 - WMS auto projections
\item urn:ogc:def:crs:EPSG::n - OGC URNs
\item PROJ.4 definitions
\item filename - file to read for WKT, XML or PROJ.4 definition
\item well known name such as NAD27, NAD83, WGS84 or WGS72
\item IGNF:xxxx, ESRI:xxxx - definitions from the PROJ database
\item PROJJSON (PROJ >= 6.2)
}

\code{srs_to_wkt()} is intended to be flexible, but by its nature it is
imprecise as it must guess information about the format intended.
\code{\link[=epsg_to_wkt]{epsg_to_wkt()}} could be used instead for EPSG codes.

As of GDAL 3.0, the default format for WKT export is OGC WKT 1.
The WKT version can be overridden by using the OSR_WKT_FORMAT
configuration option (see \code{\link[=set_config_option]{set_config_option()}}).
Valid values are one of: SFSQL, WKT1_SIMPLE, WKT1, WKT1_GDAL,
WKT1_ESRI, WKT2_2015, WKT2_2018, WKT2, DEFAULT.
If SFSQL, a WKT1 string without AXIS, TOWGS84, AUTHORITY or
EXTENSION node is returned. If WKT1_SIMPLE, a WKT1 string without
AXIS, AUTHORITY or EXTENSION node is returned. WKT1 is an alias of
WKT1_GDAL. WKT2 will default to the latest revision implemented
(currently WKT2_2018). WKT2_2019 can be used as an alias of
WKT2_2018 since GDAL 3.2
}
\note{
Setting \code{gcs_only = TRUE} in \code{srs_to_wkt()} is a wrapper of
\code{OSRCloneGeogCS()} in the GDAL API. The returned WKT will be for the GEOGCS
node of the input \code{srs}.
}
\examples{
epsg_to_wkt(5070)
writeLines(epsg_to_wkt(5070, pretty = TRUE))

srs_to_wkt("NAD83")
writeLines(srs_to_wkt("NAD83", pretty = TRUE))
set_config_option("OSR_WKT_FORMAT", "WKT2")
writeLines(srs_to_wkt("NAD83", pretty = TRUE))
set_config_option("OSR_WKT_FORMAT", "")

srs_to_wkt("EPSG:5070", gcs_only = TRUE)

srs_to_projjson("NAD83") |> writeLines()
}
\seealso{
\link{srs_query}
}
