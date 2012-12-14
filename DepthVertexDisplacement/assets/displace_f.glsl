#version 110

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;

void main()
{
	vec2 depthCorrect	= texture2D(tex2, gl_TexCoord[0].xy).xy;

	gl_FragColor		= gl_Color * texture2D(tex1, gl_TexCoord[0].xy);
}