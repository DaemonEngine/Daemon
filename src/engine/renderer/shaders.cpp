#include <string>
#include <unordered_map>

#include "common.glsl.h"
#include "common_cp.glsl.h"
#include "shaderProfiler_vp.glsl.h"
#include "shaderProfiler_fp.glsl.h"

#include "material_cp.glsl.h"
#include "material_vp.glsl.h"
#include "material_fp.glsl.h"
#include "clearSurfaces_cp.glsl.h"
#include "cull_cp.glsl.h"
#include "depthReduction_cp.glsl.h"
#include "processSurfaces_cp.glsl.h"

#include "screenSpace_vp.glsl.h"
#include "blur_fp.glsl.h"
#include "cameraEffects_fp.glsl.h"
#include "contrast_fp.glsl.h"
#include "fogGlobal_fp.glsl.h"
#include "fxaa_fp.glsl.h"
#include "fxaa3_11_fp.glsl.h"
#include "motionblur_fp.glsl.h"
#include "ssao_fp.glsl.h"

#include "depthtile1_vp.glsl.h"
#include "depthtile1_fp.glsl.h"
#include "depthtile2_fp.glsl.h"
#include "lighttile_vp.glsl.h"
#include "lighttile_fp.glsl.h"
#include "computeLight_fp.glsl.h"
#include "reliefMapping_fp.glsl.h"
#include "luminanceReduction_cp.glsl.h"
#include "clearFrameData_cp.glsl.h"

#include "deformVertexes_vp.glsl.h"
#include "vertexAnimation_vp.glsl.h"
#include "vertexSimple_vp.glsl.h"
#include "vertexSkinning_vp.glsl.h"

#include "debugShadowMap_vp.glsl.h"
#include "debugShadowMap_fp.glsl.h"
#include "fogQuake3_vp.glsl.h"
#include "fogQuake3_fp.glsl.h"
#include "forwardLighting_vp.glsl.h"
#include "forwardLighting_fp.glsl.h"
#include "generic_vp.glsl.h"
#include "generic_fp.glsl.h"
#include "heatHaze_vp.glsl.h"
#include "heatHaze_fp.glsl.h"
#include "lightMapping_vp.glsl.h"
#include "lightMapping_fp.glsl.h"
#include "liquid_vp.glsl.h"
#include "liquid_fp.glsl.h"
#include "portal_vp.glsl.h"
#include "portal_fp.glsl.h"
#include "reflection_CB_vp.glsl.h"
#include "reflection_CB_fp.glsl.h"
#include "screen_vp.glsl.h"
#include "screen_fp.glsl.h"
#include "shadowFill_vp.glsl.h"
#include "shadowFill_fp.glsl.h"
#include "skybox_vp.glsl.h"
#include "skybox_fp.glsl.h"

