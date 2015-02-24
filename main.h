/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * main.h
 *
 * -----------------------------------------------
 */

#ifndef _SUPER_NANOVOL_MAIN_H__
#define _SUPER_NANOVOL_MAIN_H__

#include <string>
#include "VectorT.hxx"

using namespace std;

class TransferFunction;
class Camera;

void _bgColor(float r, float g, float b);

void _colorScale(float f);
void _dT(float f);

void _skipEmpty(bool b);
void _drawVolume(bool b);
void _drawBalls(bool b);

void _clipBox(bool b);
void _clipBoxMin(vec3);
void _clipBoxMax(vec3);

int getWinWidth();
int getWinHeight();

TransferFunction * getCurrentTF();
void compileShaders();
string getShadersDir();
void balls_ray_cast(Camera * cam, const vec2 * win_coordinates);


#endif

