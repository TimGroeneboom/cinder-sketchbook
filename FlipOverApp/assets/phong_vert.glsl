#version 110

varying vec3 V;
varying vec3 N;

uniform vec2 scale;
uniform vec2 offset;
uniform vec2 size;

void main()
{
	// pass the vertex position to the fragment shader
	V = vec3(gl_ModelViewMatrix * gl_Vertex);

	vec2 V2 = vec2( gl_Vertex.x, gl_Vertex.y ) * size;	// size = 1.0f / objectSize
	V2		+= vec2( 0.5, 0.5 );						// correct vertex coordinates to match texture coordinates
	V2.x	= 1.0 - V2.x;								// invert x for some reason ?
	V2		*= scale;									// scale
	V2		+= offset;									// offset

	// pass the normal to the fragment shader      
	N = normalize(gl_NormalMatrix * gl_Normal);

	gl_TexCoord[0].xy = V2;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
