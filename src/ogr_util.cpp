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
    std::string geom_type_in = _str_toupper(geom_type);
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

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    if (with_update)
        hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                         nullptr, nullptr, nullptr);
    else
        hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR,
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

//' Test if capabilities are available for a vector dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_test_cap")]]
SEXP _ogr_ds_test_cap(std::string dsn, bool with_update = true) {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    if (with_update)
        hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                         nullptr, nullptr, nullptr);
    else
        hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR,
                         nullptr, nullptr, nullptr);
    CPLPopErrorHandler();

    if (hDS == nullptr)
        return R_NilValue;

    Rcpp::List cap = Rcpp::List::create(
        Rcpp::Named("CreateLayer") = static_cast<bool>(
            GDALDatasetTestCapability(hDS, ODsCCreateLayer)),
        Rcpp::Named("DeleteLayer") = static_cast<bool>(
            GDALDatasetTestCapability(hDS, ODsCDeleteLayer)),
        Rcpp::Named("CreateGeomFieldAfterCreateLayer") = static_cast<bool>(
            GDALDatasetTestCapability(hDS,
                ODsCCreateGeomFieldAfterCreateLayer)),
        Rcpp::Named("CurveGeometries") = static_cast<bool>(
            GDALDatasetTestCapability(hDS, ODsCCurveGeometries)),
        Rcpp::Named("Transactions") = static_cast<bool>(
            GDALDatasetTestCapability(hDS, ODsCTransactions)),
        Rcpp::Named("EmulatedTransactions") = static_cast<bool>(
            GDALDatasetTestCapability(hDS, ODsCEmulatedTransactions)),
        Rcpp::Named("RandomLayerRead") = static_cast<bool>(
            GDALDatasetTestCapability(hDS, ODsCRandomLayerRead)),
        Rcpp::Named("RandomLayerWrite") = static_cast<bool>(
            GDALDatasetTestCapability(hDS, ODsCRandomLayerWrite)));

    return cap;
}

//' Create a vector dataset. Optionally create a layer in the dataset.
//' A field is also created optionally (name and type only).
//'
//' @noRd
// [[Rcpp::export(name = ".create_ogr")]]
bool _create_ogr(std::string format, std::string dst_filename,
        int xsize, int ysize, int nbands, std::string dataType,
        std::string layer, std::string geom_type,
        std::string srs = "", std::string fld_name = "",
        std::string fld_type = "OFTInteger",
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

    OGRwkbGeometryType eGeomType = wkbUnknown;
    if (geom_type != "") {
        eGeomType = _getWkbGeomType(geom_type);
        if (eGeomType == wkbUnknown)
            Rcpp::stop("'geom_type' is unknown");
    }

    if (fld_name != "" && fld_type == "")
        Rcpp::stop("'fld_type' required when 'fld_name' is given");

    OGRFieldType fld_oft = OFTInteger;
    if (fld_type != "")
        fld_oft = _getOFT(fld_type);

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
            hFieldDefn = OGR_Fld_Create(fld_name.c_str(), fld_oft);
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

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr)
        return -1;
    CPLPopErrorHandler();

    int cnt = GDALDatasetGetLayerCount(hDS);
    GDALReleaseDataset(hDS);
    return cnt;
}

//' Get names of layers in a dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_layer_names")]]
SEXP _ogr_ds_layer_names(std::string dsn) {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr)
        return R_NilValue;
    CPLPopErrorHandler();

    int cnt = GDALDatasetGetLayerCount(hDS);
    if (cnt == 0)
        return R_NilValue;

    Rcpp::CharacterVector names = Rcpp::CharacterVector::create();
    for (int i = 0; i < cnt; ++i) {
        OGRLayerH hLayer = nullptr;
        hLayer = GDALDatasetGetLayer(hDS, i);
        if (hLayer != nullptr) {
            names.push_back(OGR_L_GetName(hLayer));
        }
        else {
            Rcpp::warning("failed to obtain layer handle");
            names.push_back("");
        }
    }

    GDALReleaseDataset(hDS);
    return names;
}

