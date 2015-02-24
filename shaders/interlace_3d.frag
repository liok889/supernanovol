
/* ----------------------------------------
 * Interlaced stereo shader
 * interlace_3d.frag
 * ----------------------------------------
 */

uniform sampler2D	left;
uniform sampler2D	right;

void main()
{
	int y = int(gl_FragCoord.y) % 2;
	if (y == 0)
	{
		// even, left eye
		gl_FragColor = texture2D(left,  gl_TexCoord[0].xy);
		//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);

	}
	else
	{
		// odd, right eye
		gl_FragColor = texture2D(right, gl_TexCoord[0].xy);
		//gl_FragColor = vec4(0.0, 1.0, 1.0, 1.0);

	}
}

