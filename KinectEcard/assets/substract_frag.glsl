#version 110

uniform sampler2D tex0;
uniform sampler2D tex1;

void main()
{
	vec4 colorVideo = texture2D(tex0, gl_TexCoord[0].xy);
	vec4 colorDepth = texture2D(tex1, gl_TexCoord[0].xy);
	vec4 color		= vec4(1,1,1,1);

	if( colorDepth.r < 1 )
		color = colorVideo;

	gl_FragColor = color;
}