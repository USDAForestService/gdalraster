% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RcppExports.R
\name{vsi_is_local}
\alias{vsi_is_local}
\title{Returns if the file/filesystem is "local".}
\usage{
vsi_is_local(filename)
}
\arguments{
\item{filename}{Character string. The path of the filesystem object to be
tested.}
}
\value{
Logical scalar. \code{TRUE} if if the input file path is local.
}
\description{
\code{vsi_is_local()} returns whether the file/filesystem is "local".
Wrapper for \code{VSIIsLocal()} in the GDAL API. Requires GDAL >= 3.6.
}
\note{
The concept of local is mostly by opposition with a network / remote file
system whose access time can be long.

/vsimem/ is considered to be a local file system, although a
non-persistent one.
}
\examples{
# Requires GDAL >= 3.6
if (gdal_version_num() >= gdal_compute_version(3, 6, 0))
  print(vsi_is_local("/vsimem/test-mem-file.tif"))
}
