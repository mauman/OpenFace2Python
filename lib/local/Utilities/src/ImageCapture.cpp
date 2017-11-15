///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017, Tadas Baltrusaitis, all rights reserved.
//
// ACADEMIC OR NON-PROFIT ORGANIZATION NONCOMMERCIAL RESEARCH USE ONLY
//
// BY USING OR DOWNLOADING THE SOFTWARE, YOU ARE AGREEING TO THE TERMS OF THIS LICENSE AGREEMENT.  
// IF YOU DO NOT AGREE WITH THESE TERMS, YOU MAY NOT USE OR DOWNLOAD THE SOFTWARE.
//
// License can be found in OpenFace-license.txt
//
//     * Any publications arising from the use of this software, including but
//       not limited to academic journal and conference publications, technical
//       reports and manuals, must cite at least one of the following works:
//
//       OpenFace: an open source facial behavior analysis toolkit
//       Tadas Baltrušaitis, Peter Robinson, and Louis-Philippe Morency
//       in IEEE Winter Conference on Applications of Computer Vision, 2016  
//
//       Rendering of Eyes for Eye-Shape Registration and Gaze Estimation
//       Erroll Wood, Tadas Baltrušaitis, Xucong Zhang, Yusuke Sugano, Peter Robinson, and Andreas Bulling 
//       in IEEE International. Conference on Computer Vision (ICCV),  2015 
//
//       Cross-dataset learning and person-speci?c normalisation for automatic Action Unit detection
//       Tadas Baltrušaitis, Marwa Mahmoud, and Peter Robinson 
//       in Facial Expression Recognition and Analysis Challenge, 
//       IEEE International Conference on Automatic Face and Gesture Recognition, 2015 
//
//       Constrained Local Neural Fields for robust facial landmark detection in the wild.
//       Tadas Baltrušaitis, Peter Robinson, and Louis-Philippe Morency. 
//       in IEEE Int. Conference on Computer Vision Workshops, 300 Faces in-the-Wild Challenge, 2013.    
//
///////////////////////////////////////////////////////////////////////////////

#include "ImageCapture.h"

#include <iostream>

// Boost includes
#include <filesystem.hpp>
#include <filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>

// OpenCV includes
#include <opencv2/imgproc.hpp>

using namespace Utilities;

#define INFO_STREAM( stream ) \
std::cout << stream << std::endl

#define WARN_STREAM( stream ) \
std::cout << "Warning: " << stream << std::endl

#define ERROR_STREAM( stream ) \
std::cout << "Error: " << stream << std::endl

