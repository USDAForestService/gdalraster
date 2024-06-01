/* Utility functions for vector data sources
   Chris Toney <chris.toney at usda.gov> */

#include <vector>

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
        it != MAP_OGR_GEOM_TYPE.end()) {

        return it->second;
    }
    else {
        return wkbUnknown;
    }
}

// Internal lookup of geometry type string by OGRwkbGeometryType
// Returns "UNKNOWN" if no match
std::string _getWkbGeomString(OGRwkbGeometryType eType) {
    for (auto it = MAP_OGR_GEOM_TYPE.begin();
         it != MAP_OGR_GEOM_TYPE.end(); ++it) {

        if (it->second == eType)
            return it->first;
    }
    return "UNKNOWN";
}

// Internal lookup of OGRFieldType by string descriptor
// Error if no match
OGRFieldType _getOFT(std::string fld_type) {
    if (auto it = MAP_OGR_FLD_TYPE.find(fld_type);
        it != MAP_OGR_FLD_TYPE.end()) {

        return it->second;
    }
    else {
        Rcpp::stop("unrecognized OGR field type descriptor");
    }
}

// Internal lookup of OGR field type string by OGRFieldType
// Returns empty string if no match, with warning emitted
std::string _getOFTString(OGRFieldType eType) {
    for (auto it = MAP_OGR_FLD_TYPE.begin();
         it != MAP_OGR_FLD_TYPE.end(); ++it) {

        if (it->second == eType)
            return it->first;
    }
    Rcpp::warning("unrecognized OGRFieldType enumerator");
    return "";
}

// Internal lookup of OGRFieldSubType by string descriptor
// Returns OFSTNone if no match
OGRFieldSubType _getOFTSubtype(std::string fld_subtype) {
    if (auto it = MAP_OGR_FLD_SUBTYPE.find(fld_subtype);
        it != MAP_OGR_FLD_SUBTYPE.end()) {

        return it->second;
    }
    else {
        return OFSTNone;
    }
}

// Internal lookup of OGR field subtype string by OGRFieldSubType
// Returns "OFSTNone" if no match
std::string _getOFTSubtypeString(OGRFieldSubType eType) {
    for (auto it = MAP_OGR_FLD_SUBTYPE.begin();
         it != MAP_OGR_FLD_SUBTYPE.end(); ++it) {

        if (it->second == eType)
            return it->first;
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

//' Get the format driver short name for a vector dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_format")]]
std::string _ogr_ds_format(std::string dsn) {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    std::string fmt = "";

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR,
                     nullptr, nullptr, nullptr);
    CPLPopErrorHandler();

    if (hDS == nullptr)
        return "";

    GDALDriverH hDriver = GDALGetDatasetDriver(hDS);
    if (hDriver)
        fmt = GDALGetDriverShortName(hDriver);

    GDALReleaseDataset(hDS);
    return fmt;
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

    GDALReleaseDataset(hDS);
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
        Rcpp::Nullable<Rcpp::CharacterVector> lco = R_NilValue,
        Rcpp::Nullable<Rcpp::List> layer_defn = R_NilValue) {

    GDALDriverH hDriver = GDALGetDriverByName(format.c_str());
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver for the specified format");

    std::string dsn_in = Rcpp::as<std::string>(
            _check_gdal_filename(dst_filename));

    char **papszMetadata = GDALGetMetadata(hDriver, nullptr);
    if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATE, FALSE))
        Rcpp::stop("driver does not support create");

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

    if (layer == "" && layer_defn.isNull()) {
        GDALReleaseDataset(hDstDS);
        return true;
    }

    if (!GDALDatasetTestCapability(hDstDS, ODsCCreateLayer)) {
        GDALReleaseDataset(hDstDS);
        return false;
    }

    OGRLayerH  hLayer = nullptr;
    bool layer_ok = false;
    bool fld_ok = false;

    hLayer = _CreateLayer(hDstDS, layer, layer_defn, geom_type, srs, lco);

    if (hLayer != nullptr) {
        layer_ok = true;
        if (layer_defn.isNull() && fld_name != "") {
            OGRFieldDefnH hFieldDefn = nullptr;
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
    CPLPopErrorHandler();

    int cnt = 0;
    if (hDS != nullptr) {
        cnt = GDALDatasetGetLayerCount(hDS);
        GDALReleaseDataset(hDS);
    }
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
    if (cnt == 0) {
        GDALReleaseDataset(hDS);
        return R_NilValue;
    }

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

//' Test if capabilities are available for a vector layer
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_test_cap")]]
SEXP _ogr_layer_test_cap(std::string dsn, std::string layer,
                         bool with_update = true) {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    if (with_update)
        hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                         nullptr, nullptr, nullptr);
    else
        hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR,
                         nullptr, nullptr, nullptr);
    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    CPLPopErrorHandler();

    if (hDS == nullptr || hLayer == nullptr)
        return R_NilValue;

    Rcpp::List cap = Rcpp::List::create(
        Rcpp::Named("RandomRead") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCRandomRead)),
        Rcpp::Named("SequentialWrite") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCSequentialWrite)),
        Rcpp::Named("RandomWrite") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCRandomWrite)),
