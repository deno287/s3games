#ifdef WIN32
#include <windows.h>
#endif

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>

using namespace cv;
using namespace std;

inline int f2i(double f) { return (int) floor(f + 0.5); }

#ifdef WIN32
double usec()
{
	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER tick;	
	QueryPerformanceFrequency(&ticksPerSecond); 
	QueryPerformanceCounter(&tick);
	double ticks_per_micro= (double)ticksPerSecond.QuadPart/1000000.0;
	return (double)(tick.QuadPart / ticks_per_micro);
}
#endif

class ElementType
{
private:
	float hueMin, hueMax;
	float satMin, satMax;
	float valueMin, valueMax;
	int sizeMin, sizeMax;	

public:
	char *elementTypeName;
	int index;
	
	ElementType(double hueMin, double hueMax, double satMin, double satMax, double valueMin, double valueMax, int sizeMin, int sizeMax, char *elementTypeName, int index)
	{
		this->hueMin = (float)hueMin;
		this->hueMax = (float)hueMax;
		this->satMin = (float)satMin;
		this->satMax = (float)satMax;
		this->valueMin = (float)valueMin;
		this->valueMax = (float)valueMax;
		this->sizeMin = sizeMin;
		this->sizeMax = sizeMax;
		this->elementTypeName = elementTypeName;		
		this->index = index;
	}

	bool matches(double hue, double sat, double value)
	{		
		bool hues = (hueMin < hueMax) ? ((hue >= hueMin) && (hue <= hueMax)) :
			                            ((hue >= hueMin) || (hue <= hueMax));
		if (!hues) return false;
		return  
				(sat >= satMin) && (sat <= satMax) &&
				(value >= valueMin) && (value <= valueMax);
	}

	bool matchesSize(int size)
	{				
		return (size >= sizeMin) && (size <= sizeMax);
	}

	void visualize(Vec3f &pt)
	{
		float avgHue = (hueMin < hueMax) ? ((hueMin + hueMax) / 2.0f) :
			                               (f2i((hueMin + 360.0 + hueMax) / 2.0f) % 360);
		pt[0] = avgHue;
		pt[1] = ((satMin + satMax) / 2.0f);
		pt[2] = ((valueMin + valueMax) / 2.0f);
	}

	char *toString()
	{
		static char str[200];
		sprintf_s(str, 199, "Hue: %d - %d, Sat: %.2f - %.2f, Val: %d - %d\n", f2i(hueMin), f2i(hueMax), satMin, satMax, f2i(valueMin), f2i(valueMax));
		return str;
	}

	void decHueMin()
	{
		hueMin--;
		if (hueMin < 0.0f) hueMin = 360.0f;
	}

	void incHueMin()
	{
		hueMin++;
		if (hueMin > 360.0f) hueMin = 0.0f;
	}

	void decSatMin()
	{
		satMin -= 0.01f;
		if (satMin < 0.0f) satMin = 0.0f;
	}

	void incSatMin()
	{
		satMin += 0.01f;
		if (satMin > 1.0f) satMin = 1.0f;
	}

	void decValMin()
	{
		valueMin--;
		if (valueMin < 0.0f) valueMin = 0.0f;
	}

	void incValMin()
	{
		valueMin++;
		if (valueMin > 255.0f) valueMin = 255.0f;
	}

		void decHueMax()
	{
		hueMax--;
		if (hueMax < 0.0f) hueMax = 360.0f;
	}

	void incHueMax()
	{
		hueMax++;
		if (hueMax > 360.0f) hueMax = 0.0f;
	}

	void decSatMax()
	{
		satMax -= 0.01f;
		if (satMax < 0.0f) satMax = 0.0f;
	}

	void incSatMax()
	{
		satMax += 0.01f;
		if (satMax > 1.0f) satMax = 1.0f;
	}

	void decValMax()
	{
		valueMax--;
		if (valueMax < 0.0f) valueMax = 0.0f;
	}

	void incValMax()
	{
		valueMax++;
		if (valueMax > 255.0f) valueMax = 255.0f;
	}

};

class Location 
{
public:
	int x, y;
	char *elementType;	

	Location(int x, int y, char *elementType)
	{
		this->x = x;
		this->y = y;
		this->elementType = elementType;		
	}
};

class ElementMatcher
{

public:
	ElementMatcher(ElementType &etype, Mat &img, int x, int y) 
		: fImg(img), et(etype)
	{		
		fsize = -1;
		dir = -5;
		fwidth = img.size().width;
		fheight = img.size().height;
		fx = x;
		fy = y;
		centerx = 0;
		centery = 0;
	}

	long size(int &cx, int &cy)
	{
		if (fsize == -1)		
			if (follow()) 
			{				
				getSize();
				centerx /= fsize;
				centery /= fsize;
			}		
		cx = (int)centerx;
		cy = (int)centery;
		return fsize;
	}

private:	
	Mat &fImg;
	ElementType &et;
	int fx, fy, startx, starty;
	int	fwidth, fheight;
	long long centerx, centery;
	double fhue, fsat, fval;
	long fsize;
	int dir;

