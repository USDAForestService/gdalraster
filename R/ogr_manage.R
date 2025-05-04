# This file contains the R interface for src/ogr_util.h.
# Chris Toney <chris.toney at usda.gov>

#' Utility functions for managing vector data sources
#'
#' This set of functions can be used to create new vector datasets,
#' test existence of dataset/layer/field, test dataset and layer capabilities,
#' create new layers in an existing dataset, delete layers, create new
#' attribute and geometry fields on an existing layer, rename and delete
#' fields, and edit data with SQL statements.
#'
#' @name ogr_manage
#' @details
#' These functions are complementary to `ogrinfo()` and `ogr2ogr()` for
#' vector data management. Bindings to OGR wrap portions of the GDAL
#' Vector API (ogr_core.h and ogr_api.h,
#' \url{https://gdal.org/en/stable/api/vector_c_api.html}).
#'
#' `ogr_ds_exists()` tests whether a vector dataset can be opened from the
#' given data source name (DSN), potentially testing for update access.
#' Returns a logical scalar.
#'
#' `ogr_ds_format()` returns a character string containing the short name of
#' the format driver for a given DSN, or `NULL` if the dataset cannot be
#' opened as a vector source.
#'
#' `ogr_ds_test_cap()` tests the capabilities of a vector data source,
#' attempting to open it with update access by default.
#' Returns a list of capabilities with values `TRUE` or `FALSE`, or `NULL` is
#' returned if `dsn` cannot be opened with the requested access.
#' Wrapper of `GDALDatasetTestCapability()` in the GDAL API.
#' The returned list contains the following named elements:
#' * `CreateLayer`: `TRUE` if this datasource can create new layers
#' * `DeleteLayer`: `TRUE` if this datasource can delete existing layers
#' * `CreateGeomFieldAfterCreateLayer`: `TRUE` if the layers of this
#' datasource support geometry field creation just after layer creation
#' * `CurveGeometries`: `TRUE` if this datasource supports curve geometries
#' * `Transactions`: `TRUE` if this datasource supports (efficient)
#' transactions
#' * `EmulatedTransactions`: `TRUE` if this datasource supports transactions
#' through emulation
#' * `RandomLayerRead`: `TRUE` if this datasource has a dedicated
#' `GetNextFeature()` implementation, potentially returning features from
#' layers in a non-sequential way
#' * `RandomLayerWrite`: `TRUE` if this datasource supports calling
#' `CreateFeature()` on layers in a non-sequential way
#'
#' `ogr_ds_create()` creates a new vector datasource, optionally also creating
#' a layer, and optionally creating one or more fields on the layer.
#' The attribute fields and geometry field(s) to create can be specified as a
#' feature class definition (`layer_defn` as list, see [ogr_define]), or
#' alternatively, by giving the `geom_type` and `srs`, optionally along with
#' one `fld_name` and `fld_type` to be created in the layer.
#' By default, returns logical `TRUE` indicating success (output written to
#' `dst_filename`), or an object of class [`GDALVector`][GDALVector] for the
#' output layer will be returned if `return_obj = TRUE`. An error is raised if
#' the operation fails.
#'
#' `ogr_ds_layer_count()` returns the number of layers in a vector dataset.
#'
#' `ogr_ds_layer_names()` returns a character vector of layer names in a
#' vector dataset, or `NULL` if no layers are found.
#'
#' `ogr_layer_exists()` tests whether a layer can be accessed by name in a
#' given vector dataset. Returns a logical scalar.
#'
#' `ogr_layer_test_cap()` tests whether a layer supports named capabilities,
#' attempting to open the dataset with update access by default.
#' Returns a list of capabilities with values `TRUE` or `FALSE`. `NULL` is
#' returned if `dsn` cannot be opened with the requested access, or `layer`
#' cannot be found. The returned list contains the following named elements:
#' `RandomRead`, `SequentialWrite`, `RandomWrite`, `UpsertFeature`,
#' `FastSpatialFilter`, `FastFeatureCount`, `FastGetExtent`,
#' `FastSetNextByIndex`, `CreateField`, `CreateGeomField`, `DeleteField`,
#' `ReorderFields`, `AlterFieldDefn`, `AlterGeomFieldDefn`, `IgnoreFields`,
#' `DeleteFeature`, `Rename`, `StringsAsUTF8`, `CurveGeometries`.
#' See the GDAL documentation for
#' [`OGR_L_TestCapability()`](https://gdal.org/en/stable/api/vector_c_api.html#_CPPv420OGR_L_TestCapability9OGRLayerHPKc).
#'
#' `ogr_layer_create()` creates a new layer in an existing vector data source,
#' with a specified geometry type and spatial reference definition.
#' This function also accepts a feature class definition given as a list of
#' field names and their definitions (see [ogr_define]).
#' (Note: use `ogr_ds_create()` to create single-layer formats such as "ESRI
#' Shapefile", "FlatGeobuf", "GeoJSON", etc.)
#' By default, returns logical `TRUE` indicating success, or an object of class
#' [`GDALVector`][GDALVector] will be returned if `return_obj = TRUE`.
#' An error is raised if the operation fails.
#'
#' `ogr_layer_field_names()` returns a character vector of field names on a
#' layer, or `NULL` if no fields are found. The first layer by index is opened
#' if `NULL` is given for the `layer` argument.
#'
#' `ogr_layer_rename()` renames a layer in a vector dataset. This operation is
#' implemented only by layers that expose the `Rename` capability (see
#' `ogr_layer_test_cap()` above). This operation will fail if a layer with the
#' new name already exists. Returns a logical scalar, `TRUE` indicating success.
#' Requires GDAL >= 3.5.
#'
#' `ogr_layer_delete()` deletes an existing layer in a vector dataset.
#' Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_field_index()` tests for existence of an attribute field by name.
#' Returns the field index on the layer (0-based), or `-1` if the field does
#' not exist.
#'
#' `ogr_field_create()` creates a new attribute field of specified data type in
#' a given DSN/layer. Several optional field properties can be specified in
#' addition to the type. Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_geom_field_create()` creates a new geometry field of specified type in
#' a given DSN/layer. Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_field_rename()` renames an existing field on a vector layer.
#' Not all format drivers support this function. Some drivers may only support
#' renaming a field while there are still no features in the layer.
#' `AlterFieldDefn` is the relevant layer capability to check.
#' Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_field_delete()` deletes an existing field on a vector layer.
#' Not all format drivers support this function. Some drivers may only support
#' deleting a field while there are still no features in the layer.
#' Returns a logical scalar, `TRUE` indicating success.
#'
#' `ogr_execute_sql()` executes an SQL statement against the data store.
#' This function can be used to modify the schema or edit data using SQL
#' (e.g., `ALTER TABLE`, `DROP TABLE`, `CREATE INDEX`, `DROP INDEX`, `INSERT`,
#' `UPDATE`, `DELETE`), or to execute a query (i.e., `SELECT`).
#' Returns `NULL` (invisibly) for statements that are in error, or that have no
#' results set, or an object of class `GDALVector` representing a results set
#' from the query. Wrapper of `GDALDatasetExecuteSQL()` in the GDAL API.
#'
#' @param dsn Character string. The vector data source name, e.g., a filename
#' or database connection string.
#' @param with_update Logical scalar. `TRUE` to request update access when
#' opening the dataset, or `FALSE` to open read-only.
#' @param format GDAL short name of the vector format as character string.
#' Examples of some common output formats include: `"GPKG"`, `"FlatGeobuf"`,
#' `"ESRI Shapefile"`, `"SQLite"`.
#' @param layer Character string for a layer name in a vector dataset.
#' The `layer` argument may be given as empty string (`""`) in which case the
#' first layer by index will be opened (except with `ogr_layer_delete()` and
#' `ogr_layer_rename()` for which a layer name must be specified).
#' @param layer_defn A feature class definition for `layer` as a list of
#' zero or more attribute field definitions, and at least one geometry field
#' definition (see [ogr_define]).
#' Each field definition is a list with named elements containing values for
#' the field `$type` and other properties.
#' If `layer_defn` is given, it will be used and any additional parameters
#' passed that relate to the feature class definition will be ignored (i.e.,
#' `geom_type` and `srs`, as well as `fld_name` and `fld_type` in
#' `ogr_ds_create()`).
#' The first geometry field definition in `layer_defn` defines the
#' geometry type and spatial reference system for the layer (the geom field
#' definition must contain `$type`, and should also contain `$srs` when
#' creating a layer from a feature class definition).
#' @param geom_type Character string specifying a geometry type (see Details).
#' @param srs Character string containing a spatial reference system definition
#' as OGC WKT or other well-known format (e.g., the input formats usable with
#' [srs_to_wkt()]).
#' @param fld_name Character string containing the name of an attribute field
#' in `layer`.
#' @param fld_defn A field definition as list (see [ogr_def_field()]).
#' Additional arguments in `ogr_field_create()` will be ignored if a `fld_defn`
#' is given.
#' @param fld_type Character string containing the name of a field data type
#' (e.g., `OFTInteger`, `OFTReal`, `OFTString`).
#' @param fld_subtype Character string containing the name of a field subtype.
#' One of  `OFSTNone` (the default), `OFSTBoolean`, `OFSTInt16`, `OFSTFloat32`,
#' `OFSTJSON`, `OFSTUUID`.
#' @param fld_width Optional integer scalar specifying max number of characters.
#' @param fld_precision Optional integer scalar specifying number of digits
#' after the decimal point.
#' @param is_nullable Optional NOT NULL field constraint (logical scalar).
#' Defaults to `TRUE`.
#' @param is_unique Optional UNIQUE constraint on the field (logical scalar).
#' Defaults to `FALSE`.
#' @param default_value Optional default value for the field as a character
#' string.
#' @param geom_fld_defn A geometry field definition as list
#' (see [ogr_def_geom_field()]).
#' Additional arguments in `ogr_geom_field_create()` will be ignored if a
#' `geom_fld_defn` is given.
#' @param new_name Character string containing a new name to assign.
#' @param dsco Optional character vector of format-specific creation options
#' for `dsn` (`"NAME=VALUE"` pairs).
#' @param lco Optional character vector of format-specific creation options
#' for `layer` (`"NAME=VALUE"` pairs).
#' @param sql Character string containing an SQL statement (see Note).
#' @param spatial_filter Either a numeric vector of length four containing a
#' bounding box (xmin, ymin, xmax, ymax), or a character string containing a
#' geometry as OGC WKT, representing a spatial filter.
#' @param dialect Character string specifying the SQL dialect to use.
#' The OGR SQL engine (`"OGRSQL"`) will be used by default if a value is not
#' given. The `"SQLite"` dialect can also be used (see Note).
#' @param overwrite Logical scalar. `TRUE` to overwrite `dsn` if it already
#' exists when calling `ogr_ds_create()`. Default is `FALSE`.
#' @param return_obj Logical scalar. If `TRUE`, an object of class
#' [`GDALVector`][GDALVector] open on the newly created layer will be
#' returned. Defaults to `FALSE`. Must be used with either the `layer` or
#' `layer_defn` arguments.
#'
#' @note
#' The OGR SQL document linked under **See Also** contains information on the
#' SQL dialect supported internally by GDAL/OGR. Some format drivers (e.g.,
#' PostGIS) pass the SQL directly through to the underlying RDBMS (unless
#' `OGRSQL` is explicitly passed as the dialect). The SQLite dialect can also
#' be requested with the `SQLite` string passed as the `dialect` argument of
#' `ogr_execute_sql()`. This assumes that GDAL/OGR is built with support for
#' SQLite, and preferably also with Spatialite support to benefit from spatial
#' functions. The GDAL document for SQLite dialect has detailed information.
#'
#' Other SQL dialects may also be present for some vector formats.
#' For example, the `"INDIRECT_SQLITE"` dialect might potentially be used with
#' GeoPackage format (\url{https://gdal.org/en/stable/drivers/vector/gpkg.html#sql}).
#'
#' The function [ogrinfo()] can also be used to edit data with SQL statements
#' (GDAL >= 3.7).
#'
#' The name of the geometry column of a layer is empty (`""`) with some formats
#' such as ESRI Shapefile and FlatGeobuf. Implications for SQL may depend on the
#' dialect used. See the GDAL documentation for the "OGR SQL" and "SQLite"
#' dialects for details.
#'
#' @seealso
#  [ogr_define], [ogrinfo()], [ogr2ogr()]
#'
#' OGR SQL dialect and SQLite SQL dialect:\cr
#' \url{https://gdal.org/en/stable/user/ogr_sql_sqlite_dialect.html}
#'
#' @examples
#' ## Create GeoPackage and manage schema
#' dsn <- file.path(tempdir(), "test1.gpkg")
#' ogr_ds_create("GPKG", dsn)
#' ogr_ds_exists(dsn, with_update = TRUE)
#' # dataset capabilities
#' ogr_ds_test_cap(dsn)
#'
#' ogr_layer_create(dsn, layer = "layer1", geom_type = "Polygon",
#'                  srs = "EPSG:5070")
#'
#' ogr_field_create(dsn, layer = "layer1",
#'                  fld_name = "field1",
#'                  fld_type = "OFTInteger64",
#'                  is_nullable = FALSE)
#' ogr_field_create(dsn, layer = "layer1",
#'                  fld_name = "field2",
#'                  fld_type = "OFTString")
#'
#' ogr_ds_layer_count(dsn)
#' ogr_ds_layer_names(dsn)
#' ogr_layer_field_names(dsn, layer = "layer1")
#'
#' # delete a field
#' if (ogr_layer_test_cap(dsn, "layer1")$DeleteField) {
#'   ogr_field_delete(dsn, layer = "layer1", fld_name = "field2")
#' }
#'
#' ogr_layer_field_names(dsn, "layer1")
#'
#' # define a feature class and create layer
#' defn <- ogr_def_layer("Point", srs = epsg_to_wkt(4326))
#' # add the attribute fields
#' defn$id_field <- ogr_def_field(fld_type = "OFTInteger64",
#'                                is_nullable = FALSE,
#'                                is_unique = TRUE)
#' defn$str_field <- ogr_def_field(fld_type = "OFTString",
#'                                 fld_width = 25,
#'                                 is_nullable = FALSE,
#'                                 default_value = "'a default string'")
#' defn$numeric_field <- ogr_def_field(fld_type = "OFTReal",
#'                                     default_value = "0.0")
#'
#' ogr_layer_create(dsn, layer = "layer2", layer_defn = defn)
#' ogr_ds_layer_names(dsn)
#' ogr_layer_field_names(dsn, layer = "layer2")
#'
#' # add a field using SQL instead
#' ogr_execute_sql(dsn, sql = "ALTER TABLE layer2 ADD field4 float")
#'
#' # rename a field
#' if (ogr_layer_test_cap(dsn, "layer1")$AlterFieldDefn) {
#'   ogr_field_rename(dsn, layer = "layer2",
#'                    fld_name = "field4",
#'                    new_name = "renamed_field")
#' }
#' ogr_layer_field_names(dsn, layer = "layer2")
#'
#' # GDAL >= 3.7
#' if (gdal_version_num() >= gdal_compute_version(3, 7, 0))
#'   ogrinfo(dsn, "layer2")
#'
#' \dontshow{deleteDataset(dsn)}
#' ## Edit data using SQL
#' src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
#' perims_shp <- file.path(tempdir(), "mtbs_perims.shp")
#' ogr2ogr(src_dsn = src, dst_dsn = perims_shp, src_layers = "mtbs_perims")
#' ogr_ds_format(dsn = perims_shp)
#' ogr_ds_layer_names(dsn = perims_shp)
#' ogr_layer_field_names(dsn = perims_shp, layer = "mtbs_perims")
#'
#' alt_tbl <- "ALTER TABLE mtbs_perims ADD burn_bnd_ha float"
#' ogr_execute_sql(dsn = perims_shp, sql = alt_tbl)
#'
#' upd <- "UPDATE mtbs_perims SET burn_bnd_ha = (burn_bnd_ac / 2.471)"
#' ogr_execute_sql(dsn = perims_shp, sql = upd, dialect = "SQLite")
#' ogr_layer_field_names(dsn = perims_shp, layer = "mtbs_perims")
#'
#' # if GDAL >= 3.7:
#' #   ogrinfo(dsn = perims_shp, layer = "mtbs_perims")
#' # or, for output incl. the feature data (omit the default "-so" arg):
#' #   ogrinfo(dsn = perims_shp, layer = "mtbs_perims", cl_arg = "-nomd")
#' \dontshow{deleteDataset(perims_shp)}
#' @export
ogr_ds_exists <- function(dsn, with_update = FALSE) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    return(.ogr_ds_exists(dsn, with_update))
}

