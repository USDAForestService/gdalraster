# Return the list of creation options for a GDAL driver

`getCreationOptions()` returns the list of creation options supported by
a GDAL format driver. This function is a wrapper of
`GDALGetDriverCreationOptionList()` in the GDAL API, parsing its XML
output into a named list.

## Usage

``` r
getCreationOptions(format, filter = NULL)
```

## Arguments

- format:

  Format short name (e.g., `"GTiff"`).

- filter:

  Optional character vector of creation option names.

## Value

A named list with names matching the creation option names, and each
element a named list with elements `type`, `description`, `default` and
`values` (see Details).

## Details

The output is a nested list with names matching the creation option
names. The information for each creation option is a named list with the
following elements:

- `type`: a character string describing the data type, e.g., `"int"`,
  `"float"`, `"string"`. The type `"string-select"` denotes a list of
  allowed string values which are returned as a character vector in the
  `values` element (see below).

- `description`: a character string describing the option, or `NA` if no
  description is provided by the GDAL driver.

- `default`: the default value of the option as either a character
  string or numeric value, or `NA` if no description is provided by the
  GDAL driver.

- `values`: a character vector of allowed string values for the creation
  option if `type` is `"string-select"`, otherwise `NULL` if the option
  is not a `"string-select"` type.

- `min`: (GDAL \>= 3.11) the minimum value of the valid range for the
  option, or `NA` if not provided by the GDAL driver or the option is
  not a numeric type.

- `max`: (GDAL \>= 3.11) the maximum value of the valid range for the
  option, or `NA` if not provided by the GDAL driver or the option is
  not a numeric type.

## See also

[`create()`](https://firelab.github.io/gdalraster/reference/create.md),
[`createCopy()`](https://firelab.github.io/gdalraster/reference/createCopy.md),
[`translate()`](https://firelab.github.io/gdalraster/reference/translate.md),
[`validateCreationOptions()`](https://firelab.github.io/gdalraster/reference/validateCreationOptions.md),
[`warp()`](https://firelab.github.io/gdalraster/reference/warp.md)

## Examples

``` r
opt <- getCreationOptions("GTiff", "COMPRESS")
names(opt)
#> [1] "COMPRESS"

(opt$COMPRESS$type == "string-select")  # TRUE
#> [1] TRUE
opt$COMPRESS$values
#>  [1] "NONE"         "LZW"          "PACKBITS"     "JPEG"         "CCITTRLE"    
#>  [6] "CCITTFAX3"    "CCITTFAX4"    "DEFLATE"      "LZMA"         "ZSTD"        
#> [11] "WEBP"         "LERC"         "LERC_DEFLATE" "LERC_ZSTD"   

all_opt <- getCreationOptions("GTiff")
names(all_opt)
#>  [1] "COMPRESS"                       "PREDICTOR"                     
#>  [3] "DISCARD_LSB"                    "JPEG_QUALITY"                  
#>  [5] "JPEGTABLESMODE"                 "ZLEVEL"                        
#>  [7] "LZMA_PRESET"                    "ZSTD_LEVEL"                    
#>  [9] "MAX_Z_ERROR"                    "MAX_Z_ERROR_OVERVIEW"          
#> [11] "WEBP_LOSSLESS"                  "WEBP_LEVEL"                    
#> [13] "NUM_THREADS"                    "NBITS"                         
#> [15] "INTERLEAVE"                     "TILED"                         
#> [17] "TFW"                            "RPB"                           
#> [19] "RPCTXT"                         "BLOCKXSIZE"                    
#> [21] "BLOCKYSIZE"                     "PHOTOMETRIC"                   
#> [23] "SPARSE_OK"                      "ALPHA"                         
#> [25] "PROFILE"                        "PIXELTYPE"                     
#> [27] "BIGTIFF"                        "ENDIANNESS"                    
#> [29] "COPY_SRC_OVERVIEWS"             "SOURCE_ICC_PROFILE"            
#> [31] "SOURCE_PRIMARIES_RED"           "SOURCE_PRIMARIES_GREEN"        
#> [33] "SOURCE_PRIMARIES_BLUE"          "SOURCE_WHITEPOINT"             
#> [35] "TIFFTAG_TRANSFERFUNCTION_RED"   "TIFFTAG_TRANSFERFUNCTION_GREEN"
#> [37] "TIFFTAG_TRANSFERFUNCTION_BLUE"  "TIFFTAG_TRANSFERRANGE_BLACK"   
#> [39] "TIFFTAG_TRANSFERRANGE_WHITE"    "STREAMABLE_OUTPUT"             
#> [41] "GEOTIFF_KEYS_FLAVOR"            "GEOTIFF_VERSION"               

# $description and $default will be NA if no value is provided by the driver
# $values will be NULL if the option is not a 'string-select' type

all_opt$PREDICTOR
#> $type
#> [1] "int"
#> 
#> $description
#> [1] "Predictor Type (1=default, 2=horizontal differencing, 3=floating point prediction)"
#> 
#> $default
#> [1] NA
#> 
#> $values
#> NULL
#> 

all_opt$BIGTIFF
#> $type
#> [1] "string-select"
#> 
#> $description
#> [1] "Force creation of BigTIFF file"
#> 
#> $default
#> [1] NA
#> 
#> $values
#> [1] "YES"       "NO"        "IF_NEEDED" "IF_SAFER" 
#> 
```
