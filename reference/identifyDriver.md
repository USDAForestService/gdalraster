# Identify the GDAL driver that can open a dataset

`identifyDriver()` will try to identify the driver that can open the
passed file name by invoking the Identify method of each registered
GDALDriver in turn. The short name of the first driver that successfully
identifies the file name will be returned as a character string. If all
drivers fail then `NULL` is returned. Wrapper of
`GDALIdentifyDriverEx()` in the GDAL C API.

## Usage

``` r
identifyDriver(
  filename,
  raster = TRUE,
  vector = TRUE,
  allowed_drivers = NULL,
  file_list = NULL
)
```

## Arguments

- filename:

  Character string containing the name of the file to access. This may
  not refer to a physical file, but instead contain information for the
  driver on how to access a dataset (e.g., connection string, URL, etc.)

- raster:

  Logical value indicating whether to include raster format drivers in
  the search, `TRUE` by default. May be set to `FALSE` to include only
  vector drivers.

- vector:

  Logical value indicating whether to include vector format drivers in
  the search, `TRUE` by default. May be set to `FALSE` to include only
  raster drivers.

- allowed_drivers:

  Optional character vector of driver short names that must be
  considered. Set to `NULL` to consider all candidate drivers (the
  default).

- file_list:

  Optional character vector of filenames, including those that are
  auxiliary to the main filename (see Note). May contain the input
  `filename` but this is not required. Defaults to `NULL`.

## Value

A character string with the short name of the first driver that
successfully identifies the input file name, or `NULL` on failure.

## Note

In order to reduce the need for such searches to touch the file system
machinery of the operating system, it is possible to give an optional
list of files. This is the list of all files at the same level in the
file system as the target file, including the target file. The filenames
should not include any path components. If the target object does not
have filesystem semantics then the file list should be `NULL`.

At least one of the `raster` or `vector` arguments must be `TRUE`.

## See also

[`gdal_formats()`](https://firelab.github.io/gdalraster/reference/gdal_formats.md)

## Examples

``` r
src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")

identifyDriver(src) |> gdal_formats()
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
