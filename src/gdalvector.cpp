/* Implementation of class GDALVector. Encapsulates an OGRLayer and its
   GDALDataset. Requires {bit64} on the R side for its integer64 S3 type.

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "cpl_time.h"
#include "ogr_geometry.h"
#include "ogr_srs_api.h"

#include "gdalraster.h"
#include "gdalvector.h"
#include "geom_api.h"
#include "ogr_util.h"

#include "nanoarrow/r.h"

// value for marking FID when used along with regular attribute field indexes
constexpr int FID_MARKER = -999;

GDALVector::GDALVector() : m_open_options(Rcpp::CharacterVector::create()),
                           m_ignored_fields(Rcpp::CharacterVector::create()) {
    // undocumented default constructor with no arguments
    // currently not intended for user code

#if __has_include("ogr_recordbatch.h")
    // initialize the release callback since it will be checked at closing
    m_stream.release = nullptr;
#endif
}

GDALVector::GDALVector(const Rcpp::CharacterVector &dsn) :

            GDALVector(dsn, "", true, Rcpp::CharacterVector::create(),
                       "", "") {}

GDALVector::GDALVector(const Rcpp::CharacterVector &dsn,
                       const std::string &layer) :

            GDALVector(dsn, layer, true, Rcpp::CharacterVector::create(),
                       "", "") {}

GDALVector::GDALVector(const Rcpp::CharacterVector &dsn,
                       const std::string &layer,
                       bool read_only) :

            GDALVector(dsn, layer, read_only, Rcpp::CharacterVector::create(),
                       "", "") {}

GDALVector::GDALVector(const Rcpp::CharacterVector &dsn,
                       const std::string &layer,
                       bool read_only,
                       const Rcpp::CharacterVector &open_options) :

            GDALVector(dsn, layer, read_only, open_options, "", "") {}

GDALVector::GDALVector(const Rcpp::CharacterVector &dsn,
                       const std::string &layer,
                       bool read_only,
                       const Rcpp::Nullable<Rcpp::CharacterVector>
                           &open_options,
                       const std::string &spatial_filter,
                       const std::string &dialect = "") :

            m_layer_name(layer),
            m_dialect(dialect),
            m_open_options(open_options.isNotNull() ?
                           open_options : Rcpp::CharacterVector::create()),
            m_spatial_filter(spatial_filter),
            m_ignored_fields(Rcpp::CharacterVector::create()),
            m_hDataset(nullptr),
            m_eAccess(GA_ReadOnly),
            m_hLayer(nullptr) {

    m_dsn = Rcpp::as<std::string>(check_gdal_filename(dsn));
    open(read_only);
    setFieldNames_();
}

GDALVector::~GDALVector() {
    close();
}

void GDALVector::open(bool read_only) {
    if (m_dsn == "")
        Rcpp::stop("DSN is not set");

    if (m_hDataset != nullptr)
        close();

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

    nOpenFlags |= GDAL_OF_VERBOSE_ERROR;

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
            Rcpp::warning("SpatiaLite is not available");
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

    if (m_layer_name == "") {
        // default layer first by index was opened
        m_layer_name = OGR_L_GetName(m_hLayer);
    }

#if __has_include("ogr_recordbatch.h")
    // initialize the release callback since it will be checked at closing
    m_stream.release = nullptr;
#endif

    // potentially enable this in the future for geoarrow
    // if (GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 8, 0)) {
    //     // override default to ensure CRS from GDAL is propagated to Arrow
    //     this->arrowStreamOptions = {"GEOMETRY_METADATA_ENCODING=GEOARROW"};
    // }

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

    const int items = CSLCount(papszFiles);
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

void GDALVector::info() const {
    checkAccess_(GA_ReadOnly);

    if (GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 7, 0)) {
        Rcpp::CharacterVector cl_arg = {"-so", "-nomd"};
        if (getSpatialFilter() != "") {
            cl_arg.push_back("-spat");
            Rcpp::NumericVector bb = bbox_from_wkt(getSpatialFilter(), 0, 0);
            cl_arg.push_back(std::to_string(bb[0]));
            cl_arg.push_back(std::to_string(bb[1]));
            cl_arg.push_back(std::to_string(bb[2]));
            cl_arg.push_back(std::to_string(bb[3]));
        }
        if (m_attr_filter != "") {
            cl_arg.push_back("-where");
            cl_arg.push_back(m_attr_filter);
        }
        if (m_dialect != "") {
            cl_arg.push_back("-dialect");
            cl_arg.push_back(m_dialect);
        }
        if (m_is_sql) {
            cl_arg.push_back("-sql");
            cl_arg.push_back(m_layer_name);
            Rcpp::String out = ogrinfo(m_dsn, R_NilValue, cl_arg,
                                       m_open_options, true, false);
            Rcpp::Rcout << out.get_cstring();
        }
        else {
            Rcpp::String out = ogrinfo(m_dsn, Rcpp::wrap(m_layer_name), cl_arg,
                                       m_open_options, true, false);
            Rcpp::Rcout << out.get_cstring();
        }
    }
    else {
        // fallback for GDAL < 3.7
        Rcpp::Rcout << "ogrinfo() requires GDAL >= 3.7" << std::endl;
        Rcpp::Rcout << " DSN:   " << m_dsn << std::endl;
        Rcpp::Rcout << " Layer: " << m_layer_name << std::endl;
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

Rcpp::CharacterVector GDALVector::getFieldNames() const {
    checkAccess_(GA_ReadOnly);

    return m_field_names;
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
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 6, 0)
        Rcpp::Named("FastGetArrowStream") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCFastGetArrowStream)),
#endif
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 8, 0)
        Rcpp::Named("FastWriteArrowBatch") = static_cast<bool>(
            OGR_L_TestCapability(m_hLayer, OLCFastWriteArrowBatch)),
#endif
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

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 3, 0)
        list_fld_defn.push_back(OGR_Fld_GetDomainName(hFieldDefn), "domain");
#endif

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
            if (!quiet)
                Rcpp::warning("could not obtain geometry field SRS");
            list_geom_fld_defn.push_back(NA_STRING, "srs");
        }
        else {
            char *pszSRS_WKT = nullptr;
            if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE) {
                 if (!quiet)
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

        bValue = true;
        list_geom_fld_defn.push_back(bValue, "is_geom");

        std::string geomFldName(OGR_GFld_GetNameRef(hGeomFldDefn));
        if (geomFldName == "")
            geomFldName = this->defaultGeomColName;
        list_out.push_back(list_geom_fld_defn, geomFldName);
    }

    return list_out;
}

SEXP GDALVector::getFieldDomain(const std::string &domain_name) const {
/*
 * The code for this method was adapted from ReportFieldDomain() in
 * gdal/apps/ogrinfo_lib.cpp:
 *
 * Copyright (c) 1999, Frank Warmerdam
 * Copyright (c) 2008-2013, Even Rouault <even dot rouault at spatialys.com>
 *
 * SPDX-License-Identifier: MIT
 */

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
    Rcpp::stop("getFieldDomain() requires GDAL >= 3.3.0");
