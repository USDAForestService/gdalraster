# Pop error handler off stack

`pop_error_handler()` is a wrapper for `CPLPopErrorHandler()` in the
GDAL Common Portability Library. Discards the current error handler on
the error handler stack, and restores the one in use before the last
[`push_error_handler()`](https://firelab.github.io/gdalraster/reference/push_error_handler.md)
call. This method has no effect if there are no error handlers on the
current thread's error handler stack.

## Usage

``` r
pop_error_handler()
```

## Value

No return value, called for side effects.

## See also

[`push_error_handler()`](https://firelab.github.io/gdalraster/reference/push_error_handler.md)

## Examples

``` r
push_error_handler("quiet")
# ...
pop_error_handler()
```
