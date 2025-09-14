# R interface to the GDAL CLI algorithms
# functions for using exposed class `GDALAlg`
# see R/gdalalg.R and src/gdalalg.h
# Chris Toney <jctoney at gmail.com>

#' Functions for using GDAL CLI algorithms
#'
#' @name gdal_cli
#' @description
#' This set of functions can be used to access and run `gdal` command line
#' interface (CLI) algorithms.
#'
#' **Requires GDAL >= 3.11.3**
#'
#' **Experimental** (see the section `Development Status` below)
#'
#' @details
#' These functions provide an interface to GDAL CLI algorithms by way of the
#' C++ exposed class [`GDALAlg`][GDALAlg]. See the class documentation for
#' additional information (`?GDALAlg`).
#'
#' `gdal_commands()` prints a list of commands and their descriptions to the
#' console, and returns (invisibly) a data frame with columns `command`,
#' `description`, `URL`. The `contains` argument can be used to filter the
#' output, e.g., `gdal_commands("vector")` to return only commands for working
#' with vector inputs.
#'
#' `gdal_usage()` prints a help message to the console for a given command, or
#' for the root `gdal` entry point if called with no argument. No return value,
#' called for that side effect only.
#'
#' `gdal_run()` executes a GDAL CLI algorithm and returns it as an object of
#' class [`GDALAlg`][GDALAlg]. A list containing algorithm output(s) can be
#' accessed by calling the \code{$outputs()} method (plural) on the returned
#' object, or, more conveniently in most cases, by calling \code{$output()}
#' (singular) to return the the single output value when there is only one.
#' After assigning the output, or otherwise completing work with the `GDALAlg`
#' object, its \code{$release()} method can be called to close datasets and
#' free resources.
#'
#' `gdal_alg()` instantiates and returns an object of class [`GDALAlg`][GDALAlg]
#' without running it. Passing argument values to the requested CLI algorithm is
#' optional. This function may be useful (with or without argument values) for
#' obtaining algorithm properties with the returned object's \code{$info()}
#' method, obtaining properties of algorithm arguments
#' (\code{$argInfo(arg_name)}), or obtaining algorithm usage as a
#' JSON-formatted string (\code{$usageAsJSON()}).
#' This function is simply an alternative to calling the `new()` constructor
#' for class `GDALAlg`. Executing the returned algorithm is optional by calling
#' the object's \code{$run()} method (assuming argument values were given).
#'
#' `gdal_global_reg_names()` returns a character vector containing the names of
#' the algorithms in the GDAL global algorithm registry. These are the
#' top-level nodes (`raster`, `vector`, etc.) known to GDAL. Potentially code
#' external to GDAL could register a new command available for CLI use in a
#' GDAL plugin. This function may be useful in certain troubleshooting
#' scenarios. It will return a vector of length zero if no names are returned
#' from the global registry.
#'
#' @param contains Optional character string for filtering output to certain
#' commands, e.g., `gdal_commands("vector")`.
#' @param recurse Logical value, `TRUE` to include all subcommands recursively
#' (the default). Set to `FALSE` to include only the top-level `gdal` commands
#' (i.e., `raster`, `vector`, etc.)
#' @param cout Logical value, `TRUE` to print a list of commands along with
#' their descriptions and help URLS to the console (the default).
#' @param cmd A character string or character vector containing the path to the
#' algorithm, e.g., `"raster reproject"` or `c("raster", "reproject")`.
#' Defaults to `"gdal"`, the main entry point to CLI commands.
#' @param args Either a character vector or a named list containing input
#' arguments of the algorithm (see section `Algorithm Argument Syntax` below).
#' @param setVectorArgsFromObject Logical value, `TRUE` to set algorithm
#' arguments automatically when the `"input"` argument or the `"like"` argument
#' is an object of class `GDALVector` (the default). Can be set to `FALSE` to
#' disable automatically setting algorithm arguments from `GDALVector` input
#' (see Note).
#' @param parse Logical value, `TRUE` to attempt parsing `args` if they are
#' given in `gdal_alg()` (the default). Set to `FALSE` to instantiate the
#' algorithm without parsing arguments. The \code{$parseCommandLineArgs()}
#' method on the returned object can be called to parse arguments and obtain
#' the result of that, with potentially useful error messages.
#'
#' @section Algorithm Argument Syntax:
#' Arguments are given in \R as a character vector or named list, but
#' otherwise syntax basically matches the GDAL specification for arguments as
#' they are given on the command line. Those specifications are listed here
#' along with some amendments regarding the character vector and named list
#' formats. Programmatic usage also allows passing and receiving datasets as
#' objects (i.e., `GDALRaster` or `GDALVector`), in addition to dataset names
#' (e.g., filename, URL, database connection string).
#' * Commands accept one or several positional arguments, typically for dataset
#' names (or in \R as `GDALRaster` or `GDALVector` datasets). The order is
#' input(s) first, output last. Positional arguments can also be specified as
#' named arguments, if preferred to avoid any ambiguity.
#' * Named arguments have:
#'   * at least one "long" name, preceded by two dash characters
#'   * optionally, auxiliary long names (i.e., aliases),
#'   * and optionally a one-letter short name, preceded by a single dash
#'   character, e.g., \code{-f, --of, --format, --output-format <OUTPUT-FORMAT>}
#' * Boolean arguments are specified by just specifying the argument name in
#' character vector format. In \R `list` format, the named element must be
#' assigned a value of logical `TRUE`.
#' * Arguments that require a value are specified like:
#'   * \code{-f VALUE} for one-letter short names
#'   * \code{--format VALUE} or \code{--format=VALUE} for long names
#'   * in a named list, this might look like: \code{args$format <- VALUE}
#' * Some arguments can be multi-valued. Some of them require all values to be
#' packed together and separated with comma. This is, e.g., the case of:\cr
#' \code{--bbox <BBOX> Clipping bounding box as xmin,ymin,xmax,ymax} \cr
#' e.g., \code{--bbox=2.1,49.1,2.9,49.9}
#' * Others accept each value to be preceded by a new mention of the argument
#' name, e.g., \code{c("--co", "COMPRESS=LZW", "--co", "TILED=YES")}. For that
#' one, if the value of the argument does not contain commas, the packed form
#' is also accepted: \code{--co COMPRESS=LZW,TILED=YES}. Note that repeated
#' mentions of an argument are possible in the character vector format for
#' argument input, whereas arguments given in named list format must use
#' argument long names as the list element names, and the packed format for the
#' values (which can be a character vector or numeric vector of values).
#' * Named arguments can be placed before or after positional arguments.
#'
#' @section Development Status:
#' The GDAL Command Line Interface Modernization was first introduced in the
#' [GDAL 3.11.0 release](https://github.com/OSGeo/gdal/releases/tag/v3.11.0)
#' (2025-05-09). The GDAL project provides warning that the new CLI "is
#' provisionally provided as an alternative interface to GDAL and OGR command
#' line utilities. The project reserves the right to modify, rename,
#' reorganize, and change the behavior until it is officially frozen via PSC
#' vote in a future major GDAL release. The utility needs time to mature,
#' benefit from incremental feedback, and explore enhancements without carrying
#' the burden of full backward compatibility. Your usage of it should have no
#' expectation of compatibility until that time."
#' (\url{https://gdal.org/en/latest/programs/#gdal-application})
#'
#' Initial bindings to enable programmatic use of the CLI algorithms from \R
#' were added in \pkg{gdalraster} 2.2.0, and will evolve over future releases.
#' *The bindings are considered experimental until the upstream API is declared
#' stable*. Breaking changes in minor version releases are possible until then.
#' Please use with those cautions in mind. Bug reports may be filed at:
#' \url{https://github.com/USDAForestService/gdalraster/issues}.
#'
#' @note
#' Commands do not require the leading `"gdal"` root node. They may begin
#' with a top-level command (e.g., `"raster"`, `"vector"`, etc.).
#'
#' When using argument names as the element names of a list, the underscore
#' character can be substituted for the dash characters that are used in some
#' names. This avoids having to surround names in backticks when they are used
#' to access list elements in the form \code{args$arg_name} (the form
#' \code{args[["arg-name"]]} also works).
#'
#' When `setVectorArgsFromObject` is `TRUE` (the default) and the `"input"` or
#' `"like"` argument for an algorithm is given as a `GDALVector` object,
#' corresponding algorithm arguments will be set automatically based on
#' properties of the object (when the argument is available to the algorithm):
#' * `"input-format"`: set to the `GDALVector` object's driver short name
#' * `"input-layer"`: set to the `GDALVector` layer name if it is not a SQL
#' layer
#' * `"sql"`: set to the SQL statement if the `GDALVector` layer is defined by
#' one
#' * `"dialect"`: set to the SQL dialect if one is specified for a SQL layer
#' * `"like-layer"`: set to the `GDALVector` layer name if it is not a SQL
#' layer
#' * `"like-sql"`: set to the SQL statement if the `GDALVector` layer is
#' defined by one
#'
#' Argument values specified explicitly will override the automatic setting (as
#' long as they result in a parsable set of arguments). If
#' `setVectorArgsFromObject` is `FALSE`, then only the vector dataset is passed
#' to the algorithm, i.e., without automatically passing any layer
#' specifications.
#'
#' @seealso
#' [`GDALAlg-class`][GDALAlg]
#'
#' `gdal` Command Line Interface (CLI) \cr
#' \url{https://gdal.org/en/stable/programs/index.html}
#'
#' Using `gdal` CLI algorithms from R\cr
#' \url{https://usdaforestservice.github.io/gdalraster/articles/use-gdal-cli-from-r.html}
#'
#' @examplesIf length(gdal_global_reg_names()) > 0
#' ## top-level commands
#' gdal_commands(recurse = FALSE)
#'
#' ## convert storml_elev.tif to GeoPackage raster
#' gdal_commands("convert")
#'
#' gdal_usage("raster convert")
#'
#' f_tif <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' f_gpkg <- file.path(tempdir(), "storml_elev.gpkg")
#'
#' args <- c("--overwrite", f_tif, f_gpkg)
#' (alg <- gdal_run("raster convert", args))
#'
#' (ds <- alg$output())
#'
#' alg$release()
#'
#' plot_raster(ds, legend = TRUE)
#'
#' ds$close()
#' unlink(f_gpkg)
#'
#' ## get help for vector commands
#' gdal_usage("vector")
#'
#' ## clip a vector layer by a bounding box
#' gdal_usage("vector clip")
#'
#' f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
#' f_clip <- file.path(tempdir(), "ynp_fires_clip.gpkg")
#'
#' # some multi-valued arguments require all values packed and comma separated
#' # e.g., --bbox <BBOX>
#' bb <- c(469686, 11442, 544070, 85508)
#' bb <- paste(bb, collapse = ",")
#'
#' args <- c("--bbox", bb, "--overwrite", f, f_clip)
#' (alg <- gdal_run("vector clip", args))
#'
#' (lyr <- alg$output())
#'
#' lyr$bbox()
#'
#' lyr$getFeatureCount()
#'
#' lyr$close()
#' alg$release()
#' unlink(f_clip)
#'
#' ## rasterize a vector layer and return output as a GDALRaster object
#' gdal_usage("vector rasterize")
#'
#' f_out = file.path(tempdir(), "ynp_fire_year.tif")
#'
#' # arguments in list format
#' args <- list(input = f,
#'              sql = "SELECT * FROM mtbs_perims ORDER BY ig_year",
#'              attribute_name = "ig_year",
#'              output = f_out,
#'              overwrite = TRUE,
#'              creation_option = c("TILED=YES", "COMPRESS=DEFLATE"),
#'              resolution = c(90, 90),
#'              output_data_type = "Int16",
#'              init = -32767,
#'              nodata = -32767)
#'
#' (alg <- gdal_run("vector rasterize", args))
#'
#' (ds <- alg$output())
#'
#' alg$release()
#'
#' pal <- scales::viridis_pal(end = 0.8, direction = -1)(6)
#' ramp <- scales::colour_ramp(pal)
#' plot_raster(ds, legend = TRUE, col_map_fn = ramp, na_col = "#d9d9d9",
#'             main = "YNP Fires 1984-2022 - Most Recent Burn Year")
#'
#' ds$close()
#' deleteDataset(f_out)
#'
#' ## pipeline syntax
#' # "raster pipeline" example 2 from:
#' # https://gdal.org/en/latest/programs/gdal_raster_pipeline.html
#' # serialize the command to reproject a GTiff file into GDALG format, and
#' # then later read the GDALG file
#' # GDAL Streamed Algorithm format:
#' # https://gdal.org/en/stable/drivers/raster/gdalg.html
#'
#' gdal_usage("raster pipeline")
#'
#' f_tif <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' f_out <- file.path(tempdir(), "storml_elev_epsg_32100.gdalg.json")
#'
#' args <- c("read", "--input", f_tif, "!",
#'           "reproject", "--dst-crs=EPSG:32100", "!",
#'           "write", "--output", f_out, "--overwrite")
#'
#' alg <- gdal_run("raster pipeline", args)
#' alg$release()
#'
#' # content of the .gdalg.json file
#' readLines(f_out, warn = FALSE) |> writeLines()
#'
#' (ds <- new(GDALRaster, f_out))
#'
#' plot_raster(ds, legend = TRUE)
#'
#' ds$close()
#' unlink(f_out)
#' @export
gdal_commands <- function(contains = NULL, recurse = TRUE, cout = TRUE) {
    if (gdal_version_num() < gdal_compute_version(3, 11, 3)) {
        stop("gdal_commands() requires GDAL >= 3.11.3", call. = FALSE)
    }

    if (missing(contains) || is.null(contains) || all(is.na(contains)))
        contains <- ""
    if (!is.character(contains))
        stop("'contains' must be a character string", call. = FALSE)
    else if (length(contains) > 1)
        contains <- paste(contains, collapse = " ")

    if (missing(recurse) || is.null(recurse) || all(is.na(recurse)))
        recurse <- TRUE
    if (!(is.logical(recurse) && length(recurse) == 1))
        stop("'recurse' must be a length-1 logical vector", call. = FALSE)

    if (missing(cout) || is.null(cout) || all(is.na(cout)))
        cout <- TRUE
    if (!(is.logical(cout) && length(cout) == 1))
        stop("'cout' must be a length-1 logical vector", call. = FALSE)

    return(invisible(.gdal_commands(contains, recurse, cout)))
}

