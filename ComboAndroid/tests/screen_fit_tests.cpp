#include <ship/window/gui/ScreenFit.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main() {
    using Ship::ScreenFit::Fit;
    // Phone landscape/portrait, foldable, tablet, and a game narrowed by a docked tracker.
    for (const auto size : {std::pair{2400.0f, 1080.0f}, {1080.0f, 2400.0f}, {1840.0f, 2208.0f},
                            {2560.0f, 1600.0f}, {600.0f, 1080.0f}, {320.0f, 240.0f}}) {
        for (int mode=1; mode<=3; ++mode) {
            const auto r=Fit(size.first,size.second,mode);
            assert(r.x>=0 && r.y>=0 && r.width>0 && r.height>0);
            assert(r.x+r.width<=size.first+.01f && r.y+r.height<=size.second+.01f);
            assert(std::abs(2*r.x+r.width-size.first)<.01f && std::abs(2*r.y+r.height-size.second)<.01f);
            assert(r.width/r.height>=4.0f/3.0f-.001f);
            if(mode==1) assert(r.width/r.height<=16.0f/9.0f+.001f);
            if(mode==3) assert(std::abs(r.width/r.height-4.0f/3.0f)<.001f);
        }
    }
    assert(Fit(0,240,1).width==0);
    assert(Fit(320,-1,1).height==0);
    assert(Fit(std::numeric_limits<float>::quiet_NaN(),240,1).width==0);
    assert(Fit(320,std::numeric_limits<float>::infinity(),1).height==0);
    std::cout<<"Screen fit: aspect preservation, centered bounded view, portrait, folding and tracker docking passed\n";
}