bool ImageCapture::Open(std::vector<std::string>& arguments)
{

	// Consuming the input arguments
	bool* valid = new bool[arguments.size()];

	for (size_t i = 0; i < arguments.size(); ++i)
	{
		valid[i] = true;
	}

	// Some default values
	std::string input_root = "";
	fx = -1; fy = -1; cx = -1; cy = -1;
	frame_num = 0;

	std::string separator = std::string(1, boost::filesystem::path::preferred_separator);

	// First check if there is a root argument (so that videos and input directories could be defined more easily)
	for (size_t i = 0; i < arguments.size(); ++i)
	{
		if (arguments[i].compare("-root") == 0)
		{
			input_root = arguments[i + 1] + separator;
			i++;
		}
		if (arguments[i].compare("-inroot") == 0)
		{
			input_root = arguments[i + 1] + separator;
			i++;
		}
	}

	std::string input_directory;
	std::string bbox_directory;

	bool directory_found = false;
	has_bounding_boxes = false;

	std::vector<std::string> input_image_files;

	for (size_t i = 0; i < arguments.size(); ++i)
	{
		if (arguments[i].compare("-f") == 0)
		{
			input_image_files.push_back(input_root + arguments[i + 1]);
			valid[i] = false;
			valid[i + 1] = false;
			i++;
		}
		else if (arguments[i].compare("-fdir") == 0)
		{
			if (directory_found)
			{
				WARN_STREAM("Input directory already found, using the first one:" + input_directory);
			}
			else 
			{
				input_directory = (input_root + arguments[i + 1]);
				valid[i] = false;
				valid[i + 1] = false;
				i++;
				directory_found = true;
			}
		}
		else if (arguments[i].compare("-bboxdir") == 0)
		{
			bbox_directory = (input_root + arguments[i + 1]);
			valid[i] = false;
			valid[i + 1] = false;
			has_bounding_boxes = true;
			i++;
		}
		else if (arguments[i].compare("-fx") == 0)
		{
			std::stringstream data(arguments[i + 1]);
			data >> fx;
			i++;
		}
		else if (arguments[i].compare("-fy") == 0)
		{
			std::stringstream data(arguments[i + 1]);
			data >> fy;
			i++;
		}
		else if (arguments[i].compare("-cx") == 0)
		{
			std::stringstream data(arguments[i + 1]);
			data >> cx;
			i++;
		}
		else if (arguments[i].compare("-cy") == 0)
		{
			std::stringstream data(arguments[i + 1]);
			data >> cy;
			i++;
		}
	}

	for (int i = (int)arguments.size() - 1; i >= 0; --i)
	{
		if (!valid[i])
		{
			arguments.erase(arguments.begin() + i);
		}
	}

	// Based on what was read in open the sequence
	if (!input_image_files.empty())
	{
		return OpenImageFiles(input_image_files, fx, fy, cx, cy);
	}
	if (!input_directory.empty())
	{
		return OpenDirectory(input_directory, bbox_directory, fx, fy, cx, cy);
	}

	// If no input found return false and set a flag for it
	no_input_specified = true;

	return false;
}

// TODO proper destructors and move constructors

bool ImageCapture::OpenImageFiles(const std::vector<std::string>& image_files, float fx, float fy, float cx, float cy)
{

	no_input_specified = false;

	latest_frame = cv::Mat();
	latest_gray_frame = cv::Mat();
	this->image_files = image_files;

	// Allow for setting the camera intrinsics, but have to be the same ones for every image
	if (fx != -1 && fy != -1 && cx != -1 && cy != -1)
	{
		image_intrinsics_set = true;
		this->fx = fx;
		this->fy = fy;
		this->cx = cx;
		this->cy = cy;
	}
	else
	{
		image_intrinsics_set = false;
	}
	return true;

}

bool ImageCapture::OpenDirectory(std::string directory, std::string bbox_directory, float fx, float fy, float cx, float cy)
{
	INFO_STREAM("Attempting to read from directory: " << directory);

	no_input_specified = false;

	image_files.clear();

	boost::filesystem::path image_directory(directory);
	std::vector<boost::filesystem::path> file_in_directory;
	copy(boost::filesystem::directory_iterator(image_directory), boost::filesystem::directory_iterator(), back_inserter(file_in_directory));

	// Sort the images in the directory first
	sort(file_in_directory.begin(), file_in_directory.end());

	std::vector<std::string> curr_dir_files;

	for (std::vector<boost::filesystem::path>::const_iterator file_iterator(file_in_directory.begin()); file_iterator != file_in_directory.end(); ++file_iterator)
	{
		// Possible image extension .jpg and .png
		if (file_iterator->extension().string().compare(".jpg") == 0 || file_iterator->extension().string().compare(".jpeg") == 0 || file_iterator->extension().string().compare(".png") == 0 || file_iterator->extension().string().compare(".bmp") == 0)
		{
			curr_dir_files.push_back(file_iterator->string());

			// If bounding box directory is specified, read the bounding boxes from it
			if (!bbox_directory.empty())
			{
				boost::filesystem::path current_file = *file_iterator;
				boost::filesystem::path bbox_file = current_file.replace_extension("txt");

				// If there is a bounding box file push it to the list of bounding boxes
				if (boost::filesystem::exists(bbox_file))
				{
					std::ifstream in_bbox(bbox_file.string().c_str(), std::ios_base::in);

					std::vector<cv::Rect_<double> > bboxes_image;
					while (!in_bbox.eof())
					{
						double min_x, min_y, max_x, max_y;

						in_bbox >> min_x >> min_y >> max_x >> max_y;
						bboxes_image.push_back(cv::Rect_<double>(min_x, min_y, max_x - min_x, max_y - min_y));
					}
					in_bbox.close();

					bounding_boxes.push_back(bboxes_image);
				}
				else
				{
					ERROR_STREAM("Could not find the corresponding bounding box for file:" + file_iterator->string());
					exit(1);
				}
			}
		}
	}

	image_files = curr_dir_files;

	if (image_files.empty())
	{
		std::cout << "No images found in the directory: " << directory << std::endl;
		return false;
	}

	// Allow for setting the camera intrinsics, but have to be the same ones for every image
	if (fx != -1 && fy != -1 && cx != -1 && cy != -1)
	{
		image_intrinsics_set = true;
		this->fx = fx;
		this->fy = fy;
		this->cx = cx;
		this->cy = cy;
	}
	else
	{
		image_intrinsics_set = false;
	}
	return true;

}