#else
    checkAccess_(GA_ReadOnly);

    OGRFieldDomainH hDomain = nullptr;
    hDomain = GDALDatasetGetFieldDomain(m_hDataset, domain_name.c_str());
    if (hDomain == nullptr)
        return R_NilValue;

    Rcpp::List list_out = Rcpp::List::create();

    const char *pszType = "";
    switch (OGR_FldDomain_GetDomainType(hDomain)) {
        case OFDT_CODED:
            pszType = "Coded";
            break;
        case OFDT_RANGE:
            pszType = "Range";
            break;
        case OFDT_GLOB:
            pszType = "GLOB";
            break;
    }
    list_out.push_back(pszType, "type");

    list_out.push_back(OGR_FldDomain_GetName(hDomain), "domain_name");

    list_out.push_back(OGR_FldDomain_GetDescription(hDomain), "description");

    std::string fld_type_name = "OFT" + std::string(
        OGR_GetFieldTypeName(OGR_FldDomain_GetFieldType(hDomain)));
    list_out.push_back(fld_type_name, "field_type");

    std::string fld_subtype_name = "OFST" + std::string(
        OGR_GetFieldSubTypeName(OGR_FldDomain_GetFieldSubType(hDomain)));
    list_out.push_back(fld_subtype_name, "field_subtype");

    const char *pszSplitPolicy = "";
    switch (OGR_FldDomain_GetSplitPolicy(hDomain)) {
        case OFDSP_DEFAULT_VALUE:
            pszSplitPolicy = "DEFAULT_VALUE";
            break;
        case OFDSP_DUPLICATE:
            pszSplitPolicy = "DUPLICATE";
            break;
        case OFDSP_GEOMETRY_RATIO:
            pszSplitPolicy = "GEOMETRY_RATIO";
            break;
    }
    list_out.push_back(pszSplitPolicy, "split_policy");

    const char *pszMergePolicy = "";
    switch (OGR_FldDomain_GetMergePolicy(hDomain)) {
        case OFDMP_DEFAULT_VALUE:
            pszMergePolicy = "DEFAULT_VALUE";
            break;
        case OFDMP_SUM:
            pszMergePolicy = "SUM";
            break;
        case OFDMP_GEOMETRY_WEIGHTED:
            pszMergePolicy = "GEOMETRY_WEIGHTED";
            break;
    }
    list_out.push_back(pszMergePolicy, "merge_policy");

    switch (OGR_FldDomain_GetDomainType(hDomain)) {
        case OFDT_CODED:
        {
            const OGRCodedValue *enumeration =
                    OGR_CodedFldDomain_GetEnumeration(hDomain);

            if (enumeration == nullptr) {
                list_out.push_back(R_NilValue, "coded_values");
                break;
            }

            Rcpp::CharacterVector codes = Rcpp::CharacterVector::create();
            Rcpp::CharacterVector values = Rcpp::CharacterVector::create();

            for (int i = 0; enumeration[i].pszCode != nullptr; ++i) {
                codes.push_back(std::string(enumeration[i].pszCode));
                if (enumeration[i].pszValue) {
                    values.push_back(std::string(enumeration[i].pszValue));
                }
                else {
                    values.push_back(NA_STRING);
                }
            }

            Rcpp::DataFrame df = Rcpp::DataFrame::create();
            if (EQUAL(fld_type_name.c_str(), "OFTInteger") ||
                EQUAL(fld_type_name.c_str(), "OFTReal")) {

                Rcpp::NumericVector codes_num = Rcpp::no_init(codes.size());
                for (R_xlen_t i = 0; i < codes.size(); ++i) {
                    codes_num[i] = std::stod(Rcpp::as<std::string>(codes[i]));
                }
                df.push_back(codes_num, "codes");
            }
            else if (EQUAL(fld_type_name.c_str(), "OFTInteger64")) {
                std::vector<int64_t> codes_int64(codes.size());
                for (R_xlen_t i = 0; i < codes.size(); ++i) {
                    codes_int64[i] = static_cast<int64_t>(std::stoll(
                                        Rcpp::as<std::string>(codes[i])));
                }
                df.push_back(Rcpp::wrap(codes_int64), "codes");
            }
            else {
                df.push_back(codes, "codes");
            }

            df.push_back(values, "values");

            list_out.push_back(df, "coded_values");
            break;
        }

        case OFDT_RANGE:
        {
            bool bMinIsIncluded = false;
            const OGRField *sMin =
                    OGR_RangeFldDomain_GetMin(hDomain, &bMinIsIncluded);

            bool bMaxIsIncluded = false;
            const OGRField *sMax =
                    OGR_RangeFldDomain_GetMax(hDomain, &bMaxIsIncluded);

            if (OGR_FldDomain_GetFieldType(hDomain) == OFTInteger) {
                if (!OGR_RawField_IsUnset(sMin)) {
                    list_out.push_back(sMin->Integer, "min_value");
                    list_out.push_back(bMinIsIncluded, "min_is_inclusive");
                }
                else {
                    list_out.push_back(R_NilValue, "min_value");
                    list_out.push_back(R_NilValue, "min_is_inclusive");
                }
                if (!OGR_RawField_IsUnset(sMax)) {
                    list_out.push_back(sMax->Integer, "max_value");
                    list_out.push_back(bMaxIsIncluded, "max_is_inclusive");
                }
                else {
                    list_out.push_back(R_NilValue, "max_value");
                    list_out.push_back(R_NilValue, "max_is_inclusive");
                }
            }
            else if (OGR_FldDomain_GetFieldType(hDomain) == OFTInteger64) {
                if (!OGR_RawField_IsUnset(sMin)) {
                    list_out.push_back(Rcpp::toInteger64(sMin->Integer64),
                                       "min_value");
                    list_out.push_back(bMinIsIncluded, "min_is_inclusive");
                }
                else {
                    list_out.push_back(R_NilValue, "min_value");
                    list_out.push_back(R_NilValue, "min_is_inclusive");
                }
                if (!OGR_RawField_IsUnset(sMax)) {
                    list_out.push_back(Rcpp::toInteger64(sMax->Integer64),
                                       "max_value");
                    list_out.push_back(bMaxIsIncluded, "max_is_inclusive");
                }
                else {
                    list_out.push_back(R_NilValue, "max_value");
                    list_out.push_back(R_NilValue, "max_is_inclusive");
                }
            }
            else if (OGR_FldDomain_GetFieldType(hDomain) == OFTReal) {
                if (!OGR_RawField_IsUnset(sMin)) {
                    list_out.push_back(sMin->Real, "min_value");
                    list_out.push_back(bMinIsIncluded, "min_is_inclusive");
                }
                else {
                    list_out.push_back(R_NilValue, "min_value");
                    list_out.push_back(R_NilValue, "min_is_inclusive");
                }
                if (!OGR_RawField_IsUnset(sMax)) {
                    list_out.push_back(sMax->Real, "max_value");
                    list_out.push_back(bMaxIsIncluded, "max_is_inclusive");
                }
                else {
                    list_out.push_back(R_NilValue, "max_value");
                    list_out.push_back(R_NilValue, "max_is_inclusive");
                }
            }
            else if (OGR_FldDomain_GetFieldType(hDomain) == OFTDateTime) {
                list_out[0] = "RangeDateTime";

                if (!OGR_RawField_IsUnset(sMin)) {
                    struct tm brokendowntime;
                    brokendowntime.tm_year =
                            static_cast<int>(sMin->Date.Year) - 1900;
                    brokendowntime.tm_mon =
                            static_cast<int>(sMin->Date.Month) - 1;
                    brokendowntime.tm_mday =
                            static_cast<int>(sMin->Date.Day);
                    brokendowntime.tm_hour =
                            static_cast<int>(sMin->Date.Hour);
                    brokendowntime.tm_min =
                            static_cast<int>(sMin->Date.Minute);
                    brokendowntime.tm_sec =
                            static_cast<int>(sMin->Date.Second + 0.5);
                    int64_t nUnixTime = CPLYMDHMSToUnixTime(&brokendowntime);
                    int nTZflag = static_cast<int>(sMin->Date.TZFlag);
                    if (nTZflag > 1 && nTZflag != 100) {
                        // convert to UTC
                        const int tzoffset = std::abs(nTZflag - 100) * 15;
                        const int tzhour = tzoffset / 60;
                        const int tzmin = tzoffset - tzhour * 60;
                        const int offset_sec = tzhour * 3600 + tzmin * 60;
                        if (nTZflag >= 100)
                            nUnixTime -= offset_sec;
                        else
                            nUnixTime += offset_sec;
                    }
                    Rcpp::NumericVector dt_min =
                            {static_cast<double>(nUnixTime)};
                    Rcpp::CharacterVector classes = {"POSIXct", "POSIXt"};
                    dt_min.attr("class") = classes;
                    dt_min.attr("tzone") = "UTC";
                    list_out.push_back(dt_min, "min_value");
                    list_out.push_back(bMinIsIncluded, "min_is_inclusive");
                }
                else {
                    list_out.push_back(R_NilValue, "min_value");
                    list_out.push_back(R_NilValue, "min_is_inclusive");
                }
                if (!OGR_RawField_IsUnset(sMax)) {
                    struct tm brokendowntime;
                    brokendowntime.tm_year =
                            static_cast<int>(sMax->Date.Year) - 1900;
                    brokendowntime.tm_mon =
                            static_cast<int>(sMax->Date.Month) - 1;
                    brokendowntime.tm_mday =
                            static_cast<int>(sMax->Date.Day);
                    brokendowntime.tm_hour =
                            static_cast<int>(sMax->Date.Hour);
                    brokendowntime.tm_min =
                            static_cast<int>(sMax->Date.Minute);
                    brokendowntime.tm_sec =
                            static_cast<int>(sMax->Date.Second + 0.5);
                    int64_t nUnixTime = CPLYMDHMSToUnixTime(&brokendowntime);
                    int nTZflag = static_cast<int>(sMin->Date.TZFlag);
                    if (nTZflag > 1 && nTZflag != 100) {
                        // convert to UTC
                        const int tzoffset = std::abs(nTZflag - 100) * 15;
                        const int tzhour = tzoffset / 60;
                        const int tzmin = tzoffset - tzhour * 60;
                        const int offset_sec = tzhour * 3600 + tzmin * 60;
                        if (nTZflag >= 100)
                            nUnixTime -= offset_sec;
                        else
                            nUnixTime += offset_sec;
                    }
                    Rcpp::NumericVector dt_max =
                            {static_cast<double>(nUnixTime)};
                    Rcpp::CharacterVector classes = {"POSIXct", "POSIXt"};
                    dt_max.attr("class") = classes;
                    dt_max.attr("tzone") = "UTC";
                    list_out.push_back(dt_max, "max_value");
                    list_out.push_back(bMaxIsIncluded, "max_is_inclusive");
                }
                else {
                    list_out.push_back(R_NilValue, "max_value");
                    list_out.push_back(R_NilValue, "max_is_inclusive");
                }
            }
            break;
        }

        case OFDT_GLOB:
        {
            const char *pszGlob = nullptr;
            pszGlob = OGR_GlobFldDomain_GetGlob(hDomain);
            if (pszGlob)
                list_out.push_back(pszGlob, "glob");
            else
                list_out.push_back(R_NilValue, "glob");
            break;
        }
    }

    return list_out;
#endif
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

    if (!OGR_L_TestCapability(m_hLayer, OLCIgnoreFields)) {
         if (!quiet) {
            Rcpp::Rcout << "this layer does not have IgnoreFields capability"
                    << std::endl;
         }
        return;
    }

    if (fields.isNULL() || !Rcpp::is<Rcpp::CharacterVector>(fields))
        Rcpp::stop("'fields' must be a character vector");

    Rcpp::CharacterVector fields_in(fields);
    std::vector<const char *> oFields(fields_in.begin(), fields_in.end());
    oFields.push_back(nullptr);

    if (oFields[0] == nullptr || EQUAL(oFields[0], "")) {
        // reset to none
        OGR_L_SetIgnoredFields(m_hLayer, nullptr);
        m_ignored_fields = Rcpp::CharacterVector::create();
        return;
    }
    else {
        if (OGR_L_SetIgnoredFields(m_hLayer, oFields.data()) != OGRERR_NONE) {
             Rcpp::stop("not all field names could be resolved");
        }
        else {
            m_ignored_fields = Rcpp::clone(fields_in);
        }
        return;
    }
}

void GDALVector::setSelectedFields(const Rcpp::RObject &fields) {
    checkAccess_(GA_ReadOnly);

    if (!OGR_L_TestCapability(m_hLayer, OLCIgnoreFields)) {
        Rcpp::Rcout << "capability to ignore fields is needed to set selected"
                << std::endl;
        Rcpp::Rcout << "this layer does not have IgnoreFields capability"
                << std::endl;
        return;
    }

    if (fields.isNULL() || !Rcpp::is<Rcpp::CharacterVector>(fields))
        Rcpp::stop("'fields' must be a character vector");

    Rcpp::CharacterVector fields_in(fields);
    if (EQUAL(fields_in[0], "")) {
        // reset to none
        OGR_L_SetIgnoredFields(m_hLayer, nullptr);
        m_ignored_fields = Rcpp::CharacterVector::create();
        return;
    }

    // if special field "OGR_GEOMETRY" is used here, we need to replace with
    // the geometry column name
    if (getGeometryColumn() != "") {
        for (auto& x : fields_in) {
            if (EQUAL(x, "OGR_GEOMETRY")) {
                x = getGeometryColumn();
            }
        }
    }

    Rcpp::CharacterVector unmatched_fields_in = Rcpp::setdiff(fields_in,
                                                              m_field_names);

    if (unmatched_fields_in.size() == fields_in.size()) {
        Rcpp::stop("none of the input field names could be resolved");
    }
    else if (unmatched_fields_in.size() > 0) {
        Rcpp::Rcout << "some input field names could not be resolved:"
                << std::endl;
        Rcpp::Rcout << unmatched_fields_in << std::endl;
    }

    Rcpp::CharacterVector ignore_fields = Rcpp::setdiff(m_field_names,
                                                        fields_in);

    std::vector<const char *> oFields(ignore_fields.begin(),
                                      ignore_fields.end());
    oFields.push_back(nullptr);

    // reset first
    OGR_L_SetIgnoredFields(m_hLayer, nullptr);
    OGRErr err = OGR_L_SetIgnoredFields(m_hLayer, oFields.data());
    if (err != OGRERR_NONE) {
        Rcpp::Rcout << "not all field names could be resolved"
                << std::endl;
    }
    else {
        m_ignored_fields = Rcpp::clone(ignore_fields);
    }
}

Rcpp::CharacterVector GDALVector::getIgnoredFields() const {
    checkAccess_(GA_ReadOnly);

    if (!OGR_L_TestCapability(m_hLayer, OLCIgnoreFields)) {
         if (!quiet) {
            Rcpp::Rcout << "this layer does not have IgnoreFields capability"
                    << std::endl;
         }
        return Rcpp::CharacterVector::create();
    }

    return Rcpp::clone(m_ignored_fields);
}

