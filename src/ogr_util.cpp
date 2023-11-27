/* Utility functions for vector data sources
   Chris Toney <chris.toney at usda.gov> */

#include "ogr_util.h"

#include "gdal.h"
#include "cpl_error.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "ogr_srs_api.h"

//' Does vector dataset exist
//' 
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_exists")]]
bool _ogr_ds_exists(std::string dsn, bool with_update = false) {

	GDALDatasetH hDS;
	
	CPLPushErrorHandler(CPLQuietErrorHandler);	
	if (with_update)
		hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
				NULL, NULL, NULL);
	else
		hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
	CPLPopErrorHandler();
	if (hDS == NULL)
		return false;

	GDALClose(hDS);
	return true;
}

//' Create a vector dataset with layer and field
//' currently hard coded as layer of wkbPolygon, field of OFTInteger
//'
//' @noRd
// [[Rcpp::export(name = ".create_ogr")]]
bool _create_ogr(std::string format, std::string dst_filename,
		int xsize, int ysize, int nbands, std::string dataType,
		std::string layer, std::string srs = "", std::string fld_name = "",
		Rcpp::Nullable<Rcpp::CharacterVector> dsco = R_NilValue,
		Rcpp::Nullable<Rcpp::CharacterVector> lco = R_NilValue) {

	GDALDriverH hDriver = GDALGetDriverByName( format.c_str() );
	if (hDriver == NULL)
		Rcpp::stop("Failed to get driver for the specified format.");
		
	char **papszMetadata = GDALGetMetadata(hDriver, NULL);
	if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATE, FALSE))
		Rcpp::stop("Driver does not support create.");
	
	GDALDataType dt = GDALGetDataTypeByName( dataType.c_str() );
	
	std::vector<char *> opt_list = {NULL};
	if (dsco.isNotNull()) {
		Rcpp::CharacterVector dsco_in(dsco);
		opt_list.resize(dsco_in.size() + 1);
		for (R_xlen_t i = 0; i < dsco_in.size(); ++i) {
			opt_list[i] = (char *) (dsco_in[i]);
		}
		opt_list[dsco_in.size()] = NULL;
	}
	
	GDALDatasetH hDstDS = NULL;
	hDstDS = GDALCreate(hDriver, dst_filename.c_str(),
						xsize, ysize, nbands, dt,
                    	opt_list.data());

	if (hDstDS == NULL)
		Rcpp::stop("Create dataset failed.");

	if (!GDALDatasetTestCapability(hDstDS, ODsCCreateLayer)) {
		GDALClose(hDstDS);
		return false;
	}

	OGRLayerH  hLayer;
	OGRFieldDefnH hFieldDefn;
	bool layer_ok = false;
	bool fld_ok = false;
	
	opt_list.clear();
	if (lco.isNotNull()) {
		Rcpp::CharacterVector lco_in(lco);
		opt_list.resize(lco_in.size() + 1);
		for (R_xlen_t i = 0; i < lco_in.size(); ++i) {
			opt_list[i] = (char *) (lco_in[i]);
		}
	}
	opt_list.push_back(NULL);
	
	OGRSpatialReferenceH hSRS = OSRNewSpatialReference(NULL);
	if (srs != "") {
		if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
			GDALClose(hDstDS);
			Rcpp::stop("Error importing SRS from user input.");
		}
	}

	hLayer = GDALDatasetCreateLayer(hDstDS, layer.c_str(), hSRS, wkbPolygon,
				opt_list.data());
	
	if (hLayer == NULL) {
		layer_ok = false;
	}
	else {
		layer_ok = true;
		hFieldDefn = OGR_Fld_Create(fld_name.c_str(), OFTInteger);
		if (OGR_L_CreateField(hLayer, hFieldDefn, TRUE) != OGRERR_NONE)
			fld_ok = false;
		else
			fld_ok = true;

		OGR_Fld_Destroy(hFieldDefn);
	}
	
	OSRDestroySpatialReference(hSRS);
	GDALClose(hDstDS);
	
	if (layer_ok && fld_ok)
		return true;
	else
		return false;
}

//' Get number of layers in a dataset
//' 
//' @noRd
// [[Rcpp::export(name = ".ogr_ds_layer_count")]]
int _ogr_ds_layer_count(std::string dsn) {

	GDALDatasetH hDS;
	
	CPLPushErrorHandler(CPLQuietErrorHandler);
	hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
	if (hDS == NULL)
		return -1;
	CPLPopErrorHandler();
	
	int cnt = GDALDatasetGetLayerCount(hDS);
	GDALClose(hDS);
	return cnt;
}

//' Does layer exist
//' 
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_exists")]]
bool _ogr_layer_exists(std::string dsn, std::string layer) {

	GDALDatasetH hDS;
	OGRLayerH hLayer;
	bool ret;
	
	CPLPushErrorHandler(CPLQuietErrorHandler);
	hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
	if (hDS == NULL)
		return false;
	hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
	CPLPopErrorHandler();
	if (hLayer == NULL)
		ret = false;
	else
		ret = true;
	
	GDALClose(hDS);
	return ret;
}

