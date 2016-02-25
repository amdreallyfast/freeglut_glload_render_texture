#version 440

// must have the same name as its corresponding "out" item in the vert shader
smooth in vec3 vertOutColor;
smooth in vec2 texPos;
uniform sampler2D tex;

void main()
{
    // set "vert out color" multiplier to 0.0f to let color data come from texture, or 
    // 1.0f to let it come from the color that was indexed with the vertex
    vec4 netColor = vec4((vertOutColor * 0.0f) + texture2D(tex, texPos).rgb, 1.0f);

    // fun fact: if I set gl_FragColor to the values above, then I get a "gl_FragColor is
    // deprecated" shader compiler error, but if I set the values to an intermediate vec4 and
    // then assign them to gl_FragColor, then the error goes away (??why??)
    gl_FragColor = netColor;
}
