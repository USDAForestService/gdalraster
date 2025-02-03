#include "gdalmultidimraster.h"

#include "gdal_utils.h"

// GDALMultiDimRaster::GDALMultiDimRaster() : dataset(nullptr) {
//   // Register all GDAL drivers
//   GDALAllRegister();
// }

GDALMultiDimRaster::GDALMultiDimRaster() : 
  m_fname(""),
  m_open_options(Rcpp::CharacterVector::create()),
  m_shared(false),
  m_hDataset(nullptr),
  m_eAccess(GA_ReadOnly) {}

GDALMultiDimRaster::GDALMultiDimRaster(Rcpp::CharacterVector filename) :
  GDALMultiDimRaster(
    filename,
    true,
    R_NilValue,
    true) {}

GDALMultiDimRaster::GDALMultiDimRaster(Rcpp::CharacterVector filename, bool read_only) :
  GDALMultiDimRaster(
    filename,
    read_only,
    R_NilValue,
    true) {}


GDALMultiDimRaster::GDALMultiDimRaster(Rcpp::CharacterVector filename, bool read_only,
                       Rcpp::CharacterVector open_options) :
  GDALMultiDimRaster(
    filename,
    read_only,
    open_options,
    true) {}

GDALMultiDimRaster::GDALMultiDimRaster(Rcpp::CharacterVector filename, bool read_only,
                       Rcpp::Nullable<Rcpp::CharacterVector> open_options, bool shared) :
  m_shared(shared),
  m_hDataset(nullptr),
  m_eAccess(GA_ReadOnly) {
  // turn this back on we don't have check_gdal_filename
  //m_fname = Rcpp::as<std::string>(check_gdal_filename(filename));
  m_fname = Rcpp::as<std::string>(filename); 
  if (open_options.isNotNull())
    m_open_options = open_options;
  else
    m_open_options = Rcpp::CharacterVector::create();
  
  open(read_only);
  
  // warn for now if 64-bit integer
 // if (hasInt64_())
  //  warnInt64_();
}

std::string GDALMultiDimRaster::getDescription() const {
  checkAccess_(GA_ReadOnly);
  std::string desc;
  
  desc = GDALGetDescription(m_hDataset);

  
  return desc;
}

void GDALMultiDimRaster::setDescription(std::string desc) {
  checkAccess_(GA_ReadOnly);
  GDALSetDescription(m_hDataset, desc.c_str());
  
}

std::string GDALMultiDimRaster::getFilename() const {
  return m_fname;
}
void GDALMultiDimRaster::setFilename(std::string filename) {
   if (m_hDataset != nullptr) {
  if (m_fname == "" && getDescription() == "") {
        //filename = Rcpp::as<std::string>(check_gdal_filename(filename));
       setDescription(filename);
    }

    else {
      Rcpp::stop("the filename cannot be set on this object");
    }
  }
  else {
    if (m_fname == "")
//      m_fname = Rcpp::as<std::string>(check_gdal_filename(filename));
    m_fname = filename; 
    else
      Rcpp::stop("the filename cannot be set on this object");
  }
 }



void GDALMultiDimRaster::open(bool read_only) {
  if (m_fname == "")
    Rcpp::stop("'filename' is not set");
  
  if (m_hDataset != nullptr)
   close();
  
  std::vector<char *> dsoo(m_open_options.size() + 1);
  if (m_open_options.size() > 0) {
    for (R_xlen_t i = 0; i < m_open_options.size(); ++i) {
      dsoo[i] = (char *) (m_open_options[i]);
    }
  }
  dsoo[m_open_options.size()] = nullptr;
  
  unsigned int nOpenFlags = GDAL_OF_MULTIDIM_RASTER;
  if (read_only) {
    m_eAccess = GA_ReadOnly;
    nOpenFlags |= GDAL_OF_READONLY;
  }
  else {
    m_eAccess = GA_Update;
    nOpenFlags |= GDAL_OF_UPDATE;
  }
  if (m_shared)
    nOpenFlags |= GDAL_OF_SHARED;
 
  m_hDataset = GDALOpenEx(m_fname.c_str(), nOpenFlags, nullptr,
                          dsoo.data(), nullptr);
  
  if (m_hDataset == nullptr)
    Rcpp::stop("open multidim raster failed");
  
  hRootGroup = GDALDatasetGetRootGroup(m_hDataset);
  GDALReleaseDataset(m_hDataset); 
}

