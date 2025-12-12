# Build a GDAL Raster Attribute Table with VALUE, COUNT

`buildRAT()` reads all pixels of an input raster to obtain the set of
unique values and their counts. The result is returned as a data frame
suitable for use with the class method `GDALRaster$setDefaultRAT()`. The
returned data frame might be further modified before setting as a Raster
Attribute Table in a dataset, for example, by adding columns containing
class names, color values, or other information (see Details). An
optional input data frame containing such attributes may be given, in
which case `buildRAT()` will attempt to join the additional columns and
automatically assign the appropriate metadata on the output data frame
(i.e., assign R attributes on the data frame and its columns that define
usage in a GDAL Raster Attribute Table).

## Usage

``` r
buildRAT(
  raster,
  band = 1L,
  col_names = c("VALUE", "COUNT"),
  table_type = "athematic",
  na_value = NULL,
  join_df = NULL,
  quiet = FALSE
)
```

## Arguments

- raster:

  Either a `GDALRaster` object, or a character string containing the
  file name of a raster dataset to open.

- band:

  Integer scalar, band number to read (default `1L`).

- col_names:

  Character vector of length two containing names to use for column 1
  (pixel values) and column 2 (pixel counts) in the output data frame
  (defaults are `c("VALUE", "COUNT")`).

- table_type:

  Character string describing the type of the attribute table. One of
  either `"thematic"`, or `"athematic"` for continuous data (the
  default).

- na_value:

  Numeric scalar. If the set of unique pixel values has an `NA`, it will
  be recoded to `na_value` in the returned data frame. If `NULL` (the
  default), `NA` will not be recoded.

- join_df:

  Optional data frame for joining additional attributes. Must have a
  column of unique values with the same name as `col_names[1]`
  (`"VALUE"` by default).

- quiet:

  Logical scalar. If
  ``` TRUE``, a progress bar will not be displayed. Defaults to  ```FALSE“.

## Value

