/* Functions for coordinate transformation using PROJ via GDAL headers
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#ifndef SRC_TRANSFORM_H_
#define SRC_TRANSFORM_H_

#include <string>
#include <vector>

#include "rcpp_util.h"

std::vector<int> getPROJVersion();
Rcpp::CharacterVector getPROJSearchPaths();
void setPROJSearchPaths(Rcpp::CharacterVector paths);
Rcpp::LogicalVector getPROJEnableNetwork();
void setPROJEnableNetwork(int enabled);

Rcpp::NumericMatrix inv_project(const Rcpp::RObject &pts,
                                const std::string &srs,
                                std::string well_known_gcs);

Rcpp::NumericMatrix transform_xy(const Rcpp::RObject &pts,
                                 const std::string &srs_from,
                                 const std::string &srs_to);


#endif  // SRC_TRANSFORM_H_