#' @name ogr_manage
#' @export
ogr_ds_format <- function(dsn) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    ret <- .ogr_ds_format(dsn)
    if (ret == "")
        return(NULL)
    else
        return(ret)
}

#' @name ogr_manage
#' @export
ogr_ds_test_cap <- function(dsn, with_update = TRUE) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    return(.ogr_ds_test_cap(dsn, with_update))
}

#' @name ogr_manage
#' @export
ogr_ds_create <- function(format, dsn, layer = NULL, layer_defn = NULL,
                          geom_type = NULL, srs = NULL, fld_name = NULL,
                          fld_type = NULL, dsco = NULL, lco = NULL,
                          overwrite = FALSE, return_obj = FALSE) {

    # format
    if (!(is.character(format) && length(format) == 1))
        stop("'format' must be a length-1 character vector", call. = FALSE)
    # dsn
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (vsi_stat(dsn) && !overwrite)
        stop("'dsn' exists but 'overwrite' is `FALSE`", call. = FALSE)
    # layer
    if (is.null(layer))
        layer <- ""
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    # layer_defn
    if (!is.null(layer_defn)) {
        if (!is.list(layer_defn) || is.data.frame(layer_defn)) {
            stop("'layer_defn' must be a list object", call. = FALSE)
        }
    }
    # geom_type
    if (is.null(geom_type))
        geom_type <- ""
    if (!(is.character(geom_type) && length(geom_type) == 1))
        stop("'geom_type' must be a length-1 character vector", call. = FALSE)
    # srs
    if (is.null(srs))
        srs <- ""
    if (!(is.character(srs) && length(srs) == 1))
        stop("'srs' must be a length-1 character vector", call. = FALSE)
    # fld_name
    if (is.null(fld_name))
        fld_name <- ""
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)
    # fld_type
    if (is.null(fld_type))
        fld_type <- ""
    if (!(is.character(fld_type) && length(fld_type) == 1))
        stop("'fld_type' must be a length-1 character vector", call. = FALSE)
    # dsco
    if (!is.null(dsco)) {
        if (!is.character(dsco))
            stop("'dsco' must be a character vector", call. = FALSE)
    }
    # lco
    if (!is.null(lco)) {
        if (!is.character(lco))
            stop("'lco' must be a character vector", call. = FALSE)
    }
    # overwrite
    if (is.null(overwrite))
        stop("'overwrite' must be a logical value", call. = FALSE)
    if (!is.logical(overwrite) || length(overwrite) > 1)
        stop("'overwrite' must be a logical scalar", call. = FALSE)
    # return_obj
    if (is.null(return_obj))
        stop("'return_obj' must be a logical value", call. = FALSE)
    if (!is.logical(return_obj) || length(return_obj) > 1)
        stop("'return_obj' must be a logical scalar", call. = FALSE)
    if (return_obj && layer == "" && is.null(layer_defn)) {
        stop("'layer' or 'layer_defn' must be given with 'return_obj = TRUE'",
             call. = FALSE)
    }

    if (is.null(layer_defn)) {
        # signature for create_ogr() object factory
        lyr <- new(GDALVector, format, dsn, layer, geom_type, srs, fld_name,
                   fld_type, dsco, lco, NULL)

    } else {
        # signature for create_ogr() object factory
        lyr <- new(GDALVector, format, dsn, layer = layer, geom_type = "",
                   srs = "", fld_name = "", fld_type = "", dsco = dsco,
                   lco = lco, layer_defn = layer_defn)
    }

    if (return_obj) {
        return(lyr)
    } else {
        lyr$close()
        return(TRUE)
    }
}

