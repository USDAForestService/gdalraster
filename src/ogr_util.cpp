/* Utility functions for vector data sources
   Chris Toney <chris.toney at usda.gov> */

#include "gdal.h"
#include "cpl_error.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "ogr_srs_api.h"

#include "ogr_util.h"
#include "gdalraster.h"

// Internal lookup of OGRwkbGeometryType by string descriptor
// Returns wkbUnknown if no match
OGRwkbGeometryType _getWkbGeomType(std::string geom_type) {
    std::string geom_type_in = str_toupper(geom_type);
    if (auto it = MAP_OGR_GEOM_TYPE.find(geom_type_in);
            it != MAP_OGR_GEOM_TYPE.end())
        return it->second;
    else
        return wkbUnknown;
}

// Internal lookup of geometry type string by OGRwkbGeometryType
// Returns "UNKNOWN" if no match
std::string _getWkbGeomString(OGRwkbGeometryType eType) {
    for (auto it = MAP_OGR_GEOM_TYPE.begin();
            it != MAP_OGR_GEOM_TYPE.end(); ++it) {
        if (it->second == eType) {
            return it->first;
        }
    }
    return "UNKNOWN";
}

// Internal lookup of OGRFieldType by string descriptor
// Error if no match
OGRFieldType _getOFT(std::string fld_type) {
    if (auto it = MAP_OGR_FLD_TYPE.find(fld_type);
            it != MAP_OGR_FLD_TYPE.end())
        return it->second;
    else
        Rcpp::stop("unrecognized OGR field type descriptor");
}

// Internal lookup of OGR field type string by OGRFieldType
// Returns empty string if no match, with warning emitted
std::string _getOFTString(OGRFieldType eType) {
    for (auto it = MAP_OGR_FLD_TYPE.begin();
            it != MAP_OGR_FLD_TYPE.end(); ++it) {
        if (it->second == eType) {
            return it->first;
        }
    }
    Rcpp::warning("unrecognized OGRFieldType enumerator");
    return "";
}

// Internal lookup of OGRFieldSubType by string descriptor
// Returns OFSTNone if no match
OGRFieldSubType _getOFTSubtype(std::string fld_subtype) {
    if (auto it = MAP_OGR_FLD_SUBTYPE.find(fld_subtype);
            it != MAP_OGR_FLD_SUBTYPE.end())
        return it->second;
    else
        return OFSTNone;
}

// Internal lookup of OGR field subtype string by OGRFieldSubType
// Returns "OFSTNone" if no match
std::string _getOFTSubtypeString(OGRFieldSubType eType) {
    for (auto it = MAP_OGR_FLD_SUBTYPE.begin();
            it != MAP_OGR_FLD_SUBTYPE.end(); ++it) {
        if (it->second == eType) {
            return it->first;
        }
    }
    return "OFSTNone";
}

//' Does vector dataset exist
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_exists")]]
bool _ogr_ds_exists(std::string dsn, bool with_update = false) {

    GDALDatasetH hDS = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    if (with_update)
        hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                         nullptr, nullptr, nullptr);
    else
        hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR,
                         nullptr, nullptr, nullptr);
    CPLPopErrorHandler();

    if (hDS != nullptr) {
        GDALReleaseDataset(hDS);
        return true;
    }
    else {
        return false;
    }
}

