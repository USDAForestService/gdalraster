/* R interface to GDALAlgorithm and related classes
   Chris Toney <jctoney at gmail.com>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include "gdalalg.h"

#include <cpl_port.h>
#include <cpl_string.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

#include "gdalraster.h"

#define GDALALG_MIN_GDAL_ GDAL_COMPUTE_VERSION(3, 11, 3)

constexpr char GDALALG_MIN_GDAL_MSG_[] =
    "GDAL CLI bindings require GDAL >= 3.11.3";

#if GDAL_VERSION_NUM >= GDALALG_MIN_GDAL_
constexpr R_xlen_t CMD_TOKENS_MAX_ = 5;

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
                               std::vector<std::string> *names,
                               std::vector<std::string> *desc,
                               std::vector<std::string> *urls,
                               const std::string &contains,
                               bool cout) {

    char **subnames = nullptr;
    subnames = GDALAlgorithmGetSubAlgorithmNames(alg);
    int num_subnames = CSLCount(subnames);

    for (int i = 0; i < num_subnames; ++i) {
        GDALAlgorithmH subalg = nullptr;
        subalg = GDALAlgorithmInstantiateSubAlgorithm(alg, subnames[i]);
        if (!subalg) {
            Rcpp::Rcout << "failed to instantiate alg name: " << subnames[i] <<
                "\n";
            continue;
        }

        std::string this_cmd_str = cmd_str + " " + GDALAlgorithmGetName(subalg);
        bool cout_this = true;
        if (contains == "" ||
            this_cmd_str.find(contains) != std::string::npos) {

            names->push_back(this_cmd_str);
            desc->push_back(GDALAlgorithmGetDescription(subalg));
            urls->push_back(GDALAlgorithmGetHelpFullURL(subalg));
        }
        else {
            cout_this = false;
        }

        if (cout && cout_this) {
            Rcpp::Rcout << this_cmd_str.c_str() << ":\n";
            Rcpp::Rcout << GDALAlgorithmGetDescription(subalg) << "\n";
            if (!EQUAL(GDALAlgorithmGetHelpFullURL(subalg), ""))
                Rcpp::Rcout << GDALAlgorithmGetHelpFullURL(subalg) << "\n";
            Rcpp::Rcout << "\n";
        }

        if (GDALAlgorithmHasSubAlgorithms(subalg)) {
            append_subalg_names_desc_(subalg, this_cmd_str, names, desc, urls,
                                      contains, cout);
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
                              bool cout) {

#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);

#else
    GDALAlgorithmRegistryH reg = nullptr;
    reg = GDALGetGlobalAlgorithmRegistry();
    if (!reg)
        Rcpp::stop("failed to obtain global algorithm registry");

    GDALAlgorithmH gdal_alg = nullptr;
    gdal_alg = GDALAlgorithmRegistryInstantiateAlg(reg,
        GDALGlobalAlgorithmRegistry::ROOT_ALG_NAME);

    if (!gdal_alg) {
        GDALAlgorithmRegistryRelease(reg);
        Rcpp::stop("failed to instantiate \"gdal\" entry point");
    }

    char **names = nullptr;
    names = GDALAlgorithmGetSubAlgorithmNames(gdal_alg);
    int num_names = CSLCount(names);
    if (num_names == 0) {
        GDALAlgorithmRegistryRelease(reg);
        Rcpp::stop("failed to obtain top-level algorithm names");
    }

    std::vector<std::string> cmd_names = {};
    std::vector<std::string> cmd_descriptions = {};
    std::vector<std::string> cmd_urls = {};

    const std::string &contains_in = str_tolower_(contains);

    for (int i = 0; i < num_names; ++i) {
        GDALAlgorithmH alg = nullptr;
        alg = GDALAlgorithmRegistryInstantiateAlg(reg, names[i]);
        if (!alg) {
            Rcpp::Rcout << "failed to instantiate alg name: " << names[i] <<
                "\n";
            continue;
        }

        bool cout_this = true;
        if (contains_in == "" ||
            std::string(names[i]).find(contains_in) != std::string::npos) {

            cmd_names.push_back(names[i]);
            cmd_descriptions.push_back(GDALAlgorithmGetDescription(alg));
            cmd_urls.push_back(GDALAlgorithmGetHelpFullURL(alg));
        }
        else {
            cout_this = false;
        }

        if (cout && cout_this) {
            Rcpp::Rcout << names[i] << ":\n";
            Rcpp::Rcout << GDALAlgorithmGetDescription(alg) << "\n";
            if (!EQUAL(GDALAlgorithmGetHelpFullURL(alg), ""))
                Rcpp::Rcout << GDALAlgorithmGetHelpFullURL(alg) << "\n";
            Rcpp::Rcout << "\n";
        }

        if (recurse && GDALAlgorithmHasSubAlgorithms(alg)) {
            append_subalg_names_desc_(alg, std::string(names[i]),
                                      &cmd_names, &cmd_descriptions,
                                      &cmd_urls, contains_in, cout);
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

    char **names = nullptr;
    names = GDALAlgorithmRegistryGetAlgNames(reg);
    int num_names = CSLCount(names);
    for (int i = 0; i < num_names; ++i) {
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

GDALAlg::GDALAlg() :
            GDALAlg("gdal", Rcpp::CharacterVector::create())  {}

GDALAlg::GDALAlg(const Rcpp::CharacterVector &cmd) :
            GDALAlg(cmd, Rcpp::CharacterVector::create()) {}

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

    bool has_tokens = false;
    for (char c : m_cmd_str) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            has_tokens = true;
            break;
        }
    }

    if (has_tokens) {
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
    alg_info.push_back(GDALAlgorithmGetDescription(alg), "description");
    alg_info.push_back(GDALAlgorithmGetLongDescription(alg),
                       "long_description");
    alg_info.push_back(GDALAlgorithmGetHelpFullURL(alg), "URL");
    alg_info.push_back(GDALAlgorithmHasSubAlgorithms(alg),
                       "has_subalgorithms");

    if (GDALAlgorithmHasSubAlgorithms(alg)) {
        char **papszNames = GDALAlgorithmGetSubAlgorithmNames(alg);
        int nCount = 0;
        nCount = CSLCount(papszNames);
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

    char **papszArgNames = nullptr;
    papszArgNames = GDALAlgorithmGetArgNames(alg);
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

    if (arg_name == "" || arg_name == NA_STRING)
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
    int nCount = 0;
    nCount = CSLCount(papszAliases);
    if (papszAliases && nCount > 0) {
        std::vector<std::string> v(papszAliases, papszAliases + nCount);
        arg_info.push_back(Rcpp::wrap(v), "aliases");
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
        int nCount = 0;
        nCount = CSLCount(papszChoices);
        if (papszChoices && nCount > 0) {
            std::vector<std::string> v(papszChoices, papszChoices + nCount);
            arg_info.push_back(Rcpp::wrap(v), "choices");
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
            Rcpp::CharacterVector val = Rcpp::CharacterVector::create();
            if (papszValue && nCount > 0) {
                std::vector<std::string> v(papszValue, papszValue + nCount);
                val = Rcpp::wrap(v);
            }
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
            Rcpp::IntegerVector val = Rcpp::IntegerVector::create();
            if (panValue && nCount > 0) {
                std::vector<int> v(panValue, panValue + nCount);
                val = Rcpp::wrap(v);
            }
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
            Rcpp::NumericVector val = Rcpp::NumericVector::create();
            if (padfValue && nCount > 0) {
                std::vector<double> v(padfValue, padfValue + nCount);
                val = Rcpp::wrap(v);
            }
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
    Rcpp::Environment pkg = Rcpp::Environment::namespace_env("gdalraster");
    Rcpp::Function fn = pkg[".print_alg_usage"];
    fn(Rcpp::wrap(m_cmd_str));
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

bool GDALAlg::parseCommandLineArgs() {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    // parses cl args which sets the values, and also instantiates m_hActualAlg

    if (!m_hAlg)
        Rcpp::stop("algorithm not instantiated");

    if (m_haveParsedCmdLineArgs) {
        Rcpp::stop(
            "parseCommandLineArgs() can only be called once per instance");
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
        (m_input_GDALVector || m_like_GDALVector)) {

        // directly set certain arguments from input GDALVector object(s)

        if (m_input_GDALVector) {
            GDALAlgorithmArgH hArg = GDALAlgorithmGetArg(m_hAlg, "input");
            if (GDALAlgorithmArgGetType(hArg) == GAAT_DATASET_LIST) {
                if (m_num_input_datasets > 1) {
                    GDALAlgorithmArgRelease(hArg);
                    Rcpp::stop(
                        "set args from GDALVector currently unsupported for multiple input objects");
                }
            }
            else if (GDALAlgorithmArgGetType(hArg) != GAAT_DATASET) {
                GDALAlgorithmArgRelease(hArg);
                Rcpp::stop(
                    "set args from GDALVector incompatible with \"input\" arg type");
            }
            GDALAlgorithmArgRelease(hArg);
        }

        const Rcpp::List &alginfo = info();
        const Rcpp::CharacterVector &arg_names = alginfo["arg_names"];
        const std::string warning_msg =
            "value given in 'args' overrides setting from input object";

        for (Rcpp::String arg_name : arg_names) {
            const char *pszArgName = arg_name.get_cstring();

            // TODO: arg aliases are currently hard coded, look up instead

            if (m_input_GDALVector && EQUAL(pszArgName, "input-format")) {
                if (contains_str_(m_args, "--input-format") ||
                    contains_str_(m_args, "--if")) {

                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(warning_msg);
                    }
                }
                else {
                    auto hArg = GDALAlgorithmGetArg(m_hAlg, pszArgName);
                    // this one can fail silently since it is optional anyway
                    if (hArg) {
                        res = GDALAlgorithmArgSetAsString(hArg,
                            m_input_GDALVector->getDriverShortName().c_str());

                        GDALAlgorithmArgRelease(hArg);
                    }
                }
                continue;
            }
            else if (m_input_GDALVector && EQUAL(pszArgName, "input-layer") &&
                     !m_input_GDALVector->m_is_sql) {

                if (contains_str_(m_args, "--input-layer") ||
                    contains_str_(m_args, "--layer") ||
                    contains_str_(m_args, "-l")) {

                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(warning_msg);
                    }
                }
                else {
                    auto hArg = GDALAlgorithmGetArg(m_hAlg, pszArgName);
                    if (hArg) {
                        res = GDALAlgorithmArgSetAsString(hArg,
                            m_input_GDALVector->getName().c_str());

                        GDALAlgorithmArgRelease(hArg);
                    }
                    else {
                        res = false;
                    }
                    if (!res) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::Rcout << "  failed to set from object\n";
                        break;
                    }
                }
                continue;
            }
            else if (m_input_GDALVector && EQUAL(pszArgName, "sql") &&
                     m_input_GDALVector->m_is_sql) {

                if (contains_str_(m_args, "--sql")) {
                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(warning_msg);
                    }
                }
                else {
                    auto hArg = GDALAlgorithmGetArg(m_hAlg, pszArgName);
                    if (hArg) {
                        const std::string &sql =
                            m_input_GDALVector->m_layer_name;

                        res = GDALAlgorithmArgSetAsString(hArg, sql.c_str());
                        GDALAlgorithmArgRelease(hArg);
                    }
                    else {
                        res = false;
                    }
                }
                if (!res) {
                    Rcpp::Rcout << "argument: " << pszArgName << "\n";
                    Rcpp::Rcout << "  failed to set from object\n";

                    break;
                }
                continue;
            }
            else if (m_input_GDALVector && EQUAL(pszArgName, "dialect") &&
                     m_input_GDALVector->m_is_sql) {

                if (contains_str_(m_args, "--dialect")) {
                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(warning_msg);
                    }
                }
                else {
                    auto hArg = GDALAlgorithmGetArg(m_hAlg, pszArgName);
                    if (hArg) {
                        const std::string &dialect =
                            m_input_GDALVector->m_dialect;

                        if (dialect != "") {
                            res = GDALAlgorithmArgSetAsString(hArg,
                                                              dialect.c_str());
                        }
                        GDALAlgorithmArgRelease(hArg);
                    }
                    else {
                        res = false;
                    }
                }
                if (!res) {
                    Rcpp::Rcout << "argument: " << pszArgName << "\n";
                    Rcpp::Rcout << "  failed to set from object\n";

                    break;
                }
                continue;
            }
            else if (m_like_GDALVector && EQUAL(pszArgName, "like-sql") &&
                     m_like_GDALVector->m_is_sql) {

                if (contains_str_(m_args, "--like-sql")) {
                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(warning_msg);
                    }
                }
                else {
                    auto hArg = GDALAlgorithmGetArg(m_hAlg, pszArgName);
                    if (hArg) {
                        const std::string &sql =
                            m_like_GDALVector->m_layer_name;

                        res = GDALAlgorithmArgSetAsString(hArg,
                                                          sql.c_str());

                        GDALAlgorithmArgRelease(hArg);
                    }
                    else {
                        res = false;
                    }
                }
                if (!res) {
                    Rcpp::Rcout << "argument: " << pszArgName << "\n";
                    Rcpp::Rcout << "  failed to set from object\n";

                    break;
                }
                continue;
            }
            else if (m_like_GDALVector && EQUAL(pszArgName, "like-layer") &&
                     !m_like_GDALVector->m_is_sql) {

                if (contains_str_(m_args, "--like-layer")) {
                    if (!quiet) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::warning(warning_msg);
                    }
                }
                else {
                    auto hArg = GDALAlgorithmGetArg(m_hAlg, pszArgName);
                    if (hArg) {
                        res = GDALAlgorithmArgSetAsString(hArg,
                            m_like_GDALVector->getName().c_str());

                        GDALAlgorithmArgRelease(hArg);
                    }
                    else {
                        res = false;
                    }
                    if (!res) {
                        Rcpp::Rcout << "argument: " << pszArgName << "\n";
                        Rcpp::Rcout << "  failed to set from object\n";
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

bool GDALAlg::run() {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else

    if (!m_hAlg)
        Rcpp::stop("algorithm not instantiated");

    if (m_hasRun)
        Rcpp::stop("algorithm has already run");

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
        Rcpp::stop("no output arg names found");
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
        Rcpp::stop("no output arg names found");

    Rcpp::List out = Rcpp::List::create();

    for (std::string arg_name : out_arg_names) {
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
            Rcpp::String s(arg_name);
            s.replace_all("-", "_");
            out.push_back(getOutputArgValue_(hArg), s);
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

    if (!m_hAlg)
        Rcpp::stop("algorithm not instantiated");

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
            Rcpp::Rcout << "actual algorithm is nullptr\n";
        return false;
    }
#endif  // GDALALG_MIN_GDAL_
}

void GDALAlg::release() {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
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
        Rcpp::stop("arg list must have named elements");

    for (R_xlen_t i = 0; i < num_args; ++i) {
        Rcpp::String nm(arg_names[i]);
        if (nm == NA_STRING || nm == "")
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
                const Rcpp::RObject &val = list_tmp[j];
                if (!val.isNULL() && val.isObject()) {
                    const Rcpp::String cls = val.attr("class");
                    if (cls == "Rcpp_GDALRaster") {
                        const GDALRaster* const &ds = list_tmp[j];
                        ds_list.push_back(ds->getGDALDatasetH_());
                    }
                    else if (cls == "Rcpp_GDALVector") {
                        GDALVector* const &ds = list_tmp[j];
                        ds_list.push_back(ds->getGDALDatasetH_());
                        if (EQUAL(nm.get_cstring(), "--input") &&
                            !m_input_GDALVector) {

                            m_input_GDALVector = ds;
                        }
                    }
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
        const Rcpp::RObject &val = list_args[i];
        if (val.isObject()) {
            const Rcpp::String cls = val.attr("class");
            if (cls == "Rcpp_GDALRaster") {
                const GDALRaster* const &ds = list_args[i];
                std::vector<GDALDatasetH> ds_list = {};
                ds_list.push_back(ds->getGDALDatasetH_());
                m_map_input_hDS[nm_no_lead_dashes] = ds_list;
                continue;
            }
            if (cls == "Rcpp_GDALVector") {
                GDALVector* const &ds = list_args[i];
                std::vector<GDALDatasetH> ds_list = {};
                ds_list.push_back(ds->getGDALDatasetH_());
                m_map_input_hDS[nm_no_lead_dashes] = ds_list;
                if (EQUAL(nm.get_cstring(), "--input"))
                    m_input_GDALVector = ds;
                else if (EQUAL(nm.get_cstring(), "--like"))
                    m_like_GDALVector = ds;
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
        if (m_num_input_datasets > 1 && m_input_GDALVector) {
            // becasue currently, auto setting args from GDALVector input is
            // not supported for multiple input datasets
            m_input_GDALVector = nullptr;
        }
    }

    return arg_vec;
}

void GDALAlg::instantiateAlg_() {
#if GDAL_VERSION_NUM < GDALALG_MIN_GDAL_
    Rcpp::stop(GDALALG_MIN_GDAL_MSG_);
#else
    // instantiate m_hAlg

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
                    Rcpp::Rcout << "got nullptr for arg: " << arg_name.c_str()
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
SEXP GDALAlg::getOutputArgValue_(const GDALAlgorithmArgH &hArg) const {
    if (!hArg)
        Rcpp::stop("got nullptr for GDALAlgorithmArgH hArg");

    SEXP out = R_NilValue;

    switch (GDALAlgorithmArgGetType(hArg)) {
        case GAAT_BOOLEAN:
        {
            Rcpp::LogicalVector v =
                Rcpp::LogicalVector::create(GDALAlgorithmArgGetAsBoolean(hArg));
            out = v;
        }
        break;

        case GAAT_STRING:
        {
            out = Rcpp::wrap(GDALAlgorithmArgGetAsString(hArg));
        }
        break;

        case GAAT_INTEGER:
        {
            Rcpp::IntegerVector v =
                Rcpp::IntegerVector::create(GDALAlgorithmArgGetAsInteger(hArg));
            out = v;
        }
        break;

        case GAAT_REAL:
        {
            Rcpp::NumericVector v =
                Rcpp::NumericVector::create(GDALAlgorithmArgGetAsDouble(hArg));
            out = v;
        }
        break;

        case GAAT_STRING_LIST:
        {
            char **papszValue = nullptr;
            papszValue = GDALAlgorithmArgGetAsStringList(hArg);
            int nCount = CSLCount(papszValue);
            if (papszValue && nCount > 0) {
                std::vector<std::string> v(papszValue, papszValue + nCount);
                out = Rcpp::wrap(v);
            }
            else {
                Rcpp::CharacterVector v =
                    Rcpp::CharacterVector::create(NA_STRING);
                out = v;
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
                std::vector<int> v(panValue, panValue + nCount);
                out = Rcpp::wrap(v);
            }
            else {
                Rcpp::IntegerVector v =
                    Rcpp::IntegerVector::create(NA_INTEGER);
                out = v;
            }
        }
        break;

        case GAAT_REAL_LIST:
        {
            size_t nCount = 0;
            const double *padfValue =
                GDALAlgorithmArgGetAsDoubleList(hArg, &nCount);

            if (padfValue && nCount > 0) {
                std::vector<double> v(padfValue, padfValue + nCount);
                out = Rcpp::wrap(v);
            }
            else {
                Rcpp::NumericVector v =
                    Rcpp::NumericVector::create(NA_REAL);
                out = v;
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

            GDALArgDatasetType ds_type = GDALAlgorithmArgGetDatasetType(hArg);
            bool with_update = false;
            if (ds_type & GDAL_OF_UPDATE)
                with_update = true;

            GDALDatasetH hDS = nullptr;
            hDS = GDALArgDatasetValueGetDatasetIncreaseRefCount(hArgDSValue);
            if (!hDS) {
                GDALArgDatasetValueRelease(hArgDSValue);
                Rcpp::stop("GDAL dataset object is NULL");
            }

            // raster
            if (ds_type & GDAL_OF_RASTER) {
                std::string ds_name(GDALArgDatasetValueGetName(hArgDSValue));
                GDALRaster *ds = new GDALRaster();
                ds->setFilename(ds_name);
                ds->setGDALDatasetH_(hDS, with_update);
                const GDALRaster &ds_ref = *ds;
                out = Rcpp::wrap(ds_ref);
            }
            // vector
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

                GDALVector *lyr = new GDALVector();
                lyr->setDsn_(ds_name);
                lyr->setGDALDatasetH_(hDS, true);
                lyr->setOGRLayerH_(hLayer, layer_name);
                if (hLayer)
                    lyr->setFieldNames_();
                const GDALVector &lyr_ref = *lyr;
                out = Rcpp::wrap(lyr_ref);
            }
            // multidim raster - currently only as dataset name
            else if (ds_type & GDAL_OF_MULTIDIM_RASTER) {
                out = Rcpp::wrap(GDALArgDatasetValueGetName(hArgDSValue));
            }
            // unrecognized dataset type - should not occur
            else {
                out = Rcpp::wrap("unrecognized dataset type");
            }

            GDALArgDatasetValueRelease(hArgDSValue);
        }
        break;

        case GAAT_DATASET_LIST:
        {
            // TODO: confirm
            // This seems to apply to input only, at least currently.
            Rcpp::warning(
                "unhandled output of type DATASET_LIST (returned NULL)");
            out = R_NilValue;
        }
        break;

        default:
        {
            Rcpp::Rcout << "GDALAlgorithmArgType: " <<
                GDALAlgorithmArgGetType(hArg) << "\n";

            out = Rcpp::wrap("unhandled arg type");
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
    .method("parseCommandLineArgs", &GDALAlg::parseCommandLineArgs,
        "Parse command line arguments")
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
