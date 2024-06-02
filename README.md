
<!-- README.md is generated from README.Rmd. Please edit that file -->

# gdalraster

<!-- badges: start -->

[![R-CMD-check](https://github.com/USDAForestService/gdalraster/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/USDAForestService/gdalraster/actions/workflows/R-CMD-check.yaml)
[![codecov](https://codecov.io/gh/ctoney/gdalraster/graph/badge.svg?token=MXIOPZQ2IU)](https://app.codecov.io/gh/ctoney/gdalraster)
[![R-hub](https://github.com/USDAForestService/gdalraster/actions/workflows/rhub.yaml/badge.svg)](https://github.com/USDAForestService/gdalraster/actions/workflows/rhub.yaml)
[![CRAN
status](https://www.r-pkg.org/badges/version/gdalraster)](https://CRAN.R-project.org/package=gdalraster)
[![r-universe
status](https://usdaforestservice.r-universe.dev/badges/gdalraster)](https://usdaforestservice.r-universe.dev/gdalraster)
<!-- badges: end -->

## Overview

**gdalraster** is an R interface to the [Raster
API](https://gdal.org/api/raster_c_api.html) of the Geospatial Data
Abstraction Library ([GDAL](https://gdal.org/)). Bindings to a subset of
the GDAL [Vector API](https://gdal.org/api/vector_c_api.html) are
included to provide utilities for managing vector data sources. Bindings
to the GDAL Virtual Systems Interface
([VSI](https://gdal.org/api/cpl.html#cpl-vsi-h)) support file system
operations and binary I/O on URLs, cloud storage services,
Zip/GZip/7z/RAR, and in-memory files, as well as regular file systems.
Calling signatures resemble the native C, C++ and Python APIs provided
by the GDAL project.

Bindings to GDAL are implemented in the exposed C++ class
[`GDALRaster`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.html)
along with several [stand-alone
functions](https://usdaforestservice.github.io/gdalraster/reference/index.html#stand-alone-functions),
supporting:

  - manual creation of uninitialized raster datasets
  - creation from existing raster as template
  - read/set raster dataset parameters and metadata
  - low-level I/O
  - build/read/set color tables and raster attribute tables
  - virtual raster (VRT) for virtual cropping, resampling, kernel
    filtering, mosaicing
  - `gdalwarp` wrapper for reproject/resample/crop/mosaic
  - coordinate transformation
  - spatial reference systems
  - GDAL algorithms (`dem_proc()`, `polygonize()`, `rasterize()`,
    [`...`](https://usdaforestservice.github.io/gdalraster/reference/index.html#algorithms))
  - OGR vector utilities (`ogrinfo()`, `ogr2ogr()`,
    [`ogr_manage`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.html)
    interface)
  - copy files/move/rename/delete raster and vector datasets
  - create/append to Seek-Optimized ZIP
    ([SOZip](https://github.com/sozip/sozip-spec))
  - abstraction of file system operations on URLs and cloud storage
  - Standard C binary file I/O through VSI (class
    [`VSIFile`](https://usdaforestservice.github.io/gdalraster/reference/VSIFile-class.html))

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

**gdalraster** may be useful in applications that need scalable,
low-level I/O, or prefer a direct GDAL API. Comprehensive
[documentation](#documentation) is provided in the package and online.

## Installation

Install the released version from CRAN with:

``` r
install.packages("gdalraster")
```

CRAN provides pre-compiled binary packages for Windows and macOS. These
do not require any separate installation of external libraries for GDAL.

### From source code

#### Linux

GDAL \>= 3.1.0 built with GEOS is required, but a more recent GDAL
version is recommended, e.g., \>= 3.6.4. GDAL as of version 3.9 requires
PROJ \>= 6.3.1, but a more recent version of PROJ is also recommended.
PROJ requires sqlite3, and libxml2 is required for the imported R
package **xml2**.

On Ubuntu, recent versions of geospatial libraries can be installed from
the [ubuntugis-unstable
PPA](https://launchpad.net/~ubuntugis/+archive/ubuntu/ubuntugis-unstable)
with the following commands:

    sudo add-apt-repository ppa:ubuntugis/ubuntugis-unstable
    sudo apt update
    sudo apt install libgdal-dev libgeos-dev libproj-dev libsqlite3-dev libxml2-dev

The versions in ubuntugis-unstable generally work well and are more
up-to-date, but less recent versions in the [ubuntugis-stable
PPA](https://launchpad.net/~ubuntugis/+archive/ubuntu/ppa) could be used
instead.

Package **sf** provides helpful
[instructions](https://github.com/r-spatial/sf#linux) for installing the
geospatial libraries on other Linux distributions.

With the dependent libraries available on the system, install from CRAN:

``` r
install.packages("gdalraster")
```

Or install the development version from GitHub using package
[remotes](https://remotes.r-lib.org/):

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
[release 6104](https://cran.r-project.org/bin/windows/Rtools/rtools44/rtools.html)
of RTools 4.4 contains GDAL 3.8.4 and PROJ 9.3.1.

With RTools installed:

``` r
# Install the development version from GitHub
remotes::install_github("USDAForestService/gdalraster")
```

#### macOS

GDAL and PROJ can be installed with Homebrew:

    brew install pkg-config gdal proj

Then `configure.args` may be needed:

``` r
# Install the development version from GitHub
remotes::install_github("USDAForestService/gdalraster", configure.args = "--with-proj-lib=$(brew --prefix)/lib/")
```

Caution seems warranted on macOS with regard to mixing a source
installation with installation of binaries from CRAN.

### From R-universe

[R-universe](https://usdaforestservice.r-universe.dev/gdalraster)
provides pre-compiled binary packages for Windows and macOS that track
the development version of **gdalraster**. New packages are built
usually within \~1 hour of the most recent commit.

``` r
# Install the development version from r-universe
install.packages("gdalraster", repos = c("https://usdaforestservice.r-universe.dev", "https://cran.r-project.org"))
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