#if GDAL_VERSION_NUM >= 3060000
        Rcpp::Named("UpsertFeature") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCUpsertFeature)),
#endif
        Rcpp::Named("FastSpatialFilter") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCFastSpatialFilter)),
        Rcpp::Named("FastFeatureCount") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCFastFeatureCount)),
        Rcpp::Named("FastGetExtent") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCFastGetExtent)),
        Rcpp::Named("FastSetNextByIndex") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCFastSetNextByIndex)),
        Rcpp::Named("CreateField") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCCreateField)),
        Rcpp::Named("CreateGeomField") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCCreateGeomField)),
        Rcpp::Named("DeleteField") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCDeleteField)),
        Rcpp::Named("ReorderFields") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCReorderFields)),
        Rcpp::Named("AlterFieldDefn") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCAlterFieldDefn)),
#if GDAL_VERSION_NUM >= 3060000
        Rcpp::Named("AlterGeomFieldDefn") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCAlterGeomFieldDefn)),
#endif
        Rcpp::Named("DeleteFeature") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCDeleteFeature)),
        Rcpp::Named("StringsAsUTF8") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCStringsAsUTF8)),
        Rcpp::Named("Transactions") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCTransactions)),
        Rcpp::Named("CurveGeometries") = static_cast<bool>(
            OGR_L_TestCapability(hLayer, OLCCurveGeometries)));

    GDALReleaseDataset(hDS);
    return cap;
}

