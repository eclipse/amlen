#!/bin/bash

test_template5_ShowSummary() {
  echo
  echo ================================================================================================
  echo
  echo xml[${n}]=\"${xml[${n}]}\" 
  echo test_template_initialize_test \"${xml[${n}]}\"  
  echo scenario[${n}]=\"${scenario[${n}]}\" 
  echo timeouts[${n}]=${timeouts[${n}]}

  echo 
  for I in ${!component[@]}; do
     echo component[${I}]=\"${component[${I}]}\" 
  done

  echo 
  for I in ${!test_template_compare_string[@]}; do
    echo test_template_compare_string[${I}]=\"${test_template_compare_string[${I}]}\" 
  done

  echo 
  for I in ${!test_template_metrics_v1[@]}; do
    echo test_template_metrics_v1[${I}]=\"${test_template_metrics_v1[${I}]}\" 
  done

  echo 
  echo test_template_runorder=\"${test_template_runorder}\" 
  echo test_template_finalize_test 
  echo
  echo ================================================================================================
  echo
}


