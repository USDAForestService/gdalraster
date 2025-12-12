# Set a path specific option for a given path prefix

`vsi_set_path_option()` sets a path specific option for a given path
prefix. Such an option is typically, but not limited to, setting
credentials for a virtual file system. Wrapper for
`VSISetPathSpecificOption()` in the GDAL Common Portability Library.
Requires GDAL \>= 3.6.

## Usage

``` r
vsi_set_path_option(path_prefix, key, value)
```

## Arguments

- path_prefix:

  Character string. A path prefix of a virtual file system handler.
  Typically of the form `/vsiXXX/bucket`.

- key:

  Character string. Option key.

- value:

  Character string. Option value. Passing `value = ""` (empty string)
  will unset a value previously set by `vsi_set_path_option()`.

## Value

No return value, called for side effect.

## Details

Options may also be set with
[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md),
but `vsi_set_path_option()` allows specifying them with a granularity at
the level of a file path. This makes it easier if using the same virtual
file system but with different credentials (e.g., different credentials
for buckets "/vsis3/foo" and "/vsis3/bar"). This is supported for the
following virtual file systems: /vsis3/, /vsigs/, /vsiaz/, /vsioss/,
/vsiwebhdfs, /vsiswift.

## Note

Setting options for a path starting with /vsiXXX/ will also apply for
/vsiXXX_streaming/ requests.

No particular care is taken to store options in RAM in a secure way. So
they might accidentally hit persistent storage if swapping occurs, or
someone with access to the memory allocated by the process may be able
to read them.

## See also

[`set_config_option()`](https://firelab.github.io/gdalraster/reference/set_config_option.md),
[`vsi_clear_path_options()`](https://firelab.github.io/gdalraster/reference/vsi_clear_path_options.md)
