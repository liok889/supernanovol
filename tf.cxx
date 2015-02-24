/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * tf.cxx
 *
 * -----------------------------------------------
 */
#include <iostream>
#include <cfloat>
#include <math.h>
#include <assert.h>
#include "tf.h"
#include "graphics/color.h"

using namespace std;

const int DEFAULT_TF_WIDTH	= 200;
const int DEFAULT_TF_HEIGHT	= 100;
const float TF_ALPHA		= 0.7;		// alpha of the TF widget
const float MIN_MAX_ALPHA	= 1.2;		// minimum alpha height of the TF

const int TF_TEX_WIDTH		= 1024;	// width of TF texture (when uploaded to GLSL)
const int TF_TEX_HEIGHT	= 1;		// height of TF texture (when uploaded to GLSL)

// defined in main.cxx
extern int getWinWidth();
extern int getWinHeight();

TransferFunction::TransferFunction(string filename, bool _loadToGPU)
:ScalableWidget(20, 20, DEFAULT_TF_WIDTH, DEFAULT_TF_HEIGHT), tfFB(TF_TEX_WIDTH, 1)
{
	maxAlpha = MIN_MAX_ALPHA;
	hasGPUData = false;
	loadToGPU = _loadToGPU;
	texInited = false;
	pickedPoint = -1;
	callback = NULL;
	interpTarget = NULL;
	interpAlpha = 0.0f;
	
	
	if (filename.length() == 0 || !load(filename))
	{
		// make default transfer function
		makeDefault();
	}
}

TransferFunction::TransferFunction(const TransferFunction & rhs)
:ScalableWidget(20, 20, DEFAULT_TF_WIDTH, DEFAULT_TF_HEIGHT), tfFB(TF_TEX_WIDTH, 1)
{
	maxAlpha = MIN_MAX_ALPHA;
	hasGPUData = false;
	loadToGPU = false;
	texInited = false;
	pickedPoint = -1;
	callback = NULL;
	interpTarget = NULL;
	interpAlpha = 0.0f;

	this->points = rhs.points;	
}

TransferFunction::~TransferFunction()
{
	if (hasGPUData) {
		glDeleteTextures(1, &tfTex);
		hasGPUData = false;
	}
}

bool TransferFunction::testPointPick(int i, int mouseX, int mouseY)
{
	const TFPoint & p = points[i];
				
	int xx = floor(float(x) + float(w) * p.value                + .5f);
	int yy = floor(float(y) + float(h) * p.color.a() / maxAlpha + .5f);
					
	if ( abs(xx-mouseX) < PICK_DIST && abs(yy-mouseY) < PICK_DIST )
	{
		picked = WIDGET_POINT;
		pickedPoint = i;
		return true;
	}
	else
	{
		return false;
	}
}


bool TransferFunction::pointerPick(int mouseX, int mouseY)
{
	if (ScalableWidget::pointerPick(mouseX, mouseY) && picked != WIDGET_INSIDE)
	{
		// special case to deal with picking the first and last points, which are often annoying to pick
		if (points.size() > 0) 
		{
			if (picked == WIDGET_BL && mouseX > x && testPointPick(0, mouseX, mouseY));
			else if (picked == WIDGET_BR && mouseX < x+w && testPointPick(points.size()-1, mouseX, mouseY));			
		}
		return true;
	}
	else if (inWidget(mouseX, mouseY));
	{
		pickedPoint = -1;

		// check all points
		for (int i = 0; i < points.size(); i++)
		{
			if (testPointPick(i, mouseX, mouseY)) {
				break;
			}
		}
	}
	
	return (picked != WIDGET_NOTHING);
}

void TransferFunction::pointerUnpick()
{
	ScalableWidget::pointerUnpick();
}

