# Is SpatiaLite available?

`has_spatialite()` returns a logical value indicating whether GDAL was
built with support for the SpatiaLite library. SpatiaLite extends the
SQLite core to support full Spatial SQL capabilities.

## Usage

``` r
has_spatialite()
```

## Value

Logical scalar. `TRUE` if SpatiaLite is available to GDAL.

## Details

GDAL supports executing SQL statements against a datasource. For most
file formats (e.g. Shapefiles, GeoJSON, FlatGeobuf files), the built-in
OGR SQL dialect will be used by default. It is also possible to request
the alternate `"SQLite"` dialect, which will use the SQLite engine to
evaluate commands on GDAL datasets. This assumes that GDAL is built with
support for SQLite, and preferably with Spatialite support too to
benefit from spatial functions.

## Note

All GDAL/OGR drivers for database systems, e.g., PostgreSQL / PostGIS,
Oracle Spatial, SQLite / Spatialite RDBMS, GeoPackage, etc., override
the `GDALDataset::ExecuteSQL()` function with a dedicated implementation
and, by default, pass the SQL statements directly to the underlying
RDBMS. In these cases the SQL syntax varies in some particulars from OGR
SQL. Also, anything possible in SQL can then be accomplished for these
particular databases. For those drivers, it is also possible to
explicitly request the `OGRSQL` or `SQLite` dialects, although
performance will generally be much less than the native SQL engine of
those database systems.

## See also

[`ogrinfo()`](https://firelab.github.io/gdalraster/reference/ogrinfo.md),
[`ogr_execute_sql()`](https://firelab.github.io/gdalraster/reference/ogr_manage.md)

OGR SQL dialect and SQLITE SQL dialect:  
<https://gdal.org/en/stable/user/ogr_sql_sqlite_dialect.html>

## Examples

``` r
has_spatialite()
#> [1] TRUE
```
