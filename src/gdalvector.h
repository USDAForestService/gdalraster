/* R interface to a subset of the GDAL C API for vector.
   A class for OGRLayer, a layer of features in a GDALDataset.
   https://gdal.org/api/vector_c_api.html
   Chris Toney <chris.toney at usda.gov> */

#ifndef gdalvector_H
#define gdalvector_H

#include "rcpp_util.h"

#include <string>
#include <vector>

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
    GDALDatasetH  hDataset;
    GDALAccess eAccess;
    OGRLayerH hLayer;
    OGRFeatureDefnH hFDefn;

    public:
    GDALVector();
    GDALVector(Rcpp::CharacterVector dsn);
    GDALVector(Rcpp::CharacterVector dsn, std::string layer);
    GDALVector(Rcpp::CharacterVector dsn, std::string layer, bool read_only);
    GDALVector(Rcpp::CharacterVector dsn, std::string layer, bool read_only,
               Rcpp::CharacterVector open_options);

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
    void resetReading();

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
    void _checkAccess(GDALAccess access_needed) const;
    OGRLayerH _getOGRLayerH();
};

RCPP_EXPOSED_CLASS(GDALVector)

#endif
