#version 110

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;

void main()
{
	vec4 colorVideo = texture2D(tex0, gl_TexCoord[0].xy);
	vec4 colorDepth = texture2D(tex1, gl_TexCoord[0].xy);
	vec4 storedColorVideo = texture2D(tex2, gl_TexCoord[0].xy);
	vec4 storedColorDepth = texture2D(tex3, gl_TexCoord[0].xy);

	vec4 color = storedColorVideo;

	if( colorDepth.r < storedColorDepth.r ){
		color = colorVideo;
	}

	gl_FragColor = color;
}