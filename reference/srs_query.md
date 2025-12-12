# Obtain information about a spatial reference system

Bindings to a subset of the GDAL Spatial Reference System API
(<https://gdal.org/en/stable/api/ogr_srs_api.html>). These functions
return various information about a spatial reference system passed as
text in any of the formats supported by
[`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

## Usage

``` r
srs_get_name(srs)

srs_find_epsg(srs, all_matches = FALSE)

srs_is_geographic(srs)

srs_is_derived_gcs(srs)

srs_is_local(srs)

srs_is_projected(srs)

srs_is_compound(srs)

srs_is_geocentric(srs)

srs_is_vertical(srs)

srs_is_dynamic(srs)

srs_is_same(
  srs,
  srs_other,
  criterion = "",
  ignore_axis_mapping = FALSE,
  ignore_coord_epoch = FALSE
)

srs_get_angular_units(srs)

srs_get_linear_units(srs)

srs_get_coord_epoch(srs)

srs_get_utm_zone(srs)

srs_get_axis_mapping_strategy(srs)

srs_get_area_of_use(srs)

srs_get_axes_count(srs)

srs_get_axes(srs, target_key = NULL)

srs_epsg_treats_as_lat_long(srs)

srs_epsg_treats_as_northing_easting(srs)

srs_get_celestial_body_name(srs)
```

## Arguments

- srs:

  Character string containing an SRS definition in various formats
  (e.g., WKT, PROJ.4 string, well known name such as NAD27, NAD83,
  WGS84, etc., see
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md)).

- all_matches:

  Logical scalar. `TRUE` to return all identified matches in a data
  frame, including a confidence value (0-100) for each match. The
  default is `FALSE` which returns a character string in the form
  `"EPSG:<code>"` for the first match (highest confidence).

- srs_other:

  Character string containing an SRS definition in various formats(see
  above).

- criterion:

  Character string. One of `STRICT`, `EQUIVALENT`,
  `EQUIVALENT_EXCEPT_AXIS_ORDER_GEOGCRS`. Defaults to
  `EQUIVALENT_EXCEPT_AXIS_ORDER_GEOGCRS`.

- ignore_axis_mapping:

  Logical scalar. If `TRUE`, sets
  `IGNORE_DATA_AXIS_TO_SRS_AXIS_MAPPING=YES` in the call to
  `OSRIsSameEx()` in the GDAL Spatial Reference System API. Defaults to
  `NO`.

- ignore_coord_epoch:

  Logical scalar. If `TRUE`, sets `IGNORE_COORDINATE_EPOCH=YES` in the
  call to `OSRIsSameEx()` in the GDAL Spatial Reference System API.
  Defaults to `NO`.

- target_key:

  Optional character string giving the coordinate system part to query,
  either `"PROJCS"` or `"GEOGCS"` (case-insensitive)

## Details

`srs_find_epsg()` tries to find a matching EPSG code. Matching may be
partial, or may fail. If `all_matches = TRUE`, returns a data frame with
entries sorted by decreasing match confidence (first entry has the
highest match confidence); the default is `FALSE` which returns a
character string in the form "EPSG:####" for the first match (highest
confidence). Wrapper of `OSRFindMatches()` in the GDAL SRS API.

`srs_get_name()` returns the SRS name. Wrapper of `OSRGetName()` in the
GDAL API.

`srs_is_geographic()` returns `TRUE` if the root is a GEOGCS node.
Wrapper of `OSRIsGeographic()` in the GDAL API.

`srs_is_derived_gcs()` returns `TRUE` if the SRS is a derived geographic
coordinate system (for example a rotated long/lat grid). Wrapper of
`OSRIsDerivedGeographic()` in the GDAL API.

`srs_is_local()` returns `TRUE` if the SRS is a local coordinate system
(the root is a LOCAL_CS node). Wrapper of `OSRIsLocal()` in the GDAL
API.

`srs_is_projected()` returns `TRUE` if the SRS contains a PROJCS node
indicating a it is a projected coordinate system. Wrapper of
`OSRIsProjected()` in the GDAL API.

`srs_is_compound()` returns `TRUE` if the SRS is compound. Wrapper of
`OSRIsCompound()` in the GDAL API.

`srs_is_geocentric()` returns `TRUE` if the SRS is a geocentric
coordinate system. Wrapper of `OSRIsGeocentric()` in the GDAL API.

`srs_is_vertical()` returns `TRUE` if the SRS is a vertical coordinate
system. Wrapper of `OSRIsVertical()` in the GDAL API.

`srs_is_dynamic()` returns `TRUE` if the SRS is is a dynamic coordinate
system (relies on a dynamic datum, i.e., a datum that is not
plate-fixed). Wrapper of `OSRIsDynamic()` in the GDAL API. Requires GDAL
\>= 3.4.

`srs_is_same()` returns `TRUE` if two spatial references describe the
same system. Wrapper of `OSRIsSame()` in the GDAL API.

`srs_get_angular_units()` fetches the angular geographic coordinate
system units. Returns a list of length two: the first element contains
the unit name as a character string, and the second element contains a
numeric value to multiply by angular distances to transform them to
radians. Wrapper of `OSRGetAngularUnits()` in the GDAL API.

`srs_get_linear_units()` fetches the linear projection units. Returns a
list of length two: the first element contains the unit name as a
character string, and the second element contains a numeric value to
multiply by linear distances to transform them to meters. If no units
are available, values of "Meters" and 1.0 will be assumed. Wrapper of
`OSRGetLinearUnits()` in the GDAL API.

`srs_get_coord_epoch()` returns the coordinate epoch, as decimal year
(e.g. 2021.3), or `0` if not set or not relevant. Wrapper of
`OSRGetCoordinateEpoch()` in the GDAL API. Requires GDAL \>= 3.4.

`srs_get_utm_zone()` returns the UTM zone number or zero if `srs` isn't
a UTM definition. A positive value indicates northern hemisphere; a
negative value is in the southern hemisphere. Wrapper of
`OSRGetUTMZone()` in the GDAL API.

`srs_get_axis_mapping_strategy()` returns the data axis to CRS axis
mapping strategy as a character string, one of:

- `OAMS_TRADITIONAL_GIS_ORDER`: for geographic CRS with lat/long order,
  the data will still be long/lat ordered. Similarly for a projected CRS
  with northing/easting order, the data will still be easting/northing
  ordered.

- `OAMS_AUTHORITY_COMPLIANT`: the data axis will be identical to the CRS
  axis.

- `OAMS_CUSTOM`: custom-defined data axis

`srs_get_area_of_use()` is a wrapper of `OSRGetAreaOfUse()` in the GDAL
API. Returns a named list containing the following elements (or returns
`NULL` if the API call does not succeed):

- `AreaName`: the area of use

- `WestLongitudeDeg`: the western-most longitude expressed in degree, or
  `NA` if the bounding box is unknown

- `SouthLatitudeDeg`: the southern-most latitude expressed in degree, or
  `NA` if the bounding box is unknown

- `EastLongitudeDeg`: the eastern-most longitude expressed in degree, or
  `NA` if the bounding box is unknown

- `NorthLatitudeDeg`: the northern-most latitude expressed in degree, or
  `NA` if the bounding box is unknown

`srs_get_axes_count()` returns the integer number of axes of the
coordinate system of the SRS. Wrapper of `OSRGetAxesCount()` in the GDAL
API.

`srs_get_axes()` returns a named list of the axis names and their
orientations. Wrapper of `OSRGetAxis()` in the GDAL API.

`srs_epsg_treats_as_lat_long()` returns `TRUE` if this geographic
coordinate system should be treated as having latitude/longitude
coordinate ordering. Wrapper of `OSREPSGTreatsAsLatLong()` in the GDAL
API.

`srs_epsg_treats_as_northing_easting()` returns `TRUE` if this
geographic coordinate system should be treated as having
northing/easting coordinate ordering. Wrapper of
`OSREPSGTreatsAsNorthingEasting()` in the GDAL API.

`srs_get_celestial_body_name()` returns the name of the celestial body
of the SRS, e.g., `"Earth"` for an Earth SRS. Wrapper of
`OSRGetCelestialBodyName()` in the GDAL API. Requires GDAL \>= 3.12 and
PROJ \>= 8.1.

## See also

[srs_convert](https://firelab.github.io/gdalraster/reference/srs_convert.md)

## Examples

``` r
wkt <- 'PROJCS["ETRS89 / UTM zone 32N (N-E)",
        GEOGCS["ETRS89",
            DATUM["European_Terrestrial_Reference_System_1989",
                SPHEROID["GRS 1980",6378137,298.257222101,
                    AUTHORITY["EPSG","7019"]],
                TOWGS84[0,0,0,0,0,0,0],
                AUTHORITY["EPSG","6258"]],
            PRIMEM["Greenwich",0,
                AUTHORITY["EPSG","8901"]],
            UNIT["degree",0.0174532925199433,
                AUTHORITY["EPSG","9122"]],
            AUTHORITY["EPSG","4258"]],
        PROJECTION["Transverse_Mercator"],
        PARAMETER["latitude_of_origin",0],
        PARAMETER["central_meridian",9],
        PARAMETER["scale_factor",0.9996],
        PARAMETER["false_easting",500000],
        PARAMETER["false_northing",0],
        UNIT["metre",1,
            AUTHORITY["EPSG","9001"]],
        AXIS["Northing",NORTH],
        AXIS["Easting",EAST]]'

srs_find_epsg(wkt)
#> [1] "EPSG:3044"

srs_find_epsg(wkt, all_matches = TRUE)
#>   authority_name authority_code confidence
#> 1           EPSG           3044        100

srs_get_name("EPSG:5070")
#> [1] "NAD83 / Conus Albers"

srs_is_geographic("EPSG:5070")
#> [1] FALSE
srs_is_geographic("EPSG:4326")
#> [1] TRUE

srs_is_derived_gcs("WGS84")
#> [1] FALSE

srs_is_projected("EPSG:5070")
#> [1] TRUE
srs_is_projected("EPSG:4326")
#> [1] FALSE

srs_is_compound("EPSG:4326")
#> [1] FALSE

srs_is_geocentric("EPSG:7789")
#> [1] TRUE

srs_is_vertical("EPSG:5705")
#> [1] TRUE

srs_get_area_of_use("EPSG:3976")
#> $AreaName
#> [1] "Southern hemisphere - south of 60°S onshore and offshore - Antarctica."
#> 
#> $WestLongitudeDeg
#> [1] -180
#> 
#> $SouthLatitudeDeg
#> [1] -90
#> 
#> $EastLongitudeDeg
#> [1] 180
#> 
#> $NorthLatitudeDeg
#> [1] -60
#> 

srs_get_axes_count("EPSG:4326")
#> [1] 2
srs_get_axes_count("EPSG:4979")
#> [1] 3

# ordered list of axis names and their orientation
srs_get_axes("EPSG:4326+5773")
#> $Geodetic_latitude
#> [1] "north"
#> 
#> $Geodetic_longitude
#> [1] "east"
#> 
#> $`Gravity-related_height`
#> [1] "up"
#> 

srs_epsg_treats_as_lat_long("WGS84")
#> [1] TRUE

# NAD83 / Conus Albers:
srs_epsg_treats_as_northing_easting("EPSG:5070")
#> [1] FALSE
# WGS 84 / UPS North (N,E):
srs_epsg_treats_as_northing_easting("EPSG:32661")
#> [1] TRUE
# WGS 84 / UPS South (N,E):
srs_epsg_treats_as_northing_easting("EPSG:32761")
#> [1] TRUE

## Requires GDAL >= 3.12 and PROJ >= 8.1
# srs_get_celestial_body_name("EPSG:4326")
#> [1] "Earth"
# srs_get_celestial_body_name("IAU_2015:30100")
#> [1] "Moon"

f <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, f)

ds$getProjection() |> srs_is_projected()
#> [1] TRUE
ds$getProjection() |> srs_get_utm_zone()
#> [1] 12
ds$getProjection() |> srs_get_angular_units()
#> $unit_name
#> [1] "degree"
#> 
#> $to_radians
#> [1] 0.01745329
#> 
ds$getProjection() |> srs_get_linear_units()
#> $unit_name
#> [1] "metre"
#> 
#> $to_meters
#> [1] 1
#> 
ds$getProjection() |> srs_get_axis_mapping_strategy()
#> [1] "OAMS_AUTHORITY_COMPLIANT"

ds$getProjection() |> srs_is_same("EPSG:26912")
#> [1] TRUE
ds$getProjection() |> srs_is_same("NAD83")
#> [1] FALSE

ds$close()

## Requires GDAL >= 3.4
if (gdal_version_num() >= gdal_compute_version(3, 4, 0)) {
  if (srs_is_dynamic("WGS84"))
    print("WGS84 is dynamic")

  if (!srs_is_dynamic("NAD83"))
    print("NAD83 is not dynamic")
}
#> [1] "WGS84 is dynamic"
#> [1] "NAD83 is not dynamic"
```
