# Report structure and content of a multidimensional dataset

`mdim_info()` is an interface to the `gdalmdiminfo` command-line utility
(see <https://gdal.org/en/stable/programs/gdalmdiminfo.html>). This
function lists various information about a GDAL supported
multidimensional raster dataset as JSON output. It follows the JSON
schema
[gdalmdiminfo_output.schema.json](https://github.com/OSGeo/gdal/blob/release/3.11/apps/data/gdalmdiminfo_output.schema.json).
Requires GDAL \>= 3.2.

## Usage

``` r
mdim_info(
  dsn,
  array_name = "",
  pretty = TRUE,
  detailed = FALSE,
  limit = -1L,
  stats = FALSE,
  array_options = NULL,
  allowed_drivers = NULL,
  open_options = NULL,
  cout = TRUE
)
```

## Arguments

- dsn:

  Character string giving the data source name of the multidimensional
  raster (e.g., file, VSI path).

- array_name:

  Character string giving the name of the MDarray in `dsn`.

- pretty:

  Logical value, `FALSE` to output a single line without any
  indentation. Defaults to `TRUE`.

- detailed:

  Logical value, `TRUE` for verbose output. Report attribute data types
  and array values. Defaults to `FALSE`.

- limit:

  Integer value. Number of values in each dimension that is used to
  limit the display of array values. By default, unlimited. Only taken
  into account if used with `detailed = TRUE`. Set to a positive integer
  to enable.

- stats:

  Logical value, `TRUE` to read and display array statistics. Forces
  computation if no statistics are stored in an array. Defaults to
  `FALSE`.

- array_options:

  Optional character vector of `"NAME=VALUE"` pairs to filter reported
  arrays. Such option is format specific. Consult driver documentation
  (passed to `GDALGroup::GetMDArrayNames()`).

- allowed_drivers:

  Optional character vector of driver short names that must be
  considered when opening `dsn`. It is generally not necessary to
  specify it, but it can be used to skip automatic driver detection,
  when it fails to select the appropriate driver.

- open_options:

  Optional character vector of format-specific dataset openoptions as
  `"NAME=VALUE"` pairs.

- cout:

  Logical value, `TRUE` to print info to the console (the default), or
  `FALSE` to suppress console output.

## Value

Invisibly, a JSON string containing information about the
multidimensional raster dataset. By default, the info string is also
printed to the console unless `cout` is set to `FALSE`.

## See also

[`mdim_as_classic()`](https://firelab.github.io/gdalraster/reference/mdim_as_classic.md),
[`mdim_translate()`](https://firelab.github.io/gdalraster/reference/mdim_translate.md)

## Examples

``` r
f <- system.file("extdata/byte.nc", package="gdalraster")
mdim_info(f)
#> {
#>   "type": "group",
#>   "driver": "netCDF",
#>   "name": "/",
#>   "attributes": {
#>     "GDAL_AREA_OR_POINT": "Area",
#>     "Conventions": "CF-1.5",
#>     "GDAL": "GDAL 3.8.0dev-refs/heads-dirty, released 2023/10/09 (debug build)",
#>     "history": "Mon Oct 09 18:27:35 2023: GDAL CreateCopy( byte.nc, ... )"
#>   },
#>   "dimensions": [
#>     {
#>       "name": "x",
#>       "full_name": "/x",
#>       "size": 20,
#>       "type": "HORIZONTAL_X",
#>       "indexing_variable": {
#>         "x": {
#>           "datatype": "Float64",
#>           "dimensions": [
#>             "/x"
#>           ],
#>           "dimension_size": [
#>             20
#>           ],
#>           "attributes": {
#>             "standard_name": "projection_x_coordinate",
#>             "long_name": "x coordinate of projection"
#>           },
#>           "unit": "m"
#>         }
#>       }
#>     },
#>     {
#>       "name": "y",
#>       "full_name": "/y",
#>       "size": 20,
#>       "type": "HORIZONTAL_Y",
#>       "indexing_variable": {
#>         "y": {
#>           "datatype": "Float64",
#>           "dimensions": [
#>             "/y"
#>           ],
#>           "dimension_size": [
#>             20
#>           ],
#>           "attributes": {
#>             "standard_name": "projection_y_coordinate",
#>             "long_name": "y coordinate of projection"
#>           },
#>           "unit": "m"
#>         }
#>       }
#>     }
#>   ],
#>   "arrays": {
#>     "x": {
#>       "datatype": "Float64",
#>       "dimensions": [
#>         "/x"
#>       ],
#>       "dimension_size": [
#>         20
#>       ],
#>       "attributes": {
#>         "standard_name": "projection_x_coordinate",
#>         "long_name": "x coordinate of projection"
#>       },
#>       "unit": "m"
#>     },
#>     "y": {
#>       "datatype": "Float64",
#>       "dimensions": [
#>         "/y"
#>       ],
#>       "dimension_size": [
#>         20
#>       ],
#>       "attributes": {
#>         "standard_name": "projection_y_coordinate",
#>         "long_name": "y coordinate of projection"
#>       },
#>       "unit": "m"
#>     },
#>     "Band1": {
#>       "datatype": "Byte",
#>       "dimensions": [
#>         "/y",
#>         "/x"
#>       ],
#>       "dimension_size": [
#>         20,
#>         20
#>       ],
#>       "attributes": {
#>         "long_name": "GDAL Band Number 1",
#>         "valid_range": [0, 255]
#>       },
#>       "srs": {
#>         "wkt": "PROJCRS[\"NAD27 / UTM zone 11N\",BASEGEOGCRS[\"NAD27\",DATUM[\"North American Datum 1927\",ELLIPSOID[\"Clarke 1866\",6378206.4,294.978698213898,LENGTHUNIT[\"metre\",1]]],PRIMEM[\"Greenwich\",0,ANGLEUNIT[\"degree\",0.0174532925199433]],ID[\"EPSG\",4267]],CONVERSION[\"UTM zone 11N\",METHOD[\"Transverse Mercator\",ID[\"EPSG\",9807]],PARAMETER[\"Latitude of natural origin\",0,ANGLEUNIT[\"degree\",0.0174532925199433],ID[\"EPSG\",8801]],PARAMETER[\"Longitude of natural origin\",-117,ANGLEUNIT[\"degree\",0.0174532925199433],ID[\"EPSG\",8802]],PARAMETER[\"Scale factor at natural origin\",0.9996,SCALEUNIT[\"unity\",1],ID[\"EPSG\",8805]],PARAMETER[\"False easting\",500000,LENGTHUNIT[\"metre\",1],ID[\"EPSG\",8806]],PARAMETER[\"False northing\",0,LENGTHUNIT[\"metre\",1],ID[\"EPSG\",8807]]],CS[Cartesian,2],AXIS[\"easting\",east,ORDER[1],LENGTHUNIT[\"metre\",1]],AXIS[\"northing\",north,ORDER[2],LENGTHUNIT[\"metre\",1]],ID[\"EPSG\",26711]]",
#>         "data_axis_to_srs_axis_mapping": [2, 1]
#>       }
#>     }
#>   },
#>   "structural_info": {
#>     "NC_FORMAT": "CLASSIC"
#>   }
#> }
```
