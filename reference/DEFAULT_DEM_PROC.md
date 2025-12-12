# List of default DEM processing options

These values are used in
[`dem_proc()`](https://firelab.github.io/gdalraster/reference/dem_proc.md)
as the default processing options:

        list(
             "hillshade" =    c("-z", "1", "-s", "1", "-az", "315",
                                "-alt", "45", "-alg", "Horn",
                                "-combined", "-compute_edges"),
             "slope" =        c("-s", "1", "-alg", "Horn", "-compute_edges"),
             "aspect" =       c("-alg", "Horn", "-compute_edges"),
             "color-relief" = character(),
             "TRI" =          c("-alg", "Riley", "-compute_edges"),
             "TPI" =          c("-compute_edges"),
             "roughness" =    c("-compute_edges"))

## Usage

``` r
DEFAULT_DEM_PROC
```

## Format

An object of class `list` of length 7.

## See also

[`dem_proc()`](https://firelab.github.io/gdalraster/reference/dem_proc.md)

<https://gdal.org/en/stable/programs/gdaldem.html> for a description of
all available command-line options for each processing mode
