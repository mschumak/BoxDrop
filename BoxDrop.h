#ifndef SEDEEN_SRC_PLUGINS_CELLULARITYMEASUREMENT_CELLULARITYMEASUREMENT_H
#define SEDEEN_SRC_PLUGINS_CELLULARITYMEASUREMENT_CELLULARITYMEASUREMENT_H

// System headers
#include <memory>
#include <QtGui/qimage.h>
#include <Windows.h>

// DPTK headers - a minimal set 
#include "algorithm/AlgorithmBase.h"
#include "algorithm/Parameters.h"
#include "algorithm/Results.h"
#include "algorithm/RegionListParameter.h"
#include "archive/Session.h"
#include "geometry/graphic/Rectangle.h"

namespace sedeen {
namespace image {
class RawImage;
}
namespace algorithm {
class CellularityMeasurement: public AlgorithmBase {
public:
	CellularityMeasurement();
	virtual ~CellularityMeasurement();
private:
	virtual void run();
	virtual void init(const image::ImageHandle& image);
	std::string generateReport() const;
	void updateIntermediateResult();
	bool buildPipeline();
	algorithm::GraphicItemParameter m_region_toProcess;
	IntegerParameter m_size;
	int point,xCenter,yCenter;
	TextFieldParameter m_text;
	ImageResult m_intermediate_result;
	Rectangle m_rect;
	GraphicStyle m_style;
	std::string m_name;
	/// The output result
	algorithm::TextResult m_output_text;
	OverlayResult m_results;
	std::string m_type;

};
} //namespace algorithm
} //namespace sedeen

#endif // ifndef SEDEEN_SRC_PLUGINS_CELLULARITYMEASUREMENT_CELLULARITYMEASUREMENT_H
