#include "../src/filter-restore.hpp"
#include <obs.h>
#include <cstdio>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"failed: %s\n",#x);return 1;}}while(false)
struct Fixture {obs_source_t*source;};
static int loads=0,loadedIndex=-1,loadedValue=0;
static obs_source_t*expectedParent=nullptr;
static const char*name(void*){return "Restore fixture";}
static void*create(obs_data_t*,obs_source_t*s){return new Fixture{s};}
static void destroy(void*p){delete static_cast<Fixture*>(p);}
static void load(void*p,obs_data_t*d){
 auto*s=static_cast<Fixture*>(p)->source;++loads;
 auto*parent=obs_filter_get_parent(s);
 if(parent==expectedParent)loadedIndex=obs_source_filter_get_index(parent,s);
 loadedValue=int(obs_data_get_int(d,"value"));
}
int main(){
 CHECK(obs_startup("en-US",nullptr,nullptr));
 obs_source_info input{};input.id="unscene_restore_input";input.type=OBS_SOURCE_TYPE_INPUT;input.output_flags=OBS_SOURCE_AUDIO;input.get_name=name;input.create=create;input.destroy=destroy;
 obs_register_source(&input);
 auto filter=input;filter.id="unscene_restore_filter";filter.type=OBS_SOURCE_TYPE_FILTER;filter.load=load;
 obs_register_source(&filter);
 auto*parent=obs_source_create(input.id,"Parent",nullptr,nullptr);CHECK(parent);expectedParent=parent;
 auto*first=obs_source_create(filter.id,"First",nullptr,nullptr);CHECK(first);
 obs_source_filter_add(parent,first);
 auto*settings=obs_data_create();obs_data_set_int(settings,"value",73);
 auto*original=obs_source_create(filter.id,"Restored",settings,nullptr);CHECK(original);obs_data_release(settings);
 auto*saved=obs_save_source(original);CHECK(saved);
 obs_source_release(original);obs_wait_for_destroy_queue();
 auto*restored=obs_load_source(saved);obs_data_release(saved);CHECK(restored);
 CHECK(loads==0);
 CHECK(attachRestoredFilter(parent,restored,1));
 CHECK(loads==1&&loadedIndex==1&&loadedValue==73);
 obs_source_filter_remove(parent,restored);obs_source_release(restored);
 obs_source_filter_remove(parent,first);obs_source_release(first);obs_source_release(parent);
 obs_wait_for_destroy_queue();obs_shutdown();return 0;
}
