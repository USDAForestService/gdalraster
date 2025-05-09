% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/ogr_reproject.R
\name{ogr_reproject}
\alias{ogr_reproject}
\title{Reproject a vector layer}
\usage{
ogr_reproject(
  src_dsn,
  src_layer,
  out_dsn,
  out_srs,
  out_fmt = NULL,
  overwrite = FALSE,
  append = FALSE,
  nln = NULL,
  nlt = NULL,
  dsco = NULL,
  lco = NULL,
  dialect = NULL,
  spat_bbox = NULL,
  src_open_options = NULL,
  progress = FALSE,
  add_cl_arg = NULL,
  return_obj = TRUE
)
}
\arguments{
\item{src_dsn}{Character string. The filename or database connection string
specifying the vector data source containing the input layer.}

\item{src_layer}{Character string. The name of the input layer in \code{src_dsn}
to reproject. Optionally can be given as an SQL SELECT statement to be
executed against \code{src_dsn}, defining the source layer as the result set.
May also be given as empty string (\code{""}), in which case the first layer by
index will be used (mainly useful for single-layer file formats such as
ESRI Shapefile).}

\item{out_dsn}{Character string. The filename or database connection string
specifying the vector data source to which the output layer will be written.}

\item{out_srs}{Character string specifying the output spatial reference
system. May be in WKT format or any of the formats supported by
\code{\link[=srs_to_wkt]{srs_to_wkt()}}.}

\item{out_fmt}{Optional character string giving the GDAL short name
of the output dataset format. Only used if \code{out_dsn} needs to be created.
Generally can be \code{NULL} in which case the format will be guessed from the
file extension.}

\item{overwrite}{Logical value. \code{TRUE} to overwrite the output layer if it
already exists. Defaults to \code{FALSE}.}

\item{append}{Logical value. \code{TRUE} to append to the output layer if it
already exists. Defaults to \code{FALSE}.}

\item{nln}{Optional character string giving an alternate name to assign the
new layer. By default, \code{src_layer} is used, but \code{nln} is required if
\code{src_layer}is a SQL SELECT statement.}

\item{nlt}{Optional character string to define the geometry type for the
output layer. Mainly useful when \code{nlt = PROMOTE_TO_MULTI} might be given to
automatically promote layers that mix polygon / multipolygons to
multipolygons, and layers that mix linestrings / multilinestrings to
multilinestrings. Can be useful when converting shapefiles to PostGIS and
other output formats that implement strict checks for geometry types.}

\item{dsco}{Optional character vector of format-specific creation options
for \code{out_dsn} (\code{"NAME=VALUE"} pairs). Should only be used if \code{out_dsn} does
not already exist.}

\item{lco}{Optional character vector of format-specific creation options
for the output layer (\code{"NAME=VALUE"} pairs). Should not be used if appending
to an existing layer.}

\item{dialect}{Optional character string specifying the SQL dialect to use.
The OGR SQL engine (\code{"OGRSQL"}) will be used by default if a value is not
given. The \code{"SQLite"} dialect can also be use. Only relevant if \code{src_layer}
is given as a SQL SELECT statement.}

\item{spat_bbox}{Optional numeric vector of length four specifying a spatial
bounding box (xmin, ymin, xmax, ymax), \emph{in the SRS of the source layer}.
Only features whose geometry intersects \code{spat_bbox} will be selected for
reprojection.}

\item{src_open_options}{Optional character vector of dataset open options
for \code{src_dsn} (format-specific \code{"NAME=VALUE"} pairs).}

\item{progress}{Logical value, \code{TRUE} to display progress on the terminal.
Defaults to \code{FALSE}. Only works if the input layer has "fast feature count"
capability.}

\item{add_cl_arg}{Optional character vector of additional command-line
arguments to be passed to \code{ogr2ogr()} (see Note).}

\item{return_obj}{Logical value, \code{TRUE} to return an object of class
\code{\link{GDALVector}} open on the output layer (the default).}
}
\value{
Upon successful completion, an object of class \code{\link{GDALVector}} is
returned by default (if \code{return_obj = TRUE}), or logical \code{TRUE} is returned
(invisibly) if \code{return_obj = FALSE}. An error is raised if reprojection
fails.
}
\description{
\code{ogr_reproject()} reprojects the features of a vector layer to a different
spatial reference system and writes the new layer to a specified output
dataset. The output may be in a different vector file format than the input
dataset. A source SRS definition must be available in the source layer
for reprojection to occur.
}
\details{
\code{ogr_reproject()} is a convenience wrapper to perform vector reprojection
via \code{ogr2ogr()}, which in turn is an API binding to GDAL's \code{ogr2ogr}
command-line utility.
}
\note{
For advanced use, additional command-line arguments may be passed to
\code{ogr2ogr()} in \code{add_cl_arg} (e.g., advanced geometry and SRS related
options). Users should be aware of possible implications and
compatibility with the arguments already implied by the parameterization
of \code{ogr_reproject()}.

The function will attempt to create the output datasource if it does not
already exist. Some formats (e.g., PostgreSQL) do not support creation of
new datasets (i.e., a database within PostgreSQL), but output layers can be
written to an existing database.
}
\examples{
# MTBS fire perimeters
f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
(mtbs <- new(GDALVector, f, "mtbs_perims"))

mtbs$getSpatialRef() |> srs_is_projected()  # TRUE

# YNP boundary
f <- system.file("extdata/ynp_features.zip", package = "gdalraster")
ynp_dsn <- file.path("/vsizip", f, "ynp_features.gpkg")
(bnd <- new(GDALVector, ynp_dsn, "ynp_bnd"))

bnd$getSpatialRef() |> srs_is_projected()  # FALSE

# project the boundary to match the MTBS layer
out_dsn <- tempfile(fileext = ".gpkg")
(bnd_mtsp <- ogr_reproject(ynp_dsn, "ynp_bnd", out_dsn, mtbs$getSpatialRef()))

bnd_mtsp$getFeatureCount()

plot(bnd_mtsp$getNextFeature(), col = "wheat")

mtbs$setAttributeFilter("incid_name = 'MAPLE'")
mtbs$getFeatureCount()  # 1

(feat <- mtbs$getNextFeature())

plot(feat, col = "red", border = NA, add = TRUE)

mtbs$close()
bnd$close()
bnd_mtsp$close()
\dontshow{unlink(out_dsn)}
}
\seealso{
\code{\link[=ogr2ogr]{ogr2ogr()}}, \code{\link[=warp]{warp()}} for raster reprojection

GDAL documentation for \code{ogr2ogr}:\cr
\url{https://gdal.org/en/stable/programs/ogr2ogr.html}
}
