/* GDAL VSI API for operations on virtual file systems
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#ifndef SRC_GDAL_VSI_H_
#define SRC_GDAL_VSI_H_

#include <string>
#include <Rcpp.h>

int vsi_copy_file(Rcpp::CharacterVector src_file,
                  Rcpp::CharacterVector target_file,
                  bool show_progess);

void vsi_curl_clear_cache(bool partial, Rcpp::CharacterVector file_prefix,
                          bool quiet);

Rcpp::CharacterVector vsi_read_dir(Rcpp::CharacterVector path, int max_files,
                                   bool recursive, bool all_files);

bool vsi_sync(Rcpp::CharacterVector src,
              Rcpp::CharacterVector target,
              bool show_progess,
              Rcpp::Nullable<Rcpp::CharacterVector> options);

int vsi_mkdir(Rcpp::CharacterVector path, std::string mode, bool recursive);
int vsi_rmdir(Rcpp::CharacterVector path, bool recursive);
int vsi_unlink(Rcpp::CharacterVector filename);
SEXP vsi_unlink_batch(Rcpp::CharacterVector filenames);
SEXP vsi_stat(Rcpp::CharacterVector filename, std::string info);
int vsi_rename(Rcpp::CharacterVector oldpath, Rcpp::CharacterVector newpath);
std::string vsi_get_fs_options_(Rcpp::CharacterVector filename);
Rcpp::CharacterVector vsi_get_fs_prefixes();
bool vsi_supports_seq_write(Rcpp::CharacterVector filename,
                            bool allow_local_tmpfile);

bool vsi_supports_rnd_write(Rcpp::CharacterVector filename,
                            bool allow_local_tmpfile);

Rcpp::NumericVector vsi_get_disk_free_space(Rcpp::CharacterVector path);
SEXP vsi_get_file_metadata(Rcpp::CharacterVector filename, std::string domain);
SEXP vsi_get_actual_url(Rcpp::CharacterVector filename);
SEXP vsi_get_signed_url(Rcpp::CharacterVector filename,
                        Rcpp::Nullable<Rcpp::CharacterVector> options);

bool vsi_is_local(Rcpp::CharacterVector filename);


#endif  // SRC_GDAL_VSI_H_
