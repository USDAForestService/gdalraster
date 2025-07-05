/* Utility functions for vector data sources
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include <cstdint>
#include <vector>

#include "gdal.h"
#include "cpl_port.h"
#include "cpl_error.h"
#include "cpl_string.h"
#include "cpl_time.h"
#include "ogr_srs_api.h"

#include "gdalraster.h"
#include "ogr_util.h"


OGRwkbGeometryType getWkbGeomType_(const std::string &geom_type) {
    std::string geom_type_in = str_toupper_(geom_type);
    if (auto it = MAP_OGR_GEOM_TYPE.find(geom_type_in);
        it != MAP_OGR_GEOM_TYPE.end()) {

        return it->second;
    }
    else {
        return wkbUnknown;
    }
}

std::string getWkbGeomString_(OGRwkbGeometryType eType) {
    for (auto it = MAP_OGR_GEOM_TYPE.begin();
         it != MAP_OGR_GEOM_TYPE.end(); ++it) {

        if (it->second == eType)
            return it->first;
    }
    return "UNKNOWN";
}

OGRFieldType getOFT_(const std::string &fld_type) {
    if (auto it = MAP_OGR_FLD_TYPE.find(fld_type);
        it != MAP_OGR_FLD_TYPE.end()) {

        return it->second;
    }
    else {
        Rcpp::stop("unrecognized OGR field type descriptor");
    }
}

std::string getOFTString_(OGRFieldType eType) {
    for (auto it = MAP_OGR_FLD_TYPE.begin();
         it != MAP_OGR_FLD_TYPE.end(); ++it) {

        if (it->second == eType)
            return it->first;
    }
    Rcpp::warning("unrecognized OGRFieldType enumerator");
    return "";
}

OGRFieldSubType getOFTSubtype_(const std::string &fld_subtype) {
    if (auto it = MAP_OGR_FLD_SUBTYPE.find(fld_subtype);
        it != MAP_OGR_FLD_SUBTYPE.end()) {

        return it->second;
    }
    else {
        return OFSTNone;
    }
}

std::string getOFTSubtypeString_(OGRFieldSubType eType) {
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
bool ogr_ds_exists(const std::string &dsn, bool with_update = false) {

    const std::string &dsn_in = Rcpp::as<std::string>(
        check_gdal_filename(dsn));

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
std::string ogr_ds_format(const std::string &dsn) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));

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
SEXP ogr_ds_test_cap(const std::string &dsn, bool with_update = true) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
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

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 3, 0)
        cap.push_back(
            static_cast<bool>(
                GDALDatasetTestCapability(hDS, ODsCAddFieldDomain)),
            "AddFieldDomain");
#endif
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 5, 0)
        cap.push_back(
            static_cast<bool>(
                GDALDatasetTestCapability(hDS, ODsCDeleteFieldDomain)),
            "DeleteFieldDomain");
        cap.push_back(
            static_cast<bool>(
                GDALDatasetTestCapability(hDS, ODsCUpdateFieldDomain)),
            "UpdateFieldDomain");
#endif

    GDALReleaseDataset(hDS);
    return cap;
}

// Create a vector dataset. Optionally create a layer in the dataset.
// A field is also created optionally (name and type only).
//
// Implemented as a GDALVector object factory registered in
// RCPP_MODULE(mod_GDALVector), see src/gdalvector.cpp.
// Unique function signature based on number of parameters.
// Called in R with `lyr <- new(GDALVector, ...)` giving all 10 parameters,
// as in R/ogr_manage.R.
GDALVector *create_ogr(const std::string &format,
                       const std::string &dst_filename,
                       const std::string &layer,
                       const std::string &geom_type,
                       const std::string &srs,
                       const std::string &fld_name,
                       const std::string &fld_type ,
                       const Rcpp::Nullable<Rcpp::CharacterVector> &dsco,
                       const Rcpp::Nullable<Rcpp::CharacterVector> &lco,
                       const Rcpp::Nullable<Rcpp::List> &layer_defn) {

    GDALDriverH hDriver = GDALGetDriverByName(format.c_str());
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver for the specified format");

    std::string dsn_in = Rcpp::as<std::string>(
            check_gdal_filename(dst_filename));

    char **papszMetadata = GDALGetMetadata(hDriver, nullptr);
    if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATE, FALSE))
        Rcpp::stop("driver does not support create");

    if (fld_name != "" && fld_type == "")
        Rcpp::stop("'fld_type' required when 'fld_name' is given");

    OGRFieldType fld_oft = OFTInteger;
    if (fld_type != "")
        fld_oft = getOFT_(fld_type);

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
                        0, 0, 0, GDT_Unknown,
                        opt_list.data());

    if (hDstDS == nullptr)
        Rcpp::stop("failed to create vector data source");

    if (layer == "" && layer_defn.isNull()) {
        GDALReleaseDataset(hDstDS);
        GDALVector *ds = new GDALVector;
        ds->setDsn_(dsn_in);
        // for internal use, return object has no dataset or layer in this case
        // currently would not be usable from R, but could change in the future
        return ds;
    }

    if (!GDALDatasetTestCapability(hDstDS, ODsCCreateLayer)) {
        GDALReleaseDataset(hDstDS);
        Rcpp::stop("data source does not have CreateLayer capability");
    }

    OGRLayerH  hLayer = nullptr;
    bool layer_ok = false;
    bool fld_ok = false;

    hLayer = CreateLayer_(hDstDS, layer, layer_defn, geom_type, srs, lco);

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

    if (!layer_ok) {
        GDALReleaseDataset(hDstDS);
        Rcpp::stop("layer creation failed");
    }
    else if (!fld_ok) {
        // TODO(ctoney): make layer + field creation atomic
        GDALReleaseDataset(hDstDS);
        Rcpp::stop("the layer was created but field creation failed");
    }
    else {
        GDALVector *lyr = new GDALVector;
        lyr->setDsn_(dsn_in);
        lyr->setGDALDatasetH_(hDstDS, true);
        lyr->setOGRLayerH_(hLayer, layer);
        lyr->setFieldNames_();
        return lyr;
    }
}

//' Get number of layers in a dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_layer_count")]]
int ogr_ds_layer_count(const std::string &dsn) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
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
SEXP ogr_ds_layer_names(const std::string &dsn) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
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

//' Return a list of the names of all field domains stored in the dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_field_domain_names")]]
SEXP ogr_ds_field_domain_names(const std::string &dsn) {
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 5, 0)
    Rcpp::stop("'ogr_ds_field_domain_names() requires GDAL >= 3.5");

#else
    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;

    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr) {
        Rcpp::warning("failed to open dataset");
        return R_NilValue;
    }

    GDALDriverH hDriver = GDALGetDatasetDriver(hDS);
    char **papszMD = GDALGetMetadata(hDriver, nullptr);
    if (!CPLFetchBool(papszMD, GDAL_DCAP_FIELD_DOMAINS, false)) {
        Rcpp::warning("format does not support reading field domains");
        GDALReleaseDataset(hDS);
        return R_NilValue;
    }

    Rcpp::CharacterVector names = Rcpp::CharacterVector::create();
    char **papszFldDomNames = nullptr;
    papszFldDomNames = GDALDatasetGetFieldDomainNames(hDS, nullptr);
    int items = CSLCount(papszFldDomNames);
    if (items > 0) {
        for (int i = 0; i < items; ++i) {
            names.push_back(papszFldDomNames[i]);
        }
    }
    CSLDestroy(papszFldDomNames);

    GDALReleaseDataset(hDS);
    return names;
#endif
}

//' Add a field domain to a dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_add_field_domain")]]
bool ogr_ds_add_field_domain(const std::string &dsn,
                             const Rcpp::List &fld_dom_defn) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
    Rcpp::stop("'ogr_ds_field_domain_names() requires GDAL >= 3.3");

#else
    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    bool ret = false;

    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);

    if (hDS == nullptr) {
        Rcpp::stop("failed to open dataset");
    }

    //
    // get the inputs common to all domain types
    //

    // domain type
    if (!GDALDatasetTestCapability(hDS, ODsCAddFieldDomain)) {
        GDALReleaseDataset(hDS);
        Rcpp::stop("format does not support adding field domains");
    }

    if (!fld_dom_defn.containsElementNamed("type") ||
        fld_dom_defn["type"] == R_NilValue ||
        !Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["type"])) {

        GDALReleaseDataset(hDS);
        Rcpp::stop("'$type' must be a character string");
    }
    const std::string domain_type =
            Rcpp::as<std::string>(fld_dom_defn["type"]);

    // domain_name
    if (!fld_dom_defn.containsElementNamed("domain_name") ||
        fld_dom_defn["domain_name"] == R_NilValue ||
        !Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["domain_name"])) {

        GDALReleaseDataset(hDS);
        Rcpp::stop("'$domain_name' must be a character string");
    }
    const std::string domain_name =
            Rcpp::as<std::string>(fld_dom_defn["domain_name"]);

    // optional description
    std::string description = "";
    if (fld_dom_defn.containsElementNamed("description") &&
        fld_dom_defn["description"] != R_NilValue &&
        Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["description"])) {

        description = Rcpp::as<std::string>(fld_dom_defn["description"]);
    }

    // field_type
    if (!fld_dom_defn.containsElementNamed("field_type") ||
        fld_dom_defn["field_type"] == R_NilValue ||
        !Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["field_type"])) {

        GDALReleaseDataset(hDS);
        Rcpp::stop("'$field_type' must be a character string");
    }
    const std::string field_type =
            Rcpp::as<std::string>(fld_dom_defn["field_type"]);
    OGRFieldType eFieldType = getOFT_(field_type);

    // optional field_subtype
    std::string field_subtype = "OFSTNone";
    if (fld_dom_defn.containsElementNamed("field_subtype") &&
        fld_dom_defn["field_subtype"] != R_NilValue &&
        Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["field_subtype"])) {

        field_subtype = Rcpp::as<std::string>(fld_dom_defn["field_subtype"]);
    }
    OGRFieldSubType eFieldSubType = getOFTSubtype_(field_subtype);

    // split policy
    std::string split_policy = "DEFAULT_VALUE";
    OGRFieldDomainSplitPolicy eOFDSP;
    if (fld_dom_defn.containsElementNamed("split_policy") &&
        fld_dom_defn["split_policy"] != R_NilValue &&
        Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["split_policy"])) {

        split_policy = Rcpp::as<std::string>(fld_dom_defn["split_policy"]);
    }
    if (EQUAL(split_policy.c_str(), "DEFAULT_VALUE")) {
        eOFDSP = OFDSP_DEFAULT_VALUE;
    }
    else if (EQUAL(split_policy.c_str(), "DUPLICATE")) {
        eOFDSP = OFDSP_DUPLICATE;
    }
    else if (EQUAL(split_policy.c_str(), "GEOMETRY_RATIO")) {
        eOFDSP = OFDSP_GEOMETRY_RATIO;
    }
    else {
        GDALReleaseDataset(hDS);
        Rcpp::stop("invalid '$split_policy'");
    }

    // merge policy
    std::string merge_policy = "DEFAULT_VALUE";
    OGRFieldDomainMergePolicy eOFDMP;
    if (fld_dom_defn.containsElementNamed("merge_policy") &&
        fld_dom_defn["merge_policy"] != R_NilValue &&
        Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["merge_policy"])) {

        merge_policy = Rcpp::as<std::string>(fld_dom_defn["merge_policy"]);
    }
    if (EQUAL(merge_policy.c_str(), "DEFAULT_VALUE")) {
        eOFDMP = OFDMP_DEFAULT_VALUE;
    }
    else if (EQUAL(merge_policy.c_str(), "SUM")) {
        eOFDMP = OFDMP_SUM;
    }
    else if (EQUAL(merge_policy.c_str(), "GEOMETRY_WEIGHTED")) {
        eOFDMP = OFDMP_GEOMETRY_WEIGHTED;
    }
    else {
        GDALReleaseDataset(hDS);
        Rcpp::stop("invalid '$merge_policy'");
    }

    //
    // create and add field domains
    //

    // Coded
    if (EQUAL(domain_type.c_str(), "coded")) {
        if (!fld_dom_defn.containsElementNamed("coded_values") ||
            fld_dom_defn["coded_values"] == R_NilValue ||
            (!Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["coded_values"]) &&
             !Rcpp::is<Rcpp::DataFrame>(fld_dom_defn["coded_values"]))) {

            GDALReleaseDataset(hDS);
            Rcpp::stop(
                "'$coded_values' must be a character vector or data frame");
        }

        std::vector<OGRCodedValue> ogr_coded_values = {};

        // as character vector of codes, or "code=value" pairs
        if (Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["coded_values"])) {
            Rcpp::CharacterVector coded_values = fld_dom_defn["coded_values"];

            if (coded_values.size() == 0) {
                GDALReleaseDataset(hDS);
                Rcpp::stop("'coded_values' is empty");
            }

            if (Rcpp::any(Rcpp::is_na(coded_values))) {
                GDALReleaseDataset(hDS);
                Rcpp::stop("'coded_values' cannot contain NA codes");
            }

            for (Rcpp::CharacterVector::iterator i = coded_values.begin();
                    i != coded_values.end(); ++i) {

                char **papszTokens = CSLTokenizeString2(*i, "=",
                        CSLT_STRIPLEADSPACES | CSLT_STRIPENDSPACES);

                if (CSLCount(papszTokens) < 1 || CSLCount(papszTokens) > 2) {
                    GDALReleaseDataset(hDS);
                    CSLDestroy(papszTokens);
                    if (!ogr_coded_values.empty()) {
                        for (auto &cv : ogr_coded_values) {
                            VSIFree(cv.pszCode);
                            VSIFree(cv.pszValue);
                        }
                    }
                    Rcpp::stop(
                        "elements of 'coded_values' must be \"CODE\" or \"CODE=VALUE\"");
                }

                OGRCodedValue cv;
                cv.pszCode = CPLStrdup(papszTokens[0]);
                if (CSLCount(papszTokens) == 2)
                    cv.pszValue = CPLStrdup(papszTokens[1]);
                else
                    cv.pszValue = nullptr;
                ogr_coded_values.emplace_back(cv);
                CSLDestroy(papszTokens);
            }
        }
        // as two-column data frame of codes, values
        else if (Rcpp::is<Rcpp::DataFrame>(fld_dom_defn["coded_values"])) {
            Rcpp::DataFrame coded_values =
                Rcpp::as<Rcpp::DataFrame>(fld_dom_defn["coded_values"]);

            if (coded_values.nrows() == 0) {
                GDALReleaseDataset(hDS);
                Rcpp::stop("'coded_values' is empty");
            }

            if (coded_values.size() != 2) {
                GDALReleaseDataset(hDS);
                Rcpp::stop("'coded_values' data frame must have two columns");
            }

            if (!Rcpp::is<Rcpp::CharacterVector>(coded_values[0]) ||
                !Rcpp::is<Rcpp::CharacterVector>(coded_values[1])) {

                GDALReleaseDataset(hDS);
                Rcpp::stop("columns of 'coded_values' must be character type");
            }

            Rcpp::CharacterVector codes = coded_values[0];
            Rcpp::CharacterVector values = coded_values[1];

            if (Rcpp::any(Rcpp::is_na(codes))) {
                GDALReleaseDataset(hDS);
                Rcpp::stop("'coded_values' cannot contain NA codes");
            }

            for (R_xlen_t i = 0; i < codes.size(); ++i) {
                OGRCodedValue cv;
                cv.pszCode = CPLStrdup(codes[i]);
                if (!Rcpp::CharacterVector::is_na(values[i]))
                    cv.pszValue = CPLStrdup(values[i]);
                else
                    cv.pszValue = nullptr;
                ogr_coded_values.emplace_back(cv);
            }
        }
        else {
            // should not ever reach this
            GDALReleaseDataset(hDS);
            Rcpp::stop(
                "'$coded_values' must be a character vector or data frame");
        }
        OGRCodedValue cv;
        cv.pszCode = nullptr;
        cv.pszValue = nullptr;
        ogr_coded_values.emplace_back(cv);

        OGRFieldDomainH hFldDom = nullptr;
        hFldDom = OGR_CodedFldDomain_Create(domain_name.c_str(),
                                            description.c_str(),
                                            eFieldType, eFieldSubType,
                                            ogr_coded_values.data());
        if (hFldDom != nullptr) {
            OGR_FldDomain_SetSplitPolicy(hFldDom, eOFDSP);
            OGR_FldDomain_SetMergePolicy(hFldDom, eOFDMP);
            char *pszFailureReason = nullptr;
            ret = GDALDatasetAddFieldDomain(hDS, hFldDom, &pszFailureReason);
            if (pszFailureReason != nullptr) {
                Rcpp::Rcout << pszFailureReason << std::endl;
                VSIFree(pszFailureReason);
            }
            OGR_FldDomain_Destroy(hFldDom);
        }
        for (auto &cv : ogr_coded_values) {
            VSIFree(cv.pszCode);
            VSIFree(cv.pszValue);
        }
    }

    // Range
    else if (EQUAL(domain_type.c_str(), "range")) {
        bool min_is_null = false;
        bool max_is_null = false;
        bool min_is_inclusive = true;
        bool max_is_inclusive = true;

        if (!fld_dom_defn.containsElementNamed("min_is_inclusive") ||
            fld_dom_defn["min_is_inclusive"] == R_NilValue ||
            !Rcpp::is<Rcpp::LogicalVector>(
                fld_dom_defn["min_is_inclusive"])) {

            min_is_inclusive = true;
        }
        else {
            Rcpp::LogicalVector tmp = fld_dom_defn["min_is_inclusive"];
            if (tmp[0] == FALSE)
                min_is_inclusive = false;
        }

        if (!fld_dom_defn.containsElementNamed("max_is_inclusive") ||
            fld_dom_defn["max_is_inclusive"] == R_NilValue ||
            !Rcpp::is<Rcpp::LogicalVector>(
                fld_dom_defn["max_is_inclusive"])) {

            max_is_inclusive = true;
        }
        else {
            Rcpp::LogicalVector tmp = fld_dom_defn["max_is_inclusive"];
            if (tmp[0] == FALSE)
                max_is_inclusive = false;
        }

        // Integer and Real ranges
        if (eFieldType == OFTInteger || eFieldType == OFTReal) {
            double min_value = -99999.0;
            double max_value = -99999.0;

            if (fld_dom_defn.containsElementNamed("min_value") &&
                fld_dom_defn["min_value"] == R_NilValue) {

                min_is_null = true;
            }
            else if (!fld_dom_defn.containsElementNamed("min_value") ||
                     (!Rcpp::is<Rcpp::NumericVector>(
                        fld_dom_defn["min_value"]) &&
                      !Rcpp::is<Rcpp::IntegerVector>(
                        fld_dom_defn["min_value"]) &&
                      !Rcpp::is<Rcpp::LogicalVector>(
                        fld_dom_defn["min_value"]))) {

                GDALReleaseDataset(hDS);
                Rcpp::stop("'$min_value' must be integer, double or NULL");
            }
            else {
                Rcpp::NumericVector tmp = fld_dom_defn["min_value"];
                if (tmp.size() != 1) {
                    GDALReleaseDataset(hDS);
                    Rcpp::stop("'$min_value' must be a single numeric value");
                }
                min_value = tmp[0];
                if (Rcpp::NumericVector::is_na(min_value)) {
                    min_is_null = true;
                    min_value = -99999.0;  // unused
                }
            }

            if (fld_dom_defn.containsElementNamed("max_value") &&
                fld_dom_defn["max_value"] == R_NilValue) {

                max_is_null = true;
            }
            else if (!fld_dom_defn.containsElementNamed("max_value") ||
                     (!Rcpp::is<Rcpp::NumericVector>(
                        fld_dom_defn["max_value"]) &&
                      !Rcpp::is<Rcpp::IntegerVector>(
                        fld_dom_defn["max_value"]) &&
                      !Rcpp::is<Rcpp::LogicalVector>(
                        fld_dom_defn["max_value"]))) {

                GDALReleaseDataset(hDS);
                Rcpp::stop("'$max_value' must be integer, double or NULL");
            }
            else {
                Rcpp::NumericVector tmp = fld_dom_defn["max_value"];
                if (tmp.size() != 1) {
                    GDALReleaseDataset(hDS);
                    Rcpp::stop("'$max_value' must be a single numeric value");
                }
                max_value = tmp[0];
                if (Rcpp::NumericVector::is_na(max_value)) {
                    max_is_null = true;
                    max_value = -99999.0;  // unused
                }
            }

            OGRField sMin, sMax;
            if (eFieldType == OFTInteger) {
                if (min_value < INT32_MIN || max_value > INT32_MAX) {
                    GDALReleaseDataset(hDS);
                    Rcpp::stop("min/max out of range for OFTInteger");
                }
                sMin.Integer = static_cast<int>(min_value);
                sMax.Integer = static_cast<int>(max_value);
            }
            else {
                sMin.Real = min_value;
                sMax.Real = max_value;
            }

            OGRFieldDomainH hFldDom = nullptr;
            hFldDom = OGR_RangeFldDomain_Create(domain_name.c_str(),
                                                description.c_str(),
                                                eFieldType, eFieldSubType,
                                                min_is_null ? nullptr : &sMin,
                                                min_is_inclusive,
                                                max_is_null ? nullptr : &sMax,
                                                max_is_inclusive);

            if (hFldDom != nullptr) {
                OGR_FldDomain_SetSplitPolicy(hFldDom, eOFDSP);
                OGR_FldDomain_SetMergePolicy(hFldDom, eOFDMP);
                char *pszFailureReason = nullptr;
                ret = GDALDatasetAddFieldDomain(hDS, hFldDom,
                                                &pszFailureReason);
                if (pszFailureReason != nullptr) {
                    Rcpp::Rcout << pszFailureReason << std::endl;
                    VSIFree(pszFailureReason);
                }
                OGR_FldDomain_Destroy(hFldDom);
            }
        }

        // Integer64 range
        else if (eFieldType == OFTInteger64) {
            int64_t min_value = MIN_INTEGER64;
            int64_t max_value = MAX_INTEGER64;

            if (fld_dom_defn.containsElementNamed("min_value") &&
                fld_dom_defn["min_value"] == R_NilValue) {

                min_is_null = true;
            }
            else if (!fld_dom_defn.containsElementNamed("min_value") ||
                     (!Rcpp::is<Rcpp::NumericVector>(
                        fld_dom_defn["min_value"]) &&
                      !Rcpp::is<Rcpp::IntegerVector>(
                        fld_dom_defn["min_value"]))) {

                GDALReleaseDataset(hDS);
                Rcpp::stop("'$min_value' must be numeric (integer64) or NULL");
            }
            else {
                Rcpp::NumericVector tmp =
                    Rcpp::as<Rcpp::NumericVector>(fld_dom_defn["min_value"]);

                if (tmp.size() != 1) {
                    GDALReleaseDataset(hDS);
                    Rcpp::stop("'$min_value' must be a single numeric value");
                }

                if (Rcpp::isInteger64(tmp)) {
                    min_value = Rcpp::fromInteger64(tmp[0]);
                    if (ISNA_INTEGER64(min_value))
                        min_is_null = true;
                }
                else {
                    if (Rcpp::NumericVector::is_na(tmp[0]))
                        min_is_null = true;
                    else
                        min_value = static_cast<int64_t>(tmp[0]);
                }
            }

            if (fld_dom_defn.containsElementNamed("max_value") &&
                fld_dom_defn["max_value"] == R_NilValue) {

                max_is_null = true;
            }
            else if (!fld_dom_defn.containsElementNamed("max_value") ||
                     (!Rcpp::is<Rcpp::NumericVector>(
                        fld_dom_defn["max_value"]) &&
                      !Rcpp::is<Rcpp::IntegerVector>(
                        fld_dom_defn["max_value"]))) {

                GDALReleaseDataset(hDS);
                Rcpp::stop("'$max_value' must be numeric (integer64) or NULL");
            }
            else {
                Rcpp::NumericVector tmp =
                    Rcpp::as<Rcpp::NumericVector>(fld_dom_defn["max_value"]);

                if (tmp.size() != 1) {
                    GDALReleaseDataset(hDS);
                    Rcpp::stop("'$max_value' must be a single numeric value");
                }

                if (Rcpp::isInteger64(tmp)) {
                    max_value = Rcpp::fromInteger64(tmp[0]);
                    if (ISNA_INTEGER64(max_value))
                        max_is_null = true;
                }
                else {
                    if (Rcpp::NumericVector::is_na(tmp[0]))
                        max_is_null = true;
                    else
                        max_value = static_cast<int64_t>(tmp[0]);
                }
            }

            OGRField sMin, sMax;
            sMin.Integer64 = min_value;
            sMax.Integer64 = max_value;

            OGRFieldDomainH hFldDom = nullptr;
            hFldDom = OGR_RangeFldDomain_Create(domain_name.c_str(),
                                                description.c_str(),
                                                eFieldType, eFieldSubType,
                                                min_is_null ? nullptr : &sMin,
                                                min_is_inclusive,
                                                max_is_null ? nullptr : &sMax,
                                                max_is_inclusive);

            if (hFldDom != nullptr) {
                OGR_FldDomain_SetSplitPolicy(hFldDom, eOFDSP);
                OGR_FldDomain_SetMergePolicy(hFldDom, eOFDMP);
                char *pszFailureReason = nullptr;
                ret = GDALDatasetAddFieldDomain(hDS, hFldDom,
                                                &pszFailureReason);
                if (pszFailureReason != nullptr) {
                    Rcpp::Rcout << pszFailureReason << std::endl;
                    VSIFree(pszFailureReason);
                }
                OGR_FldDomain_Destroy(hFldDom);
            }
        }

        else {
            GDALReleaseDataset(hDS);
            if (eFieldType == OFTDateTime) {
                Rcpp::Rcout << "use '$type' RangeDateTime for OFTDateTime"
                    << std::endl;
            }
            Rcpp::stop(
                "'$field_type' must be OFTInteger, OFTInteger64 or OFTReal");
        }
    }

    // RangeDateTime
    else if (EQUAL(domain_type.c_str(), "rangedatetime")) {
        if (eFieldType != OFTDateTime) {
            GDALReleaseDataset(hDS);
            Rcpp::stop("'$field_type' must be OFTDateTime");
        }

        double min_value = 0.0;
        double max_value = 0.0;
        bool min_is_null = false;
        bool max_is_null = false;
        bool min_is_inclusive = true;
        bool max_is_inclusive = true;

        if (fld_dom_defn.containsElementNamed("min_value") &&
            fld_dom_defn["min_value"] == R_NilValue) {

            min_is_null = true;
        }
        else if (!fld_dom_defn.containsElementNamed("min_value") ||
                 !Rcpp::is<Rcpp::NumericVector>(fld_dom_defn["min_value"])) {

            GDALReleaseDataset(hDS);
            Rcpp::stop("'$min_value' must be 'numeric' of class 'POSIXct'");
        }
        else {
            Rcpp::NumericVector v_min = fld_dom_defn["min_value"];
            Rcpp::CharacterVector attr{};
            if (v_min.hasAttribute("class"))
                attr = Rcpp::wrap(v_min.attr("class"));
            if (std::find(attr.begin(), attr.end(), "POSIXct") == attr.end()) {
                GDALReleaseDataset(hDS);
                Rcpp::stop("'$min_value' must be 'numeric' of class 'POSIXct'");
            }
            min_value = v_min[0];
        }

        const int64_t nUnixTime_min = static_cast<int64_t>(min_value);
        struct tm brokendowntime;
        CPLUnixTimeToYMDHMS(nUnixTime_min, &brokendowntime);
        float sec = brokendowntime.tm_sec +
                    std::fmod(static_cast<float>(min_value), 1.0f);

        OGRField sMin;
        sMin.Date.Year = static_cast<GInt16>(brokendowntime.tm_year) + 1900;
        sMin.Date.Month = static_cast<GByte>(brokendowntime.tm_mon) + 1;
        sMin.Date.Day = static_cast<GByte>(brokendowntime.tm_mday);
        sMin.Date.Hour = static_cast<GByte>(brokendowntime.tm_hour);
        sMin.Date.Minute = static_cast<GByte>(brokendowntime.tm_min);
        sMin.Date.Second = static_cast<float>(sec);
        sMin.Date.TZFlag = 100;

        if (fld_dom_defn.containsElementNamed("max_value") &&
            fld_dom_defn["max_value"] == R_NilValue) {

            max_is_null = true;
        }
        else if (!fld_dom_defn.containsElementNamed("max_value") ||
                 !Rcpp::is<Rcpp::NumericVector>(fld_dom_defn["max_value"])) {

            GDALReleaseDataset(hDS);
            Rcpp::stop("'$max_value' must be 'numeric' of class 'POSIXct'");
        }
        else {
            Rcpp::NumericVector v_max = fld_dom_defn["max_value"];
            Rcpp::CharacterVector attr{};
            if (v_max.hasAttribute("class"))
                attr = Rcpp::wrap(v_max.attr("class"));
            if (std::find(attr.begin(), attr.end(), "POSIXct") == attr.end()) {
                GDALReleaseDataset(hDS);
                Rcpp::stop("'$max_value' must be 'numeric' of class 'POSIXct'");
            }
            max_value = v_max[0];
        }

        const int64_t nUnixTime_max = static_cast<int64_t>(max_value);
        CPLUnixTimeToYMDHMS(nUnixTime_max, &brokendowntime);
        sec = brokendowntime.tm_sec +
                std::fmod(static_cast<float>(max_value), 1.0f);

        OGRField sMax;
        sMax.Date.Year = static_cast<GInt16>(brokendowntime.tm_year) + 1900;
        sMax.Date.Month = static_cast<GByte>(brokendowntime.tm_mon) + 1;
        sMax.Date.Day = static_cast<GByte>(brokendowntime.tm_mday);
        sMax.Date.Hour = static_cast<GByte>(brokendowntime.tm_hour);
        sMax.Date.Minute = static_cast<GByte>(brokendowntime.tm_min);
        sMax.Date.Second = static_cast<float>(sec);
        sMax.Date.TZFlag = 100;

        if (!fld_dom_defn.containsElementNamed("min_is_inclusive") ||
            fld_dom_defn["min_is_inclusive"] == R_NilValue ||
            !Rcpp::is<Rcpp::LogicalVector>(
                fld_dom_defn["min_is_inclusive"])) {

            min_is_inclusive = true;
        }
        else {
            Rcpp::LogicalVector tmp = Rcpp::as<Rcpp::LogicalVector>(
                                        fld_dom_defn["min_is_inclusive"]);

            if (tmp[0] == FALSE)
                min_is_inclusive = false;
        }

        if (!fld_dom_defn.containsElementNamed("max_is_inclusive") ||
            fld_dom_defn["max_is_inclusive"] == R_NilValue ||
            !Rcpp::is<Rcpp::LogicalVector>(
                fld_dom_defn["max_is_inclusive"])) {

            max_is_inclusive = true;
        }
        else {
            Rcpp::LogicalVector tmp = Rcpp::as<Rcpp::LogicalVector>(
                                        fld_dom_defn["max_is_inclusive"]);
            if (tmp[0] == FALSE)
                max_is_inclusive = false;
        }

        OGRFieldDomainH hFldDom = nullptr;
        hFldDom = OGR_RangeFldDomain_Create(domain_name.c_str(),
                                            description.c_str(),
                                            eFieldType, eFieldSubType,
                                            min_is_null ? nullptr : &sMin,
                                            min_is_inclusive,
                                            max_is_null ? nullptr : &sMax,
                                            max_is_inclusive);

        if (hFldDom != nullptr) {
            OGR_FldDomain_SetSplitPolicy(hFldDom, eOFDSP);
            OGR_FldDomain_SetMergePolicy(hFldDom, eOFDMP);
            char *pszFailureReason = nullptr;
            ret = GDALDatasetAddFieldDomain(hDS, hFldDom, &pszFailureReason);
            if (pszFailureReason != nullptr) {
                Rcpp::Rcout << pszFailureReason << std::endl;
                VSIFree(pszFailureReason);
            }
            OGR_FldDomain_Destroy(hFldDom);
        }
    }

    // GLOB
    else if (EQUAL(domain_type.c_str(), "glob")) {
        if (eFieldType != OFTString) {
            GDALReleaseDataset(hDS);
            Rcpp::stop("'$field_type' must be OFTString");
        }

        if (!fld_dom_defn.containsElementNamed("glob") ||
            fld_dom_defn["glob"] == R_NilValue ||
            !Rcpp::is<Rcpp::CharacterVector>(fld_dom_defn["glob"])) {

            GDALReleaseDataset(hDS);
            Rcpp::stop("'$glob' must be a character string");
        }

        Rcpp::CharacterVector tmp = fld_dom_defn["glob"];
        if (tmp.size() != 1) {
            GDALReleaseDataset(hDS);
            Rcpp::stop("'$glob' must be a character string");
        }

        std::string glob = Rcpp::as<std::string>(tmp[0]);

        OGRFieldDomainH hFldDom = nullptr;
        hFldDom = OGR_GlobFldDomain_Create(domain_name.c_str(),
                                           description.c_str(),
                                           eFieldType, eFieldSubType,
                                           glob.c_str());

        if (hFldDom != nullptr) {
            OGR_FldDomain_SetSplitPolicy(hFldDom, eOFDSP);
            OGR_FldDomain_SetMergePolicy(hFldDom, eOFDMP);
            char *pszFailureReason = nullptr;
            ret = GDALDatasetAddFieldDomain(hDS, hFldDom, &pszFailureReason);
            if (pszFailureReason != nullptr) {
                Rcpp::Rcout << pszFailureReason << std::endl;
                VSIFree(pszFailureReason);
            }
            OGR_FldDomain_Destroy(hFldDom);
        }
    }

    // unrecognized type
    else {
        GDALReleaseDataset(hDS);
        Rcpp::stop("unrecognized domain type");
    }

    // finished
    GDALReleaseDataset(hDS);
    return ret;
#endif
}

//' Delete a field domain from a dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_delete_field_domain")]]
bool ogr_ds_delete_field_domain(const std::string &dsn,
                                const std::string &domain_name) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 5, 0)
    Rcpp::stop("'ogr_ds_delete_field_domain() requires GDAL >= 3.5");

#else
    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;

    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr) {
        Rcpp::warning("failed to open dataset");
        return false;
    }

    bool ret = false;
    ret = GDALDatasetDeleteFieldDomain(hDS, domain_name.c_str(), nullptr);
    GDALReleaseDataset(hDS);
    return ret;
#endif
}

//' Does layer exist
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_exists")]]
bool ogr_layer_exists(const std::string &dsn, const std::string &layer) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
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
SEXP ogr_layer_test_cap(const std::string &dsn, const std::string &layer,
                        bool with_update = true) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    if (with_update)
        hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                         nullptr, nullptr, nullptr);
    else
        hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR,
                         nullptr, nullptr, nullptr);

    if (layer == "")
        hLayer = GDALDatasetGetLayer(hDS, 0);
    else
        hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    CPLPopErrorHandler();

    if (hDS == nullptr || hLayer == nullptr)
        return R_NilValue;
    else
        GDALReleaseDataset(hDS);

    GDALVector lyr = GDALVector(dsn_in.c_str(), layer, !with_update);
    Rcpp::List cap = lyr.testCapability();
    lyr.close();

    return cap;
}

// Internal wrapper of GDALDatasetCreateLayer()
OGRLayerH CreateLayer_(GDALDatasetH hDS, const std::string &layer,
                       Rcpp::Nullable<Rcpp::List> layer_defn,
                       const std::string &geom_type,
                       const std::string &srs,
                       Rcpp::Nullable<Rcpp::CharacterVector> options) {

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
            Rcpp::stop("'layer_defn' does not have a geometry field definition");
    }

    OGRwkbGeometryType eGeomType = getWkbGeomType_(geom_type_in);
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
                bool is_unique = false;
                std::string default_value = "";
                std::string domain_name = "";
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
                    if (fld.containsElementNamed("is_unique")) {
                        Rcpp::LogicalVector fld_is_unique = fld["is_unique"];
                        is_unique = Rcpp::is_true(Rcpp::all(fld_is_unique));
                    }
                    if (fld.containsElementNamed("default"))
                        default_value = Rcpp::as<std::string>(fld["default"]);
                    if (fld.containsElementNamed("domain"))
                        domain_name = Rcpp::as<std::string>(fld["default"]);

                    if (!CreateField_(hDS, hLayer, fld_name, fld_type,
                                      fld_subtype, fld_width, fld_precision,
                                      is_nullable, is_unique, default_value,
                                      domain_name)) {

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
                            eThisGeomType = getWkbGeomType_(this_geom_type);
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

                    if (!CreateGeomField_(hDS, hLayer, fld_name, eThisGeomType,
                                          srs, is_nullable)) {

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

// Create a layer in a vector dataset
//
// Implemented as a GDALVector object factory registered in
// RCPP_MODULE(mod_GDALVector), see src/gdalvector.cpp.
// Unique function signature based on number of parameters.
// Called in R with `lyr <- new(GDALVector, ...)` giving all 7 parameters,
// as in R/ogr_manage.R (param `reserved1` currently ignored).
GDALVector *ogr_layer_create(
        const std::string &dsn, const std::string &layer,
        const Rcpp::Nullable<Rcpp::List> &layer_defn,
        const std::string &geom_type, const std::string &srs,
        const Rcpp::Nullable<Rcpp::CharacterVector> &options,
        bool reserved1) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;

    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);

    if (hDS == nullptr)
        Rcpp::stop("failed to open 'dsn' for update");

    if (!GDALDatasetTestCapability(hDS, ODsCCreateLayer)) {
        GDALReleaseDataset(hDS);
        Rcpp::stop("the data source does not have CreateLayer capability");
    }

    hLayer = CreateLayer_(hDS, layer, layer_defn, geom_type, srs, options);

    if (hLayer == nullptr) {
        GDALReleaseDataset(hDS);
        Rcpp::stop("failed to create layer");
    }
    else {
        GDALVector *lyr = new GDALVector;
        lyr->setDsn_(dsn_in);
        lyr->setGDALDatasetH_(hDS, true);
        lyr->setOGRLayerH_(hLayer, layer);
        lyr->setFieldNames_();
        return lyr;
    }
}

//' Rename a layer in a vector dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_rename")]]
bool ogr_layer_rename(const std::string &dsn, const std::string &layer,
                      const std::string &new_name) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 5, 0)
    Rcpp::stop("ogr_layer_rename() requires GDAL >= 3.5");

#else
    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH  hLayer = nullptr;

    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);

    if (hDS == nullptr)
        return false;

    hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
    if (hLayer == nullptr) {
        Rcpp::Rcerr << "failed to access 'layer'\n";
        GDALReleaseDataset(hDS);
        return false;
    }

    if (!OGR_L_TestCapability(hLayer, OLCRename)) {
        Rcpp::Rcerr << "layer does not have Rename capability\n";
        GDALReleaseDataset(hDS);
        return false;
    }

    bool ret = false;
    if (OGR_L_Rename(hLayer, new_name.c_str()) == OGRERR_NONE)
        ret = true;

    GDALReleaseDataset(hDS);
    return ret;

#endif
}

//' Delete a layer in a vector dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_delete")]]
bool ogr_layer_delete(const std::string &dsn, const std::string &layer) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
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
SEXP ogr_layer_field_names(const std::string &dsn, const std::string &layer) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr)
        return R_NilValue;
    if (layer == "")
        hLayer = GDALDatasetGetLayer(hDS, 0);
    else
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
int ogr_field_index(const std::string &dsn, const std::string &layer,
                    const std::string &fld_name) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;
    int iField;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (hDS == nullptr)
        return -1;
    if (layer == "")
        hLayer = GDALDatasetGetLayer(hDS, 0);
    else
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
bool CreateField_(GDALDatasetH hDS, OGRLayerH hLayer,
                  const std::string &fld_name, const std::string &fld_type,
                  const std::string &fld_subtype = "OFSTNone",
                  int fld_width = 0,
                  int fld_precision = 0,
                  bool is_nullable = true,
                  bool is_unique = false,
                  const std::string &default_value = "",
                  const std::string &domain_name = "") {

    if (hDS == nullptr || hLayer == nullptr)
        return false;

    OGRFieldDefnH hFieldDefn = nullptr;
    OGRFieldType eFieldType = getOFT_(fld_type);
    OGRFieldSubType eFieldSubType = getOFTSubtype_(fld_subtype);
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

        if (default_value != "") {
            if (CPLFetchBool(papszMD, GDAL_DCAP_DEFAULT_FIELDS, false))
                OGR_Fld_SetDefault(hFieldDefn, default_value.c_str());
            else
                Rcpp::warning(
                    "default field value not supported by the format driver");
        }

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 2, 0)
        if (is_unique) {
            if (CPLFetchBool(papszMD, GDAL_DCAP_UNIQUE_FIELDS, false))
                OGR_Fld_SetUnique(hFieldDefn, true);
            else
                Rcpp::warning(
                    "unique constraint not supported by the format driver");
        }
#endif

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 3, 0)
        if (domain_name != "") {
            if (GDALDatasetTestCapability(hDS, ODsCAddFieldDomain))
                OGR_Fld_SetDomainName(hFieldDefn, domain_name.c_str());
            else
                Rcpp::warning(
                    "add field domain not supported by the format driver");
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
bool ogr_field_create(const std::string &dsn, const std::string &layer,
                      const std::string &fld_name, const std::string &fld_type,
                      const std::string &fld_subtype = "OFSTNone",
                      int fld_width = 0,
                      int fld_precision = 0,
                      bool is_nullable = true,
                      bool is_unique = false,
                      const std::string &default_value = "",
                      const std::string &domain_name = "") {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;
    int iField;

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);

    if (hDS == nullptr)
        return false;

    if (layer == "")
        hLayer = GDALDatasetGetLayer(hDS, 0);
    else
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

    bool ret = CreateField_(hDS, hLayer, fld_name, fld_type, fld_subtype,
                            fld_width, fld_precision, is_nullable, is_unique,
                            default_value, domain_name);

    GDALReleaseDataset(hDS);
    return ret;
}

// Internal wrapper of OGR_L_CreateGeomField()
bool CreateGeomField_(GDALDatasetH hDS, OGRLayerH hLayer,
                      const std::string &fld_name,
                      OGRwkbGeometryType eGeomType,
                      const std::string &srs = "",
                      bool is_nullable = true) {

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
bool ogr_geom_field_create(const std::string &dsn, const std::string &layer,
                           const std::string &fld_name,
                           const std::string &geom_type,
                           const std::string &srs = "",
                           bool is_nullable = true) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    OGRLayerH hLayer = nullptr;
    OGRFeatureDefnH hFDefn = nullptr;
    int iField;

    OGRwkbGeometryType eGeomType = getWkbGeomType_(geom_type);
    if (eGeomType == wkbUnknown && !EQUALN(geom_type.c_str(), "UNKNOWN", 7))
        Rcpp::stop("'geom_type' not recognized");

    CPLPushErrorHandler(CPLQuietErrorHandler);
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);

    if (hDS == nullptr)
        return false;

    if (layer == "")
        hLayer = GDALDatasetGetLayer(hDS, 0);
    else
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

    bool ret = CreateGeomField_(hDS, hLayer, fld_name, eGeomType, srs,
                                is_nullable);

    GDALReleaseDataset(hDS);
    return ret;
}

//' Rename an attribute field on a vector layer
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_field_rename")]]
bool ogr_field_rename(const std::string &dsn, const std::string &layer,
                      const std::string &fld_name,
                      const std::string &new_name) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);
    if (hDS == nullptr) {
        Rcpp::Rcerr << "failed to open 'dsn' for update\n";
        return false;
    }

    OGRLayerH  hLayer = nullptr;
    if (layer == "")
        hLayer = GDALDatasetGetLayer(hDS, 0);
    else
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

//' Set the field domain of an existing attribute field on a vector layer
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_field_set_domain_name")]]
bool ogr_field_set_domain_name(const std::string &dsn,
                               const std::string &layer,
                               const std::string &fld_name,
                               const std::string &domain_name) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
    Rcpp::stop("'ogr_field_set_domain_name() requires GDAL >= 3.3");

#else
    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);
    if (hDS == nullptr) {
        Rcpp::Rcerr << "failed to open 'dsn' for update\n";
        return false;
    }

    OGRLayerH  hLayer = nullptr;
    if (layer == "")
        hLayer = GDALDatasetGetLayer(hDS, 0);
    else
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
    hNewFieldDefn = OGR_Fld_Create("temp", eFieldType);
    OGR_Fld_SetDomainName(hNewFieldDefn, domain_name.c_str());
    OGRErr err = OGR_L_AlterFieldDefn(hLayer, iField, hNewFieldDefn,
                                      ALTER_DOMAIN_FLAG);
    OGR_Fld_Destroy(hNewFieldDefn);
    GDALReleaseDataset(hDS);

    if (err != OGRERR_NONE) {
        Rcpp::Rcerr << "failed to set field domain name\n";
        return false;
    }
    else {
        return true;
    }
#endif
}

//' Delete an attribute field on a vector layer
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_field_delete")]]
bool ogr_field_delete(const std::string &dsn, const std::string &layer,
                      const std::string &fld_name) {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
    GDALDatasetH hDS = nullptr;
    hDS = GDALOpenEx(dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                     nullptr, nullptr, nullptr);
    if (hDS == nullptr) {
        Rcpp::Rcerr << "failed to open 'dsn' for update\n";
        return false;
    }

    OGRLayerH  hLayer = nullptr;
    if (layer == "")
        hLayer = GDALDatasetGetLayer(hDS, 0);
    else
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
SEXP ogr_execute_sql(const std::string &dsn, const std::string &sql,
                     const std::string &spatial_filter = "",
                     const std::string &dialect = "") {

    std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));
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
