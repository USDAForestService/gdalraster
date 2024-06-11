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
  \code{\link[=translate]{translate()}},
  \code{\link[=getCreationOptions]{getCreationOptions()}}

  \item virtual raster: \code{\link[=buildVRT]{buildVRT()}},
  \code{\link[=rasterToVRT]{rasterToVRT()}}

  \item reproject/resample/crop/mosaic: \code{\link[=warp]{warp()}}

  \item algorithms: \code{\link[=dem_proc]{dem_proc()}},
  \code{\link[=fillNodata]{fillNodata()}},
  \code{\link[=footprint]{footprint()}},
  \code{\link[=polygonize]{polygonize()}},
  \code{\link[=rasterize]{rasterize()}},
  \code{\link[=sieveFilter]{sieveFilter()}},
  \code{\link[=GDALRaster]{GDALRaster$getChecksum()}}

  \item raster attribute tables: \code{\link[=buildRAT]{buildRAT()}},
  \code{\link[=displayRAT]{displayRAT()}},
  \code{\link[=GDALRaster]{GDALRaster$getDefaultRAT()}},
  \code{\link[=GDALRaster]{GDALRaster$setDefaultRAT()}}

  \item geotransform conversion:
  \code{\link[=apply_geotransform]{apply_geotransform()}},
  \code{\link[=get_pixel_line]{get_pixel_line()}},
  \code{\link[=inv_geotransform]{inv_geotransform()}}

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
  \code{\link[=bbox_transform]{bbox_transform()}},
  \code{\link[=g_area]{g_area()}},
  \code{\link[=g_buffer]{g_buffer()}},
  \code{\link[=g_centroid]{g_centroid()}},
  \code{\link[=g_contains]{g_contains()}},
  \code{\link[=g_crosses]{g_crosses()}},
  \code{\link[=g_difference]{g_difference()}},
  \code{\link[=g_disjoint]{g_disjoint()}},
  \code{\link[=g_distance]{g_distance()}},
  \code{\link[=g_equals]{g_equals()}},
  \code{\link[=g_intersection]{g_intersection()}},
  \code{\link[=g_intersects]{g_intersects()}},
  \code{\link[=g_is_empty]{g_is_empty()}},
  \code{\link[=g_is_valid]{g_is_valid()}},
  \code{\link[=g_length]{g_length()}},
  \code{\link[=g_name]{g_name()}},
  \code{\link[=g_overlaps]{g_overlaps()}},
  \code{\link[=g_sym_difference]{g_sym_difference()}},
  \code{\link[=g_touches]{g_touches()}},
  \code{\link[=g_transform]{g_transform()}},
  \code{\link[=g_union]{g_union()}},
  \code{\link[=g_within]{g_within()}},
  \code{\link[=geos_version]{geos_version()}}

  \item data management: \code{\link[=addFilesInZip]{addFilesInZip()}},
  \code{\link[=copyDatasetFiles]{copyDatasetFiles()}},
  \code{\link[=deleteDataset]{deleteDataset()}},\cr
  \code{\link[=renameDataset]{renameDataset()}},
  \code{\link[=bandCopyWholeRaster]{bandCopyWholeRaster()}}

  \item OGR vector utilities:
  \code{\link[=ogr2ogr]{ogr2ogr()}},
  \code{\link[=ogrinfo]{ogrinfo()}},
  \code{\link[=ogr_manage]{ogr_manage}},
  \code{\link[=ogr_define]{ogr_define}}

  \item virtual file systems:
  \code{\link[=VSIFile-class]{VSIFile}},
  \code{\link[=vsi_clear_path_options]{vsi_clear_path_options()}},
  \code{\link[=vsi_copy_file]{vsi_copy_file()}},
  \code{\link[=vsi_curl_clear_cache]{vsi_curl_clear_cache()}},
  \code{\link[=vsi_get_disk_free_space]{vsi_get_disk_free_space()}},
  \code{\link[=vsi_get_file_metadata]{vsi_get_file_metadata()}},
  \code{\link[=vsi_get_fs_options]{vsi_get_fs_options()}},
  \code{\link[=vsi_get_fs_prefixes]{vsi_get_fs_prefixes()}},
  \code{\link[=vsi_mkdir]{vsi_mkdir()}},
  \code{\link[=vsi_read_dir]{vsi_read_dir()}},
  \code{\link[=vsi_rename]{vsi_rename()}},
  \code{\link[=vsi_rmdir]{vsi_rmdir()}},
  \code{\link[=vsi_set_path_option]{vsi_set_path_option()}},
  \code{\link[=vsi_stat]{vsi_stat()}},
  \code{\link[=vsi_supports_rnd_write]{vsi_supports_rnd_write()}},
  \code{\link[=vsi_supports_seq_write]{vsi_supports_seq_write()}},
  \code{\link[=vsi_sync]{vsi_sync()}},
  \code{\link[=vsi_unlink]{vsi_unlink()}},
  \code{\link[=vsi_unlink_batch]{vsi_unlink_batch()}}

  \item GDAL configuration:
  \code{\link[=gdal_version]{gdal_version()}},
  \code{\link[=gdal_formats]{gdal_formats()}},
  \code{\link[=get_cache_used]{get_cache_used()}},
  \code{\link[=get_config_option]{get_config_option()}},
  \code{\link[=set_config_option]{set_config_option()}},
  \code{\link[=get_num_cpus]{get_num_cpus()}},
  \code{\link[=get_usable_physical_ram]{get_usable_physical_ram()}},
  \code{\link[=has_spatialite]{has_spatialite()}},
  \code{\link[=http_enabled]{http_enabled()}},
  \code{\link[=push_error_handler]{push_error_handler()}},
  \code{\link[=pop_error_handler]{pop_error_handler()}},
  \code{\link[=dump_open_datasets]{dump_open_datasets()}}

  \item PROJ configuration:
  \code{\link[=proj_version]{proj_version()}},
  \code{\link[=proj_search_paths]{proj_search_paths()}},
  \code{\link[=proj_networking]{proj_networking()}}
  }
  Additional functionality includes:
  \itemize{
  \item \code{\link{RunningStats-class}} calculates mean and variance in one
  pass. The min, max, sum, and count are also tracked (efficient summary
  statistics on data streams).

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

  GDAL Virtual File Systems:\cr
  \url{https://gdal.org/user/virtual_file_systems.html}
}
\note{
  Documentation for \code{GDALRaster-class} and several wrapper functions
  borrows from the GDAL API documentation, (c) 1998-2024, Frank Warmerdam,
  Even Rouault, and others, \href{https://gdal.org/license.html}{MIT license}.

  Sample datasets included with the package are used in examples throughout
  the documentation. The sample data include
  \href{https://landfire.gov/}{LANDFIRE} raster layers describing terrain,
  vegetation and wildland fuels (LF 2020 version),
  Landsat C2 Analysis Ready Data from
  \href{https://earthexplorer.usgs.gov/}{USGS Earth Explorer}, and
  Monitoring Trends in Burn Severity (\href{https://www.mtbs.gov/}{MTBS})
  fire perimeters from 1984-2022. Metadata for the sample datasets are in
  inst/extdata/metadata.zip.

  \code{system.file()} is used in the examples to access the sample datasets.
  This enables the code to run regardless of where R is installed. Users will
  normally give file names as a regular full path or relative to the current
  working directory.
}

