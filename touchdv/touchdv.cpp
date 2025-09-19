#include <iostream>
#include <chrono>
#include <thread>
#include "VirtualTouchDevice.h"

using namespace std::chrono;

// Demo main function for Virtual Touch Device
int main(){
    Config cfg;
    cfg.screenWidth=2560; cfg.screenHeight=1440;
    cfg.inputRateHz=30.0; cfg.outputRateHz=120.0;
    cfg.maxExtrapolationMs=50.0; // Stop extrapolation after 50ms to prevent drift
    cfg.touchTimeoutMs=200.0; // Auto-release touch after 200ms of no input
    cfg.deviceName="Virtual IR Touch Demo";

    VirtualTouchDevice vdev(cfg);
    // Example: use OneEuro smoother
    SmoothingConfig smoothConfig;
    smoothConfig.oneEuroFreq = 120.0;
    smoothConfig.oneEuroMinCutoff = 1.0;
    smoothConfig.oneEuroBeta = 0.007;
    smoothConfig.oneEuroDCutoff = 1.0;
    vdev.setSmoothingType(SmoothingType::OneEuro, smoothConfig);

    if(!vdev.start()){std::cerr<<"Failed to start device\n";return 1;}
    std::cout<<"Virtual device started. Simulating input...\n";

    auto start=steady_clock::now();
    double simDuration=20.0;
    double inputPeriod=1.0/cfg.inputRateHz;
    while(toSeconds(steady_clock::now()-start)<simDuration){
        double progress=toSeconds(steady_clock::now()-start)/simDuration;
        TouchPoint p;
        p.ts=steady_clock::now();
        p.x=float(progress*(cfg.screenWidth-1));
        p.y=float(progress*(cfg.screenHeight-1));
        p.touching=true;
        // pressure uses default value of 150 (perfect for IR touch)
        vdev.pushInputPoint(p);
        std::this_thread::sleep_for(duration<double>(inputPeriod));
    }
    TouchPoint release;
    release.ts=steady_clock::now();
    release.pressure=0; // Explicitly set to 0 for release
    // x, y, touching use defaults (0, 0, false)
    vdev.pushInputPoint(release);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    vdev.stop();
    std::cout<<"Stopped.\n";
    return 0;
}