//' Create a vector dataset. Optionally create a layer in the dataset.
//' A field is also created optionally, currently hard coded as type OFTInteger.
//'
//' @noRd
// [[Rcpp::export(name = ".create_ogr")]]
bool _create_ogr(std::string format, std::string dst_filename,
        int xsize, int ysize, int nbands, std::string dataType,
        std::string layer, std::string geom_type,
        std::string srs = "", std::string fld_name = "",
        Rcpp::Nullable<Rcpp::CharacterVector> dsco = R_NilValue,
        Rcpp::Nullable<Rcpp::CharacterVector> lco = R_NilValue) {

    GDALDriverH hDriver = GDALGetDriverByName(format.c_str());
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver for the specified format");

    std::string dsn_in = Rcpp::as<std::string>(
            _check_gdal_filename(dst_filename));

    char **papszMetadata = GDALGetMetadata(hDriver, nullptr);
    if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATE, FALSE))
        Rcpp::stop("driver does not support create");

    OGRwkbGeometryType eGeomType = _getWkbGeomType(geom_type);
    if (eGeomType == wkbUnknown)
        Rcpp::stop("'geom_type' is unknown");

    GDALDataType dt = GDALGetDataTypeByName(dataType.c_str());

    std::vector<char *> opt_list = {nullptr};
    if (dsco.isNotNull()) {
        Rcpp::CharacterVector dsco_in(dsco);
        opt_list.resize(dsco_in.size() + 1);
        for (R_xlen_t i = 0; i < dsco_in.size(); ++i) {
            opt_list[i] = (char *) (dsco_in[i]);
        }
        opt_list[dsco_in.size()] = nullptr;
    }

    GDALDatasetH hDstDS = nullptr;
    hDstDS = GDALCreate(hDriver, dsn_in.c_str(),
                        xsize, ysize, nbands, dt,
                        opt_list.data());

    if (hDstDS == nullptr)
        return false;

    if (layer == "") {
        GDALReleaseDataset(hDstDS);
        return true;
    }

    if (!GDALDatasetTestCapability(hDstDS, ODsCCreateLayer)) {
        GDALReleaseDataset(hDstDS);
        return false;
    }

    OGRLayerH  hLayer = nullptr;
    OGRFieldDefnH hFieldDefn = nullptr;
    bool layer_ok = false;
    bool fld_ok = false;

    opt_list.clear();
    if (lco.isNotNull()) {
        Rcpp::CharacterVector lco_in(lco);
        opt_list.resize(lco_in.size() + 1);
        for (R_xlen_t i = 0; i < lco_in.size(); ++i) {
            opt_list[i] = (char *) (lco_in[i]);
        }
    }
    opt_list.push_back(nullptr);

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (srs != "") {
        if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
            if (hSRS != nullptr)
                OSRDestroySpatialReference(hSRS);
            GDALReleaseDataset(hDstDS);
            Rcpp::stop("error importing SRS from user input");
        }
    }

    hLayer = GDALDatasetCreateLayer(hDstDS, layer.c_str(), hSRS, eGeomType,
                                    opt_list.data());

    if (hLayer != nullptr) {
        layer_ok = true;
        if (fld_name != "") {
            hFieldDefn = OGR_Fld_Create(fld_name.c_str(), OFTInteger);
            if (hFieldDefn == nullptr)
                fld_ok = false;
            else if (OGR_L_CreateField(hLayer, hFieldDefn, true) != OGRERR_NONE)
                fld_ok = false;
            else
                fld_ok = true;

            if (hFieldDefn != nullptr)
                OGR_Fld_Destroy(hFieldDefn);
        }
        else {
            fld_ok = true;
        }
    }

    if (hSRS != nullptr)
        OSRDestroySpatialReference(hSRS);

    GDALReleaseDataset(hDstDS);

    if (layer_ok && fld_ok)
        return true;
    else
        return false;
}

//' Get number of layers in a dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_layer_count")]]
int _ogr_ds_layer_count(std::string dsn) {

    GDALDatasetH hDS = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr)
        return -1;
    CPLPopErrorHandler();

    int cnt = GDALDatasetGetLayerCount(hDS);
    GDALReleaseDataset(hDS);
    return cnt;
}

//' Does layer exist
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_exists")]]
bool _ogr_layer_exists(std::string dsn, std::string layer) {

    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    bool ret = false;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr)
        return false;
    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    CPLPopErrorHandler();

    if (hLayer != nullptr)
        ret = true;

    GDALReleaseDataset(hDS);
    return ret;
}

