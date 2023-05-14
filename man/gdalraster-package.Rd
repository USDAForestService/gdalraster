\name{gdalraster-package}
\alias{gdalraster-package}
\docType{package}
\title{
  Bindings to the GDAL Raster API
}
\description{
  \code{gdalraster} is an interface to the Geospatial Data Abstraction 
  Library (GDAL) for low level raster I/O. Calling signatures resemble those 
  of the native C, C++ and Python APIs provided by the GDAL project.
  See \url{https://gdal.org/api/index.html} for details of the GDAL Raster API.
}
\details{
  Core functionality is contained in the \code{GDALRaster} class and several 
  GDAL related stand-alone functions:
  \itemize{
  \item class \code{GDALRaster} allows opening a raster dataset and calling 
  methods on the Dataset, Driver and RasterBand objects in the underlying API 
  (get/set parameters, read/write pixel data).
  
  \item \code{rasterToVRT()} creates a virtual raster file (VRT) pointing 
  to an existing raster, with options for virtual subsetting, resampling and
  kernel filtering.
  
  \item creation functions: \code{create()}, \code{createCopy()}, 
  \code{rasterFromRaster()}
  
  \item geotransform conversion: \code{inv_geotransform()}, 
  \code{get_pixel_line()}
  
  \item reproject: \code{warp()}
  
  \item coordinate transformation: \code{transform_xy()}, \code{inv_project()}
  
  \item spatial reference convenience functions: \code{epsg_to_wkt()},
  \code{srs_to_wkt()},\cr
  \code{srs_is_geographic()}, \code{srs_is_projected()}
  
  \item bounding box convenience functions: \code{bbox_from_wkt()}, 
  \code{bbox_to_wkt()},\cr
  \code{bbox_intersect()}, \code{bbox_union()}
  
  \item GDAL configuration: \code{gdal_version()}, \code{get_config_option()}, 
  \code{set_config_option()}
  }
  Additional functionality is intended to support scalable raster analysis:
  \itemize{
  \item class \code{RunningStats} calculates mean and variance in one pass.
  The min, max, sum, and count are also tracked (i.e., summary statistics on 
  a data stream).
  
  \item class \code{CmbTable} implements a hash table for counting unique 
  combinations of integer values.
  
  \item \code{combine()} overlays multiple rasters so that a unique ID is 
  assigned to each unique combination of input values. Pixel counts for each 
  unique combination are obtained, and combination IDs are optionally written
  to an output raster.
  
  \item \code{calc()} evaluates an R expression for each pixel in a raster 
  layer or stack of layers. Individual pixel coordinates are available for 
  use as variables in the R expession, as either x/y in the raster projected 
  coordinate system or inverse projected longitude/latitude.
  }
}
\author{
Chris Toney

Maintainer: Chris Toney <chris.toney at usda.gov>
}
\keyword{ package }
\seealso{
  GDAL Raster Data Model:\cr
  \url{https://gdal.org/user/raster_data_model.html}
  
  Raster format descriptions:\cr
  \url{https://gdal.org/drivers/raster/index.html}
  
  Geotransform tutorial:\cr 
  \url{https://gdal.org/tutorials/geotransforms_tut.html}
}
\note{
  Sample datasets are included with the package and used in examples 
  throughout the documentation. The sample data include LANDFIRE raster layers 
  describing terrain, vegetation and wildland fuels (LF 2020 version, 
  \url{https://landfire.gov/}). Landsat C2 Analysis Ready Data were 
  downloaded from USGS Earth Explorer (\url{https://earthexplorer.usgs.gov/}).

  \code{system.file()} is used in the examples to access the sample datasets. 
  This enables the code to run regardless of where R is installed. Users will 
  normally give file names as a regular full path or relative to the current 
  working directory.
}

