# Helper functions for GDAL raster data types

These are convenience functions that return information about a raster
data type, return the smallest data type that can fully express two
input data types, or find the smallest data type able to support
specified requirements.

## Usage

``` r
dt_size(dt, as_bytes = TRUE)

dt_is_complex(dt)

dt_is_integer(dt)

dt_is_floating(dt)

dt_is_signed(dt)

dt_union(dt, dt_other)

dt_union_with_value(dt, value, is_complex = FALSE)

dt_find(bits, is_signed, is_floating, is_complex = FALSE)

dt_find_for_value(value, is_complex = FALSE)
```

## Arguments

- dt:

  Character string containing a GDAL data type name (e.g., `"Byte"`,
  `"Int16"`, `"UInt16"`, `"Int32"`, `"UInt32"`, `"Float32"`,
  `"Float64"`, etc.)

- as_bytes:

  Logical value, `TRUE` to return data type size in bytes (the default),
  `FALSE` to return the size in bits.

- dt_other:

  Character string containing a GDAL data type name.

- value:

  Numeric value for which to find a data type (passing the real part if
  `is_complex = TRUE`).

- is_complex:

  Logical value, `TRUE` if `value` is complex (default is `FALSE`), or
  if complex values are necessary in `dt_find()`.

- bits:

  Integer value specifying the number of bits necessary.

- is_signed:

  Logical value, `TRUE` if negative values are necessary.

- is_floating:

  Logical value, `TRUE` if non-integer values are necessary.

## Details

`dt_size()` returns the data type size in bytes by default, optionally
in bits (returns zero if `dt` is not recognized).

`dt_is_complex()` returns `TRUE` if the passed type is complex (one of
CInt16, CInt32, CFloat32 or CFloat64), i.e., if it consists of a real
and imaginary component.

`dt_is_integer()` returns `TRUE` if the passed type is integer (one of
Byte, Int16, UInt16, Int32, UInt32, CInt16, CInt32).

`dt_is_floating()` returns `TRUE` if the passed type is floating (one of
Float32, Float16, Float64, CFloat16, CFloat32, CFloat64).

`dt_is_signed()` returns `TRUE` if the passed type is signed.

`dt_union()` returns the smallest data type that can fully express both
input data types (returns a data type name as character string).

`dt_union_with_value()` unions a data type with the data type found for
a given value, and returns the resulting data type name as character
string.

`dt_find()` finds the smallest data type able to support the given
requirements (returns a data type name as character string).

`dt_find_for_value()` finds the smallest data type able to support the
given `value` (returns a data type name as character string).

## See also

[`GDALRaster$getDataTypeName()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md)

## Examples

``` r
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, elev_file)

ds$getDataTypeName(band = 1) |> dt_size()
#> [1] 2
ds$getDataTypeName(band = 1) |> dt_size(as_bytes = FALSE)
#> [1] 16
ds$getDataTypeName(band = 1) |> dt_is_complex()
#> [1] FALSE
ds$getDataTypeName(band = 1) |> dt_is_integer()
#> [1] TRUE
ds$getDataTypeName(band = 1) |> dt_is_floating()
#> [1] FALSE
ds$getDataTypeName(band = 1) |> dt_is_signed()
#> [1] TRUE

ds$close()

f <- system.file("extdata/complex.tif", package="gdalraster")
ds <- new(GDALRaster, f)

ds$getDataTypeName(band = 1) |> dt_size()
#> [1] 8
ds$getDataTypeName(band = 1) |> dt_size(as_bytes = FALSE)
#> [1] 64
ds$getDataTypeName(band = 1) |> dt_is_complex()
#> [1] TRUE
ds$getDataTypeName(band = 1) |> dt_is_integer()
#> [1] FALSE
ds$getDataTypeName(band = 1) |> dt_is_floating()
#> [1] TRUE
ds$getDataTypeName(band = 1) |> dt_is_signed()
#> [1] TRUE

ds$close()

dt_union("Byte", "Int16")
#> [1] "Int16"
dt_union_with_value("Byte", -1)
#> [1] "Int16"
dt_union_with_value("Byte", 256)
#> [1] "UInt16"

dt_find(bits = 32, is_signed = FALSE, is_floating = FALSE)
#> [1] "UInt32"
dt_find_for_value(0)
#> [1] "Byte"
dt_find_for_value(-1)
#> [1] "Int8"
dt_find_for_value(NaN)
#> [1] "Float64"
dt_find_for_value(.Machine$integer.max)
#> [1] "UInt32"
```
