import os

def init():
    global headerText
    global functionDefinitionsText

    global functionLoadInstanceText
    global functionLoadDeviceText
    global allFeatures
    global coreFeatures

    global featureStructsSkip

    global vendorFeatures

    global currentVersionExtension
    
    headerText               = ""
    functionDefinitionsText  = ""

    functionLoadInstanceText = ""
    functionLoadDeviceText   = ""
    allFeatures              = {}
    coreFeatures             = {}

    vendorFeatures           = {}

    currentVersionExtension  = ""

    featureStructsSkip = [
        "VkPhysicalDeviceFeatures",
        "VkPhysicalDeviceVulkan11Features",
        "VkPhysicalDeviceVulkan12Features",
        "VkPhysicalDeviceVulkan13Features",
        "VkPhysicalDeviceVulkan14Features"
    ]

def SkipFeatures( featureStructs, skip ):
    skipFeatures = []

    for skipStruct in skip:
        if skipStruct in featureStructs:
            for feature in featureStructs[skipStruct][1]:
                skipFeatures.append( feature )
    
    return skipFeatures

def ProcessFeatureStruct( outCfg, outGet, outCreate, outCreateDevice, outFeatureMap, featureType, features, skipFeatures, suffixedFeatures,
                          prev = "", extraFeature = "", extraDeviceFeature = "",
                          skipCreate = False, skipGet = False, skipCfg = False, skipCreateDevice = False, intelWorkaround = False ):
    f = [k for k in features[1] if k not in skipFeatures]

    if len( f ) == 0:
        return outCfg, outGet, outCreate, outCreateDevice, outFeatureMap, prev, True

    getStruct          = ""
    createDeviceStruct = ""

    if not skipGet:
        getStruct          = "\t" + featureType + " features" + featureType + " {"
        createDeviceStruct = getStruct
        
        if prev:
            if intelWorkaround:
                getStruct          += " .pNext = intelWorkaround ? nullptr : &"      + prev + " "
                createDeviceStruct += "\n\t\t.pNext = intelWorkaround ? nullptr : &" + prev + ",\n"
            else:
                getStruct          += " .pNext = &"      + prev + " "
                createDeviceStruct += "\n\t\t.pNext = &" + prev + ",\n"
    
        outGet          += getStruct + "};\n"

    createDeviceFeatures = ""
    
    if extraDeviceFeature:
        createDeviceFeatures  = "\t" + extraDeviceFeature + " features" + extraDeviceFeature + " {\n"
    elif not prev:
        createDeviceFeatures += "\n"

    for feature in features[1]:
        suffixedFeature = feature

        if featureType in suffixedFeatures:
            suffixedFeature += suffixedFeatures[featureType]
        
        if not skipCfg:
            outCfg    += "\tbool " + suffixedFeature + ";\n"

        if not skipCreate:
            if extraFeature:
                outCreate       += "\t\t." + suffixedFeature + " = ( bool ) " + extraFeature             + "." + feature + ",\n"
            else:
                outCreate       += "\t\t." + suffixedFeature + " = ( bool ) " + "features" + featureType + "." + feature + ",\n"
        
        if not skipCreateDevice:
            createDeviceFeatures += "\t\t."          + feature + " = cfg." + suffixedFeature + ",\n"
        
        ext            = features[0] if features[0] not in ( "VK_VERSION_1_0", "VK_VERSION_1_1", "VK_VERSION_1_2", "VK_VERSION_1_3", "VK_VERSION_1_4" ) else ""
        outFeatureMap += "\t{ \"" + suffixedFeature + "\", { offsetof( FeaturesConfig, " + suffixedFeature + " ),\"" + ext + "\" } },\n"
    
    if not skipCreateDevice:
        createDeviceFeatures = createDeviceFeatures.rstrip( "," )

        if extraDeviceFeature:
            outCreateDevice += createDeviceFeatures + "\t};\n\n" + createDeviceStruct + "\t\t.features = features" + extraDeviceFeature + "\n\t};\n"
        else:
            outCreateDevice += createDeviceStruct + createDeviceFeatures + "\t};\n\n"

    return outCfg, outGet, outCreate, outCreateDevice, outFeatureMap, "features" + featureType, False
    
