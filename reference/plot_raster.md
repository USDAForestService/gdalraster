# Display raster data

`plot_raster()` displays raster data using base `graphics`.

## Usage

``` r
plot_raster(
  data,
  xsize = NULL,
  ysize = NULL,
  nbands = NULL,
  max_pixels = 2.5e+07,
  col_tbl = NULL,
  maxColorValue = 1,
  normalize = TRUE,
  minmax_def = NULL,
  minmax_pct_cut = NULL,
  col_map_fn = NULL,
  pixel_fn = NULL,
  xlim = NULL,
  ylim = NULL,
  interpolate = TRUE,
  asp = 1,
  axes = TRUE,
  main = "",
  xlab = "x",
  ylab = "y",
  xaxs = "i",
  yaxs = "i",
  legend = FALSE,
  digits = 2,
  na_col = rgb(0, 0, 0, 0),
  ...
)
```

## Arguments

- data:

  Either a `GDALRaster` object from which data will be read, or a
  numeric vector of pixel values arranged in left to right, top to
  bottom order, or a list of band vectors. If input is vector or list,
  the information in attribute `gis` will be used if present (see
  [`read_ds()`](https://firelab.github.io/gdalraster/reference/read_ds.md)),
  potentially ignoring values below for `xsize`, `ysize`, `nbands`.

- xsize:

  The number of pixels along the x dimension in `data`. If `data` is a
  `GDALRaster` object, specifies the size at which the raster will be
  read (used for argument `out_xsize` in `GDALRaster$read()`). By
  default, the entire raster will be read at full resolution.

- ysize:

  The number of pixels along the y dimension in `data`. If `data` is a
  `GDALRaster` object, specifies the size at which the raster will be
  read (used for argument `out_ysize` in `GDALRaster$read()`). By
  default, the entire raster will be read at full resolution.

- nbands:

  The number of bands in `data`. Must be either 1 (grayscale) or 3
  (RGB). For RGB, `data` are interleaved by band. If `nbands` is `NULL`
  (the default), then `nbands = 3` is assumed if the input data contain
  3 bands, otherwise band 1 is used.

- max_pixels:

  The maximum number of pixels that the function will attempt to display
  (per band). An error is raised if `(xsize * ysize)` exceeds this
  value. Setting to `NULL` turns off this check.

- col_tbl:

  A color table as a matrix or data frame with four or five columns.
  Column 1 contains the numeric pixel values. Columns 2:4 contain the
  intensities of the red, green and blue primaries (`0:1` by default, or
  use integer `0:255` by setting `maxColorValue = 255`). An optional
  column 5 may contain alpha transparency values, `0` for fully
  transparent to `1` (or `maxColorValue`) for opaque (the default if
  column 5 is missing). If `data` is a `GDALRaster` object, a built-in
  color table will be used automatically if one exists in the dataset.

- maxColorValue:

  A number giving the maximum of the color values range in `col_tbl`
  (see above). The default is `1`.

- normalize:

  Logical. `TRUE` to rescale pixel values so that their range is
  `[0,1]`, normalized to the full range of the pixel data by default
  (`min(data)`, `max(data)`, per band). Ignored if `col_tbl` is used.
  Set `normalize` to `FALSE` if a color map function is used that
  operates on raw pixel values (see `col_map_fn` below).

- minmax_def:

  Normalize to user-defined min/max values (in terms of the pixel data,
  per band). For single-band grayscale, a numeric vector of length two
  containing min, max. For 3-band RGB, a numeric vector of length six
  containing b1_min, b2_min, b3_min, b1_max, b2_max, b3_max.

- minmax_pct_cut:

  Normalize to a truncated range of the pixel data using percentile
  cutoffs (removes outliers). A numeric vector of length two giving the
  percentiles to use (e.g., `c(2, 98)`). Applied per band. Ignored if
  `minmax_def` is used.

- col_map_fn:

  An optional color map function that maps input pixel values to colors
  in "#RRGGBB" (or "#RRGGBBAA") character form (default is
  [`grDevices::gray`](https://rdrr.io/r/grDevices/gray.html) for
  single-band data or
  [`grDevices::rgb`](https://rdrr.io/r/grDevices/rgb.html) for 3-band).
  Ignored if `col_tbl` is used. Set `normalize = FALSE` if using a color
  map function that operates on raw pixel values. This argument can also
  be given as a color palette, in which case a color-ramp function is
  assumed. The color palette can be a character vector of `"#RRGGBB"` or
  `"#RRGGBBAA"`, color names from
  [`grDevices::colors()`](https://rdrr.io/r/grDevices/colors.html), or a
  positive integer that indexes into
  [`grDevices::palette()`](https://rdrr.io/r/grDevices/palette.html)
  (i.e., must be a valid argument to
  [`grDevices::col2rgb()`](https://rdrr.io/r/grDevices/col2rgb.html)).

- pixel_fn:

  An optional function that will be applied to the input pixel data.
  Must accept vector input and return a numeric vector of the same
  length as its input.

- xlim:

  Numeric vector of length two giving the x coordinate range. If `data`
  is a `GDALRaster` object, the default is the raster xmin, xmax in
  georeferenced coordinates, otherwise the default uses pixel/line
  coordinates (`c(0, xsize)`).

- ylim:

  Numeric vector of length two giving the y coordinate range. If `data`
  is a `GDALRaster` object, the default is the raster ymin, ymax in
  georeferenced coordinates, otherwise the default uses pixel/line
  coordinates (`c(ysize, 0)`).

- interpolate:

  Logical indicating whether to apply linear interpolation to the image
  when drawing (default `TRUE`).

- asp:

  Numeric. The aspect ratio y/x (see
  [`?plot.window`](https://rdrr.io/r/graphics/plot.window.html)).

- axes:

  Logical. `TRUE` to draw axes (the default).

- main:

  The main title (on top).

- xlab:

  Title for the x axis (see
  [`?title`](https://rdrr.io/r/graphics/title.html)).

- ylab:

  Title for the y axis (see
  [`?title`](https://rdrr.io/r/graphics/title.html)).

- xaxs:

  The style of axis interval calculation to be used for the x axis (see
  [`?par`](https://rdrr.io/r/graphics/par.html)).

- yaxs:

  The style of axis interval calculation to be used for the y axis (see
  [`?par`](https://rdrr.io/r/graphics/par.html)).

- legend:

  Logical indicating whether to include a legend on the plot. Currently,
  legends are only supported for continuous data. A color table will be
  used if one is specified or the raster has a built-in color table,
  otherwise the value for `col_map_fn` will be used.

- digits:

  The number of digits to display after the decimal point in the legend
  labels when raster data are floating point.

- na_col:

  Color to use for `NA` as a 7- or 9-character hexadecimal code. The
  default is transparent (`"#00000000"`, the return value of
  `rgb(0,0,0,0)`).

- ...:

  Other parameters to be passed to
  [`plot.default()`](https://rdrr.io/r/graphics/plot.default.html).

## Details

By default, contrast enhancement by stretch to min/max is applied when
the input data are single-band grayscale with any raster data type, or
three-band RGB with raster data type larger than Byte. The
minimum/maximum of the input data are used by default (i.e., no outlier
removal). No stretch is applied by default when the input is an RGB byte
raster. These defaults can be overridden by specifying either the
`minmax_def` argument (user-defined min/max per band), or the
`minmax_pct_cut` argument (ignore outlier pixels based on a percentile
range per band). These settings (and the `normalize` argument) are
ignored if a color table is used.

## Note

`plot_raster()` uses the function
[`graphics::rasterImage()`](https://rdrr.io/r/graphics/rasterImage.html)
for plotting which is not supported on some devices (see
[`?rasterImage`](https://rdrr.io/r/graphics/rasterImage.html)).

If `data` is an object of class `GDALRaster`, then `plot_raster()` will
attempt to read the entire raster into memory by default (unless the
number of pixels per band would exceed `max_pixels`). A reduced
resolution overview can be read by setting `xsize`, `ysize` smaller than
the raster size on disk. (If `data` is instead specified as a vector of
pixel values, a reduced resolution overview would be read by setting
`out_xsize` and `out_ysize` smaller than the raster region defined by
`xsize`, `ysize` in a call to `GDALRaster$read()`). The
GDAL_RASTERIO_RESAMPLING configuration option can be defined to override
the default resampling (NEAREST) to one of BILINEAR, CUBIC, CUBICSPLINE,
LANCZOS, AVERAGE or MODE, for example:

    set_config_option("GDAL_RASTERIO_RESAMPLING", "BILINEAR")

## See also

[`GDALRaster$read()`](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md),
[`read_ds()`](https://firelab.github.io/gdalraster/reference/read_ds.md),
[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md)

## Examples

``` r
## Elevation
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, elev_file)

# all other arguments are optional when passing a GDALRaster object
# grayscale
plot_raster(ds, legend = TRUE, main = "Storm Lake elevation (m)")


# color ramp from a user-defined palette: the `col_map_fn` can optionally
# be given as a color palette, for which a color ramp function is assumed
pal <- c("#00A60E", "#63C600", "#E6E600", "#E9BD3B", "#ECB176", "#EFC2B3",
         "#F2F2F2")

plot_raster(ds, col_map_fn = pal, legend = TRUE,
            main = "Storm Lake elevation (m)")


ds$close()

## Landsat band combination
b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
b6_file <- system.file("extdata/sr_b6_20200829.tif", package="gdalraster")
band_files <- c(b6_file, b5_file, b4_file)

vrt_file <- file.path(tempdir(), "storml_b6_b5_b4.vrt")
buildVRT(vrt_file, band_files, cl_arg = "-separate")
#> 0...10...20...30...40...50...60...70...80...90...100 - done.

ds <- new(GDALRaster, vrt_file)

plot_raster(ds, main = "Landsat 6-5-4 (vegetative analysis)")


ds$close()

## LANDFIRE Existing Vegetation Cover (EVC) with color map
evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")

# colors from the CSV attribute table distributed by LANDFIRE
evc_csv <- system.file("extdata/LF20_EVC_220.csv", package="gdalraster")
vat <- read.csv(evc_csv)
head(vat)
#>   VALUE                        CLASSNAMES   R   G   B      RED    GREEN
#> 1 -9999                       Fill-NoData 255 255 255 1.000000 1.000000
#> 2    11                        Open Water   0   0 255 0.000000 0.000000
#> 3    12                          Snow/Ice 159 161 240 0.623529 0.631373
#> 4    13 Developed-Upland Deciduous Forest  64  61 168 0.250980 0.239216
#> 5    14 Developed-Upland Evergreen Forest  68  79 137 0.266667 0.309804
#> 6    15     Developed-Upland Mixed Forest 102 119 205 0.400000 0.466667
#>       BLUE
#> 1 1.000000
#> 2 1.000000
#> 3 0.941176
#> 4 0.658824
#> 5 0.537255
#> 6 0.803922
vat <- vat[, c(1, 6:8)]

ds <- new(GDALRaster, evc_file)
plot_raster(ds, col_tbl = vat, interpolate = FALSE,
            main = "Storm Lake LANDFIRE EVC")


ds$close()

## Apply a pixel function
f <- system.file("extdata/complex.tif", package="gdalraster")
ds <- new(GDALRaster, f)
ds$getDataTypeName(band = 1)  # complex floating point
#> [1] "CFloat32"

plot_raster(ds,
            pixel_fn = Arg,
            col_map_fn = scales::pal_viridis(option = "plasma")(6),
            interpolate = FALSE,
            legend = TRUE,
            main = "Arg(complex.tif)")


ds$close()
```