	void getHSV()
	{
		fhue = fImg.at<Vec3f>(fy, fx)[0];
		fsat = fImg.at<Vec3f>(fy, fx)[1];
		fval = fImg.at<Vec3f>(fy, fx)[2];
	}

	bool follow()
	{
		if (!goToNeighbor(dir)) return false;
		getHSV();
		if (et.matches(fhue, fsat, fval)) return true;
		goToNeighbor(opposite(dir));
		return false;
	}

	int opposite(int dir)
	{
		return (dir ^ 2);
	}

	int next(int dir)
	{
		return (dir % 4) - 1;
	}

	int previous(int dir)
	{
		return next(opposite(dir));
	}

	void getSize()
	{	
		dir = -1;
		fsize = 0;
		startx = fx;
		starty = fy;
		do { step(); } while (findStep());			
	}

	void step()
	{		
		fImg.at<Vec3f>(fy, fx)[1] = (float)opposite(dir);
		dir = previous(dir);
		fImg.at<Vec3f>(fy, fx)[0] = (float)dir; 
		fImg.at<Vec3f>(fy, fx)[2] = (float)(-2 - et.index); 
		fsize++;
		centerx += fx;
		centery += fy;
	}

	bool goToNeighbor(int dir)
	{
		switch (dir)
		{
		case -1: if (fx > 0) fx--; else return false; break;
		case -2: if (fy > 0) fy--; else return false; break;
		case -3: if (fx < fwidth - 1) fx++; else return false; break;
		case -4: if (fy < fheight - 1) fy++; else return false; break;
		}
		return true;
	}

	bool findStep()
	{		
		while (!follow()) 
		{
			dir = next(dir);
			fImg.at<Vec3f>(fy, fx)[0] = (float)dir;
			while (dir == (f2i(fImg.at<Vec3f>(fy, fx)[1]))) 
			{
				if ((fx == startx) && (fy == starty)) return false;
				goToNeighbor(dir);				
				dir = f2i(fImg.at<Vec3f>(fy, fx)[0]);
				dir = next(dir);
				fImg.at<Vec3f>(fy, fx)[0] = (float)dir;
			}
		}
		return true;
	}
};

class ObjectLocator
{
public:
	ObjectLocator(Mat &img, vector<ElementType *> &elTypes, vector<Location *> &locs)
		: locations(locs), elementTypes(elTypes), findingImg(img)
	{
	}

	void findObjects()
	{
		int fwidth = findingImg.size().width; 
		int fheight = findingImg.size().height;

		for (int x = 0; x < fwidth; x++)
			for (int y = 0; y < fheight; y++)		
				for (vector<ElementType *>::iterator it = elementTypes.begin(); it < elementTypes.end(); it++)
				{
					ElementMatcher elo(**it, findingImg, x, y);
					int cx, cy;
					if ((*it)->matchesSize(elo.size(cx, cy)))
						locations.push_back(new Location(cx, cy, (*it)->elementTypeName));
				}
	}

private:
	vector<Location *> &locations;
	vector<ElementType *> &elementTypes;
	Mat &findingImg;
};

static void onMouse( int event, int x, int y, int, void* mouse )
{
	if( event != EVENT_LBUTTONDOWN ) return;
	int *mouseData = (int *)mouse;
	mouseData[0] = 1;
	mouseData[1] = x;
	mouseData[2] = y;
}

void visualize(Mat &img, Mat &vis, vector<ElementType *> &ets)
{
	int width = img.size().width; 
	int height = img.size().height;

	for (int x = 0; x < width; x++)
		for (int y = 0; y < height; y++)
			if (img.at<Vec3f>(y, x)[2] < -1)
			{
				int i = -2 - (int)(img.at<Vec3f>(y, x)[2]);
				ets.at(i)->visualize(img.at<Vec3f>(y, x));
			}

	cvtColor(img, vis, CV_HSV2BGR );
	Mat vis2(width, height, CV_8UC3);
	vis.convertTo(vis2, CV_8UC3);

	namedWindow( "result", CV_WINDOW_AUTOSIZE );
	imshow( "result", vis2 );
}

