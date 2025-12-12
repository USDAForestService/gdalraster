# GDAL DEM processing

`dem_proc()` generates DEM derivatives from an input elevation raster.
This function is a wrapper for the `gdaldem` command-line utility. See
<https://gdal.org/en/stable/programs/gdaldem.html> for details.

## Usage

``` r
dem_proc(
  mode,
  srcfile,
  dstfile,
  mode_options = DEFAULT_DEM_PROC[[mode]],
  color_file = NULL,
  quiet = FALSE
)
```

## Arguments

- mode:

  Character. Name of the DEM processing mode. One of hillshade, slope,
  aspect, color-relief, TRI, TPI or roughness.

- srcfile:

  Filename of the source elevation raster.

- dstfile:

  Filename of the output raster.

- mode_options:

  An optional character vector of command-line options (see
  [DEFAULT_DEM_PROC](https://firelab.github.io/gdalraster/reference/DEFAULT_DEM_PROC.md)
  for default values).

- color_file:

  Filename of a text file containing lines formatted as:
  "elevation_value red green blue". Only used when
  `mode = "color-relief"`.

- quiet:

  Logical scalar. If `TRUE`, a progress bar will not be displayed.
  Defaults to `FALSE`.

## Value

Logical indicating success (invisible `TRUE`). An error is raised if the
operation fails.

## Note

Band 1 of the source elevation raster is read by default, but this can
be changed by including a `-b` command-line argument in `mode_options`.
See the [documentation for
`gdaldem`](https://gdal.org/en/stable/programs/gdaldem.html) for a
description of all available options for each processing mode.

## Examples

``` r
## hillshade
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
out_file <- file.path(tempdir(), "storml_hillshade.tif")
dem_proc("hillshade", elev_file, out_file)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.

ds <- new(GDALRaster, out_file)
plot_raster(ds)


ds$close()
```