void TransferFunction::pointerMove(int mouseX, int mouseY, int dX, int dY)
{
	if (picked == WIDGET_POINT)
	{
		// map the current point
		TFPoint & tP = points[pickedPoint];
		vec2 p = mapPoint(tP.value, tP.color.a());
		
		// see our range
		int minX = floor(.5f+ (pickedPoint == 0 ? x+1 : mapPoint(points[pickedPoint-1].value, 0.0).x() + 0.0));
		int maxX = floor(.5f +(pickedPoint == points.size()-1 ? x+w-1 : mapPoint(points[pickedPoint+1].value, 0.0).x() - 4.0));
		
		 // assign the X
		 int newX = min(maxX, max(minX, mouseX));
		 int newY = max(mouseY, y);
		 
		 float newValue =  float(newX - x) / float(w);
		 float newAlpha = (float(newY - y) / float(h)) * maxAlpha;
		 
		 // find max new alpha
		 float m = FLT_MIN; for (int i = 0; i < points.size(); i++) m = max(m, points[i].color.a());
		 float newMaxAlpha = max(newAlpha, m);
		 
		 if ( fabs(newMaxAlpha - maxAlpha) != 0.2 )
		 {
		 	 maxAlpha = newMaxAlpha + 0.2;
		 }
		 
		 maxAlpha = max(MIN_MAX_ALPHA, maxAlpha);
		 
		 tP.value = newValue;
		 tP.color.a() = newAlpha;
		 
		 // update
		 upload_GLSL();
	}
	else
	{
		ScalableWidget::pointerMove(mouseX, mouseY, dX, dY);
	}
}

vec2 TransferFunction::mapPoint(float u, float v)
{
	return vec2(
		float(x) + float(w) * u,
		float(y) + float(h) * v / maxAlpha
	);
}

void TransferFunction::rightPick(int pX, int pY)
{
	if (!inWidget(pX, pY)) {
		return;
	}
	
	bool deletion = false;
	
	// try to remove any close points
	vector<TFPoint>::iterator it = points.begin();
	for (int j = 0; it != points.end(); it++, j++)
	{
		vec2 p = mapPoint(it->value, it->color.a());
		vec2i iP = vec2i(floor(p.x()+.5f), floor(p.y()+.5f));
		
		if ( abs(iP.x() - pX) < PICK_DIST && abs(iP.y() - pY) < PICK_DIST )
		{
			// remove this point
			points.erase(it);
			
			if (pickedPoint == j) {
				pickedPoint = -1;
			}
			
			// resacle
			float m = FLT_MIN;
			for (int i = 0; i < points.size(); i++) m = max(m, points[i].color.a());
			if ( fabs(maxAlpha - m) != 0.2 ) {
				maxAlpha = m + 0.2;
			}
			if (maxAlpha < MIN_MAX_ALPHA) {
				maxAlpha = MIN_MAX_ALPHA;
			}

			deletion = true;
			break;
		}
	}
	
	if (!deletion) {
		// add point instead
		addPoint(pX, pY);
	}
	
	// update
	upload_GLSL();
}

void TransferFunction::addPoint(int pX, int pY)
{
	float v = float(pX - x) / float(w);
	
	// determine position
	int pos = 0; vector<TFPoint>::iterator it = points.begin();
	for (; pos < points.size(); pos++, it++)
	{
		if (it->value > v)
		{
			break;
		}
	}
		
	TFPoint p1(0.0, Color(.1f, .1f, .1f, .2f));
	TFPoint p2(1.0, Color(.9f, .9f, .9f, .2f));
	
	if (pos>0) {
		p1 = points[pos-1];
	}
	if (pos < points.size()) {
		p2 = points[pos];
	}
	
	TFPoint newPoint(v, interpolate((v - p1.value) / (p2.value - p1.value), p1.color, p2.color));
	newPoint.color.a() = (float(pY - y) / float(h)) * maxAlpha;
	if ( maxAlpha - newPoint.color.a() < 0.2 ) {
		maxAlpha = newPoint.color.a() + 0.2;
	}
	
	points.insert(it, newPoint);
	
	// update
	upload_GLSL();
}

