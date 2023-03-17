// Copyright [2021] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com

#include <lp_rta_pfp_mpcp_heterogeneous.h>
#include <sched_test_factory.h>


SchedTestBase *SchedTestFactory::createSchedTest(string test_name,
                                                 TaskSet *tasks,
                                                 DAG_TaskSet *dag_tasks,
                                                 ProcessorSet *processors,
                                                 ResourceSet *resources) {
 /* if (0 == strcmp(test_name.data(), "RTA-GFP-native")) {
    tasks->RM_Order();
    return new RTA_GFP_native((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GFP-BC")) {
    tasks->RM_Order();
    return new RTA_GFP_BC((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GFP-NG")) {
    tasks->RM_Order();
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-WF")) {
    tasks->RM_Order();
    return new RTA_PFP_WF((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-FF")) {
    tasks->RM_Order();
    return new RTA_PFP_FF((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-FF-2")) {
    tasks->DC_Order();
    return new RTA_PFP_FF((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-FF-OPA")) {
    tasks->RM_Order();
    return new RTA_PFP_FF_OPA((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-FF-SM-OPA")) {
    tasks->DC_Order();
    return new RTA_PFP_FF_OPA((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-WF-spinlock")) {
    tasks->RM_Order();
    return new RTA_PFP_WF_spinlock((*tasks), (*processors), (*resources));
  } else if (0 ==
             strcmp(test_name.data(), "RTA-PFP-WF-spinlock-heterogeneous")) {
    tasks->RM_Order();
    return new RTA_PFP_WF_spinlock_heterogeneous((*tasks), (*processors),
                                                 (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PDC-WF-spinlock")) {
    tasks->DC_Order();
    return new RTA_PFP_WF_spinlock((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-WF-semaphore")) {
    tasks->RM_Order();
    return new RTA_PFP_WF_semaphore((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PDC-WF-semaphore")) {
    tasks->DC_Order();
    return new RTA_PFP_WF_semaphore((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-PIP")) {
    return new LP_RTA_GFP_PIP((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-FMLP")) {
    return new LP_RTA_GFP_FMLP((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-PFP-DPCP")) {
    return new LP_RTA_PFP_DPCP((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-PFP-MPCP")) {
    return new LP_RTA_PFP_MPCP((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-PFP-MPCP-HETEROGENEOUS")) {
    return new LP_RTA_PFP_MPCP_HETEROGENEOUS((*tasks), (*processors),
                                             (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-PFP-ROP-DPCP")) {
    return new LP_RTA_PFP_ROP_DPCP((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-PFP-ROP-DPCP-PLUS")) {
    return new LP_RTA_PFP_ROP_DPCP_PLUS((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-ROP")) {
    tasks->RM_Order();
    return new RTA_PFP_ROP((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-ROP-HETEROGENEOUS")) {
    tasks->RM_Order();
    return new RTA_PFP_ROP_HETEROGENEOUS((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-ROP-FAST-FIRST")) {
    tasks->RM_Order();
    return new RTA_PFP_ROP_FAST_FIRST((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-ROP-FAST-FIRST-V2")) {
    tasks->RM_Order();
    return new RTA_PFP_ROP_FAST_FIRST_V2((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-ROP-SLOW-FIRST")) {
    tasks->RM_Order();
    return new RTA_PFP_ROP_SLOW_FIRST((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-DC")) {
    tasks->DC_Order();
    return new RTA_PFP_ROP((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-SM")) {
    tasks->RM_Order();
    return new RTA_PFP_RO_SM((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-OPA")) {
    tasks->RM_Order();
    return new RTA_PFP_RO_OPA((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-SM-OPA")) {
    tasks->RM_Order();
    return new RTA_PFP_RO_SM_OPA((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-NP")) {
    tasks->RM_Order();
    return new RTA_PFP_RO_NP((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-NP-OPA")) {
    tasks->RM_Order();
    return new RTA_PFP_RO_NP_OPA((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-NP-OPA-P2")) {
    tasks->RM_Order();
    return new RTA_PFP_RO_NP_OPA_P2((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-NP-OPA-P3")) {
    tasks->RM_Order();
    return new RTA_PFP_RO_NP_OPA_P3((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-NP-SM")) {
    tasks->RM_Order();
    return new RTA_PFP_RO_NP_SM((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-NP-SM-OPA")) {
    tasks->RM_Order();
    return new RTA_PFP_RO_NP_SM_OPA((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-RO-FEASIBLE")) {
    return new RTA_PFP_RO_FEASIBLE((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-PFP-spinlock")) {
    return new ILP_RTA_PFP_spinlock((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-PFP-ROP")) {
    return new ILP_RTA_PFP_ROP((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PFP-GS")) {
    return new RTA_PFP_GS((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "NC-LP-EE-VPR")) {
    return new NC_LP_EE_VPR((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GRM")) {
    tasks->RM_Order();
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GDM-NG")) {
    tasks->DM_Order();
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GDC-NG")) {
    tasks->DC_Order();
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GSMP-NG")) {
    tasks->SM_PLUS_Order();
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GSMP2-NG")) {
    tasks->SM_PLUS_2_Order();
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GSMP3-NG")) {
    tasks->SM_PLUS_3_Order();
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GSMP4-NG")) {
    tasks->SM_PLUS_4_Order(processors->get_processor_num());
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GLO-NG")) {
    tasks->Leisure_Order();
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GDCC")) {
    tasks->DCC_Order();
    return new RTA_GFP_NG((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PDC-WF")) {
    tasks->DC_Order();
    return new RTA_PFP_WF((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GDC-native")) {
    tasks->DC_Order();
    return new RTA_GDC_native((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GDCC-native")) {
    tasks->DCC_Order();
    return new RTA_GDC_native((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GDDC-native")) {
    tasks->DDC_Order();
    return new RTA_GDC_native((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GUDC-native")) {
    tasks->UDC_Order();
    return new RTA_GDC_native((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GSMP-native")) {
    tasks->SM_PLUS_Order();
    return new RTA_GDC_native((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GDC-BC")) {
    tasks->DC_Order();
    return new RTA_GFP_BC((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GDDC-BC")) {
    tasks->DDC_Order();
    return new RTA_GFP_BC((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GSMP-BC")) {
    tasks->SM_PLUS_Order();
    return new RTA_GFP_BC((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GSMP2-BC")) {
    tasks->SM_PLUS_2_Order();
    return new RTA_GFP_BC((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GSMP3-BC")) {
    tasks->SM_PLUS_3_Order();
    return new RTA_GFP_BC((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-GSMP4-BC")) {
    tasks->SM_PLUS_4_Order(processors->get_processor_num());
    return new RTA_GFP_BC((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-PDC-RO")) {
    return new RTA_PDC_RO((*tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GFP-FMLP")) {
    return new RTA_DAG_GFP_FMLP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GFP-PIP")) {
    return new RTA_DAG_GFP_PIP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-CGFP-FMLP")) {
    return new RTA_DAG_CGFP_FMLP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-CGFP-FMLP-V2")) {
    return new RTA_DAG_CGFP_FMLP_v2((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-CGFP-PIP")) {
    return new RTA_DAG_CGFP_PIP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "CAB-DAG-FED-SPIN-FIFO")) {
    return new CAB_DAG_FED_SPIN_FIFO((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "CAB-DAG-FED-SPIN-PRIO")) {
    return new CAB_DAG_FED_SPIN_PRIO((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GFP-SPIN-FIFO")) {
    return new RTA_DAG_GFP_SPIN_FIFO((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GFP-SPIN-PRIO")) {
    return new RTA_DAG_GFP_SPIN_PRIO((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-DAG-GFP-FMLP")) {
    return new LP_RTA_DAG_GFP_FMLP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-DAG-GFP-PIP")) {
    return new LP_RTA_DAG_GFP_PIP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-LPP")) {
    return new RTA_DAG_FED_LPP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-OMLP")) {
    return new RTA_DAG_FED_OMLP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-OMIP")) {
    return new RTA_DAG_FED_OMIP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-SPIN-FIFO-XU")) {
    return new RTA_DAG_FED_SPIN_FIFO_XU((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-SPIN-PRIO-XU")) {
    return new RTA_DAG_FED_SPIN_PRIO_XU((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-HYBRIDLOCKS")) {
    return new RTA_DAG_FED_HYBRIDLOCKS((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-HYBRIDLOCKS-FF")) {
    return new RTA_DAG_FED_HYBRIDLOCKS_FF((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-HYBRIDLOCKS-HEAVY")) {
    return new RTA_DAG_FED_HYBRIDLOCKS_HEAVY((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-FMLP")) {
    return new RTA_DAG_FED_FMLP((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-FP")) {
    return new RTA_DAG_FED_FP((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-FP-HEAVY")) {
    return new RTA_DAG_FED_FP_HEAVY((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-LI")) {
    return new RTA_DAG_FED_LI((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-LI-HEAVY")) {
    return new RTA_DAG_FED_LI_HEAVY((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-SFED-XU")) {
    return new RTA_DAG_SFED_XU((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-CGFP")) {
    return new RTA_DAG_CGFP((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-CGFP-v2")) {
    return new RTA_DAG_CGFP_v2((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-CGFP-v3")) {
    return new RTA_DAG_CGFP_v3((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-CGEDF")) {
    return new RTA_DAG_CGEDF((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-CGEDF-v2")) {
    return new RTA_DAG_CGEDF_v2((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-CGEDF-v3")) {
    return new RTA_DAG_CGEDF_v3((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GFP-MEL")) {
    return new RTA_DAG_GFP_MEL((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GEDF-MEL")) {
    return new RTA_DAG_GEDF_MEL((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "CAB-DAG-GFP-LI")) {
    return new CAB_DAG_GFP_LI((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "CAB-DAG-GEDF-LI")) {
    return new CAB_DAG_GEDF_LI((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-DPCP")) {
    return new RTA_DAG_FED_DPCP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-FED-DPCP-v2")) {
    return new RTA_DAG_FED_DPCP_v2((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GFP-JOSE")) {
    return new RTA_DAG_GFP_JOSE((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GFP-JOSE-FMLP")) {
    return new RTA_DAG_GFP_JOSE_FMLP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GFP-JOSE-PIP")) {
    return new RTA_DAG_GFP_JOSE_PIP((*dag_tasks), (*processors), (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GEDF-HOO")) {
    return new RTA_DAG_GEDF_HOO((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "RTA-DAG-GEDF-HOO-v2")) {
    return new RTA_DAG_GEDF_HOO_v2((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-H2LP")) {
    return new ILP_RTA_FED_H2LP((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-H2LP-FF")) {
    return new ILP_RTA_FED_H2LP_FF((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-H2LP-HEAVY")) {
    return new ILP_RTA_FED_H2LP_HEAVY((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-PFMLP")) {
    return new LP_RTA_GFP_PFMLP((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-PPIP")) {
    return new LP_RTA_GFP_PPIP((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-H2LP-v2")) {
    return new ILP_RTA_FED_H2LP_v2((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-H2LP-v3")) {
    return new ILP_RTA_FED_H2LP_v3((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-H2LP-v3-HEAVY")) {
    return new ILP_RTA_FED_H2LP_v3_HEAVY((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-H2LP-v4")) {
    return new ILP_RTA_FED_H2LP_v4((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-H2LP-v4-UTIL")) {
    return new ILP_RTA_FED_H2LP_v4_UTIL((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-H2LP-v4-HEAVY")) {
    return new ILP_RTA_FED_H2LP_v4_HEAVY((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-FED-SPIN-FIFO")) {
    return new LP_RTA_FED_SPIN_FIFO((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-FIFO")) {
    return new LP_RTA_GFP_SPIN_FIFO((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-XU-FIFO")) {
    return new LP_RTA_GFP_SPIN_XU_FIFO((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-XU-FIFO-P")) {
    return new LP_RTA_GFP_SPIN_XU_FIFO_P((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-PRIO")) {
    return new LP_RTA_GFP_SPIN_PRIO((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-XU-PRIO")) {
    return new LP_RTA_GFP_SPIN_XU_PRIO((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-XU-PRIO-P")) {
    return new LP_RTA_GFP_SPIN_XU_PRIO_P((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-UNOR")) {
    return new LP_RTA_GFP_SPIN_UNOR((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-XU-UNOR")) {
    return new LP_RTA_GFP_SPIN_XU_UNOR((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-XU-UNOR-P")) {
    return new LP_RTA_GFP_SPIN_XU_UNOR_P((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-GFP-SPIN-GENERAL")) {
    return new LP_RTA_GFP_SPIN_GENERAL((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-FED-SPIN-FIFO-v2")) {
    return new LP_RTA_FED_SPIN_FIFO_v2((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-FED-SPIN-FIFO-v3")) {
    return new LP_RTA_FED_SPIN_FIFO_v3((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-FED-SPIN-PRIO")) {
    return new LP_RTA_FED_SPIN_PRIO((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-PFP-GPU")) {
    return new LP_RTA_PFP_GPU((*tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-PFP-GPU-USS")) {
    return new LP_RTA_PFP_GPU_USS((*tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-PFP-GPU-USS-v2")) {
    return new LP_RTA_PFP_GPU_USS_v2((*tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "LP-RTA-PFP-GPU-PRIO")) {
    return new LP_RTA_PFP_GPU_prio((*tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-MPCP")) {
    return new ILP_RTA_FED_MPCP((*dag_tasks), (*processors),
                                       (*resources));
  } else if (0 == strcmp(test_name.data(), "ILP-RTA-FED-CMPCP")) {
    return new ILP_RTA_FED_CMPCP((*dag_tasks), (*processors),
                                       (*resources));
  } else {
    return NULL;
  }
} */


 return NULL;}
