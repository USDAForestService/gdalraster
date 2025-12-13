# Class encapsulating a GDAL CLI algorithm

`GDALAlg` provides bindings to `GDALAlgorithm` and related classes that
implement the `gdal` command line interface (CLI) in the GDAL API. An
object of class `GDALAlg` represents an instance of a CLI algorithm with
methods to obtain algorithm information and argument information, run
the algorithm, and access its output.

**Requires GDAL \>= 3.11.3**.

**Experimental** (see the section `Development Status` below)

`GDALAlg` is a C++ class exposed directly to R (via
`RCPP_EXPOSED_CLASS`). Fields and methods of the class are accessed
using the `$` operator. Arguments to class constructors and class
methods must be given in the order documented (naming optional).

## Arguments

- cmd:

  A character string or character vector containing the path to the
  algorithm, e.g., `"raster reproject"` or `c("raster", "reproject")`.

- args:

  Either a character vector or a named list containing input arguments
  of the algorithm (see section `Algorithm Argument Syntax` below).

## Value

An object of class `GDALAlg`, which contains a pointer to the algorithm
instance. Class methods are described in Details, along with a set of
writable fields for per-object settings.

## Usage (see Details)

    ## Constructors
    alg <- new(GDALAlg, cmd)
    # or, with arguments
    alg <- new(GDALAlg, cmd, args)

    ## Read/write fields (per-object settings)
    alg$setVectorArgsFromObject
    alg$outputLayerNameForOpen
    alg$quiet

    ## Methods
    alg$info()
    alg$argInfo(arg_name)
    alg$usage()
    alg$usageAsJSON()

    alg$setArg(arg_name, arg_value)
    alg$parseCommandLineArgs()
    alg$getExplicitlySetArgs()
    alg$run()
    alg$output()
    alg$outputs()

    alg$close()
    alg$release()

## Details

### Constructors

`new(GDALAlg, cmd)`  
Instantiate an algorithm without specifying input arguments.

`new(GDALAlg, cmd, args)`  
Instantiate an algorithm giving input arguments as a character vector or
named list. See the section `Algorithm Argument Syntax` for details.

### Read/write fields (per-object settings)

`$setVectorArgsFromObject`  
A logical value, `TRUE` (the default) to set algorithm arguments
automatically when the `"input"` argument or the `"like"` argument is an
object of class `GDALVector`. Argument values specified explicitly will
override the automatic setting (as long as they result in a parsable set
of arguments). Automatically setting arguments from `GDALVector` input
can be disabled by setting this field to `FALSE`. In that case, only the
vector dataset would be passed to the algorithm, i.e., without
automatically passing any layer specifications. When enabled and the
`"input"` or `"like"` argument for an algorithm is given as a
`GDALVector` object, corresponding arguments will be set automatically
based on properties of the object (when the argument is available to the
algorithm):

- `"input-format"`: set to the `GDALVector` object's driver short name

- `"input-layer"`: set to the `GDALVector` layer name if it is not a SQL
  layer

- `"sql"`: set to the SQL statement if the `GDALVector` layer is defined
  by one

- `"dialect"`: set to the SQL dialect if one is specified for a SQL
  layer

- `"like-layer"`: set to the `GDALVector` layer name if it is not a SQL
  layer

- `"like-sql"`: set to the SQL statement if the `GDALVector` layer is
  defined by one

`$outputLayerNameForOpen`  
A character string specifying a layer name to open when obtaining
algorithm output as an object of class `GDALVector`. See method
`$output()` below. The default value is empty string (`""`) in which
case the first layer by index is opened. Ignored if output is not a
vector dataset.

`$quiet`  
A logical value, `FALSE` by default. Set to `TRUE` to suppress progress
reporting along with various messages and warnings.

### Methods

`$info()`  
Returns a named list of algorithm properties with the following
elements:

- `name`: character string, the algorithm name

- `full_path`: character string, the algorithm name as full path

- `description`: character string, the algorithm (short) description

- `long_description`: character string, the algorithm longer description

- `URL`: character string, the algorithm help URL

- `has_subalgorithms`: logical, `TRUE` if the algorithm has
  sub-algorithms

- `subalgorithm_names`: character vector of sub-algorithm names (may be
  empty)parseCommandLineArgsparseCommanparseCommandLineArgsparseCommandLineArgsdLineArgs

