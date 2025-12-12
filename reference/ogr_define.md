# OGR feature class definition for vector data

This topic contains documentation and helper functions for defining an
OGR feature class. A named list containing zero or more attribute field
definitions, along with one or more geometry field definitions, comprise
an OGR feature class definition (a.k.a. layer definition).
`ogr_def_layer()` initializes such a list with the geometry type and
(optionally) a spatial reference system. Attribute fields may then be
added to the layer definition. `ogr_def_field()` creates an attribute
field definition, a list containing the field's data type and
potentially other optional field properties. `ogr_def_geom_field()`
similarly creates a geometry field definition. This might be used with
certain vector formats that support multiple geometry columns (e.g.,
PostGIS). `ogr_def_field_domain()` creates a field domain definition. A
field domain is a set of constraints that apply to one or several
fields. This is a concept found, e.g., in ESRI File Geodatabase and in
GeoPackage via the Schema extension (see
<https://github.com/OSGeo/gdal/pull/3638>). GDAL \>= 3.3 supports
reading and writing field domains with certain drivers (e.g., GPKG and
OpenFileGDB).

## Usage

``` r
ogr_def_layer(geom_type, geom_fld_name = "geom", srs = NULL)

ogr_def_field(
  fld_type,
  fld_subtype = NULL,
  fld_width = NULL,
  fld_precision = NULL,
  is_nullable = NULL,
  is_unique = NULL,
  default_value = NULL,
  domain_name = NULL
)

ogr_def_geom_field(geom_type, srs = NULL, is_nullable = NULL)

ogr_def_field_domain(
  domain_type,
  domain_name,
  description = NULL,
  fld_type = NULL,
  fld_subtype = "OFSTNone",
  split_policy = "DEFAULT_VALUE",
  merge_policy = "DEFAULT_VALUE",
  coded_values = NULL,
  range_min = NULL,
  min_is_inclusive = TRUE,
  range_max = NULL,
  max_is_inclusive = TRUE,
  glob = ""
)
```

## Arguments

- geom_type:

  Character string specifying a geometry type (see Details).

- geom_fld_name:

  Character string specifying a geometry field name Defaults to
  `"geom"`.

- srs:

  Character string containing a spatial reference system definition as
  OGC WKT or other well-known format (e.g., the input formats usable
  with
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md)).

- fld_type:

  Character string containing the name of a field data type (e.g.,
  `"OFTInteger"`, `"OFTInteger64"`, `"OFTReal"`, `"OFTString"`, ...).

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

  Optional default value for the field given as a character string.

- domain_name:

  Character string specifying the name of the field domain. Optional for
  `ogr_def_field()`, required for `ogr_def_field_domain()`.

- domain_type:

  Character string specifying a field domain type (see Details). Must be
  one of `"Coded"`, `"Range"`, `"RangeDateTime"`, `"GLOB"`
  (case-insensitive).

- description:

  Optional character string giving a description of the field domain.

- split_policy:

  Character string specifying the split policy of the field domain. One
  of `"DEFAULT_VALUE"`, `"DUPLICATE"`, `"GEOMETRY_RATIO"` (supported by
  ESRI File Geodatabase format via OpenFileGDB driver).

- merge_policy:

  Character string specifying the merge policy of the field domain. One
  of `"DEFAULT_VALUE"`, `"SUM"`, `"GEOMETRY_WEIGHTED"` (supported by
  ESRI File Geodatabase format via OpenFileGDB driver).

- coded_values:

  Either a vector of the allowed codes, or character vector of
  `"CODE=VALUE"` pairs (the expanded "value" associated with a code is
  optional), or a two-column data frame with (codes, values). If data
  frame, the second column of values must be character type (i.e., the
  descriptive text for each code). This argument is required if
  `domain_type = "Coded"`. Each code should appear only once, but it is
  the responsibility of the user to check it.

- range_min:

  Minimum value in a Range or RangeDateTime field domain (can be NULL).
  The data type must be consistent with with the field type given in the
  `fld_type` argument.

- min_is_inclusive:

  = Logical value, whether the minimum value is included in the range.
  Defaults to `TRUE`. Required argument if `domain_type` is `"Range"` or
  `"RangeDateTime"`.

