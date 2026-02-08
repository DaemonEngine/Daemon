def init():
    global headerText
    global functionDefinitionsText

    global functionLoadInstanceText
    global functionLoadDeviceText
    
    headerText               = ""
    functionDefinitionsText  = ""

    functionLoadInstanceText = ""
    functionLoadDeviceText   = ""

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
