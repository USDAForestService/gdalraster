/* GDAL VSI API for operations on virtual file systems
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef GDAL_VSI_H_
#define GDAL_VSI_H_

#include <Rcpp.h>

#include <string>

int vsi_copy_file(const Rcpp::CharacterVector &src_file,
                  const Rcpp::CharacterVector &target_file,
                  bool show_progess);

void vsi_curl_clear_cache(bool partial,
                          const Rcpp::CharacterVector &file_prefix,
                          bool quiet);

Rcpp::CharacterVector vsi_read_dir(const Rcpp::CharacterVector &path,
                                   int max_files,
                                   bool recursive, bool all_files);

bool vsi_sync(const Rcpp::CharacterVector &src,
              const Rcpp::CharacterVector &target,
              bool show_progess,
              const Rcpp::Nullable<Rcpp::CharacterVector> &options);

int vsi_mkdir(const Rcpp::CharacterVector &path, const std::string &mode,
              bool recursive);

int vsi_rmdir(const Rcpp::CharacterVector &path, bool recursive);
int vsi_unlink(const Rcpp::CharacterVector &filename);
SEXP vsi_unlink_batch(const Rcpp::CharacterVector &filenames);
SEXP vsi_stat(const Rcpp::CharacterVector &filename, const std::string &info);
Rcpp::LogicalVector vsi_stat_exists(const Rcpp::CharacterVector &filenames);
Rcpp::CharacterVector vsi_stat_type(const Rcpp::CharacterVector &filenames);
Rcpp::NumericVector vsi_stat_size(const Rcpp::CharacterVector &filenames);
int vsi_rename(const Rcpp::CharacterVector &oldpath,
               const Rcpp::CharacterVector &newpath);

std::string vsi_get_fs_options_(const Rcpp::CharacterVector &filename);

Rcpp::CharacterVector vsi_get_fs_prefixes();
bool vsi_supports_seq_write(const Rcpp::CharacterVector &filename,
                            bool allow_local_tmpfile);

bool vsi_supports_rnd_write(const Rcpp::CharacterVector &filename,
                            bool allow_local_tmpfile);

Rcpp::NumericVector vsi_get_disk_free_space(const Rcpp::CharacterVector &path);
SEXP vsi_get_file_metadata(const Rcpp::CharacterVector &filename,
                           const std::string &domain);

SEXP vsi_get_actual_url(const Rcpp::CharacterVector &filename);
SEXP vsi_get_signed_url(const Rcpp::CharacterVector &filename,
                        const Rcpp::Nullable<Rcpp::CharacterVector> &options);

bool vsi_is_local(const Rcpp::CharacterVector &filename);


#endif  // GDAL_VSI_H_
