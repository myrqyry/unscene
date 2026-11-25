#pragma once
#include <obs.h>
#include <graphics/graphics.h>
#include <graphics/vec4.h>
#include <QImage>
#include <QSize>
#include <algorithm>

// Capture only already-showing video. Never acquire an active/showing reference.
inline QImage captureSnapshot(obs_source_t *source) {
 if(!source||!obs_source_showing(source)||!(obs_source_get_output_flags(source)&OBS_SOURCE_VIDEO))return {};
 const auto sw=obs_source_get_width(source),sh=obs_source_get_height(source);
 if(!sw||!sh||sw>32768||sh>32768)return {};
 auto size=QSize(int(sw),int(sh)).scaled(QSize(96,54),Qt::KeepAspectRatio);
 const auto w=std::max(1,size.width()),h=std::max(1,size.height());
 QImage result;
 obs_enter_graphics();
 auto *render=gs_texrender_create(GS_RGBA,GS_ZS_NONE);
 auto *stage=gs_stagesurface_create(w,h,GS_RGBA);
 if(render&&stage&&gs_texrender_begin(render,w,h)){
  gs_blend_state_push();gs_blend_function(GS_BLEND_ONE,GS_BLEND_ZERO);
  gs_projection_push();gs_matrix_push();gs_matrix_identity();
  vec4 clear;vec4_zero(&clear);gs_clear(GS_CLEAR_COLOR,&clear,0,0);
  gs_ortho(0,float(sw),0,float(sh),-100,100);
  obs_source_video_render(source);
  gs_matrix_pop();gs_projection_pop();gs_blend_state_pop();
  gs_texrender_end(render);
  gs_stage_texture(stage,gs_texrender_get_texture(render));
  uint8_t *data=nullptr;uint32_t stride=0;
  if(gs_stagesurface_map(stage,&data,&stride)){
   if(data&&stride>=unsigned(w*4))result=QImage(data,w,h,int(stride),QImage::Format_RGBA8888).copy();
   gs_stagesurface_unmap(stage);
  }
 }
 if(stage)gs_stagesurface_destroy(stage);
 if(render)gs_texrender_destroy(render);
 obs_leave_graphics();
 return result;
}
