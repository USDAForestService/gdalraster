/* R interface to a subset of the GDAL C API for vector. A class for OGRLayer,
   a layer of features in a GDALDataset. https://gdal.org/en/stable/api/vector_c_api.html

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef SRC_GDALVECTOR_H_
#define SRC_GDALVECTOR_H_

#include <map>
#include <string>
#include <vector>

#include "rcpp_util.h"

#if __has_include("ogr_recordbatch.h")  // for Arrow structs (GDAL >= 3.6)
    #include "ogr_recordbatch.h"
#endif

// Predeclare some GDAL types until the public header is included
#ifndef GDAL_H_INCLUDED
    #ifndef SRC_GDALRASTER_H_
        typedef void *GDALDatasetH;
        typedef enum {GA_ReadOnly = 0, GA_Update = 1} GDALAccess;
    #endif
    typedef void *OGRLayerH;
    typedef void *OGRFeatureH;
#endif

// value for marking FID when used along with regular attribute field indexes
const int FID_MARKER = -999;

class GDALVector {
 public:
    GDALVector();
    explicit GDALVector(const Rcpp::CharacterVector &dsn);
    GDALVector(const Rcpp::CharacterVector &dsn, const std::string &layer);
    GDALVector(const Rcpp::CharacterVector &dsn, const std::string &layer,
               bool read_only);
    GDALVector(const Rcpp::CharacterVector &dsn, const std::string &layer,
               bool read_only, const Rcpp::CharacterVector &open_options);
    GDALVector(const Rcpp::CharacterVector &dsn, const std::string &layer,
               bool read_only,
               const Rcpp::Nullable<Rcpp::CharacterVector> &open_options,
               const std::string &spatial_filter, const std::string &dialect);
    ~GDALVector();

    // undocumented, exposed read-only fields for internal use
    std::string m_layer_name {""};  // layer name or sql statement
    bool m_is_sql {false};
    std::string m_dialect {""};

    // exposed read/write fields
    std::string defaultGeomColName {"geom"};
    bool promoteToMulti {false};
    bool convertToLinear {false};
    std::string returnGeomAs {"WKB"};
    std::string wkbByteOrder {"LSB"};
    Rcpp::CharacterVector arrowStreamOptions {""};
    bool quiet {false};
    bool transactionsForce {false};

    // exposed methods
    void open(bool read_only);
    bool isOpen() const;
    std::string getDsn() const;
    Rcpp::CharacterVector getFileList() const;
    void info() const;
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
    SEXP getFieldDomain(const std::string &domain_name) const;

    void setAttributeFilter(const std::string &query);
    std::string getAttributeFilter() const;
    void setIgnoredFields(const Rcpp::RObject &fields);
    void setSelectedFields(const Rcpp::RObject &fields);
    Rcpp::CharacterVector getIgnoredFields() const;

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

    SEXP getArrowStream();
    void releaseArrowStream();

    bool setFeature(const Rcpp::List &feature);
    bool createFeature(const Rcpp::List &feature);
    Rcpp::LogicalVector batchCreateFeature(const Rcpp::DataFrame &feature_set);
    bool upsertFeature(const Rcpp::List &feature);
    SEXP getLastWriteFID() const;
    bool deleteFeature(const Rcpp::RObject &fid);
    bool syncToDisk() const;

    bool startTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    Rcpp::CharacterVector getMetadata() const;
    bool setMetadata(const Rcpp::CharacterVector &metadata);
    std::string getMetadataItem(const std::string &mdi_name) const;

    bool layerIntersection(
            GDALVector* const &method_layer,
            GDALVector* const &result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerUnion(
            GDALVector* const &method_layer,
            GDALVector* const &result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerSymDifference(
            GDALVector* const &method_layer,
            GDALVector* const &result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerIdentity(
            GDALVector* const &method_layer,
            GDALVector* const &result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerUpdate(
            GDALVector* const &method_layer,
            GDALVector* const &result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerClip(
            GDALVector* const &method_layer,
            GDALVector* const &result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);
    bool layerErase(
            GDALVector* const &method_layer,
            GDALVector* const &result_layer,
            bool quiet,
            const Rcpp::Nullable<const Rcpp::CharacterVector> &options);

    void close();

    void OGRFeatureFromList_dumpReadble(const Rcpp::List &feat) const;

    void show() const;

    // methods for internal use not exposed to R
    void checkAccess_(GDALAccess access_needed) const;

    void setDsn_(const std::string &dsn);
    GDALDatasetH getGDALDatasetH_() const;
    void setGDALDatasetH_(GDALDatasetH hDs, bool with_update);
    OGRLayerH getOGRLayerH_() const;
    void setOGRLayerH_(OGRLayerH hLyr, const std::string &lyr_name);
    void setFieldNames_();

    SEXP createDF_(R_xlen_t nrow) const;
    void attachGISattributes_(Rcpp::List *ogr_feat_obj,
                              const Rcpp::CharacterVector &geom_col,
                              const Rcpp::CharacterVector &geom_col_type,
                              const Rcpp::CharacterVector &geom_col_srs,
                              const std::string &geom_format) const;

    std::vector<std::map<R_xlen_t, int>> validateFeatInput_(
            const Rcpp::List &feature) const;

    OGRFeatureH OGRFeatureFromList_(
            const Rcpp::List &feature, R_xlen_t row_idx,
            const std::map<R_xlen_t, int> &map_flds,
            const std::map<R_xlen_t, int> &map_geom_flds) const;

#if __has_include("ogr_recordbatch.h")
    int arrow_get_schema(struct ArrowSchema* out);
    int arrow_get_next(struct ArrowArray* out);
    const char* arrow_get_last_error();
    static int arrow_get_schema_wrap(struct ArrowArrayStream* stream,
                                     struct ArrowSchema* out);

    static int arrow_get_next_wrap(struct ArrowArrayStream* stream,
                                   struct ArrowArray* out);

    static const char* arrow_get_last_error_wrap(
                                   struct ArrowArrayStream* stream);

    static void arrow_release_wrap(struct ArrowArrayStream* stream);
#endif

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
#if __has_include("ogr_recordbatch.h")
    struct ArrowArrayStream m_stream;
    std::vector<SEXP> m_stream_xptrs {};
#endif
};

// cppcheck-suppress unknownMacro
RCPP_EXPOSED_CLASS(GDALVector)

#endif  // SRC_GDALVECTOR_H_
