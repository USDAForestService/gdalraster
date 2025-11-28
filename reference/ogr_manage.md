# Utility functions for managing vector data sources

This set of functions can be used to create new vector datasets, test
existence of dataset/layer/field, test dataset and layer capabilities,
create new layers in an existing dataset, get the names of field domains
stored in a dataset, write field domains in a dataset, delete layers,
create new attribute and geometry fields on an existing layer, rename
and delete fields, and edit data with SQL statements.

## Usage

``` r
ogr_ds_exists(dsn, with_update = FALSE)

ogr_ds_format(dsn)

ogr_ds_test_cap(dsn, with_update = TRUE)

ogr_ds_create(
  format,
  dsn,
  layer = NULL,
  layer_defn = NULL,
  geom_type = NULL,
  srs = NULL,
  fld_name = NULL,
  fld_type = NULL,
  dsco = NULL,
  lco = NULL,
  overwrite = FALSE,
  return_obj = FALSE
)

ogr_ds_layer_count(dsn)

ogr_ds_layer_names(dsn)

ogr_ds_field_domain_names(dsn)

ogr_ds_add_field_domain(dsn, fld_dom_defn)

ogr_ds_delete_field_domain(dsn, domain_name)

ogr_layer_exists(dsn, layer)

ogr_layer_test_cap(dsn, layer = NULL, with_update = TRUE)

ogr_layer_create(
  dsn,
  layer,
  layer_defn = NULL,
  geom_type = NULL,
  srs = NULL,
  lco = NULL,
  return_obj = FALSE
)

ogr_layer_field_names(dsn, layer = NULL)

ogr_layer_rename(dsn, layer, new_name)

ogr_layer_delete(dsn, layer)

ogr_field_index(dsn, layer, fld_name)

ogr_field_create(
  dsn,
  layer,
  fld_name,
  fld_defn = NULL,
  fld_type = "OFTInteger",
  fld_subtype = "OFSTNone",
  fld_width = 0L,
  fld_precision = 0L,
  is_nullable = TRUE,
  is_unique = FALSE,
  default_value = "",
  domain_name = NULL
)

ogr_geom_field_create(
  dsn,
  layer,
  fld_name,
  geom_fld_defn = NULL,
  geom_type = NULL,
  srs = NULL,
  is_nullable = TRUE
)

ogr_field_rename(dsn, layer, fld_name, new_name)

ogr_field_set_domain_name(dsn, layer, fld_name, domain_name)

ogr_field_delete(dsn, layer, fld_name)

ogr_execute_sql(dsn, sql, spatial_filter = NULL, dialect = NULL)
```

## Arguments

- dsn:

  Character string. The vector data source name, e.g., a filename or
  database connection string.

- with_update:

  Logical value. `TRUE` to request update access when opening the
  dataset, or `FALSE` to open read-only.

- format:

  GDAL short name of the vector format as character string. Examples of
  some common output formats include: `"GPKG"`, `"FlatGeobuf"`,
  `"ESRI Shapefile"`, `"SQLite"`.

- layer:

  Character string for a layer name in a vector dataset. The `layer`
  argument may be given as empty string (`""`) in which case the first
  layer by index will be opened (except with `ogr_layer_delete()` and
  `ogr_layer_rename()` for which a layer name must be specified).

- layer_defn:

  A feature class definition for `layer` as a list of zero or more
  attribute field definitions, and at least one geometry field
  definition (see
  [ogr_define](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)).
  Each field definition is a list with named elements containing values
  for the field `type` element and other properties. If `layer_defn` is
  given, it will be used and any additional parameters passed that
  relate to the feature class definition will be ignored (i.e.,
  `geom_type` and `srs`, as well as `fld_name` and `fld_type` in
  `ogr_ds_create()`). The first geometry field definition in
  `layer_defn` defines the geometry type and spatial reference system
  for the layer (the geom field definition must contain `type`, and
  should also contain `srs` when creating a layer from a feature class
  definition).

- geom_type:

  Character string specifying a geometry type (see Details).

