# List of default nodata values by raster data type

These values are currently used in `gdalraster` when a nodata value is
needed but has not been specified:

        list("Byte" = 255, "Int8" = -128,
             "UInt16" = 65535, "Int16" = -32767,
             "UInt32" = 4294967293, "Int32" = -2147483647,
             "Float32" = -99999.0, "Float64" = -99999.0)

## Usage

``` r
DEFAULT_NODATA
```

## Format

An object of class `list` of length 8.