def ProcessFeatures( outCfgH, outCfgGet, outCfgCreate, outCfgCreateDevice, outCfgFeatureMap ):
    outCfg           = ""
    outGet           = ""
    outCreate        = ""
    outCreateDevice  = ""
    prev             = ""

    suffixedFeatures = {}
    outFeatureMap    = ""
                
    for k, v in vendorFeatures.items():
        if len( v["suffixes"] ) > 1:
            for suffix in v["suffixes"]:
                suffixedFeatures[k + suffix] = suffix

    skipFeatures = SkipFeatures( allFeatures, featureStructsSkip )

    if os.path.exists( "prev" ):
        with open( "prev", mode = "r", encoding = "utf-8", newline = "\n" ) as prevIn:
            prev = prevIn.read()

    # Driver bug: Intel proprietary driver fails to fill out the next structs in the pNext chain when it encounters this one,
    # so we have to process it first
    useIntelWorkaround = False
    if "VkPhysicalDevicePipelineBinaryFeaturesKHR" in allFeatures:
        outCfg, outGet, outCreate, outCreateDevice, outFeatureMap, prev, unused               = ProcessFeatureStruct( outCfg, outGet, outCreate, outCreateDevice, outFeatureMap,
                                                                                   "VkPhysicalDevicePipelineBinaryFeaturesKHR",
                                                                                   allFeatures["VkPhysicalDevicePipelineBinaryFeaturesKHR"],
                                                                                   skipFeatures, suffixedFeatures, prev )
        del allFeatures["VkPhysicalDevicePipelineBinaryFeaturesKHR"]
        useIntelWorkaround = True
    
    for featureType, features      in allFeatures.items():
        if featureType == "VkPhysicalDeviceFeatures" or featureType == "VkPhysicalDeviceFeatures2":
            continue
        
        outCfg, outGet, outCreate, outCreateDevice, outFeatureMap, prev, useIntelWorkaround   = ProcessFeatureStruct( outCfg, outGet, outCreate, outCreateDevice, outFeatureMap,
                                                                                   featureType, features, skipFeatures, suffixedFeatures, prev,
                                                                                   intelWorkaround = useIntelWorkaround )
    
    outCfgH.write( outCfg )
    outCfgGet.write( outGet )
    outCfgCreate.write( outCreate )
    outCfgCreateDevice.write( outCreateDevice )
    outCfgFeatureMap.write( outFeatureMap )

    with open( "prev", mode = "w", encoding = "utf-8", newline = "\n" ) as prevOut:
        prevOut.write( prev )

    if "VkPhysicalDeviceFeatures" in allFeatures and "VkPhysicalDeviceFeatures2" in allFeatures:
        outCfg, outGet, outCreate, outCreateDevice, outFeatureMap, unused, unused2            = ProcessFeatureStruct( "", "", "", "", "",
                                                                                   "VkPhysicalDeviceFeatures",
                                                                                   allFeatures["VkPhysicalDeviceFeatures"], [], {},
                                                                                   skipGet = True, skipCfg = True, skipCreateDevice = True,
                                                                                   extraFeature = "featuresVkPhysicalDeviceFeatures2.features" )

        outCfg, outGet, outCreate, outCreateDevice, outFeatureMap, prev, unused               = ProcessFeatureStruct( outCfg, outGet, outCreate, outCreateDevice, outFeatureMap,
                                                                                   "VkPhysicalDeviceFeatures2",
                                                                                   allFeatures["VkPhysicalDeviceFeatures"], [], {},
                                                                                   prev,
                                                                                   skipCreate = True, extraDeviceFeature = "VkPhysicalDeviceFeatures",
                                                                                   extraFeature = "featuresVkPhysicalDeviceFeatures2.features" )
        
        outGet          += "\n\tvkGetPhysicalDeviceFeatures2( physicalDevice, &featuresVkPhysicalDeviceFeatures2 );\n\n"

        outCreate       += "\t};\n\n"
        outCreate       += "\treturn cfg;\n"
        outCreate       += "}\n\n"

        outCreateDevice += "\n\tdeviceInfo.pNext = &featuresVkPhysicalDeviceFeatures2;\n"
        outCreateDevice += "\n\treturn vkCreateDevice( physicalDevice, &deviceInfo, allocator, device );\n"
        outCreateDevice += "}"

        # Must be last in the pNext chain
        with open( "FeaturesConfigMain.h", mode = "w", encoding = "utf-8", newline = "\n" ) as cfgHMainOut:
            cfgHMainOut.write( outCfg )
        
        with open( "FeaturesConfigGetMain", mode = "w", encoding = "utf-8", newline = "\n" ) as cfgGetMainOut:
            cfgGetMainOut.write( outGet )
        
        with open( "FeaturesConfigCreateMain", mode = "w", encoding = "utf-8", newline = "\n" ) as cfgCreateMainOut:
            cfgCreateMainOut.write( outCreate )
        
        with open( "FeaturesConfigCreateDeviceMain", mode = "w", encoding = "utf-8", newline = "\n" ) as cfgCreateDeviceMainOut:
            cfgCreateDeviceMainOut.write( outCreateDevice )
        
        with open( "FeaturesConfigMapMain", mode = "w", encoding = "utf-8", newline = "\n" ) as cfgMapMainOut:
            cfgMapMainOut.write( outFeatureMap )