// Internal wrapper of GDALDatasetCreateLayer()
OGRLayerH _CreateLayer(GDALDatasetH hDS, std::string layer,
        Rcpp::Nullable<Rcpp::List> layer_defn = R_NilValue,
        std::string geom_type = "UNKNOWN", std::string srs = "",
        Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

    if (hDS == nullptr)
        return nullptr;

    OGRLayerH hLayer = nullptr;
    std::string geom_type_in = geom_type;
    std::string srs_in = srs;
    Rcpp::List layer_defn_in;
    Rcpp::CharacterVector fld_names;
    std::string geom_fld_name = "";
    if (layer_defn.isNotNull()) {
        // layer_defn given so get geom_type and srs from first geom field defn
        layer_defn_in = layer_defn;
        fld_names = layer_defn_in.names();
        bool has_geom_fld_defn = false;
        for (R_xlen_t i = 0; i < layer_defn_in.size(); ++i) {
            Rcpp::List fld = layer_defn_in[i];
            Rcpp::LogicalVector fld_is_geom = fld["is_geom"];
            if (Rcpp::is_true(Rcpp::all(fld_is_geom))) {
                geom_type_in = Rcpp::as<std::string>(fld["type"]);
                geom_fld_name = Rcpp::as<std::string>(fld_names(i));
                srs_in = Rcpp::as<std::string>(fld["srs"]);
                has_geom_fld_defn = true;
                break;
            }
        }
        if (!has_geom_fld_defn)
            Rcpp::stop("'layer_defn' is missing a geometry field definition");
    }

    OGRwkbGeometryType eGeomType = _getWkbGeomType(geom_type_in);
    if (eGeomType == wkbUnknown && !EQUALN(geom_type_in.c_str(), "UNKNOWN", 7))
        Rcpp::stop("'geom_type' not recognized");

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (srs_in != "") {
        if (OSRSetFromUserInput(hSRS, srs_in.c_str()) != OGRERR_NONE) {
            if (hSRS != nullptr)
                OSRDestroySpatialReference(hSRS);
            Rcpp::stop("error importing SRS from user input");
        }
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

    if (hLayer != nullptr) {
        if (layer_defn.isNotNull()) {
            // create fields
            for (R_xlen_t i = 0; i < layer_defn_in.size(); ++i) {
                std::string fld_name;
                std::string fld_type;
                std::string fld_subtype = "OFSTNone";
                int fld_width = 0;
                int fld_precision = 0;
                bool is_nullable = true;
                bool is_ignored = false;
                bool is_unique = false;
                std::string default_value = "";
                std::string srs = "";

                fld_name = Rcpp::as<std::string>(fld_names(i));
                if (fld_name == geom_fld_name)
                    continue;

                Rcpp::List fld = layer_defn_in[i];
                Rcpp::LogicalVector fld_is_geom = fld["is_geom"];
                if (Rcpp::is_false(Rcpp::all(fld_is_geom))) {
                    // attribute field defn
                    if (fld.containsElementNamed("type")) {
                        fld_type = Rcpp::as<std::string>(fld["type"]);
                    }
                    else {
                        Rcpp::Rcerr << "$type missing in field definition\n" <<
                                "could not create field: " <<
                                fld_name.c_str() << "\n";

                        continue;
                    }
                    if (fld.containsElementNamed("subtype"))
                        fld_subtype = Rcpp::as<std::string>(fld["subtype"]);
                    if (fld.containsElementNamed("width"))
                        fld_width = Rcpp::as<int>(fld["width"]);
                    if (fld.containsElementNamed("precision"))
                        fld_precision = Rcpp::as<int>(fld["precision"]);
                    if (fld.containsElementNamed("is_nullable")) {
                        Rcpp::LogicalVector fld_is_nullable =
                                fld["is_nullable"];

                        is_nullable =
                                Rcpp::is_true(Rcpp::all(fld_is_nullable));
                    }
                    if (fld.containsElementNamed("is_ignored")) {
                        Rcpp::LogicalVector fld_is_ignored = fld["is_ignored"];
                        is_ignored = Rcpp::is_true(Rcpp::all(fld_is_ignored));
                    }
                    if (fld.containsElementNamed("is_unique")) {
                        Rcpp::LogicalVector fld_is_unique = fld["is_unique"];
                        is_unique = Rcpp::is_true(Rcpp::all(fld_is_unique));
                    }
                    if (fld.containsElementNamed("default"))
                        default_value = Rcpp::as<std::string>(fld["default"]);

                    if (!_CreateField(hDS, hLayer, fld_name, fld_type,
                                      fld_subtype, fld_width, fld_precision,
                                      is_nullable, is_ignored, is_unique,
                                      default_value)) {

                        Rcpp::Rcerr << "failed to create field: " <<
                                fld_name.c_str() << "\n";
                    }
                }
                else {
                    // geometry field defn
                    OGRwkbGeometryType eThisGeomType = eGeomType;
                    if (fld.containsElementNamed("type")) {
                        std::string this_geom_type;
                        this_geom_type = Rcpp::as<std::string>(fld["type"]);
                        if (this_geom_type != geom_type_in) {
                            eThisGeomType = _getWkbGeomType(this_geom_type);
                            if (eThisGeomType == wkbUnknown &&
                                !EQUALN(this_geom_type.c_str(), "UNKNOWN", 7)) {

                                Rcpp::warning("geometry type not recognized");
                            }
                        }
                    }
                    else {
                        Rcpp::Rcerr << "$type missing in field definition\n" <<
                                "could not create geom field: " <<
                                fld_name.c_str() << "\n";

                        continue;
                    }
                    if (fld.containsElementNamed("srs"))
                        srs = Rcpp::as<std::string>(fld["srs"]);
                    if (fld.containsElementNamed("is_nullable")) {
                        Rcpp::LogicalVector fld_is_nullable =
                                fld["is_nullable"];

                        is_nullable =
                                Rcpp::is_true(Rcpp::all(fld_is_nullable));
                    }
                    if (fld.containsElementNamed("is_ignored")) {
                        Rcpp::LogicalVector fld_is_ignored = fld["is_ignored"];
                        is_ignored = Rcpp::is_true(Rcpp::all(fld_is_ignored));
                    }

                    if (!_CreateGeomField(hDS, hLayer, fld_name, eThisGeomType,
                                          srs, is_nullable, is_ignored)) {

                        Rcpp::Rcerr << "failed to create geom field: " <<
                                fld_name.c_str() << "\n";
                    }
                }
            }
        }
    }

    if (hSRS != nullptr)
        OSRDestroySpatialReference(hSRS);

    return hLayer;
}

//' Create a layer in a vector dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_create")]]
bool _ogr_layer_create(std::string dsn, std::string layer,
        Rcpp::Nullable<Rcpp::List> layer_defn = R_NilValue,
        std::string geom_type = "UNKNOWN", std::string srs = "",
        Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

    std::string dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;

    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);

    if (hDS == nullptr)
        return false;

    if (!GDALDatasetTestCapability(hDS, ODsCCreateLayer)) {
        GDALReleaseDataset(hDS);
        Rcpp::Rcerr << "dataset does not have CreateLayer capability\n";
        return false;
    }

    hLayer = _CreateLayer(hDS, layer, layer_defn, geom_type, srs, options);

    GDALReleaseDataset(hDS);

    if (hLayer != nullptr)
        return true;
    else
        return false;
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
        Rcpp::Rcerr << "dataset does not have DeleteLayer capability\n";
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
// [[Rcpp::export(name = ".ogr_layer_field_names")]]
SEXP _ogr_layer_field_names(std::string dsn, std::string layer) {
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
    if (hFDefn == nullptr) {
        GDALReleaseDataset(hDS);
        return R_NilValue;
    }

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
    GDALDriverH hDriver = GDALGetDatasetDriver(hDS);
    char **papszMD = GDALGetMetadata(hDriver, nullptr);
    bool ret = false;

    hFieldDefn = OGR_Fld_Create(fld_name.c_str(), eFieldType);
    if (hFieldDefn != nullptr) {
        OGR_Fld_SetSubType(hFieldDefn, eFieldSubType);

        if (fld_width > 0)
            OGR_Fld_SetWidth(hFieldDefn, fld_width);

        if (fld_precision > 0)
            OGR_Fld_SetPrecision(hFieldDefn, fld_precision);

        if (!is_nullable) {
            if (CPLFetchBool(papszMD, GDAL_DCAP_NOTNULL_FIELDS, false))
                OGR_Fld_SetNullable(hFieldDefn, false);
            else
                Rcpp::warning(
                    "not-null constraint is unsupported by the format driver");
        }

        if (is_ignored)
            OGR_Fld_SetIgnored(hFieldDefn, true);

        if (default_value != "") {
            if (CPLFetchBool(papszMD, GDAL_DCAP_DEFAULT_FIELDS, false))
                OGR_Fld_SetDefault(hFieldDefn, default_value.c_str());
            else
                Rcpp::warning(
                    "default field value not supported by the format driver");
        }

#if GDAL_VERSION_NUM >= 3020000
        if (is_unique) {
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

    if (!OGR_L_TestCapability(hLayer, OLCCreateField)) {
        GDALReleaseDataset(hDS);
        Rcpp::Rcerr << "'layer' does not have CreateField capability\n";
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
bool _CreateGeomField(GDALDatasetH hDS, OGRLayerH hLayer,
                      std::string fld_name,
                      OGRwkbGeometryType eGeomType,
                      std::string srs = "",
                      bool is_nullable = true,
                      bool is_ignored = false) {

    if (hDS == nullptr || hLayer == nullptr)
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

    GDALDriverH hDriver = GDALGetDatasetDriver(hDS);
    char **papszMD = GDALGetMetadata(hDriver, nullptr);
    bool ret = false;

    OGRGeomFieldDefnH hGeomFieldDefn = nullptr;
    hGeomFieldDefn = OGR_GFld_Create(fld_name.c_str(), eGeomType);
    if (hGeomFieldDefn != nullptr) {
        if (!is_nullable) {
            if (CPLFetchBool(papszMD, GDAL_DCAP_NOTNULL_GEOMFIELDS, false))
                OGR_GFld_SetNullable(hGeomFieldDefn, false);
            else
                Rcpp::warning(
                    "not-null constraint is unsupported by the format driver");
        }

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

    if (!OGR_L_TestCapability(hLayer, OLCCreateGeomField)) {
        GDALReleaseDataset(hDS);
        Rcpp::Rcerr << "'layer' does not have CreateGeomField capability\n";
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

    bool ret = _CreateGeomField(hDS, hLayer, fld_name, eGeomType, srs,
                                is_nullable, is_ignored);

    GDALReleaseDataset(hDS);
    return ret;
}

//' Rename an attribute field on a vector layer
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_field_rename")]]
bool _ogr_field_rename(std::string dsn, std::string layer,
                       std::string fld_name, std::string new_name) {

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
    if (!OGR_L_TestCapability(hLayer, OLCAlterFieldDefn)) {
        Rcpp::Rcerr << "'layer' does not have AlterFieldDefn capability\n";
        GDALReleaseDataset(hDS);
        return false;
    }

    int iField;
    OGRFeatureDefnH hFDefn = nullptr;
    hFDefn = OGR_L_GetLayerDefn(hLayer);
    if (hFDefn != nullptr) {
        iField = OGR_FD_GetFieldIndex(hFDefn, fld_name.c_str());
    }
    else {
        GDALReleaseDataset(hDS);
        return false;
    }
    if (iField == -1) {
        Rcpp::Rcerr << "'fld_name' not found on 'layer'\n";
        GDALReleaseDataset(hDS);
        return false;
    }

    OGRFieldDefnH hFieldDefn = nullptr;
    OGRFieldType eFieldType;
    hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, iField);
    if (hFieldDefn != nullptr)
        eFieldType = OGR_Fld_GetType(hFieldDefn);
    else
        eFieldType = OFTString;  // not changing the type anyway

    OGRFieldDefnH hNewFieldDefn;
    hNewFieldDefn = OGR_Fld_Create(new_name.c_str(), eFieldType);
    OGRErr err = OGR_L_AlterFieldDefn(hLayer, iField, hNewFieldDefn,
                                      ALTER_NAME_FLAG);
    OGR_Fld_Destroy(hNewFieldDefn);
    GDALReleaseDataset(hDS);

    if (err != OGRERR_NONE) {
        Rcpp::Rcerr << "failed to rename field\n";
        return false;
    }
    else {
        return true;
    }
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
        Rcpp::Rcerr << "'layer' does not have DeleteField capability\n";
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
        Rcpp::Rcerr << "failed to open DSN for update:\n'" <<
                dsn_in.c_str() << "'\n";
        return R_NilValue;
    }
    else {
        Rcpp::Rcout << "info: open dataset successful on DSN:\n  '" <<
                dsn_in.c_str() << "'\n";
    }

    const char* pszDialect = dialect.c_str();
    if (EQUALN(pszDialect, "SQLITE", 6) && !has_spatialite())
        Rcpp::Rcout << "info: GDAL built without Spatialite support\n" <<
                "Spatial functions may be unavailable in SQLite dialect.\n";

    OGRLayerH hLayer = nullptr;
    hLayer = GDALDatasetExecuteSQL(hDS, sql.c_str(), hGeom_filter, pszDialect);

    if (hLayer != nullptr)
        GDALDatasetReleaseResultSet(hDS, hLayer);

    if (hGeom_filter != nullptr)
        OGR_G_DestroyGeometry(hGeom_filter);

    GDALReleaseDataset(hDS);
    return R_NilValue;
}
