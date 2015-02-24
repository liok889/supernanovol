    //uniform mat4 View;
    uniform mat4 Texm;
    //attribute vec4 Position;
    varying  vec2 oTexCoord;
    void main()
    {
       //gl_Position = ftransform();
       gl_Position = gl_ModelViewProjectionMatrix * vec4(1.0 * gl_Vertex.xyz, 1.0);
       
       oTexCoord = vec2(Texm * vec4(gl_MultiTexCoord0.xy,0,1));
       oTexCoord.x *= 2.0;
       //oTexCoord.y = 1.0-oTexCoord.y;
    }
