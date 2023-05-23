#include <string>
#include <unordered_map>
#include "skybox_vp.glsl.h"
#include "ssao_fp.glsl.h"
#include "ssao_vp.glsl.h"
#include "vertexAnimation_vp.glsl.h"
#include "vertexSimple_vp.glsl.h"
#include "vertexSkinning_vp.glsl.h"
#include "vertexSprite_vp.glsl.h"
#include "blurX_fp.glsl.h"
#include "blurX_vp.glsl.h"
#include "blurY_fp.glsl.h"
#include "blurY_vp.glsl.h"
#include "cameraEffects_fp.glsl.h"
#include "cameraEffects_vp.glsl.h"
#include "computeLight_fp.glsl.h"
#include "contrast_fp.glsl.h"
#include "contrast_vp.glsl.h"
#include "debugShadowMap_fp.glsl.h"
#include "debugShadowMap_vp.glsl.h"
#include "deformVertexes_vp.glsl.h"
#include "depthtile1_fp.glsl.h"
#include "depthtile1_vp.glsl.h"
#include "depthtile2_fp.glsl.h"
#include "depthtile2_vp.glsl.h"
#include "dispersion_C_fp.glsl.h"
#include "dispersion_C_vp.glsl.h"
#include "fogGlobal_fp.glsl.h"
#include "fogGlobal_vp.glsl.h"
#include "fogQuake3_fp.glsl.h"
#include "fogQuake3_vp.glsl.h"
#include "forwardLighting_fp.glsl.h"
#include "forwardLighting_vp.glsl.h"
#include "fxaa_fp.glsl.h"
#include "fxaa_vp.glsl.h"
#include "fxaa3_11_fp.glsl.h"
#include "generic_fp.glsl.h"
#include "generic_vp.glsl.h"
#include "heatHaze_fp.glsl.h"
#include "heatHaze_vp.glsl.h"
#include "lightMapping_fp.glsl.h"
#include "lightMapping_vp.glsl.h"
#include "lighttile_fp.glsl.h"
#include "lighttile_vp.glsl.h"
#include "liquid_fp.glsl.h"
#include "liquid_vp.glsl.h"
#include "motionblur_fp.glsl.h"
#include "motionblur_vp.glsl.h"
#include "portal_fp.glsl.h"
#include "portal_vp.glsl.h"
#include "reflection_CB_fp.glsl.h"
#include "reflection_CB_vp.glsl.h"
#include "refraction_C_fp.glsl.h"
#include "refraction_C_vp.glsl.h"
#include "reliefMapping_fp.glsl.h"
#include "screen_fp.glsl.h"
#include "screen_vp.glsl.h"
#include "shadowFill_fp.glsl.h"
#include "shadowFill_vp.glsl.h"
#include "skybox_fp.glsl.h"

