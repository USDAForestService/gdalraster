# Obtain the 2D or 3D bounding envelope for input geometries

`g_envelope()` computes and returns the bounding envelope(s) for the
input geometries. Wrapper of `OGR_G_GetEnvelope()` /
`OGR_G_GetEnvelope3D()` in the GDAL Geometry API.

## Usage

``` r
g_envelope(geom, as_3d = FALSE, quiet = FALSE)
```

## Arguments

- geom:

  Either a raw vector of WKB or list of raw vectors, or a character
  vector containing one or more WKT strings.

- as_3d:

  Logical value. `TRUE` to return the 3D bounding envelope. The 2D
  envelope is returned by default (`as_3d = FALSE`).

- quiet:

  Logical value, `TRUE` to suppress warnings. Defaults to `FALSE`.

## Value

Either a numeric vector of length `4` containing the 2D envelope
`(xmin, xmax, ymin, ymax)` or of length `6` containing the 3D envelope
`(xmin, xmax, ymin, ymax, zmin, zmax)`, or a four-column or six-column
numeric matrix with number of rows equal to the number of input
geometries and column names `("xmin", "xmax", "ymin", "ymax")`, or
`("xmin", "xmax", "ymin", "ymax", "zmin", "zmax")` for the 3D case.
