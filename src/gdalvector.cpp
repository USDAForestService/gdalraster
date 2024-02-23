/* Implementation of class GDALVector
   Encapsulates a GDALDataset and one OGRLayer
   Chris Toney <chris.toney at usda.gov> */

#include "gdal.h"
#include "cpl_error.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "ogr_srs_api.h"

#include "gdalraster.h"
#include "gdalvector.h"


GDALVector::GDALVector() : 
				dsn_in(""),
				hDataset(nullptr),
				eAccess(GA_ReadOnly),
				hLayer(nullptr),
				bVirtual(true) {}

GDALVector::GDALVector(OGRLayerH lyr_obj) :
				dsn_in(""),
				hDataset(nullptr),
				eAccess(GA_ReadOnly),
				hLayer(lyr_obj),
				bVirtual(true) {}

GDALVector::GDALVector(Rcpp::CharacterVector dsn, std::string layer) : 
				GDALVector(dsn, layer, true) {}

GDALVector::GDALVector(Rcpp::CharacterVector dsn, std::string layer,
		bool read_only) :
				hDataset(nullptr),
				eAccess(GA_ReadOnly),
				hLayer(nullptr),
				bVirtual(false) {

	dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
	if (!read_only)
		eAccess = GA_Update;

	unsigned int nOpenFlags = GDAL_OF_VECTOR | GDAL_OF_SHARED;
	if (read_only)
		nOpenFlags |= GDAL_OF_READONLY;
	else
		nOpenFlags |= GDAL_OF_UPDATE;
		
	hDataset = GDALOpenEx(dsn_in.c_str(), nOpenFlags,
			nullptr, nullptr, nullptr);
	if (hDataset == nullptr)
		Rcpp::stop("Open dataset failed.");
		
	hLayer = GDALDatasetGetLayerByName(hDataset, layer.c_str());
	if (hLayer == nullptr)
		Rcpp::stop("Failed to get layer object.");
	else
		OGR_L_ResetReading(hLayer);
		
}

GDALVector::GDALVector(Rcpp::CharacterVector dsn, std::string layer,
		bool read_only, Rcpp::CharacterVector open_options) :
				hDataset(nullptr),
				eAccess(GA_ReadOnly),
				hLayer(nullptr),
				bVirtual(false) {

	dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));
	if (!read_only)
		eAccess = GA_Update;

	std::vector<char *> dsoo(open_options.size() + 1);
	for (R_xlen_t i = 0; i < open_options.size(); ++i) {
		dsoo[i] = (char *) (open_options[i]);
	}
	dsoo.push_back(nullptr);

	unsigned int nOpenFlags = GDAL_OF_VECTOR | GDAL_OF_SHARED;
	if (read_only)
		nOpenFlags |= GDAL_OF_READONLY;
	else
		nOpenFlags |= GDAL_OF_UPDATE;

	hDataset = GDALOpenEx(dsn_in.c_str(), nOpenFlags,
			nullptr, dsoo.data(), nullptr);
	if (hDataset == nullptr)
		Rcpp::stop("Open raster failed.");
		
	hLayer = GDALDatasetGetLayerByName(hDataset, layer.c_str());
	if (hLayer == nullptr)
		Rcpp::stop("Failed to get layer object.");
	else
		OGR_L_ResetReading(hLayer);
		
}

std::string GDALVector::getDsn() const {
	return dsn_in;
}

bool GDALVector::isOpen() const {
	if (hDataset == nullptr)
		return false;
	else
		return true;
}

bool GDALVector::isVirtual() const {
	return bVirtual;
}

Rcpp::CharacterVector GDALVector::getFileList() const {
	_checkAccess(GA_ReadOnly);
	
	char **papszFiles;
	papszFiles = GDALGetFileList(hDataset);
	
	int items = CSLCount(papszFiles);
	if (items > 0) {
		Rcpp::CharacterVector files(items);
		for (int i=0; i < items; ++i) {
			files(i) = papszFiles[i];
		}
		CSLDestroy(papszFiles);
		return files;
	}
	else {
		CSLDestroy(papszFiles);
		return "";	
	}
}

std::string GDALVector::getDriverShortName() const {
	_checkAccess(GA_ReadOnly);
		
	GDALDriverH hDriver = GDALGetDatasetDriver(hDataset);
	return GDALGetDriverShortName(hDriver);
}

std::string GDALVector::getDriverLongName() const {
	_checkAccess(GA_ReadOnly);
		
	GDALDriverH hDriver = GDALGetDatasetDriver(hDataset);
	return GDALGetDriverLongName(hDriver);
}

