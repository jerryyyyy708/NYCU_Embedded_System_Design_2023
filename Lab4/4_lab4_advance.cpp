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
#include <vector>

struct termios original_termios;


void setRawInput() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &original_termios); // Save original settings

    tty = original_termios; // Start with a copy of the original settings
    
    // Switch to raw mode
    tty.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void restoreOriginalInput() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios); // Restore original settings
}

struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path);

int main(int argc, const char *argv[])
{
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    setRawInput();
    int direction = 1;
    cv::Mat image;
    cv::Size2f image_size;
    std::cout<<"hi1";
    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");

    // read image file (sample.bmp) from opencv libs.
    // https://docs.opencv.org/3.4.7/d4/da8/group__imgcodecs.html#ga288b8b3da0892bd651fce07b3bbd3a56
    //image = cv::imread("NYCU_logo.bmp");
    image = cv::imread("picture.png",cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        std::cout << "No image" << std::endl;
        return 1;  // Return an error code
    }
    
    std::cout<<"hi1";
    // get image size of the image.
    // https://docs.opencv.org/3.4.7/d3/d63/classcv_1_1Mat.html#a146f8e8dda07d1365a575ab83d9828d1
    image_size = image.size();
    std::cout<<"hi2";
    // transfer color space from BGR to BGR565 (16-bit image) to fit the requirement of the LCD
    // https://docs.opencv.org/3.4.7/d8/d01/group__imgproc__color__conversions.html#ga397ae87e1288a81d2363b61574eb8cab
    // https://docs.opencv.org/3.4.7/d8/d01/group__imgproc__color__conversions.html#ga4e0972be5de079fed4e3a10e24ef5ef0
    cv::Mat imconvert;
    cv::cvtColor(image, imconvert, cv::COLOR_BGR2BGR565);
    std::cout<<"hi3";
    // output to framebufer row by row
    int cur = 0;
    int cut = 384;
    char c;
    while(true){
        //sleep(1);
        std::cout<<cur<<std::endl;

        // 計算每段的寬度
        int slice_width = imconvert.cols / cut * cur;
        if(slice_width == 0)
            slice_width = 1;
        
        // 定義要移動的部分（最左側的1/24）
        cv::Mat slice = imconvert(cv::Rect(0, 0, slice_width, imconvert.rows));

        // 定義剩餘的部分
        cv::Mat remaining = imconvert(cv::Rect(slice_width, 0, imconvert.cols - slice_width, imconvert.rows));

        // 創建一個新圖像來組合這些部分
        cv::Mat swapped_image;
        swapped_image.create(imconvert.rows, imconvert.cols, imconvert.type());

        // 首先放入剩餘部分
        remaining.copyTo(swapped_image(cv::Rect(0, 0, remaining.cols, remaining.rows)));

        // 然後將slice放在最右側
        slice.copyTo(swapped_image(cv::Rect(remaining.cols, 0, slice_width, slice.rows)));

   
        for (int y = 0; y < image_size.height; y++)
        {
            // move to the next written position of output device framebuffer by "std::ostream::seekp()".
            // posisiotn can be calcluated by "y", "fb_info.xres_virtual", and "fb_info.bits_per_pixel".
            // http://www.cplusplus.com/reference/ostream/ostream/seekp/
            int position = y * fb_info.xres_virtual * fb_info.bits_per_pixel / 8;
            ofs.seekp(position);
            // write to the framebuffer by "std::ostream::write()".
            // you could use "cv::Mat::ptr()" to get the pointer of the corresponding row.
            // you also have to count how many bytes to write to the buffer
            // http://www.cplusplus.com/reference/ostream/ostream/write/
            // https://docs.opencv.org/3.4.7/d3/d63/classcv_1_1Mat.html#a13acd320291229615ef15f96ff1ff738
            ofs.write(reinterpret_cast<const char*>(swapped_image.ptr(y)), fb_info.xres_virtual * fb_info.bits_per_pixel / 8);
        }

        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == 'j') {
                direction = 1;
            }
            else if(c == 'l'){
                direction = -1;
            }
            else if (c == 'b')
                break;
        }
        if(direction == 1){
        cur++;
        if(cur == cut)
            cur = 0;
        }
        else{
            cur--;
            if(cur == -1){
                cur = cut-1;
            }
        }
        
        
    }
    restoreOriginalInput();
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