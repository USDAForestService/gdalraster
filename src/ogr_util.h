/* GDAL OGR utility functions for vector data sources
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef OGR_UTIL_H_
#define OGR_UTIL_H_

#include <gdal.h>

#include <Rcpp.h>

#include <string>

#include "gdalvector.h"
#include "rcpp_util.h"

// Lookup of OGRwkbGeometryType by string descriptor
// Returns wkbUnknown if no match
OGRwkbGeometryType getWkbGeomType_(const std::string &geom_type);

// Lookup of geometry type string by OGRwkbGeometryType
// Returns "UNKNOWN" if no match
std::string getWkbGeomString_(OGRwkbGeometryType eType);

// Lookup of OGRFieldType by string descriptor
// Error if no match
OGRFieldType getOFT_(const std::string &fld_type);

// Lookup of OGR field type string by OGRFieldType
// Returns empty string if no match, with warning emitted
std::string getOFTString_(OGRFieldType eType);

// Lookup of OGRFieldSubType by string descriptor
// Returns OFSTNone if no match
OGRFieldSubType getOFTSubtype_(const std::string &fld_subtype);

// Lookup of OGR field subtype string by OGRFieldSubType
// Returns "OFSTNone" if no match
std::string getOFTSubtypeString_(OGRFieldSubType eType);

bool ogr_ds_exists(const std::string &dsn, bool with_update);

std::string ogr_ds_format(const std::string &dsn);

SEXP ogr_ds_test_cap(const std::string &dsn, bool with_update);

GDALVector *create_ogr(const std::string &format,
                       const std::string &dst_filename,
                       const std::string &layer,
                       const std::string &geom_type,
                       const std::string &srs,
                       const std::string &fld_name,
                       const std::string &fld_type,
                       const Rcpp::Nullable<Rcpp::CharacterVector> &dsco,
                       const Rcpp::Nullable<Rcpp::CharacterVector> &lco,
                       const Rcpp::Nullable<Rcpp::List> &layer_defn);

int ogr_ds_layer_count(const std::string &dsn);

SEXP ogr_ds_layer_names(const std::string &dsn);

SEXP ogr_ds_field_domain_names(const std::string &dsn);

bool ogr_ds_add_field_domain(const std::string &dsn,
                             const Rcpp::List &fld_dom_defn);

bool ogr_ds_delete_field_domain(const std::string &dsn,
                                const std::string &domain_name);

bool ogr_layer_exists(const std::string &dsn, const std::string &layer);

SEXP ogr_layer_test_cap(const std::string &dsn, const std::string &layer,
                         bool with_update);

GDALVector *ogr_layer_create(
        const std::string &dsn, const std::string &layer,
        const Rcpp::Nullable<Rcpp::List> &layer_defn,
        const std::string &geom_type, const std::string &srs,
        const Rcpp::Nullable<Rcpp::CharacterVector> &options,
        bool reserved1);

bool ogr_layer_rename(const std::string &dsn, const std::string &layer,
                      const std::string &new_name);

bool ogr_layer_delete(const std::string &dsn, const std::string &layer);

SEXP ogr_layer_field_names(const std::string &dsn, const std::string &layer);

int ogr_field_index(const std::string &dsn, const std::string &layer,
                    const std::string &fld_name);

bool ogr_field_create(const std::string &dsn, const std::string &layer,
                      const std::string &fld_name, const std::string &fld_type,
                      const std::string &fld_subtype, int fld_width ,
                      int fld_precision, bool is_nullable,
                      bool is_unique, const std::string &default_value,
                      const std::string &domain_name);

bool ogr_geom_field_create(const std::string &dsn, const std::string &layer,
                           const std::string &fld_name,
                           const std::string &geom_type,
                           const std::string &srs, bool is_nullable);

bool ogr_field_rename(const std::string &dsn, const std::string &layer,
                      const std::string &fld_name,
                      const std::string &new_name);

bool ogr_field_set_domain_name(const std::string &dsn,
                               const std::string &layer,
                               const std::string &fld_name,
                               const std::string &domain_name);

bool ogr_field_delete(const std::string &dsn, const std::string &layer,
                      const std::string &fld_name);

SEXP ogr_execute_sql(const std::string &dsn, const std::string &sql,
                     const std::string &spatial_filter,
                     const std::string &dialect);

#endif  // OGR_UTIL_H_