std::vector<std::string> GDALMultiDimRaster::getDimensionNames(std::string variable) const {
  GDALMDArrayH hVar = GDALGroupOpenMDArray(hRootGroup, variable.c_str(), NULL);

  GDALDimensionH* dims; 
  size_t nDimCount;
  size_t i;
  dims = GDALMDArrayGetDimensions(hVar, &nDimCount);
  std::vector<std::string> dimnames; 
  for( i = 0; i < nDimCount; i++ )
  {
    dimnames.push_back(std::string(GDALDimensionGetName(dims[i]))); 
  }
  GDALReleaseDimensions(dims, nDimCount);
  return dimnames; 
}
std::vector<size_t> GDALMultiDimRaster::getDimensionSizes(std::string variable) const {
  GDALMDArrayH hVar = GDALGroupOpenMDArray(hRootGroup, variable.c_str(), NULL);
  
  GDALDimensionH* dims; 
  size_t nDimCount;
  size_t i;
  dims = GDALMDArrayGetDimensions(hVar, &nDimCount);
  std::vector<size_t> dimsizes; 
  for( i = 0; i < nDimCount; i++ )
  {
    dimsizes.push_back(GDALDimensionGetSize(dims[i])); 
  }
  GDALReleaseDimensions(dims, nDimCount);
  return dimsizes; 
}
// copied from the GDALRaster implementation for now with no flushCache or vsi_curl_clear_cache
void GDALMultiDimRaster::close() {
  // make sure caches are flushed when access was GA_Update:
  // since the dataset was opened shared, and could still have a shared
  // read-only handle (not recommended), or may be re-opened for read and
  // is on a /vsicurl/ filesystem,
 // if (m_eAccess == GA_Update) {
  //  flushCache();
  //  vsi_curl_clear_cache(true, m_fname, true);
  //}
  
#if GDAL_VERSION_NUM >= 3070000
  if (GDALClose(m_hDataset) != CE_None)
    Rcpp::warning("error occurred during GDALClose()!");
#else
  GDALClose(m_hDataset);
#endif
  
  m_hDataset = nullptr;
}

bool GDALMultiDimRaster::isOpen() const {
  if (m_hDataset == nullptr)
    return false;
  else
    return true;
}


std::string GDALMultiDimRaster::infoAsJSON() const {
  checkAccess_(GA_ReadOnly);
  
  const Rcpp::CharacterVector argv = infoOptions;
  std::vector<char *> opt = {nullptr};
  // passing in options is not working ...
  for (R_xlen_t i = 0; i < argv.size(); ++i) {
      opt.push_back((char *) argv[i]);
      //Rprintf("%lu\n", i); 
    }
  opt.push_back(nullptr);
 
  
  GDALMultiDimInfoOptions* psOptions = GDALMultiDimInfoOptionsNew(opt.data(), nullptr);
  if (psOptions == nullptr)
    Rcpp::stop("creation of GDALInfoOptions failed (check $infoOptions)");
  
  char *pszGDALInfoOutput = GDALMultiDimInfo(m_hDataset, psOptions);
  std::string out = "";
  if (pszGDALInfoOutput != nullptr)
    out = pszGDALInfoOutput;
  
  GDALMultiDimInfoOptionsFree(psOptions);
  CPLFree(pszGDALInfoOutput);
  
  out.erase(std::remove(out.begin(),
                        out.end(),
                        '\n'),
                        out.cend());
  
  return out;
}