#' @name ogr_manage
#' @export
ogr_ds_layer_count <- function(dsn) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    return(.ogr_ds_layer_count(dsn))
}

#' @name ogr_manage
#' @export
ogr_ds_layer_names <- function(dsn) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)

    return(.ogr_ds_layer_names(dsn))
}

#' @name ogr_manage
#' @export
ogr_layer_exists <- function(dsn, layer) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)

    return(.ogr_layer_exists(dsn, layer))
}

#' @name ogr_manage
#' @export
ogr_layer_test_cap <- function(dsn, layer = NULL, with_update = TRUE) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (is.null(layer))
        layer <- ""
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)

    return(.ogr_layer_test_cap(dsn, layer, with_update))
}

#' @name ogr_manage
#' @export
ogr_layer_create <- function(dsn, layer, layer_defn = NULL, geom_type = NULL,
                             srs = NULL, lco = NULL, return_obj = FALSE) {

    # dsn / layer
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!ogr_ds_exists(dsn, with_update = TRUE))
        stop("'dsn' does not exist, or no update access", call. = FALSE)
    if (ogr_layer_exists(dsn, layer))
        stop("'layer' already exists", call. = FALSE)
    # layer_defn
    if (!is.null(layer_defn)) {
        if (!is.list(layer_defn) || is.data.frame(layer_defn)) {
            stop("'layer_defn' must be a list object", call. = FALSE)
        }
    }

    if (!is.null(layer_defn)) {
        # using layer_defn so get geom_type and srs from the geom field defn
        has_geom_fld_defn <- FALSE
        for (nm in names(layer_defn)) {
            if (!is.list(layer_defn[[nm]]))
                stop("field definition must be a list", call. = FALSE)

            if (is.null(layer_defn[[nm]]$is_geom) ||
                    !is.logical(layer_defn[[nm]]$is_geom)) {
                stop("field definitions must contain `$is_geom=[TRUE|FALSE]`",
                     call. = FALSE)
            }

            if (layer_defn[[nm]]$is_geom) {
                geom_type <- layer_defn[[nm]]$type
                srs <- layer_defn[[nm]]$srs
                has_geom_fld_defn <- TRUE
                break
            }
        }
        if (!has_geom_fld_defn) {
            stop("'layer_defn' is missing a geometry field definition",
                 call. = FALSE)
        }
    }

    # geom_type
    if (is.null(geom_type)) {
        warning("geometry type not specified, using 'UNKNOWN'", call. = FALSE)
        geom_type <- "UNKNOWN"
    }
    # srs
    if (is.null(srs))
        srs <- ""
    if (!(is.character(srs) && length(srs) == 1))
        stop("'srs' must be a length-1 character vector", call. = FALSE)
    # lco
    if (!is.null(lco)) {
        if (!is.character(lco))
            stop("'lco' must be a character vector", call. = FALSE)
    }
    # return_obj
    if (is.null(return_obj))
        stop("'return_obj' must be a logical value", call. = FALSE)
    if (!is.logical(return_obj) || length(return_obj) > 1)
        stop("'return_obj' must be a logical scalar", call. = FALSE)

    # signature for ogr_layer_create() object factory
    lyr <- new(GDALVector, dsn, layer, layer_defn, geom_type, srs, lco, TRUE)

    if (return_obj) {
        return(lyr)
    } else {
        lyr$close()
        return(TRUE)
    }
}

