/**
 @file    predict_cluster.cc
 @ingroup Cluster_Prediction/src/predict_cluster.cc
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

#include "../include/predict_cluster.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "../include/cluster_predictor.h"
#ifdef __cplusplus
}
#endif
#include <tmm/tuning_model_manager.hpp>

#include <iostream>

#include <omp.h>
#include <mpi.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>

extern "C" {
#include <papi.h>
}

using namespace std;

#define TOTAL_EVENTS PAPI::metrics.size()
#define NOISE -2

static bool clustering_initialized(false);
static unsigned int phase_num(0);
static int cluster_num(1);

void cluster_predictor::initialize_papi_lib() {
    int retval = PAPI_NULL;
    // PAPI Lib initialization
    cout << "\n[Cluster Prediction Library] PAPI: Initializing the PAPI library" << endl;
    if((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT ) {
        cerr << "\n[Cluster Prediction Library] PAPI Error : PAPI Library initialization error!" << endl;
    }
    // Initializing the PAPI thread support
#ifdef _OPENMP
    if(( retval = PAPI_thread_init( ( unsigned long ( * )( void ) ) pthread_self)) != PAPI_OK) {
        cerr << "\n[Cluster Prediction Library] PAPI Error : PAPI Library thread support initialization error! Error string : " << PAPI_strerror(retval) << " Error code : " << retval << endl;
    }
#endif

    cout << "[Cluster Prediction Library] PAPI: Initialize done!\n" << endl;
}


void cluster_predictor::create_eventset() {
    int retval = PAPI_NULL;
    int eventCode;
    EventSet = PAPI_NULL;
    // Create the event set
    if((retval = PAPI_create_eventset(&EventSet)) != PAPI_OK) {
        cerr << "[Cluster Prediction Library] PAPI Error : PAPI failed to create the EventSet. Error string : " << PAPI_strerror(retval) << " Error code : " << retval << endl;
        //exit(-1);
    }

    // Add the events to the eventset
    for( auto &metric : PAPI::metrics )
    {
        if(( retval = PAPI_event_name_to_code((char*)metric.c_str(), &eventCode)) != PAPI_OK) {
            cerr << "[Cluster Prediction Library] PAPI Error : PAPI could not find the event! Failed to convert the event code to named event. Error string : " << PAPI_strerror(retval) << " Error code : " << retval << endl;
        }

        if((retval = PAPI_add_event(EventSet, eventCode)) != PAPI_OK ) {
            cerr << "[Cluster Prediction Library] PAPI Error : PAPI failed to add " << metric << " to the EventSet. Error string : " << PAPI_strerror(retval) << " Error code : " << retval << endl;
        }
        else  {
            cout << "[Cluster Prediction Library] PAPI: Added " << metric << " to the EventSet" << endl;
        }
    }
}


void cluster_predictor::start_metrics() {
    // Checking if EventSet is already counting
    cout << "[Cluster Prediction Library] PAPI: Checking if the EventSet is already counting before starting the counters" << endl;
    int papi_state;
    //Ignore return value
    ( void )PAPI_state(EventSet, &papi_state);
    if(papi_state == PAPI_RUNNING ) {
        cout << "[Cluster Prediction Library] PAPI: PAPI_EventSet is already RUNNING!" << endl;
    }
    else {
        int retval = PAPI_NULL;
        // Start counting the events of the EventSet
        if((retval = PAPI_start(EventSet)) != PAPI_OK ) {
            cerr << "[Cluster Prediction Library] PAPI Error : PAPI failed to start the events in created Eventset. Error string : " << PAPI_strerror(retval) << " Error code : " << retval << endl;
        }
        else {
            cout << "[Cluster Prediction Library] PAPI: Started counting events" << endl;
        }
    }
}


void cluster_predictor::read_metrics() {
    int retval = PAPI_NULL;
    int papi_state;

    //Ignore return value
    ( void )PAPI_state(EventSet, &papi_state);
    if( papi_state == PAPI_RUNNING ) {
        cout << "[Cluster Prediction Library] PAPI: PAPI_EventSet is already RUNNING!...Reading metrics now" << endl;
    }

    vector<long long> papi_values;
    for( int i = 0; i < TOTAL_EVENTS ; i++) {
        papi_values.push_back(0);
    }

    papi_values_per_thread.clear();
    papi_values_per_thread.emplace(make_pair((unsigned long)pthread_self(),papi_values));

    if((retval = PAPI_read(EventSet, papi_values_per_thread.find((unsigned long)pthread_self())->second.data())) != PAPI_OK) {
        cerr << " PAPI Error : PAPI failed to read counters per thread. Error string : " << PAPI_strerror(retval) << " Error code : " << retval << endl;
        //exit(-1);
    }

    //    for(auto &entry : papi_values_per_thread) {
    //        cout << "\n \n Current Values in papi_values_per_thread after insertion are : " << entry.first << " : " << entry.second[0] << "  " << entry.second[1] << "  "
    //                << entry.second[2] <<  endl;
    //    }
}


void cluster_predictor::accumulate_metrics() {

    //    for(auto &entry : papi_values_per_thread) {
    //        cout << "[Cluster Prediction Library] Current Values in papi_values_per_thread before accumulation are : " << entry.first << " : " << entry.second[0] << "  " << entry.second[1] << "  "
    //                << entry.second[2] << endl;
    //    }

    // First accumulate the values of the threads. Then, accumulate the values across all the processes with a PMPI_AllGather()
    cout << "[Cluster Prediction Library] Accumulating the PAPI metrics" << endl;
    vector<long long> papi_thread_total(TOTAL_EVENTS);
    for(int i = 0 ; i < TOTAL_EVENTS; ++i ) {
        for(auto &entry : papi_values_per_thread) {
            papi_thread_total[i] += entry.second[i];
        }
    }

    int initialized(0);
    PMPI_Initialized( &initialized );
    if ( initialized ) {
        // Now aggregate all the values from the processes s
        PMPI_Allreduce(MPI_IN_PLACE, (void *)papi_thread_total.data(), TOTAL_EVENTS , MPI::LONG_LONG, MPI_SUM, MPI_COMM_WORLD );
    }

    for(int i = 0 ; i < TOTAL_EVENTS; ++i ) {
        papi_values_total[PAPI::metrics[i]] = papi_thread_total[i];
    }

    //    cout << "[Cluster Prediction Library] Printing accumulated PAPI values:  " << endl;
    //    for(auto &i : papi_values_total) {
    //        cout << i.first << " = " << i.second << endl;
    //    }
}


void cluster_predictor::clear_metrics() {
    int retval = PAPI_NULL;
    int papi_state;

    //Ignore return value
    ( void )PAPI_state(EventSet, &papi_state);
    if( papi_state == PAPI_RUNNING ) {
        cout << "[Cluster Prediction Library] PAPI: PAPI_EventSet is already RUNNING!...Clearing metrics now" << endl;
    }

    vector<long long> papi_values;
    for( int i = 0; i < TOTAL_EVENTS ; i++) {
        papi_values.push_back(0);
    }

    if (PAPI_reset(EventSet) != PAPI_OK)
        cerr << "[Cluster Prediction Library] PAPI Error : PAPI failed to clear counters per thread. Error string : " << PAPI_strerror(retval) << " Error code : " << retval << endl;
}


void cluster_predictor::predict() {
    std::unordered_map<int, phase_data_t>::iterator pos;
    if(phase_num <= max_phase) {
        //Find the cluster number of the current phase
        pos = find_if(cluster_info.begin(), cluster_info.end(), [&](auto& cl_inf) {
            auto ph = (*cl_inf.second.first.find(::phase_num));
            return ph == ::phase_num; });
    }
    else if(phase_num == max_phase + 1) {
        //If the current phase is the phase immediately following the max_phase, return the cluster number of the max_phase
        pos = find_if(cluster_info.begin(), cluster_info.end(), [&](auto& cl_inf) {
            auto ph = (*cl_inf.second.first.find(max_phase));
            return ph == max_phase; });
    }
    else {
        //If the current phase is atleast max_phase+2, correct the cluster number of the previous phase and return the corrected cluster number
        cout << "[Cluster Prediction Library] Prediction: Computing phase features" << endl;
        double comp_intensity =  (double)(papi_values_total.find("perf_raw::r04C6")->second)/(double)(papi_values_total.find("PAPI_L3_TCM")->second);
        double branch_instr = (double)(papi_values_total.find("PAPI_BR_CN")->second)/(double)(papi_values_total.find("perf_raw::r04C6")->second);

        //        cout << "[Cluster Prediction Library] Compute Intensity = " << comp_intensity << endl;
        //        cout << "[Cluster Prediction Library] Branch Instructions = " << branch_instr << endl;

        //If the PAPI values lie in the range of the phase features, return the cluster number else return NOISE
        pos = find_if(cluster_info.begin(), cluster_info.end(),[&comp_intensity, &branch_instr](auto& cl_inf) {
            double comp_min = cl_inf.second.second.find("ComputeIntensity")->second.first;
            double comp_max = cl_inf.second.second.find("ComputeIntensity")->second.second;
            double br_min = cl_inf.second.second.find("BranchInstructions")->second.first;
            double br_max = cl_inf.second.second.find("BranchInstructions")->second.second;

            //Check if (unsigned)(number-lower) <= (upper-lower)). Checking if the phase features are in the ranges
            return (comp_intensity >= comp_min && comp_intensity <= comp_max && branch_instr >= br_min && branch_instr <= br_max);
        });
    }

    if( pos != cluster_info.end() ) {
        cluster_num = pos->first;
    }
    else cluster_num = NOISE;

    if(phase_num > max_phase) {
        //Correct the predicted cluster of the previous phase
        auto pos =  predicted_clusters.find(phase_num-1);
        if(pos != predicted_clusters.end()) {
            pos->second = cluster_num;
        }
        //Add the newly predicted cluster number for the current phase
        predicted_clusters.emplace(make_pair(phase_num,cluster_num));
    }
    cout << "[Cluster Prediction Library] Cluster number : " << cluster_num << endl;
}


void cluster_predictor::add_barrier() {
    //#if USE_MPI
    int initialized(0);
    PMPI_Initialized( &initialized );
    if ( initialized )
        PMPI_Barrier( MPI_COMM_WORLD );
    //#endif
}

int cluster_predictor::do_prediction() {
    ++phase_num;
    if(clustering_initialized) {
        //Read EventSet
        cluster_predictor::instance().read_metrics();
        cluster_predictor::instance().accumulate_metrics();

        // Correct the previous prediction and predict the current cluster
        cluster_predictor::instance().predict();
        cluster_predictor::instance().clear_metrics();
        cluster_predictor::instance().add_barrier();
        return cluster_num;
    }
    else {
        clustering_initialized = true;
        cout << "[Cluster Prediction Library] Clustering initialized? " << clustering_initialized << endl;
        //Get the TM values from RRL
        cluster_predictor::instance().cluster_info = rrl::tmm::get_tuning_model_manager(getenv("SCOREP_RRL_TMM_PATH"))->get_phase_data();

        cout << "[Cluster Prediction Library] Cluster information read from the TM:" << endl;
        for(auto &i : cluster_predictor::instance().cluster_info) {
            cout << "[Cluster Prediction Library] Cluster = " << i.first << endl;
            for(auto &ph_f : i.second.second) {
                cout << ph_f.first << " min = " << ph_f.second.first << " max = "<< ph_f.second.second << endl;
            }
            cout << "[Cluster Prediction Library] Phases in cluster: " << endl;
            for(auto &f : i.second.first) {
                cout << f << "\n";
            }
        }

        for(auto &i : cluster_predictor::instance().cluster_info) {
            for(auto &ph_i : i.second.first) {
                cluster_predictor::instance().max_phase = (ph_i > cluster_predictor::instance().max_phase) ? ph_i : cluster_predictor::instance().max_phase;
            }
        }

        //Find the cluster number of the current phase
        cluster_predictor::instance().predict();
        cluster_predictor::instance().initialize_papi_lib();
        cluster_predictor::instance().create_eventset();
        cluster_predictor::instance().add_barrier();
        cluster_predictor::instance().start_metrics();

        return cluster_num;
    }
}


int predict_cluster() {
    return cluster_predictor::instance().do_prediction();
}

//For Fortran compatibility
int predict_cluster_() {
    return predict_cluster();
}


cluster_predictor::~cluster_predictor() {
    //    retval = PAPI_unregister_thread(  );
    //    if ( retval != PAPI_OK )
    //        test_fail( __FILE__, __LINE__, "PAPI_unregister_thread", retval );
}
