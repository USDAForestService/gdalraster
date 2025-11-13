# Class to calculate mean and variance in one pass

`RunningStats` computes summary statistics on a data stream efficiently.
Mean and variance are calculated with Welford's online algorithm
(<https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance>).
The min, max, sum and count are also tracked. The input data values are
not stored in memory, so this class can be used to compute statistics
for very large data streams.

`RunningStats` is a C++ class exposed directly to R (via
`RCPP_EXPOSED_CLASS`). Methods of the class are accessed using the `$`
operator.

## Arguments

- na_rm:

  Logical scalar. `TRUE` to remove `NA` from the input data (the
  default) or `FALSE` to retain `NA`.

## Value

An object of class `RunningStats`. A `RunningStats` object maintains the
current minimum, maximum, mean, variance, sum and count of values that
have been read from the stream. It can be updated repeatedly with new
values (i.e., chunks of data read from the input stream), but its memory
footprint is negligible. Class methods for updating with new values, and
retrieving the current values of statistics, are described in Details.

## Note

The intended use is computing summary statistics for specific subsets or
zones of a raster that could be defined in various ways and are
generally not contiguous. The algorithm as implemented here incurs the
cost of floating point division for each new value updated (i.e., per
pixel), but is reasonably efficient for the use case. Note that GDAL
internally uses an optimized version of Welford's algorithm to compute
raster statistics as described in detail by Rouault, 2016
(<https://github.com/OSGeo/gdal/blob/master/gcore/statistics.txt>). The
class method `GDALRaster$getStatistics()` is a GDAL API wrapper that
computes statistics for a whole raster band.

## Usage (see Details)

    ## Constructor
    rs <- new(RunningStats, na_rm)

    ## Methods
    rs$update(newvalues)
    rs$get_count()
    rs$get_mean()
    rs$get_min()
    rs$get_max()
    rs$get_sum()
    rs$get_var()
    rs$get_sd()
    rs$reset()

## Details

### Constructor

`new(RunningStats, na_rm)`  
Returns an object of class `RunningStats`. The `na_rm` argument defaults
to `TRUE` if omitted.

### Methods

`$update(newvalues)`  
Updates the `RunningStats` object with a numeric vector of `newvalues`
(i.e., a chunk of values from the data stream). No return value, called
for side effects.

`$get_count()`  
Returns the count of values received from the data stream.

`$get_mean()`  
Returns the mean of values received from the data stream.

`$get_min()`  
Returns the minimum value received from the data stream.

`$get_max()`  
Returns the maximum value received from the data stream.

`$get_sum()`  
Returns the sum of values received from the data stream.

`$get_var()`  
Returns the variance of values from the data stream (denominator n - 1).

`$get_sd()`  
Returns the standard deviation of values from the data stream
(denominator n - 1).

`$reset()`  
Clears the `RunningStats` object to its initialized state (count = 0).
No return value, called for side effects.

## Examples

``` r
set.seed(42)
rs <- new(RunningStats, na_rm = TRUE)
rs
#> C++ object of class RunningStats
#>  Number of values: 0

chunk <- runif(1000)
rs$update(chunk)
object.size(rs)
#> 704 bytes

rs$get_count()
#> [1] 1000
length(chunk)
#> [1] 1000

rs$get_mean()
#> [1] 0.4882555
mean(chunk)
#> [1] 0.4882555

rs$get_min()
#> [1] 0.0002388966
min(chunk)
#> [1] 0.0002388966

rs$get_max()
#> [1] 0.9984908
max(chunk)
#> [1] 0.9984908

rs$get_var()
#> [1] 0.08493159
var(chunk)
#> [1] 0.08493159

rs$get_sd()
#> [1] 0.2914302
sd(chunk)
#> [1] 0.2914302

# \donttest{
## 10^9 values read in 10,000 chunks
## should take under 1 minute on most PC hardware
for (i in 1:1e4) {
  chunk <- runif(1e5)
  rs$update(chunk)
}
rs$get_count()
#> [1] 1000001000
rs$get_mean()
#> [1] 0.5000044
rs$get_var()
#> [1] 0.08333479

object.size(rs)
#> 704 bytes
# }
```
