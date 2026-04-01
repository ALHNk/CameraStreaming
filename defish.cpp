#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/calib3d.hpp>
#include <vector>
#include <cstring>

extern "C" int defish(unsigned char* in_data, size_t in_size,
                      unsigned char** out_data, size_t* out_size)
{
    if(in_size == 0)
    {
        return -1;
    }
    
    try
    {
        std::vector<uchar> input(in_data, in_data + in_size);
        cv::Mat img = cv::imdecode(input, cv::IMREAD_COLOR);

        if (img.empty())
            return -1;

        cv::Mat undistorted;

        cv::Mat K = (cv::Mat_<double>(3,3) <<
            img.cols * 0.5, 0, img.cols/2.0,
            0, img.cols * 0.5, img.rows/2.0,
            0, 0, 1);

        cv::Mat D = (cv::Mat_<double>(4,1) << -0.1, 0.05, 0.0, 0.0);

        cv::fisheye::undistortImage(img, undistorted, K, D, K);

        if (undistorted.empty())
            return -1;

        std::vector<uchar> outbuf;
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 85};

        if (!cv::imencode(".jpg", undistorted, outbuf, params) || outbuf.empty())
            return -1;

        *out_size = outbuf.size();
        *out_data = (unsigned char*)malloc(*out_size);

        if (!*out_data)
            return -1;

        memcpy(*out_data, outbuf.data(), *out_size);

        return 0;
    }
    catch (const cv::Exception& e)
    {
        printf("OpenCV error: %s\n", e.what());
        return -1;
    }
}