- srs:

  Character string containing a spatial reference system definition as
  OGC WKT or other well-known format (e.g., the input formats usable
  with
  [`srs_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md)).

- fld_name:

  Character string containing the name of an attribute field in `layer`.

- fld_type:

  Character string containing the name of a field data type (e.g.,
  `"OFTInteger"`, `"OFTInteger64"`, `"OFTReal"`, `"OFTString"`, ...).

- dsco:

  Optional character vector of format-specific creation options for
  `dsn` (`"NAME=VALUE"` pairs).

- lco:

  Optional character vector of format-specific creation options for
  `layer` (`"NAME=VALUE"` pairs).

- overwrite:

  Logical value. `TRUE` to overwrite `dsn` if it already exists when
  calling `ogr_ds_create()`. Default is `FALSE`.

- return_obj:

  Logical value. If `TRUE`, an object of class
  [`GDALVector`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.md)
  open on the newly created layer will be returned. Defaults to `FALSE`.
  Must be used with either the `layer` or `layer_defn` arguments.

- fld_dom_defn:

  A field domain definition, i.e., a list generated with
  [`ogr_def_field_domain()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md).

- domain_name:

  Character string specifying the name of the field domain.

- new_name:

  Character string containing a new name to assign.

- fld_defn:

  A field definition as list (see
  [`ogr_def_field()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)).
  Additional arguments in `ogr_field_create()` will be ignored if a
  `fld_defn` is given.

- fld_subtype:

  Character string containing the name of a field subtype. One of
  `"OFSTNone"` (the default), `"OFSTBoolean"`, `"OFSTInt16"`,
  `"OFSTFloat32"`, `"OFSTJSON"`, `"OFSTUUID"`.

- fld_width:

  Optional integer value specifying max number of characters.

- fld_precision:

  Optional integer value specifying number of digits after the decimal
  point.

- is_nullable:

  Optional NOT NULL field constraint (logical value). Defaults to
  `TRUE`.

- is_unique:

  Optional UNIQUE constraint on the field (logical value). Defaults to
  `FALSE`.

- default_value:

  Optional default value for the field as a character string.

- geom_fld_defn:

  A geometry field definition as list (see
  [`ogr_def_geom_field()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)).
  Additional arguments in `ogr_geom_field_create()` will be ignored if a
  `geom_fld_defn` is given.

- sql:

  Character string containing an SQL statement (see Note).

- spatial_filter:

  Either a numeric vector of length four containing a bounding box
  (xmin, ymin, xmax, ymax), or a character string containing a geometry
  as OGC WKT, representing a spatial filter.

- dialect:

  Character string specifying the SQL dialect to use. The OGR SQL engine
  (`"OGRSQL"`) will be used by default if a value is not given. The
  `"SQLite"` dialect can also be used (see Note).

## Details

These functions are complementary to
[`ogrinfo()`](https://usdaforestservice.github.io/gdalraster/reference/ogrinfo.md)
and
[`ogr2ogr()`](https://usdaforestservice.github.io/gdalraster/reference/ogr2ogr.md)
for vector data management. Bindings to OGR wrap portions of the GDAL
Vector API (ogr_core.h and ogr_api.h,
<https://gdal.org/en/stable/api/vector_c_api.html>).

`ogr_ds_exists()` tests whether a vector dataset can be opened from the
given data source name (DSN), potentially testing for update access.
Returns a logical value.

`ogr_ds_format()` returns a character string containing the short name
of the format driver for a given DSN, or `NULL` if the dataset cannot be
opened as a vector source.

`ogr_ds_test_cap()` tests the capabilities of a vector data source,
attempting to open it with update access by default. Returns a list of
capabilities with values `TRUE` or `FALSE`, or `NULL` is returned if
`dsn` cannot be opened with the requested access. Wrapper of
`GDALDatasetTestCapability()` in the GDAL API. The returned list
contains the following named elements:

- `CreateLayer`: `TRUE` if this dataset can create new layers

- `DeleteLayer`: `TRUE` if this dataset can delete existing layers

- `CreateGeomFieldAfterCreateLayer`: `TRUE` if the layers of this
  dataset supports geometry field creation just after layer creation

- `CurveGeometries`: `TRUE` if this dataset supports curve geometries

- `Transactions`: `TRUE` if this dataset supports (efficient)
  transactions

- `EmulatedTransactions`: `TRUE` if this dataset supports transactions
  through emulation

- `RandomLayerRead`: `TRUE` if this dataset has a dedicated
  `GetNextFeature()` implementation, potentially returning features from
  layers in a non-sequential way

- `RandomLayerWrite`: `TRUE` if this dataset supports calling
  `CreateFeature()` on layers in a non-sequential way

- `AddFieldDomain`: `TRUE` if this dataset supports adding field domains
  (GDAL \>= 3.3)

- `DeleteFieldDomain`: `TRUE` if this dataset supports deleting field
  domains (GDAL \>= 3.5) `UpdateFieldDomain`: `TRUE` if this dataset
  supports updating a an existing field domain by replacing its
  definition (GDAL \>= 3.5)

`ogr_ds_create()` creates a new vector dataset, optionally also creating
a layer, and optionally creating one or more fields on the layer. The
attribute fields and geometry field(s) to create can be specified as a
feature class definition (`layer_defn` as list, see
[ogr_define](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)),
or alternatively, by giving the `geom_type` and `srs`, optionally along
with one `fld_name` and `fld_type` to be created in the layer. By
default, returns logical `TRUE` indicating success (output written to
`dst_filename`), or an object of class
[`GDALVector`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.md)
for the output layer will be returned if `return_obj = TRUE`. An error
is raised if the operation fails.

`ogr_ds_layer_count()` returns the number of layers in a vector dataset.

`ogr_ds_layer_names()` returns a character vector of layer names in a
vector dataset, or `NULL` if no layers are found.

`ogr_ds_field_domain_names()` returns a character vector with the names
of all field domains stored in the dataset. Returns `NULL` and emits a
warning if an error occurs or if the format does not support reading
field domains. Returns a character vector of length 0 (`character(0)`)
if the format supports field domains but none are present in the
dataset. Requires GDAL \>= 3.5. See
[ogr_define](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)
for information on field domains.

`ogr_ds_add_field_domain()` adds a field domain to a vector dataset.
Only a few drivers will support this operation, and some of them might
only support it only for some types of field domains. GeoPackage and
OpenFileGDB drivers support this operation. A dataset having at least
some support should report the `AddFieldDomain` dataset capability (see
`ogr_ds_test_cap()`above). Returns a logical value, `TRUE` indicating
success. Requires GDAL \>= 3.3.

`ogr_ds_delete_field_domain()` deletes a field domain from a vector
dataset if supported (see `ogr_ds_test_cap()`above for the
`DeleteFieldDomain` dataset capability). Returns a logical value, `TRUE`
indicating success. Requires GDAL \>= 3.5.

`ogr_layer_exists()` tests whether a layer can be accessed by name in a
given vector dataset. Returns a logical value

`ogr_layer_test_cap()` tests whether a layer supports named
capabilities, attempting to open the dataset with update access by
default. Returns a list of capabilities with values `TRUE` or `FALSE`.
`NULL` is returned if `dsn` cannot be opened with the requested access,
or `layer` cannot be found. The returned list contains the following
named elements: `RandomRead`, `SequentialWrite`, `RandomWrite`,
`UpsertFeature`, `FastSpatialFilter`, `FastFeatureCount`,
`FastGetExtent`, `FastSetNextByIndex`, `CreateField`, `CreateGeomField`,
`DeleteField`, `ReorderFields`, `AlterFieldDefn`, `AlterGeomFieldDefn`,
`IgnoreFields`, `DeleteFeature`, `Rename`, `StringsAsUTF8`,
`CurveGeometries`. See the GDAL documentation for
[`OGR_L_TestCapability()`](https://gdal.org/en/stable/api/vector_c_api.html#_CPPv420OGR_L_TestCapability9OGRLayerHPKc).

`ogr_layer_create()` creates a new layer in an existing vector data
source, with a specified geometry type and spatial reference definition.
This function also accepts a feature class definition given as a list of
field names and their definitions (see
[ogr_define](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)).
(Note: use `ogr_ds_create()` to create single-layer formats such as
"ESRI Shapefile", "FlatGeobuf", "GeoJSON", etc.) By default, returns
logical `TRUE` indicating success, or an object of class
[`GDALVector`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.md)
will be returned if `return_obj = TRUE`. An error is raised if the
operation fails.

`ogr_layer_field_names()` returns a character vector of field names on a
layer, or `NULL` if no fields are found. The first layer by index is
opened if `NULL` is given for the `layer` argument.

`ogr_layer_rename()` renames a layer in a vector dataset. This operation
is implemented only by layers that expose the `Rename` capability (see
`ogr_layer_test_cap()` above). This operation will fail if a layer with
the new name already exists. Returns a logical value, `TRUE` indicating
success. Requires GDAL \>= 3.5.

`ogr_layer_delete()` deletes an existing layer in a vector dataset.
Returns a logical value, `TRUE` indicating success.

`ogr_field_index()` tests for existence of an attribute field by name.
Returns the field index on the layer (0-based), or `-1` if the field
does not exist.

`ogr_field_create()` creates a new attribute field of specified data
type in a given DSN/layer. Several optional field properties can be
specified in addition to the type. Returns a logical value, `TRUE`
indicating success.

`ogr_geom_field_create()` creates a new geometry field of specified type
in a given DSN/layer. Returns a logical value, `TRUE` indicating
success.

`ogr_field_rename()` renames an existing field on a vector layer. Not
all format drivers support this function. Some drivers may only support
renaming a field while there are still no features in the layer.
`AlterFieldDefn` is the relevant layer capability to check. Returns a
logical value, `TRUE` indicating success.

`ogr_field_set_domain_name()` sets the field domain name for an existing
attribute field on a vector layer. `AlterFieldDefn` layer capability is
required. Returns a logical value, `TRUE` indicating success. Requires
GDAL \>= 3.3.

`ogr_field_delete()` deletes an existing field on a vector layer. Not
all format drivers support this function. Some drivers may only support
deleting a field while there are still no features in the layer. Returns
a logical value, `TRUE` indicating success.

`ogr_execute_sql()` executes an SQL statement against the data store.
This function can be used to modify the schema or edit data using SQL
(e.g., `ALTER TABLE`, `DROP TABLE`, `CREATE INDEX`, `DROP INDEX`,
`INSERT`, `UPDATE`, `DELETE`), or to execute a query (i.e., `SELECT`).
Returns `NULL` (invisibly) for statements that are in error, or that
have no results set, or an object of class `GDALVector` representing a
results set from the query. Wrapper of `GDALDatasetExecuteSQL()` in the
GDAL API.

## Note

The OGR SQL document linked under **See Also** contains information on
the SQL dialect supported internally by GDAL/OGR. Some format drivers
(e.g., PostGIS) pass the SQL directly through to the underlying RDBMS
(unless `OGRSQL` is explicitly passed as the dialect). The SQLite
dialect can also be requested with the `SQLite` string passed as the
`dialect` argument of `ogr_execute_sql()`. This assumes that GDAL/OGR is
built with support for SQLite, and preferably also with Spatialite
support to benefit from spatial functions. The GDAL document for SQLite
dialect has detailed information.

Other SQL dialects may also be present for some vector formats. For
example, the `"INDIRECT_SQLITE"` dialect might potentially be used with
GeoPackage format
(<https://gdal.org/en/stable/drivers/vector/gpkg.html#sql>).

The function
[`ogrinfo()`](https://usdaforestservice.github.io/gdalraster/reference/ogrinfo.md)
can also be used to edit data with SQL statements (GDAL \>= 3.7).

The name of the geometry column of a layer is empty (`""`) with some
formats such as ESRI Shapefile and FlatGeobuf. Implications for SQL may
depend on the dialect used. See the GDAL documentation for the "OGR SQL"
and "SQLite" dialects for details.

## See also

OGR SQL dialect and SQLite SQL dialect:  
<https://gdal.org/en/stable/user/ogr_sql_sqlite_dialect.html>

## Examples

``` r
## Create GeoPackage and manage schema
dsn <- file.path(tempdir(), "test1.gpkg")
ogr_ds_create("GPKG", dsn)
#> [1] TRUE
ogr_ds_exists(dsn, with_update = TRUE)
#> [1] TRUE
# dataset capabilities
ogr_ds_test_cap(dsn)
#> $CreateLayer
#> [1] TRUE
#> 
#> $DeleteLayer
#> [1] TRUE
#> 
#> $CreateGeomFieldAfterCreateLayer
#> [1] FALSE
#> 
#> $CurveGeometries
#> [1] TRUE
#> 
#> $Transactions
#> [1] TRUE
#> 
#> $EmulatedTransactions
#> [1] FALSE
#> 
#> $RandomLayerRead
#> [1] FALSE
#> 
#> $RandomLayerWrite
#> [1] TRUE
#> 
#> $AddFieldDomain
#> [1] TRUE
#> 
#> $DeleteFieldDomain
#> [1] FALSE
#> 
#> $UpdateFieldDomain
#> [1] FALSE
#> 

ogr_layer_create(dsn, layer = "layer1", geom_type = "Polygon",
                 srs = "EPSG:5070")
#> [1] TRUE

ogr_field_create(dsn, layer = "layer1",
                 fld_name = "field1",
                 fld_type = "OFTInteger64",
                 is_nullable = FALSE)
#> [1] TRUE
ogr_field_create(dsn, layer = "layer1",
                 fld_name = "field2",
                 fld_type = "OFTString")
#> [1] TRUE

ogr_ds_layer_count(dsn)
#> [1] 1
ogr_ds_layer_names(dsn)
#> [1] "layer1"
ogr_layer_field_names(dsn, layer = "layer1")
#> [1] "field1" "field2" "geom"  

# delete a field
if (ogr_layer_test_cap(dsn, "layer1")$DeleteField) {
  ogr_field_delete(dsn, layer = "layer1", fld_name = "field2")
}
#> [1] TRUE

ogr_layer_field_names(dsn, "layer1")
#> [1] "field1" "geom"  

# define a feature class and create layer
defn <- ogr_def_layer("Point", srs = epsg_to_wkt(4326))
# add the attribute fields
defn$id_field <- ogr_def_field(fld_type = "OFTInteger64",
                               is_nullable = FALSE,
                               is_unique = TRUE)
defn$str_field <- ogr_def_field(fld_type = "OFTString",
                                fld_width = 25,
                                is_nullable = FALSE,
                                default_value = "'a default string'")
defn$numeric_field <- ogr_def_field(fld_type = "OFTReal",
                                    default_value = "0.0")

ogr_layer_create(dsn, layer = "layer2", layer_defn = defn)
#> [1] TRUE
ogr_ds_layer_names(dsn)
#> [1] "layer1" "layer2"
ogr_layer_field_names(dsn, layer = "layer2")
#> [1] "id_field"      "str_field"     "numeric_field" "geom"         

# add a field using SQL instead
ogr_execute_sql(dsn, sql = "ALTER TABLE layer2 ADD field4 float")
#> info: open dataset successful on DSN:
#>   '/tmp/RtmpqKhUt9/test1.gpkg'

# rename a field
if (ogr_layer_test_cap(dsn, "layer1")$AlterFieldDefn) {
  ogr_field_rename(dsn, layer = "layer2",
                   fld_name = "field4",
                   new_name = "renamed_field")
}
#> [1] TRUE
ogr_layer_field_names(dsn, layer = "layer2")
#> [1] "id_field"      "str_field"     "numeric_field" "renamed_field"
#> [5] "geom"         

# GDAL >= 3.7
if (gdal_version_num() >= gdal_compute_version(3, 7, 0))
  ogrinfo(dsn, "layer2")
#> INFO: Open of `/tmp/RtmpqKhUt9/test1.gpkg'
#>       using driver `GPKG' successful.
#> 
#> Layer name: layer2
#> Geometry: Point
#> Feature Count: 0
#> Layer SRS WKT:
#> GEOGCRS["WGS 84",
#>     DATUM["World Geodetic System 1984",
#>         ELLIPSOID["WGS 84",6378137,298.257223563,
#>             LENGTHUNIT["metre",1]]],
#>     PRIMEM["Greenwich",0,
#>         ANGLEUNIT["degree",0.0174532925199433]],
#>     CS[ellipsoidal,2],
#>         AXIS["geodetic latitude (Lat)",north,
#>             ORDER[1],
#>             ANGLEUNIT["degree",0.0174532925199433]],
#>         AXIS["geodetic longitude (Lon)",east,
#>             ORDER[2],
#>             ANGLEUNIT["degree",0.0174532925199433]],
#>     USAGE[
#>         SCOPE["Horizontal component of 3D system."],
#>         AREA["World."],
#>         BBOX[-90,-180,90,180]],
#>     ID["EPSG",4326]]
#> Data axis to CRS axis mapping: 2,1
#> FID Column = fid
#> Geometry Column = geom
#> id_field: Integer64 (0.0) UNIQUE NOT NULL
#> str_field: String (25.0) NOT NULL DEFAULT 'a default string'
#> numeric_field: Real (0.0) DEFAULT 0.0
#> renamed_field: Real(Float32) (0.0)

## Edit data using SQL
src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
perims_shp <- file.path(tempdir(), "mtbs_perims.shp")
ogr2ogr(src_dsn = src, dst_dsn = perims_shp, src_layers = "mtbs_perims")
#> GDAL WARNING 6: Normalized/laundered field name: 'burn_bnd_ac' to 'burn_bnd_a'
#> GDAL WARNING 6: Normalized/laundered field name: 'burn_bnd_lat' to 'burn_bnd_l'
#> GDAL WARNING 6: Normalized/laundered field name: 'burn_bnd_lon' to 'burn_bnd_1'
ogr_ds_format(dsn = perims_shp)
#> [1] "ESRI Shapefile"
ogr_ds_layer_names(dsn = perims_shp)
#> [1] "mtbs_perims"
ogr_layer_field_names(dsn = perims_shp, layer = "mtbs_perims")
#>  [1] "event_id"   "incid_name" "incid_type" "map_id"     "burn_bnd_a"
#>  [6] "burn_bnd_l" "burn_bnd_1" "ig_date"    "ig_year"    ""          

alt_tbl <- "ALTER TABLE mtbs_perims ADD burn_bnd_ha float"
ogr_execute_sql(dsn = perims_shp, sql = alt_tbl)
#> info: open dataset successful on DSN:
#>   '/tmp/RtmpqKhUt9/mtbs_perims.shp'
#> GDAL WARNING 6: Normalized/laundered field name: 'burn_bnd_ha' to 'burn_bnd_h'

upd <- "UPDATE mtbs_perims SET burn_bnd_ha = (burn_bnd_ac / 2.471)"
ogr_execute_sql(dsn = perims_shp, sql = upd, dialect = "SQLite")
#> info: open dataset successful on DSN:
#>   '/tmp/RtmpqKhUt9/mtbs_perims.shp'
#> GDAL FAILURE 1: In ExecuteSQL(): sqlite3_prepare_v2(UPDATE mtbs_perims SET burn_bnd_ha = (burn_bnd_ac / 2.471)):
#>   no such column: burn_bnd_ac
ogr_layer_field_names(dsn = perims_shp, layer = "mtbs_perims")
#>  [1] "event_id"   "incid_name" "incid_type" "map_id"     "burn_bnd_a"
#>  [6] "burn_bnd_l" "burn_bnd_1" "ig_date"    "ig_year"    "burn_bnd_h"
#> [11] ""          

# if GDAL >= 3.7:
#   ogrinfo(dsn = perims_shp, layer = "mtbs_perims")
# or, for output incl. the feature data (omit the default "-so" arg):
#   ogrinfo(dsn = perims_shp, layer = "mtbs_perims", cl_arg = "-nomd")
```
