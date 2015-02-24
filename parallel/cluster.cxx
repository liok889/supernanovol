/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Aaron Knoll, 2011-2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * cluster.cxx
 * 
 * -----------------------------------------------
 */

#include <mpi.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include "cluster.h"
#include "rendernode.h"
#include "../graphics/misc.h"

using namespace std;

string CONF_FILE = "conf/cluster.conf";
void setConfFile(string _conf) { CONF_FILE = _conf; }

ClusterConfig clusterConfig;
const ClusterConfig & getClusterConfig() { return clusterConfig; }

void parseClusterConfigFile()
{
	char buffer[1024*2];
	
	ifstream input;
	input.open(CONF_FILE.c_str());
	if (!input.is_open())
	{
		cerr << "Could not open cluster config file " << CONF_FILE << "\n";
		exit(1);
	}
	
	// node information
	Node * newNode = NULL;
	RenderWindow * newWindow = NULL;
	RenderSurface * newSurface = NULL;
	
	bool topLeftSet = false;
	bool bottomRightSet = false;
	bool sizeSet = false;
	bool offsetSet = false;

	// boolean flags
	bool pixelBottomLeftSet = false;
	bool worldBottomLeftSet = false;			
	bool pixelTopRightSet = false;
	bool worldTopRightSet = false;
				
	for (int lineNum = 1; !input.eof(); lineNum++)
	{
		input.getline(buffer, 1024*2);
		string newLine = trim( buffer );
		if (newLine.length() == 0 || newLine[0] == '#' || newLine[0] == '\r') {
			continue;
		}
		
		if ( strcasecmp(newLine.c_str(), "node") == 0)
		{
			if (newNode != NULL )
			{
				if ( newNode->hostname.length() != 0 && newNode->windows.size() > 0)
				{
					// data of last node complete, add					
					clusterConfig.nodeMap[ newNode->hostname ].push_back(*newNode);
					clusterConfig.nodeList.push_back( *newNode );
					delete newNode;
					newNode = NULL;
					topLeftSet = bottomRightSet = false;
				}
				else
				{
					// parse error, missing data
					cerr << "Error parsing " << CONF_FILE << " at line " << lineNum << endl;
					cerr << "New 'Node' encountered but last one not complete." << endl;
					exit(1);
				}
			}
			
			newNode = new Node;
		}
		else if (newNode == NULL)
		{
			cerr << "Error parsing cluster config file at line " << lineNum << endl;
			cerr << "Expecting keyword 'Node' to indicate beginning of a node property list." << endl;
			exit(1);
		}
		else
		{
			if ( strcasecmp(newLine.c_str(), "window") == 0)
			{
				if (newWindow != NULL)
				{
					// parse error, missing data
					cerr << "Error parsing " << CONF_FILE << " at line " << lineNum << endl;
					cerr << "New 'Window' encountered but last one not complete." << endl;
					exit(1);
				}
				else
				{
					newWindow = new RenderWindow;
				}
				
			}
			else if (strcasecmp(newLine.c_str(), "endWindow") == 0)
			{
				if (newWindow == NULL || newWindow->surfaces.size() == 0 || !offsetSet || !sizeSet || newSurface != NULL)
				{
					// parse error, missing data
					cerr << "Error parsing " << CONF_FILE << " at line " << lineNum << endl;
					cerr << "'EndWindow' encountered but no window specified, or window info not complete." << endl;
					exit(1);
				}
				newNode->windows.push_back( *newWindow );
				delete newWindow; newWindow = NULL;
				offsetSet = sizeSet = false;
			}
			else if (strcasecmp(newLine.c_str(), "surface") == 0)
			{
				if (newSurface != NULL)
				{
					// parse error, surface not yet complete
					cerr << "Error parsing " << CONF_FILE << " at line " << lineNum << endl;
					cerr << "'Surface' encountered but last Surface info not complete." << endl;
					exit(1);					
				}
				
				// clear parameters
				pixelBottomLeftSet = false;
				worldBottomLeftSet = false;
				
				pixelTopRightSet = false;
				worldTopRightSet = false;
				
				newSurface = new RenderSurface;
			}
			
			else
			{
			
				vector<string> tokens = splitWhite( newLine );
				if (tokens.size() < 2)
				{
					cerr << "Error parsing cluster config file at line " << lineNum << endl;
					cerr << "Expecting 'proprty value'" << endl;
					exit(1);
				}
				
				string key = tokens[0];
				if (strcasecmp(key.c_str(), "hostname") == 0)
				{
					newNode->hostname = tokens[1];
				}
				else
				{
					if (newWindow == NULL)
					{
						cerr << "Error parsing cluster config file at line " << lineNum << endl;
						cerr << "Expecting 'Window'" << endl;
						exit(1);
					}
					
					if (strcasecmp(key.c_str(), "fullscreen") == 0)
					{
						if (strcasecmp(tokens[1].c_str(), "1") == 0 || strcasecmp(tokens[1].c_str(), "true") == 0)
						{
							newWindow->fullscreen = true;
						}
					}
					else if (strcasecmp(key.c_str(), "Xserver") == 0)
					{
						newWindow->Xserver = tokens[1];
					}
					
					else if (strcasecmp(key.c_str(), "size") == 0)
					{
						if (tokens.size() < 3)
						{
							cerr << "Error parsing cluster config file at line " << lineNum << endl;
							cerr << "Expecting size w h" << endl;
							exit(1);
						}
						from_string( newWindow->width, tokens[1] );
						from_string( newWindow->height, tokens[2] );
						sizeSet = true;
							
					}
					else if (strcasecmp(key.c_str(), "offset") == 0)
					{
						if (tokens.size() < 3)
						{
							cerr << "Error parsing cluster config file at line " << lineNum << endl;
							cerr << "Expecting offset w h" << endl;
							exit(1);
						}
						from_string( newWindow->x, tokens[1] );
						from_string( newWindow->y, tokens[2] );
						offsetSet = true;							
					}
					
					else
					{
						if (newSurface == NULL)
						{
							cerr << "Error parsing cluster config file at line " << lineNum << endl;
							cerr << "Expecting 'Surface'" << endl;
							exit(1);
						}
						
						if (strcasecmp(key.c_str(), "world_bottomLeft") == 0)
						{
							if (tokens.size() < 4) 
							{
								cerr << "Error parsing cluster config file at line " << lineNum << endl;
								cerr << "Expecting world_bottomLeft x y z" << endl;
								exit(1);
							}
							
							from_string( newSurface->w_bottomLeft.x(), tokens[1] );
							from_string( newSurface->w_bottomLeft.y(), tokens[2] );
							from_string( newSurface->w_bottomLeft.z(), tokens[3] );
							worldBottomLeftSet = true;
						}
						else if (strcasecmp(key.c_str(), "world_topRight") == 0)
						{
							if (tokens.size() < 4) 
							{
								cerr << "Error parsing cluster config file at line " << lineNum << endl;
								cerr << "Expecting world_topRight x y z" << endl;
								exit(1);
							}
							
							from_string( newSurface->w_topRight.x(), tokens[1] );
							from_string( newSurface->w_topRight.y(), tokens[2] );
							from_string( newSurface->w_topRight.z(), tokens[3] );
							worldTopRightSet = true;
						}
						else if (strcasecmp(key.c_str(), "world_topLeft") == 0)
						{
							if (tokens.size() < 4) 
							{
								cerr << "Error parsing cluster config file at line " << lineNum << endl;
								cerr << "Expecting world_topLeft x y z" << endl;
								exit(1);
							}
							
							from_string( newSurface->w_topLeft.x(), tokens[1] );
							from_string( newSurface->w_topLeft.y(), tokens[2] );
							from_string( newSurface->w_topLeft.z(), tokens[3] );
							newSurface->tlSet = true;
						}
						else if (strcasecmp(key.c_str(), "world_bottomRight") == 0)
						{
							if (tokens.size() < 4) 
							{
								cerr << "Error parsing cluster config file at line " << lineNum << endl;
								cerr << "Expecting world_bottomRight x y z" << endl;
								exit(1);
							}
							
							from_string( newSurface->w_bottomRight.x(), tokens[1] );
							from_string( newSurface->w_bottomRight.y(), tokens[2] );
							from_string( newSurface->w_bottomRight.z(), tokens[3] );
							newSurface->brSet = true;
						}
						else if (strcasecmp(key.c_str(), "pixel_bottomLeft") == 0)
						{
							if (tokens.size() < 3) 
							{
								cerr << "Error parsing cluster config file at line " << lineNum << endl;
								cerr << "Expecting pixel_bottomLeft x y" << endl;
								exit(1);
							}
							
							from_string( newSurface->p_bottomLeft.x(), tokens[1] );
							from_string( newSurface->p_bottomLeft.y(), tokens[2] );
							pixelBottomLeftSet =  true;
						}
						else if (strcasecmp(key.c_str(), "pixel_topRight") == 0)
						{
							if (tokens.size() < 3) 
							{
								cerr << "Error parsing cluster config file at line " << lineNum << endl;
								cerr << "Expecting pixel_topRight x y" << endl;
								exit(1);
							}
							
							from_string( newSurface->p_topRight.x(), tokens[1] );
							from_string( newSurface->p_topRight.y(), tokens[2] );
							pixelTopRightSet = true;
						}						
						else
						{
							cerr << "Error parsing cluster config file at line " << lineNum << endl;
							cerr << "Unrecognized keyword " << key << endl;
							exit(1);
						}
						if (pixelBottomLeftSet && worldBottomLeftSet && pixelTopRightSet && worldTopRightSet && newSurface)
						{
							// scale and calculate U V W
							newSurface->scaleWorldCorners(FEET_2_GRID);
							newSurface->calculateUVW();
							
							// calculate normalized coordinates of the two corners of surface
							newSurface->n_bottomLeft.x() = (double) newSurface->p_bottomLeft.x() / (double) (newWindow->width-1);
							newSurface->n_bottomLeft.y() = (double) newSurface->p_bottomLeft.y() / (double) (newWindow->height-1);
					
							newSurface->n_topRight.x() = (double) newSurface->p_topRight.x() / (double) (newWindow->width-1);
							newSurface->n_topRight.y() = (double) newSurface->p_topRight.y() / (double) (newWindow->height-1);
					
							newSurface->n_bottomRight.x() = newSurface->n_topRight.x();
							newSurface->n_bottomRight.y() = newSurface->n_bottomLeft.y();
					
							newSurface->n_topLeft.x() = newSurface->n_bottomLeft.x();
							newSurface->n_topLeft.y() = newSurface->n_topRight.y();
							
							// add surface
							newWindow->surfaces.push_back( *newSurface );
							delete newSurface; newSurface = NULL;
							pixelBottomLeftSet = worldBottomLeftSet = pixelTopRightSet = worldTopRightSet = false;
						}
					}
				}
			}
		}
	}
	
	// add last node
	if (newNode)
	{
		if ( newNode->hostname.length() != 0 && newNode->windows.size() > 0)
		{
			// data of last node complete, add
			clusterConfig.nodeMap[ newNode->hostname ].push_back( *newNode );
			clusterConfig.nodeList.push_back( *newNode );
			delete newNode;
			newNode = NULL;
			topLeftSet = bottomRightSet = false;
		}
		else
		{
			// parse error, missing data
			cerr << "Error parsing cluster config at end of file." << endl;
			cerr << "Information for last node is not complete." << endl;
			exit(1);
		}
	}
	
	input.close();
	
}