void TransferFunction::draw()
{
	// setup projection
	glPushAttrib(GL_TRANSFORM_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT );
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, getWinWidth()-1, 0, getWinHeight()-1, -1.0, 1.0);
		
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		// enables
		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);		
		glLineWidth(2.0);
		
		// draw grey quad
		glColor4f(.4, .4, .4, TF_ALPHA);
		glBegin(GL_QUADS);
		{
			glVertex2f(x,   y);
			glVertex2f(x+w, y);
			glVertex2f(x+w, y+h);
			glVertex2f(x,   y+h);		
		}
		glEnd();
		
		// draw borders
		glColor3f(0.0, 0.0, 0.0);
		glBegin(GL_LINE_STRIP);
		{
			glVertex2f(x,   y);
			glVertex2f(x+w, y);
			glVertex2f(x+w, y+h);
			glVertex2f(x,   y+h);
			glVertex2f(x,   y);
		}
		glEnd();
		
		// draw scale
		glLineWidth(1.0);
		glColor4fv( Color(0xFF8688, 64).v() );
		if (1.0 < maxAlpha) {
			glBegin(GL_LINES);
				glVertex2f(x+2, (1.0 / maxAlpha) * float(h) + y);
				glVertex2f(x+w-2, (1.0 / maxAlpha) * float(h) + y);
			glEnd();
		}
		
		if (5.0 < maxAlpha) {
			glLineWidth(2.0);
			glBegin(GL_LINES);
				glVertex2f(x+2, (5.0 / maxAlpha) * float(h) + y);
				glVertex2f(x+w-2, (5.0 / maxAlpha) * float(h) + y);
			glEnd();
		}

		if (10.0 < maxAlpha) {
			glLineWidth(3.0);
			glBegin(GL_LINES);
				glVertex2f(x+2, (10.0 / maxAlpha) * float(h) + y);
				glVertex2f(x+w-2, (10.0 / maxAlpha) * float(h) + y);
			glEnd();
		}
		
		
		// draw QUADS
		if (points.size() >= 2)
		{
			glBegin(GL_QUAD_STRIP);
			for (int i = 0; i < points.size(); i++)
			{
				const TFPoint & p = points[i];
				vec2 pp = mapPoint(p.value, p.color.a());
					
				glColor4f(p.color.r(), p.color.g(), p.color.b(), TF_ALPHA);
				glVertex2f(pp.x(), pp.y());
				glVertex2f(pp.x(), y);
			}
			glEnd();
		}

		// draw lines		
		if (points.size() >= 2)
		{
			glLineWidth(2.0);
			glBegin(GL_LINE_STRIP);
			for (int i = 0; i < points.size(); i++)
			{
				const TFPoint & p = points[i];
				vec2 pp = mapPoint(p.value, p.color.a());

				glColor4f(p.color.r(), p.color.g(), p.color.b(), TF_ALPHA);
				glVertex2f(pp.x(), pp.y());
			}
			glEnd();
		}
		
		// draw points
		glPointSize(3.0);
		glEnable(GL_POINT_SMOOTH);
		//glBegin(GL_POINTS);
		for (int i = 0; i < points.size(); i++) 
		{
			if (i == pickedPoint)
			{
				glPointSize(8.0);
			}
			
			glBegin(GL_POINTS);
			const TFPoint & p = points[i];
			vec2 pp = mapPoint(p.value, p.color.a());
			glColor4f(p.color.r(), p.color.g(), p.color.b(), 1.0f);
			glVertex2f(pp.x(), pp.y());
			glEnd();
			if (i == pickedPoint)
			{
				glPointSize(3.0);
			}
		}
		//glEnd();
		
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
	glPopAttrib();
}

void TransferFunction::makeDefault()
{
	points.push_back(TFPoint(0.0, 0x7C9FFF, 0.0));
	points.push_back(TFPoint(0.2, 0x122798, 0.2));
	points.push_back(TFPoint(0.4, 0x6ABC13, 0.4));
	points.push_back(TFPoint(0.7, 0xFF4649, 1.0));
	points.push_back(TFPoint(0.8, 0xFF4649, 0.2));
	points.push_back(TFPoint(0.9, 0xcccccc, 0.0));
	
	upload_GLSL();
}


void TransferFunction::setInterpTarget(const TransferFunction * target)
{
	interpTarget = target;
	interpAlpha = 0.0;
}

void TransferFunction::tick(float dT)
{	
	static const float TF_INTERP_TIME = 2.7f;
	if (interpTarget == NULL) {
		return;
	}
	
	
	interpAlpha += dT / TF_INTERP_TIME;
	if (interpAlpha >= 1.0f) 
	{
		// interpolation done
		interpAlpha = 1.0f;
		this->points = interpTarget->points;
		interpTarget = NULL;	
	}
	
	// upload without callback
	this->upload_GLSL(false);
}