void GDALVector::setSpatialFilter(const std::string &wkt) {
    checkAccess_(GA_ReadOnly);

    OGRGeometryH hFilterGeom = nullptr;
    OGRSpatialReferenceH hSRS = OGR_L_GetSpatialRef(m_hLayer);

    if (wkt != "") {
        char *pszWKT = (char *) wkt.c_str();
        if (OGR_G_CreateFromWkt(&pszWKT, hSRS, &hFilterGeom) !=
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
        Rcpp::stop("'bbox' must be a `numeric` vector");

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
        // return as list potentially with S3 class attribute "OGRFeature"
        df.attr("class") = R_NilValue;
        df.attr("row.names") = R_NilValue;
        if (df.hasAttribute("gis"))
            df.attr("class") = Rcpp::CharacterVector{"OGRFeature", "list"};

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
        Rcpp::stop("'fid' must be a length-1 `numeric` vector (integer64)");

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
        // return as list potentially with S3 class attribute "OGRFeature"
        df.attr("class") = R_NilValue;
        df.attr("row.names") = R_NilValue;
        if (df.hasAttribute("gis"))
            df.attr("class") = Rcpp::CharacterVector{"OGRFeature", "list"};

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
    // this method must be kept consistent with createDF_()
    checkAccess_(GA_ReadOnly);

    const OGRFeatureDefnH hFDefn = OGR_L_GetLayerDefn(m_hLayer);
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

    const int nFields = OGR_FD_GetFieldCount(hFDefn);
    const int nGeomFields = OGR_FD_GetGeomFieldCount(hFDefn);
    bool include_geom = true;
    Rcpp::CharacterVector geom_column{};  // column name(s) for gis attributes
    Rcpp::CharacterVector geom_col_type{};  // geom type(s) for gis attributes
    Rcpp::CharacterVector geom_col_srs{};  // SRS for gis attributes
    std::string geom_format{};   // taken from this->returnGeomAs;

    if (EQUAL(this->returnGeomAs.c_str(), "NONE")) {
        include_geom = false;
    }
    else if (!(EQUAL(this->returnGeomAs.c_str(), "WKB") ||
               EQUAL(this->returnGeomAs.c_str(), "WKB_ISO") ||
               EQUAL(this->returnGeomAs.c_str(), "WKT") ||
               EQUAL(this->returnGeomAs.c_str(), "WKT_ISO") ||
               EQUAL(this->returnGeomAs.c_str(), "SUMMARY") ||
               EQUAL(this->returnGeomAs.c_str(), "TYPE_NAME") ||
               EQUAL(this->returnGeomAs.c_str(), "BBOX"))) {
        Rcpp::stop("unsupported value of object field 'returnGeomAs'");
    }

    OGRwkbByteOrder eOrder;
    if (EQUAL(this->wkbByteOrder.c_str(), "LSB"))
        eOrder = wkbNDR;
    else if (EQUAL(this->wkbByteOrder.c_str(), "MSB"))
        eOrder = wkbXDR;
    else
        Rcpp::stop("invalid value of object field 'wkbByteOrder'");

    bool reset_ignored_fields = false;
    Rcpp::CharacterVector orig_ignored_fields;
    if (EQUAL(this->returnGeomAs.c_str(), "NONE") &&
        OGR_L_TestCapability(m_hLayer, OLCIgnoreFields)) {

        orig_ignored_fields = Rcpp::clone(m_ignored_fields);
        m_ignored_fields.push_back("OGR_GEOMETRY");
        setIgnoredFields(m_ignored_fields);
        reset_ignored_fields = true;
    }

    Rcpp::DataFrame df = createDF_(fetch_num);

    if (include_geom && nGeomFields > 0) {
        // get gis attributes
        geom_format = this->returnGeomAs;

        for (int i = 0; i < nGeomFields; ++i) {
            OGRGeomFieldDefnH hGeomFldDefn = OGR_FD_GetGeomFieldDefn(hFDefn, i);
            if (hGeomFldDefn == nullptr)
                Rcpp::stop("could not obtain geometry field definition");

            if (OGR_GFld_IsIgnored(hGeomFldDefn))
                continue;

            if (EQUAL(OGR_GFld_GetNameRef(hGeomFldDefn), ""))
                geom_column.push_back(this->defaultGeomColName);
            else
                geom_column.push_back(OGR_GFld_GetNameRef(hGeomFldDefn));

            OGRwkbGeometryType eType = OGR_GFld_GetType(hGeomFldDefn);
            if (this->promoteToMulti || this->convertToLinear) {
                eType = getTargetGeomType(eType, this->convertToLinear,
                                          this->promoteToMulti);
            }
            geom_col_type.push_back(getWkbGeomString_(eType));

            OGRSpatialReferenceH hSRS =
                    OGR_GFld_GetSpatialRef(hGeomFldDefn);

            if (hSRS != nullptr) {
                char *pszSRS_WKT = nullptr;
                if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE) {
                    if (!quiet)
                        Rcpp::warning("error exporting geometry SRS to WKT");
                }
                else {
                    geom_col_srs.push_back(pszSRS_WKT);
                }
                CPLFree(pszSRS_WKT);
            }
            else {
                 geom_col_srs.push_back("");
            }
        }
    }
    else {
        geom_format = "NONE";
    }

    attachGISattributes_(&df, geom_column, geom_col_type, geom_col_srs,
                         geom_format);

    if (fetch_num == 0) {
        if (reset_ignored_fields)
            setIgnoredFields(orig_ignored_fields);

        return df;
    }

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

            const OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);

            switch (fld_type) {
                case OFTInteger:
                {
                    OGRFieldSubType fld_subtype =
                            OGR_Fld_GetSubType(hFieldDefn);

                    if (fld_subtype == OFSTBoolean) {
                        Rcpp::LogicalVector col = df[col_num];
                        if (!has_value)
                            col[row_num] = NA_LOGICAL;
                        else
                            col[row_num] = OGR_F_GetFieldAsInteger(hFeat, i);
                    }
                    else {
                        Rcpp::IntegerVector col = df[col_num];
                        if (!has_value)
                            col[row_num] = NA_INTEGER;
                        else
                            col[row_num] = OGR_F_GetFieldAsInteger(hFeat, i);
                    }
                }
                break;

                case OFTInteger64:
                {
                    Rcpp::NumericVector col = df[col_num];
                    if (!has_value) {
                        col[row_num] = Rcpp::toInteger64(NA_INTEGER64)[0];
                    }
                    else {
                        const int64_t value = static_cast<int64_t>(
                                OGR_F_GetFieldAsInteger64(hFeat, i));

                        col[row_num] = Rcpp::toInteger64(value)[0];
                    }
                }
                break;

                case OFTReal:
                {
                    Rcpp::NumericVector col = df[col_num];
                    if (!has_value)
                        col[row_num] = NA_REAL;
                    else
                        col[row_num] = OGR_F_GetFieldAsDouble(hFeat, i);
                }
                break;

                case OFTDate:
                case OFTDateTime:
                {
                    Rcpp::NumericVector col = df[col_num];
                    if (!has_value) {
                        col[row_num] = NA_REAL;
                        continue;
                    }

                    int nYr = 0, nMo = 0, nDay = 0, nHr = 0, nMin = 0,
                        nTZflag = 0;

                    float fSec = 0;

                    if (!OGR_F_GetFieldAsDateTimeEx(hFeat, i, &nYr, &nMo,
                                                    &nDay, &nHr, &nMin, &fSec,
                                                    &nTZflag)) {

                        col[row_num] = NA_REAL;
                        continue;
                    }

                    struct tm brokendowntime;
                    brokendowntime.tm_year = nYr - 1900;
                    brokendowntime.tm_mon = nMo - 1;
                    brokendowntime.tm_mday = nDay;
                    brokendowntime.tm_hour = nHr;
                    brokendowntime.tm_min = nMin;
                    brokendowntime.tm_sec = static_cast<int>(fSec);
                    int64_t nUnixTime = CPLYMDHMSToUnixTime(&brokendowntime);

                    if (fld_type == OFTDate) {
                        col[row_num] = static_cast<double>(nUnixTime / 86400);
                    }
                    else {
                        // OFTDateTime
                        if (nTZflag > 1 && nTZflag != 100) {
                            // convert to UTC
                            const int tzoffset = std::abs(nTZflag - 100) * 15;
                            const int tzhour = tzoffset / 60;
                            const int tzmin = tzoffset - tzhour * 60;
                            const int offset_sec = tzhour * 3600 + tzmin * 60;
                            if (nTZflag >= 100)
                                nUnixTime -= offset_sec;
                            else
                                nUnixTime += offset_sec;
                        }
                        col[row_num] = static_cast<double>(
                                nUnixTime + std::fmod(fSec, 1));
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
                    Rcpp::CharacterVector col = df[col_num];
                    if (has_value)
                        col[row_num] = OGR_F_GetFieldAsString(hFeat, i);
                    else
                        col[row_num] = NA_STRING;
                }
                break;
            }
        }

        if (include_geom) {
            for (int i = 0; i < nGeomFields; ++i) {
                OGRGeomFieldDefnH hGeomFldDefn =
                        OGR_FD_GetGeomFieldDefn(hFDefn, i);

                if (hGeomFldDefn == nullptr)
                    Rcpp::stop("could not obtain geometry field definition");

                if (OGR_GFld_IsIgnored(hGeomFldDefn))
                    continue;

                OGRGeometryH hGeom = nullptr;
                bool destroy_geom = false;

                if (this->promoteToMulti || this->convertToLinear) {

                    OGRGeometryH hGeomRef = OGR_F_GetGeomFieldRef(hFeat, i);
                    OGRwkbGeometryType out_geom_type = getTargetGeomType(
                            OGR_G_GetGeometryType(hGeomRef),
                            this->convertToLinear,
                            this->promoteToMulti);

                    hGeom = OGR_G_ForceTo(OGR_G_Clone(hGeomRef),
                                          out_geom_type, nullptr);

                    destroy_geom = true;
                    }
                else {
                    hGeom = OGR_F_GetGeomFieldRef(hFeat, i);
                }

                col_num += 1;

                if (STARTS_WITH_CI(this->returnGeomAs.c_str(), "WKB")) {
                    Rcpp::List col = df[col_num];

                    if (hGeom == nullptr) {
                        col[row_num] = R_NilValue;
                        continue;
                    }

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
                    const int nWKBSize = OGR_G_WkbSizeEx(hGeom);
#else
                    const int nWKBSize = OGR_G_WkbSize(hGeom);
#endif
                    if (nWKBSize) {
                        Rcpp::RawVector wkb(nWKBSize);
                        if (EQUAL(this->returnGeomAs.c_str(), "WKB_ISO"))
                            OGR_G_ExportToIsoWkb(hGeom, eOrder, &wkb[0]);
                        else
                            OGR_G_ExportToWkb(hGeom, eOrder, &wkb[0]);
                        col[row_num] = wkb;
                    }
                    else {
                        col[row_num] = Rcpp::RawVector::create();
                    }
                }

                else if (STARTS_WITH_CI(this->returnGeomAs.c_str(), "WKT")) {
                    Rcpp::CharacterVector col = df[col_num];
                    if (hGeom == nullptr) {
                        col[row_num] = NA_STRING;
                    }
                    else {
                        char *pszWKT = nullptr;
                        if (EQUAL(this->returnGeomAs.c_str(), "WKT_ISO"))
                            OGR_G_ExportToIsoWkt(hGeom, &pszWKT);
                        else
                            OGR_G_ExportToWkt(hGeom, &pszWKT);
                        col[row_num] = pszWKT;
                        CPLFree(pszWKT);
                    }
                }

                else if (EQUAL(this->returnGeomAs.c_str(), "SUMMARY")) {
                    Rcpp::CharacterVector col = df[col_num];
                    if (hGeom == nullptr) {
                        col[row_num] = NA_STRING;
                    }
                    else {
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 7, 0)
                        // fall back to geom type name
                        col[row_num] = OGR_G_GetGeometryName(hGeom);
#else
                        const auto poGeom = OGRGeometry::FromHandle(hGeom);
                        std::vector<const char *> options =
                                {"DISPLAY_GEOMETRY=SUMMARY", nullptr};

                        CPLString s = poGeom->dumpReadable(nullptr,
                                                           options.data());

                        s.replaceAll('\n', ' ');
                        col[row_num] = s.Trim();

                        if (destroy_geom) {
                            delete poGeom;
                            destroy_geom = false;
                        }
#endif
                    }
                }

                else if (EQUAL(this->returnGeomAs.c_str(), "TYPE_NAME")) {
                    Rcpp::CharacterVector col = df[col_num];
                    if (hGeom != nullptr)
                        col[row_num] = OGR_G_GetGeometryName(hGeom);
                    else
                        col[row_num] = NA_STRING;
                }

                else if (EQUAL(this->returnGeomAs.c_str(), "BBOX")) {
                  Rcpp::List col = df[col_num];
                  if (hGeom != nullptr) {
                    OGREnvelope envelope;
                    OGR_G_GetEnvelope(hGeom, &envelope);
                    col[row_num] = Rcpp::NumericVector::create(envelope.MinX,
                                                               envelope.MinY,
                                                               envelope.MaxX,
                                                               envelope.MaxY);
                    if (destroy_geom) {
                      OGR_G_DestroyGeometry(hGeom);
                      destroy_geom = false;
                    }
                  } else {
                    // do.call(rbind, x$geom) to get a table of bbox,
                    // so we use NA not NULL
                    col[row_num] = Rcpp::NumericVector::create(NA_REAL,
                                                               NA_REAL,
                                                               NA_REAL,
                                                               NA_REAL);
                  }

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
            Rcpp::Rcout << "`getFeatureCount()` reported: " << row_num
                    << std::endl;
            std::string msg =
                    "more features potentially available than reported by `getFeatureCount()`";
            if (!quiet)
                Rcpp::warning(msg);
            OGR_F_Destroy(hFeat);
            hFeat = nullptr;
        }
    }

    if (reset_ignored_fields)
        setIgnoredFields(orig_ignored_fields);

    if (row_num == fetch_num) {
        return df;
    }
    else {
        // Truncate the data frame by copying to a new one. Hard to avoid
        // a copy here since Rcpp vectors cannot be resized. This is only
        // needed for the last page when paging through features with repeated
        // calls to fetch(n), so the data generally should not be large enough
        // for this to be a problem.
        Rcpp::DataFrame df_trunc = createDF_(row_num);
        attachGISattributes_(&df_trunc, geom_column, geom_col_type, geom_col_srs,
                             geom_format);

        if (row_num == 0)
            return df_trunc;

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
                        OGR_FD_GetGeomFieldDefn(hFDefn, i);

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

SEXP GDALVector::getArrowStream() {
    /*
    Exposes an Arrow C stream to be consumed by {nanoarrow}
    Implementation adapted from GDALStreamWrapper by Dewey Dunnington in:
    https://github.com/r-spatial/sf/blob/main/src/gdal_read_stream.cpp
    */

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 6, 0)
    Rcpp::stop("getArrowStream() requires GDAL >= 3.6");

#else
    checkAccess_(GA_ReadOnly);

    std::vector<char *> opt{};
    if (this->arrowStreamOptions.size() > 0) {
        for (R_xlen_t i = 0; i < this->arrowStreamOptions.size(); ++i) {
            if (!EQUAL(this->arrowStreamOptions[i], ""))
                opt.push_back((char *) (this->arrowStreamOptions[i]));
        }
    }
    opt.push_back(nullptr);

    if (!OGR_L_GetArrowStream(m_hLayer, &m_stream, opt.data())) {
        Rcpp::stop("OGR_L_GetArrowStream() failed: " +
            std::string(CPLGetLastErrorMsg()));
    }

    m_stream_xptrs.push_back(nanoarrow_array_stream_owning_xptr());
    size_t i = m_stream_xptrs.size() - 1;

    auto stream_out = reinterpret_cast<struct ArrowArrayStream*>(
        R_ExternalPtrAddr(m_stream_xptrs[i]));

    stream_out->get_schema = &arrow_get_schema_wrap;
    stream_out->get_next = &arrow_get_next_wrap;
    stream_out->get_last_error = &arrow_get_last_error_wrap;
    stream_out->release = &arrow_release_wrap;
    stream_out->private_data = this;

    return m_stream_xptrs[i];
#endif
}

void GDALVector::releaseArrowStream() {
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 6, 0)

    if (m_stream.release) {
        m_stream.release(&m_stream);
        m_stream.release = nullptr;

        if (m_stream_xptrs.empty()) {
            // should not be possible here
            return;
        }

        size_t i = m_stream_xptrs.size() - 1;
        if (R_ExternalPtrAddr(m_stream_xptrs[i])) {
            auto stream_out = reinterpret_cast<struct ArrowArrayStream*>(
                R_ExternalPtrAddr(m_stream_xptrs[i]));

            stream_out->release = nullptr;
        }
    }

#endif
}

