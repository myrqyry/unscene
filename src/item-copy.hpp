#pragma once
#include <obs.h>
#include <QString>
#include <optional>

struct ItemCopy {
 QString uid;
 obs_transform_info transform{};
 obs_sceneitem_crop crop{};
 obs_scale_type scale=OBS_SCALE_DISABLE;
 obs_blending_method blendMethod=OBS_BLEND_METHOD_DEFAULT;
 obs_blending_type blend=OBS_BLEND_NORMAL;
 bool visible=true,locked=false;
 static std::optional<ItemCopy> capture(obs_sceneitem_t*item){
  if(!item)return {};
  auto*s=obs_sceneitem_get_source(item);
  if(!s||obs_source_removed(s)||obs_source_is_group(s)||obs_source_is_scene(s))return {};
  ItemCopy c;c.uid=QString::fromUtf8(obs_source_get_uuid(s));
  if(c.uid.isEmpty())return {};
  obs_sceneitem_get_info2(item,&c.transform);obs_sceneitem_get_crop(item,&c.crop);
  c.scale=obs_sceneitem_get_scale_filter(item);c.blendMethod=obs_sceneitem_get_blending_method(item);c.blend=obs_sceneitem_get_blending_mode(item);
  c.visible=obs_sceneitem_visible(item);c.locked=obs_sceneitem_locked(item);
  return c;
 }
 obs_sceneitem_t* paste(obs_scene_t*scene,obs_source_t*source)const{
  if(!scene||!source||obs_source_removed(source)||obs_source_is_group(source)||obs_source_is_scene(source))return nullptr;
  struct Context{const ItemCopy*c;obs_source_t*s;obs_sceneitem_t*result=nullptr;}ctx{this,source};
  obs_scene_atomic_update(scene,[](void*opaque,obs_scene_t*scene){
   auto&ctx=*static_cast<Context*>(opaque);auto*i=obs_scene_add(scene,ctx.s);if(!i)return;
   auto&c=*ctx.c;
   obs_sceneitem_defer_update_begin(i);
   obs_sceneitem_set_visible(i,c.visible);obs_sceneitem_set_info2(i,&c.transform);obs_sceneitem_set_crop(i,&c.crop);
   obs_sceneitem_set_scale_filter(i,c.scale);obs_sceneitem_set_blending_method(i,c.blendMethod);obs_sceneitem_set_blending_mode(i,c.blend);
   obs_sceneitem_set_locked(i,c.locked);obs_sceneitem_defer_update_end(i);ctx.result=i;
  },&ctx);
  return ctx.result;
 }
};