//' Does layer exist
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_exists")]]
bool _ogr_layer_exists(std::string dsn, std::string layer) {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    bool ret = false;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
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

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
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

    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
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

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH  hLayer = nullptr;
    int layer_cnt, layer_idx;

    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);

    if (hDS == nullptr)
        return false;

    if (!GDALDatasetTestCapability(hDS, ODsCDeleteLayer)) {
        Rcpp::Rcerr << "dataset does not support delete layer\n";
        GDALReleaseDataset(hDS);
        return false;
    }

    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    if (hLayer == nullptr) {
        Rcpp::Rcerr << "failed to access 'layer'\n";
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

//' Get names of fields on a layer
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_fld_names")]]
SEXP _ogr_layer_fld_names(std::string dsn, std::string layer) {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr)
        return R_NilValue;
    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    CPLPopErrorHandler();

    if (hLayer == nullptr) {
        GDALReleaseDataset(hDS);
        return R_NilValue;
    }

    hFDefn = OGR_L_GetLayerDefn(hLayer);
    if (hFDefn == nullptr)
        return R_NilValue;

    Rcpp::CharacterVector names = Rcpp::CharacterVector::create();

    for (int i = 0; i < OGR_FD_GetFieldCount(hFDefn); ++i) {
        OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, i);
        if (hFieldDefn != nullptr) {
            names.push_back(OGR_Fld_GetNameRef(hFieldDefn));
        }
        else {
            Rcpp::warning("failed to obtain field definition");
            names.push_back("");
        }
    }

    for (int i = 0; i < OGR_FD_GetGeomFieldCount(hFDefn); ++i) {
        OGRGeomFieldDefnH hGeomFldDefn =
                OGR_FD_GetGeomFieldDefn(hFDefn, i);
        if (hGeomFldDefn != nullptr) {
            names.push_back(OGR_GFld_GetNameRef(hGeomFldDefn));
        }
        else {
            Rcpp::warning("failed to obtain geom field definition");
            names.push_back("");
        }
    }

    GDALReleaseDataset(hDS);
    return names;
}

//' Get field index or -1 if fld_name not found
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_field_index")]]
int _ogr_field_index(std::string dsn, std::string layer,
        std::string fld_name) {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;
    int iField;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
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

// Internal wrapper of OGR_L_CreateField()
bool _CreateField(GDALDatasetH hDS, OGRLayerH hLayer,
                  std::string fld_name, std::string fld_type,
                  std::string fld_subtype = "OFSTNone",
                  int fld_width = 0,
                  int fld_precision = 0,
                  bool is_nullable = true,
                  bool is_ignored = false,
                  bool is_unique = false,
                  std::string default_value = "") {

    if (hDS == nullptr || hLayer == nullptr)
        return false;

    OGRFieldDefnH hFieldDefn = nullptr;
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
        if (is_unique) {
            GDALDriverH hDriver = GDALGetDatasetDriver(hDS);
            char **papszMD = GDALGetMetadata(hDriver, nullptr);
            if (CPLFetchBool(papszMD, GDAL_DCAP_UNIQUE_FIELDS, false))
                OGR_Fld_SetUnique(hFieldDefn, true);
            else
                Rcpp::warning(
                    "unique constraint not supported by the format driver");
        }
#endif

        if (OGR_L_CreateField(hLayer, hFieldDefn, TRUE) == OGRERR_NONE)
            ret = true;

        OGR_Fld_Destroy(hFieldDefn);
    }

    return ret;
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

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;
    int iField;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
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

    bool ret = _CreateField(hDS, hLayer, fld_name, fld_type, fld_subtype,
                            fld_width, fld_precision, is_nullable, is_ignored,
                            is_unique, default_value);

    GDALReleaseDataset(hDS);
    return ret;
}

