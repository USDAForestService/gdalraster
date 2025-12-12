# Convert spatial reference definitions to OGC WKT or PROJJSON

These functions convert various spatial reference formats to Well Known
Text (WKT) or PROJJSON.

## Usage

``` r
epsg_to_wkt(epsg, pretty = FALSE)

srs_to_wkt(srs, pretty = FALSE, gcs_only = FALSE)

srs_to_projjson(
  srs,
  multiline = TRUE,
  indent_width = 2L,
  schema = NA_character_
)
```

## Arguments

- epsg:

  Integer EPSG code.

- pretty:

  Logical value. `TRUE` to return a nicely formatted WKT string for
  display to a person. `FALSE` for a regular WKT string (the default).

- srs:

  Character string containing an SRS definition in various formats (see
  Details).

- gcs_only:

  Logical value. `TRUE` to return only the definition of the GEOGCS node
  of the input `srs`. Defaults to `FALSE` (see Note).

- multiline:

  Logical value. `TRUE` for PROJJSON multiline output (the default).

- indent_width:

  Integer value. Defaults to `2`. Only used if `multiline = TRUE` for
  PROJJSON output.

- schema:

  Character string containing URL to PROJJSON schema. Can be set to
  empty string to disable it.

## Value

Character string containing OGC WKT.

## Details

`epsg_to_wkt()` exports the spatial reference for an EPSG code to WKT
format. Wrapper for `OSRImportFromEPSG()` in the GDAL Spatial Reference
System API with output to WKT.

`srs_to_wkt()` converts a spatial reference system (SRS) definition in
various text formats to WKT. The function will examine the input SRS,
try to deduce the format, and then export it to WKT. Wrapper for
`OSRSetFromUserInput()` in the GDAL Spatial Reference System API with
output to WKT.

`srs_to_projjson()` accepts a spatial reference system (SRS) definition
in any of the formats supported by `srs_to_wkt()`, and converts into
PROJJSON format. Wrapper for `OSRExportToPROJJSON()` in the GDAL Spatial
Reference System API.

The input SRS may take the following forms:

- WKT - to convert WKT versions (see below)

- EPSG:n - EPSG code n

- AUTO:proj_id,unit_id,lon0,lat0 - WMS auto projections

- urn:ogc:def:crs:EPSG::n - OGC URNs

- PROJ.4 definitions

- filename - file to read for WKT, XML or PROJ.4 definition

- well known name such as NAD27, NAD83, WGS84 or WGS72

- IGNF:xxxx, ESRI:xxxx - definitions from the PROJ database

- PROJJSON (PROJ \>= 6.2)

`srs_to_wkt()` is intended to be flexible, but by its nature it is
imprecise as it must guess information about the format intended.
`epsg_to_wkt()` could be used instead for EPSG codes.

As of GDAL 3.0, the default format for WKT export is OGC WKT 1. The WKT
version can be overridden by using the OSR_WKT_FORMAT configuration
option (see
[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md)).
Valid values are one of: SFSQL, WKT1_SIMPLE, WKT1, WKT1_GDAL, WKT1_ESRI,
WKT2_2015, WKT2_2018, WKT2, DEFAULT. If SFSQL, a WKT1 string without
AXIS, TOWGS84, AUTHORITY or EXTENSION node is returned. If WKT1_SIMPLE,
a WKT1 string without AXIS, AUTHORITY or EXTENSION node is returned.
WKT1 is an alias of WKT1_GDAL. WKT2 will default to the latest revision
implemented (currently WKT2_2018). WKT2_2019 can be used as an alias of
WKT2_2018 since GDAL 3.2

## Note

Setting `gcs_only = TRUE` in `srs_to_wkt()` is a wrapper of
`OSRCloneGeogCS()` in the GDAL API. The returned WKT will be for the
GEOGCS node of the input `srs`.

## See also

