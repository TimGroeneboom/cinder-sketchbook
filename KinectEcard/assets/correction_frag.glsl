#version 110

uniform sampler2D tex0;
uniform sampler2D tex1;

void main()
{
	vec4 colorDisplace	= texture2D(tex1, gl_TexCoord[0].xy);
	vec4 color			= texture2D(tex0, vec2( colorDisplace.r, colorDisplace.g ) );

	gl_FragColor = color;
}