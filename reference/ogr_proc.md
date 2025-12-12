# GDAL OGR facilities for vector geoprocessing

`ogr_proc()` performs GIS overlay operations on vector layers
(<https://en.wikipedia.org/wiki/Vector_overlay>). It provides an
interface to the GDAL API methods for these operations
(`OGRLayer::Intersection()`, `OGRLayer::Union()`, etc). Inputs are given
as objects of class
[`GDALVector`](https://firelab.github.io/gdalraster/reference/GDALVector-class.md),
which may have spatial and/or attribute filters applied. The output
layer will be created if it does not exist, but output can also be
appended to an existing layer, or written to an existing empty layer
that has a custom schema defined. `ogr_proc()` is basically a port of
the
[`ogr_layer_algebra`](https://gdal.org/en/stable/programs/ogr_layer_algebra.html#ogr-layer-algebra)
utility in the GDAL Python bindings.

## Usage

``` r
ogr_proc(
  mode,
  input_lyr,
  method_lyr,
  out_dsn,
  out_lyr_name = NULL,
  out_geom_type = NULL,
  out_fmt = NULL,
  dsco = NULL,
  lco = NULL,
  mode_opt = NULL,
  overwrite = FALSE,
  quiet = FALSE,
  return_obj = TRUE
)
```

## Arguments

- mode:

  Character string specifying the operation to perform. One of
  `Intersection`, `Union`, `SymDifference`, `Identity`, `Update`, `Clip`
  or `Erase` (see Details).

- input_lyr:

  An object of class
  [`GDALVector`](https://firelab.github.io/gdalraster/reference/GDALVector-class.md)
  to use as the input layer. For overlay operations, this is the first
  layer in the operation.

- method_lyr:

  An object of class
  [`GDALVector`](https://firelab.github.io/gdalraster/reference/GDALVector-class.md)
  to use as the method layer. This is the conditional layer supplied to
  an operation (e.g., Clip, Erase, Update), or the second layer in
  overlay operations (e.g., Union, Intersection, SymDifference).

- out_dsn:

  The destination vector filename or database connection string to which
  the output layer will be written.

- out_lyr_name:

  Layer name where the output vector will be written. May be `NULL`
  (e.g., shapefile), but typically must be specified.

- out_geom_type:

  Character string specifying the geometry type of the output layer. One
  of NONE, GEOMETRY, POINT, LINESTRING, POLYGON, GEOMETRYCOLLECTION,
  MULTIPOINT, MULTIPOLYGON, GEOMETRY25D, POINT25D, LINESTRING25D,
  POLYGON25D, GEOMETRYCOLLECTION25D, MULTIPOINT25D, MULTIPOLYGON25D.
  Defaults to UNKNOWN if not specified.

- out_fmt:

  GDAL short name of the output vector format. If unspecified, the
  function will attempt to guess the format from the value of `out_dsn`.

- dsco:

  Optional character vector of format-specific creation options for
  `out_dsn` (`"NAME=VALUE"` pairs).

- lco:

  Optional character vector of format-specific creation options for
  `out_layer` (`"NAME=VALUE"` pairs).

- mode_opt:

  Optional character vector of `"NAME=VALUE"` pairs that specify
  processing options. Available options depend on the value of `mode`
  (see Details).

- overwrite:

  Logical value. `TRUE` to overwrite the output layer if it already
  exists. Defaults to `FALSE`.

- quiet:

  Logical value. If `TRUE`, a progress bar will not be displayed.
  Defaults to `FALSE`.

- return_obj:

  Logical value. If `TRUE` (the default), an object of class
  [`GDALVector`](https://firelab.github.io/gdalraster/reference/GDALVector-class.md)
  opened on the output layer will be returned, otherwise the function
  returns a logical value.

## Value

Upon successful completion, an object of class
[`GDALVector`](https://firelab.github.io/gdalraster/reference/GDALVector-class.md)
is returned by default (`return_obj = TRUE`), or logical `TRUE` is
returned if `return_obj = FALSE`. Logical `FALSE` is returned if an
error occurs during processing.

## Details

Seven processing modes are available:

- `Intersection`: The output layer contains features whose geometries
  represent areas that are common between features in the input layer
  and in the method layer. The features in the output layer have
  attributes from both input and method layers.

- `Union`: The output layer contains features whose geometries represent
  areas that are either in the input layer, in the method layer, or in
  both. The features in the output layer have attributes from both input
  and method layers. For features which represent areas that are only in
  the input layer or only in the method layer the respective attributes
  have undefined values.

- `SymDifference`: The output layer contains features whose geometries
  represent areas that are in either in the input layer or in the method
  layer but not in both. The features in the output layer have
  attributes from both input and method layers. For features which
  represent areas that are only in the input or only in the method layer
  the respective attributes have undefined values.

- `Identity`: Identifies the features of the input layer with the ones
  from the method layer. The output layer contains features whose
  geometries represent areas that are in the input layer. The features
  in the output layer have attributes from both the input and method
  layers.

- `Update`: The update method creates a layer which adds features into
  the input layer from the method layer, possibly cutting features in
  the input layer. The features in the output layer have areas of the
  features of the method layer or those areas of the features of the
  input layer that are not covered by the method layer. The features of
  the output layer get their attributes from the input layer.

- `Clip`: The clip method creates a layer which has features from the
  input layer clipped to the areas of the features in the method layer.
  By default the output layer has attributes of the input layer.

- `Erase`: The erase method creates a layer which has features from the
  input layer whose areas are erased by the features in the method
  layer. By default, the output layer has attributes of the input layer.

By default, `ogr_proc()` will create the output layer with an empty
schema. It will be initialized by GDAL to contain all fields in the
input layer, or depending on the operation, all fields in both the input
and method layers. In the latter case, the prefixes `"input_"` and
`"method_"` will be added to the output field names by default. The
default prefixes can be overridden in the `mode_opt` argument as
described below.

Alternatively, the functions in the
[`ogr_manage`](https://firelab.github.io/gdalraster/reference/ogr_manage.md)
interface could be used to create an empty layer with user-defined
schema (e.g.,
[`ogr_ds_create()`](https://firelab.github.io/gdalraster/reference/ogr_manage.md),
[`ogr_layer_create()`](https://firelab.github.io/gdalraster/reference/ogr_manage.md)
and
[`ogr_field_create()`](https://firelab.github.io/gdalraster/reference/ogr_manage.md)).
If the schema of the output layer is set by the user and contains fields
that have the same name as a field in both the input and method layers,
then the attribute for an output feature will get the value from the
feature of the method layer.

Options that affect processing can be set as NAME=VALUE pairs passed in
the `mode_opt` argument. Some options are specific to certain processing
modes as noted below:

- SKIP_FAILURES=YES/NO. Set it to YES to go on, even when a feature
  could not be inserted or a GEOS call failed.

- PROMOTE_TO_MULTI=YES/NO. Set to YES to convert Polygons into
  MultiPolygons, LineStrings to MultiLineStrings or Points to
  MultiPoints (only since GDAL 3.9.2 for the latter).

- INPUT_PREFIX=string. Set a prefix for the field names that will be
  created from the fields of the input layer.

- METHOD_PREFIX=string. Set a prefix for the field names that will be
  created from the fields of the method layer.

- USE_PREPARED_GEOMETRIES=YES/NO. Set to NO to not use prepared
  geometries to pretest intersection of features of method layer with
  features of input layer. Applies to `Intersection`, `Union`,
  `Identity`.

- PRETEST_CONTAINMENT=YES/NO. Set to YES to pretest the containment of
  features of method layer within the features of input layer. This will
  speed up the operation significantly in some cases. Requires that the
  prepared geometries are in effect. Applies to `Intersection`.

- KEEP_LOWER_DIMENSION_GEOMETRIES=YES/NO. Set to NO to skip result
  features with lower dimension geometry that would otherwise be added
  to the output layer. The default is YES, to add features with lower
  dimension geometry, but only if the result output has an UNKNOWN
  geometry type. Applies to `Intersection`, `Union`, `Identity`.

The input and method layers should have the same spatial reference
system. No on-the-fly reprojection is done. When an output layer is
created it will have the SRS of `input_lyr`.

## Note

The first geometry field on a layer is always used.

For best performance use the minimum amount of features in the method
layer and copy into a memory layer.

## See also

[`GDALVector-class`](https://firelab.github.io/gdalraster/reference/GDALVector-class.md),
[`ogr_define`](https://firelab.github.io/gdalraster/reference/ogr_define.md),
[`ogr_manage`](https://firelab.github.io/gdalraster/reference/ogr_manage.md)

Vector overlay operators:  
<https://en.wikipedia.org/wiki/Vector_overlay>

## Examples

``` r
# MTBS fires in Yellowstone National Park 1984-2022
dsn <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")

# layer filtered to fires after 1988
lyr1 <- new(GDALVector, dsn, "mtbs_perims")
lyr1$setAttributeFilter("ig_year > 1988")
lyr1$getFeatureCount()
#> [1] 46

# second layer for the 1988 North Fork fire perimeter
sql <- "SELECT incid_name, ig_year, geom FROM mtbs_perims
        WHERE incid_name = 'NORTH FORK'"
lyr2 <- new(GDALVector, dsn, sql)
lyr2$getFeatureCount()
#> [1] 1

# intersect to obtain areas in the North Fork perimeter that have re-burned
tmp_dsn <- tempfile(fileext = ".gpkg")
opt <- c("INPUT_PREFIX=layer1_",
         "METHOD_PREFIX=layer2_",
         "PROMOTE_TO_MULTI=YES")

lyr_out <- ogr_proc(mode = "Intersection",
                    input_lyr = lyr1,
                    method_lyr = lyr2,
                    out_dsn = tmp_dsn,
                    out_lyr_name = "north_fork_reburned",
                    out_geom_type = "MULTIPOLYGON",
                    mode_opt = opt)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.

# the output layer has attributes of both the input and method layers
(d <- lyr_out$fetch(-1))
#> OGR feature set
#>   FID       layer1_event_id layer1_incid_name layer1_incid_type layer1_map_id
#> 1   1 WY4484611038620100914          ANTELOPE          Wildfire      10013735
#> 2   2 WY4466711063920120810            CYGNET          Wildfire          1961
#> 3   3 WY4474311097820160809             MAPLE          Wildfire      10005020
#> 4   4 WY4492611093820160805              FAWN          Wildfire      10005117
#> 5   5 WY4457911058620160826           CENTRAL          Wildfire      10014143
#>   layer1_burn_bnd_ac layer1_burn_bnd_lat layer1_burn_bnd_lon layer1_ig_date
#> 1               4888              44.839            -110.368     2010-09-14
#> 2               3188              44.682            -110.622     2012-08-10
#> 3             103193              44.731            -110.982     2016-08-09
#> 4               3161              44.936            -110.913     2016-08-05
#> 5               2340              44.595            -110.574     2016-08-26
#>   layer1_ig_year layer2_incid_name layer2_ig_year
#> 1           2010        NORTH FORK           1988
#> 2           2012        NORTH FORK           1988
#> 3           2016        NORTH FORK           1988
#> 4           2016        NORTH FORK           1988
#> 5           2016        NORTH FORK           1988
#>                                    geom
#> 1 WKB MULTIPOLYGON: raw 01 06 00 00 ...
#> 2 WKB MULTIPOLYGON: raw 01 06 00 00 ...
#> 3 WKB MULTIPOLYGON: raw 01 06 00 00 ...
#> 4 WKB MULTIPOLYGON: raw 01 06 00 00 ...
#> 5 WKB MULTIPOLYGON: raw 01 06 00 00 ...

# clean up
lyr1$close()
lyr2$close()
lyr_out$close()
```
