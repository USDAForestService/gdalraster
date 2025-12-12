# Return the list of virtual file system handlers currently registered

`vsi_get_fs_prefixes()` returns the list of prefixes for virtual file
system handlers currently registered (e.g., `"/vsimem/"`, `"/vsicurl/"`,
etc). Wrapper for `VSIGetFileSystemsPrefixes()` in the GDAL API.

## Usage

``` r
vsi_get_fs_prefixes()
```

## Value

Character vector containing prefixes of the virtual file system
handlers.

## See also

[`vsi_get_fs_options()`](https://firelab.github.io/gdalraster/reference/vsi_get_fs_options.md)

<https://gdal.org/en/stable/user/virtual_file_systems.html>

## Examples

``` r
vsi_get_fs_prefixes()
#>  [1] "/vsiadls/"            "/vsiaz/"              "/vsiaz_streaming/"   
#>  [4] "/vsicached?"          "/vsicrypt/"           "/vsicurl/"           
#>  [7] "/vsicurl_streaming/"  "/vsigs/"              "/vsigs_streaming/"   
#> [10] "/vsigzip/"            "/vsimem/"             "/vsioss/"            
#> [13] "/vsioss_streaming/"   "/vsipmtiles/"         "/vsis3/"             
#> [16] "/vsis3_streaming/"    "/vsisparse/"          "/vsistdin/"          
#> [19] "/vsistdin?"           "/vsistdout/"          "/vsistdout_redirect/"
#> [22] "/vsisubfile/"         "/vsiswift/"           "/vsiswift_streaming/"
#> [25] "/vsitar/"             "/vsiwebhdfs/"         "/vsizip/"            
```
