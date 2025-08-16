\docType{package}
\name{gdalraster-package}
\alias{gdalraster}
\alias{gdalraster-package}
\title{Bindings to the GDAL API
}
\description{
  \pkg{gdalraster} is an interface to the Geospatial Data Abstraction
  Library (GDAL) providing an R implementation of the GDAL Raster and Vector
  Data Models. Bindings also include the GDAL Geometry API, Spatial Reference
  Systems API, utilities and algorithms, methods for coordinate transformation,
  and the Virtual Systems Interface (VSI) API. Calling signatures resemble those
  of the native C, C++ and Python APIs provided by the GDAL project.
  See \url{https://gdal.org/en/stable/api/} for details of the GDAL API.
}
\details{
  Initial experimental bindings to the modernized \dQuote{gdal} Command Line
  Interface (CLI) are implemented in \code{\link{GDALAlg-class}} and related
  convenience functions: \code{\link[=gdal_commands]{gdal_commands()}},
  \code{\link[=gdal_usage]{gdal_usage()}}, \code{\link[=gdal_run]{gdal_run()}},
  \code{\link[=gdal_alg]{gdal_alg()}} (\emph{Requires GDAL >= 3.11.3})

  Core raster functionality is contained in class \code{GDALRaster} and several
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
  \code{\link[=getCreationOptions]{getCreationOptions()}},
  \code{\link[=validateCreationOptions]{validateCreationOptions()}}

  \item virtual raster:
  \code{\link[=autoCreateWarpedVRT]{autoCreateWarpedVRT()}},
  \code{\link[=buildVRT]{buildVRT()}},
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
  \code{\link[=inv_geotransform]{inv_geotransform()}},
  \code{\link[=pixel_extract]{pixel_extract()}}

  \item data type convenience functions:
  \code{\link[=dt_size]{dt_size()}},
  \code{\link[=dt_is_complex]{dt_is_complex()}},
  \code{\link[=dt_is_integer]{dt_is_integer()}},
  \code{\link[=dt_is_floating]{dt_is_floating()}},
  \code{\link[=dt_is_signed]{dt_is_signed()}},
  \code{\link[=dt_union]{dt_union()}},
  \code{\link[=dt_union_with_value]{dt_union_with_value()}},
  \code{\link[=dt_find]{dt_find()}},
  \code{\link[=dt_find_for_value]{dt_find_for_value()}}
  }

  Core vector functionality is contained in class \code{GDALVector} and several
  related stand-alone functions:
  \itemize{
  \item \code{\link{GDALVector-class}} is an exposed C++ class that allows
  opening a vector dataset and calling methods on a specified \code{OGRLayer}
  object that it contains (e.g., obtain layer information, set attribute and/or
  spatial filters, read/write feature data).

  \item OGR vector utilities:
  \code{\link[=ogrinfo]{ogrinfo()}},
  \code{\link[=ogr2ogr]{ogr2ogr()}},
  \code{\link[=ogr_reproject]{ogr_reproject()}},
  \code{\link[=ogr_define]{ogr_define}},
  \code{\link[=ogr_manage]{ogr_manage}},
  \code{\link[=ogr_proc]{ogr_proc()}}
  }

  Bindings to the GDAL Geometry API, Spatial Reference Systems API, methods for
  coordinate transformation, the Virtual Systems Interface (VSI) API, general
  data management and system configuration are implemented in several
  stand-alone functions:
  \itemize{
  \item Geometry API:
  \code{\link[=bbox_from_wkt]{bbox_from_wkt()}},
  \code{\link[=bbox_to_wkt]{bbox_to_wkt()}},
  \code{\link[=bbox_intersect]{bbox_intersect()}},
  \code{\link[=bbox_union]{bbox_union()}},
  \code{\link[=bbox_transform]{bbox_transform()}},
  \code{\link[=g_factory]{g_factory}},
  \code{\link[=g_wk2wk]{g_wk2wk()}},
  \code{\link[=g_query]{g_query}},
  \code{\link[=g_util]{g_util}},
  \code{\link[=g_binary_pred]{g_binary_pred}},
  \code{\link[=g_binary_op]{g_binary_op}},
  \code{\link[=g_unary_op]{g_unary_op}},
  \code{\link[=g_measures]{g_measures}},
  \code{\link[=g_coords]{g_coords()}},
  \code{\link[=g_envelope]{g_envelope()}},
  \code{\link[=g_transform]{g_transform()}},
  \code{\link[=geos_version]{geos_version()}}

  \item Spatial Reference Systems API:
  \code{\link[=srs_convert]{srs_convert}},
  \code{\link[=srs_query]{srs_query}}

  \item coordinate transformation:
  \code{\link[=transform_xy]{transform_xy()}},
  \code{\link[=inv_project]{inv_project()}},
  \code{\link[=transform_bounds]{transform_bounds()}}

  \item data management: \code{\link[=addFilesInZip]{addFilesInZip()}},
  \code{\link[=copyDatasetFiles]{copyDatasetFiles()}},
  \code{\link[=deleteDataset]{deleteDataset()}},
  \code{\link[=renameDataset]{renameDataset()}},
  \code{\link[=bandCopyWholeRaster]{bandCopyWholeRaster()}},
  \code{\link[=identifyDriver]{identifyDriver()}},
  \code{\link[=inspectDataset]{inspectDataset()}}

  \item Virtual Systems Interface API:
  \code{\link{VSIFile-class}},
  \code{\link[=vsi_clear_path_options]{vsi_clear_path_options()}},
  \code{\link[=vsi_copy_file]{vsi_copy_file()}},
  \code{\link[=vsi_curl_clear_cache]{vsi_curl_clear_cache()}},
  \code{\link[=vsi_get_disk_free_space]{vsi_get_disk_free_space()}},
  \code{\link[=vsi_get_file_metadata]{vsi_get_file_metadata()}},
  \code{\link[=vsi_get_fs_options]{vsi_get_fs_options()}},
  \code{\link[=vsi_get_fs_prefixes]{vsi_get_fs_prefixes()}},
  \code{\link[=vsi_is_local]{vsi_is_local()}},
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
  \code{\link[=gdal_version]{gdal_version}},
  \code{\link[=gdal_compute_version]{gdal_compute_version()}},
  \code{\link[=gdal_formats]{gdal_formats()}},
  \code{\link[=get_cache_used]{get_cache_used()}},
  \code{\link[=get_cache_max]{get_cache_max()}},
  \code{\link[=set_cache_max]{set_cache_max()}},
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

  Maintainer: Chris Toney <jctoney at gmail.com>
}
\seealso{
  GDAL Raster Data Model:\cr
  \url{https://gdal.org/en/stable/user/raster_data_model.html}

  Raster driver descriptions:\cr
  \url{https://gdal.org/en/stable/drivers/raster/index.html}

  Geotransform tutorial:\cr
  \url{https://gdal.org/en/stable/tutorials/geotransforms_tut.html}

  GDAL Vector Data Model:\cr
  \url{https://gdal.org/en/stable/user/vector_data_model.html}

  Vector driver descriptions:\cr
  \url{https://gdal.org/en/stable/drivers/vector/index.html}

  GDAL Virtual File Systems:\cr
  \url{https://gdal.org/en/stable/user/virtual_file_systems.html}
}
\note{
  Documentation for the API bindings borrows heavily from the GDAL
  documentation, (c) 1998-2025, Frank Warmerdam, Even Rouault, and others,
  \href{https://gdal.org/en/stable/license.html}{MIT license}.

  Sample datasets included with the package are used in examples throughout
  the documentation. The sample data sources include:
  \itemize{
  \item \href{https://landfire.gov/}{LANDFIRE} raster layers describing terrain,
  vegetation and wildland fuels (LF 2020 version)

  \item Landsat C2 Analysis Ready Data from
  \href{https://earthexplorer.usgs.gov/}{USGS Earth Explorer}

  \item Monitoring Trends in Burn Severity (\href{https://www.mtbs.gov/}{MTBS})
  fire perimeters from 1984-2022

  \item
  \href{https://data.fs.usda.gov/geodata/rastergateway/treecanopycover/}{NLCD Tree Canopy Cover}
  produced by USDA Forest Service

  \item
  \href{https://public-nps.opendata.arcgis.com/}{National Park Service Open Data}
  vector layers for roads and points-of-interest

  \item \href{https://msl.mt.gov/geoinfo/}{Montana State Library} boundary layer
  for Yellowstone National Park
  }

  Metadata for these sample datasets are in inst/extdata/metadata.zip and
  inst/extdata/ynp_features.zip.

  \code{system.file()} is used in the examples to access the sample datasets.
  This enables the code to run regardless of where R is installed. Users will
  normally give file names as a regular full path or relative to the current
  working directory.

  Temporary files are created in some examples which have cleanup code wrapped
  in \code{dontshow{}}. While the cleanup code is not shown in the
  documentation, note that this code runs by default if examples are run with
  \code{example()}.
}
