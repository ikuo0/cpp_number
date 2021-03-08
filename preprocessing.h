
#pragma once

#include <numxd.h>

class StandardScaler {
    public:
    MemoryManager mm;
    Num1D<double> Mean;
    Num1D<double> StdDev;
    Num2D<double> Data;
    StandardScaler(Num2D<double> data): Mean(mm), StdDev(mm), Data(data) {
        Num1D<double> n1d(this->mm);
        this->Mean = n1d.Create(this->Data.Col);
        this->StdDev = n1d.Create(this->Data.Col);
    }
    
    void Fit() {
        Num2D<double> n2d(this->mm);
        this->Mean.Release();
        this->StdDev.Release();
        this->Mean = this->Data.Mean();
        this->StdDev = this->Data.StdDev(1);
    }
    
    Num2D<double> Transform(MemoryManager& memoryManager) {
        Num2D<double> n2d(memoryManager);
        auto subtract = this->Data.Subtract(this->Mean);
        auto answer = subtract.Division(this->StdDev);
        auto response = n2d.Clone(answer);
        
        subtract.Release();
        answer.Release();
        
        return response;
    }
};
