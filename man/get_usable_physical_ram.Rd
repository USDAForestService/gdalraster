% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RcppExports.R
\name{get_usable_physical_ram}
\alias{get_usable_physical_ram}
\title{Get usable physical RAM reported by GDAL}
\usage{
get_usable_physical_ram()
}
\value{
Numeric scalar, number of bytes as \code{bit64::integer64} type (or 0 in
case of failure).
}
\description{
\code{get_usable_physical_ram()} returns the total physical RAM, usable by a
process, in bytes. It will limit to 2 GB for 32 bit processes. Starting
with GDAL 2.4.0, it will also take into account resource limits (virtual
memory) on Posix systems. Starting with GDAL 3.6.1, it will also take into
account RLIMIT_RSS on Linux. Wrapper of \code{CPLGetUsablePhysicalRAM()} in the
GDAL Common Portability Library.
}
\note{
This memory may already be partly used by other processes.
}
\examples{
get_usable_physical_ram()
}
