# Functions for using GDAL CLI algorithms

This set of functions can be used to access and run `gdal` command line
interface (CLI) algorithms.

**Requires GDAL \>= 3.11.3**

**Experimental** (see the section `Development Status` below)

## Usage

``` r
gdal_commands(contains = NULL, recurse = TRUE, cout = TRUE)

gdal_usage(cmd = NULL)

gdal_run(cmd, args, setVectorArgsFromObject = TRUE)

gdal_alg(cmd = NULL, args = NULL, parse = TRUE)

gdal_global_reg_names()
```

## Arguments

- contains:

  Optional character string for filtering output to certain commands,
  e.g., `gdal_commands("vector")`.

- recurse:

  Logical value, `TRUE` to include all subcommands recursively (the
  default). Set to `FALSE` to include only the top-level `gdal` commands
  (i.e., `raster`, `vector`, etc.)

- cout:

  Logical value, `TRUE` to print a list of commands along with their
  descriptions and help URLS to the console (the default).

- cmd:

  A character string or character vector containing the path to the
  algorithm, e.g., `"raster reproject"` or `c("raster", "reproject")`.
  Defaults to `"gdal"`, the main entry point to CLI commands.

- args:

  Either a character vector or a named list containing input arguments
  of the algorithm (see section `Algorithm Argument Syntax` below).

- setVectorArgsFromObject:

  Logical value, `TRUE` to set algorithm arguments automatically when
  the `"input"` argument or the `"like"` argument is an object of class
  `GDALVector` (the default). Can be set to `FALSE` to disable
  automatically setting algorithm arguments from `GDALVector` input (see
  Note).

- parse:

  Logical value, `TRUE` to attempt parsing `args` if they are given in
  `gdal_alg()` (the default). Set to `FALSE` to instantiate the
  algorithm without parsing arguments. The `$parseCommandLineArgs()`
  method on the returned object can be called to parse arguments and
  obtain the result of that, with potentially useful error messages.

## Details

