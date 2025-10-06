struct embeddedFileMapEntry_t
{
	const unsigned char* data;
	size_t size;
};

using embeddedFileMap_t = std::unordered_map<std::string, const embeddedFileMapEntry_t>;