bool GDALVector::setFeature(const Rcpp::List &feature) {
    checkAccess_(GA_Update);

    if (Rcpp::is<Rcpp::DataFrame>(feature)) {
        Rcpp::DataFrame df = feature;
        if (df.nrows() > 1)
            Rcpp::stop("this method does not support multi-row input");
    }

    std::vector<std::map<R_xlen_t, int>> fld_maps =
            validateFeatInput_(feature);

    if (fld_maps.size() != 2)
        Rcpp::stop("failed to obtain field index mappings");

    OGRFeatureH hFeat = OGRFeatureFromList_(feature, 0, fld_maps[0],
                                            fld_maps[1]);

    if (!hFeat)
        return false;

    OGRErr err = OGR_L_SetFeature(m_hLayer, hFeat);
     if (err != OGRERR_NONE) {
        Rcpp::Rcout << CPLGetLastErrorMsg() << std::endl;
        OGR_F_Destroy(hFeat);
        return false;
     }
     else {
        m_last_write_fid = OGR_F_GetFID(hFeat);
        OGR_F_Destroy(hFeat);
        return true;
     }
}

bool GDALVector::createFeature(const Rcpp::List &feature) {
    checkAccess_(GA_Update);

    if (Rcpp::is<Rcpp::DataFrame>(feature)) {
        Rcpp::DataFrame df = feature;
        if (df.nrows() > 1)
            Rcpp::stop("this method does not support multi-row input");
    }

    std::vector<std::map<R_xlen_t, int>> fld_maps =
            validateFeatInput_(feature);

    if (fld_maps.size() != 2)
        Rcpp::stop("failed to obtain field index mappings");

    OGRFeatureH hFeat = OGRFeatureFromList_(feature, 0, fld_maps[0],
                                            fld_maps[1]);

    if (!hFeat)
        return false;

    OGRErr err = OGR_L_CreateFeature(m_hLayer, hFeat);
     if (err != OGRERR_NONE) {
        Rcpp::Rcout << CPLGetLastErrorMsg() << std::endl;
        OGR_F_Destroy(hFeat);
        return false;
     }
     else {
        m_last_write_fid = OGR_F_GetFID(hFeat);
        OGR_F_Destroy(hFeat);
        return true;
     }
}

Rcpp::LogicalVector GDALVector::batchCreateFeature(
                                const Rcpp::DataFrame &feature_set) {

    checkAccess_(GA_Update);

    std::vector<std::map<R_xlen_t, int>> fld_maps =
            validateFeatInput_(feature_set);

    if (fld_maps.size() != 2)
        Rcpp::stop("failed to obtain field index mappings");

    const R_xlen_t num_rows = feature_set.nrows();
    Rcpp::LogicalVector out = Rcpp::no_init(num_rows);

    GDALProgressFunc pfnProgress = nullptr;
    if (!quiet && num_rows > 1)
        pfnProgress = GDALTermProgressR;

    for (R_xlen_t i = 0; i < num_rows; ++i) {
        OGRFeatureH hFeat = OGRFeatureFromList_(feature_set, i, fld_maps[0],
                                                fld_maps[1]);

        if (!hFeat) {
            out[i] = FALSE;
            continue;
        }

        OGRErr err = OGR_L_CreateFeature(m_hLayer, hFeat);
        if (err != OGRERR_NONE) {
            Rcpp::Rcout << CPLGetLastErrorMsg() << std::endl;
            OGR_F_Destroy(hFeat);
            out[i] = FALSE;
            continue;
        }
        else {
            out[i] = TRUE;
            m_last_write_fid = OGR_F_GetFID(hFeat);
            OGR_F_Destroy(hFeat);
        }
        if (!quiet && num_rows > 1)
            pfnProgress(i / (num_rows - 1.0), nullptr, nullptr);
    }

    return out;
}

bool GDALVector::upsertFeature(const Rcpp::List &feature) {
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 6, 0)
    Rcpp::stop("'upsertFeature() requires GDAL >= 3.6");