#' @name gdal_cli
#' @export
gdal_usage <- function(cmd = NULL) {
    if (gdal_version_num() < gdal_compute_version(3, 11, 3)) {
        stop("gdal_usage() requires GDAL >= 3.11.3", call. = FALSE)
    }

    alg <- gdal_alg(cmd)
    alg$usage()
    alg$release()
}

#' @name gdal_cli
#' @export
gdal_run <- function(cmd, args, setVectorArgsFromObject = TRUE) {
    if (gdal_version_num() < gdal_compute_version(3, 11, 3)) {
        stop("gdal_run() requires GDAL >= 3.11.3", call. = FALSE)
    }

    if (missing(cmd) || is.null(cmd) || all(is.na(cmd)))
        stop("'cmd' is required", call. = FALSE)
    if (!is.character(cmd))
        stop("'cmd' must be a character vector", call. = FALSE)

    if (missing(args) || is.null(args) || all(is.na(args)))
        stop("'args' is required", call. = FALSE)
    if (!is.character(args) && !is.list(args))
        stop("'args' must be a character vector or named list", call. = FALSE)

    if (missing(setVectorArgsFromObject) || is.null(setVectorArgsFromObject) ||
        all(is.na(setVectorArgsFromObject))) {

        setVectorArgsFromObject <- TRUE
    }
    if (!is.logical(setVectorArgsFromObject) ||
        length(setVectorArgsFromObject) != 1) {

        stop("'setVectorArgsFromObject' must be a single logical value",
             call. = FALSE)
    }

    alg <- new(GDALAlg, cmd, args)
    alg$setVectorArgsFromObject <- setVectorArgsFromObject

    if (!alg$parseCommandLineArgs()) {
        cat("parseCommandLineArgs() failed\n")
        alg$release()
        stop("failed to parse arguments and set their values", call. = FALSE)
    }

    if (!alg$run()) {
        cat("run() failed\n")
        alg$release()
        stop("failed to execute the command", call. = FALSE)
    }

    return(alg)
}

