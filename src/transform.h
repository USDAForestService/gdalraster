/* Functions for coordinate transformation using PROJ via GDAL headers
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include <Rcpp.h>

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
                                const std::string &well_known_gcs);

Rcpp::NumericMatrix transform_xy(const Rcpp::RObject &pts,
                                 const std::string &srs_from,
                                 const std::string &srs_to);

SEXP transform_bounds(const Rcpp::RObject &bbox,
                      const std::string &srs_from,
                      const std::string &srs_to,
                      int densify_pts,
                      bool traditional_gis_order);

#endif  // TRANSFORM_H_