- range_max:

  Maximum value in a Range or RangeDateTime field domain (can be NULL).
  The data type must be consistent with with the field type given in the
  `fld_type` argument.

- max_is_inclusive:

  = Logical value, whether the maximum value is included in the range.
  Defaults to `TRUE`. Required argument if `domain_type` is `"Range"` or
  `"RangeDateTime"`.

- glob:

  Character string containing the GLOB expression. Required if
  `domain_type` is `"GLOB"`.

## Details

All features in an OGR Layer share a common schema (feature class),
modeled in GDAL by its `OGRFeatureDefn` class. A feature class
definition includes the set of attribute fields and their data types and
the geometry field(s). In R, a feature class definition is represented
as a named list, with names being the attribute/geometry field names,
and each list element holding an attribute or geometry field definition.

The definition for an attribute field is a named list with elements:

    $type       : OGR Field Type ("OFTReal", "OFTString" etc.)
    $subtype    : optional ("OFSTBoolean", ...)
    $width      : optional max number of characters
    $precision  : optional number of digits after the decimal point
    $is_nullable: optional NOT NULL constraint (logical value)
    $is_unique  : optional UNIQUE constraint (logical value)
    $default    : optional default value as character string
    $domain     : optional field domain name
    $is_geom    : FALSE (the default) for attribute fields

An OGR field type is specified as a character string with possible
values: `OFTInteger`, `OFTIntegerList`, `OFTReal`, `OFTRealList`,
`OFTString`, `OFTStringList`, `OFTBinary`, `OFTDate`, `OFTTime`,
`OFTDateTime`, `OFTInteger64`, `OFTInteger64List`.

An optional field subtype is specified as a character string with
possible values: `OFSTNone`, `OFSTBoolean`, `OFSTInt16`, `OFSTFloat32`,
`OFSTJSON`, `OFSTUUID`.

By default, fields are nullable and have no unique constraint. Not-null
and unique constraints are not supported by all format drivers.

A default field value is taken into account by format drivers (generally
those with a SQL interface) that support it at field creation time. If
given in the field definition, `default` must be a character string. The
accepted values are `"NULL"`, a numeric value (e.g., `"0"`), a literal
value enclosed between single quote characters (e.g.,
`"'a default value'"`, with any inner single quote characters escaped by
repetition of the single quote character), `"CURRENT_TIMESTAMP"`,
`"CURRENT_TIME"`, `"CURRENT_DATE"` or a driver-specific expression (that
might be ignored by other drivers). For a datetime literal value, format
should be `"'YYYY/MM/DD HH:MM:SS[.sss]'"` (considered as UTC time).

The definition for a geometry field is a named list with elements:

    $type       : geom type ("Point", "Polygon", etc.)
    $srs        : optional spatial reference as WKT string
    $is_nullable: optional NOT NULL constraint (logical value)
    $is_geom    : TRUE (required) for geometry fields