- `arg_names`: character vector of available argument names

`$argInfo(arg_name)`  
Returns a named list of properties for an algorithm argument given as a
character string, with the following elements:

- `name`: character string, the name of the argument

- `type`: character string, the argument type as one of `"BOOLEAN"`,
  `"STRING"`, `"INTEGER"`, `"REAL"`, `"DATASET"`, `"STRING_LIST"`,
  `"INTEGER_LIST"`, `"REAL_LIST"`, `"DATASET_LIST"`

- `description`: character string, the argument description

- `short_name`: character string, the short name or empty string if
  there is none

- `aliases`: character vector of aliases (empty if none)

- `meta_var`: character string, the "meta-var" hint (by default, the
  meta-var value is the long name of the argument in upper case)

- `category`: character string, the argument category

- `is_positional`: logical, `TRUE` if the argument is a positional one

- `is_required`: logical, `TRUE` if the argument is required

- `min_count`: integer, the minimum number of values for the argument
  (only applies to list type of arguments)

- `max_count`: integer, the maximum number of values for the argument
  (only applies to list type of arguments)

- `packed_values_allowed`: logical, `TRUE` if, for list type of
  arguments, several comma-separated values may be specified (i.e.,
  `"--foo=bar,baz"`)

- `repeated_arg_allowed`: logical, `TRUE` if, for list type of
  arguments, the argument may be repeated (i.e.,
  `c("--foo=bar", "--foo=baz")`)

- `choices`: character vector of allowed values for the argument (may be
  empty and only applies for argument types `"STRING"` and
  `"STRING_LIST"`)

- `is_explicitly_set`: logical, `TRUE` if the argument value has been
  explicitly set

- `has_default_value`: logical, `TRUE` if the argument has a declared
  default value

- `default_value`: character or integer or numeric, the default value if
  the argument has one, otherwise `NULL`

- `is_hidden_for_api`: logical, `TRUE` if the argument is only for CLI
  usage (e.g., "–help") (GDAL \>= 3.12 only, see `is_only_for_cli`)

- `is_hidden_for_cli`: logical, `TRUE` if the argument must not be
  mentioned in CLI usage (e.g., "output-value" for "gdal raster info",
  which is only meant when the algorithm is used from a non-CLI context
  such as programmatically from R)

- `is_only_for_cli`: logical, `TRUE` if the argument is only for CLI
  usage (e.g., "–help") (deprecated at GDAL 3.12, see
  `is_hidden_for_api`)

- `is_input`: logical, `TRUE` if the value of the argument is read-only
  during the execution of the algorithm

- `is_output`: logical, `TRUE` if (at least part of) the value of the
  argument is set during the execution of the algorithm

- `dataset_type_flags`: character vector containing strings `"RASTER"`,
  `"VECTOR"`, `"MULTIDIM_RASTER"`, possibly with `"UPDATE"` (`NULL` if
  the argument is not a dataset type)

- `dataset_input_flags`: character vector indicating if a dataset
  argument supports specifying only the dataset name (`"NAME"`), only
  the dataset object (`"OBJECT"`), or both (`"NAME", "OBJECT"`) when it
  is used as an input (`NULL` if the argument is not a dataset type)

- `dataset_output_flags`: character vector indicating if a dataset
  argument supports specifying only the dataset name (`"NAME"`), only
  the dataset object (`"OBJECT"`), or both (`"NAME", "OBJECT"`) when it
  is used as an output (`NULL` if the argument is not a dataset type)

- `mutual_exclusion_group`: character string, the name of the mutual
  exclusion group to which this argument belongs

`$usage()`  
Print a help message for the algorithm to the console. No return value.

`$usageAsJSON()`  
Returns the usage of the algorithm as a JSON-serialized string.

`$setArg(arg_name, arg_value)`  
Set the value of input algorithm argument `arg_name`, a character string
containing the argument's “long” name or an alias. The type of the
`arg_value` parameter must be compatible with the algorithm argument
type. Objects of class `GDALRaster` or `GDALVector` may be passed for
algorithm arguments that accept dataset object input. A list of
`GDALRaster` or `GDALVector` objects may be given for algorithm
arguments of type `DATASET_LIST` that accept object input. Generally, an
input dataset can also be specified by name as a character string (DSN),
or character vector of DSNs for a `DATASET_LIST`.

