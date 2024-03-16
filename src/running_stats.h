/* class RunningStats
   Get mean and variance in one pass using Welford's online algorithm
   (see https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance)
   Also tracks the min, max, sum and count.
   Chris Toney <chris.toney at usda.gov> */

#ifndef running_stats_H
#define running_stats_H

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
    RunningStats(bool na_rm);

    void update(const Rcpp::NumericVector& newvalues);

    void reset();

    double get_count() const;
    double get_mean() const;
    double get_min() const;
    double get_max() const;
    double get_sum() const;
    double get_var() const ;
    double get_sd() const;
};

RCPP_EXPOSED_CLASS(RunningStats)

#endif
