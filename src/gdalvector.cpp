/* Implementation of class GDALVector. Encapsulates an OGRLayer and its
   GDALDataset. Requires {bit64} on the R side for its integer64 S3 type.

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "gdal.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "cpl_time.h"
// #include "ogrsf_frmts.h"
#include "ogr_srs_api.h"

#include "gdalraster.h"
#include "gdalvector.h"
#include "ogr_util.h"

GDALVector::GDALVector() :
            dsn_in(""),
            layer_in(""),
            is_sql_in(false),
            open_options_in(Rcpp::CharacterVector::create()),
            spatial_filter_in(""),
            dialect_in(""),
            hDataset(nullptr),
            eAccess(GA_ReadOnly),
            hLayer(nullptr) {}

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

            layer_in(layer),
            open_options_in(open_options.isNotNull() ? open_options :
                            Rcpp::CharacterVector::create()),
            spatial_filter_in(spatial_filter),
            dialect_in(dialect),
            hDataset(nullptr),
            eAccess(GA_ReadOnly),
            hLayer(nullptr) {

    dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    open(read_only);
}

void GDALVector::open(bool read_only) {
    if (dsn_in == "")
        Rcpp::stop("DSN is not set");

    if (hDataset != nullptr) {
        if (is_sql_in)
            GDALDatasetReleaseResultSet(hDataset, hLayer);
        GDALReleaseDataset(hDataset);
        hDataset = nullptr;
        hLayer = nullptr;
    }

    if (read_only)
        eAccess = GA_ReadOnly;
    else
        eAccess = GA_Update;

    std::vector<char *> dsoo(open_options_in.size() + 1);
    if (open_options_in.size() > 0) {
        for (R_xlen_t i = 0; i < open_options_in.size(); ++i) {
            dsoo[i] = (char *) (open_options_in[i]);
        }
    }
    dsoo.push_back(nullptr);

    OGRGeometryH hGeom_filter = nullptr;
    if (spatial_filter_in != "") {
        char* pszWKT = (char*) spatial_filter_in.c_str();
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

    hDataset = GDALOpenEx(dsn_in.c_str(), nOpenFlags, nullptr,
                          dsoo.data(), nullptr);
    if (hDataset == nullptr)
        Rcpp::stop("open dataset failed");

    const char* pszDialect = dialect_in.c_str();

    if (layer_in == "") {
        is_sql_in = false;
        hLayer = GDALDatasetGetLayer(hDataset, 0);
    }
    else if (STARTS_WITH_CI(layer_in.c_str(), "SELECT ")) {
        is_sql_in = true;
        if (EQUAL(pszDialect, "SQLite") && !has_spatialite())
            Rcpp::warning("spatialite not available");
        hLayer = GDALDatasetExecuteSQL(hDataset, layer_in.c_str(),
                                       hGeom_filter, pszDialect);
    }
    else {
        is_sql_in = false;
        hLayer = GDALDatasetGetLayerByName(hDataset, layer_in.c_str());
    }

    if (hLayer == nullptr) {
        GDALReleaseDataset(hDataset);
        Rcpp::stop("failed to get layer");
    }
    else {
        OGR_L_ResetReading(hLayer);
    }

    if (hGeom_filter != nullptr)
        OGR_G_DestroyGeometry(hGeom_filter);
}

bool GDALVector::isOpen() const {
    if (hDataset == nullptr)
        return false;
    else
        return true;
}

std::string GDALVector::getDsn() const {
    return dsn_in;
}

Rcpp::CharacterVector GDALVector::getFileList() const {
    checkAccess_(GA_ReadOnly);

    char **papszFiles;
    papszFiles = GDALGetFileList(hDataset);

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

    GDALDriverH hDriver = GDALGetDatasetDriver(hDataset);
    return GDALGetDriverShortName(hDriver);
}

std::string GDALVector::getDriverLongName() const {
    checkAccess_(GA_ReadOnly);

    GDALDriverH hDriver = GDALGetDatasetDriver(hDataset);
    return GDALGetDriverLongName(hDriver);
}

std::string GDALVector::getName() const {
    checkAccess_(GA_ReadOnly);

    return OGR_L_GetName(hLayer);
}

bool GDALVector::testCapability(std::string capability) const {
    checkAccess_(GA_ReadOnly);

    return OGR_L_TestCapability(hLayer, capability.c_str());
}

std::string GDALVector::getFIDColumn() const {
    checkAccess_(GA_ReadOnly);

    return OGR_L_GetFIDColumn(hLayer);
}

std::string GDALVector::getGeomType() const {
    checkAccess_(GA_ReadOnly);

    OGRwkbGeometryType eType = OGR_L_GetGeomType(hLayer);
    return OGRGeometryTypeToName(eType);
}

std::string GDALVector::getGeometryColumn() const {
    checkAccess_(GA_ReadOnly);

    return OGR_L_GetGeometryColumn(hLayer);
}

std::string GDALVector::getSpatialRef() const {
    // OGRLayer::GetSpatialRef() as WKT string
    checkAccess_(GA_ReadOnly);

    OGRSpatialReferenceH hSRS = OGR_L_GetSpatialRef(hLayer);
    if (hSRS == nullptr)
        Rcpp::stop("could not obtain spatial reference");
    char *pszSRS_WKT = nullptr;
    if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE)
        Rcpp::stop("error exporting SRS to WKT");
    std::string srs_wkt(pszSRS_WKT);
    CPLFree(pszSRS_WKT);

    return srs_wkt;
}

Rcpp::NumericVector GDALVector::bbox() {
    // Note: bForce=true in tha call to OGR_L_GetExtent(), so the entire
    // layer may be scanned to compute MBR.
    // see: testCapability("FastGetExtent")
    // Depending on the driver, a spatial filter may/may not be taken into
    // account. So it is safer to call bbox() without setting a spatial filter.
    checkAccess_(GA_ReadOnly);

    OGREnvelope envelope;
    if (OGR_L_GetExtent(hLayer, &envelope, true) != OGRERR_NONE)
        Rcpp::stop("the extent of the layer cannot be determined");

    Rcpp::NumericVector bbox_out =
            {envelope.MinX, envelope.MinY, envelope.MaxX, envelope.MaxY};

    return bbox_out;
}

Rcpp::List GDALVector::getLayerDefn() const {
    checkAccess_(GA_ReadOnly);

    OGRFeatureDefnH hFDefn;
    hFDefn = OGR_L_GetLayerDefn(hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    Rcpp::List list_out = Rcpp::List::create();
    std::string sValue = "";
    int nValue = -1;
    bool bValue;
    int iField;

    // attribute fields
    // TODO(ctoney): add field domain name
    for (iField = 0; iField < OGR_FD_GetFieldCount(hFDefn); ++iField) {
        Rcpp::List list_fld_defn = Rcpp::List::create();
        OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, iField);
        if (hFieldDefn == nullptr)
            Rcpp::stop("could not obtain field definition");

        OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);
        sValue = getOFTString_(fld_type);
        list_fld_defn.push_back(sValue, "type");

        OGRFieldSubType fld_subtype = OGR_Fld_GetSubType(hFieldDefn);
        sValue = getOFTSubtypeString_(fld_subtype);
        list_fld_defn.push_back(sValue, "subtype");

        nValue = OGR_Fld_GetWidth(hFieldDefn);
        list_fld_defn.push_back(nValue, "width");

        nValue = OGR_Fld_GetPrecision(hFieldDefn);
        list_fld_defn.push_back(nValue, "precision");

        bValue = OGR_Fld_IsNullable(hFieldDefn);
        list_fld_defn.push_back(bValue, "is_nullable");

        bValue = OGR_Fld_IsUnique(hFieldDefn);
        list_fld_defn.push_back(bValue, "is_unique");

        if (OGR_Fld_GetDefault(hFieldDefn) != nullptr)
            sValue = std::string(OGR_Fld_GetDefault(hFieldDefn));
        else
            sValue = "";
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

        OGRSpatialReferenceH hSRS = OGR_GFld_GetSpatialRef(hGeomFldDefn);
        if (hSRS == nullptr)
            Rcpp::stop("could not obtain geometry SRS");
        char *pszSRS_WKT = nullptr;
        if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE) {
            Rcpp::stop("error exporting geometry SRS to WKT");
        }
        sValue = std::string(pszSRS_WKT);
        list_geom_fld_defn.push_back(sValue, "srs");

        bValue = OGR_GFld_IsNullable(hGeomFldDefn);
        list_geom_fld_defn.push_back(bValue, "is_nullable");

        bValue = OGR_GFld_IsIgnored(hGeomFldDefn);
        list_geom_fld_defn.push_back(bValue, "is_ignored");

        bValue = true;
        list_geom_fld_defn.push_back(bValue, "is_geom");

        std::string geomFldName(OGR_GFld_GetNameRef(hGeomFldDefn));
        if (geomFldName == "")
            geomFldName = defaultGeomFldName;

        list_out.push_back(list_geom_fld_defn, geomFldName);

        CPLFree(pszSRS_WKT);
    }

    return list_out;
}

void GDALVector::setAttributeFilter(std::string query) {
    checkAccess_(GA_ReadOnly);

    const char* query_in = nullptr;
    if (query != "")
        query_in = query.c_str();

    if (OGR_L_SetAttributeFilter(hLayer, query_in) != OGRERR_NONE)
        Rcpp::stop("error setting filter, possibly in the query expression");
    else
        m_attr_filter = query;
}

void GDALVector::setSpatialFilterRect(Rcpp::NumericVector bbox) {
    checkAccess_(GA_ReadOnly);

    if (Rcpp::any(Rcpp::is_na(bbox)))
        Rcpp::stop("'bbox' has one or more 'NA' values");

    OGR_L_SetSpatialFilterRect(hLayer, bbox[0], bbox[1], bbox[2], bbox[3]);
}

void GDALVector::clearSpatialFilter() {
    checkAccess_(GA_ReadOnly);

    OGR_L_SetSpatialFilter(hLayer, nullptr);
}

double GDALVector::getFeatureCount() {
    // OGR_L_GetFeatureCount() returns GIntBig, return as R numeric for now
    // GDAL doc: Note that some implementations of this method may alter the
    // read cursor of the layer.
    // see: testCapability("FastFeatureCount")
    checkAccess_(GA_ReadOnly);

    return static_cast<double>(OGR_L_GetFeatureCount(hLayer, true));
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
        return df;
    }
}

SEXP GDALVector::getFeature(Rcpp::NumericVector fid) {
    // fid must be an R numeric vector of length 1
    // i.e., a scalar but use NumericVector here since it can carry the class
    // attribute for integer64
    checkAccess_(GA_ReadOnly);

    if (fid.size() != 1)
        Rcpp::stop("'fid' must be a length-1 numeric vector (integer64)");

    int64_t fid_in = -1;
    if (Rcpp::isInteger64(fid))
        fid_in = Rcpp::fromInteger64(fid[0]);
    else
        fid_in = static_cast<int64_t>(fid[0]);

    // save the current attribute and spatial filters
    std::string orig_filter = m_attr_filter;
    OGRGeometryH hOrigFilterGeom = nullptr;
    OGRGeometryH hFilterGeom = nullptr;
    hFilterGeom = OGR_L_GetSpatialFilter(hLayer);
    if (hFilterGeom != nullptr) {
        hOrigFilterGeom = OGR_G_Clone(hFilterGeom);
        hFilterGeom = nullptr;
    }

    // filter based on FID
    clearSpatialFilter();
    setAttributeFilter("FID = " + std::to_string(fid_in));

    Rcpp::DataFrame df = fetch(1);

    // restore original filters
    setAttributeFilter(orig_filter);
    OGR_L_SetSpatialFilter(hLayer, hOrigFilterGeom);
    if (hOrigFilterGeom != nullptr) {
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
        return df;
    }
}

void GDALVector::resetReading() {
    checkAccess_(GA_ReadOnly);

    OGR_L_ResetReading(hLayer);
}

Rcpp::DataFrame GDALVector::fetch(double n) {
    // Analog of DBI::dbFetch(), mostly following its specification:
    // https://dbi.r-dbi.org/reference/dbFetch.html#specification
    // n should be passed as a whole number (integer or numeric). A value of
    // Inf for the n argument is supported and also returns the full result.

    checkAccess_(GA_ReadOnly);

    OGRFeatureDefnH hFDefn = nullptr;
    hFDefn = OGR_L_GetLayerDefn(hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    bool fetch_all = true;
    size_t fetch_num = 0;
    if (n == -1 || (n > 0 && std::isinf(n))) {
        resetReading();
        fetch_num = OGR_L_GetFeatureCount(hLayer, true);
    }
    else if (Rcpp::NumericVector::is_na(n)) {
        fetch_num = OGR_L_GetFeatureCount(hLayer, true);
    }
    else if (n >= 0 && std::isfinite(n)) {
        if (n > 9007199254740992)
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

    OGRFeatureH hFeat = nullptr;
    size_t row_num = 0;
    int nFields = OGR_FD_GetFieldCount(hFDefn);
    int nGeomFields = OGR_FD_GetGeomFieldCount(hFDefn);
    bool include_geom = true;
    if (EQUAL(returnGeomAs.c_str(), "NONE")) {
        include_geom = false;
    }
    else if (!(EQUAL(returnGeomAs.c_str(), "WKB") ||
               EQUAL(returnGeomAs.c_str(), "WKB_ISO") ||
               EQUAL(returnGeomAs.c_str(), "WKT") ||
               EQUAL(returnGeomAs.c_str(), "WKT_ISO") ||
               EQUAL(returnGeomAs.c_str(), "TYPE_NAME"))) {
        Rcpp::stop("unrecognized value of field 'returnGeomAs'");
    }

    OGRwkbByteOrder eOrder;
    if (EQUAL(wkbByteOrder.c_str(), "LSB"))
        eOrder = wkbNDR;
    else if (EQUAL(wkbByteOrder.c_str(), "MSB"))
        eOrder = wkbXDR;
    else
        Rcpp::stop("invalid value of field 'wkbByteOrder'");

    while ((hFeat = OGR_L_GetNextFeature(hLayer)) != nullptr) {
        const int64_t fid = static_cast<int64_t>(OGR_F_GetFID(hFeat));
        Rcpp::NumericVector fid_col = df[0];
        fid_col[row_num] = Rcpp::toInteger64(fid)[0];

        for (int i = 0; i < nFields; ++i) {
            OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, i);
            if (hFieldDefn == nullptr)
                Rcpp::stop("could not obtain field definition");

            bool has_value = true;
            if (!OGR_F_IsFieldSet(hFeat, i) || OGR_F_IsFieldNull(hFeat, i))
                has_value = false;

            OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);

            if (fld_type == OFTInteger && has_value) {
                OGRFieldSubType fld_subtype = OGR_Fld_GetSubType(hFieldDefn);
                if (fld_subtype == OFSTBoolean) {
                    Rcpp::LogicalVector col = df[i + 1];
                    col[row_num] = OGR_F_GetFieldAsInteger(hFeat, i);
                }
                else {
                    Rcpp::IntegerVector col = df[i + 1];
                    col[row_num] = OGR_F_GetFieldAsInteger(hFeat, i);
                }
            }
            else if (fld_type == OFTInteger64 && has_value) {
                const int64_t value = static_cast<int64_t>(
                        OGR_F_GetFieldAsInteger64(hFeat, i));

                OGRFieldSubType fld_subtype = OGR_Fld_GetSubType(hFieldDefn);
                if (fld_subtype == OFSTBoolean) {
                    Rcpp::LogicalVector col = df[i + 1];
                    col[row_num] = Rcpp::toInteger64(value)[0];
                }
                else {
                    Rcpp::NumericVector col = df[i + 1];
                    col[row_num] = Rcpp::toInteger64(value)[0];
                }
            }
            else if (fld_type == OFTReal && has_value) {
                Rcpp::NumericVector col = df[i + 1];
                col[row_num] = OGR_F_GetFieldAsDouble(hFeat, i);
            }
            else if ((fld_type == OFTDate || fld_type == OFTDateTime)
                        && has_value) {

                int yr = 9999;
                int mo, day = 9;
                int hr, min, sec, tzflag = 0;
                if (OGR_F_GetFieldAsDateTime(hFeat, i, &yr, &mo, &day,
                                             &hr, &min, &sec, &tzflag)) {

                    struct tm brokendowntime;
                    brokendowntime.tm_year = yr - 1900;
                    brokendowntime.tm_mon = mo - 1;
                    brokendowntime.tm_mday = day;
                    brokendowntime.tm_hour = hr;
                    brokendowntime.tm_min = min;
                    brokendowntime.tm_sec = sec;
                    int64_t nUnixTime = CPLYMDHMSToUnixTime(&brokendowntime);
                    Rcpp::NumericVector col = df[i + 1];
                    if (fld_type == OFTDate) {
                        const int64_t nUnixTime_days = nUnixTime / 86400;
                        col[row_num] = static_cast<double>(nUnixTime_days);
                    }
                    else {
                        // OFTDateTime
                        if (tzflag > 1 && tzflag != 100) {
                            // convert to GMT
                            const int tzoffset = std::abs(tzflag - 100) * 15;
                            const int tzhour = tzoffset / 60;
                            const int tzmin = tzoffset - tzhour * 60;
                            const int offset = tzhour * 3600 + tzmin * 60;
                            if (tzflag >= 100)
                                nUnixTime -= offset;
                            else
                                nUnixTime += offset;
                        }
                        col[row_num] = static_cast<double>(nUnixTime);
                    }
                }
            }
            else if (fld_type == OFTBinary) {
                Rcpp::List col = df[i + 1];
                if (has_value) {
                    int nDataSize = 0;
                    GByte *pabyData = OGR_F_GetFieldAsBinary(hFeat, i, &nDataSize);
                    if (nDataSize > 0) {
                        Rcpp::RawVector blob(nDataSize);
                        std::memcpy(&blob[0], pabyData, nDataSize);
                        col[row_num] = blob;
                    }
                    else {
                        col[row_num] = Rcpp::RawVector::create();
                    }
                }
                else {
                    col[row_num] = Rcpp::RawVector::create();
                }
            }
            else if (fld_type == OFTIntegerList) {
                Rcpp::List col = df[i + 1];
                if (has_value) {
                    int nCount = 0;
                    const int *panValue = OGR_F_GetFieldAsIntegerList(hFeat, i,
                                                                      &nCount);
                    if (nCount > 0) {
                        std::vector<int> v(panValue, panValue + nCount);
                        col[row_num] = Rcpp::wrap(v);
                    }
                    else {
                        col[row_num] = Rcpp::IntegerVector::create();
                    }
                }
                else {
                    col[row_num] = NA_INTEGER;
                }
            }
            else if (fld_type == OFTInteger64List) {
                Rcpp::List col = df[i + 1];
                if (has_value) {
                    int nCount = 0;
                    const int64_t *panValue = reinterpret_cast<const int64_t *>(
                            OGR_F_GetFieldAsInteger64List(hFeat, i, &nCount));

                    if (nCount > 0) {
                        std::vector<int64_t> v(panValue, panValue + nCount);
                        col[row_num] = Rcpp::wrap(v);
                    }
                    else {
                        Rcpp::NumericVector v = Rcpp::NumericVector::create();
                        v.attr("class") = "integer64";
                        col[row_num] = v;
                    }
                }
                else {
                    std::vector<int64_t> v(1);
                    v[0] = NA_INTEGER64;
                    col[row_num] = Rcpp::wrap(v);
                }
            }
            else if (fld_type == OFTRealList) {
                Rcpp::List col = df[i + 1];
                if (has_value) {
                    int nCount = 0;
                    const double *padfValue =
                            OGR_F_GetFieldAsDoubleList(hFeat, i, &nCount);

                    if (nCount > 0) {
                        std::vector<double> v(padfValue, padfValue + nCount);
                        col[row_num] = Rcpp::wrap(v);
                    }
                    else {
                        col[row_num] = Rcpp::NumericVector::create();
                    }
                }
                else {
                    col[row_num] = NA_REAL;
                }
            }
            else if (fld_type == OFTStringList) {
                Rcpp::List col = df[i + 1];
                if (has_value) {
                    char **papszValue = OGR_F_GetFieldAsStringList(hFeat, i);
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
                else {
                    col[row_num] = NA_STRING;
                }
            }
            else {
                // use string
                if (has_value) {
                    Rcpp::CharacterVector col = df[i + 1];
                    col[row_num] = OGR_F_GetFieldAsString(hFeat, i);
                }
            }
        }

        if (include_geom) {
            for (int i = 0; i < nGeomFields; ++i) {
                OGRGeomFieldDefnH hGeomFldDefn =
                        OGR_F_GetGeomFieldDefnRef(hFeat, i);
                if (hGeomFldDefn == nullptr)
                    Rcpp::stop("could not obtain geometry field def");

                if (STARTS_WITH_CI(returnGeomAs.c_str(), "WKB")) {
                    OGRGeometryH hGeom = OGR_F_GetGeomFieldRef(hFeat, i);
                    if (hGeom != nullptr) {
#if GDAL_VERSION_NUM >= 3030000
                        const int nWKBSize = OGR_G_WkbSizeEx(hGeom);
#else
                        const int nWKBSize = OGR_G_WkbSize(hGeom);
#endif
                        if (nWKBSize) {
                            Rcpp::RawVector wkb(nWKBSize);
                            if (EQUAL(returnGeomAs.c_str(), "WKB"))
                                OGR_G_ExportToWkb(hGeom, eOrder, &wkb[0]);
                            else if (EQUAL(returnGeomAs.c_str(), "WKB_ISO"))
                                OGR_G_ExportToIsoWkb(hGeom, eOrder, &wkb[0]);

                            Rcpp::List col = df[nFields + 1 + i];
                            col[row_num] = wkb;
                        }
                        else {
                            Rcpp::List col = df[nFields + 1 + i];
                            col[row_num] = Rcpp::RawVector::create();
                        }
                    }
                }
                else if (STARTS_WITH_CI(returnGeomAs.c_str(), "WKT")) {
                    Rcpp::CharacterVector col = df[nFields + 1 + i];
                    OGRGeometryH hGeom = OGR_F_GetGeomFieldRef(hFeat, i);
                    if (hGeom != nullptr) {
                        char* pszWKT;
                        if (EQUAL(returnGeomAs.c_str(), "WKT"))
                            OGR_G_ExportToWkt(hGeom, &pszWKT);
                        else if (EQUAL(returnGeomAs.c_str(), "WKT_ISO"))
                            OGR_G_ExportToIsoWkt(hGeom, &pszWKT);

                        col[row_num] = pszWKT;
                        CPLFree(pszWKT);
                    }
                    else {
                        col[row_num] = NA_STRING;
                    }
                }
                else if (EQUAL(returnGeomAs.c_str(), "TYPE_NAME")) {
                    OGRGeometryH hGeom = OGR_F_GetGeomFieldRef(hFeat, i);
                    Rcpp::CharacterVector col = df[nFields + 1 + i];
                    if (hGeom != nullptr)
                        col[row_num] = OGR_G_GetGeometryName(hGeom);
                    else
                        col[row_num] = NA_STRING;
                }
            }
        }

        OGR_F_Destroy(hFeat);
        hFeat = nullptr;

        row_num += 1;
        if (row_num == fetch_num)
            break;
    }

    if (fetch_all) {
        hFeat = OGR_L_GetNextFeature(hLayer);
        if (hFeat != nullptr) {
            Rcpp::Rcout << "getFeatureCount() reported " << row_num
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

        Rcpp::NumericVector fid_col = df[0];
        Rcpp::NumericVector fid_col_trunc = df_trunc[0];
        std::copy_n(fid_col.cbegin(), row_num, fid_col_trunc.begin());

        for (int i = 0; i < OGR_FD_GetFieldCount(hFDefn); ++i) {
            OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, i);
            OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);

            if (fld_type == OFTInteger) {
                OGRFieldSubType fld_subtype = OGR_Fld_GetSubType(hFieldDefn);
                if (fld_subtype == OFSTBoolean) {
                    Rcpp::LogicalVector col = df[i + 1];
                    Rcpp::LogicalVector col_trunc = df_trunc[i + 1];
                    std::copy_n(col.cbegin(), row_num, col_trunc.begin());
                }
                else {
                    Rcpp::IntegerVector col = df[i + 1];
                    Rcpp::IntegerVector col_trunc = df_trunc[i + 1];
                    std::copy_n(col.cbegin(), row_num, col_trunc.begin());
                }
            }
            else if (fld_type == OFTInteger64 || fld_type == OFTReal ||
                     fld_type == OFTDate || fld_type == OFTDateTime) {

                Rcpp::NumericVector col = df[i + 1];
                Rcpp::NumericVector col_trunc = df_trunc[i + 1];
                std::copy_n(col.cbegin(), row_num, col_trunc.begin());
            }
            else if (fld_type == OFTBinary || fld_type == OFTIntegerList ||
                     fld_type == OFTInteger64List || fld_type == OFTRealList ||
                     fld_type == OFTStringList) {

                Rcpp::List col = df[i + 1];
                Rcpp::List col_trunc = df_trunc[i + 1];
                for (size_t n = 0; n < row_num; ++n)
                    col_trunc[n] = col[n];
            }
            else {
                Rcpp::CharacterVector col = df[i + 1];
                Rcpp::CharacterVector col_trunc = df_trunc[i + 1];
                std::copy_n(col.cbegin(), row_num, col_trunc.begin());
            }
        }

        if (include_geom) {
            for (int i = 0; i < nGeomFields; ++i) {
                if (STARTS_WITH_CI(returnGeomAs.c_str(), "WKB")) {
                    Rcpp::List col = df[nFields + i + 1];
                    Rcpp::List col_trunc = df_trunc[nFields + i + 1];
                    for (size_t n = 0; n < row_num; ++n)
                        col_trunc[n] = col[n];
                }
                else {
                    Rcpp::CharacterVector col = df[nFields + i + 1];
                    Rcpp::CharacterVector col_trunc = df_trunc[nFields + i + 1];
                    std::copy_n(col.cbegin(), row_num, col_trunc.begin());
                }
            }
        }

        return df_trunc;
    }
}

void GDALVector::layerIntersection(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        Rcpp::Nullable<Rcpp::CharacterVector> options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    OGRErr err = OGR_L_Intersection(
                    hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err != OGRERR_NONE)
        Rcpp::stop("error during Intersection, or execution was interrupted");
}

void GDALVector::layerUnion(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        Rcpp::Nullable<Rcpp::CharacterVector> options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    OGRErr err = OGR_L_Union(
                    hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err != OGRERR_NONE)
        Rcpp::stop("error during Union, or execution was interrupted");
}

void GDALVector::layerSymDifference(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        Rcpp::Nullable<Rcpp::CharacterVector> options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    OGRErr err = OGR_L_SymDifference(
                    hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err != OGRERR_NONE)
        Rcpp::stop("error during SymDifference, or execution was interrupted");
}

void GDALVector::layerIdentity(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        Rcpp::Nullable<Rcpp::CharacterVector> options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    OGRErr err = OGR_L_Identity(
                    hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err != OGRERR_NONE)
        Rcpp::stop("error during Identity, or execution was interrupted");
}

void GDALVector::layerUpdate(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        Rcpp::Nullable<Rcpp::CharacterVector> options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    OGRErr err = OGR_L_Update(
                    hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err != OGRERR_NONE)
        Rcpp::stop("error during Update, or execution was interrupted");
}

void GDALVector::layerClip(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        Rcpp::Nullable<Rcpp::CharacterVector> options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    OGRErr err = OGR_L_Clip(
                    hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err != OGRERR_NONE)
        Rcpp::stop("error during Clip, or execution was interrupted");
}

void GDALVector::layerErase(
        GDALVector method_layer,
        GDALVector result_layer,
        bool quiet,
        Rcpp::Nullable<Rcpp::CharacterVector> options) {

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    OGRErr err = OGR_L_Erase(
                    hLayer,
                    method_layer.getOGRLayerH_(),
                    result_layer.getOGRLayerH_(),
                    opt_list.data(),
                    quiet ? nullptr : GDALTermProgressR,
                    nullptr);

    if (err != OGRERR_NONE)
        Rcpp::stop("error during Erase, or execution was interrupted");
}

void GDALVector::close() {
    if (hDataset != nullptr) {
        if (is_sql_in)
            GDALDatasetReleaseResultSet(hDataset, hLayer);
        GDALReleaseDataset(hDataset);
        hDataset = nullptr;
        hLayer = nullptr;
    }
}

// ****************************************************************************
// class methods for internal use not exposed in R
// ****************************************************************************

void GDALVector::checkAccess_(GDALAccess access_needed) const {
    if (!isOpen())
        Rcpp::stop("dataset is not open");

    if (access_needed == GA_Update && eAccess == GA_ReadOnly)
        Rcpp::stop("dataset is read-only");
}

OGRLayerH GDALVector::getOGRLayerH_() const {
    checkAccess_(GA_ReadOnly);

    return hLayer;
}

SEXP GDALVector::initDF_(R_xlen_t nrow) const {
    // initialize a data frame with nrow rows for the layer definition
    OGRFeatureDefnH hFDefn = nullptr;
    hFDefn = OGR_L_GetLayerDefn(hLayer);
    if (hFDefn == nullptr)
        Rcpp::stop("failed to get layer definition");

    int nFields = OGR_FD_GetFieldCount(hFDefn);
    int nGeomFields = 0;
    if (!EQUAL(returnGeomAs.c_str(), "NONE"))
        nGeomFields = OGR_FD_GetGeomFieldCount(hFDefn);

    // construct the data frame as list and convert at return
    Rcpp::List df(1 + nFields + nGeomFields);
    Rcpp::CharacterVector col_names(1 + nFields + nGeomFields);

    std::vector<int64_t> fid(nrow, NA_INTEGER64);
    df[0] = Rcpp::wrap(fid);
    col_names[0] = "FID";

    for (int i = 0; i < nFields; ++i) {
        OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, i);
        if (hFieldDefn == nullptr)
            Rcpp::stop("could not obtain field definition");

        OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);

        if (fld_type == OFTInteger) {
            OGRFieldSubType fld_subtype = OGR_Fld_GetSubType(hFieldDefn);
            if (fld_subtype == OFSTBoolean) {
                Rcpp::LogicalVector v(nrow, NA_LOGICAL);
                df[i + 1] = v;
            }
            else {
                Rcpp::IntegerVector v(nrow, NA_INTEGER);
                df[i + 1] = v;
            }
            col_names[i + 1] = OGR_Fld_GetNameRef(hFieldDefn);
        }
        else if (fld_type == OFTInteger64) {
            OGRFieldSubType fld_subtype = OGR_Fld_GetSubType(hFieldDefn);
            if (fld_subtype == OFSTBoolean) {
                Rcpp::LogicalVector v(nrow, NA_LOGICAL);
                df[i + 1] = v;
            }
            else {
                std::vector<int64_t> v(nrow, NA_INTEGER64);
                df[i + 1] = Rcpp::wrap(v);
            }
            col_names[i + 1] = OGR_Fld_GetNameRef(hFieldDefn);
        }
        else if (fld_type == OFTReal) {
            Rcpp::NumericVector v(nrow, NA_REAL);
            df[i + 1] = v;
            col_names[i + 1] = OGR_Fld_GetNameRef(hFieldDefn);
        }
        else if (fld_type == OFTDate) {
            Rcpp::NumericVector v(nrow, NA_REAL);
            v.attr("class") = "Date";
            df[i + 1] = v;
            col_names[i + 1] = OGR_Fld_GetNameRef(hFieldDefn);
        }
        else if (fld_type == OFTDateTime) {
            Rcpp::NumericVector v(nrow, NA_REAL);
            Rcpp::CharacterVector class_names = {"POSIXt", "POSIXct"};
            v.attr("class") = class_names;
            v.attr("tzone") = "UTC";
            df[i + 1] = v;
            col_names[i + 1] = OGR_Fld_GetNameRef(hFieldDefn);
        }
        else if (fld_type == OFTBinary || fld_type == OFTIntegerList ||
                 fld_type == OFTInteger64List || fld_type == OFTRealList ||
                 fld_type == OFTStringList) {

            Rcpp::List v(nrow);
            df[i + 1] = v;
            col_names[i + 1] = OGR_Fld_GetNameRef(hFieldDefn);
        }
        else {
            // use string
            Rcpp::CharacterVector v(nrow, NA_STRING);
            df[i + 1] = v;
            col_names[i + 1] = OGR_Fld_GetNameRef(hFieldDefn);
        }
    }

    for (int i = 0; i < nGeomFields; ++i) {
        OGRGeomFieldDefnH hGeomFldDefn = OGR_FD_GetGeomFieldDefn(hFDefn, i);
        if (hGeomFldDefn == nullptr)
            Rcpp::stop("could not obtain geometry field def");

        if (STARTS_WITH_CI(returnGeomAs.c_str(), "WKB")) {
            Rcpp::List v(nrow);
            df[i + 1 + nFields] = v;
        }
        else {
            Rcpp::CharacterVector v(nrow, NA_STRING);
            df[i + 1 + nFields] = v;
        }

        std::string geomFldName(OGR_GFld_GetNameRef(hGeomFldDefn));
        if (geomFldName == "")
            geomFldName = defaultGeomFldName;

        col_names[i + 1 + nFields] = geomFldName;
    }

    df.names() = col_names;
    df.attr("class") = "data.frame";
    df.attr("row.names") = Rcpp::seq_len(nrow);
    return df;
}

// ****************************************************************************

RCPP_MODULE(mod_GDALVector) {
    Rcpp::class_<GDALVector>("GDALVector")

    .constructor
        ("Default constructor, only for allocations in std::vector")
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

    // exposed read/write fields
    .field("defaultGeomFldName", &GDALVector::defaultGeomFldName)
    .field("returnGeomAs", &GDALVector::returnGeomAs)
    .field("wkbByteOrder", &GDALVector::wkbByteOrder)

    // exposed member functions
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
    .method("setSpatialFilterRect", &GDALVector::setSpatialFilterRect,
        "Set a new rectangular spatial filter")
    .method("clearSpatialFilter", &GDALVector::clearSpatialFilter,
        "Clear the current spatial filter")
    .method("getFeatureCount", &GDALVector::getFeatureCount,
        "Fetch the feature count in this layer")
    .method("getNextFeature", &GDALVector::getNextFeature,
        "Fetch the next available feature from this layer")
    .method("getFeature", &GDALVector::getFeature,
        "Fetch a feature by its identifier")
    .method("resetReading", &GDALVector::resetReading,
        "Reset feature reading to start on the first feature")
    .method("fetch", &GDALVector::fetch,
        "Fetch a set features as a data frame")
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

    ;
}
