#include "spatial_domain_filtering.h"
#include "../common/convolution_mask/average_filtering_convolution.h"

void spatial_domain_filtering::read_file(){
    std::ifstream file(file_path_, std::ios::binary);
    if (!file) {
        std::cout << "failed to read the BMP file." << std::endl;
        return;
    }

    // 1. 读取位图文件头和信息头
    char* header_buffer = new char[FILEHEADER_SIZE + INFOHEDER_SIZE];
    file.read(header_buffer, FILEHEADER_SIZE + INFOHEDER_SIZE);

    bmp_file_.deserialize_file_header(header_buffer);
    bmp_file_.deserialize_info_header(header_buffer + FILEHEADER_SIZE);
    delete header_buffer;

    // 2. 如果有调色板，读取调色板
    // 如果颜色位数是8
    if(bmp_file_.get_bit_count() == 8){
        int32_t size_of_queue = sizeof(RGBQUAD)*(256);

        char* queue_buffer = new char[size_of_queue];
        file.read(queue_buffer, size_of_queue);

        
        bmp_file_.set_queue(reinterpret_cast<RGBQUAD *>(queue_buffer));
        
    }else{
        
    }

    // 3. 读取真实数据
    char *data = new char[bmp_file_.get_size_image()];
    bmp_file_.set_data(data);
    file.seekg(bmp_file_.get_bf_off_bits(), std::ios::beg);
    file.read(data, bmp_file_.get_size_image());


    // 

    // 关闭文件资源
    file.close();
}

