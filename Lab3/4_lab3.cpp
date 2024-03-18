#include <iostream>
#include <fcntl.h> 
#include <fstream>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/ioctl.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <termios.h>
#include <unistd.h>
#include <sstream>




void setRawInput() {
    termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    
    // Switch to raw mode
    tty.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

bool flag = false;
using namespace std;

struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path);

int main(int argc, const char *argv[])
{
    int count = 0;
    setRawInput();
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    char c;
    cv::Mat frame;
    cv::Size2f frame_size;
    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");
    int fourcc = cv::VideoWriter::fourcc('M','J','P','G'); 
    
    // Set output video properties
    cv::VideoWriter outVideo;

    
    
    bool flag=false;
    // Create a VideoWriter object
    // You may want to modify "/run/media/mmcblk1p2/video_output.avi" to the desired output path and file name
    
    cv::VideoCapture cap(2);
    int to_shot = 0;
    double fps = 10;
    std::cout << "Frames per second: " << fps << std::endl;
    while(true)
    {
        cap >> frame;
        frame_size = frame.size();
        if(!flag){
        outVideo.open("video_output.avi", fourcc, fps, frame_size, true);
        flag = true;
        }// cout<<frame_size.width<<std::endl;
        // cout<<fb_info.xres_virtual<<std::endl;
        //break;

        cv::Mat converted_frame;
        cv::cvtColor(frame, converted_frame, cv::COLOR_BGR2BGR565);
        outVideo<<frame;
        for (int y = 0; y < frame_size.height; y++)
        {
            int position = y * (fb_info.xres_virtual) * fb_info.bits_per_pixel / 8 + 160;
            //int position = y * (fb_info.xres_virtual) * 2 + 160;
            ofs.seekp(position);
            //ofs.write(reinterpret_cast<const char*>(converted_frame.ptr(y)), fb_info.xres_virtual * fb_info.bits_per_pixel / 8);
            ofs.write(reinterpret_cast<const char*>(converted_frame.ptr(y)), frame_size.width * fb_info.bits_per_pixel/8);
        }
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == 'c') {
            // Convert back to BGR format from BGR565 before saving if necessary
            
            // No need to convert if the original frame is already in BGR format
            // Just assign it directly to save_frame
            
            std::stringstream ss;
            ss << count;
            std::string nn = ss.str();
            std::string filename = "/run/media/mmcblk1p2/screenshot/screenshot_" + nn + ".bmp";
            
            // Now save the BGR image in BMP format
            cv::imwrite(filename, frame);
            count++;
        }
            else if (c == 'b')
                break;
        }

    }
    outVideo.release();
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