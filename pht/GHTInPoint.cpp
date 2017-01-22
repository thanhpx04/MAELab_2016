/*
 * GHTInPoint.cpp
 *
 *  Created on: Jan 17, 2017
 *      Author: linh
 */
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <stdio.h>
#include <float.h>
#include <string.h>
using namespace std;

#include "../utils/Converter.h"
#include "../imageModel/Point.h"
#include "../imageModel/Line.h"
#include "../imageModel/Matrix.h"
#include "GHTInPoint.h"

double angleVector(Point p1, Point p2, Point q1, Point q2)
{
	Point vp(p2.getX() - p1.getX(), p2.getY() - p1.getY());
	Point vq(q2.getX() - q1.getX(), q2.getY() - q1.getY());

	double lengthVP = sqrt(
		(double) (vp.getX() * vp.getX()) + (double) (vp.getY() * vp.getY()));
	double lengthVQ = sqrt(
		(double) (vq.getX() * vq.getX()) + (double) (vq.getY() * vq.getY()));

	double vpq = (double) (vp.getX() * vq.getX())
		+ (double) (vp.getY() * vq.getY());

	return acos(vpq / (lengthVP * lengthVQ));
}
RTable rTableConstruct(ptr_IntMatrix gradMatrix, Point center)
{
	RTable rtable;
	rtable.center.setX(center.getX());
	rtable.center.setY(center.getY());
	rtable.entriesTable.resize(361);

	// in default, rtable includes 360 rows for 360 degrees of gradient directions.
	int rows = gradMatrix->getRows();
	int cols = gradMatrix->getCols();

	int gd;
	REntry rentry;
	//ofstream off("rtable.txt");
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			gd = gradMatrix->getAtPosition(r, c);
			if (gd != -1)
			{
				PolarValue pv;
				pv.p.setX(c);
				pv.p.setY(r);
				Point hPoint(pv.p.getX() + 100, pv.p.getY());
				pv.angle = angleVector(pv.p, hPoint, pv.p, rtable.center);
				pv.distance = (Line(rtable.center, pv.p)).getLength();
				rentry = rtable.entriesTable.at((gd));

				(rtable.entriesTable.at((gd))).gradient = gd;
				(rtable.entriesTable.at((gd))).polarValues.push_back(pv);
				//off << "\n" << pv.angle << "\t" << pv.distance;
			}
		}
	}
	//off.close();
	return rtable;
}

Point houghSpace(ptr_IntMatrix gradMatrix, RTable rentries)
{
	int rows = gradMatrix->getRows();
	int cols = gradMatrix->getCols();
	ptr_IntMatrix acc = new Matrix<int>(rows, cols, 0);

	int gradient = 0;
	REntry entry;
	int maxValue = 0, maxXIndex = 0, maxYIndex = 0;
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			gradient = gradMatrix->getAtPosition(r, c);
			if (gradient != -1)
			{
				entry = rentries.entriesTable.at(gradient);
				if (entry.gradient == gradient)
				{
					vector<PolarValue> polarValues = entry.polarValues;
					if (polarValues.size() > 0)
					{
						//PolarValue pvalue;
						double avalue = 0, dvalue = 0;
						int xindex = 0, yindex = 0, accValue = 0;
						for (size_t k = 0; k < polarValues.size(); k++)
						{
							PolarValue pvalue = polarValues.at(k);
							avalue = pvalue.angle;
							dvalue = pvalue.distance;

							xindex = c + (dvalue * cos(avalue));
							yindex = r + (dvalue * sin(avalue));
							//cout << "\n" << xindex << "\t" << yindex;
							if (xindex < cols && yindex < rows)
							{
								// increase the value in accumulator
								accValue = acc->getAtPosition(yindex, xindex);
								accValue += 1;
								acc->setAtPosition(yindex, xindex, accValue);
							}
							if (accValue > maxValue && xindex > 0 && xindex < cols
								&& yindex > 0 && yindex < rows)
							{
								maxValue = accValue;
								maxXIndex = xindex;
								maxYIndex = yindex;
							}
						}
					}
				}
			}
		}
	}
	cout << "\n center: ";
	rentries.center.toString();
	cout << "\nMax Value:" << maxValue;
	cout << "\nMax X index:" << maxXIndex;
	cout << "\nMax Y index:" << maxYIndex << endl;
	return Point(maxXIndex, maxYIndex);
}

double avgDistance(vector<Point> listPoints, Line axis)
{
	double totalDist = 0;
	size_t nPoints = listPoints.size();
	for (size_t j = 0; j < nPoints; j++)
	{
		Point pj = listPoints.at(j);
		double distance = axis.perpendicularDistance(pj);
		totalDist += distance;
	}
	return totalDist/(int)nPoints;
}

Line principalAxis(ptr_IntMatrix gradMatrix, Point &cPoint)
{
	int rows = gradMatrix->getRows();
	int cols = gradMatrix->getCols();
	int totalX = 0, totalY = 0, count = 0;
	// get list of points
	vector<Point> listOfPoints;
	for (int r = 0; r < rows; ++r)
	{
		for (int c = 0; c < cols; ++c)
		{
			if (gradMatrix->getAtPosition(r, c) != -1)
			{
				listOfPoints.push_back(Point(c, r));
				totalX += c;
				totalY += r;
				count++;
			}
		}
	}
	// compute the centroid point
	cPoint.setX(totalX / count);
	cPoint.setY(totalY / count);

	Point sPoint;
	double minAvgDist = DBL_MAX;
	size_t nPoints = listOfPoints.size();
	for (size_t i = 0; i < nPoints; ++i)
	{
		Point pi = listOfPoints.at(i);
		Line l(cPoint, pi);
		double avgDist = avgDistance(listOfPoints, l);
		if (avgDist < minAvgDist)
		{
			minAvgDist =avgDist;
			sPoint.setX(pi.getX());
			sPoint.setY(pi.getY());
		}
	}

	return Line(cPoint, sPoint);
}
vector<Point> detectLandmarks(Point refPoint, Point ePoint,
	vector<Point> mlandmarks)
{
	vector<Point> esLandmarks;
	Point mlm;
	int deltaX = 0, deltaY = 0, xn = 0, yn = 0;
	for (size_t i = 0; i < mlandmarks.size(); i++)
	{
		mlm = mlandmarks.at(i);
		deltaX = refPoint.getX() - mlm.getX();
		deltaY = refPoint.getY() - mlm.getY();

		xn = ePoint.getX() - deltaX;
		yn = ePoint.getY() - deltaY;
		esLandmarks.push_back(Point(xn, yn));
	}
	return esLandmarks;
}

Point centroidPoint(ptr_IntMatrix gradMatrix)
{
	int gd = 0;
	int rows = gradMatrix->getRows();
	int cols = gradMatrix->getCols();
	int totalX = 0, totalY = 0, count = 0;
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			gd = gradMatrix->getAtPosition(r, c);
			if (gd != -1)
			{
				totalX += c;
				totalY += r;
				count++;
			}
		}
	}
	return Point(totalX / count, totalY / count);
}