#else
    checkAccess_(GA_Update);

    if (Rcpp::is<Rcpp::DataFrame>(feature)) {
        Rcpp::DataFrame df = feature;
        if (df.nrows() > 1)
            Rcpp::stop("this method does not support multi-row input");
    }

    std::vector<std::map<R_xlen_t, int>> fld_maps =
            validateFeatInput_(feature);

    if (fld_maps.size() != 2)
        Rcpp::stop("failed to obtain field index mappings");

    OGRFeatureH hFeat = OGRFeatureFromList_(feature, 0, fld_maps[0],
                                            fld_maps[1]);

    if (!hFeat)
        return false;

    OGRErr err = OGR_L_UpsertFeature(m_hLayer, hFeat);
     if (err != OGRERR_NONE) {
        Rcpp::Rcout << CPLGetLastErrorMsg() << std::endl;
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
         if (!quiet) {
            Rcpp::Rcout << "cannot delete, the layer was opened read-only" <<
                    std::endl;
         }
        return false;
    }
    else if (!OGR_L_TestCapability(m_hLayer, OLCDeleteFeature)) {
         if (!quiet) {
            Rcpp::Rcout <<
                "the layer does not have delete feature capability"
                << std::endl;
         }
        return false;
    }

    if (fid.isNULL() || !Rcpp::is<Rcpp::NumericVector>(fid))
        Rcpp::stop("'fid' must be a length-1 `numeric` vector (integer64)");

    Rcpp::NumericVector fid_(fid);
    if (fid_.size() != 1)
        Rcpp::stop("'fid' must be a length-1 `numeric` vector (integer64)");

    int64_t fid_in = OGRNullFID;
    if (Rcpp::isInteger64(fid_))
        fid_in = Rcpp::fromInteger64(fid_[0]);
    else
        fid_in = static_cast<int64_t>(fid_[0]);

    if (OGR_L_DeleteFeature(m_hLayer, fid_in) != OGRERR_NONE) {
         if (!quiet)
            Rcpp::Rcout << CPLGetLastErrorMsg() << std::endl;
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

bool GDALVector::startTransaction() {
    checkAccess_(GA_ReadOnly);

    bool force = this->transactionsForce;

    if (!force) {
        if (!GDALDatasetTestCapability(m_hDataset, ODsCTransactions)) {
            if (!quiet) {
                Rcpp::Rcout <<
                    "dataset does not have (efficient) transaction capability"
                    << std::endl;
             }
            return false;
        }
    }
    else {
        if (!GDALDatasetTestCapability(m_hDataset, ODsCTransactions) &&
            !GDALDatasetTestCapability(m_hDataset, ODsCEmulatedTransactions)) {

             if (!quiet) {
                Rcpp::Rcout << "dataset does not have transaction capability"
                    << std::endl;
             }
            return false;
        }
    }

    if (GDALDatasetStartTransaction(m_hDataset, force) != OGRERR_NONE) {
         if (!quiet)
            Rcpp::Rcout << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    else {
        return true;
    }
}

bool GDALVector::commitTransaction() {
    checkAccess_(GA_ReadOnly);

    if (GDALDatasetCommitTransaction(m_hDataset) != OGRERR_NONE) {
        if (!quiet)
            Rcpp::Rcout << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    else {
        return true;
    }
}

bool GDALVector::rollbackTransaction() {
    checkAccess_(GA_ReadOnly);

    if (GDALDatasetRollbackTransaction(m_hDataset) != OGRERR_NONE) {
        if (!quiet)
            Rcpp::Rcout << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    else {
        return true;
    }
}

Rcpp::CharacterVector GDALVector::getMetadata() const {

    checkAccess_(GA_ReadOnly);

    char **papszMD = nullptr;
    papszMD = GDALGetMetadata(m_hLayer, nullptr);

    int items = CSLCount(papszMD);
    if (items > 0) {
        Rcpp::CharacterVector md(items);
        for (int i=0; i < items; ++i) {
            md(i) = papszMD[i];
        }
        return md;
    }
    else {
        return "";
    }
}

bool GDALVector::setMetadata(const Rcpp::CharacterVector &metadata) {
    checkAccess_(GA_ReadOnly);

    std::vector<const char *> metadata_in(metadata.size() + 1);
    if (metadata.size() > 0) {
        for (R_xlen_t i = 0; i < metadata.size(); ++i) {
            metadata_in[i] = (const char *) (metadata[i]);
        }
    }
    metadata_in[metadata.size()] = nullptr;

    OGRErr err = GDALSetMetadata(m_hLayer, metadata_in.data(), nullptr);

    if (err != CE_None) {
        if (!quiet)
            Rcpp::Rcout << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    else {
        return true;
    }
}

std::string GDALVector::getMetadataItem(const std::string &mdi_name) const {
    checkAccess_(GA_ReadOnly);

    std::string mdi = "";

    if (GDALGetMetadataItem(m_hLayer, mdi_name.c_str(), nullptr) != nullptr) {
        mdi += std::string(GDALGetMetadataItem(m_hLayer, mdi_name.c_str(),
                                               nullptr));
    }

    return mdi;
}

bool GDALVector::layerIntersection(
        GDALVector* const &method_layer,
        GDALVector* const &result_layer,
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
                    method_layer->getOGRLayerH_(),
                    result_layer->getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if (!quiet) {
        Rcpp::Rcerr << "error during Intersection, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerUnion(
        GDALVector* const &method_layer,
        GDALVector* const &result_layer,
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
                    method_layer->getOGRLayerH_(),
                    result_layer->getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if (!quiet) {
        Rcpp::Rcerr << "error during Union, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerSymDifference(
        GDALVector* const &method_layer,
        GDALVector* const &result_layer,
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
                    method_layer->getOGRLayerH_(),
                    result_layer->getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if (!quiet) {
        Rcpp::Rcerr << "error during SymDifference, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerIdentity(
        GDALVector* const &method_layer,
        GDALVector* const &result_layer,
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
                    method_layer->getOGRLayerH_(),
                    result_layer->getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if (!quiet) {
        Rcpp::Rcerr << "error during Identity, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerUpdate(
        GDALVector* const &method_layer,
        GDALVector* const &result_layer,
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
                    method_layer->getOGRLayerH_(),
                    result_layer->getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if (!quiet) {
        Rcpp::Rcerr << "error during Update, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerClip(
        GDALVector* const &method_layer,
        GDALVector* const &result_layer,
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
                    method_layer->getOGRLayerH_(),
                    result_layer->getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if (!quiet) {
        Rcpp::Rcerr << "error during Clip, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

bool GDALVector::layerErase(
        GDALVector* const &method_layer,
        GDALVector* const &result_layer,
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
                    method_layer->getOGRLayerH_(),
                    result_layer->getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err == OGRERR_NONE) {
        ret = true;
    }
    else if (!quiet) {
        Rcpp::Rcerr << "error during Erase, or execution interrupted" <<
                std::endl;
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    return ret;
}

void GDALVector::close() {
    releaseArrowStream();
    if (m_hDataset != nullptr) {
        if (m_is_sql)
            GDALDatasetReleaseResultSet(m_hDataset, m_hLayer);
        GDALReleaseDataset(m_hDataset);
        m_hDataset = nullptr;
        m_hLayer = nullptr;
    }
}

void GDALVector::OGRFeatureFromList_dumpReadble(
                 const Rcpp::List &feat) const {

    // undocumented method exposed in R for diagnostic use

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 8, 0)
    Rcpp::stop("'OGRFeatureFromList_dumpReadble()' requires GDAL >= 3.8");

#else
    std::vector<std::map<R_xlen_t, int>> fld_maps = validateFeatInput_(feat);
    OGRFeatureH hFeat = OGRFeatureFromList_(feat, 0, fld_maps[0], fld_maps[1]);

    if (hFeat == nullptr)
        Rcpp::stop("`OGRFeatureFromList_()` returned `nullptr`");

    std::vector<const char *> options = {"DISPLAY_GEOMETRY=SUMMARY", nullptr};
    char *out = nullptr;
    out = OGR_F_DumpReadableAsString(hFeat, options.data());
    if (out != nullptr)
        Rcpp::Rcout << out;
    CPLFree(out);
    OGR_F_Destroy(hFeat);
#endif
}

void GDALVector::show() const {
    std::string lyr_name = "";
    if (m_is_sql) {
        // the API call to OGR_L_GetName() returns only "SELECT" for SQL layer
        // instead get SQL statement used to open the layer
        lyr_name = m_layer_name;
    }
    else {
        lyr_name = getName();
    }

    Rcpp::Environment pkg = Rcpp::Environment::namespace_env("gdalraster");
    Rcpp::Function fn = pkg[".get_crs_name"];
    std::string crs_name = Rcpp::as<std::string>(fn(getSpatialRef()));

    Rcpp::Rcout << "C++ object of class GDALVector" << std::endl;
    Rcpp::Rcout << " Driver : " << getDriverLongName() << " (" <<
                                   getDriverShortName() << ")" << std::endl;
    Rcpp::Rcout << " DSN    : " << getDsn() << std::endl;
    Rcpp::Rcout << " Layer  : " << lyr_name << std::endl;
    Rcpp::Rcout << " CRS    : " << crs_name << std::endl;
    Rcpp::Rcout << " Geom   : " << getGeomType() << std::endl;
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

void GDALVector::setDsn_(const std::string &dsn) {
    if (m_hDataset != nullptr) {
        std::string desc(GDALGetDescription(m_hDataset));
        if (m_dsn == "" && desc == "") {
            m_dsn = Rcpp::as<std::string>(check_gdal_filename(dsn));
            GDALSetDescription(m_hDataset, m_dsn.c_str());
        }
        else {
            Rcpp::Rcout << "the DSN cannot be set on this object" << std::endl;
            return;
        }
    }
    else {
        if (m_dsn == "") {
            m_dsn = Rcpp::as<std::string>(check_gdal_filename(dsn));
        }
        else {
            Rcpp::Rcout << "the DSN cannot be set on this object" << std::endl;
            return;
        }
    }
#if __has_include("ogr_recordbatch.h")
    // ensure release callback initialized since it will be checked at closing
    m_stream.release = nullptr;
#endif
}

GDALDatasetH GDALVector::getGDALDatasetH_() const {
    checkAccess_(GA_ReadOnly);

    return m_hDataset;
}

void GDALVector::setGDALDatasetH_(const GDALDatasetH &hDs, bool with_update) {
    m_hDataset = hDs;
    if (with_update)
        m_eAccess = GA_Update;
    else
        m_eAccess = GA_ReadOnly;

    if (GDALDataset::FromHandle(m_hDataset)->GetShared() == TRUE)
        m_shared = true;
    else
        m_shared = false;
}

OGRLayerH GDALVector::getOGRLayerH_() const {
    checkAccess_(GA_ReadOnly);

    return m_hLayer;
}

void GDALVector::setOGRLayerH_(const OGRLayerH &hLyr,
                               const std::string &lyr_name) {
    m_hLayer = hLyr;
    m_layer_name = lyr_name;
#if __has_include("ogr_recordbatch.h")
    // ensure release callback initialized since it will be checked at closing
    m_stream.release = nullptr;
#endif
}

void GDALVector::setFieldNames_() {
    if (m_hLayer == nullptr)
        return;

    OGRFeatureDefnH hFDefn = nullptr;
    hFDefn = OGR_L_GetLayerDefn(m_hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    // attribute fields
    for (int iField = 0; iField < OGR_FD_GetFieldCount(hFDefn); ++iField) {
        OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, iField);
        if (hFieldDefn == nullptr)
            Rcpp::stop("could not obtain field definition");

        m_field_names.push_back(OGR_Fld_GetNameRef(hFieldDefn));
    }

    // geometry fields
    for (int i = 0; i < OGR_FD_GetGeomFieldCount(hFDefn); ++i) {
        OGRGeomFieldDefnH hGeomFldDefn =
                OGR_FD_GetGeomFieldDefn(hFDefn, i);
        if (hGeomFldDefn == nullptr)
            Rcpp::stop("could not obtain geometry field definition");

        std::string geom_fld_name(OGR_GFld_GetNameRef(hGeomFldDefn));
        if (geom_fld_name == "")
            geom_fld_name = "OGR_GEOMETRY";
        m_field_names.push_back(geom_fld_name);
    }

    // TODO
    // m_field_names.push_back("OGR_STYLE");
}

SEXP GDALVector::createDF_(R_xlen_t nrow) const {
    // create a data frame based on the layer definition
    // this method must be kept consistent with fetch()

    const OGRFeatureDefnH hFDefn = OGR_L_GetLayerDefn(m_hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    const int nFields = OGR_FD_GetFieldCount(hFDefn);
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
    const int nOutCols = (1 + nFields - nIgnoredFields + nGeomFields -
                          nIgnoredGeomFields);
    Rcpp::List df(nOutCols);
    Rcpp::CharacterVector col_names(nOutCols);

    size_t col_num = 0;

    // FID column
    if (nrow > 0) {
        std::vector<int64_t> fid{};
        try {
            fid.resize(nrow, NA_INTEGER64);
        }
        catch (const std::exception &) {
            Rcpp::stop("failed to allocate memory for 'fid' column");
        }
        df[col_num] = Rcpp::wrap(fid);
    }
    else {
        Rcpp::NumericVector fid(0);
        fid.attr("class") = "integer64";
        df[col_num] = fid;
    }
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
                    Rcpp::LogicalVector v = Rcpp::no_init(nrow);
                    df[col_num] = v;
                }
                else {
                    Rcpp::IntegerVector v = Rcpp::no_init(nrow);
                    df[col_num] = v;
                }
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            case OFTInteger64:
            {
                if (nrow > 0) {
                    std::vector<int64_t> v{};
                    try {
                        v.resize(nrow, NA_INTEGER64);
                    }
                    catch (const std::exception &) {
                        Rcpp::stop("failed to allocate integer64 column");
                    }
                    df[col_num] = Rcpp::wrap(v);
                }
                else {
                    Rcpp::NumericVector v(0);
                    v.attr("class") = "integer64";
                    df[col_num] = v;
                }
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            case OFTReal:
            {
                Rcpp::NumericVector v = Rcpp::no_init(nrow);
                df[col_num] = v;
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            case OFTDate:
            {
                Rcpp::NumericVector v = Rcpp::no_init(nrow);
                v.attr("class") = "Date";
                df[col_num] = v;
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            case OFTDateTime:
            {
                Rcpp::NumericVector v = Rcpp::no_init(nrow);
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
                Rcpp::List v = Rcpp::no_init(nrow);
                df[col_num] = v;
                col_names[col_num] = OGR_Fld_GetNameRef(hFieldDefn);
            }
            break;

            default:
            {
                // use string
                Rcpp::CharacterVector v = Rcpp::no_init(nrow);
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

        if (STARTS_WITH_CI(this->returnGeomAs.c_str(), "WKB") ||
             STARTS_WITH_CI(this->returnGeomAs.c_str(), "BBOX")) {
            Rcpp::List v = Rcpp::no_init(nrow);
            df[col_num] = v;
        }
        else {
            Rcpp::CharacterVector v = Rcpp::no_init(nrow);
            df[col_num] = v;
        }

        std::string geomFldName(OGR_GFld_GetNameRef(hGeomFldDefn));
        if (geomFldName == "")
            geomFldName = this->defaultGeomColName;

        col_names[col_num] = geomFldName;
    }

    df.attr("class") = Rcpp::CharacterVector{"OGRFeatureSet", "data.frame"};
    df.names() = col_names;
    df.attr("row.names") = Rcpp::seq_len(nrow);
    return df;
}

void GDALVector::attachGISattributes_(Rcpp::List *ogr_feat_obj,
        const Rcpp::CharacterVector &geom_col,
        const Rcpp::CharacterVector &geom_col_type,
        const Rcpp::CharacterVector &geom_col_srs,
        const std::string &geom_format) const {

    /* ************************************************************************
    'ogr_feat_obj' is expected to be one of:
        Rcpp::DataFrame (for S3 class "OGRFeatureSet")
        Rcpp::List (for S3 class "OGRFeature")

    currently called from fetch(), but GIS attributes are also included via
    fetch() in getFeature() and getNextFeature()
    ************************************************************************ */

    Rcpp::List gis = Rcpp::List::create(
        Rcpp::Named("type") = "vector",
        Rcpp::Named("geom_column") = geom_col,
        Rcpp::Named("geom_col_type") = geom_col_type,
        Rcpp::Named("geom_col_srs") = geom_col_srs,
        Rcpp::Named("geom_format") = geom_format);

    ogr_feat_obj->attr("gis") = gis;
}

std::vector<std::map<R_xlen_t, int>> GDALVector::validateFeatInput_(
                                        const Rcpp::List &feature) const {

    // This function is always called before OGRFeatureFromList_() and does
    // most of the input validation, and provides the field name-to-field index
    // mappings.
    // *** It must be kept consistent with OGRFeatureFromList_() ***

    if (feature.size() == 0)
        Rcpp::stop("feature input is empty");

    if (!feature.hasAttribute("names"))
        Rcpp::stop("feature input must be a named list or data frame");

    Rcpp::CharacterVector names = feature.names();
    if (names.size() == 0)
        Rcpp::stop("names vector is empty");

    const OGRFeatureDefnH hFDefn = OGR_L_GetLayerDefn(m_hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    OGRFeatureH hFeat = nullptr;
    hFeat = OGR_F_Create(hFDefn);
    if (hFeat == nullptr)
        Rcpp::stop("failed to create OGRFeature object");

    int nGeomFields = OGR_F_GetGeomFieldCount(hFeat);

    std::map<R_xlen_t, int> map_flds;
    std::map<R_xlen_t, int> map_geom_flds;

    // go over the list element names and map list index to field index
    for (R_xlen_t i = 0; i < names.size(); ++i) {
        // FID is not a regular attribute field on the layer
        // track it with the attribute fields and a special marker as its index
        if (EQUAL(names[i], "FID")) {
            if (!Rcpp::is<Rcpp::NumericVector>(feature[i])) {
                OGR_F_Destroy(hFeat);
                Rcpp::stop("FID must be `numeric` (may be `integer64`)");
            }
            map_flds.insert({i, FID_MARKER});
            continue;
        }

        // attribute fields
        int iField = OGR_F_GetFieldIndex(hFeat, names[i]);
        if (iField != -1) {
            map_flds.insert({i, iField});
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
                EQUAL(names[i], this->defaultGeomColName.c_str()) ||
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

    // check attribute fields
    for (auto it = map_flds.cbegin(); it != map_flds.cend(); ++it) {
        const R_xlen_t col_idx = it->first;
        const int fld_idx = it->second;

        if (fld_idx == FID_MARKER)
            continue;

        OGRFieldDefnH hFieldDefn = nullptr;
        hFieldDefn = OGR_F_GetFieldDefnRef(hFeat, fld_idx);
        if (hFieldDefn == nullptr) {
            OGR_F_Destroy(hFeat);
            Rcpp::stop("could not obtain field definition");
        }

        const OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);
        std::string msg_not_nullable =
                "`NA` or empty value given but field is not nullable";

        // Allow a column to be NULL.
        // Allow any column to be a LogicalVector to support a column having
        // all `NA` which is logical type (if not specified in R as a typed
        // NA like `NA_integer_`, `NA_real_`, etc.). These need to be handled
        // accordingly in OGRFeatureFromList_().
        if (feature[col_idx] == R_NilValue) {
            continue;
        }
        else if (Rcpp::is<Rcpp::LogicalVector>(feature[col_idx])) {
            Rcpp::LogicalVector v = feature[col_idx];
            if (v.size() == 0 || Rcpp::is_true(Rcpp::all(Rcpp::is_na(v))))
                continue;
        }
        else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
            Rcpp::List list_tmp = feature[col_idx];
            if (list_tmp[0] == R_NilValue)
                continue;
        }

        switch (fld_type) {
            case OFTInteger:
            {
                if (!(Rcpp::is<Rcpp::NumericVector>(feature[col_idx]) ||
                      Rcpp::is<Rcpp::IntegerVector>(feature[col_idx]) ||
                      Rcpp::is<Rcpp::LogicalVector>(feature[col_idx]))) {

                        OGR_F_Destroy(hFeat);
                        Rcpp::stop(
                            "OFTInteger field requires a compatible data type");
                }
            }
            break;

            case OFTInteger64:
            {
                if (!(Rcpp::is<Rcpp::NumericVector>(feature[col_idx]) ||
                      Rcpp::is<Rcpp::IntegerVector>(feature[col_idx]) ||
                      Rcpp::is<Rcpp::LogicalVector>(feature[col_idx]))) {

                    OGR_F_Destroy(hFeat);
                    Rcpp::stop(
                        "OFTInteger64 field requires a compatible data type");
                }
            }
            break;

            case OFTReal:
            {
                if (!(Rcpp::is<Rcpp::NumericVector>(feature[col_idx]) ||
                      Rcpp::is<Rcpp::IntegerVector>(feature[col_idx]))) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("OFTReal field requires 'numeric' data");
                }
            }
            break;

            case OFTString:
            {
                if (!Rcpp::is<Rcpp::CharacterVector>(feature[col_idx])) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("OFTString field requires 'character' data");
                }
            }
            break;

            case OFTDate:
            {
                if (!Rcpp::is<Rcpp::NumericVector>(feature[col_idx])) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop(
                            "OFTDate field requires 'numeric' data ('Date')");
                }
                Rcpp::NumericVector v = feature[col_idx];
                Rcpp::CharacterVector attr{};
                if (v.hasAttribute("class"))
                    attr = Rcpp::wrap(v.attr("class"));
                if (std::find(attr.begin(), attr.end(), "Date") == attr.end()) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop(
                        "value for OFTDate field must be of class 'Date'");
                }
            }
            break;

            case OFTDateTime:
            {
                if (!Rcpp::is<Rcpp::NumericVector>(feature[col_idx])) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop(
                        "OFTDateTime requires 'numeric' data ('POSIXct')");
                }
                Rcpp::NumericVector v = feature[col_idx];
                Rcpp::CharacterVector attr{};
                if (v.hasAttribute("class"))
                    attr = Rcpp::wrap(v.attr("class"));
                if (std::find(attr.begin(), attr.end(), "POSIXct") ==
                        attr.end()) {

                    OGR_F_Destroy(hFeat);
                    Rcpp::stop(
                        "value for OFTDateTime field must be of class 'POSIXct'");
                }
            }
            break;

            case OFTTime:
            {
                if (!Rcpp::is<Rcpp::CharacterVector>(feature[col_idx])) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::stop("OFTTime field requires 'character' data");
                }
            }
            break;

            case OFTIntegerList:
            {
                bool type_mismatch = true;
                if (Rcpp::is<Rcpp::NumericVector>(feature[col_idx]) ||
                    Rcpp::is<Rcpp::IntegerVector>(feature[col_idx]) ||
                    Rcpp::is<Rcpp::LogicalVector>(feature[col_idx])) {

                        type_mismatch = false;
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::NumericVector>(list_tmp[0]) ||
                        Rcpp::is<Rcpp::IntegerVector>(list_tmp[0]) ||
                        Rcpp::is<Rcpp::LogicalVector>(list_tmp[0])) {

                        type_mismatch = false;
                    }
                }

                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop(
                        "OFTIntegerList field requires a compatible data type");
                }
            }
            break;

            case OFTInteger64List:
            {
                bool type_mismatch = true;
                if (Rcpp::is<Rcpp::NumericVector>(feature[col_idx])) {
                    type_mismatch = false;
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::NumericVector>(list_tmp[0])) {
                        type_mismatch = false;
                    }
                }

                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop(
                        "OFTInteger64List field requires 'numeric' vectors ('integer64')");
                }
            }
            break;

            case OFTRealList:
            {
                bool type_mismatch = true;
                if (Rcpp::is<Rcpp::NumericVector>(feature[col_idx])) {
                    type_mismatch = false;
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::NumericVector>(list_tmp[0])) {
                        type_mismatch = false;
                    }
                }

                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OFTRealList field requires 'numeric' vectors");
                }
            }
            break;

            case OFTStringList:
            {
                bool type_mismatch = true;
                if (Rcpp::is<Rcpp::CharacterVector>(feature[col_idx])) {
                    type_mismatch = false;
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::CharacterVector>(list_tmp[0])) {
                        type_mismatch = false;
                    }
                }

                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop(
                        "OFTStringList field requires 'character' vectors");
                }
            }
            break;

            case OFTBinary:
            {
                bool type_mismatch = true;
                if (Rcpp::is<Rcpp::RawVector>(feature[col_idx])) {
                    type_mismatch = false;
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::RawVector>(list_tmp[0])) {
                        type_mismatch = false;
                    }
                }

                if (type_mismatch) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::stop("OFTBinary field requires 'raw' vectors");
                }
            }
            break;

            default:
                Rcpp::Rcout << "unhandled OGRFieldType: " << fld_type
                        << std::endl;
                break;
        }
    }

    OGR_F_Destroy(hFeat);
    std::vector<std::map<R_xlen_t, int>> ret = {map_flds, map_geom_flds};
    return ret;
}