[srs_query](https://firelab.github.io/gdalraster/reference/srs_query.md)

## Examples

``` r
epsg_to_wkt(5070)
#> [1] "PROJCS[\"NAD83 / Conus Albers\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4269\"]],PROJECTION[\"Albers_Conic_Equal_Area\"],PARAMETER[\"latitude_of_center\",23],PARAMETER[\"longitude_of_center\",-96],PARAMETER[\"standard_parallel_1\",29.5],PARAMETER[\"standard_parallel_2\",45.5],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"5070\"]]"
writeLines(epsg_to_wkt(5070, pretty = TRUE))
#> PROJCS["NAD83 / Conus Albers",
#>     GEOGCS["NAD83",
#>         DATUM["North_American_Datum_1983",
#>             SPHEROID["GRS 1980",6378137,298.257222101,
#>                 AUTHORITY["EPSG","7019"]],
#>             AUTHORITY["EPSG","6269"]],
#>         PRIMEM["Greenwich",0,
#>             AUTHORITY["EPSG","8901"]],
#>         UNIT["degree",0.0174532925199433,
#>             AUTHORITY["EPSG","9122"]],
#>         AUTHORITY["EPSG","4269"]],
#>     PROJECTION["Albers_Conic_Equal_Area"],
#>     PARAMETER["latitude_of_center",23],
#>     PARAMETER["longitude_of_center",-96],
#>     PARAMETER["standard_parallel_1",29.5],
#>     PARAMETER["standard_parallel_2",45.5],
#>     PARAMETER["false_easting",0],
#>     PARAMETER["false_northing",0],
#>     UNIT["metre",1,
#>         AUTHORITY["EPSG","9001"]],
#>     AXIS["Easting",EAST],
#>     AXIS["Northing",NORTH],
#>     AUTHORITY["EPSG","5070"]]

srs_to_wkt("NAD83")
#> [1] "GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AXIS[\"Latitude\",NORTH],AXIS[\"Longitude\",EAST],AUTHORITY[\"EPSG\",\"4269\"]]"
writeLines(srs_to_wkt("NAD83", pretty = TRUE))
#> GEOGCS["NAD83",
#>     DATUM["North_American_Datum_1983",
#>         SPHEROID["GRS 1980",6378137,298.257222101,
#>             AUTHORITY["EPSG","7019"]],
#>         AUTHORITY["EPSG","6269"]],
#>     PRIMEM["Greenwich",0,
#>         AUTHORITY["EPSG","8901"]],
#>     UNIT["degree",0.0174532925199433,
#>         AUTHORITY["EPSG","9122"]],
#>     AXIS["Latitude",NORTH],
#>     AXIS["Longitude",EAST],
#>     AUTHORITY["EPSG","4269"]]
set_config_option("OSR_WKT_FORMAT", "WKT2")
writeLines(srs_to_wkt("NAD83", pretty = TRUE))
#> GEOGCRS["NAD83",
#>     DATUM["North American Datum 1983",
#>         ELLIPSOID["GRS 1980",6378137,298.257222101,
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
#>     ID["EPSG",4269]]
set_config_option("OSR_WKT_FORMAT", "")

srs_to_wkt("EPSG:5070", gcs_only = TRUE)
#> [1] "GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AXIS[\"Latitude\",NORTH],AXIS[\"Longitude\",EAST],AUTHORITY[\"EPSG\",\"4269\"]]"

srs_to_projjson("NAD83") |> writeLines()
#> {
#>   "$schema": "https://proj.org/schemas/v0.7/projjson.schema.json",
#>   "type": "GeographicCRS",
#>   "name": "NAD83",
#>   "datum": {
#>     "type": "GeodeticReferenceFrame",
#>     "name": "North American Datum 1983",
#>     "ellipsoid": {
#>       "name": "GRS 1980",
#>       "semi_major_axis": 6378137,
#>       "inverse_flattening": 298.257222101
#>     }
#>   },
#>   "coordinate_system": {
#>     "subtype": "ellipsoidal",
#>     "axis": [
#>       {
#>         "name": "Geodetic latitude",
#>         "abbreviation": "Lat",
#>         "direction": "north",
#>         "unit": "degree"
#>       },
#>       {
#>         "name": "Geodetic longitude",
#>         "abbreviation": "Lon",
#>         "direction": "east",
#>         "unit": "degree"
#>       }
#>     ]
#>   },
#>   "id": {
#>     "authority": "EPSG",
#>     "code": 4269
#>   }
#> }
```
