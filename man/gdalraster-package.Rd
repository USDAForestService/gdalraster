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
  Core functionality is contained in class \code{GDALRaster} and several
  related stand-alone functions:
  \itemize{
  \item \code{\link{GDALRaster-class}} is an exposed C++ class that allows
  opening a raster dataset and calling methods on the \code{GDALDataset},
  \code{GDALDriver} and \code{GDALRasterBand} objects in the underlying API
  (e.g., get/set parameters, read/write pixel data).
  
  \item raster creation: \code{\link[=create]{create()}},
  \code{\link[=createCopy]{createCopy()}},
  \code{\link[=rasterFromRaster]{rasterFromRaster()}},
  \code{\link[=getCreationOptions]{getCreationOptions()}}
  
  \item data management: \code{\link[=copyDatasetFiles]{copyDatasetFiles()}},
  \code{\link[=deleteDataset]{deleteDataset()}},
  \code{\link[=renameDataset]{renameDataset()}},\cr
  \code{\link[=bandCopyWholeRaster]{bandCopyWholeRaster()}}
 
  \item virtual raster: \code{\link[=rasterToVRT]{rasterToVRT()}}
  
  \item reproject: \code{\link[=warp]{warp()}}
  
  \item algorithms: \code{\link[=dem_proc]{dem_proc()}},
  \code{\link[=fillNodata]{fillNodata()}},
  \code{\link[=sieveFilter]{sieveFilter()}},
  \code{\link[=GDALRaster]{GDALRaster$getChecksum()}}
  
  \item raster attribute tables: \code{\link[=buildRAT]{buildRAT()}},
  \code{\link[=displayRAT]{displayRAT()}},
  \code{\link[=GDALRaster]{GDALRaster$getDefaultRAT()}},\cr
  \code{\link[=GDALRaster]{GDALRaster$setDefaultRAT()}}
  
  \item geotransform conversion:
  \code{\link[=inv_geotransform]{inv_geotransform()}}, 
  \code{\link[=get_pixel_line]{get_pixel_line()}}
  
  \item coordinate transformation:
  \code{\link[=transform_xy]{transform_xy()}}, 
  \code{\link[=inv_project]{inv_project()}}
  
  \item spatial reference convenience functions:
  \code{\link[=epsg_to_wkt]{epsg_to_wkt()}},
  \code{\link[=srs_to_wkt]{srs_to_wkt()}},\cr
  \code{\link[=srs_is_geographic]{srs_is_geographic()}},
  \code{\link[=srs_is_projected]{srs_is_projected()}},
  \code{\link[=srs_is_same]{srs_is_same()}}
  
  \item geometry convenience functions:
  \code{\link[=bbox_from_wkt]{bbox_from_wkt()}}, 
  \code{\link[=bbox_to_wkt]{bbox_to_wkt()}},
  \code{\link[=bbox_intersect]{bbox_intersect()}},
  \code{\link[=bbox_union]{bbox_union()}}, 
  \code{\link[=g_buffer]{g_buffer()}},
  \code{\link[=has_geos]{has_geos()}}
  
  \item GDAL configuration:
  \code{\link[=gdal_version]{gdal_version()}},
  \code{\link[=gdal_formats]{gdal_formats()}},
  \code{\link[=get_cache_used]{get_cache_used()}},\cr
  \code{\link[=get_config_option]{get_config_option()}},
  \code{\link[=set_config_option]{set_config_option()}}
  
  \item PROJ configuration:
  \code{\link[=proj_version]{proj_version()}},
  \code{\link[=proj_search_paths]{proj_search_paths()}},
  \code{\link[=proj_networking]{proj_networking()}}
  }
  Additional functionality includes:
  \itemize{
  \item \code{\link{RunningStats-class}} calculates mean and variance in one
  pass. The min, max, sum, and count are also tracked (efficient summary
  statistics on large data streams).
  
  \item \code{\link{CmbTable-class}} implements a hash table for counting
  unique combinations of integer values.
  
  \item \code{\link[=combine]{combine()}} overlays multiple rasters so that a
  unique ID is assigned to each unique combination of input values. Pixel
  counts for each unique combination are obtained, and combination IDs are
  optionally written to an output raster.
  
  \item \code{\link[=calc]{calc()}} evaluates an R expression for each pixel in
  a raster layer or stack of layers. Individual pixel coordinates are available
  as variables in the R expression, as either x/y in the raster projected
  coordinate system or inverse projected longitude/latitude.
  
  \item \code{\link[=plot_raster]{plot_raster()}} displays raster data using
  base R \code{graphics}. Supports single-band grayscale, RGB, color tables and
  color map functions (e.g., color ramp).
  }
}
\author{
GDAL is by: Frank Warmerdam, Even Rouault and others\cr
(see \url{https://github.com/OSGeo/gdal/graphs/contributors})

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
  Documentation for \code{GDALRaster-class} and several wrapper functions
  borrows from the GDAL API documentation, (c) 1998-2023, Frank Warmerdam,
  Even Rouault, and others, \href{https://gdal.org/license.html}{MIT license}.

  Sample datasets included with the package are used in examples throughout
  the documentation. The sample data include
  \href{https://landfire.gov/}{LANDFIRE} raster layers describing terrain,
  vegetation and wildland fuels (LF 2020 version), and
  Landsat C2 Analysis Ready Data downloaded from
  \href{https://earthexplorer.usgs.gov/}{USGS Earth Explorer}.

  \code{system.file()} is used in the examples to access the sample datasets. 
  This enables the code to run regardless of where R is installed. Users will 
  normally give file names as a regular full path or relative to the current 
  working directory.
}