#' @name gdal_cli
#' @export
gdal_alg <- function(cmd = NULL, args = NULL, parse = TRUE) {
    if (gdal_version_num() < gdal_compute_version(3, 11, 3)) {
        stop("gdal_alg() requires GDAL >= 3.11.3", call. = FALSE)
    }

    if (missing(cmd) || is.null(cmd) || all(is.na(cmd)))
        cmd <- "gdal"
    if (!is.character(cmd))
        stop("'cmd' must be a character vector", call. = FALSE)

    if (missing(parse) || is.null(parse) || all(is.na(parse)))
        parse <- TRUE
    if (!(is.logical(parse) && length(parse) == 1))
        stop("'parse' must be a length-1 logical vector", call. = FALSE)

    has_args <- FALSE
    if (!is.null(args)) {
        if (!is.character(args) && !is.list(args)) {
            stop("'args' must be a character vector or named list",
                 call. = FALSE)
        } else {
            has_args <- TRUE
        }
    }

    alg <- new(GDALAlg, cmd, args)

    if (has_args && parse) {
        if (!alg$parseCommandLineArgs()) {
            cat("parseCommandLineArgs() failed\n")
            alg$release()
            stop("failed to parse arguments and set their values",
                 call. = FALSE)
        }
    }

    return(alg)
}