#' @name ogr_manage
#' @export
ogr_layer_field_names <- function(dsn, layer = NULL) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (is.null(layer))
        layer <- ""
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)


    return(.ogr_layer_field_names(dsn, layer))
}

#' @name ogr_manage
#' @export
ogr_layer_rename <- function(dsn, layer, new_name) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(new_name) && length(new_name) == 1))
        stop("'new_name' must be a length-1 character vector", call. = FALSE)

    return(.ogr_layer_rename(dsn, layer, new_name))
}

#' @name ogr_manage
#' @export
ogr_layer_delete <- function(dsn, layer) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)

    return(.ogr_layer_delete(dsn, layer))
}

#' @name ogr_manage
#' @export
ogr_field_index <- function(dsn, layer, fld_name) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)

    return(.ogr_field_index(dsn, layer, fld_name))
}

#' @name ogr_manage
#' @export
ogr_field_create <- function(dsn, layer, fld_name,
                             fld_defn = NULL,
                             fld_type = "OFTInteger",
                             fld_subtype = "OFSTNone",
                             fld_width = 0L,
                             fld_precision = 0L,
                             is_nullable = TRUE,
                             is_unique = FALSE,
                             default_value = "") {

    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)

    if (!ogr_ds_exists(dsn, with_update = TRUE))
        stop("'dsn' does not exist, or no update access", call. = FALSE)
    if (!ogr_layer_exists(dsn, layer))
        stop("'layer' does not exist", call. = FALSE)

    fld_idx <- ogr_field_index(dsn, layer, fld_name)
    if (fld_idx >= 0)
        stop("field '", fld_name, "' already exists", call. = FALSE)

    # use field definition if given
    if (!is.null(fld_defn)) {
        if (!is.list(fld_defn))
            stop("'fld_defn' must be a list", call. = FALSE)

        if (is.null(fld_defn$type))
            stop("'fld_defn$type' is required", call. = FALSE)
        else
            fld_type <- fld_defn$type

        if (!is.null(fld_defn$subtype))
            fld_subtype <- fld_defn$subtype
        else
            fld_subtype <- "OFSTNone"

        if (!is.null(fld_defn$width))
            fld_width <- fld_defn$width
        else
            fld_width <- 0

        if (!is.null(fld_defn$precision))
            fld_precision <- fld_defn$precision
        else
            fld_precision <- 0

        if (!is.null(fld_defn$is_nullable))
            is_nullable <- fld_defn$is_nullable
        else
            is_nullable <- TRUE

        if (!is.null(fld_defn$is_unique))
            is_unique <- fld_defn$is_unique
        else
            is_unique <- FALSE

        if (!is.null(fld_defn$default))
            default_value <- fld_defn$default
        else
            default_value <- ""
    }

    return(.ogr_field_create(dsn, layer, fld_name, fld_type, fld_subtype,
                             fld_width, fld_precision, is_nullable, is_unique,
                             default_value))
}

