#version 110

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;

uniform float multiplier;

void main()
{
	gl_FrontColor = gl_Color; 
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	vec4 newVert = gl_Vertex;
	newVert.z += texture2D(tex0, gl_TexCoord[0].xy).r * multiplier;
	gl_Position = gl_ModelViewProjectionMatrix * newVert;
}
