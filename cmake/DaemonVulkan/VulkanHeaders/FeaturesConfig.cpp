// Auto-generated, do not modify

#include "Vulkan.h"

#include "GraphicsCoreStore.h"

#include "EngineConfig.h"

#include "FeaturesConfig.h"

FeaturesConfig GetPhysicalDeviceFeatures( const VkPhysicalDevice physicalDevice, const EngineConfig& engineCfg ) {
    const bool intelWorkaround = std::string( engineCfg.driverName ).find( "Intel" ) != std::string::npos;

