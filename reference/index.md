# Package index

## Package overview

- [`gdalraster`](https://usdaforestservice.github.io/gdalraster/reference/gdalraster-package.md)
  [`gdalraster-package`](https://usdaforestservice.github.io/gdalraster/reference/gdalraster-package.md)
  : Bindings to the GDAL API

## Exposed C++ classes

- [`GDALAlg-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALAlg-class.md)
  [`Rcpp_GDALAlg`](https://usdaforestservice.github.io/gdalraster/reference/GDALAlg-class.md)
  [`Rcpp_GDALAlg-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALAlg-class.md)
  [`GDALAlg`](https://usdaforestservice.github.io/gdalraster/reference/GDALAlg-class.md)
  : Class encapsulating a GDAL CLI algorithm
- [`GDALRaster-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.md)
  [`Rcpp_GDALRaster`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.md)
  [`Rcpp_GDALRaster-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.md)
  [`GDALRaster`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.md)
  : Class encapsulating a raster dataset and associated band objects
- [`GDALVector-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.md)
  [`Rcpp_GDALVector`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.md)
  [`Rcpp_GDALVector-class`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.md)
  [`GDALVector`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.md)
  : Class encapsulating a vector layer in a GDAL dataset
- [`CmbTable-class`](https://usdaforestservice.github.io/gdalraster/reference/CmbTable-class.md)
  [`Rcpp_CmbTable`](https://usdaforestservice.github.io/gdalraster/reference/CmbTable-class.md)
  [`Rcpp_CmbTable-class`](https://usdaforestservice.github.io/gdalraster/reference/CmbTable-class.md)
  [`CmbTable`](https://usdaforestservice.github.io/gdalraster/reference/CmbTable-class.md)
  : Class for counting unique combinations of integers
- [`RunningStats-class`](https://usdaforestservice.github.io/gdalraster/reference/RunningStats-class.md)
  [`Rcpp_RunningStats`](https://usdaforestservice.github.io/gdalraster/reference/RunningStats-class.md)
  [`Rcpp_RunningStats-class`](https://usdaforestservice.github.io/gdalraster/reference/RunningStats-class.md)
  [`RunningStats`](https://usdaforestservice.github.io/gdalraster/reference/RunningStats-class.md)
  : Class to calculate mean and variance in one pass
- [`VSIFile-class`](https://usdaforestservice.github.io/gdalraster/reference/VSIFile-class.md)
  [`Rcpp_VSIFile`](https://usdaforestservice.github.io/gdalraster/reference/VSIFile-class.md)
  [`Rcpp_VSIFile-class`](https://usdaforestservice.github.io/gdalraster/reference/VSIFile-class.md)
  [`VSIFile`](https://usdaforestservice.github.io/gdalraster/reference/VSIFile-class.md)
  : Class wrapping the GDAL VSIVirtualHandle API for binary file I/O

## Stand-alone functions

### gdal CLI (GDAL \>= 3.11.3)

- [`gdal_commands()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md)
  [`gdal_usage()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md)
  [`gdal_run()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md)
  [`gdal_alg()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md)
  [`gdal_global_reg_names()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md)
  : Functions for using GDAL CLI algorithms

### Raster creation

- [`create()`](https://usdaforestservice.github.io/gdalraster/reference/create.md)
  : Create a new uninitialized raster
- [`createCopy()`](https://usdaforestservice.github.io/gdalraster/reference/createCopy.md)
  : Create a copy of a raster
- [`getCreationOptions()`](https://usdaforestservice.github.io/gdalraster/reference/getCreationOptions.md)
  : Return the list of creation options for a GDAL driver
- [`validateCreationOptions()`](https://usdaforestservice.github.io/gdalraster/reference/validateCreationOptions.md)
  : Validate the list of creation options that are handled by a driver
- [`rasterFromRaster()`](https://usdaforestservice.github.io/gdalraster/reference/rasterFromRaster.md)
  : Create a raster from an existing raster as template
- [`translate()`](https://usdaforestservice.github.io/gdalraster/reference/translate.md)
  : Convert raster data between different formats

### Virtual raster

- [`autoCreateWarpedVRT()`](https://usdaforestservice.github.io/gdalraster/reference/autoCreateWarpedVRT.md)
  : Create a virtual warped dataset automatically
- [`buildVRT()`](https://usdaforestservice.github.io/gdalraster/reference/buildVRT.md)
  : Build a GDAL virtual raster from a list of datasets
- [`rasterToVRT()`](https://usdaforestservice.github.io/gdalraster/reference/rasterToVRT.md)
  : Create a GDAL virtual raster derived from one source dataset

### Raster utilities

- [`calc()`](https://usdaforestservice.github.io/gdalraster/reference/calc.md)
  : Raster calculation
- [`combine()`](https://usdaforestservice.github.io/gdalraster/reference/combine.md)
  : Raster overlay for unique combinations
- [`dem_proc()`](https://usdaforestservice.github.io/gdalraster/reference/dem_proc.md)
  : GDAL DEM processing
- [`fillNodata()`](https://usdaforestservice.github.io/gdalraster/reference/fillNodata.md)
  : Fill selected pixels by interpolation from surrounding areas
- [`footprint()`](https://usdaforestservice.github.io/gdalraster/reference/footprint.md)
  : Compute footprint of a raster
- [`polygonize()`](https://usdaforestservice.github.io/gdalraster/reference/polygonize.md)
  : Create a polygon feature layer from raster data
- [`rasterize()`](https://usdaforestservice.github.io/gdalraster/reference/rasterize.md)
  : Burn vector geometries into a raster
- [`sieveFilter()`](https://usdaforestservice.github.io/gdalraster/reference/sieveFilter.md)
  : Remove small raster polygons
- [`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md)
  : Raster reprojection and mosaicing

### Raster display

- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md)
  : Display raster data

- [`read_ds()`](https://usdaforestservice.github.io/gdalraster/reference/read_ds.md)
  :

  Convenience wrapper for `GDALRaster$read()`

### Raster attribute tables

- [`buildRAT()`](https://usdaforestservice.github.io/gdalraster/reference/buildRAT.md)
  : Build a GDAL Raster Attribute Table with VALUE, COUNT
- [`createColorRamp()`](https://usdaforestservice.github.io/gdalraster/reference/createColorRamp.md)
  : Create a color ramp
- [`displayRAT()`](https://usdaforestservice.github.io/gdalraster/reference/displayRAT.md)
  : Display a GDAL Raster Attribute Table

### Raster data types

- [`dt_size()`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md)
  [`dt_is_complex()`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md)
  [`dt_is_integer()`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md)
  [`dt_is_floating()`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md)
  [`dt_is_signed()`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md)
  [`dt_union()`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md)
  [`dt_union_with_value()`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md)
  [`dt_find()`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md)
  [`dt_find_for_value()`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md)
  : Helper functions for GDAL raster data types

### Multidimensional raster

- [`mdim_as_classic()`](https://usdaforestservice.github.io/gdalraster/reference/mdim_as_classic.md)
  : Return a view of an MDArray as a "classic" GDALDataset (i.e., 2D)
- [`mdim_info()`](https://usdaforestservice.github.io/gdalraster/reference/mdim_info.md)
  : Report structure and content of a multidimensional dataset
- [`mdim_translate()`](https://usdaforestservice.github.io/gdalraster/reference/mdim_translate.md)
  : Convert multidimensional data between different formats, and subset

### Geotransform conversion

- [`apply_geotransform()`](https://usdaforestservice.github.io/gdalraster/reference/apply_geotransform.md)
  : Apply geotransform (raster column/row to geospatial x/y)
- [`get_pixel_line()`](https://usdaforestservice.github.io/gdalraster/reference/get_pixel_line.md)
  : Raster pixel/line from geospatial x,y coordinates
- [`inv_geotransform()`](https://usdaforestservice.github.io/gdalraster/reference/inv_geotransform.md)
  : Invert geotransform
- [`pixel_extract()`](https://usdaforestservice.github.io/gdalraster/reference/pixel_extract.md)
  : Extract pixel values at geospatial point locations

### Coordinate transformation

- [`inv_project()`](https://usdaforestservice.github.io/gdalraster/reference/inv_project.md)
  : Inverse project geospatial x/y coordinates to longitude/latitude
- [`transform_xy()`](https://usdaforestservice.github.io/gdalraster/reference/transform_xy.md)
  : Transform geospatial x/y coordinates
- [`transform_bounds()`](https://usdaforestservice.github.io/gdalraster/reference/transform_bounds.md)
  : Transform boundary

### Spatial reference systems

- [`epsg_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md)
  [`srs_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md)
  [`srs_to_projjson()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md)
  : Convert spatial reference definitions to OGC WKT or PROJJSON
- [`srs_get_name()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_find_epsg()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_is_geographic()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_is_derived_gcs()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_is_local()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_is_projected()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_is_compound()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_is_geocentric()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_is_vertical()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_is_dynamic()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_is_same()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_get_angular_units()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_get_linear_units()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_get_coord_epoch()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_get_utm_zone()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  [`srs_get_axis_mapping_strategy()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  : Obtain information about a spatial reference system

### Vector utilities

- [`ogrinfo()`](https://usdaforestservice.github.io/gdalraster/reference/ogrinfo.md)
  : Retrieve information about a vector data source
- [`ogr2ogr()`](https://usdaforestservice.github.io/gdalraster/reference/ogr2ogr.md)
  : Convert vector data between different formats
- [`ogr_proc()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_proc.md)
  : GDAL OGR facilities for vector geoprocessing
- [`ogr_reproject()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_reproject.md)
  : Reproject a vector layer

### Vector data management

- [`ogr_ds_exists()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_ds_format()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_ds_test_cap()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_ds_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_ds_layer_count()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_ds_layer_names()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_ds_field_domain_names()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_ds_add_field_domain()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_ds_delete_field_domain()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_layer_exists()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_layer_test_cap()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_layer_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_layer_field_names()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_layer_rename()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_layer_delete()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_field_index()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_field_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_geom_field_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_field_rename()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_field_set_domain_name()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_field_delete()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  [`ogr_execute_sql()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  : Utility functions for managing vector data sources

- [`ogr_def_layer()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)
  [`ogr_def_field()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)
  [`ogr_def_geom_field()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)
  [`ogr_def_field_domain()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)
  : OGR feature class definition for vector data

- [`plot(`*`<OGRFeature>`*`)`](https://usdaforestservice.github.io/gdalraster/reference/plot.OGRFeature.md)
  :

  Plot the geometry of an `OGRFeature` object

- [`plot(`*`<OGRFeatureSet>`*`)`](https://usdaforestservice.github.io/gdalraster/reference/plot.OGRFeatureSet.md)
  :

  Plot the geometry column of an `OGRFeatureSet`

- [`print(`*`<OGRFeature>`*`)`](https://usdaforestservice.github.io/gdalraster/reference/print.OGRFeature.md)
  :

  Print an `OGRFeature` object

- [`print(`*`<OGRFeatureSet>`*`)`](https://usdaforestservice.github.io/gdalraster/reference/print.OGRFeatureSet.md)
  :

  Print an `OGRFeatureSet`

### Geometry

- [`bbox_from_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_from_wkt.md)
  : Get the bounding box of a geometry specified in OGC WKT format
- [`bbox_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_to_wkt.md)
  : Convert a bounding box to POLYGON in OGC WKT format
- [`bbox_intersect()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_intersect.md)
  [`bbox_union()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_intersect.md)
  : Bounding box intersection / union
- [`bbox_transform()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_transform.md)
  : Transform a bounding box to a different projection
- [`g_create()`](https://usdaforestservice.github.io/gdalraster/reference/g_factory.md)
  [`g_add_geom()`](https://usdaforestservice.github.io/gdalraster/reference/g_factory.md)
  [`g_get_geom()`](https://usdaforestservice.github.io/gdalraster/reference/g_factory.md)
  : Create WKB/WKT geometries from vertices, and add/get sub-geometries
- [`g_wk2wk()`](https://usdaforestservice.github.io/gdalraster/reference/g_wk2wk.md)
  : Geometry WKB/WKT conversion
- [`g_is_empty()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  [`g_is_valid()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  [`g_is_3D()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  [`g_is_measured()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  [`g_is_ring()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  [`g_name()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  [`g_summary()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  [`g_geom_count()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  : Obtain information about WKB/WKT geometries
- [`g_make_valid()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md)
  [`g_normalize()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md)
  [`g_set_3D()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md)
  [`g_set_measured()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md)
  [`g_swap_xy()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md)
  : Geometry utility functions operating on WKB or WKT
- [`g_intersects()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md)
  [`g_disjoint()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md)
  [`g_touches()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md)
  [`g_contains()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md)
  [`g_within()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md)
  [`g_crosses()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md)
  [`g_overlaps()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md)
  [`g_equals()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md)
  : Geometry binary predicates operating on WKB or WKT
- [`g_intersection()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_op.md)
  [`g_union()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_op.md)
  [`g_difference()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_op.md)
  [`g_sym_difference()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_op.md)
  : Binary operations on WKB or WKT geometries
- [`g_buffer()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md)
  [`g_boundary()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md)
  [`g_convex_hull()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md)
  [`g_concave_hull()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md)
  [`g_delaunay_triangulation()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md)
  [`g_simplify()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md)
  [`g_unary_union()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md)
  : Unary operations on WKB or WKT geometries
- [`g_area()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
  [`g_centroid()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
  [`g_distance()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
  [`g_length()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
  [`g_geodesic_area()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
  [`g_geodesic_length()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
  : Compute measurements for WKB/WKT geometries
- [`g_coords()`](https://usdaforestservice.github.io/gdalraster/reference/g_coords.md)
  : Extract coordinate values from geometries
- [`g_envelope()`](https://usdaforestservice.github.io/gdalraster/reference/g_envelope.md)
  : Obtain the 2D or 3D bounding envelope for input geometries
- [`g_transform()`](https://usdaforestservice.github.io/gdalraster/reference/g_transform.md)
  : Apply a coordinate transformation to a WKB/WKT geometry
- [`geos_version()`](https://usdaforestservice.github.io/gdalraster/reference/geos_version.md)
  : Get GEOS version
- [`has_geos()`](https://usdaforestservice.github.io/gdalraster/reference/has_geos.md)
  : Is GEOS available?
- [`plot_geom()`](https://usdaforestservice.github.io/gdalraster/reference/plot_geom.md)
  : Plot WKT or WKB geometries

### General data management

- [`addFilesInZip()`](https://usdaforestservice.github.io/gdalraster/reference/addFilesInZip.md)
  : Create/append to a potentially Seek-Optimized ZIP file (SOZip)
- [`bandCopyWholeRaster()`](https://usdaforestservice.github.io/gdalraster/reference/bandCopyWholeRaster.md)
  : Copy a whole raster band efficiently
- [`copyDatasetFiles()`](https://usdaforestservice.github.io/gdalraster/reference/copyDatasetFiles.md)
  : Copy the files of a dataset
- [`deleteDataset()`](https://usdaforestservice.github.io/gdalraster/reference/deleteDataset.md)
  : Delete named dataset
- [`identifyDriver()`](https://usdaforestservice.github.io/gdalraster/reference/identifyDriver.md)
  : Identify the GDAL driver that can open a dataset
- [`inspectDataset()`](https://usdaforestservice.github.io/gdalraster/reference/inspectDataset.md)
  : Obtain information about a GDAL raster or vector dataset
- [`renameDataset()`](https://usdaforestservice.github.io/gdalraster/reference/renameDataset.md)
  : Rename a dataset

### Virtual file systems

- [`vsi_clear_path_options()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_clear_path_options.md)
  : Clear path specific configuration options
- [`vsi_copy_file()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_copy_file.md)
  : Copy a source file to a target filename
- [`vsi_curl_clear_cache()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_curl_clear_cache.md)
  : Clean cache associated with /vsicurl/ and related file systems
- [`vsi_get_actual_url()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_actual_url.md)
  : Returns the actual URL of a supplied VSI filename
- [`vsi_get_disk_free_space()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_disk_free_space.md)
  : Return free disk space available on the filesystem
- [`vsi_get_file_metadata()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_file_metadata.md)
  : Get metadata on files
- [`vsi_get_fs_options()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_fs_options.md)
  : Return the list of options associated with a virtual file system
  handler
- [`vsi_get_fs_prefixes()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_fs_prefixes.md)
  : Return the list of virtual file system handlers currently registered
- [`vsi_get_signed_url()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_signed_url.md)
  : Returns a signed URL for a supplied VSI filename
- [`vsi_is_local()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_is_local.md)
  : Returns if the file/filesystem is "local".
- [`vsi_mkdir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_mkdir.md)
  : Create a directory
- [`vsi_read_dir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_read_dir.md)
  : Read names in a directory
- [`vsi_rename()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_rename.md)
  : Rename a file
- [`vsi_rmdir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_rmdir.md)
  : Delete a directory
- [`vsi_set_path_option()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_set_path_option.md)
  : Set a path specific option for a given path prefix
- [`vsi_stat()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md)
  : Get filesystem object info
- [`vsi_supports_rnd_write()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_supports_rnd_write.md)
  : Return whether the filesystem supports random write
- [`vsi_supports_seq_write()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_supports_seq_write.md)
  : Return whether the filesystem supports sequential write
- [`vsi_sync()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_sync.md)
  : Synchronize a source file/directory with a target file/directory
- [`vsi_unlink()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_unlink.md)
  : Delete a file
- [`vsi_unlink_batch()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_unlink_batch.md)
  : Delete several files in a batch

### GDAL configuration

- [`gdal_version()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_version.md)
  [`gdal_version_num()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_version.md)
  : Get GDAL version
- [`gdal_compute_version()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_compute_version.md)
  : Compute a GDAL integer version number from major, minor, revision
- [`gdal_formats()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_formats.md)
  : Retrieve information on GDAL format drivers for raster and vector
- [`gdal_get_driver_md()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_get_driver_md.md)
  : Get metadata for a GDAL format driver
- [`get_cache_max()`](https://usdaforestservice.github.io/gdalraster/reference/get_cache_max.md)
  : Get the maximum memory size available for the GDAL block cache
- [`get_cache_used()`](https://usdaforestservice.github.io/gdalraster/reference/get_cache_used.md)
  : Get the size of memory in use by the GDAL block cache
- [`set_cache_max()`](https://usdaforestservice.github.io/gdalraster/reference/set_cache_max.md)
  : Set the maximum memory size for the GDAL block cache
- [`get_config_option()`](https://usdaforestservice.github.io/gdalraster/reference/get_config_option.md)
  : Get GDAL configuration option
- [`set_config_option()`](https://usdaforestservice.github.io/gdalraster/reference/set_config_option.md)
  : Set GDAL configuration option
- [`get_num_cpus()`](https://usdaforestservice.github.io/gdalraster/reference/get_num_cpus.md)
  : Get the number of processors detected by GDAL
- [`get_usable_physical_ram()`](https://usdaforestservice.github.io/gdalraster/reference/get_usable_physical_ram.md)
  : Get usable physical RAM reported by GDAL
- [`has_spatialite()`](https://usdaforestservice.github.io/gdalraster/reference/has_spatialite.md)
  : Is SpatiaLite available?
- [`http_enabled()`](https://usdaforestservice.github.io/gdalraster/reference/http_enabled.md)
  : Check if GDAL CPLHTTP services can be useful (libcurl)
- [`push_error_handler()`](https://usdaforestservice.github.io/gdalraster/reference/push_error_handler.md)
  : Push a new GDAL CPLError handler
- [`pop_error_handler()`](https://usdaforestservice.github.io/gdalraster/reference/pop_error_handler.md)
  : Pop error handler off stack
- [`dump_open_datasets()`](https://usdaforestservice.github.io/gdalraster/reference/dump_open_datasets.md)
  : Report open datasets

### PROJ configuration

- [`proj_networking()`](https://usdaforestservice.github.io/gdalraster/reference/proj_networking.md)
  : Check, enable or disable PROJ networking capabilities
- [`proj_search_paths()`](https://usdaforestservice.github.io/gdalraster/reference/proj_search_paths.md)
  : Get or set search path(s) for PROJ resource files
- [`proj_version()`](https://usdaforestservice.github.io/gdalraster/reference/proj_version.md)
  : Get PROJ version

## Constants

- [`DEFAULT_DEM_PROC`](https://usdaforestservice.github.io/gdalraster/reference/DEFAULT_DEM_PROC.md)
  : List of default DEM processing options
- [`DEFAULT_NODATA`](https://usdaforestservice.github.io/gdalraster/reference/DEFAULT_NODATA.md)
  : List of default nodata values by raster data type
- [`SEEK_SET`](https://usdaforestservice.github.io/gdalraster/reference/vsi_constants.md)
  [`SEEK_CUR`](https://usdaforestservice.github.io/gdalraster/reference/vsi_constants.md)
  [`SEEK_END`](https://usdaforestservice.github.io/gdalraster/reference/vsi_constants.md)
  : Constants for VSIFile\$seek()
