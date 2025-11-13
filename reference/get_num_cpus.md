# Get the number of processors detected by GDAL

`get_num_cpus()` returns the number of processors detected by GDAL.
Wrapper of `CPLGetNumCPUs()` in the GDAL Common Portability Library.

## Usage

``` r
get_num_cpus()
```

## Value

Integer scalar, number of CPUs.

## Examples

``` r
get_num_cpus()
#> [1] 4
```
