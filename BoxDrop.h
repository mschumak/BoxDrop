/*=============================================================================
 *
 *  Copyright (c) 2021 Sunnybrook Research Institute
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

#ifndef SEDEEN_SRC_PLUGINS_BOXDROP_BOXDROP_H
#define SEDEEN_SRC_PLUGINS_BOXDROP_BOXDROP_H

// System headers
#include <memory>
//#include <QtGui/qimage.h>
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
class BoxDrop: public AlgorithmBase {
public:
	BoxDrop();
	virtual ~BoxDrop();

private:
	virtual void run();
	virtual void init(const image::ImageHandle& image);

	std::string generateReport() const;
	void updateIntermediateResult();
	bool buildPipeline();

private:
    ///Define the save file dialog options outside of init
    sedeen::file::FileDialogOptions defineSaveFileDialogOptions();

    ///Save the separated image to a TIF/PNG/BMP/GIF/JPG flat format file
    bool SaveFlatImageToFile(const std::string &p);

    ///Given a full file path as a string, identify if there is an extension and return it
    const std::string getExtension(const std::string &p);

    ///Search the m_saveFileExtensionText vector for a given extension, and return the index, or -1 if not found
    const int findExtensionIndex(const std::string &x) const;

    ///Check if the file exists and accessible for reading or writing, or that the directory to write to exists
    static bool checkFile(const std::string &, const std::string &);

private:
    algorithm::GraphicItemParameter m_region_toProcess;
    IntegerParameter m_size;
    int point, xCenter, yCenter;
    TextFieldParameter m_text;
    ImageResult m_intermediate_result;
    Rectangle m_rect;
    GraphicStyle m_style;

    ///User choice whether to save the image within the box as output
    BoolParameter m_saveOutputImage;
    ///Choose what format to write the separated images in
    OptionParameter m_saveFileFormat;
    ///User choice of file name stem and type
    SaveFileDialogParameter m_saveFileAs;

    ///Create a cached factory for faster image saving
    std::shared_ptr<image::tile::Factory> m_cached_output_factory;

private:
    std::string m_name;
	/// The output result
	algorithm::TextResult m_output_text;
	OverlayResult m_results;
	std::string m_type;

    std::vector<std::string> m_saveFileExtensionText;
};
} //namespace algorithm
} //namespace sedeen

#endif // ifndef SEDEEN_SRC_PLUGINS_BOXDROP_BOXDROP_H
