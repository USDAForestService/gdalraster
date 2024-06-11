/* GDAL OGR utility functions for vector data sources
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#ifndef SRC_OGR_UTIL_H_
#define SRC_OGR_UTIL_H_

#include <map>
#include <string>
#include "rcpp_util.h"

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

bool ogr_ds_exists(std::string dsn, bool with_update);

std::string ogr_ds_format(std::string dsn);

SEXP ogr_ds_test_cap(std::string dsn, bool with_update);

bool create_ogr(std::string format, std::string dst_filename,
                int xsize, int ysize, int nbands, std::string dataType,
                std::string layer, std::string geom_type, std::string srs,
                std::string fld_name, std::string fld_type,
                Rcpp::Nullable<Rcpp::CharacterVector> dsco,
                Rcpp::Nullable<Rcpp::CharacterVector> lco,
                Rcpp::Nullable<Rcpp::List> layer_defn);

int ogr_ds_layer_count(std::string dsn);

SEXP ogr_ds_layer_names(std::string dsn);

bool ogr_layer_exists(std::string dsn, std::string layer);

SEXP ogr_layer_test_cap(std::string dsn, std::string layer,
                         bool with_update);

OGRLayerH CreateLayer_(GDALDatasetH hDS, std::string layer,
                       Rcpp::Nullable<Rcpp::List> layer_defn,
                       std::string geom_type, std::string srs,
                       Rcpp::Nullable<Rcpp::CharacterVector> options);

bool ogr_layer_create(std::string dsn, std::string layer,
                       Rcpp::Nullable<Rcpp::List> layer_defn,
                       std::string geom_type, std::string srs,
                       Rcpp::Nullable<Rcpp::CharacterVector> options);

bool ogr_layer_delete(std::string dsn, std::string layer);

SEXP ogr_layer_field_names(std::string dsn, std::string layer);

int ogr_field_index(std::string dsn, std::string layer, std::string fld_name);

bool CreateField_(GDALDatasetH hDS, OGRLayerH hLayer, std::string fld_name,
                  std::string fld_type, std::string fld_subtype, int fld_width,
                  int fld_precision, bool is_nullable, bool is_ignored,
                  bool is_unique, std::string default_value);

bool ogr_field_create(std::string dsn, std::string layer,
                      std::string fld_name, std::string fld_type,
                      std::string fld_subtype, int fld_width ,
                      int fld_precision, bool is_nullable,
                      bool is_ignored, bool is_unique,
                      std::string default_value);

bool CreateGeomField_(GDALDatasetH hDS, OGRLayerH hLayer, std::string fld_name,
                      OGRwkbGeometryType eGeomType, std::string srs,
                      bool is_nullable, bool is_ignored);

bool ogr_geom_field_create(std::string dsn, std::string layer,
                           std::string fld_name, std::string geom_type,
                           std::string srs, bool is_nullable,
                           bool is_ignored);

bool ogr_field_delete(std::string dsn, std::string layer,
                      std::string fld_name);

SEXP ogr_execute_sql(std::string dsn, std::string sql,
                     std::string spatial_filter, std::string dialect);

#endif  // SRC_OGR_UTIL_H_
