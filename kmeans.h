
#pragma once

#include "numxd.h"

class KMeans {
    public:
    enum {
        enumInitializeRandom,
    };
    MemoryManager mm;
    const int Clusters;
    Num2D<double> InitCentroids;
    Num2D<double> Centroids;
    
    KMeans(const int clusters):
        Clusters(clusters),
        InitCentroids(mm),
        Centroids(mm)
    {
        Num2D<double> n2d(this->mm);
        this->InitCentroids = n2d.Create(1, 1);
        this->Centroids = n2d.Create(1, 1);
    }
    
    void InitializeRandom(Num2D<double> x) {
        SpotNum1D<int> n1d;
        SpotNum2D<double> n2d;
        auto indexes = n1d.Arange(0, x.Row);
        auto shuffled = n1d.Shuffle(indexes);
        auto selected = n1d.Slice(shuffled, 0, this->Clusters);
        auto initCentroids = n2d.Indexing(x, selected);
        this->InitCentroids.Release();
        this->InitCentroids = n2d.Clone(this->mm, initCentroids);
    }
    void Initialize(Num2D<double> x, const int init) {
        if(init == this->enumInitializeRandom) {
            this->InitializeRandom(x);
        } else {
            throw Format("error in %s: %d, unknown initialize parameter", __FUNCTION__, __LINE__);
        }
    }
    Num1D<int> EStep(Num2D<double> means, Num2D<double> x) {
        SpotNum1D<double> n1d;
        SpotNum2D<double> n2d;
        auto distances = n2d.Create(x.Row, this->Clusters);
        for(int i = 0; i < x.Row; i += 1) {
            for(int cluster = 0; cluster < this->Clusters; cluster += 1) {
                distances[i][cluster] = n1d.CalcDistance(x.Ref(i), means.Ref(cluster));
            }
        }
        
        Num1D<int> myN1d(this->mm);
        auto predict = myN1d.Create(x.Row);
        for(int i = 0; i < x.Row; i += 1) {
            predict[i] = n1d.ArgMin(distances.Ref(i));
        }
        return predict;
    }
    
    Num2D<double> MStep(Num1D<int> predict, Num2D<double> x) {
        Num2D<double> myN2d(this->mm);
        auto means = myN2d.Create(this->Clusters, x.Col);
        for(int cluster = 0; cluster < this->Clusters; cluster += 1) {
            SpotNum1D<int> n1d;
            SpotNum2D<double> n2d;
            auto idxs = n1d.WhereEq(predict, cluster);
            auto ix = n2d.Indexing(x, idxs);
            auto mean = ix.Mean();
            means.Ref(cluster).Copy(mean);
        }
        return means;
    }
    
    double CalcMeansDistance(Num2D<double> a, Num2D<double> b) {
        SpotNum2D<double> n2d;
        auto ia = n2d.Clone(a);
        auto ib = n2d.Clone(b);
        auto subtract = ia - ib;
        auto power = subtract.Power(2);
        auto total = power.TotalX();
        return sqrt(total / (double)a.Row);
    }
    
    void Training(Num2D<double> x, int maxIter=100, double threshold=1e-5) {
        Num2D<double> myN2d(this->mm);
        this->Centroids.Release();
        this->Centroids = myN2d.Create(this->Clusters, x.Col);
        SpotNum2D<double> n2d;
        auto means = n2d.Clone(this->InitCentroids);
        for(int i = 0; i < maxIter; i += 1) {
            auto predict = this->EStep(means, x);
            auto newMeans = this->MStep(predict, x);
            
            predict.Release();
            
            double distance = this->CalcMeansDistance(means, newMeans);
            means.Copy(newMeans);
            newMeans.Release();
            if(distance < threshold) {
                break;
            }
        }
        this->Centroids.Copy(means);
    }
    
    Num2D<double> GetInitCentroids(MemoryManager& mm) {
        Num2D<double> n2d(mm);
        return n2d.Clone(this->InitCentroids);
    }
    
    Num2D<double> GetCentroids(MemoryManager& mm) {
        Num2D<double> n2d(mm);
        return n2d.Clone(this->Centroids);
    }
    
    Num1D<int> GetPredict(MemoryManager& mm, Num2D<double> x) {
        auto predict = this->EStep(this->Centroids, x);
        Num1D<int> n1d(mm);
        return n1d.Clone(predict);
    }
};