//' Create a layer in a vector dataset
//' currently hard coded as layer of wkbPolygon
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_create")]]
bool _ogr_layer_create(std::string dsn, std::string layer,
		std::string srs = "",
		Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

	GDALDatasetH hDS;
	OGRLayerH  hLayer;
	bool ret;

	OGRSpatialReferenceH hSRS = OSRNewSpatialReference(NULL);
	if (srs != "") {
		if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE)
			Rcpp::stop("Error importing SRS from user input.");
	}
		
	hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
			NULL, NULL, NULL);
			
	if (hDS == NULL)
		return false;

	if (!GDALDatasetTestCapability(hDS, ODsCCreateLayer)) {
		GDALClose(hDS);
		return false;
	}
	
	std::vector<char *> opt_list = {NULL};
	if (options.isNotNull()) {
		Rcpp::CharacterVector options_in(options);
		opt_list.resize(options_in.size() + 1);
		for (R_xlen_t i = 0; i < options_in.size(); ++i) {
			opt_list[i] = (char *) (options_in[i]);
		}
		opt_list[options_in.size()] = NULL;
	}

	hLayer = GDALDatasetCreateLayer(hDS, layer.c_str(), hSRS, wkbPolygon,
				opt_list.data());
	
	if (hLayer == NULL)
		ret = false;
	else
		ret = true;
		
	OSRDestroySpatialReference(hSRS);
	GDALClose(hDS);
	return ret;
}

//' Delete a layer in a vector dataset
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_layer_delete")]]
bool _ogr_layer_delete(std::string dsn, std::string layer) {

	GDALDatasetH hDS;
	OGRLayerH  hLayer;
	int layer_cnt, layer_idx;
	bool ret;
		
	hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
			NULL, NULL, NULL);
			
	if (hDS == NULL)
		return false;

	if (!GDALDatasetTestCapability(hDS, ODsCDeleteLayer)) {
		GDALClose(hDS);
		return false;
	}

	hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
	if (hLayer == NULL) {
		GDALClose(hDS);
		return false;
	}
	
	layer_cnt = GDALDatasetGetLayerCount(hDS);
	for (layer_idx=0; layer_idx < layer_cnt; ++layer_idx) {
		hLayer = GDALDatasetGetLayer(hDS, layer_idx);
		if (EQUAL(OGR_L_GetName(hLayer), layer.c_str()))
			break;
	}
	
	if (GDALDatasetDeleteLayer(hDS, layer_idx) != OGRERR_NONE)
		ret = false;
	else
		ret = true;
		
	GDALClose(hDS);
	return ret;
}

//' Get field index or -1 if fld_name not found
//' 
//' @noRd
// [[Rcpp::export(name = ".ogr_field_index")]]
int _ogr_field_index(std::string dsn, std::string layer,
		std::string fld_name) {

	GDALDatasetH hDS;
	OGRLayerH hLayer;
	OGRFeatureDefnH hFDefn;
	int iField;
	
	CPLPushErrorHandler(CPLQuietErrorHandler);	
	hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
	if (hDS == NULL)
		return -1;
	hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
	CPLPopErrorHandler();
	
	if (hLayer == NULL) {
		GDALClose(hDS);
		return -1;
	}

	hFDefn = OGR_L_GetLayerDefn(hLayer);
	iField = OGR_FD_GetFieldIndex(hFDefn, fld_name.c_str());
	GDALClose(hDS);
	return iField;
}

//' Create a new field on layer
//' currently hard coded for OFTInteger
//'
//' @noRd
// [[Rcpp::export(name = ".ogr_field_create")]]
bool _ogr_field_create(std::string dsn, std::string layer,
		std::string fld_name) {

	GDALDatasetH hDS;
	OGRLayerH hLayer;
	OGRFeatureDefnH hFDefn;
	int iField;
	OGRFieldDefnH hFieldDefn;
	bool ret;
	
	CPLPushErrorHandler(CPLQuietErrorHandler);	
	hDS = GDALOpenEx(dsn.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
			NULL, NULL, NULL);
			
	if (hDS == NULL)
		return false;
		
	hLayer = GDALDatasetGetLayerByName(hDS, layer.c_str());
	CPLPopErrorHandler();
	
	if (hLayer == NULL) {
		GDALClose(hDS);
		return false;
	}

	hFDefn = OGR_L_GetLayerDefn(hLayer);
	iField = OGR_FD_GetFieldIndex(hFDefn, fld_name.c_str());
	if (iField >= 0) {
		// fld_name already exists
		GDALClose(hDS);
		return false;
	}
	
	hFieldDefn = OGR_Fld_Create(fld_name.c_str(), OFTInteger);
	if (OGR_L_CreateField(hLayer, hFieldDefn, TRUE) != OGRERR_NONE)
		ret = false;
	else
		ret = true;

	OGR_Fld_Destroy(hFieldDefn);
	GDALClose(hDS);
	return ret;
}

