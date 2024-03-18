#include <vector>
#include <string>
#include <iostream>
#include <fcntl.h> 
#include <fstream>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/ioctl.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <sstream>
struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path);

int main() {
    cv::Mat img = cv::imread("example001.png");
    cv::Mat gray, edge;

    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
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
        std::ifstream configFile("config.txt");
        int min_area, max_area;
        float min_ratio, max_ratio;
        std::string label;

        while (configFile >> label >> min_area >> max_area >> min_ratio >> max_ratio) {
            if (rectArea >= min_area && max_area >= rectArea && ratio >= min_ratio && max_ratio >= ratio) {
                cv::rectangle(img, boundingBox, cv::Scalar(0, 255, 0), 2);
                cv::putText(img, label, cv::Point(boundingBox.x, boundingBox.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
            }
        }

        configFile.close();

    }

    cv::imwrite("result.png", img);
    cv::Mat image;
    cv::Size2f image_size;
    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");

    image = cv::imread("result.png");
    image_size = image.size();

    cv::Mat imconvert;
    cv::cvtColor(image, imconvert, cv::COLOR_BGR2BGR565);
    

    for (int y = 0; y < image_size.height; y++)
    {
        // move to the next written position of output device framebuffer by "std::ostream::seekp()".
        // posisiotn can be calcluated by "y", "fb_info.xres_virtual", and "fb_info.bits_per_pixel".
        // http://www.cplusplus.com/reference/ostream/ostream/seekp/
        int position = y * fb_info.xres_virtual * fb_info.bits_per_pixel / 8;
        ofs.seekp(position);
        ofs.write(reinterpret_cast<const char*>(imconvert.ptr(y)), fb_info.xres_virtual * fb_info.bits_per_pixel / 8);
    }

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