Rcpp::List GDALVector::getLayerDefn() const {
	_checkAccess(GA_ReadOnly);
	
	OGRFeatureDefnH hFDefn = OGR_L_GetLayerDefn(hLayer);
	if (hFDefn == nullptr)
		Rcpp::stop("Error: could not obtain layer definition.");
	
	Rcpp::List list_out = Rcpp::List::create();
	std::string sValue;
	int nValue;
	bool bValue;
	int iField;

	// attribute fields
	for (iField=0; iField < OGR_FD_GetFieldCount(hFDefn); ++iField) {
	
		Rcpp::List list_fld_defn = Rcpp::List::create();
		OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, iField);
		if (hFieldDefn == nullptr)
			Rcpp::stop("Error: could not obtain field definition.");

		OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);
		// TODO: add list types, date, time, binary, etc.
		if (fld_type == OFTInteger) {
			sValue = "OFTInteger";
		}
		else if (fld_type == OFTInteger64) {
			sValue = "OFTInteger64";
		}
		else if (fld_type == OFTReal) {
			sValue = "OFTReal";
		}
		else if (fld_type == OFTString) {
			sValue = "OFTString";
		}
		else {
			sValue = "default (read as OFTString)";
		}
		list_fld_defn.push_back(sValue, "type");
		
		nValue = OGR_Fld_GetWidth(hFieldDefn);
		list_fld_defn.push_back(nValue, "width");
		
		nValue = OGR_Fld_GetPrecision(hFieldDefn);
		list_fld_defn.push_back(nValue, "precision");
		
		bValue = OGR_Fld_IsNullable(hFieldDefn);
		list_fld_defn.push_back(bValue, "is_nullable");

		bValue = OGR_Fld_IsUnique(hFieldDefn);
		list_fld_defn.push_back(bValue, "is_unique");

		if (OGR_Fld_GetDefault(hFieldDefn) != nullptr)
			sValue = std::string(OGR_Fld_GetDefault(hFieldDefn));
		else
			sValue = "";
		list_fld_defn.push_back(sValue, "default");

		bValue = OGR_Fld_IsIgnored(hFieldDefn);
		list_fld_defn.push_back(bValue, "is_ignored");

		list_out.push_back(list_fld_defn, OGR_Fld_GetNameRef(hFieldDefn));
	}

	// geometry fields
	for (int i = 0; i < OGR_FD_GetGeomFieldCount(hFDefn); ++i) {
	
		Rcpp::List list_geom_fld_defn = Rcpp::List::create();
		OGRGeomFieldDefnH hGeomFldDefn =
				OGR_FD_GetGeomFieldDefn(hFDefn, i);
		if (hGeomFldDefn == nullptr)
			Rcpp::stop("Error: could not obtain geometry field definition.");
		
		// TODO: get geometry type name ("geometry" for now)
		list_geom_fld_defn.push_back("geometry", "type");
		
		// include the geom type enum value?
		//nValue = OGR_GFld_GetType(hGeomFldDefn);
		//list_geom_fld_defn.push_back(nValue, "OGRwkbGeometryType");
		
		// TODO: make this always WKT2?
		OGRSpatialReferenceH hSRS = OGR_GFld_GetSpatialRef(hGeomFldDefn);
		if (hSRS == nullptr)
			Rcpp::stop("Error: could not obtain geometry SRS.");
		char *pszSRS_WKT = nullptr;
		if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE) {
			Rcpp::stop("Error exporting geometry SRS to WKT.");
		}
		sValue = std::string(pszSRS_WKT);
		list_geom_fld_defn.push_back(sValue, "srs");
		
		bValue = OGR_GFld_IsNullable(hGeomFldDefn);
		list_geom_fld_defn.push_back(bValue, "is_nullable");
		
		bValue = OGR_GFld_IsIgnored(hGeomFldDefn);
		list_geom_fld_defn.push_back(bValue, "is_ignored");
		
		list_out.push_back(list_geom_fld_defn,
				OGR_GFld_GetNameRef(hGeomFldDefn));
				
		CPLFree(pszSRS_WKT);
	}
	
	return list_out;
}

void GDALVector::setAttributeFilter(std::string query) {
	_checkAccess(GA_ReadOnly);

	const char* query_in = NULL;
	if (query != "")
		query_in = query.c_str();
		
	if (OGR_L_SetAttributeFilter(hLayer, query_in) != OGRERR_NONE)
		Rcpp::stop("Error setting filter, possibly in the query expression");
}

double GDALVector::getFeatureCount(bool force) {
	// OGR_L_GetFeatureCount() returns GIntBig so we return as double to R.
	// GDAL doc: Note that some implementations of this method may alter the
	// read cursor of the layer.
	_checkAccess(GA_ReadOnly);
		
	return OGR_L_GetFeatureCount(hLayer, force);
}