`$parseCommandLineArgs()`  
Sets the value of arguments previously specified in the class
constructor, and instantiates the actual algorithm that will be run (but
without running it). Returns a logical value, `TRUE` indicating success
or `FALSE` if an error occurs.

`$getExplicitlySetArgs()`  
Returns a named list of arguments that have been set explicitly along
with their values. For arguments with dataset object values, the value
of the list element is a string of the form
`"<TYPE dataset object: DSN>"` where `TYPE` is one of `"raster"`,
`"vector"` or `"multidim raster"` and `DSN` is the dataset name
(filename, URL, etc.).

`$run()`  
Executes the algorithm, first parsing arguments if
`$parseCommandLineArgs()` has not already been called explicitly.
Returns a logical value, `TRUE` indicating success or `FALSE` if an
error occurs.

`$output()`  
Returns the single output value of the algorithm, after it has been run.
If there are multiple output values, this method will raise an error,
and the `$outputs()` (plural) method should be called instead. The type
of the return value corresponds to the type of the single output
argument value (see method `$argInfo()` above). If the output argument
has type `"DATASET"`, an object of class `GDALRaster` will be returned
if the dataset is raster, or an object of class `GDALVector` if the
dataset is vector. In the latter case, by default the `GDALVector`
object will be opened on the first layer by index, but a specific layer
name may be specified by setting the value of the field
`$outputLayerNameForOpen` before calling the `$output()` method (see
above). Note that currently, if the output dataset is multidimensional
raster, only the dataset name will be returned as a character string.

`$outputs()`  
Returns the output value(s) of the algorithm as a named list, after it
has been run. Most algorithms only return a single output, in which case
the `$output()` method (singular) is preferable for easier use. The
element names in the returned list are the names of the arguments that
have outputs (with any dash characters replaced by underscore), and the
values are the argument values which may include `GDALRaster` or
`GDALVector` objects.

`$close()`  
Completes any pending actions, and returns the final status as a logical
value (`TRUE` if no errors occur during the underlying call to
`GDALAlgorithmFinalize()`). This is typically useful for algorithms that
generate an output dataset. It closes datasets and gets back potential
error status resulting from that, e.g., if an error occurs during
flushing to disk of the output dataset after successful `$run()`
execution.

`$release()`  
Release memory associated with the algorithm, potentially after
attempting to finalize. No return value, called for side-effects.

## Algorithm Argument Syntax

Arguments are given in R as a character vector or named list, but
otherwise syntax basically matches the GDAL specification for arguments
as they are given on the command line. Those specifications are listed
here along with some amendments regarding the character vector and named
list formats. Programmatic usage also allows passing and receiving
datasets as objects (i.e., `GDALRaster` or `GDALVector`), in addition to
dataset names (e.g., filename, URL, database connection string).

- Commands accept one or several positional arguments, typically for
  dataset names (or in R as `GDALRaster` or `GDALVector` datasets). The
  order is input(s) first, output last. Positional arguments can also be
  specified as named arguments, if preferred to avoid any ambiguity.

- Named arguments have:

  - at least one "long" name, preceded by two dash characters

  - optionally, auxiliary long names (i.e., aliases),

  - and optionally a one-letter short name, preceded by a single dash
    character, e.g.,
    `-f, --of, --format, --output-format <OUTPUT-FORMAT>`

- Boolean arguments are specified by just specifying the argument name
  in character vector format. In R `list` format, the named element must
  be assigned a value of logical `TRUE`.

- Arguments that require a value are specified like:

  - `-f VALUE` for one-letter short names

  - `--format VALUE` or `--format=VALUE` for long names

  - in a named list, this might look like: `args$format <- VALUE`

- Some arguments can be multi-valued. Some of them require all values to
  be packed together and separated with comma. This is, e.g., the case
  of:  
  `--bbox <BBOX> Clipping bounding box as xmin,ymin,xmax,ymax`  
  e.g., `--bbox=2.1,49.1,2.9,49.9`

