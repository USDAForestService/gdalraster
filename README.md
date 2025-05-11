
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
[![OpenSSF Best
Practices](https://www.bestpractices.dev/projects/9382/badge)](https://www.bestpractices.dev/projects/9382)
[![OpenSSF
Scorecard](https://api.scorecard.dev/projects/github.com/USDAForestService/gdalraster/badge)](https://scorecard.dev/viewer/?uri=github.com/USDAForestService/gdalraster)
<!-- badges: end -->

## Overview

**gdalraster** is an R interface to the
[Raster](https://gdal.org/en/stable/api/raster_c_api.html) and
[Vector](https://gdal.org/en/stable/api/vector_c_api.html) APIs of the
Geospatial Data Abstraction Library
([GDAL](https://gdal.org/en/stable/)).

API-level bindings are implemented in the exposed C++ classes
[`GDALRaster`](https://usdaforestservice.github.io/gdalraster/reference/GDALRaster-class.html)
and
[`GDALVector`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.html),
along with several [stand-alone
functions](https://usdaforestservice.github.io/gdalraster/reference/index.html#stand-alone-functions).
Bindings to the GDAL Virtual Systems Interface
([VSI](https://gdal.org/en/stable/api/cpl.html#cpl-vsi-h)) are also
included to support file system operations and binary I/O on URLs, cloud
storage services, Zip/GZip/7z/RAR, and in-memory files, as well as
regular file systems. Calling signatures resemble the native C, C++ and
Python APIs provided by the GDAL project. Supports:

- manual creation of uninitialized raster and vector datasets
- vector layer creation and schema management
- read/set parameters and metadata for raster bands and vector layers
- low-level I/O
- build/read/set color tables and raster attribute tables
- virtual raster (VRT) for virtual cropping, resampling, kernel
  filtering, mosaicing
- [`gdalwarp`
  wrapper](https://usdaforestservice.github.io/gdalraster/reference/warp.html)
  for reproject/resample/crop/mosaic
- coordinate transformation
- spatial reference systems
- geometry operations on raw vectors of WKB, or WKT strings
- GDAL algorithms (`dem_proc()`, `polygonize()`, `rasterize()`,
  [`...`](https://usdaforestservice.github.io/gdalraster/reference/index.html#algorithms))
- OGR vector utilities (`ogrinfo()`, `ogr2ogr()`,
  [`ogr_manage`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.html)
  interface)
- OGR facilities for vector geoprocessing
  ([`ogr_proc()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_proc.html))
- raster and vector dataset management (copy files/move/rename/delete)
- create/append to Seek-Optimized ZIP
  ([SOZip](https://github.com/sozip/sozip-spec))
- abstraction of [file system
  operations](https://usdaforestservice.github.io/gdalraster/reference/index.html#virtual-file-systems)
  on URLs, cloud storage, in-memory files, etc.
- Standard C binary file I/O through VSI (class
  [`VSIFile`](https://usdaforestservice.github.io/gdalraster/reference/VSIFile-class.html))

Additional functionality includes:

- class
  [`RunningStats`](https://usdaforestservice.github.io/gdalraster/reference/RunningStats-class.html)
  calculates mean and variance in one pass, and tracks the min, max,
  sum, and count (i.e., summary statistics on a data stream). The input
  data values are not stored in memory, so this class can be used to
  compute statistics for very large data streams.
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
  evaluates an R expression for each pixel in a raster layer or stack of
  layers. Individual pixel coordinates are available as variables in the
  R expression, as either x/y in the raster projected coordinate system
  or inverse projected longitude/latitude.
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

GDAL \>= 3.1.0 built with GEOS is required, but a more recent version is
recommended.

On Ubuntu, recent versions of geospatial libraries can be installed from
the [ubuntugis-unstable
PPA](https://launchpad.net/~ubuntugis/+archive/ubuntu/ubuntugis-unstable)
with the following commands. Note that libxml2 is required for the R
package **xml2** which is a dependency, so we go ahead and install it
here as well:

    sudo add-apt-repository ppa:ubuntugis/ubuntugis-unstable
    sudo apt update
    sudo apt install libgdal-dev libgeos-dev libxml2-dev

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
install from source on Windows. RTools since version 4.2 includes GDAL
and all other dependent libraries that are needed to compile
**gdalraster**. Note that CRAN releases periodic revisions to RTools
that often include updates to the libraries as new versions become
available. [Release
6536](https://cran.r-project.org/bin/windows/Rtools/rtools45/rtools.html)
of RTools 4.5 contains GDAL 3.10.2, GEOS 3.13.1 and PROJ 9.5.1.

With RTools installed:

``` r
# Install the development version from GitHub
remotes::install_github("USDAForestService/gdalraster")
```

#### macOS

GDAL can be installed with Homebrew:

    brew install gdal

then

``` r
# Install the development version from GitHub
remotes::install_github("USDAForestService/gdalraster")
```

Caution is warranted on macOS with regard to mixing a source
installation with installation of binaries from CRAN. Consider
installing the development version from R-universe instead.

### From R-universe

[R-universe](https://usdaforestservice.r-universe.dev/gdalraster)
provides pre-compiled binary packages for Windows and macOS that track
the development version of **gdalraster**. New packages are built
usually within ~1 hour of the most recent commit in branch `main`.

``` r
# Install the development version from r-universe
install.packages("gdalraster", repos = c("https://usdaforestservice.r-universe.dev", "https://cran.r-project.org"))
```

## Documentation

- [Reference
  Manual](https://usdaforestservice.github.io/gdalraster/reference/)
- [Raster API
  Tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html)
- [Vector API
  Overview](https://usdaforestservice.github.io/gdalraster/articles/vector-api-overview.html)
- [Raster Attribute
  Tables](https://usdaforestservice.github.io/gdalraster/articles/raster-attribute-tables.html)
- [Raster
  Display](https://usdaforestservice.github.io/gdalraster/articles/raster-display.html)
- [GDAL Block
  Caching](https://usdaforestservice.github.io/gdalraster/articles/gdal-block-cache.html)
- [GDAL Config Quick
  Ref](https://usdaforestservice.github.io/gdalraster/articles/gdal-config-quick-ref.html)