OGRFeatureH GDALVector::OGRFeatureFromList_(const Rcpp::List &feature,
        R_xlen_t row_idx, const std::map<R_xlen_t, int> &map_flds,
        const std::map<R_xlen_t, int> &map_geom_flds) const {

    // Returns an OGRFeature object from input given as R named list, which
    // might be a row of a data frame specified with row_idx.

    // Most of the input validation is done in validateFeatInput_(), which
    // must always be called first and provides the field name-to-field index
    // mappings, and *must be kept consistent with OGRFeatureFromList_()*.

    // The returned feature must be destroyed with OGR_F_Destroy().

    const OGRFeatureDefnH hFDefn = OGR_L_GetLayerDefn(m_hLayer);
    OGRFeatureH hFeat = OGR_F_Create(hFDefn);

    // set FID if one is given and is not NA
    bool has_fid = false;
    R_xlen_t fid_col = 0;
    for (auto it = map_flds.cbegin(); it != map_flds.cend(); ++it) {
        if (it->second == FID_MARKER) {
            has_fid = true;
            fid_col = it->first;
        }
    }
    if (has_fid && feature[fid_col] != R_NilValue) {
        bool set_fid = false;
        Rcpp::NumericVector fid_in = feature[fid_col];
        int64_t fid = OGRNullFID;
        if (Rcpp::isInteger64(fid_in)) {
            fid = Rcpp::fromInteger64(fid_in[row_idx]);
            if (!ISNA_INTEGER64(fid))
                set_fid = true;
        }
        else {
            if (!Rcpp::NumericVector::is_na(fid_in[row_idx])) {
                fid = static_cast<int64_t>(fid_in[row_idx]);
                set_fid = true;
            }
        }
        if (set_fid) {
            if (OGR_F_SetFID(hFeat, fid) != OGRERR_NONE) {
                OGR_F_Destroy(hFeat);
                Rcpp::Rcout << "failed to set FID: " << Rcpp::wrap(fid) <<
                    " (row index " << row_idx << ")" << std::endl;

                return nullptr;
            }
        }
    }

    std::string msg_not_nullable =
            "`NA` or 0-length value given but field is not nullable";

    // set attribute fields
    for (auto it = map_flds.cbegin(); it != map_flds.cend(); ++it) {
        const R_xlen_t col_idx = it->first;
        const int fld_idx = it->second;

        if (fld_idx == FID_MARKER)
            continue;

        const OGRFieldDefnH hFieldDefn = OGR_F_GetFieldDefnRef(hFeat, fld_idx);

        // partial checks for NULL and NA in the input for this field
        bool set_field_null = false;
        if (feature[col_idx] == R_NilValue) {
            set_field_null = true;
        }
        else if (Rcpp::is<Rcpp::LogicalVector>(feature[col_idx])) {
            Rcpp::LogicalVector lv = feature[col_idx];
            if (lv.size() == 0 || Rcpp::LogicalVector::is_na(lv[row_idx]))
                set_field_null = true;
        }
        else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
            Rcpp::List list_tmp = feature[col_idx];
            if (list_tmp[row_idx] == R_NilValue)
                set_field_null = true;
        }
        if (set_field_null) {
            if (OGR_Fld_IsNullable(hFieldDefn)) {
                OGR_F_SetFieldNull(hFeat, fld_idx);
                continue;
            }
            else {
                OGR_F_Destroy(hFeat);
                Rcpp::Rcout <<
                    "`NA` or `NULL` for non-nullable field with column index: "
                    << col_idx << " (row index " << row_idx << ")" << std::endl;

                return nullptr;
            }
        }

        const OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);

        switch (fld_type) {
            case OFTInteger:
            {
                const OGRFieldSubType fld_subtype =
                    OGR_Fld_GetSubType(hFieldDefn);

                if (fld_subtype == OFSTBoolean) {
                    Rcpp::LogicalVector lv = feature[col_idx];
                    // logical NA or 0-length has already been checked above
                    OGR_F_SetFieldInteger(hFeat, fld_idx, lv[row_idx]);
                }
                else {
                    Rcpp::IntegerVector iv = feature[col_idx];
                    if (iv.size() == 0 ||
                        Rcpp::IntegerVector::is_na(iv[row_idx])) {

                        if (OGR_Fld_IsNullable(hFieldDefn)) {
                            OGR_F_SetFieldNull(hFeat, fld_idx);
                        }
                        else {
                            OGR_F_Destroy(hFeat);
                            Rcpp::Rcout << msg_not_nullable << " (row index "
                                << row_idx << ")" << std::endl;

                            return nullptr;
                        }
                    }
                    else {
                        OGR_F_SetFieldInteger(hFeat, fld_idx, iv[row_idx]);
                    }
                }
            }
            break;

            case OFTInteger64:
            {
                Rcpp::NumericVector nv = feature[col_idx];
                int64_t value = NA_INTEGER64;
                if (nv.size() > 0) {
                    if (Rcpp::isInteger64(nv))
                        value = Rcpp::fromInteger64(nv[row_idx]);
                    else
                        value = static_cast<int64_t>(nv[row_idx]);
                }
                if (nv.size() > 0 && !ISNA_INTEGER64(value))
                    OGR_F_SetFieldInteger64(hFeat, fld_idx, value);
                else {
                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
            }
            break;

            case OFTReal:
            {
                Rcpp::NumericVector nv = feature[col_idx];
                if (nv.size() == 0 || Rcpp::NumericVector::is_na(nv[row_idx])) {
                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else {
                    OGR_F_SetFieldDouble(hFeat, fld_idx, nv[row_idx]);
                }
            }
            break;

            case OFTString:
            {
                Rcpp::CharacterVector cv = feature[col_idx];
                if (cv.size() == 0 ||
                    Rcpp::CharacterVector::is_na(cv[row_idx])) {

                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else {
                    OGR_F_SetFieldString(hFeat, fld_idx, cv[row_idx]);
                }
            }
            break;

            case OFTDate:
            {
                Rcpp::NumericVector nv = feature[col_idx];
                if (nv.size() == 0 || Rcpp::NumericVector::is_na(nv[row_idx])) {
                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                        continue;
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else {
                    int64_t nUnixTime = nv[row_idx] * 86400;
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
                Rcpp::NumericVector nv = feature[col_idx];
                if (nv.size() == 0 || Rcpp::NumericVector::is_na(nv[row_idx])) {
                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                        continue;
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else {
                    const int64_t nUnixTime = static_cast<int64_t>(nv[row_idx]);
                    struct tm brokendowntime;
                    CPLUnixTimeToYMDHMS(nUnixTime, &brokendowntime);
                    float sec = brokendowntime.tm_sec +
                                std::fmod(static_cast<float>(nv[row_idx]),
                                          1.0f);

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
                Rcpp::CharacterVector cv = feature[col_idx];
                if (cv.size() == 0 ||
                    Rcpp::CharacterVector::is_na(cv[row_idx])) {

                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                        continue;
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }

                int nYr = 0, nMo = 0, nDay = 0, nHr = 0, nMin = 0, nSec = 0;
                if (std::sscanf(cv[row_idx], "%02d:%02d:%02d",
                                &nHr, &nMin, &nSec) == 3) {

                    OGR_F_SetFieldDateTime(hFeat, fld_idx, nYr, nMo, nDay,
                                           nHr, nMin, nSec, 0);
                }
                else {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout <<
                        "value for OFTTime field requires format 'HH:MM:SS'"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }
            }
            break;

            case OFTIntegerList:
            {
                Rcpp::IntegerVector iv;
                if (Rcpp::is<Rcpp::NumericVector>(feature[col_idx]) ||
                    Rcpp::is<Rcpp::IntegerVector>(feature[col_idx]) ||
                    Rcpp::is<Rcpp::LogicalVector>(feature[col_idx])) {

                    iv = feature[col_idx];
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::NumericVector>(list_tmp[row_idx]) ||
                        Rcpp::is<Rcpp::IntegerVector>(list_tmp[row_idx]) ||
                        Rcpp::is<Rcpp::LogicalVector>(list_tmp[row_idx])) {

                        iv = list_tmp[row_idx];
                    }
                }
                else {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout <<
                        "OFTIntegerList field requires a compatible data type"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }

                if (iv.size() == 0 ||
                    Rcpp::is_true(Rcpp::all(Rcpp::is_na(iv)))) {

                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else {
                    std::vector<int> values = Rcpp::as<std::vector<int>>(iv);
                    OGR_F_SetFieldIntegerList(hFeat, fld_idx, values.size(),
                                              values.data());
                }
            }
            break;

            case OFTInteger64List:
            {
                Rcpp::NumericVector nv;
                if (Rcpp::is<Rcpp::NumericVector>(feature[col_idx])) {
                    nv = feature[col_idx];
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::NumericVector>(list_tmp[row_idx]))
                        nv = list_tmp[row_idx];
                }
                else {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout <<
                        "OFTInteger64List field requires a 'numeric' vector"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }

                if (nv.size() == 0) {
                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else if (Rcpp::isInteger64(nv)) {
                    std::vector<int64_t> values =
                            Rcpp::fromInteger64(nv, false);

                    OGR_F_SetFieldInteger64List(hFeat, fld_idx, values.size(),
                            reinterpret_cast<const GInt64 *>(values.data()));
                }
                else {
                    std::vector<GIntBig> values(nv.begin(), nv.end());
                    OGR_F_SetFieldInteger64List(hFeat, fld_idx, values.size(),
                                                values.data());
                }
            }
            break;

            case OFTRealList:
            {
                Rcpp::NumericVector nv;
                if (Rcpp::is<Rcpp::NumericVector>(feature[col_idx])) {
                    nv = feature[col_idx];
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::NumericVector>(list_tmp[row_idx]))
                        nv = list_tmp[row_idx];
                }
                else {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout <<
                        "OFTRealList field requires a 'numeric' vector"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }

                if (nv.size() == 0 ||
                    Rcpp::is_true(Rcpp::all(Rcpp::is_na(nv)))) {

                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else {
                    std::vector<double> values =
                            Rcpp::as<std::vector<double>>(nv);

                    OGR_F_SetFieldDoubleList(hFeat, fld_idx, values.size(),
                                             values.data());
                }
            }
            break;

            case OFTStringList:
            {
                Rcpp::CharacterVector cv;
                if (Rcpp::is<Rcpp::CharacterVector>(feature[col_idx])) {
                    cv = feature[col_idx];
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::CharacterVector>(list_tmp[row_idx]))
                        cv = list_tmp[row_idx];
                }
                else {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout <<
                        "OFTStringList field requires a 'character' vector"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }

                if (cv.size() == 0 ||
                    Rcpp::is_true(Rcpp::all(Rcpp::is_na(cv)))) {

                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else {
                    std::vector<const char *> values(cv.begin(), cv.end());
                    values.push_back(nullptr);
                    OGR_F_SetFieldStringList(hFeat, fld_idx, values.data());
                }
            }
            break;

            case OFTBinary:
            {
                Rcpp::RawVector rv;
                if (Rcpp::is<Rcpp::RawVector>(feature[col_idx])) {
                    rv = feature[col_idx];
                }
                else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
                    Rcpp::List list_tmp = feature[col_idx];
                    if (Rcpp::is<Rcpp::RawVector>(list_tmp[row_idx]))
                        rv = list_tmp[row_idx];
                }
                else {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout << "OFTBinary field requires a 'raw' vector"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }

                if (rv.size() == 0) {
                    if (OGR_Fld_IsNullable(hFieldDefn)) {
                        OGR_F_SetFieldNull(hFeat, fld_idx);
                    }
                    else {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << msg_not_nullable << " (row index "
                            << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else {
                    OGR_F_SetFieldBinary(hFeat, fld_idx, rv.size(), &rv[0]);
                }
            }
            break;

            default:
                OGR_F_Destroy(hFeat);
                Rcpp::Rcout << "unhandled OGRFieldType: " << fld_type
                    << std::endl;

                return nullptr;

                break;
        }
    }

    // set geom fields
    for (auto it = map_geom_flds.cbegin(); it != map_geom_flds.cend(); ++it) {
        const R_xlen_t col_idx = it->first;
        const int gfld_idx = it->second;

        // geometry fields may originate in a data frame list column
        // set up a generic RObject with the correct reference
        Rcpp::RObject robj;
        bool have_geom = false;
        bool is_raw = false;

        if (feature[col_idx] == R_NilValue) {
            robj = feature[col_idx];
        }
        else if (Rcpp::is<Rcpp::RawVector>(feature[col_idx])) {
            robj = feature[col_idx];
            have_geom = true;
            is_raw = true;
        }
        else if (Rcpp::is<Rcpp::CharacterVector>(feature[col_idx])) {
            Rcpp::CharacterVector cv = feature[col_idx];
            robj = cv[row_idx];
            have_geom = true;
            is_raw = false;
        }
        else if (Rcpp::is<Rcpp::List>(feature[col_idx])) {
            Rcpp::List list_tmp = feature[col_idx];
            robj = list_tmp[row_idx];
            if (!robj.isNULL()) {
                if (Rcpp::is<Rcpp::RawVector>(robj)) {
                    have_geom = true;
                    is_raw = true;
                }
                else if (Rcpp::is<Rcpp::CharacterVector>(robj)) {
                    have_geom = true;
                    is_raw = false;
                }
            }
        }
        else {
            robj = feature[col_idx];
        }

        if (!have_geom) {
            // not raw vector or character
            // check for R NULL or NA and attempt to set NULL geom in that case
            bool set_null_geom = false;
            if (robj.isNULL()) {
                set_null_geom = true;
            }
            else if (Rcpp::is<Rcpp::LogicalVector>(robj)) {
                Rcpp::LogicalVector lv = Rcpp::as<Rcpp::LogicalVector>(robj);
                if (Rcpp::LogicalVector::is_na(lv[row_idx]))
                    set_null_geom = true;
            }

            if (set_null_geom) {
                OGRErr err = OGRERR_NONE;
                err = OGR_F_SetGeomField(hFeat, gfld_idx, nullptr);
                if (err != OGRERR_NONE) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout << "failed to set geometry field as NULL"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }
                continue;
            }
            else {
                OGR_F_Destroy(hFeat);
                Rcpp::Rcout <<
                    "geometry must be `raw` (WKB) or `character` (WKT)"
                    << " (row index " << row_idx << ")" << std::endl;

                return nullptr;
            }
            // code below will also check for raw(0) and character NA
        }

        if (is_raw) {
            Rcpp::RawVector v = Rcpp::as<Rcpp::RawVector>(robj);
            OGRGeometryH hGeom = nullptr;
            OGRErr err = OGRERR_NONE;
            if (v.size() > 0) {
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
                err = OGR_G_CreateFromWkb(&v[0], nullptr, &hGeom,
                                          static_cast<int>(v.size()));
#else
                err = OGR_G_CreateFromWkbEx(&v[0], nullptr, &hGeom, v.size());
#endif
            }
            // if raw(0), hGeom will be nullptr, so attempts to set NULL geom
            if (err == OGRERR_NONE) {
                err = OGR_F_SetGeomFieldDirectly(hFeat, gfld_idx, hGeom);
                if (err != OGRERR_NONE) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout << "failed to set geometry field"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }
            }
            else if (err == OGRERR_NOT_ENOUGH_DATA) {
                OGR_F_Destroy(hFeat);
                Rcpp::Rcout << "OGRERR_NOT_ENOUGH_DATA, failed to create geom"
                    << " (row index " << row_idx << ")" << std::endl;

                return nullptr;
            }
            else if (err == OGRERR_UNSUPPORTED_GEOMETRY_TYPE) {
                OGR_F_Destroy(hFeat);
                Rcpp::Rcout << "OGRERR_UNSUPPORTED_GEOMETRY_TYPE"
                    << " (row index " << row_idx << ")" << std::endl;

                return nullptr;
            }
            else if (err == OGRERR_CORRUPT_DATA) {
                OGR_F_Destroy(hFeat);
                Rcpp::Rcout << "OGRERR_CORRUPT_DATA, failed to create geom"
                    << " (row index " << row_idx << ")" << std::endl;

                return nullptr;
            }
        }
        else {
            // wkt
            Rcpp::CharacterVector v = Rcpp::as<Rcpp::CharacterVector>(robj);
            if (v.size() == 1) {
                OGRGeometryH hGeom = nullptr;
                OGRErr err = OGRERR_NONE;
                if (!Rcpp::CharacterVector::is_na(v[0])) {
                    std::string wkt = Rcpp::as<std::string>(v[0]);
                    char *pszWKT = const_cast<char *>(wkt.c_str());
                    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
                }
                // if NA, hGeom will be nullptr, so attempts to set NULL geom
                if (err == OGRERR_NONE) {
                    err = OGR_F_SetGeomFieldDirectly(hFeat, gfld_idx, hGeom);
                    if (err != OGRERR_NONE) {
                        OGR_F_Destroy(hFeat);
                        Rcpp::Rcout << "failed to set geometry field"
                            << " (row index " << row_idx << ")" << std::endl;

                        return nullptr;
                    }
                }
                else if (err == OGRERR_NOT_ENOUGH_DATA) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout <<
                        "OGRERR_NOT_ENOUGH_DATA, failed to create geom"
                            << " (row index " << row_idx << ")" << std::endl;

                        return nullptr;
                }
                else if (err == OGRERR_NOT_ENOUGH_DATA) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout  << "OGRERR_UNSUPPORTED_GEOMETRY_TYPE"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }
                else if (err == OGRERR_CORRUPT_DATA) {
                    OGR_F_Destroy(hFeat);
                    Rcpp::Rcout  << "OGRERR_CORRUPT_DATA, failed to create geom"
                        << " (row index " << row_idx << ")" << std::endl;

                    return nullptr;
                }
            }
            else {
                OGR_F_Destroy(hFeat);
                Rcpp::Rcout <<
                    "WKT geometry must be a length-1 character vector"
                    << " (row index " << row_idx << ")" << std::endl;

                return nullptr;
            }
        }
    }

    return hFeat;
}

// Arrow callbacks
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 6, 0)
int GDALVector::arrow_get_schema(struct ArrowSchema* out) {
    return m_stream.get_schema(&m_stream, out);
}

int GDALVector::arrow_get_next(struct ArrowArray* out) {
    return m_stream.get_next(&m_stream, out);
}

const char* GDALVector::arrow_get_last_error() {
    return m_stream.get_last_error(&m_stream);
}

int GDALVector::arrow_get_schema_wrap(struct ArrowArrayStream* stream,
                                      struct ArrowSchema* out) {

    return reinterpret_cast<GDALVector*>(
            stream->private_data)->arrow_get_schema(out);
}

int GDALVector::arrow_get_next_wrap(struct ArrowArrayStream* stream,
                                    struct ArrowArray* out) {

    return reinterpret_cast<GDALVector*>(
            stream->private_data)->arrow_get_next(out);
}

const char* GDALVector::arrow_get_last_error_wrap(
        struct ArrowArrayStream* stream) {

    return reinterpret_cast<GDALVector*>(
            stream->private_data)->arrow_get_last_error();
}

void GDALVector::arrow_release_wrap(struct ArrowArrayStream* stream) {
    reinterpret_cast<GDALVector*>(stream->private_data)->releaseArrowStream();
}
#endif

// ****************************************************************************

RCPP_MODULE(mod_GDALVector) {
    Rcpp::class_<GDALVector>("GDALVector")

    .constructor
        ("Default constructor, no dataset opened")
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

    // create_ogr() object factory with 10 parameters
    .factory<const std::string&, const std::string&, const std::string&,
             const std::string&, const std::string&, const std::string&,
             const std::string&, const Rcpp::Nullable<Rcpp::CharacterVector>&,
             const Rcpp::Nullable<Rcpp::CharacterVector>&,
             const Rcpp::Nullable<Rcpp::List>&>
             (create_ogr)
    // ogr_layer_create() object factory with 7 parameters
    .factory<const std::string&, const std::string&,
             const Rcpp::Nullable<Rcpp::List>&,
             const std::string&, const std::string&,
             const Rcpp::Nullable<Rcpp::CharacterVector>&, bool>
             (ogr_layer_create)

    // undocumented read-only fields for internal use
    .field_readonly("m_layer_name", &GDALVector::m_layer_name)
    .field_readonly("m_is_sql", &GDALVector::m_is_sql)
    .field_readonly("m_dialect", &GDALVector::m_dialect)

    // read/write fields
    .field("defaultGeomColName", &GDALVector::defaultGeomColName)
    .field("promoteToMulti", &GDALVector::promoteToMulti)
    .field("convertToLinear", &GDALVector::convertToLinear)
    .field("returnGeomAs", &GDALVector::returnGeomAs)
    .field("wkbByteOrder", &GDALVector::wkbByteOrder)
    .field("arrowStreamOptions", &GDALVector::arrowStreamOptions)
    .field("quiet", &GDALVector::quiet)
    .field("transactionsForce", &GDALVector::transactionsForce)

    // methods
    .const_method("getDsn", &GDALVector::getDsn,
        "Return the DSN")
    .const_method("isOpen", &GDALVector::isOpen,
        "Is the dataset open?")
    .method("open", &GDALVector::open,
        "(Re-)open the dataset on the existing DSN and layer")
    .const_method("getFileList", &GDALVector::getFileList,
        "Fetch files forming dataset")
    .const_method("info", &GDALVector::info,
        "Print information about the vector layer")
    .const_method("getDriverShortName", &GDALVector::getDriverShortName,
         "Return the short name of the format driver")
    .const_method("getDriverLongName", &GDALVector::getDriverLongName,
        "Return the long name of the format driver")
    .const_method("getName", &GDALVector::getName,
        "Return the layer name")
    .const_method("getFieldNames", &GDALVector::getFieldNames,
        "Return a character vector of the layer's field names")
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
    .const_method("getFieldDomain", &GDALVector::getFieldDomain,
        "Get field domain specifications for the passed domain name")
    .method("setAttributeFilter", &GDALVector::setAttributeFilter,
        "Set a new attribute query")
    .const_method("getAttributeFilter", &GDALVector::getAttributeFilter,
        "Get the attribute filter string (or empty string if not set")
    .method("setIgnoredFields", &GDALVector::setIgnoredFields,
        "Set which fields can be omitted when retrieving features")
    .method("setSelectedFields", &GDALVector::setSelectedFields,
        "Set which fields to include when retrieving features")
    .const_method("getIgnoredFields", &GDALVector::getIgnoredFields,
        "Get the list of ignored fields in a character vector (may be empty)")
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
    .method("getArrowStream", &GDALVector::getArrowStream,
        "Expose an Arrow C stream on the layer")
    .method("releaseArrowStream", &GDALVector::releaseArrowStream,
        "Release the Arrow C stream on the layer")
    .method("setFeature", &GDALVector::setFeature,
        "Rewrite/replace an existing feature within the layer")
    .method("createFeature", &GDALVector::createFeature,
        "Create and write a new feature within the layer")
    .method("batchCreateFeature", &GDALVector::batchCreateFeature,
        "Create and write a new batch of features within the layer")
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
    .const_method("getMetadata", &GDALVector::getMetadata,
        "Return a list of metadata name=value")
    .method("setMetadata", &GDALVector::setMetadata,
        "Set metadata from a list of name=value")
    .const_method("getMetadataItem", &GDALVector::getMetadataItem,
        "Return the value of a metadata item")
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
    .const_method("show", &GDALVector::show,
        "S4 show()")

    ;
}
