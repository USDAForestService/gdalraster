# Convert vector data between different formats

`ogr2ogr()` is a wrapper of the `ogr2ogr` command-line utility (see
<https://gdal.org/en/stable/programs/ogr2ogr.html>). This function can
be used to convert simple features data between file formats. It can
also perform various operations during the process, such as spatial or
attribute selection, reducing the set of attributes, setting the output
coordinate system or even reprojecting the features during translation.
Refer to the GDAL documentation at the URL above for a description of
command-line arguments that can be passed in `cl_arg`.

## Usage

``` r
ogr2ogr(
  src_dsn,
  dst_dsn,
  src_layers = NULL,
  cl_arg = NULL,
  open_options = NULL
)
```

## Arguments

- src_dsn:

  Character string. Data source name of the source vector dataset.

- dst_dsn:

  Character string. Data source name of the destination vector dataset.

- src_layers:

  Optional character vector of layer names in the source dataset.
  Defaults to all layers.

- cl_arg:

  Optional character vector of command-line arguments for the GDAL
  `ogr2ogr` command-line utility (see URL above).

- open_options:

  Optional character vector of dataset open options.

## Value

Logical indicating success (invisible `TRUE`). An error is raised if the
operation fails.

## Note

For progress reporting, see command-line argument `-progress`: Display
progress on terminal. Only works if input layers have the "fast feature
count" capability.

## See also

[`ogrinfo()`](https://firelab.github.io/gdalraster/reference/ogrinfo.md),
the
[ogr_manage](https://firelab.github.io/gdalraster/reference/ogr_manage.md)
utilities

[`translate()`](https://firelab.github.io/gdalraster/reference/translate.md)
for raster data

## Examples

``` r
src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")

# Convert GeoPackage to Shapefile
ynp_shp <- file.path(tempdir(), "ynp_fires.shp")
ogr2ogr(src, ynp_shp, src_layers = "mtbs_perims")
#> GDAL WARNING 6: Normalized/laundered field name: 'burn_bnd_ac' to 'burn_bnd_a'
#> GDAL WARNING 6: Normalized/laundered field name: 'burn_bnd_lat' to 'burn_bnd_l'
#> GDAL WARNING 6: Normalized/laundered field name: 'burn_bnd_lon' to 'burn_bnd_1'

# Reproject to WGS84
ynp_gpkg <- file.path(tempdir(), "ynp_fires.gpkg")
args <- c("-t_srs", "EPSG:4326", "-nln", "fires_wgs84")
ogr2ogr(src, ynp_gpkg, cl_arg = args)

# Clip to a bounding box (xmin, ymin, xmax, ymax in the source SRS)
# This will select features whose geometry intersects the bounding box.
# The geometries themselves will not be clipped unless "-clipsrc" is
# specified.
# The source SRS can be overridden with "-spat_srs" "<srs_def>"
# Using -update mode to write a new layer in the existing DSN
bb <- c(469685.97, 11442.45, 544069.63, 85508.15)
args <- c("-update", "-nln", "fires_clip", "-spat", bb)
ogr2ogr(src, ynp_gpkg, cl_arg = args)

# Filter features by a -where clause
sql <- "ig_year >= 2000 ORDER BY ig_year"
args <- c("-update", "-nln", "fires_2000-2020", "-where", sql)
ogr2ogr(src, ynp_gpkg, src_layers = "mtbs_perims", cl_arg = args)

# Dissolve features based on a shared attribute value
if (has_spatialite()) {
    sql <- "SELECT ig_year, ST_Union(geom) AS geom
              FROM mtbs_perims GROUP BY ig_year"
    args <- c("-update", "-sql", sql, "-dialect", "SQLITE")
    args <- c(args, "-nlt", "MULTIPOLYGON", "-nln", "dissolved_on_year")
    ogr2ogr(src, ynp_gpkg, cl_arg = args)
}
```
