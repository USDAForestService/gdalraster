# Create a polygon feature layer from raster data

`polygonize()` creates vector polygons for all connected regions of
pixels in a source raster sharing a common pixel value. Each polygon is
created with an attribute indicating the pixel value of that polygon. A
raster mask may also be provided to determine which pixels are eligible
for processing. The function will create the output vector layer if it
does not already exist, otherwise it will try to append to an existing
one. This function is a wrapper of `GDALPolygonize` in the GDAL
Algorithms API. It provides essentially the same functionality as the
`gdal_polygonize.py` command-line program
(<https://gdal.org/en/stable/programs/gdal_polygonize.html>).

## Usage

``` r
polygonize(
  raster_file,
  out_dsn,
  out_layer,
  fld_name = "DN",
  out_fmt = NULL,
  connectedness = 4,
  src_band = 1,
  mask_file = NULL,
  nomask = FALSE,
  overwrite = FALSE,
  dsco = NULL,
  lco = NULL,
  quiet = FALSE
)
```

## Arguments

- raster_file:

  Filename of the source raster.

- out_dsn:

  The destination vector filename to which the polygons will be written
  (or database connection string).

- out_layer:

  Name of the layer for writing the polygon features. For single-layer
  file formats such as `"ESRI Shapefile"`, the layer name is the same as
  the filename without the path or extension (e.g.,
  `out_dsn = "path_to_file/polygon_output.shp"`, the layer name is
  `"polygon_output"`).

- fld_name:

  Name of an integer attribute field in `out_layer` to which the pixel
  values will be written. Will be created if necessary when using an
  existing layer.

- out_fmt:

  GDAL short name of the output vector format. If unspecified, the
  function will attempt to guess the format from the filename/connection
  string.

- connectedness:

  Integer scalar. Must be either `4` or `8`. For the default
  4-connectedness, pixels with the same value are considered connected
  only if they touch along one of the four sides, while 8-connectedness
  also includes pixels that touch at one of the corners.

- src_band:

  The band on `raster_file` to build the polygons from (default is `1`).

- mask_file:

  Use the first band of the specified raster as a validity mask (zero is
  invalid, non-zero is valid). If not specified, the default validity
  mask for the input band (such as nodata, or alpha masks) will be used
  (unless `nomask` is set to `TRUE`).

- nomask:

  Logical scalar. If `TRUE`, do not use the default validity mask for
  the input band (such as nodata, or alpha masks). Default is `FALSE`.

- overwrite:

  Logical scalar. If `TRUE`, overwrite `out_layer` if it already exists.
  Default is `FALSE`.

- dsco:

  Optional character vector of format-specific creation options for
  `out_dsn` (`"NAME=VALUE"` pairs).

- lco:

  Optional character vector of format-specific creation options for
  `out_layer` (`"NAME=VALUE"` pairs).

- quiet:

  Logical scalar. If `TRUE`, a progress bar will not be displayed.
  Defaults to `FALSE`.

## Details

Polygon features will be created on the output layer, with polygon
geometries representing the polygons. The polygon geometries will be in
the georeferenced coordinate system of the raster (based on the
geotransform of the source dataset). It is acceptable for the output
layer to already have features. If the output layer does not already
exist, it will be created with coordinate system matching the source
raster.

The algorithm attempts to minimize memory use so that very large rasters
can be processed. However, if the raster has many polygons or very
large/complex polygons, the memory use for holding polygon enumerations
and active polygon geometries may grow to be quite large.

The algorithm will generally produce very dense polygon geometries, with
edges that follow exactly on pixel boundaries for all non-interior
pixels. For non-thematic raster data (such as satellite images) the
result will essentially be one small polygon per pixel, and memory and
output layer sizes will be substantial. The algorithm is primarily
intended for relatively simple thematic rasters, masks, and
classification results.

## Note

The source pixel band values are read into a signed 64-bit integer
buffer (`Int64`) by `GDALPolygonize`, so floating point or complex bands
will be implicitly truncated before processing.

When 8-connectedness is used, many of the resulting polygons will likely
be invalid due to ring self-intersection (in the strict OGC definition
of polygon validity). See
[`g_is_valid()`](https://firelab.github.io/gdalraster/reference/g_query.md)
/
[`g_make_valid()`](https://firelab.github.io/gdalraster/reference/g_util.md)
(single polygons can become MultiPolygon' in the case of
self-intersections).

If writing to a SQLite database format as either `GPKG` (GeoPackage
vector) or `SQLite` (Spatialite vector), setting the
`SQLITE_USE_OGR_VFS` and `OGR_SQLITE_JOURNAL` configuration options may
increase performance substantially. If writing to `PostgreSQL` (PostGIS
vector), setting `PG_USE_COPY=YES` is faster:

    # SQLite: GPKG (.gpkg) and Spatialite (.sqlite)
    # enable extra buffering/caching by the GDAL/OGR I/O layer
    set_config_option("SQLITE_USE_OGR_VFS", "YES")
    # set the journal mode for the SQLite database to MEMORY
    set_config_option("OGR_SQLITE_JOURNAL", "MEMORY")

    # PostgreSQL / PostGIS
    # use COPY for inserting data rather than INSERT
    set_config_option("PG_USE_COPY", "YES")

## See also

[`rasterize()`](https://firelab.github.io/gdalraster/reference/rasterize.md)

[`vignette("gdal-config-quick-ref")`](https://firelab.github.io/gdalraster/articles/gdal-config-quick-ref.md)

## Examples

``` r
evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
dsn <- file.path(tempdir(), "storm_lake.gpkg")
layer <- "lf_evt"
fld <- "evt_value"
set_config_option("SQLITE_USE_OGR_VFS", "YES")
set_config_option("OGR_SQLITE_JOURNAL", "MEMORY")
polygonize(evt_file, dsn, layer, fld)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
set_config_option("SQLITE_USE_OGR_VFS", "")
set_config_option("OGR_SQLITE_JOURNAL", "")
```
