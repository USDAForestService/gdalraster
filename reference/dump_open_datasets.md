# Report open datasets

`dump_open_datasets()` dumps a list of all open datasets (shared or not)
to the console. This function is primarily intended to assist in
debugging "dataset leaks" and reference counting issues. The information
reported includes the dataset name, referenced count, shared status,
driver name, size, and band count. This a wrapper for
`GDALDumpOpenDatasets()` with output to the console.

## Usage

``` r
dump_open_datasets()
```

## Value

Number of open datasets.

## Examples

``` r
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, elev_file)
dump_open_datasets()
#> Open GDAL Datasets:
#>   1 S GTiff  -1044068352 143x107x1 /home/runner/work/_temp/Library/gdalraster/extdata/storml_elev.tif
#> [1] 1
ds2 <- new(GDALRaster, elev_file)
dump_open_datasets()
#> Open GDAL Datasets:
#>   2 S GTiff  -1044068352 143x107x1 /home/runner/work/_temp/Library/gdalraster/extdata/storml_elev.tif
#> [1] 1
# open without using shared mode
ds3 <- new(GDALRaster, elev_file, read_only = TRUE,
           open_options = NULL, shared = FALSE)
dump_open_datasets()
#> Open GDAL Datasets:
#>   1 N GTiff       -1 143x107x1 /home/runner/work/_temp/Library/gdalraster/extdata/storml_elev.tif
#>   2 S GTiff  -1044068352 143x107x1 /home/runner/work/_temp/Library/gdalraster/extdata/storml_elev.tif
#> [1] 2
ds$close()
dump_open_datasets()
#> Open GDAL Datasets:
#>   1 N GTiff       -1 143x107x1 /home/runner/work/_temp/Library/gdalraster/extdata/storml_elev.tif
#>   1 S GTiff  -1044068352 143x107x1 /home/runner/work/_temp/Library/gdalraster/extdata/storml_elev.tif
#> [1] 2
ds2$close()
dump_open_datasets()
#> Open GDAL Datasets:
#>   1 N GTiff       -1 143x107x1 /home/runner/work/_temp/Library/gdalraster/extdata/storml_elev.tif
#> [1] 1
ds3$close()
dump_open_datasets()
#> [1] 0
```