Rcpp::CharacterVector GDALMultiDimRaster::getFileList() const {
  checkAccess_(GA_ReadOnly);
  
  char **papszFiles;
  papszFiles = GDALGetFileList(m_hDataset);
  
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

std::string GDALMultiDimRaster::getDriverShortName() const {
  checkAccess_(GA_ReadOnly);
  
  GDALDriverH hDriver = GDALGetDatasetDriver(m_hDataset);
  return GDALGetDriverShortName(hDriver);
}

std::string GDALMultiDimRaster::getDriverLongName() const {
  checkAccess_(GA_ReadOnly);
  
  GDALDriverH hDriver = GDALGetDatasetDriver(m_hDataset);
  return GDALGetDriverLongName(hDriver);
}


std::string GDALMultiDimRaster::getRootGroupName() const {
  checkAccess_(GA_ReadOnly);
  
  return GDALGroupGetName(hRootGroup);  
}


std::vector<std::string> GDALMultiDimRaster::getArrayNames() const {
  std::vector<std::string> arrayNames;
  if (!m_hDataset) {
    return arrayNames;
  }

 // auto rootGroup = getRootGroup();
  if (!hRootGroup) {
    return arrayNames;
  }
  char **papszArrays = GDALGroupGetMDArrayNames(hRootGroup, nullptr);
  int n_items = CSLCount(papszArrays);
  for (int i=0; i < n_items; ++i) {
    arrayNames.push_back(std::string(papszArrays[i]));
  }
  CSLDestroy(papszArrays);
  return arrayNames;
}



// ****************************************************************************
// class methods for internal use not exposed in R
// ****************************************************************************

void GDALMultiDimRaster::checkAccess_(GDALAccess access_needed) const {
  if (!isOpen())
    Rcpp::stop("dataset is not open");
  
  if (access_needed == GA_Update && m_eAccess == GA_ReadOnly)
    Rcpp::stop("dataset is read-only");
}

// GDALGroup* GDALMultiDimRaster::getRootGroup() const {
//   if (!m_hDataset) {
//     throw std::runtime_error("Dataset not open");
//   }
//   return m_hDataset->GetRootGroup();
// }



// GDALMDArray* GDALMultiDimRaster::getArray(const std::string& arrayName) const {
//   if (!dataset) {
//     throw std::runtime_error("Dataset not open");
//   }
//   
//   auto rootGroup = getRootGroup();
//   if (!rootGroup) {
//     throw std::runtime_error("Could not get root group");
//   }
//   
//   return rootGroup->OpenMDArray(arrayName.c_str());
// }




// ****************************************************************************

RCPP_MODULE(mod_GDALMultiDimRaster) {
  Rcpp::class_<GDALMultiDimRaster>("GDALMultiDimRaster")
  
  .constructor
  ("Default constructor, no dataset opened")
  .constructor<Rcpp::CharacterVector>
  ("Usage: new(GDALRaster, filename)")
  .constructor<Rcpp::CharacterVector, bool>
  ("Usage: new(GDALRaster, filename, read_only=[TRUE|FALSE])")
  .constructor<Rcpp::CharacterVector, bool, Rcpp::CharacterVector>
  ("Usage: new(GDALRaster, filename, read_only, open_options)")
  .constructor<Rcpp::CharacterVector, bool, Rcpp::Nullable<Rcpp::CharacterVector>, bool>
  ("Usage: new(GDALRaster, filename, read_only, open_options, shared)")
  
  // exposed read/write fields
  .field("infoOptions", &GDALMultiDimRaster::infoOptions)
  
  // exposed member functions
  .method("open", &GDALMultiDimRaster::open,
  "(Re-)open the multdim raster dataset on the existing filename")
  .method("close", &GDALMultiDimRaster::close,
  "Close the GDAL multidim dataset for proper cleanup")
  .method("setFilename", &GDALMultiDimRaster::setFilename,
  "Set the multidim raster filename")
  .const_method("getFilename", &GDALMultiDimRaster::getFilename,
  "Return the multidim raster filename")
  .const_method("infoAsJSON", &GDALMultiDimRaster::infoAsJSON,
  "Returns full output of gdalmdiminfo as a JSON-formatted string")
  
  .const_method("getFileList", &GDALMultiDimRaster::getFileList,
  "Fetch files forming dataset")
  .const_method("getArrayNames", &GDALMultiDimRaster::getArrayNames,
  "Fetch names of arrays in the root group")
  
  .const_method("getDimensionNames", &GDALMultiDimRaster::getDimensionNames,
  "Fetch names of dimensions of the given variable 'getDimensionNames(<varname>)'")
  .const_method("getDimensionSizes", &GDALMultiDimRaster::getDimensionSizes,
  "Fetch sizes of dimensions of the given variable 'getDimensionSizes(<varname>)'")
  
  .const_method("getDriverShortName", &GDALMultiDimRaster::getDriverShortName,
  "Return the short name of the format driver")
  .const_method("getDriverLongName", &GDALMultiDimRaster::getDriverLongName,
  "Return the long name of the format driver")
  
  .const_method("getRootGroupName", &GDALMultiDimRaster::getRootGroupName,
  "Return the root group name")
  
  .const_method("getDescription", &GDALMultiDimRaster::getDescription,
  "Return object description for a multdim raster")
  .method("setDescription", &GDALMultiDimRaster::setDescription,
  "Set object description for a multidim raster")
  
  .const_method("isOpen", &GDALMultiDimRaster::isOpen,
  "Is the multidim raster dataset open")
  
  ;
} 
  