#' @name ogr_manage
#' @export
ogr_geom_field_create <- function(dsn, layer, fld_name,
                                  geom_fld_defn = NULL,
                                  geom_type = NULL,
                                  srs = NULL,
                                  is_nullable = TRUE) {

    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)

    if (!ogr_ds_exists(dsn, with_update = TRUE))
        stop("'dsn' does not exist, or no update access", call. = FALSE)
    if (!ogr_layer_exists(dsn, layer))
        stop("'layer' does not exist", call. = FALSE)

    fld_idx <- ogr_field_index(dsn, layer, fld_name)
    if (fld_idx >= 0)
        stop("field ", fld_name, " already exists", call. = FALSE)

    if (is.null(geom_type))
        geom_type <- "UNKNOWN"
    else if (!(is.character(geom_type) && length(geom_type) == 1))
        stop("'geom_type' must be a length-1 character vector", call. = FALSE)

    if (is.null(srs))
        srs <- ""
    else if (!(is.character(srs) && length(srs) == 1))
        stop("'srs' must be a length-1 character vector", call. = FALSE)

    # use field definition if given
    if (!is.null(geom_fld_defn)) {
        if (!is.list(geom_fld_defn))
            stop("'geom_fld_defn' must be a list", call. = FALSE)

        if (is.null(geom_fld_defn$type))
            stop("'geom_fld_defn$type' is required", call. = FALSE)
        else
            geom_type <- geom_fld_defn$type

        if (is.null(geom_fld_defn$srs))
            srs <- ""
        else
            srs <- geom_fld_defn$srs

        if (!is.null(geom_fld_defn$is_nullable))
            is_nullable <- geom_fld_defn$is_nullable
        else
            is_nullable <- TRUE
    }

    return(.ogr_geom_field_create(dsn, layer, fld_name, geom_type, srs,
                                  is_nullable))
}

