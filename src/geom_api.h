/* GEOS wrapper functions operating on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.
   For spatial predicate definitions: https://en.wikipedia.org/wiki/DE-9IM

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#ifndef SRC_GEOM_API_H_
#define SRC_GEOM_API_H_

#include <string>
#include <vector>

#include <Rcpp.h>

#include "ogr_geometry.h"


std::vector<int> getGEOSVersion();
bool has_geos();  // GDAL built against GEOS is required at gdalraster 1.10

OGRGeometryH createGeomFromWkb(const Rcpp::RawVector &wkb);
bool exportGeomToWkb(OGRGeometryH hGeom, unsigned char *wkb, bool as_iso,
                     const std::string &byte_order);

std::string g_wkb2wkt(const Rcpp::RawVector &geom, bool as_iso);

Rcpp::CharacterVector g_wkb_list2wkt(const Rcpp::List &geom, bool as_iso);

Rcpp::RawVector g_wkt2wkb(const std::string &geom, bool as_iso,
                          const std::string &byte_order);

Rcpp::List g_wkt_vector2wkb(const Rcpp::CharacterVector &geom, bool as_iso,
                            const std::string &byte_order);

std::string g_create(const Rcpp::NumericMatrix &xy, std::string geom_type);
std::string g_add_geom(const std::string &sub_geom,
                       const std::string &container);

SEXP g_is_valid(const Rcpp::RawVector &geom, bool quiet);
SEXP g_make_valid(const Rcpp::RawVector &geom, const std::string &method,
                  bool keep_collapsed, bool as_iso,
                  const std::string &byte_order, bool quiet);

SEXP g_is_empty(const Rcpp::RawVector &geom, bool quiet);
SEXP g_name(const Rcpp::RawVector &geom, bool quiet);
SEXP g_summary(const Rcpp::RawVector &geom, bool quiet);

bool g_intersects(const std::string &this_geom, const std::string &other_geom);
bool g_equals(const std::string &this_geom, const std::string &other_geom);
bool g_disjoint(const std::string &this_geom, const std::string &other_geom);
bool g_touches(const std::string &this_geom, const std::string &other_geom);
bool g_contains(const std::string &this_geom, const std::string &other_geom);
bool g_within(const std::string &this_geom, const std::string &other_geom);
bool g_crosses(const std::string &this_geom, const std::string &other_geom);
bool g_overlaps(const std::string &this_geom, const std::string &other_geom);

SEXP g_buffer(const Rcpp::RawVector &geom, double dist, int quad_segs,
              bool as_iso, const std::string &byte_order, bool quiet);

std::string g_intersection(const std::string &this_geom,
                           const std::string &other_geom);

std::string g_union(const std::string &this_geom,
                    const std::string &other_geom);

std::string g_difference(const std::string &this_geom,
                         const std::string &other_geom);

std::string g_sym_difference(const std::string &this_geom,
                             const std::string &other_geom);

double g_distance(const std::string &this_geom, const std::string &other_geom);
double g_length(const std::string &geom);
double g_area(const std::string &geom);
Rcpp::NumericVector g_centroid(const std::string &geom);

std::string g_transform(const std::string &geom, const std::string &srs_from,
                        const std::string &srs_to, bool wrap_date_line,
                        int date_line_offset);

#endif  // SRC_GEOM_API_H_
