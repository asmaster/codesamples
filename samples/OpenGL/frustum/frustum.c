#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include <math.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>
#include <GL/glut.h>

/* == basic Q^3 vector math functions == */

void crossproduct(
	double ax, double ay, double az,
	double bx, double by, double bz,
	double *rx, double *ry, double *rz )
{
	*rx = ay*bz - az*by;
	*ry = az*bx - ax*bz;
	*rz = ax*by - ay*bx;
}

void crossproduct_v(
	double const * const a,
	double const * const b,
	double * const c )
{
	crossproduct(
		a[0], a[1], a[2],
		b[0], b[1], b[2],
		c, c+1, c+2 );
}

double scalarproduct(
	double ax, double ay, double az,
	double bx, double by, double bz )
{
	return ax*bx + ay*by + az*bz;
}

double scalarproduct_v(
	double const * const a,
	double const * const b )
{
	return scalarproduct(
		a[0], a[1], a[2],
		b[0], b[1], b[2] );
}

double length(
	double ax, double ay, double az )
{
	return sqrt(
		scalarproduct(
			ax, ay, az,
			ax, ay, az ) );
}

double length_v( double const * const a )
{
	return sqrt( scalarproduct_v(a, a) );
}

double normalize(
	double *x, double *y, double *z)
{
	double const k = 1./length(*x, *y, *z);

	*x *= k;
	*y *= k;
	*z *= k;
}

double normalize_v( double *a )
{
	double const k = 1./length_v(a);
	a[0] *= k;
	a[1] *= k;
	a[2] *= k;
}

/* == annotation drawing functions == */

void draw_strokestring(void *font, float const size, char const *string)
{
	glPushMatrix();
	float const scale = size * 0.01; /* GLUT character base size is 100 units */
	glScalef(scale, scale, scale);

	char const *c = string;
	for(; c && *c; c++) {
		glutStrokeCharacter(font, *c);
	}
	glPopMatrix();
}

void draw_arrow(
	float ax, float ay, float az,
	float bx, float by, float bz,
	float ah, float bh,
	char const * const annotation,
	float annot_size )
{
	int i;

	GLdouble mv[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, mv);
	
	/* We're assuming the modelview RS part is (isotropically scaled)
	 * orthonormal, so the inverse is the transpose.
	 * The local view direction vector is the 3rd column of the matrix;
	 * assuming the view direction to be the normal on the arrows tangent
	 * space  taking the cross product of this with the arrow direction
	 * yields the binormal to be used as the orthonormal base to the 
	 * arrow direction to be used for drawing the arrowheads */

	double d[3] = {
	      bx - ax,
	      by - ay,
	      bz - az
	};
	normalize_v(d);

	double n[3] = { mv[2], mv[6], mv[10] };
	{
		double const s = scalarproduct_v(d,n);
		for(int i = 0; i < 3; i++)
			n[i] -= d[i]*s;
	}
	normalize_v(n);

	double b[3];
	crossproduct_v(n, d, b);

	GLfloat const pos[][3] = {
		{ax, ay, az},
		{bx, by, bz},
		{ ax + (0.866*d[0] + 0.5*b[0])*ah,
		  ay + (0.866*d[1] + 0.5*b[1])*ah,
		  az + (0.866*d[2] + 0.5*b[2])*ah },
		{ ax + (0.866*d[0] - 0.5*b[0])*ah,
		  ay + (0.866*d[1] - 0.5*b[1])*ah,
		  az + (0.866*d[2] - 0.5*b[2])*ah },
		{ bx + (-0.866*d[0] + 0.5*b[0])*bh,
		  by + (-0.866*d[1] + 0.5*b[1])*bh,
		  bz + (-0.866*d[2] + 0.5*b[2])*bh },
		{ bx + (-0.866*d[0] - 0.5*b[0])*bh,
		  by + (-0.866*d[1] - 0.5*b[1])*bh,
		  bz + (-0.866*d[2] - 0.5*b[2])*bh }
	};
	GLushort const idx[][2] = {
		{0, 1},
		{0, 2}, {0, 3},
		{1, 4}, {1, 5}
	};
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, pos);

	glDrawElements(GL_LINES, 2*5, GL_UNSIGNED_SHORT, idx);
	glDisableClientState(GL_VERTEX_ARRAY);

	if(annotation) {
		float w = 0;
		for(char const *c = annotation; *c; c++)
			w += glutStrokeWidth(GLUT_STROKE_ROMAN, *c);
		w *= annot_size / 100.;

		float tx = (ax + bx - w*d[0])/2.;
		float ty = (ay + by - w*d[1])/2.;
		float tz = (az + bz - w*d[2])/2.;

		GLdouble r[16] = {
			d[0], d[1], d[2], 0,
			b[0], b[1], b[2], 0,
			n[0], n[1], n[2], 0,
			   0,    0,    0, 1
		};
		glPushMatrix();
		glTranslatef(tx, ty, tz);
		glMultMatrixd(r);
		glTranslatef(0, annot_size*0.1, 0);
		draw_strokestring(GLUT_STROKE_ROMAN, annot_size, annotation);
		glPopMatrix();
	}
}

