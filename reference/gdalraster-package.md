# Bindings to the GDAL API

gdalraster is an interface to the Geospatial Data Abstraction Library
(GDAL) providing an R implementation of the GDAL Raster and Vector Data
Models. Bindings also include the GDAL Geometry API, Spatial Reference
Systems API, utilities and algorithms, methods for coordinate
transformation, and the Virtual Systems Interface (VSI) API. Calling
signatures resemble those of the native C, C++ and Python APIs provided
by the GDAL project. See <https://gdal.org/en/stable/api/> for details
of the GDAL API.

## Details

Experimental bindings to the modernized `gdal` Command Line Interface
(CLI) are implemented in
[`GDALAlg-class`](https://firelab.github.io/gdalraster/reference/GDALAlg-class.md)
and related convenience functions:
[`gdal_commands()`](https://firelab.github.io/gdalraster/reference/gdal_cli.md),
[`gdal_usage()`](https://firelab.github.io/gdalraster/reference/gdal_cli.md),
[`gdal_run()`](https://firelab.github.io/gdalraster/reference/gdal_cli.md),
[`gdal_alg()`](https://firelab.github.io/gdalraster/reference/gdal_cli.md)
(*Requires GDAL \>= 3.11.3*)

Core raster functionality is contained in class `GDALRaster` and several
related stand-alone functions:

- [`GDALRaster-class`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md)
  is an exposed C++ class that allows opening a raster dataset and
  calling methods on the `GDALDataset`, `GDALDriver` and
  `GDALRasterBand` objects in the underlying API (e.g., get/set
  parameters, read/write pixel data).

- raster creation:
  [`create()`](https://firelab.github.io/gdalraster/reference/create.md),
  [`createCopy()`](https://firelab.github.io/gdalraster/reference/createCopy.md),
  [`rasterFromRaster()`](https://firelab.github.io/gdalraster/reference/rasterFromRaster.md),
  [`translate()`](https://firelab.github.io/gdalraster/reference/translate.md),
  [`getCreationOptions()`](https://firelab.github.io/gdalraster/reference/getCreationOptions.md),
  [`validateCreationOptions()`](https://firelab.github.io/gdalraster/reference/validateCreationOptions.md)

- virtual raster:
  [`autoCreateWarpedVRT()`](https://firelab.github.io/gdalraster/reference/autoCreateWarpedVRT.md),
  [`buildVRT()`](https://firelab.github.io/gdalraster/reference/buildVRT.md),
  [`rasterToVRT()`](https://firelab.github.io/gdalraster/reference/rasterToVRT.md)

- algorithms/utilities:
  [`dem_proc()`](https://firelab.github.io/gdalraster/reference/dem_proc.md),
  [`fillNodata()`](https://firelab.github.io/gdalraster/reference/fillNodata.md),
  [`footprint()`](https://firelab.github.io/gdalraster/reference/footprint.md),
  [`make_chunk_index()`](https://firelab.github.io/gdalraster/reference/make_chunk_index.md),
  [`polygonize()`](https://firelab.github.io/gdalraster/reference/polygonize.md),
  [`rasterize()`](https://firelab.github.io/gdalraster/reference/rasterize.md),
  [`sieveFilter()`](https://firelab.github.io/gdalraster/reference/sieveFilter.md),
  [`warp()`](https://firelab.github.io/gdalraster/reference/warp.md),
  [`GDALRaster$getChecksum()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md)

- raster attribute tables:
  [`buildRAT()`](https://firelab.github.io/gdalraster/reference/buildRAT.md),
  [`displayRAT()`](https://firelab.github.io/gdalraster/reference/displayRAT.md),
  [`GDALRaster$getDefaultRAT()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
  [`GDALRaster$setDefaultRAT()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md)

- multidimensional raster:
  [`mdim_as_classic()`](https://firelab.github.io/gdalraster/reference/mdim_as_classic.md),
  [`mdim_info()`](https://firelab.github.io/gdalraster/reference/mdim_info.md),
  [`mdim_translate()`](https://firelab.github.io/gdalraster/reference/mdim_translate.md)

- geotransform conversion:
  [`apply_geotransform()`](https://firelab.github.io/gdalraster/reference/apply_geotransform.md),
  [`get_pixel_line()`](https://firelab.github.io/gdalraster/reference/get_pixel_line.md),
  [`inv_geotransform()`](https://firelab.github.io/gdalraster/reference/inv_geotransform.md),
  [`pixel_extract()`](https://firelab.github.io/gdalraster/reference/pixel_extract.md)

- data type convenience functions:
  [`dt_size()`](https://firelab.github.io/gdalraster/reference/data_type_helpers.md),
  [`dt_is_complex()`](https://firelab.github.io/gdalraster/reference/data_type_helpers.md),
  [`dt_is_integer()`](https://firelab.github.io/gdalraster/reference/data_type_helpers.md),
  [`dt_is_floating()`](https://firelab.github.io/gdalraster/reference/data_type_helpers.md),
  [`dt_is_signed()`](https://firelab.github.io/gdalraster/reference/data_type_helpers.md),
  [`dt_union()`](https://firelab.github.io/gdalraster/reference/data_type_helpers.md),
  [`dt_union_with_value()`](https://firelab.github.io/gdalraster/reference/data_type_helpers.md),
  [`dt_find()`](https://firelab.github.io/gdalraster/reference/data_type_helpers.md),
  [`dt_find_for_value()`](https://firelab.github.io/gdalraster/reference/data_type_helpers.md)

Core vector functionality is contained in class `GDALVector` and several
related stand-alone functions:

- [`GDALVector-class`](https://firelab.github.io/gdalraster/reference/GDALVector-class.md)
  is an exposed C++ class that allows opening a vector dataset and
  calling methods on a specified `OGRLayer` object that it contains
  (e.g., obtain layer information, set attribute and/or spatial filters,
  read/write feature data).

- OGR vector utilities:
  [`ogrinfo()`](https://firelab.github.io/gdalraster/reference/ogrinfo.md),
  [`ogr2ogr()`](https://firelab.github.io/gdalraster/reference/ogr2ogr.md),
  [`ogr_reproject()`](https://firelab.github.io/gdalraster/reference/ogr_reproject.md),
  [`ogr_define`](https://firelab.github.io/gdalraster/reference/ogr_define.md),
  [`ogr_manage`](https://firelab.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_proc()`](https://firelab.github.io/gdalraster/reference/ogr_proc.md)

Bindings to the GDAL Geometry API, Spatial Reference Systems API,
methods for coordinate transformation, the Virtual Systems Interface
(VSI) API, general data management and system configuration are
implemented in several stand-alone functions:

- Geometry API:
  [`bbox_from_wkt()`](https://firelab.github.io/gdalraster/reference/bbox_from_wkt.md),
  [`bbox_to_wkt()`](https://firelab.github.io/gdalraster/reference/bbox_to_wkt.md),
  [`bbox_intersect()`](https://firelab.github.io/gdalraster/reference/bbox_intersect.md),
  [`bbox_union()`](https://firelab.github.io/gdalraster/reference/bbox_intersect.md),
  [`bbox_transform()`](https://firelab.github.io/gdalraster/reference/bbox_transform.md),
  [`g_factory`](https://firelab.github.io/gdalraster/reference/g_factory.md),
  [`g_wk2wk()`](https://firelab.github.io/gdalraster/reference/g_wk2wk.md),
  [`g_query`](https://firelab.github.io/gdalraster/reference/g_query.md),
  [`g_util`](https://firelab.github.io/gdalraster/reference/g_util.md),
  [`g_binary_pred`](https://firelab.github.io/gdalraster/reference/g_binary_pred.md),
  [`g_binary_op`](https://firelab.github.io/gdalraster/reference/g_binary_op.md),
  [`g_unary_op`](https://firelab.github.io/gdalraster/reference/g_unary_op.md),
  [`g_measures`](https://firelab.github.io/gdalraster/reference/g_measures.md),
  [`g_coords()`](https://firelab.github.io/gdalraster/reference/g_coords.md),
  [`g_envelope()`](https://firelab.github.io/gdalraster/reference/g_envelope.md),
  [`g_transform()`](https://firelab.github.io/gdalraster/reference/g_transform.md),
  [`geos_version()`](https://firelab.github.io/gdalraster/reference/geos_version.md),
  [`plot_geom()`](https://firelab.github.io/gdalraster/reference/plot_geom.md)

- Spatial Reference Systems API:
  [`srs_convert`](https://firelab.github.io/gdalraster/reference/srs_convert.md),
  [`srs_info_from_db()`](https://firelab.github.io/gdalraster/reference/srs_info_from_db.md),
  [`srs_query`](https://firelab.github.io/gdalraster/reference/srs_query.md)

- coordinate transformation:
  [`transform_xy()`](https://firelab.github.io/gdalraster/reference/transform_xy.md),
  [`inv_project()`](https://firelab.github.io/gdalraster/reference/inv_project.md),
  [`transform_bounds()`](https://firelab.github.io/gdalraster/reference/transform_bounds.md)

- data management:
  [`addFilesInZip()`](https://firelab.github.io/gdalraster/reference/addFilesInZip.md),
  [`copyDatasetFiles()`](https://firelab.github.io/gdalraster/reference/copyDatasetFiles.md),
  [`deleteDataset()`](https://firelab.github.io/gdalraster/reference/deleteDataset.md),
  [`renameDataset()`](https://firelab.github.io/gdalraster/reference/renameDataset.md),
  [`bandCopyWholeRaster()`](https://firelab.github.io/gdalraster/reference/bandCopyWholeRaster.md),
  [`identifyDriver()`](https://firelab.github.io/gdalraster/reference/identifyDriver.md),
  [`inspectDataset()`](https://firelab.github.io/gdalraster/reference/inspectDataset.md)

- Virtual Systems Interface API:
  [`VSIFile-class`](https://firelab.github.io/gdalraster/reference/VSIFile-class.md),
  [`vsi_clear_path_options()`](https://firelab.github.io/gdalraster/reference/vsi_clear_path_options.md),
  [`vsi_copy_file()`](https://firelab.github.io/gdalraster/reference/vsi_copy_file.md),
  [`vsi_curl_clear_cache()`](https://firelab.github.io/gdalraster/reference/vsi_curl_clear_cache.md),
  [`vsi_get_disk_free_space()`](https://firelab.github.io/gdalraster/reference/vsi_get_disk_free_space.md),
  [`vsi_get_file_metadata()`](https://firelab.github.io/gdalraster/reference/vsi_get_file_metadata.md),
  [`vsi_get_fs_options()`](https://firelab.github.io/gdalraster/reference/vsi_get_fs_options.md),
  [`vsi_get_fs_prefixes()`](https://firelab.github.io/gdalraster/reference/vsi_get_fs_prefixes.md),
  [`vsi_is_local()`](https://firelab.github.io/gdalraster/reference/vsi_is_local.md),
  [`vsi_mkdir()`](https://firelab.github.io/gdalraster/reference/vsi_mkdir.md),
  [`vsi_read_dir()`](https://firelab.github.io/gdalraster/reference/vsi_read_dir.md),
  [`vsi_rename()`](https://firelab.github.io/gdalraster/reference/vsi_rename.md),
  [`vsi_rmdir()`](https://firelab.github.io/gdalraster/reference/vsi_rmdir.md),
  [`vsi_set_path_option()`](https://firelab.github.io/gdalraster/reference/vsi_set_path_option.md),
  [`vsi_stat()`](https://firelab.github.io/gdalraster/reference/vsi_stat.md),
  [`vsi_supports_rnd_write()`](https://firelab.github.io/gdalraster/reference/vsi_supports_rnd_write.md),
  [`vsi_supports_seq_write()`](https://firelab.github.io/gdalraster/reference/vsi_supports_seq_write.md),
  [`vsi_sync()`](https://firelab.github.io/gdalraster/reference/vsi_sync.md),
  [`vsi_unlink()`](https://firelab.github.io/gdalraster/reference/vsi_unlink.md),
  [`vsi_unlink_batch()`](https://firelab.github.io/gdalraster/reference/vsi_unlink_batch.md)

- GDAL configuration:
  [`gdal_version`](https://firelab.github.io/gdalraster/reference/gdal_version.md),
  [`gdal_compute_version()`](https://firelab.github.io/gdalraster/reference/gdal_compute_version.md),
  [`gdal_formats()`](https://firelab.github.io/gdalraster/reference/gdal_formats.md),
  [`gdal_get_driver_md()`](https://firelab.github.io/gdalraster/reference/gdal_get_driver_md.md),
  [`get_cache_used()`](https://firelab.github.io/gdalraster/reference/get_cache_used.md),
  [`get_cache_max()`](https://firelab.github.io/gdalraster/reference/get_cache_max.md),
  [`set_cache_max()`](https://firelab.github.io/gdalraster/reference/set_cache_max.md),
  [`get_config_option()`](https://firelab.github.io/gdalraster/reference/get_config_option.md),
  [`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md),
  [`get_num_cpus()`](https://firelab.github.io/gdalraster/reference/get_num_cpus.md),
  [`get_usable_physical_ram()`](https://firelab.github.io/gdalraster/reference/get_usable_physical_ram.md),
  [`has_spatialite()`](https://firelab.github.io/gdalraster/reference/has_spatialite.md),
  [`http_enabled()`](https://firelab.github.io/gdalraster/reference/http_enabled.md),
  [`push_error_handler()`](https://firelab.github.io/gdalraster/reference/push_error_handler.md),
  [`pop_error_handler()`](https://firelab.github.io/gdalraster/reference/pop_error_handler.md),
  [`dump_open_datasets()`](https://firelab.github.io/gdalraster/reference/dump_open_datasets.md)

- PROJ configuration:
  [`proj_version()`](https://firelab.github.io/gdalraster/reference/proj_version.md),
  [`proj_search_paths()`](https://firelab.github.io/gdalraster/reference/proj_search_paths.md),
  [`proj_networking()`](https://firelab.github.io/gdalraster/reference/proj_networking.md)

Additional functionality includes:

- [`RunningStats-class`](https://firelab.github.io/gdalraster/reference/RunningStats-class.md)
  calculates mean and variance in one pass. The min, max, sum, and count
  are also tracked (efficient summary statistics on data streams).

- [`CmbTable-class`](https://firelab.github.io/gdalraster/reference/CmbTable-class.md)
  implements a hash table for counting unique combinations of integer
  values.

- [`combine()`](https://firelab.github.io/gdalraster/reference/combine.md)
  overlays multiple rasters so that a unique ID is assigned to each
  unique combination of input values. Pixel counts for each unique
  combination are obtained, and combination IDs are optionally written
  to an output raster.

- [`calc()`](https://firelab.github.io/gdalraster/reference/calc.md)
  evaluates an R expression for each pixel in a raster layer or stack of
  layers. Individual pixel coordinates are available as variables in the
  R expression, as either x/y in the raster projected coordinate system
  or inverse projected longitude/latitude.

- [`plot_raster()`](https://firelab.github.io/gdalraster/reference/plot_raster.md)
  displays raster data using base R `graphics`. Supports single-band
  grayscale, RGB, color tables and color map functions (e.g., color
  ramp).

## Author

GDAL is by: Frank Warmerdam, Even Rouault and others  
(see <https://github.com/OSGeo/gdal/graphs/contributors>)

R interface/additional functionality: Chris Toney

Maintainer: Chris Toney \<jctoney at gmail.com\>

## See also

GDAL Raster Data Model:  
<https://gdal.org/en/stable/user/raster_data_model.html>

Raster driver descriptions:  
<https://gdal.org/en/stable/drivers/raster/index.html>

Geotransform tutorial:  
<https://gdal.org/en/stable/tutorials/geotransforms_tut.html>

GDAL Vector Data Model:  
<https://gdal.org/en/stable/user/vector_data_model.html>

Vector driver descriptions:  
<https://gdal.org/en/stable/drivers/vector/index.html>

GDAL Virtual File Systems:  
<https://gdal.org/en/stable/user/virtual_file_systems.html>

## Note

Documentation for the API bindings borrows heavily from the GDAL
documentation, (c) 1998-2025, Frank Warmerdam, Even Rouault, and others,
[MIT license](https://gdal.org/en/stable/license.html).

Sample datasets included with the package are used in examples
throughout the documentation. The sample data sources include:

- [LANDFIRE](https://landfire.gov/) raster layers describing terrain,
  vegetation and wildland fuels (LF 2020 version)

- Landsat C2 Analysis Ready Data from [USGS Earth
  Explorer](https://earthexplorer.usgs.gov/)

- Monitoring Trends in Burn Severity ([MTBS](https://www.mtbs.gov/))
  fire perimeters from 1984-2022

- [NLCD Tree Canopy
  Cover](https://data.fs.usda.gov/geodata/rastergateway/treecanopycover/)
  produced by USDA Forest Service

- [National Park Service Open
  Data](https://public-nps.opendata.arcgis.com/) vector layers for roads
  and points-of-interest

- [Montana State Library](https://msl.mt.gov/geoinfo/) boundary layer
  for Yellowstone National Park

Metadata for these sample datasets are in inst/extdata/metadata.zip and
inst/extdata/ynp_features.zip.

[`system.file()`](https://rdrr.io/r/base/system.file.html) is used in
the examples to access the sample datasets. This enables the code to run
regardless of where R is installed. Users will normally give file names
as a regular full path or relative to the current working directory.

Temporary files are created in some examples which have cleanup code
wrapped in `dontshow`. While the cleanup code is not shown in the
documentation, note that this code runs by default if examples are run
with [`example()`](https://rdrr.io/r/utils/example.html).