int main( int argc, char** argv )
{
	vector<Location *> locations;
	vector<ElementType *> elementTypes;
	int selectedElement = 0;

	cout << "S:S3 Games Camera" << endl;

    VideoCapture *cap = new VideoCapture(0);
	Mat image;

	bool show = false;
	
	ElementType t1(82.0, 167.0, 0.3, 1.0, 76.0, 230.0, 500, 5000, "green", 0);
	//ElementType t1(12.0, 347.0, 0.3, 1.0, 76.0, 230.0, 100, 5000, "green", 1, 0);
	ElementType t2(343.0, 19.0, 0.5, 1.0, 100.0, 255.0, 10, 5000, "red", 1);

	elementTypes.push_back(&t1);
	elementTypes.push_back(&t2);

	if (!cap->isOpened())
	{
		cerr << "could not open video capture device.";
		return 0;
	}

	char *camera = "View from Camera";
	namedWindow( camera, CV_WINDOW_AUTOSIZE );
	
	int mouseData[3];
	int &mouseClicked = mouseData[0];
	int &mouseX = mouseData[1];
	int &mouseY = mouseData[2];
	mouseClicked = 0;
	setMouseCallback(camera, onMouse, mouseData);

	if (!cap->read(image))
	{
		cout << "F:Could not read frame from camera.";
		return 0;
	}
	Mat image32(image.size().width, image.size().height, CV_32FC3);
	Mat hsv32(image.size().width, image.size().height, CV_32FC3);
	Mat vis(image.size().width, image.size().height, CV_32FC3);

	char key;

	do
	{
		key = waitKey(1);
		
		char k = key;
		while (k != -1) k = waitKey(1);
		
		double tm1 = usec();

		if (!cap->read(image))
		{
			cerr << "could not read frame from camera.";
			return 0;
		}

		double tm2 = usec();

		imshow( camera, image );
				
		if (mouseClicked)
		{
			image.convertTo(image32, CV_32FC3);
			cvtColor(image32, hsv32, CV_RGB2HSV);
			cout << "I:[" << mouseX << "," << mouseY << "]: ";
			cout << "hue=" << hsv32.at<cv::Vec3f>(mouseY,mouseX)[0] << " ";
			cout << "sat=" << hsv32.at<cv::Vec3f>(mouseY,mouseX)[1] << " ";
			cout << "val=" << hsv32.at<cv::Vec3f>(mouseY,mouseX)[2] << endl;
			mouseClicked = false;
			key = -1;
		}
		
		if (key == 'h')
		{
			elementTypes.at(selectedElement)->decHueMin();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'j')
		{
			elementTypes.at(selectedElement)->decHueMax();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'H')
		{
			elementTypes.at(selectedElement)->incHueMin();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'J')
		{
			elementTypes.at(selectedElement)->incHueMax();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 's')
		{
			elementTypes.at(selectedElement)->decSatMin();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'd')
		{
			elementTypes.at(selectedElement)->decSatMax();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'S')
		{
			elementTypes.at(selectedElement)->incSatMin();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'D')
		{
			elementTypes.at(selectedElement)->incSatMax();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'v')
		{
			elementTypes.at(selectedElement)->decValMin();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'b')
		{
			elementTypes.at(selectedElement)->decValMax();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'V')
		{
			elementTypes.at(selectedElement)->incValMin();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == 'B')
		{
			elementTypes.at(selectedElement)->incValMax();
			cout << "I:" << elementTypes.at(selectedElement)->toString();
			key = ' ';
		}
		else if (key == '+')
		{
			selectedElement++;
			if (selectedElement >= (int)(elementTypes.size())) selectedElement = elementTypes.size() - 1;		
			cout << "I:Selected element " << selectedElement << '=' + elementTypes.at(selectedElement)->elementTypeName;
		}
		else if (key == '-')
		{
			selectedElement--;
			if (selectedElement < 0) selectedElement = 0;				
			cout << "I:Selected element " << selectedElement << '=' + elementTypes.at(selectedElement)->elementTypeName;
		}

		if (key == 13)
		{
			key = ' ';
			show = true;
		}
		else show = false;

		if (key == ' ')
		{
			double tm3 = usec();
			image.convertTo(image32, CV_32FC3);
			double tm4 = usec();
			cvtColor(image32, hsv32, CV_BGR2HSV);
			double tm5 = usec();
			locations.clear();
			ObjectLocator ol(hsv32, elementTypes, locations);
			ol.findObjects();
			double tm6 = usec();
			if (!show)
			{				
				for (vector<Location *>::iterator it = locations.begin(); it < locations.end(); it++)
					cout << "O:" << (*it)->elementType << " at " << (*it)->x << ", " << (*it)->y << endl;
			}
			else
			{
				visualize(hsv32, vis, elementTypes);
				double tm7 = usec();
				cout << "D:frame grabbed in " << tm2 - tm1 << "us," << endl << "D:      shown in " << tm3 - tm2 << "us," << endl 
					 << "D:      switched to floats in " << tm4 - tm3 << "us," << endl << "D:      converted to HSV in " 
					 << tm5 - tm4 << "us," << endl << "D:      objects detected in " << tm6 - tm5 << "us," << endl 
					 << "D:      visualized in "  << tm7 - tm6 << "us." << endl << "D:   total: " << tm7 - tm1 << "us." << endl;
			}
			key = -1;
		}

	} while (key == -1);
	
	delete cap;

    exit(0);
}