A data frame with at least two columns containing the set of unique
pixel values and their counts. These columns have attribute `"GFU"` set
to `"MinMax"` for the values, and `"PixelCount"` for the counts. If
`join_df` is given, the returned data frame will have additional columns
that result from [`merge()`](https://rdrr.io/r/base/merge.html). The
`"GFU"` attribute of the additional columns will be assigned
automatically based on the column names (*case-insensitive* matching,
see Details). The returned data frame has attribute `"GDALRATTableType"`
set to `table_type`.

## Details

A GDAL Raster Attribute Table (or RAT) provides attribute information
about pixel values. Raster attribute tables can be used to represent
histograms, color tables, and classification information. Each row in
the table applies to either a single pixel value or a range of values,
and might have attributes such as the histogram count for that value (or
range), the color that pixels of that value (or range) should be
displayed, names of classes, or various other information.

Each column in a raster attribute table has a name, a type (integer,
double, or string), and a `GDALRATFieldUsage`. The usage distinguishes
columns with particular understood purposes (such as color, histogram
count, class name), and columns that have other purposes not understood
by the library (long labels, ancillary attributes, etc).

In the general case, each row has a field indicating the minimum pixel
value falling into that category, and a field indicating the maximum
pixel value. In the GDAL API, these are indicated with usage values of
`GFU_Min` and `GFU_Max`. In the common case where each row is a discrete
pixel value, a single column with usage `GFU_MinMax` would be used
instead. In R, the table is represented as a data frame with column
attribute `"GFU"` containing the field usage as a string, e.g., `"Max"`,
`"Min"` or `"MinMax"` (case-sensitive). The full set of possible field
usage descriptors is:

|                |                  |                            |
|----------------|------------------|----------------------------|
| GFU attr       | GDAL enum        | Description                |
| `"Generic"`    | `GFU_Generic`    | General purpose field      |
| `"PixelCount"` | `GFU_PixelCount` | Histogram pixel count      |
| `"Name"`       | `GFU_Name`       | Class name                 |
| `"Min"`        | `GFU_Min`        | Class range minimum        |
| `"Max"`        | `GFU_Max`        | Class range maximum        |
| `"MinMax"`     | `GFU_MinMax`     | Class value (min=max)      |
| `"Red"`        | `GFU_Red`        | Red class color (0-255)    |
| `"Green"`      | `GFU_Green`      | Green class color (0-255)  |
| `"Blue"`       | `GFU_Blue`       | Blue class color (0-255)   |
| `"Alpha"`      | `GFU_Alpha`      | Alpha transparency (0-255) |
| `"RedMin"`     | `GFU_RedMin`     | Color range red minimum    |
| `"GreenMin"`   | `GFU_GreenMin`   | Color range green minimum  |
| `"BlueMin"`    | `GFU_BlueMin`    | Color range blue minimum   |
| `"AlphaMin"`   | `GFU_AlphaMin`   | Color range alpha minimum  |
| `"RedMax"`     | `GFU_RedMax`     | Color range red maximum    |
| `"GreenMax"`   | `GFU_GreenMax`   | Color range green maximum  |
| `"BlueMax"`    | `GFU_BlueMax`    | Color range blue maximum   |
| `"AlphaMax"`   | `GFU_AlphaMax`   | Color range alpha maximum  |

`buildRAT()` assigns GFU `"MinMax"` on the column of pixel values (named
`"VALUE"` by default) and GFU `"PixelCount"` on the column of counts
(named `"COUNT"` by default). If `join_df` is given, the additional
columns that result from joining will have GFU assigned automatically
based on the column names (*ignoring case*). First, the additional
column names are checked for containing the string `"name"` (e.g.,
`"classname"`, `"TypeName"`, `"EVT_NAME"`, etc). The first matching
column (if any) will be assigned a GFU of `"Name"` (=`GFU_Name`, the
field usage descriptor for class names). Next, columns named `"R"` or
`"Red"` will be assigned GFU `"Red"`, columns named `"G"` or `"Green"`
will be assigned GFU `"Green"`, columns named `"B"` or `"Blue"` will be
assigned GFU `"Blue"`, and columns named `"A"` or `"Alpha"` will be
assigned GFU `"Alpha"`. Finally, any remaining columns that have not
been assigned a GFU will be assigned `"Generic"`.

In a variation of RAT, all the categories are of equal size and
regularly spaced, and the categorization can be determined by knowing
the value at which the categories start and the size of a category. This
is called "Linear Binning" and the information is kept specially on the
raster attribute table as a whole. In R, a RAT that uses linear binning
would have the following attributes set on the data frame: attribute
`"Row0Min"` = the numeric lower bound (pixel value) of the first
category, and attribute `"BinSize"` = the numeric width of each category
(in pixel value units). `buildRAT()` does not create tables with linear
binning, but one could be created manually based on the specifications
above, and applied to a raster with the class method
`GDALRaster$setDefaultRAT()`.

A raster attribute table is thematic or athematic (continuous). In R,
this is defined by an attribute on the data frame named
`"GDALRATTableType"` with value of either `"thematic"` or `"athematic"`.

## Note

The full raster will be scanned.

If `na_value` is not specified, then an `NA` pixel value (if present)
will not be recoded in the output data frame. This may have implications
if joining to other data (`NA` will not match), or when using the
returned data frame to set a default RAT on a dataset (`NA` will be
interpreted as the value that R uses internally to represent it for the
type, e.g., -2147483648 for `NA_integer_`). In some cases, removing the
row in the output data frame with value `NA`, rather than recoding, may
be desirable (i.e., by removing manually or by side effect of joining
via [`merge()`](https://rdrr.io/r/base/merge.html), for example). Users
should consider what is appropriate for a particular case.

## See also

[`GDALRaster$getDefaultRAT()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`GDALRaster$setDefaultRAT()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`displayRAT()`](https://firelab.github.io/gdalraster/reference/displayRAT.md)

[`vignette("raster-attribute-tables")`](https://firelab.github.io/gdalraster/articles/raster-attribute-tables.md)

## Examples

``` r
evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
# make a copy to modify
f <- file.path(tempdir(), "storml_evt_tmp.tif")
file.copy(evt_file,  f)
#> [1] TRUE

ds <- new(GDALRaster, f, read_only=FALSE)
ds$getDefaultRAT(band = 1) # NULL
#> NULL

# get the full attribute table for LANDFIRE EVT from the CSV file
evt_csv <- system.file("extdata/LF20_EVT_220.csv", package="gdalraster")
evt_df <- read.csv(evt_csv)
nrow(evt_df)
#> [1] 860
head(evt_df)
#>   VALUE                                            EVT_NAME EVT_LF EVT_PHYS   R
#> 1 -9999                                         Fill-NoData   <NA>     <NA> 255
#> 2  7008                          North Pacific Oak Woodland   Tree Hardwood 203
#> 3  7009 Northwestern Great Plains Aspen Forest and Parkland   Tree Hardwood 192
#> 4  7010       Northern Rocky Mountain Western Larch Savanna   Tree  Conifer 180
#> 5  7011            Rocky Mountain Aspen Forest and Woodland   Tree Hardwood 192
#> 6  7012       Rocky Mountain Bigtooth Maple Ravine Woodland   Tree Hardwood 171
#>     G   B      RED GREEN     BLUE
#> 1 255 255 1.000000     1 1.000000
#> 2 255 171 0.796078     1 0.670588
#> 3 255 138 0.752941     1 0.541176
#> 4 255 148 0.705882     1 0.580392
#> 5 255 138 0.752941     1 0.541176
#> 6 255 138 0.670588     1 0.541176
evt_df <- evt_df[,1:7]

tbl <- buildRAT(ds,
                table_type = "thematic",
                na_value = -9999,
                join_df = evt_df)
#> scanning raster...
#> 0...10...20...30...40...50...60...70...80...90...100 - done.

nrow(tbl)
#> [1] 24
head(tbl)
#>   VALUE COUNT                                                          EVT_NAME
#> 1 -9999   876                                                       Fill-NoData
#> 2  7011    28                          Rocky Mountain Aspen Forest and Woodland
#> 3  7046  4564           Northern Rocky Mountain Subalpine Woodland and Parkland
#> 4  7050   570                              Rocky Mountain Lodgepole Pine Forest
#> 5  7055   889 Rocky Mountain Subalpine Dry-Mesic Spruce-Fir Forest and Woodland
#> 6  7056   304 Rocky Mountain Subalpine Mesic-Wet Spruce-Fir Forest and Woodland
#>   EVT_LF EVT_PHYS   R   G   B
#> 1   <NA>     <NA> 255 255 255
#> 2   Tree Hardwood 192 255 138
#> 3   Tree  Conifer 191 255 233
#> 4   Tree  Conifer 163 240 219
#> 5   Tree  Conifer 236 252 204
#> 6   Tree  Conifer 236 252 204

# attributes on the data frame and its columns define usage in a GDAL RAT
attributes(tbl)
#> $names
#> [1] "VALUE"    "COUNT"    "EVT_NAME" "EVT_LF"   "EVT_PHYS" "R"        "G"       
#> [8] "B"       
#> 
#> $row.names
#>  [1]  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
#> 
#> $class
#> [1] "data.frame"
#> 
#> $GDALRATTableType
#> [1] "thematic"
#> 
attributes(tbl$VALUE)
#> $GFU
#> [1] "MinMax"
#> 
attributes(tbl$COUNT)
#> $GFU
#> [1] "PixelCount"
#> 
attributes(tbl$EVT_NAME)
#> $GFU
#> [1] "Name"
#> 
attributes(tbl$EVT_LF)
#> $GFU
#> [1] "Generic"
#> 
attributes(tbl$EVT_PHYS)
#> $GFU
#> [1] "Generic"
#> 
attributes(tbl$R)
#> $GFU
#> [1] "Red"
#> 
attributes(tbl$G)
#> $GFU
#> [1] "Green"
#> 
attributes(tbl$B)
#> $GFU
#> [1] "Blue"
#> 

ds$setDefaultRAT(band = 1, tbl)
#> [1] TRUE
ds$flushCache()

tbl2 <- ds$getDefaultRAT(band = 1)
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
nrow(tbl2)
#> [1] 24
head(tbl2)
#>   VALUE COUNT                                                          EVT_NAME
#> 1 -9999   876                                                       Fill-NoData
#> 2  7011    28                          Rocky Mountain Aspen Forest and Woodland
#> 3  7046  4564           Northern Rocky Mountain Subalpine Woodland and Parkland
#> 4  7050   570                              Rocky Mountain Lodgepole Pine Forest
#> 5  7055   889 Rocky Mountain Subalpine Dry-Mesic Spruce-Fir Forest and Woodland
#> 6  7056   304 Rocky Mountain Subalpine Mesic-Wet Spruce-Fir Forest and Woodland
#>   EVT_LF EVT_PHYS   R   G   B
#> 1     NA       NA 255 255 255
#> 2   Tree Hardwood 192 255 138
#> 3   Tree  Conifer 191 255 233
#> 4   Tree  Conifer 163 240 219
#> 5   Tree  Conifer 236 252 204
#> 6   Tree  Conifer 236 252 204

ds$close()

# Display
evt_gt <- displayRAT(tbl2, title = "Storm Lake EVT Raster Attribute Table")
class(evt_gt)  # an object of class "gt_tbl" from package gt
#> [1] "gt_tbl" "list"  
# To show the table:
# evt_gt
# or simply call `displayRAT()` as above but without assignment
# `vignette("raster-attribute-tables")` has example output
```
