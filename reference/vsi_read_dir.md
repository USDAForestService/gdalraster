# Read names in a directory

`vsi_read_dir()` abstracts access to directory contents. It returns a
character vector containing the names of files and directories in this
directory. With `recursive = TRUE`, reads the list of entries in the
directory and subdirectories. This function is a wrapper for
`VSIReadDirEx()` and `VSIReadDirRecursive()` in the GDAL Common
Portability Library.

## Usage

``` r
vsi_read_dir(path, max_files = 0L, recursive = FALSE, all_files = FALSE)
```

## Arguments

- path:

  Character string. The relative or absolute path of a directory to
  read.

- max_files:

  Integer scalar. The maximum number of files after which to stop, or 0
  for no limit (see Note). Ignored if `recursive = TRUE`.

- recursive:

  Logical scalar. `TRUE` to read the directory and its subdirectories.
  Defaults to `FALSE`.

- all_files:

  Logical scalar. If `FALSE` (the default), only the names of visible
  files are returned (following Unix-style visibility, that is files
  whose name does not start with a dot). If `TRUE`, all file names will
  be returned.

## Value

A character vector containing the names of files and directories in the
directory given by `path`. The listing is in alphabetical order, and
does not include the special entries '.' and '..' even if they are
present in the directory. An empty string (`""`) is returned if `path`
does not exist.

## Note

If `max_files` is set to a positive number, directory listing will stop
after that limit has been reached. Note that to indicate truncation, at
least one element more than the `max_files` limit will be returned. If
the length of the returned character vector is lesser or equal to
`max_files`, then no truncation occurred. The `max_files` parameter is
ignored when `recursive = TRUE`.

## See also

[`vsi_mkdir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_mkdir.md),
[`vsi_rmdir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_rmdir.md),
[`vsi_stat()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md),
[`vsi_sync()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_sync.md)

## Examples

``` r
# regular file system for illustration
data_dir <- system.file("extdata", package="gdalraster")
vsi_read_dir(data_dir)
#>  [1] "LF20_EVC_220.csv"                       
#>  [2] "LF20_EVH_220.csv"                       
#>  [3] "LF20_EVT_220.csv"                       
#>  [4] "LF20_F40_220.csv"                       
#>  [5] "byte.nc"                                
#>  [6] "byte.tif"                               
#>  [7] "complex.tif"                            
#>  [8] "doctype.xml"                            
#>  [9] "domains.gdb.zip"                        
#> [10] "domains.gpkg"                           
#> [11] "geomatrix.tif"                          
#> [12] "int64.tif"                              
#> [13] "metadata.zip"                           
#> [14] "multisurface.zip"                       
#> [15] "poly_multipoly.dbf"                     
#> [16] "poly_multipoly.prj"                     
#> [17] "poly_multipoly.shp"                     
#> [18] "poly_multipoly.shx"                     
#> [19] "small_world_and_byte.gpkg"              
#> [20] "south-up.tif"                           
#> [21] "sr_b4_20200829.tif"                     
#> [22] "sr_b5_20200829.tif"                     
#> [23] "sr_b6_20200829.tif"                     
#> [24] "storm_lake.lcp"                         
#> [25] "storm_lake.prj"                         
#> [26] "storml_elev.tif"                        
#> [27] "storml_elev_orig.tif"                   
#> [28] "storml_evc.tif"                         
#> [29] "storml_evh.tif"                         
#> [30] "storml_evt.tif"                         
#> [31] "storml_pts.csv"                         
#> [32] "storml_tcc.tif"                         
#> [33] "test.geojson"                           
#> [34] "test_ogr_geojson_mixed_timezone.geojson"
#> [35] "ynp_features.zip"                       
#> [36] "ynp_fires_1984_2022.gpkg"               
```