std::unordered_map<std::string, std::string> shadermap({
	{ "glsl/blurX_fp.glsl", std::string(reinterpret_cast<const char*>(blurX_fp_glsl), sizeof(blurX_fp_glsl)) },
	{ "glsl/blurX_vp.glsl", std::string(reinterpret_cast<const char*>(blurX_vp_glsl), sizeof(blurX_vp_glsl)) },
	{ "glsl/blurY_fp.glsl", std::string(reinterpret_cast<const char*>(blurY_fp_glsl), sizeof(blurY_fp_glsl)) },
	{ "glsl/blurY_vp.glsl", std::string(reinterpret_cast<const char*>(blurY_vp_glsl), sizeof(blurY_vp_glsl)) },
	{ "glsl/cameraEffects_fp.glsl", std::string(reinterpret_cast<const char*>(cameraEffects_fp_glsl), sizeof(cameraEffects_fp_glsl)) },
	{ "glsl/cameraEffects_vp.glsl", std::string(reinterpret_cast<const char*>(cameraEffects_vp_glsl), sizeof(cameraEffects_vp_glsl)) },
	{ "glsl/computeLight_fp.glsl", std::string(reinterpret_cast<const char*>(computeLight_fp_glsl), sizeof(computeLight_fp_glsl)) },
	{ "glsl/contrast_fp.glsl", std::string(reinterpret_cast<const char*>(contrast_fp_glsl), sizeof(contrast_fp_glsl)) },
	{ "glsl/contrast_vp.glsl", std::string(reinterpret_cast<const char*>(contrast_vp_glsl), sizeof(contrast_vp_glsl)) },
	{ "glsl/debugShadowMap_fp.glsl", std::string(reinterpret_cast<const char*>(debugShadowMap_fp_glsl), sizeof(debugShadowMap_fp_glsl)) },
	{ "glsl/debugShadowMap_vp.glsl", std::string(reinterpret_cast<const char*>(debugShadowMap_vp_glsl), sizeof(debugShadowMap_vp_glsl)) },
	{ "glsl/deformVertexes_vp.glsl", std::string(reinterpret_cast<const char*>(deformVertexes_vp_glsl), sizeof(deformVertexes_vp_glsl)) },
	{ "glsl/depthtile1_fp.glsl", std::string(reinterpret_cast<const char*>(depthtile1_fp_glsl), sizeof(depthtile1_fp_glsl)) },
	{ "glsl/depthtile1_vp.glsl", std::string(reinterpret_cast<const char*>(depthtile1_vp_glsl), sizeof(depthtile1_vp_glsl)) },
	{ "glsl/depthtile2_fp.glsl", std::string(reinterpret_cast<const char*>(depthtile2_fp_glsl), sizeof(depthtile2_fp_glsl)) },
	{ "glsl/depthtile2_vp.glsl", std::string(reinterpret_cast<const char*>(depthtile2_vp_glsl), sizeof(depthtile2_vp_glsl)) },
	{ "glsl/dispersion_C_fp.glsl", std::string(reinterpret_cast<const char*>(dispersion_C_fp_glsl), sizeof(dispersion_C_fp_glsl)) },
	{ "glsl/dispersion_C_vp.glsl", std::string(reinterpret_cast<const char*>(dispersion_C_vp_glsl), sizeof(dispersion_C_vp_glsl)) },
	{ "glsl/fogGlobal_fp.glsl", std::string(reinterpret_cast<const char*>(fogGlobal_fp_glsl), sizeof(fogGlobal_fp_glsl)) },
	{ "glsl/fogGlobal_vp.glsl", std::string(reinterpret_cast<const char*>(fogGlobal_vp_glsl), sizeof(fogGlobal_vp_glsl)) },
	{ "glsl/fogQuake3_fp.glsl", std::string(reinterpret_cast<const char*>(fogQuake3_fp_glsl), sizeof(fogQuake3_fp_glsl)) },
	{ "glsl/fogQuake3_vp.glsl", std::string(reinterpret_cast<const char*>(fogQuake3_vp_glsl), sizeof(fogQuake3_vp_glsl)) },
	{ "glsl/forwardLighting_fp.glsl", std::string(reinterpret_cast<const char*>(forwardLighting_fp_glsl), sizeof(forwardLighting_fp_glsl)) },
	{ "glsl/forwardLighting_vp.glsl", std::string(reinterpret_cast<const char*>(forwardLighting_vp_glsl), sizeof(forwardLighting_vp_glsl)) },
	{ "glsl/fxaa3_11_fp.glsl", std::string(reinterpret_cast<const char*>(fxaa3_11_fp_glsl), sizeof(fxaa3_11_fp_glsl)) },
	{ "glsl/fxaa_fp.glsl", std::string(reinterpret_cast<const char*>(fxaa_fp_glsl), sizeof(fxaa_fp_glsl)) },
	{ "glsl/fxaa_vp.glsl", std::string(reinterpret_cast<const char*>(fxaa_vp_glsl), sizeof(fxaa_vp_glsl)) },
	{ "glsl/generic_fp.glsl", std::string(reinterpret_cast<const char*>(generic_fp_glsl), sizeof(generic_fp_glsl)) },
	{ "glsl/generic_vp.glsl", std::string(reinterpret_cast<const char*>(generic_vp_glsl), sizeof(generic_vp_glsl)) },
	{ "glsl/heatHaze_fp.glsl", std::string(reinterpret_cast<const char*>(heatHaze_fp_glsl), sizeof(heatHaze_fp_glsl)) },
	{ "glsl/heatHaze_vp.glsl", std::string(reinterpret_cast<const char*>(heatHaze_vp_glsl), sizeof(heatHaze_vp_glsl)) },
	{ "glsl/lightMapping_fp.glsl", std::string(reinterpret_cast<const char*>(lightMapping_fp_glsl), sizeof(lightMapping_fp_glsl)) },
	{ "glsl/lightMapping_vp.glsl", std::string(reinterpret_cast<const char*>(lightMapping_vp_glsl), sizeof(lightMapping_vp_glsl)) },
	{ "glsl/lighttile_fp.glsl", std::string(reinterpret_cast<const char*>(lighttile_fp_glsl), sizeof(lighttile_fp_glsl)) },
	{ "glsl/lighttile_vp.glsl", std::string(reinterpret_cast<const char*>(lighttile_vp_glsl), sizeof(lighttile_vp_glsl)) },
	{ "glsl/liquid_fp.glsl", std::string(reinterpret_cast<const char*>(liquid_fp_glsl), sizeof(liquid_fp_glsl)) },
	{ "glsl/liquid_vp.glsl", std::string(reinterpret_cast<const char*>(liquid_vp_glsl), sizeof(liquid_vp_glsl)) },
	{ "glsl/motionblur_fp.glsl", std::string(reinterpret_cast<const char*>(motionblur_fp_glsl), sizeof(motionblur_fp_glsl)) },
	{ "glsl/motionblur_vp.glsl", std::string(reinterpret_cast<const char*>(motionblur_vp_glsl), sizeof(motionblur_vp_glsl)) },
	{ "glsl/portal_fp.glsl", std::string(reinterpret_cast<const char*>(portal_fp_glsl), sizeof(portal_fp_glsl)) },
	{ "glsl/portal_vp.glsl", std::string(reinterpret_cast<const char*>(portal_vp_glsl), sizeof(portal_vp_glsl)) },
	{ "glsl/reflection_CB_fp.glsl", std::string(reinterpret_cast<const char*>(reflection_CB_fp_glsl), sizeof(reflection_CB_fp_glsl)) },
	{ "glsl/reflection_CB_vp.glsl", std::string(reinterpret_cast<const char*>(reflection_CB_vp_glsl), sizeof(reflection_CB_vp_glsl)) },
	{ "glsl/refraction_C_fp.glsl", std::string(reinterpret_cast<const char*>(refraction_C_fp_glsl), sizeof(refraction_C_fp_glsl)) },
	{ "glsl/refraction_C_vp.glsl", std::string(reinterpret_cast<const char*>(refraction_C_vp_glsl), sizeof(refraction_C_vp_glsl)) },
	{ "glsl/reliefMapping_fp.glsl", std::string(reinterpret_cast<const char*>(reliefMapping_fp_glsl), sizeof(reliefMapping_fp_glsl)) },
	{ "glsl/screen_fp.glsl", std::string(reinterpret_cast<const char*>(screen_fp_glsl), sizeof(screen_fp_glsl)) },
	{ "glsl/screen_vp.glsl", std::string(reinterpret_cast<const char*>(screen_vp_glsl), sizeof(screen_vp_glsl)) },
	{ "glsl/shadowFill_fp.glsl", std::string(reinterpret_cast<const char*>(shadowFill_fp_glsl), sizeof(shadowFill_fp_glsl)) },
	{ "glsl/shadowFill_vp.glsl", std::string(reinterpret_cast<const char*>(shadowFill_vp_glsl), sizeof(shadowFill_vp_glsl)) },
	{ "glsl/skybox_fp.glsl", std::string(reinterpret_cast<const char*>(skybox_fp_glsl), sizeof(skybox_fp_glsl)) },
	{ "glsl/skybox_vp.glsl", std::string(reinterpret_cast<const char*>(skybox_vp_glsl), sizeof(skybox_vp_glsl)) },
	{ "glsl/ssao_fp.glsl", std::string(reinterpret_cast<const char*>(ssao_fp_glsl), sizeof(ssao_fp_glsl)) },
	{ "glsl/ssao_vp.glsl", std::string(reinterpret_cast<const char*>(ssao_vp_glsl), sizeof(ssao_vp_glsl)) },
	{ "glsl/vertexAnimation_vp.glsl", std::string(reinterpret_cast<const char*>(vertexAnimation_vp_glsl), sizeof(vertexAnimation_vp_glsl)) },
	{ "glsl/vertexSimple_vp.glsl", std::string(reinterpret_cast<const char*>(vertexSimple_vp_glsl), sizeof(vertexSimple_vp_glsl)) },
	{ "glsl/vertexSkinning_vp.glsl", std::string(reinterpret_cast<const char*>(vertexSkinning_vp_glsl), sizeof(vertexSkinning_vp_glsl)) },
	{ "glsl/vertexSprite_vp.glsl", std::string(reinterpret_cast<const char*>(vertexSprite_vp_glsl), sizeof(vertexSprite_vp_glsl)) },
	});