These functions provide an interface to GDAL CLI algorithms by way of
the C++ exposed class
[`GDALAlg`](https://firelab.github.io/gdalraster/reference/GDALAlg-class.md).
See the class documentation for additional information
([`?GDALAlg`](https://firelab.github.io/gdalraster/reference/GDALAlg-class.md)).

`gdal_commands()` prints a list of commands and their descriptions to
the console, and returns (invisibly) a data frame with columns
`command`, `description`, `URL`. The `contains` argument can be used to
filter the output, e.g., `gdal_commands("vector")` to return only
commands for working with vector inputs.

`gdal_usage()` prints a help message to the console for a given command,
or for the root `gdal` entry point if called with no argument. No return
value, called for that side effect only.

`gdal_run()` executes a GDAL CLI algorithm and returns it as an object
of class
[`GDALAlg`](https://firelab.github.io/gdalraster/reference/GDALAlg-class.md).
A list containing algorithm output(s) can be accessed by calling the
`$outputs()` method (plural) on the returned object, or, more
conveniently in most cases, by calling `$output()` (singular) to return
the the single output value when there is only one. After assigning the
output, or otherwise completing work with the `GDALAlg` object, its
`$release()` method can be called to close datasets and free resources.

`gdal_alg()` instantiates and returns an object of class
[`GDALAlg`](https://firelab.github.io/gdalraster/reference/GDALAlg-class.md)
without running it. Passing argument values to the requested CLI
algorithm is optional. This function may be useful (with or without
argument values) for obtaining algorithm properties with the returned
object's `$info()` method, obtaining properties of algorithm arguments
(`$argInfo(arg_name)`), or obtaining algorithm usage as a JSON-formatted
string (`$usageAsJSON()`). This function is simply an alternative to
calling the `new()` constructor for class `GDALAlg`. Executing the
returned algorithm is optional by calling the object's `$run()` method
(assuming argument values were given).

`gdal_global_reg_names()` returns a character vector containing the
names of the algorithms in the GDAL global algorithm registry. These are
the top-level nodes (`raster`, `vector`, etc.) known to GDAL.
Potentially code external to GDAL could register a new command available
for CLI use in a GDAL plugin. This function may be useful in certain
troubleshooting scenarios. It will return a vector of length zero if no
names are returned from the global registry.

## Note

Commands do not require the leading `"gdal"` root node. They may begin
with a top-level command (e.g., `"raster"`, `"vector"`, etc.).

When using argument names as the element names of a list, the underscore
character can be substituted for the dash characters that are used in
some names. This avoids having to surround names in backticks when they
are used to access list elements in the form `args$arg_name` (the form
`args[["arg-name"]]` also works).

When `setVectorArgsFromObject` is `TRUE` (the default) and the `"input"`
or `"like"` argument for an algorithm is given as a `GDALVector` object,
corresponding algorithm arguments will be set automatically based on
properties of the object (when the argument is available to the
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

Argument values specified explicitly will override the automatic setting
(as long as they result in a parsable set of arguments). If
`setVectorArgsFromObject` is `FALSE`, then only the vector dataset is
passed to the algorithm, i.e., without automatically passing any layer
specifications.

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

[`GDALAlg-class`](https://firelab.github.io/gdalraster/reference/GDALAlg-class.md)

`gdal` Command Line Interface (CLI)  
<https://gdal.org/en/stable/programs/index.html>

Using `gdal` CLI algorithms from R  
<https://firelab.github.io/gdalraster/articles/use-gdal-cli-from-r.html>

## Examples

``` r
if (FALSE) { # length(gdal_global_reg_names()) > 0
## top-level commands
gdal_commands(recurse = FALSE)

## convert storml_elev.tif to GeoPackage raster
gdal_commands("convert")

gdal_usage("raster convert")

f_tif <- system.file("extdata/storml_elev.tif", package="gdalraster")
f_gpkg <- file.path(tempdir(), "storml_elev.gpkg")

args <- c("--overwrite", f_tif, f_gpkg)
(alg <- gdal_run("raster convert", args))

(ds <- alg$output())

alg$release()

plot_raster(ds, legend = TRUE)

ds$close()
unlink(f_gpkg)

## get help for vector commands
gdal_usage("vector")

## clip a vector layer by a bounding box
gdal_usage("vector clip")

f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
f_clip <- file.path(tempdir(), "ynp_fires_clip.gpkg")

# some multi-valued arguments require all values packed and comma separated
# e.g., --bbox <BBOX>
bb <- c(469686, 11442, 544070, 85508)
bb <- paste(bb, collapse = ",")

args <- c("--bbox", bb, "--overwrite", f, f_clip)
(alg <- gdal_run("vector clip", args))

(lyr <- alg$output())

lyr$bbox()

lyr$getFeatureCount()

lyr$close()
alg$release()
unlink(f_clip)

## rasterize a vector layer and return output as a GDALRaster object
gdal_usage("vector rasterize")

f_out = file.path(tempdir(), "ynp_fire_year.tif")

# arguments in list format
args <- list(input = f,
             sql = "SELECT * FROM mtbs_perims ORDER BY ig_year",
             attribute_name = "ig_year",
             output = f_out,
             overwrite = TRUE,
             creation_option = c("TILED=YES", "COMPRESS=DEFLATE"),
             resolution = c(90, 90),
             output_data_type = "Int16",
             init = -32767,
             nodata = -32767)

(alg <- gdal_run("vector rasterize", args))

(ds <- alg$output())

alg$release()

pal <- scales::viridis_pal(end = 0.8, direction = -1)(6)
ramp <- scales::colour_ramp(pal)
plot_raster(ds, legend = TRUE, col_map_fn = ramp, na_col = "#d9d9d9",
            main = "YNP Fires 1984-2022 - Most Recent Burn Year")

ds$close()
deleteDataset(f_out)

## pipeline syntax
# "raster pipeline" example 2 from:
# https://gdal.org/en/latest/programs/gdal_raster_pipeline.html
# serialize the command to reproject a GTiff file into GDALG format, and
# then later read the GDALG file
# GDAL Streamed Algorithm format:
# https://gdal.org/en/stable/drivers/raster/gdalg.html

gdal_usage("raster pipeline")

f_tif <- system.file("extdata/storml_elev.tif", package="gdalraster")
f_out <- file.path(tempdir(), "storml_elev_epsg_32100.gdalg.json")

args <- c("read", "--input", f_tif, "!",
          "reproject", "--dst-crs=EPSG:32100", "!",
          "write", "--output", f_out, "--overwrite")

alg <- gdal_run("raster pipeline", args)
alg$release()

# content of the .gdalg.json file
readLines(f_out, warn = FALSE) |> writeLines()

(ds <- new(GDALRaster, f_out))

plot_raster(ds, legend = TRUE)

ds$close()
unlink(f_out)
}
```
