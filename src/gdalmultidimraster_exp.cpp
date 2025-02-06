//#include "gdalmultidimraster.h"

//#include "gdal_priv.h"
// 
// //' Convert multidim raster data between different formats
// //'
// //' `mdimtranslate()` is a wrapper of the \command{gdalmdimtranslate} command-line
// //' utility (see \url{https://gdal.org/en/stable/programs/gdalmdimtranslate.html}).
// //'
// //' Called from and documented in R/gdalmdim_util.R
// //'
// //' @noRd
// // [[Rcpp::export(name = ".mdimtranslate")]]
// bool mdimtranslate(GDALMultiDimRaster src_ds, Rcpp::CharacterVector dst_filename,
//                                           Rcpp::Nullable<Rcpp::CharacterVector> cl_arg = R_NilValue,
//                                           bool quiet = false)  {
//     
//     bool ret = false;
//     
//     std::string dst_filename_in;
//     // FIXME
//     // dst_filename_in = Rcpp::as<std::string>(check_gdal_filename(dst_filename));
//     
//     
//     std::vector<char *> argv = {nullptr};
//     if (cl_arg.isNotNull()) {
//       Rcpp::CharacterVector cl_arg_in(cl_arg);
//       argv.resize(cl_arg_in.size() + 1);
//       for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
//         argv[i] = (char *) cl_arg_in[i];
//       }
//       argv[cl_arg_in.size()] = nullptr;
//     }
//     
//     
//     GDALMultiDimTranslateOptions* psOptions = GDALMultiDimTranslateOptionsNew(argv.data(),
//                                                                               nullptr);
//     
//     if (psOptions == nullptr)
//       Rcpp::stop("translate failed (could not create options struct)");
//     
//     
//     // FIXME
//     // if (!quiet)
//     //   GDALTranslateOptionsSetProgress(psOptions, GDALTermProgressR, nullptr);
//     
//     GDALDatasetH hSrcDS = src_ds.getGDALDatasetH_();
//     // I think the passing of src_ds here is wrong, but I'm stuck in the overall integration before here
//     GDALDatasetH hDstDS = GDALMultiDimTranslate(dst_filename_in.c_str(), nullptr,
//                                                 1,        hSrcDS,
//                                                 psOptions, nullptr);
//     
//     GDALMultiDimTranslateOptionsFree(psOptions);
//     
//     if (hDstDS != nullptr) {
//       GDALClose(hDstDS);
//       ret = true;
//     }
//     
//     return ret;
//   }