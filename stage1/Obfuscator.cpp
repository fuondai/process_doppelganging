#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// Khóa mã hóa - có thể thay đổi theo ý muốn
const unsigned char KEY[] = {
    0x89, 0x45, 0x32, 0xFA, 0xB7, 0x21, 0xD0, 0xE5,
    0x77, 0x19, 0xAC, 0x8B, 0x5D, 0x33, 0x69, 0xF1};

// Mã hóa dữ liệu sử dụng phương pháp XOR + byte rotation
void EncryptData(std::vector<unsigned char> &data)
{
    const size_t keyLength = sizeof(KEY);

    // XOR với khóa
    for (size_t i = 0; i < data.size(); i++)
    {
        data[i] ^= KEY[i % keyLength];
    }

    // Rotation từng byte (ROL)
    for (size_t i = 0; i < data.size(); i++)
    {
        data[i] = (data[i] << 3) | (data[i] >> 5); // Rotate left 3 bits
    }

    // Thêm một lớp nữa để làm phức tạp
    for (size_t i = 0; i < data.size() - 1; i++)
    {
        data[i] ^= (data[i + 1] ^ 0xA5);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: Obfuscator.exe <input_file> <output_file>" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    // Đọc file đầu vào
    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Error: Cannot open input file!" << std::endl;
        return 1;
    }

    // Đọc toàn bộ nội dung vào vector
    std::vector<unsigned char> fileData(
        (std::istreambuf_iterator<char>(inFile)),
        (std::istreambuf_iterator<char>()));
    inFile.close();

    // Mã hóa dữ liệu
    EncryptData(fileData);

    // Ghi file đã mã hóa
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile)
    {
        std::cerr << "Error: Cannot create output file!" << std::endl;
        return 1;
    }

    outFile.write(reinterpret_cast<const char *>(fileData.data()), fileData.size());
    outFile.close();

    std::cout << "Successfully encrypted " << inputPath << " to " << outputPath << std::endl;
    std::cout << "Original size: " << fileData.size() << " bytes" << std::endl;

    return 0;
}