void printClusterConfig()
{
	for (int i = 0; i < clusterConfig.nodeList.size(); i++)
	{
	
		const Node & node = clusterConfig.nodeList[i];
		cout << "Node" << endl;
		
			cout << "\t Hostname\t\t" << node.hostname << endl;
			cout << "\t Windows:\t\t" << node.windows.size() << endl;
			for (int i = 0; i < node.windows.size(); i++)
			{
				const RenderWindow & win = node.windows[i];
				cout << "\t\t size\t\t\t" << win.width << " x " << win.height << endl;
				cout << "\t\t offset\t\t\t" << win.x << " x " << win.y << endl;
				if (win.fullscreen) {
					cout << "\t\t FULLSCREEN\n";
				}
				
				cout << "\t\t Surfaces:\t" << win.surfaces.size() << endl;
				for (int k = 0; k < win.surfaces.size(); k++)
				{
					const RenderSurface & surface = win.surfaces[k];
					cout << "\t\t\t bottom left: " << 
						surface.w_bottomLeft.x() << ", " <<
						surface.w_bottomLeft.y() << ", " <<
						surface.w_bottomLeft.z() << " --> " <<
						surface.p_bottomLeft.x() << ", " <<
						surface.p_bottomLeft.y() << endl;
					
					cout << "\t\t\t top right: " << 
						surface.w_topRight.x() << ", " <<
						surface.w_topRight.y() << ", " <<
						surface.w_topRight.z() << " --> " <<
						surface.p_topRight.x() << ", " <<
						surface.p_topRight.y() << endl;
				}
				cout << "\n";
			}
			cout << "\n";
					
		cout << endl;
	}
}


