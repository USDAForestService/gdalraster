# Utility functions for managing vector data sources using GDAL's OGR API.
# This file has the R interface to src/ogr_util.h.
# Chris Toney <chris.toney at usda.gov>

#' Utility functions for managing vector data sources
#'
#' These functions can be used to manage vector data sources, e.g, test
#' existence of data source name (DSN), layers and fields; create new data
#' stores; get layer count; create and delete layers; create new attribute
#' and geometry fields in a layer.
#' @name ogr_manage
#' @details
#' These functions use GDAL's Vector C API (ogr_core.h and ogr_api.h).
#'
#' `ogr_ds_exists()` tests whether a GDAL vector dataset can be opened with the
#' given DSN, potentially testing for update access. Returns a logical scalar.
#'
#' `ogr_ds_create()` creates a new vector data store, optionally creating
#' also a layer, and optionally creating one or more fields in the layer.
#' The attribute fields and geometry field(s) to create can be specified as a
#' feature class definition (`layer_defn` as list, see below), or
#' alternatively, by giving the `geom_type` and `srs`, and optionally one
#' `fld_name` and `fld_type` to be created in the layer.
#'
#' `ogr_ds_layer_count()` returns the number of layers in a GDAL vector dataset.
#'
#' `ogr_layer_exists()` tests whether a layer can be accessed by name for a
#' given DSN.
#'
#' `ogr_layer_create()` creates a new layer in a vector dataset, with a
#' specified geometry type and spatial reference definition. This function also
#' accepts a feature class definition given as a list in `layer_defn` (see
#' below).
#'
#' `ogr_layer_delete()` deletes a layer in a vector dataset.
#'
#' `ogr_field_index()` tests for existence of a vector attribute field by name,
#' and returns the field index on the layer.
#'
#' `ogr_field_create()` creates a new attribute field of specified data type in
#' a given DSN/layer. Several optional field properties can be specified in
#' addition to the type.
#'
#' `ogr_geom_field_create()` creates a new geometry field of specified type in
#' a given DSN/layer.
#'
#' @param dsn Character string. The vector data source name, e.g., a filename
#' or database connection string.
#' @param with_update Logical scalar. `TRUE` to test for update access on the
#' dataset, or `FALSE` (the default) to test for existence regardless of
#' read/write access.
#' @param layer Character string for a layer name in a vector dataset.
#' @param layer_defn A feature class definition for `layer` as a list of
#' attribute field definition(s), and at least one geometry field definition.
#' Each field definition is a list containing the field name, type and other
#' properties (see Details). If a `layer_defn` is given, it will be used and
#' any additional values passed for defining a single field will be ignored.
#' The first geometry field definition in `layer_defn` defines the latyer
#' `geom_type` and `srs`.
#' @param geom_type Character string specifying a geometry type (see Details).
#' @param srs Character string containing a spatial reference system definition
#' as OGC WKT or other well-known format (e.g., the input formats usable with
#' [srs_to_wkt()]).
#' @param fld_name Character string containing the name of an attribute field
#' in `layer`.
#' @param fld_type Character string containing the name of a field data type
#' (e.g., `OFTInteger`, `OFTReal`, `OFTString`, see Details.)
#' @param fld_subtype Character string containing the name of a field subtype.
#' One of  `OFSTNone` (the default), `OFSTBoolean`, `OFSTInt16`, `OFSTFloat32`,
#' `OFSTJSON`, `OFSTUUID`.
#' @param fld_width Optional integer scalar specifying max number of characters.
#' @param fld_precision Optional integer scalar specifying number of digits
#' after the decimal point.
#' @param is_nullable Optional NOT NULL field constraint (logical scalar).
#' Defaults to `TRUE`.
#' @param is_ignored Whether field is ignored when retrieving features (logical
#' scalar). Defaults to `FALSE`.
#' @param is_unique Optional UNIQUE constraint on the field (logical scalar).
#' Defaults to `FALSE`.
#' @param default_value Optional default value for the field as a character
#' string.
#' @param dsco Optional character vector of format-specific creation options
#' for `dsn` (`"NAME=VALUE"` pairs).
#' @param lco Optional character vector of format-specific creation options
#' for `layer` (`"NAME=VALUE"` pairs).
