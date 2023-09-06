#include "bmp.h"


const int FILEHEADER_SIZE = sizeof(BITMAPFILEHEADER);
const int INFOHEDER_SIZE = sizeof(BITMAPINFOHEADER);

class image_process{
public:
    image_process(std::string file_path) : file_path_(file_path){}

    void read_file();

    void convert_to_gray();

    
private:
    std::string file_path_;  // 要处理的文件路径
    bmp bmp_file_;           // 待处理的bmp文件
    bmp save_file_;          // 处理后要保存的bmp文件
};