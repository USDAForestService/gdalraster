/* GEOS wrapper functions operating on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.
   For spatial predicate definitions: https://en.wikipedia.org/wiki/DE-9IM
   Chris Toney <chris.toney at usda.gov> */

#ifndef geos_wkt_H
#define geos_wkt_H

#include <Rcpp.h> 
// [[Rcpp::plugins(cpp11)]]

#include <string>

bool has_geos();

std::string _g_create(Rcpp::NumericMatrix xy, std::string geom_type);
bool _g_is_valid(std::string geom);

bool _g_intersects(std::string this_geom, std::string other_geom);
bool _g_equals(std::string this_geom, std::string other_geom);
bool _g_disjoint(std::string this_geom, std::string other_geom);
bool _g_touches(std::string this_geom, std::string other_geom);
bool _g_contains(std::string this_geom, std::string other_geom);
bool _g_within(std::string this_geom, std::string other_geom);
bool _g_crosses(std::string this_geom, std::string other_geom);
bool _g_overlaps(std::string this_geom, std::string other_geom);

std::string _g_buffer(std::string geom, double dist, int quad_segs);

std::string _g_intersection(std::string this_geom, std::string other_geom);
std::string _g_union(std::string this_geom, std::string other_geom);
std::string _g_difference(std::string this_geom, std::string other_geom);
std::string _g_sym_difference(std::string this_geom, std::string other_geom);

double _g_distance(std::string this_geom, std::string other_geom);
double _g_length(std::string geom);
double _g_area(std::string geom);
Rcpp::NumericVector _g_centroid(std::string geom);

#endif

