# Extract coordinate values from geometries

`g_coords()` extracts coordinate values (vertices) from the input
geometries and returns a data frame with coordinates as columns.

## Usage

``` r
g_coords(geom)
```

## Arguments

- geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings.

## Value

A data frame as returned by
[`wk::wk_coords()`](https://paleolimbot.github.io/wk/reference/wk_vertices.html):
columns `feature_id` (the index of the feature from the input),
`part_id` (an arbitrary integer identifying the point, line, or polygon
from whence it came), `ring_id` (an arbitrary integer identifying
individual rings within polygons), and one column per coordinate (`x`,
`y`, and/or `z` and/or `m`).

## Examples

``` r
dsn <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
lyr <- new(GDALVector, dsn)
d <- lyr$fetch(10)

vertices <- g_coords(d$geom)
head(vertices)
#>   feature_id part_id ring_id        x         y
#> 1          1       2       1 503099.4 -12893.97
#> 2          1       2       1 503169.8 -12756.37
#> 3          1       2       1 502689.8 -12131.53
#> 4          1       2       1 503098.4 -11311.95
#> 5          1       2       1 503796.8 -11250.05
#> 6          1       2       1 503950.2 -11631.99

lyr$close()
```
