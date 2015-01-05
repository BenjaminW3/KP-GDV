
#include <kinjo/vision/OpenNiVision.hpp>

#include <iostream>
#include <cstdint>		// std::uint16_t

namespace kinjo {
    namespace vision {

		OpenNiVision::OpenNiVision()
		{
			status = context.Init();
			checkStatus("Initializing context");

			status = depthGenerator.Create(context);
			checkStatus("Creating depth generator");

			status = rgbGenerator.Create(context);
			checkStatus("Creating RGB generator");

			status = context.StartGeneratingAll();
			checkStatus("Starting generation");

			depthImageSize = getImageSize(depthGenerator);
			rgbImageSize = getImageSize(rgbGenerator);

			// Align the depth image center to the color image center.
			depthGenerator.GetAlternativeViewPointCap().SetViewPoint(rgbGenerator);
		}

		void OpenNiVision::updateImages(bool bRequireUpdates)
		{
			// Update the internal generator buffers if there is new data.
			if(bRequireUpdates)
			{
				status = context.WaitAndUpdateAll();
			}
			else
			{
				status = context.WaitNoneUpdateAll();
			}
			checkStatus("Updating data");

			// Update the depth data.
			const XnDepthPixel* depthPixels = depthGenerator.GetDepthMap();
			matDepth = cv::Mat(depthImageSize, CV_16UC1, const_cast<void*>(reinterpret_cast<void const *>(depthPixels)));

			// Update the color data.
			const XnRGB24Pixel* rgbPixels = rgbGenerator.GetRGB24ImageMap();
			cv::Mat bgr(rgbImageSize, CV_8UC3, const_cast<void*>(reinterpret_cast<void const *>(rgbPixels)));
			cv::cvtColor(bgr, matRgb, CV_RGB2BGR);
		}

		cv::Mat const & OpenNiVision::getDepth()
		{
			return matDepth;
		}

		cv::Mat const & OpenNiVision::getRgb()
		{
			return matRgb;
		}

		cv::Vec3f OpenNiVision::getPositionFromImagePointPx(
			cv::Point const & v2iPointPx)
		{
			// Look up the depth at this position.
			std::uint16_t const uiDepth(getDepth().at<std::uint16_t>(v2iPointPx.y, v2iPointPx.x));
			// FIXME: How to get the real x,y coordinates in vision?.
			return cv::Vec3f(
				static_cast<float>(v2iPointPx.x),
				static_cast<float>(v2iPointPx.y),
				(uiDepth==0) 
					? 0.0f 
					: (static_cast<float>(uiDepth)/10.0f));
		}

		void OpenNiVision::checkStatus(std::string action) const
		{
			if (status != XN_STATUS_OK) {
				std::string cause = xnGetStatusString(status);
				std::stringstream ss;
				ss << action << " failed: " << cause << std::endl;
				throw std::runtime_error(ss.str().c_str());
			}
		}

		cv::Size OpenNiVision::getImageSize(xn::MapGenerator& generator) const
		{
			XnMapOutputMode output;
			generator.GetMapOutputMode(output);
			return cv::Size(output.nXRes, output.nYRes);
		}

    }
}

