#version 110

uniform sampler2D tex0;
uniform sampler2D tex1;

void main()
{
	vec4 colorDepth			= texture2D(tex0, gl_TexCoord[0].xy);
	vec4 colorStoredDepth	= texture2D(tex1, gl_TexCoord[0].xy);

	vec4 color = colorStoredDepth;

	if( colorDepth.r < colorStoredDepth.r ){
		color = colorDepth;
	}

	gl_FragColor = color;
}