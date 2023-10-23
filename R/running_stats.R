#' @name RunningStats-class
#'
#' @aliases
#' Rcpp_RunningStats Rcpp_RunningStats-class RunningStats
#'
#' @title Class to calculate mean and variance in one pass
#'
#' @description
#' `RunningStats` computes summary statistics on a data stream efficiently.
#' Mean and variance are calculated with Welford's online algorithm
#' (\url{https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance}).
#' The min, max, sum and count are also tracked. The input data values are not 
#' stored in memory, so this class can be used to compute statistics for very 
#' large data streams.
#'
#' @param na_rm Logical scalar. `TRUE` to remove `NA` from the input data or
#' `FALSE` to retain `NA` (defaults to `TRUE`).
#' @returns An object of class `RunningStats`. A `RunningStats` object 
#' maintains the current minimum, maximum, mean, variance, sum and count of 
#' values that have been read from the stream. It can be updated repeatedly 
#' with new values (i.e., chunks of data read from the input stream), but its 
#' memory footprint is negligible. Class methods for updating with new values 
#' and retrieving current values of statistics are described in Details.
#' `RunningStats` is a C++ class exposed directly to R (via
#' `RCPP_EXPOSED_CLASS`). Methods of the class are accessed in R using the `$`
#' operator.
#'
#' @note
#' The intended use is computing summary statistics for specific subsets or
#' zones of a raster that could be defined in various ways and are generally
#' not contiguous. The algorithm as implemented here incurs the cost of
#' floating point division for each new value updated (i.e., per pixel), but is
#' reasonably efficient for the use case. Note that GDAL internally uses an
#' optimized version of Welford's algorithm to compute raster statistics as
#' described in detail by Rouault, 2016
#' (\url{https://github.com/OSGeo/gdal/blob/master/gcore/statistics.txt}).
#' The class method `GDALRaster$getStatistics()` is a GDAL API wrapper that
#' computes statistics for a whole raster band.
#'
#' @section Usage:
#' \preformatted{
#' rs <- new(RunningStats, na_rm)
#'
#' ## Methods (see Details)
#' rs$update(newvalues)
#' rs$get_count()
#' rs$get_mean()
#' rs$get_min()
#' rs$get_max()
#' rs$get_sum()
#' rs$get_var()
#' rs$get_sd()
#' rs$reset()
#' }
#'
#' @section Details:
#'
#' \code{new(RunningStats, na_rm)}
#' Constructor. Returns an object of class \code{RunningStats}.
#'
#' \code{$update(newvalues)}
#' Updates the `RunningStats` object with a numeric vector of `newvalues` 
#' (i.e., a chunk of values from the data stream). No return value, called 
#' for side effects.
#'
#' \code{$get_count()}
#' Returns the count of values received from the data stream.
#'
#' \code{$get_mean()}
#' Returns the mean of values received from the data stream.
#'
#' \code{$get_min()}
#' Returns the minimum value received from the data stream.
#'
#' \code{$get_max()}
#' Returns the maximum value received from the data stream.
#'
#' \code{$get_sum()}
#' Returns the sum of values received from the data stream.
#'
#' \code{$get_var()}
#' Returns the variance of values from the data stream 
#' (denominator n - 1).
#'
#' \code{$get_sd()}
#' Returns the standard deviation of values from the data stream
#' (denominator n - 1).
#'
#' \code{$reset()}
#' Clears the \code{RunningStats} object to its initialized state (count = 0).
#' No return value, called for side effects.
#' 
#' @examples
#' set.seed(42)
#'
#' rs <- new(RunningStats, na_rm=TRUE)
#' chunk <- runif(1000)
#' rs$update(chunk)
#' object.size(rs)
#'
#' rs$get_count()
#' length(chunk)
#'
#' rs$get_mean()
#' mean(chunk)
#'
#' rs$get_min()
#' min(chunk)
#'
#' rs$get_max()
#' max(chunk)
#'
#' rs$get_var()
#' var(chunk)
#'
#' rs$get_sd()
#' sd(chunk)
#' 
#' \donttest{
#' ## 10^9 values read in 10,000 chunks
#' ## should take under 1 minute on most PC hardware
#' for (i in 1:1e4) {
#'   chunk <- runif(1e5)
#'   rs$update(chunk)
#' }
#' rs$get_count()
#' rs$get_mean()
#' rs$get_var()
#'
#' object.size(rs)
#' }
NULL

Rcpp::loadModule("mod_running_stats", TRUE)