void spatial_domain_filtering::do_filtering(int border){
    // 颜色位数
    uint16_t bit_count = bmp_file_.get_bit_count();
    uint8_t unit = bit_count / 8;
    // order不能超过原位图数据的大小
    int32_t width = bmp_file_.get_width();
    int32_t height = bmp_file_.get_height();
    if(border > height/2 || border > width/2){
        throw std::exception();
    }

    // 对原图像数据进行扩充
    uint8_t* data = reinterpret_cast<uint8_t*>(bmp_file_.get_data());
    int32_t line_bytes = (width * 8 / 8 + 3) / 4 * 4;

    int32_t image_size = bmp_file_.get_size_image();
    // 有的图片读取不出image_size?
    if(image_size == 0){
        image_size = line_bytes * height;
    }

    // 根据border计算出扩充的位图数据
    int32_t width_copy = width + 2 * unit * border;
    int32_t height_copy = height + 2 * unit * border;
    int32_t line_bytes_copy = (width_copy *8 / 8 + 3) / 4 * 4;
    int32_t image_size_copy = line_bytes_copy * height_copy;
    uint8_t *data_copy = new uint8_t[image_size_copy];
    // 定义一个用来保存经过滤波处理后的新的位图数据
    uint8_t *data_filter = new uint8_t[image_size];

    int32_t width_copy_r = width_copy - (unit - 1);
    // 将新的扩充的数据中的非边界部分直接赋值为原来的位图数据
    for(uint32_t i = border; i < height_copy - border; i ++){
        for(uint32_t j = unit * border;j < width_copy_r - unit * border ;j+= unit){
            for(uint32_t k = 0 ; k < unit; k++){
                *(data_copy + i*line_bytes_copy + j + k) = *(data + (i - border)*line_bytes + (j + k - border));
            }
            
        }
    }

    // 考虑扩充的位图数据的边界数据
    // 上下边界
    for(uint32_t height_t = 0; height_t < border; height_t++){
        for(uint32_t width_t = unit * border; width_t < width_copy_r - unit * border; width_t += unit){
            for(uint32_t k = 0 ; k < unit; k++){
                *(data_copy + height_t * line_bytes_copy + width_t + k) = *(data_copy + (height_t + border) * line_bytes_copy + width_t + k);
            }
        }
    }

    for(uint32_t height_t = height_copy ; height_t > height_copy - border; height_t --){
        for(uint32_t width_t = unit * border ; width_t < width_copy_r - unit * border; width_t += unit){
            for(uint32_t k = 0; k < unit; k++){
                *(data_copy + height_t * line_bytes_copy + width_t + k) = *(data_copy + (height_t - border) * line_bytes_copy + width_t + k);
            }
        }
    }
    // 左右边界
    for(uint32_t height_t = border; height_t < height_copy - border; height_t ++){
        for(uint32_t width_t = 0; width_t < border * unit; width_t += unit){
            for(uint32_t k = 0; k < unit; k++){
                *(data_copy + height_t * line_bytes_copy + width_t + k) = *(data_copy + height_t * line_bytes_copy + width_t + border + k);
            }
        }
    }

    for(uint32_t height_t = border; height_t < height_copy - border; height_t ++){
        for(uint32_t width_t = width_copy_r; width_t > width_copy_r - border * unit; width_t -= unit){
            for(uint32_t k = 0; k < unit; k++){
                *(data_copy + height_t * line_bytes_copy + width_t + k) = *(data_copy + height_t * line_bytes_copy + width_t - border + k);
            }
        }
    }
    // 四个边角
    // 左下角
    for(uint32_t height_t = 0; height_t < border; height_t ++){
        for(uint32_t width_t = 0; width_t < border * unit; width_t += unit){
            for(uint32_t k = 0; k < unit; k++){
                *(data_copy + height_t * line_bytes_copy + width_t + k) = *(data_copy + (height_t + border) * line_bytes_copy + width_t + border + k);
            }
        }
    }
    // 右下角
    for(uint32_t height_t = 0; height_t < border; height_t ++){
        for(uint32_t width_t = width_copy_r ; width_t > width_copy_r - border * unit; width_t -= unit){
            for(uint32_t k = 0; k < unit; k++){
                *(data_copy + height_t * line_bytes_copy + width_t + k) = *(data_copy + (height_t + border) * line_bytes_copy + width_t - border + k);
            }
        }
    }
    // 左上角
    for(uint32_t height_t = height_copy; height_t > height_copy - border; height_t --){
        for(uint32_t width_t = 0; width_t < border * unit; width_t ++){
            for(uint32_t k = 0; k < unit; k++){
                *(data_copy + height_t * line_bytes_copy + width_t + k) = *(data_copy + (height_t - border) * line_bytes_copy + width_t + border + k);
            }
        }
    }
    // 右上角
    for(uint32_t height_t = height_copy; height_t > height_copy - border; height_t --){
        for(uint32_t width_t = width_copy_r; width_t > width_copy_r - border * unit; width_t -= unit){
            for(uint32_t k = 0; k < unit; k++){
                *(data_copy + height_t * line_bytes_copy + width_t + k) = *(data_copy + (height_t - border) * line_bytes_copy + width_t - border + k);
            }
        }
    }

    // 通过扩充后的边界值进行卷积运算
    // 将卷积运算的结果赋值给新的位图数组
    int32_t size_mask = mask_->mask_height_ * mask_->mask_width_;
    for(uint32_t height_c = border; height_c < height_copy - border; height_c ++){
        for(uint32_t width_c = border*unit; width_c < width_copy_r - border*unit; width_c += unit){
            
            for(uint32_t k = 0; k < unit; k++){
                uint8_t *data_start = new uint8_t[size_mask];
                for(int32_t i = 0; i< mask_->mask_height_; ++i){
                    for(int32_t j = 0; j < mask_->mask_width_; ++j){
                        memcpy(data_start + i*mask_->mask_width_*sizeof(uint8_t) + j, data_copy + line_bytes_copy* (height_c + i) + width_c + j * unit + k - line_bytes_copy - unit , sizeof(uint8_t));
                    }
                    
                } 
                uint8_t res = mask_->convolution_compute(data_start) ;
                *(data_filter + line_bytes * (height_c - border) + width_c - border * unit + k) = res;
                delete[] data_start;
            }
            
        }
    }

    // 将新的位图数据替换并写入文件

    // 在当前file_path下加一个_average_filtering后缀
    size_t pos = file_path_.find_last_of(".");
    if(pos == std::string::npos) {
        throw std::exception();
    }
    std::string file_name = file_path_.substr(0, pos);
    std::string file_ext = file_path_.substr(pos + 1);


    file_name += "_" + get_type() + "_filtering";
    std::string save_file_path = file_name + "." + file_ext;

    // 将位图文件头和位图信息头保存到文件中
    std::ofstream file(save_file_path, std::ios::binary);
    if (!file) {
        std::cout << "failed to create the BMP file to save." << std::endl;
        return;
    }

    // 将文件头和调色板的信息写入
    BITMAPFILEHEADER file_header = bmp_file_.get_file_header();
    BITMAPINFOHEADER info_header = bmp_file_.get_info_header();
    file.write(reinterpret_cast<const char*>(&file_header), FILEHEADER_SIZE);
    file.write(reinterpret_cast<const char*>(&info_header), INFOHEDER_SIZE);

    // 24色和非24色单独处理
    if(bmp_file_.get_queue() != nullptr){
        file.write(reinterpret_cast<const char*>(bmp_file_.get_queue()), sizeof(RGBQUAD)*(256));
    }
    
    // 将新的位图数据写入
    file.write(reinterpret_cast<const char*>(data_filter), sizeof(uint8_t)*(image_size));

    // file.write(reinterpret_cast<const char*>(data_copy), sizeof(uint8_t)*(image_size));

    // 释放内存资源
    // delete[] data_copy;
    // delete[] data_filter;

    // 关闭文件资源
    file.close();
}