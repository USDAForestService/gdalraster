# Retrieve information about a vector data source

`ogrinfo()` is a wrapper of the `ogrinfo` command-line utility (see
<https://gdal.org/en/stable/programs/ogrinfo.html>). This function lists
information about an OGR-supported data source. It is also possible to
edit data with SQL statements. Refer to the GDAL documentation at the
URL above for a description of command-line arguments that can be passed
in `cl_arg`. Requires GDAL \>= 3.7.

## Usage

``` r
ogrinfo(
  dsn,
  layers = NULL,
  cl_arg = as.character(c("-so", "-nomd")),
  open_options = NULL,
  read_only = TRUE,
  cout = TRUE
)
```

## Arguments

- dsn:

  Character string. Data source name (e.g., filename, database
  connection string, etc.)

- layers:

  Optional character vector of layer names in the source dataset.

- cl_arg:

  Optional character vector of command-line arguments for the `ogrinfo`
  command-line utility in GDAL (see URL above for reference). The
  default is `c("-so", "-nomd")` (see Note).

- open_options:

  Optional character vector of dataset open options.

- read_only:

  Logical scalar. `TRUE` to open the data source read-only (the
  default), or `FALSE` to open with write access.

- cout:

  Logical scalar. `TRUE` to write info to the standard C output stream
  (the default). `FALSE` to suppress console output.

## Value

Invisibly, a character string containing information about the vector
dataset, or empty string (`""`) in case of error.

## Note

The command-line argument `-so` provides a summary only, i.e., does not
include details about every single feature of a layer. `-nomd`
suppresses metadata printing. Some datasets may contain a lot of
metadata strings.

## See also

[`ogr2ogr()`](https://firelab.github.io/gdalraster/reference/ogr2ogr.md),
[ogr_manage](https://firelab.github.io/gdalraster/reference/ogr_manage.md)

## Examples

``` r
src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")

# Get the names of the layers in a GeoPackage file
ogrinfo(src)
#> INFO: Open of `/home/runner/work/_temp/Library/gdalraster/extdata/ynp_fires_1984_2022.gpkg'
#>       using driver `GPKG' successful.
#> 1: mtbs_perims (Multi Polygon)

# Summary of a layer
ogrinfo(src, "mtbs_perims")
#> INFO: Open of `/home/runner/work/_temp/Library/gdalraster/extdata/ynp_fires_1984_2022.gpkg'
#>       using driver `GPKG' successful.
#> 
#> Layer name: mtbs_perims
#> Geometry: Multi Polygon
#> Feature Count: 61
#> Extent: (469685.726682, -12917.756287) - (573531.719643, 96577.336358)
#> Layer SRS WKT:
#> PROJCRS["NAD83 / Montana",
#>     BASEGEOGCRS["NAD83",
#>         DATUM["North American Datum 1983",
#>             ELLIPSOID["GRS 1980",6378137,298.257222101,
#>                 LENGTHUNIT["metre",1]]],
#>         PRIMEM["Greenwich",0,
#>             ANGLEUNIT["degree",0.0174532925199433]],
#>         ID["EPSG",4269]],
#>     CONVERSION["SPCS83 Montana zone (meter)",
#>         METHOD["Lambert Conic Conformal (2SP)",
#>             ID["EPSG",9802]],
#>         PARAMETER["Latitude of false origin",44.25,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8821]],
#>         PARAMETER["Longitude of false origin",-109.5,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8822]],
#>         PARAMETER["Latitude of 1st standard parallel",49,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8823]],
#>         PARAMETER["Latitude of 2nd standard parallel",45,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8824]],
#>         PARAMETER["Easting at false origin",600000,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8826]],
#>         PARAMETER["Northing at false origin",0,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8827]]],
#>     CS[Cartesian,2],
#>         AXIS["easting (X)",east,
#>             ORDER[1],
#>             LENGTHUNIT["metre",1]],
#>         AXIS["northing (Y)",north,
#>             ORDER[2],
#>             LENGTHUNIT["metre",1]],
#>     USAGE[
#>         SCOPE["Engineering survey, topographic mapping."],
#>         AREA["United States (USA) - Montana - counties of Beaverhead; Big Horn; Blaine; Broadwater; Carbon; Carter; Cascade; Chouteau; Custer; Daniels; Dawson; Deer Lodge; Fallon; Fergus; Flathead; Gallatin; Garfield; Glacier; Golden Valley; Granite; Hill; Jefferson; Judith Basin; Lake; Lewis and Clark; Liberty; Lincoln; Madison; McCone; Meagher; Mineral; Missoula; Musselshell; Park; Petroleum; Phillips; Pondera; Powder River; Powell; Prairie; Ravalli; Richland; Roosevelt; Rosebud; Sanders; Sheridan; Silver Bow; Stillwater; Sweet Grass; Teton; Toole; Treasure; Valley; Wheatland; Wibaux; Yellowstone."],
#>         BBOX[44.35,-116.07,49.01,-104.04]],
#>     ID["EPSG",32100]]
#> Data axis to CRS axis mapping: 1,2
#> FID Column = fid
#> Geometry Column = geom
#> event_id: String (254.0)
#> incid_name: String (254.0)
#> incid_type: String (254.0)
#> map_id: Integer64 (0.0)
#> burn_bnd_ac: Integer64 (0.0)
#> burn_bnd_lat: String (10.0)
#> burn_bnd_lon: String (10.0)
#> ig_date: Date
#> ig_year: Integer (0.0)

# Query an attribute to restrict the output of the features in a layer
args <- c("-ro", "-nomd", "-where", "ig_year = 2020")
ogrinfo(src, "mtbs_perims", args)
#> INFO: Open of `/home/runner/work/_temp/Library/gdalraster/extdata/ynp_fires_1984_2022.gpkg'
#>       using driver `GPKG' successful.
#> 
#> Layer name: mtbs_perims
#> Geometry: Multi Polygon
#> Feature Count: 1
#> Extent: (469685.726682, -12917.756287) - (573531.719643, 96577.336358)
#> Layer SRS WKT:
#> PROJCRS["NAD83 / Montana",
#>     BASEGEOGCRS["NAD83",
#>         DATUM["North American Datum 1983",
#>             ELLIPSOID["GRS 1980",6378137,298.257222101,
#>                 LENGTHUNIT["metre",1]]],
#>         PRIMEM["Greenwich",0,
#>             ANGLEUNIT["degree",0.0174532925199433]],
#>         ID["EPSG",4269]],
#>     CONVERSION["SPCS83 Montana zone (meter)",
#>         METHOD["Lambert Conic Conformal (2SP)",
#>             ID["EPSG",9802]],
#>         PARAMETER["Latitude of false origin",44.25,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8821]],
#>         PARAMETER["Longitude of false origin",-109.5,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8822]],
#>         PARAMETER["Latitude of 1st standard parallel",49,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8823]],
#>         PARAMETER["Latitude of 2nd standard parallel",45,
#>             ANGLEUNIT["degree",0.0174532925199433],
#>             ID["EPSG",8824]],
#>         PARAMETER["Easting at false origin",600000,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8826]],
#>         PARAMETER["Northing at false origin",0,
#>             LENGTHUNIT["metre",1],
#>             ID["EPSG",8827]]],
#>     CS[Cartesian,2],
#>         AXIS["easting (X)",east,
#>             ORDER[1],
#>             LENGTHUNIT["metre",1]],
#>         AXIS["northing (Y)",north,
#>             ORDER[2],
#>             LENGTHUNIT["metre",1]],
#>     USAGE[
#>         SCOPE["Engineering survey, topographic mapping."],
#>         AREA["United States (USA) - Montana - counties of Beaverhead; Big Horn; Blaine; Broadwater; Carbon; Carter; Cascade; Chouteau; Custer; Daniels; Dawson; Deer Lodge; Fallon; Fergus; Flathead; Gallatin; Garfield; Glacier; Golden Valley; Granite; Hill; Jefferson; Judith Basin; Lake; Lewis and Clark; Liberty; Lincoln; Madison; McCone; Meagher; Mineral; Missoula; Musselshell; Park; Petroleum; Phillips; Pondera; Powder River; Powell; Prairie; Ravalli; Richland; Roosevelt; Rosebud; Sanders; Sheridan; Silver Bow; Stillwater; Sweet Grass; Teton; Toole; Treasure; Valley; Wheatland; Wibaux; Yellowstone."],
#>         BBOX[44.35,-116.07,49.01,-104.04]],
#>     ID["EPSG",32100]]
#> Data axis to CRS axis mapping: 1,2
#> FID Column = fid
#> Geometry Column = geom
#> event_id: String (254.0)
#> incid_name: String (254.0)
#> incid_type: String (254.0)
#> map_id: Integer64 (0.0)
#> burn_bnd_ac: Integer64 (0.0)
#> burn_bnd_lat: String (10.0)
#> burn_bnd_lon: String (10.0)
#> ig_date: Date
#> ig_year: Integer (0.0)
#> OGRFeature(mtbs_perims):61
#>   event_id (String) = WY4438911082120200822
#>   incid_name (String) = LONE STAR
#>   incid_type (String) = Wildfire
#>   map_id (Integer64) = 10020495
#>   burn_bnd_ac (Integer64) = 3348
#>   burn_bnd_lat (String) = 44.4
#>   burn_bnd_lon (String) = -110.782
#>   ig_date (Date) = 2020/08/22
#>   ig_year (Integer) = 2020
#>   MULTIPOLYGON (((496593.122306971 15506.8828590633,496491.761299067 15605.3612548792,496290.812130161 15388.0465179707,496283.32058496 15550.7644680398,495884.310957458 15387.7693642235,495381.498326246 15486.6808036345,494602.564947774 16084.5219875909,494585.357018058 16645.0572271436,495449.965836048 17082.648614691,495525.640892666 17349.0599324865,496097.733732823 17561.7347277213,496257.076620071 17855.1480298667,496528.929476077 17642.2200632185,496381.989621625 18210.3014108741,496658.623671105 18438.1622229537,496910.765693779 18401.6123902032,497124.005995667 18641.1244857657,497385.057088859 18431.8151440043,497742.662074771 18486.7201558508,498208.745156148 18684.677772214,498121.821253373 18790.2831039076,498326.37319691 19018.3598994669,498728.212799118 18905.4308658479,498920.731848098 19218.8393941772,499304.507662952 19088.0477058364,499719.773581338 19427.3696509425,499951.447535602 19280.5161594677,500309.224626601 19407.1241777105,500350.581048002 19515.2429516582,500076.319969425 19635.6901072339,500585.91703226 19670.8582722843,500834.488053378 19367.7099358598,501033.423276492 19380.0736723167,500824.027693014 19222.9522488669,501072.354115678 19104.0152762244,501094.913433901 18907.5622339096,500627.002229506 18832.1354026641,499946.728772314 18292.1207493556,499222.789246595 18426.7059182243,498360.994034972 17933.0977969646,499077.730444483 17521.134480303,499293.084153096 17745.599510172,499800.921713803 17647.4449673673,499785.561166901 17281.9472585942,500335.380841845 17251.0600839751,500381.818490701 16812.2886367497,499899.5387543 16765.5825321997,499579.392382428 16152.9342602384,499429.399837531 16121.8098907012,499265.572201365 16590.287674071,499028.649451778 16664.7644176055,498928.284642745 16318.141684453,498664.147923921 16184.1267454724,498679.339316004 15930.3618365987,498015.351459606 15310.80515169,497110.236344284 15635.150719899,496862.739246991 15529.0560671027,496817.793936327 15701.9140384736,496593.122306971 15506.8828590633)))
#> 

# Copy to a temporary in-memory file that is writeable
src_mem <- paste0("/vsimem/", basename(src))
vsi_copy_file(src, src_mem)
#> [1] 0

# Add a column to a layer
args <- c("-sql", "ALTER TABLE mtbs_perims ADD burn_bnd_ha float")
ogrinfo(src_mem, cl_arg = args, read_only = FALSE)
#> INFO: Open of `/vsimem/ynp_fires_1984_2022.gpkg'
#>       using driver `GPKG' successful.

# Update values of the column with SQL and specify a dialect
sql <- "UPDATE mtbs_perims SET burn_bnd_ha = (burn_bnd_ac / 2.471)"
args <- c("-dialect", "sqlite", "-sql", sql)
ogrinfo(src_mem, cl_arg = args, read_only = FALSE)
#> INFO: Open of `/vsimem/ynp_fires_1984_2022.gpkg'
#>       using driver `GPKG' successful.
```