void draw_frustum(
	float l, float r, float b, float t,
	float n, float f )
{
	GLfloat const kf = f/n;
	GLfloat const pos[][3] = {
		{0,0,0},
		{l, b, -n},
		{r, b, -n},
		{r, t, -n},
		{l, t, -n},
		{kf*l, kf*b, -f},
		{kf*r, kf*b, -f},
		{kf*r, kf*t, -f},
		{kf*l, kf*t, -f}
	};
	GLushort const idx_tip[][2] = {
		{0, 1},
		{0, 2},
		{0, 3},
		{0, 4}
	};
	GLushort const idx_vol[][2] = {
		{1, 5}, {2, 6},	{3, 7},	{4, 8},
		{1, 2},	{2, 3},	{3, 4},	{4, 1},
		{5, 6},	{6, 7},	{7, 8},	{8, 5}
	};

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, pos);

	glLineWidth(1);
	glLineStipple(2, 0xf3cf);
	glEnable(GL_LINE_STIPPLE);
	glDrawElements(GL_LINES, 2*4, GL_UNSIGNED_SHORT, idx_tip);

	glLineWidth(2);
	glLineStipple(1, 0xffff);
	glDisable(GL_LINE_STIPPLE);
	glDrawElements(GL_LINES, 2*4*3, GL_UNSIGNED_SHORT, idx_vol);

	glLineWidth(1);
	glDisableClientState(GL_VERTEX_ARRAY);
}

/* == scene drawing code == */

void display_observer(void)
{
	static float alpha = 0;

	int const win_width  = glutGet(GLUT_WINDOW_WIDTH);
	int const win_height = glutGet(GLUT_WINDOW_HEIGHT);
	float const win_aspect = (float)win_width / (float)win_height;

	glViewport(0, 0, win_width, win_height);
	glClearColor(1., 1., 1., 1.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#ifdef USE_ORTHO
	glOrtho(-10*win_aspect, 10*win_aspect, -10, 10, 0, 100);
#else
	gluPerspective(35, win_aspect, 1, 50);
#endif

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if(1) {
		glTranslatef(0, 0, -5);
		glRotatef(30, 1, 0, 0);
		glRotatef(alpha, 0, 1, 0);
		glTranslatef(0, 0, 2.5);
	} else {
		gluLookAt(3, 1, -5, 0, 0, -2.5, 0, 1, 0);
	}

	float const l = -0.5,
	            r =  0.5,
		    b = -0.5,
		    t =  0.5,
		    n =  1,
		    f =  4;

	glEnable(GL_MULTISAMPLE);

	glColor3f(0.,0.,0.);
	draw_frustum(l, r, b, t, n, f);

	glLineWidth(1);
	draw_arrow(0, 0, 0, 0, 0, -n, 0.1, 0.1, "near", 0.075);

	draw_arrow(l, 0, -n, 0, 0, -n, 0.1, 0, "left", 0.075);
	draw_arrow(0, 0, -n, r, 0, -n, 0, 0.1, "right", 0.075);
	draw_arrow(0, b, -n, 0, 0, -n, 0.1, 0, "bottom", 0.075);
	draw_arrow(0, 0, -n, 0, t, -n, 0, 0.1, "top", 0.075);

	glutSwapBuffers();

	alpha = fmodf(alpha + 0.1, 360);
	glutPostRedisplay();
}

void display_frustum_view(void)
{
	int const win_width  = glutGet(GLUT_WINDOW_WIDTH);
	int const win_height = glutGet(GLUT_WINDOW_HEIGHT);
	float const win_aspect = (float)win_width / (float)win_height;

	glViewport(0, 0, win_width, win_height);
	glClearColor(0.3, 0.3, 0.6, 1.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glutSwapBuffers();
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_MULTISAMPLE);

	glutCreateWindow("Observer");
	glutDisplayFunc(display_observer);

	glutCreateWindow("Frustum View");
	glutDisplayFunc(display_frustum_view);

	glutMainLoop();
	return 0;
}
