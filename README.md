
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

**gdalraster** is an R interface to the Raster API of the Geospatial
Data Abstraction Library ([GDAL](https://gdal.org/)). Calling signatures
resemble those of the native C, C++ and Python APIs provided by the GDAL
project.

Bindings to GDAL are implemented in the exposed C++ class
[`GDALRaster`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.html)
along with several [stand-alone
functions](https://usdaforestservice.github.io/gdalraster/reference/index.html#stand-alone-functions),
supporting:

  - manual creation of uninitialized raster datasets
  - creation from existing raster as template
  - read/set raster dataset parameters
  - low-level I/O
  - read/set color tables and raster attribute tables
  - copy files/move/rename/delete datasets
  - virtual raster (VRT) for virtual cropping, resampling, kernel
    filtering, mosaicing
  - `gdalwarp` wrapper for reproject/resample/crop/mosaic
  - coordinate transformation
  - spatial reference convenience functions
  - several GDAL algorithms (`dem_proc()`, `polygonize()`,
    `rasterize()`,
    [`...`](https://usdaforestservice.github.io/gdalraster/reference/gdalraster-package.html#details))

Additional functionality includes:

  - class
    [`RunningStats`](https://usdaforestservice.github.io/gdalraster/reference/RunningStats-class.html)
    calculates mean and variance in one pass, and tracks the min, max,
    sum, and count (i.e., summary statistics on a data stream). The
    input data values are not stored in memory, so this class can be
    used to compute statistics for very large data streams.
  - class
    [`CmbTable`](https://usdaforestservice.github.io/gdalraster/reference/CmbTable-class.html)
    identifies and counts unique combinations of integer values using a
    hash table.
  - [`combine()`](https://usdaforestservice.github.io/gdalraster/reference/combine.html)
    overlays multiple rasters so that a unique ID is assigned to each
    unique combination of input values. Pixel counts for each unique
    combination are obtained, and combination IDs are optionally written
    to an output raster.
  - [`calc()`](https://usdaforestservice.github.io/gdalraster/reference/calc.html)
    evaluates an R expression for each pixel in a raster layer or stack
    of layers. Individual pixel coordinates are available as variables
    in the R expression, as either x/y in the raster projected
    coordinate system or inverse projected longitude/latitude.
  - [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.html)
    displays raster data using base R graphics.

**gdalraster** should be useful in applications that need low-level
raster I/O or prefer a direct GDAL API. The additional functionality is
somewhat aimed at thematic data analysis but may have other utility.
Comprehensive [documentation](#documentation) is provided in the package
and online.

## Installation

Install the released version from CRAN with:

``` r
install.packages("gdalraster")
```

CRAN provides pre-compiled binary packages for Windows and macOS. These
do not require any separate installation of external libraries for GDAL
and PROJ.

### From source code

#### Linux

GDAL (\>= 2.4.0, built against GEOS), PROJ (\>= 4.8.0), and sqlite3 are
required.

On Ubuntu, recent versions of geospatial libraries can be installed from
the [ubuntugis-unstable
PPA](https://launchpad.net/~ubuntugis/+archive/ubuntu/ubuntugis-unstable)
with the following commands:

    sudo add-apt-repository ppa:ubuntugis/ubuntugis-unstable
    sudo apt update
    sudo apt install libgdal-dev libgeos-dev libproj-dev libsqlite3-dev

The releases in ubuntugis-unstable generally work well and are more
up-to-date, but less recent versions in the [ubuntugis-stable
PPA](https://launchpad.net/~ubuntugis/+archive/ubuntu/ppa) could be used
instead.

Package **sf** provides helpful
[instructions](https://github.com/r-spatial/sf#linux) for installing the
geospatial libraries on other Linux distributions.

With the dependent libraries available on the system, install
**gdalraster** from CRAN:

``` r
install.packages("gdalraster")
```

Or install the development version from GitHub using package
[remotes](https://CRAN.R-project.org/package=remotes):

``` r
remotes::install_github("USDAForestService/gdalraster")
```

#### Windows

[RTools](https://cran.r-project.org/bin/windows/Rtools/) is needed to
install from source on Windows. RTools since version 4.2 includes GDAL,
PROJ and all other dependent libraries that are needed to compile
**gdalraster**. Note that CRAN releases periodic revisions to RTools
that often include updates to the libraries as new versions become
available. For example,
[revision 5863](https://cran.r-project.org/bin/windows/Rtools/rtools43/news.html)
of RTools 4.3 contains GDAL 3.7.2 and PROJ 9.3.0.

With RTools installed:

``` r
# Install the development version from GitHub
remotes::install_github("USDAForestService/gdalraster")
```

#### macOS

GDAL and PROJ can be installed with Homebrew:

    brew install pkg-config gdal proj

Then `configure.args` is needed:

``` r
# Install the development version from GitHub
remotes::install_github("USDAForestService/gdalraster", configure.args = "--with-proj-lib=$(brew --prefix)/lib/")
```

## Documentation

  - [Reference
    Manual](https://usdaforestservice.github.io/gdalraster/reference/)
  - [Raster API
    Tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html)
  - [Raster Attribute
    Tables](https://usdaforestservice.github.io/gdalraster/articles/raster-attribute-tables.html)
  - [Raster
    Display](https://usdaforestservice.github.io/gdalraster/articles/raster-display.html)
  - [GDAL Block
    Caching](https://usdaforestservice.github.io/gdalraster/articles/gdal-block-cache.html)
  - [GDAL Config Quick
    Ref](https://usdaforestservice.github.io/gdalraster/articles/gdal-config-quick-ref.html)
