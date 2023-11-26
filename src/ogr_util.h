/* GDAL OGR utility functions for vector data sources
   Chris Toney <chris.toney at usda.gov> */

#ifndef ogr_util_H
#define ogr_util_H

#include <Rcpp.h> 

#include <string>

bool _ogr_ds_exists(std::string dsn, bool with_update);

bool _create_ogr(std::string format, std::string dst_filename,
		int xsize, int ysize, int nbands, std::string dataType,
		std::string layer, std::string srs, std::string fld_name,
		Rcpp::Nullable<Rcpp::CharacterVector> dsco,
		Rcpp::Nullable<Rcpp::CharacterVector> lco);
		
int _ogr_ds_layer_count(std::string dsn);

bool _ogr_layer_exists(std::string dsn, std::string layer);

bool _ogr_layer_create(std::string dsn, std::string layer,
		std::string srs, Rcpp::Nullable<Rcpp::CharacterVector> options);
		
bool _ogr_layer_delete(std::string dsn, std::string layer);

int _ogr_field_index(std::string dsn, std::string layer, std::string fld_name);

bool _ogr_field_create(std::string dsn, std::string layer,
		std::string fld_name);

#endif

