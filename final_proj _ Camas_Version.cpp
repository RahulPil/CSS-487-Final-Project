#define BIT_ISSET(var, pos) (!!((var) & (1ULL<<(pos))))

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <bitset>
#include <cmath>
#include <stdio.h>

using namespace cv;
using namespace std;

int bpc;

const Vec3b color[8] = {

   Vec3b(0, 0, 0),       // black   - 000
   Vec3b(255, 255, 255), // white   - 001
   Vec3b(0, 0, 255),     // red     - 010
   Vec3b(0, 255, 0),     // green   - 011
   Vec3b(0, 255, 255),   // yellow  - 100
   Vec3b(255, 0, 0),     // blue    - 101   
   Vec3b(255, 0, 255),   // magenta - 110
   Vec3b(255, 255, 0)    // cyan    - 111

};

const enum {
    black = 0,
    white,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan
};

void writeCell(Mat& db, const int cellN, const int cellIndex, const int val) {
    int cellWidth = db.cols / cellN;
    int r = (cellIndex / cellN) * cellWidth;
    int c = (cellIndex % cellN) * cellWidth;
    for (int row = 0; row < cellWidth; row++) {
        for (int col = 0; col < cellWidth; col++) {
            db.at<Vec3b>(r + row, c + col) = color[val];
        }
    }
}

Mat makeDataBox(const char* data, const int dataArrSize, const int cellN, const int dbWidth, const int bitsPerCell = 1 /* can be 1,2,3 */) {
    const int dims[] = { dbWidth, dbWidth };
    const int cellWidth = dbWidth / cellN;
    Mat db(2, dims, CV_8UC3, Scalar::all(0));
    if (bitsPerCell > 3 || bitsPerCell < 1) {
        std::cout << "ERROR: can only do 1,2,3 bits per cell. Input: " << bitsPerCell << endl;
        return db;
    }
    int val = 0;
    int bitIndex = 0;
    while (bitIndex < (dataArrSize * 8)) {
        if ((bitIndex) / bitsPerCell >= (cellN * cellN)) {
            std::cout << "ERROR: to much data for this box" << endl;
            std::cout << "ERROR: missed " << (dataArrSize * 8) - bitIndex << " bits or " << ((dataArrSize * 8) - bitIndex) / 8 << " bytes" << endl;
            break;
        }
        if (BIT_ISSET(data[bitIndex / 8], bitIndex % 8)) {
            std::cout << "1";
            val += pow(2, bitIndex % bitsPerCell);
        }
        else cout << "0";
        if ((bitIndex + 1) % bitsPerCell == 0) {
            writeCell(db, cellN, (((bitIndex + 1) / bitsPerCell) - 1), val);
            val = 0;
        }
        bitIndex++;
    }
    std::cout << endl;
    if (val != 0) writeCell(db, cellN, (((bitIndex + 1) / bitsPerCell) - 1), val);
    return db;
}

string toAscii(string binaryValues) {
    stringstream sstream(binaryValues);
    string text;
    while (sstream.good()) {
        bitset<8> bits;
        sstream >> bits;
        char c = char(bits.to_ulong());
        text += c;
    }
    return text;
}

char findColorValue(Mat& patch) {
    int average[] = { 0, 0, 0 };
    int size = patch.rows * patch.cols;

    for (int r = 0; r < patch.rows; r++) {
        for (int c = 0; c < patch.cols; c++) {
            for (int a = 0; a < 3; a++) {
                average[a] += patch.at<Vec3b>(r, c)[a];
            }
        }
    }

    for (int j = 0; j < 3; j++) {
        average[j] = average[j] / size;
    }

    int closest = 255;
    char val;
    for (int i = 0; i < 8; i++) {
        int temp = 0;
        for (int h = 0; h < 3; h++) {
            temp += abs(color[i][h] - average[h]);
        }
        if (temp < closest) {
            closest = temp;
            val = i;
        }
    }
    return val;
}

void readDataBox(const Mat& db, const int bitsPerCell, const int resolution, const int dataArrSize) {

    if (bitsPerCell > 3 || bitsPerCell < 1) {
        std::cout << "ERROR: can only do 1,2,3 bits per cell. Input: " << bitsPerCell << endl;
        return;
    }
    const int bitCount = bitsPerCell * resolution * resolution;
    int cellWidth = db.cols / resolution;
    int cellHeight = db.rows / resolution;
    string binaryString = "";
    int bitPosition = 0;
    char curLet = NULL;
    int count = 0;
    for (int xCellPos = cellWidth / 2; xCellPos < db.cols; xCellPos += cellWidth) {
        for (int yCellPos = cellHeight / 2; yCellPos < db.rows; yCellPos += cellHeight) {

            if (count < dataArrSize) {
                int r = yCellPos - (cellHeight / 3);
                int c = xCellPos - (cellWidth / 3);
                int w = (cellWidth / 3) * 2;
                int h = (cellHeight / 3) * 2;

                Mat patch = db(Rect(r, c, w, h));
                char colorPos = findColorValue(patch);

                for (int l = 0; l < bitsPerCell; l++) {
                    if (bitPosition < 8) {
                        if (BIT_ISSET(colorPos, l)) {
                            curLet += pow(2, bitPosition);
                            std::cout << 1;
                        }
                        else {
                            std::cout << 0;
                        }
                        bitPosition++;
                    }
                    else {
                        count++;
                        bitPosition = 0;
                        binaryString += curLet;
                        curLet = NULL;
                        if (BIT_ISSET(colorPos, l)) {
                            curLet += pow(2, bitPosition);
                            std::cout << 1;
                        }
                        else {
                            std::cout << 0;
                        }
                        bitPosition++;
                    }
                }
            }
        }
    }

    std::cout << endl << "binary string output: " << binaryString << endl;
    string outputMessage = toAscii(binaryString);
    std::cout << "output: " << outputMessage << endl;
    ofstream message("OutputMessage.txt");
    message << outputMessage;
    message.close();
}

int main(int argc, char* argv[])
{
    //string s = "da";
    //string s = "test data: our names are griffin, camas, and rahul. we are the coolest";
    //string s = "test data: our names are griffin, camas, and rahul. we are the coolest. we are in the css487 class with professor olsen. rahul's aaaaaaaa";
    string s = "Computer vision is the study of methods to extract content from digital images/video for applications such as recognizing people, navigating in an unfamiliar environment, image-based rendering, and retrieving images from a vast library. This class will examine all stages of computer vision from low-level to high-level. Low-level tasks include noise reduction and edge detection. In the middle, are estimating 3D properties of the scene, determining the camera motion from an image sequence, and segmentation of an image into coherent subsets. At the highest level, object recognition and scene interpretation are performed.";
    std::cout << "length = " << s.length() << endl;
    const char* sd = s.data();
    char* dataOut = (char*)malloc(sizeof(char) * (s.length() + 1));
    Mat overlay = makeDataBox(sd, s.length(), 32, 1024, 1);
    readDataBox(overlay, 1, 32, s.length());
    free(dataOut);
    namedWindow("overlay Image");
    resizeWindow("overlay Image", overlay.cols / 8, overlay.rows / 8);
    imshow("overlay Image", overlay);
    imwrite("overlay.jpg", overlay);

    waitKey(0);

    return 0;
}