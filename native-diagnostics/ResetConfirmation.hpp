#pragma once
#include "IgnitionInteraction.hpp"
struct ResetDialogHit {int button=-1;float distance=2;};
inline ResetDialogHit HitResetDialog(DriverIK::V center,DriverIK::Q rotation,DriverIK::V origin,DriverIK::V direction){using namespace DriverIK;auto inverse=Inverse(rotation);auto p=Rotate(inverse,Sub(origin,center)),d=Rotate(inverse,direction);if(!std::isfinite(d[2])||d[2]>=-1e-5f)return {};float t=-p[2]/d[2];if(!std::isfinite(t)||t<0||t>2)return {};auto hit=Add(p,Mul(d,t));if(std::abs(hit[0])>.35f||std::abs(hit[1])>.175f)return {};int button=hit[1]>=-.135f&&hit[1]<=-.025f?(hit[0]>=-.30f&&hit[0]<=-.03f?0:hit[0]>=.03f&&hit[0]<=.30f?1:-1):-1;return {button,t};}
struct ResetConfirmation {
 int action=-1;uint64_t deadline=0;IgnitionTriggerGate triggers[2];
 void Cancel(){action=-1;deadline=0;triggers[0]=triggers[1]={};}
 void Open(unsigned request,uint64_t now){Cancel();if(request<3){action=request;deadline=now+30000;}}
 int Step(bool valid,const float (&values)[2],const bool (&tracked)[2],const int (&buttons)[2],uint64_t now){if(action<0)return -1;if(!valid||now>=deadline){Cancel();return -1;}for(unsigned side=0;side<2;++side)if(triggers[side].Step(values[side],tracked[side],buttons[side]>=0)){int selected=action;bool confirm=buttons[side]==0;Cancel();return confirm?selected:-1;}return -1;}
};
