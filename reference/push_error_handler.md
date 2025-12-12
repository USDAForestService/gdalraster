# Push a new GDAL CPLError handler

`push_error_handler()` is a wrapper for `CPLPushErrorHandler()` in the
GDAL Common Portability Library. This pushes a new error handler on the
thread-local error handler stack. This handler will be used until
removed with
[`pop_error_handler()`](https://firelab.github.io/gdalraster/reference/pop_error_handler.md).
A typical use is to temporarily set `CPLQuietErrorHandler()` which
doesn't make any attempt to report passed error or warning messages, but
will process debug messages via `CPLDefaultErrorHandler`.

## Usage

``` r
push_error_handler(handler)
```

## Arguments

- handler:

  Character name of the error handler to push. One of `quiet`, `logging`
  or `default`.

## Value

No return value, called for side effects.

## Note

This function is for advanced use cases. It is intended for setting a
*temporary* error handler in a limited segment of code. A global error
handler specific to the R environment is in use by default.

Setting `handler = "logging"` will use `CPLLoggingErrorHandler()`, error
handler that logs into the file defined by the `CPL_LOG` configuration
option. Be sure that option is set when using this error handler.

This only affects error reporting from GDAL.

## See also

[`pop_error_handler()`](https://firelab.github.io/gdalraster/reference/pop_error_handler.md)

## Examples

``` r
push_error_handler("quiet")
# ...
pop_error_handler()
```