- Others accept each value to be preceded by a new mention of the
  argument name, e.g., `c("--co", "COMPRESS=LZW", "--co", "TILED=YES")`.
  For that one, if the value of the argument does not contain commas,
  the packed form is also accepted: `--co COMPRESS=LZW,TILED=YES`. Note
  that repeated mentions of an argument are possible in the character
  vector format for argument input, whereas arguments given in named
  list format must use argument long names as the list element names,
  and the packed format for the values (which can be a character vector
  or numeric vector of values).

- Named arguments can be placed before or after positional arguments.

## Development Status

The GDAL Command Line Interface Modernization was first introduced in
the [GDAL 3.11.0
release](https://github.com/OSGeo/gdal/releases/tag/v3.11.0)
(2025-05-09). The GDAL project provides warning that the new CLI "is
provisionally provided as an alternative interface to GDAL and OGR
command line utilities. The project reserves the right to modify,
rename, reorganize, and change the behavior until it is officially
frozen via PSC vote in a future major GDAL release. The utility needs
time to mature, benefit from incremental feedback, and explore
enhancements without carrying the burden of full backward compatibility.
Your usage of it should have no expectation of compatibility until that
time." (<https://gdal.org/en/latest/programs/#gdal-application>)

Initial bindings to enable programmatic use of the CLI algorithms from R
were added in gdalraster 2.2.0, and will evolve over future releases.
*The bindings are considered experimental until the upstream API is
declared stable*. Breaking changes in minor version releases are
possible until then. Please use with those cautions in mind. Bug reports
may be filed at: <https://github.com/firelab/gdalraster/issues>.

## See also

[`gdal_alg()`](https://firelab.github.io/gdalraster/reference/gdal_cli.md),
[`gdal_commands()`](https://firelab.github.io/gdalraster/reference/gdal_cli.md),
[`gdal_run()`](https://firelab.github.io/gdalraster/reference/gdal_cli.md),
[`gdal_usage()`](https://firelab.github.io/gdalraster/reference/gdal_cli.md)

Using `gdal` CLI algorithms from R  
<https://firelab.github.io/gdalraster/articles/use-gdal-cli-from-r.html>

## Examples

``` r
if (FALSE) { # length(gdal_global_reg_names()) > 0
f <- system.file("extdata/storml_elev.tif", package="gdalraster")

## raster info
gdal_usage("raster info")

# arguments given as character vector
args <- c("--format=text", "--no-md", f)
(alg <- new(GDALAlg, "raster info", args))

alg$run()

alg$output() |> writeLines()

alg$release()

## raster clip
gdal_usage("raster clip")

# arguments in list format
args <- list()
args$input <- f
f_clip <- file.path(tempdir(), "elev_clip.tif")
args$output <- f_clip
args$overwrite <- TRUE
args$bbox <- c(323776.1, 5102172.0,  327466.1, 5104782.0)

(alg <- new(GDALAlg, "raster clip", args))

alg$run()

# get output as a GDALRaster object
(ds_clip <- alg$output())

alg$release()

ds_clip$dim()
ds_clip$bbox()

## raster hillshade
gdal_usage("raster hillshade")

# input as a GDALRaster object and output to an in-memory raster
args <- list(input = ds_clip,
             output_format = "MEM",
             output = "")

(alg <- new(GDALAlg, "raster hillshade", args))

alg$run()

(ds_hillshade <- alg$output())

alg$release()

plot_raster(ds_hillshade)

ds_clip$close()
ds_hillshade$close()
deleteDataset(f_clip)

## vector convert shapefile to GeoPackage
gdal_usage("vector convert")

f_shp <- system.file("extdata/poly_multipoly.shp", package="gdalraster")
f_gpkg <- file.path(tempdir(), "polygons_test.gpkg")
args <- c("--input", f_shp, "--output", f_gpkg, "--overwrite")

(alg <- new(GDALAlg, "vector convert", args))

alg$run()

# output as a GDALVector object
(lyr <- alg$output())

alg$release()

lyr$info()

lyr$close()
deleteDataset(f_gpkg)

## raster reproject (usage and info)
# no arguments given, only for retrieving usage and properties via API
# cf. gdal_usage("raster reproject")

(alg <- new(GDALAlg, "raster reproject"))

# print usage to the console, no return value
alg$usage()

# or, get usage as a JSON string
# json <- alg$usageAsJSON()
# writeLines(json)

# list of algorithm properties
alg$info()

# list of properties for an algorithm argument
alg$argInfo("resampling")

alg$release()
}
```
