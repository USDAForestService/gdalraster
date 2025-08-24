/* R interface to GDALAlgorithm and related classes that implement GDAL CLI
   GDAL >= MIN_GDAL_FOR_ALG_BINDINGS_
   Chris Toney <jctoney at gmail.com>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef SRC_GDALALG_H_
#define SRC_GDALALG_H_

#include "rcpp_util.h"

#if __has_include(<gdalalgorithm.h>)
    #include <gdalalgorithm.h>
#endif
#include <gdal.h>

#include <map>
#include <string>
#include <vector>

#include "gdalvector.h"

Rcpp::DataFrame gdal_commands(const std::string starts_with, bool recurse,
                              bool cout);

Rcpp::CharacterVector gdal_global_reg_names();

class GDALAlg {
 public:
    GDALAlg();
    explicit GDALAlg(const Rcpp::CharacterVector &cmd);
    GDALAlg(const Rcpp::CharacterVector &cmd,
            const Rcpp::RObject &args);
    ~GDALAlg();

    // undocumented, exposed read-only fields for internal use
    bool m_haveParsedCmdLineArgs {false};
    bool m_hasRun {false};
    bool m_hasFinalized {false};

    // exposed read/write fields
    bool setVectorArgsFromObject {true};
    Rcpp::String outputLayerNameForOpen {""};
    bool quiet {false};

    // exposed methods
    Rcpp::List info() const;
    Rcpp::List argInfo(const Rcpp::String &arg_name) const;
    void usage() const;
    Rcpp::String usageAsJSON() const;

    // TODO:
    // Rcpp::List getExplicitlySetArgs() const;
    // bool setArg(const Rcpp::String &arg_name, const SEXP &arg_value);

    bool parseCommandLineArgs();
    bool run();
    SEXP output() const;
    Rcpp::List outputs() const;
    bool close();
    void release();

    void show() const;

    // methods for internal use not exposed to R
    Rcpp::CharacterVector parseListArgs_(const Rcpp::List &list_args);
    void instantiateAlg_();
    std::vector<std::string> getOutputArgNames_() const;
#if __has_include(<gdalalgorithm.h>)
    SEXP getOutputArgValue_(const GDALAlgorithmArgH &hArg) const;
#endif

 private:
    Rcpp::CharacterVector m_cmd {};
    std::string m_cmd_str {""};
    Rcpp::CharacterVector m_args {};
#if __has_include(<gdalalgorithm.h>)
    GDALAlgorithmH m_hAlg {nullptr};
    GDALAlgorithmH m_hActualAlg {nullptr};
#endif
    bool m_input_is_object {false};
    std::map<std::string, std::vector<GDALDatasetH>> m_map_input_hDS {};
    size_t m_num_input_datasets {0};
    GDALVector *m_input_GDALVector {nullptr};
    GDALVector *m_like_GDALVector {nullptr};
};

// cppcheck-suppress unknownMacro
RCPP_EXPOSED_CLASS(GDALAlg)

#endif  // SRC_GDALALG_H_