// TODO this should be shared somewhere
void convertToGrayscale(const cv::Mat& in, cv::Mat& out)
{
	if (in.channels() == 3)
	{
		// Make sure it's in a correct format
		if (in.depth() != CV_8U)
		{
			if (in.depth() == CV_16U)
			{
				cv::Mat tmp = in / 256;
				tmp.convertTo(tmp, CV_8U);
				cv::cvtColor(tmp, out, CV_BGR2GRAY);
			}
		}
		else
		{
			cv::cvtColor(in, out, CV_BGR2GRAY);
		}
	}
	else if (in.channels() == 4)
	{
		cv::cvtColor(in, out, CV_BGRA2GRAY);
	}
	else
	{
		if (in.depth() == CV_16U)
		{
			cv::Mat tmp = in / 256;
			out = tmp.clone();
		}
		else if (in.depth() != CV_8U)
		{
			in.convertTo(out, CV_8U);
		}
		else
		{
			out = in.clone();
		}
	}
}

void ImageCapture::SetCameraIntrinsics(float fx, float fy, float cx, float cy)
{
	// If optical centers are not defined just use center of image
	if (cx == -1)
	{
		this->cx = this->image_width / 2.0f;
		this->cy = this->image_height / 2.0f;
	}
	else
	{
		this->cx = cx;
		this->cy = cy;
	}
	// Use a rough guess-timate of focal length
	if (fx == -1)
	{
		this->fx = 500.0f * (this->image_width / 640.0f);
		this->fy = 500.0f * (this->image_height / 480.0f);

		this->fx = (this->fx + this->fy) / 2.0f;
		this->fy = this->fx;
	}
	else
	{
		this->fx = fx;
		this->fy = fy;
	}
}

cv::Mat ImageCapture::GetNextImage()
{
	frame_num++;
	if (image_files.empty() || frame_num - 1 > image_files.size())
	{
		// Indicate lack of success by returning an empty image
		latest_frame = cv::Mat();
	}

	latest_frame = cv::imread(image_files[frame_num - 1], -1);

	if (latest_frame.empty())
	{
		ERROR_STREAM("Could not open the image: " + image_files[frame_num - 1]);
	}

	image_height = latest_frame.size().height;
	image_width = latest_frame.size().width;

	// Reset the intrinsics for every image if they are not set globally
	if (!image_intrinsics_set)
	{
		SetCameraIntrinsics(-1, -1, -1, -1);
	}

	// Set the grayscale frame
	convertToGrayscale(latest_frame, latest_gray_frame);

	this->name = boost::filesystem::path(image_files[frame_num - 1]).filename().replace_extension("").string();

	return latest_frame;
}

std::vector<cv::Rect_<double> > ImageCapture::GetBoundingBoxes()
{
	if (!bounding_boxes.empty())
	{
		return bounding_boxes[frame_num - 1];
	}
	else
	{
		return std::vector<cv::Rect_<double> >();
	}
}

double ImageCapture::GetProgress()
{
	return (double)frame_num / (double)image_files.size();
}

cv::Mat_<uchar> ImageCapture::GetGrayFrame()
{
	return latest_gray_frame;
}