
#include "kmeans.h"
#include "numxd.h"
#include "preprocessing.h"
#include "tsv.h"

Num1D<int> Test1Sub(Num1D<int> src, Num1D<int> dst) {
    auto po = src.Power(2);
    auto answer = src + po;
    
    po.Release();
    
    return answer;
}

void Test1() {
    //MemoryManager mm;
    //Num1D<int> n1d(mm);
    
    SpotNum1D<int> n1d;
    
    auto x1 = n1d.Arange(0, 10);
    auto x2 = n1d.Empty();
    auto x3 = Test1Sub(x1, x2);
    //auto x3 = Test1Sub(x1, x2);
    
    Dump1D(x1);
    Dump1D(x2);
    Dump1D(x3);
    
}

void TestTSV() {
    
    //TSV::Buffer buffer;
    auto tsvData = TSV::Read("seeds_dataset.txt");
    for(auto line = tsvData.begin(); line != tsvData.end(); line++) {
        for(auto cell = line->begin(); cell != line->end(); cell++) {
            auto cidx = std::distance(line->begin(), cell);
            std::cout << *cell;
            if(cidx < (long int)(line->size() - 1)) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
    
    MemoryManager mm;
    auto X = TSV::ToDouble(mm, tsvData);
    printf("################\n");
    Dump2D(X);
}

void TestScaler() {
    try {
        MemoryManager mm;
        auto tsvData = TSV::Read("./seeds_dataset.txt");
        auto data = TSV::ToDouble(mm, tsvData);
        
        StandardScaler scaler(data);
        scaler.Fit();
        auto scaled = scaler.Transform(mm);
        Dump2D(scaled);
        TSV::Write<double>("./cp_scaled.txt", scaled);
    } catch(const char* err) {
        std::cout << "catch error" << std::endl;
        std::cout << err << std::endl;
    }
}

void TestKMeans() {
    MemoryManager mm;
    auto tsvData = TSV::Read("./seeds_dataset.txt");
    auto data = TSV::ToDouble(mm, tsvData);
    
    Num1D<int> n1d(mm);
    Num2D<double> n2d(mm);
    
    auto xIndexes = n1d.Arange(0, 7);
    auto xData = n2d.IndexingT(data, xIndexes);
    auto yData = n2d.ValT(data, 7);
    
    StandardScaler scaler(xData);
    scaler.Fit();
    auto scaledX = scaler.Transform(mm);
    
    KMeans km(3);
    km.Initialize(scaledX, KMeans::enumInitializeRandom);
    km.Training(scaledX, 100, 1e-5);
    
    printf("########## centroids\n");
    Dump2D(km.Centroids);
    
    auto initCentroids = km.GetInitCentroids(mm);
    auto centroids = km.GetCentroids(mm);
    auto predict = km.GetPredict(mm, scaledX);
    
    TSV::Write("./cp_init_means.txt", initCentroids);
    TSV::Write("./cp_means.txt", centroids);
    TSV::Write("./cp_predict.txt", predict);
}

int main(int argc, char** argv) {
    //Test1();
    //TestTSV();
    //TestScaler();
    TestKMeans();
    return 0;
}
// /mnt/d/project/000018_cpp_number
// g++ numxd_test.cpp -o a.out -Wall -I./
// g++ -fsanitize=address -fno-omit-frame-pointer -g numxd_test.cpp -o a.out -Wall -I./

// …or create a new repository on the command line
// echo "# cpp_number" >> README.md
// git init
// git add README.md
// git commit -m "first commit"
// git branch -M main
// git remote add origin git@github.com:ikuo0/cpp_number.git
// git push -u origin main
// …or push an existing repository from the command line
// git remote add origin git@github.com:ikuo0/cpp_number.git
// git branch -M main
// git push -u origin main

