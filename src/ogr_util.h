/* GDAL OGR utility functions for vector data sources
   Chris Toney <chris.toney at usda.gov> */

#ifndef ogr_util_H
#define ogr_util_H

#include <Rcpp.h> 

#include <string>

bool _ogr_ds_exists(std::string dsn, bool with_update);
int _ogr_ds_layer_count(std::string dsn);
bool _ogr_layer_exists(std::string dsn, std::string layer);
bool _ogr_layer_create(std::string dsn, std::string layer,
		std::string srs, Rcpp::Nullable<Rcpp::CharacterVector> options);
bool _ogr_layer_delete(std::string dsn, std::string layer);
int _ogr_field_index(std::string dsn, std::string layer, std::string fld_name);
bool _ogr_field_create(std::string dsn, std::string layer,
		std::string fld_name);

#endif

