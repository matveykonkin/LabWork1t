#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <algorithm>

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t file_type;       
    uint32_t file_size;       
    uint16_t reserved1;       
    uint16_t reserved2;       
    uint32_t offset_data;     
};

struct DIBHeader {
    uint32_t header_size;    
    int32_t width;           
    int32_t height;           
    uint16_t planes;          
    uint16_t bit_count;      
    uint32_t compression;     
    uint32_t image_size;      
    int32_t x_pixels_per_meter; 
    int32_t y_pixels_per_meter; 
    uint32_t colors_used;     
    uint32_t important_colors; 
};
#pragma pack(pop)

void readBMP(const std::string& filepath, BMPHeader& bmpHeader, DIBHeader& dibHeader, std::vector<uint8_t>& pixelData) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open file: " + filepath);
    }

    file.read(reinterpret_cast<char*>(&bmpHeader), sizeof(bmpHeader));
    if (bmpHeader.file_type != 0x4D42) { 
        throw std::runtime_error("Not a valid BMP file.");
    }

    file.read(reinterpret_cast<char*>(&dibHeader), sizeof(dibHeader));

    if (dibHeader.bit_count != 24 || dibHeader.compression != 0) {
        throw std::runtime_error("Only 24-bit uncompressed BMP files are supported.");
    }

    pixelData.resize(dibHeader.image_size);
    file.seekg(bmpHeader.offset_data, std::ios::beg);
    file.read(reinterpret_cast<char*>(pixelData.data()), dibHeader.image_size);

    file.close();
}

void writeBMP(const std::string& filepath, const BMPHeader& bmpHeader, const DIBHeader& dibHeader, const std::vector<uint8_t>& pixelData) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to create file: " + filepath);
    }

    file.write(reinterpret_cast<const char*>(&bmpHeader), sizeof(bmpHeader));
    file.write(reinterpret_cast<const char*>(&dibHeader), sizeof(dibHeader));
    file.write(reinterpret_cast<const char*>(pixelData.data()), pixelData.size());

    file.close();
}

std::vector<uint8_t> flip90Clockwise(const DIBHeader& dibHeader, const std::vector<uint8_t>& pixelData) {
    int width = dibHeader.width;
    int height = dibHeader.height;
    int rowSize = ((width * 3 + 3) & ~3); 
    int newRowSize = ((height * 3 + 3) & ~3);

    std::vector<uint8_t> rotatedData(newRowSize * width, 0);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < 3; ++c) { 
                rotatedData[(x * newRowSize) + (height - y - 1) * 3 + c] = pixelData[y * rowSize + x * 3 + c];
            }
        }
    }
    return rotatedData;
}

std::vector<uint8_t> flip90CounterClockwise(const DIBHeader& dibHeader, const std::vector<uint8_t>& pixelData) {
    int width = dibHeader.width;
    int height = dibHeader.height;
    int rowSize = ((width * 3 + 3) & ~3); 
    int newRowSize = ((height * 3 + 3) & ~3);

    std::vector<uint8_t> rotatedData(newRowSize * width, 0);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < 3; ++c) { 
                rotatedData[(x * newRowSize) + y * 3 + c] = pixelData[y * rowSize + (width - x - 1) * 3 + c];
            }
        }
    }
    return rotatedData;
}

std::vector<uint8_t> applyGaussianFilter(const DIBHeader& dibHeader, const std::vector<uint8_t>& pixelData) {
    int width = dibHeader.width;
    int height = dibHeader.height;
    int rowSize = ((width * 3 + 3) & ~3); 

    std::vector<uint8_t> filteredData(pixelData.size(), 0);

    const float kernel[3][3] = {
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f},
        {2 / 16.0f, 4 / 16.0f, 2 / 16.0f},
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f}
    };

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            for (int c = 0; c < 3; ++c) {
                float sum = 0.0f;
                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        sum += kernel[ky + 1][kx + 1] *
                               pixelData[(y + ky) * rowSize + (x + kx) * 3 + c];
                    }
                }
                filteredData[y * rowSize + x * 3 + c] = static_cast<uint8_t>(std::clamp(sum, 0.0f, 255.0f));
            }
        }
    }

    return filteredData;
}

int main() {
    const std::string inputFile = "input.bmp";
    const std::string outputClockwise = "output_90_clockwise.bmp";
    const std::string outputCounterClockwise = "output_90_counterclockwise.bmp";
    const std::string filteredClockwise = "filtered_90_clockwise.bmp";
    const std::string filteredCounterClockwise = "filtered_90_counterclockwise.bmp";

    BMPHeader bmpHeader;
    DIBHeader dibHeader;
    std::vector<uint8_t> pixelData;

    try {
        readBMP(inputFile, bmpHeader, dibHeader, pixelData);

        auto rotatedClockwise = flip90Clockwise(dibHeader, pixelData);
        DIBHeader newDibHeaderClockwise = dibHeader;
        std::swap(newDibHeaderClockwise.width, newDibHeaderClockwise.height);
        writeBMP(outputClockwise, bmpHeader, newDibHeaderClockwise, rotatedClockwise);

        auto rotatedCounterClockwise = flip90CounterClockwise(dibHeader, pixelData);
        DIBHeader newDibHeaderCounterClockwise = dibHeader;
        std::swap(newDibHeaderCounterClockwise.width, newDibHeaderCounterClockwise.height);
        writeBMP(outputCounterClockwise, bmpHeader, newDibHeaderCounterClockwise, rotatedCounterClockwise);

        auto filteredClockwiseImage = applyGaussianFilter(newDibHeaderClockwise, rotatedClockwise);
        writeBMP(filteredClockwise, bmpHeader, newDibHeaderClockwise, filteredClockwiseImage);

        auto filteredCounterClockwiseImage = applyGaussianFilter(newDibHeaderCounterClockwise, rotatedCounterClockwise);
        writeBMP(filteredCounterClockwise, bmpHeader, newDibHeaderCounterClockwise, filteredCounterClockwiseImage);

        std::cout << "Processing complete! Images saved.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}