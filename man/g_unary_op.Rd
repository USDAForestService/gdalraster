% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/geom.R
\name{g_unary_op}
\alias{g_unary_op}
\alias{g_buffer}
\alias{g_convex_hull}
\alias{g_simplify}
\title{Unary operations on WKB or WKT geometries}
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

g_convex_hull(
  geom,
  as_wkb = TRUE,
  as_iso = FALSE,
  byte_order = "LSB",
  quiet = FALSE
)

g_simplify(
  geom,
  tolerance,
  preserve_topology = TRUE,
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

\item{tolerance}{Numeric value of the simplification tolerance, as distance
in units of the input \code{geom}. Simplification removes vertices which are
within the tolerance distance of the simplified linework (as long as
topology is preserved when \code{preserve_topology = TRUE}).}

\item{preserve_topology}{Logical value, \code{TRUE} to simplify geometries while
preserving topology (the default). Setting to \code{FALSE} simplifies geometries
using the standard Douglas-Peucker algorithm which is significantly faster
(see Note).}
}
\value{
A geometry as WKB raw vector or WKT string, or a list/character vector of
geometries as WKB/WKT with length equal to the number of input geometries.
\code{NA} is returned with a warning if WKB input cannot be converted into an
OGR geometry object, or if an error occurs in the call to the underlying
OGR API.
}
\description{
These functions implement algorithms that operate on one input geometry
for which a new output geometry is generated.
The input geometries may be given as a single raw vector of WKB, a list of
WKB raw vectors, or a character vector containing one or more WKT strings.
}
\details{
These functions use the GEOS library via GDAL headers.

\code{g_buffer()} builds a new geometry containing the buffer region around
the geometry on which it is invoked. The buffer is a polygon containing
the region within the buffer distance of the original geometry.
Wrapper of \code{OGR_G_Buffer()}in the GDAL Geometry API.

\code{g_convex_hull()} computes a convex hull, the smallest convex geometry that
contains all the points in the input geometry. Wrapper of
\code{OGR_G_ConvexHull()} in the GDAL API.

\code{g_simplify()} computes a simplified geometry. By default, it simplifies
the input geometries while preserving topology (see Note). Wrapper of
\code{OGR_G_Simplify()} / \code{OGR_G_SimplifyPreserveTopology()} in the GDAL API.
}
\note{
Definitions of these operations are given in the GEOS documentation
(\url{https://libgeos.org/doxygen/}, GEOS 3.14.0dev), some of which is
copied here.

\code{g_buffer()} always returns a polygonal result. The negative or
zero-distance buffer of lines and points is always an empty Polygon.

\code{g_convex_hull()} uses the Graham Scan algorithm.

\code{g_simplify()}:
\itemize{
\item With \code{preserve_topology = TRUE} (the default):\cr
Simplifies a geometry, ensuring that the result is a valid geometry having
the same dimension and number of components as the input. The simplification
uses a maximum distance difference algorithm similar to the one used in the
Douglas-Peucker algorithm. In particular, if the input is an areal geometry
(Polygon or MultiPolygon), the result has the same number of shells and
holes (rings) as the input, in the same order. The result rings touch at no
more than the number of touching point in the input (although they may touch
at fewer points).
\item With \code{preserve_topology = FALSE}:\cr
Simplifies a geometry using the standard Douglas-Peucker algorithm. Ensures
that any polygonal geometries returned are valid. Simple lines are not
guaranteed to remain simple after simplification. Note that in general D-P
does not preserve topology - e.g. polygons can be split, collapse to lines
or disappear, holes can be created or disappear, and lines can cross. To
simplify geometry while preserving topology use TopologyPreservingSimplifier.
(However, using D-P is significantly faster).
}

N.B., \code{preserve_topology = TRUE} does not preserve boundaries shared between
polygons.
}
\examples{
g_buffer("POINT (0 0)", dist = 10, as_wkb = FALSE)

g1 <- "GEOMETRYCOLLECTION(POINT(0 1), POINT(0 0), POINT(1 0), POINT(1 1))"
g_convex_hull(g1, as_wkb = FALSE)

g2 <- "LINESTRING(0 0,1 1,10 0)"
g_simplify(g2, tolerance = 5, as_wkb = FALSE)
}
