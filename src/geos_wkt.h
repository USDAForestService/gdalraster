/* GEOS wrapper functions operating on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.
   For spatial predicate definitions: https://en.wikipedia.org/wiki/DE-9IM

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#ifndef SRC_GEOS_WKT_H_
#define SRC_GEOS_WKT_H_

#include <string>
#include <vector>

#include <Rcpp.h>

std::vector<int> getGEOSVersion();
bool has_geos();  // GDAL built against GEOS is required at gdalraster 1.10

std::string g_create(Rcpp::NumericMatrix xy, std::string geom_type);
std::string g_add_geom(std::string sub_geom, std::string container);
bool g_is_valid(std::string geom);
bool g_is_empty(std::string geom);
std::string g_name(std::string geom);

bool g_intersects(std::string this_geom, std::string other_geom);
bool g_equals(std::string this_geom, std::string other_geom);
bool g_disjoint(std::string this_geom, std::string other_geom);
bool g_touches(std::string this_geom, std::string other_geom);
bool g_contains(std::string this_geom, std::string other_geom);
bool g_within(std::string this_geom, std::string other_geom);
bool g_crosses(std::string this_geom, std::string other_geom);
bool g_overlaps(std::string this_geom, std::string other_geom);

std::string g_buffer(std::string geom, double dist, int quad_segs);

std::string g_intersection(std::string this_geom, std::string other_geom);
std::string g_union(std::string this_geom, std::string other_geom);
std::string g_difference(std::string this_geom, std::string other_geom);
std::string g_sym_difference(std::string this_geom, std::string other_geom);

double g_distance(std::string this_geom, std::string other_geom);
double g_length(std::string geom);
double g_area(std::string geom);
Rcpp::NumericVector g_centroid(std::string geom);

std::string g_transform(std::string geom, std::string srs_from,
                         std::string srs_to, bool wrap_date_line,
                         int date_line_offset);

#endif  // SRC_GEOS_WKT_H_