#' @name gdal_cli
#' @export
gdal_global_reg_names <- function() {
    return(.gdal_global_reg_names())
}

# helper function to print usage to the console
# called from GDALAlg::usage() in src/gdalalg.cpp
#' @noRd
#' @export
.print_alg_usage <- function(cmd) {
    alg <- new(GDALAlg, cmd)
    alginfo <- alg$info()

    has_non_positionals <- FALSE
    positional_args <- character()
    for (nm in alginfo$arg_names) {
        arginfo <- alg$argInfo(nm)
        if (gdal_version_num() < gdal_compute_version(3, 12, 0)) {
            # GDAL < 3.12 use `is_only_for_cli`
            if (!arginfo$is_only_for_cli && !arginfo$is_positional) {
                has_non_positionals <- TRUE
            } else if (!arginfo$is_only_for_cli && arginfo$is_positional) {
                positional_args <- append(positional_args, nm)
            }
        } else {
            # GDAL >= 3.12 use `is_hidden_for_api` instead
            if (!arginfo$is_hidden_for_api && !arginfo$is_positional) {
                has_non_positionals <- TRUE
            } else if (!arginfo$is_hidden_for_api && arginfo$is_positional) {
                positional_args <- append(positional_args, nm)
            }
        }
    }

    cat("\nUsage:", cmd)

    if (alginfo$has_subalgorithms) {
        cat(" <SUBCOMMAND>")
        if (has_non_positionals) {
            cat(" [OPTIONS]")
        }
        cat("\nwhere <SUBCOMMAND> is one of:\n")

        max_name_len <- 0
        for (nm in alginfo$subalgorithm_names) {
            if (nchar(nm) > max_name_len)
                max_name_len <- nchar(nm)
        }
        for (nm in alginfo$subalgorithm_names) {
            num_add_spaces <- max_name_len - nchar(nm)
            cat("  - ", nm, rep_len(" ", num_add_spaces + 1), sep = "")
            cat(": ")
            sub_alg <- new(GDALAlg, c(cmd, nm))
            cat(sub_alg$info()$description)
            cat("\n")
            sub_alg$release()
        }
        cat("\n")
    } else {
        if (length(alginfo$arg_names) > 0) {
            if (has_non_positionals && alginfo$name != "pipeline") {
                cat(" [OPTIONS]")
            }
            for (arg_nm in positional_args) {
                arginfo <- alg$argInfo(arg_nm)
                optional <- (!arginfo$is_required &&
                                !(alginfo$name == "pipeline" &&
                                  arginfo$name == "pipeline"))

                cat(" ")
                if (optional)
                    cat("[")
                if (arginfo$meta_var != "") {
                    if (startsWith(arginfo$meta_var, "<")) {
                        cat(arginfo$meta_var)
                    } else {
                        cat("<", arginfo$meta_var, ">", sep = "")
                    }
                }
                if (arginfo$type == "DATASET_LIST" && arginfo$max_count > 1)
                    cat("...")
                if (optional)
                    cat("]")
            }
        }

        cat("\n\n")
        cat(alginfo$description, "\n\n")
    }

    print_arg <- function(arg_name) {
        this_arg <- alg$argInfo(arg_name)

        cat("  ")
        if (this_arg$short_name != "")
            cat("-", this_arg$short_name, ", ", sep = "")
        if (length(this_arg$aliases) > 0) {
            for (nm in this_arg$aliases) {
                if (nchar(nm) == 1) {
                    cat("-", nm, ", ", sep = "")
                } else {
                    cat("--", nm, ", ", sep = "")
                }
            }
        }
        if (this_arg$name != "")
            cat("--", this_arg$name, sep = "")
        if (this_arg$meta_var != "") {
            cat(" ")
            if (startsWith(this_arg$meta_var, "<")) {
                cat(this_arg$meta_var)
            } else {
                cat("<", this_arg$meta_var, ">", sep = "")
            }
        }
        cat("\n")

        cat("    ")
        cat(this_arg$description)
        cat("\n")

        if (length(this_arg$choices)) {
            cat("    ")
            cat("[", paste(this_arg$choices, collapse = "|"), "]", sep = "")
            cat("\n")
        }

        if (this_arg$type == "DATASET" || this_arg$type == "DATASET_LIST") {
            if (all(this_arg$dataset_input_flags == "NAME") &&
                    all(this_arg$dataset_output_flags == "OBJECT")) {

                cat("    [created by algorithm]\n")
            }
        }

        if (this_arg$has_default_value) {
            cat("    [default: ")
            cat(paste(this_arg$default_value, collapse = ", "))
            cat("]\n")
        }

        if (endsWith(this_arg$type, "LIST")) {
            if (this_arg$min_count > 0 &&
                this_arg$min_count == this_arg$max_count) {

                if (this_arg$min_count != 1)
                    cat("    [", this_arg$max_count, " values]\n", sep = "")

            } else if (this_arg$min_count > 0 &&
                       this_arg$max_count < .Machine$integer.max) {

                cat("    [", this_arg$min_count, "..", this_arg$max_count,
                    " values]\n", sep = "")

            } else if (this_arg$min_count > 0) {
                cat("    [", this_arg$min_count, "..", " values]\n", sep = "")
            } else if (this_arg$max_count > 1) {
                cat("    [may be repeated]\n")
            }
        }

        if (this_arg$is_required)
            cat("    [required]\n")

        if (nzchar(this_arg$mutual_exclusion_group)) {
            other_args <- character()
            for (nm in alginfo$arg_names) {
                other_arg <- alg$argInfo(nm)
                if (gdal_version_num() < gdal_compute_version(3, 12, 0)) {
                    # GDAL < 3.12 use `is_only_for_cli`
                    if (other_arg$is_only_for_cli || this_arg$name == nm)
                        next
                } else {
                    # GDAL >= 3.12 use `is_hidden_for_api` instead
                    if (other_arg$is_hidden_for_api || this_arg$name == nm)
                        next
                }
                if (other_arg$mutual_exclusion_group ==
                        this_arg$mutual_exclusion_group) {

                    other_args <- append(other_args,
                                         paste0("--", other_arg$name))
                }
            }
            if (length(other_args) > 0) {
                cat("    [mutually exclusive with ")
                cat(paste(other_args, collapse = ", "))
                cat("]\n")
            }
        }
    }

    if (length(positional_args) > 0) {
        cat("Positional arguments:\n")
        for (arg_nm in positional_args) {
            print_arg(arg_nm)
        }
        cat("\n")
    }

    if (alginfo$name == "pipeline") {
        cat("<PIPELINE> is of the form: ")
        if (isTRUE(grepl("vector", cmd, ignore.case = TRUE)))
            str_out <- "read|concat [READ-OPTIONS] ( ! <STEP-NAME> [STEP-OPTIONS] )* ! write [WRITE-OPTIONS]\n"
        else
            str_out <- "read [READ-OPTIONS] ( ! <STEP-NAME> [STEP-OPTIONS] )* ! write [WRITE-OPTIONS]\n"

        cat(str_out)
        cat("\n")
    }

    # non-positional args by category
    common_args <- character()
    base_args <- character()
    advanced_args <- character()
    esoteric_args <- character()
    for (nm in alginfo$arg_names) {
        arginfo <- alg$argInfo(nm)
        if ((isTRUE(!as.logical(arginfo$is_only_for_cli)) ||  # GDAL < 3.12
            isTRUE(!as.logical(arginfo$is_hidden_for_api)))  # GDAL >= 3.12
                && !arginfo$is_positional) {

            if (tolower(arginfo$category) == "common")
                common_args <- append(common_args, nm)
            else if (tolower(arginfo$category) == "base")
                base_args <- append(base_args, nm)
            else if (tolower(arginfo$category) == "advanced")
                advanced_args <- append(advanced_args, nm)
            else if (tolower(arginfo$category) == "esoteric")
                esoteric_args <- append(esoteric_args, nm)
        }
    }

    if (length(common_args) > 0) {
        cat("Common options:\n")
        for (arg_nm in common_args) {
            print_arg(arg_nm)
        }
        cat("\n")
    }

    if (length(base_args) > 0) {
        if (alginfo$name == "pipeline")
            cat("Options for read input/write output:\n")
        else
            cat("Options:\n")

        for (arg_nm in base_args) {
            print_arg(arg_nm)
        }
        cat("\n")
    }

    if (length(advanced_args) > 0) {
        if (alginfo$name == "pipeline")
            cat("Advanced options for read input/write output:\n")
        else
            cat("Advanced options:\n")

        for (arg_nm in advanced_args) {
            print_arg(arg_nm)
        }
        cat("\n")
    }

    if (length(esoteric_args) > 0) {
        if (alginfo$name == "pipeline")
            cat("Esoteric options for read input/write output:\n")
        else
            cat("Esoteric options:\n")

        for (arg_nm in esoteric_args) {
            print_arg(arg_nm)
        }
        cat("\n")
    }

    if (alginfo$name == "pipeline") {
        x <- alg$usageAsJSON() |> yyjsonr::read_json_str()
        if (!is.null(x$pipeline_algorithms) &&
            is.data.frame(x$pipeline_algorithms) &&
            nrow(x$pipeline_algorithms) > 0) {

            cat("Potential steps are:\n")
            for (i in seq_len(nrow(x$pipeline_algorithms))) {
                if (x$pipeline_algorithms[i, "name"] != "read" &&
                    x$pipeline_algorithms[i, "name"] != "write") {

                cat("  ", x$pipeline_algorithms[i, "name"], "\n    ",
                    x$pipeline_algorithms[i, "description"], "\n", sep = "")
                }
            }
            cat("\n")
        } else {
            cat("See `gdal_usage(\"raster pipeline\")` or `gdal_usage(\"vector pipeline\")`\n")
            cat("\n")
        }
    } else if (alginfo$long_description != "") {
        cat(alginfo$long_description, "\n", sep = "")
        cat("\n")
    }

    if (alginfo$URL != "")
        cat("For more details: ", alginfo$URL, "\n", sep = "")

    alg$release()
}
