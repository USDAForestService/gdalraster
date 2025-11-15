/* R interface to GDALAlgorithm and related classes
   Chris Toney <jctoney at gmail.com>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include "gdalalg.h"

#if __has_include(<gdalalgorithm.h>)
    #include <gdalalgorithm.h>
#endif
#include <gdal.h>
#include <cpl_port.h>
#include <cpl_string.h>

#include <Rcpp.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "gdalraster.h"
#include "rcpp_util.h"

#define GDALALG_MIN_GDAL_ GDAL_COMPUTE_VERSION(3, 11, 3)

constexpr char GDALALG_MIN_GDAL_MSG_[] =
    "GDAL CLI bindings require GDAL >= 3.11.3";

#if GDAL_VERSION_NUM >= GDALALG_MIN_GDAL_
constexpr R_xlen_t CMD_TOKENS_MAX_ = 6;  // used here for a rough bound check

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
// https://lists.osgeo.org/pipermail/gdal-dev/2025-August/060818.html
// https://github.com/OSGeo/gdal/pull/12853 for GDAL >= 3.12
struct GDALAlgorithmArgHS
{
    GDALAlgorithmArg *ptr = nullptr;

    explicit GDALAlgorithmArgHS(GDALAlgorithmArg *arg) : ptr(arg)
    {
    }
};
#endif  // GDAL < 3.12
#endif  // GDALALG_MIN_GDAL_

#if GDAL_VERSION_NUM >= GDALALG_MIN_GDAL_
// internal helper to get subalgorithm names, descriptions and URLs,
// potentially filtering on 'contains'
void append_subalg_names_desc_(const GDALAlgorithmH alg,
                               const std::string &cmd_str,
                               std::vector<std::string> &names,
                               std::vector<std::string> &desc,
                               std::vector<std::string> &urls,
                               const std::string &contains,
                               bool console_out) {

    char **subnames = GDALAlgorithmGetSubAlgorithmNames(alg);
    for (int i = 0; i < CSLCount(subnames); ++i) {
        GDALAlgorithmH subalg = nullptr;
        subalg = GDALAlgorithmInstantiateSubAlgorithm(alg, subnames[i]);
        if (!subalg) {
            Rcpp::Rcout << "failed to instantiate alg name: " << subnames[i] <<
                "\n";
            continue;
        }

        std::string this_cmd_str = cmd_str + " " + GDALAlgorithmGetName(subalg);
        bool console_out_this = true;
        if (contains == "" ||
            this_cmd_str.find(contains) != std::string::npos) {

            names.push_back(this_cmd_str);
            desc.push_back(GDALAlgorithmGetDescription(subalg));
            urls.push_back(GDALAlgorithmGetHelpFullURL(subalg));
        }
        else {
            console_out_this = false;
        }

        if (console_out && console_out_this) {
            Rcpp::Rcout << this_cmd_str.c_str() << ":\n";
            Rcpp::Rcout << GDALAlgorithmGetDescription(subalg) << "\n";
            if (!EQUAL(GDALAlgorithmGetHelpFullURL(subalg), ""))
                Rcpp::Rcout << GDALAlgorithmGetHelpFullURL(subalg) << "\n";
            Rcpp::Rcout << "\n";
        }

        if (GDALAlgorithmHasSubAlgorithms(subalg)) {
            append_subalg_names_desc_(subalg, this_cmd_str, names, desc, urls,
                                      contains, console_out);
        }

        GDALAlgorithmRelease(subalg);
    }

    CSLDestroy(subnames);
}
#endif  // GDALALG_MIN_GDAL_

//  public R wrapper in R/gdal_cli.R
//' @noRd
// [[Rcpp::export(name = ".gdal_commands")]]
Rcpp::DataFrame gdal_commands(const std::string &contains, bool recurse,
                              bool console_out) {

#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);

#else
    GDALAlgorithmRegistryH reg = nullptr;
    reg = GDALGetGlobalAlgorithmRegistry();
    if (!reg)
        Rcpp::stop("failed to obtain global algorithm registry");

    GDALAlgorithmH gdal_alg = nullptr;
    gdal_alg = GDALAlgorithmRegistryInstantiateAlg(
        reg, GDALGlobalAlgorithmRegistry::ROOT_ALG_NAME);

    if (!gdal_alg) {
        GDALAlgorithmRegistryRelease(reg);
        Rcpp::stop("failed to instantiate \"gdal\" entry point");
    }

    char **names = GDALAlgorithmGetSubAlgorithmNames(gdal_alg);
    const int num_names = CSLCount(names);
    if (num_names == 0) {
        GDALAlgorithmRegistryRelease(reg);
        Rcpp::stop("failed to obtain top-level algorithm names");
    }

    std::vector<std::string> cmd_names = {};
    std::vector<std::string> cmd_descriptions = {};
    std::vector<std::string> cmd_urls = {};

    std::string contains_in = str_tolower_(contains);

    for (int i = 0; i < num_names; ++i) {
        GDALAlgorithmH alg = nullptr;
        alg = GDALAlgorithmRegistryInstantiateAlg(reg, names[i]);
        if (!alg) {
            Rcpp::Rcout << "failed to instantiate alg name: " << names[i] <<
                "\n";
            continue;
        }

        bool console_out_this = true;
        if (contains_in == "" ||
            std::string(names[i]).find(contains_in) != std::string::npos) {

            cmd_names.push_back(names[i]);
            cmd_descriptions.push_back(GDALAlgorithmGetDescription(alg));
            cmd_urls.push_back(GDALAlgorithmGetHelpFullURL(alg));
        }
        else {
            console_out_this = false;
        }

        if (console_out && console_out_this) {
            Rcpp::Rcout << names[i] << ":\n";
            Rcpp::Rcout << GDALAlgorithmGetDescription(alg) << "\n";
            if (!EQUAL(GDALAlgorithmGetHelpFullURL(alg), ""))
                Rcpp::Rcout << GDALAlgorithmGetHelpFullURL(alg) << "\n";
            Rcpp::Rcout << "\n";
        }

        if (recurse && GDALAlgorithmHasSubAlgorithms(alg)) {
            append_subalg_names_desc_(alg, std::string(names[i]),
                                      cmd_names, cmd_descriptions,
                                      cmd_urls, contains_in, console_out);
        }

        GDALAlgorithmRelease(alg);
    }

    CSLDestroy(names);
    GDALAlgorithmRelease(gdal_alg);
    GDALAlgorithmRegistryRelease(reg);

    Rcpp::DataFrame df = Rcpp::DataFrame::create(
        Rcpp::Named("command") = Rcpp::wrap(cmd_names),
        Rcpp::Named("description") = Rcpp::wrap(cmd_descriptions),
        Rcpp::Named("URL") = Rcpp::wrap(cmd_urls));

    return df;
#endif  // GDALALG_MIN_GDAL_
}

//  public R wrapper in R/gdal_cli.R
//' @noRd
// [[Rcpp::export(name = ".gdal_global_reg_names")]]
Rcpp::CharacterVector gdal_global_reg_names() {

    Rcpp::CharacterVector out = Rcpp::CharacterVector::create();

#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::Rcout << GDALALG_MIN_GDAL_MSG_ << "\n";
    return out;

#else
    GDALAlgorithmRegistryH reg = nullptr;
    reg = GDALGetGlobalAlgorithmRegistry();
    if (!reg) {
        Rcpp::Rcout << "failed to obtain global algorithm registry\n";
        return out;
    }

    char **names = GDALAlgorithmRegistryGetAlgNames(reg);
    for (int i = 0; i < CSLCount(names); ++i) {
        out.push_back(names[i]);
    }

    CSLDestroy(names);
    GDALAlgorithmRegistryRelease(reg);
    return out;
#endif  // GDALALG_MIN_GDAL_
}

// ****************************************************************************
//  Implementation of exposed class GDALAlg, which wraps GDALAlgorithm and
//  its related classes GDALAlgorithmArg and GDALArgDatasetValue.
//  Documented in R/gdalalg.R.
// ****************************************************************************

GDALAlg::GDALAlg()
        : GDALAlg("gdal", Rcpp::CharacterVector::create())  {}

GDALAlg::GDALAlg(const Rcpp::CharacterVector &cmd)
        : GDALAlg(cmd, Rcpp::CharacterVector::create()) {}

GDALAlg::GDALAlg(const Rcpp::CharacterVector &cmd, const Rcpp::RObject &args) {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (cmd.size() == 0 ||
        (cmd.size() == 1 && EQUAL(cmd[0], "")) ||
        (cmd.size() == 1 && Rcpp::String(cmd[0]) == NA_STRING)) {

        Rcpp::stop("'cmd' is empty");
    }
    else if (Rcpp::is_true(Rcpp::any(Rcpp::is_na(cmd)))) {
        Rcpp::stop("'cmd' contains one or more NA values");
    }
    else if (cmd.size() > CMD_TOKENS_MAX_) {
        Rcpp::stop("number of elements in 'cmd' is out of range");
    }

    Rcpp::CharacterVector cmd_in = enc_to_utf8_(cmd);
    m_cmd_str = "";
    for (R_xlen_t i = 0; i < cmd_in.size(); ++i) {
        m_cmd_str += Rcpp::as<std::string>(cmd_in[i]);
        if (i < cmd_in.size() - 1) {
            m_cmd_str += " ";
        }
    }

    if (has_space_char_(m_cmd_str)) {
        m_cmd = Rcpp::CharacterVector::create();
        std::stringstream ss(m_cmd_str);
        std::string token;
        while (ss >> token) {
            m_cmd.push_back(token);
        }
    }
    else {
        m_cmd = cmd_in;
    }

    if (!args.isNULL()) {
        if (Rcpp::is<Rcpp::CharacterVector>(args)) {
            Rcpp::CharacterVector args_in(args);
            m_args = enc_to_utf8_(args_in);
        }
        else if (Rcpp::is<Rcpp::List>(args)) {
            m_args = parseListArgs_(Rcpp::as<Rcpp::List>(args));
        }
        else {
            Rcpp::stop("'args' must be a character vector or named list");
        }
    }
    else {
        m_args = Rcpp::CharacterVector::create();
    }

    instantiateAlg_();
#endif  // GDALALG_MIN_GDAL_
}

GDALAlg::~GDALAlg() {
#if GDAL_VERSION_NUM >= GDALALG_MIN_GDAL_
    if (m_hActualAlg) {
        if (m_hasRun && !m_hasFinalized)
            GDALAlgorithmFinalize(m_hActualAlg);
        GDALAlgorithmRelease(m_hActualAlg);
    }

    if (m_hAlg)
        GDALAlgorithmRelease(m_hAlg);

#endif  // GDALALG_MIN_GDAL_
}

Rcpp::List GDALAlg::info() const {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (!m_hAlg)
        Rcpp::stop("algorithm not instantiated");

    Rcpp::List alg_info = Rcpp::List::create();
    const GDALAlgorithmH &alg = m_hActualAlg ? m_hActualAlg : m_hAlg;

    alg_info.push_back(GDALAlgorithmGetName(alg), "name");
    alg_info.push_back(m_cmd_str, "full_path");
    alg_info.push_back(GDALAlgorithmGetDescription(alg), "description");
    alg_info.push_back(GDALAlgorithmGetLongDescription(alg),
                       "long_description");
    alg_info.push_back(GDALAlgorithmGetHelpFullURL(alg), "URL");
    alg_info.push_back(GDALAlgorithmHasSubAlgorithms(alg),
                       "has_subalgorithms");

    if (GDALAlgorithmHasSubAlgorithms(alg)) {
        char **papszNames = GDALAlgorithmGetSubAlgorithmNames(alg);
        int nCount = CSLCount(papszNames);
        if (nCount > 0) {
            std::vector<std::string> names(papszNames, papszNames + nCount);
            alg_info.push_back(Rcpp::wrap(names), "subalgorithm_names");
        }
        else {
            alg_info.push_back(Rcpp::CharacterVector::create(),
                               "subalgorithm_names");
        }
        CSLDestroy(papszNames);
    }
    else {
        alg_info.push_back(Rcpp::CharacterVector::create(),
                           "subalgorithm_names");
    }

    char **papszArgNames = GDALAlgorithmGetArgNames(alg);
    int nCount = CSLCount(papszArgNames);
    if (nCount > 0) {
        std::vector<std::string> names(papszArgNames, papszArgNames + nCount);

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
        // see https://lists.osgeo.org/pipermail/gdal-dev/2025-August/060825.html
        names.erase(std::remove(names.begin(), names.end(), "help-doc"),
                    names.end());
#else
        // see https://github.com/OSGeo/gdal/pull/12890
        std::vector<std::string> hidden_args = {};
        for (const std::string &nm : names) {
            auto hArg = GDALAlgorithmGetArg(alg, nm.c_str());
            if (!hArg)
                continue;

            if (GDALAlgorithmArgIsHidden(hArg))
                hidden_args.push_back(nm);

            GDALAlgorithmArgRelease(hArg);
        }

        for (const std::string &nm : hidden_args) {
            names.erase(std::remove(names.begin(), names.end(), nm.c_str()),
                        names.end());
        }
#endif  // GDAL < 3.12

        alg_info.push_back(Rcpp::wrap(names), "arg_names");
    }
    else {
        alg_info.push_back(Rcpp::CharacterVector::create(), "arg_names");
    }
    CSLDestroy(papszArgNames);

    return alg_info;
#endif  // GDALALG_MIN_GDAL_
}

Rcpp::List GDALAlg::argInfo(const Rcpp::String &arg_name) const {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (!m_hAlg)
        Rcpp::stop("algorithm not instantiated");

    if (!arg_name.get_cstring() ||  arg_name == "" || arg_name == NA_STRING)
        Rcpp::stop("'arg_name' is required");

    Rcpp::List arg_info = Rcpp::List::create();

    GDALAlgorithmArgH hArg = nullptr;
    hArg = GDALAlgorithmGetArg(m_hActualAlg ? m_hActualAlg : m_hAlg,
                               arg_name.get_cstring());

    if (!hArg)
        Rcpp::stop("failed to obtain GDALAlgorithmArg object for 'arg_name'");

    arg_info.push_back(GDALAlgorithmArgGetName(hArg), "name");

    GDALAlgorithmArgType eType = GDALAlgorithmArgGetType(hArg);
    std::string arg_type = str_toupper_(GDALAlgorithmArgTypeName(eType));
    arg_info.push_back(arg_type, "type");

    arg_info.push_back(GDALAlgorithmArgGetDescription(hArg), "description");
    arg_info.push_back(GDALAlgorithmArgGetShortName(hArg), "short_name");

    char **papszAliases = GDALAlgorithmArgGetAliases(hArg);
    int nCount = CSLCount(papszAliases);
    if (papszAliases && nCount > 0) {
        Rcpp::CharacterVector v(papszAliases, papszAliases + nCount);
        arg_info.push_back(v, "aliases");
    }
    else {
        Rcpp::CharacterVector v = Rcpp::CharacterVector::create();
        arg_info.push_back(v, "aliases");
    }
    CSLDestroy(papszAliases);

    arg_info.push_back(GDALAlgorithmArgGetMetaVar(hArg), "meta_var");
    arg_info.push_back(GDALAlgorithmArgGetCategory(hArg), "category");
    arg_info.push_back(GDALAlgorithmArgIsPositional(hArg), "is_positional");
    arg_info.push_back(GDALAlgorithmArgIsRequired(hArg), "is_required");
    arg_info.push_back(GDALAlgorithmArgGetMinCount(hArg), "min_count");
    arg_info.push_back(GDALAlgorithmArgGetMaxCount(hArg), "max_count");
    arg_info.push_back(GDALAlgorithmArgGetPackedValuesAllowed(hArg),
                       "packed_values_allowed");
    arg_info.push_back(GDALAlgorithmArgGetRepeatedArgAllowed(hArg),
                       "repeated_arg_allowed");

    if (eType == GAAT_STRING || eType == GAAT_STRING_LIST) {
        char **papszChoices = GDALAlgorithmArgGetChoices(hArg);
        int nCount = CSLCount(papszChoices);
        if (papszChoices && nCount > 0) {
            Rcpp::CharacterVector v(papszChoices, papszChoices + nCount);
            arg_info.push_back(v, "choices");
        }
        else {
            arg_info.push_back(Rcpp::CharacterVector::create(), "choices");
        }
        CSLDestroy(papszChoices);
    }
    else {
        arg_info.push_back(Rcpp::CharacterVector::create(), "choices");
    }

    arg_info.push_back(GDALAlgorithmArgIsExplicitlySet(hArg),
                       "is_explicitly_set");
    arg_info.push_back(GDALAlgorithmArgHasDefaultValue(hArg),
                       "has_default_value");

    if (GDALAlgorithmArgHasDefaultValue(hArg)) {
    // see https://lists.osgeo.org/pipermail/gdal-dev/2025-August/060818.html
    // see https://github.com/OSGeo/gdal/pull/12853 for GDAL >= 3.12

        if (eType == GAAT_STRING) {
    #if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
            const std::string &val = hArg->ptr->GetDefault<std::string>();
    #else
            const char *val = GDALAlgorithmArgGetDefaultAsString(hArg);
    #endif
            arg_info.push_back(Rcpp::wrap(val), "default_value");
        }

        else if (eType == GAAT_BOOLEAN) {
    #if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
            const bool &val = hArg->ptr->GetDefault<bool>();
    #else
            const bool val = GDALAlgorithmArgGetDefaultAsBoolean(hArg);
    #endif
            arg_info.push_back(Rcpp::wrap(val), "default_value");
        }

        else if (eType == GAAT_INTEGER) {
    #if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
            const int &val = hArg->ptr->GetDefault<int>();
    #else
            const int val = GDALAlgorithmArgGetDefaultAsInteger(hArg);
    #endif
            arg_info.push_back(Rcpp::wrap(val), "default_value");
        }

        else if (eType == GAAT_REAL) {
    #if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
            const double &val = hArg->ptr->GetDefault<double>();
    #else
            const double val = GDALAlgorithmArgGetDefaultAsDouble(hArg);
    #endif
            arg_info.push_back(Rcpp::wrap(val), "default_value");
        }

        else if (eType == GAAT_STRING_LIST) {
    #if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
            const auto &val =
                hArg->ptr->GetDefault<std::vector<std::string>>();

            arg_info.push_back(Rcpp::wrap(val), "default_value");
    #else
            char **papszValue = nullptr;
            papszValue = GDALAlgorithmArgGetDefaultAsStringList(hArg);
            int nCount = CSLCount(papszValue);
            Rcpp::CharacterVector val;
            if (papszValue && nCount > 0)
                val = Rcpp::CharacterVector(papszValue, papszValue + nCount);
            else
                val = Rcpp::CharacterVector::create();
            CSLDestroy(papszValue);
            arg_info.push_back(val, "default_value");
    #endif
        }

        else if (eType == GAAT_INTEGER_LIST) {
    #if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
            const auto &val =
                hArg->ptr->GetDefault<std::vector<int>>();

            arg_info.push_back(Rcpp::wrap(val), "default_value");
    #else
            size_t nCount = 0;
            const int *panValue =
                GDALAlgorithmArgGetDefaultAsIntegerList(hArg, &nCount);
            Rcpp::IntegerVector val;
            if (panValue && nCount > 0)
                val = Rcpp::IntegerVector(panValue, panValue + nCount);
            else
                val = Rcpp::IntegerVector::create();
            arg_info.push_back(val, "default_value");
    #endif
        }

        else if (eType == GAAT_REAL_LIST) {
    #if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
            const auto &val =
                hArg->ptr->GetDefault<std::vector<double>>();

            arg_info.push_back(Rcpp::wrap(val), "default_value");
    #else
            size_t nCount = 0;
            const double *padfValue =
                GDALAlgorithmArgGetDefaultAsDoubleList(hArg, &nCount);
            Rcpp::NumericVector val;
            if (padfValue && nCount > 0)
                val = Rcpp::NumericVector(padfValue, padfValue + nCount);
            else
                val = Rcpp::NumericVector::create();
            arg_info.push_back(val, "default_value");
    #endif
        }

        else {
            arg_info.push_back(R_NilValue, "default_value");
        }
    }
    else {
        arg_info.push_back(R_NilValue, "default_value");
    }

    #if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
    arg_info.push_back(GDALAlgorithmArgIsHiddenForCLI(hArg),
                       "is_hidden_for_cli");
    arg_info.push_back(GDALAlgorithmArgIsOnlyForCLI(hArg),
                       "is_only_for_cli");
    #else
    arg_info.push_back(GDALAlgorithmArgIsHiddenForAPI(hArg),
                       "is_hidden_for_api");
    arg_info.push_back(GDALAlgorithmArgIsHiddenForCLI(hArg),
                       "is_hidden_for_cli");
    // GDALAlgorithmArgIsOnlyForCLI() is deprecated at GDAL 3.12
    // renamed to GDALAlgorithmArgIsHiddenForAPI()
    // see https://github.com/OSGeo/gdal/pull/12890
    arg_info.push_back(GDALAlgorithmArgIsHiddenForAPI(hArg),
                       "is_only_for_cli");
    #endif

    arg_info.push_back(GDALAlgorithmArgIsInput(hArg), "is_input");
    arg_info.push_back(GDALAlgorithmArgIsOutput(hArg), "is_output");

    if (eType == GAAT_DATASET || eType == GAAT_DATASET_LIST) {

        //  type flags
        GDALArgDatasetType ds_type = GDALAlgorithmArgGetDatasetType(hArg);
        Rcpp::CharacterVector ds_type_flags = Rcpp::CharacterVector::create();
        if (ds_type & GDAL_OF_RASTER)
            ds_type_flags.push_back("RASTER");
        if (ds_type & GDAL_OF_VECTOR)
            ds_type_flags.push_back("VECTOR");
        if (ds_type & GDAL_OF_MULTIDIM_RASTER)
            ds_type_flags.push_back("MULTIDIM_RASTER");
        if (ds_type & GDAL_OF_UPDATE)
            ds_type_flags.push_back("UPDATE");

        arg_info.push_back(ds_type_flags, "dataset_type_flags");

        // input flags
        int ds_input_flags = GDALAlgorithmArgGetDatasetInputFlags(hArg);
        Rcpp::CharacterVector ds_input_flags_out =
            Rcpp::CharacterVector::create();

        if (ds_input_flags & GADV_NAME)
            ds_input_flags_out.push_back("NAME");
        if (ds_input_flags & GADV_OBJECT)
            ds_input_flags_out.push_back("OBJECT");

        arg_info.push_back(ds_input_flags_out, "dataset_input_flags");

        // output flags
        int ds_output_flags = GDALAlgorithmArgGetDatasetOutputFlags(hArg);
        Rcpp::CharacterVector ds_output_flags_out =
            Rcpp::CharacterVector::create();

        if (ds_output_flags & GADV_NAME)
            ds_output_flags_out.push_back("NAME");
        if (ds_output_flags & GADV_OBJECT)
            ds_output_flags_out.push_back("OBJECT");

        arg_info.push_back(ds_output_flags_out, "dataset_output_flags");
    }
    else {
        arg_info.push_back(R_NilValue, "dataset_type_flags");
        arg_info.push_back(R_NilValue, "dataset_input_flags");
        arg_info.push_back(R_NilValue, "dataset_output_flags");
    }

    arg_info.push_back(GDALAlgorithmArgGetMutualExclusionGroup(hArg),
                       "mutual_exclusion_group");

    GDALAlgorithmArgRelease(hArg);

    return arg_info;
#endif  // GDALALG_MIN_GDAL_
}

void GDALAlg::usage() const {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    Rcpp::Environment pkg = Rcpp::Environment::namespace_env("gdalraster");
    Rcpp::Function print_usage = pkg[".print_alg_usage"];
    print_usage(Rcpp::wrap(m_cmd_str));
#endif
}

Rcpp::String GDALAlg::usageAsJSON() const {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (!m_hAlg)
        Rcpp::stop("algorithm not instantiated");

    char *pszUsage = nullptr;
    pszUsage = GDALAlgorithmGetUsageAsJSON(
        m_hActualAlg ? m_hActualAlg : m_hAlg);

    Rcpp::String json = "";
    if (pszUsage)
        json = Rcpp::String(pszUsage);
    CPLFree(pszUsage);

    return json;
#endif  // GDALALG_MIN_GDAL_
}

bool GDALAlg::setArg(const Rcpp::String &arg_name,
                     const Rcpp::RObject &arg_value) {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (!m_hAlg) {
        if (!quiet)
            Rcpp::Rcout << "algorithm not instantiated\n";
        return false;
    }

    if (m_hasRun) {
        if (!quiet)
            Rcpp::Rcout << "algorithm has already run\n";
        return false;
    }

    if (!arg_name.get_cstring() || arg_name == NA_STRING || arg_name == "") {
        if (!quiet)
            Rcpp::Rcout << "invalid value for 'arg_name'\n";
        return false;
    }

    if (arg_value.isNULL()) {
        if (!quiet)
            Rcpp::Rcout << "'arg_value' is NULL\n";
        return false;
    }

    Rcpp::String arg_name_in(arg_name);
    arg_name_in.replace_all("--", "");
    arg_name_in.replace_all("_", "-");
    if (STARTS_WITH(arg_name_in.get_cstring(), "-") &&
        std::strlen(arg_name_in.get_cstring()) < 3) {

        if (!quiet)
            Rcpp::Rcout << "'arg_name' must be an argument \"long\" name\n";
        return false;
    }

    GDALAlgorithmArgH hArg = nullptr;
    hArg = GDALAlgorithmGetArg(m_hActualAlg ? m_hActualAlg : m_hAlg,
                               arg_name_in.get_cstring());
    if (!hArg) {
        if (!quiet)
            Rcpp::Rcout << "failed to instantiate algorithm argument from "
                           "'arg_name = " << arg_name.get_cstring() << "'\n";
        return false;
    }
    if (!GDALAlgorithmArgIsInput(hArg)) {
        if (!quiet)
            Rcpp::Rcout << "'" << arg_name.get_cstring() << "' is not an input "
                           "argument of the algorithm\n";
        return false;
    }

    bool ret = false;

    switch (GDALAlgorithmArgGetType(hArg)) {
        case GAAT_BOOLEAN:
        {
            if (!Rcpp::is<Rcpp::LogicalVector>(arg_value)) {
                Rcpp::Rcout << "logical type required for a BOOLEAN type "
                               "algorithm argument\n";
                break;
            }

            Rcpp::LogicalVector v(arg_value);
            if (v.size() != 1 || Rcpp::LogicalVector::is_na(v[0])) {
                Rcpp::Rcout << "'arg_value' must be a single logical value for "
                               "BOOLEAN type algorithm argument (TRUE|FALSE)\n";
                break;
            }

            ret = GDALAlgorithmArgSetAsBoolean(hArg,
                                               v[0] == TRUE ? true : false);
        }
        break;

        case GAAT_STRING:
        {
            if (!Rcpp::is<Rcpp::CharacterVector>(arg_value)) {
                Rcpp::Rcout << "character type required for a STRING type "
                               "algorithm argument\n";
                break;
            }

            Rcpp::CharacterVector v(arg_value);
            if (v.size() != 1 || Rcpp::CharacterVector::is_na(v[0])) {
                Rcpp::Rcout << "'arg_value' must be a character string for a "
                               "STRING algorithm argument (i.e., a length-1 "
                               "vector)\n";
                break;
            }

            Rcpp::String val(v[0]);
            ret = GDALAlgorithmArgSetAsString(hArg, val.get_cstring());
        }
        break;

        case GAAT_INTEGER:
        {
            if (!Rcpp::is<Rcpp::LogicalVector>(arg_value) &&
                !Rcpp::is<Rcpp::IntegerVector>(arg_value) &&
                !Rcpp::is<Rcpp::NumericVector>(arg_value)) {

                Rcpp::Rcout << "integer or numeric type required for an "
                               "INTEGER type algorithm argument\n";
                break;
            }

            Rcpp::IntegerVector v(arg_value);
            if (v.size() != 1) {
                Rcpp::Rcout << "'arg_value' must be a single numeric value for "
                               "an INTEGER type algorithm argument\n";
                break;
            }
            if (Rcpp::IntegerVector::is_na(v[0])) {
                Rcpp::Rcout << "'arg_value' cannot be NA\n";
                break;
            }

            ret = GDALAlgorithmArgSetAsInteger(hArg, v[0]);
        }
        break;

        case GAAT_REAL:
        {
            if (!Rcpp::is<Rcpp::LogicalVector>(arg_value) &&
                !Rcpp::is<Rcpp::IntegerVector>(arg_value) &&
                !Rcpp::is<Rcpp::NumericVector>(arg_value)) {

                Rcpp::Rcout << "numeric type required for a REAL type "
                               "algorithm argument\n";
                break;
            }

            Rcpp::NumericVector v(arg_value);
            if (v.size() != 1) {
                Rcpp::Rcout << "'arg_value' must be a single numeric value for "
                               "a REAL type algorithm argument\n";
                break;
            }
            if (Rcpp::NumericVector::is_na(v[0])) {
                Rcpp::Rcout << "'arg_value' cannot be NA\n";
                break;
            }

            ret = GDALAlgorithmArgSetAsDouble(hArg, v[0]);
        }
        break;

        case GAAT_STRING_LIST:
        {
            if (!Rcpp::is<Rcpp::CharacterVector>(arg_value)) {
                Rcpp::Rcout << "character type required for a STRING_LIST type "
                               "algorithm argument\n";
                break;
            }

            Rcpp::CharacterVector v(arg_value);
            if (v.size() < 1) {
                Rcpp::Rcout << "'arg_value' is empty\n";
                break;
            }
            if (Rcpp::is_true(Rcpp::any(Rcpp::is_na(v)))) {
                Rcpp::Rcout << "'arg_value' cannot contain missing values for "
                               "a STRING_LIST algorithm argument\n";
                break;
            }

            std::vector<const char *> str_list;
            for (R_xlen_t i = 0; i < v.size(); ++i) {
                Rcpp::String s(v[i]);
                str_list.push_back(s.get_cstring());
            }
            str_list.push_back(nullptr);

            ret = GDALAlgorithmArgSetAsStringList(hArg, str_list.data());
        }
        break;

        case GAAT_INTEGER_LIST:
        {
            if (!Rcpp::is<Rcpp::LogicalVector>(arg_value) &&
                !Rcpp::is<Rcpp::IntegerVector>(arg_value) &&
                !Rcpp::is<Rcpp::NumericVector>(arg_value)) {

                Rcpp::Rcout << "integer or numeric type required for an "
                               "INTEGER_LIST type algorithm argument\n";
                break;
            }

            Rcpp::IntegerVector v(arg_value);
            if (v.size() < 1) {
                Rcpp::Rcout << "'arg_value' is empty\n";
                break;
            }
            if (Rcpp::is_true(Rcpp::any(Rcpp::is_na(v)))) {
                Rcpp::Rcout << "'arg_value' cannot contain missing values for "
                               "an INTEGER_LIST algorithm argument\n";
                break;
            }

            ret = GDALAlgorithmArgSetAsIntegerList(hArg, v.size(), v.begin());
        }
        break;

        case GAAT_REAL_LIST:
        {
            if (!Rcpp::is<Rcpp::LogicalVector>(arg_value) &&
                !Rcpp::is<Rcpp::IntegerVector>(arg_value) &&
                !Rcpp::is<Rcpp::NumericVector>(arg_value)) {

                Rcpp::Rcout << "numeric vector required for a REAL_LIST type "
                               "algorithm argument\n";
                break;
            }

            Rcpp::NumericVector v(arg_value);
            if (v.size() < 1) {
                Rcpp::Rcout << "'arg_value' is empty\n";
                break;
            }
            if (Rcpp::is_true(Rcpp::any(Rcpp::is_na(v)))) {
                Rcpp::Rcout << "'arg_value' cannot contain missing values for "
                               "a REAL_LIST algorithm argument\n";
                break;
            }

            ret = GDALAlgorithmArgSetAsDoubleList(hArg, v.size(), v.begin());
        }
        break;

        case GAAT_DATASET:
        {
            int ds_input_flags = GDALAlgorithmArgGetDatasetInputFlags(hArg);

            if (Rcpp::is<Rcpp::CharacterVector>(arg_value)) {
                if (!(ds_input_flags & GADV_NAME)) {
                    Rcpp::Rcout << "this argument does not accept a dataset "
                                   "name as input (object input required).\n";
                    break;
                }
                Rcpp::CharacterVector v(arg_value);
                if (v.size() != 1 || Rcpp::CharacterVector::is_na(v[0])) {
                    Rcpp::Rcout << "string input must be a length-1 character "
                                   "vector for a DATASET algorithm argument\n";
                    break;
                }
                Rcpp::String val(enc_to_utf8_(v));
                ret = GDALAlgorithmArgSetAsString(hArg, val.get_cstring());
                break;
            }
            else if (is_gdalraster_obj_(arg_value)) {
                if (!(ds_input_flags & GADV_OBJECT)) {
                    Rcpp::Rcout << "this argument does not accept a dataset "
                                   "object as input (DSN required, may be "
                                   "created by algorithm)\n";
                    break;
                }
                const Rcpp::String cls = arg_value.attr("class");
                if (cls == "Rcpp_GDALRaster") {
                    const GDALRaster &ds = Rcpp::as<GDALRaster &>(arg_value);
                    ret = GDALAlgorithmArgSetDataset(hArg,
                                                     ds.getGDALDatasetH_());
                    break;
                }
                else if (cls == "Rcpp_GDALVector") {
                    const GDALVector &ds = Rcpp::as<GDALVector &>(arg_value);
                    ret = GDALAlgorithmArgSetDataset(hArg,
                                                     ds.getGDALDatasetH_());
                    break;
                }
                else {
                    Rcpp::Rcout << "unhandled object of class " <<
                                cls.get_cstring() << "\n";
                    break;
                }
            }
            else {
                Rcpp::Rcout << "unsupported input type for a DATASET algorithm "
                               "argument\n";
            }
        }
        break;

        case GAAT_DATASET_LIST:
        {
            int ds_input_flags = GDALAlgorithmArgGetDatasetInputFlags(hArg);

            if (Rcpp::is<Rcpp::CharacterVector>(arg_value)) {
                if (!(ds_input_flags & GADV_NAME)) {
                    Rcpp::Rcout << "this argument does not accept dataset "
                                   "names as input\n";
                    break;
                }
                Rcpp::CharacterVector v(arg_value);
                if (v.size() < 1) {
                    Rcpp::Rcout << "'arg_value' is empty\n";
                    break;
                }
                if (Rcpp::is_true(Rcpp::any(Rcpp::is_na(v)))) {
                    Rcpp::Rcout << "'arg_value' cannot contain missing values "
                                   "for a DATASET_LIST algorithm argument\n";
                    break;
                }
                v = (enc_to_utf8_(v));
                std::vector<const char *> str_list;
                for (R_xlen_t i = 0; i < v.size(); ++i) {
                    Rcpp::String s(v[i]);
                    str_list.push_back(s.get_cstring());
                }
                str_list.push_back(nullptr);
                ret = GDALAlgorithmArgSetDatasetNames(hArg, str_list.data());
                break;
            }
            else if (Rcpp::is<Rcpp::List>(arg_value) ||
                     is_gdalraster_obj_(arg_value)) {

                if (!(ds_input_flags & GADV_OBJECT)) {
                    Rcpp::Rcout << "this argument does not accept dataset "
                                   "objects as input\n";
                    break;
                }

                Rcpp::List ds_list;
                if (is_gdalraster_obj_(arg_value))
                    ds_list = Rcpp::List::create(arg_value);
                else
                    ds_list = Rcpp::List(arg_value);

                std::vector<GDALDatasetH> pahDS;
                for (R_xlen_t i = 0; i < ds_list.size(); ++i) {
                    if (is_gdalraster_obj_(ds_list[i])) {
                        const Rcpp::RObject &x(ds_list[i]);
                        const Rcpp::String cls = x.attr("class");
                        if (cls == "Rcpp_GDALRaster") {
                            const GDALRaster &ds = Rcpp::as<GDALRaster &>(x);
                            pahDS.push_back(ds.getGDALDatasetH_());
                        }
                        else if (cls == "Rcpp_GDALVector") {
                            const GDALVector &ds = Rcpp::as<GDALVector &>(x);
                            pahDS.push_back(ds.getGDALDatasetH_());
                        }
                        else {
                            Rcpp::Rcout << "unhandled object of class " <<
                                           cls.get_cstring() << "\n";
                            break;
                        }
                    }
                    else {
                        Rcpp::Rcout << "list element contains invalid input "
                                       "type\n";
                        break;
                    }
                }
                if (!pahDS.empty()) {
                    ret = GDALAlgorithmArgSetDatasets(hArg, pahDS.size(),
                                                      pahDS.data());
                    break;
                }
            }
            else {
                Rcpp::Rcout << "DATASET_LIST algorithm argument requires a "
                               "character vector of names, or list of "
                               "objects\n";
            }
        }
        break;

        default:
        {
            Rcpp::Rcout << "GDALAlgorithmArgType: " <<
                           GDALAlgorithmArgGetType(hArg) << "\n";
            Rcpp::Rcout << "unhandled argument type\n";
        }
        break;
    }

    GDALAlgorithmArgRelease(hArg);
    return ret;
#endif  // GDALALG_MIN_GDAL_
}

bool GDALAlg::parseCommandLineArgs() {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    // parses cl args which sets the values, and also instantiates m_hActualAlg

    if (!m_hAlg) {
        if (!quiet)
            Rcpp::Rcout << "algorithm not instantiated\n";
        return false;
    }

    if (m_haveParsedCmdLineArgs) {
        if (!quiet)
            Rcpp::Rcout << "parseCommandLineArgs() can only be called once "
                           "per instance\n";
        return false;
    }

    bool res = true;

    if (!m_map_input_hDS.empty()) {

        // set dataset object inputs directly

        for (auto it = m_map_input_hDS.begin(); it != m_map_input_hDS.end();
             ++it) {

            GDALAlgorithmArgH hArg = GDALAlgorithmGetArg(m_hAlg,
                                                         it->first.c_str());

            if (hArg) {
                if (GDALAlgorithmArgGetType(hArg) == GAAT_DATASET) {
                    if (it->second.size() > 1) {
                        Rcpp::Rcout << it->first.c_str() << ": " <<
                            "dataset list given where single dataset expected"
                            << "\n";

                        res = false;
                    } else {
                        res = GDALAlgorithmArgSetDataset(hArg, it->second[0]);
                    }
                }
                else if (GDALAlgorithmArgGetType(hArg) == GAAT_DATASET_LIST) {
                    res = GDALAlgorithmArgSetDatasets(hArg, it->second.size(),
                                                      it->second.data());
                }
                else {
                    res = false;
                }

                GDALAlgorithmArgRelease(hArg);
                if (!res)
                    break;
            }
        }
    }

    if (res && this->setVectorArgsFromObject &&
        (m_in_vector_props.is_set || m_like_vector_props.is_set)) {

        // directly set certain arguments from input GDALVector object(s)

        if (m_in_vector_props.is_set) {
            GDALAlgorithmArgH hArg = GDALAlgorithmGetArg(m_hAlg, "input");
            if (GDALAlgorithmArgGetType(hArg) == GAAT_DATASET_LIST) {
                if (m_num_input_datasets > 1) {
                    GDALAlgorithmArgRelease(hArg);
                    Rcpp::stop("setting args from GDALVector currently "
                               "unsupported for multiple input objects");
                }
            }
            else if (GDALAlgorithmArgGetType(hArg) != GAAT_DATASET) {
                GDALAlgorithmArgRelease(hArg);
                Rcpp::stop("setting args from GDALVector incompatible with "
                           "the algorithm argument type of \"input\"");
            }
            GDALAlgorithmArgRelease(hArg);
        }

        const Rcpp::List &alg_info = info();
        const Rcpp::CharacterVector &arg_names = alg_info["arg_names"];
        constexpr char override_warning_msg[] =
            "value given in 'args' overrides setting from input object";

        for (Rcpp::String arg_name : arg_names) {
            const char *pszArgName = arg_name.get_cstring();

            // FIXME: arg aliases are currently hard coded, look up instead

            if (m_in_vector_props.is_set && EQUAL(pszArgName, "input-format")) {
                if (contains_str_(m_args, "--input-format") ||
                    contains_str_(m_args, "--if")) {

                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(override_warning_msg);
                    }
                }
                else {
                    // this one can fail silently since it is optional anyway
                    res = setArg(
                        arg_name,
                        Rcpp::wrap(m_in_vector_props.driver_short_name));
                }
                continue;
            }
            else if (m_in_vector_props.is_set && !m_in_vector_props.is_sql &&
                     EQUAL(pszArgName, "input-layer")) {

                if (contains_str_(m_args, "--input-layer") ||
                    contains_str_(m_args, "--layer") ||
                    contains_str_(m_args, "-l")) {

                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(override_warning_msg);
                    }
                }
                else {
                    res = setArg(
                        arg_name, Rcpp::wrap(m_in_vector_props.layer_name));

                    if (!res) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning("failed to set from object");
                        break;
                    }
                }
                continue;
            }
            else if (m_in_vector_props.is_set && m_in_vector_props.is_sql &&
                     EQUAL(pszArgName, "sql")) {

                if (contains_str_(m_args, "--sql")) {
                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(override_warning_msg);
                    }
                }
                else {
                    res = setArg(
                        arg_name, Rcpp::wrap(m_in_vector_props.layer_sql));

                    if (!res) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning("failed to set from object");
                        break;
                    }
                }
                continue;
            }
            else if (m_in_vector_props.is_set && m_in_vector_props.is_sql &&
                     EQUAL(pszArgName, "dialect")) {

                if (contains_str_(m_args, "--dialect")) {
                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(override_warning_msg);
                    }
                }
                else {
                    if (!m_in_vector_props.sql_dialect.empty()) {
                        res = setArg(
                            arg_name,
                            Rcpp::wrap(m_in_vector_props.sql_dialect));
                    }

                    if (!res) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning("failed to set from object");
                        break;
                    }
                }
                continue;
            }
            else if (m_like_vector_props.is_set && m_like_vector_props.is_sql &&
                     EQUAL(pszArgName, "like-sql")) {

                if (contains_str_(m_args, "--like-sql")) {
                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(override_warning_msg);
                    }
                }
                else {
                    res = setArg(
                        arg_name, Rcpp::wrap(m_like_vector_props.layer_sql));

                    if (!res) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning("failed to set from object");
                        break;
                    }
                }
                continue;
            }
            else if (m_like_vector_props.is_set && !m_like_vector_props.is_sql
                     && EQUAL(pszArgName, "like-layer")) {

                if (contains_str_(m_args, "--like-layer")) {
                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(override_warning_msg);
                    }
                }
                else {
                    res = setArg(
                        arg_name, Rcpp::wrap(m_like_vector_props.layer_name));

                    if (!res) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning("failed to set from object");
                        break;
                    }
                }
                continue;
            }
        }
    }

    std::vector<const char*> arg_list = {};
    if (m_args.size() > 0) {
        for (Rcpp::String arg : m_args) {
            arg_list.push_back(arg.get_cstring());
        }
    }
    arg_list.push_back(nullptr);

    if (res && arg_list.size() > 1) {
        // parse arguments from m_args
        res = GDALAlgorithmParseCommandLineArguments(m_hAlg, arg_list.data());
    }

    if (res) {
        m_haveParsedCmdLineArgs = true;
        if (m_hActualAlg == nullptr) {
            m_hActualAlg = GDALAlgorithmGetActualAlgorithm(m_hAlg);
        }
    }

    return res;
#endif  // GDALALG_MIN_GDAL_
}

Rcpp::List GDALAlg::getExplicitlySetArgs() const {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else
    if (!m_hAlg)
        Rcpp::stop("algorithm not instantiated");

    char **papszArgNames = nullptr;
    papszArgNames =
        GDALAlgorithmGetArgNames(m_hActualAlg ? m_hActualAlg : m_hAlg);

    Rcpp::List out = Rcpp::List::create();

    const int nCount = CSLCount(papszArgNames);
    if (nCount > 0) {
        for (int i = 0; i < nCount; ++i) {
            GDALAlgorithmArgH hArg = nullptr;
            hArg = GDALAlgorithmGetArg(m_hActualAlg ? m_hActualAlg : m_hAlg,
                                       papszArgNames[i]);
            if (!hArg) {
                if (!quiet)
                    Rcpp::Rcout << "got NULL for: " << papszArgNames[i] << "\n";
                continue;
            }
            if (GDALAlgorithmArgIsExplicitlySet(hArg)) {
                Rcpp::String arg_name_out(papszArgNames[i]);
                arg_name_out.replace_all("-", "_");

                // dataset object
                if (GDALAlgorithmArgGetType(hArg) == GAAT_DATASET) {
                    GDALArgDatasetValueH hArgDSValue = nullptr;
                    hArgDSValue = GDALAlgorithmArgGetAsDatasetValue(hArg);
                    if (!hArgDSValue) {
                        if (!quiet)
                            Rcpp::Rcout << "output dataset value is NULL\n";
                        GDALAlgorithmArgRelease(hArg);
                        continue;
                    }

                    GDALArgDatasetType ds_type =
                        GDALAlgorithmArgGetDatasetType(hArg);

                    std::string ds_name(
                        GDALArgDatasetValueGetName(hArgDSValue));

                    if (ds_type & GDAL_OF_RASTER) {
                        ds_name = "<raster dataset object: " + ds_name + ">";
                    }
                    else if (ds_type & GDAL_OF_VECTOR) {
                        ds_name = "<vector dataset object: " + ds_name + ">";
                    }
                    else if (ds_type & GDAL_OF_MULTIDIM_RASTER) {
                        ds_name = ("<mutlidim raster dataset object: " +
                                   ds_name + ">");
                    }
                    else {
                        // should not occur
                        ds_name = "<unrecognized dataset object>";
                    }

                    GDALArgDatasetValueRelease(hArgDSValue);
                    out.push_back(ds_name, arg_name_out);
                }
                // list of GDAL datasets
                else if ((GDALAlgorithmArgGetType(hArg) ==
                          GAAT_DATASET_LIST)) {

                    out.push_back("<list of dataset objects>", arg_name_out);
                }
                // boolean, string, integer, real, and lists of
                else {
                    out.push_back(getArgValue_(hArg), arg_name_out);
                }
            }
            GDALAlgorithmArgRelease(hArg);
        }
    }
    CSLDestroy(papszArgNames);

    return out;
#endif
}

bool GDALAlg::run() {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (!m_hAlg) {
        if (!quiet)
            Rcpp::Rcout << "algorithm not instantiated\n";
        return false;
    }

    if (m_hasRun) {
        if (!quiet)
            Rcpp::Rcout << "algorithm has already run\n";
        return false;
    }

    if (!m_haveParsedCmdLineArgs) {
        if (!parseCommandLineArgs()) {
            if (!quiet)
                Rcpp::Rcout << "parse command line arguments failed\n";
            return false;
        }
    }

    if (!m_hActualAlg) {
        if (!quiet)
            Rcpp::Rcout << "actual algorithm handle is NULL\n";
        return false;
    }

    bool res = GDALAlgorithmRun(m_hActualAlg,
                                quiet ? nullptr : GDALTermProgressR,
                                nullptr);

    if (res)
        m_hasRun = true;

    return res;
#endif  // GDALALG_MIN_GDAL_
}

SEXP GDALAlg::output() const {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (!m_hAlg)
        Rcpp::stop("algorithm not instantiated");

    if (!m_hasRun || !m_hActualAlg)
        Rcpp::stop("algorithm has not run");

    std::vector<std::string> out_arg_names = getOutputArgNames_();
    if (out_arg_names.size() == 0) {
        Rcpp::stop("no output argument names found");
    }
    if (out_arg_names.size() > 1) {
        Rcpp::stop(
            "algorithm has multiple outputs, use method `outputs()` instead");
    }

    Rcpp::List out = outputs();
    if (out.size() > 0)
        return out[0];
    else
        return R_NilValue;
#endif  // GDALALG_MIN_GDAL_
}

Rcpp::List GDALAlg::outputs() const {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (!m_hAlg)
        Rcpp::stop("algorithm not instantiated");

    if (!m_hasRun || !m_hActualAlg)
        Rcpp::stop("algorithm has not run");

    std::vector<std::string> out_arg_names = getOutputArgNames_();
    if (out_arg_names.size() == 0)
        Rcpp::stop("no output argument names found");

    Rcpp::List out = Rcpp::List::create();

    for (const std::string &arg_name : out_arg_names) {
        GDALAlgorithmArgH hArg = nullptr;
        hArg = GDALAlgorithmGetArg(m_hActualAlg, arg_name.c_str());
        if (!hArg) {
            if (!quiet) {
                Rcpp::Rcout << "got NULL for arg: " << arg_name.c_str()
                    << "\n";
            }
            continue;
        }

        if (GDALAlgorithmArgIsOutput(hArg)) {
            Rcpp::String arg_name_out(arg_name);
            arg_name_out.replace_all("-", "_");
            out.push_back(getArgValue_(hArg), arg_name_out);
        }

        GDALAlgorithmArgRelease(hArg);
    }

    return out;
#endif  // GDALALG_MIN_GDAL_
}

bool GDALAlg::close() {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (!m_hAlg) {
        if (!quiet)
            Rcpp::Rcout << "algorithm not instantiated\n";
        return false;
    }

    if (!m_hasRun) {
        if (!quiet)
            Rcpp::Rcout << "algorithm has not run\n";
        return false;
    }

    if (m_hasFinalized) {
        if (!quiet)
            Rcpp::Rcout << "algorithm has already been finalized\n";
        return false;
    }

    if (m_hActualAlg) {
        return GDALAlgorithmFinalize(m_hActualAlg);
    }
    else {
        if (!quiet)
            Rcpp::Rcout << "actual algorithm is NULL\n";
        return false;
    }
#endif  // GDALALG_MIN_GDAL_
}

void GDALAlg::release() {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    return;
#else

    if (m_hActualAlg) {
        if (m_hasRun && !m_hasFinalized)
            GDALAlgorithmFinalize(m_hActualAlg);
        GDALAlgorithmRelease(m_hActualAlg);
        m_hActualAlg = nullptr;
    }

    if (m_hAlg) {
        GDALAlgorithmRelease(m_hAlg);
        m_hAlg = nullptr;
    }

#endif  // GDALALG_MIN_GDAL_
}

void GDALAlg::show() const {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::Rcout << GDALALG_MIN_GDAL_MSG_ << "\n";
#else

    Rcpp::Rcout << "C++ object of class GDALAlg\n";

    if (!m_hAlg) {
        Rcpp::Rcout << " algorithm not instantiated\n";
    }
    else {
        const GDALAlgorithmH &alg = m_hActualAlg ? m_hActualAlg : m_hAlg;

        Rcpp::Rcout << " Name        : " << GDALAlgorithmGetName(alg) << "\n";
        Rcpp::Rcout << " Description : " << GDALAlgorithmGetDescription(alg) <<
            "\n";
        Rcpp::Rcout << " Help URL    : " << GDALAlgorithmGetHelpFullURL(alg) <<
            "\n";
    }
#endif  // GDALALG_MIN_GDAL_
}

// ****************************************************************************
// class methods for internal use not exposed in R
// ****************************************************************************

Rcpp::CharacterVector GDALAlg::parseListArgs_(
                                const Rcpp::List &list_args) {

    // convert arguments in a named list to character vector form and return it
    // arguments in list form must use arg long names
    // dataset/dataset_list args are added to the std::map m_map_input_hDS

    Rcpp::CharacterVector arg_vec = Rcpp::CharacterVector::create();

    R_xlen_t num_args = 0;
    if (list_args.size() == 0)
        return arg_vec;
    else
        num_args = list_args.size();

    Rcpp::CharacterVector arg_names = list_args.names();
    if (arg_names.size() == 0 || arg_names.size() != num_args)
        Rcpp::stop("argument list must have named elements");

    for (R_xlen_t i = 0; i < num_args; ++i) {
        Rcpp::String nm(arg_names[i]);
        if (!nm.get_cstring() || nm == NA_STRING || nm == "")
            continue;

        if (list_args[i] == R_NilValue) {
            Rcpp::Rcout << "ignoring NULL arg: " << nm.get_cstring() << "\n";
            continue;
        }

        nm.replace_all("--", "");
        nm.replace_all("_", "-");
        if (STARTS_WITH(nm.get_cstring(), "-") &&
            std::strlen(nm.get_cstring()) < 3) {

            Rcpp::Rcout << "argument: " << arg_names[i] << "\n";
            Rcpp::stop("arguments in list format must use \"long\" names");
        }
        std::string nm_no_lead_dashes(nm.get_cstring());
        nm.push_front("--");

        // boolean
        if (Rcpp::is<Rcpp::LogicalVector>(list_args[i])) {
            const Rcpp::LogicalVector &lv = list_args[i];
            if (lv[0] == TRUE) {
                arg_vec.push_back(nm);
                continue;
            }
        }

        // string, string list
        if (Rcpp::is<Rcpp::CharacterVector>(list_args[i])) {
            Rcpp::CharacterVector arg_value = enc_to_utf8_(list_args[i]);
            nm += "=";
            nm += paste_collapse_(arg_value, ",");
            arg_vec.push_back(nm);
            continue;
        }

        // integer, integer list, real, real list
        if (Rcpp::is<Rcpp::IntegerVector>(list_args[i]) ||
            Rcpp::is<Rcpp::NumericVector>(list_args[i])) {

            nm += "=";
            nm += paste_collapse_(list_args[i], ",");
            arg_vec.push_back(nm);
            continue;
        }

        // potentially a list of datasets
        if (Rcpp::is<Rcpp::List>(list_args[i])) {
            const Rcpp::List &list_tmp = list_args[i];

            std::vector<GDALDatasetH> ds_list = {};

            for (R_xlen_t j = 0; j < list_tmp.size(); ++j) {
                if (is_gdalraster_obj_(list_tmp[j])) {
                    const Rcpp::RObject &val = list_tmp[j];
                    const Rcpp::String cls = val.attr("class");
                    if (cls == "Rcpp_GDALRaster") {
                        const GDALRaster &ds =
                            Rcpp::as<GDALRaster &>(list_tmp[j]);
                        ds_list.push_back(ds.getGDALDatasetH_());
                    }
                    else if (cls == "Rcpp_GDALVector") {
                        const GDALVector &ds =
                            Rcpp::as<GDALVector &>(list_tmp[j]);

                        ds_list.push_back(ds.getGDALDatasetH_());
                        if (EQUAL(nm.get_cstring(), "--input") &&
                            !m_in_vector_props.is_set) {

                            m_in_vector_props.driver_short_name =
                                ds.getDriverShortName();
                            m_in_vector_props.layer_name = ds.getName();
                            m_in_vector_props.is_sql = ds.m_is_sql;
                            m_in_vector_props.layer_sql = ds.m_layer_name;
                            m_in_vector_props.sql_dialect = ds.m_dialect;
                            m_in_vector_props.is_set = true;
                        }
                    }
                }
                else {
                    Rcpp::Rcout << "a list element is not a dataset object\n";
                }
            }

            if (!ds_list.empty()) {
                m_map_input_hDS[nm_no_lead_dashes] = ds_list;
            }
            else {
                Rcpp::Rcout << "unhandled list input: " <<
                               nm_no_lead_dashes.c_str() << "\n";
            }
            continue;
        }

        // potentially a single dataset
        if (is_gdalraster_obj_(list_args[i])) {
            const Rcpp::RObject &val = list_args[i];
            const Rcpp::String cls = val.attr("class");
            if (cls == "Rcpp_GDALRaster") {
                const GDALRaster &ds = Rcpp::as<GDALRaster &>(list_args[i]);
                std::vector<GDALDatasetH> ds_list = {};
                ds_list.push_back(ds.getGDALDatasetH_());
                m_map_input_hDS[nm_no_lead_dashes] = ds_list;
                continue;
            }
            if (cls == "Rcpp_GDALVector") {
                const GDALVector &ds = Rcpp::as<GDALVector &>(list_args[i]);
                std::vector<GDALDatasetH> ds_list = {};
                ds_list.push_back(ds.getGDALDatasetH_());
                m_map_input_hDS[nm_no_lead_dashes] = ds_list;
                if (EQUAL(nm.get_cstring(), "--input")) {
                    m_in_vector_props.driver_short_name =
                        ds.getDriverShortName();
                    m_in_vector_props.layer_name = ds.getName();
                    m_in_vector_props.is_sql = ds.m_is_sql;
                    m_in_vector_props.layer_sql = ds.m_layer_name;
                    m_in_vector_props.sql_dialect = ds.m_dialect;
                    m_in_vector_props.is_set = true;
                }
                else if (EQUAL(nm.get_cstring(), "--like")) {
                    m_like_vector_props.driver_short_name =
                        ds.getDriverShortName();
                    m_like_vector_props.layer_name = ds.getName();
                    m_like_vector_props.is_sql = ds.m_is_sql;
                    m_like_vector_props.layer_sql = ds.m_layer_name;
                    m_like_vector_props.sql_dialect = ds.m_dialect;
                    m_like_vector_props.is_set = true;
                }
                continue;
            }
        }

        Rcpp::Rcout << "unhandled input type for " <<
            nm_no_lead_dashes.c_str() << "\n";
    }

    if (m_map_input_hDS.count("input") > 0) {
        m_input_is_object = true;
        std::vector<GDALDatasetH> ds_list = m_map_input_hDS["input"];
        m_num_input_datasets = ds_list.size();
        if (m_num_input_datasets > 1 && m_in_vector_props.is_set) {
            // since currently, auto setting args from GDALVector input is
            // not supported for multiple input datasets
            m_in_vector_props = {};
        }
    }

    return arg_vec;
}

void GDALAlg::instantiateAlg_() {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else
    // instantiate m_hAlg
    // TODO: GDAL 3.12 adds GDALAlgorithmRegistryInstantiateAlgFromPath().

    // For GDAL < 3.12, step through the path if there are subcommands, and
    // use GDALAlgorithmRegistryInstantiateAlg().
    if (m_hAlg || m_hActualAlg) {
        Rcpp::stop(
            "instantiateAlg_(): algorithm object already instantiated");
    }

    GDALAlgorithmRegistryH reg = nullptr;
    reg = GDALGetGlobalAlgorithmRegistry();
    if (!reg)
        Rcpp::stop("failed to obtain global algorithm registry");

    if (m_cmd.size() == 1) {
        Rcpp::String cmd(m_cmd[0]);

        m_hAlg = GDALAlgorithmRegistryInstantiateAlg(reg, cmd.get_cstring());

        if (!m_hAlg) {
            GDALAlgorithmRegistryRelease(reg);
            Rcpp::Rcout << "top-level command: " << cmd.get_cstring() << "\n";
            Rcpp::stop("failed to instantiate CLI algorithm");
        }
    }
    else {
        std::vector<GDALAlgorithmH> alg_tmp = {};
        Rcpp::String cmd(m_cmd[0]);

        alg_tmp.push_back(
            GDALAlgorithmRegistryInstantiateAlg(reg, cmd.get_cstring()));

        if (!alg_tmp[0]) {
            GDALAlgorithmRegistryRelease(reg);
            Rcpp::Rcout << "top-level command: " << cmd.get_cstring() << "\n";
            Rcpp::stop("failed to instantiate CLI algorithm");
        }
        for (R_xlen_t i = 1; i < m_cmd.size(); ++i) {
            if (i == (m_cmd.size() - 1)) {
                // the final subcommand, instantiate m_hAlg here
                Rcpp::String sub_cmd(m_cmd[i]);

                m_hAlg =
                    GDALAlgorithmInstantiateSubAlgorithm(alg_tmp[i - 1],
                                                         sub_cmd.get_cstring());

                if (!m_hAlg) {
                    for (GDALAlgorithmH alg : alg_tmp) {
                        if (alg)
                            GDALAlgorithmRelease(alg);
                    }
                    GDALAlgorithmRegistryRelease(reg);
                    Rcpp::Rcout << "subcommand: " << sub_cmd.get_cstring() <<
                        "\n";
                    Rcpp::stop("failed to instantiate CLI algorithm");
                }
            }
            else {
                Rcpp::String sub_cmd(m_cmd[i]);

                alg_tmp.push_back(
                    GDALAlgorithmInstantiateSubAlgorithm(
                        alg_tmp[i - 1], sub_cmd.get_cstring()));

                if (alg_tmp.back() == nullptr) {
                    for (GDALAlgorithmH alg : alg_tmp) {
                        if (alg)
                            GDALAlgorithmRelease(alg);
                    }
                    GDALAlgorithmRegistryRelease(reg);
                    Rcpp::Rcout << "subcommand: " << sub_cmd.get_cstring() <<
                        "\n";
                    Rcpp::stop("failed to instantiate CLI algorithm");
                }
            }
        }

        for (GDALAlgorithmH alg : alg_tmp) {
            if (alg)
                GDALAlgorithmRelease(alg);
        }
    }
#endif  // GDALALG_MIN_GDAL_
}

std::vector<std::string> GDALAlg::getOutputArgNames_() const {

    std::vector<std::string> names_out = {};

#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    return names_out;
#else
    char **papszArgNames = nullptr;
    if (m_hActualAlg) {
        papszArgNames = GDALAlgorithmGetArgNames(m_hActualAlg);
    }
    else {
        if (!quiet)
            Rcpp::Rcout << "actual algorithm object not instantiated\n";
        return names_out;
    }

    int nCount = CSLCount(papszArgNames);
    if (nCount > 0) {
        std::vector<std::string> names(papszArgNames, papszArgNames + nCount);
        for (std::string arg_name : names) {
            GDALAlgorithmArgH hArg = nullptr;
            hArg = GDALAlgorithmGetArg(m_hActualAlg, arg_name.c_str());
            if (!hArg) {
                if (!quiet) {
                    Rcpp::Rcout << "got NULL for arg: " << arg_name.c_str()
                        << "\n";
                }
                continue;
            }
            if (GDALAlgorithmArgIsOutput(hArg)) {
                names_out.push_back(arg_name);
            }
            GDALAlgorithmArgRelease(hArg);
        }
    }
    CSLDestroy(papszArgNames);

    return names_out;
#endif  // GDALALG_MIN_GDAL_
}

#if GDAL_VERSION_NUM >= GDALALG_MIN_GDAL_
SEXP GDALAlg::getArgValue_(const GDALAlgorithmArgH &hArg) const {
    if (!hArg)
        Rcpp::stop("getArgValue_(): got NULL for the GDALAlgorithmArgH");

    SEXP out = R_NilValue;

    switch (GDALAlgorithmArgGetType(hArg)) {
        case GAAT_BOOLEAN:
        {
            out =
                Rcpp::LogicalVector::create(GDALAlgorithmArgGetAsBoolean(hArg));
        }
        break;

        case GAAT_STRING:
        {
            out = Rcpp::wrap(GDALAlgorithmArgGetAsString(hArg));
        }
        break;

        case GAAT_INTEGER:
        {
            out =
                Rcpp::IntegerVector::create(GDALAlgorithmArgGetAsInteger(hArg));
        }
        break;

        case GAAT_REAL:
        {
            out =
                Rcpp::NumericVector::create(GDALAlgorithmArgGetAsDouble(hArg));
        }
        break;

        case GAAT_STRING_LIST:
        {
            char **papszValue = nullptr;
            papszValue = GDALAlgorithmArgGetAsStringList(hArg);
            int nCount = CSLCount(papszValue);
            if (papszValue && nCount > 0) {
                out = Rcpp::CharacterVector(papszValue, papszValue + nCount);
            }
            else {
                out = Rcpp::CharacterVector::create(NA_STRING);
            }
            CSLDestroy(papszValue);
        }
        break;

        case GAAT_INTEGER_LIST:
        {
            size_t nCount = 0;
            const int *panValue =
                GDALAlgorithmArgGetAsIntegerList(hArg, &nCount);

            if (panValue && nCount > 0) {
                out = Rcpp::IntegerVector(panValue, panValue + nCount);
            }
            else {
                out = Rcpp::IntegerVector::create(NA_INTEGER);
            }
        }
        break;

        case GAAT_REAL_LIST:
        {
            size_t nCount = 0;
            const double *padfValue =
                GDALAlgorithmArgGetAsDoubleList(hArg, &nCount);

            if (padfValue && nCount > 0) {
                out = Rcpp::NumericVector(padfValue, padfValue + nCount);
            }
            else {
                out = Rcpp::NumericVector::create(NA_REAL);
            }
        }
        break;

        case GAAT_DATASET:
        {
            GDALArgDatasetValueH hArgDSValue = nullptr;
            hArgDSValue = GDALAlgorithmArgGetAsDatasetValue(hArg);
            if (!hArgDSValue) {
                if (!quiet)
                    Rcpp::Rcout << "output dataset value is NULL\n";
                return R_NilValue;
            }

            GDALDatasetH hDS = nullptr;
            hDS = GDALArgDatasetValueGetDatasetIncreaseRefCount(hArgDSValue);
            if (!hDS) {
                GDALArgDatasetValueRelease(hArgDSValue);
                Rcpp::stop("GDAL dataset object is NULL");
            }

            GDALArgDatasetType ds_type = GDALAlgorithmArgGetDatasetType(hArg);

            if (ds_type & GDAL_OF_RASTER) {
                std::string ds_name(GDALArgDatasetValueGetName(hArgDSValue));
                auto ds = std::make_unique<GDALRaster>();
                ds->setFilename(ds_name);
                ds->setGDALDatasetH_(hDS);
                out = Rcpp::wrap(*ds.release());
            }
            else if (ds_type & GDAL_OF_VECTOR) {
                std::string ds_name(GDALArgDatasetValueGetName(hArgDSValue));
                OGRLayerH hLayer = nullptr;
                std::string layer_name = "";
                if (this->outputLayerNameForOpen == "" ||
                    this->outputLayerNameForOpen == NA_STRING) {

                    hLayer = GDALDatasetGetLayer(hDS, 0);
                }
                else {
                    layer_name = this->outputLayerNameForOpen;
                    hLayer = GDALDatasetGetLayerByName(hDS, layer_name.c_str());
                }
                if (layer_name == "") {
                    // default layer first by index was opened
                    if (hLayer)
                        layer_name = OGR_L_GetName(hLayer);
                }

                auto lyr = std::make_unique<GDALVector>();
                lyr->setDsn_(ds_name);
                lyr->setGDALDatasetH_(hDS, true);
                lyr->setOGRLayerH_(hLayer, layer_name);
                if (hLayer)
                    lyr->setFieldNames_();
                out = Rcpp::wrap(*lyr.release());
            }
            else if (ds_type & GDAL_OF_MULTIDIM_RASTER) {
                // multidim raster currently only as dataset name
                out = Rcpp::wrap(GDALArgDatasetValueGetName(hArgDSValue));
            }
            else {
                // should not occur
                out = Rcpp::wrap("unrecognized dataset type");
            }

            GDALArgDatasetValueRelease(hArgDSValue);
        }
        break;

        case GAAT_DATASET_LIST:
        {
            // seems to apply to input only, at least currently
            Rcpp::warning(
                "unhandled output of type DATASET_LIST (NULL returned)");
            out = R_NilValue;
        }
        break;

        default:
        {
            Rcpp::Rcout << "GDALAlgorithmArgType: " <<
                GDALAlgorithmArgGetType(hArg) << "\n";

            out = Rcpp::wrap("unhandled argument type");
        }
        break;
    }

    return out;
}
#endif  // GDALALG_MIN_GDAL_

// ****************************************************************************

RCPP_MODULE(mod_GDALAlg) {
    Rcpp::class_<GDALAlg>("GDALAlg")

    .constructor
        ("Default constructor, instantiates the root \"gdal\" entry point")
    .constructor<Rcpp::CharacterVector>
        ("Usage: new(GDALAlg, cmd)")
    .constructor<Rcpp::CharacterVector, Rcpp::RObject>
        ("Usage: new(GDALAlg, cmd, cl_arg)")

    // undocumented read-only fields for internal use
    .field_readonly("m_haveParsedCmdLineArgs",
                    &GDALAlg::m_haveParsedCmdLineArgs)
    .field_readonly("m_hasRun", &GDALAlg::m_hasRun)
    .field_readonly("m_hasFinalized", &GDALAlg::m_hasFinalized)

    // read/write fields
    .field("setVectorArgsFromObject", &GDALAlg::setVectorArgsFromObject)
    .field("outputLayerNameForOpen", &GDALAlg::outputLayerNameForOpen)
    .field("quiet", &GDALAlg::quiet)

    // methods
    .const_method("info", &GDALAlg::info,
        "Return a list of algorithm information")
    .const_method("argInfo", &GDALAlg::argInfo,
        "Return a list of information for an algorithm argument")
    .const_method("usage", &GDALAlg::usage,
        "Print usage information to the console")
    .const_method("usageAsJSON", &GDALAlg::usageAsJSON,
        "Return a list of algorithm information")
    .method("setArg", &GDALAlg::setArg,
        "Set the value of an algorithm argument")
    .method("parseCommandLineArgs", &GDALAlg::parseCommandLineArgs,
        "Parse command line arguments")
    .const_method("getExplicitlySetArgs", &GDALAlg::getExplicitlySetArgs,
        "Return a named list of explicitly set arguments and their values")
    .method("run", &GDALAlg::run,
        "Execute the algorithm")
    .const_method("output", &GDALAlg::output,
        "Return the single output value of this algorithm")
    .const_method("outputs", &GDALAlg::outputs,
        "Return the output value(s) of this algorithm as a named list")
    .method("close", &GDALAlg::close,
        "Complete any pending actions, and return the final status")
    .method("release", &GDALAlg::release,
        "Release memory associated with the algorithm (potentially finalize)")
    .const_method("show", &GDALAlg::show,
        "S4 show()")

    ;
}
