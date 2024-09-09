/* Implementation of class GDALVector. Encapsulates an OGRLayer and its
   GDALDataset. Requires {bit64} on the R side for its integer64 S3 type.

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>

#include "gdal.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "cpl_time.h"
#include "ogr_srs_api.h"

#include "gdalraster.h"
#include "gdalvector.h"
#include "ogr_util.h"


GDALVector::GDALVector(Rcpp::CharacterVector dsn) :

            GDALVector(dsn, "", true, Rcpp::CharacterVector::create(),
                       "", "") {}

GDALVector::GDALVector(Rcpp::CharacterVector dsn, std::string layer) :

            GDALVector(dsn, layer, true, Rcpp::CharacterVector::create(),
                       "", "") {}

GDALVector::GDALVector(Rcpp::CharacterVector dsn, std::string layer,
                       bool read_only) :

            GDALVector(dsn, layer, read_only, Rcpp::CharacterVector::create(),
                       "", "") {}

GDALVector::GDALVector(Rcpp::CharacterVector dsn, std::string layer,
                       bool read_only, Rcpp::CharacterVector open_options) :

            GDALVector(dsn, layer, read_only, open_options, "", "") {}

GDALVector::GDALVector(Rcpp::CharacterVector dsn, std::string layer,
                       bool read_only,
                       Rcpp::Nullable<Rcpp::CharacterVector> open_options,
                       std::string spatial_filter, std::string dialect = "") :

            m_layer_name(layer),
            m_dialect(dialect),
            m_open_options(open_options.isNotNull() ? open_options :
                           Rcpp::CharacterVector::create()),
            m_spatial_filter(spatial_filter),
            m_hDataset(nullptr),
            m_eAccess(GA_ReadOnly),
            m_hLayer(nullptr) {

    m_dsn = Rcpp::as<std::string>(check_gdal_filename(dsn));
    open(read_only);
    setFeatureTemplate_();
}

void GDALVector::open(bool read_only) {
    if (m_dsn == "")
        Rcpp::stop("DSN is not set");

    if (m_hDataset != nullptr) {
        if (m_is_sql)
            GDALDatasetReleaseResultSet(m_hDataset, m_hLayer);
        GDALReleaseDataset(m_hDataset);
        m_hDataset = nullptr;
        m_hLayer = nullptr;
    }

    if (read_only)
        m_eAccess = GA_ReadOnly;
    else
        m_eAccess = GA_Update;

    std::vector<char *> dsoo(m_open_options.size() + 1);
    if (m_open_options.size() > 0) {
        for (R_xlen_t i = 0; i < m_open_options.size(); ++i) {
             dsoo[i] = (char *) (m_open_options[i]);
        }
    }
    dsoo[m_open_options.size()] = nullptr;

    OGRGeometryH hGeom_filter = nullptr;
    if (m_spatial_filter != "") {
        char *pszWKT = (char *) m_spatial_filter.c_str();
        if (OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom_filter) !=
                OGRERR_NONE) {
            if (hGeom_filter != nullptr)
                OGR_G_DestroyGeometry(hGeom_filter);
            Rcpp::stop("failed to create geometry from 'spatial_filter'");
        }
    }

    unsigned int nOpenFlags = GDAL_OF_VECTOR;
    if (read_only)
        nOpenFlags |= GDAL_OF_READONLY;
    else
        nOpenFlags |= GDAL_OF_UPDATE;

    m_hDataset = GDALOpenEx(m_dsn.c_str(), nOpenFlags, nullptr,
                            dsoo.data(), nullptr);
    if (m_hDataset == nullptr)
        Rcpp::stop("open dataset failed");

    const char *pszDialect = m_dialect.c_str();

    if (m_layer_name == "") {
        m_is_sql = false;
        m_hLayer = GDALDatasetGetLayer(m_hDataset, 0);
    }
    else if (STARTS_WITH_CI(m_layer_name.c_str(), "SELECT ")) {
        m_is_sql = true;
        if (EQUAL(pszDialect, "SQLite") && !has_spatialite())
            Rcpp::warning("SpatiaLite not available");
        m_hLayer = GDALDatasetExecuteSQL(m_hDataset, m_layer_name.c_str(),
                                         hGeom_filter, pszDialect);
    }
    else {
        m_is_sql = false;
        m_hLayer = GDALDatasetGetLayerByName(m_hDataset, m_layer_name.c_str());
    }

    if (m_hLayer == nullptr) {
        GDALReleaseDataset(m_hDataset);
        Rcpp::stop("failed to get layer");
    }
    else {
        OGR_L_ResetReading(m_hLayer);
    }

    if (hGeom_filter != nullptr)
        OGR_G_DestroyGeometry(hGeom_filter);
}

bool GDALVector::isOpen() const {
    if (m_hDataset == nullptr)
        return false;
    else
        return true;
}

std::string GDALVector::getDsn() const {
    return m_dsn;
}

Rcpp::CharacterVector GDALVector::getFileList() const {
    checkAccess_(GA_ReadOnly);

    char **papszFiles = GDALGetFileList(m_hDataset);

    int items = CSLCount(papszFiles);
    if (items > 0) {
        Rcpp::CharacterVector files(items);
        for (int i=0; i < items; ++i) {
            files(i) = papszFiles[i];
        }
        CSLDestroy(papszFiles);
        return files;
    }
    else {
        CSLDestroy(papszFiles);
        return "";
    }
}

std::string GDALVector::getDriverShortName() const {
    checkAccess_(GA_ReadOnly);

    GDALDriverH hDriver = GDALGetDatasetDriver(m_hDataset);
    return GDALGetDriverShortName(hDriver);
}

std::string GDALVector::getDriverLongName() const {
    checkAccess_(GA_ReadOnly);

    GDALDriverH hDriver = GDALGetDatasetDriver(m_hDataset);
    return GDALGetDriverLongName(hDriver);
}

std::string GDALVector::getName() const {
    checkAccess_(GA_ReadOnly);

    return OGR_L_GetName(m_hLayer);
}

Rcpp::List GDALVector::testCapability() const {
    checkAccess_(GA_ReadOnly);

    Rcpp::List capabilities = Rcpp::List::create(
        Rcpp::Named("RandomRead") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCRandomRead)),
        Rcpp::Named("SequentialWrite") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCSequentialWrite)),
        Rcpp::Named("RandomWrite") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCRandomWrite)),
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 6, 0)
        Rcpp::Named("UpsertFeature") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCUpsertFeature)),
#endif
        Rcpp::Named("FastSpatialFilter") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCFastSpatialFilter)),
        Rcpp::Named("FastFeatureCount") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCFastFeatureCount)),
        Rcpp::Named("FastGetExtent") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCFastGetExtent)),
        Rcpp::Named("FastSetNextByIndex") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCFastSetNextByIndex)),
        Rcpp::Named("CreateField") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCCreateField)),
        Rcpp::Named("CreateGeomField") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCCreateGeomField)),
        Rcpp::Named("DeleteField") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCDeleteField)),
        Rcpp::Named("ReorderFields") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCReorderFields)),
        Rcpp::Named("AlterFieldDefn") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCAlterFieldDefn)),
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 6, 0)
        Rcpp::Named("AlterGeomFieldDefn") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCAlterGeomFieldDefn)),
#endif
        Rcpp::Named("IgnoreFields") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCIgnoreFields)),
        Rcpp::Named("DeleteFeature") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCDeleteFeature)),
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 5, 0)
        Rcpp::Named("Rename") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCRename)),
#endif
        Rcpp::Named("StringsAsUTF8") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCStringsAsUTF8)),
        Rcpp::Named("CurveGeometries") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCCurveGeometries)));

    return capabilities;
}

std::string GDALVector::getFIDColumn() const {
    checkAccess_(GA_ReadOnly);

    return OGR_L_GetFIDColumn(m_hLayer);
}

std::string GDALVector::getGeomType() const {
    checkAccess_(GA_ReadOnly);

    OGRwkbGeometryType eType = OGR_L_GetGeomType(m_hLayer);
    return getWkbGeomString_(eType);
}

std::string GDALVector::getGeometryColumn() const {
    checkAccess_(GA_ReadOnly);

    return OGR_L_GetGeometryColumn(m_hLayer);
}

std::string GDALVector::getSpatialRef() const {
    // OGRLayer::GetSpatialRef() as WKT string
    checkAccess_(GA_ReadOnly);

    OGRSpatialReferenceH hSRS = OGR_L_GetSpatialRef(m_hLayer);
    if (hSRS == nullptr) {
        return "";
    }
    char *pszSRS_WKT = nullptr;
    if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE)
        Rcpp::stop("error exporting SRS to WKT");
    std::string srs_wkt(pszSRS_WKT);
    CPLFree(pszSRS_WKT);

    return srs_wkt;
}

Rcpp::NumericVector GDALVector::bbox() {
    // Note: bForce = true in the call to OGR_L_GetExtent(), so the entire
    // layer may be scanned to compute MBR.
    // see: testCapability("FastGetExtent")
    // Depending on the driver, a spatial filter may/may not be taken into
    // account. So it is safer to call bbox() without setting a spatial filter.
    checkAccess_(GA_ReadOnly);

    OGREnvelope envelope;
    if (OGR_L_GetExtent(m_hLayer, &envelope, true) != OGRERR_NONE)
        Rcpp::stop("the extent of the layer cannot be determined");

    Rcpp::NumericVector bbox_out =
            {envelope.MinX, envelope.MinY, envelope.MaxX, envelope.MaxY};

    return bbox_out;
}

Rcpp::List GDALVector::getLayerDefn() const {
    checkAccess_(GA_ReadOnly);

    OGRFeatureDefnH hFDefn = nullptr;
    hFDefn = OGR_L_GetLayerDefn(m_hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    Rcpp::List list_out = Rcpp::List::create();
    bool bValue = false;

    // attribute fields
    // TODO(ctoney): add field domain name
    for (int iField = 0; iField < OGR_FD_GetFieldCount(hFDefn); ++iField) {
        Rcpp::List list_fld_defn = Rcpp::List::create();
        OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, iField);
        if (hFieldDefn == nullptr)
            Rcpp::stop("could not obtain field definition");

        OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);
        list_fld_defn.push_back(getOFTString_(fld_type), "type");

        OGRFieldSubType fld_subtype = OGR_Fld_GetSubType(hFieldDefn);
        list_fld_defn.push_back(getOFTSubtypeString_(fld_subtype), "subtype");

        list_fld_defn.push_back(OGR_Fld_GetWidth(hFieldDefn), "width");

        list_fld_defn.push_back(OGR_Fld_GetPrecision(hFieldDefn), "precision");

        bValue = OGR_Fld_IsNullable(hFieldDefn);
        list_fld_defn.push_back(bValue, "is_nullable");

        bValue = OGR_Fld_IsUnique(hFieldDefn);
        list_fld_defn.push_back(bValue, "is_unique");

        std::string sValue = "";
        if (OGR_Fld_GetDefault(hFieldDefn) != nullptr)
            sValue = std::string(OGR_Fld_GetDefault(hFieldDefn));
        list_fld_defn.push_back(sValue, "default");

        bValue = OGR_Fld_IsIgnored(hFieldDefn);
        list_fld_defn.push_back(bValue, "is_ignored");

        bValue = false;
        list_fld_defn.push_back(bValue, "is_geom");

        list_out.push_back(list_fld_defn, OGR_Fld_GetNameRef(hFieldDefn));
    }

    // geometry fields
    for (int i = 0; i < OGR_FD_GetGeomFieldCount(hFDefn); ++i) {
        Rcpp::List list_geom_fld_defn = Rcpp::List::create();
        OGRGeomFieldDefnH hGeomFldDefn =
                OGR_FD_GetGeomFieldDefn(hFDefn, i);
        if (hGeomFldDefn == nullptr)
            Rcpp::stop("could not obtain geometry field definition");

        OGRwkbGeometryType eType = OGR_GFld_GetType(hGeomFldDefn);
        list_geom_fld_defn.push_back(getWkbGeomString_(eType), "type");

        OGRSpatialReferenceH hSRS = nullptr;
        hSRS = OGR_GFld_GetSpatialRef(hGeomFldDefn);
        if (hSRS == nullptr) {
            Rcpp::warning("could not obtain geometry field SRS");
            list_geom_fld_defn.push_back(NA_STRING, "srs");
        }
        else {
            char *pszSRS_WKT = nullptr;
            if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE) {
                Rcpp::warning("error exporting geometry SRS to WKT");
                list_geom_fld_defn.push_back(NA_STRING, "srs");
            }
            else {
                list_geom_fld_defn.push_back(std::string(pszSRS_WKT), "srs");
            }
            CPLFree(pszSRS_WKT);
        }

        bValue = OGR_GFld_IsNullable(hGeomFldDefn);
        list_geom_fld_defn.push_back(bValue, "is_nullable");

        bValue = OGR_GFld_IsIgnored(hGeomFldDefn);
        list_geom_fld_defn.push_back(bValue, "is_ignored");

        bValue = true;
        list_geom_fld_defn.push_back(bValue, "is_geom");

        std::string geomFldName(OGR_GFld_GetNameRef(hGeomFldDefn));
        if (geomFldName == "")
            geomFldName = this->defaultGeomFldName;
        list_out.push_back(list_geom_fld_defn, geomFldName);
    }

    return list_out;
}

void GDALVector::setAttributeFilter(const std::string &query) {
    checkAccess_(GA_ReadOnly);

    const char *query_in = nullptr;
    if (query != "")
        query_in = query.c_str();

    if (OGR_L_SetAttributeFilter(m_hLayer, query_in) != OGRERR_NONE) {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
        Rcpp::stop("error setting attribute filter");
    }
    else {
        m_attr_filter = query;
    }
}

std::string GDALVector::getAttributeFilter() const {
    checkAccess_(GA_ReadOnly);

    return(m_attr_filter);
}

void GDALVector::setIgnoredFields(const Rcpp::RObject &fields) {
    checkAccess_(GA_ReadOnly);

    if (fields.isNULL() || !Rcpp::is<Rcpp::CharacterVector>(fields))
        Rcpp::stop("'fields' must be a character vector");

    Rcpp::CharacterVector fields_(fields);
    std::vector<const char *> fields_in(fields_.begin(), fields_.end());
    fields_in.push_back(nullptr);

    if (!OGR_L_TestCapability(m_hLayer, OLCIgnoreFields))
        Rcpp::stop("this layer does not have ignored fields capability");

    if (fields_in[0] == nullptr || EQUAL(fields_in[0], "")) {
        OGR_L_SetIgnoredFields(m_hLayer, nullptr);
    }
    else {
        if (OGR_L_SetIgnoredFields(m_hLayer, fields_in.data()) != OGRERR_NONE)
            Rcpp::stop("not all field names could be resolved");
    }
}

void GDALVector::setSpatialFilter(const std::string &wkt) {
    checkAccess_(GA_ReadOnly);

    OGRGeometryH hFilterGeom = nullptr;
    if (wkt != "") {
        char *pszWKT = (char *) wkt.c_str();
        if (OGR_G_CreateFromWkt(&pszWKT, nullptr, &hFilterGeom) !=
                OGRERR_NONE) {
            if (hFilterGeom != nullptr)
                OGR_G_DestroyGeometry(hFilterGeom);
            Rcpp::stop("failed to create geometry from 'wkt'");
        }
    }

    OGR_L_SetSpatialFilter(m_hLayer, hFilterGeom);

    if (hFilterGeom != nullptr)
        OGR_G_DestroyGeometry(hFilterGeom);
}

void GDALVector::setSpatialFilterRect(const Rcpp::RObject &bbox) {
    checkAccess_(GA_ReadOnly);

    if (bbox.isNULL() || !Rcpp::is<Rcpp::NumericVector>(bbox))
        Rcpp::stop("'bbox' must be a numeric vector");

    Rcpp::NumericVector bbox_in(bbox);

    if (Rcpp::any(Rcpp::is_na(bbox_in)))
        Rcpp::stop("'bbox' has one or more 'NA' values");

    OGR_L_SetSpatialFilterRect(m_hLayer, bbox_in[0], bbox_in[1], bbox_in[2],
                               bbox_in[3]);
}

std::string GDALVector::getSpatialFilter() const {
    checkAccess_(GA_ReadOnly);

    OGRGeometryH hFilterGeom = nullptr;
    hFilterGeom = OGR_L_GetSpatialFilter(m_hLayer);
    if (hFilterGeom != nullptr) {
        char *pszWKT = nullptr;
        OGR_G_ExportToWkt(hFilterGeom, &pszWKT);
        std::string wkt(pszWKT);
        CPLFree(pszWKT);
        return wkt;
    }
    else {
        return "";
    }
}

void GDALVector::clearSpatialFilter() {
    checkAccess_(GA_ReadOnly);

    OGR_L_SetSpatialFilter(m_hLayer, nullptr);
}

double GDALVector::getFeatureCount() {
    // OGR_L_GetFeatureCount() returns GIntBig, return as R numeric for now
    // GDAL doc: Note that some implementations of this method may alter the
    // read cursor of the layer.
    // see: testCapability("FastFeatureCount")
    checkAccess_(GA_ReadOnly);

    return static_cast<double>(OGR_L_GetFeatureCount(m_hLayer, true));
}

SEXP GDALVector::getNextFeature() {
    checkAccess_(GA_ReadOnly);

    Rcpp::DataFrame df = fetch(1);
    if (df.nrows() == 0) {
        return R_NilValue;
    }
    else {
        // return as list
        df.attr("class") = R_NilValue;
        df.attr("row.names") = R_NilValue;

        // unlist fields that originate in a data frame list column
        for (R_xlen_t i = 0; i < df.size(); i++) {
            if (Rcpp::is<Rcpp::List>(df[i])) {
                Rcpp::List list_tmp = df[i];
                df[i] = list_tmp[0];
            }
        }

        return df;
    }
}

void GDALVector::setNextByIndex(double i) {
    checkAccess_(GA_ReadOnly);

    GIntBig index_in = 0;
    if (i < 0 || Rcpp::NumericVector::is_na(i) || std::isnan(i)) {
        Rcpp::stop("'i' must be a whole number >= 0");
    }
    else if (std::isinf(i) || i > MAX_INT_AS_R_NUMERIC) {
        Rcpp::stop("'i' is out of range");
    }
    else {
        index_in = static_cast<GIntBig>(std::trunc(i));
    }

    if (OGR_L_SetNextByIndex(m_hLayer, index_in) != OGRERR_NONE) {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
        Rcpp::stop("failed to set cursor position by index");
    }
}

SEXP GDALVector::getFeature(const Rcpp::RObject &fid) {
    // fid must be an R numeric vector of length 1, i.e., a scalar but using
    // NumericVector since it can carry the class attribute for integer64.
    // Instead of wrapping OGR_L_GetFeature(), we use fetch() because it
    // already builds the return data structure.

    checkAccess_(GA_ReadOnly);

    if (fid.isNULL() || !Rcpp::is<Rcpp::NumericVector>(fid))
        return R_NilValue;

    Rcpp::NumericVector fid_(fid);
    if (fid_.size() != 1)
        Rcpp::stop("'fid' must be a length-1 numeric vector (integer64)");

    int64_t fid_in = OGRNullFID;
    if (Rcpp::isInteger64(fid_))
        fid_in = Rcpp::fromInteger64(fid_[0]);
    else
        fid_in = static_cast<int64_t>(fid_[0]);

    // save the current attribute and spatial filters
    std::string orig_filter = m_attr_filter;
    OGRGeometryH hOrigFilterGeom = nullptr;
    OGRGeometryH hFilterGeom = nullptr;
    hFilterGeom = OGR_L_GetSpatialFilter(m_hLayer);
    if (hFilterGeom != nullptr) {
        hOrigFilterGeom = OGR_G_Clone(hFilterGeom);
        hFilterGeom = nullptr;
    }

    // filter on FID
    if (hOrigFilterGeom != nullptr)
        clearSpatialFilter();
    std::string fid_name = "FID";
    if (EQUAL(m_dialect.c_str(), "SQLITE"))
        fid_name = "rowid";
    else if (getFIDColumn() != "")
        fid_name = getFIDColumn();
    setAttributeFilter(fid_name + " = " + std::to_string(fid_in));

    Rcpp::DataFrame df = fetch(1);

    // restore originals
    setAttributeFilter(orig_filter);
    if (hOrigFilterGeom != nullptr) {
        OGR_L_SetSpatialFilter(m_hLayer, hOrigFilterGeom);
        OGR_G_DestroyGeometry(hOrigFilterGeom);
        hOrigFilterGeom = nullptr;
    }

    if (df.nrows() == 0) {
        return R_NilValue;
    }
    else {
        // return as list
        df.attr("class") = R_NilValue;
        df.attr("row.names") = R_NilValue;

        // unlist fields that originate in a data frame list column
        for (R_xlen_t i = 0; i < df.size(); i++) {
            if (Rcpp::is<Rcpp::List>(df[i])) {
                Rcpp::List list_tmp = df[i];
                df[i] = list_tmp[0];
            }
        }

        return df;
    }
}

void GDALVector::resetReading() {
    checkAccess_(GA_ReadOnly);

    OGR_L_ResetReading(m_hLayer);
}

Rcpp::DataFrame GDALVector::fetch(double n) {
    // Analog of DBI::dbFetch(), generally following its specification:
    // https://dbi.r-dbi.org/reference/dbFetch.html#specification
    // this method must be kept consistent with initDF_()
    checkAccess_(GA_ReadOnly);

    OGRFeatureDefnH hFDefn = nullptr;
    hFDefn = OGR_L_GetLayerDefn(m_hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    bool fetch_all = true;
    size_t fetch_num = 0;
    if (n == -1 || (std::isinf(n) && n > 0)) {
        resetReading();
        fetch_num = OGR_L_GetFeatureCount(m_hLayer, true);
    }
    else if (Rcpp::NumericVector::is_na(n)) {
        fetch_num = OGR_L_GetFeatureCount(m_hLayer, true);
    }
    else if (n >= 0) {
        if (n > MAX_INT_AS_R_NUMERIC)
            Rcpp::stop("'n' is out of range");
        fetch_all = false;
        fetch_num = static_cast<size_t>(std::trunc(n));
    }
    else {
        Rcpp::stop("'n' is invalid");
    }

    Rcpp::DataFrame df = initDF_(fetch_num);
    if (fetch_num == 0)
        return df;

    int nFields = OGR_FD_GetFieldCount(hFDefn);
    int nGeomFields = OGR_FD_GetGeomFieldCount(hFDefn);
    bool include_geom = true;
    if (EQUAL(this->returnGeomAs.c_str(), "NONE")) {
        include_geom = false;
    }
    else if (!(EQUAL(this->returnGeomAs.c_str(), "WKB") ||
               EQUAL(this->returnGeomAs.c_str(), "WKB_ISO") ||
               EQUAL(this->returnGeomAs.c_str(), "WKT") ||
               EQUAL(this->returnGeomAs.c_str(), "WKT_ISO") ||
               EQUAL(this->returnGeomAs.c_str(), "TYPE_NAME"))) {
        Rcpp::stop("unsupported value of field 'returnGeomAs'");
    }

    OGRwkbByteOrder eOrder;
    if (EQUAL(this->wkbByteOrder.c_str(), "LSB"))
        eOrder = wkbNDR;
    else if (EQUAL(this->wkbByteOrder.c_str(), "MSB"))
        eOrder = wkbXDR;
    else
        Rcpp::stop("invalid value of field 'wkbByteOrder'");

    OGRFeatureH hFeat = nullptr;
    size_t row_num = 0;

    while ((hFeat = OGR_L_GetNextFeature(m_hLayer)) != nullptr) {
        size_t col_num = 0;

        const int64_t fid = static_cast<int64_t>(OGR_F_GetFID(hFeat));
        Rcpp::NumericVector fid_col = df[col_num];
        fid_col[row_num] = Rcpp::toInteger64(fid)[0];

        for (int i = 0; i < nFields; ++i) {
            OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, i);
            if (hFieldDefn == nullptr)
                Rcpp::stop("could not obtain field definition");

            if (OGR_Fld_IsIgnored(hFieldDefn))
                continue;

            col_num += 1;

            bool has_value = true;
            if (!OGR_F_IsFieldSetAndNotNull(hFeat, i))
                has_value = false;

            OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);

            switch (fld_type) {
                case OFTInteger:
                {
                    if (!has_value)
                        continue;

                    OGRFieldSubType fld_subtype =
                            OGR_Fld_GetSubType(hFieldDefn);

                    if (fld_subtype == OFSTBoolean) {
                        Rcpp::LogicalVector col = df[col_num];
                        col[row_num] = OGR_F_GetFieldAsInteger(hFeat, i);
                    }
                    else {
                        Rcpp::IntegerVector col = df[col_num];
                        col[row_num] = OGR_F_GetFieldAsInteger(hFeat, i);
                    }
                }
                break;

                case OFTInteger64:
                {
                    if (!has_value)
                        continue;

                    const int64_t value = static_cast<int64_t>(
                            OGR_F_GetFieldAsInteger64(hFeat, i));

                    Rcpp::NumericVector col = df[col_num];
                    col[row_num] = Rcpp::toInteger64(value)[0];
                }
                break;

                case OFTReal:
                {
                    if (!has_value)
                        continue;

                    Rcpp::NumericVector col = df[col_num];
                    col[row_num] = OGR_F_GetFieldAsDouble(hFeat, i);
                }
                break;

                case OFTDate:
                case OFTDateTime:
                {
                    if (!has_value)
                        continue;

                    int yr = 0, mo = 0, day = 0, hr = 0, min = 0, tzflag = 0;
                    float sec = 0;
                    if (!OGR_F_GetFieldAsDateTimeEx(hFeat, i, &yr, &mo,
                                                    &day, &hr, &min, &sec,
                                                    &tzflag)) {
                        continue;
                    }

                    struct tm brokendowntime;
                    brokendowntime.tm_year = yr - 1900;
                    brokendowntime.tm_mon = mo - 1;
                    brokendowntime.tm_mday = day;
                    brokendowntime.tm_hour = hr;
                    brokendowntime.tm_min = min;
                    brokendowntime.tm_sec = static_cast<int>(sec);
                    int64_t nUnixTime = CPLYMDHMSToUnixTime(&brokendowntime);

                    if (fld_type == OFTDate) {
                        Rcpp::NumericVector col = df[col_num];
                        col[row_num] = static_cast<double>(nUnixTime / 86400);
                    }
                    else {
                        // OFTDateTime
                        if (tzflag > 1 && tzflag != 100) {
                            // convert to UTC
                            const int tzoffset = std::abs(tzflag - 100) * 15;
                            const int tzhour = tzoffset / 60;
                            const int tzmin = tzoffset - tzhour * 60;
                            const int offset_sec = tzhour * 3600 + tzmin * 60;
                            if (tzflag >= 100)
                                nUnixTime -= offset_sec;
                            else
                                nUnixTime += offset_sec;
                        }
                        Rcpp::NumericVector col = df[col_num];
                        col[row_num] = static_cast<double>(
                                nUnixTime + std::fmod(sec, 1));
                    }
                }
                break;

                case OFTBinary:
                {
                    Rcpp::List col = df[col_num];
                    if (!has_value) {
                        col[row_num] = R_NilValue;
                    }
                    else {
                        int nDataSize = 0;
                        GByte *pabyData = OGR_F_GetFieldAsBinary(hFeat, i,
                                                                 &nDataSize);

                        if (nDataSize > 0) {
                            Rcpp::RawVector blob(nDataSize);
                            std::memcpy(&blob[0], pabyData, nDataSize);
                            col[row_num] = blob;
                        }
                        else {
                            col[row_num] = Rcpp::RawVector::create();
                        }
                    }
                }
                break;

                case OFTIntegerList:
                {
                    Rcpp::List col = df[col_num];
                    if (!has_value) {
                        col[row_num] = NA_INTEGER;
                    }
                    else {
                        int nCount = 0;
                        const int *panValue =
                                OGR_F_GetFieldAsIntegerList(hFeat, i, &nCount);

                        if (nCount > 0) {
                            std::vector<int> v(panValue, panValue + nCount);
                            col[row_num] = Rcpp::wrap(v);
                        }
                        else {
                            col[row_num] = Rcpp::IntegerVector::create();
                        }
                    }
                }
                break;

                case OFTInteger64List:
                {
                    Rcpp::List col = df[col_num];
                    if (!has_value) {
                        std::vector<int64_t> v = {NA_INTEGER64};
                        col[row_num] = Rcpp::wrap(v);
                    }
                    else {
                        int nCount = 0;
                        const int64_t *panValue =
                                reinterpret_cast<const int64_t *>(
                                    OGR_F_GetFieldAsInteger64List(hFeat, i,
                                                                  &nCount));

                        if (nCount > 0) {
                            std::vector<int64_t> v(panValue, panValue + nCount);
                            col[row_num] = Rcpp::wrap(v);
                        }
                        else {
                            Rcpp::NumericVector v = {};
                            v.attr("class") = "integer64";
                            col[row_num] = v;
                        }
                    }
                }
                break;

                case OFTRealList:
                {
                    Rcpp::List col = df[col_num];
                    if (!has_value) {
                        col[row_num] = NA_REAL;
                    }
                    else {
                        int nCount = 0;
                        const double *padfValue =
                                OGR_F_GetFieldAsDoubleList(hFeat, i, &nCount);

                        if (nCount > 0) {
                            std::vector<double> v(padfValue,
                                                  padfValue + nCount);

                            col[row_num] = Rcpp::wrap(v);
                        }
                        else {
                            col[row_num] = Rcpp::NumericVector::create();
                        }
                    }
                }
                break;

                case OFTStringList:
                {
                    Rcpp::List col = df[col_num];
                    if (!has_value) {
                        col[row_num] = NA_STRING;
                    }
                    else {
                        char **papszValue =
                                OGR_F_GetFieldAsStringList(hFeat, i);

                        int nCount = 0;
                        nCount = CSLCount(papszValue);
                        if (nCount > 0) {
                            std::vector<std::string> v(papszValue,
                                                       papszValue + nCount);

                            col[row_num] = Rcpp::wrap(v);
                        }
                        else {
                            col[row_num] = Rcpp::CharacterVector::create();
                        }
                    }
                }
                break;

                default:
                {
                    if (has_value) {
                        Rcpp::CharacterVector col = df[col_num];
                        col[row_num] = OGR_F_GetFieldAsString(hFeat, i);
                    }
                }
                break;
            }
        }

        if (include_geom) {
            for (int i = 0; i < nGeomFields; ++i) {
                OGRGeomFieldDefnH hGeomFldDefn =
                        OGR_F_GetGeomFieldDefnRef(hFeat, i);
                if (hGeomFldDefn == nullptr)
                    Rcpp::stop("could not obtain geometry field def");

                if (OGR_GFld_IsIgnored(hGeomFldDefn))
                    continue;

                OGRGeometryH hGeom = nullptr;
                bool destroy_geom = false;
                OGRGeometryH hGeomRef = OGR_F_GetGeomFieldRef(hFeat, i);
                if (hGeomRef && this->promoteToMulti) {
                    OGRwkbGeometryType geom_type = OGR_GT_Flatten(
                            OGR_G_GetGeometryType(hGeomRef));

                    switch (geom_type) {
                        case wkbPolygon:
                        {
                            hGeom = OGR_G_ForceToMultiPolygon(
                                    OGR_G_Clone(hGeomRef));
                            destroy_geom = true;
                        }
                        break;

                        case wkbPoint:
                        {
                            hGeom = OGR_G_ForceToMultiPoint(
                                    OGR_G_Clone(hGeomRef));
                            destroy_geom = true;
                        }
                        break;

                        case wkbLineString:
                        {
                            hGeom = OGR_G_ForceToMultiLineString(
                                    OGR_G_Clone(hGeomRef));
                            destroy_geom = true;
                        }
                        break;

                        default:
                            hGeom = OGR_F_GetGeomFieldRef(hFeat, i);
                    }
                }
                else {
                    hGeom = OGR_F_GetGeomFieldRef(hFeat, i);
                }

                col_num += 1;

                if (STARTS_WITH_CI(this->returnGeomAs.c_str(), "WKB")) {
                    if (hGeom == nullptr)
                        continue;

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
                    const int nWKBSize = OGR_G_WkbSizeEx(hGeom);
#else
                    const int nWKBSize = OGR_G_WkbSize(hGeom);
#endif
                    if (nWKBSize) {
                        Rcpp::RawVector wkb(nWKBSize);

                        if (EQUAL(this->returnGeomAs.c_str(), "WKB"))
                            OGR_G_ExportToWkb(hGeom, eOrder, &wkb[0]);
                        else if (EQUAL(this->returnGeomAs.c_str(), "WKB_ISO"))
                            OGR_G_ExportToIsoWkb(hGeom, eOrder, &wkb[0]);

                        Rcpp::List col = df[col_num];
                        col[row_num] = wkb;
                    }
                    else {
                        Rcpp::List col = df[col_num];
                        col[row_num] = Rcpp::RawVector::create();
                    }
                }

                else if (STARTS_WITH_CI(this->returnGeomAs.c_str(), "WKT")) {
                    Rcpp::CharacterVector col = df[col_num];
                    if (hGeom == nullptr) {
                        col[row_num] = NA_STRING;
                    }
                    else {
                        char *pszWKT;
                        if (EQUAL(this->returnGeomAs.c_str(), "WKT"))
                            OGR_G_ExportToWkt(hGeom, &pszWKT);
                        else if (EQUAL(this->returnGeomAs.c_str(), "WKT_ISO"))
                            OGR_G_ExportToIsoWkt(hGeom, &pszWKT);

                        col[row_num] = pszWKT;
                        CPLFree(pszWKT);
                    }
                }

                else if (EQUAL(this->returnGeomAs.c_str(), "TYPE_NAME")) {
                    Rcpp::CharacterVector col = df[col_num];
                    if (hGeom != nullptr)
                        col[row_num] = OGR_G_GetGeometryName(hGeom);
                    else
                        col[row_num] = NA_STRING;
                }

                if (destroy_geom)
                    OGR_G_DestroyGeometry(hGeom);
            }
        }

        OGR_F_Destroy(hFeat);
        hFeat = nullptr;

        row_num += 1;
        if (row_num == fetch_num)
            break;
    }

    if (fetch_all) {
        hFeat = OGR_L_GetNextFeature(m_hLayer);
        if (hFeat != nullptr) {
            Rcpp::Rcout << "getFeatureCount() reported: " << row_num
                    << std::endl;
            std::string msg =
                    "more features potentially available than reported by getFeatureCount()";
            Rcpp::warning(msg);
            OGR_F_Destroy(hFeat);
            hFeat = nullptr;
        }
    }

    if (row_num == fetch_num) {
        return df;
    }
    else {
        // Truncate the data frame by copying to a new one. Hard to avoid
        // a copy here since Rcpp vectors cannot be resized. This is only
        // needed for the last page when paging through features with repeated
        // calls to fetch(n), so the data generally should not be large enough
        // for this to be a problem.
        Rcpp::DataFrame df_trunc = initDF_(row_num);
        size_t col_num = 0;

        Rcpp::NumericVector fid_col = df[col_num];
        Rcpp::NumericVector fid_col_trunc = df_trunc[0];
        std::copy_n(fid_col.cbegin(), row_num, fid_col_trunc.begin());

        for (int i = 0; i < OGR_FD_GetFieldCount(hFDefn); ++i) {
            OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, i);
            if (OGR_Fld_IsIgnored(hFieldDefn))
                continue;

            col_num += 1;

            switch (OGR_Fld_GetType(hFieldDefn)) {
                case OFTInteger:
                {
                    OGRFieldSubType fld_subtype =
                            OGR_Fld_GetSubType(hFieldDefn);

                    if (fld_subtype == OFSTBoolean) {
                        Rcpp::LogicalVector col = df[col_num];
                        Rcpp::LogicalVector col_trunc = df_trunc[i + 1];
                        std::copy_n(col.cbegin(), row_num, col_trunc.begin());
                    }
                    else {
                        Rcpp::IntegerVector col = df[col_num];
                        Rcpp::IntegerVector col_trunc = df_trunc[i + 1];
                        std::copy_n(col.cbegin(), row_num, col_trunc.begin());
                    }
                }
                break;

                case OFTInteger64:
                case OFTReal:
                case OFTDate:
                case OFTDateTime:
                {
                    Rcpp::NumericVector col = df[col_num];
                    Rcpp::NumericVector col_trunc = df_trunc[i + 1];
                    std::copy_n(col.cbegin(), row_num, col_trunc.begin());
                }
                break;

                case OFTBinary:
                case OFTIntegerList:
                case OFTInteger64List:
                case OFTRealList:
                case OFTStringList:
                {
                    Rcpp::List col = df[col_num];
                    Rcpp::List col_trunc = df_trunc[i + 1];
                    for (size_t n = 0; n < row_num; ++n)
                        col_trunc[n] = col[n];
                }
                break;

                default:
                {
                    Rcpp::CharacterVector col = df[col_num];
                    Rcpp::CharacterVector col_trunc = df_trunc[i + 1];
                    std::copy_n(col.cbegin(), row_num, col_trunc.begin());
                }
                break;
            }
        }

        if (include_geom) {
            for (int i = 0; i < nGeomFields; ++i) {
                OGRGeomFieldDefnH hGeomFldDefn =
                        OGR_F_GetGeomFieldDefnRef(hFeat, i);

                if (OGR_GFld_IsIgnored(hGeomFldDefn))
                    continue;

                col_num += 1;

                if (STARTS_WITH_CI(this->returnGeomAs.c_str(), "WKB")) {
                    Rcpp::List col = df[col_num];
                    Rcpp::List col_trunc = df_trunc[col_num];
                    for (size_t n = 0; n < row_num; ++n)
                        col_trunc[n] = col[n];
                }
                else {
                    Rcpp::CharacterVector col = df[col_num];
                    Rcpp::CharacterVector col_trunc = df_trunc[col_num];
                    std::copy_n(col.cbegin(), row_num, col_trunc.begin());
                }
            }
        }

        return df_trunc;
    }
}

bool GDALVector::setFeature(const Rcpp::RObject &feature) {
    checkAccess_(GA_Update);

    // input validation is done in OGRFeatureFromList_():
    OGRFeatureH hFeat = OGRFeatureFromList_(feature);

    OGRErr err = OGR_L_SetFeature(m_hLayer, hFeat);
     if (err != OGRERR_NONE) {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
        OGR_F_Destroy(hFeat);
        return false;
     }
     else {
        m_last_write_fid = OGR_F_GetFID(hFeat);
        OGR_F_Destroy(hFeat);
        return true;
     }
}

bool GDALVector::createFeature(const Rcpp::RObject &feature) {
    checkAccess_(GA_Update);

    // input validation is done in OGRFeatureFromList_():
    OGRFeatureH hFeat = OGRFeatureFromList_(feature);

    OGRErr err = OGR_L_CreateFeature(m_hLayer, hFeat);
     if (err != OGRERR_NONE) {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
        OGR_F_Destroy(hFeat);
        return false;
     }
     else {
        m_last_write_fid = OGR_F_GetFID(hFeat);
        OGR_F_Destroy(hFeat);
        return true;
     }
}

bool GDALVector::upsertFeature(const Rcpp::RObject &feature) {
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 6, 0)
    Rcpp::stop("'upsertFeature() requires GDAL >= 3.6");
#else
    checkAccess_(GA_Update);

    // input validation is done in OGRFeatureFromList_():
    OGRFeatureH hFeat = OGRFeatureFromList_(feature);

    OGRErr err = OGR_L_UpsertFeature(m_hLayer, hFeat);
     if (err != OGRERR_NONE) {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
        OGR_F_Destroy(hFeat);
        return false;
     }
     else {
        m_last_write_fid = OGR_F_GetFID(hFeat);
        OGR_F_Destroy(hFeat);
        return true;
     }
#endif
}

SEXP GDALVector::getLastWriteFID() const {
    checkAccess_(GA_ReadOnly);

    if (m_last_write_fid == NA_INTEGER64) {
        return R_NilValue;
    }
    else {
        std::vector<int64_t> last_fid = {m_last_write_fid};
        return Rcpp::wrap(last_fid);
    }
}

bool GDALVector::deleteFeature(const Rcpp::RObject &fid) {
    // fid must be an R numeric vector of length 1, i.e., a scalar but using
    // NumericVector since it can carry the class attribute for integer64.

    if (m_eAccess == GA_ReadOnly) {
        Rcpp::Rcerr << "cannot delete, the layer was opened read-only" <<
                std::endl;
        return false;
    }
    else if (!OGR_L_TestCapability(m_hLayer, OLCDeleteFeature)) {
        Rcpp::Rcerr << "the layer does not have delete feature capability" <<
                std::endl;
        return false;
    }

    if (fid.isNULL() || !Rcpp::is<Rcpp::NumericVector>(fid))
        return R_NilValue;

    Rcpp::NumericVector fid_(fid);
    if (fid_.size() != 1)
        Rcpp::stop("'fid' must be a length-1 numeric vector (integer64)");

    if (fid_.size() != 1)
        Rcpp::stop("'fid' must be a length-1 numeric vector (integer64)");

    int64_t fid_in = OGRNullFID;
    if (Rcpp::isInteger64(fid_))
        fid_in = Rcpp::fromInteger64(fid_[0]);
    else
        fid_in = static_cast<int64_t>(fid_[0]);

    if (OGR_L_DeleteFeature(m_hLayer, fid_in) != OGRERR_NONE) {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    else {
        return true;
    }
}

bool GDALVector::syncToDisk() const {
    checkAccess_(GA_ReadOnly);

    OGRErr err = OGR_L_SyncToDisk(m_hLayer);
    if (err == OGRERR_NONE)
        return true;
    else
        return false;
}

bool GDALVector::startTransaction(bool force) {
    checkAccess_(GA_ReadOnly);

    if (!force) {
        if (!GDALDatasetTestCapability(m_hDataset, ODsCTransactions)) {
            Rcpp::Rcerr << "dataset does not have (efficient) transaction capability"
                    << std::endl;
            return false;
        }
    }
    else {
        if (!GDALDatasetTestCapability(m_hDataset, ODsCTransactions) &&
            !GDALDatasetTestCapability(m_hDataset, ODsCEmulatedTransactions)) {

            Rcpp::Rcerr << "dataset does not have transaction capability"
                    << std::endl;
            return false;
        }
    }

    if (GDALDatasetStartTransaction(m_hDataset, force) != OGRERR_NONE) {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    else {
        return true;
    }
}

bool GDALVector::commitTransaction() {
    checkAccess_(GA_ReadOnly);

    if (GDALDatasetCommitTransaction(m_hDataset) != OGRERR_NONE) {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    else {
        return true;
    }
}

bool GDALVector::rollbackTransaction() {
    checkAccess_(GA_ReadOnly);

    if (GDALDatasetRollbackTransaction(m_hDataset) != OGRERR_NONE) {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    else {
        return true;
    }
}

bool GDALVector::layerIntersection(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        const Rcpp::Nullable<const Rcpp::CharacterVector> &options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    bool ret = false;
    OGRErr err = OGR_L_Intersection(
                    m_hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if(!quiet) {
        Rcpp::Rcerr << "error during Intersection, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerUnion(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        const Rcpp::Nullable<const Rcpp::CharacterVector> &options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    bool ret = false;
    OGRErr err = OGR_L_Union(
                    m_hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if(!quiet) {
        Rcpp::Rcerr << "error during Union, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerSymDifference(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        const Rcpp::Nullable<const Rcpp::CharacterVector> &options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    bool ret = false;
    OGRErr err = OGR_L_SymDifference(
                    m_hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if(!quiet) {
        Rcpp::Rcerr << "error during SymDifference, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerIdentity(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        const Rcpp::Nullable<const Rcpp::CharacterVector> &options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    bool ret = false;
    OGRErr err = OGR_L_Identity(
                    m_hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if(!quiet) {
        Rcpp::Rcerr << "error during Identity, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerUpdate(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        const Rcpp::Nullable<const Rcpp::CharacterVector> &options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    bool ret = false;
    OGRErr err = OGR_L_Update(
                    m_hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if(!quiet) {
        Rcpp::Rcerr << "error during Update, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerClip(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        const Rcpp::Nullable<const Rcpp::CharacterVector> &options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    bool ret = false;
    OGRErr err = OGR_L_Clip(
                    m_hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if(!quiet) {
        Rcpp::Rcerr << "error during Clip, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerErase(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        const Rcpp::Nullable<const Rcpp::CharacterVector> &options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    bool ret = false;
    OGRErr err = OGR_L_Erase(
                    m_hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if(!quiet) {
        Rcpp::Rcerr << "error during Erase, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

void GDALVector::close() {
    if (m_hDataset != nullptr) {
        if (m_is_sql)
            GDALDatasetReleaseResultSet(m_hDataset, m_hLayer);
        GDALReleaseDataset(m_hDataset);
        m_hDataset = nullptr;
        m_hLayer = nullptr;
    }
}

void GDALVector::OGRFeatureFromList_dumpReadble(
                 const Rcpp::RObject &feat) const {

    // undocumented method exposed in R for diagnostic use

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 8, 0)
    Rcpp::stop("'OGRFeatureFromList_dumpReadble()' requires GDAL >= 3.8");

#else
    // input validation is done in OGRFeatureFromList_()
    OGRFeatureH hFeat = OGRFeatureFromList_(feat);

    if (hFeat == nullptr)
        Rcpp::stop("'OGRFeatureFromList_()' returned nullptr");
    std::vector<const char *> options = {"DISPLAY_GEOMETRY=SUMMARY", nullptr};
    char *out = nullptr;
    out = OGR_F_DumpReadableAsString(hFeat, options.data());
    if (out != nullptr)
        Rcpp::Rcout << out;
    CPLFree(out);
    OGR_F_Destroy(hFeat);
#endif
}

// ****************************************************************************
// class methods for internal use not exposed in R
// ****************************************************************************

void GDALVector::checkAccess_(GDALAccess access_needed) const {
    if (!isOpen())
        Rcpp::stop("dataset is not open");

    if (access_needed == GA_Update && m_eAccess == GA_ReadOnly)
        Rcpp::stop("dataset is read-only");
}

GDALDatasetH GDALVector::getGDALDatasetH_() const {
    checkAccess_(GA_ReadOnly);

    return m_hDataset;
}

void GDALVector::setGDALDatasetH_(const GDALDatasetH hDs, bool with_update) {
    m_hDataset = hDs;
    if (with_update)
        m_eAccess = GA_Update;
    else
        m_eAccess = GA_ReadOnly;
}

OGRLayerH GDALVector::getOGRLayerH_() const {
    checkAccess_(GA_ReadOnly);

    return m_hLayer;
}

void GDALVector::setOGRLayerH_(const OGRLayerH hLyr,
                               const std::string &lyr_name) {
    m_hLayer = hLyr;
    m_layer_name = lyr_name;
}

void GDALVector::setFeatureTemplate_() {
    std::string orig_geom_as(this->returnGeomAs);
    this->returnGeomAs = "WKT";

    Rcpp::DataFrame feat_template = initDF_(1);
    // as list
    feat_template.attr("class") = R_NilValue;
    feat_template.attr("row.names") = R_NilValue;

    // set null values for the list columns if any
    OGRFeatureDefnH hFDefn = nullptr;
    hFDefn = OGR_L_GetLayerDefn(m_hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    int nFields = OGR_FD_GetFieldCount(hFDefn);
    for (int i = 0; i < nFields; ++i) {
        OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, i);
        if (hFieldDefn == nullptr)
            Rcpp::stop("could not obtain field definition");

        OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);

        // the first element in feat_template is FID so [i + 1]
        if (fld_type == OFTBinary)
            feat_template[i + 1] = Rcpp::RawVector::create();
        else if (fld_type == OFTIntegerList)
            feat_template[i + 1] = NA_INTEGER;
        else if (fld_type == OFTInteger64List)
            feat_template[i + 1] = NA_INTEGER64;
        else if (fld_type == OFTRealList)
            feat_template[i + 1] = NA_REAL;
        else if (fld_type == OFTStringList)
            feat_template[i + 1] = NA_STRING;
    }

    this->featureTemplate = feat_template;
    this->returnGeomAs = orig_geom_as;
}

SEXP GDALVector::initDF_(R_xlen_t nrow) const {
    // initialize a data frame based on the layer definition
    // this mthod must be kept consistent with fetch()

    OGRFeatureDefnH hFDefn = nullptr;
    hFDefn = OGR_L_GetLayerDefn(m_hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    int nFields = OGR_FD_GetFieldCount(hFDefn);
    int nIgnoredFields = 0;
    for (int i = 0; i < nFields; ++i) {
        OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, i);
        if (hFieldDefn == nullptr)
            Rcpp::stop("could not obtain field definition");

        if (OGR_Fld_IsIgnored(hFieldDefn))
            nIgnoredFields += 1;
    }

    int nGeomFields = 0;
    int nIgnoredGeomFields = 0;
    if (!EQUAL(this->returnGeomAs.c_str(), "NONE")) {
        nGeomFields = OGR_FD_GetGeomFieldCount(hFDefn);
        for (int i = 0; i < nGeomFields; ++i) {
            OGRGeomFieldDefnH hGeomFldDefn = OGR_FD_GetGeomFieldDefn(hFDefn, i);
            if (hGeomFldDefn == nullptr)
                Rcpp::stop("could not obtain geometry field def");

            if (OGR_GFld_IsIgnored(hGeomFldDefn))
                nIgnoredGeomFields += 1;
        }
    }

    // construct as list and convert to data frame at return
    int nOutCols = (1 + nFields - nIgnoredFields + nGeomFields -
                    nIgnoredGeomFields);
    Rcpp::List df(nOutCols);
    Rcpp::CharacterVector col_names(nOutCols);

    size_t col_num = 0;

    std::vector<int64_t> fid{};
    try {
        fid.resize(nrow, NA_INTEGER64);
    }
    catch (const std::exception &) {
        Rcpp::stop("failed to allocate memory for 'fid' column");
    }
    df[col_num] = Rcpp::wrap(fid);
    col_names[col_num] = "FID";

    for (int i = 0; i < nFields; ++i) {
        OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, i);
        if (hFieldDefn == nullptr)
            Rcpp::stop("could not obtain field definition");

        if (OGR_Fld_IsIgnored(hFieldDefn))
            continue;

        col_num += 1;
        if (static_cast<int>(col_num) == nOutCols)
            Rcpp::stop("indexing the output columns failed");

        switch (OGR_Fld_GetType(hFieldDefn)) {
            case OFTInteger:
            {
                OGRFieldSubType fld_subtype = OGR_Fld_GetSubType(hFieldDefn);
                if (fld_subtype == OFSTBoolean) {
                    Rcpp::LogicalVector v(nrow, NA_LOGICAL);
                    df[col_num] = v;
                }
                else {
                    Rcpp::IntegerVector v(nrow, NA_INTEGER);
                    df[col_num] = v;
                }
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            case OFTInteger64:
            {
                std::vector<int64_t> v{};
                try {
                    v.resize(nrow, NA_INTEGER64);
                }
                catch (const std::exception &) {
                    Rcpp::stop("failed to allocate memory 'integer64' column");
                }
                df[col_num] = Rcpp::wrap(v);
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            case OFTReal:
            {
                Rcpp::NumericVector v(nrow, NA_REAL);
                df[col_num] = v;
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            case OFTDate:
            {
                Rcpp::NumericVector v(nrow, NA_REAL);
                v.attr("class") = "Date";
                df[col_num] = v;
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            case OFTDateTime:
            {
                Rcpp::NumericVector v(nrow, NA_REAL);
                Rcpp::CharacterVector classes = {"POSIXct", "POSIXt"};
                v.attr("class") = classes;
                v.attr("tzone") = "UTC";
                df[col_num] = v;
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            case OFTBinary:
            case OFTIntegerList:
            case OFTInteger64List:
            case OFTRealList:
            case OFTStringList:
            {
                Rcpp::List v(nrow);
                df[col_num] = v;
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            default:
            {
                // use string
                Rcpp::CharacterVector v(nrow, NA_STRING);
                df[col_num] = v;
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;
        }
    }

    for (int i = 0; i < nGeomFields; ++i) {
        OGRGeomFieldDefnH hGeomFldDefn = OGR_FD_GetGeomFieldDefn(hFDefn, i);
        if (hGeomFldDefn == nullptr)
            Rcpp::stop("could not obtain geometry field def");

        if (OGR_GFld_IsIgnored(hGeomFldDefn))
            continue;

        col_num += 1;
        if (static_cast<int>(col_num) == nOutCols)
            Rcpp::stop("indexing the output columns failed");

        if (STARTS_WITH_CI(this->returnGeomAs.c_str(), "WKB")) {
            Rcpp::List v(nrow);
            df[col_num] = v;
        }
        else {
            Rcpp::CharacterVector v(nrow, NA_STRING);
            df[col_num] = v;
        }

        std::string geomFldName(OGR_GFld_GetNameRef(hGeomFldDefn));
        if (geomFldName == "")
            geomFldName = this->defaultGeomFldName;

        col_names[col_num] = geomFldName;
    }

    df.names() = col_names;
    df.attr("class") = "data.frame";
    df.attr("row.names") = Rcpp::seq_len(nrow);
    return df;
}

OGRFeatureH GDALVector::OGRFeatureFromList_(
                        const Rcpp::RObject &feature) const {

    // the returned feature must be destroyed with OGR_F_Destroy()

    if (feature.isNULL() || !Rcpp::is<Rcpp::List>(feature))
        Rcpp::stop("input must be a list or 1-row data frame");

    if (Rcpp::is<Rcpp::DataFrame>(feature)) {
        Rcpp::DataFrame df(feature);
        if (df.nrows() != 1)
            Rcpp::stop("data frame input must have 1 row");
    }

    Rcpp::List list_in(feature);

    if (!list_in.hasAttribute("names"))
        Rcpp::stop("input must be a named list");

    Rcpp::CharacterVector names = list_in.names();
    if (names.size() == 0)
        Rcpp::stop("input must be a named list");

    OGRFeatureDefnH hFDefn = nullptr;
    hFDefn = OGR_L_GetLayerDefn(m_hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    OGRFeatureH hFeat = nullptr;
    hFeat = OGR_F_Create(hFDefn);
    if (hFeat == nullptr)
        Rcpp::stop("failed to create feature object");

    int nGeomFields = OGR_F_GetGeomFieldCount(hFeat);

    std::map<R_xlen_t, int> map_flds{};
    std::map<R_xlen_t, int> map_geom_flds{};

    // go over the list element names and map list index to field index
    // set FID here if given
    for (R_xlen_t i = 0; i < names.size(); ++i) {
        // attribute fields
        int iField = OGR_F_GetFieldIndex(hFeat, names[i]);
        if (iField != -1) {
            map_flds.insert({i, iField});
            continue;
        }

        // set FID if one is given and is not NA
        if (EQUAL(names[i], "FID")) {
            if (!Rcpp::is<Rcpp::NumericVector>(list_in[i])) {
                OGR_F_Destroy(hFeat);
                Rcpp::stop("FID must be a numeric value (integer64)");
            }
            Rcpp::NumericVector fid_in = list_in[i];
            if (fid_in.size() != 1) {
                OGR_F_Destroy(hFeat);
                Rcpp::stop("FID must be length-1 numeric (integer64)");
            }
            int64_t fid = -1;
            if (Rcpp::isInteger64(fid_in)) {
                fid = Rcpp::fromInteger64(fid_in[0]);
                if (ISNA_INTEGER64(fid))
                    continue;
            }
            else {
                if (Rcpp::NumericVector::is_na(Rcpp::as<double>(fid_in)))
                    continue;
                fid = static_cast<int64_t>(Rcpp::as<double>(fid_in));
            }
            if (OGR_F_SetFID(hFeat, fid) != OGRERR_NONE) {
                OGR_F_Destroy(hFeat);
                Rcpp::Rcerr << "failed to set: " << names[i] << " = " << fid
                        << std::endl;
                Rcpp::stop("failed to set FID with the given input value");
            }
            continue;
        }

        // geom fields by name
        int iGeomField = OGR_F_GetGeomFieldIndex(hFeat, names[i]);
        if (iGeomField != -1) {
            map_geom_flds.insert({i, iGeomField});
            continue;
        }

        // case of geometry column name is empty as with shapefiles etc.
        if (nGeomFields == 1 && (
                EQUAL(names[1], this->defaultGeomFldName.c_str()) ||
                EQUAL(names[i], "_ogr_geometry_") ||
                EQUAL(names[i], "geometry") ||
                EQUAL(names[i], "geom"))) {

            map_geom_flds.insert({i, 0});
            continue;
        }

        OGR_F_Destroy(hFeat);
        Rcpp::Rcerr << "list element not matched: " << names[i] << std::endl;
        Rcpp::stop("failed to map input field names to layer definition");
    }

    // set attribute fields
    for (auto it = map_flds.begin(); it != map_flds.end(); ++it) {
        R_xlen_t list_idx = it->first;
        int fld_idx = it->second;

        OGRFieldDefnH hFieldDefn = nullptr;
        hFieldDefn = OGR_F_GetFieldDefnRef(hFeat, fld_idx);
        if (hFieldDefn == nullptr)
            Rcpp::stop("could not obtain field definition");

        OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);

        switch (fld_type) {
            case OFTInteger:
            {
                if (!(Rcpp::is<Rcpp::NumericVector>(list_in[list_idx]) ||
                      Rcpp::is<Rcpp::IntegerVector>(list_in[list_idx]) ||
                      Rcpp::is<Rcpp::LogicalVector>(list_in[list_idx]))) {

                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("OFTInteger requires compatible data type");
                }
                OGRFieldSubType fld_subtype = OGR_Fld_GetSubType(hFieldDefn);
                if (fld_subtype == OFSTBoolean) {
                    Rcpp::LogicalVector v = list_in[list_idx];
                    if (v.size() == 0 || Rcpp::LogicalVector::is_na(v(0)))
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    else
                        OGR_F_SetFieldInteger(hFeat, fld_idx, v[0]);
                }
                else {
                    Rcpp::IntegerVector v = list_in[list_idx];
                    if (v.size() == 0 || Rcpp::IntegerVector::is_na(v(0)))
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    else
                        OGR_F_SetFieldInteger(hFeat, fld_idx, v[0]);
                }
            }
            break;

            case OFTInteger64:
            {
                if (!Rcpp::is<Rcpp::NumericVector>(list_in[list_idx])) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("OFTInteger64 requires a numeric value");
                }
                Rcpp::NumericVector v = list_in[list_idx];
                int64_t value = NA_INTEGER64;
                if (v.size() > 0) {
                    if (Rcpp::isInteger64(v))
                        value = Rcpp::fromInteger64(v[0]);
                    else
                        value = static_cast<int64_t>(v[0]);
                }
                if (v.size() > 0 && !ISNA_INTEGER64(value))
                    OGR_F_SetFieldInteger64(hFeat, fld_idx, value);
                else
                    OGR_F_SetFieldNull(hFeat, fld_idx);
            }
            break;

            case OFTReal:
            {
                if (!Rcpp::is<Rcpp::NumericVector>(list_in[list_idx])) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("OFTReal requires a numeric value");
                }
                Rcpp::NumericVector v = list_in[list_idx];
                if (v.size() == 0 || Rcpp::NumericVector::is_na(v(0)))
                    OGR_F_SetFieldNull(hFeat, fld_idx);
                else
                    OGR_F_SetFieldDouble(hFeat, fld_idx, v[0]);
            }
            break;

            case OFTString:
            {
                if (!Rcpp::is<Rcpp::CharacterVector>(list_in[list_idx])) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("OFTString requires a character string");
                }
                Rcpp::CharacterVector v = list_in[list_idx];
                if (v.size() == 0 || Rcpp::CharacterVector::is_na(v(0)))
                    OGR_F_SetFieldNull(hFeat, fld_idx);
                else
                    OGR_F_SetFieldString(hFeat, fld_idx, v[0]);
            }
            break;

            case OFTDate:
            {
                if (!Rcpp::is<Rcpp::NumericVector>(list_in[list_idx])) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("OFTDate requires a numeric value (Date)");
                }
                Rcpp::NumericVector v = list_in[list_idx];
                if (v.size() == 0 || Rcpp::NumericVector::is_na(v[0])) {
                    OGR_F_SetFieldNull(hFeat, fld_idx);
                    continue;
                }
                Rcpp::CharacterVector attr{};
                if (v.hasAttribute("class"))
                    attr = Rcpp::wrap(v.attr("class"));
                if (std::find(attr.begin(), attr.end(), "Date") == attr.end()) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("value for OGR date must be of class 'Date'");
                }
                else {
                    int64_t nUnixTime = v[0] * 86400;
                    struct tm brokendowntime;
                    CPLUnixTimeToYMDHMS(nUnixTime, &brokendowntime);
                    OGR_F_SetFieldDateTime(hFeat, fld_idx,
                                           brokendowntime.tm_year + 1900,
                                           brokendowntime.tm_mon + 1,
                                           brokendowntime.tm_mday,
                                           0, 0, 0, 0);
                }
            }
            break;

            case OFTDateTime:
            {
                if (!Rcpp::is<Rcpp::NumericVector>(list_in[list_idx])) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("OFTDateTime requires a numeric value (POSIXct)");
                }
                Rcpp::NumericVector v = list_in[list_idx];
                if (v.size() == 0 || Rcpp::NumericVector::is_na(v[0])) {
                    OGR_F_SetFieldNull(hFeat, fld_idx);
                    continue;
                }
                Rcpp::CharacterVector attr{};
                if (v.hasAttribute("class"))
                    attr = Rcpp::wrap(v.attr("class"));
                if (std::find(attr.begin(), attr.end(), "POSIXct") ==
                        attr.end()) {

                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("value for OGR datetime must be of class 'POSIXct'");
                }
                else {
                    int64_t nUnixTime = static_cast<int64_t>(v[0]);
                    struct tm brokendowntime;
                    CPLUnixTimeToYMDHMS(nUnixTime, &brokendowntime);
                    float sec = brokendowntime.tm_sec +
                            std::fmod(static_cast<float>(v[0]), 1.0f);
                    OGR_F_SetFieldDateTimeEx(hFeat, fld_idx,
                                             brokendowntime.tm_year + 1900,
                                             brokendowntime.tm_mon + 1,
                                             brokendowntime.tm_mday,
                                             brokendowntime.tm_hour,
                                             brokendowntime.tm_min,
                                             sec, 100);
                }
            }
            break;

            case OFTTime:
            {
                // TODO(ctoney): OFTTime as character string from R?
                Rcpp::warning("writing OFTTime is currently not supported");
            }
            break;

            case OFTIntegerList:
            {
                Rcpp::IntegerVector v;
                bool type_mismatch = false;
                if (Rcpp::is<Rcpp::NumericVector>(list_in[list_idx]) ||
                    Rcpp::is<Rcpp::IntegerVector>(list_in[list_idx])||
                    Rcpp::is<Rcpp::LogicalVector>(list_in[list_idx])) {

                    v = list_in[list_idx];
                }
                else if (Rcpp::is<Rcpp::List>(list_in[list_idx])) {
                    Rcpp::List list_tmp = list_in[list_idx];
                    if (Rcpp::is<Rcpp::NumericVector>(list_in[list_idx]) ||
                        Rcpp::is<Rcpp::IntegerVector>(list_in[list_idx])||
                        Rcpp::is<Rcpp::LogicalVector>(list_in[list_idx])) {

                        v = list_tmp[0];
                    }
                    else
                        type_mismatch = true;
                }
                else {
                    type_mismatch = true;
                }
                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OFTIntegerList requires compatible data type");
                }

                if (v.size() == 0 || Rcpp::all(Rcpp::is_na(v)).is_true()) {
                    OGR_F_SetFieldNull(hFeat, fld_idx);
                }
                else {
                    std::vector<int> values = Rcpp::as<std::vector<int>>(v);
                    OGR_F_SetFieldIntegerList(hFeat, fld_idx, values.size(),
                                              values.data());
                }
            }
            break;

            case OFTInteger64List:
            {
                Rcpp::NumericVector v;
                bool type_mismatch = false;
                if (Rcpp::is<Rcpp::NumericVector>(list_in[list_idx])) {
                    v = list_in[list_idx];
                }
                else if (Rcpp::is<Rcpp::List>(list_in[list_idx])) {
                    Rcpp::List list_tmp = list_in[list_idx];
                    if (Rcpp::is<Rcpp::NumericVector>(list_tmp[0]))
                        v = list_tmp[0];
                    else
                        type_mismatch = true;
                }
                else {
                    type_mismatch = true;
                }
                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OFTInteger64List requires a numeric vector");
                }

                if (v.size() == 0) {
                    OGR_F_SetFieldNull(hFeat, fld_idx);
                }
                else if (Rcpp::isInteger64(v)) {
                    std::vector<int64_t> values = Rcpp::fromInteger64(v, false);
                    OGR_F_SetFieldInteger64List(hFeat, fld_idx, values.size(),
                            reinterpret_cast<const GInt64 *>(values.data()));
                }
                else {
                    std::vector<GIntBig> values(v.begin(), v.end());
                    OGR_F_SetFieldInteger64List(hFeat, fld_idx, values.size(),
                                                values.data());
                }
            }
            break;

            case OFTRealList:
            {
                Rcpp::NumericVector v;
                bool type_mismatch = false;
                if (Rcpp::is<Rcpp::NumericVector>(list_in[list_idx])) {
                    v = list_in[list_idx];
                }
                else if (Rcpp::is<Rcpp::List>(list_in[list_idx])) {
                    Rcpp::List list_tmp = list_in[list_idx];
                    if (Rcpp::is<Rcpp::NumericVector>(list_tmp[0]))
                        v = list_tmp[0];
                    else
                        type_mismatch = true;
                }
                else {
                    type_mismatch = true;
                }
                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OFTRealList requires a numeric vector");
                }

                if (v.size() == 0 || Rcpp::all(Rcpp::is_na(v)).is_true()) {
                    OGR_F_SetFieldNull(hFeat, fld_idx);
                }
                else {
                    std::vector<double> values =
                            Rcpp::as<std::vector<double>>(v);

                    OGR_F_SetFieldDoubleList(hFeat, fld_idx, values.size(),
                                             values.data());
                }
            }
            break;

            case OFTStringList:
            {
                Rcpp::CharacterVector v;
                bool type_mismatch = false;
                if (Rcpp::is<Rcpp::CharacterVector>(list_in[list_idx])) {
                    v = list_in[list_idx];
                }
                else if (Rcpp::is<Rcpp::List>(list_in[list_idx])) {
                    Rcpp::List list_tmp = list_in[list_idx];
                    if (Rcpp::is<Rcpp::CharacterVector>(list_tmp[0]))
                        v = list_tmp[0];
                    else
                        type_mismatch = true;
                }
                else {
                    type_mismatch = true;
                }
                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OFTStringList requires a character vector");
                }

                if (v.size() == 0 || Rcpp::all(Rcpp::is_na(v)).is_true()) {
                    OGR_F_SetFieldNull(hFeat, fld_idx);
                }
                else {
                    std::vector<const char *> values(v.begin(), v.end());
                    values.push_back(nullptr);
                    OGR_F_SetFieldStringList(hFeat, fld_idx, values.data());
                }
            }
            break;

            case OFTBinary:
            {
                Rcpp::RawVector v;
                bool type_mismatch = false;
                if (Rcpp::is<Rcpp::RawVector>(list_in[list_idx])) {
                    v = list_in[list_idx];
                }
                else if (Rcpp::is<Rcpp::List>(list_in[list_idx])) {
                    Rcpp::List list_tmp = list_in[list_idx];
                    if (Rcpp::is<Rcpp::RawVector>(list_tmp[0]))
                        v = list_tmp[0];
                    else
                        type_mismatch = true;
                }
                else {
                    type_mismatch = true;
                }
                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OFTBinary requires a raw vector");
                }

                if (v.size() == 0)
                    OGR_F_SetFieldNull(hFeat, fld_idx);
                else
                    OGR_F_SetFieldBinary(hFeat, fld_idx, v.size(), &v[0]);
            }
            break;

            default:
                Rcpp::Rcerr << "unhandled OGRFieldType: " << fld_type
                        << std::endl;
                break;
        }
    }

    // set geom fields
    for (auto it = map_geom_flds.begin(); it != map_geom_flds.end(); ++it) {
        R_xlen_t list_idx = it->first;
        int gfld_idx = it->second;

        OGRErr err = OGRERR_NONE;

        // geometry fields may originate in a data frame list column
        // set up a generic RObject with the correct reference
        Rcpp::RObject robj;
        bool have_geom = false;
        bool is_raw = false;

        if (Rcpp::is<Rcpp::RawVector>(list_in[list_idx])) {
            have_geom = true;
            is_raw = true;
            robj = list_in[list_idx];
        }
        else if (Rcpp::is<Rcpp::CharacterVector>(list_in[list_idx])) {
            have_geom = true;
            is_raw = false;
            robj = list_in[list_idx];
        }
        else if (Rcpp::is<Rcpp::List>(list_in[list_idx])) {
            Rcpp::List list_tmp = list_in[list_idx];
            robj = list_tmp[0];
            if (Rcpp::is<Rcpp::RawVector>(robj)) {
                have_geom = true;
                is_raw = true;
            }
            else if (Rcpp::is<Rcpp::CharacterVector>(robj)) {
                have_geom = true;
                is_raw = false;
            }
        }

        if (!have_geom) {
            OGR_F_Destroy(hFeat);
            Rcpp::stop("geometry must be 'raw' (WKB) or 'character' (WKT)");
        }

        if (is_raw) {
            Rcpp::RawVector v = Rcpp::as<Rcpp::RawVector>(robj);
            if (v.size() > 0) {
                OGRGeometryH hGeom = nullptr;
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
                err = OGR_G_CreateFromWkb(&v[0], nullptr, &hGeom,
                        static_cast<int>(v.size()));
#else
                err = OGR_G_CreateFromWkbEx(&v[0], nullptr, &hGeom, v.size());
#endif
                if (err == OGRERR_NONE) {
                    err = OGR_F_SetGeomFieldDirectly(hFeat, gfld_idx, hGeom);
                    if (err != OGRERR_NONE) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("failed to set geometry");
                    }
                }
                else if (err == OGRERR_NOT_ENOUGH_DATA) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OGRERR_NOT_ENOUGH_DATA, failed to create geom");
                }
                else if (err == OGRERR_NOT_ENOUGH_DATA) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OGRERR_UNSUPPORTED_GEOMETRY_TYPE");
                }
                else if (err == OGRERR_CORRUPT_DATA) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OGRERR_CORRUPT_DATA, failed to create geom");
                }
            }
        }
        else {
            // wkt
            Rcpp::CharacterVector v = Rcpp::as<Rcpp::CharacterVector>(robj);
            if (v.size() > 0 && !Rcpp::CharacterVector::is_na(v[0])) {
                OGRGeometryH hGeom = nullptr;
                std::string wkt = Rcpp::as<std::string>(v[0]);
                char *pszWKT = const_cast<char *>(wkt.c_str());
                err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
                if (err == OGRERR_NONE) {
                    err = OGR_F_SetGeomFieldDirectly(hFeat, gfld_idx, hGeom);
                    if (err != OGRERR_NONE) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("failed to set geometry");
                    }
                }
                else if (err == OGRERR_NOT_ENOUGH_DATA) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OGRERR_NOT_ENOUGH_DATA, failed to create geom");
                }
                else if (err == OGRERR_NOT_ENOUGH_DATA) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OGRERR_UNSUPPORTED_GEOMETRY_TYPE");
                }
                else if (err == OGRERR_CORRUPT_DATA) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OGRERR_CORRUPT_DATA, failed to create geom");
                }
            }
        }
    }

    return hFeat;
}

// ****************************************************************************

RCPP_MODULE(mod_GDALVector) {
    Rcpp::class_<GDALVector>("GDALVector")

    .constructor<Rcpp::CharacterVector>
        ("Usage: new(GDALVector, dsn)")
    .constructor<Rcpp::CharacterVector, std::string>
        ("Usage: new(GDALVector, dsn, layer)")
    .constructor<Rcpp::CharacterVector, std::string, bool>
        ("Usage: new(GDALVector, dsn, layer, read_only=[TRUE|FALSE])")
    .constructor<Rcpp::CharacterVector, std::string, bool,
                 Rcpp::CharacterVector>
        ("Usage: new(GDALVector, dsn, layer, read_only, open_options)")
    .constructor<Rcpp::CharacterVector, std::string, bool,
                 Rcpp::Nullable<Rcpp::CharacterVector>, std::string>
        ("Usage: new(GDALVector, dsn, layer, read_only, open_options, spatial_filter)")
    .constructor<Rcpp::CharacterVector, std::string, bool,
                 Rcpp::Nullable<Rcpp::CharacterVector>, std::string,
                 std::string>
        ("Usage: new(GDALVector, dsn, layer, read_only, open_options, spatial_filter, dialect)")

    // read-only fields
    .field_readonly("featureTemplate", &GDALVector::featureTemplate)

    // undocumented read-only fields for internal use
    .field_readonly("m_layer_name", &GDALVector::m_layer_name)
    .field_readonly("m_is_sql", &GDALVector::m_is_sql)
    .field_readonly("m_dialect", &GDALVector::m_dialect)

    // read/write fields
    .field("defaultGeomFldName", &GDALVector::defaultGeomFldName)
    .field("promoteToMulti", &GDALVector::promoteToMulti)
    .field("returnGeomAs", &GDALVector::returnGeomAs)
    .field("wkbByteOrder", &GDALVector::wkbByteOrder)

    // methods
    .const_method("getDsn", &GDALVector::getDsn,
        "Return the DSN")
    .const_method("isOpen", &GDALVector::isOpen,
        "Is the dataset open?")
    .method("open", &GDALVector::open,
        "(Re-)open the dataset on the existing DSN and layer")
    .const_method("getFileList", &GDALVector::getFileList,
        "Fetch files forming dataset")
    .const_method("getDriverShortName", &GDALVector::getDriverShortName,
         "Return the short name of the format driver")
    .const_method("getDriverLongName", &GDALVector::getDriverLongName,
        "Return the long name of the format driver")
    .const_method("getName", &GDALVector::getName,
        "Return the layer name")
    .const_method("testCapability", &GDALVector::testCapability,
        "Test if this layer supports the named capability")
    .const_method("getFIDColumn", &GDALVector::getFIDColumn,
        "Return name of the underlying db column being used as FID column")
    .const_method("getGeomType", &GDALVector::getGeomType,
        "Return the layer geometry type")
    .const_method("getGeometryColumn", &GDALVector::getGeometryColumn,
        "Return name of the underlying db column being used as geom column")
    .const_method("getSpatialRef", &GDALVector::getSpatialRef,
        "Fetch the spatial reference system for this layer as WKT string")
    .method("bbox", &GDALVector::bbox,
        "Return the bounding box (xmin, ymin, xmax, ymax)")
    .const_method("getLayerDefn", &GDALVector::getLayerDefn,
        "Fetch the schema information for this layer")
    .method("setAttributeFilter", &GDALVector::setAttributeFilter,
        "Set a new attribute query")
    .const_method("getAttributeFilter", &GDALVector::getAttributeFilter,
        "Get the attribute filter string (or empty string if not set")
    .method("setIgnoredFields", &GDALVector::setIgnoredFields,
        "Set which fields can be omitted when retrieving features")
    .method("setSpatialFilter", &GDALVector::setSpatialFilter,
        "Set a new spatial filter from a geometry in WKT format")
    .method("setSpatialFilterRect", &GDALVector::setSpatialFilterRect,
        "Set a new rectangular spatial filter")
    .const_method("getSpatialFilter", &GDALVector::getSpatialFilter,
        "Return the current spatial filter of this layer as WKT")
    .method("clearSpatialFilter", &GDALVector::clearSpatialFilter,
        "Clear the current spatial filter")
    .method("getFeatureCount", &GDALVector::getFeatureCount,
        "Fetch the feature count in this layer")
    .method("getNextFeature", &GDALVector::getNextFeature,
        "Fetch the next available feature from this layer")
    .method("setNextByIndex", &GDALVector::setNextByIndex,
        "Move read cursor to the i'th feature")
    .method("getFeature", &GDALVector::getFeature,
        "Fetch a feature by its identifier")
    .method("resetReading", &GDALVector::resetReading,
        "Reset feature reading to start on the first feature")
    .method("fetch", &GDALVector::fetch,
        "Fetch a set features as a data frame")
    .method("setFeature", &GDALVector::setFeature,
        "Rewrite/replace an existing feature within the layer")
    .method("createFeature", &GDALVector::createFeature,
        "Create and write a new feature within the layer")
    .method("upsertFeature", &GDALVector::upsertFeature,
        "Rewrite/replace an existing feature or create a new feature")
    .const_method("getLastWriteFID", &GDALVector::getLastWriteFID,
        "Return the FID of the last feature written, or NULL if no writes")
    .method("deleteFeature", &GDALVector::deleteFeature,
        "Delete feature from layer")
    .const_method("syncToDisk", &GDALVector::syncToDisk,
        "Flush pending changes to disk")
    .method("startTransaction", &GDALVector::startTransaction,
        "Create a transaction on the dataset")
    .method("commitTransaction", &GDALVector::commitTransaction,
        "Commit a transaction")
    .method("rollbackTransaction", &GDALVector::rollbackTransaction,
        "Roll back a transaction")
    .method("layerIntersection", &GDALVector::layerIntersection,
        "Intersection of this layer with a method layer")
    .method("layerUnion", &GDALVector::layerUnion,
        "Union of this layer with a method layer")
    .method("layerSymDifference", &GDALVector::layerSymDifference,
        "Symmetrical difference of this layer and a method layer")
    .method("layerIdentity", &GDALVector::layerIdentity,
        "Identify features of this layer with the ones from the method layer")
    .method("layerUpdate", &GDALVector::layerUpdate,
        "Update this layer with features from the method layer")
    .method("layerClip", &GDALVector::layerClip,
        "Clip off areas that are not covered by the method layer")
    .method("layerErase", &GDALVector::layerErase,
        "Remove areas that are covered by the method layer")
    .method("close", &GDALVector::close,
        "Release the dataset for proper cleanup")
    .const_method("OGRFeatureFromList_dumpReadble",
        &GDALVector::OGRFeatureFromList_dumpReadble,
        "Create an OGRFeature from list and dump to console in readable form")

    ;
}