SEXP GDALVector::getNextFeature() {
	_checkAccess(GA_ReadOnly);

	OGRFeatureH hFeature = OGR_L_GetNextFeature(hLayer);
	
	if (hFeature != nullptr) {
		Rcpp::List list_out = Rcpp::List::create();
		OGRFeatureDefnH hFDefn = OGR_L_GetLayerDefn(hLayer);
		if (hFDefn == nullptr)
			Rcpp::stop("Error: could not obtain layer definition.");
		int iField;

		for (iField=0; iField < OGR_FD_GetFieldCount(hFDefn); ++iField) {
			OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hFDefn, iField);
			if (hFieldDefn == nullptr)
				Rcpp::stop("Error: could not obtain field definition.");
			if (!OGR_F_IsFieldSet(hFeature, iField) ||
					OGR_F_IsFieldNull(hFeature, iField)) {
				continue;
			}

			OGRFieldType fld_type = OGR_Fld_GetType(hFieldDefn);
			if (fld_type == OFTInteger) {
				Rcpp::IntegerVector value(1);
				value[0] = OGR_F_GetFieldAsInteger(hFeature, iField);
				list_out.push_back(value, OGR_Fld_GetNameRef(hFieldDefn));
			}
			else if (fld_type == OFTInteger64) {
				// R does not have native int64 so handled as double for now
				Rcpp::NumericVector value(1);
				value[0] = static_cast<double>(
						OGR_F_GetFieldAsInteger64(hFeature, iField));
				list_out.push_back(value, OGR_Fld_GetNameRef(hFieldDefn));
			}
			else if (fld_type == OFTReal) {
				Rcpp::NumericVector value(1);
				value[0] = OGR_F_GetFieldAsDouble(hFeature, iField);
				list_out.push_back(value, OGR_Fld_GetNameRef(hFieldDefn));
			}
			else {
				// TODO: support date, time, binary, etc.
				// read as string for now
				Rcpp::CharacterVector value(1);
				value[0] = OGR_F_GetFieldAsString(hFeature, iField);
				list_out.push_back(value, OGR_Fld_GetNameRef(hFieldDefn));
			}
		}

		for (int i = 0; i < OGR_F_GetGeomFieldCount(hFeature); ++i) {
			OGRGeometryH hGeometry = OGR_F_GetGeomFieldRef(hFeature, i);
			if (hGeometry == nullptr)
				Rcpp::stop("Error: could not obtain geometry reference.");
			char* pszWKT;
			OGR_G_ExportToWkt(hGeometry, &pszWKT);
			Rcpp::CharacterVector wkt(1);
			wkt[0] = pszWKT;
			OGRGeomFieldDefnH hGeomFldDefn =
					OGR_F_GetGeomFieldDefnRef(hFeature, i);
			if (hGeomFldDefn == nullptr)
				Rcpp::stop("Error: could not obtain geometry field def.");
			list_out.push_back(wkt, OGR_GFld_GetNameRef(hGeomFldDefn));
			CPLFree(pszWKT);
		}

		return list_out;
	}
	else {
		return R_NilValue;
	}
}

void GDALVector::resetReading() {
	_checkAccess(GA_ReadOnly);
	
	OGR_L_ResetReading(hLayer);
}

void GDALVector::close() {
	GDALReleaseDataset(hDataset);
	hDataset = nullptr;
}

// ****************************************************************************
// class methods for internal use not exposed in R
// ****************************************************************************

void GDALVector::_checkAccess(GDALAccess access_needed) const {
	if (!isOpen())
		Rcpp::stop("Dataset is not open.");
	
	if (access_needed == GA_Update && eAccess == GA_ReadOnly)
		Rcpp::stop("Dataset is read-only.");
}


// ****************************************************************************

RCPP_MODULE(mod_GDALVector) {

    Rcpp::class_<GDALVector>("GDALVector")

    .constructor
    	("Default constructor, only for allocation in std::vector.")
    .constructor<OGRLayerH>
    	("Usage: new(GDALVector, lyr_obj)")
    .constructor<Rcpp::CharacterVector, std::string>
    	("Usage: new(GDALVector, dsn, layer)")
    .constructor<Rcpp::CharacterVector, std::string, bool>
    	("Usage: new(GDALVector, dsn, layer, read_only=[TRUE|FALSE])")
    .constructor<Rcpp::CharacterVector, std::string, bool, Rcpp::CharacterVector>
    	("Usage: new(GDALVector, dsn, layer, read_only, open_options)")
    
    // exposed member functions
    .const_method("getDsn", &GDALVector::getDsn,
    	"Return the DSN.")
    .const_method("isOpen", &GDALVector::isOpen,
    	"Is the dataset open?")
    .const_method("isVirtual", &GDALVector::isVirtual,
    	"Is this a virtual layer?")
    .const_method("getFileList", &GDALVector::getFileList,
    	"Fetch files forming dataset.")
    .const_method("getDriverShortName", &GDALVector::getDriverShortName,
    	 "Return the short name of the format driver.")
    .const_method("getDriverLongName", &GDALVector::getDriverLongName,
    	"Return the long name of the format driver.")
    .const_method("getLayerDefn", &GDALVector::getLayerDefn,
    	"Fetch the schema information for this layer.")
    .method("setAttributeFilter", &GDALVector::setAttributeFilter,
    	"Set a new attribute query.")
    .method("getFeatureCount", &GDALVector::getFeatureCount,
    	"Fetch the feature count in this layer.")
    .method("getNextFeature", &GDALVector::getNextFeature,
    	"Fetch the next available feature from this layer.")
    .method("resetReading", &GDALVector::resetReading,
    	"Reset feature reading to start on the first feature.")
    .method("close", &GDALVector::close,
    	"Release the dataset for proper cleanup.")
    
    ;
}