Typically, there is one geometry field on a layer, but some formats
support more than one geometry column per table (e.g., "PostgreSQL /
PostGIS" and "SQLite / Spatialite RDBMS").

Geometry types are specified as a character string containing OGC WKT.
Common types include: `Point`, `LineString`, `Polygon`, `MultiPoint`,
`MultiLineString`, `MultiPolygon`. See the GDAL documentation for a list
of all supported geometry types:  
<https://gdal.org/en/stable/api/vector_c_api.html#_CPPv418OGRwkbGeometryType>

Format drivers may or may not support not-null constraints on attribute
and geometry fields. If they support creating fields with not-null
constraints, this is generally before creating any features to the
layer. In some cases, a not-null constraint may be available as a layer
creation option. For example, GeoPackage format has a layer creation
option `GEOMETRY_NULLABLE=[YES/NO]`.

The definition for a field domain is a named list with elements:

    $type             : domain type ("Coded", "Range", "RangeDateTime", "GLOB")
    $domain_name      : name of the field domain (character string)
    $description      : optional domain description (character string)
    $field_type       : OGR Field Type (see attribute field definitions above)
    $field_subtype    : optional OGR Field Subtype ("OFSTBoolean", ...)
    $split_policy     : split policy of the field domain (see below)
    $merge_policy     : merge policy of the field domain (see below)
    $coded_values     : vector of allowed codes, or data frame of (codes, values)
    $min_value        : minimum value (data type compatible with $field_type)
    $min_is_inclusive : whether the minimum value is included in the range
    $max_value        : maximum value (data type compatible with $field_type)
    $max_is_inclusive : whether the maximum value is included in the range
    $glob             : GLOB expression (character string)

A field domain can be one of three types:

- `Coded`: an enumerated list of allowed codes with their descriptive
  values

- `Range`: a range constraint (min, max)

- `GLOB`: a GLOB expression (matching expression like `"*[a-z][0-1]?"`)

`type` can also be specified as `"RangeDateTime"`, a range constraint
for `OFTDateTime` fields with `min_value` and `max_value` given as
`"POSIXct"` DateTimes.

Split and merge policies are supported by ESRI File Geodatabase format
via OpenFileGDB driver (or FileGDB driver dependent on FileGDB API
library). When a feature is split in two, `split_policy` defines how the
value of attributes following the domain are computed. Possible values
are `"DEFAULT_VALUE"` (default value), `"DUPLICATE"` (duplicate), and
`"GEOMETRY_RATIO"` (new values are computed by the ratio of their
area/length compared to the area/length of the original feature). When a
feature is built by merging two features, `merge_policy` defines how the
value of attributes following the domain are computed. Possible values
are `"DEFAULT_VALUE"` (default value), `"SUM"` (sum), and
`"GEOMETRY_WEIGHTED"` (new values are computed as the weighted average
of the source values).

## Note

The feature id (FID) is a special property of a feature and not treated
as an attribute of the feature. Additional information is given in the
GDAL documentation for the [OGR
SQL](https://gdal.org/en/stable/user/ogr_sql_dialect.html#feature-id-fid)
and
[SQLite](https://gdal.org/en/stable/user/sql_sqlite_dialect.html#feature-id-fid)
SQL dialects. Implications for SQL statements and result sets may depend
on the dialect used.

Some vector formats do not support schema definition prior to creating
features. For example, with GeoJSON only the *Feature* object has a
member with name *properties*. The specification does not require all
*Feature* objects in a collection to have the same schema of properties,
nor does it require all *Feature* objects in a collection to have
geometry of the same type (<https://geojson.org/>).

## See also

[`ogr_ds_create()`](https://firelab.github.io/gdalraster/reference/ogr_manage.md),
[`ogr_layer_create()`](https://firelab.github.io/gdalraster/reference/ogr_manage.md),
[`ogr_field_create()`](https://firelab.github.io/gdalraster/reference/ogr_manage.md)

WKT representation of geometry:  
<https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry>

Field domains:  
<https://desktop.arcgis.com/en/arcmap/latest/manage-data/geodatabases/an-overview-of-attribute-domains.htm>  
<https://www.geopackage.org/spec/#extension_schema>  
<https://gdal.org/en/stable/doxygen/classOGRFieldDomain.html#details>

## Examples

``` r
# create a SQLite data source, with SpatiaLite extensions if available
dsn <- file.path(tempdir(), "test.sqlite")
opt <- NULL
if (has_spatialite()) {
  opt <- "SPATIALITE=YES"
}

# This creates an empty data source. Note that we could also create a layer
# at the same time in this function call, but for this example we do that
# separately, to show creation of a layer on an existing data source.
ogr_ds_create("SQLite", dsn, dsco = opt)
#> [1] TRUE

# define a layer
defn <- ogr_def_layer("Point", srs = "EPSG:4326")
defn$my_id <- ogr_def_field("OFTInteger64")
defn$my_description <- ogr_def_field("OFTString")

# create a layer in the existing data source
ogr_ds_test_cap(dsn)$CreateLayer  # TRUE
#> [1] TRUE

ogr_layer_create(dsn, "layer1", layer_defn = defn)
#> [1] TRUE

ogr_ds_layer_names(dsn)
#> [1] "layer1"

ogr_layer_field_names(dsn, "layer1")
#> [1] "my_id"          "my_description" "GEOMETRY"      

deleteDataset(dsn)
#> [1] TRUE
```
