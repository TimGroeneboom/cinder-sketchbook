#version 110

void main()
{
	vec4 color		= gl_Color;
	color.gb		= gl_TexCoord[0].xy;

	gl_FragColor	= color;
}