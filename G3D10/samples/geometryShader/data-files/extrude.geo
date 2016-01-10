#version 330 // -*- c++ -*-

layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

/**

 Extrudes each triangle into a triangular prism.  This is the geometry needed for several algorithms,
 including shadow volumes, fin-and-shell rendering, and various kinds of advanced parallax mapping.
 The prism is shrunk slightly so that adjacent prisms are visible; this would not be done in typical
 rendering.

 There are multiple OpenGL geometry shader specifications.  This shader is written against GLSL 1.20
 with the geometry shader extension, which supports a wide variety of GPUs.
 */

uniform mat4 MVP;

// Example of a geometry shader custom input
in vec3 normal[3];

// Example of a geometry shader custom output
out vec3 faceColor;

void main() {
    
    vec3 color = normalize(cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, 
                                 gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz)) * 0.5 + vec3(0.5);

    /*
    // Simple surface expansion and per-face coloring example
	for (int i = 0; i < 3; ++i){
		gl_Position = MVP * (gl_in[i].gl_Position + vec4(normal[i] * 0.1, 0.0));
        faceColor = color;
		EmitVertex();
	}
	EndPrimitive();
    */
  
    // A triangular prism can be created as a single triangle strip containing 12 vertices and 10 triangles,
    // two of which are degenerate, or as two triangle strips containing 12 vertices and 8 triangles total.
    // We use the second method.
    //
    // Input vertices:
    //                            2
    //                         __*
    //                   ___--- /
    //             ___---      / 
    //            *-----------* 
    //           0            1
    //
    // Output vertices:
    //                             2, 6
    //                         __*
    //                   ___--- /|
    //             ___---  1,10/ |
    //       0, 8 *-----------*__* 4, 7
    //            |      ___--| /
    //            |___---     |/
    //            *-----------*
    //          5, 9           3, 11
    //
    // Output strips:
    //
    //                   2     4
    //                   *-----*              6   8  10   
    //                 / |\    | \             *--*--*
    //               /   | \   |   \           | /| /|
    //           0 *     |  \  |    * 5        |/ |/ |
    //               \   |   \ |   /           *--*--*
    //                 \ |    \| /            7   9  11
    //                   *-----*
    //                   1     3
    //

    // Extrusion distance
    const float d = 0.2;

    // Shrink fraction
    const float shrink = 0.6;

    vec3 v[3];
    vec3 center = (gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz) / 3.0; 
    for (int i = 0; i < 3; ++i) {
        v[i] = mix(gl_in[i].gl_Position.xyz, center, shrink);
    }

    // First strip
    {
        //  Top triangle
	    gl_Position = MVP * vec4(normal[0] * d + v[0], 1.0);    faceColor = color;	EmitVertex();
	    gl_Position = MVP * vec4(normal[1] * d + v[1], 1.0);    faceColor = color;	EmitVertex();
	    gl_Position = MVP * vec4(normal[2] * d + v[2], 1.0);    faceColor = color;	EmitVertex();

        //  Right side quad
	    gl_Position = MVP * vec4(v[1], 1.0);                    faceColor = color;	EmitVertex();
	    gl_Position = MVP * vec4(v[2], 1.0);                    faceColor = color;	EmitVertex();

        //  Bottom triangle
	    gl_Position = MVP * vec4(v[0], 1.0);                    faceColor = color;	EmitVertex();
    }
	EndPrimitive();

    // Second strip
    {
        //  Back-left quad
	    gl_Position = MVP * vec4(normal[2] * d + v[2], 1.0);    faceColor = color;	EmitVertex();
	    gl_Position = MVP * vec4(v[2], 1.0);                    faceColor = color;	EmitVertex();
	    gl_Position = MVP * vec4(normal[0] * d + v[0], 1.0);    faceColor = color;	EmitVertex();
	    gl_Position = MVP * vec4(v[0], 1.0);                    faceColor = color;	EmitVertex();

        //  Front quad
	    gl_Position = MVP * vec4(normal[1] * d + v[1], 1.0);    faceColor = color;	EmitVertex();
	    gl_Position = MVP * vec4(v[1], 1.0);                    faceColor = color;	EmitVertex();
    }
	EndPrimitive();
}
