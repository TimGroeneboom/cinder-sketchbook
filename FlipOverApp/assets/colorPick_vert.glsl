#version 110

void main()
{
	gl_FrontColor = gl_Color; 
	
	gl_TexCoord[0].xy = (0.01 * gl_Vertex.xy) + 0.5;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
