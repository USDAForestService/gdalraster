/* R interface to a subset of the GDAL C API for vector. A class for OGRLayer,
   a layer of features in a GDALDataset. https://gdal.org/api/vector_c_api.html

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#ifndef SRC_GDALVECTOR_H_
#define SRC_GDALVECTOR_H_

#include <string>
#include <vector>

#include "rcpp_util.h"

// Predeclare some GDAL types until the public header is included
#ifndef GDAL_H_INCLUDED
typedef void *GDALDatasetH;
typedef void *OGRLayerH;
typedef enum {GA_ReadOnly = 0, GA_Update = 1} GDALAccess;
#endif

class GDALVector {
 private:
    std::string dsn_in;
    std::string layer_in;  // layer name or sql statement
    bool is_sql_in;
    Rcpp::CharacterVector open_options_in;
    std::string spatial_filter_in;
    std::string dialect_in;
    GDALDatasetH hDataset;
    GDALAccess eAccess;
    OGRLayerH hLayer;

 public:
    GDALVector();
    explicit GDALVector(Rcpp::CharacterVector dsn);
    GDALVector(Rcpp::CharacterVector dsn, std::string layer);
    GDALVector(Rcpp::CharacterVector dsn, std::string layer, bool read_only);
    GDALVector(Rcpp::CharacterVector dsn, std::string layer, bool read_only,
               Rcpp::CharacterVector open_options);
    GDALVector(Rcpp::CharacterVector dsn, std::string layer, bool read_only,
               Rcpp::Nullable<Rcpp::CharacterVector> open_options,
               std::string spatial_filter, std::string dialect);

    std::string defaultGeomFldName = "geometry";
    std::string returnGeomAs = "WKB";
    std::string wkbByteOrder = "LSB";

    void open(bool read_only);
    bool isOpen() const;
    std::string getDsn() const;
    Rcpp::CharacterVector getFileList() const;
    std::string getDriverShortName() const;
    std::string getDriverLongName() const;

    std::string getName() const;
    bool testCapability(std::string capability) const;
    std::string getFIDColumn() const;
    std::string getGeomType() const;
    std::string getGeometryColumn() const;
    std::string getSpatialRef() const;
    Rcpp::NumericVector bbox();
    Rcpp::List getLayerDefn() const;

    void setAttributeFilter(std::string query);
    void setSpatialFilterRect(Rcpp::NumericVector bbox);
    void clearSpatialFilter();

    double getFeatureCount();
    SEXP getNextFeature();
    // fid must be a length-1 numeric vector, since numeric vector can carry
    // the class attribute for integer64:
    SEXP getFeature(Rcpp::NumericVector fid);
    void resetReading();

    Rcpp::DataFrame fetch(double n);

    void layerIntersection(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            Rcpp::Nullable<Rcpp::CharacterVector> options);
    void layerUnion(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            Rcpp::Nullable<Rcpp::CharacterVector> options);
    void layerSymDifference(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            Rcpp::Nullable<Rcpp::CharacterVector> options);
    void layerIdentity(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            Rcpp::Nullable<Rcpp::CharacterVector> options);
    void layerUpdate(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            Rcpp::Nullable<Rcpp::CharacterVector> options);
    void layerClip(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            Rcpp::Nullable<Rcpp::CharacterVector> options);
    void layerErase(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            Rcpp::Nullable<Rcpp::CharacterVector> options);

    void close();

    // methods for internal use not exported to R
    void checkAccess_(GDALAccess access_needed) const;
    OGRLayerH getOGRLayerH_() const;
    Rcpp::List featureToList_(OGRFeatureH hFeature) const;
    SEXP initDF_(R_xlen_t nrow) const;
};

RCPP_EXPOSED_CLASS(GDALVector)

#endif  // SRC_GDALVECTOR_H_
