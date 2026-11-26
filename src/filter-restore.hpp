#pragma once
#include <obs.h>
inline bool attachRestoredFilter(obs_source_t*parent,obs_source_t*filter,size_t index){
 obs_source_filter_add(parent,filter);
 if(obs_source_filter_get_index(parent,filter)<0)return false;
 obs_source_filter_set_index(parent,filter,index);
 obs_source_load(filter);
 return true;
}
