white
{
	cull none
	{
		map $white
		blend blend
		rgbgen vertex
	}
}

// console font fallback
gfx/2d/bigchars
{
	nopicmip
	nomipmaps
	{
		map gfx/2d/bigchars
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}
