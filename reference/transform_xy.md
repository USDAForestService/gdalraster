# Transform geospatial x/y coordinates

`transform_xy()` transforms geospatial x, y coordinates to a new
projection. The input points may optionally have z vertices (x, y, z) or
time values (x, y, z, t). Wrapper for
`OGRCoordinateTransformation::Transform()` in the GDAL Spatial Reference
System C++ API.

## Usage

``` r
transform_xy(pts, srs_from, srs_to)
```

## Arguments

- pts:

  A data frame or numeric matrix containing geospatial point
  coordinates, or point geometries as a list of WKB raw vectors or
  character vector of WKT strings. If data frame or matrix, the number
  of columns must be either two (x, y), three (x, y, z) or four (x, y,
  z, t). May be also be given as a numeric vector for one point (xy,
  xyz, or xyzt).

- srs_from:

  Character string specifying the spatial reference system for `pts`.
  May be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

- srs_to:

  Character string specifying the output spatial reference system. May
  be in WKT format or any of the formats supported by
  [`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md).

## Value

Numeric matrix of geospatial (x, y) coordinates in the projection
specified by `srs_to` (potentially also with z, or z and t columns).

## Note

`transform_xy()` uses traditional GIS order for the input and output xy
(i.e., longitude/latitude ordered for geographic coordinates).

Input points that contain missing values (`NA`) will be assigned `NA` in
the output and a warning emitted. Input points that fail to transform
with the GDAL API call will also be assigned `NA` in the output with a
specific warning indicating that case.

## See also

[`srs_to_wkt()`](https://firelab.github.io/gdalraster/reference/srs_convert.md),
[`inv_project()`](https://firelab.github.io/gdalraster/reference/inv_project.md)

## Examples

``` r
pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
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
# id, x, y in NAD83 / UTM zone 12N
# transform to NAD83 / CONUS Albers
transform_xy(pts = pts[, -1], srs_from = "EPSG:26912", srs_to = "EPSG:5070")
#>           [,1]    [,2]
#>  [1,] -1330885 2684892
#>  [2,] -1331408 2684660
#>  [3,] -1331994 2685048
#>  [4,] -1330297 2684967
#>  [5,] -1329991 2683777
#>  [6,] -1329167 2685212
#>  [7,] -1329903 2685550
#>  [8,] -1329432 2683821
#>  [9,] -1327683 2685541
#> [10,] -1331265 2685514
```
