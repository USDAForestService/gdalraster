#ifndef SRC_GDALMULTIDIMRASTER_H
#define SRC_GDALMULTIDIMRASTER_H

#include "gdal_priv.h"

#include "rcpp_util.h"

class GDALMultiDimRaster {
public:

  GDALMultiDimRaster();
  explicit GDALMultiDimRaster(Rcpp::CharacterVector filename);
  GDALMultiDimRaster(Rcpp::CharacterVector filename, bool read_only);
  GDALMultiDimRaster(Rcpp::CharacterVector filename, bool read_only,
             Rcpp::CharacterVector open_options);
  GDALMultiDimRaster(Rcpp::CharacterVector filename, bool read_only,
             Rcpp::Nullable<Rcpp::CharacterVector> open_options,
             bool shared);
  
  // read/write fields exposed to R
  Rcpp::CharacterVector infoOptions = Rcpp::CharacterVector::create();
  
  // Open/close dataset
  void open(bool read_only);
  void close();
  
  std::string getDescription() const; 
  void setDescription(std::string desc);
  // Basic dataset information
  bool isOpen() const;  /// const{ return m_hDataset; != nullptr; }
  std::string getFilename() const;  //const { return filename; }
  void setFilename(std::string filename); 
  
  std::string infoAsJSON() const;
  
  std::string getDriverShortName() const;
  std::string getDriverLongName() const;
  Rcpp::CharacterVector getFileList() const;

  // Multidimensional API specific methods
  std::vector<std::string> getArrayNames() const;
  std::string getRootGroupName() const;

  std::vector<std::string> getDimensionNames(std::string variable) const;
  std::vector<size_t> getDimensionSizes(std::string variable) const;
  
  // methods for internal use not exported to R
  void checkAccess_(GDALAccess access_needed) const;
  GDALGroup* getRootGroup() const;
  GDALMDArray* getArray(const std::string& arrayName) const;
  GDALGroupH hRootGroup;

  std::vector<double> getCoordinateValues(std::string variable) const; 
  
private:
 // GDALDataset* dataset;
  GDALDatasetH m_hDataset;
  std::string m_fname;
  Rcpp::CharacterVector m_open_options;
  bool m_shared;
  

  GDALAccess m_eAccess;
};

RCPP_EXPOSED_CLASS(GDALMultiDimRaster)
  
#endif // SRC_GDALMULTIDIMRASTER_H