//' Create a layer in a vector dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_create")]]
bool _ogr_layer_create(std::string dsn, std::string layer,
        std::string geom_type, std::string srs = "",
        Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;

    OGRwkbGeometryType eGeomType = _getWkbGeomType(geom_type);
    if (eGeomType == wkbUnknown && !EQUALN(geom_type.c_str(), "UNKNOWN", 7))
        Rcpp::stop("'geom_type' not recognized");

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (srs != "") {
        if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
            if (hSRS != nullptr)
                OSRDestroySpatialReference(hSRS);
            Rcpp::stop("error importing SRS from user input");
        }
    }

    hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);

    if (hDS == nullptr) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        return false;
    }

    if (!GDALDatasetTestCapability(hDS, ODsCCreateLayer)) {
        GDALReleaseDataset(hDS);
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        return false;
    }

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;
    }

    hLayer = GDALDatasetCreateLayer(hDS, layer.c_str(), hSRS, eGeomType,
                                    opt_list.data());

    bool ret = false;
    if (hLayer != nullptr)
        ret = true;

    if (hSRS != nullptr)
        OSRDestroySpatialReference(hSRS);

    GDALReleaseDataset(hDS);

    return ret;
}

//' Delete a layer in a vector dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_delete")]]
bool _ogr_layer_delete(std::string dsn, std::string layer) {

    GDALDatasetH hDS = nullptr;
    OGRLayerH  hLayer = nullptr;
    int layer_cnt, layer_idx;

    hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);

    if (hDS == nullptr)
        return false;

    if (!GDALDatasetTestCapability(hDS, ODsCDeleteLayer)) {
        GDALReleaseDataset(hDS);
        return false;
    }

    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    if (hLayer == nullptr) {
        GDALReleaseDataset(hDS);
        return false;
    }

    layer_cnt = GDALDatasetGetLayerCount(hDS);
    for (layer_idx=0; layer_idx < layer_cnt; ++layer_idx) {
        hLayer = GDALDatasetGetLayer(hDS, layer_idx);
        if (EQUAL(OGR_L_GetName(hLayer), layer.c_str()))
            break;
    }

    bool ret = false;
    if (GDALDatasetDeleteLayer(hDS, layer_idx) == OGRERR_NONE)
        ret = true;

    GDALReleaseDataset(hDS);
    return ret;
}

//' Get field index or -1 if fld_name not found
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_field_index")]]
int _ogr_field_index(std::string dsn, std::string layer,
        std::string fld_name) {

    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;
    int iField;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr)
        return -1;
    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    CPLPopErrorHandler();

    if (hLayer == nullptr) {
        GDALReleaseDataset(hDS);
        return -1;
    }

    hFDefn = OGR_L_GetLayerDefn(hLayer);
    if (hFDefn != nullptr)
        iField = OGR_FD_GetFieldIndex(hFDefn, fld_name.c_str());
    else
        iField = -1;

    GDALReleaseDataset(hDS);
    return iField;
}

