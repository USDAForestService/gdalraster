# Display a GDAL Raster Attribute Table

`displayRAT()` generates a presentation table. Colors are shown if the
Raster Attribute Table contains RGB columns. This function requires
package `gt`.

## Usage

``` r
displayRAT(tbl, title = "Raster Attribute Table")
```

## Arguments

- tbl:

  A data frame formatted as a GDAL RAT (e.g., as returned by
  [`buildRAT()`](https://firelab.github.io/gdalraster/reference/buildRAT.md)
  or `GDALRaster$getDefaultRAT()`).

- title:

  Character string to be used in the table title.

## Value

An object of class `"gt_tbl"` (i.e., a table created with
[`gt::gt()`](https://gt.rstudio.com/reference/gt.html)).

## See also

[`buildRAT()`](https://firelab.github.io/gdalraster/reference/buildRAT.md),
[`GDALRaster$getDefaultRAT()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md)

[`vignette("raster-attribute-tables")`](https://firelab.github.io/gdalraster/articles/raster-attribute-tables.md)

## Examples

``` r
# see examples for `buildRAT()`
```
