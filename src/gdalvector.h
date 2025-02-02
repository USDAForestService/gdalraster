/* R interface to a subset of the GDAL C API for vector. A class for OGRLayer,
   a layer of features in a GDALDataset. https://gdal.org/en/stable/api/vector_c_api.html

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
#ifndef SRC_GDALRASTER_H_
typedef void *GDALDatasetH;
typedef enum {GA_ReadOnly = 0, GA_Update = 1} GDALAccess;
#endif
typedef void *OGRLayerH;
typedef void *OGRFeatureH;
#endif

class GDALVector {
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

    // undocumented, exposed read-only fields for internal use
    std::string m_layer_name {""};  // layer name or sql statement
    bool m_is_sql {false};
    std::string m_dialect {""};

    // exposed read/write fields
    std::string defaultGeomColName {"geometry"};
    bool promoteToMulti {false};
    bool quiet {false};
    std::string returnGeomAs {"WKB"};
    std::string wkbByteOrder {"LSB"};

    // exposed methods
    void open(bool read_only);
    bool isOpen() const;
    std::string getDsn() const;
    Rcpp::CharacterVector getFileList() const;
    std::string getDriverShortName() const;
    std::string getDriverLongName() const;

    std::string getName() const;
    Rcpp::CharacterVector getFieldNames() const;
    Rcpp::List testCapability() const;
    std::string getFIDColumn() const;
    std::string getGeomType() const;
    std::string getGeometryColumn() const;
    std::string getSpatialRef() const;
    Rcpp::NumericVector bbox();
    Rcpp::List getLayerDefn() const;
    SEXP getFieldDomain(std::string domain_name) const;

    void setAttributeFilter(const std::string &query);
    std::string getAttributeFilter() const;
    void setIgnoredFields(const Rcpp::RObject &fields);
    void setSelectedFields(const Rcpp::RObject &fields);

    void setSpatialFilter(const std::string &wkt);
    void setSpatialFilterRect(const Rcpp::RObject &bbox);
    std::string getSpatialFilter() const;
    void clearSpatialFilter();

    double getFeatureCount();
    SEXP getNextFeature();
    void setNextByIndex(double i);
    // fid must be a length-1 numeric vector, since numeric vector can carry
    // the class attribute for integer64:
    SEXP getFeature(const Rcpp::RObject &fid);
    void resetReading();

    Rcpp::DataFrame fetch(double n);

    bool setFeature(const Rcpp::RObject &feature);
    bool createFeature(const Rcpp::RObject &feature);
    bool upsertFeature(const Rcpp::RObject &feature);
    SEXP getLastWriteFID() const;
    bool deleteFeature(const Rcpp::RObject &fid);
    bool syncToDisk() const;

    bool startTransaction(bool force);
    bool commitTransaction();
    bool rollbackTransaction();

    Rcpp::CharacterVector getMetadata() const;
    bool setMetadata(const Rcpp::CharacterVector metadata);
    std::string getMetadataItem(std::string mdi_name) const;

    bool layerIntersection(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerUnion(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerSymDifference(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerIdentity(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerUpdate(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerClip(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerErase(
            GDALVector method_layer,
            GDALVector result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);

    void close();

    void OGRFeatureFromList_dumpReadble(const Rcpp::RObject &feat) const;

    // methods for internal use not exposed to R
    void checkAccess_(GDALAccess access_needed) const;

    void setDsn_(std::string dsn);
    GDALDatasetH getGDALDatasetH_() const;
    void setGDALDatasetH_(const GDALDatasetH hDs, bool with_update);
    OGRLayerH getOGRLayerH_() const;
    void setOGRLayerH_(const OGRLayerH hLyr, const std::string &lyr_name);
    void setFieldNames_();

    SEXP createDF_(R_xlen_t nrow) const;
    void attachGISattributes_(Rcpp::List ogr_feat_obj,
                              const Rcpp::CharacterVector &geom_col,
                              const Rcpp::CharacterVector &geom_col_type,
                              const Rcpp::CharacterVector &geom_col_srs,
                              const std::string &geom_format) const;

    OGRFeatureH OGRFeatureFromList_(const Rcpp::RObject &feature) const;

 private:
    std::string m_dsn {""};
    Rcpp::CharacterVector m_open_options {};
    std::string m_attr_filter {""};
    std::string m_spatial_filter {""};
    Rcpp::CharacterVector m_field_names {};
    Rcpp::CharacterVector m_ignored_fields {};
    GDALDatasetH m_hDataset {nullptr};
    GDALAccess m_eAccess {GA_ReadOnly};
    OGRLayerH m_hLayer {nullptr};
    int64_t m_last_write_fid {NA_INTEGER64};
};

RCPP_EXPOSED_CLASS(GDALVector)

#endif  // SRC_GDALVECTOR_H_
