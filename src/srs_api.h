/* Spatial reference system functions operating on WKT
   Wraps a subset of the GDAL Spatial Reference System C API (ogr_srs_api.h)

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef SRS_API_H_
#define SRS_API_H_

#include <Rcpp.h>
#include <string>

std::string epsg_to_wkt(int epsg, bool pretty);
std::string srs_to_wkt(const std::string &srs, bool pretty);
std::string srs_to_wkt(const std::string &srs, bool pretty, bool gcs_only);
std::string srs_to_projjson(const std::string &srs, bool multiline,
                            int indent_width, const std::string &schema);

std::string srs_get_name(const std::string &srs);
SEXP srs_find_epsg(const std::string &srs, bool all_matches);
bool srs_is_geographic(const std::string &srs);
bool srs_is_derived_gcs(const std::string &srs);
bool srs_is_local(const std::string &srs);
bool srs_is_projected(const std::string &srs);
bool srs_is_compound(const std::string &srs);
bool srs_is_geocentric(const std::string &srs);
bool srs_is_vertical(const std::string &srs);
bool srs_is_dynamic(const std::string &srs);
bool srs_is_same(const std::string &srs1, const std::string &srs2,
                 std::string criterion, bool ignore_axis_mapping,
                 bool ignore_coord_epoch);
SEXP srs_get_angular_units(const std::string &srs);
SEXP srs_get_linear_units(const std::string &srs);
double srs_get_coord_epoch(const std::string &srs);
int srs_get_utm_zone(const std::string &srs);
std::string srs_get_axis_mapping_strategy(const std::string &srs);

#endif  // SRS_API_H_
