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
 
 // Primary header
#include "BoxDrop.h"

// System headers
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <fstream>

//DPTK headres
#include "Algorithm.h"
#include "Geometry.h"
#include "Global.h"
#include "Image.h"
#include "image/io/Image.h"
#include "image/tile/Factory.h"

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
    : m_region_toProcess(),
    m_size(),
    m_text(),
    m_rect(),
    m_intermediate_result(),
    m_saveOutputImage(),
    m_saveFileFormat(),
    m_saveFileAs(),
    m_output_text(),
    m_cached_output_factory(nullptr)
{
    //List the extensions that should be included in the save dialog window
    m_saveFileExtensionText.push_back("tif");
    m_saveFileExtensionText.push_back("png");
    m_saveFileExtensionText.push_back("bmp");
    m_saveFileExtensionText.push_back("gif");
    m_saveFileExtensionText.push_back("jpg");
}//end constructor

BoxDrop::~BoxDrop() {
}

void BoxDrop::init(const image::ImageHandle& image) {
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

    //Allow the user to write separated images to file
    m_saveOutputImage = createBoolParameter(*this, "Save Image",
        "If checked, the final image will be saved to a flat image file.",
        true, false);

    //Allow the user to choose where to save the image files
    sedeen::file::FileDialogOptions saveFileDialogOptions = defineSaveFileDialogOptions();
    m_saveFileAs = createSaveFileDialogParameter(*this, "Save As...",
        "The output image will be saved to this file name. If the file name includes an extension of type TIF/PNG/BMP/GIF/JPG, it will override the Save File Format choice.",
        saveFileDialogOptions, true);

	m_output_text = createTextResult(*this, "text Result");
	// Default number of regions to place along the narrowest dimension.
	//const auto DEFAULT_NUM_REGIONS = 8;
	//const auto DEFAULT_SPACING = min_dim / DEFAULT_NUM_REGIONS;

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
		point = static_cast<int>(points[0][0].getX());
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

		s.setGraphics(graphics);
		pipelineChanged = true;
	}
	s.saveToFile();
	return pipelineChanged;
}

void BoxDrop::run()
{
	using namespace image::tile;
    auto source_factory = image()->getFactory();
    //assemble the final report that will go to the output window
    std::string final_report_text("");

    //Check whether any of the GUI controls changed
    bool guiControlsChanged(false);
    guiControlsChanged = (m_text.isChanged()
        || m_region_toProcess.isChanged()
        || m_saveOutputImage.isChanged()
        || m_saveFileAs.isChanged()
        || (nullptr == m_cached_output_factory) );

    //The pipeline uses the center of the m_region_toProcess to define a rectangle
	auto pipeline_changed = buildPipeline();
	if (pipeline_changed && guiControlsChanged)
	{
        std::string text = m_text;
        text = "Cellularity: "+text;
        m_results.drawRectangle(m_rect, m_style, m_name, text);
        m_results.setVisible(true);
        //updateIntermediateResult();

        //Use m_rect to define a cached constrained factory to use as output
        if (!m_rect.isNull()) {
            //Define a GraphicItemBase from the Rectangle

            //Create a constrained factory
            //RegionFactory constrained_factory = std::make_shared<RegionFactory>(source_factory);

            //Create a cached factory from the constrained factory
            //m_cached_output_factory = std::make_shared<Cache>(constrained_factory, RecentCachePolicy(30));
        }

        //Check whether the user wants to write to image files, that the field is not blank,
        //and that the file can be created or written to
        std::string outputFilePath;
        if (m_saveOutputImage == true) {
            //Get the full path file name from the file dialog parameter
            sedeen::algorithm::parameter::SaveFileDialog::DataType fileDialogDataType = this->m_saveFileAs;
            outputFilePath = fileDialogDataType.getFilename();
            //Is the file field blank?
            if (outputFilePath.empty()) {
                m_output_text.sendText("The filename is blank. Please choose a file to save the image to, or uncheck Save Image.");
                return;
            }
            //Does it exist or can it be created, and can it be written to?
            bool validFileCheck = checkFile(outputFilePath, "w");
            if (!validFileCheck) {
                m_output_text.sendText("The file name selected cannot be written to. Please choose another, or check the permissions of the directory.");
                return;
            }
            //Does it have a valid extension? RawImage.save relies on the extension to determine save format
            std::string theExt = getExtension(outputFilePath);
            int extensionIndex = findExtensionIndex(theExt);
            //findExtensionIndex returns -1 if not found
            if (extensionIndex == -1) {
                std::stringstream ss;
                ss << "The extension of the file is not a valid type. The file extension must be: ";
                auto vec = m_saveFileExtensionText;
                for (auto it = vec.begin(); it != vec.end() - 1; ++it) {
                    ss << (*it) << ", ";
                }
                std::string last = vec.back();
                ss << "or " << last << ". Choose a correct file type and try again." << std::endl;
                m_output_text.sendText(ss.str());
                return;
            }

            std::stringstream fileSaveUpdate;
            fileSaveUpdate << "Image saving in progress." << std::endl;
            fileSaveUpdate << "Saving image as " << outputFilePath << std::endl;
            m_output_text.sendText(fileSaveUpdate.str());
    
            //Save the image within the new rectangle
            bool saveResult = SaveFlatImageToFile(outputFilePath);
            //Check whether saving was successful
            std::stringstream ss;
            if (saveResult) {
                fileSaveUpdate << std::endl << "Stain-separated image saved as " << outputFilePath << std::endl;
            }
            else {
                fileSaveUpdate << std::endl << "Saving the stain-separated image failed. Please check the file name and directory permissions." << std::endl;
            }
            final_report_text.append(fileSaveUpdate.str());
        }    
    }

    //Save the new annotation to the session file
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
	xCenter = static_cast<int>(numberOfOverlays);
	s.setGraphics(newGraphics);
	s.saveToFile();
	auto report = generateReport();
    final_report_text.append(report);

	m_output_text.sendText(final_report_text);

	//updateIntermediateResult();

    //kludge. Trying it.
    Session reload_s(path_to_image);
    reload_s.loadFromFile();

    //Ensure that the plugin can run again after user Abort
    if (askedToStop()) {
        m_cached_output_factory.reset();
    }
}//end run

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