#' @name ogr_manage
#' @export
ogr_field_rename <- function(dsn, layer, fld_name, new_name) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(new_name) && length(new_name) == 1))
        stop("'new_name' must be a length-1 character vector", call. = FALSE)

    return(.ogr_field_rename(dsn, layer, fld_name, new_name))
}

#' @name ogr_manage
#' @export
ogr_field_delete <- function(dsn, layer, fld_name) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(layer) && length(layer) == 1))
        stop("'layer' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(fld_name) && length(fld_name) == 1))
        stop("'fld_name' must be a length-1 character vector", call. = FALSE)

    return(.ogr_field_delete(dsn, layer, fld_name))
}

#' @name ogr_manage
#' @export
ogr_execute_sql <- function(dsn, sql, spatial_filter = NULL, dialect = NULL) {
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a length-1 character vector", call. = FALSE)
    if (!(is.character(sql) && length(sql) == 1))
        stop("'sql' must be a length-1 character vector", call. = FALSE)

    if (!is.null(spatial_filter)) {
        if (is.numeric(spatial_filter) && length(spatial_filter) == 4)
            spatial_filter <- bbox_to_wkt(spatial_filter)
        if (!(is.character(spatial_filter) && length(spatial_filter) == 1))
            stop("spatial_filter must be length-4 numeric, or WKT string",
                 call. = FALSE)
    } else {
        spatial_filter <- ""
    }

    if (!is.null(dialect)) {
        if (!(is.character(dialect) && length(dialect) == 1))
            stop("'dialect' must be a length-1 character vector", call. = FALSE)
    } else {
        dialect <- ""
    }

    if (startsWith(toupper(sql), "SELECT ")) {
        return(new(GDALVector, dsn, sql, read_only = TRUE,
                   open_options = NULL, spatial_filter = spatial_filter,
                   dialect = dialect))
    } else {
        return(.ogr_execute_sql(dsn, sql, spatial_filter, dialect))
    }
}
