/* Implementation of class RunningStats
   One-pass algorithm for mean and variance.
   Chris Toney <chris.toney at usda.gov> */

#include "running_stats.h"

RunningStats::RunningStats():
	na_rm(true), count(0) {}

RunningStats::RunningStats(bool na_rm):
	na_rm(na_rm), count(0) {}

void RunningStats::update(const Rcpp::NumericVector& newvalues) {
	for (auto const& i : newvalues) {
		if (na_rm) {
			if (Rcpp::NumericVector::is_na(i))
				continue;
		}
		count += 1;
		if (count == 1) {
			mean = min = max = sum = i;
			M2 = 0.0;
		}
		else {
			const double delta = i - mean;
			mean += (delta / count);
			const double delta2 = i - mean;
			M2 += (delta * delta2);
			if (i < min)
				min = i;
			else if (i > max)
				max = i;
			sum += i;
		}
	}
}

void RunningStats::reset() {
	count = 0;
}

double RunningStats::get_count() const {
	// return as double in R (no native int64)
	return static_cast<double>(count);
}

double RunningStats::get_mean() const {
	if (count > 0)
		return mean;
	else
		return NA_REAL;
}

// From R help for min/max:
// "The minimum and maximum of a numeric empty set are ‘+Inf’ and
// ‘-Inf’ (in this order!) which ensures _transitivity_, e.g.,
// ‘min(x1, min(x2)) == min(x1, x2)’."
double RunningStats::get_min() const {
	if (Rcpp::NumericVector::is_na(sum))
		return NA_REAL;
		
	if (count > 0)
		return min;
	else
		return R_PosInf;
}

double RunningStats::get_max() const {
	if (Rcpp::NumericVector::is_na(sum))
		return NA_REAL;
		
	if (count > 0)
		return max;
	else
		return R_NegInf;
}

double RunningStats::get_sum() const {
	if (count > 0)
		return sum;
	else
		return 0;
}

double RunningStats::get_var() const {
	if (count < 2)
		return NA_REAL;
	else
		return (M2 / (count-1));
}

double RunningStats::get_sd() const {
	if (count < 2)
		return NA_REAL;
	else
		return sqrt(M2 / (count-1));
}

RCPP_MODULE(mod_running_stats) {

    Rcpp::class_<RunningStats>("RunningStats")

    .constructor
    	("Default constructor initialized with na_rm = TRUE.")
    .constructor<bool>
    	("Initialize with na_rm = TRUE or FALSE")

	.method("update", &RunningStats::update,
		"Add new values from a numeric vector")
    .method("reset", &RunningStats::reset,
    	"Reset the data stream to count = 0")
    	
	.const_method("get_count", &RunningStats::get_count, 
		"Return the count of values currently in the stream")
	.const_method("get_mean", &RunningStats::get_mean, 
		"Return the mean of the values currently in the stream")
	.const_method("get_min", &RunningStats::get_min, 
		"Return the minimum value currently in the stream")
	.const_method("get_max", &RunningStats::get_max, 
		"Return the maximum value currently in the stream")
	.const_method("get_sum", &RunningStats::get_sum, 
		"Return the sum of values currently in the stream")
	.const_method("get_var", &RunningStats::get_var, 
		"Return the variance of the values currently in the stream")
	.const_method("get_sd", &RunningStats::get_sd, 
		"Return standard deviation of the values currently in the stream")
    ;
}

