# Get usable physical RAM reported by GDAL

`get_usable_physical_ram()` returns the total physical RAM, usable by a
process, in bytes. It will limit to 2 GB for 32 bit processes. Starting
with GDAL 2.4.0, it will also take into account resource limits (virtual
memory) on Posix systems. Starting with GDAL 3.6.1, it will also take
into account RLIMIT_RSS on Linux. Wrapper of `CPLGetUsablePhysicalRAM()`
in the GDAL Common Portability Library.

## Usage

``` r
get_usable_physical_ram()
```

## Value

Numeric scalar, number of bytes as
[`bit64::integer64`](https://rdrr.io/pkg/bit64/man/bit64-package.html)
type (or 0 in case of failure).

## Note

This memory may already be partly used by other processes.

## Examples

``` r
get_usable_physical_ram()
#> integer64
#> [1] 16772583424
```
