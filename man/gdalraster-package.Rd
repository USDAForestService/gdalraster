\docType{package}
\name{gdalraster-package}
\alias{gdalraster}
\alias{gdalraster-package}
\title{Bindings to the GDAL Raster API
}
\description{
  \code{gdalraster} is an interface to the Geospatial Data Abstraction 
  Library (GDAL) for low level raster I/O. Calling signatures resemble those 
  of the native C, C++ and Python APIs provided by the GDAL project.
  See \url{https://gdal.org/api/} for details of the GDAL Raster API.
}
\details{
  Core functionality is contained in the \code{GDALRaster} class and several 
  GDAL related stand-alone functions:
  \itemize{
  \item class \code{\link{GDALRaster}} allows opening a raster dataset and 
  calling methods on the Dataset, Driver and RasterBand objects in the 
  underlying API (get/set parameters, read/write pixel data).
  
  \item raster creation: \code{\link{create}}, \code{\link{createCopy}}, 
  \code{\link{rasterFromRaster}}
 
  \item virtual raster: \code{\link{rasterToVRT}}
  
  \item reproject: \code{\link{warp}}
  
  \item algorithms: \code{\link{fillNodata}},
  \code{\link[=GDALRaster]{GDALRaster$getChecksum}}
  
  \item geotransform conversion: \code{\link{inv_geotransform}}, 
  \code{\link{get_pixel_line}}
  
  \item coordinate transformation: \code{\link{transform_xy}}, 
  \code{\link{inv_project}}
  
  \item spatial reference convenience functions: \code{\link{epsg_to_wkt}}, 
  \code{\link{srs_to_wkt}},\cr
  \code{\link{srs_is_geographic}}, \code{\link{srs_is_projected}},
  \code{\link{srs_is_same}}
  
  \item geometry convenience functions: \code{\link{bbox_from_wkt}}, 
  \code{\link{bbox_to_wkt}},\cr
  \code{\link{bbox_intersect}}, \code{\link{bbox_union}}, 
  \code{\link{has_geos}}
  
  \item GDAL configuration: \code{\link{gdal_version}},
  \code{\link{get_cache_used}}, \code{\link{get_config_option}},\cr
  \code{\link{set_config_option}}
  }
  Additional functionality includes:
  \itemize{
  \item class \code{\link{RunningStats}} calculates mean and variance in one 
  pass. The min, max, sum, and count are also tracked (i.e., summary 
  statistics on a data stream).
  
  \item class \code{\link{CmbTable}} implements a hash table for counting 
  unique combinations of integer values.
  
  \item \code{\link{combine}} overlays multiple rasters so that a unique ID 
  is assigned to each unique combination of input values. Pixel counts for 
  each unique combination are obtained, and combination IDs are optionally 
  written to an output raster.
  
  \item \code{\link{calc}} evaluates an R expression for each pixel in a 
  raster layer or stack of layers. Individual pixel coordinates are available 
  as variables in the R expression, as either x/y in the raster projected 
  coordinate system or inverse projected longitude/latitude.
  
  \item \code{\link{plot_raster}} displays raster data using base R
  graphics.
  }
}
\author{
GDAL: \url{https://github.com/OSGeo/gdal/graphs/contributors}

R interface/additional functionality: Chris Toney

Maintainer: Chris Toney <chris.toney at usda.gov>
}
\seealso{
  GDAL Raster Data Model:\cr
  \url{https://gdal.org/user/raster_data_model.html}
  
  Raster format descriptions:\cr
  \url{https://gdal.org/drivers/raster/index.html}
  
  Geotransform tutorial:\cr 
  \url{https://gdal.org/tutorials/geotransforms_tut.html}
}
\note{
  Documentation for several wrapper functions borrows from the GDAL API 
  documentation, (c) 1998-2023, Frank Warmerdam, Even Rouault, and 
  others, \href{https://gdal.org/license.html}{MIT license}.

  Sample datasets are included with the package and used in examples 
  throughout the documentation. The sample data include
  \href{https://landfire.gov/}{LANDFIRE} raster layers describing terrain,
  vegetation and wildland fuels (LF 2020 version), and
  Landsat C2 Analysis Ready Data downloaded from
  \href{https://earthexplorer.usgs.gov/}{USGS Earth Explorer}.

  \code{system.file()} is used in the examples to access the sample datasets. 
  This enables the code to run regardless of where R is installed. Users will 
  normally give file names as a regular full path or relative to the current 
  working directory.
}

