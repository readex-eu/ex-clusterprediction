/**
 @file    predict_cluster.h
 @ingroup Cluster_Prediction/include/predict_cluster.h
 @brief   Runtime cluster prediction for inter-phase tuning
 @author  Madhura Kumaraswamy
 @verbatim
 Revision:       $Revision$
 Revision date:  $Date$
 Committed by:   $Author$

 Copyright (c) 2018, Technische Universitaet Muenchen, Germany
 See the COPYING file in the base directory of the package for details.
 @endverbatim
 */

#ifndef PREDICT_CLUSTER_H
#define PREDICT_CLUSTER_H

#include <stdlib.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>

using phases_in_cluster_t = std::set<unsigned int>;
using phase_feature_range_t = std::unordered_map<std::string, std::pair<double, double> >;
using phase_data_t = std::pair<phases_in_cluster_t, phase_feature_range_t>;

//struct cluster_inf {
//    std::unordered_map<std::string, std::pair<double,double> > phase_feature_range;
//    std::set<unsigned int>                                     phases_in_cluster;
//};

class cluster_predictor {
public:

    cluster_predictor(cluster_predictor const &) = delete;
    cluster_predictor& operator=(cluster_predictor const &) = delete;
    static cluster_predictor& instance() {
        static cluster_predictor cp;
        return cp;
    }
    int do_prediction();
    virtual ~cluster_predictor(); //= default;
private:
    std::unordered_map<int, phase_data_t> cluster_info;
    int EventSet;
    int max_phase;
    //    std::unordered_map<int, std::unique_ptr<cluster_inf> > cluster_info;

    cluster_predictor() = default;
    void initialize_papi_lib();
    void create_eventset();
    void start_metrics();
    void read_metrics();
    void accumulate_metrics();
    void compute_features();
    void clear_metrics();
    void predict();
    void add_barrier();

    std::map<unsigned long, std::vector<long long> > papi_values_per_thread;
    std::map<std::string, long long > papi_values_total;
    std::map<unsigned int, std::map<std::string, long long> > phase_result;
    std::map<unsigned int, int> predicted_clusters;
};
//int predict_cluster();

namespace PAPI {
std::vector<std::string> metrics{"perf_raw::r04C6","PAPI_L3_TCM", "PAPI_BR_CN" };
}

#endif /* PREDICT_CLUSTER_H */

