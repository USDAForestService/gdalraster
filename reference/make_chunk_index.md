# Generate an index of chunk offsets and sizes for iterating a raster

`make_chunk_index()` returns a matrix of `xchunkoff`, `ychunkoff`,
`xoff`, `yoff`, `xsize`, `ysize`, `xmin`, `xmax`, `ymin` and `ymax`,
i.e., indexing of potentially multi-block chunks defined on block
boundaries for iterating I/O operations over a raster. The last four
columns are geospatial coordinates of the bounding box. Note that class
`GDALRaster` provides a method of the same name that is more convenient
to use with a dataset object.

## Usage

``` r
make_chunk_index(
  raster_xsize,
  raster_ysize,
  block_xsize,
  block_ysize,
  gt = c(0, 1, 0, 0, 0, 1),
  max_pixels = 0
)
```

## Arguments

- raster_xsize:

  Integer value giving the number of raster columns.

- raster_ysize:

  Integer value giving the number of raster rows.

- block_xsize:

  Integer value giving the horizontal size of a raster block in number
  of pixels.

- block_ysize:

  Integer value giving the vertical size of a raster block in number of
  pixels.

- gt:

  A numeric vector of length six containing the affine geotransform for
  the raster (defaults to `c(0, 1, 0, 0, 0, 1)`). Required only if
  geospatial bounding boxes of the chunks are needed in the output.

- max_pixels:

  Numeric value (a whole number), optionally carrying the
  [`bit64::integer64`](https://rdrr.io/pkg/bit64/man/bit64-package.html)
  class attribute. Specifies the maximum number of pixels per chunk. Can
  be set to zero to define chunks as the blocks.

## Value

A numeric matrix with number of rows equal to the number of chunks, and
named columns: `xchunkoff`, `ychunkoff`, `xoff`, `yoff`, `xsize`,
`ysize`, `xmin`, `xmax`, `ymin`, `ymax`. Offsets are 0-based.

## Details

The stand-alone function here supports the general case of
chunking/tiling a raster layout without using a dataset object. If the
`max_pixels` argument is set to zero, the chunks are raster blocks (the
internal tiles in the case of a tiled format). Otherwise, chunks are
defined as the maximum number of consecutive whole blocks containing
`<= max_pixels`, that may span one or multiple whole rows of blocks.

## See also

Methods `make_chunk_index()`, `readChunk()` and `writeChunk()` in class
[GDALRaster](https://firelab.github.io/gdalraster/reference/GDALRaster-class.md).

Usage example in the web article [GDAL Block
Cache](https://firelab.github.io/gdalraster/articles/gdal-block-cache.html).

## Examples

``` r
## chunk as one block
blocks <- make_chunk_index(raster_xsize = 156335, raster_ysize = 101538,
                           block_xsize = 256, block_ysize = 256,
                           gt = c(-2362395, 30, 0, 3267405, 0, -30),
                           max_pixels = 0)

nrow(blocks)
#> [1] 242567

head(blocks)
#>      xchunkoff ychunkoff xoff yoff xsize ysize     xmin     xmax    ymin
#> [1,]         0         0    0    0   256   256 -2362395 -2354715 3259725
#> [2,]         1         0  256    0   256   256 -2354715 -2347035 3259725
#> [3,]         2         0  512    0   256   256 -2347035 -2339355 3259725
#> [4,]         3         0  768    0   256   256 -2339355 -2331675 3259725
#> [5,]         4         0 1024    0   256   256 -2331675 -2323995 3259725
#> [6,]         5         0 1280    0   256   256 -2323995 -2316315 3259725
#>         ymax
#> [1,] 3267405
#> [2,] 3267405
#> [3,] 3267405
#> [4,] 3267405
#> [5,] 3267405
#> [6,] 3267405

tail(blocks)
#>           xchunkoff ychunkoff   xoff   yoff xsize ysize    xmin    xmax   ymin
#> [242562,]       605       396 154880 101376   256   162 2284005 2291685 221265
#> [242563,]       606       396 155136 101376   256   162 2291685 2299365 221265
#> [242564,]       607       396 155392 101376   256   162 2299365 2307045 221265
#> [242565,]       608       396 155648 101376   256   162 2307045 2314725 221265
#> [242566,]       609       396 155904 101376   256   162 2314725 2322405 221265
#> [242567,]       610       396 156160 101376   175   162 2322405 2327655 221265
#>             ymax
#> [242562,] 226125
#> [242563,] 226125
#> [242564,] 226125
#> [242565,] 226125
#> [242566,] 226125
#> [242567,] 226125

## chunk as 16 consectutive blocks
chunks <- make_chunk_index(raster_xsize = 156335, raster_ysize = 101538,
                           block_xsize = 256, block_ysize = 256,
                           gt = c(-2362395, 30, 0, 3267405, 0, -30),
                           max_pixels = 256 * 256 * 16)

nrow(chunks)
#> [1] 15483

head(chunks)
#>      xchunkoff ychunkoff  xoff yoff xsize ysize     xmin     xmax    ymin
#> [1,]         0         0     0    0  4096   256 -2362395 -2239515 3259725
#> [2,]         1         0  4096    0  4096   256 -2239515 -2116635 3259725
#> [3,]         2         0  8192    0  4096   256 -2116635 -1993755 3259725
#> [4,]         3         0 12288    0  4096   256 -1993755 -1870875 3259725
#> [5,]         4         0 16384    0  4096   256 -1870875 -1747995 3259725
#> [6,]         5         0 20480    0  4096   256 -1747995 -1625115 3259725
#>         ymax
#> [1,] 3267405
#> [2,] 3267405
#> [3,] 3267405
#> [4,] 3267405
#> [5,] 3267405
#> [6,] 3267405

tail(chunks)
#>          xchunkoff ychunkoff   xoff   yoff xsize ysize    xmin    xmax   ymin
#> [15478,]        33       396 135168 101376  4096   162 1692645 1815525 221265
#> [15479,]        34       396 139264 101376  4096   162 1815525 1938405 221265
#> [15480,]        35       396 143360 101376  4096   162 1938405 2061285 221265
#> [15481,]        36       396 147456 101376  4096   162 2061285 2184165 221265
#> [15482,]        37       396 151552 101376  4096   162 2184165 2307045 221265
#> [15483,]        38       396 155648 101376   687   162 2307045 2327655 221265
#>            ymax
#> [15478,] 226125
#> [15479,] 226125
#> [15480,] 226125
#> [15481,] 226125
#> [15482,] 226125
#> [15483,] 226125
```
