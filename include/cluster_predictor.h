/**
 @file    cluster_predictor.h
 @ingroup Cluster_Prediction/include/cluster_predictor.h
 @brief   Interface to the runtime cluster prediction for inter-phase tuning
 @author  Madhura Kumaraswamy
 @verbatim
 Revision:       $Revision$
 Revision date:  $Date$
 Committed by:   $Author$

 Copyright (c) 2018, Technische Universitaet Muenchen, Germany
 See the COPYING file in the base directory of the package for details.
 @endverbatim
 */

#ifndef CLUSTER_PREDICTOR_H
#define CLUSTER_PREDICTOR_H

#ifdef __cplusplus
extern "C" {
#endif
int predict_cluster();
int predict_cluster_();

#ifdef __cplusplus
}
#endif

#endif /* CLUSTER_PREDICTOR_H */

