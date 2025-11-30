# Retrieve information on GDAL format drivers for raster and vector

`gdal_formats()` returns a table of the supported raster and vector
formats, with information about the capabilities of each format driver.

## Usage

``` r
gdal_formats(format = "")
```

## Arguments

- format:

  A character string containing a driver short name. By default,
  information for all configured raster and vector format drivers will
  be returned.

## Value

A data frame containing:

- the format short name

- file extension(s) if applicable (space-delimited string), or empty
  string (`""`)

- supports raster (logical)

- supports multidimensional raster (logical)

- supports vector (logical)

- supports geography network (logical)

- read/write flags (concatenated string, `ro` is read-only, `w` supports
  CreateCopy, `w+` supports Create, and `u` supports Update is reported
  if GDAL \>= 3.11)

- supports virtual I/O e.g. /vsimem/ (logical)

- supports subdatasets (logical)

- supported SQL dialects reported if GDAL \>= 3.6 (e.g.,
  `"NATIVE OGRSQL SQLITE"`)

- creation raster data types (e.g., `"Byte Int16 UInt16 Float32"` etc.)

- creation vector field types (e.g.,
  `"Integer Integer64 Real String Date DateTime Binary"` etc.)

- creation field sub-types (e.g., `"Boolean Int16 Float32 JSON"` etc.)

- supports multiple vector layers reported if GDAL \>= 3.4 (logical)

- supports reading field domains if GDAL \>= 3.5 (logical)

- creation field domain types if GDAL \>= 3.5 (supported values are
  `Coded`, `Range` and `Glob`)

## Note

Virtual I/O refers to operations on GDAL Virtual File Systems. See
<https://gdal.org/en/stable/user/virtual_file_systems.html#virtual-file-systems>.

## Examples

``` r
gdal_formats() |> str()
#> 'data.frame':    216 obs. of  17 variables:
#>  $ short_name             : chr  "VRT" "DERIVED" "GTiff" "COG" ...
#>  $ extensions             : chr  "vrt" "" "tif tiff" "tif tiff" ...
#>  $ raster                 : logi  TRUE TRUE TRUE TRUE TRUE TRUE ...
#>  $ multidim_raster        : logi  TRUE FALSE FALSE FALSE FALSE FALSE ...
#>  $ vector                 : logi  FALSE FALSE FALSE FALSE FALSE FALSE ...
#>  $ geography_network      : logi  FALSE FALSE FALSE FALSE FALSE FALSE ...
#>  $ rw_flag                : chr  "rw+" "ro" "rw+" "w" ...
#>  $ virtual_io             : logi  TRUE FALSE TRUE TRUE TRUE TRUE ...
#>  $ subdatasets            : logi  FALSE FALSE TRUE FALSE TRUE TRUE ...
#>  $ long_name              : chr  "Virtual Raster" "Derived datasets using VRT pixel functions" "GeoTIFF" "Cloud optimized GeoTIFF generator" ...
#>  $ sql_dialects           : chr  "" "" "" "" ...
#>  $ creation_datatypes     : chr  "Byte Int8 Int16 UInt16 Int32 UInt32 Int64 UInt64 Float32 Float64 CInt16 CInt32 CFloat32 CFloat64" "" "Byte Int8 UInt16 Int16 UInt32 Int32 Float32 Float64 CInt16 CInt32 CFloat32 CFloat64" "Byte Int8 UInt16 Int16 UInt32 Int32 UInt64 Int64 Float32 Float64 CInt16 CInt32 CFloat32 CFloat64" ...
#>  $ creation_field_types   : chr  "" "" "" "" ...
#>  $ creation_field_subtypes: chr  "" "" "" "" ...
#>  $ multiple_vec_layers    : logi  FALSE FALSE FALSE FALSE FALSE FALSE ...
#>  $ read_field_domains     : logi  FALSE FALSE FALSE FALSE FALSE FALSE ...
#>  $ creation_fld_dom_types : chr  "" "" "" "" ...

gdal_formats("GPKG")
#>   short_name    extensions raster multidim_raster vector geography_network
#> 1       GPKG gpkg gpkg.zip   TRUE           FALSE   TRUE             FALSE
#>   rw_flag virtual_io subdatasets  long_name         sql_dialects
#> 1     rw+       TRUE        TRUE GeoPackage NATIVE OGRSQL SQLITE
#>          creation_datatypes                               creation_field_types
#> 1 Byte Int16 UInt16 Float32 Integer Integer64 Real String Date DateTime Binary
#>      creation_field_subtypes multiple_vec_layers read_field_domains
#> 1 Boolean Int16 Float32 JSON                TRUE               TRUE
#>   creation_fld_dom_types
#> 1       Coded Range Glob
```