vector<GLWindow*> openRenderWindows(float resScaleX, float resScaleY, Node *& myNode)
{
	vector<GLWindow *> myWindows;
	
	// find my node name and rank
	char myhost[1024*3];
	gethostname(myhost, 1024*3 - 1);
	
	int myRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	
	if (clusterConfig.nodeMap.find( myhost ) == clusterConfig.nodeMap.end())
	{
		// can not find my node name
		cerr << "\n";
		cerr << "PANIC !!" << endl;
		cerr << "Node: " << myhost << " can not find its configuration in cluster file." << endl;
		exit(1);
	}
	
	NodeVector & myNodeVector = clusterConfig.nodeMap[ myhost ];
	
	// see which node I should get
	if (myNodeVector.size() == 1)
	{
		myNode = & myNodeVector[ 0 ];
	}
	else
	{
		myNode = & myNodeVector[ myRank % myNodeVector.size() ];
	}
	
	for (int i = 0; i < myNode->windows.size(); i++)
	{
		RenderWindow & curWindow = myNode->windows[i];
		
		if (curWindow.Xserver.length() > 0)
		{
			char Xserver[1024];
			sprintf(Xserver, "%s%s", "", curWindow.Xserver.c_str());
			
			if (0 != setenv( "DISPLAY", Xserver, 1 ) )
			{
				cout << "\t\t >> " << myhost << ", rank: " << myRank << ", Couldn't put environment variable." << endl;
			}
			cout << "\t >> " << myhost << ", rank: " << myRank << ", display variable: " << getenv("DISPLAY") << endl;
		}
		
		// create GL window
		curWindow.theWindow = new GLWindow
		( 
			curWindow.x, curWindow.y, 
			curWindow.width, curWindow.height, 
			curWindow.fullscreen,
			resScaleX,
			resScaleY
		);
				
		// give the window a copy of its render surfaces
		curWindow.theWindow->surfaces = curWindow.surfaces;
		curWindow.theWindow->myHostName = myhost;
		
		myWindows.push_back(curWindow.theWindow);
	}
	return myWindows;
}

