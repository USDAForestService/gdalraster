#' @name GDALAlg-class
#'
#' @aliases
#' Rcpp_GDALAlg Rcpp_GDALAlg-class GDALAlg
#'
#' @title Class encapsulating a GDAL CLI algorithm
#'
#' @description
#' `GDALAlg` provides bindings to `GDALAlgorithm` and related classes
#' that implement the `gdal` command line interface (CLI) in the GDAL
#' API. An object of class `GDALAlg` represents an instance of a CLI algorithm
#' with methods to obtain algorithm information and argument information, run
#' the algorithm, and access its output.
#'
#' **Requires GDAL >= 3.11.3**.
#'
#' **Experimental** (see the section `Development Status` below)
#'
#' `GDALAlg` is a C++ class exposed directly to \R (via `RCPP_EXPOSED_CLASS`).
#' Fields and methods of the class are accessed using the `$` operator.
#' Arguments to class constructors and class methods must be given in the order
#' documented (naming optional).
#'
#' @param cmd A character string or character vector containing the path to the
#' algorithm, e.g., `"raster reproject"` or `c("raster", "reproject")`.
#' @param args Either a character vector or a named list containing input
#' arguments of the algorithm (see section `Algorithm Argument Syntax` below).
#' @returns An object of class `GDALAlg`, which contains a pointer to the
#' algorithm instance. Class methods are described in Details, along with a set
#' of writable fields for per-object settings.
#'
#' @inheritSection gdal_cli Algorithm Argument Syntax
#'
#' @inheritSection gdal_cli Development Status
#'
#' @section Usage (see Details):
#' ```
#' ## Constructors
#' alg <- new(GDALAlg, cmd)
#' # or, with arguments
#' alg <- new(GDALAlg, cmd, args)
#'
#' ## Read/write fields (per-object settings)
#' alg$setVectorArgsFromObject
#' alg$outputLayerNameForOpen
#' alg$quiet
#'
#' ## Methods
#' alg$info()
#' alg$argInfo(arg_name)
#' alg$usage()
#' alg$usageAsJSON()
#'
#' alg$parseCommandLineArgs()
#' alg$run()
#' alg$output()
#' alg$outputs()
#'
#' alg$close()
#' alg$release()
#' ```
#' @section Details:
#' ## Constructors
#'
#' \code{new(GDALAlg, cmd)}\cr
#' Instantiate an algorithm without specifying input arguments.
#'
#' \code{new(GDALAlg, cmd, args)}\cr
#' Instantiate an algorithm giving input arguments as a character vector or
#' named list. See the section `Algorithm Argument Syntax` for details.
#'
#' ## Read/write fields (per-object settings)
#'
#' \code{$setVectorArgsFromObject}\cr
#' A logical value, `TRUE` (the default) to set algorithm arguments
#' automatically when the `"input"` argument or the `"like"` argument is an
#' object of class `GDALVector`. Argument values specified explicitly will
#' override the automatic setting (as long as they result in a parsable set of
#' arguments). Automatically setting arguments from `GDALVector` input can
#' be disabled by setting this field to `FALSE`.
#' When enabled and the `"input"` or `"like"` argument for an algorithm is a
#' `GDALVector` object, the following arguments will be set automatically based
#' on properties of the object (when the argument is available to the
#' algorithm):
#' * `"input-format"`: set to the object's driver short name
#' * `"input-layer"`: set to the object's layer name if it is not a SQL layer
#' * `"sql"`: set to the SQL statement if the object's layer is defined by one
#' * `"dialect"`: set to the SQL dialect if one is specified for a SQL layer
#' * `"like-layer"`: set to the `GDALVector` layer name if it is not a SQL
#' layer
#' * `"like-sql"`: set to the SQL statement if the `GDALVector` layer is
#' defined by one
#'
#' \code{$outputLayerNameForOpen}\cr
#' A character string specifying a layer name to open when obtaining algorithm
#' output as an object of class `GDALVector`. See method \code{$output()} below.
#' The default value is empty string (`""`) in which case the first layer by
#' index is opened. Ignored if output is not a vector dataset.
#'
#' \code{$quiet}\cr
#' A logical value, `FALSE` by default. Set to `TRUE` to suppress progress
#' reporting along with various messages and warnings.
#'
#' ## Methods
#'
#' \code{$info()}\cr
#' Returns a named list of algorithm properties with the following elements:
#' * `name`: character string, the algorithm name
#' * `description`: character string, the algorithm (short) description
#' * `long_description`: character string, the algorithm longer description
#' * `URL`: character string, the algorithm help URL
#' * `has_subalgorithms`: logical, `TRUE` if the algorithm has sub-algorithms
#' * `subalgorithm_names`: character vector of sub-algorithm names (may be
#' empty)
#' * `arg_names`: character vector of available argument names
#'
#' \code{$argInfo(arg_name)}\cr
#' Returns a named list of properties for an algorithm argument given as a
#' character string, with the following elements:
#' * `name`: character string, the name of the argument
#' * `type`: character string, the argument type as one of `"BOOLEAN"`,
#' `"STRING"`, `"INTEGER"`, `"REAL"`, `"DATASET"`, `"STRING_LIST"`,
#' `"INTEGER_LIST"`, `"REAL_LIST"`, `"DATASET_LIST"`
#' * `description`: character string, the argument description
#' * `short_name`: character string, the short name or empty string if there
#' is none
#' * `aliases`: character vector of aliases (empty if none)
#' * `meta_var`: character string, the "meta-var" hint (by default, the
#' meta-var value is the long name of the argument in upper case)
#' * `category`: character string, the argument category
#' * `is_positional`: logical, `TRUE` if the argument is a positional one
#' * `is_required`: logical, `TRUE` if the argument is required
#' * `min_count`: integer, the minimum number of values for the argument (only
#' applies to list type of arguments)
#' * `max_count`: integer, the maximum number of values for the argument (only
#' applies to list type of arguments)
#' * `packed_values_allowed`: logical, `TRUE` if, for list type of arguments,
#' several comma-separated values may be specified (i.e., `"--foo=bar,baz"`)
#' * `repeated_arg_allowed`: logical, `TRUE` if, for list type of arguments,
#' the argument may be repeated (i.e., `c("--foo=bar", "--foo=baz")`)
#' * `choices`: character vector of allowed values for the argument (may be
#' empty and only applies for argument types `"STRING"` and `"STRING_LIST"`)
#' * `is_explicitly_set`: logical, `TRUE` if the argument value has been
#' explicitly set
#' * `has_default_value`: logical, `TRUE` if the argument has a declared
#' default value
#' * `default_value`: character or integer or numeric, the default value if the
#' argument has one, otherwise `NULL`
#' * `is_hidden_for_api`: logical, `TRUE` if the argument is only for CLI usage
#' (e.g., "--help") (GDAL >= 3.12 only, see `is_only_for_cli`)
#' * `is_hidden_for_cli`: logical, `TRUE` if the argument must not be
#' mentioned in CLI usage (e.g., "output-value" for "gdal raster info", which
#' is only meant when the algorithm is used from a non-CLI context such as
#' programmatically from \R)
#' * `is_only_for_cli`: logical, `TRUE` if the argument is only for CLI usage
#' (e.g., "--help") (deprecated at GDAL 3.12, see `is_hidden_for_api`)
#' * `is_input`: logical, `TRUE` if the value of the argument is read-only
#' during the execution of the algorithm
#' * `is_output`: logical, `TRUE` if (at least part of) the value of the
#' argument is set during the execution of the algorithm
#' * `dataset_type_flags`: character vector containing strings `"RASTER"`,
#' `"VECTOR"`, `"MULTIDIM_RASTER"`, possibly with `"UPDATE"` (`NULL` if
#' the argument is not a dataset type)
#' * `dataset_input_flags`: character vector indicating if a dataset argument
#' supports specifying only the dataset name (`"NAME"`), only the dataset
#' object (`"OBJECT"`), or both (`"NAME", "OBJECT"`) when it is used as an
#' input (`NULL` if the argument is not a dataset type)
#' * `dataset_output_flags`: character vector indicating if a dataset argument
#' supports specifying only the dataset name (`"NAME"`), only the dataset
#' object (`"OBJECT"`), or both (`"NAME", "OBJECT"`) when it is used as an
#' output (`NULL` if the argument is not a dataset type)
#' * `mutual_exclusion_group`: character string, the name of the mutual
#' exclusion group to which this argument belongs
#'
#' \code{$usage()}\cr
#' Print a help message for the algorithm to the console. No return value.
#'
#' \code{$usageAsJSON()}\cr
#' Returns the usage of the algorithm as a JSON-serialized string.
#'
#' \code{$parseCommandLineArgs()}\cr
#' Sets the value of arguments previously specified in the class constructor,
#' and instantiates the actual algorithm that will be run (but without running
#' it). Returns a logical value, `TRUE` indicating success or `FALSE` if an
#' error occurs.
#'
#' \code{$run()}\cr
#' Executes the algorithm, first parsing arguments if
#' \code{$parseCommandLineArgs()} has not already been called explicitly.
#' Returns a logical value, `TRUE` indicating success or `FALSE` if an error
#' occurs.
#'
#' \code{$output()}\cr
#' Returns the single output value of the algorithm, after it has been run.
#' If there are multiple output values, this method will raise an error, and
#' the \code{$outputs()} (plural) method should be called instead. The type of
#' the return value corresponds to the type of the single output argument value
#' (see method \code{$argInfo()} above).
#' If the output argument has type `"DATASET"`, an object of class `GDALRaster`
#' will be returned if the dataset is raster, or an object of class
#' `GDALVector` if the dataset is vector. In the latter case, by default the
#' `GDALVector` object will be opened on the first layer by index, but a
#' specific layer name may be specified by setting the value of the field
#' \code{$outputLayerNameForOpen} before calling the \code{$output()} method
#' (see above).
#' Note that currently, if the output dataset is multidimensional raster, only
#' the dataset name will be returned as a character string.
#'
#' \code{$outputs()}\cr
#' Returns the output value(s) of the algorithm as a named list, after it has
#' been run. Most algorithms only return a single output, in which case the
#' \code{$output()} method (singular) is preferable for easier use. The element
#' names in the returned list are the names of the arguments that have outputs
#' (with any dash characters replaced by underscore), and the values are the
#' argument values which may include `GDALRaster` or `GDALVector` objects.
#'
#' \code{$close()}\cr
#' Completes any pending actions, and returns the final status as a logical
#' value (`TRUE` if no errors occur during the underlying call to
#' `GDALAlgorithmFinalize()`). This is typically useful for algorithms that
#' generate an output dataset. It closes datasets and gets back potential error
#' status resulting from that, e.g., if an error occurs during flushing to disk
#' of the output dataset after successful \code{$run()} execution.
#'
#' \code{$release()}\cr
#' Release memory associated with the algorithm, potentially after attempting
#' to finalize. No return value, called for side-effects.
#'
#' @seealso
#' [gdal_alg()], [gdal_commands()], [gdal_run()], [gdal_usage()]
#'
#' Using `gdal` CLI algorithms from R\cr
#' \url{https://usdaforestservice.github.io/gdalraster/articles/use-gdal-cli-from-r.html}
#'
#' @examplesIf length(gdal_global_reg_names()) > 0
#' f <- system.file("extdata/storml_elev.tif", package="gdalraster")
#'
#' ## raster info
#' gdal_usage("raster info")
#'
#' # arguments given as character vector
#' args <- c("--format=text", "--no-md", f)
#' (alg <- new(GDALAlg, "raster info", args))
#'
#' alg$run()
#'
#' alg$output() |> writeLines()
#'
#' alg$release()
#'
#' ## raster clip
#' gdal_usage("raster clip")
#'
#' # arguments in list format
#' args <- list()
#' args$input <- f
#' f_clip <- file.path(tempdir(), "elev_clip.tif")
#' args$output <- f_clip
#' args$overwrite <- TRUE
#' args$bbox <- c(323776.1, 5102172.0,  327466.1, 5104782.0)
#'
#' (alg <- new(GDALAlg, "raster clip", args))
#'
#' alg$run()
#'
#' # get output as a GDALRaster object
#' (ds_clip <- alg$output())
#'
#' alg$release()
#'
#' ds_clip$dim()
#' ds_clip$bbox()
#'
#' ## raster hillshade
#' gdal_usage("raster hillshade")
#'
#' # input as a GDALRaster object and output to an in-memory raster
#' args <- list(input = ds_clip,
#'              output_format = "MEM",
#'              output = "")
#'
#' (alg <- new(GDALAlg, "raster hillshade", args))
#'
#' alg$run()
#'
#' (ds_hillshade <- alg$output())
#'
#' alg$release()
#'
#' plot_raster(ds_hillshade)
#'
#' ds_clip$close()
#' ds_hillshade$close()
#' deleteDataset(f_clip)
#'
#' ## vector convert shapefile to GeoPackage
#' gdal_usage("vector convert")
#'
#' f_shp <- system.file("extdata/poly_multipoly.shp", package="gdalraster")
#' f_gpkg <- file.path(tempdir(), "polygons_test.gpkg")
#' args <- c("--input", f_shp, "--output", f_gpkg, "--overwrite")
#'
#' (alg <- new(GDALAlg, "vector convert", args))
#'
#' alg$run()
#'
#' # output as a GDALVector object
#' (lyr <- alg$output())
#'
#' alg$release()
#'
#' lyr$info()
#'
#' lyr$close()
#' deleteDataset(f_gpkg)
#'
#' ## raster reproject (usage and info)
#' # no arguments given, only for retrieving usage and properties via API
#' # cf. gdal_usage("raster reproject")
#'
#' (alg <- new(GDALAlg, "raster reproject"))
#'
#' # print usage to the console, no return value
#' alg$usage()
#'
#' # or, get usage as a JSON string
#' # json <- alg$usageAsJSON()
#' # writeLines(json)
#'
#' # list of algorithm properties
#' alg$info()
#'
#' # list of properties for an algorithm argument
#' alg$argInfo("resampling")
#'
#' alg$release()
NULL

Rcpp::loadModule("mod_GDALAlg", TRUE)
