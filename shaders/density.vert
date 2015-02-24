
attribute vec3	vWorld;
varying vec3	pixel;

void main()
{
	pixel = vWorld;
	gl_Position = ftransform();
}