def GenerateHeaders( inputDir, outputDir, mode, define ):
        headerStart       = ""
        functionLoadStart = ""

        if mode == "w":
            with open( inputDir + "Vulkan.h", mode = "r", encoding = "utf-8", newline = "\n" ) as inp:
                headerStart = inp.read()
                
            with open( inputDir + "VulkanLoadFunctions.cpp", mode = "r", encoding = "utf-8", newline = "\n" ) as inp:
                functionLoadStart = inp.read()

        outputDir = outputDir.rstrip( "/" ).rsplit( "/", 1 )[0] + "/"
        
        indent    = "\t" if define else ""
        
        with open( "FunctionDecls.h", mode = mode, encoding = "utf-8", newline = "\n" ) as out:
            if define:
                out.write( "#if defined( " + define + " )\n" )
            
            out.write( headerStart + indent + headerText )

            if define:
                out.write( "#endif\n\n" )
        
        with open( outputDir + "Vulkan.cpp", mode = mode, encoding = "utf-8", newline = "\n" ) as out:
            if mode == "w":
                out.write( "// Auto-generated, do not modify\n\n" )
                out.write( "#include \"Vulkan.h\"\n\n" )
            
            if define:
                out.write( "#if defined( " + define + " )\n" )
            
            out.write( indent + functionDefinitionsText )

            if define:
                out.write( "#endif\n\n" )
        
        #with open( outputDir + "VulkanLoadFunctions.cpp", mode = mode, encoding = "utf-8", newline = "\n" ) as out:
        #    out.write( functionLoadStart )
        #    out.write( "\n\nvoid VulkanLoadInstanceFunctions( VkInstance instance ) {\n" )
        #    out.write( Globals.functionLoadInstanceText )
        #    out.write( "}\n\n" )
        #    out.write( "void VulkanLoadDeviceFunctions( VkDevice device ) {\n" )
        #    out.write( Globals.functionLoadDeviceText )
        #    out.write( "}" )
        
        with open( "FunctionLoaderInstance.cpp", mode = mode, encoding = "utf-8", newline = "\n" ) as out:
            if define:
                out.write( "#if defined( " + define + " )\n" )
            
            out.write( functionLoadInstanceText )

            if define:
                out.write( "#endif\n\n" )
        
        with open( "FunctionLoaderDevice.cpp", mode = mode, encoding = "utf-8", newline = "\n" ) as out:
            if define:
                out.write( "#if defined( " + define + " )\n" )
            
            out.write( functionLoadDeviceText )

            if define:
                out.write( "#endif\n\n" )
        
        with open( "FeaturesConfig.h", mode = mode, encoding = "utf-8", newline = "\n") as outCfgH:
            with open( "FeaturesConfigGet", mode = mode, encoding = "utf-8", newline = "\n") as outCfgGet:
                with open( "FeaturesConfigCreate", mode = mode, encoding = "utf-8", newline = "\n") as outCfgCreate:
                    with open( "FeaturesConfigCreateDevice", mode = mode, encoding = "utf-8", newline = "\n") as outCfgCreateDevice:
                        with open( "FeaturesConfigMap", mode = mode, encoding = "utf-8", newline = "\n") as outCfgFeatureMap:
                            ProcessFeatures( outCfgH, outCfgGet, outCfgCreate, outCfgCreateDevice, outCfgFeatureMap )