void MatrixTransform(vec4 & out, const Matrix44 & matrix, const vec4 & v)
{
	out.x() = 
		matrix.m[0][0] * v.x() +
		matrix.m[0][1] * v.y() +
		matrix.m[0][2] * v.z() +
		matrix.m[0][3] * v.w()	;

	out.y() = 
		matrix.m[1][0] * v.x() +
		matrix.m[1][1] * v.y() +
		matrix.m[1][2] * v.z() +
		matrix.m[1][3] * v.w()	;

	out.z() = 
		matrix.m[2][0] * v.x() +
		matrix.m[2][1] * v.y() +
		matrix.m[2][2] * v.z() +
		matrix.m[2][3] * v.w()	;

	out.w() = 
		matrix.m[3][0] * v.x() +
		matrix.m[3][1] * v.y() +
		matrix.m[3][2] * v.z() +
		matrix.m[3][3] * v.w()	;
}

void MatrixTransform(vec3 & out, const Matrix44 & matrix, const vec3 & v)
{
	out.x() = 
		matrix.m[0][0] * v.x() +
		matrix.m[0][1] * v.y() +
		matrix.m[0][2] * v.z() +
		matrix.m[0][3] * 1.0f;

	out.y() = 
		matrix.m[1][0] * v.x() +
		matrix.m[1][1] * v.y() +
		matrix.m[1][2] * v.z() +
		matrix.m[1][3] * 1.0f;

	out.z() = 
		matrix.m[2][0] * v.x() +
		matrix.m[2][1] * v.y() +
		matrix.m[2][2] * v.z() +
		matrix.m[2][3] * 1.0f;
}

void MatrixTransformColMajor(vec3 & out, const Matrix44 & matrix, const vec3 & v)
{
	out.x() = 
		matrix.m[0][0] * v.x() +
		matrix.m[1][0] * v.y() +
		matrix.m[2][0] * v.z() +
		matrix.m[3][0] * 1.0f;

	out.y() = 
		matrix.m[0][1] * v.x() +
		matrix.m[1][1] * v.y() +
		matrix.m[2][1] * v.z() +
		matrix.m[3][1] * 1.0f;

	out.z() = 
		matrix.m[0][2] * v.x() +
		matrix.m[1][2] * v.y() +
		matrix.m[2][2] * v.z() +
		matrix.m[3][2] * 1.0f;
}

RenderSurface RenderSurface::transform(const Matrix44 & caveTransform) const
{
	
	// copy other parameters
	RenderSurface newSurface(*this);	

	MatrixTransform( newSurface.bL(), caveTransform, this->bL() );
	MatrixTransform( newSurface.bR(), caveTransform, this->bR() );
	MatrixTransform( newSurface.tL(), caveTransform, this->tL() );
	MatrixTransform( newSurface.tR(), caveTransform, this->tR() );

	// transform UVW
	
	Matrix44::transform(newSurface.U, caveTransform, this->U);
	Matrix44::transform(newSurface.V, caveTransform, this->V);
	Matrix44::transform(newSurface.W, caveTransform, this->W);
	return newSurface;
}