std::unordered_map<std::string, std::string> shadermap({
	// Common shader libraries
	{ "common.glsl", std::string( reinterpret_cast< const char* >( common_glsl ), sizeof( common_glsl ) ) },
	{ "common_cp.glsl", std::string( reinterpret_cast< const char* >( common_cp_glsl ), sizeof( common_cp_glsl ) ) },
	{ "shaderProfiler_vp.glsl", std::string( reinterpret_cast< const char* >( shaderProfiler_vp_glsl ), sizeof( shaderProfiler_vp_glsl ) ) },
	{ "shaderProfiler_fp.glsl", std::string( reinterpret_cast< const char* >( shaderProfiler_fp_glsl ), sizeof( shaderProfiler_fp_glsl ) ) },

	// Material system shaders
	{ "material_cp.glsl", std::string( reinterpret_cast< const char* >( material_cp_glsl ), sizeof( material_cp_glsl ) ) },
	{ "material_vp.glsl", std::string( reinterpret_cast< const char* >( material_vp_glsl ), sizeof( material_vp_glsl ) ) },
	{ "material_fp.glsl", std::string( reinterpret_cast< const char* >( material_fp_glsl ), sizeof( material_fp_glsl ) ) },
	{ "clearSurfaces_cp.glsl", std::string( reinterpret_cast< const char* >( clearSurfaces_cp_glsl ), sizeof( clearSurfaces_cp_glsl ) ) },
	{ "cull_cp.glsl", std::string( reinterpret_cast< const char* >( cull_cp_glsl ), sizeof( cull_cp_glsl ) ) },
	{ "depthReduction_cp.glsl", std::string( reinterpret_cast< const char* >( depthReduction_cp_glsl ), sizeof( depthReduction_cp_glsl ) ) },
	{ "processSurfaces_cp.glsl", std::string( reinterpret_cast< const char* >( processSurfaces_cp_glsl ), sizeof( processSurfaces_cp_glsl ) ) },

	//  Screen-space shaders
	{ "screenSpace_vp.glsl", std::string( reinterpret_cast< const char* >( screenSpace_vp_glsl ), sizeof( screenSpace_vp_glsl ) ) },
	{ "blur_fp.glsl", std::string( reinterpret_cast< const char* >( blur_fp_glsl ), sizeof( blur_fp_glsl ) ) },
	{ "cameraEffects_fp.glsl", std::string( reinterpret_cast< const char* >( cameraEffects_fp_glsl ), sizeof( cameraEffects_fp_glsl ) ) },
	{ "contrast_fp.glsl", std::string( reinterpret_cast< const char* >( contrast_fp_glsl ), sizeof( contrast_fp_glsl ) ) },
	{ "fogGlobal_fp.glsl", std::string( reinterpret_cast< const char* >( fogGlobal_fp_glsl ), sizeof( fogGlobal_fp_glsl ) ) },
	{ "fxaa3_11_fp.glsl", std::string( reinterpret_cast< const char* >( fxaa3_11_fp_glsl ), sizeof( fxaa3_11_fp_glsl ) ) },
	{ "fxaa_fp.glsl", std::string( reinterpret_cast< const char* >( fxaa_fp_glsl ), sizeof( fxaa_fp_glsl ) ) },
	{ "motionblur_fp.glsl", std::string( reinterpret_cast< const char* >( motionblur_fp_glsl ), sizeof( motionblur_fp_glsl ) ) },
	{ "ssao_fp.glsl", std::string( reinterpret_cast< const char* >( ssao_fp_glsl ), sizeof( ssao_fp_glsl ) ) },

	// Lighting shaders
	{ "depthtile1_vp.glsl", std::string( reinterpret_cast< const char* >( depthtile1_vp_glsl ), sizeof( depthtile1_vp_glsl ) ) },
	{ "depthtile1_fp.glsl", std::string( reinterpret_cast< const char* >( depthtile1_fp_glsl ), sizeof( depthtile1_fp_glsl ) ) },
	{ "depthtile2_fp.glsl", std::string( reinterpret_cast< const char* >( depthtile2_fp_glsl ), sizeof( depthtile2_fp_glsl ) ) },
	{ "lighttile_vp.glsl", std::string( reinterpret_cast< const char* >( lighttile_vp_glsl ), sizeof( lighttile_vp_glsl ) ) },
	{ "lighttile_fp.glsl", std::string( reinterpret_cast< const char* >( lighttile_fp_glsl ), sizeof( lighttile_fp_glsl ) ) },
	{ "computeLight_fp.glsl", std::string( reinterpret_cast< const char* >( computeLight_fp_glsl ), sizeof( computeLight_fp_glsl ) ) },
	{ "reliefMapping_fp.glsl", std::string( reinterpret_cast< const char* >( reliefMapping_fp_glsl ), sizeof( reliefMapping_fp_glsl ) ) },
	{ "luminanceReduction_cp.glsl", std::string( reinterpret_cast< const char* >( luminanceReduction_cp_glsl ), sizeof( luminanceReduction_cp_glsl ) ) },
	{ "clearFrameData_cp.glsl", std::string( reinterpret_cast< const char* >( clearFrameData_cp_glsl ), sizeof( clearFrameData_cp_glsl ) ) },

	// Common vertex shader libraries
	{ "deformVertexes_vp.glsl", std::string( reinterpret_cast< const char* >( deformVertexes_vp_glsl ), sizeof( deformVertexes_vp_glsl ) ) },
	{ "vertexAnimation_vp.glsl", std::string( reinterpret_cast< const char* >( vertexAnimation_vp_glsl ), sizeof( vertexAnimation_vp_glsl ) ) },
	{ "vertexSimple_vp.glsl", std::string( reinterpret_cast< const char* >( vertexSimple_vp_glsl ), sizeof( vertexSimple_vp_glsl ) ) },
	{ "vertexSkinning_vp.glsl", std::string( reinterpret_cast< const char* >( vertexSkinning_vp_glsl ), sizeof( vertexSkinning_vp_glsl ) ) },

	// Regular shaders
	{ "debugShadowMap_vp.glsl", std::string( reinterpret_cast< const char* >( debugShadowMap_vp_glsl ), sizeof( debugShadowMap_vp_glsl ) ) },
	{ "debugShadowMap_fp.glsl", std::string( reinterpret_cast< const char* >( debugShadowMap_fp_glsl ), sizeof( debugShadowMap_fp_glsl ) ) },
	{ "fogQuake3_vp.glsl", std::string( reinterpret_cast< const char* >( fogQuake3_vp_glsl ), sizeof( fogQuake3_vp_glsl ) ) },
	{ "fogQuake3_fp.glsl", std::string( reinterpret_cast< const char* >( fogQuake3_fp_glsl ), sizeof( fogQuake3_fp_glsl ) ) },
	{ "forwardLighting_vp.glsl", std::string( reinterpret_cast< const char* >( forwardLighting_vp_glsl ), sizeof( forwardLighting_vp_glsl ) ) },
	{ "forwardLighting_fp.glsl", std::string( reinterpret_cast< const char* >( forwardLighting_fp_glsl ), sizeof( forwardLighting_fp_glsl ) ) },
	{ "generic_vp.glsl", std::string( reinterpret_cast< const char* >( generic_vp_glsl ), sizeof( generic_vp_glsl ) ) },
	{ "generic_fp.glsl", std::string( reinterpret_cast< const char* >( generic_fp_glsl ), sizeof( generic_fp_glsl ) ) },
	{ "heatHaze_vp.glsl", std::string( reinterpret_cast< const char* >( heatHaze_vp_glsl ), sizeof( heatHaze_vp_glsl ) ) },
	{ "heatHaze_fp.glsl", std::string( reinterpret_cast< const char* >( heatHaze_fp_glsl ), sizeof( heatHaze_fp_glsl ) ) },
	{ "lightMapping_vp.glsl", std::string( reinterpret_cast< const char* >( lightMapping_vp_glsl ), sizeof( lightMapping_vp_glsl ) ) },
	{ "lightMapping_fp.glsl", std::string( reinterpret_cast< const char* >( lightMapping_fp_glsl ), sizeof( lightMapping_fp_glsl ) ) },
	{ "liquid_vp.glsl", std::string( reinterpret_cast< const char* >( liquid_vp_glsl ), sizeof( liquid_vp_glsl ) ) },
	{ "liquid_fp.glsl", std::string( reinterpret_cast< const char* >( liquid_fp_glsl ), sizeof( liquid_fp_glsl ) ) },
	{ "portal_vp.glsl", std::string( reinterpret_cast< const char* >( portal_vp_glsl ), sizeof( portal_vp_glsl ) ) },
	{ "portal_fp.glsl", std::string( reinterpret_cast< const char* >( portal_fp_glsl ), sizeof( portal_fp_glsl ) ) },
	{ "reflection_CB_vp.glsl", std::string( reinterpret_cast< const char* >( reflection_CB_vp_glsl ), sizeof( reflection_CB_vp_glsl ) ) },
	{ "reflection_CB_fp.glsl", std::string( reinterpret_cast< const char* >( reflection_CB_fp_glsl ), sizeof( reflection_CB_fp_glsl ) ) },
	{ "screen_vp.glsl", std::string( reinterpret_cast< const char* >( screen_vp_glsl ), sizeof( screen_vp_glsl ) ) },
	{ "screen_fp.glsl", std::string( reinterpret_cast< const char* >( screen_fp_glsl ), sizeof( screen_fp_glsl ) ) },
	{ "shadowFill_vp.glsl", std::string( reinterpret_cast< const char* >( shadowFill_vp_glsl ), sizeof( shadowFill_vp_glsl ) ) },
	{ "shadowFill_fp.glsl", std::string( reinterpret_cast< const char* >( shadowFill_fp_glsl ), sizeof( shadowFill_fp_glsl ) ) },
	{ "skybox_vp.glsl", std::string( reinterpret_cast< const char* >( skybox_vp_glsl ), sizeof( skybox_vp_glsl ) ) },
	{ "skybox_fp.glsl", std::string( reinterpret_cast< const char* >( skybox_fp_glsl ), sizeof( skybox_fp_glsl ) ) },
	});
