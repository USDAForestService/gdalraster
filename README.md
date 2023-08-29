
<!-- README.md is generated from README.Rmd. Please edit that file -->

# gdalraster

<!-- badges: start -->

[![R-CMD-check](https://github.com/USDAForestService/gdalraster/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/USDAForestService/gdalraster/actions/workflows/R-CMD-check.yaml)
[![codecov](https://codecov.io/gh/ctoney/gdalraster/branch/main/graph/badge.svg?token=MXIOPZQ2IU)](https://app.codecov.io/gh/ctoney/gdalraster)
[![CRAN
status](https://www.r-pkg.org/badges/version/gdalraster)](https://CRAN.R-project.org/package=gdalraster)
[![cran
checks](https://badges.cranchecks.info/worst/gdalraster.svg)](https://cran.r-project.org/web/checks/check_results_gdalraster.html)
[![r-universe
status](https://usdaforestservice.r-universe.dev/badges/gdalraster)](https://usdaforestservice.r-universe.dev/gdalraster)
<!-- badges: end -->

## Overview

`gdalraster` is an R interface to the Raster API of the Geospatial Data
Abstraction Library ([GDAL](https://gdal.org/)). Calling signatures
resemble those of the native C, C++ and Python APIs provided by the GDAL
project.

Bindings to GDAL are implemented in class `GDALRaster` along with
several related stand-alone functions. These support:

  - manual creation of uninitialized raster datasets
  - creation from existing raster as template
  - read/set raster dataset parameters
  - low-level I/O
  - virtual raster (VRT) for virtual subsetting, resampling and kernel
    filtering
  - access to `gdalwarp` utility for reprojection
  - coordinate transformation
  - spatial reference convenience functions

Additional functionality includes:

  - class `RunningStats` calculates mean and variance in one pass, and
    tracks the min, max, sum, and count (i.e., summary statistics on a
    data stream). The input data values are not stored in memory, so
    this class can be used to compute statistics for very large data
    streams.
  - class `CmbTable` identifies and counts unique combinations of
    integer values using a hash table.
  - `combine()` overlays multiple rasters so that a unique ID is
    assigned to each unique combination of input values. Pixel counts
    for each unique combination are obtained, and combination IDs are
    optionally written to an output raster.
  - `calc()` evaluates an R expression for each pixel in a raster layer
    or stack of layers. Individual pixel coordinates are available as
    variables in the R expression, as either x/y in the raster projected
    coordinate system or inverse projected longitude/latitude.
  - `plot_raster()` displays raster data using base R graphics.

`gdalraster` may be suitable for applications that primarily need
low-level raster I/O or prefer a direct GDAL API. The additional
functionality is somewhat aimed at thematic data analysis but may have
other utility.

## Installation

``` r
# Install the released version from CRAN
install.packages("gdalraster")

# Or the development version from GitHub:
remotes::install_github("USDAForestService/gdalraster")
```

## Documentation

  - [Reference
    manual](https://usdaforestservice.github.io/gdalraster/reference/)
  - [Raster API
    tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html)
  - [Raster
    display](https://usdaforestservice.github.io/gdalraster/articles/raster-display.html)
  - [GDAL block
    caching](https://usdaforestservice.github.io/gdalraster/articles/gdal-block-cache.html)
