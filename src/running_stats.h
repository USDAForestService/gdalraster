/* class RunningStats
   Get mean and variance in one pass using Welford's online algorithm
   (see https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance)
   Also tracks the min, max, sum and count.
   Chris Toney <chris.toney at usda.gov> */

#ifndef SRC_RUNNING_STATS_H_
#define SRC_RUNNING_STATS_H_

#include <cstdint>
#include <Rcpp.h>

class RunningStats {
 private:
    bool na_rm;
    uint64_t count;
    double mean, min, max, sum;
    double M2;

 public:
    RunningStats();
    explicit RunningStats(bool na_rm);

    void update(const Rcpp::NumericVector& newvalues);

    void reset();

    double get_count() const;
    double get_mean() const;
    double get_min() const;
    double get_max() const;
    double get_sum() const;
    double get_var() const;
    double get_sd() const;
};

RCPP_EXPOSED_CLASS(RunningStats)

#endif  // SRC_RUNNING_STATS_H_