// Internal wrapper of OGR_L_CreateGeomField()
bool _CreateGeomField(OGRLayerH hLayer, std::string fld_name,
                      OGRwkbGeometryType eGeomType,
                      std::string srs = "",
                      bool is_nullable = true,
                      bool is_ignored = false) {

    if (hLayer == nullptr)
        return false;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (srs != "") {
        if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
            if (hSRS != nullptr)
                OSRDestroySpatialReference(hSRS);
            Rcpp::Rcerr << "error importing SRS from user input\n";
            return false;
        }
    }

    bool ret = false;
    OGRGeomFieldDefnH hGeomFieldDefn = nullptr;
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

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;
    int iField;

    OGRwkbGeometryType eGeomType = _getWkbGeomType(geom_type);
    if (eGeomType == wkbUnknown && !EQUALN(geom_type.c_str(), "UNKNOWN", 7))
        Rcpp::stop("'geom_type' not recognized");

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
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

    bool ret = _CreateGeomField(hLayer, fld_name, eGeomType, srs,
                                is_nullable, is_ignored);

    GDALReleaseDataset(hDS);
    return ret;
}

//' Delete an attribute field on a vector layer
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_field_delete")]]
bool _ogr_field_delete(std::string dsn, std::string layer,
                       std::string fld_name) {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);
    if (hDS == nullptr) {
        Rcpp::Rcerr << "failed to open 'dsn' for update\n";
        return false;
    }

    OGRLayerH  hLayer = nullptr;
    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    if (hLayer == nullptr) {
        Rcpp::Rcerr << "failed to access 'layer'\n";
        GDALReleaseDataset(hDS);
        return false;
    }
    if (!OGR_L_TestCapability(hLayer, OLCDeleteField)) {
        Rcpp::Rcerr << "layer does not support delete field\n";
        GDALReleaseDataset(hDS);
        return false;
    }

    OGRFeatureDefnH hFDefn = nullptr;
    int iField;
    hFDefn = OGR_L_GetLayerDefn(hLayer);
    if (hFDefn != nullptr) {
        iField = OGR_FD_GetFieldIndex(hFDefn, fld_name.c_str());
    }
    else {
        Rcpp::Rcerr << "failed to obtain OGRFeatureDefnH\n";
        GDALReleaseDataset(hDS);
        return false;
    }
    if (iField == -1) {
        Rcpp::Rcerr << "'fld_name' not found on 'layer'\n";
        GDALReleaseDataset(hDS);
        return false;
    }

    bool ret = false;
    if (OGR_L_DeleteField(hLayer, iField) == OGRERR_NONE)
        ret = true;

    GDALReleaseDataset(hDS);
    return ret;
}

//' Execute an SQL statement against the data store
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_execute_sql", invisible = true)]]
SEXP _ogr_execute_sql(std::string dsn, std::string sql,
                      std::string spatial_filter = "",
                      std::string dialect = "") {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;

    OGRGeometryH hGeom_filter = nullptr;
    if (spatial_filter != "") {
        char* pszWKT = (char*) spatial_filter.c_str();
        if (OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom_filter) !=
                OGRERR_NONE) {
            if (hGeom_filter != nullptr)
                OGR_G_DestroyGeometry(hGeom_filter);
            Rcpp::Rcerr << "failed to create geometry from 'spatial_filter'\n";
            return R_NilValue;
        }
    }

    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);
    if (hDS == nullptr) {
        Rcpp::Rcerr << "failed to open dsn for update:\n'" <<
                dsn_in.c_str() << "'\n";
        return R_NilValue;
    }
    else {
        Rcpp::Rcout << "info: open for update successful on dsn:\n'" <<
                dsn_in.c_str() << "'\n";
    }

    if (EQUALN(dialect.c_str(), "SQLITE", 6) && !_has_spatialite())
        Rcpp::Rcout << "info: GDAL built without Spatialite support.\n" <<
                "Spatial functions may be unavailable in SQLite dialect.\n";

    const char* pszDialect = nullptr;
    if (dialect != "")
        pszDialect = dialect.c_str();

    OGRLayerH hLayer = nullptr;
    hLayer = GDALDatasetExecuteSQL(hDS, sql.c_str(), hGeom_filter, pszDialect);

    if (hLayer != nullptr)
        GDALDatasetReleaseResultSet(hDS, hLayer);

    if (hGeom_filter != nullptr)
        OGR_G_DestroyGeometry(hGeom_filter);

    GDALReleaseDataset(hDS);
    return R_NilValue;
}
