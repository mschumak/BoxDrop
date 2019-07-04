/*=============================================================================
 *
 *  Copyright (c) 2019 Sunnybrook Research Institute
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 *=============================================================================*/
 
 // Primary header
#include "BoxDrop.h"

// System headers
#include <algorithm>
#include <iostream>
#include <iomanip>

//DPTK headres
#include "Algorithm.h"
#include "Geometry.h"
#include "Global.h"
#include "Image.h"
#include <sstream>

// Poco header needed for the macros below 
#include <Poco/ClassLibrary.h>

// Declare that this object has AlgorithmBase subclasses
//  and declare each of those sub-classes
POCO_BEGIN_MANIFEST(sedeen::algorithm::AlgorithmBase)
POCO_EXPORT_CLASS(sedeen::algorithm::BoxDrop)
POCO_END_MANIFEST

namespace sedeen {
namespace algorithm {
BoxDrop::BoxDrop()
{
}

BoxDrop::~BoxDrop()
{
}

void BoxDrop::init(const image::ImageHandle& image)
{
	if (image::isNull(image)) return;
	auto dims = getDimensions(image,0);
	auto min_dim = std::min(dims.width(),dims.height());
	const auto DEFAULT_SIZE = 512;
	m_size = createIntegerParameter(
		*this, 
		"ROI Size", 
		"Value assigned as both the width and height of each box",
		DEFAULT_SIZE,
		1,
		min_dim,
		false);

		m_text = createTextFieldParameter(*this,
			"ROI Description",
			"Percentage of Cellularity",
			"cellularity(%)",
			true);

		m_region_toProcess = 
			createGraphicItemParameter(*this,	// Algorithm - to be bound to UI
			"Processing ROI",					// Widget label
			"Region to operate on.",
			true);								// Widget tooltip

		m_output_text = createTextResult(*this, "text Result");
		// Default number of regions to place along the narrowest dimension.
		const auto DEFAULT_NUM_REGIONS = 8;
		const auto DEFAULT_SPACING = min_dim / DEFAULT_NUM_REGIONS;

		m_results = createOverlayResult(*this);
}

bool BoxDrop::buildPipeline()
{
	using namespace image::tile;
	bool pipelineChanged = false;
	std::string path_to_image = 
	image()->getMetaData()->get(image::StringTags::SOURCE_DESCRIPTION,0);
	Session s(path_to_image);
		auto b = s.loadFromFile();
		auto p = s.imagePath();
	if (m_region_toProcess.isUserDefined())
	{
		std::shared_ptr<GraphicItemBase> region = m_region_toProcess;
		auto rect = containingRect(region->graphic());
		auto xmin = rect.x();
		auto ymin = rect.y();
		auto xmax = xMax(rect);
		auto ymax = yMax(rect);
		xCenter = (xmin+xmax)/2;
		yCenter = (ymin+ymax)/2;
		auto xTopLeft = xCenter-m_size/2;
		auto yTopLeft = yCenter-m_size/2;


		auto graphics = s.getGraphics();
		auto numberOfOverlays = graphics.size();
		auto points = graphics[numberOfOverlays-1].getPoints();
		m_style = graphics[numberOfOverlays-1].getStyle();
		m_name = graphics[numberOfOverlays-1].getName();
		point = points[0][0].getX();
		m_rect = Rectangle(xTopLeft,yTopLeft,m_size,m_size,0,Center);
		GraphicDescription graph;
		std::string text = m_text; 
		text = "Cellularity: "+text;

		graph.setDescription(text.c_str());
		graph.setName(m_name.c_str());
		graph.setStyle(m_style);
		graph.setGeometry(graphics[numberOfOverlays-1].getGeometry());
		std::vector<std::vector<PointF>> newPoints;
		PointF p;
		std::vector<PointF> vectorP;
		p.setX(xTopLeft);
		p.setY(yTopLeft);
		vectorP.push_back(p);
		p.setX(xTopLeft+m_size);
		p.setY(yTopLeft);
		vectorP.push_back(p);
		p.setX(xTopLeft+m_size);
		p.setY(yTopLeft+m_size);
		vectorP.push_back(p);
		p.setX(xTopLeft);
		p.setY(yTopLeft+m_size);
		vectorP.push_back(p);


		newPoints.push_back(vectorP);
		graph.setPoints(newPoints);
		graphics.push_back(graph);



		//	if(!std::string(graphics[i].getDescription()).empty())
		//	//if(std::string(graphics[i].getName()).find("toBeDeleted")!=std::string::npos)
		//	{
		//		if(std::string(graphics[i].getName()).find("Cellularity")==std::string::npos)
		//		{
		//			std::string name = "Cellularity"+std::string(graphics[i].getName());
		//			graphics[i].setName(name.c_str());
		//		}
		//		newGraphics.push_back(graphics[i]);
		//	}
		s.setGraphics(graphics);
		pipelineChanged = true;
	}
	s.saveToFile();
	return pipelineChanged;
}
void BoxDrop::run()
{
	using namespace image::tile;
	auto pipeline_changed = buildPipeline();
	if (pipeline_changed)
	{
		std::string text = m_text;
		text = "Cellularity: "+text;
		m_results.drawRectangle(m_rect,m_style,m_name,text);
		m_results.setVisible(true);
		updateIntermediateResult();


	}
	std::string path_to_image = 
	image()->getMetaData()->get(image::StringTags::SOURCE_DESCRIPTION,0);
	Session s(path_to_image);
	auto b = s.loadFromFile();
	auto p = s.imagePath();	
	auto graphics = s.getGraphics();
	auto numberOfOverlays =graphics.size();
	std::vector<sedeen::GraphicDescription> newGraphics;
	int i = 0;
	while(i<numberOfOverlays)
	{
		if(i<numberOfOverlays-1)
		{
			if((std::string(graphics[i].getDescription()).empty()) &&
				std::strcmp(graphics[i].getName(),graphics[i+1].getName())==0 &&
				std::string(graphics[i+1].getDescription()).find("Cellularity:")!=std::string::npos)
			{
				i++;
			}
		}
		newGraphics.push_back(graphics[i]);
		i++;
	}
	xCenter = numberOfOverlays;
	s.setGraphics(newGraphics);
	s.saveToFile();
	auto report = generateReport();
	m_output_text.sendText(report);

	updateIntermediateResult();

}

std::string BoxDrop::generateReport() const
{
	std::ostringstream ss;
	ss << std::left << std::setfill(' ') << std::setw(20);
	ss <<"ROI Size:" << m_size<<"x"<<m_size<< std::endl;
	ss<<std::left<<std::setfill(' ')<<std::setw(20);
	ss<<"Processed Box:"<<std::fixed<<std::setprecision(1)<<m_name<<xCenter;
	ss<<std::endl;

	return ss.str();
}
void BoxDrop::updateIntermediateResult()
{
	auto update_result = [&](const std::shared_ptr<image::tile::Factory> &factory) 
	{
		// Create a compositor
		auto compositor = std::unique_ptr<image::tile::Compositor>
			(new image::tile::Compositor(factory));
		Size size;
		size.setWidth(m_size);
		size.setHeight(m_size);
		Point point;
		point.setX((int)(m_rect.topLeft().getX()));
		point.setY((int)(m_rect.topLeft().getY()));
	    DisplayRegion region = DisplayRegion(Rect(point,size),size);
		auto outputImage = 
			compositor->getImage(region.source_region, region.output_size);

		// Update UI
		m_intermediate_result.update(outputImage, region.source_region);
	};
}
} // namespace algorithm
} // namespace sedeen