void TransferFunction::upload_GLSL(bool enableCallback)
{
	if (loadToGPU) 
	{
		tfFB.bind();
		
		glPushAttrib( GL_ALL_ATTRIB_BITS );
		{
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
			
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			
			glEnable(GL_BLEND);
			glViewport(0, 0, tfFB.getWidth(), tfFB.getHeight());
			
			// clear
			glClear(GL_COLOR_BUFFER_BIT);
			
			// draw TF as quad strip			
			int passes = interpTarget != NULL ? 2 : 1;
			for (int pass = 0; pass < passes; pass++)
			{
				glBegin(GL_QUAD_STRIP);
				float alphaMod = interpTarget == NULL ? 1.0f : (pass == 0 ? 1.0f-interpAlpha : interpAlpha);
				const vector<TFPoint> & thePoints = pass == 0 ? points : interpTarget->points;
				
				for (int i = 0; i < thePoints.size(); i++)
				{
					const TFPoint & p = thePoints[i];
										
					glColor4f(p.color.r(), p.color.g(), p.color.b(), p.color.a() * alphaMod);
					glVertex2f(p.value, 0.0);
					glVertex2f(p.value, 1.0);
				}
				glEnd();
			}

		
		}
		glPopAttrib();
		glFinish();
		
		tfFB.unbind();
	}
	
	if (callback && enableCallback) {
		callback();
	}
}

bool TransferFunction::save(string filename)
{
	ofstream output(filename.c_str());
	if (!output.is_open()) 
	{
		return false;
	}
	else
	{
		for (int i = 0; i < points.size(); i++)
		{
			const TFPoint & p = points[i];
			output << 
				p.color.r() << " " << p.color.g() << " " << 
				p.color.b() << " " << p.color.a() << " " << p.value << 
				(i < points.size()-1 ? "\n" : "");
		}
		output.close();
		return true;
	}
}

bool TransferFunction::load(string filename)
{
	ifstream input(filename.c_str());
	if (!input.is_open())
	{
		return false;
	}
	else
	{
		vector<TFPoint> newTF;
		while (!input.eof())
		{
			
			float r, g, b, a, value;
			
			input >> r;
			input >> g;
			input >> b;
			input >> a;
			input >> value;
			
			TFPoint p(value, Color(r, g, b), a);
			newTF.push_back(p);
		}
		input.close();
		
		if (newTF.size() > 0) {
			points = newTF;
		}
		upload_GLSL();
		return true;
	}
}


void TransferFunction::marshal(float * data, int & count)
{
	int originalCount = count;
	count = 5*points.size() + 1;
	
	assert(originalCount >= count);
	
	*data = float(points.size());
	data++;
	for (int i = 0; i < points.size(); i++, data += 5)
	{
		const TFPoint & p = points[i];
		data[0] = p.color.r();
		data[1] = p.color.g();
		data[2] = p.color.b();
		data[3] = p.color.a();
		data[4] = p.value;
	}
	
}

void TransferFunction::unmarshal(const float * data, int count)
{
	int actualCount = int(*data);
	assert(count == 0 || count == actualCount*5+1);

	data++;
	vector<TFPoint> newTF;
	for (int i = 0; i < actualCount; i++, data += 5)
	{
		float r = data[0];
		float g = data[1];
		float b = data[2];
		float a = data[3];
		float v = data[4];
		
		TFPoint p(v, Color(r, g, b), a);
		newTF.push_back(p);
	}
	points = newTF;
	upload_GLSL();
}

void TransferFunction::updatePointColor( Color c )
{
	if (pickedPoint >= 0)
	{
		TFPoint & p = points[pickedPoint];
		p.color.r() = c.r();
		p.color.g() = c.g();
		p.color.b() = c.b();
		upload_GLSL();
	}
}

Color interpolate(float a, Color c1, Color c2)
{
	return Color(
		float((1.0-a) * c1.r() + a * c2.r()),
		float((1.0-a) * c1.g() + a * c2.g()),
		float((1.0-a) * c1.b() + a * c2.b()),
		float((1.0-a) * c1.a() + a * c2.a())
	);
}


