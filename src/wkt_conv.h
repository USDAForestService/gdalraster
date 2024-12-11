/* WKT-related convenience functions
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#ifndef SRC_WKT_CONV_H_
#define SRC_WKT_CONV_H_

#include <Rcpp.h>
#include <string>

std::string epsg_to_wkt(int epsg, bool pretty);
std::string srs_to_wkt(const std::string &srs, bool pretty);
bool srs_is_geographic(const std::string &srs);
bool srs_is_projected(const std::string &srs);
bool srs_is_same(const std::string &srs1, const std::string &srs2,
                 std::string criterion, bool ignore_axis_mapping,
                 bool ignore_coord_epoch);

Rcpp::NumericVector bbox_from_wkt(const std::string &wkt,
                                  double extend_x, double extend_y);

Rcpp::String bbox_to_wkt(const Rcpp::NumericVector &bbox,
                         double extend_x, double extend_y);



#endif  // SRC_WKT_CONV_H_