//' Create a new field on layer
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_field_create")]]
bool _ogr_field_create(std::string dsn, std::string layer,
                       std::string fld_name, std::string fld_type,
                       std::string fld_subtype = "OFSTNone",
                       int fld_width = 0,
                       int fld_precision = 0,
                       bool is_nullable = true,
                       bool is_ignored = false,
                       bool is_unique = false,
                       std::string default_value = "") {

    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;
    int iField;
    OGRFieldDefnH hFieldDefn = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
            nullptr, nullptr, nullptr);

    if (hDS == nullptr)
        return false;

    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    CPLPopErrorHandler();

    if (hLayer == nullptr) {
        GDALReleaseDataset(hDS);
        return false;
    }

    hFDefn = OGR_L_GetLayerDefn(hLayer);
    if (hFDefn != nullptr) {
        iField = OGR_FD_GetFieldIndex(hFDefn, fld_name.c_str());
    }
    else {
        GDALReleaseDataset(hDS);
        return false;
    }

    if (iField >= 0) {
        // fld_name already exists
        GDALReleaseDataset(hDS);
        return false;
    }

    OGRFieldType eFieldType = _getOFT(fld_type);
    OGRFieldSubType eFieldSubType = _getOFTSubtype(fld_subtype);
    bool ret = false;

    hFieldDefn = OGR_Fld_Create(fld_name.c_str(), eFieldType);
    if (hFieldDefn != nullptr) {
        OGR_Fld_SetSubType(hFieldDefn, eFieldSubType);
        if (fld_width > 0)
            OGR_Fld_SetWidth(hFieldDefn, fld_width);
        if (fld_precision > 0)
            OGR_Fld_SetPrecision(hFieldDefn, fld_precision);
        if (!is_nullable)
            OGR_Fld_SetNullable(hFieldDefn, false);
        if (is_ignored)
            OGR_Fld_SetIgnored(hFieldDefn, true);
        if (default_value != "")
            OGR_Fld_SetDefault(hFieldDefn, default_value.c_str());
#if GDAL_VERSION_NUM >= 3020000
        if (is_unique)
            OGR_Fld_SetUnique(hFieldDefn, true);
#endif

        if (OGR_L_CreateField(hLayer, hFieldDefn, TRUE) == OGRERR_NONE)
            ret = true;

        OGR_Fld_Destroy(hFieldDefn);
    }

    GDALReleaseDataset(hDS);
    return ret;
}

//' Create a new geom field on layer
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_geom_field_create")]]
bool _ogr_geom_field_create(std::string dsn, std::string layer,
                            std::string fld_name, std::string geom_type,
                            std::string srs = "",
                            bool is_nullable = true,
                            bool is_ignored = false) {

    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;
    int iField;
    OGRGeomFieldDefnH hGeomFieldDefn = nullptr;

    OGRwkbGeometryType eGeomType = _getWkbGeomType(geom_type);
    if (eGeomType == wkbUnknown && !EQUALN(geom_type.c_str(), "UNKNOWN", 7))
        Rcpp::stop("'geom_type' not recognized");

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (srs != "") {
        if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
            if (hSRS != nullptr)
                OSRDestroySpatialReference(hSRS);
            Rcpp::stop("error importing SRS from user input");
        }
    }

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
            nullptr, nullptr, nullptr);

    if (hDS == nullptr)
        return false;

    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    CPLPopErrorHandler();

    if (hLayer == nullptr) {
        GDALReleaseDataset(hDS);
        return false;
    }

    hFDefn = OGR_L_GetLayerDefn(hLayer);
    if (hFDefn != nullptr) {
        iField = OGR_FD_GetFieldIndex(hFDefn, fld_name.c_str());
    }
    else {
        GDALReleaseDataset(hDS);
        return false;
    }

    if (iField >= 0) {
        // fld_name already exists
        GDALReleaseDataset(hDS);
        return false;
    }

    bool ret = false;

    hGeomFieldDefn = OGR_GFld_Create(fld_name.c_str(), eGeomType);
    if (hGeomFieldDefn != nullptr) {
        if (!is_nullable)
            OGR_GFld_SetNullable(hGeomFieldDefn, false);
        if (is_ignored)
            OGR_GFld_SetIgnored(hGeomFieldDefn, true);
        if (hSRS != nullptr)
            OGR_GFld_SetSpatialRef(hGeomFieldDefn, hSRS);

        if (OGR_L_CreateGeomField(hLayer, hGeomFieldDefn, TRUE) == OGRERR_NONE)
            ret = true;

        OGR_GFld_Destroy(hGeomFieldDefn);
    }

    if (hSRS != nullptr)
        OSRDestroySpatialReference(hSRS);

    GDALReleaseDataset(hDS);
    return ret;
}
