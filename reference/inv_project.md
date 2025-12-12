# Inverse project geospatial x/y coordinates to longitude/latitude

`inv_project()` transforms geospatial x/y coordinates to
longitude/latitude in the same geographic coordinate system used by the
given projected spatial reference system. The output long/lat can
optionally be set to a specific geographic coordinate system by
specifying a well known name (see Details).

## Usage

``` r
inv_project(pts, srs, well_known_gcs = NULL)
```

## Arguments

- pts:

  A data frame or numeric matrix containing geospatial point
  coordinates, or point geometries as a list of WKB raw vectors or
  character vector of WKT strings. If data frame or matrix, the number
  of columns must be either two (x, y), three (x, y, z) or four (x, y,
  z, t). May be also be given as a numeric vector for one point (xy,
  xyz, or xyzt).

- srs:

  Character string specifying the projected spatial reference system for
  `pts`. May be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

- well_known_gcs:

  Optional character string containing a supported well known name of a
  geographic coordinate system (see Details for supported values).

## Value

Numeric matrix of longitude, latitude (potentially also with z, or z and
t columns).

## Details

By default, the geographic coordinate system of the projection specified
by `srs` will be used. If a specific geographic coordinate system is
desired, then `well_known_gcs` can be set to one of the values below:

|        |                                                       |
|--------|-------------------------------------------------------|
| EPSG:n | where n is the code of a geographic coordinate system |
| WGS84  | same as EPSG:4326                                     |
| WGS72  | same as EPSG:4322                                     |
| NAD83  | same as EPSG:4269                                     |
| NAD27  | same as EPSG:4267                                     |
| CRS84  | same as WGS84                                         |
| CRS72  | same as WGS72                                         |
| CRS27  | same as NAD27                                         |

The coordinates returned by `inv_project()`will always be in longitude,
latitude order (traditional GIS order) regardless of the axis order
defined for the GCS names above.

## Note

Input points that contain missing values (`NA`) will be assigned `NA` in
the output and a warning emitted. Input points that fail to transform
with the GDAL API call will also be assigned `NA` in the output with a
specific warning indicating that case.

## See also

[`transform_xy()`](https://firelab.github.io/gdalraster/reference/transform_xy.md)

## Examples

``` r
pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
# id, x, y in NAD83 / UTM zone 12N
pts <- read.csv(pt_file)
print(pts)
#>    id   xcoord  ycoord
#> 1   1 324650.9 5103344
#> 2   2 324171.0 5103034
#> 3   3 323533.4 5103329
#> 4   4 325220.0 5103508
#> 5   5 325703.1 5102377
#> 6   6 326297.8 5103924
#> 7   7 325520.4 5104146
#> 8   8 326247.7 5102506
#> 9   9 327711.7 5104476
#> 10 10 324181.7 5103901
inv_project(pts[,-1], "EPSG:26912")
#>            [,1]     [,2]
#>  [1,] -113.2671 46.06118
#>  [2,] -113.2732 46.05827
#>  [3,] -113.2815 46.06076
#>  [4,] -113.2598 46.06280
#>  [5,] -113.2531 46.05276
#>  [6,] -113.2460 46.06682
#>  [7,] -113.2561 46.06862
#>  [8,] -113.2461 46.05405
#>  [9,] -113.2279 46.07214
#> [10,] -113.2733 46.06607
```
