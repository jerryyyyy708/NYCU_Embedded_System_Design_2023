#include <iostream>
#include <fcntl.h> 
#include <fstream>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/ioctl.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <sstream>


bool flag = false;
using namespace std;

struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path);

int main(int argc, char** argv)
{
    std::string config;
    if (argc < 2) {
        config = "config.txt";
    } else {
        config = argv[1];
    }
    int count = 0;
    cv::Mat frame;
    cv::Size2f frame_size;
    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");
    int fourcc = cv::VideoWriter::fourcc('M','J','P','G'); 
    
    // Set output video properties
    cv::VideoWriter outVideo;
    
    bool flag=false;
    
    cv::VideoCapture cap(2);
    cv::Mat gray, edge;
    cv::Mat converted_frame;
    while(true)
    {
        cap >> frame;
        frame_size = frame.size();

        //cv::Mat img;
        //cv::cvtColor(frame, img, cv::COLOR_BGR2BGR565);
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::Canny(gray, edge, 100, 200); //100, 200

        std::vector<std::vector<cv::Point> > contours;
        cv::findContours(edge, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

        //double areaThreshold = 3000.0; 

        for (size_t i = 0; i < contours.size(); i++) {

            cv::Rect boundingBox = cv::boundingRect(contours[i]);
            float width = boundingBox.width;
            float height = boundingBox.height;
            float ratio;
            if (width > height) {
                ratio = width / height;
            }
            else {
                ratio = height / width;
            }

            int rectArea = boundingBox.width * boundingBox.height;

            //load labels
            std::ifstream configFile(config.c_str());
            int min_area, max_area;
            float min_ratio, max_ratio;
            std::string label;
            while (configFile >> label >> min_area >> max_area >> min_ratio >> max_ratio) {
                if (rectArea >= min_area && max_area >= rectArea && ratio >= min_ratio && max_ratio >= ratio) {
                    cv::rectangle(frame, boundingBox, cv::Scalar(0, 255, 0), 2);
                    cv::putText(frame, label, cv::Point(boundingBox.x, boundingBox.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
                }
            }

            configFile.close();

        }
        cv::cvtColor(frame, converted_frame, cv::COLOR_BGR2BGR565);
        for (int y = 0; y < frame_size.height; y++)
        {
            int position = y * (fb_info.xres_virtual) * fb_info.bits_per_pixel / 8 + 160;
            //int position = y * (fb_info.xres_virtual) * 2 + 160;
            ofs.seekp(position);
            //ofs.write(reinterpret_cast<const char*>(converted_frame.ptr(y)), fb_info.xres_virtual * fb_info.bits_per_pixel / 8);
            ofs.write(reinterpret_cast<const char*>(converted_frame.ptr(y)), frame_size.width * fb_info.bits_per_pixel/8);
        }

    }
    
    cap.release();
    return 0;
}

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path)
{
    struct framebuffer_info fb_info;        // Used to return the required attrs.
    struct fb_var_screeninfo screen_info;   // Used to get attributes of the device from OS kernel.

    // open device with linux system call "open()"
    // https://man7.org/linux/man-pages/man2/open.2.html
    int fd = open(framebuffer_device_path, O_RDWR);
    // get attributes of the framebuffer device thorugh linux system call "ioctl()".
    // the command you would need is "FBIOGET_VSCREENINFO"
    // https://man7.org/linux/man-pages/man2/ioctl.2.html
    // https://www.kernel.org/doc/Documentation/fb/api.txt
    ioctl(fd, FBIOGET_VSCREENINFO, &screen_info);
    // put the required attributes in variable "fb_info" you found with "ioctl() and return it."
    // fb_info.xres_virtual =       // 8
    // fb_info.bits_per_pixel =     // 16
    fb_info.xres_virtual = screen_info.xres_virtual;
    fb_info.bits_per_pixel = screen_info.bits_per_pixel;

    close(fd);
    return fb_info;
};