void convertFromSRGB(inout vec3 color) {
//	float gamma = 2.2;
//	return pow(color, vec3(gamma));

// sRGB to Linear formula:
//	(((c) <= 0.04045f) ? (c) * (1.0f / 12.92f) : (float)pow(((c) + 0.055f)*(1.0f/1.055f), 2.4f))

#if 0
	color.r = color.r <= 0.04045f ? color.r * (1.0f / 12.92f) : pow((color.r + 0.055f) * (1.0f / 1.055f), 2.4f);
	color.g = color.g <= 0.04045f ? color.g * (1.0f / 12.92f) : pow((color.g + 0.055f) * (1.0f / 1.055f), 2.4f);
	color.b = color.b <= 0.04045f ? color.b * (1.0f / 12.92f) : pow((color.b + 0.055f) * (1.0f / 1.055f), 2.4f);
#elif 0
	float threshold = 0.04045f;

	vec3 yes = vec3(float(color.r <= threshold), float(color.g <= threshold), float(color.b <= threshold));
	vec3 no = yes * -1.0f + 1.0f;

	vec3 low = color * (1.0f / 12.92f);
	vec3 high = pow((color + 0.055f) * (1.0f / 1.055f), vec3(2.4f));

	return yes * low + no * high;
#else
	bvec3 cutoff = lessThan(color, vec3(0.04045f));
	vec3 low = color / vec3(12.92f);
	vec3 high = pow((color + vec3(0.055f)) / vec3(1.055f), vec3(2.4f));

	color = mix(high, low, cutoff);
#endif
}

void convertToSRGB(inout vec3 color) {
//	float gamma = 2.2;
//	return pow(color, vec3(1/gamma));

// Linear to sRGB formula:
//	(((c) < 0.0031308f) ? (c) * 12.92f : 1.055f * (float)pow((c), 1.0f/2.4f) - 0.055f)

#if 0
	color.r = color.r < 0.0031308f ? color.r * 12.92f : 1.055f * pow(color.r, 1.0f / 2.4f) - 0.055f;
	color.g = color.g < 0.0031308f ? color.g * 12.92f : 1.055f * pow(color.g, 1.0f / 2.4f) - 0.055f;
	color.b = color.b < 0.0031308f ? color.b * 12.92f : 1.055f * pow(color.b, 1.0f / 2.4f) - 0.055f;
	return color;
#elif 0
	float threshold = 0.0031308f;

	vec3 yes = vec3(float(color.r < threshold), float(color.g < threshold), float(color.b < threshold));
	vec3 no = yes * -1.0f + 1.0f;

	vec3 low = color * 12.92f;
	vec3 high = pow(color, vec3( 1.0f / 2.4f)) - 0.055f;

	return yes * low + no * high;
#else
	bvec3 cutoff = lessThan(color, vec3(0.0031308f));
	vec3 low = vec3(12.92f) * color;
	vec3 high = vec3(1.055f) * pow(color, vec3(1.0f / 2.4f)) - vec3(0.055f);

	color = mix(high, low, cutoff);
#endif
}
