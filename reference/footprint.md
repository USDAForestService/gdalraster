# Compute footprint of a raster

`footprint()` is a wrapper of the `gdal_footprint` command-line utility
(see <https://gdal.org/en/stable/programs/gdal_footprint.html>). The
function can be used to compute the footprint of a raster file, taking
into account nodata values (or more generally the mask band attached to
the raster bands), and generating polygons/multipolygons corresponding
to areas where pixels are valid, and write to an output vector file.
Refer to the GDAL documentation at the URL above for a list of
command-line arguments that can be passed in `cl_arg`. Requires GDAL \>=
3.8.

## Usage

``` r
footprint(src_filename, dst_filename, cl_arg = NULL)
```

## Arguments

- src_filename:

  Character string. Filename of the source raster.

- dst_filename:

  Character string. Filename of the destination vector. If the file and
  the output layer exist, the new footprint is appended to them, unless
  the `-overwrite` command-line argument is used.

- cl_arg:

  Optional character vector of command-line arguments for
  `gdal_footprint`.

## Value

Logical indicating success (invisible `TRUE`). An error is raised if the
operation fails.

## Details

Post-vectorization geometric operations are applied in the following
order:

- optional splitting (`-split_polys`)

- optional densification (`-densify`)

- optional reprojection (`-t_srs`)

- optional filtering by minimum ring area (`-min_ring_area`)

- optional application of convex hull (`-convex_hull`)

- optional simplification (`-simplify`)

- limitation of number of points (`-max_points`)

## See also

[`polygonize()`](https://firelab.github.io/gdalraster/reference/polygonize.md)

## Examples

``` r
evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
out_file <- file.path(tempdir(), "storml.geojson")

# Requires GDAL >= 3.8
if (gdal_version_num() >= gdal_compute_version(3, 8, 0)) {
  # command-line arguments for gdal_footprint
  args <- c("-t_srs", "EPSG:4326")
  footprint(evt_file, out_file, args)
  DONTSHOW({deleteDataset(out_file)})
}
#> 0...10...20...30...40...50...60...70...80...90...100 - done.
```
