# GDAL Config Quick Reference

## Overview

Configuration options are essentially global variables the user can set.
They are used to alter the default behavior of certain raster format
drivers, and in some cases the GDAL core. A large number of
configuration options are available. An overall discussion along with
full list of available options and where they apply is in the GDAL
documentation at <https://gdal.org/en/stable/user/configoptions.html>.

This quick reference covers a small subset of configuration options that
may be useful in common scenarios, with links to topic-specific
documentation provided by the GDAL project. Options can be set from R
with
[`gdalraster::set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md).
Note that specific usage is context dependent. Passing `value = ""`
(empty string) will unset a value previously set by
[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md):

``` r
library(gdalraster)
#> GDAL 3.8.4 (released 2024-02-08), GEOS 3.12.1, PROJ 9.4.0

set_config_option("GDAL_NUM_THREADS", "ALL_CPUS")
# unset:
set_config_option("GDAL_NUM_THREADS", "")
```

## General options

GDAL doc:
<https://gdal.org/en/stable/user/configoptions.html#general-options>

**`GDAL_RASTERIO_RESAMPLING`**

The `$read()` method of a `GDALRaster` object will perform automatic
resampling if the specified output size (`out_xsize * out_ysize`) is
different than the size of the source region being read
(`xsize * ysize`). In that case, resampling can be configured to
override the default `NEAR` to one of `BILINEAR`, `CUBIC`,
`CUBICSPLINE`, `LANCZOS`, `AVERAGE`, `MODE`, `RMS`, or `GAUSS`:

``` r
# bilinear interpolation (2x2 neighborhood of pixels)
set_config_option("GDAL_RASTERIO_RESAMPLING", "BILINEAR")
```

**`CPL_TMPDIR`**

By default, temporary files are written into the current working
directory. This can be changed with:

``` r
set_config_option("CPL_TMPDIR", "<dirname>")  # tmpdir to use
```

## Performance and caching

GDAL doc:
<https://gdal.org/en/stable/user/configoptions.html#performance-and-caching>

**`GDAL_NUM_THREADS`**

Sets the number of worker threads to be used by GDAL operations that
support multithreading. This affects several different parts of GDAL
including multi-threaded compression for GeoTiff and SOZip, and
multithreaded computation during
[`warp()`](https://firelab.github.io/gdalraster/reference/warp.md) (see
topics below).

**`GDAL_CACHEMAX`**

The size limit of the block cache is set upon first use (first I/O).
Setting `GDAL_CACHEMAX` after that point will not resize the cache. It
is a per-session setting. If `GDAL_CACHEMAX` has not been set upon first
use of the cache, then the default cache size (`5%` of physical RAM)
will be in effect for the current session.
[`gdalraster::set_cache_max()`](https://firelab.github.io/gdalraster/reference/set_cache_max.md)
can be used to change the value programmatically during operation of the
program. See also [GDAL Block
Cache](https://firelab.github.io/gdalraster/articles/gdal-block-cache.html).

``` r
# set to a specific size in MB
set_config_option("GDAL_CACHEMAX", "800")

# or percent of physical RAM
set_config_option("GDAL_CACHEMAX", "10%")
```

**`GDAL_MAX_DATASET_POOL_SIZE`**

The default number of datasets that can be opened simultaneously by the
`GDALProxyPool` mechanism (used by VRT for example) is `100`. This can
be increased to get better random I/O performance with VRT mosaics made
of numerous underlying raster files. Note: on Linux systems, the number
of file handles that can be opened by a process is generally limited to
`1024`. This is currently clamped between `2` and `1000`. Also note that
[`gdalwarp` increases the pool size to
`450`](https://gdal.org/en/stable/drivers/raster/vrt.html#performance-considerations):

``` r
# default is 100
set_config_option("GDAL_MAX_DATASET_POOL_SIZE", "450")
```

**`PG_USE_COPY`**

This configures PostgreSQL/PostGIS to use `COPY` for inserting data
which is significantly faster than `INSERT`. This can increase
performance substantially when using
[`gdalraster::polygonize()`](https://firelab.github.io/gdalraster/reference/polygonize.md)
to write polygons to PostGIS vector. See also [GDAL configuration
options for
PostgreSQL](https://gdal.org/en/stable/drivers/vector/pg.html#configuration-options).

``` r
# use COPY for inserting to PostGIS
set_config_option("PG_USE_COPY", "YES")
```

**`SQLITE_USE_OGR_VFS`**

For the SQLite-based formats GeoPackage (.gpkg) and Spatialite
(.sqlite), setting `SQLITE_USE_OGR_VFS` enables extra buffering/caching
by the GDAL/OGR I/O layer and can speed up I/O. Be aware that no file
locking will occur if this option is activated, so concurrent edits may
lead to database corruption. This setting may increase performance
substantially when using
[`gdalraster::polygonize()`](https://firelab.github.io/gdalraster/reference/polygonize.md)
to write polygons to a vector layer in these formats. Additional
configuration and performance hints for SQLite databases are in the
driver documentation at:
<https://gdal.org/en/stable/drivers/vector/sqlite.html#configuration-options>.

``` r
# SQLite: GPKG (.gpkg) and Spatialite (.sqlite)
# enable extra buffering/caching by the GDAL/OGR I/O layer
set_config_option("SQLITE_USE_OGR_VFS", "YES")
```

**`OGR_SQLITE_JOURNAL`**

SQLite is a transactional DBMS. When many `INSERT` statements are
executed in close sequence, application code may group them into large
batches within transactions in order to get optimal performance. By
default, if no transaction is explicitly started, SQLite will autocommit
on every statement which will be slow.

The `OGR_SQLITE_JOURNAL` option configures operation of the rollback
journal that implements transactions in SQLite. The [SQLite
documentation](https://www.sqlite.org/pragma.html#pragma_journal_mode)
describes the default operation:

> The DELETE journaling mode is the normal behavior. In the DELETE mode,
> the rollback journal is deleted at the conclusion of each transaction.
> Indeed, the delete operation is the action that causes the transaction
> to commit.

The `DELETE` mode requires file system I/O so performance is degraded if
many `INSERT`s are autocommitted individually. Using `MEMORY` journaling
mode (or even `OFF`) can be much faster in this case:

> The MEMORY journaling mode stores the rollback journal in volatile
> RAM. This saves disk I/O but at the expense of database safety and
> integrity. If the application using SQLite crashes in the middle of a
> transaction when the MEMORY journaling mode is set, then the database
> file will very likely go corrupt.

See the SQLite documentation for all available [journal
modes](https://www.sqlite.org/pragma.html#pragma_journal_mode). This
setting also applies when using
[`gdalraster::polygonize()`](https://firelab.github.io/gdalraster/reference/polygonize.md)
to write polygons to a vector layer in GeoPackage (.gpkg) or Spatialite
(.sqlite) formats (see `SQLITE_USE_OGR_VFS` above).

``` r
# configure SQLite to store the rollback journal in RAM
set_config_option("OGR_SQLITE_JOURNAL", "MEMORY")
```

## Networking

GDAL doc:
<https://gdal.org/en/stable/user/configoptions.html#networking-options>

**`CPL_VSIL_USE_TEMP_FILE_FOR_RANDOM_WRITE`**

Whether to use a local temporary file to support random writes in
certain virtual file systems. The temporary file will be located in
`CPL_TMPDIR` (see above).

``` r
# YES|NO to use a temp file
set_config_option("CPL_VSIL_USE_TEMP_FILE_FOR_RANDOM_WRITE", "YES")
```

## PROJ

GDAL doc:
<https://gdal.org/en/stable/user/configoptions.html#proj-options>

**`OSR_DEFAULT_AXIS_MAPPING_STRATEGY`**

This option can be set to either `TRADITIONAL_GIS_ORDER` or
`AUTHORITY_COMPLIANT`. GDAL \>= 3.5 defaults to `AUTHORITY_COMPLIANT`.
Determines whether to honor the declared axis mapping of a CRS or
override it with the traditional GIS ordering (x = longitude, y =
latitude).

**`OSR_WKT_FORMAT`**

As of GDAL 3.0, the default format for exporting a spatial reference
definition to Well Known Text is WKT 1. This can be overridden with:

``` r
# SFSQL/WKT1_SIMPLE/WKT1/WKT1_GDAL/WKT1_ESRI/WKT2_2015/WKT2_2018/WKT2/DEFAULT
set_config_option("OSR_WKT_FORMAT", "WKT2")
```

## Warp

GDAL doc:
<https://gdal.org/en/stable/programs/gdalwarp.html#memory-usage>

The [performance and caching](#performance-and-caching) topic above
generally applies to processing with
[`gdalraster::warp()`](https://firelab.github.io/gdalraster/reference/warp.md)
(reproject/resample/crop/mosaic).

**`GDAL_NUM_THREADS`**

Multithreaded computation in
[`warp()`](https://firelab.github.io/gdalraster/reference/warp.md) can
be enabled with:

``` r
# note this also affects several other parts of GDAL
set_config_option("GDAL_NUM_THREADS", "4")  # number of threads or ALL_CPUS
```

Increasing the memory available to
[`warp()`](https://firelab.github.io/gdalraster/reference/warp.md) may
also increase performance (i.e., the options passed in `cl_arg` include
a value like `c("-wm", "1000")`). The warp memory specified by `"-wm"`
is shared among all threads. It is especially beneficial to increase
this value when running
[`warp()`](https://firelab.github.io/gdalraster/reference/warp.md) with
multithreading enabled.

Multithreading could also be enabled by including a GDAL warp option in
`cl_arg` with `c("-wo", "NUM_THREADS=<value>")` greater than 1, which is
equivalent to setting the `GDAL_NUM_THREADS` configuration option as
shown above.

This option can be combined with the [`-multi` command-line
argument](https://gdal.org/en/stable/programs/gdalwarp.html#cmdoption-gdalwarp-multi)
passed to
[`warp()`](https://firelab.github.io/gdalraster/reference/warp.md) in
`cl_arg`. With `-multi`, two threads will be used to process chunks of
the raster and perform input/output operation simultaneously, whereas
the `GDAL_NUM_THREADS` configuration option affects computation
separately.

**`GDAL_CACHEMAX`**

Increasing the size of the I/O block cache may also help. This can be
done by setting `GDAL_CACHEMAX` as described in the [performance and
caching](#performance-and-caching) topic above.

## GeoTIFF

GDAL doc:
<https://gdal.org/en/stable/drivers/raster/gtiff.html#configuration-options>

The behavior of the GTiff driver is highly configurable, including with
respect to overview creation. For full discussion, see the link above
and also the documentation for the
[`gdaladdo`](https://gdal.org/en/stable/programs/gdaladdo.html)
command-line utility.

**`GDAL_NUM_THREADS`**

The GTiff driver supports multi-threaded compression (default is
compression in the main thread). GDAL documentation states that it is
worth it for slow compression algorithms such as `DEFLATE` or `LZMA`.
Starting with GDAL 3.6, this option also enables multi-threaded decoding
when read requests intersect several tiles/strips:

``` r
# specify the number of worker threads or ALL_CPUS
# note this also affects several other parts of GDAL
set_config_option("GDAL_NUM_THREADS", "ALL_CPUS")
```

**`COMPRESS_OVERVIEW`**

Raster overviews (a.k.a. pyramids) can be built with the
`$buildOverviews()` method of a `GDALRaster` object. It may be desirable
to compress the overviews when building:

``` r
# applies to external overviews (.ovr), and internal overviews if GDAL >= 3.6
# LZW is a good default but several other compression algorithms are available
set_config_option("COMPRESS_OVERVIEW", "LZW")
```

**`PREDICTOR_OVERVIEW`**

Sets the predictor to use for overviews with `LZW`, `DEFLATE` and `ZSTD`
compression. The default is `1` (no predictor), `2` is horizontal
differencing and `3` is floating point prediction. `PREDICTOR=2` is only
supported for 8, 16, 32 and 64 bit samples (support for 64 bit was added
in libtiff \> 4.3.0). `PREDICTOR=3` is only supported for 16, 32 and 64
bit floating-point data.

``` r
# horizontal differencing
set_config_option("PREDICTOR_OVERVIEW", "2")
```

## HTTP/HTTPS

GDAL doc:
[/vsicurl/](https://gdal.org/en/stable/user/configoptions.html#networking-options)
(HTTP/HTTPS random access)

**`GDAL_HTTP_CONNECTTIMEOUT`**

Maximum delay for connection to be established before being aborted.

``` r
# max delay for connection establishment in seconds
set_config_option("GDAL_HTTP_CONNECTTIMEOUT", "<seconds>")
```

**`GDAL_HTTP_TIMEOUT`**

Maximum delay for the whole request to complete before being aborted.

``` r
# max delay for whole request completion in seconds
set_config_option("GDAL_HTTP_TIMEOUT", "<seconds>")
```

**`CPL_VSIL_CURL_CHUNK_SIZE`**

Partial downloads (requires the HTTP server to support random reading)
are done with a 16 KB granularity by default. The chunk size can be
configured with this option.

If the driver detects sequential reading, it will progressively increase
the chunk size up to 128 times `CPL_VSIL_CURL_CHUNK_SIZE` (so 2 MB by
default) to improve download performance. When increasing the value of
`CPL_VSIL_CURL_CHUNK_SIZE` to optimize sequential reading, it is
recommended to increase `CPL_VSIL_CURL_CACHE_SIZE` as well to 128 times
the value of `CPL_VSIL_CURL_CHUNK_SIZE`.

``` r
# chunk size in bytes
set_config_option("CPL_VSIL_CURL_CHUNK_SIZE", "<bytes>")
```

**`CPL_VSIL_CURL_CACHE_SIZE`**

A global least-recently-used cache of 16 MB shared among all downloaded
content is used, and content in it may be reused after a file handle has
been closed and reopen, during the life-time of the process or until
[`vsi_curl_clear_cache()`](https://firelab.github.io/gdalraster/reference/vsi_curl_clear_cache.md)
is called. The size of this global LRU cache can be modified with:

``` r
# size in bytes defaults to 16 MB
set_config_option("CPL_VSIL_CURL_CACHE_SIZE", "<bytes>")
```

## AWS S3 buckets

GDAL doc:
[/vsis3/](https://gdal.org/en/stable/user/virtual_file_systems.html#vsis3-aws-s3-files)
(AWS S3 file system handler)

**`AWS_NO_SIGN_REQUEST`**

Request signing can be disabled for public buckets that do not require
an AWS account:

``` r
# public bucket no AWS account required
set_config_option("AWS_NO_SIGN_REQUEST", "YES")
```

**`AWS_ACCESS_KEY_ID`**  
**`AWS_SECRET_ACCESS_KEY`** **`AWS_SESSION_TOKEN`**  
**`AWS_REQUEST_PAYER`**

If authentication is required, configure credentials with:

``` r
set_config_option("AWS_ACCESS_KEY_ID", "<value>")  # key ID
set_config_option("AWS_SECRET_ACCESS_KEY", "<value>")  # secret access key
# used for validation if using temporary credentials:
set_config_option("AWS_SESSION_TOKEN", "<value>")  # session token
# if requester pays:
set_config_option("AWS_REQUEST_PAYER", "<value>")  # requester
```

**`AWS_REGION`**

Sets the AWS region to which requests should be sent. Defaults to
`us-east-1`.

``` r
# specify region
set_config_option("AWS_REGION", "us-west-2")
```

## Google Cloud Storage

GDAL doc:
[/vsigs/](https://gdal.org/en/stable/user/virtual_file_systems.html#vsigs-google-cloud-storage-files)
(Google Cloud Storage files)

## Microsoft Azure Blob

GDAL doc:
[/vsiaz/](https://gdal.org/en/stable/user/virtual_file_systems.html#vsiaz-microsoft-azure-blob-files)
(Microsoft Azure Blob files)

Recognized filenames are of the form `/vsiaz/container/key`, where
`container` is the name of the container and `key` is the object “key”,
i.e. a filename potentially containing subdirectories.

**`AZURE_NO_SIGN_REQUEST`**

Controls whether requests are signed.

``` r
# public access
set_config_option("AZURE_NO_SIGN_REQUEST", "YES")
```

**`AZURE_STORAGE_CONNECTION_STRING`**

Credential string provided in the Access Keys section of the
administrative interface, containing both the account name and a secret
key.

``` r
set_config_option("AZURE_STORAGE_CONNECTION_STRING", "<my_connection_string>")
```

**`AZURE_STORAGE_ACCOUNT`**  
**`AZURE_STORAGE_ACCESS_TOKEN`**  
**`AZURE_STORAGE_ACCESS_KEY`**  
**`AZURE_STORAGE_SAS_TOKEN`**

Whereas an Azure connection string contains both the account name and
key, the storage account name might be set using `AZURE_STORAGE_ACCOUNT`
along with one of:

- `AZURE_STORAGE_ACCESS_TOKEN`: value obtained using Microsoft
  Authentication Library (MSAL)
- `AZURE_STORAGE_ACCESS_KEY`: value is the secret key associated with
  `AZURE_STORAGE_ACCOUNT`
- `AZURE_STORAGE_SAS_TOKEN`: value is a Shared Access Signature
- `AZURE_NO_SIGN_REQUEST=YES` to disable request signing

The `AZURE_STORAGE_SAS_TOKEN` is used, for example, with Microsoft
Planetary Computer as documented at:
<https://planetarycomputer.microsoft.com/docs/concepts/sas/>

SAS token can be requested via API with the `token` endpoint:
`https://planetarycomputer.microsoft.com/api/sas/v1/token/{collection_id}`
or
`https://planetarycomputer.microsoft.com/api/sas/v1/token/{storage_account}/{container}`

``` r
# e.g., Planetary Computer access to STAC items as geoparquet datasets
# https://planetarycomputer.microsoft.com/docs/quickstarts/stac-geoparquet/
set_config_option("AZURE_STORAGE_ACCOUNT", "pcstacitems")
# SAS token is the value of "token" in the JSON returned by:
# https://planetarycomputer.microsoft.com/api/sas/v1/token/pcstacitems/items
set_config_option("AZURE_STORAGE_SAS_TOKEN", "<token>")
```

Other authentication methods are possible for Azure. See the GDAL
documentation for details.

## Microsoft Azure Data Lake

GDAL doc:
[/vsiadls/](https://gdal.org/en/stable/user/virtual_file_systems.html#vsiadls-microsoft-azure-data-lake-storage-gen2)
(Microsoft Azure Data Lake Storage Gen2)

## SOZip

GDAL doc:
[/vsizip/](https://gdal.org/en/stable/user/virtual_file_systems.html#vsizip-zip-archives)
(Seek-Optimized ZIP files, GDAL \>= 3.7)

The function
[`gdalraster::addFilesInZip()`](https://firelab.github.io/gdalraster/reference/addFilesInZip.md)
can be used to create new or append to existing ZIP files, potentially
using the seek optimization extension. Function arguments are available
for the options below, or the configuration options can be set to change
the default behavior.

**`GDAL_NUM_THREADS`**

The `GDAL_NUM_THREADS` configuration option can be set to `ALL_CPUS` or
an integer value to specify the number of threads to use for
SOZip-compressed files. This option is similarly described above for
compression in [GeoTiff](#geotiff). Note that this option also affects
several other parts of GDAL.

**`CPL_SOZIP_ENABLED`**

Defaults to `AUTO`. Determines whether the SOZip optimization should be
enabled. If `AUTO`, SOZip will be enabled for uncompressed files larger
than `CPL_SOZIP_MIN_FILE_SIZE`.

``` r
# SOZip optimization defaults to AUTO
set_config_option("CPL_SOZIP_ENABLED", "YES")
```

**`CPL_SOZIP_MIN_FILE_SIZE`**

Defaults to `1M`. Determines the minimum file size for SOZip to be
automatically enabled. Specified in bytes, or `K`, `M` or `G` suffix can
be used respectively to specify a value in kilobytes, megabytes or
gigabytes.

``` r
# SOZip minimum file size
set_config_option("CPL_SOZIP_MIN_FILE_SIZE", "100K")
```