///Define the save file dialog options outside of init
sedeen::file::FileDialogOptions BoxDrop::defineSaveFileDialogOptions() {
    sedeen::file::FileDialogOptions theOptions;
    theOptions.caption = "Save separated images as...";
    //theOptions.flags = sedeen::file::FileDialogFlags:: None currently needed
    //theOptions.startDir; //no current preference
    //Define the file type dialog filter
    sedeen::file::FileDialogFilter theDialogFilter;
    theDialogFilter.name = "Image type";
    //Add extensions in m_saveFileExtensionText to theDialogFilter.extensions 
    //Note: std::copy does not work here
    for (auto it = m_saveFileExtensionText.begin(); it != m_saveFileExtensionText.end(); ++it) {
        theDialogFilter.extensions.push_back(*it);
    }
    theOptions.filters.push_back(theDialogFilter);
    return theOptions;
}//end defineSaveFileDialogOptions


bool BoxDrop::SaveFlatImageToFile(const std::string &p) {
    //It is assumed that error checks have already been performed, and that the type is valid
    //In RawImage::save, the used file format is defined by the file extension.
    //Supported extensions are : .tif, .png, .bmp, .gif, .jpg
    std::string outFilePath = p;
    bool imageSaved = false;
    //Access the output from the output factory
    //In this case, the image is not modified
    // Get source image properties
    auto source_factory = image()->getFactory();
    auto outputFactory = source_factory;

    //Create a compositor
    auto compositor = std::make_unique<image::tile::Compositor>(outputFactory);
    //Get the new rectangle
    Size size;
    size.setWidth(m_size);
    size.setHeight(m_size);
    Point point;
    point.setX((int)(m_rect.topLeft().getX()));
    point.setY((int)(m_rect.topLeft().getY()));
    DisplayRegion region = DisplayRegion(Rect(point, size), size);
    //Get the output image defined by the new rectangle
    sedeen::image::RawImage outputImage = 
        compositor->getImage(region.source_region, region.output_size);

    //Save the outputImage to a file at the given location
    imageSaved = outputImage.save(outFilePath);
    //true on successful save, false otherwise
    return imageSaved;
}//end SaveFlatImageToFile


const std::string BoxDrop::getExtension(const std::string &p) {
    namespace fs = std::filesystem; //an alias
    const std::string errorVal = std::string(); //empty
    //Convert the string to a filesystem::path
    fs::path filePath(p);
    //Does the filePath have an extension?
    bool hasExtension = filePath.has_extension();
    if (!hasExtension) { return errorVal; }
    //else
    fs::path ext = filePath.extension();
    return ext.string();
}//end getExtension

const int BoxDrop::findExtensionIndex(const std::string &x) const {
    const int notFoundVal = -1; //return -1 if not found
    //This method works if the extension has a leading . or not
    std::string theExt(x);
    auto range = std::find(theExt.begin(), theExt.end(), '.');
    theExt.erase(range);
    //Find the extension in the m_saveFileExtensionText vector
    auto vec = m_saveFileExtensionText;
    auto vecIt = std::find(vec.begin(), vec.end(), theExt);
    if (vecIt != vec.end()) {
        ptrdiff_t vecDiff = vecIt - vec.begin();
        int extLoc = static_cast<int>(vecDiff);
        return extLoc;
    }
    else {
        return notFoundVal;
    }
}//end fileExtensionIndex

bool BoxDrop::checkFile(const std::string &fileString, const std::string &op) {
    namespace fs = std::filesystem;
    const bool success = true;
    const bool errorVal = false;
    if (fileString.empty()) { return errorVal; }
    //Convert the input fileString into a filesystem::path type
    fs::path theFilePath = fs::path(fileString);
    //Check if the file exists 
    bool fileExists = fs::exists(theFilePath);

    //If op is set to "r" (read), check that the file can be opened for reading
    if (!op.compare("r") && fileExists) {
        std::ifstream inFile(fileString.c_str());
        bool result = inFile.good();
        inFile.close();
        return result;
    }
    //If op is set to "w" (write) and the file exists, check without overwriting
    else if (!op.compare("w") && fileExists) {
        //Open for appending (do not overwrite current contents)
        std::ofstream outFile(fileString.c_str(), std::ios::app);
        bool result = outFile.good();
        outFile.close();
        return result;
    }
    //If op is set to "w" (write) and the file does not exist, check if the directory exists
    else if (!op.compare("w") && !fileExists) {
        fs::path parentPath = theFilePath.parent_path();
        bool dirExists = fs::is_directory(parentPath);
        //If it does not exist, return errorVal
        if (!dirExists) { return errorVal; }
        //If it exists, does someone (anyone) have write permission?
        fs::file_status dirStatus = fs::status(parentPath);
        fs::perms dPerms = dirStatus.permissions();
        bool someoneCanWrite = ((dPerms & fs::perms::owner_write) != fs::perms::none)
            || ((dPerms & fs::perms::group_write) != fs::perms::none)
            || ((dPerms & fs::perms::others_write) != fs::perms::none);
        return dirExists && someoneCanWrite;
    }
    else {
        return errorVal;
    }
}//end checkFile

} // namespace algorithm
} // namespace sedeen