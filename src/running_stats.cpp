/* Get mean and variance in one pass using Welford's online algorithm
(see https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance)
Also tracks the min, max, sum and count.
Chris Toney <chris.toney at usda.gov> */

#include <Rcpp.h> 
// [[Rcpp::plugins(cpp11)]]

class RunningStats {

	private:
	bool na_rm;
	unsigned long long count;
	double mean, min, max, sum;
	double M2;

	public:
	RunningStats() : na_rm(true), count(0) {}
	RunningStats(bool na_rm) : na_rm(na_rm), count(0) {}

	void update(const Rcpp::NumericVector& newvalues) {
		R_xlen_t n = newvalues.size();
		for (R_xlen_t i=0; i!=n; ++i) {
			if (na_rm) {
				if (Rcpp::NumericVector::is_na(newvalues[i]))
					continue;
			}
			++count;
			if (count == 1) {
				mean = min = max = sum = newvalues[i];
				M2 = 0;
			}
			else {
				const double delta = newvalues[i] - mean;
				mean += (delta / count);
				const double delta2 = newvalues[i] - mean;
				M2 += (delta * delta2);
				if (newvalues[i] < min) min = newvalues[i];
				if (newvalues[i] > max) max = newvalues[i];
				sum += newvalues[i];
			}
		}
	}
	
	void reset() {
		count = 0;
	}
	
	unsigned long long get_count() const {
		return count;
	}

	double get_mean() const {
		if (count > 0) return mean;
		else return NA_REAL;
	}

	// From R help for min/max:
	// "The minimum and maximum of a numeric empty set are ‘+Inf’ and
	// ‘-Inf’ (in this order!) which ensures _transitivity_, e.g.,
	// ‘min(x1, min(x2)) == min(x1, x2)’."
	double get_min() const {
		if (Rcpp::NumericVector::is_na(sum)) return NA_REAL;
		if (count > 0) return min;
		else return R_PosInf;
	}
	
	double get_max() const {
		if (Rcpp::NumericVector::is_na(sum)) return NA_REAL;
		if (count > 0) return max;
		else return R_NegInf;
	}
	
	double get_sum() const {
		if (count > 0) return sum;
		else return 0;
	}

	double get_var() const {
		if (count < 2) return NA_REAL;
		else return (M2 / (count-1));
	}

	double get_sd() const {
		if (count < 2) return NA_REAL;
		else return sqrt(M2 / (count-1));
	}
};

RCPP_EXPOSED_CLASS(RunningStats)
RCPP_MODULE(mod_running_stats) {

    Rcpp::class_<RunningStats>("RunningStats")

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

