/* GDAL OGR utility functions for vector data sources
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#ifndef SRC_OGR_UTIL_H_
#define SRC_OGR_UTIL_H_

#include <map>
#include <string>
#include "rcpp_util.h"

#include "gdalvector.h"

#ifdef GDAL_H_INCLUDED
// map OGRwkbGeometryType enum to string names for use in R
const std::map<std::string, OGRwkbGeometryType> MAP_OGR_GEOM_TYPE{
    {"UNKNOWN", wkbUnknown},
    {"POINT", wkbPoint},
    {"LINESTRING", wkbLineString},
    {"POLYGON", wkbPolygon},
    {"MULTIPOINT", wkbMultiPoint},
    {"MULTILINESTRING", wkbMultiLineString},
    {"MULTIPOLYGON", wkbMultiPolygon},
    {"GEOMETRYCOLLECTION", wkbGeometryCollection},
    {"CIRCULARSTRING", wkbCircularString},
    {"COMPOUNDCURVE", wkbCompoundCurve},
    {"CURVEPOLYGON", wkbCurvePolygon},
    {"MULTICURVE", wkbMultiCurve},
    {"MULTISURFACE", wkbMultiSurface},
    {"CURVE", wkbCurve},
    {"SURFACE", wkbSurface},
    {"POLYHEDRALSURFACE", wkbPolyhedralSurface},
    {"TIN", wkbTIN},
    {"TRIANGLE", wkbTriangle},
    {"NONE", wkbNone},
    {"LINEARRING", wkbLinearRing},
    {"CIRCULARSTRINGZ", wkbCircularStringZ},
    {"COMPOUNDCURVEZ", wkbCompoundCurveZ},
    {"CURVEPOLYGONZ", wkbCurvePolygonZ},
    {"MULTICURVEZ", wkbMultiCurveZ},
    {"MULTISURFACEZ", wkbMultiSurfaceZ},
    {"CURVEZ", wkbCurveZ},
    {"SURFACEZ", wkbSurfaceZ},
    {"POLYHEDRALSURFACEZ", wkbPolyhedralSurfaceZ},
    {"TINZ", wkbTINZ},
    {"TRIANGLEZ", wkbTriangleZ},
    {"POINTM", wkbPointM},
    {"LINESTRINGM", wkbLineStringM},
    {"POLYGONM", wkbPolygonM},
    {"MULTIPOINTM", wkbMultiPointM},
    {"MULTILINESTRINGM", wkbMultiLineStringM},
    {"MULTIPOLYGONM", wkbMultiPolygonM},
    {"GEOMETRYCOLLECTIONM", wkbGeometryCollectionM},
    {"CIRCULARSTRINGM", wkbCircularStringM},
    {"COMPOUNDCURVEM", wkbCompoundCurveM},
    {"CURVEPOLYGONM", wkbCurvePolygonM},
    {"MULTICURVEM", wkbMultiCurveM},
    {"MULTISURFACEM", wkbMultiSurfaceM},
    {"CURVEM", wkbCurveM},
    {"SURFACEM", wkbSurfaceM},
    {"POLYHEDRALSURFACEM", wkbPolyhedralSurfaceM},
    {"TINM", wkbTINM},
    {"TRIANGLEM", wkbTriangleM},
    {"POINTZM", wkbPointZM},
    {"LINESTRINGZM", wkbLineStringZM},
    {"POLYGONZM", wkbPolygonZM},
    {"MULTIPOINTZM", wkbMultiPointZM},
    {"MULTILINESTRINGZM", wkbMultiLineStringZM},
    {"MULTIPOLYGONZM", wkbMultiPolygonZM},
    {"GEOMETRYCOLLECTIONZM", wkbGeometryCollectionZM},
    {"CIRCULARSTRINGZM", wkbCircularStringZM},
    {"COMPOUNDCURVEZM", wkbCompoundCurveZM},
    {"CURVEPOLYGONZM", wkbCurvePolygonZM},
    {"MULTICURVEZM", wkbMultiCurveZM},
    {"MULTISURFACEZM", wkbMultiSurfaceZM},
    {"CURVEZM", wkbCurveZM},
    {"SURFACEZM", wkbSurfaceZM},
    {"POLYHEDRALSURFACEZM", wkbPolyhedralSurfaceZM},
    {"TINZM", wkbTINZM},
    {"TRIANGLEZM", wkbTriangleZM},
    {"POINT25D", wkbPoint25D},
    {"LINESTRING25D", wkbLineString25D},
    {"POLYGON25D", wkbPolygon25D},
    {"MULTIPOINT25D", wkbMultiPoint25D},
    {"MULTILINESTRING25D", wkbMultiLineString25D},
    {"MULTIPOLYGON25D", wkbMultiPolygon25D},
    {"GEOMETRYCOLLECTION25D", wkbGeometryCollection25D}
};

// map OGRFieldType enum to string names for use in R
const std::map<std::string, OGRFieldType, _ci_less> MAP_OGR_FLD_TYPE{
    {"OFTInteger", OFTInteger},
    {"OFTIntegerList", OFTIntegerList},
    {"OFTReal", OFTReal},
    {"OFTRealList", OFTRealList},
    {"OFTString", OFTString},
    {"OFTStringList", OFTStringList},
    {"OFTBinary", OFTBinary},
    {"OFTDate", OFTDate},
    {"OFTTime", OFTTime},
    {"OFTDateTime", OFTDateTime},
    {"OFTInteger64", OFTInteger64},
    {"OFTInteger64List", OFTInteger64List}
};

// map OGRFieldSubType enum to string names for use in R
// A subtype represents a hint, a restriction of the main type, that is not
// strictly necessary to consult.
const std::map<std::string, OGRFieldSubType, _ci_less> MAP_OGR_FLD_SUBTYPE{
    {"OFSTNone", OFSTNone},
    {"OFSTBoolean", OFSTBoolean},
    {"OFSTInt16", OFSTInt16},
    {"OFSTFloat32", OFSTFloat32},
    {"OFSTJSON", OFSTJSON},
    {"OFSTUUID", OFSTUUID}
};
#endif

// Internal lookup of OGRwkbGeometryType by string descriptor
// Returns wkbUnknown if no match
OGRwkbGeometryType getWkbGeomType_(const std::string &geom_type);

// Internal lookup of geometry type string by OGRwkbGeometryType
// Returns "UNKNOWN" if no match
std::string getWkbGeomString_(OGRwkbGeometryType eType);

// Internal lookup of OGRFieldType by string descriptor
// Error if no match
OGRFieldType getOFT_(const std::string &fld_type);

// Internal lookup of OGR field type string by OGRFieldType
// Returns empty string if no match, with warning emitted
std::string getOFTString_(OGRFieldType eType);

// Internal lookup of OGRFieldSubType by string descriptor
// Returns OFSTNone if no match
OGRFieldSubType getOFTSubtype_(const std::string &fld_subtype);

// Internal lookup of OGR field subtype string by OGRFieldSubType
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

bool ogr_layer_exists(const std::string &dsn, const std::string &layer);

SEXP ogr_layer_test_cap(const std::string &dsn, const std::string &layer,
                         bool with_update);

// internal CreateLayer
OGRLayerH CreateLayer_(GDALDatasetH hDS, const std::string &layer,
                       Rcpp::Nullable<Rcpp::List> layer_defn,
                       const std::string &geom_type, const std::string &srs,
                       Rcpp::Nullable<Rcpp::CharacterVector> options);

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

// internal CreateField
bool CreateField_(GDALDatasetH hDS, OGRLayerH hLayer,
                  const std::string &fld_name,
                  const std::string &fld_type, const std::string &fld_subtype,
                  int fld_width, int fld_precision, bool is_nullable,
                  bool is_unique, const std::string &default_value);

bool ogr_field_create(const std::string &dsn, const std::string &layer,
                      const std::string &fld_name, const std::string &fld_type,
                      const std::string &fld_subtype, int fld_width ,
                      int fld_precision, bool is_nullable,
                      bool is_unique, const std::string &default_value);

// internal CreateGeomField
bool CreateGeomField_(GDALDatasetH hDS, OGRLayerH hLayer,
                      const std::string &fld_name,
                      OGRwkbGeometryType eGeomType, const std::string &srs,
                      bool is_nullable);

bool ogr_geom_field_create(const std::string &dsn, const std::string &layer,
                           const std::string &fld_name,
                           const std::string &geom_type,
                           const std::string &srs, bool is_nullable);

bool ogr_field_rename(const std::string &dsn, const std::string &layer,
                      const std::string &fld_name,
                      const std::string &new_name);

bool ogr_field_delete(const std::string &dsn, const std::string &layer,
                      const std::string &fld_name);

SEXP ogr_execute_sql(const std::string &dsn, const std::string &sql,
                     const std::string &spatial_filter,
                     const std::string &dialect);

#endif  // SRC_OGR_UTIL_H_
